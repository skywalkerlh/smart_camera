/*
 * fpga.h
 *
 *  Created on: 2014年9月17日
 *      Author: work
 */

#ifndef FPGA_H_
#define FPGA_H_

#include <linux/types.h>


#ifndef FPGA_C_
extern __u32 *fpga_base;
#endif


#define FPGA_READ16(base,offset)  (*((unsigned short*)base + offset))
#define FPGA_READ32(base,offset)    (*((unsigned short*)base  + offset)  \
		                                           | (*((unsigned short*)base  + offset +1) << 16))
#define FPGA_WRITE16(base,offset, val)  (*((unsigned short*)base + offset) = val)

#define FPGA_WRITE32(base,offset,val)    *((unsigned short*)base + offset) = val;  \
		                                            *((unsigned short*)base + offset +1) = (val>>16)

typedef struct FPGA_DEBUG_INFO
{
	__u32 ext_tri_num;
	__u32 sin_tri_num;
	__u32 ch1_trig_amount;
	__u32 ch2_trig_amount;
	__u32 ch1_recv_frames;
	__u32 ch1_img_height;
	__u32 ch1_img_width;
	__u32 ch2_recv_frames;
	__u32 ch2_img_height;
	__u32 ch2_img_width;

}FPGA_DEBUG_INFO;

__u64 fpga_get_current_code();
__u64 fpga_get_vp5_shoot_code();
__u64 fpga_get_vp6_shoot_code();

__u32 fpga_get_shoot_count();

__s32  fpga_reset(void);
__s32  fpga_stop(void);

__s32  fpga_set_shoot_delay(__u32 *val);
__s32  fpga_set_kick_delay(__u32 *val);


__s32  fpga_set_signal_polar(__u16 val);
__s32  fpga_set_signal_width(__u32 *val);


__s32  fpga_set_cam_trig(__u32 video1_valid, __u32 video2_valid);

__s32  fpga_set_wheel_code_src(__u32 val);
__s32  fpga_enable_second_tube(__u32 val);
__s32  fpga_set_ol_shoot_mode(__u32 val);

__s32  fpga_set_working_mode(__u16 mode);
__s32  fpga_update_program(__u32 file_len, __u32 unit_len, char* data);

//__s32  fpga_set_image_format(__u32 video_id, __u32 width, __u32 height);

__s32  fpga_get_version(char *asc_ver);


__s32  fpga_write_lvds1_txfifo_pre();
__s32  fpga_write_lvds1_txfifo_fni();
__s32  fpga_write_lvds1_txfifo(__u16 *data, __u16 len);
__s32  fpga_write_lvds2_txfifo_pre();
__s32  fpga_write_lvds2_txfifo_fni();
__s32  fpga_write_lvds2_txfifo(__u16 *data, __u16 len);
void fpga_check_lvds1_rxfifo();
void fpga_check_lvds2_rxfifo();
__s32  fpga_read_lvds1_rxfifo(__u16 *data, __u16 len);
__s32  fpga_read_lvds2_rxfifo(__u16 *data, __u16 len);

void fpga_lvds1_send(__u16 *data, __u16 len);
__u32 fpga_lvds1_recv(__u16 *data, __u16 *len);
void fpga_lvds2_send(__u16 *data, __u16 len);
__u32 fpga_lvds2_recv(__u16 *data, __u16 *len);


__s32  fpga_set_sim_trig(__u16 *data);

__s32  fpga_start(void);


__s32  fpga_set_sample_length(__u32 len);
__u32  fpga_get_sample_length();

__s32  fpga_set_overclock_threshold(__u16 val);
__u32  fpga_get_overclock_threshold();
__s32  fpga_set_oc_sample_mode(__u16 mode);

__u32  fpga_get_oc_amount();

__u32  fpga_get_ol_amount();
__u32  fpga_get_aver_flow();
__u32  fpga_get_max_flow();
__s32  fpga_get_emulator_state();

FPGA_DEBUG_INFO *fpga_get_debug_info(__u32 *len);

__s32  fpga_set_image_size(__u32 id, __u16 width, __u16 height);

__s32  fpga_set_camera_state(__u32 id,  __u32 state);

__s32 fpga_set_out_signal_width(__u32 *val);
__s32 fpga_set_in_signal_width(__u32 *val);


__s32 fpga_set_kick_mode(__u32 val);
__s32 fpga_set_feedback_tube(__u32 val);


	FPGA_DEBUG_INFO * (*get_debug_info)();

void fpga_context_init();
void sig_handler(int signo);
void fpga_burn(__u8 *data);

