/*
 * fpga.c
 *
 *  Created on: 2014年9月17日
 *      Author: work
 */

#define FPGA_C_

#include <stddef.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "debug.h"
#include "fpga.h"
#include "sys_conf.h"

#include "thread.h"
#include "crc16.h"
#include "sample_count.h"
#include "local_protocol.h"
#include "sample_factory.h"
#include "net_protocol.h"
#include "net_context.h"
#include "video_context.h"
#include "sys_log.h"
#include "epcs.h"
#include "key_file.h"

__u32 *fpga_base = NULL;
extern __u32 video_num_enabled;

__u32 sample_count_sum = 0;

static DisInfo dis_info;
static struct SysDelayConfig *g_sys_delay_p;
static __u32 isr_num = 0;
void sig_handler(__s32 signo)
{
	__u32 status = 0;
	sigset_t waitset;
	siginfo_t info;
	__s32 rc;
	__u16 isr_val;
	static __u32 sample_len = 0;
	struct SampleResult *pSampleResult = NULL;
	struct SampleImageGroup  *pSampleImageGroup;

	pthread_t ppid = pthread_self();
	pthread_detach(ppid);

	sigemptyset(&waitset);
	sigaddset(&waitset, SIGIO);

	g_sys_delay_p = sc_get_sys_delay_config(NULL);

	while (1)
	{
		rc = sigwaitinfo(&waitset, &info);
		if (rc != -1)
		{
			if (info.si_signo == SIGIO)
			{
				status = FPGA_READ16(fpga_base, FPGA_STA_REG);
				while (status)
				{
					/* 过长 */
					if (status & 0x04)
					{
						isr_val = FPGA_READ16(fpga_base, FPGA_CLR_REG);
						FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val|0x4);
						FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val&0xFFFB);
					}

					/* 样品后沿过触发对管*/
					if (status & 0x02)
					{
						sample_len = FPGA_READ32(fpga_base, FPGA_SAMPLENL_REG);
						dis_info.length = sample_len;
						dis_info.item = 1;
						send_length_info(&dis_info, sizeof(dis_info));
						isr_val = FPGA_READ16(fpga_base, FPGA_CLR_REG);
						FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val|0x2);
						FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val&0xFFFD);
//						printf("distance\n");
					}
					/* 超频 */
					if (status & 0x01)
					{
						isr_num++;
//						printf("overclock\n");

						isr_val = FPGA_READ16(fpga_base, FPGA_CLR_REG);
						FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val|0x1);
						FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val&0xFFFE);

						sample_count_sum = FPGA_READ32(fpga_base, FPGA_FACTTRIGNUML_REG);

						//发送样品信息至网络
						pSampleImageGroup = sample_factory_produce();
						pSampleImageGroup->sample_info.count = sample_count_sum;
						write_sample_count(sample_count_sum);

						pSampleImageGroup->shoot_wheel_code = fpga_get_current_code();
						pSampleImageGroup->sample_info.wheel_code = pSampleImageGroup->shoot_wheel_code;
						if(video_num_enabled == 2)//如果两个摄像头，此中断会在相机二下发送，必须折算回相机1
							pSampleImageGroup->sample_info.wheel_code -= g_sys_delay_p->C1_C2_delay;

						pSampleImageGroup->sample_info.flag = 1;//相机超频
						pSampleImageGroup->sample_info.kick_pos = 1;

//填写释放元素-------------------------------------------------------------
						pSampleImageGroup->image[0] = get_overclock_img_buf();
						pSampleImageGroup->image_num = 1;
						//默认只有dsp,net需要图像
						pthread_mutex_lock(&(pSampleImageGroup->image[0]->mutex));
						pSampleImageGroup->image[0]->r_count = 1;
						pthread_mutex_unlock(&(pSampleImageGroup->image[0]->mutex));
						pSampleImageGroup->image_source = 1;
//填写释放元素-------------------------------------------------------------

						/* 发送样品信息至踢废管理进程*/
						pSampleResult = get_sample_result_array();
						memcpy(pSampleResult->result, pSampleImageGroup->sample_info.decision, 8);
						pSampleResult->count = sample_count_sum;
						pSampleResult->wheel_code = pSampleImageGroup->shoot_wheel_code;
						if(video_num_enabled == 2)//如果两个摄像头，此中断会在相机二下发送，必须折算回相机1
							pSampleResult->wheel_code -= g_sys_delay_p->C1_C2_delay;
						pSampleResult->pos = 1;
						//pSampleResult->alarm =;
						send_sample_reault(pSampleResult, sizeof(SampleResult));

						send_sample_info(pSampleImageGroup, video_context_release);
					}

					//Vp5产生延时触发模拟图像
					if (status & 0x10)
					{
						isr_val = FPGA_READ16(fpga_base, FPGA_CLR_REG);
						FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val|0x10);
						FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val&0xFFEF);


					}

					//Vp6产生延时触发模拟图像
					if (status & 0x20)
					{
						isr_val = FPGA_READ16(fpga_base, FPGA_CLR_REG);
						FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val|0x20);
						FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val&0xFFDF);
					}

					status = FPGA_READ16(fpga_base, FPGA_STA_REG);
				}
			}
		}
		else
			perror("sigwaitinfo");
	}
}

