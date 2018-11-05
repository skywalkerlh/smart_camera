/*
 * dsp.c
 *
 *  Created on: 2015年4月28日
 *      Author: work
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "dsp.h"
#include "sys_log.h"
#include "debug.h"
#include "video_context.h"
#include "shared_mem_manager.h"
#include "sample_factory.h"
#include "v4l2.h"

static struct list_head busy_list;
static struct list_head idle_list;
static pthread_mutex_t mutex_dsp_list;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


__u32 g_dsp1_reset_count = 0;
__u32 g_dsp2_reset_count = 0;
__u32 g_dsp1_timeout_count = 0;
__u32 g_dsp2_timeout_count = 0;


static __s32 dsp1_set_algo_params(void *p, struct AlgoParams *pAlgoParams);
static __s32 dsp1_get_version(void *p, char *version);
static __s32 dsp1_process(void *p);

static __s32 dsp2_set_algo_params(void *p, struct AlgoParams *pAlgoParams);
static __s32 dsp2_get_version(void *p, char *version);
static __s32 dsp2_process(void *p);

static __s32 is_dsps_idle();



static struct DSP* g_dsp1,*g_dsp2;

struct DSP *get_dsp(__u32 id)
{
	if(id == DSP1)
		return g_dsp1;
	else if(id == DSP2)
		return g_dsp2;
	else
	{
		ERROR("get_dsp1");
		return NULL;
	}
}

struct DSP *connect_to_dsp1()
{
	struct DSP *p = NULL;
	Engine_Error ec;
	XDAS_Int32 err;
	XDAS_Int8 **pOutBuffer;
	XDAS_Int32 *pOutBufSizes;
	struct shared_memory *pSharedMemory;

	struct AlgoParams *pAlgoParams = sc_get_algo_params(NULL);

	p = malloc(sizeof(struct DSP));
	if (p == NULL)
		ERROR("malloc");

	p->pArgsInfo = (struct DSPArgsInfo *)malloc(sizeof(struct DSPArgsInfo));
	if (p->pArgsInfo == NULL)
	{
		free(p);
		ERROR("malloc");
	}

	p->id = DSP1;
	p->process = dsp1_process;
#if 1
	while(1)
	{
		p->engine = Engine_open("dsp_vidsvr", NULL, &ec);
		if ( !p->engine )
		{
			ERROR("Engine_open");
			sleep(1);
			continue;
		}
		else
			break;
	}
#endif
	p->params = dsp_dce_alloc(sizeof(IVIDDEC2_Params));
	p->params->size = sizeof(IVIDDEC2_Params);
#if 1
	p->codec = VIDDEC2_create(p->engine, "dsp_universalCopy",
			(IVIDDEC2_Params*) (p->params));
	if ( !p->codec )
		ERROR("VIDDEC2_create");
#endif
	p->dynParams = dsp_dce_alloc(sizeof(struct ICVIDDEC_Params));
	p->dynParams->base.size = sizeof(struct ICVIDDEC_Params);

	p->status = dsp_dce_alloc(sizeof(struct ICVIDDEC_Status));
	p->status->base.size = sizeof(struct ICVIDDEC_Status);

	p->status->base.data.buf = dsp_dce_alloc(STATUS_BUF_LEN);
	p->status->base.data.bufSize = STATUS_BUF_LEN;

	/* 创建DCE process接口输入缓存区 */
	p->inBufs = dsp_dce_alloc(sizeof(XDM1_BufDesc));
	p->inBufs->numBufs = 2;
	p->inBufs->descs[0].buf = NULL;
	p->inBufs->descs[0].bufSize = 0;
	p->inBufs->descs[1].buf = NULL;
	p->inBufs->descs[1].bufSize = 0;


	/* 创建DCE process接口输出缓存区 */
	p->outBufs = dsp_dce_alloc(sizeof(XDM_BufDesc));
	p->outBufs->numBufs = 1;
	pOutBuffer = dsp_dce_alloc(sizeof(XDAS_Int8*));
	pOutBufSizes = dsp_dce_alloc(sizeof(XDAS_Int32) );
	pSharedMemory = shared_memory_alloc( OUTPUT_BUF_WIDTH * OUTPUT_BUF_HEIGHT );
	*pOutBuffer = (XDAS_Int8*) (pSharedMemory->fd);
	*pOutBufSizes = OUTPUT_BUF_WIDTH*OUTPUT_BUF_HEIGHT;
	p->outBufs->bufs = pOutBuffer;
	p->outBufs->bufSizes = pOutBufSizes;

	/* 创建DCE process接口输入参数数据区 */
	p->inArgs = dsp_dce_alloc(sizeof(struct ICVIDDEC_InArgs));
	p->inArgs->base.size = sizeof(struct ICVIDDEC_InArgs);
	p->inArgs->emulatorConnected = check_emulator_state();

	/* 创建DCE process接口输出参数数据区 */
	p->outArgs = dsp_dce_alloc(sizeof(struct ICVIDDEC_OutArgs));
	p->outArgs->base.size = sizeof(struct ICVIDDEC_OutArgs);

	dsp1_set_algo_params(p,  pAlgoParams);

	sem_init(&p->sem_img_ready, NULL, 0);

	return p;
}

