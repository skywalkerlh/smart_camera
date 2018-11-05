/*
 * video.c
 *
 *  Created on: 2015年4月24日
 *      Author: work
 */
//#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "video.h"

#include "debug.h"
#include "v4l2.h"

__s32 video_start_capture(struct Video *pVideo)
{
	__s32 ret = 0;

    ret = v4l2_streamon(pVideo->video_fd);
    if(ret < 0)
    	ERROR("v4l2_streamon");

	return ret;
}

__s32 video_stop_capture(struct Video *pVideo)
{
	__s32 ret = 0;

	ret = v4l2_streamoff(pVideo->video_fd);
    if(ret < 0)
    	ERROR("v4l2_streamoff");

	return ret;
}

struct image_buffer *video_request_img_buf(struct Video *pVideo)
{
	__u32 index = 0;
	struct v4l2_buffer v4l2_buf;

	memset(&v4l2_buf, 0, sizeof(v4l2_buf));
	v4l2_buf.memory = V4L2_MEMORY_DMABUF;
	v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_dqbuf(pVideo->video_fd, &v4l2_buf);

	index = v4l2_buf.index;
	memcpy(&(pVideo->img_buf[index].v4l2_buf), &v4l2_buf, sizeof(v4l2_buf));
	shared_memory_cpu_prep(pVideo->img_buf[index].frame);
	return &(pVideo->img_buf[index]);
}

void video_release_img_buf(struct Video *pVideo,  struct image_buffer *img_buf)
{
	struct v4l2_buffer *v4l2_buf;

	v4l2_buf = &(img_buf->v4l2_buf);
	shared_memory_cpu_fini(img_buf->frame);
	v4l2_qbuf(pVideo->video_fd, v4l2_buf);
}

struct Video * create_video_channel(__u8 *dev_name, __s32 video_id,
																								__u32 width, __u32 height, __u32 depth, __u32 bayer_fmt, __u32 buf_num)
{
	__s32 i = 0;
	__s32 ret = 0;
	struct Video *pVideo;

	pVideo = malloc(sizeof(struct Video));
	if(pVideo == NULL)
		ERROR("malloc");

	pVideo->video_id = video_id;
	pVideo->width = width;
	pVideo->height = height;
	pVideo->bayer_fmt = bayer_fmt;

	/* 打开视频设备 */
	pVideo->video_fd = v4l2_open(dev_name);
	if(pVideo->video_fd < 0)
		ERROR("v4l2_open");

	/* 设置图像尺寸 */
	ret = v4l2_setfmt(pVideo->video_fd, width >> 1, height);
	if(ret < 0)
		ERROR("v4l2_setfmt");

	/* 分配图像缓存 */
	pVideo->img_buf = v4l2_reqbufs(pVideo->video_fd, width >> 1, height, 16, buf_num);
	if(pVideo->img_buf == NULL)
		ERROR("v4l2_reqbufs");

	for(i = 0;i < buf_num;i++)
	{
		pVideo->img_buf[i].video_id = video_id;
		pVideo->img_buf[i].width = width;
		pVideo->img_buf[i].height = height;
		pVideo->img_buf[i].bayer_fmt = bayer_fmt;
	}

	return pVideo;
}
