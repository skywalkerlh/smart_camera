/*
  * video_context.c
 *
 *  Created on: 2015年5月22日
 *      Author: work
 */

//#include <stddef.h>
#include <linux/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "video_context.h"
#include "debug.h"
#include "video.h"
#include "dsp.h"
#include "message.h"
#include "msg_factory.h"
#include "sys_conf.h"
#include "sample_factory.h"
#include "shared_vid_buf.h"
#include "v4l2.h"
#include "net_protocol.h"
#include "mailbox.h"
#include "thread.h"
#include "fpga.h"
#include "key_file.h"
#include "local_protocol.h"
#include "sample_count.h"
#include "ui_protocol.h"

struct image_buffer g_net_send_img_buf[2];
struct image_buffer g_net_recv_img_buf[2];

sem_t gsem_algo_sim;

__u32 g_process_overclock_amount = 0;

#define SCREEN_IMAGE_WIDTH 600	//认为在触摸屏上显示的图像宽度不大于600

static struct Video *g_video1, *g_video2;
__u32 video_num_enabled = 0;
static __u32 vid1_valid = 0;
static __u32 vid2_valid = 0;

static __s32 g_mbx_dsp = 0;
static __s32 g_mbx_show = 0;
__s32 g_mbx_send = 0;
static __u32 g_img_not_process = 0;

static __u8 *g_shared_img1_buf = NULL;
static __u8 *g_shared_img2_buf = NULL;

static pthread_mutex_t g_mutex;

extern __u32 sample_count_sum;
__u32 sample_count_shoot = 0;

#define OVERCLOCK_ARRAY_MAX 32
__u32 overclock_img_array_index = 0;
struct image_buffer *g_overclock_img_buf[OVERCLOCK_ARRAY_MAX];


struct image_buffer* get_overclock_img_buf()
{
	overclock_img_array_index++;
	if (overclock_img_array_index >= OVERCLOCK_ARRAY_MAX)
		overclock_img_array_index = 0;
	return (g_overclock_img_buf[overclock_img_array_index]);
}


void sample_count_reset()
{
	sample_count_shoot = 0;
	sample_count_sum = 0;
}

void video_context_release(void *resource)
{
	struct SampleImageGroup *pSampleImageGroup;
	__u32 image_num = 0;
	__s32 i = 0;

	pSampleImageGroup = (struct SampleImageGroup *) resource;
	image_num = pSampleImageGroup->image_num;

	if (image_num == 1)
	{
		if (vid1_valid)
		{
			pthread_mutex_lock(&(pSampleImageGroup->image[0]->mutex));
			pSampleImageGroup->image[0]->r_count--;
			if(pSampleImageGroup->image[0]->r_count == 0)
			{
				if(pSampleImageGroup->image_source == 0)
				{
					video_release_img_buf(g_video1, pSampleImageGroup->image[0]);
				}
				sample_factory_recycle(pSampleImageGroup);
			}
			pthread_mutex_unlock(&(pSampleImageGroup->image[0]->mutex));
		}
		else if (vid2_valid)
		{
			pthread_mutex_lock(&(pSampleImageGroup->image[0]->mutex));
			pSampleImageGroup->image[0]->r_count--;
			if(pSampleImageGroup->image[0]->r_count == 0)
			{
				if(pSampleImageGroup->image_source == 0)
				{
					video_release_img_buf(g_video2, pSampleImageGroup->image[0]);
				}
				sample_factory_recycle(pSampleImageGroup);
			}

			pthread_mutex_unlock(&(pSampleImageGroup->image[0]->mutex));
		}
		else
		{
			MSG("video_num_enabled = %d", video_num_enabled);
			ERROR("video_num_enabled");
		}
	}
	else if(image_num == 2)
	{
		pthread_mutex_lock(&(pSampleImageGroup->image[0]->mutex));
		pSampleImageGroup->image[0]->r_count--;
		if(pSampleImageGroup->image[0]->r_count == 0)
		{
			if(pSampleImageGroup->image_source == 0)
			{
				video_release_img_buf(g_video1, pSampleImageGroup->image[0]);
			}
		}
		pthread_mutex_unlock(&(pSampleImageGroup->image[0]->mutex));

		pthread_mutex_lock(&(pSampleImageGroup->image[1]->mutex));
		pSampleImageGroup->image[1]->r_count--;
		if(pSampleImageGroup->image[1]->r_count == 0)
		{
			if(pSampleImageGroup->image_source == 0)
			{
				video_release_img_buf(g_video2, pSampleImageGroup->image[1]);
			}
		}
		pthread_mutex_unlock(&(pSampleImageGroup->image[1]->mutex));

		if((pSampleImageGroup->image[0]->r_count == 0)&&(pSampleImageGroup->image[1]->r_count == 0))
		{
			sample_factory_recycle(pSampleImageGroup);
		}
	}
}


