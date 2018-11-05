/*
 * sys_conf.c
 *
 *  Created on: 2015年5月18日
 *      Author: work
 */

#ifndef _SYS_CONF_C_
#define _SYS_CONF_C_

#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/time.h>

#include "sys_conf.h"
#include "key_file.h"
#include "fpga.h"
#include "debug.h"
#include "local_protocol.h"
#include "video_context.h"
#include "upstate.h"

extern sem_t gsem_reject_reset;

/********************查询主控板版本****************************/
struct MainBoardVersion g_main_board_version=
{
		.head =
		{
				.type = 0x45530104,
				.length = sizeof(struct MainBoardVersion) - 4
		},
		.mpu_ver  = BUILD_DATE,
		.dsp1_ver = "unknow",
		.dsp2_ver = "unknow",
		.kick_ver = "unknow",
		.ui_ver 	 = "unknow",
		.fpga_ver = "unknow",
		.system_name = "智能相机主控板"
};

void sc_set_sys_sn(char *version)
{
	memcpy(&g_main_board_version.system_sn, version, 32);
	key_file_set_string("system_sn", "name", g_main_board_version.system_sn);
}

void sc_set_fpga_version(char *version)
{
	memcpy(&g_main_board_version.fpga_ver,  version, 32);
}

void sc_set_dsp1_version(char *version)
{
	memcpy(&g_main_board_version.dsp1_ver,  version, 32);
}

void sc_set_dsp2_version(char *version)
{
	memcpy(&g_main_board_version.dsp2_ver,  version, 32);
}

void sc_set_kick_version(char *version)
{
	memcpy(&g_main_board_version.kick_ver,  version, 32);
}

void sc_set_ui_version(char *version)
{
	memcpy(&g_main_board_version.ui_ver,  version, 32);
}

void *sc_get_main_board_version(__u32 *len)
{
	if(len != NULL)
		*len = sizeof(struct MainBoardVersion);
	return &g_main_board_version;
}
/**************************************************************/


/********************FPGA信号配置查询*************************/
struct SysSignalConfig g_sys_signal_config =
{
		.head =
		{
				.type = 0x45530106,
				.length = sizeof(struct SysSignalConfig) - 4
		},
};

void sc_set_signal_polar(void *data)
{
	__u32 value = *(__u32 *)data;
	g_sys_signal_config.polar = value;
	fpga_set_signal_polar(value);
	key_file_set_int("signal", "polar", value);
}

void sc_set_out_signal_width(void *data)
{
	__u32  width1 = 0;
	__u32  width2 = 0;
	__u32  width3 = 0;
	__u32  width4 = 0;
	__u32  width5 = 0;
	__u32  width6 = 0;
	__u32  width7 = 0;
	__u32  width8 = 0;

	memcpy(&width1, data+0x00, 4);
	memcpy(&width2, data+0x04, 4);
	memcpy(&width3, data+0x08, 4);
	memcpy(&width4, data+0x0C, 4);
	memcpy(&width5, data+0x10, 4);
	memcpy(&width6, data+0x14, 4);
	memcpy(&width7, data+0x18, 4);
	memcpy(&width8, data+0x1C, 4);

	g_sys_signal_config.kp1_width = width1;
	g_sys_signal_config.kp2_width = width2;

	key_file_set_int("signal", "kp1_width", width1);
	key_file_set_int("signal", "kp2_width", width2);
	key_file_set_int("signal", "out3_width", width3);
	key_file_set_int("signal", "out4_width", width4);
	key_file_set_int("signal", "out5_width", width5);
	key_file_set_int("signal", "out6_width", width6);
	key_file_set_int("signal", "out7_width", width7);
	key_file_set_int("signal", "out8_width", width8);

	fpga_set_out_signal_width((__u32*)data);
}

