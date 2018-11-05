/*
 * local_protocol.h
 *
 *  Created on: 2015年8月11日
 *      Author: work
 */

#ifndef LOCAL_PROTOCOL_H_
#define LOCAL_PROTOCOL_H_

#include "local_socket.h"

#pragma pack(1)
typedef struct SampleResult
{
	__u8  result[8];		//算法结果
	__u32 count;				//样品计数
	__u64 wheel_code;	//拍摄时刻码盘
	__u32 pos;						//待剔除工位
	__u32 alarm;				//报警
}SampleResult;
#pragma pack()

typedef struct LocalInfoHead
{
	__u32 length;
	__u32 type;
}LocalInfoHead;


typedef struct DisInfo
{
	__u32 item;
	__u32 length;
}DisInfo;

void send_sample_reault(void *buf, __u32 len);
SampleResult* get_sample_result_array();
void local_cmd_analyze(Clt_CMD_STR* CltCmd);
void send_test_pos(__u32 item);
void send_reset_cmd();
void send_handshake_cmd();

#endif /* LOCAL_PROTOCOL_H_ */