void fpga_context_init()
{
	__s32 fd = 0;
	__u16 reg_val;
	__u16 tmp1,tmp2;

	fd = open("/dev/altera_fpga", O_RDWR);
	if (fd < 0)
		ERROR("open");

	fcntl(fd, F_SETOWN, getpid());  //告诉驱动当前进程的PID
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | FASYNC);  //设置驱动的FASYNC属性，支持异步通知

	fpga_base = mmap(NULL, 1024 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	reg_val = FPGA_READ16(fpga_base, FPGA_FUNCSLT_REG);
//使能GPIO4
	reg_val = reg_val|0x4;
	FPGA_WRITE16(fpga_base, FPGA_FUNCSLT_REG, reg_val);

//VP5补图延时间隔
	FPGA_WRITE16(fpga_base, FPGA_VP5DELAYNUM_REG, 2000);
	reg_val = FPGA_READ16(fpga_base, FPGA_VP5DELAYNUM_REG);

//关闭过长、触发中断
	reg_val = FPGA_READ16(fpga_base, FPGA_CLR_REG);
	FPGA_WRITE16(fpga_base, FPGA_CLR_REG, reg_val|0x06);

	add_new_thread(NULL, (void *) &sig_handler, 20, 0, 8*1024);
}


__u64 fpga_get_current_code()
{
	__u64 code = 0;
	__u32 code_low32 = 0;
	__u32 code_high32 = 0;

	code_low32 = FPGA_READ32(fpga_base, FPGA_WHEELCOUNTER1_REG);
	code_high32 = FPGA_READ32(fpga_base, FPGA_WHEELCOUNTER3_REG);

	code = ((__u64)code_high32 << 32) | (__u64)code_low32;

	return code;
}

__u64 fpga_get_vp5_shoot_code()
{
	__u64 code = 0;
	__u16 tmp1;
	__u16 tmp2;
	__u16 tmp3;
	__u16 tmp4;

	tmp1 = FPGA_READ16(fpga_base, FPGA_VP5TRIGCAMWHEEL1_REG);
	tmp2 = FPGA_READ16(fpga_base, FPGA_VP5TRIGCAMWHEEL2_REG);
	tmp3 = FPGA_READ16(fpga_base, FPGA_VP5TRIGCAMWHEEL3_REG);
	tmp4 = FPGA_READ16(fpga_base, FPGA_VP5TRIGCAMWHEEL4_REG);

	code = ((__u64)tmp4 << 48)
					|((__u64)tmp3 << 32)
					|((__u64)tmp2 << 16)
					|(__u64)tmp1;

	return code;
}


__u64 fpga_get_vp6_shoot_code()
{
	__u64 code = 0;
	__u16 tmp1;
	__u16 tmp2;
	__u16 tmp3;
	__u16 tmp4;

	tmp1 = FPGA_READ16(fpga_base, FPGA_VP6TRIGCAMWHEEL1_REG);
	tmp2 = FPGA_READ16(fpga_base, FPGA_VP6TRIGCAMWHEEL2_REG);
	tmp3 = FPGA_READ16(fpga_base, FPGA_VP6TRIGCAMWHEEL3_REG);
	tmp4 = FPGA_READ16(fpga_base, FPGA_VP6TRIGCAMWHEEL4_REG);

	code = ((__u64)tmp4 << 48)
					|((__u64)tmp3 << 32)
					|((__u64)tmp2 << 16)
					|(__u64)tmp1;

	return code;
}

__u32 fpga_get_shoot_count()
{
	__u32 count = 0;
	__u16 count_low16 = 0;
	__u16 count_high16 = 0;

	count_low16 = FPGA_READ16(fpga_base, FPGA_TRIGCAMCOUNTERL_REG1);
	count_high16 = FPGA_READ16(fpga_base, FPGA_TRIGCAMCOUNTERH_REG1);

	count = (count_high16 << 16) | count_low16;

	return count;
}

