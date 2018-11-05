#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

pthread_t add_new_thread(void* args, void *(*func)(void *), int priority, int cpu_id, int stacksize);

#endif