void sc_set_in_signal_width(void *data)
{
	__u32  width1 = 0;
	__u32  width2 = 0;
	__u32  width3 = 0;
	__u32  width4 = 0;
	__u32  width5 = 0;
	__u32  width6 = 0;
	__u32  width7 = 0;
	__u32  width8 = 0;

	memcpy(&width1, data+0x00, 4);
	memcpy(&width2, data+0x04, 4);
	memcpy(&width3, data+0x08, 4);
	memcpy(&width4, data+0x0C, 4);
	memcpy(&width5, data+0x10, 4);
	memcpy(&width6, data+0x14, 4);
	memcpy(&width7, data+0x18, 4);
	memcpy(&width8, data+0x1C, 4);

	g_sys_signal_config.tt_width = width1;
	g_sys_signal_config.kt_width = width2;
	g_sys_signal_config.kft_width = width3;
	g_sys_signal_config.extwheel_width = width4;

	key_file_set_int("signal", "tt_width", width1);
	key_file_set_int("signal", "kt_width", width2);
	key_file_set_int("signal", "kft_width", width3);
	key_file_set_int("signal", "extwheel_width", width4);
	key_file_set_int("signal", "in5_width", width5);
	key_file_set_int("signal", "in6_width", width6);
	key_file_set_int("signal", "in7_width", width7);
	key_file_set_int("signal", "in8_width", width8);

	fpga_set_in_signal_width((__u32*)data);
}

void *sc_get_signal_config(__u32 *len)
{
	if(len != NULL)
		*len = sizeof(struct SysSignalConfig);
	return &g_sys_signal_config;
}
/**************************************************************/


/********************FPGA延时配置设置/查询*********************/
struct SysDelayConfig g_sys_delay_config =
{
		.head =
		{
				.type = 0x45530107,
				.length = sizeof(struct SysDelayConfig) - 4
		},
};

void sc_set_shoot_delay(void *data)
{
	__u32 value[2];
	value[0] = *(__u32 *)data;
	value[1] = *((__u32 *)data+1);

	g_sys_delay_config.TT_C1_delay = value[0];
	g_sys_delay_config.C1_C2_delay = value[1];

	key_file_set_int("shoot_delay", "TT_C1_delay", value[0]);
	key_file_set_int("shoot_delay", "C1_C2_delay", value[1]);

	fpga_set_shoot_delay(value);
}

void sc_set_kick_delay(void *data)
{
	__u32 value[6];
	value[0] = *(__u32 *)data;
	value[1] = *((__u32 *)data+1);
	value[2] = *((__u32 *)data+2);
	value[3] = *((__u32 *)data+3);
	value[4] = *((__u32 *)data+4);
	value[5] = *((__u32 *)data+5);

	g_sys_delay_config.C1_KT1_delay 		= value[0];
	g_sys_delay_config.KT1_KP1_delay 	= value[1];
	g_sys_delay_config.KP1_KFT1_delay 	= value[2];
	g_sys_delay_config.KP1_KT2_delay 	= value[3];
	g_sys_delay_config.KT2_KP2_delay 	= value[4];
	g_sys_delay_config.KP2_KFT2_delay 	= value[5];

	key_file_set_int("station_delay", "C1_KT1_delay", 		value[0]);
	key_file_set_int("station_delay", "KT1_KP1_delay", 	value[1]);
	key_file_set_int("station_delay", "KP1_KFT1_delay", 	value[2]);
	key_file_set_int("station_delay", "KP1_KT2_delay", 	value[3]);
	key_file_set_int("station_delay", "KT2_KP2_delay", 	value[4]);
	key_file_set_int("station_delay", "KP2_KFT2_delay", 	value[5]);

	fpga_set_kick_delay(value);
}

void *sc_get_sys_delay_config(__u32 *len)
{
	if(len != NULL)
		*len = sizeof(struct SysDelayConfig);
	return &g_sys_delay_config;
}
/**************************************************************/


/**************************视频配置/查询************************/
struct VideoConfig	g_video_config =
{
		.head =
		{
				.type = 0x45530109,
				.length = sizeof(struct VideoConfig) - 4
		},
};

void sc_set_video_config(void *data)
{
	void *p = (struct NetInfoHead *)&g_video_config + 1;
	memcpy(p, data, sizeof(struct VideoConfig) - sizeof(struct NetInfoHead));

	key_file_set_int("video_channel1", "valid", g_video_config.video1_valid);
	key_file_set_string("video_channel1", "name", g_video_config.video1_name);
	key_file_set_int("video_channel2", "valid", g_video_config.video2_valid);
	key_file_set_string("video_channel2", "name", g_video_config.video2_name);

	//fpga操作
}

void *sc_get_video_config(__u32 *len)
{
	*len = sizeof(struct VideoConfig);
	return &g_video_config;
}
/**************************************************************/


/**************************视频需求配置/查询********************/
struct VideoRequirement	g_video_requirement[] =
{
		{
				.head =
				{
						.type = 0x4553010C,
						.length = sizeof(struct VideoRequirement) - 4
				},
				.video_id = 0
		},
		{
				.head =
				{
						.type = 0x4553010C,
						.length = sizeof(struct VideoRequirement) - 4
				},
				.video_id = 1
		}
};

