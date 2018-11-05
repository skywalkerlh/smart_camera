/*
 * net.h
 *
 *  Created on: 2015年7月28日
 *      Author: work
 */

#ifndef NET_H_
#define NET_H_

#include "net_context.h"

#ifndef NET_C_

/* 网络连接状态 */
extern __u32 g_conn_state;
extern pthread_mutex_t g_mutex_conn_state;

/* 图像发送能力 */
extern __u32 g_img_ability;
extern pthread_mutex_t g_mutex_img_ability;

/* 样品发送能力 */
extern __u32 g_sample_ability;
extern pthread_mutex_t g_mutex_sample_ability;

extern __u32 g_net_mbx_id;

extern void net_context_init();

#endif

#endif /* NET_H_ */
