/*
 * message.h
 *
 *  Created on: 2015年4月23日
 *      Author: work
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <linux/types.h>
#include "list.h"
#define PAYLOAD_NUM_MAX		16

struct msg_payload
{
	void *data;
	__s32 data_len;

	void (*release)(int arg, void *data);
	__s32 arg;
};

struct message_operations
{
	__s32 (*set_data)(void *msg, void *data, __u32 len,
			void (*release)(__s32 arg, void *data), __s32 arg);
	void* (*get_data)(void *msg, __s32 *len);
	__s32 (*get_data_num)(void *msg);
	__s32 (*set_prio)(void *msg, __s32 prio);
	__s32 (*set_queue)(void *msg, __s32 id);
};

struct message
{
	__s32 id;			//消息队列id
	__s32 prio;		//消息优先级

	struct msg_payload *payload;
	__s32 (*finish)(void *msg);
	void *resource;
	void (*release)(void *data);
//	void (*release)(int arg, void *data);
	__s32 payload_num;
	__s32 cur_payload_idx;

	struct message_operations *ops;
	struct list_head *node;
};

__s32 msg_set_data(void *msg, void *data, __u32 len,
		void (*release)(__s32 arg, void *data), __s32 arg);
void* msg_get_data(void *msg, __s32 *len);
__s32 msg_get_data_num(void *msg);
void msg_clear(void *msg);
__s32 msg_set_queue(void *msg, __s32 id);
__s32 msg_set_prio(void *msg, __s32 id);

#endif /* MESSAGE_H_ */