void image_sim_tsk(void *args)
{
	struct SampleImageGroup  *pSampleImageGroup;
	extern struct AlgoParamsConfig g_algo_params_config;
	__u32 i;
	struct message *msg;
	struct ImageSendMode *pImageSendMode = sc_get_img_send_mode(NULL);


	while (1)
	{
		sem_wait(&gsem_algo_sim);
		pSampleImageGroup = sample_factory_produce();
		if(pSampleImageGroup == NULL)
		{
			usleep(1000);
			continue;
		}

		for(i=0; i<video_num_enabled; i++)
		{
			pSampleImageGroup->image[i] = &g_net_recv_img_buf[i];
			pSampleImageGroup->image[i]->frame->data	= g_net_recv_img_buf[i].frame->data;
		}

		/* 单通道 */
		if (1 == video_num_enabled)
		{
			pSampleImageGroup->image_num = 1;
			//默认只有dsp,net需要图像
			pthread_mutex_lock(&(pSampleImageGroup->image[0]->mutex));
			pSampleImageGroup->image[0]->r_count = 2;
			pthread_mutex_unlock(&(pSampleImageGroup->image[0]->mutex));
		}
		/* 双通道 */
		else if (2 == video_num_enabled)
		{
			pSampleImageGroup->image_num = 2;
			//默认只有dsp,net需要图像
			pthread_mutex_lock(&(pSampleImageGroup->image[0]->mutex));
			pSampleImageGroup->image[0]->r_count = 2;
			pthread_mutex_unlock(&(pSampleImageGroup->image[0]->mutex));

			pthread_mutex_lock(&(pSampleImageGroup->image[1]->mutex));
			pSampleImageGroup->image[1]->r_count = 2;
			pthread_mutex_unlock(&(pSampleImageGroup->image[1]->mutex));
		}

		pSampleImageGroup->image_source = 1;
		msg = msg_factory_produce(g_mbx_dsp, 0);
		msg->ops->set_data(msg, pSampleImageGroup, 0, NULL, 0);
		mailbox_post(msg);

		/* 向网络发送图像 */
		if(pImageSendMode->mode == 1)
		{
			//增加图像拷贝代码，拷贝完即时释放内存，以防100M网络不能即时发送所有图像造成底层缓存耗尽
			msg = msg_factory_produce(g_mbx_send, 0);
			msg->ops->set_data(msg, pSampleImageGroup, 0, NULL, 0);
			mailbox_post(msg);
		}
		else if(pImageSendMode->mode == 0)
		{
			video_context_release(pSampleImageGroup);
		}
	}
}

static struct SysDelayConfig *g_sys_delay_p;

