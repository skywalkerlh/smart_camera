/*
 * tcp.c
 *
 *  Created on: 2015年7月27日
 *      Author: work
 */

#include <sys/socket.h>

#include <arpa/inet.h>
#include <linux/tcp.h>
#include <linux/if.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "sys_log.h"
#include "debug.h"
#include "net_context.h"
#include "net.h"

__s32 g_tcp_client_fd;


__s32 tcp_client_create_socket()
{
	struct timeval tv_out;

	/* 创建客户端socket */
	g_tcp_client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (g_tcp_client_fd < 0)
	{
		ERROR("socket");
	}

	/* 设置socket发送超时, 以便在网络状况不好时及时将资源释放 */
	tv_out.tv_sec = 1;
	tv_out.tv_usec = 0;
	setsockopt(g_tcp_client_fd, SOL_SOCKET, SO_SNDTIMEO, &tv_out, sizeof(tv_out));

	return 0;
}

__s32 client_connect_server(__s8 *ip_addr)
{

	struct sockaddr_in server_addr;
	__s32 err = 0;

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip_addr);
	server_addr.sin_port = htons(atoi("5000"));

	/* 向服务器发起连接 */
	err = connect(g_tcp_client_fd, (struct sockaddr*) &server_addr,
			sizeof(server_addr));

	return err;
}

__s32 client_break_socket()
{
	shutdown(g_tcp_client_fd, SHUT_RDWR);
	close(g_tcp_client_fd);
	pthread_mutex_lock(&g_mutex_conn_state);
	g_conn_state = LINK_OFF;
	pthread_mutex_unlock(&g_mutex_conn_state);
	MSG("socket is break.\n");
	return 0;
}

__s32 check_link_status()
{

	struct tcp_info info;
	struct ifreq ifr;
	int length;

	/* 检测网线连接状态 */
	strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name) - 1);
	if (ioctl(g_tcp_client_fd, SIOCGIFFLAGS, &ifr) < 0)
	{
		MSG("failed to get link state ! \n");
		log_builder("tcp link break.");
		return -1;
	}
	if (!(ifr.ifr_flags & IFF_RUNNING))
	{
		MSG("Link is Down.\n");
		log_builder("net line pull out.");
		client_break_socket();
		return -2;
	}
	return 0;
}

__s32 client_recv_data(void *buf, __u32 len)
{
	__u32 nHasRecv = 0;
	__s32 nRecv = 0;

	while (nHasRecv < len)
	{
		nRecv = recv(g_tcp_client_fd, buf + nHasRecv, len - nHasRecv, 0);
		if(nRecv <= 0)
		{
			return nRecv;
		}
		nHasRecv += nRecv;
	}
	return nHasRecv;
}

__s32 client_send_data(void *buf, __u32 len)
{
	__s32 nSent = 0;

	nSent = send(g_tcp_client_fd,buf, len, 0);

	return nSent;
}
