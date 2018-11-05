/*
 * message_factory.h
 *
 *  Created on: 2015年4月23日
 *      Author: work
 */

#ifndef MSG_FACTORY_H_
#define MSG_FACTORY_H_

#include <pthread.h>

#include "list.h"
#include "message.h"

#define MSG_NUM_MAX 128

struct message_factory
{
	struct message *msg_array;
	struct list_head list;
	pthread_mutex_t list_mutex;
};

/*创办消息工场*/
int create_msg_factory();

/*
 * 消息工厂生产一条消息
 * id：消息对应的对列
 * */
struct message *msg_factory_produce(__s32 id, __s32 prio);

/*
 * 消息工厂回收一条消息
 * msg：待回收的消息
 * */
void msg_factory_recycle(struct message *msg);

/*
 * 非本工厂生产的消息强制按本工厂的产品格式进行包装
 * msg：待转换的消息
 * id：消息对应的对列
 * */
void msg_factory_cast(struct message *msg, int id);

#endif /* MSG_FACTORY_H_ */
