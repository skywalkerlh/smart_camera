/*
 * local_socket.h
 *
 *  Created on: 2015年8月10日
 *      Author: work
 */

#ifndef LOCAL_SOCKET_H_
#define LOCAL_SOCKET_H_



void local_socket_init(void);

extern __u32 g_local_mbx_id;
extern pthread_mutex_t g_mutex_array;


typedef struct Clt_CMD_STR
{
	__u32   length;		 	// 协议长度
	__u32   cmdword;   	// 协议命令
	__u8*   param;		 		// 要利用param这个地址，因为提前参数不知道有多长
}Clt_CMD_STR;


#endif /* LOCAL_SOCKET_H_ */