__s32 fpga_reset(void)
{
	FPGA_WRITE16(fpga_base, FPGA_RST_REG, 0);
	usleep(1);
	FPGA_WRITE16(fpga_base, FPGA_RST_REG, 1);

	return 0;
}

__s32 fpga_stop(void)
{
	__u16 val;
	val = FPGA_READ16(fpga_base, FPGA_SYSSLT_REG);

	val &= ~0x07;
	val |= 0x04;

	FPGA_WRITE16(fpga_base, FPGA_SYSSLT_REG, val);

	return 0;
}

//设置拍照延时
__s32 fpga_set_shoot_delay(__u32 *val)
{
	FPGA_WRITE16(fpga_base, FPGA_CAMTRIGPOSL_REG1, *val);
	FPGA_WRITE16(fpga_base, FPGA_CAMTRIGPOSH_REG1, *val>>16);
	FPGA_WRITE16(fpga_base, FPGA_CAMTRIGPOSL_REG2, *(val+1));
	FPGA_WRITE16(fpga_base, FPGA_CAMTRIGPOSH_REG2, *(val+1)>>16);
	return 0;
}

//设置踢废延时
__s32 fpga_set_kick_delay(__u32 *val)
{
	FPGA_WRITE16(fpga_base, FPGA_KICKTRIGPOSL_REG1, *val);
	FPGA_WRITE16(fpga_base, FPGA_KICKTRIGPOSH_REG1, *val>>16);

	FPGA_WRITE16(fpga_base, FPGA_KICKEXECPOSL_REG1, *(val+1));
	FPGA_WRITE16(fpga_base, FPGA_KICKEXECPOSH_REG1, *(val+1)>>16);

	FPGA_WRITE16(fpga_base, FPGA_KICKBACKPOSL_REG1, *(val+2));
	FPGA_WRITE16(fpga_base, FPGA_KICKBACKPOSH_REG1, *(val+2)>>16);

	FPGA_WRITE16(fpga_base, FPGA_KICKTRIGPOSL_REG2, *(val+3));
	FPGA_WRITE16(fpga_base, FPGA_KICKTRIGPOSH_REG2, *(val+3)>>16);

	FPGA_WRITE16(fpga_base, FPGA_KICKEXECPOSL_REG2, *(val+4));
	FPGA_WRITE16(fpga_base, FPGA_KICKEXECPOSH_REG2, *(val+4)>>16);

	FPGA_WRITE16(fpga_base, FPGA_KICKBACKPOSL_REG2, *(val+5));
	FPGA_WRITE16(fpga_base, FPGA_KICKBACKPOSH_REG2, *(val+5)>>16);

	return 0;
}

//设置信号极性
__s32 fpga_set_signal_polar(__u16 val)
{
	__u16 reg_val;
	reg_val = FPGA_READ16(fpga_base, FPGA_POLAR_REG);
	reg_val |= 0x0A;//bit1,bit3必须设置为1

	//取val bit0,触发对管
	if(val&0x01)
	{
		reg_val |= 0x1; //FPGA寄存器 bit0
	}
	else
	{
		reg_val &= (~0x1); //FPGA寄存器 bit0
	}

	//取val bit1,剔除对管
	if(val&0x02)
	{
		reg_val |= 0x30;//FPGA寄存器 bit4\5
	}
	else
	{
		reg_val &= (~0x30);//FPGA寄存器 bit4\5
	}

	//取val bit2,剔除反馈对管
	if(val&0x04)
	{
		reg_val |= 0x300;//FPGA寄存器 bit8\9
	}
	else
	{
		reg_val &= (~0x300);//FPGA寄存器 bit8\9
	}

	//取val bit3,剔除信号1
	if(val&0x08)
	{
		reg_val |= 0x4; //FPGA寄存器 bit2
	}
	else
	{
		reg_val &= (~0x4); //FPGA寄存器 bit2
	}

	//取val bit4,剔除信号2
	if(val&0x10)
	{
		reg_val |= 0x80; //FPGA寄存器 bit7
	}
	else
	{
		reg_val &= (~0x80); //FPGA寄存器 bit7
	}

	//取val bit5,光源1
	if(val&0x20)
	{
		reg_val |= 0x400; //FPGA寄存器 bit10
	}
	else
	{
		reg_val &= (~0x400); //FPGA寄存器 bit10
	}

	//取val bit6,光源2
	if(val&0x40)
	{
		reg_val |= 0x800; //FPGA寄存器 bit11
	}
	else
	{
		reg_val &= (~0x800); //FPGA寄存器 bit11
	}

	FPGA_WRITE16(fpga_base, FPGA_POLAR_REG, reg_val);
	return 0;
}