struct DSP *connect_to_dsp2()
{
	struct DSP *p = NULL;
		Engine_Error ec;
		XDAS_Int32 err;
		XDAS_Int8 **pOutBuffer;
		XDAS_Int32 *pOutBufSizes;
		struct shared_memory *pSharedMemory;

		struct AlgoParams *pAlgoParams = sc_get_algo_params(NULL);

		p = malloc(sizeof(struct DSP));
		if (p == NULL)
			ERROR("malloc");

		p->pArgsInfo = (struct DSPArgsInfo *)malloc(sizeof(struct DSPArgsInfo));
		if (p->pArgsInfo == NULL)
		{
			free(p);
			ERROR("malloc");
		}

		p->id = DSP2;
		p->process = dsp2_process;
	#if 1
		while(1)
		{
			p->engine = Engine_open("dsp2_vidsvr", NULL, &ec);
			if ( !p->engine )
			{
				ERROR("Engine_open");
				sleep(1);
				continue;
			}
			else
				break;
		}
	#endif
		p->params = dsp2_dce_alloc(sizeof(IVIDDEC2_Params));
		p->params->size = sizeof(IVIDDEC2_Params);
	#if 1
		p->codec = VIDDEC2_dsp2_create(p->engine, "dsp_universalCopy",
				(IVIDDEC2_Params*) (p->params));
		if (!p->codec)
			ERROR("VIDDEC2_dsp2_create");
	#endif
		p->dynParams = dsp2_dce_alloc(sizeof(struct ICVIDDEC_Params));
		p->dynParams->base.size = sizeof(struct ICVIDDEC_Params);

		p->status = dsp2_dce_alloc(sizeof(struct ICVIDDEC_Status));
		p->status->base.size = sizeof(struct ICVIDDEC_Status);

		p->status->base.data.buf = dsp2_dce_alloc(STATUS_BUF_LEN);
		p->status->base.data.bufSize = STATUS_BUF_LEN;

		/* 创建DCE process接口输入缓存区 */
		p->inBufs = dsp2_dce_alloc(sizeof(XDM1_BufDesc));
		p->inBufs->numBufs = 2;
		p->inBufs->descs[0].buf = NULL;
		p->inBufs->descs[0].bufSize = 0;
		p->inBufs->descs[1].buf = NULL;
		p->inBufs->descs[1].bufSize = 0;

		/* 创建DCE process接口输出缓存区 */
		p->outBufs = dsp2_dce_alloc(sizeof(XDM_BufDesc));
		p->outBufs->numBufs = 1;
		pOutBuffer = dsp2_dce_alloc(sizeof(XDAS_Int8*));
		pOutBufSizes = dsp2_dce_alloc(sizeof(XDAS_Int32));
		pSharedMemory = shared_memory_alloc( OUTPUT_BUF_WIDTH * OUTPUT_BUF_HEIGHT );
		*pOutBuffer = (XDAS_Int8*) (pSharedMemory->fd);
		*pOutBufSizes = OUTPUT_BUF_WIDTH*OUTPUT_BUF_HEIGHT;
		p->outBufs->bufs = pOutBuffer;
		p->outBufs->bufSizes = pOutBufSizes;

		/* 创建DCE process接口输入参数数据区 */
		p->inArgs = dsp2_dce_alloc(sizeof(struct ICVIDDEC_InArgs));
		p->inArgs->base.size = sizeof(struct ICVIDDEC_InArgs);
		p->inArgs->emulatorConnected = check_emulator_state();

		/* 创建DCE process接口输出参数数据区 */
		p->outArgs = dsp2_dce_alloc(sizeof(struct ICVIDDEC_OutArgs));
		p->outArgs->base.size = sizeof(struct ICVIDDEC_OutArgs);

		dsp2_set_algo_params(p,  pAlgoParams);

		sem_init(&p->sem_img_ready, NULL, 0);

		return p;
}






