/*
 * shared_mem_manager.c
 *
 *  Created on: 2015年4月24日
 *      Author: work
 */

//#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <libdce.h>
#include <omap_drmif.h>

#include "shared_mem_manager.h"

#include "debug.h"

struct omap_device *omap_dev = NULL;

struct shared_memory * shared_memory_alloc(int size)
{
	static int once = 0;
	struct shared_memory *shared_mem;

	shared_mem = malloc(sizeof(struct shared_memory ));
	if(shared_mem == NULL)
		ERROR("malloc");

	if (!once)
	{
		omap_dev = dce_init();
		if (omap_dev == NULL)
		{
			free(shared_mem);
			ERROR("dce_init");
		}
		once = 1;
	}

	shared_mem->bo = omap_bo_new(omap_dev, size, OMAP_BO_CACHED);
	if(shared_mem == NULL)
	{
		free(shared_mem);
		ERROR("omap_bo_new");
	}
	shared_mem->fd = omap_bo_dmabuf(shared_mem->bo);
	shared_mem->data = omap_bo_map(shared_mem->bo);

	return shared_mem;
}

void shared_memory_free(struct  shared_memory  *shared_mem)
{
	if(shared_mem == NULL)
		return;

	omap_bo_del(shared_mem->bo);
	free(shared_mem);
}

void shared_memory_cpu_fini(struct  shared_memory  *shared_mem)
{
	if(shared_mem == NULL)
		return;

	omap_bo_cpu_fini(shared_mem->bo, OMAP_GEM_READ);
}

void shared_memory_cpu_prep(struct  shared_memory  *shared_mem)
{
	if(shared_mem == NULL)
		return;

	omap_bo_cpu_prep(shared_mem->bo, OMAP_GEM_READ);
}
