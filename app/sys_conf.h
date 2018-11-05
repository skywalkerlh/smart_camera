/*
 * sys_conf.h
 *
 *  Created on: 2015年5月18日
 *      Author: work
 */

#ifndef SYSTEM_CONFIGURE_H_
#define SYSTEM_CONFIGURE_H_

//#include "v4l2.h"
#include "net_context.h"


struct AlgoParams
{
	struct NetInfoHead head;
	__s32  params_grp_amount;	/* 参数分组编号 */
	__s32 valid_params_grp;		/* 有效分组编号 */
	__u32 reckon_time; 				/* 算法时间估计值 */
	__u8   params[1024];
};

struct AlgoParamsConfig
{
	struct AlgoParams *algo_params;
	__u32 update;/*参数更新标志*/
	__s32 fd; /* 算法参数配置文件 */
};


struct DSPArgsInfo
{
	struct SampleImageGroup *sample_image_group;
	__u32 symbol;
	__u32 img_upload_flag;
};

#pragma pack(2)
struct SysDateTime
{
	__u16 year;
	__u16 month;
	__u16 day;
	__u16 hour;
	__u16 minute;
	__u16 second;
}SysDateTime;


struct SystemTempInfo
{
	//__u16 main_board_temp;
	__s16 mpu_temp;
	__s16 gpu_temp;
	__s16 core_temp;
	__s16 iva_temp;
	__s16 dspeve_temp;
	__s16 camera_temp[2];
};

struct SampleStatisticsInfo
{
	__u32 sample_amount;
	__u32 aver_flux;
	__u32 peak_flux;
	__u32 dsp1_timeout_amount;
	__u32 dsp2_timeout_amount;
	__u32 dsp1_reset_amount;
	__u32 dsp2_reset_amount;
	__u32 camera_overclock_amount;
	__u32 process_overclock_amount;
	__u32 overlength_amount;
	__u32 pos1_lost_amount;
	__u32 pos2_lost_amount;
};

struct AlgostatisticsInfo
{
	__u32 counter_1;
	__u32 counter_2;
	__u32 counter_3;
	__u32 counter_4;
	__u32 counter_5;
	__u32 counter_6;
	__u32 counter_7;
	__u32 counter_8;
};

struct SysStatisticsInfo
{
	struct NetInfoHead head;
	struct SysDateTime sys_date_time;
	__u32	sys_work_time;
	struct SystemTempInfo sys_temp_info;
	struct SampleStatisticsInfo sample_stat_info;
	struct AlgostatisticsInfo algo_stat_info;
};
#pragma pack()


/* 系统判决模式 */
struct SampleJudgeMode
{
	struct NetInfoHead head;
	/*
	*  0:算法判决模式
	*  1:测试模式1
	*  2:测试模式2
	*	N:测试模式N
	*/
	__u32 mode;
	__u8 params[8]; /* 测试参数 */
};

/* 屏幕角度 */
struct SysScreenAngle
{
//	struct NetInfoHead head;
	__u32 angle;	/*
										 * 0:0
										 * 1:90
										 * 2:180
										 * 3:270
										 */

};

/* 图像发送模式 */
struct ImageSendMode
{
	struct NetInfoHead head;
	__u32 mode;	/* 发送使能控制
	                         *  0:禁止分组内图像上传
	                         *  1:允许分组内所有图像上传
	                         *  2:允许分组内判怀图像上传
	                         */
	__u32 reserved;
};

struct SysSignalConfig
{
	struct NetInfoHead head;
	__u32 polar;/* 信号极性，定义取决于FPGA */
	__u32 kp1_width;
	__u32 kp2_width;
	__u32 out_width3;
	__u32 out_width4;
	__u32 out_width5;
	__u32 out_width6;
	__u32 out_width7;
	__u32 out_width8;
	__u32 tt_width;
	__u32 kt_width;
	__u32 kft_width;
	__u32 extwheel_width;
	__u32 in_width5;
	__u32 in_width6;
	__u32 in_width7;
	__u32 in_width8;
};

struct FPGAFuncConfig
{
	struct NetInfoHead head;
	__u32 kick_mode;											/* 剔除模式 */
	__u32 ol_shoot_mode;								/* 过长拍摄模式 */
	__u32 second_station_en;					/* 第2对管使能控制 */
	__u32 oc_sample_mode;							/* 超频样品处理模式 */
	__u32 oc_sample_station;					/* 超频样品处理工位*/
	__u32 feedback_tube_en;						/* 反馈对管使能*/

};