void disconnect_from_dsp1()
{
	VIDDEC2_delete(g_dsp1->codec);
	Engine_close(g_dsp1->engine);
	sem_destroy(&(g_dsp1->sem_img_ready));
	free(g_dsp1->pArgsInfo);
	free(g_dsp1);
}

void disconnect_from_dsp2()
{
	VIDDEC2_dsp2_delete(g_dsp2->codec);
	Engine_close(g_dsp2->engine);
	sem_destroy(&(g_dsp2->sem_img_ready));
	free(g_dsp2->pArgsInfo);
	free(g_dsp2);
}

void  create_dsp_factory(char * dsp1_ver, char *dsp2_ver)
{
	struct DSP *p = NULL;

	init_list_head(&busy_list);
	init_list_head(&idle_list);
	pthread_mutex_init(&mutex_dsp_list, NULL);

	p = connect_to_dsp1();
	if(p == NULL)
		ERROR("create_dsp1_object");
	g_dsp1 = p;
	dsp1_get_version(g_dsp1, dsp1_ver);

	list_add_tail(&(p->node), &idle_list);

	p = connect_to_dsp2();
	if(p == NULL)
		ERROR("create_dsp2_object");
	g_dsp2 = p;
	dsp2_get_version(g_dsp2, dsp2_ver);

	list_add_tail(&(p->node), &idle_list);
}

__s32 dsp1_process(void *p )
{
	__u32 video_id = 0;
	__u32 width = 0;
	__u32 height = 0;
	__u32 bayer_fmt = 0;
	__u8  buf[32];
	__u32 i = 0;
	__u32 num = 0;
	struct timeval t_start, t_end;
	struct DSP *dsp = (struct DSP *)p;

	XDM1_BufDesc *inBufs = dsp->inBufs;
	XDM_BufDesc *outBufs = dsp->outBufs;
	struct ICVIDDEC_InArgs *inArgs = dsp->inArgs;
	struct ICVIDDEC_OutArgs *outArgs = dsp->outArgs;
	XDAS_Int32 err = XDM_EOK;

	struct DSPArgsInfo *pArgsInfo = dsp->pArgsInfo;

	struct SampleInfo *pSampleInfo = &(pArgsInfo->sample_image_group->sample_info);

	num = pArgsInfo->sample_image_group->image_num;
	dsp->inBufs->numBufs = num;
	inArgs->num = num;

	for (i = 0; i < num; i++)
	{
		video_id = pArgsInfo->sample_image_group->image[i]->video_id;
		width = pArgsInfo->sample_image_group->image[i]->width;
		height = pArgsInfo->sample_image_group->image[i]->height;
		bayer_fmt = pArgsInfo->sample_image_group->image[i]->bayer_fmt;
		dsp->inBufs->descs[i].buf =
				(XDAS_Int8 *) (pArgsInfo->sample_image_group->image[i]->frame->fd);
		dsp->inBufs->descs[i].bufSize = width * height;

		inArgs->videoIDs[i] = video_id;
		inArgs->imageWidth[video_id] = width;
		inArgs->imageHeight[video_id] = height;
		inArgs->bayerFormat[video_id] = bayer_fmt;
	}

	inArgs->emulatorConnected = check_emulator_state();

//	outArgs->timeout_flag = 0;
//	outArgs->img_upload_flag = 0;

	gettimeofday(&t_start, NULL);
	err = VIDDEC2_process(dsp->codec, inBufs, outBufs, (VIDDEC2_InArgs*) inArgs,
			(VIDDEC2_OutArgs*) outArgs);
	if (err)
	{
		MSG("%p: VIDDEC2_process returned error: %d", p, err);
	}
	gettimeofday(&t_end, NULL);

	/* 更新样品信息 */
	pSampleInfo->state  = 0;
	pSampleInfo->algoCalcTime = 1000000 * (t_end.tv_sec - t_start.tv_sec)
			+ (t_end.tv_usec - t_start.tv_usec);
	pSampleInfo->algoCalcTime /= 1000;

	memcpy(pSampleInfo->calc_result, outArgs->calc_result,
			sizeof(XDAS_Int8) * 128);	//算法计算结果
	memcpy(pSampleInfo->decision, outArgs->decision, sizeof(XDAS_Int8) * 8);// 分组对样品的处理结论
	memcpy(&(pArgsInfo->img_upload_flag), &(outArgs->img_upload_flag),
			sizeof(XDAS_Int32));  //图像是否需要上传

	pSampleInfo->flag = 0;

	if(outArgs->timeout_flag)
	{
		pSampleInfo->state |= (1<<16);//处理样品时DSP超时
		g_dsp1_timeout_count++;
	}
	if(err)
	{
		pSampleInfo->state |= (3<<16);//处理样品时DSP崩溃
		g_dsp1_reset_count++;
		sprintf(buf, "dsp1 reset %d times",g_dsp1_reset_count);
		log_builder(buf);
	}
	/* 统计DSP1超时次数, 未实现 */

	for (i = 0; i < num; i++)
	{
		if (pArgsInfo->sample_image_group->image[i]->reserved)
			memcpy(pArgsInfo->sample_image_group->image[i]->reserved,
					outArgs->img_flaw_areas + 32, 64);
	}

	/*
	 * 更新算法统计信息，未实现
	 */

	return err;
}

