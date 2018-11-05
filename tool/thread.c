
#define __USE_GNU
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>

#include "thread.h"
#include "debug.h"

/* 函数名： add_new_thread_prepare 
 * 描  述：   设置线程属性
 * 参  数：   attr：线程属性，主调函数创建并传入
 * 				priority：线程优先级
* 					cpu_id：线程依附的cpu核
* 					stacksize：线程栈空间（必须大于16K）
 *
 * 返回值： 无          
 */
static void add_new_thread_prepare(pthread_attr_t *attr, int priority, int cpu_id, int stacksize)
{
	struct sched_param schedParam;
	cpu_set_t cpu_mask;
	int priority_max = 0;
	int res = 0;
	int cpu_num;

	/* 抢占式调度 */
	res = pthread_attr_setschedpolicy(attr, SCHED_FIFO);
	if (res != 0)
	{
		ERROR("pthread_attr_setschedpolicy");
	}

	/* 设置优先级 */
	priority_max = sched_get_priority_max(SCHED_FIFO);
	if ((0 > priority) || (priority > priority_max))
	{
		ERROR("sched_get_priority_max");
	}

	schedParam.sched_priority = priority;
	res = pthread_attr_setschedparam(attr, &schedParam);
	if (res != 0)
	{
		ERROR("pthread_attr_setschedparam");
	}

	if(stacksize != 0)
	{
		res = pthread_attr_setstacksize(attr, stacksize + PTHREAD_STACK_MIN);
		if (res != 0)
		{
			ERROR("pthread_attr_setstacksize");
		}
	}

	cpu_num = sysconf(_SC_NPROCESSORS_CONF);
	if(cpu_id>=cpu_num)
	{
		ERROR("sysconf");
	}
	CPU_ZERO(&cpu_mask);
	CPU_SET(cpu_id, &cpu_mask);

	if (0 != pthread_attr_setaffinity_np(attr, sizeof(cpu_mask), &cpu_mask))
	{
		ERROR("pthread_attr_setaffinity_np");
	}
}

/* 函数名： add_new_thread 
 * 描  述：   创建线程
 * 参  数：   func线程函数
 *                 args线程参数
 *                 priority线程优先级
 *                 cpu_id线程依附的cpu核
 * 返回值： 无          
 */
pthread_t add_new_thread(void* args, void *(*func)(void *), int priority, int cpu_id, int stacksize)
{
	int res = 0;
	pthread_attr_t thread_attr;
	pthread_t id;

	res = pthread_attr_init(&thread_attr);
	if (res != 0)
	{
		ERROR("pthread_attr_init");
	}
	add_new_thread_prepare(&thread_attr, priority, cpu_id, stacksize);
	pthread_create(&id, &thread_attr, func, args);
	if (res != 0)
	{
		ERROR("pthread_create");
	}

	pthread_attr_destroy(&thread_attr);
	if (res != 0)
	{
		ERROR("pthread_attr_destroy");
	}
	return id;
}

