/*
 * sample_factory.h
 *
 *  Created on: 2015年6月15日
 *      Author: work
 */

#ifndef SAMPLE_FACTORY_H_
#define SAMPLE_FACTORY_H_

#include "net_context.h"
#include "list.h"
#include "v4l2.h"
struct SampleInfo
{
	__u32	count;								/* 样品计数*/
	__u32	kick_pos;						/* 样品剔除工位*/
	__u32	algoCalcTime;				/* 样品算法处理时间*/
	__u32	wheel_code;					/* 码盘值*/
	__u8		decision[8];				/* 样品结论 */
	__u8		calc_result[128];	/* 算法中间结果 */

	/*
	 * 0:	正常
	 * 1:	相机超频
	 * 2:	处理超频
	 * 3:	超时（算法）
	 */
	__u32	flag;//样品标志

	__u32 state;//（预留）
}SampleInfo;

struct SampleImageGroup
{
	struct SampleInfo sample_info;
	struct image_buffer *image[2];
	__u32 image_num;
	struct list_head node;
	__s32 image_source; // 0:VP获取的图像（需要qbuf释放）， 1:其他途径的图像
	__u64 shoot_wheel_code;
}SampleImageGroup;


struct NetImageBuf
{
	__s32 video_id;
	__u32 width;
	__u32 height;
	__u32 bayer_fmt;
	__u32 code;
	void *image;
};

struct SampleImageGroup * sample_factory_produce();
void sample_factory_recycle(struct SampleImageGroup *__sample__);
void create_sample_factory(__u32 num);

#endif /* SAMPLE_FACTORY_H_ */