void image_capture_tsk()
{
	__s32 idx = 0;//test
	__u8 *p = NULL;//test
	__s32 test_rlt[8] = {0,1,1,0,1,1,1,0};//test

	__s32 ret = 0;
	__u32 i = 0;
	struct timeval tv;
	extern struct AlgoParamsConfig g_algo_params_config;
	struct SampleImageGroup  *pSampleImageGroup;
	struct SampleInfo *pSampleInfo = NULL;
	struct SampleResult *pSampleResult = NULL;
//	struct SysWorkingMode *pSysWorkingMode = sc_get_sys_working_mode(NULL);
	struct SampleJudgeMode *pSampleJudgeMode = sc_get_sample_judge_mode(NULL);
	struct ImageSendMode *pImageSendMode = sc_get_img_send_mode(NULL);
	struct message *msg;
	__u16 reg_val;

	g_sys_delay_p = sc_get_sys_delay_config(NULL);

	while (1)
	{
		pSampleImageGroup = sample_factory_produce();
		if(pSampleImageGroup == NULL)
		{
			usleep(1000);
			continue;
		}
		/* 单通道 */
		if (1 == video_num_enabled)
		{
			if(vid1_valid)
			{
				pSampleImageGroup->image[0] = video_request_img_buf(g_video1);
			}
			else if(vid2_valid)
			{
				pSampleImageGroup->image[0] = video_request_img_buf(g_video2);
			}
			else
			{
				MSG("video_num_enabled = %d", video_num_enabled);
				ERROR("video_num_enabled");
			}

			p = pSampleImageGroup->image[0]->frame->data;//
			p[0] = test_rlt[idx];//
			idx++;//
			if(idx == 8)//
				idx = 0;//

			pSampleImageGroup->image_num = 1;
			//默认ui,dsp,net全部需要图像
			pthread_mutex_lock(&(pSampleImageGroup->image[0]->mutex));
			pSampleImageGroup->image[0]->r_count = 3;
			pthread_mutex_unlock(&(pSampleImageGroup->image[0]->mutex));
//			printf("get a image\n");
		}
		/* 双通道 */
		else if (2 == video_num_enabled)
		{

			pSampleImageGroup->image[0] = video_request_img_buf(g_video1);

			if(check_sys_reset_flag()&&(g_sys_delay_p->C1_C2_delay))//复位期间
			{
				video_release_img_buf(g_video1, pSampleImageGroup->image[0]);
				continue;
			}

			pSampleImageGroup->image[1] = video_request_img_buf(g_video2);

			p = pSampleImageGroup->image[0]->frame->data;//
			p[0] = test_rlt[idx];//
			idx++;//
			if(idx == 8)//
				idx = 0;//

			pSampleImageGroup->image_num = 2;
			//默认ui,dsp,net全部需要图像
			pthread_mutex_lock(&(pSampleImageGroup->image[0]->mutex));
			pSampleImageGroup->image[0]->r_count = 3;
			pthread_mutex_unlock(&(pSampleImageGroup->image[0]->mutex));

			pthread_mutex_lock(&(pSampleImageGroup->image[1]->mutex));
			pSampleImageGroup->image[1]->r_count = 3;
			pthread_mutex_unlock(&(pSampleImageGroup->image[1]->mutex));

		}
		else
		{
			MSG("video_num_enabled = %d", video_num_enabled);
			ERROR("video_num_enabled");
		}

		//考虑超频样品编入计数
		if(sample_count_shoot == sample_count_sum)
		{
			sample_count_shoot++;
			pSampleImageGroup->sample_info.count = sample_count_shoot;
			sample_count_sum = FPGA_READ32(fpga_base, FPGA_FACTTRIGNUML_REG);

			if(sample_count_sum == 0)//复位期间来的图一律作废
			{
				sample_count_shoot = 0;
				for(__u32 i=0; i<3; i++)
				{
					video_context_release(pSampleImageGroup);
				}
				continue;
			}
		}
		else if(sample_count_shoot < sample_count_sum)
		{
			if(find_current_count(sample_count_shoot+1) != 0)//1\3\2\4 3超频
			{
				sample_count_shoot = sample_count_shoot+1;
				pSampleImageGroup->sample_info.count = sample_count_shoot;
				sample_count_shoot = sample_count_sum;

			}
			else //1\2\3\4 3超频
			{
				sample_count_shoot = sample_count_sum+1;
				pSampleImageGroup->sample_info.count = sample_count_shoot;
				sample_count_sum = sample_count_shoot;
			}
		}
		else
		{
			printf("sample_count_error !\n");
			printf("sample_count_shoot = %d\n",sample_count_shoot);
			printf("sample_count_sum = %d\n",sample_count_sum);
		}

		if (1 == video_num_enabled)
		{
			pSampleImageGroup->shoot_wheel_code = fpga_get_vp5_shoot_code();
			pSampleImageGroup->sample_info.wheel_code = pSampleImageGroup->shoot_wheel_code;
			pSampleImageGroup->image[0]->code = pSampleImageGroup->sample_info.wheel_code;
			pSampleImageGroup->image[0]->count = pSampleImageGroup->sample_info.count;
		}
		else if(2 == video_num_enabled)
		{
			pSampleImageGroup->shoot_wheel_code = fpga_get_vp6_shoot_code();
			pSampleImageGroup->sample_info.wheel_code = pSampleImageGroup->shoot_wheel_code;
			pSampleImageGroup->image[0]->code = pSampleImageGroup->sample_info.wheel_code;
			pSampleImageGroup->image[1]->code = pSampleImageGroup->sample_info.wheel_code;
			pSampleImageGroup->image[0]->count = pSampleImageGroup->sample_info.count;
			pSampleImageGroup->image[1]->count = pSampleImageGroup->sample_info.count;
		}
		pSampleInfo = &(pSampleImageGroup->sample_info);

		pSampleImageGroup->image_source = 0;

		//向ui发送图像
		msg = msg_factory_produce(g_mbx_show, 0);
		msg->ops->set_data(msg, pSampleImageGroup, 0, NULL, 0);
		mailbox_post(msg);

		/* 向网络发送图像 */
		if(pImageSendMode->mode == 1)
		{
			//增加图像拷贝代码，拷贝完即时释放内存，以防100M网络不能即时发送所有图像造成底层缓存耗尽
			msg = msg_factory_produce(g_mbx_send, 0);
			msg->ops->set_data(msg, pSampleImageGroup, 0, NULL, 0);
			mailbox_post(msg);
		}
		else if(pImageSendMode->mode == 0)
		{
			video_context_release(pSampleImageGroup);
		}
		else if((pImageSendMode->mode == 2)&&(pSampleJudgeMode->mode != 0))
		{
			video_context_release(pSampleImageGroup);
		}

		/* 算法判决模式 */
		if (pSampleJudgeMode->mode == 0)
		{
			/*
			 * 如果有3个以上样品未处理或者当前正在进行算法参数更新，则不再申请DSP,
			 * 并将样品标记为算法未处理。
			 */
			pthread_mutex_lock(&g_mutex);
			if ((g_img_not_process <= 2)&&(g_algo_params_config.update == 0))
			{
				msg = msg_factory_produce(g_mbx_dsp, 0);
				msg->ops->set_data(msg, pSampleImageGroup, 0, NULL, 0);
				g_img_not_process++;
				///////////////////////////////////////////////////////
				mailbox_post(msg);
			}
			else
			{
				pSampleInfo->flag = 2;//处理超频
				pSampleInfo->algoCalcTime = 0xFFFFFFFF;//算法处理时间
				g_process_overclock_amount++;
				/* 发送样品信息至踢废管理进程*/
				pSampleResult = get_sample_result_array();
				memcpy(pSampleResult->result, pSampleInfo->decision, 8);
				pSampleResult->count = pSampleInfo->count;
				pSampleResult->wheel_code = pSampleImageGroup->shoot_wheel_code;
				pSampleResult->pos = 1;
				pSampleInfo->kick_pos = 1;
				send_sample_reault(pSampleResult, sizeof(SampleResult));
				/* 发送样品信息至网络 */
				send_sample_info(pSampleImageGroup, video_context_release);
			}
			pthread_mutex_unlock(&g_mutex);

		}
		/* 测试模式 1*/
		else if(pSampleJudgeMode->mode == 1)//全进工位1
		{
			/* 发送样品信息至踢废管理进程 */
			pSampleResult = get_sample_result_array();
			memcpy(pSampleResult->result, pSampleInfo->decision, 8);
			pSampleResult->count = pSampleInfo->count;
			pSampleResult->wheel_code = pSampleImageGroup->shoot_wheel_code;
			pSampleResult->pos =1;
			pSampleInfo->kick_pos = 1;
			send_sample_reault(pSampleResult, sizeof(SampleResult));

			/* 发送样品信息至网络 */
			send_sample_info(pSampleImageGroup, video_context_release);
		}

		/* 测试模式 2*/
		else if(pSampleJudgeMode->mode == 2)//全进工位2
		{
			/* 发送样品信息至踢废管理进程 */
			pSampleResult = get_sample_result_array();
			memcpy(pSampleResult->result, pSampleInfo->decision, 8);
			pSampleResult->count = pSampleInfo->count;
			pSampleResult->wheel_code = pSampleImageGroup->shoot_wheel_code;
			pSampleResult->pos =2;
			pSampleInfo->kick_pos = 2;
			send_sample_reault(pSampleResult, sizeof(SampleResult));

			/* 发送样品信息至网络 */
			send_sample_info(pSampleImageGroup, video_context_release);
		}


		/* 测试模式 3*/
		else if(pSampleJudgeMode->mode == 3)//全过
		{
			/* 发送样品信息至踢废管理进程 */
			pSampleResult = get_sample_result_array();
			memcpy(pSampleResult->result, pSampleInfo->decision, 8);
			pSampleResult->count = pSampleInfo->count;
			pSampleResult->wheel_code = pSampleImageGroup->shoot_wheel_code;
			pSampleResult->pos = 0;
			pSampleInfo->kick_pos = 0;
			send_sample_reault(pSampleResult, sizeof(SampleResult));

			/* 发送样品信息至网络 */
			send_sample_info(pSampleImageGroup, video_context_release);

		}
	}
}


