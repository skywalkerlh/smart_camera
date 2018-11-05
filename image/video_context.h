/*
 * video_context.h
 *
 *  Created on: 2015年5月22日
 *      Author: work
 */

#ifndef VIDEO_CONTEXT_H_
#define VIDEO_CONTEXT_H_

#define VID1_BUF_MAX	 17

#define VID2_BUF_MAX	 7



void video_context_init();
void video_context_release(void *resource);
void sample_count_reset();
struct image_buffer* get_overclock_img_buf();
#endif /* VIDEO_CONTEXT_H_ */
