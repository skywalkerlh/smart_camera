/*
 * message_factory.c
 *
 *  Created on: 2015年4月23日
 *      Author: work
 */

//#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "message.h"
#include "msg_factory.h"
#include "debug.h"

static struct message_factory *g_msg_factory;

struct message_operations g_msg_ops =
{
		.set_data = &msg_set_data,
		.get_data = &msg_get_data,
		.get_data_num = &msg_get_data_num,
		.set_queue = &msg_set_queue,
		.set_prio = &msg_set_prio
};

__s32 create_msg_factory()
{
	__s32 i = 0;
	__s32 p = 0;

	g_msg_factory =  malloc(sizeof(struct message_factory));
	if(g_msg_factory == NULL)
	{
		ERROR("malloc");
	}

	init_list_head(&g_msg_factory->list);
	pthread_mutex_init (&g_msg_factory->list_mutex, NULL);

	g_msg_factory->msg_array = calloc(MSG_NUM_MAX, sizeof(struct message));
	if(g_msg_factory->msg_array == NULL)
	{
		free(g_msg_factory);
		ERROR("calloc");
	}

	for(i=0;i<MSG_NUM_MAX;i++)
	{
		__s32  j = 0;

		g_msg_factory->msg_array[i].payload = calloc(PAYLOAD_NUM_MAX, sizeof(struct msg_payload));
		if(g_msg_factory->msg_array[i].payload == NULL)
		{
			for(j=i-1; j>=0; j--)
				free(g_msg_factory->msg_array[j].payload);
			free(g_msg_factory->msg_array);
			free(g_msg_factory);
			ERROR("calloc");
		}

		g_msg_factory->msg_array[i].node = malloc(sizeof(struct list_head));
		if(g_msg_factory->msg_array[i].node == NULL)
		{
			for(j=i-1; j>=0; j--)
				free(g_msg_factory->msg_array[j].node);

			for(p=0;p<PAYLOAD_NUM_MAX;p++)
				free(g_msg_factory->msg_array[i].payload);

			free(g_msg_factory->msg_array);
			free(g_msg_factory);
			ERROR("malloc");
		}
		g_msg_factory->msg_array[i].node->owner = &g_msg_factory->msg_array[i];
		g_msg_factory->msg_array[i].ops = &g_msg_ops;

		list_add_tail(g_msg_factory->msg_array[i].node, &g_msg_factory->list);
	}
	return 0;
}

struct message *msg_factory_produce(__s32 id, __s32 prio)
{
	__s32 i =0;
	struct list_head *node_of_msg;
	struct message *msg;

	pthread_mutex_lock(&g_msg_factory->list_mutex);

	if(list_empty(&g_msg_factory->list))
	{
		MSG("msg factory crash !");
		pthread_mutex_unlock(&g_msg_factory->list_mutex);
		return NULL;
	}
	node_of_msg = g_msg_factory->list.next;
	msg = (struct message *)node_of_msg->owner;
	msg->id = id;
	msg->prio = prio;
	msg->finish = NULL;
	msg->resource = NULL;
	msg->release = NULL;

	for(i=0;i<PAYLOAD_NUM_MAX;i++)
		msg->payload[i].release = NULL;

	list_del(node_of_msg);

	pthread_mutex_unlock(&g_msg_factory->list_mutex);

	return msg;
}

void msg_factory_recycle(struct message *__msg__)
{
	__s32 i = 0;
	struct message *msg;

	pthread_mutex_lock(&g_msg_factory->list_mutex);
	msg = __msg__->node->owner;
	for(i=0;i<PAYLOAD_NUM_MAX;i++)
		if(msg->payload[i].release)
			msg->payload[i].release(msg->payload[i].arg, msg->payload[i].data);

	if(msg->resource && msg->release)
		msg->release(msg->resource);

	if(msg->finish)
	{
		msg->finish(msg);
	}

	msg_clear(msg);

	list_add_tail(__msg__->node, &g_msg_factory->list);

	pthread_mutex_unlock(&g_msg_factory->list_mutex);
}

void msg_factory_cast(struct message *msg, __s32 id)
{
	msg->id = id;
	msg->prio = 0;
}
