/*
 * udp.c
 *
 *  Created on: 2015年7月27日
 *      Author: work
 */


#include <arpa/inet.h>
#include <linux/if.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "debug.h"

__s32 g_udp_fd;


__s32 udp_socket_create()
{
	__s32 opt = 1;
	struct timeval tv_out;

	if ((g_udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
		ERROR("socket");
    }

    //设置该套接字为广播类型，
	if(setsockopt(g_udp_fd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)) == -1)
    {
		printf("setsockopt error!\n");
    }

	// 设置超时
	tv_out.tv_sec = 1;
	tv_out.tv_usec = 0;
	setsockopt(g_udp_fd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));

	return 0;
}

__s32 check_dhcp_state()
{
	struct   sockaddr_in *sin;
	struct   ifreq ifr_ip;
	__s32 		state = -1;

	memset(&ifr_ip, 0, sizeof(ifr_ip));
	strncpy(ifr_ip.ifr_name, "eth0", sizeof(ifr_ip.ifr_name) - 1);
	if( ioctl( g_udp_fd, SIOCGIFADDR, &ifr_ip) < 0 )
	{
		ERROR("Get local IP error!");
	}

	sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;

	MSG("local ip is %s. \n",inet_ntoa(sin->sin_addr));


	/* 若本机IP不是169.xxx.xxx.xxx, 认为申请IP成功 */
	if((sin->sin_addr.s_addr & 0xFF) != 0xA9 )
	{
		state = 0;
	}

	return state;
}

__s32  shakehand_with_pc(char *ip_addr, struct sockaddr_in *pc)
{
	__s32 ret = 0;
	static __s32 once = 0;
	char smsgto[] = {"askip"};

	//从广播地址发送消息
	ret = sendto(g_udp_fd, smsgto, strlen(smsgto)+1, 0, (struct sockaddr*)pc, sizeof(struct sockaddr_in));
	if( ret < 0)
	{
		perror("sendto");
		ret = -1;
	}

	//接受消息
	ret = recvfrom(g_udp_fd, ip_addr, 100, 0, NULL, NULL);
	if (ret < 0)
	{
		if (errno == EAGAIN)
		{
			if(!once)
			{
				once = 1;
				MSG("server isn't on line !\n");
			}
		}
		else
			perror("recvfrom");

		ret = -1;
	}
	else
	{
		MSG("connect to server(%s).\n", ip_addr);
		once = 0;
		ret = 0;
	}

	return ret;
}
