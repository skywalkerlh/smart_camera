/*
 * protocol.c
 *
 *  Created on: 2015年7月27日
 *      Author: work
 */

#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>

#include "buf_factory.h"
#include "net_context.h"
#include "sample_factory.h"
#include "msg_factory.h"
#include "sys_conf.h"
#include "message.h"
#include "v4l2.h"
#include "debug.h"
#include "mailbox.h"
#include "net.h"
#include "fpga.h"
#include "mailbox.h"
#include "lvds_context.h"
#include "video_context.h"
#include "local_protocol.h"
#include "sys_log.h"
#include "epcs.h"

extern struct image_buffer g_net_send_img_buf[2];

/***************************嵌入式上传主板信息*************************************/

#define SAMPLE_INFO_ARRAY_MAX 32
static __u32 sample_info_array_index = 0;
struct SampleInfo sample_info_array[SAMPLE_INFO_ARRAY_MAX];
static __u32 sample_send_process = 0;

__s32  allow_send_sample(void *__msg__)
{
	struct message *msg = __msg__;
	pthread_mutex_lock(&g_mutex_sample_ability);
	if(sample_send_process)
	{
		sample_send_process--;
		if(sample_send_process < SAMPLE_INFO_ARRAY_MAX)
		{
			g_sample_ability = YES;
		}
	}
	pthread_mutex_unlock(&g_mutex_sample_ability);
	return 0;
}


/* 样品信息 */
void NET_UL(0x45530101)(void *resource, void (*release)(void *resource))
{

	struct message *msg = NULL;
	void *data2send = NULL;
	__s32 ret = 0;
	__u32 conn_state = 0;
	struct SampleImageGroup *pSampleImageGroup;

	static struct NetInfoHead sample_info_head =
	{
			.type = 0x45530101,
			.length = 4 + sizeof(struct SampleInfo)
	};

	pSampleImageGroup = (struct SampleImageGroup *) resource;

	pthread_mutex_lock(&g_mutex_conn_state);
	conn_state = g_conn_state;
	pthread_mutex_unlock(&g_mutex_conn_state);
	if (conn_state == LINK_ON)
	{
		if (g_sample_ability)
		{
			pthread_mutex_lock(&g_mutex_sample_ability);
			sample_send_process++;
			if(sample_send_process >= SAMPLE_INFO_ARRAY_MAX)
				g_sample_ability = NO;
			pthread_mutex_unlock(&g_mutex_sample_ability);

			msg = msg_factory_produce(g_net_mbx_id, 5);
			if(msg == NULL)
			{
				release(resource);
				return;
			}

			memcpy(&sample_info_array[sample_info_array_index], &pSampleImageGroup->sample_info, sizeof(SampleInfo));
			release(resource);
			data2send = &sample_info_array[sample_info_array_index];
			sample_info_array_index++;
			if (sample_info_array_index >= SAMPLE_INFO_ARRAY_MAX)
				sample_info_array_index = 0;

			msg->finish = allow_send_sample;

			msg->release = NULL;
			msg->ops->set_data(msg, &sample_info_head, sizeof(sample_info_head), NULL, 0);
			msg->ops->set_data(msg, data2send, sizeof(SampleInfo), NULL, 0);
			ret = mailbox_timedpost(msg);
			if (ret < 0)
			{
				msg_factory_recycle(msg);
				MSG("%s\n", __FUNCTION__);
				return;
			}
			else
			{
//				printf("send a sample info\n");
				return;
			}
		}
	}
	//释放图像缓冲
	release(resource);
	return;
}



__s32  allow_send_image(void *__msg__)
{
	struct message *msg = __msg__;
	pthread_mutex_lock(&g_mutex_img_ability);
	g_img_ability = YES;
	pthread_mutex_unlock(&g_mutex_img_ability);
	return 0;
}