void sc_set_video_requirement(void *data)
{
	__u32 id =  *(__u32 *)data;
	void *p = NULL;

	if(0 == id)
		p = (struct NetInfoHead *)&g_video_requirement[0] + 1;
	else if(1 == id)
		p = (struct NetInfoHead *)&g_video_requirement[1] + 1;
	else
		return;

	memcpy(p, data, sizeof(struct VideoRequirement) - sizeof(struct NetInfoHead));

	if(0 == id)
	{
		key_file_set_int("video_channel1", "height", g_video_requirement[id].height);
		key_file_set_int("video_channel1", "width", g_video_requirement[id].width);
		key_file_set_int("video_channel1", "bayer_fmt", g_video_requirement[id].bayer_fmt);
	}
	else if(1 == id)
	{
		key_file_set_int("video_channel2", "height", g_video_requirement[id].height);
		key_file_set_int("video_channel2", "width", g_video_requirement[id].width);
		key_file_set_int("video_channel2", "bayer_fmt", g_video_requirement[id].bayer_fmt);
	}

}

void *sc_get_video_requirement(__u32 id, __u32 *len)
{
	*len = sizeof(struct VideoRequirement);

	return &g_video_requirement[id];
}
/**************************************************************/


/********************FPGA功能配置查询**************************/
struct FPGAFuncConfig g_fpga_func_config =
{
		.head =
		{
				.type = 0x4553010D,
				.length = sizeof(struct FPGAFuncConfig) - 4
		},
};


void sc_set_fpga_func_config(__u32 *data)
{
	//剔除方式
	fpga_set_kick_mode(*data);
	g_fpga_func_config.kick_mode = *data;
	//过长连拍
	fpga_set_ol_shoot_mode(*(data+1));
	g_fpga_func_config.ol_shoot_mode = *(data+1);
	//第二工位
	fpga_enable_second_tube(*(data+2));
	g_fpga_func_config.second_station_en = *(data+2);
	//超频样品处理模式
	g_fpga_func_config.oc_sample_mode = *(data+3);

	//超频样品处理工位
	g_fpga_func_config.oc_sample_station = *(data+4);

	//反馈对管使能控制
	fpga_set_feedback_tube(*(data+5));
	g_fpga_func_config.feedback_tube_en = *(data+5);
//printf("feedback_tube_en = %x\n",*(data+5));
	key_file_set_int("function", "kick_mode", *data);
	key_file_set_int("function", "ol_shoot_mode", *(data+1));
	key_file_set_int("function", "second_station_en", *(data+2));
	key_file_set_int("function", "oc_sample_mode", *(data+3));
	key_file_set_int("function", "oc_sample_station", *(data+4));
	key_file_set_int("function", "feedback_tube_en", *(data+5));
}



void *sc_get_fpga_func_config(__u32 *len)
{
	if(len != NULL)
		*len = sizeof(struct FPGAFuncConfig);
	return &g_fpga_func_config;
}
/**************************************************************/

/********************超频阈值设置/查询***************************/
struct FPGAOverclockThreshold g_fpga_overclock_threshold =
{
		.head =
		{
				.type = 0x45530112,
				.length = sizeof(struct FPGAOverclockThreshold) - 4
		},
};

void sc_set_oc_th(void *data)
{
	__u32 value = *(__u32 *)data;

	g_fpga_overclock_threshold.value = value;
	fpga_set_overclock_threshold(value);

	key_file_set_int("function", "overclock_threshold", value);
}

void *sc_get_oc_th(__u32 *len)
{
	*len = sizeof(struct FPGAOverclockThreshold);
	return &g_fpga_overclock_threshold;
}
/**************************************************************/

/********************样品长度设置/查询***************************/
struct FPGASampleLength g_fpga_sample_length =
{
		.head =
		{
				.type = 0x45530117,
				.length = sizeof(struct FPGASampleLength) - 4
		},
};

void sc_set_sample_len(void *data)
{
	__u32 value = *(__u32 *)data;

	g_fpga_sample_length.value = value;
	fpga_set_sample_length(value);

	key_file_set_int("function", "sample_length", value);
}

void  *sc_get_sample_len(__u32 *len)
{
	*len = sizeof(struct FPGASampleLength);
	return &g_fpga_sample_length;
}
/**************************************************************/