__s32 dsp1_set_algo_params(void *p,  struct AlgoParams *pAlgoParams)
{
	XDAS_Int32 err = XDM_EOK;
	struct DSP *dsp = (struct DSP *)p;
	void *params = (struct NetInfoHead *)pAlgoParams + 1;
	memcpy(&(dsp->dynParams->params), params, sizeof(*pAlgoParams)-sizeof(struct NetInfoHead));
	err = VIDDEC2_control(dsp->codec, XDM_SETPARAMS,
			(VIDDEC2_DynamicParams*) (dsp->dynParams), (VIDDEC2_Status *)(dsp->status));
	if (err)
		ERROR("VIDDEC2_control");
	return err;
}

__s32 dsp1_get_version(void *p, char* version)
{
	XDAS_Int32 err = XDM_EOK;
	struct DSP *dsp = (struct DSP *)p;
	err = VIDDEC2_control(dsp->codec, XDM_GETVERSION,
			(VIDDEC2_DynamicParams*) (dsp->dynParams),
			(VIDDEC2_Status*) (dsp->status));
	if (err)
		ERROR("VIDDEC2_control");

	memcpy(version, dsp->status->data, 32);
	return err;
}

__s32 dsp2_process(void *p )
{
	__u32 video_id = 0;
	__u32 width = 0;
	__u32 height = 0;
	__u32 bayer_fmt = 0;
	__u8  buf[32];
	__u32 i = 0;
	__u32 num = 0;
	struct timeval t_start, t_end;
	struct DSP *dsp = (struct DSP *)p;

	XDM1_BufDesc *inBufs = dsp->inBufs;
	XDM_BufDesc *outBufs = dsp->outBufs;
	struct ICVIDDEC_InArgs *inArgs = dsp->inArgs;
	struct ICVIDDEC_OutArgs *outArgs = dsp->outArgs;
	XDAS_Int32 err = XDM_EOK;

	struct DSPArgsInfo *pArgsInfo = dsp->pArgsInfo;
	struct SampleInfo *pSampleInfo = &(dsp->pArgsInfo->sample_image_group->sample_info);

	num = pArgsInfo->sample_image_group->image_num;
	dsp->inBufs->numBufs = num;
	inArgs->num = num;

	for (i = 0; i < num; i++)
	{
		video_id = pArgsInfo->sample_image_group->image[i]->video_id;
		width = pArgsInfo->sample_image_group->image[i]->width;
		height = pArgsInfo->sample_image_group->image[i]->height;
		bayer_fmt = pArgsInfo->sample_image_group->image[i]->bayer_fmt;
		dsp->inBufs->descs[i].buf =
				(XDAS_Int8 *) (pArgsInfo->sample_image_group->image[i]->frame->fd);
		dsp->inBufs->descs[i].bufSize = width * height;
		inArgs->videoIDs[i] = video_id;
		inArgs->imageWidth[video_id] = width;
		inArgs->imageHeight[video_id] = height;
		inArgs->bayerFormat[video_id] = bayer_fmt;
	}
	inArgs->emulatorConnected = check_emulator_state();

//	outArgs->timeout_flag = 0;
//	outArgs->img_upload_flag = 0;

	gettimeofday(&t_start, NULL);
	err = VIDDEC2_dsp2_process(dsp->codec, inBufs, outBufs,
			(VIDDEC2_InArgs*) inArgs, (VIDDEC2_OutArgs*) outArgs);
	if (err)
	{
		MSG("%p: VIDDEC2_dsp2_process returned error: %d", p, err);
	}
	gettimeofday(&t_end, NULL);

	/*
	 * 更新样品信息
	 */
	pSampleInfo->state  = 0;
	pSampleInfo->algoCalcTime = 1000000 * (t_end.tv_sec - t_start.tv_sec)
			+ (t_end.tv_usec - t_start.tv_usec);
	pSampleInfo->algoCalcTime /= 1000;

	memcpy(pSampleInfo->calc_result, outArgs->calc_result,sizeof(XDAS_Int8) * 128);	//算法计算结果
	memcpy(pSampleInfo->decision, outArgs->decision, sizeof(XDAS_Int8) * 8);				// 分组对样品的处理结论
	memcpy(&(pArgsInfo->img_upload_flag), &(outArgs->img_upload_flag),
			sizeof(XDAS_Int32));  //图像是否需要上传

	pSampleInfo->flag = 0;

	if(outArgs->timeout_flag)
	{
		pSampleInfo->state |= (1<<16);//处理样品时DSP超时
		g_dsp2_timeout_count++;
	}
	if(err)
	{
		pSampleInfo->state |= (3<<16);//处理样品时DSP崩溃
		g_dsp2_reset_count++;
		sprintf(buf, "dsp2 reset %d times",g_dsp2_reset_count);
		log_builder(buf);
	}
	/* 统计DSP2超时次数, 未实现 */

	for (i = 0; i < num; i++)
	{
		if (pArgsInfo->sample_image_group->image[i]->reserved)
			memcpy(pArgsInfo->sample_image_group->image[i]->reserved,
					outArgs->img_flaw_areas + 32, 64);
	}

	/*
	 * 更新算法统计信息，未实现
	 */

	return err;
}