/* 图像信息 */
__s32 NET_UL(0x45530102)(void *resource)
{
	__u32 len_of_data = 0;
	__u32 i = 0;
	__u32 j=0;
	__u16  tmp1,tmp2;
	__u8 *testdata;
	__s32 ret = 0;
	__u32 conn_state = 0;
	__u32 img_ability = YES;

	struct message *msg =NULL;
	struct SampleImageGroup *pSampleImageGroup;
	static struct NetInfoHead img_info_head =
	{
			.type = 0x45530102,
			.length = 4
	};

	pSampleImageGroup = (struct SampleImageGroup *) resource;

	pthread_mutex_lock(&g_mutex_conn_state);
	conn_state = g_conn_state;
	pthread_mutex_unlock(&g_mutex_conn_state);

	if (conn_state == LINK_ON)
	{

		if (g_img_ability)
		{
			pthread_mutex_lock(&g_mutex_img_ability);
			g_img_ability = 0;
			pthread_mutex_unlock(&g_mutex_img_ability);

			msg = msg_factory_produce(g_net_mbx_id, 3);
			if(msg == NULL)
			{
				video_context_release(resource);
				allow_send_image(msg);
				return -3;
			}
			msg->finish = allow_send_image;
			msg->release = NULL;

			for (i = 0; i < pSampleImageGroup->image_num; i++)
			{
				memcpy(&g_net_send_img_buf[i],pSampleImageGroup->image[i], 92);
				memcpy(g_net_send_img_buf[i].frame->data, pSampleImageGroup->image[i]->frame->data, pSampleImageGroup->image[i]->width * pSampleImageGroup->image[i]->height);

#if 0
				for(j = 0; j< pSampleImageGroup->image[i]->width * pSampleImageGroup->image[i]->height/4; )
				{
					tmp1 = *(testdata+j+1);
					tmp2 = *(testdata+j);
					if((tmp1-tmp2)!=1)
					{
						while(1);
					}
					else
					{
						j=j+2;
					}
				}
#endif

			}
			//释放图像缓冲
			video_context_release(resource);

			msg->ops->set_data(msg, &img_info_head, sizeof(img_info_head), NULL, 0);
			img_info_head.length = 4;

			for (i = 0; i < pSampleImageGroup->image_num; i++)
			{
				msg->ops->set_data(msg, &g_net_send_img_buf[i], 92, NULL, 0);
				msg->ops->set_data(msg, g_net_send_img_buf[i].frame->data, g_net_send_img_buf[i].width * g_net_send_img_buf[i].height, NULL, 0);
				img_info_head.length += 92 + g_net_send_img_buf[i].width * g_net_send_img_buf[i].height;
			}

			ret = mailbox_timedpost(msg);
			if (ret < 0)
			{
				msg_factory_recycle(msg);
				MSG("%s\n", __FUNCTION__);
				return -1;
			}
			else
			{
				return 0;
			}
		}
	}
	//释放图像缓冲
	video_context_release(resource);

	return -1;
}

