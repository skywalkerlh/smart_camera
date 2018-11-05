/*
 * local_socket.c
 *
 *  Created on: 2015年8月10日
 *      Author: work
 */

#include <signal.h>
#include <linux/types.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "thread.h"
#include "debug.h"
#include "buf_factory.h"
#include "msg_factory.h"
#include "mailbox.h"
#include "local_socket.h"
#include "local_protocol.h"

#define UNIX_DOMAIN "/tmp/UNIX.domain"


__s32 listen_fd;
__s32 connect_fd;
struct sockaddr_un clt_addr;
struct sockaddr_un srv_addr;

sem_t g_sem_connect;
sem_t gsem_reject_reset;
__u32 g_local_mbx_id = 0;
pthread_mutex_t g_mutex_array;
__s32 connect_status = 0;



Clt_CMD_STR CltCmd_str;

void local_connect_tsk(void)
{
	__s32 ret;
	__u32 len;

	listen_fd=socket(PF_UNIX,SOCK_STREAM,0);
	if(listen_fd < 0)
	{
		MSG("creat local socket");
	 }

	//set server addr_param
	srv_addr.sun_family=AF_UNIX;
	strncpy(srv_addr.sun_path,UNIX_DOMAIN,sizeof(srv_addr.sun_path)-1);
	unlink(UNIX_DOMAIN);

	//bind sockfd & addr
	ret = bind(listen_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
	if(ret == -1)
	{
		MSG("local socket bind");
		close(listen_fd);
		unlink(UNIX_DOMAIN);
	}
	//listen sockfd
	ret = listen(listen_fd, 5);
	if(ret==-1)
	 {
		MSG("local socket listen");
		close(listen_fd);
		unlink(UNIX_DOMAIN);
	}
	//have connect request use accept
	len = sizeof(clt_addr);

	while(1)
	{
		sleep(1);
		if(connect_status)continue;

		connect_fd = accept(listen_fd,(struct sockaddr*)&clt_addr,&len);
		if(connect_fd < 0)
		 {
			MSG("local socket accept");
			close(connect_fd);
		 }
		else
		{
			connect_status = 1;
			sem_post(&g_sem_connect);
			send_handshake_cmd();//握手并查版本
		}
	}
}

void local_send_tsk(void)
{
	struct message msg;
	void *data = NULL;
	__u32 data_len = 0;
	__u32 data_num = 0;
	__u32 nhas_send = 0;
	__u32 i;
	__s32 nsend;

	msg_factory_cast(&msg, g_local_mbx_id);
	while(1)
	{
		mailbox_pend(&msg);
		if (connect_status == 0)
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
				nsend = write(connect_fd, data, data_len - nhas_send);
				if (nsend <= 0)
				{
//					break;
					goto LOCAL_OUT;
				}
				nhas_send = nhas_send + nsend;
			}
		}
	LOCAL_OUT:
		msg_factory_recycle(&msg);
	}
}

void local_recv_tsk(void)
{
	__s32 nrecv;
	__u32 nhas_recv;
	struct Buffer *buf;

	revReconnect:
		sem_wait(&g_sem_connect);
		while(1)
		{
			//接受长度与命令
			nhas_recv = 0;
			while (nhas_recv < 8)
			{
				nrecv = read(connect_fd, &CltCmd_str+nhas_recv, 8-nhas_recv);
				if(nrecv <= 0)
				{
					shutdown(connect_fd, SHUT_RDWR);
					close(connect_fd);
					connect_status = 0;
					goto revReconnect;
				}
				nhas_recv += nrecv;
			}

			//接受协议内容
			buf = buf_factory_produce();
			CltCmd_str.param = buf->memory;
			nhas_recv = 0;
			CltCmd_str.length -= 4;
			while (nhas_recv < CltCmd_str.length)
			{
				nrecv = read(connect_fd, CltCmd_str.param+nhas_recv, CltCmd_str.length-nhas_recv);
				if(nrecv <= 0)
				{
					shutdown(connect_fd, SHUT_RDWR);
					close(connect_fd);
					connect_status = 0;
					buf_factory_recycle(0, buf);
					goto revReconnect;
				}
				nhas_recv += nrecv;
			}
			local_cmd_analyze(&CltCmd_str);
			buf_factory_recycle(0, buf);
		}
}

void local_socket_init(void)
{
	g_local_mbx_id = mailbox_create("/local");
	pthread_mutex_init(&g_mutex_array, NULL);
	sem_init(&gsem_reject_reset, 0, 0);
	sem_init(&g_sem_connect, 0, 0);
	add_new_thread(NULL, (void *)&local_connect_tsk, 21, 0, 8*1024);
	add_new_thread(NULL, (void *)&local_send_tsk, 22, 0, 8*1024);
	add_new_thread(NULL, (void *)&local_recv_tsk, 22, 0, 16*1024);
}