/* FPGA寄存器地址偏移说明*/

/*	系统管理*/
#define FPGA_VER_REG          0x01  //版本号
#define FPGA_RST_REG          0x02		//系统复位
#define FPGA_SHK_REG          0x03		//系统握手
#define FPGA_SYSSLT_REG       0x04		//系统选择
#define FPGA_SIMSLT_REG       0x05		//模拟系统选择
#define FPGA_DISTANCE_REG     0x06		//模拟系统选择扩展，样品间距
#define FPGA_TRIG_REG         0x07		//Trig拉高时长
#define FPGA_DSPIN_REG        0x08		//GPIO测试_DSPIN
#define FPGA_DSPOUT_REG       0x09		//GPIO测试_DSPOUT
#define FPGA_CAMSTATE_REG     0x0A		//CAM启动状态
#define FPGA_SIMLINK_REG      0x0B		//仿真器连接状态

#define FPGA_CAMTYPE_REG      0x22		//相机类型
#define FPGA_FUNCSLT_REG      0x3C		//功能选择
#define FPGA_POLAR_REG      		0x23		//极性选择

#define FPGA_STA_REG          0x20		//状态寄存器
#define FPGA_CLR_REG          0x21		//清除寄存器

#define FPGA_ISREN_REG        0x0F   //中断屏蔽寄存器
#define FPGA_ISRSTATE_REG1    0x80   //中断状态寄存器1
#define FPGA_ISRCLR_REG1      0x81   //中断清除寄存器1
#define FPGA_ISRSTATE_REG2    0x82   //中断状态寄存器2
#define FPGA_ISRCLR_REG2      0x83   //中断清除寄存器2


/*	功能配置*/
#define FPGA_TRIGLENL_REG     			0x84		//样品触发长度低16位
#define FPGA_TRIGLENH_REG     			0x85		//样品触发长度高16位
#define FPGA_FREQLIMITL_REG   			0x86		//超频限制低16位(只能是相机1下方有效)
#define FPGA_FREQLIMITH_REG   			0x87		//超频限制高16位

#define FPGA_CAMTRIGPOSL_REG1 			0x25		//粗调相机触发位置低16位1
#define FPGA_CAMTRIGPOSH_REG1				0x26		//粗调相机触发位置高16位1
#define FPGA_CAMTRIGPOSL_REG2 			0x88		//粗调相机触发位置低16位2
#define FPGA_CAMTRIGPOSH_REG2   		0x89 	//粗调相机触发位置高16位2
#define FPGA_KICKEXECPOSL_REG1    	0x3E		//粗调踢废执行位置低16位1，踢废对管1到踢废机构1的距离
#define FPGA_KICKEXECPOSH_REG1    	0x3F		//粗调踢废执行位置高16位1
#define FPGA_KICKBACKPOSL_REG1    	0x2d		//粗调踢废反馈位置低16位1，踢废机构1到踢废反馈对管1的距离
#define FPGA_KICKBACKPOSH_REG1    	0x2E		//粗调踢废反馈位置高16位1
#define FPGA_KICKEXECPOSL_REG2    	0x76		//粗调踢废执行位置低16位2，踢废对管2到踢废机构2的距离
#define FPGA_KICKEXECPOSH_REG2    	0x77		//粗调踢废执行位置高16位2
#define FPGA_KICKBACKPOSL_REG2  		0x8A		//粗调踢废反馈位置低16位2，踢废机构2到踢废反馈对管2的距离
#define FPGA_KICKBACKPOSH_REG2    	0x8B		//粗调踢废反馈位置高16位2
#define FPGA_KICKTRIGPOSL_REG1    	0x27		//粗调踢废触发位置低16位1，相机1到踢废对管1的距离
#define FPGA_KICKTRIGPOSH_REG1    	0x28		//粗调踢废触发位置高16位1
#define FPGA_KICKTRIGPOSL_REG2    	0x8C		//粗调踢废触发位置低16位2，踢废机构1到踢废对管2的距离
#define FPGA_KICKTRIGPOSH_REG2    	0x8D		//粗调踢废触发位置高16位2

#define FPGA_TRIGFIRDIS_REG       	0x24		//触发对管滤波宽度
#define FPGA_KICKFIRDIS_REG1      	0x2F		//剔废对管滤波宽度1
#define FPGA_KICKFIRDIS_REG2      	0x74		//剔废对管滤波宽度2
#define FPGA_WHEELFIRDIS_REG    		0x30		//外码盘滤波宽度
#define FPGA_CAMDIS_REG1       			0x2A		//触发相机信号脉宽1
#define FPGA_CAMDIS_REG2       			0x32		//触发相机信号脉宽2
#define FPGA_KICKDIS_REG1      			0x2B		//踢废信号脉宽1
#define FPGA_KICKDIS_REG2      			0x29		//踢废信号脉宽2
#define FPGA_OUT3DIS_REG      			0x31		//OUT3信号脉冲宽度