//设置输出信号宽度
__s32 fpga_set_out_signal_width(__u32 *val)
{
	FPGA_WRITE16(fpga_base, FPGA_KICKDIS_REG1, 			*(val + 0));//踢废信号1宽度
	FPGA_WRITE16(fpga_base, FPGA_KICKDIS_REG2, 			*(val + 1));//踢废信号2宽度
	return 0;
}

//设置输入信号滤波宽度
__s32 fpga_set_in_signal_width(__u32 *val)
{
	FPGA_WRITE16(fpga_base, FPGA_TRIGFIRDIS_REG, 	*(val + 0));//触发对管滤波宽度
	FPGA_WRITE16(fpga_base, FPGA_KICKFIRDIS_REG1, 	*(val + 1));//踢废对管1滤波宽度
	FPGA_WRITE16(fpga_base, FPGA_KICKFIRDIS_REG2, 	*(val + 1));//踢废对管2滤波宽度
																																								//反馈对管与触发对管共用寄存器，没必要设置
	FPGA_WRITE16(fpga_base, FPGA_WHEELFIRDIS_REG, 	*(val + 3));//外码盘滤波宽度
	return 0;
}




/*
 * 设置相机触发源
 * 此设置针对conf文件的valid
 * 0：屏蔽(不对相机发送触发信号)
 * 1：不屏蔽(保持)
 */
__s32 fpga_set_cam_trig(__u32 video1_valid, __u32 video2_valid)
{
	__u16 tmp;

	tmp = FPGA_READ16(fpga_base, FPGA_FUNCSLT_REG);

	if(video1_valid)
	{
		//bit1
		tmp |= 0x02;
	}
	else
	{
		//bit1
		tmp &= (~0x02);
	}

	if(video2_valid)
	{
		//bit1
		tmp |= 0x100;
	}
	else
	{
		//bit1
		tmp &= (~0x100);
	}

	FPGA_WRITE16(fpga_base, FPGA_FUNCSLT_REG, tmp);

	return 0;
}
/*
 * 设置剔除方式
 * 0：内码盘延时剔除
 * 1：外码盘延时剔除
 * 2：外码盘对管剔除
 * 剔除方式改变，剔除对管1的中断源也要改变
 */
__s32 fpga_set_kick_mode(__u32 val)
{
	__u16 tmp;
//	struct FPGAFuncConfig *pstr;
//
//	pstr = sc_get_fpga_func_config(NULL);

	tmp = FPGA_READ16(fpga_base, FPGA_FUNCSLT_REG);

	if(val == 0)
	{
		//bit6=0,bit7=0
		tmp &= ~0xC0;
	}
	else if(val == 1)
	{
		//bit6=1,bit7=0
		tmp |= 0x40;
		tmp &= ~0x80;
		//bit11 = 0, bit15 = 1
		tmp &= ~0x800;
		tmp |= 0x8000;
	}
	else if(val == 2)
	{
		//bit6=1,bit7=1
		tmp |= 0xC0;
	}

	FPGA_WRITE16(fpga_base, FPGA_FUNCSLT_REG, tmp);

	return 0;
}

/*
 * 是否使能过长连拍功能
 * val = 0 单拍
 * val = 1 多拍
 */
__s32 fpga_set_ol_shoot_mode(__u32 val)
{
	__u16 tmp;

	tmp = FPGA_READ16(fpga_base, FPGA_FUNCSLT_REG);

	if(val == 1)
	{
		//bit10 = 0
		tmp &= ~0x0400;
	}
	else if(val == 0)
	{
		//bit10 = 1
		tmp |= 0x0400;
	}
	//bit9 = 1,bit10才能生效
	tmp |= 0x0200;

	FPGA_WRITE16(fpga_base, FPGA_FUNCSLT_REG, tmp);

	return 0;
}


/*
 * 使能第二对管
 * val = 0 不使能
 * val = 1 使能
 */
__s32 fpga_enable_second_tube(__u32 val)
{
	__u16 tmp;
	struct FPGAFuncConfig *pstr;

	pstr = sc_get_fpga_func_config(NULL);

	tmp = FPGA_READ16(fpga_base, FPGA_FUNCSLT_REG);

	if(val == 0)
	{
		//bit11 = 0, bit15 = 0
		tmp &= ~0x8800;
		//bit12 = 0, bit4 = 0
		tmp &= ~0x1010;
	}
	else if(val == 1)
	{
		//bit12 = 1, bit4 = 1
		tmp |= 0x1010;

		if(pstr->kick_mode == 1)//延时剔除
		{
			//bit11 = 0, bit15 = 1
			tmp &= ~0x800;
			tmp |= 0x8000;
		}
		else if(pstr->kick_mode == 2)//对管剔除
		{
			//bit11 = 1
			tmp |= 0x800;
		}
	}

	FPGA_WRITE16(fpga_base, FPGA_FUNCSLT_REG, tmp);

	return 0;
}

