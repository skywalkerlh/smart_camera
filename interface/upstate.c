/*
 * upstate.c
 *
 *  Created on: 2015年8月19日
 *      Author: work
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <linux/types.h>
#include <pthread.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>

#include "sys_conf.h"
#include "net_context.h"
#include "net_protocol.h"
#include "thread.h"
#include "ui_protocol.h"
#include "fpga.h"
#include "buf_factory.h"
#include "msg_factory.h"
#include "lvds_context.h"
#include "mailbox.h"

extern __u32 g_dsp1_reset_count;
extern __u32 g_dsp2_reset_count;
extern __u32 g_dsp1_timeout_count;
extern __u32 g_dsp2_timeout_count;
extern __u32 g_process_overclock_amount;

#define ADC_END_VALUE		945
#define ADC_START_VALUE	540

struct temp_info
{
	__u32 cpu_temp;
	__u32 reserve[4];
}temp_info;


static pthread_mutex_t g_mutex_kf_lost_mun;
static __u16 Kf1_lost_num = 0;
static __u16 Kf2_lost_num = 0;

int adc_to_temp[ADC_END_VALUE - ADC_START_VALUE + 1] =
{
	/* Index 540 - 549 */
	-40000, -40000, -40000, -40000, -39800, -39400, -39000, -38600, -38200,
	-37800,
	/* Index 550 - 559 */
	-37400, -37000, -36600, -36200, -35800, -35300, -34700, -34200, -33800,
	-33400,
	/* Index 560 - 569 */
	-33000, -32600, -32200, -31800, -31400, -31000, -30600, -30200, -29800,
	-29400,
	/* Index 570 - 579 */
	-29000, -28600, -28200, -27700, -27100, -26600, -26200, -25800, -25400,
	-25000,
	/* Index 580 - 589 */
	-24600, -24200, -23800, -23400, -23000, -22600, -22200, -21800, -21400,
	-21000,
	/* Index 590 - 599 */
	-20500, -19900, -19400, -19000, -18600, -18200, -17800, -17400, -17000,
	-16600,
	/* Index 600 - 609 */
	-16200, -15800, -15400, -15000, -14600, -14200, -13800, -13400, -13000,
	-12500,
	/* Index 610 - 619 */
	-11900, -11400, -11000, -10600, -10200, -9800, -9400, -9000, -8600,
	-8200,
	/* Index 620 - 629 */
	-7800, -7400, -7000, -6600, -6200, -5800, -5400, -5000, -4500,
	-3900,
	/* Index 630 - 639 */
	-3400, -3000, -2600, -2200, -1800, -1400, -1000, -600, -200,
	200,
	/* Index 640 - 649 */
	600, 1000, 1400, 1800, 2200, 2600, 3000, 3400, 3900,
	4500,
	/* Index 650 - 659 */
	5000, 5400, 5800, 6200, 6600, 7000, 7400, 7800, 8200,
	8600,
	/* Index 660 - 669 */
	9000, 9400, 9800, 10200, 10600, 11000, 11400, 11800, 12200,
	12700,
	/* Index 670 - 679 */
	13300, 13800, 14200, 14600, 15000, 15400, 15800, 16200, 16600,
	17000,
	/* Index 680 - 689 */
	17400, 17800, 18200, 18600, 19000, 19400, 19800, 20200, 20600,
	21000,
	/* Index 690 - 699 */
	21400, 21900, 22500, 23000, 23400, 23800, 24200, 24600, 25000,
	25400,
	/* Index 700 - 709 */
	25800, 26200, 26600, 27000, 27400, 27800, 28200, 28600, 29000,
	29400,
	/* Index 710 - 719 */
	29800, 30200, 30600, 31000, 31400, 31900, 32500, 33000, 33400,
	33800,
	/* Index 720 - 729 */
	34200, 34600, 35000, 35400, 35800, 36200, 36600, 37000, 37400,
	37800,
	/* Index 730 - 739 */
	38200, 38600, 39000, 39400, 39800, 40200, 40600, 41000, 41400,
	41800,
	/* Index 740 - 749 */
	42200, 42600, 43100, 43700, 44200, 44600, 45000, 45400, 45800,
	46200,
	/* Index 750 - 759 */
	46600, 47000, 47400, 47800, 48200, 48600, 49000, 49400, 49800,
	50200,
	/* Index 760 - 769 */
	50600, 51000, 51400, 51800, 52200, 52600, 53000, 53400, 53800,
	54200,
	/* Index 770 - 779 */
	54600, 55000, 55400, 55900, 56500, 57000, 57400, 57800, 58200,
	58600,
	/* Index 780 - 789 */
	59000, 59400, 59800, 60200, 60600, 61000, 61400, 61800, 62200,
	62600,
	/* Index 790 - 799 */
	63000, 63400, 63800, 64200, 64600, 65000, 65400, 65800, 66200,
	66600,
	/* Index 800 - 809 */
	67000, 67400, 67800, 68200, 68600, 69000, 69400, 69800, 70200,
	70600,
	/* Index 810 - 819 */
	71000, 71500, 72100, 72600, 73000, 73400, 73800, 74200, 74600,
	75000,
	/* Index 820 - 829 */
	75400, 75800, 76200, 76600, 77000, 77400, 77800, 78200, 78600,
	79000,
	/* Index 830 - 839 */
	79400, 79800, 80200, 80600, 81000, 81400, 81800, 82200, 82600,
	83000,
	/* Index 840 - 849 */
	83400, 83800, 84200, 84600, 85000, 85400, 85800, 86200, 86600,
	87000,
	/* Index 850 - 859 */
	87400, 87800, 88200, 88600, 89000, 89400, 89800, 90200, 90600,
	91000,
	/* Index 860 - 869 */
	91400, 91800, 92200, 92600, 93000, 93400, 93800, 94200, 94600,
	95000,
	/* Index 870 - 879 */
	95400, 95800, 96200, 96600, 97000, 97500, 98100, 98600, 99000,
	99400,
	/* Index 880 - 889 */
	99800, 100200, 100600, 101000, 101400, 101800, 102200, 102600, 103000,
	103400,
	/* Index 890 - 899 */
	103800, 104200, 104600, 105000, 105400, 105800, 106200, 106600, 107000,
	107400,
	/* Index 900 - 909 */
	107800, 108200, 108600, 109000, 109400, 109800, 110200, 110600, 111000,
	111400,
	/* Index 910 - 919 */
	111800, 112200, 112600, 113000, 113400, 113800, 114200, 114600, 115000,
	115400,
	/* Index 920 - 929 */
	115800, 116200, 116600, 117000, 117400, 117800, 118200, 118600, 119000,
	119400,
	/* Index 930 - 939 */
	119800, 120200, 120600, 121000, 121400, 121800, 122200, 122600, 123000,
	123400,
	/* Index 940 - 945 */
	123800, 124200, 124600, 124900, 125000, 125000,
};