/*样品长度与 距离信息 */
void NET_UL(0x45530103)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = data;
	__u32 len_of_data = len;
	__s32 ret = 0;

	static struct NetInfoHead length_info_head =
	{
			.type = 0x45530103,
			.length = 12
	};

	if (g_conn_state == LINK_ON)
	{
		msg = msg_factory_produce(g_net_mbx_id, 1);
		if(msg == NULL)
		{
			return;
		}
		msg->ops->set_data(msg, &length_info_head, sizeof(length_info_head), NULL, 0);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 版本信息 */
void NET_UL(0x45530104)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_main_board_version(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 统计信息 */
void NET_UL(0x45530105)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_sys_state_info_config(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		if(msg == NULL)
		{
			return;
		}
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 信号配置 */
void NET_UL(0x45530106)(void *data, __u32 len)
{
	struct message *msg = NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_signal_config(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 延时信息 */
void NET_UL(0x45530107)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_sys_delay_config(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 系统工作模式，无效*/
void NET_UL(0x45530108)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
}

/* 视频通道有效信息 */
void NET_UL(0x45530109)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_video_config(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 应用程序更新进度 */
void NET_UL(0x4553010A)(void *data, __u32 len)
{

}

/* 预留 */
void NET_UL(0x4553010B)(void *data, __u32 len)
{

}

/* 视频通道需求信息 */
void NET_UL(0x4553010C)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__u32 id = *(__u32 *)data;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_video_requirement(id, &len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* FPGA功能配置 */
void NET_UL(0x4553010D)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_fpga_func_config(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 反馈信息 */
///val高16位作为类别				0：主板反馈  1：DSP1 2：DSP2
///val低16位作为反馈信息		0：成功 1：失败 2：超时
void NET_UL(0x4553010E)(__u32 val)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_feedbackinfo(val, &len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		if(msg == NULL)
		{
			return;
		}
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 算法参数 */
void NET_UL(0x4553010F)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_algo_params(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 预留 */
void NET_UL(0x45530110)(void *data, __u32 len)
{

}

/* 预留 */
void NET_UL(0x45530111)(void *data, __u32 len)
{

}

/* 超频阈值 */
void NET_UL(0x45530112)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_oc_th(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 预留 */
void NET_UL(0x45530113)(void *data, __u32 len)
{

}

/* FPGA调试信息 */
void NET_UL(0x45530114)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	static struct NetInfoHead img_info_head =
	{
			.type = 0x45530114,
			.length = 4
	};

	if (g_conn_state == LINK_ON)
	{
		data2send = (void *) fpga_get_debug_info(&len_of_data);
		img_info_head.length = 4 + len_of_data;
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, &img_info_head, sizeof(img_info_head), NULL, 0);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 样品判决模式 */
void NET_UL(0x45530115)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_sample_judge_mode(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 图像发送模式 */
void NET_UL(0x45530116)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_img_send_mode(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

/* 样品配置长度 */
void NET_UL(0x45530117)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret =0;

	if (g_conn_state == LINK_ON)
	{
		data2send = sc_get_sample_len(&len_of_data);
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, data2send, len_of_data, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}
/*************************************************************************/



/***************************PC下传主板信息********************************/
/* 系统复位*/
__u32 NET_DL(0x45521101)(void *data, __u32 len)
{
	sc_system_reset();
	MSG("%s\n",__FUNCTION__);
	log_builder("system recv reset commed.");
	return 0;
}

/* 设置拍照延时*/
__u32 NET_DL(0x45521102)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_shoot_delay(data);
	return 0;
}

/* 设置踢废延时*/
__u32 NET_DL(0x45521103)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_kick_delay(data);
	return 0;
}

/* 设置样品判决模式*/
__u32 NET_DL(0x45521104)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_sample_judge_mode(data);
	return 0;
}


/* 设置图像发送模式*/
__u32 NET_DL(0x45521105)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_img_send_mode(data);
	return 0;
}

/* 设置信号极性*/
__u32 NET_DL(0x45521106)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_signal_polar(data);
	return 0;
}

/* 设置输出信号宽度*/
__u32 NET_DL(0x45521107)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_out_signal_width(data);
	return 0;
}

/* 设置输入信号宽度*/
__u32 NET_DL(0x45521108)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_in_signal_width(data);
	return 0;
}

/* 设置视频通道有效性*/
__u32 NET_DL(0x45521109)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_video_config(data);
	return 0;
}

/* 设置视频通道需求*/
__u32 NET_DL(0x4552110A)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_video_requirement(data);
	return 0;
}

/* 设置剔除功能*/
__u32 NET_DL(0x4552110B)(void *data, __u32 len)
{
	__u16 val;
	MSG("%s\n",__FUNCTION__);
	sc_set_fpga_func_config(data);
	return 0;
}

/* 屏幕角度*/
__u32 NET_DL(0x4552110C)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_sys_screen_angle(data);
	return 0;
}

/* 预留*/
__u32 NET_DL(0x4552110D)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	return 0;
}

/* 预留 */
__u32 NET_DL(0x4552110E)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	return 0;
}

/* 预留 */
__u32 NET_DL(0x4552110F)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	return 0;
}

/* 更新应用程序 */

