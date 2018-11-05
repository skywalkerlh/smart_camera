/*
 * sample_count.h
 *
 *  Created on: 2015年9月6日
 *      Author: work
 */
#include <linux/types.h>
#include "list.h"

typedef struct COUNT_ARRAY
{
	struct list_head node;
	__u32 valid;
	__u32 count;
}COUNT_ARRAY;


void sample_count_init(void);

__s32 find_current_count(__u32 count);

__u32 find_min_count(void);

void write_sample_count(__u32 cur_count);