/********************算法参数设置/查询***************************/

struct AlgoParamsConfig g_algo_params_config;

void sc_init_algo()
{
	g_algo_params_config.fd = open("algo_params.dat", O_RDWR);
	if(g_algo_params_config.fd < 0)
		ERROR("open");

	g_algo_params_config.algo_params = mmap(NULL, sizeof(struct AlgoParams),
			PROT_READ | PROT_WRITE, MAP_SHARED, g_algo_params_config.fd, 0);

	g_algo_params_config.algo_params->head.type = 0x4553010F;
	g_algo_params_config.algo_params->head.length  = sizeof(struct AlgoParams)-4;

	g_algo_params_config.update = 0;
}

void sc_set_algo_params(void *data)
{
	extern sem_t g_sem_algo_params_update;
	memcpy(&(g_algo_params_config.algo_params->params_grp_amount), data, sizeof(struct AlgoParams)-8);
	msync(g_algo_params_config.algo_params, sizeof(struct AlgoParams), MS_INVALIDATE|MS_SYNC);
//	printf("1111111\n");
	sem_post(&g_sem_algo_params_update);
}

void *sc_get_algo_params(__u32 *len)
{
	if(len != NULL)
		*len = sizeof(struct AlgoParams);
	return g_algo_params_config.algo_params;
}
/************************************************************/


/********************样品判决模式设置/查询********************/
struct SampleJudgeMode g_sample_judge_mode =
{
		.head =
		{
				.type = 0x45530115,
				.length = sizeof(struct SampleJudgeMode) - 4
		},
		.mode = 0,	/* 默认为算法模式 */
};

void sc_set_sample_judge_mode(void *data)
{
	__u32 value;
	value = *(__u32 *)data;
	g_sample_judge_mode.mode = value;
	key_file_set_int("working_mode", "judge_mode", value);
}

void *sc_get_sample_judge_mode(__u32 *len)
{
	if(len != NULL)
		*len = sizeof(struct SampleJudgeMode);
	return &g_sample_judge_mode;
}
/************************************************************/


/********************显示屏旋转角度********************/
struct SysScreenAngle g_sys_screen_angle =
{
		.angle = 0,	/* 默认为0度 */
};

void sc_set_sys_screen_angle(void *data)
{
	g_sys_screen_angle.angle = *(__u32 *)data;
	key_file_set_int("interface", "screen_angle", *(__u32 *)data);
}

void *sc_get_sys_screen_angle(__u32 *len)
{
	if(len != NULL)
		*len = sizeof(struct SysScreenAngle);
	return &g_sys_screen_angle;
}
/************************************************************/


/********************图像发送模式设置/查询********************/
struct ImageSendMode g_image_send_mode =
{
		.head =
		{
				.type = 0x45530116,
				.length = sizeof(struct ImageSendMode) - 4
		},
		.mode = 0,	/* 禁止分组内图像上传 */
};
void sc_set_img_send_mode(void *data)
{
	__u32 value;
	value = *(__u32 *)data;
	g_image_send_mode.mode = value;
	key_file_set_int("working_mode", "img_send_mode", value);
}

void *sc_get_img_send_mode(__u32 *len)
{
	if(len != NULL)
		*len = sizeof(struct ImageSendMode);
	return &g_image_send_mode;
}
/************************************************************/









/**************************统计信息****************************/
struct SysStatisticsInfo g_sys_sysstateinfo_config =
{
		.head =
		{
				.type = 0x45530105,
				.length = sizeof(struct SysStatisticsInfo) - 4
		},
		.sys_temp_info.camera_temp[0] = -1,
		.sys_temp_info.camera_temp[1] = -1,
};

void *sc_get_sys_state_info_config(__u32 *len)
{
	*len = sizeof(struct SysStatisticsInfo);
	return &g_sys_sysstateinfo_config;
}

/**************************************************************/







/**************************反馈信息配置*********************/
struct FeedBackInfo feedbackinfo[10];
static __u32 feedbackindex = 0;

void sc_feedbackinfo_init()
{
	__u32 i;
	for(i=0;i<10;i++)
	{
		feedbackinfo[i].head.type = 0x4553010E;
		feedbackinfo[i].head.length = sizeof(struct FeedBackInfo) - 4;
	}
}

