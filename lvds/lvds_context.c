/*
 * lvds_context.c
 *
 *  Created on: 2015年5月15日
 *      Author: work
 */
#include <linux/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lvds_context.h"
#include "message.h"
#include "msg_factory.h"
#include "buf_factory.h"
#include "thread.h"
#include "fpga.h"
#include "net_context.h"
#include "debug.h"
#include "crc16.h"
#include "mailbox.h"
#include "net.h"
#include "net_protocol.h"
#include "net_context.h"
#include "sys_conf.h"

static __s32 g_lvds1_mbx_id = 0;
static __s32 g_lvds2_mbx_id = 0;

static __u32 set_protocol_map[] =
{
		0x04, //0x45521201
		0x15, //0x45521202
		0x08, //0x45521203
		0x11, //0x45521204
		0x12, //0x45521205
		0x00, //0x45521206(保留)
		0x13, //0x45521207
		0x14, //0x45521208
		0x07, //0x45521209(保留)
		0x05, //0x4552120A
		0x0A, //0x4552120B
		0x0F, //0x4552120C
		0x17, //0x4552120D
};



static __u32 get_protocol_map[] =
{
		0x03, //0x45522201
		0x02, //0x45522202
		0x01, //0x45522203
		0x0E,	//0x45522204
		0x10,	//0x45522205
		0x16, //0x45522206
		0x06, //0x45522207(主板发送，PC保留)
};


static __u32 back_protocol_map[] =
{
	0x45530201, 	//0x01,
	0x45530202,		//0x02,
	0x45530203,		//0x03,
	0x45530208,		//0x04,
	0x45530205,		//0x05, (保留)
	0x00000000,		//0x06, (PC收不到)
	0x45530206,		//0x07,
	0x45530204,		//0x08,
	0x45530207,		//0x09,
	0x45530209			//0x0A,
};

void lvds1_communicate_tsk()
{
	struct message msg;
	struct message *net_msg;
	struct CAM_PROTOCOL_FROM *data2send = NULL;
	struct Buffer* send_buf;
	__u32 net_protocol_head = 0;
	__u16 lvds_protocol_head = 0;
	__u32 idx = 0;
	__u32 vid = 0; //video_id
	__u32 net_protocol_len = 0;
	__u16 lvds_protocol_len = 0;
	__u16 lvds_send_len = 0;
	__u16 lvds_recv_len = 0;
	__u16 lvds_valid_len = 0;
	__u16 retry_num = 0;
	__u32 type = 0;
	__u16 crc = 0;
	__u32 err;
	__s32 ret;

//	static struct NetInfoHead cam_info_head;

	msg_factory_cast(&msg, g_lvds1_mbx_id);

	while(1)
	{
		mailbox_pend(&msg);
		send_buf = msg.ops->get_data(&msg, &net_protocol_len);
		msg_factory_recycle(&msg);

		memcpy(&net_protocol_head, (char *)send_buf->memory + 0, 4);
		memcpy(&net_protocol_len, (char *)send_buf->memory + 4, 4);

		type = net_protocol_head & (~0xFF);
		lvds_protocol_len = net_protocol_len + 4 - 6;
		idx = (net_protocol_head & 0xFF) - 1;

		if(type == CAMERA_SET_CMD)
			lvds_protocol_head = set_protocol_map[idx];
		else if(type == CAMERA_GET_CMD)
			lvds_protocol_head = get_protocol_map[idx];
		else
		{
			buf_factory_recycle(0, send_buf);
			continue;
		}

		memcpy((__u8 *)(send_buf->memory) + 4, &lvds_protocol_head, 2);
		memcpy((__u8 *)(send_buf->memory) + 6, &lvds_protocol_len, 2);
		crc = crc16(0, (__u8 *)(send_buf->memory) + 4, lvds_protocol_len + 2);
		*((__u16*)(send_buf->memory) + 2 + (lvds_protocol_len + 2) / 2) = crc;
		//为了摄像头的32位宽，进行补足
		*((__u16*)(send_buf->memory) + 2 + (lvds_protocol_len + 2) / 2 + 1) = crc;
		lvds_send_len = lvds_protocol_len/2 + 2 + 1;

		fpga_check_lvds1_rxfifo();

		fpga_lvds1_send((__u16 *)send_buf->memory + 2, lvds_send_len);
		buf_factory_recycle(0, send_buf);

		data2send = sc_get_cam1_protocol();
		err = fpga_lvds1_recv((__u16*)data2send, &lvds_recv_len);

		if(err != 0)
		{
			err = err|(0x03<<16);
			send_feedback_info(err);
		}
		else
		{

			if(data2send->head == 0x06)//温度信息
			{
				struct SysStatisticsInfo* pstate;
				__u32 len = 0;
				pstate = sc_get_sys_state_info_config(&len);
				pstate->sys_temp_info.camera_temp[0] = data2send->data[0];
				continue;
			}
			else if(data2send->head == 0x01)//基本信息，
			{
				//获取自检状态
			}

			//头命令字转换
			data2send->cam_info_head.type = back_protocol_map[data2send->head-1];
			//协议长度转换
			lvds_valid_len = *((__u16*)data2send + 1);
			data2send->cam_info_head.length = lvds_valid_len - 2 + 8;

			memcpy(data2send, &vid, 4);

			if (g_conn_state == LINK_ON)
			{

				net_msg = msg_factory_produce(g_net_mbx_id, 1);
				net_msg->ops->set_data(net_msg, &(data2send->cam_info_head), sizeof(data2send->cam_info_head), NULL, 0);
				net_msg->ops->set_data(net_msg, data2send, data2send->cam_info_head.length - 4 , NULL, 0);
				ret = mailbox_timedpost(net_msg);
				if (ret < 0)
							msg_factory_recycle(net_msg);
			}
		}
	}
}