/*
 * 使能反馈对管
 * mode=1 剔除
 * mode=0 不剔除
 */
__s32 fpga_set_feedback_tube(__u32 val)
{
	__u16 tmp;

	tmp = FPGA_READ16(fpga_base, FPGA_FUNCSLT_REG);

	if(val & 0x01)
	{
		//bit13 = 1
		tmp |= 0x2000;
	}
	else
	{
		//bit13 = 0
		tmp &= (~0x2000);
	}

	if(val & 0x02)
	{
		//bit14 = 1
		tmp |= 0x4000;
	}
	else
	{
		//bit14 = 0
		tmp &= (~0x4000);
	}

	FPGA_WRITE16(fpga_base, FPGA_FUNCSLT_REG, tmp);
	return 0;
}


__s32 fpga_set_working_mode(__u16 mode)
{
	__u16 val;
	val = mode & 0x07;
	FPGA_WRITE16(fpga_base, FPGA_CAMTYPE_REG, val);
	return 0;
}


__s32 fpga_update_program(__u32 file_len, __u32 unit_len, char* data)
{

	return 0;
}

//__s32 fpga_set_image_format(__u32 video_id, __u32 width, __u32 height)
//{
//	if (video_id == 0)
//	{
//		FPGA_WRITE16(fpga_base, FPGA_VP5OUTLINENUM_REG, width);
//		FPGA_WRITE16(fpga_base, FPGA_VP5OUTPIXELMUN_REG, height);
//	}
//	else if (video_id == 1)
//	{
//		FPGA_WRITE16(fpga_base, FPGA_VP6OUTLINENUM_REG, width);
//		FPGA_WRITE16(fpga_base, FPGA_VP6OUTPIXELMUN_REG, height);
//	}
//	return 0;
//}

__s32 fpga_get_version(char *asc_ver)
{
	__u16 tmp;
	tmp = FPGA_READ16(fpga_base, FPGA_VER_REG);
	sprintf(asc_ver, "V%d.%d.%d", tmp/100, (tmp%100)/10, (tmp%10)/1);
	return 0;
}





__s32 fpga_set_sim_trig(__u16 *data)
{
	__u16 tmp = 0;
	__u16 tri_ch1, tri_ch2;
	__u16 length, loop, gap;
	tri_ch1 = *(data + 3);
	tri_ch2 = *(data + 4);

	if((tri_ch1 == 0) && (tri_ch2 == 0))
	{
		tmp = FPGA_READ16(fpga_base, FPGA_SYSSLT_REG);
		tmp &= ~0x01; //bit0 = 0
		FPGA_WRITE16(fpga_base, FPGA_SYSSLT_REG, tmp);
		return 0;
	}

	/* 设置模拟样品长度 */
	FPGA_WRITE16(fpga_base, FPGA_TRIG_REG, *data);

	/* 设置模拟样品间距 */
	FPGA_WRITE16(fpga_base, FPGA_DISTANCE_REG, *(data + 1));

	/* 设置循环触发次数 */
	tmp = FPGA_READ16(fpga_base, FPGA_SIMSLT_REG);
	tmp &= ~0xFF00;
	tmp |= (*(data + 2) << 8);

	FPGA_WRITE16(fpga_base, FPGA_SIMSLT_REG, tmp);

	tmp = FPGA_READ16(fpga_base, FPGA_SIMSLT_REG);

	/* 由实际系统切换到模拟系统 */
	tmp = FPGA_READ16(fpga_base, FPGA_SYSSLT_REG);
	tmp &= ~0x01; //bit0 = 0
	FPGA_WRITE16(fpga_base, FPGA_SYSSLT_REG, tmp);
	tmp |= 0x01; //bit0 = 1
	FPGA_WRITE16(fpga_base, FPGA_SYSSLT_REG, tmp);

	return 0;
}

__s32 fpga_start()
{
	__u16 tmp;
	__u32 tmp2 = 0;
	FPGA_WRITE16(fpga_base, FPGA_ISREN_REG, 0x3);
	FPGA_WRITE16(fpga_base, FPGA_SHK_REG, 0x0101);
	fpga_reset();
	usleep(1000);
	FPGA_WRITE16(fpga_base, FPGA_SHK_REG, 0x1010);
	return 0;
}

__s32 fpga_set_sample_length(__u32 len)
{
	FPGA_WRITE32(fpga_base, FPGA_TRIGLENL_REG, len);
	return 0;
}

