/*
 * sample_factory.c
 *
 *  Created on: 2015年6月15日
 *      Author: work
 */
#include <linux/types.h>
//#include <stddef.h>
//#include <strings.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "sample_factory.h"
#include "debug.h"

static struct list_head g_list;
static pthread_mutex_t g_mutex_list;

struct SampleImageGroup * sample_factory_produce()
{
	struct list_head *node_of_sample;
	struct SampleImageGroup *sample;

	pthread_mutex_lock(&g_mutex_list);

	if(list_empty(&g_list))
		return NULL;

	node_of_sample = g_list.next;
	sample = (struct SampleImageGroup *)(node_of_sample->owner);
	memset(&(sample->sample_info), 0, sizeof(sample->sample_info));

	list_del(node_of_sample);

	pthread_mutex_unlock(&g_mutex_list);

	return sample;
}

void sample_factory_recycle(struct SampleImageGroup *sample)
{
	pthread_mutex_lock(&g_mutex_list);
	list_add_tail(&(sample->node), &g_list);
	pthread_mutex_unlock(&g_mutex_list);
}

void create_sample_factory(__u32 num)
{
	__u32 i = 0,j=0;
	struct SampleImageGroup *pSampleImageGroup;

	init_list_head(&g_list);
	pthread_mutex_init(&g_mutex_list, NULL);

	pSampleImageGroup = malloc(sizeof(struct SampleImageGroup)*num);
	if(NULL == pSampleImageGroup)
		ERROR("malloc");

	for(i=0;i<num;i++)
	{
		pSampleImageGroup[i].node.owner = &pSampleImageGroup[i];
		list_add_tail(&(pSampleImageGroup[i].node), &g_list);
	}
}
