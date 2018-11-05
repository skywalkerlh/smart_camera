/*
 * tools.h
 *
 *  Created on: 2015年4月23日
 *      Author: work
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdio.h>

#define container_of(ptr, type, member) \
	({const typeof( ((type *)0)->member ) *__mptr = (ptr);\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define MSG(fmt, ...) \
	do {\
		    fprintf(stderr, "%s(Line%d): " fmt, __FILE__, __LINE__, ##__VA_ARGS__);\
	} while (0)


#define ERROR(msg)\
	do {\
		    fprintf(stderr, "%s(Line%d) ", __FILE__, __LINE__);\
            perror(msg); \
            exit(-1); \
	} while (0)

#endif /* DEBUG_H_ */
