/*
 * ui_protocol.h
 *
 *  Created on: 2015年9月10日
 *      Author: work
 */

#include "udisk.h"


#ifndef UI_PROTOCOL_H_
#define UI_PROTOCOL_H_

#define UI_CMD	0x45530B00

#define UI_INFO(dir, type)		dir##_info_##type
#define UI_UL(type)							UI_INFO(up, type)
#define UI_DL(type)							UI_INFO(down, type)

void UI_UL(0x45530A02)(__u32 type, __u32 val);
__s32 UI_UL(0x45530A03)(void *resource, void (*release)(void *resource));
void UI_UL(0x45530A06)(void *data, __u32 len);
void UI_UL(0x45530A05)(struct UDISK *udisk);
void UI_UL(0x45530A07)(void);

#define send_ui_udisk_info			UI_UL(0x45530A05)

#define send_ui_feedback_info		UI_UL(0x45530A02)

#define send_ui_sys_state_info		UI_UL(0x45530A06)

#define send_ui_image_info  UI_UL(0x45530A03)

#define send_ui_image_require_info  UI_UL(0x45530A07)

void ui_protocol_analyze(void* buf ,__u32 len);

__s32 UI_UL(0x45530A03)(void *resource, void (*release)(void *resource));

#endif /* UI_PROTOCOL_H_ */
