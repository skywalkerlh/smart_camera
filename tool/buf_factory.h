/*
 * buf_factory.h
 *
 *  Created on: 2015年5月8日
 *      Author: work
 */

#ifndef TOOLS_BUF_FACTORY_H_
#define TOOLS_BUF_FACTORY_H_

#include "list.h"
#include <linux/types.h>
#include <pthread.h>

struct Buffer
{
	void *memory;
	struct list_head *node;
};

struct buffer_factory
{
	struct Buffer *buf_array;
	struct list_head list;
	pthread_mutex_t list_mutex;
};

__s32 create_buf_factory(__u32 size, __u32 num);

struct Buffer *buf_factory_produce();
void buf_factory_recycle(__s32 arg, void *buf);

#endif /* TOOLS_BUF_FACTORY_H_ */
