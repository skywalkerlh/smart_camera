/*
 * ui_protocol.c
 *
 *  Created on: 2015年9月10日
 *      Author: work
 */

#include <linux/types.h>
#include <stddef.h>
#include <string.h>

#include "ui_protocol.h"
#include "buf_factory.h"
#include "debug.h"
#include "message.h"
#include "msg_factory.h"
#include "mailbox.h"
#include "sys_conf.h"
#include "sample_factory.h"
#include "v4l2.h"
#include "shared_vid_buf.h"
#include "sys_log.h"
#include "udisk.h"
#include "key_file.h"

extern __s32 g_ui_socket_state;
extern __s32 g_ui_mbx_id;


__u32 g_ui_img_ability = YES;
//pthread_mutex_t g_mutex_ui_img_ability;
//struct image_buffer g_ui_img_buf[2];

typedef struct UIInfoHead
{
	__u32 length;
	__u32 cmd;
}UIInfoHead;

typedef struct UIFeedBackInfo
{
	UIInfoHead head;
	__u32 type;
	__u32 val; // 返回值
	__u32 data;
}UIFeedBackInfo;



/*************************************************************************/
//主进程发送，ui接收命令


//算法参数
void UI_UL(0x45530A01)(void)
{
	static UIInfoHead infohead_0x45530A01 =
	{
			.length = 1044,
			.cmd = 0x45530A01,
	};

	if(g_ui_socket_state)
	{
		struct message *msg =NULL;
		void *data2send = NULL;
		__u32 len_of_data = 0;
		__s32 ret = 0;

		msg = msg_factory_produce(g_ui_mbx_id, 0);
		msg->ops->set_data(msg, &infohead_0x45530A01, 8, NULL, 0);

		data2send = sc_get_algo_params(&len_of_data);

		msg->ops->set_data(msg, (__u8*)data2send+8, len_of_data-8, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

// 反馈信息
UIFeedBackInfo uifeedbackinfo =
{
	.head =
	{
		.length = 16,
		.cmd = 0x45530A02,
	},
	.type = 0,
};

void UI_UL(0x45530A02)(__u32 type, __u32 val)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	uifeedbackinfo.val = val;

	if(g_ui_socket_state)
	{
		msg = msg_factory_produce(g_ui_mbx_id, 0);
		msg->ops->set_data(msg, &uifeedbackinfo, uifeedbackinfo.head.length, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

//图像信息

//__s32  allow_send_ui_image(void *__msg__)
//{
//	struct message *msg = __msg__;
//	pthread_mutex_lock(&g_mutex_ui_img_ability);
//	g_ui_img_ability = YES;
//	pthread_mutex_unlock(&g_mutex_ui_img_ability);
//	return 0;
//}
static __u32 KEY[2] = {IPC_KEY_A,IPC_KEY_B};

__s32 UI_UL(0x45530A03)(void *resource, void (*release)(void *resource))
{
	__u32 len_of_data = 0;
	__u32 i = 0;
	__s32 ret = 0;
	struct message *msg =NULL;
	struct SampleImageGroup *pSampleImageGroup;

	static UIInfoHead infohead_0x45530A03 =
	{
			.length = 4,
			.cmd = 0x45530A03,
	};

	pSampleImageGroup = (struct SampleImageGroup *) resource;

	if (g_ui_socket_state)
	{
		if (g_ui_img_ability)
		{
			g_ui_img_ability = 0;

			msg = msg_factory_produce(g_ui_mbx_id, 0);
			msg->finish = NULL;
			msg->resource = resource;
			msg->release = release;

			msg->ops->set_data(msg, &infohead_0x45530A03, sizeof(infohead_0x45530A03), NULL, 0);

			infohead_0x45530A03.length = 4;

			for (i = 0; i < pSampleImageGroup->image_num; i++)
			{
				msg->ops->set_data(msg, &(pSampleImageGroup->image[i]->r_count), 92, NULL, 0);
				msg->ops->set_data(msg, &KEY[i], 4, NULL, 0);
				infohead_0x45530A03.length += 96;
			}

			ret = mailbox_timedpost(msg);
			if (ret < 0)
			{
				msg_factory_recycle(msg);
				MSG("%s\n", __FUNCTION__);
				return -1;
			}
			else
			{
				return 0;
			}
		}
	}
	//释放图像缓冲
	release(resource);

	return -1;
}

//版本信息
void UI_UL(0x45530A04)(void)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	static UIInfoHead infohead_0x45530A04 =
	{
			.length = 392,
			.cmd = 0x45530A04,
	};

	if (g_ui_socket_state)
	{
		data2send = sc_get_main_board_version(&len_of_data);
		msg = msg_factory_produce(g_ui_mbx_id, 0);
		msg->ops->set_data(msg, &infohead_0x45530A04, 8, NULL, 0);
		msg->ops->set_data(msg, data2send+8, len_of_data-8, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

//存储设备热插拔通知
void UI_UL(0x45530A05)(struct UDISK *udisk)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;

	static UIInfoHead infohead_0x45530A05 =
	{
			.length = 52,
			.cmd = 0x45530A05,
	};

	if (g_ui_socket_state)
	{
		data2send = udisk;
		msg = msg_factory_produce(g_ui_mbx_id, 0);
		msg->ops->set_data(msg, &infohead_0x45530A05, 8, NULL, 0);
		msg->ops->set_data(msg, data2send, sizeof(struct UDISK), NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}

//统计信息
void UI_UL(0x45530A06)(void *data, __u32 len)
{
	struct message *msg =NULL;
	void *data2send = NULL;
	__u32 len_of_data = 0;
	__s32 ret = 0;
	static UIInfoHead infohead_0x45530A06 =
	{
			.length = 114,
			.cmd = 0x45530A06,
	};

	if (g_ui_socket_state)
	{
		data2send = sc_get_sys_state_info_config(&len_of_data);
		msg = msg_factory_produce(g_ui_mbx_id, 0);
		msg->ops->set_data(msg, &infohead_0x45530A06, 8, NULL, 0);
		msg->ops->set_data(msg, data2send+8, len_of_data-8, NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}


//static UIInfoHead infohead_0x45530A07 =
//{
//		.length = 4,
//		.cmd = 0x45530A06,
//};
struct ImageRequire
{
	UIInfoHead infohead_0x45530A07;
	__u32 video_mun;
	__u32 video1_width;
	__u32 video1_height;
	__u32 video1_key;
	__u32 video2_width;
	__u32 video2_height;
	__u32 video2_key;
};

struct ImageRequire UIInfoImageRequire  =
{
		.infohead_0x45530A07.length = 32,
		.infohead_0x45530A07.cmd = 0x45530A07,
};

//图像需求信息
void UI_UL(0x45530A07)(void)
{
	struct message *msg =NULL;
//	void *data2send = NULL;
//	__u32 len_of_data = 0;
	__s32 ret = 0;

//	static __u32 video_num = 0;
	__u32 vid1_valid = 0;
	__u32 vid2_valid = 0;

	if (g_ui_socket_state)
	{
		vid1_valid	= key_file_get_int("video_channel1", "valid",	0);
		vid2_valid	= key_file_get_int("video_channel2", "valid",	0);

		if(vid1_valid && vid2_valid)
		{
			UIInfoImageRequire.video_mun = 2;
			UIInfoImageRequire.video1_width = key_file_get_int("video_channel1", "width",	752);
			UIInfoImageRequire.video1_height = key_file_get_int("video_channel1", "height",	480);
			UIInfoImageRequire.video1_key = IPC_KEY_A;
			UIInfoImageRequire.video2_width = key_file_get_int("video_channel2", "width",	752);
			UIInfoImageRequire.video2_height = key_file_get_int("video_channel2", "height",	480);
			UIInfoImageRequire.video2_key = IPC_KEY_B;
		}

		else
		{
			UIInfoImageRequire.video_mun = 1;
			if(vid1_valid)
			{
				UIInfoImageRequire.video1_width = key_file_get_int("video_channel1", "width",	752);
				UIInfoImageRequire.video1_height = key_file_get_int("video_channel1", "height",	480);
				UIInfoImageRequire.video1_key = IPC_KEY_A;
			}
			else if(vid2_valid)
			{
				UIInfoImageRequire.video1_width = key_file_get_int("video_channel2", "width",	752);
				UIInfoImageRequire.video1_height = key_file_get_int("video_channel2", "height",	480);
				UIInfoImageRequire.video1_key = IPC_KEY_B;
			}
			UIInfoImageRequire.video2_width = 0;
			UIInfoImageRequire.video2_height = 0;
			UIInfoImageRequire.video2_key = 0;
		}

		msg = msg_factory_produce(g_ui_mbx_id, 0);
		msg->ops->set_data(msg, &UIInfoImageRequire, sizeof(UIInfoImageRequire), NULL, 0);
		ret = mailbox_timedpost(msg);
		if (ret < 0)
			msg_factory_recycle(msg);
	}
}


/*************************************************************************/
//ui进程发送，主进程接收命令

//图像处理完成通知
__u32 UI_DL(0x45530B01)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	g_ui_img_ability = YES;
	return 0;
}

//设置算法参数
__u32 UI_DL(0x45530B02)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	sc_set_algo_params(data);
	send_ui_feedback_info(0, 0);
	return 0;
}



//查询算法参数
__u32 UI_DL(0x45530B03)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	UI_UL(0x45530A01)();
	return 0;
}

//系统复位
__u32 UI_DL(0x45530B04)(void *data, __u32 len)
{
	sc_system_reset();
	MSG("%s\n",__FUNCTION__);
	log_builder("system recv reset commed.");
	return 0;
}

//UI进程版本
__u32 UI_DL(0x45530B05)(void *data, __u32 len)
{
	static __u8 ver[32]={0};
	MSG("%s\n",__FUNCTION__);
	strcpy(ver, (const char *)data);
	sc_set_ui_version(ver);
	return 0;
}

//查询系统版本
__u32 UI_DL(0x45530B06)(void *data, __u32 len)
{
	MSG("%s\n",__FUNCTION__);
	UI_UL(0x45530A04)();
	return 0;
}



__u32 (*ui_cmd_func[])(void *data, __u32 len) =
{
		NULL,
		UI_DL(0x45530B01),
		UI_DL(0x45530B02),
		UI_DL(0x45530B03),
		UI_DL(0x45530B04),
		UI_DL(0x45530B05),
		UI_DL(0x45530B06),
};



void ui_protocol_analyze(void* buf ,__u32 len)
{
	__u32 sort = 0; /* 协议类别 */
	__u32 index = 0; /* 协议索引 */
	__s32 val;
	__u32 head;

	__u8 *data = buf;
	head = *((__u32*)data);

	sort = *((__u32*)data) & (~0xFF);
	index = *((__u32*)data) & 0xFF;

	if(sort == UI_CMD)
	{
		val = ui_cmd_func[index](data+4, len);
	}
}
