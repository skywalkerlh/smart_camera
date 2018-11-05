/*
 * sample_count.c
 *
 *  Created on: 2015年9月6日
 *      Author: work
 */
#include <pthread.h>
#include "sample_count.h"
#include "list.h"
#include "debug.h"

#define MAX_COUNT_ARRAY  16

static COUNT_ARRAY count_array[MAX_COUNT_ARRAY];
static __u32 sample_count_index = 0;

static pthread_mutex_t g_mutex_array;

void sample_count_init(void)
{
	__u32 i;
	for(i=0;i<MAX_COUNT_ARRAY;i++)
	{
		count_array[i].valid = 0;
		count_array[i].count = 0;
	}
	pthread_mutex_init(&g_mutex_array, NULL);
}

__s32 find_current_count(__u32 count)
{
	__u32 i;
	pthread_mutex_lock(&g_mutex_array);
	for(i=0;i<MAX_COUNT_ARRAY;i++)
	{
		if(count_array[i].valid)
		{
			if(count_array[i].count == count)
			{
				count_array[i].valid = 0;
				pthread_mutex_unlock(&g_mutex_array);
				return 0;
			}
		}
	}
	pthread_mutex_unlock(&g_mutex_array);
	return -1;
}

__u32 find_min_count(void)
{
	__u32 i;
	__u32 min = 0xFFFFFFFF;
	COUNT_ARRAY* cur_array = NULL;
	pthread_mutex_lock(&g_mutex_array);
	for(i=0;i<MAX_COUNT_ARRAY;i++)
	{
		if(count_array[i].valid)
		{
			if(count_array[i].count<min)
			{
				min = count_array[i].count;
				cur_array = &count_array[i];
			}
		}
	}
	if(cur_array == NULL)
		return 0;
	cur_array->valid = 0;
	pthread_mutex_unlock(&g_mutex_array);
	return cur_array->count;
}

void write_sample_count(__u32 cur_count)
{
	pthread_mutex_lock(&g_mutex_array);
	count_array[sample_count_index].count = cur_count;
	count_array[sample_count_index].valid = 1;
	pthread_mutex_unlock(&g_mutex_array);
	sample_count_index++;
	if (sample_count_index >= MAX_COUNT_ARRAY)
		sample_count_index = 0;
}




