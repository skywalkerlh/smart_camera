/*
 * dsp.h
 *
 *  Created on: 2015年4月28日
 *      Author: work
 */

#ifndef DSP_H_
#define DSP_H_

#include <semaphore.h>
#include <libdce.h>
#include <linux/types.h>
#include "list.h"
//#include "define.h"
#include "sys_conf.h"

/*  与DSP交互部分 */

#define IC_MAX_VIDEO_CHANNELS 2
//#define IC_MAX_IMAGE_FLAWS 8
//#define IC_MAX_GROUPS 4

#define DSP1										1
#define DSP2										2
#define STATUS_BUF_LEN				1024	/* 存放从DSP查询的信息，见struct ICVIDDEC_Status */
#define OUTPUT_BUF_WIDTH		2048	/* 用于定义输出缓冲区的大小*/
#define OUTPUT_BUF_HEIGHT		1024	/* 用于定义输出缓冲区的大小*/

struct ICVIDDEC_Status
{
	VIDDEC2_Status base;
	XDAS_UInt8 data[STATUS_BUF_LEN];
};

 struct ICVIDDEC_InArgs
{
	VIDDEC2_InArgs base;

	XDAS_Int32 videoIDs[IC_MAX_VIDEO_CHANNELS];
	XDAS_Int32 imageWidth[IC_MAX_VIDEO_CHANNELS];
	XDAS_Int32 imageHeight[IC_MAX_VIDEO_CHANNELS];
	XDAS_Int32 bayerFormat[IC_MAX_VIDEO_CHANNELS];

	XDAS_Int32 num;
	XDAS_Int32 emulatorConnected;

};

 struct DSPAlgoParams
 {
 	__s32  params_grp_amount;	/* 参数分组编号 */
 	__s32 valid_params_grp;		/* 有效分组编号 */
 	__u32 reckon_time; 				/* 算法时间估计值 */
 	__u8   params[1024];
 };

struct ICVIDDEC_Params
{
	VIDDEC2_DynamicParams base;
	struct DSPAlgoParams params;
};

/*算法统计计数*/
struct AlgoCounter
{
	__u32 a;
	__u32 b;
	__u32 c;
	__u32 d;
	__u32 e;
	__u32 f;
	__u32 g;
	__u32 h;
};

struct ICVIDDEC_OutArgs
{
	VIDDEC2_OutArgs base;

	XDAS_UInt8 decision[8];/* 分组对样品的处理意见 */
	XDAS_UInt8 calc_result[128];/* 分组内算法计算结果 */
	XDAS_Int32 decide_pos;
	XDAS_Int32 timeout_flag; 	/* 超时标记 */
	/*
	 * 算法告知分组内各图像是否需要上传
	 * 0: 不上传
	 * 1: 上传
	 * 2: 不确定
	 */
	XDAS_Int32 img_upload_flag;
//	XDAS_Int32 symbol; /* 依据这个标识对图像文件命名 */
	XDAS_UInt16 img_flaw_areas[128];/* 支持最多4个视频通道对应的缺陷识别区域 */
	struct AlgoCounter algoCounter;
};

struct dsp_ops
{
	int (*process)(void *p);
	int (*get_version)(void  *p, char*version);
	int (*set_algo_params)(void *p, struct AlgoParams *pAlgoParams);
};

struct DSP
{
	struct list_head node;
	int id;

	Engine_Handle engine;
	VIDDEC2_Handle codec;

	XDM1_BufDesc *inBufs;
	XDM_BufDesc *outBufs;

	VIDDEC2_Params *params;
	struct ICVIDDEC_InArgs *inArgs;
	struct ICVIDDEC_OutArgs *outArgs;

	struct ICVIDDEC_Params *dynParams;
	struct ICVIDDEC_Status *status;

	struct DSPArgsInfo *pArgsInfo;
	sem_t sem_img_ready;

	__s32 (*process)(void *p );
};

void  create_dsp_factory(char *dsp1_ver, char *dsp2_ver);
__s32 dsp_factory_rectify(struct AlgoParams *pAlgoParams);

struct DSP* dsp_factory_produce();
void dsp_factory_recycle(struct DSP *p);

struct DSP *connect_to_dsp1();
struct DSP *connect_to_dsp2();

void disconnect_from_dsp1();
void disconnect_from_dsp2();

struct DSP *get_dsp(__u32 id);



#endif /* DSP_H_ */