void image_send_tsk()
{
	__u32 i;
	struct SampleImageGroup* pSampleImageGroup;
	struct message msg;

	msg_factory_cast(&msg, g_mbx_send);

	while(1)
	{
		mailbox_pend(&msg);
		pSampleImageGroup = msg.ops->get_data(&msg,NULL);
		msg_factory_recycle(&msg);

		send_image_info(pSampleImageGroup);
	}
}

extern __s32 g_ui_socket_state;
void image_show_tsk()
{
	__u32 img_num = 0;
	__u32 i;
	struct SampleImageGroup  *pSampleImageGroup;
	struct message msg;

	msg_factory_cast(&msg, g_mbx_show);

	while(1)
	{
		mailbox_pend(&msg);
		pSampleImageGroup = msg.ops->get_data(&msg,NULL);
		msg_factory_recycle(&msg);
		img_num = pSampleImageGroup->image_num;

		if(g_ui_socket_state)
		{
			if(img_num == 1)
			{
				memcpy(g_shared_img1_buf,
								pSampleImageGroup->image[0]->frame->data,
								pSampleImageGroup->image[0]->width * pSampleImageGroup->image[0]->height);
			}
			else
			{
				memcpy(g_shared_img1_buf,
								pSampleImageGroup->image[0]->frame->data,
								pSampleImageGroup->image[0]->width * pSampleImageGroup->image[0]->height);
				memcpy(g_shared_img2_buf,
								pSampleImageGroup->image[1]->frame->data,
								pSampleImageGroup->image[1]->width * pSampleImageGroup->image[1]->height);
			}
			send_ui_image_info(pSampleImageGroup, video_context_release);
		}
		else
		{
			video_context_release(pSampleImageGroup);
		}
	}
}

