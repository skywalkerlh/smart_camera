/*
 * udisk.c
 *
 *  Created on: 2015年8月19日
 *      Author: work
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <regex.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <semaphore.h>
#include "thread.h"
#include "sys_log.h"
#include "udisk.h"
#include "ui_protocol.h"

#define UEVENT_BUFFER_SIZE 2048

__u32 udisk_num = 0;

struct UDISK g_udisk[2];


extern sem_t gsem_udisk_ready;

//__u32 get_udisk_state(struct UDISK  *udisk)
//{
////	if(len!=NULL)
////	{
////		*len = sizeof(struct UDISK);
////	}
//	return udisk->state;
//}

__u32 GetStorageInfo(char * MountPoint, __u32 *AllCapacity, __u32 *LeftCapacity)
{
	struct statfs statFS;

	__u64 freeBytes = 0;
	__u64 totalBytes = 0;

	if (statfs(MountPoint, &statFS) == -1)
	{   //获取分区的状态
		printf("statfs failed for path->[%s]\n", MountPoint);
		return (-1);
	}

	totalBytes = (__u64 ) statFS.f_blocks * (__u64 ) statFS.f_frsize; //以字节为单位的总容量
	freeBytes = (__u64 ) statFS.f_bfree * (__u64 ) statFS.f_frsize; //以字节为单位的剩余容量

	*AllCapacity = totalBytes >> 10; //以KB为单位的总容量
	*LeftCapacity = freeBytes >> 10; //以KB为单位的剩余容量

	return 0;
}

static char * substr(const char * str, unsigned start, unsigned end)
{
	unsigned n = end - start;
	static char strbuf[256];
	memset(strbuf, '\0', sizeof(strbuf));
	strncpy(strbuf, str + start, n);
	strbuf[n] = '\0';
	return strbuf;
}
struct UDISK * do_mount(const char* relative_file_name)
{
	__s32 ret = 0;
	__u32 all_capacity;
	__u32 left_capacity;
	__u8  buf[128];
	__u32 n=0;
	struct UDISK *udisk;

	char absolute_file_name[16] = "/dev/";
	char mount_point[16] = "/mnt/";

	strcat(absolute_file_name, relative_file_name);
	strcat(mount_point, relative_file_name);

	ret = mount(absolute_file_name, mount_point, "vfat", MS_NOSUID, "iocharset=cp936");
	if (ret == 0)
	{
		log_builder("System power on.");
		log_builder("System check result:");
		GetStorageInfo(mount_point, &all_capacity, &left_capacity);
		n += sprintf(buf+n,"\n %s is inserted.\n", absolute_file_name);
		n += sprintf(buf+n,"\n All Capacity:%uKB\n", all_capacity);
		n += sprintf(buf+n,"\n Left Capacity:%uKB\n", left_capacity);

		if(strstr(mount_point, "sda1"))
		{
			udisk = &g_udisk[0];
		}
		else if(strstr(mount_point, "sdb1"))
		{
			udisk = &g_udisk[1];
		}

		memset(udisk->dir,0,32);
		strcpy(udisk->dir, mount_point);
//		strcat(udisk.dir, "/");
		sem_post(&gsem_udisk_ready);
		udisk->all_capacity = all_capacity;
		udisk->left_capacity = left_capacity;
		udisk->state = 0;
		udisk->port = 0;
		log_builder(buf);
	}
	if (ret != 0)
		perror("mount");
return udisk;
}

struct UDISK * do_umount(const char* relative_file_name)
{
	__u8 buf[128];
	struct UDISK *udisk;
	char mount_point[16] = "/mnt/";
	strcat(mount_point, relative_file_name);
	umount(mount_point);
	sprintf(buf,"\n /dev/%s is removed.\n", relative_file_name);
	log_builder(buf);
	if(strstr(mount_point, "sda1"))
	{
		udisk = &g_udisk[0];
	}
	else if(strstr(mount_point, "sdb1"))
	{
		udisk = &g_udisk[1];
	}
	udisk->state = 1;
	return udisk;
}

__s32 do_check(const char* relative_file_name)
{
	__s32 ret = 0;
	char absolute_file_name[16] = "/dev/";
	strcat(absolute_file_name, relative_file_name);
	ret = access(absolute_file_name, F_OK);
	return ret;
}

int udisk_tsk(void)
{
	char *action;
	char *relative_file_name;
	char *possible_udisk[] = {"sda1","sdb1","sdc1"};
	__s32 i,ret = 0;

	regex_t myreg;
	char pattern[] = "sd[a-z][1-9]$";
	char err[128];
  regmatch_t match[2];
  size_t nmatch = 2;

	struct sockaddr_nl client;
	struct timeval tv;
	__s32 fd, rcvlen;
	fd_set fds;
	int buffersize = 1024;

	struct UDISK *udisk;


	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
	memset(&client, 0, sizeof(client));
	client.nl_family = AF_NETLINK;
	client.nl_pid = getpid();
	client.nl_groups = 1; /* receive broadcast message*/
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));
	bind(fd, (struct sockaddr*) &client, sizeof(client));

	/* 编译正则表达式 */
	ret = regcomp(&myreg, pattern, REG_EXTENDED | REG_NEWLINE);
	if (ret != 0)
	{
		regerror(ret, &myreg, err, sizeof(err));
		fprintf(stderr, "%s\n",err);
		regfree(&myreg);
		exit(1);
	}

	/* 检测程序启动前是否已有u盘接入 */
	for(i=0;i<3;i++)
	{
		ret = do_check(possible_udisk[i]);
		if (ret == 0)
		{
			udisk = do_mount(possible_udisk[i]);
			send_ui_udisk_info(udisk);
		}
	}

	while (1)
	{
		int i = 0;
		char buf[UEVENT_BUFFER_SIZE] = { 0 };
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;
		ret = select(fd + 1, &fds, NULL, NULL, &tv);
		if (ret < 0)
		{
			continue;
		}
		if (!(ret > 0 && FD_ISSET(fd, &fds)))
		{
			continue;
		}
		/* receive data */
		rcvlen = recv(fd, &buf, sizeof(buf), 0);
		if (rcvlen > 0)
		{
			ret = regexec(&myreg, buf, nmatch, match, 0);
			if (ret != 0)
			{
				continue;
			}

			relative_file_name = substr(buf, match[0].rm_so, match[0].rm_eo);

			action = strtok(buf, "@");
			ret = strcmp(action, "add");
			/* 拔出U盘 */
			if (ret)
			{
				while(!do_check(relative_file_name));
				udisk = do_umount(relative_file_name);
				send_ui_udisk_info(udisk);
			}
			/* 插入U盘 */
			else
			{
				while(do_check(relative_file_name));
				udisk = do_mount(relative_file_name);
				send_ui_udisk_info(udisk);
			}
		}
	}
	close(fd);
	return 0;
}

void udisk_context_init()
{
	g_udisk[0].state = 1;
	g_udisk[1].state = 1;
	add_new_thread(NULL, (void *)&udisk_tsk, 8, 0, 16*1024);
}