__s32 dsp2_set_algo_params(void *p,  struct AlgoParams *pAlgoParams)
{
	XDAS_Int32 err = XDM_EOK;
	struct DSP *dsp = (struct DSP *)p;
	void *params = (struct NetInfoHead *)pAlgoParams + 1;
	memcpy(&(dsp->dynParams->params), params, sizeof(*pAlgoParams)-sizeof(struct NetInfoHead));
	err = VIDDEC2_dsp2_control(dsp->codec, XDM_SETPARAMS,
			(VIDDEC2_DynamicParams*) (dsp->dynParams), (VIDDEC2_Status *)(dsp->status));
	if (err)
		ERROR("VIDDEC2_control");
	return err;
}

__s32 dsp2_get_version(void *p, char* version)
{
	XDAS_Int32 err = XDM_EOK;
	struct DSP *dsp = (struct DSP *)p;
	err = VIDDEC2_dsp2_control(dsp->codec, XDM_GETVERSION,
			(VIDDEC2_DynamicParams*) (dsp->dynParams),
			(VIDDEC2_Status*) (dsp->status));
	if (err)
		ERROR("VIDDEC2_control");

	memcpy(version, dsp->status->data, 32);
	return err;
}

struct DSP* dsp_factory_produce()
{
	struct DSP *p;

	pthread_mutex_lock(&mutex_dsp_list);
	while (list_empty(&idle_list))
		pthread_cond_wait(&cond, &mutex_dsp_list);
	p = (struct DSP*) (idle_list.next);
	list_del(idle_list.next);
	list_add_tail(&(p->node), &busy_list);
	pthread_mutex_unlock(&mutex_dsp_list);

	return p;
}

void dsp_factory_recycle(struct DSP *p)
{
	pthread_mutex_lock(&mutex_dsp_list);
	list_del(&(p->node));
	list_add_tail(&(p->node), &idle_list);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex_dsp_list);
}

__s32 dsp_factory_rectify(struct AlgoParams *pAlgoParams)
{
	__s32 ret = 0;

	struct DSP *p1 = NULL;
	struct DSP *p2 = NULL;

	while(1)
	{
		if(is_dsps_idle())
			break;
		else
			usleep(1000);
	}

	p1 = dsp_factory_produce();
	if(p1->id == DSP1)
	{
		ret = dsp1_set_algo_params(p1,  pAlgoParams);
	}
	else if(p1->id == DSP2)
	{
		ret = dsp2_set_algo_params(p1,  pAlgoParams);
	}
	else
	{
		ret =  -1;
		dsp_factory_recycle(p1);
		goto OUT;
	}

	p2 = dsp_factory_produce();
	if(p2->id == DSP1)
	{
		ret = dsp1_set_algo_params(p2,  pAlgoParams);
	}
	else if(p2->id == DSP2)
	{
		ret = dsp2_set_algo_params(p2,  pAlgoParams);
	}
	else
	{
		ret =  -1;
		dsp_factory_recycle(p1);
		dsp_factory_recycle(p2);
		goto OUT;
	}

	dsp_factory_recycle(p1);
	dsp_factory_recycle(p2);

OUT:
	return ret;
}

__s32 is_dsps_idle()
{
	__s32 state = 0;

	pthread_mutex_lock(&mutex_dsp_list);
	state = list_empty(&busy_list);
	pthread_mutex_unlock(&mutex_dsp_list);

	return state;
}