__u32 NET_DL(0x45521110)(void *data, __u32 len)
{
	static __u32 type;
	static __s32 file_id;
	static __u32 file_len, unit_len, updated_len;
	__s32 err;
	static __s32 file_opened = 0;
	const char *file_name = NULL;
	static 	FILE * fstream;
	__u32 conn_state = 0;
	__s32 ret = 0;
	struct message *msg =NULL;
	__u32 len_of_data = sizeof(struct ProgramUpdateProgress);

	static struct ProgramUpdateProgress progress =
	{
			.head =
			{
					.type = 0x4553010A,
					.length = sizeof(struct ProgramUpdateProgress) - 4
			},
	};

	MSG("%s\n",__FUNCTION__);

	pthread_mutex_lock(&g_mutex_conn_state);
	conn_state = g_conn_state;
	pthread_mutex_unlock(&g_mutex_conn_state);

	memcpy(&type, data, 4);
	memcpy(&unit_len,	data+0x08, 4);

	switch(type)
	{
		case 1:
			file_name = "/home/root/new_camera_dra7.elf";
			break;
		case 2:
			file_name = "/lib/firmware/new_dra7-dsp1-fw.xe66";
			break;
		case 3:
			file_name = "/lib/firmware/new_dra7-dsp2-fw.xe66";
			break;
		case 4:
			file_name = "/home/root/new_reject_process.elf";
			break;
		case 5:
			file_name = "/home/root/new_ui_process.elf";
			break;
		case 6:
			file_name = "/home/root/algo_params.dat";
			break;
		case 8:
			file_name = "/home/root/new_fpga_output_file.jic";
			break;

		default:
			return 1;
	}

	if (!file_opened)
	{
		fstream = fopen(file_name, "wb");
		if (fstream == NULL)
		{
			perror("fopen");
			return 1;
		}
		memset(&(progress.file_type), 0, sizeof(progress)-8);
		file_opened = 1;
	}

	progress.file_type = type;
	memcpy(&progress.file_len,		data+0x04, 4);
	fseek(fstream, progress.updated_len, SEEK_SET);

	err = fwrite((data + 0x0C), unit_len, 1, fstream);
	if(err <= 0)
	{
		perror("fwrite");
		progress.status = 0;
		fclose (fstream);
		file_opened = 0;
		remove(file_name);
	}
	else
	{
		progress.status = 1;
		progress.updated_len += unit_len;
		printf("updated_len = %d , file_len = %d \n", progress.updated_len, progress.file_len);
		if(progress.updated_len == progress.file_len)
		{
			file_id = fileno(fstream);
			fsync(file_id);
			fclose (fstream);
			file_opened = 0;

			if(progress.file_type == 6)//fpga程序
			{
				__s32 fd;
				__u32 len;
				void* fpga_bin;
				if((fd = open(file_name,O_RDWR)) == -1)
				{
					printf("fpga.bin is not exist!\n");
				}
				else
				{

					fpga_bin = mmap(NULL, EPCS64_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
					fpga_burn((__u8 *)fpga_bin);
					printf("fpga updata is done!\n");
				}
			}
		}
	}

	if (conn_state == LINK_ON)
	{
		msg = msg_factory_produce(g_net_mbx_id, 1);
		msg->ops->set_data(msg, &progress, len_of_data, NULL, 0);
//		mailbox_post(msg);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
	return 0;
}

/* 设置算法参数 */
__u32 NET_DL(0x45521111)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_algo_params(data);
	return 0;
}

/* 设置主板序列号 */
__u32 NET_DL(0x45521112)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_sys_sn(data);
	return 0;
}

/* 预留*/
__u32 NET_DL(0x45521113)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	//sc_set_dispose_station(data);
	return 0;
}

/* 预留 */
__u32 NET_DL(0x45521114)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	return 0;
}

/* 保存配置 */
__u32 NET_DL(0x45521115)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_conf_save();
	return 0;
}

