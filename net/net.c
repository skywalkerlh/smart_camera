/*
 * net.c
 *
 *  Created on: 2015年7月27日
 *      Author: work
 */

#define NET_C_

#include <netinet/in.h>
#include <pthread.h>
#include <linux/types.h>
#include <stddef.h>
#include <semaphore.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include "buf_factory.h"
#include "msg_factory.h"
#include "message.h"
#include "sys_conf.h"
#include "debug.h"
#include "sample_factory.h"
#include "mailbox.h"
#include "udp.h"
#include "tcp.h"
#include "thread.h"
#include "net_protocol.h"
#include "lvds_context.h"
#include "sys_log.h"
#include "mailbox.h"

/* 网络连接状态 */
__u32 g_conn_state = LINK_OFF;
pthread_mutex_t g_mutex_conn_state;

/* 图像发送能力 */
__u32 g_img_ability = YES;
pthread_mutex_t g_mutex_img_ability;

/* 样品发送能力 */
__u32 g_sample_ability = YES;
pthread_mutex_t g_mutex_sample_ability;

sem_t g_sem_conn_state;
__u32 g_net_mbx_id = 0;



#define handle_receive_error(x) \
			if (x <= 0) \
			{ \
      client_break_socket();\
				goto RECONNECT; \
			}


void net_send_tsk()
{
	__u32 data_num = 0;
	__u32 data_len = 0;
	__u32 i = 0;
	__u32 nhas_send = 0;
	__s32 nsend;
	__u32 link_state = 0;
	__u32 net_send_num = 0;

	struct message msg;
	void *data = NULL;

	msg_factory_cast(&msg, g_net_mbx_id);

	while (1)
	{
		mailbox_pend(&msg);

		pthread_mutex_lock(&g_mutex_conn_state);
		link_state = g_conn_state;
		pthread_mutex_unlock(&g_mutex_conn_state);
		if (link_state == LINK_OFF)
		{
			msg_factory_recycle(&msg);
			continue;
		}

		data_num = msg.ops->get_data_num(&msg);

		for (i = 0; i < data_num; i++)
		{
			data = msg.ops->get_data(&msg, &data_len);
			nhas_send = 0;
			while (nhas_send < data_len)
			{
				nsend = client_send_data(data, data_len - nhas_send);
				if (nsend <= 0)
				{
					MSG("goto OUT\n");
					goto OUT;
				}
				nhas_send = nhas_send + nsend;
			}
		}
	OUT:
	msg_factory_recycle(&msg);
	}
}

void net_connect_tsk()
{
	__s32 ret = 0;
	__s32 link_status = 0;
	struct sockaddr_in udp_addr;
	char pc_ip_addr[100];

	udp_socket_create();

	/* 检测DHCP是否成功 */
	while (1)
	{
		ret = check_dhcp_state();
		if (ret != 0)
			sleep(1);
		else
			break;
	}

	/* 设置广播地址 */
	bzero(&udp_addr, sizeof(struct sockaddr_in));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	udp_addr.sin_port = htons(6000);

	while (1)
	{
		/* 与PC握手,获取服务器IP */
		while (1)
		{
			ret = shakehand_with_pc(pc_ip_addr, &udp_addr);
			if (ret != 0)
				sleep(1);
			else
			{
				log_builder("IP apply succeed.");
				break;
			}
		}

		/* 创建客户端socket, 并向服务器发起连接 */
		tcp_client_create_socket();
		while (1)
		{
			ret = client_connect_server(pc_ip_addr);
			if (ret != 0)
			{
				usleep(1000);
			}
			else
			{
				pthread_mutex_lock(&g_mutex_conn_state);
				g_conn_state = LINK_ON;
				pthread_mutex_unlock(&g_mutex_conn_state);

				sem_post(&g_sem_conn_state);
				log_builder("tcp link succeed.");
				break;
			}
		}

		/* 创建客户端socket, 并向服务器发起连接 */
		while (1)
		{
			if((check_link_status() == 0) && (g_conn_state == LINK_ON))
				sleep(1);
			else
			{
				break;
			}
		}
	}
}