//////////////////////////////////////////////////////////////////////////////////////


/*	调试信息寄存器*/
#define FPGA_SAMPLENL_REG     					0x3a		//样品长度低16位
#define FPGA_SAMPLENH_REG     					0x3b 	//样品长度高16位
#define FPGA_OVERFREQALML_REG  					0x8e		//超频警告低16位
#define FPGA_OVERFREQALMH_REG  					0x8f		//超频警告高16位
#define FPGA_TRIGDISL_REG     					0x70		//两次触发码盘值差低16位
#define FPGA_TRIGDISH_REG     					0x71		//两次触发码盘值差高16位
#define FPGA_TRIGDIFL_REG     					0x72		//两次触发码盘差最小值低16位
#define FPGA_TRIGDIFH_REG     					0x73		//两次触发码盘差最小值高16位
#define FPGA_LINESPD_REG  								0x1e		//线速度
#define FPGA_OVERLONGNUML_REG  					0x14		//过长总数低16位
#define FPGA_OVERLONGNUMH_REG  					0x14		//过长总数高16位
#define FPGA_OVERFREQNUML_REG  					0x34		//超频总数低16位
#define FPGA_OVERFREQNUMH_REG  					0x35		//超频总数高16位

#define FPGA_WHEELCOUNTER1_REG  				0x96		//码盘计数1：00～15位
#define FPGA_WHEELCOUNTER2_REG  				0x97		//码盘计数2：16～31位
#define FPGA_WHEELCOUNTER3_REG  				0x98		//码盘计数3：32～47位
#define FPGA_WHEELCOUNTER4_REG  				0x99		//码盘计数4：48～63位

#define FPGA_VP5TRIGCAMWHEEL1_REG  				0x800		//VP5触发相机时刻码盘计数1：00～15位
#define FPGA_VP5TRIGCAMWHEEL2_REG  				0x880		//VP5触发相机时刻码盘计数2：16～31位
#define FPGA_VP5TRIGCAMWHEEL3_REG  				0x900		//VP5触发相机时刻码盘计数3：32～47位
#define FPGA_VP5TRIGCAMWHEEL4_REG  				0x980 	//VP5触发相机时刻码盘计数4：48～63位

#define FPGA_VP6TRIGCAMWHEEL1_REG  				0xb00		//VP6触发相机时刻码盘计数1：00～15位
#define FPGA_VP6TRIGCAMWHEEL2_REG  				0xb80		//VP6触发相机时刻码盘计数2：16～31位
#define FPGA_VP6TRIGCAMWHEEL3_REG  				0xc00		//VP6触发相机时刻码盘计数3：32～47位
#define FPGA_VP6TRIGCAMWHEEL4_REG  				0xc80 	//VP6触发相机时刻码盘计数4：48～63位


#define FPGA_FACTTRIGNUML_REG     			0x6a		//实际触发个数低16位  ,相机1处，超频中断和相机触发个数之和
#define FPGA_FACTTRIGNUMH_REG     			0x6b		//实际触发个数高16位
#define FPGA_SIMTRIGNUML_REG      			0x6c		//模拟触发个数低16位
#define FPGA_SIMTRIGNUMH_REG      			0x6d		//模拟触发个数高16位
#define FPGA_TRIGCAMCOUNTERL_REG1  		0x68		//触发相机计数1低16位
#define FPGA_TRIGCAMCOUNTERH_REG1 			0x69		//触发相机计数1高16位
#define FPGA_TRIGCAMCOUNTERL_REG2  		0x37		//触发相机计数2低16位
#define FPGA_TRIGCAMCOUNTERH_REG2 			0x38		//触发相机计数2高16位
#define FPGA_KICKTRIGCOUNTERL_REG1  	0x7a		//剔除对管计数1低16位
#define FPGA_KICKTRIGCOUNTERH_REG1 		0x7b		//剔除对管计数1高16位
#define FPGA_KICKTRIGCOUNTERL_REG2  	0x7c		//剔除对管计数2低16位
#define FPGA_KICKTRIGCOUNTERH_REG2 		0x7d		//剔除对管计数2高16位

