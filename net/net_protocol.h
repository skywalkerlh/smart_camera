/*
 * protocol.h
 *
 *  Created on: 2015年7月28日
 *      Author: work
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "net_context.h"

void NET_UL(0x45530101)(void *resource, void (*release)(void *resource));
//__s32 NET_UL(0x45530102)(void *resource, void (*release)(__s32 arg, void *));
__s32 NET_UL(0x45530102)(void *resource);
void NET_UL(0x45530103)(void *data, __u32 len);
void NET_UL(0x45530105)(void *data, __u32 len);
void NET_UL(0x4553010E)(__u32 val);

extern __u32 (*mainboard_set_func[])(void *data, __u32 len);
extern void (*mainboard_get_func[])(void *data, __u32 len);

void protocol_analyze(__u8* data ,__u32 len);

#endif /* PROTOCOL_H_ */