struct FPGAOverclockThreshold
{
	struct NetInfoHead head;
	__u32 value;
};

struct FPGASampleLength
{
	struct NetInfoHead head;
	__u32 value;
};

struct SysDelayConfig
{
	struct NetInfoHead head;
	__u32 TT_C1_delay;
	__u32 C1_C2_delay;

	__u32 C1_KT1_delay;
	__u32 KT1_KP1_delay;
	__u32 KP1_KFT1_delay;
	__u32 KP1_KT2_delay;
	__u32 KT2_KP2_delay;
	__u32 KP2_KFT2_delay;

};

struct MainBoardVersion
{
	struct NetInfoHead head;
	char system_name[32];
	char mpu_ver[32];
	char dsp1_ver[32];
	char dsp2_ver[32];
	char kick_ver[32];
	char ui_ver[32];

	char reserved1[32];
	char reserved2[32];

	char fpga_ver[32];
	char system_sn[32];

	char reserved3[32];
	char reserved4[32];

	__u32 self_checking;
};

struct ProgramUpdateProgress
{
	struct NetInfoHead head;
	__u32 file_type;				/* 程序文件类别 */
	__u32 file_len;			/* 程序文件总长度 */
	__u32 updated_len;		/* 已更新长度  */
	__u32 status;			/* 更新状态 */
};

struct DisposeStation
{
	struct NetInfoHead head;
	__u32 enable_mask;
	char station1_name[32];
	char station2_name[32];
	char station3_name[32];
	char station4_name[32];
	char station5_name[32];
	char station6_name[32];
	char station7_name[32];
	char station8_name[32];
};

struct VideoConfig
{
	struct NetInfoHead head;
	__u32 video1_valid;
	char video1_name[32];
	__u32 video2_valid;
	char video2_name[32];

};

struct VideoRequirement
{
	struct NetInfoHead head;
	__u32 video_id;
	__u32 width;
	__u32 height;
	__u32 bayer_fmt;
};

struct FeedBackInfo // modified by LH
{
	struct NetInfoHead head;
	__u32 type;
	__u32 val; // 返回值
	__u32 data;
}FeedBackInfo;


struct CAM_PROTOCOL_FROM
{
	__u16 head;
	__u16 length;
	__u16 data[2048];
	struct NetInfoHead  cam_info_head;
}CAM_PROTOCOL_FROM;


void sc_set_algo_params(void *data);
void *sc_get_algo_params(__u32 *len);

void sc_set_sample_judge_mode(void *data);
void *sc_get_sample_judge_mode(__u32 *len);

//void sc_set_sys_working_mode(void *data);
//void *sc_get_sys_working_mode(__u32 *len);

void sc_set_img_send_mode(void *data);
void *sc_get_img_send_mode(__u32 *len);

void sc_set_fpga_func_config(__u32 *data);

void sc_set_2nd_tube_enable(void *data);
void sc_set_sample_len(void *data);
void *sc_get_sample_len(__u32 *len);
void sc_set_oc_th(void *data);
void *sc_get_oc_th(__u32 *len);
void *sc_get_fpga_func_config(__u32 *len);

void sc_set_signal_polar(void *data);
void sc_set_out_signal_width(void *data);
void sc_set_in_signal_width(void *data);
void *sc_get_signal_config(__u32 *len);

void sc_set_shoot_delay(void *data);
void sc_set_kick_delay(void *data);
void *sc_get_sys_delay_config(__u32 *len);

void sc_set_dsp1_version(char *version);
void sc_set_dsp2_version(char *version);
void *sc_get_main_board_version(__u32 *len);
void sc_set_ui_version(char *version);

void sc_set_video_config(void *data);
void * sc_get_video_config(__u32 *len);

void sc_set_video_requirement(void *data);
void * sc_get_video_requirement(__u32 id, __u32 *len);

void sc_set_dispose_station(void *data);
void *sc_get_dispose_station(__u32 *len);

void sc_conf_load();
void sc_conf_save();
void sc_system_init();

void *sc_get_feedbackinfo(__u32 val, __u32 *len);
void sc_feedbackinfo_init();

void *sc_get_cam1_protocol(void);
void *sc_get_cam2_protocol(void);

void *sc_get_sys_state_info_config(__u32 *len);

void sc_set_sys_sn(char *version);

void sc_set_sys_screen_angle(void *data);

void sc_handshake_init();
void sc_system_reset();
__u32 check_sys_reset_flag(void);
__s32 check_emulator_state(void);

#endif /* SYS_CONF_H_ */
