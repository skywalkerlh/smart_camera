/*
 * shared_img_buf.h
 *
 *  Created on: 2015年7月21日
 *      Author: work
 */

#ifndef SHARED_IMG_BUF_H_
#define SHARED_IMG_BUF_H_


#include "linux/types.h"


#define IPC_KEY_A 0x366376
//#define IPC_KEY_A 0x12345678
#define IPC_KEY_B 0x366378

struct shared_image
{
	__u32 width;
	__u32 height;
	__u8 *data;
};

__u32 shared_vid1_buf_get_width();
__u32 shared_vid2_buf_get_width();
__u32 shared_vid1_buf_get_height();
__u32 shared_vid2_buf_get_height();
__u32 shared_vid1_buf_get_size();
__u32 shared_vid2_buf_get_size();

void shared_vid1_buf_get_scaler(__u32 *scaler, __u32 *n);
void shared_vid2_buf_get_scaler(__u32 *scaler, __u32 *n);

void *shared_vid1_buf_create(__u32 width, __u32 height, __u32 fmt, __u32 num, __u32 w_th);
void *shared_vid2_buf_create(__u32 width, __u32 height, __u32 fmt, __u32 num, __u32 w_th);

#endif /* SHARED_IMG_BUF_H_ */
