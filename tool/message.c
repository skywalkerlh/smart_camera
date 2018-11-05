/*
 * message.c
 *
 *  Created on: 2015年4月23日
 *      Author: work
 */

#include <stddef.h>
#include "message.h"

 __s32 msg_set_data(void *__msg__, void *data, __u32 len, void (*release)(__s32 arg, void *data), __s32 arg)
{
	struct message *msg = __msg__;
	if(msg->payload_num == PAYLOAD_NUM_MAX)
		return -1;
	if(msg == NULL)
		return -1;

	msg->payload[msg->payload_num].data = data;
	msg->payload[msg->payload_num].data_len = len;
	msg->payload[msg->payload_num].release = release;
	msg->payload[msg->payload_num].arg = arg;
	msg->payload_num++;

	return 0;
}

void* msg_get_data(void *__msg__, __s32 *len)
{
	struct message *msg = __msg__;
	void *ptr = NULL;

	if(msg == NULL)
		return NULL;

	if(msg->cur_payload_idx == msg->payload_num)
		return NULL;

	ptr = msg->payload[msg->cur_payload_idx].data;
	if(NULL != len)
		*len = msg->payload[msg->cur_payload_idx].data_len;
	msg->cur_payload_idx++;

	return ptr;
}

__s32 msg_get_data_num(void *__msg__)
{
	struct message *msg = __msg__;

	if(msg == NULL)
		return 0;

	return msg->payload_num;
}

__s32 msg_set_queue(void *__msg__, __s32 id)
{
	struct message *msg = __msg__;

	if(msg == NULL)
		return -1;

	msg->id = id;

	return 0;
}

__s32 msg_set_prio(void *__msg__ , __s32 prio)
{
	struct message *msg = __msg__;

	if(msg == NULL)
		return -1;
	msg->prio = prio;

	return 0;
}

void msg_clear(void *__msg__)
{
	struct message *msg = __msg__;
	msg->id = 0;
	msg->cur_payload_idx = 0;
	msg->payload_num = 0;
	msg->prio = 0;
	msg->finish = NULL;
	msg->resource = NULL;
	msg->release = NULL;
}
