/*
 * v4l2.c
 *
 *  Created on: 2014年6月5日
 *      Author: work
 */

#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "v4l2.h"

#include "debug.h"

int v4l2_open(char * devname)
{
	struct v4l2_capability caps;
	__s32 video_fd = -1;
	video_fd = open(devname, O_RDWR);
	if(video_fd < 0)
	{
		printf("v4l2_open failed: %s\n", strerror(errno));
		video_fd = -1;
		goto err;
	}

	/* Check for capture device */
	memset(&caps, 0, sizeof(caps));

	if (-1 == ioctl(video_fd, VIDIOC_QUERYCAP, &caps))
	{
		perror("Setting Pixel Format");
		video_fd = -1;
		goto err;
	}

	if (~caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)
	{
		printf("Not a capture device");
		video_fd = -1;
		goto err;
	}
err:
	return video_fd;
}

int v4l2_setfmt(__s32 video_fd, __u32 width, __u32 height)
{
	__s32 ret = 0;
	struct v4l2_format fmt;
	/* Set capture format to BAYER */
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR16;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;

	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	ret = ioctl(video_fd, VIDIOC_S_FMT, &fmt);

	if (ret < 0)
	{
		printf("VIDIOC_S_FMT failed: %s (%d)\n", strerror(errno), ret);
		ret = -1;
	}
	return ret;
}

struct image_buffer * v4l2_reqbufs(__s32 video_fd, __u32 width, __u32 height, __u32 depth, __u32 num)
{
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	struct image_buffer *buffers = NULL;
	__u32 i;

	memset(&req, 0, sizeof(req));
	req.count = num;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_DMABUF;

	if (-1 == ioctl(video_fd, VIDIOC_REQBUFS, &req))
	{
		perror("Requesting Buffer");
		return NULL;
	}

	buffers = calloc(req.count, sizeof(*buffers));
	if (buffers == NULL)
	{
		ERROR("calloc");
	}

	for (i = 0; i < req.count; i++)
	{
		buffers[i].frame = shared_memory_alloc(width * height* (depth >> 3 ));
		pthread_mutex_init(&(buffers[i].mutex),NULL);
	}

	for (i = 0; i < req.count; i++)
	{
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_DMABUF;
		buf.index = i;
		if (-1 == ioctl(video_fd, VIDIOC_QUERYBUF, &buf))
		{
			perror("Querying Buffer");
			free(buffers);
			return NULL;
		}

		/* Queue the buffer for capture */
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_DMABUF;
		buf.index = i;
		buf.m.fd = buffers[i].frame->fd;
		if (-1 == ioctl(video_fd, VIDIOC_QBUF, &buf))
		{
			perror("Queue Buffer");
			free(buffers);
			return NULL;
		}
	}

	return buffers;
}

int v4l2_streamon(__s32 video_fd)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	__s32 ret = 0;

	ret = ioctl(video_fd, VIDIOC_STREAMON, &type);
    if (ret)
		printf("VIDIOC_STREAMON failed: %s (%d)", strerror(errno), ret);

	return ret;
}

int v4l2_streamoff(__s32 video_fd)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	__s32 ret;

	ret = ioctl(video_fd, VIDIOC_STREAMOFF, &type);
	if (ret)
		printf("VIDIOC_STREAMOFF failed: %s (%d)", strerror(errno), ret);

	return ret;
}

int v4l2_qbuf(__s32 video_fd, struct v4l2_buffer *buf)
{
	__s32 ret;

	ret = ioctl(video_fd, VIDIOC_QBUF, buf);
	if (ret)
		printf("VIDIOC_QBUF failed: %s (%d)", strerror(errno), ret);

	return ret;
}

void v4l2_dqbuf(int video_fd, struct v4l2_buffer *buf)
{
	int ret;

	ret = ioctl(video_fd, VIDIOC_DQBUF, buf);
	if (ret)
		printf("VIDIOC_DQBUF failed: %s (%d)", strerror(errno), ret);
}

void v4l2_close(int video_fd)
{
	close(video_fd);
}

void v4l2_release(int video_fd, void *buf)
{
}
