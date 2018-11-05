/*
 * buf_factory.c
 *
 *  Created on: 2015年5月8日
 *      Author: work
 */
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "debug.h"
#include "buf_factory.h"

static struct buffer_factory *g_buf_factory;

__s32 create_buf_factory(__u32 num, __u32 size)
{
	__s32 i = 0;
	__s32 j = 0;

	g_buf_factory  = malloc(sizeof( struct buffer_factory));
	if(g_buf_factory == NULL)
		ERROR("malloc");

	init_list_head(&g_buf_factory->list);
	pthread_mutex_init (&g_buf_factory->list_mutex, NULL);

	g_buf_factory->buf_array = calloc(num, sizeof(struct Buffer));
	if(g_buf_factory->buf_array == NULL)
	{
		free(g_buf_factory);
		ERROR("calloc");
	}

	for(i=0;i<num;i++)
	{
		g_buf_factory->buf_array[i].memory = malloc(size);
		if(g_buf_factory->buf_array[i].memory== NULL)
		{
			for(j=i-1; j>=0; j--)
				free(g_buf_factory->buf_array[i].memory);
			free(g_buf_factory->buf_array);
			free(g_buf_factory);
			ERROR("malloc");
		}
		g_buf_factory->buf_array[i].node = malloc(sizeof(struct list_head));
		if(g_buf_factory->buf_array[i].node == NULL)
		{
			for(j=i-1; j>=0; j--)
				free(g_buf_factory->buf_array[i].node);

			for(j=i; j>=0; j--)
				free(g_buf_factory->buf_array[i].memory);

			free(g_buf_factory->buf_array);
			free(g_buf_factory);
			ERROR("malloc");
		}
		g_buf_factory->buf_array[i].node->owner = &g_buf_factory->buf_array[i];
		list_add_tail(g_buf_factory->buf_array[i].node, &g_buf_factory->list);
	}

	return 0;
}

struct Buffer *buf_factory_produce()
{
	struct list_head *node_of_buf;
	struct Buffer *buf;

	pthread_mutex_lock(&g_buf_factory->list_mutex);

	if(list_empty(&g_buf_factory->list))
		ERROR("buf factory crash !");

	node_of_buf = g_buf_factory->list.next;
	buf = (struct Buffer *)(node_of_buf->owner);
	list_del(node_of_buf);

	pthread_mutex_unlock(&g_buf_factory->list_mutex);

	return buf;
}

void buf_factory_recycle(__s32 arg, void  *__buf__)
{
	struct Buffer *buf;

	buf = (struct Buffer*)__buf__;

	pthread_mutex_lock(&g_buf_factory->list_mutex);
	list_add_tail(buf->node, &g_buf_factory->list);
	pthread_mutex_unlock(&g_buf_factory->list_mutex);
}