static __u8 recv_buf[5308436]; // 2592*2048+20 //用于主板协议接收

void net_recv_tsk()
{
	struct NetInfoHead info_head;
	__u32 nhas_received = 0;
	__s32 nrecv = 0;
	__u32 length = 0;
	__u32 lvds_id = 0;
	__u32 sort = 0; /* 协议类别 */
	__u32 index = 0; /* 协议索引 */
	__u32 val;

	__s32 lvds1_context_mailbox_id = get_lvds_context_mailbox(0);
	__s32 lvds2_context_mailbox_id = get_lvds_context_mailbox(1);

RECONNECT:
	sem_wait(&g_sem_conn_state);

	while(1)
	{
		/* 接收长度和帧头 */
		nhas_received = 0;
		while (nhas_received < sizeof(info_head))
		{
			nrecv = client_recv_data(&info_head, sizeof(info_head) - nhas_received);
			handle_receive_error(nrecv);
			nhas_received += nrecv;
		}

		sort = info_head.type & (~0xFF);
		index = info_head.type & 0xFF;
		info_head.length -=4;

		/* 接收主板数据 */
		nhas_received = 0;
		if((sort == MAINBOARD_SET_CMD)||(sort == MAINBOARD_GET_CMD))
		{
			while (nhas_received < info_head.length)
			{
				nrecv = client_recv_data(recv_buf, info_head.length - nhas_received);
				handle_receive_error(nrecv);
				nhas_received += nrecv;
			}

			/* 设置类命令 */
			if(sort == MAINBOARD_SET_CMD)
			{
				val = mainboard_set_func[index](recv_buf, info_head.length);
				send_feedback_info(val);
			}
			/* 查询类命令 */
			else
				mainboard_get_func[index](recv_buf, info_head.length);
		}

		/* 接收相机头数据 */
		else if ((sort == CAMERA_SET_CMD)||(sort == CAMERA_GET_CMD))
		{
			struct message *msg2camera;
			struct Buffer *buf = buf_factory_produce();
			__s32 lvds_id = 0;

			memcpy(buf->memory, &info_head.type, 4);//用命令字占据头四个字节
			nhas_received = 0;
			while (nhas_received < info_head.length)
			{
				nrecv = client_recv_data((char *)buf->memory + 4 + nhas_received, info_head.length - nhas_received);
				if(nrecv <= 0)
					buf_factory_recycle(0,buf);
				handle_receive_error(nrecv);
				nhas_received += nrecv;
			}

			memcpy(&lvds_id, (char *) buf->memory + 4, 4);//将ID提取出来
			memcpy((char *) buf->memory + 4, &info_head.length, 4);//用长度替换ID位置
			if (lvds_id == 0)
			{
				msg2camera = msg_factory_produce(lvds1_context_mailbox_id, 0);
			}
			else if (lvds_id == 1)
			{
				msg2camera = msg_factory_produce(lvds2_context_mailbox_id, 0);
			}
			else
			{
				buf_factory_recycle(0,buf);
				continue;
			}
		 msg2camera->ops->set_data(msg2camera, buf, 0, NULL, 0);
		 mailbox_post(msg2camera);
		}
	}
}


void net_context_init()
{
	pthread_mutex_init(&g_mutex_conn_state, NULL);
	pthread_mutex_init(&g_mutex_img_ability, NULL);
	pthread_mutex_init(&g_mutex_sample_ability, NULL);
	sem_init(&g_sem_conn_state, 0, 0);

	g_net_mbx_id = mailbox_create("/net");

	add_new_thread(NULL, (void *)&net_connect_tsk, 16, 0, 8*1024);
	add_new_thread(NULL, (void *)&net_send_tsk, 17, 1, 8*1024);
	add_new_thread(NULL, (void *)&net_recv_tsk, 17, 0, 32*1024);
}