void *sc_get_feedbackinfo(__u32 val, __u32 *len)
{
	struct FeedBackInfo *info = &feedbackinfo[feedbackindex];

	feedbackindex++;
	if(feedbackindex > 9)
		feedbackindex= 0;

	info->type 	= (val&0xFFFF0000)>>16;
	info->val 		= val&0xFFFF;
	info->data 	= 0;

	*len = sizeof(struct FeedBackInfo);
	return info;
}
/**************************************************************/

struct CAM_PROTOCOL_FROM cam1_protocol[10];
static __u32 cam1protocolindex = 0;

void *sc_get_cam1_protocol(void)
{
	struct CAM_PROTOCOL_FROM *info = &cam1_protocol[cam1protocolindex];

	cam1protocolindex++;
	if(cam1protocolindex > 9)
		cam1protocolindex = 0;

	return info;
}

struct CAM_PROTOCOL_FROM cam2_protocol[10];
static __u32 cam2protocolindex = 0;

void *sc_get_cam2_protocol(void)
{
	struct CAM_PROTOCOL_FROM *info = &cam2_protocol[cam2protocolindex];

	cam2protocolindex++;
	if(cam2protocolindex > 9)
		cam2protocolindex = 0;

	return info;
}


void sc_conf_load()
{
	__s32 err = 0;

	/* 打开系统配置文件 */
	err = key_file_open("system.conf");
	if (err == -1)
		ERROR("key_file_open");
}


void sc_conf_save()
{
	key_file_save("system.conf");
	sync();
}



static __s32 g_emulator_state = 0;
void sc_system_init()
{
	char version[32];

	sc_init_algo();
	__u16 reg_val;

	/* FPGA版本 */
	fpga_get_version(version);
	sc_set_fpga_version(version);

	/* 主板版本 */
	key_file_get_string(version,"system_sn",	"name", "unknow");
	sc_set_sys_sn(version);

	/* 配置拍照延时、踢废延时 */
	g_sys_delay_config.TT_C1_delay = key_file_get_int("shoot_delay",		"TT_C1_delay", 1000);
	g_sys_delay_config.C1_C2_delay = key_file_get_int("shoot_delay",		"C1_C2_delay", 500);

	g_sys_delay_config.C1_KT1_delay 		= key_file_get_int("station_delay",	"C1_KT1_delay", 1000);
	g_sys_delay_config.KT1_KP1_delay 	= key_file_get_int("station_delay",	"KT1_KP1_delay", 1000);
	g_sys_delay_config.KP1_KFT1_delay 	= key_file_get_int("station_delay",	"KP1_KFT1_delay", 1000);
	g_sys_delay_config.KP1_KT2_delay 	= key_file_get_int("station_delay",	"KP1_KT2_delay", 1000);
	g_sys_delay_config.KT2_KP2_delay 	= key_file_get_int("station_delay",	"KT2_KP2_delay", 1000);
	g_sys_delay_config.KP2_KFT2_delay 	= key_file_get_int("station_delay",	"KP2_KFT2_delay", 1000);

	fpga_set_shoot_delay(&(g_sys_delay_config.TT_C1_delay));
	fpga_set_kick_delay(&(g_sys_delay_config.C1_KT1_delay));


	/* 工作模式*/
	g_sample_judge_mode.mode 		= key_file_get_int("working_mode", "judge_mode", 0);
	g_image_send_mode.mode 			= key_file_get_int("working_mode", "img_send_mode", 0);

	/*  功能配置 */
	g_fpga_func_config.kick_mode							=	key_file_get_int("function", "kick_mode",				0);
	g_fpga_func_config.ol_shoot_mode				=	key_file_get_int("function", "ol_shoot_mode",					1);
	g_fpga_func_config.second_station_en	=	key_file_get_int("function", "second_station_en",			0);
	g_fpga_func_config.oc_sample_mode			=	key_file_get_int("function", "oc_sample_mode",				0);
	g_fpga_func_config.oc_sample_station	=	key_file_get_int("function", "oc_sample_station",				1);
	g_fpga_func_config.feedback_tube_en		=	key_file_get_int("function", "feedback_tube_en",				0);

	fpga_set_kick_mode(g_fpga_func_config.kick_mode	);
	fpga_set_ol_shoot_mode(g_fpga_func_config.ol_shoot_mode);
	fpga_enable_second_tube(g_fpga_func_config.second_station_en);
	fpga_set_feedback_tube(g_fpga_func_config.feedback_tube_en);

	g_fpga_sample_length.value								=	key_file_get_int("function", "sample_length",				1000);
	g_fpga_overclock_threshold.value				=	key_file_get_int("function", "overclock_threshold",	680);

	fpga_set_sample_length(g_fpga_sample_length.value);
	fpga_set_overclock_threshold(g_fpga_overclock_threshold.value);

    /* 信号设置 */
	g_sys_signal_config.polar		= key_file_get_int("signal", "polar",	0);

	g_sys_signal_config.kp1_width	= key_file_get_int("signal", "kp1_width",	0);
	g_sys_signal_config.kp2_width	= key_file_get_int("signal", "kp2_width",	0);

	g_sys_signal_config.tt_width	= key_file_get_int("signal", "tt_width",	0);
	g_sys_signal_config.kt_width	= key_file_get_int("signal", "kt_width",	0);
	g_sys_signal_config.kft_width	= key_file_get_int("signal", "kft_width",	0);
	g_sys_signal_config.extwheel_width	= key_file_get_int("signal", "extwheel_width",	0);

	fpga_set_signal_polar(g_sys_signal_config.polar);

	fpga_set_out_signal_width((__u32*)&g_sys_signal_config + 3);
	fpga_set_in_signal_width((__u32*)&g_sys_signal_config + 11);

	/* 视频通道使能配置 */
	g_video_config.video1_valid = key_file_get_int("video_channel1", "valid",		0);
	g_video_config.video2_valid = key_file_get_int("video_channel2", "valid",		0);
	key_file_get_string(g_video_config.video1_name,"video_channel1", "name", "unknow");
	key_file_get_string(g_video_config.video2_name,"video_channel2", "name", "unknow");

	fpga_set_cam_trig(g_video_config.video1_valid, g_video_config.video2_valid);

	/* 视频通道需求配置 */
	g_video_requirement[0].width			= key_file_get_int("video_channel1", "width",		752);
	g_video_requirement[0].height 		= key_file_get_int("video_channel1", "height",		480);
	g_video_requirement[0].bayer_fmt	= key_file_get_int("video_channel1", "bayer_fmt",		0);
	g_video_requirement[1].width			= key_file_get_int("video_channel2", "width",		752);
	g_video_requirement[1].height		= key_file_get_int("video_channel2", "height",		480);
	g_video_requirement[1].bayer_fmt	= key_file_get_int("video_channel2", "bayer_fmt",		0);
	fpga_set_image_size(0, g_video_requirement[0].width, g_video_requirement[0].height);
	fpga_set_image_size(1, g_video_requirement[1].width, g_video_requirement[1].height);

	/* 反馈队列初始化 */
	sc_feedbackinfo_init();

	g_emulator_state = fpga_get_emulator_state();
}


