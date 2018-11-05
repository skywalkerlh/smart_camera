/*
 * protocol_net.h
 *
 *  Created on: 2015年5月4日
 *      Author: work
 */

#ifndef NET_CONTEXT_H_
#define NET_CONTEXT_H_

#include <linux/types.h>

#define NET_INFO(dir, type)		dir##_info_##type
#define NET_UL(type)						NET_INFO(up, type)
#define NET_DL(type)						NET_INFO(down, type)

#define MAINBOARD_SET_CMD	0x45521100
#define CAMERA_SET_CMD			0x45521200

#define MAINBOARD_GET_CMD	0x45522100
#define CAMERA_GET_CMD			0x45522200

#define send_sample_info	NET_UL(0x45530101)
#define send_image_info		NET_UL(0x45530102)
#define send_length_info		NET_UL(0x45530103)
#define send_sys_state_info		NET_UL(0x45530105)
#define send_feedback_info		NET_UL(0x4553010E)

struct NetInfoHead
{
	__u32 length;
	__u32 type;
};


#define LINK_ON		1
#define LINK_OFF		0
#define YES					1
#define NO					0


#endif /* NET_CONTEXT_H_ */