__u32 fpga_get_sample_length()
{
	__u32 sample_len = FPGA_READ32(fpga_base, FPGA_SAMPLENL_REG);
	return sample_len;
}

__s32 fpga_set_overclock_threshold(__u16 val)
{
	__u32 tmp;
	FPGA_WRITE32(fpga_base, FPGA_FREQLIMITL_REG, val);
//	tmp = FPGA_READ32(fpga_base, FPGA_FREQLIMITL_REG);
	return 0;
}

__u32 fpga_get_overclock_threshold()
{
	__u32 overclock_threshold = FPGA_READ32(fpga_base, FPGA_FREQLIMITL_REG);
	return overclock_threshold;
}



/* 返回超频样品总数 */
__u32 fpga_get_oc_amount()
{
	__u32 amount = FPGA_READ32(fpga_base, FPGA_OVERFREQNUML_REG);

	return amount;
}

/* 返回过长样品总数 */
__u32 fpga_get_ol_amount()
{
	__u32 amount = FPGA_READ32(fpga_base, FPGA_OVERLONGNUML_REG);

	return amount;
}

/* 返回样品流量均值 */
__u32 fpga_get_aver_flow()
{
	__u32 delta = FPGA_READ32(fpga_base, FPGA_TRIGDISL_REG);
	return delta;
}

/* 返回样品流量峰值 */
__u32 fpga_get_max_flow()
{
	__u32 delta = FPGA_READ32(fpga_base, FPGA_TRIGDIFL_REG);
	return delta;
}

/* 返回仿真器连接状态 */
__s32 fpga_get_emulator_state()
{
	__s32 state = 0;
	state = FPGA_READ16(fpga_base, FPGA_SIMLINK_REG);
	return state;
}

/* 返回fpga内部调试信息 */
FPGA_DEBUG_INFO *fpga_get_debug_info(__u32 *len)
{
	static FPGA_DEBUG_INFO fpga_debug_info;
	*len = sizeof(FPGA_DEBUG_INFO);

	fpga_debug_info.ext_tri_num 				= (FPGA_READ16(fpga_base, FPGA_FACTTRIGNUMH_REG)<<16)
																							 | FPGA_READ16(fpga_base, FPGA_FACTTRIGNUML_REG);
	fpga_debug_info.sin_tri_num 				= (FPGA_READ16(fpga_base, FPGA_SIMTRIGNUMH_REG)<<16)
																							 | FPGA_READ16(fpga_base, FPGA_SIMTRIGNUML_REG);

	fpga_debug_info.ch1_trig_amount 	= (FPGA_READ16(fpga_base, FPGA_TRIGCAMCOUNTERH_REG1)<<16)
																							 | FPGA_READ16(fpga_base, FPGA_TRIGCAMCOUNTERL_REG1);
	fpga_debug_info.ch2_trig_amount 	= (FPGA_READ16(fpga_base, FPGA_TRIGCAMCOUNTERH_REG2)<<16)
																							 | FPGA_READ16(fpga_base, FPGA_TRIGCAMCOUNTERL_REG2);

	fpga_debug_info.ch1_recv_frames 	= (FPGA_READ16(fpga_base, FPGA_VP5OUTFRAMEH_REG)<<16)
																							 | FPGA_READ16(fpga_base, FPGA_VP5OUTFRAMEL_REG);

//	printf("ch1_recv_frames = %d\n",fpga_debug_info.ch1_recv_frames);

	fpga_debug_info.ch1_img_height 		= FPGA_READ16(fpga_base, FPGA_VP5OUTLINENUM_REG);
	fpga_debug_info.ch1_img_width 		= FPGA_READ32(fpga_base, FPGA_VP5OUTPIXELMUNL_REG) * 2;

	fpga_debug_info.ch2_recv_frames 	= (FPGA_READ16(fpga_base, FPGA_VP6OUTFRAMEH_REG)<<16)
																							 | FPGA_READ16(fpga_base, FPGA_VP6OUTFRAMEL_REG);
	fpga_debug_info.ch2_img_height 		= FPGA_READ16(fpga_base, FPGA_VP6OUTLINENUM_REG);
	fpga_debug_info.ch2_img_width 		= FPGA_READ32(fpga_base, FPGA_VP6OUTPIXELMUNL_REG) * 2;

	return &fpga_debug_info;
}

