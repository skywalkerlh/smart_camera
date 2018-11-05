/*
 * shared_img_buf.c
 *
 *  Created on: 2015年7月21日
 *      Author: work
 */

#include "shared_vid_buf.h"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
//#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"

extern int errno;

static __u32 g_vid1_buf_width = 0;
static __u32 g_vid1_buf_height = 0;
static __u32 g_vid2_buf_width = 0;
static __u32 g_vid2_buf_height = 0;

static __u32 g_vid1_buf_size = 0;
static __u32 g_vid2_buf_size = 0;

static __u32 g_vid1_buf_scaler = 1;
static __u32 g_vid2_buf_scaler = 1;

__u32 shared_vid1_buf_get_size()
{
	return g_vid1_buf_size;
}

__u32 shared_vid2_buf_get_size()
{
	return g_vid2_buf_size;
}

__u32 shared_vid1_buf_get_width()
{
	return g_vid1_buf_width;
}

__u32 shared_vid2_buf_get_width()
{
	return g_vid2_buf_width;
}

__u32 shared_vid1_buf_get_height()
{
	return g_vid1_buf_height;
}

__u32 shared_vid2_buf_get_height()
{
	return g_vid2_buf_height;
}

void shared_vid1_buf_get_scaler(__u32 *scaler, __u32 *n)
{
	__u32 tmp = 0;
	*scaler = g_vid1_buf_scaler;
	while(((*scaler) >> tmp) > 1)
		tmp++;
	*n = tmp;
}

void shared_vid2_buf_get_scaler(__u32 *scaler, __u32 *n)
{
	__u32 tmp = 0;
	*scaler = g_vid2_buf_scaler;
	while(((*scaler) >> tmp) > 1)
		tmp++;
	*n = tmp;
}


void *shared_vid1_buf_create(__u32 width, __u32 height, __u32 fmt, __u32 num, __u32 w_th)
{
	__s32 shm_id;
	void *ptr;
	__s32 err;
	while(width > w_th)
	{
		width = width >> 1;
		height = height >> 1;
		g_vid1_buf_scaler = g_vid1_buf_scaler << 1;
	}

	g_vid1_buf_width = width;
	g_vid1_buf_height = height;

	if(0 == fmt)
		g_vid1_buf_size = width * height * num;
	else
		g_vid1_buf_size = width * height * 3 * num;

    //检查共享内存是否存在，存在则先删除
	shm_id = shmget(IPC_KEY_A ,g_vid1_buf_size, 0640);
	if(shm_id != -1)
	{
		ptr = shmat(shm_id,NULL,0);
		if (ptr != (void *)-1)
		{
			err = shmdt(ptr);
			if(err == -1)
			{
				MSG("%s\n", __FUNCTION__);
				printf("errno=%d\n",errno);
			}

			err = shmctl(shm_id,IPC_RMID,0);
			if(err == -1)
			{
				MSG("%s\n", __FUNCTION__);
				printf("errno=%d\n",errno);
			}
		}
		else
		{
			MSG("%s\n", __FUNCTION__);
			printf("errno=%d\n",errno);
		}
	}
//	else
//	{
//		MSG("%s\n", __FUNCTION__);
//		printf("errno=%d\n",errno);
//	}


	shm_id = shmget(IPC_KEY_A, g_vid1_buf_size, IPC_CREAT | IPC_EXCL | 0640);
	if(shm_id == -1)
	{
		ERROR("shmget");
		printf("errno=%d\n",errno);
	}

	ptr = shmat(shm_id,NULL,0);

	return ptr;
}

void *shared_vid2_buf_create(__u32 width, __u32 height, __u32 fmt, __u32 num,  __u32 w_th)
{
	__s32 shm_id;
	void *ptr;
	__s32 err;

	while(width > w_th)
	{
		width = width >> 1;
		height = height >> 1;
		g_vid2_buf_scaler = g_vid2_buf_scaler << 1;
	}

	g_vid2_buf_width = width;
	g_vid2_buf_height = height;

	if(0 == fmt)
		g_vid2_buf_size = width * height * num;
	else
		g_vid2_buf_size = width * height * 3 * num;

    //检查共享内存是否存在，存在则先删除
	shm_id = shmget(IPC_KEY_B ,g_vid2_buf_size, 0640);
	if(shm_id != -1)
	{
		ptr = shmat(shm_id,NULL,0);
		if (ptr != (void *)-1)
		{
			err = shmdt(ptr);
			if(err == -1)
			{
				MSG("%s\n", __FUNCTION__);
				printf("errno=%d\n",errno);
			}
			err = shmctl(shm_id,IPC_RMID,0);
			if(err == -1)
			{
				MSG("%s\n", __FUNCTION__);
				printf("errno=%d\n",errno);
			}
		}
		else
		{
			MSG("%s\n", __FUNCTION__);
			printf("errno=%d\n",errno);
		}
	}
//	else
//	{
//		MSG("%s\n", __FUNCTION__);
//		printf("errno=%d\n",errno);
//	}

	shm_id = shmget(IPC_KEY_B, g_vid2_buf_size, IPC_CREAT | IPC_EXCL | 0640);
	if(shm_id == -1)
	{
		ERROR("shmget");
		printf("errno=%d\n",errno);
	}

	ptr = shmat(shm_id,NULL,0);

	return ptr;
}