#define FPGA_FACTKICKCOUNTERL_REG1  	0x12		//实际剔除计数1低16位
#define FPGA_FACTKICKCOUNTERH_REG1 		0x13		//实际剔除计数1高16位
#define FPGA_FACTKICKCOUNTERL_REG2  	0x10 	//实际剔除计数2低16位
#define FPGA_FACTKICKCOUNTERH_REG2 		0x11		//实际剔除计数2高16位

#define FPGA_VP5OUTFRAMEL_REG     			0x51		//cmos输出帧数,vp5低16位
#define FPGA_VP5OUTFRAMEH_REG     			0x52		//cmos输出帧数,vp5高16位
#define FPGA_VP5OUTLINENUM_REG       	0x50		//输入行数,vp5,图像高度
#define FPGA_VP5OUTPIXELMUNL_REG  			0x53		//有效像素个数低16位,vp5
#define FPGA_VP5OUTPIXELMUNH_REG  			0x54		//有效像素个数高16位,vp5

#define FPGA_VP6OUTFRAMEL_REG     			0x56		//cmos输出帧数,vp6低16位
#define FPGA_VP6OUTFRAMEH_REG     			0x57		//cmos输出帧数,vp6高16位
#define FPGA_VP6OUTLINENUM_REG       	0x55		//输入行数,vp6,图像高度
#define FPGA_VP6OUTPIXELMUNL_REG  			0x58		//有效像素个数低16位,vp6
#define FPGA_VP6OUTPIXELMUNH_REG  			0x59		//有效像素个数高16位,vp6


/*	图像管理*/

/*	Vp5操作寄存器*/

#define FPGA_VP5WRTXFLG_REG          	0x62			//	Vp5DSP写txfifo标识
#define FPGA_VP5CLRTXFLG_REG         	0x63			//	Vp5DSP清空txfifo标识
#define FPGA_VP5RXDATANUM_REG        	0x64			//	Vp5Rxfifo数据个数
#define FPGA_VP5RDRXFLG_REG          	0x65			//	Vp5Rxfifo写入完毕标识
#define FPGA_VP5VBLNKINVTIME_REG				0x44			// Vp5,vblnk无效时间
#define FPGA_VP5ACTVIDINVTIME_REG 			0x45			// Vp5,actvid无效时间
#define FPGA_VP5PIXELMUN_REG       		0x46			// Vp5,像素个数/行
#define FPGA_VP5LINENUM_REG        		0x47			// Vp5,行数
#define FPGA_VP5DELAYNUM_REG        	0x4C			// Vp5延迟计数

/*	Vp6操作寄存器*/

#define FPGA_VP6WRTXFLG_REG          	0xA2			//	Vp6DSP写txfifo标识
#define FPGA_VP6CLRTXFLG_REG         	0xA3			//	Vp6DSP清空txfifo标识
#define FPGA_VP6RXDATANUM_REG        	0xA4			//	Vp6Rxfifo数据个数
#define FPGA_VP6RDRXFLG_REG          	0xA5			//	Vp6Rxfifo写入完毕标识
#define FPGA_VP6VBLNKINVTIME_REG				0x48			// Vp6,vblnk无效时间
#define FPGA_VP6ACTVIDINVTIME_REG 			0x49			// Vp6,actvid无效时间
#define FPGA_VP6PIXELMUN_REG       		0x4A			// Vp6,像素个数/行
#define FPGA_VP6LINENUM_REG        		0x4B			// Vp6,行数
#define FPGA_VP6DELAYNUM_REG        	0x4D			// Vp6延迟计数

#define FPGA_KF1FIFONUM_REG         	0x1B			// 踢废机构1的踢废反馈信息个数
#define FPGA_KF2FIFONUM_REG         	0x1C 		// 踢废机构2的踢废反馈信息个数
///////////////////////////////////////////////////////////////////////////////////////////////
/* FIFO*/
#define FPGA_VP5TX_REG            		0x100		//	Vp5_Tx_configfifo
#define FPGA_VP5RX_REG      						0x200		// Vp5_Rx_configfifo

#define FPGA_VP6TX_REG              0xA00		// Vp6_Tx_configfifo
#define FPGA_VP6RX_REG              0xA80		// Vp6_Rx_configfifo

#define FPGA_COM1FIFO_REG         		0x280		//踢废机构1的踢废命令
#define FPGA_COM2FIFO_REG          	0x288		//踢废机构2的踢废命令

#define FPGA_KF1FIFO_REG          		0x300		//踢废机构1的踢废反馈信息
#define FPGA_KF2FIFO_REG          		0x380 	//踢废机构2的踢废反馈信息

/* 	EPCS管理*/
//epcs相关寄存器，由epcs.h包含定义


#endif /* FPGA_H_ */