/* 告知FPGA图像尺寸 */
__s32 fpga_set_image_size(__u32 id, __u16 width, __u16 height)
{
	if(0 ==   id)
	{
		FPGA_WRITE16(fpga_base, FPGA_VP5PIXELMUN_REG, width >> 2);
		FPGA_WRITE16(fpga_base, FPGA_VP5LINENUM_REG, height);
	}
	else if(1 == id)
	{
		FPGA_WRITE16(fpga_base, FPGA_VP6PIXELMUN_REG, width >> 2);
		FPGA_WRITE16(fpga_base, FPGA_VP6LINENUM_REG, height);
	}
	else
		;
	return 0;
}

__s32 fpga_set_camera_state(__u32 ch1, __u32 ch2)
{
	__u32 tmp;
	tmp = FPGA_READ16(fpga_base, FPGA_SYSSLT_REG);

	tmp &= ~0x30; //bit4、bit5清零

	if(ch1)
	{
		tmp |= 0x10;	//vip5无头板
	}

	if(ch2)
	{
		tmp |=0x20;	//vip6无头板
	}

	FPGA_WRITE16(fpga_base, FPGA_SYSSLT_REG, tmp);
	tmp = FPGA_READ16(fpga_base, FPGA_SYSSLT_REG);

	return 0;
}


/*   lvds操作函数  */

__s32  fpga_write_lvds1_txfifo_pre()
{
	FPGA_WRITE16(fpga_base, FPGA_VP5CLRTXFLG_REG, 1);
	return 0;
}
__s32  fpga_write_lvds1_txfifo(__u16 *data, __u16 len)
{
	while(len--)
	{
		//dat = *data;
		FPGA_WRITE16(fpga_base, FPGA_VP5TX_REG, *(data++));
		//data++;
	}
	return 0;
}
__s32  fpga_write_lvds1_txfifo_fni()
{
	FPGA_WRITE16(fpga_base, FPGA_VP5WRTXFLG_REG, 1);
	return 0;
}

__s32  fpga_write_lvds2_txfifo_pre()
{
	FPGA_WRITE16(fpga_base, FPGA_VP6CLRTXFLG_REG, 1);
	return 0;
}
__s32  fpga_write_lvds2_txfifo(__u16 *data, __u16 len)
{
	__u32 i;
	for (i = 0; i < len; i++)
	{
		FPGA_WRITE16(fpga_base, FPGA_VP6TX_REG, *(data + i)); //VP6地址不对
	}
	return 0;
}
__s32  fpga_write_lvds2_txfifo_fni()
{
	FPGA_WRITE16(fpga_base, FPGA_VP6WRTXFLG_REG, 1);
	return 0;
}

void fpga_check_lvds1_rxfifo()
{
	__u32 length = 0;
	__u32 tmp;

	length = FPGA_READ16(fpga_base, FPGA_VP5RXDATANUM_REG);
	if(length)
	{
		FPGA_WRITE16(fpga_base, FPGA_VP5RDRXFLG_REG, 1);
		while(length--)
		{
			tmp = FPGA_READ16(fpga_base, FPGA_VP5RX_REG);
		}
	}
}

void fpga_check_lvds2_rxfifo()
{
	__u32 length = 0;
	__u32 tmp;

	length = FPGA_READ16(fpga_base, FPGA_VP6RXDATANUM_REG);
	if(length)
	{

		FPGA_WRITE16(fpga_base, FPGA_VP6RDRXFLG_REG, 1);
		while(length--)
		{
			tmp = FPGA_READ16(fpga_base, FPGA_VP6RX_REG);
		}
	}
}

__s32  fpga_read_lvds1_rxfifo(__u16 *data, __u16 len)
{
	__u32 i = 0;

	while(i < len)
		*(data + i) = FPGA_READ16(fpga_base, FPGA_VP5RX_REG);


	return 0;
}
__s32  fpga_read_lvds2_rxfifo(__u16 *data, __u16 len)
{
	__u32 i = 0;

	while(i < len)
		*(data + i) = FPGA_READ16(fpga_base, FPGA_VP6RX_REG);

	return 0;
}

void fpga_lvds1_send(__u16 *data, __u16 len)
{
	FPGA_WRITE16(fpga_base, FPGA_VP5CLRTXFLG_REG, 1);
	while(len--)
	{
		FPGA_WRITE16(fpga_base, FPGA_VP5TX_REG, *(data++));
	}
	FPGA_WRITE16(fpga_base, FPGA_VP5WRTXFLG_REG, 1);
}

void fpga_lvds2_send(__u16 *data, __u16 len)
{
	FPGA_WRITE16(fpga_base, FPGA_VP6CLRTXFLG_REG, 1);
	while(len--)
	{
		FPGA_WRITE16(fpga_base, FPGA_VP6TX_REG, *(data++));////VP6地址不对
	}
	FPGA_WRITE16(fpga_base, FPGA_VP6WRTXFLG_REG, 1);
}