/* 仿真图像 */
__u32 NET_DL(0x45521116)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	extern struct image_buffer g_net_recv_img_buf[2];
	extern __u32 video_num_enabled;
	extern sem_t gsem_algo_sim;
	static int img_num = 0;

	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;


	static struct NetInfoHead length_info_head =
	{
			.type = 0x45530102,
			.length = 4 + 12
	};

	memcpy(&(g_net_recv_img_buf[img_num].video_id), data, 4);
	memcpy(&(g_net_recv_img_buf[img_num].width), data + 4, 4);
	memcpy(&(g_net_recv_img_buf[img_num].height), data + 8, 4);
	memcpy(&(g_net_recv_img_buf[img_num].bayer_fmt), data + 12, 4);
	memcpy(&(g_net_recv_img_buf[img_num].code), data + 16, 4);
	memcpy(g_net_recv_img_buf[img_num].frame->data, data + 20,
			g_net_recv_img_buf[img_num].width * g_net_recv_img_buf[img_num].height);

#if 0
	if (g_conn_state == LINK_ON)
	{
		length_info_head.length = g_net_recv_img_buf[img_num].width * g_net_recv_img_buf[img_num].height +96;
		msg = msg_factory_produce(g_net_mbx_id);
		msg->ops->set_data(msg, &length_info_head, sizeof(length_info_head), NULL, 0);
		msg->ops->set_data(msg, &validdata1, 4, NULL, 0);
		msg->ops->set_data(msg, &(g_net_recv_img_buf[img_num].video_id), 16, NULL, 0);
		validdata2++;
		msg->ops->set_data(msg, &validdata2, 4, NULL, 0);
		validdata3+=10;
		msg->ops->set_data(msg, &validdata3, 4, NULL, 0);
		msg->ops->set_data(msg, validdata, 64, NULL, 0);
		msg->ops->set_data(msg, g_net_recv_img_buf[img_num].image, g_net_recv_img_buf[img_num].width * g_net_recv_img_buf[img_num].height, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
#endif
	img_num++;

	if(img_num == video_num_enabled)
	{
		img_num = 0;
		sem_post(&gsem_algo_sim);
	}
	return 0;
}

/* 模拟触发 */
__u32 NET_DL(0x45521117)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	fpga_set_sim_trig(data);
	return 0;
}

/* 设置样品长度 */
__u32 NET_DL(0x45521118)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_sample_len(data);
	return 0;
}

/* 设置超频阈值 */
__u32 NET_DL(0x45521119)(void *data, __u32 len)
{
	__u32 val;
	MSG("%s\n",__FUNCTION__);
	sc_set_oc_th(data);
//	val = FPGA_READ32(fpga_base, FPGA_FREQLIMITL_REG);
//	printf("FPGA_FREQLIMITL_REG = %d\n",val);
	return 0;
}

/* 安装测试类型*/
__u32 NET_DL(0x4552111A)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	send_test_pos(*(__u32*)data);
	return 0;
}

/* 日期时间校准 */
__u32 NET_DL(0x4552111B)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	struct tm time_tm;
	struct timeval time_tv;
	time_t timep;
	int ret = 0;
	struct rtc_time rtc_tm;
	int fd;

	time_tm.tm_sec 	= *(__u16*)data;
	time_tm.tm_min 	= *((__u16*)data+1);
	time_tm.tm_hour 	= *((__u16*)data+2);
	time_tm.tm_mday 	= *((__u16*)data+3);
	time_tm.tm_mon 	= *((__u16*)data+4);
	time_tm.tm_year 	= *((__u16*)data+5);

	time_tm.tm_year -= 1900;
	time_tm.tm_mon -= 1;
	time_tm.tm_wday = 0;
	time_tm.tm_yday = 0;
	time_tm.tm_isdst = 0;

	timep = mktime(&time_tm);
	time_tv.tv_sec = timep;
	time_tv.tv_usec = 0;

	ret = settimeofday(&time_tv, NULL);
	if(ret != 0)
    {
		perror("settimeofday");
		return -1;
    }
	rtc_tm.tm_hour = time_tm.tm_hour;
	rtc_tm.tm_min = time_tm.tm_min;
	rtc_tm.tm_sec = time_tm.tm_sec;
	rtc_tm.tm_year = time_tm.tm_year;
	rtc_tm.tm_mon = time_tm.tm_mon;
	rtc_tm.tm_mday = time_tm.tm_mday;
	rtc_tm.tm_wday = time_tm.tm_wday;
	rtc_tm.tm_yday = time_tm.tm_yday;
	rtc_tm.tm_isdst = time_tm.tm_isdst;

	fd = open("/dev/rtc0", O_RDWR);
	if (fd == -1)
    {
    	perror("/dev/rtc0");
    }
	/* Set the RTC time/date */
	ret = ioctl(fd, RTC_SET_TIME, &rtc_tm);
	if (ret == -1)
	{
		perror("ioctl");
    }

	close(fd);

	return 0;
}
/*************************************************************************/