__s32 check_emulator_state(void)
{
	return g_emulator_state;
}


static __u32 g_sys_reset_flag = 0;
__u32 check_sys_reset_flag(void)
{
	return g_sys_reset_flag;
}


void sc_system_reset()
{
	g_sys_reset_flag = 1;
	FPGA_WRITE16(fpga_base, FPGA_SHK_REG, 0x101);
	fpga_reset();
	sample_count_reset();
	kick_count_reset();
	send_reset_cmd();
	sem_wait(&gsem_reject_reset);
	FPGA_WRITE16(fpga_base, FPGA_SHK_REG, 0x1010);
	g_sys_reset_flag = 0;
}


void sc_handshake_init(__s32 argc)
{
	static __u32 vid1_valid = 0;
	static __u32 vid2_valid = 0;
	__u16 reg_value;
	__u16 tmp;

	if (argc == 1)
	{
		vid1_valid = key_file_get_int("video_channel1", "valid",	0);
		vid2_valid = key_file_get_int("video_channel2", "valid",	0);

		if(vid1_valid)
		{
			while(1)
			{
				reg_value = FPGA_READ16(fpga_base ,FPGA_CAMSTATE_REG);
				if(reg_value & 0x1)
				{
					break;
				}
				usleep(1000);

			}
		}

		if(vid2_valid)
		{
			while(1)
			{
				reg_value = FPGA_READ16(fpga_base ,FPGA_CAMSTATE_REG);
				if(reg_value & 0x2)
				{
					break;
				}
				usleep(1000);
			}
		}

		fpga_set_camera_state((!vid1_valid),(!vid2_valid));

		printf("camera handshake done.\n");
	}
	/* 与FPGA握手 */
	else if(argc >1)
	{
		fpga_set_camera_state(1, 1);//1无头，0有头
	}

	fpga_start();
}


#endif