__u8 rev_data[65535];
__u32 fpga_lvds1_recv(__u16 *data, __u16 *len)
{
	__u16 recv_len;
	__u16 ptl_len;
	__u16 ptl_cmd;
	__u16 *ptl_data;
	__u16 crc;
	__u32 i;
	__u32 j = 0;
	__u32 val;
	__u16 tmp;
	for(i=0;i<500;i++)//500ms
	{
		usleep(1000);//1ms
		//检查是否rxfifo 有返回值
		if(FPGA_READ16(fpga_base, FPGA_VP5RDRXFLG_REG))
		{
			FPGA_WRITE16(fpga_base, FPGA_VP5RDRXFLG_REG, 1);//清除数据通知
			*len = FPGA_READ16(fpga_base, FPGA_VP5RXDATANUM_REG);
			recv_len = *len;
			while(recv_len--)
			{
				*(data + j) = FPGA_READ16(fpga_base, FPGA_VP5RX_REG);
				j++;
			}
			goto crc_check;
		}
	}
	val = 2;
	log_builder("lvds1 recv timeout.");
	return val;

	crc_check:

#if 0
	memcpy(rev_data, (__u8*)data, *len);
#endif
	ptl_cmd = *(__u16*)data;
	ptl_len = *((__u16*)data+1);
	ptl_data = (__u16*)data+2;
	//计算CRC
	crc = crc16(0, (__u8*)data, ptl_len + 2);
	if(crc != *(ptl_data + ptl_len/2 - 1))
	{
		val = 1;
		log_builder("lvds1 recv crc error.");
		return val;
	}

	return 0;
}

__u32 fpga_lvds2_recv(__u16 *data, __u16 *len)
{
	__u16 recv_len;
	__u16 ptl_len;
	__u16 ptl_cmd;
	__u16 *ptl_data;
	__u16 crc;
	__u32 i;
	__u32 j = 0;
	__u32 val;
	__u16 tmp;
	for(i=0;i<500;i++)//500ms
	{
		usleep(1000);//1ms
		//检查是否rxfifo 有返回值
		if(FPGA_READ16(fpga_base, FPGA_VP6RDRXFLG_REG))
		{
			FPGA_WRITE16(fpga_base, FPGA_VP6RDRXFLG_REG, 1);//清除数据通知
			*len = FPGA_READ16(fpga_base, FPGA_VP6RXDATANUM_REG);
			recv_len = *len;
			while(recv_len--)
			{
				*(data + j) = FPGA_READ16(fpga_base, FPGA_VP6RX_REG);
				j++;
			}
			goto crc_check;
		}
	}
	val = 2;//超时
	log_builder("lvds2 recv timeout.");
	return 2;

	crc_check:

	ptl_cmd = *(__u16*)data;
	ptl_len = *((__u16*)data+1);
	ptl_data = (__u16*)data+2;
	//计算CRC
	crc = crc16(0, (__u8*)data, ptl_len + 2);
	if(crc != *(ptl_data + ptl_len/2 - 1))
	{
		val = 1;//CRC错误
		log_builder("lvds2 recv crc error.");
		return 1;
	}

	return 0;
}



// 更新应用程序 0x45521110
__u8 reverse8(__u8 c)
{
	c = ( c & 0x55 ) << 1 | ( c & 0xAA ) >> 1;
	c = ( c & 0x33 ) << 2 | ( c & 0xCC ) >> 2;
	c = ( c & 0x0F ) << 4 | ( c & 0xF0 ) >> 4;

	return c;
}

void fpga_burn(__u8 *data)
{
	int i = 0;
	__u32 val;
	unsigned char tmp = 0;
	unsigned int  gie = 0;
	__u8* PramData = data;
	static alt_flash_fd *p_epcs_fd;

	p_epcs_fd = (alt_flash_fd *)malloc(sizeof(alt_flash_fd));
	if(p_epcs_fd == NULL)
		return;

	p_epcs_fd->base_addr = (void*)((unsigned short*)fpga_base + FPGA_EPCS_BASE);

	p_epcs_fd->length = 0;
	p_epcs_fd->number_of_regions = 1;

	for (i=0; i< EPCS64_BLOCK_SIZE; i++)
	{
		tmp = PramData[i];
		PramData[i] = reverse8(tmp);
	}

	epcs_bulk_erase((__u32)(p_epcs_fd->base_addr));

	alt_epcs_flash_write_block(p_epcs_fd, 0, 0, (void*)(PramData+JIC_INVALID_BYTES),EPCS64_BLOCK_SIZE);

	free(p_epcs_fd);

}

