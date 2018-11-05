/*
 * video.h
 *
 *  Created on: 2015年4月24日
 *      Author: work
 */

#ifndef VIDEO_H_
#define VIDEO_H_

#include <linux/videodev2.h>
#include "shared_mem_manager.h"



struct Video
{
	int video_fd;
	char *name;
	int video_id;
	int width;
	int height;
	int bayer_fmt;
	struct image_buffer *img_buf;
};


__s32 video_start_capture(struct Video *pVideo);
__s32 video_stop_capture(struct Video *pVideo);
struct image_buffer *video_request_img_buf(struct Video *pVideo);
void video_release_img_buf(struct Video *pVideo,  struct image_buffer *img_buf);
struct Video * create_video_channel(__u8 *dev_name, __s32 video_id,
																								__u32 width, __u32 height, __u32 depth, __u32 bayer_fmt, __u32 buf_num);

#endif /* VIDEO_H_ */
