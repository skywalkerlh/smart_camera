/*
 * dsp_context.c
 *
 *  Created on: 2015年5月25日
 *      Author: work
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "mailbox.h"
#include "dsp.h"
#include "message.h"
#include "msg_factory.h"
#include "net_protocol.h"
#include "video_context.h"
#include "sample_factory.h"
#include "net_context.h"
#include "debug.h"
#include "thread.h"
#include "local_protocol.h"
#include "ui_protocol.h"
#include "sys_log.h"

extern __s32 g_mbx_send;
extern struct AlgoParamsConfig g_algo_params_config;
//static __u32 dsp1_process_mun = 0;
//static __u32 dsp2_process_mun = 0;

void dsp1_process_tsk()
{
	__u32 image_num = 0;
//	__s32 ret = 0;
	__s32 err = 0;
//	__u32 i;
//	__u8  buf[32];
	__u32 decide_pos;

	struct message *msg;
	struct DSPArgsInfo *pArgsInfo = NULL;
	struct SampleInfo *pSampleInfo = NULL;
	struct SampleResult *pSampleResult = NULL;
	struct SampleImageGroup  *pSampleImageGroup = NULL;

	struct DSP *dsp = get_dsp(DSP1);
	struct ImageSendMode *pImageSendMode = sc_get_img_send_mode(NULL);

	while(1)
	{
		sem_wait(&dsp->sem_img_ready);

		err = dsp->process(dsp);

		pArgsInfo = dsp->pArgsInfo;
		pSampleInfo = &(pArgsInfo->sample_image_group->sample_info);
		pSampleImageGroup = pArgsInfo->sample_image_group;
		image_num = pArgsInfo->sample_image_group->image_num;

		/* 发送样品信息至踢废管理进程*/
		pSampleResult = get_sample_result_array();
		memcpy(pSampleResult->result, pSampleInfo->decision, 8);
		pSampleResult->count = pSampleInfo->count;
		pSampleResult->wheel_code = pSampleImageGroup->shoot_wheel_code;

		decide_pos = dsp->outArgs->decide_pos & 0xFFFF;
		if((decide_pos == 0)||(decide_pos == 1)||(decide_pos == 2))
		{
			pSampleResult->pos = decide_pos;
			pSampleImageGroup->sample_info.kick_pos = dsp->outArgs->decide_pos;
		}
		else
		{
			pSampleResult->pos = 0;
			pSampleImageGroup->sample_info.kick_pos = dsp->outArgs->decide_pos & 0xFFFF0000;
		}
		//pSampleResult->alarm =;
		send_sample_reault(pSampleResult, sizeof(SampleResult));

		/* 发送样品信息至网络 */
		send_sample_info(pSampleImageGroup, video_context_release);

		/*向网络 发送图像 */
		if (pImageSendMode->mode == 2)
		{
			if(pArgsInfo->img_upload_flag == 1)
			{
				//增加图像拷贝代码，拷贝完即时释放内存，以防100M网络不能即时发送所有图像造成底层缓存耗尽
				msg = msg_factory_produce(g_mbx_send, 0);
				msg->ops->set_data(msg, pSampleImageGroup, 0, NULL, 0);
				mailbox_post(msg);
			}
			else
			{
				//释放image_process_tsk中的图像资源占用
				video_context_release(pSampleImageGroup);
			}
		}

		if(err)
		{
			disconnect_from_dsp1();
			sleep(1);
			connect_to_dsp1();
		}
//		else
//		{
//			dsp1_process_mun++;
//		}

		dsp_factory_recycle(dsp);
	}
}

void dsp2_process_tsk()
{
	__u32 image_num = 0;
//	__s32 ret = 0;
	__s32 err = 0;
//	__u32 i;
//	__u8  buf[32];
	__u32 decide_pos;

	struct message *msg;
	struct DSPArgsInfo *pArgsInfo = NULL;
	struct SampleInfo *pSampleInfo = NULL;
	struct SampleResult *pSampleResult = NULL;
	struct DSP *dsp = get_dsp(DSP2);
	struct ImageSendMode *pImageSendMode = sc_get_img_send_mode(NULL);
	struct SampleImageGroup  *pSampleImageGroup = NULL;

	while(1)
	{
		sem_wait(&dsp->sem_img_ready);
		err = dsp->process(dsp);
		pArgsInfo = dsp->pArgsInfo;

		pSampleImageGroup = pArgsInfo->sample_image_group;
		pSampleInfo = &(pArgsInfo->sample_image_group->sample_info);
		pSampleImageGroup = pArgsInfo->sample_image_group;
		image_num = pArgsInfo->sample_image_group->image_num;

		/* 发送样品信息至踢废管理进程*/
		pSampleResult = get_sample_result_array();
		memcpy(pSampleResult->result, pSampleInfo->decision, 8);
		pSampleResult->count = pSampleInfo->count;
		pSampleResult->wheel_code = pSampleImageGroup->shoot_wheel_code;

		decide_pos = dsp->outArgs->decide_pos & 0xFFFF;
		if((decide_pos == 0)||(decide_pos == 1)||(decide_pos == 2))
		{
			pSampleResult->pos = decide_pos;
			pSampleImageGroup->sample_info.kick_pos = dsp->outArgs->decide_pos;
		}
		else
		{
			pSampleResult->pos = 0;
			pSampleImageGroup->sample_info.kick_pos = dsp->outArgs->decide_pos & 0xFFFF0000;
		}
		//pSampleResult->alarm =;
		send_sample_reault(pSampleResult, sizeof(SampleResult));

		/* 发送样品信息至网络 */
		send_sample_info(pSampleImageGroup, video_context_release);

		/*向网络 发送图像 */
		if (pImageSendMode->mode == 2)
		{
			if(pArgsInfo->img_upload_flag == 1)
			{
				//增加图像拷贝代码，拷贝完即时释放内存，以防100M网络不能即时发送所有图像造成底层缓存耗尽
				msg = msg_factory_produce(g_mbx_send, 0);
				msg->ops->set_data(msg, pSampleImageGroup, 0, NULL, 0);
				mailbox_post(msg);
			}
			else
			{
				//释放image_process_tsk中的图像资源占用
				video_context_release(pSampleImageGroup);
			}
		}

		if(err)
		{
			disconnect_from_dsp2();
			sleep(1);
			connect_to_dsp2();
		}
//		else
//		{
//			dsp2_process_mun++;
//		}

		dsp_factory_recycle(dsp);
	}
}

sem_t g_sem_algo_params_update;

void dsps_update_params_tsk()
{
	struct AlgoParams *pAlgoParams = sc_get_algo_params(NULL);

	while(1)
	{
		sem_wait(&g_sem_algo_params_update);
		g_algo_params_config.update = 1;
		dsp_factory_rectify(pAlgoParams);
		g_algo_params_config.update = 0;
	}
}

void dsp_context_init()
{
	char dsp1_ver[32];
	char dsp2_ver[32];

	memset(dsp1_ver, 0, 32);
	memset(dsp1_ver, 0, 32);

	create_dsp_factory(dsp1_ver,dsp2_ver);

	sc_set_dsp1_version(dsp1_ver);
	sc_set_dsp2_version(dsp2_ver);

	sem_init(&g_sem_algo_params_update,0,0);

	add_new_thread(NULL, (void *)&dsp1_process_tsk, 30, 0, 8*1024);
	add_new_thread(NULL, (void *)&dsp2_process_tsk, 30, 0, 8*1024);
	add_new_thread(NULL, (void *)&dsps_update_params_tsk, 24, 0, 8*1024);
}

