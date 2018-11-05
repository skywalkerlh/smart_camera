/*
 * udisk.h
 *
 *  Created on: 2015年9月18日
 *      Author: work
 */

#ifndef UDISK_H_
#define UDISK_H_

struct UDISK
{
	__u32 port;
	__u32 state;
	__u32 all_capacity;
	__u32 left_capacity;
	__u8 	dir[32];
};

void udisk_context_init();
//__u32 get_udisk_state(struct UDISK  *udisk);

#endif /* UDISK_H_ */
