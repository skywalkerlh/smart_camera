/*
 * ui_socket.c
 *
 *  Created on: 2015年9月10日
 *      Author: work
 */
#include <semaphore.h>
#include <unistd.h>
#include <linux/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>	//for inet_addrinet_addr
#include <sys/msg.h>
#include <sys/socket.h>
#include <errno.h>
#include <linux/if.h>
#include <linux/tcp.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include "thread.h"
#include "mailbox.h"
#include "msg_factory.h"
#include "debug.h"
#include "udisk.h"
#include "buf_factory.h"
#include "ui_protocol.h"

static __s32 g_ui_socket_fd = -1;
__s32 g_ui_socket_state = 0;	// 连接状态：0断开，1连接
__s32 g_ui_mbx_id = -1;
static sem_t gsem_ui_conn_state;
//extern struct UDISK g_udisk[2];

void ui_connect_tsk(void)
{
	struct timeval tv_out = {1,0};
	struct sockaddr_in server_addr;
	__s32 lenght;
	struct UDISK *p_udisk;

	while (1)
	{
		usleep(1000);
		if(g_ui_socket_state)continue;

		bzero(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		server_addr.sin_port = htons(atoi("9000"));

		g_ui_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (g_ui_socket_fd < 0)
		{
			perror("socket");
			exit(1);
		}

		setsockopt(g_ui_socket_fd, SOL_SOCKET,SO_SNDTIMEO, &tv_out, sizeof(tv_out));

		//connect server
		while(connect(g_ui_socket_fd, (struct sockaddr*) &server_addr,sizeof(server_addr)))
		{
			usleep(1000);
		}

		g_ui_socket_state = 1;
		sem_post(&gsem_ui_conn_state);


		//连接之后发送U盘状态
//		p_udisk = &g_udisk[0];
//		if(p_udisk->state == 0)
//		{
//			send_ui_udisk_info(p_udisk);
//		}
//
//		p_udisk = &g_udisk[1];
//		if(p_udisk->state == 0)
//		{
//			send_ui_udisk_info(p_udisk);
//		}

		//连接之后发送图像通道需求
		send_ui_image_require_info();
	}
}



static __u8 recv_buf[65535];
void ui_recv_tsk()
{
	__u32 nHasRecv = 0;
	__s32 nrecv = 0;
	__u32 length = 0;
	__s32 nRecv = 0;
	__u32 lvds_id = 0;

RECONNECT:
	sem_wait(&gsem_ui_conn_state);
	while(1)
	{
		/* 接收长度和帧头 */
		nRecv = recv(g_ui_socket_fd, &length, 4, 0);
		if(nRecv <= 0)
		{
			shutdown(g_ui_socket_fd, SHUT_RDWR);
			close(g_ui_socket_fd);
			g_ui_socket_state = 0;
			goto RECONNECT;
		}
		nHasRecv = 0;

		while (nHasRecv < length)
		{
			nRecv = recv(g_ui_socket_fd, recv_buf + nHasRecv, length - nHasRecv, 0);
			if(nRecv <= 0)
			{
				shutdown(g_ui_socket_fd, SHUT_RDWR);
				close(g_ui_socket_fd);
				g_ui_socket_state = 0;
				goto RECONNECT;
			}
			nHasRecv += nRecv;
		}
		ui_protocol_analyze((void*)recv_buf, length);
	}
}

void ui_send_tsk()
{
	__u32 data_num = 0;
	__u32 data_len = 0;
	__u32 i = 0;
	__u32 nhas_send = 0;
	__s32 nsend;
	__u32 link_state = 0;

	struct message msg;
	void *data = NULL;


	msg_factory_cast(&msg, g_ui_mbx_id);
	while (1)
	{
		mailbox_pend(&msg);
		if (g_ui_socket_state == 0)
		{
			msg_factory_recycle(&msg);
//			MSG("%s\n", "net disconnect.");
//			while( mailbox_timedpend(&msg) == 0 )
//				msg_factory_recycle(&msg);
			continue;
		}
		data_num = msg.ops->get_data_num(&msg);

		for (i = 0; i < data_num; i++)
		{
			data = msg.ops->get_data(&msg, &data_len);
			nhas_send = 0;
			while (nhas_send < data_len)
			{
				nsend = send(g_ui_socket_fd, data, data_len - nhas_send, 0);
				if (nsend <= 0)
				{
					goto UI_OUT;
				}
				nhas_send = nhas_send + nsend;
			}
		}
		UI_OUT:
		msg_factory_recycle(&msg);
	}
}


void ui_context_init()
{
	g_ui_mbx_id = mailbox_create("/ui_client");

	sem_init(&gsem_ui_conn_state, 0, 0);

	add_new_thread(NULL, (void *)&ui_connect_tsk, 14, 0, 8*1024);
	add_new_thread(NULL, (void *)&ui_send_tsk, 14, 0, 8*1024);
	add_new_thread(NULL, (void *)&ui_recv_tsk, 14, 0, 32*1024);
}