/***************************PC查询主板信息********************************/
/* 查询延时信息*/
void NET_DL(0x45522101)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x45530107)(NULL, 0);
}

/* 查询系统工作模式*/
void NET_DL(0x45522102)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	//NET_UL(0x45530108)(NULL, 0);
}

/* 查询信号设置 */
void NET_DL(0x45522103)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x45530106)(NULL, 0);
}

/* 查询视频通道分组信息*/
void NET_DL(0x45522104)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x45530109)(data, len);
}

/* 查询视频通道需求*/
void NET_DL(0x45522105)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x4553010C)(data, len);
}

/* 查询主控板版本信息*/
void NET_DL(0x45522106)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x45530104)(NULL, 0);
}

/* 查询系统功能设置*/
void NET_DL(0x45522107)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x4553010D)(NULL, 0);
}

/* 查询算法参数*/
void NET_DL(0x45522108)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x4553010F)(NULL, 0);
}

/* 查询样品长度*/
void NET_DL(0x45522109)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x45530117)(NULL, 0);
}

/* 查询超频阈值*/
void NET_DL(0x4552210A)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x45530112)(NULL, 0);
}

/* 预留*/
void NET_DL(0x4552210B)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
}

/* 查询FPGA调试信息*/
void NET_DL(0x4552210C)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x45530114)(NULL, 0);
}

/* 查询样品判决模式*/
void NET_DL(0x4552210D)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x45530115)(NULL, 0);
}

/* 查询图像发送模式*/
void NET_DL(0x4552210E)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	NET_UL(0x45530116)(NULL, 0);
}
/*************************************************************************/

__u32 (*mainboard_set_func[])(void *data, __u32 len) =
{
	NULL,
	NET_DL(0x45521101),
	NET_DL(0x45521102),
	NET_DL(0x45521103),
	NET_DL(0x45521104),
	NET_DL(0x45521105),
	NET_DL(0x45521106),
	NET_DL(0x45521107),
	NET_DL(0x45521108),
	NET_DL(0x45521109),
	NET_DL(0x4552110A),
	NET_DL(0x4552110B),
	NET_DL(0x4552110C),
	NET_DL(0x4552110D),
	NET_DL(0x4552110E),
	NET_DL(0x4552110F),
	NET_DL(0x45521110),
	NET_DL(0x45521111),
	NET_DL(0x45521112),
	NET_DL(0x45521113),
	NET_DL(0x45521114),
	NET_DL(0x45521115),
	NET_DL(0x45521116),
	NET_DL(0x45521117),
	NET_DL(0x45521118),
	NET_DL(0x45521119),
	NET_DL(0x4552111A),
	NET_DL(0x4552111B),
};

void (*mainboard_get_func[])(void *data, __u32 len) =
{
	NULL,
	NET_DL(0x45522101),
	NET_DL(0x45522102),
	NET_DL(0x45522103),
	NET_DL(0x45522104),
	NET_DL(0x45522105),
	NET_DL(0x45522106),
	NET_DL(0x45522107),
	NET_DL(0x45522108),
	NET_DL(0x45522109),
	NET_DL(0x4552210A),
	NET_DL(0x4552210B),
	NET_DL(0x4552210C),
	NET_DL(0x4552210D),
	NET_DL(0x4552210E)
};



