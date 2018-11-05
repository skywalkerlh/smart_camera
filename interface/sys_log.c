/*
 * sys_log.c
 *
 *  Created on: 2015年8月17日
 *      Author: work
 */

#include <time.h>
#include <stdio.h>
#include <linux/types.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#include "mailbox.h"
#include "msg_factory.h"
#include "message.h"
#include "buf_factory.h"
#include "thread.h"
#include "debug.h"
#include "udisk.h"

static __u32 g_log_mbx_id;
sem_t gsem_udisk_ready;

void log_builder(void* log_data)
{
	struct message *msg2log;

	struct Buffer *buf;
	time_t timer;
	__u32 log_len = 0;
	struct tm *tblock;

//	if((g_udisk[0].state)&&(g_udisk[1].state))
//	{
//		return;
//	}
	return;

	timer = time(NULL);
	tblock = localtime(&timer);

	buf = buf_factory_produce();

	log_len += sprintf((__u8*)(buf->memory+log_len), "[%s", asctime(tblock));
	log_len = log_len - 1;
	log_len += sprintf((__u8*)(buf->memory+log_len), "] %s\n\r",log_data);

	msg2log = msg_factory_produce(g_log_mbx_id, 0);

	msg2log->ops->set_data(msg2log, buf, log_len, NULL, 0);

	mailbox_post(msg2log);
}


void log_search(__u8 *basePath)
{
	time_t lasttime = 0xFFFFFFFF;
	__u8 file_name[256] = {0};
	__u8 rmfile_name[256] = {0};
	DIR *dir;
	__s32 err;
	__u32 n = 0;
	__u32 fcnt = 0;
	struct stat filetime;
	struct dirent *ptr;
	if ((dir=opendir(basePath)) == NULL)
	{
		MSG("Open dir error...");
	}
	while ((ptr=readdir(dir)) != NULL)
	{
		if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    // current dir OR parrent dir
			continue;

		if(ptr->d_type == 8)    ///file
		{
			fcnt++;
			n = 0;
			n += sprintf(file_name+n, basePath);
			n += sprintf(file_name+n, ptr->d_name);
			stat(file_name, &filetime);
			if(filetime.st_mtim.tv_sec < (__u32)lasttime)
			{
				lasttime = filetime.st_mtim.tv_sec;
				n = 0;
				n += sprintf(rmfile_name+n, basePath);
				n += sprintf(rmfile_name+n, ptr->d_name);
			}
		}
		else if(ptr->d_type == 10)    ///link file
		{
			continue;
		}

		else if(ptr->d_type == 4)    ///dir
		{
			continue;
		}
	}
	if(fcnt>=10)
	{
		err = remove(rmfile_name);
	}
}


void logging_tsk(void)
{
	FILE * fstream;
	__u8 file_name[32] = {0};
	time_t timer;
	__u32 n = 0;
	__s32 err;
	struct stat filetime;
	struct tm *tblock;
	void *data = NULL;
	__u32 data_len = 0;
	__u32 log_has_len = 0;
	struct message msg;
	struct Buffer *buf;
	__s32 once = 1;
	__u8 sbuf[128];
//	sem_init(&gsem_udisk_ready, 0,0);

	msg_factory_cast(&msg, g_log_mbx_id);

recreatlog:
	//日志创建

	n = 0;
	timer = time(NULL);
	tblock = localtime(&timer);
	n += sprintf(file_name+n, "/mnt/sdcard/");
	log_search(file_name);//
//	n += sprintf(file_name+n, "log-2015-10-21");//, asctime(tblock));
	n += sprintf(file_name+n, "log-%d-%d-%d-%d-%d-%d",
									tblock->tm_year+1900,
									tblock->tm_mon+1,
									tblock->tm_mday,
									tblock->tm_hour,
									tblock->tm_min,
									tblock->tm_sec);
	fstream = fopen(file_name, "wb");

	log_has_len = 0;


	while(1)
	{
		mailbox_pend(&msg);
		buf = msg.ops->get_data(&msg, &data_len);
		err = fwrite(buf->memory, data_len, 1, fstream);
		if(err <= 0)
		{
			fclose (fstream);
			remove(file_name);
			perror("fwrite");
			msg_factory_recycle(&msg);
			buf_factory_recycle(0,buf);
			goto recreatlog;
		}
		fflush(fstream);
		sync();
		log_has_len += data_len;
		if(log_has_len >= (524288))//512KB
		{
			fclose (fstream);
			msg_factory_recycle(&msg);
			buf_factory_recycle(0,buf);
			goto recreatlog;
		}

		msg_factory_recycle(&msg);
		buf_factory_recycle(0,buf);
	}
}

void log_context_init()
{
	g_log_mbx_id = mailbox_create("/log");

	add_new_thread(NULL, (void *)&logging_tsk, 9, 0, 8*1024);

//	log_builder("System power on.");
//	log_builder("System check result.");
}


