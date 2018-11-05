/*
 * msg_engine.h
 *
 *  Created on: 2015年4月23日
 *      Author: work
 */

#ifndef MSG_ENGINE_H_
#define MSG_ENGINE_H_

#include <linux/types.h>
#include "message.h"

__s32 mailbox_create(char *name);
__s32 mailbox_post(struct message *msg);
__s32 mailbox_pend(struct message *msg);
__s32 mailbox_timedpend(struct message *msg);
__s32 mailbox_timedpost(struct message *msg);

#endif /* MSG_ENGINE_H_ */