#if 0
void image_show_tsk()
{
	__u32 img_num = 0;
	__u32 raw_img1_width = 0;
	__u32 raw_img1_height = 0;
	__u32 raw_img2_width = 0;
	__u32 raw_img2_height = 0;

	__s32 idx = -1;
	__u32 col = 0;
	__u32 row = 0;

	struct SampleImageGroup  *pSampleImageGroup;
	struct message msg;

	__u32 shared_vid1_buf_scaler = 0;
	__u32 shared_vid2_buf_scaler = 0;
	__u32 n1 = 0;
	__u32 n2 = 0;

	__u8 *img_data = NULL;
	__u8 *src = NULL;
	__u8 *dst = NULL;
	__u8 *vid1_bayer_data = NULL;
	__u8 *vid2_bayer_data = NULL;

	shared_vid1_buf_get_scaler(&shared_vid1_buf_scaler, &n1);
	shared_vid2_buf_get_scaler(&shared_vid2_buf_scaler, &n2);

	__u32 screen_img1_width = shared_vid1_buf_get_width();
	__u32 screen_img2_width = shared_vid2_buf_get_width();
	__u32 screen_img1_height = shared_vid1_buf_get_height();
	__u32 screen_img2_height = shared_vid2_buf_get_height();

	msg_factory_cast(&msg, g_mbx_show);

	while(1)
	{
		mailbox_pend(&msg);
		pSampleImageGroup = msg.ops->get_data(&msg,NULL);
		msg_factory_recycle(&msg);

		img_num = pSampleImageGroup->image_num;


		/* 缺少对能否显示图像的判断 */

		idx++;
		if(idx == 5)
			idx = 0;

		if(img_num == 1)
		{
			screen_img1_width = pSampleImageGroup->image[0]->width;
			screen_img1_height = pSampleImageGroup->image[0]->height;
			img_data = pSampleImageGroup->image[0]->frame->data;

			dst = screen_img1_width * screen_img1_height * idx;

			for(row = 0; row < pSampleImageGroup->image[0]->width; row += shared_vid1_buf_scaler << 1)
				for(col = 0; col < pSampleImageGroup->image[0]->height; col += shared_vid1_buf_scaler << 1)
				{
					src = img_data + col + row * pSampleImageGroup->image[0]->width;
					dst += (col >> n1) + (row >> n1) * screen_img1_width;
					memcpy(dst, src, shared_vid1_buf_scaler);
				}
		}
	}
}
#endif