void lvds2_communicate_tsk()
{
	struct message msg;
	struct message *net_msg;
	struct CAM_PROTOCOL_FROM *data2send = NULL;
	struct Buffer* send_buf;
	__u32 net_protocol_head = 0;
	__u16 lvds_protocol_head = 0;
	__u32 idx = 0;
	__u32 vid = 1;  //video_id
	__u32 net_protocol_len = 0;
	__u16 lvds_protocol_len = 0;
	__u16 lvds_send_len = 0;
	__u16 lvds_recv_len = 0;
	__u16 lvds_valid_len = 0;
	__u16 retry_num = 0;
	__u32 type = 0;
	__u16 crc = 0;
	__u32 err;
	__s32 ret;

	msg_factory_cast(&msg, g_lvds2_mbx_id);

	while(1)
	{
		mailbox_pend(&msg);
		send_buf = msg.ops->get_data(&msg, &net_protocol_len);
		msg_factory_recycle(&msg);

		memcpy(&net_protocol_head, (char *)send_buf->memory + 0, 4);
		memcpy(&net_protocol_len, (char *)send_buf->memory + 4, 4);

		type = net_protocol_head & (~0xFF);
		lvds_protocol_len = net_protocol_len + 4 - 6;
		idx = (net_protocol_head & 0xFF) - 1;

		if(type == CAMERA_SET_CMD)
			lvds_protocol_head = set_protocol_map[idx];
		else if(type == CAMERA_GET_CMD)
			lvds_protocol_head = get_protocol_map[idx];
		else
		{
			buf_factory_recycle(0, send_buf);
			continue;
		}

		memcpy((__u8 *)(send_buf->memory) + 4, &lvds_protocol_head, 2);
		memcpy((__u8 *)(send_buf->memory) + 6, &lvds_protocol_len, 2);
		crc = crc16(0, (__u8 *)(send_buf->memory) + 4, lvds_protocol_len + 2);
		*((__u16*)(send_buf->memory) + 2 + (lvds_protocol_len + 2) / 2) = crc;
		//为了摄像头的32位宽，进行补足
		*((__u16*)(send_buf->memory) + 2 + (lvds_protocol_len + 2) / 2 + 1) = crc;
		lvds_send_len = lvds_protocol_len/2 + 2 + 1;

		fpga_check_lvds2_rxfifo();

		fpga_lvds2_send((__u16 *)send_buf->memory + 2, lvds_send_len);
		buf_factory_recycle(0, send_buf);

		data2send = sc_get_cam2_protocol();
		err = fpga_lvds2_recv((__u16 *)data2send, &lvds_recv_len);

		if(err != 0)
		{
			err = err|(0x04<<16);
			send_feedback_info(err);
		}
		else
		{

			if(data2send->head == 0x06)//温度信息
			{
				struct SysStatisticsInfo* pstate;
				__u32 len = 0;
				pstate = sc_get_sys_state_info_config(&len);
				pstate->sys_temp_info.camera_temp[1] = data2send->data[0];
				continue;
			}
			else if(data2send->head == 0x01)//基本信息
			{
				//获取自检状态
			}

			//头命令字转换
			data2send->cam_info_head.type = back_protocol_map[data2send->head-1];
			//协议长度转换
			lvds_valid_len = *((__u16*)data2send + 1);
			data2send->cam_info_head.length = lvds_valid_len - 2 + 8;

			memcpy(data2send, &vid, 4);

			if (g_conn_state == LINK_ON)
			{
				net_msg = msg_factory_produce(g_net_mbx_id, 1);
				net_msg->ops->set_data(net_msg, &(data2send->cam_info_head), sizeof(data2send->cam_info_head), NULL, 0);
				net_msg->ops->set_data(net_msg, data2send, data2send->cam_info_head.length - 4 , NULL, 0);
				ret = mailbox_timedpost(net_msg);
				if (ret < 0)
							msg_factory_recycle(net_msg);
			}
		}
	}
}



void lvds_context_init()
{
	g_lvds1_mbx_id = mailbox_create("/lvds1");
	g_lvds2_mbx_id = mailbox_create("/lvds2");

	add_new_thread(NULL, (void *)&lvds1_communicate_tsk, 15, 0, 0);
	add_new_thread(NULL, (void *)&lvds2_communicate_tsk, 15, 0, 0);
}

__s32 get_lvds_context_mailbox(__u32 id)
{
	__u32 lvds_mbx_id = 0;

	if(id == 0)
		lvds_mbx_id = g_lvds1_mbx_id;
	else if(id == 1)
		lvds_mbx_id = g_lvds2_mbx_id;
	else;

	return lvds_mbx_id;
}
