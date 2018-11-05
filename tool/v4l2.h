/*
 * v4l2.h
 *
 *  Created on: 2014年6月5日
 *      Author: work
 */

#ifndef V4L2_H_
#define V4L2_H_

#include <stddef.h>  //for NULL
#include <linux/videodev2.h>
#include <stdint.h>

#include "shared_mem_manager.h"

struct image_buffer
{
	__u32 r_count;	//应用程序对此图像的引用计数
	__s32 video_id;
	__u32 width;
	__u32 height;
	__u32 bayer_fmt;
	__u32  count;
	__u32  code;
	__u16  reserved[32];

	struct v4l2_buffer v4l2_buf;
	struct shared_memory *frame;
	pthread_mutex_t mutex;
};

int v4l2_open(char *);
void v4l2_close();

int v4l2_setfmt(__s32 video_fd, __u32 width, __u32 height);

struct image_buffer * v4l2_reqbufs(__s32 video_fd, __u32 width,__u32 height, __u32 depth,__u32 num);
void v4l2_release(__s32 video_fd, void *buf);

int v4l2_streamon(__s32 video_fd);
int v4l2_streamoff(__s32 video_fd);

int v4l2_qbuf(__s32 video_fd,struct v4l2_buffer *buf);
void v4l2_dqbuf(__s32 video_fd, struct v4l2_buffer *buf);



#endif /* V4L2_H_ */