void query_temp(__u32 id)
{
	__u32 *p;
	struct message *msg2camera;
	struct Buffer *buf = buf_factory_produce();

	p = buf->memory;
	*p = 0x45522207;
	*(p+1) = 4;

	if (id == 0)
		msg2camera = msg_factory_produce(get_lvds_context_mailbox(0), 0);
	else if (id == 1)
		msg2camera = msg_factory_produce(get_lvds_context_mailbox(1), 0);
	else
	{
		buf_factory_recycle(0,buf);
	}
	msg2camera->ops->set_data(msg2camera, buf, 0, NULL, 0);
	mailbox_post(msg2camera);
}



void upstate_tsk(void)
{
	struct SysStatisticsInfo* pstate;
	int temp = 0;
//	struct rtc_time rtc_tm;
	__u32 cur_time = 0;
	__u32 len = 0;
	time_t timep;
	struct tm *tp;
//	int ret = 0;
	int idx = 0;
//	float temp = 0.0;

	__u16 tmp1,tmp2;

	time_t new_sec;
	time_t old_sec;

	pstate = sc_get_sys_state_info_config(&len);

	int fd = open("/dev/temp_dev",O_RDWR);
	if(fd < 0)
		    perror("temp_dev open");

	old_sec = time(&timep);

	while(1)
	{
		sleep(1);

		read(fd, &temp_info, sizeof(temp_info));

		idx = temp_info.cpu_temp - 540;

		pstate->sys_temp_info.mpu_temp = adc_to_temp[idx]/100;

		timep = time(NULL);
		tp=localtime(&timep);
		cur_time++;

#if 0
		new_sec = time(&timep);
		cur_time = cur_time + (new_sec - old_sec);
		old_sec = new_sec;
#endif

		pstate->sys_date_time.year = 1900+tp->tm_year;
		pstate->sys_date_time.month = 1+tp->tm_mon;
		pstate->sys_date_time.day = tp->tm_mday;
		pstate->sys_date_time.hour = tp->tm_hour;
		pstate->sys_date_time.minute = tp->tm_min;
		pstate->sys_date_time.second = tp->tm_sec;

//		printf("%d-%d-%d %d:%d:%d\n",
//				pstate->sys_date_time.year,
//				pstate->sys_date_time.month,
//				pstate->sys_date_time.day,
//				pstate->sys_date_time.hour,
//				pstate->sys_date_time.minute,
//				pstate->sys_date_time.second);

		pstate->sys_work_time = cur_time;

		query_temp(0);
		query_temp(1);

		//填充样品计数
		pstate->sample_stat_info.sample_amount 							= FPGA_READ32(fpga_base, FPGA_FACTTRIGNUML_REG);
		pstate->sample_stat_info.aver_flux 										= FPGA_READ32(fpga_base, FPGA_TRIGDISL_REG);
		pstate->sample_stat_info.peak_flux 										= FPGA_READ32(fpga_base, FPGA_TRIGDIFL_REG);
		pstate->sample_stat_info.dsp1_timeout_amount 			= g_dsp1_timeout_count;
		pstate->sample_stat_info.dsp2_timeout_amount 			= g_dsp2_timeout_count;
		pstate->sample_stat_info.dsp1_reset_amount 					= g_dsp1_reset_count;
		pstate->sample_stat_info.dsp2_reset_amount 					= g_dsp2_reset_count;
		pstate->sample_stat_info.camera_overclock_amount 	= FPGA_READ32(fpga_base, FPGA_OVERFREQNUML_REG);
		pstate->sample_stat_info.process_overclock_amount	= g_process_overclock_amount;
		pstate->sample_stat_info.overlength_amount					= FPGA_READ32(fpga_base, FPGA_OVERLONGNUML_REG);

		pthread_mutex_lock(&g_mutex_kf_lost_mun);
		tmp1 = FPGA_READ16(fpga_base, FPGA_KF1FIFONUM_REG);
		while(tmp1--)
		{
			tmp2 = FPGA_READ16(fpga_base, FPGA_KF1FIFO_REG);
			if(tmp2 == 0)
			{
				Kf1_lost_num++;
			}
		}
		pstate->sample_stat_info.pos1_lost_amount						= Kf1_lost_num;

		tmp1 = FPGA_READ16(fpga_base, FPGA_KF2FIFONUM_REG);
		while(tmp1--)
		{
			tmp2 = FPGA_READ16(fpga_base, FPGA_KF2FIFO_REG);
			if(tmp2 == 0)
			{
				Kf2_lost_num++;
			}
		}
		pstate->sample_stat_info.pos2_lost_amount						= Kf2_lost_num;
		pthread_mutex_unlock(&g_mutex_kf_lost_mun);
		//填充算法计数

		send_sys_state_info(pstate, len);
		send_ui_sys_state_info(pstate, len);
	}
}


void kick_count_reset(void)
{
	__u16 tmp1,tmp2;
	pthread_mutex_lock(&g_mutex_kf_lost_mun);
	Kf1_lost_num = 0;
	Kf2_lost_num = 0;
	tmp1 = FPGA_READ16(fpga_base, FPGA_KF1FIFONUM_REG);
	while(tmp1--)
	{
		tmp2 = FPGA_READ16(fpga_base, FPGA_KF1FIFO_REG);
	}

	tmp1 = FPGA_READ16(fpga_base, FPGA_KF2FIFONUM_REG);
	while(tmp1--)
	{
		tmp2 = FPGA_READ16(fpga_base, FPGA_KF2FIFO_REG);
	}
	pthread_mutex_unlock(&g_mutex_kf_lost_mun);
}


void upstate_context_init()
{
	pthread_mutex_init(&g_mutex_kf_lost_mun,NULL);
	add_new_thread(NULL, (void *)&upstate_tsk, 12, 0, 4*1024);
}