void image_process_tsk()
{
	struct DSP *dsp;
	struct SampleImageGroup  *pSampleImageGroup;
	__u32 i = 0;

	struct message msg;
	msg_factory_cast(&msg, g_mbx_dsp);

	while(1)
	{
		mailbox_pend(&msg);
		pSampleImageGroup = msg.ops->get_data(&msg,NULL);
		msg_factory_recycle(&msg);

		dsp = dsp_factory_produce();
		dsp->pArgsInfo->sample_image_group = pSampleImageGroup;
		pthread_mutex_lock(&g_mutex);
		g_img_not_process--;
		pthread_mutex_unlock(&g_mutex);

		sem_post(&dsp->sem_img_ready);
	}
}



//将视频通道1的缓冲数加大
void video_context_init()
{
	__u32 width, height, bayer_fmt;

	/*  视频通道1 */
	width				= key_file_get_int("video_channel1", "width",	752);
	height				= key_file_get_int("video_channel1", "height",	480);
	vid1_valid	= key_file_get_int("video_channel1", "valid",	0);
	bayer_fmt		= key_file_get_int("video_channel1", "bayer_fmt",	0);

	if(vid1_valid)
	{
		g_video1 = create_video_channel("/dev/video0", 0, width, height, 16, bayer_fmt,VID1_BUF_MAX);
		g_shared_img1_buf = shared_vid1_buf_create(width, height, bayer_fmt, 5, SCREEN_IMAGE_WIDTH);
		video_num_enabled++;
		video_start_capture(g_video1);
	}

	/*  视频通道2 */
	width				= key_file_get_int("video_channel2", "width",	752);
	height				= key_file_get_int("video_channel2", "height",	480);
	vid2_valid	= key_file_get_int("video_channel2", "valid",	0);
	bayer_fmt		= key_file_get_int("video_channel2", "bayer_fmt",	0);

	if(vid2_valid)
	{
		g_video2 = create_video_channel("/dev/video1", 1, width, height, 16, bayer_fmt,VID2_BUF_MAX);
		g_shared_img2_buf = shared_vid2_buf_create(width, height, bayer_fmt, 5, SCREEN_IMAGE_WIDTH);
		video_num_enabled++;
		video_start_capture(g_video2);
	}

	//创建网络发送用缓冲
	for(int i = 0; i < video_num_enabled; i++)
	{
		g_net_send_img_buf[i].frame = malloc(sizeof(struct shared_memory));
		g_net_send_img_buf[i].frame->data = malloc(6*1024*1024);
	}

	//创建网络图像数据接受缓冲
	for(int i = 0; i < video_num_enabled; i++)
	{
		g_net_recv_img_buf[i].frame = malloc(sizeof(struct shared_memory));
		g_net_recv_img_buf[i].frame->data = malloc(6*1024*1024);
		pthread_mutex_init(&(g_net_recv_img_buf[i].mutex), NULL);
	}

	//创建超频网络图像信息
	for(int i = 0; i < OVERCLOCK_ARRAY_MAX; i++)
	{
		g_overclock_img_buf[i] = malloc(sizeof(struct image_buffer));
		pthread_mutex_init(&(g_overclock_img_buf[i]->mutex), NULL);
	}

	sem_init(&gsem_algo_sim,0,0);

	create_sample_factory(32);
	sample_count_init();
	g_mbx_dsp = mailbox_create("/dsp");
	g_mbx_show = mailbox_create("/show");
	g_mbx_send = mailbox_create("/send");
	add_new_thread(NULL, (void *)&image_capture_tsk, 38, 0, 32*1024);
	add_new_thread(NULL, (void *)&image_process_tsk, 39, 0, 8*1024);
	add_new_thread(NULL, (void *)&image_show_tsk, 26, 0, 8*1024);
	add_new_thread(NULL, (void *)&image_send_tsk, 25, 0, 8*1024);
	add_new_thread(NULL, (void *)&image_sim_tsk, 27, 0, 16*1024);
}
