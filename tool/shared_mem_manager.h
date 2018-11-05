/*
 * shared_mem_manager.h
 *
 *  Created on: 2015年4月24日
 *      Author: work
 */

#ifndef SHARED_MEM_MANAGER_H_
#define SHARED_MEM_MANAGER_H_

#include <omap_drmif.h>

struct shared_memory
{
	struct omap_bo *bo;
	int fd;
	void *data;
};

struct shared_memory * shared_memory_alloc(int size);
void shared_memory_free(struct  shared_memory  *shared_mem);
void shared_memory_cpu_fini(struct  shared_memory  *shared_mem);
void shared_memory_cpu_prep(struct  shared_memory  *shared_mem);

#endif /* SHARED_MEM_MANAGER_H_ */
