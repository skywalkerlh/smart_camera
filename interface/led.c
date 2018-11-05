/*
 * led.c
 *
 *  Created on: 2015年11月3日
 *      Author: work
 */
#include <stdio.h>
#include <unistd.h>
#include <linux/types.h>

#include "thread.h"

__u32 myexec(char *cmd,__u8* buf)
{
	__u32 i;
	FILE *fp;
	fp = popen(cmd,"r");
	if(buf != NULL)
	{
		while(fgets(buf, sizeof(buf), fp))
		{
			i++;
		}
	}
	pclose(fp);
	return i;
}


void led_tsk(void)
{
	__u8 cmd[128];
	__u32 i=0;
	while(1)
	{
		i++;
		if(i%2)
		{
			sprintf(cmd,"echo \"1\" > /sys/class/gpio/gpio77/value");
			myexec(cmd,NULL);
		}
		else
		{
			sprintf(cmd,"echo \"0\" > /sys/class/gpio/gpio77/value");
			myexec(cmd,NULL);
		}
		sleep(1);
	}
}


void led_context_init()
{
	add_new_thread(NULL, (void *)&led_tsk, 5, 0, 1*1024);
}
