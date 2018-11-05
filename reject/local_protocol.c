/*
 * protocol.c
 *
 *  Created on: 2015年8月11日
 *      Author: work
 */

#include <linux/types.h>
#include <stddef.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#include "msg_factory.h"
#include "local_protocol.h"
#include "local_socket.h"
#include "mailbox.h"
#include "fpga.h"
#include "sys_conf.h"
#include "net_context.h"
#include "net_protocol.h"

#define SAMPLE_ARRAY_MAX 64

extern sem_t gsem_reject_reset;
SampleResult sample_result[SAMPLE_ARRAY_MAX];
__u32 sample_result_index = 0;

static DisInfo dis_info;



/*******************主进程发送协议********************/
/*握手发送*/
static struct LocalInfoHead handshake_head =
{
		.type = 0xAACC0100,
		.length = 4
};

void send_handshake_cmd()
{
	struct message *msg2reject;
	msg2reject = msg_factory_produce(g_local_mbx_id, 0);
	msg2reject->ops->set_data(msg2reject, &handshake_head, sizeof(handshake_head), NULL, 0);
	mailbox_post(msg2reject);
}


/*样品结果信息发送*/
static struct LocalInfoHead sample_result_head =
{
		.type = 0xAACC0102,
		.length = 4 + sizeof(struct SampleResult)
};


void send_sample_reault(void *buf, __u32 len)
{
	struct message *msg2reject;
	if(check_sys_reset_flag())
	{
		return;
	}
	msg2reject = msg_factory_produce(g_local_mbx_id, 0);
	msg2reject->ops->set_data(msg2reject, &sample_result_head, sizeof(sample_result_head), NULL, 0);
	msg2reject->ops->set_data(msg2reject, buf, len, NULL, 0);
	mailbox_post(msg2reject);
}


SampleResult* get_sample_result_array()
{
	SampleResult* SampleResult;
	pthread_mutex_lock(&g_mutex_array);
	sample_result_index++;
	if (sample_result_index >= SAMPLE_ARRAY_MAX)
		sample_result_index = 0;

	SampleResult = &(sample_result[sample_result_index]);
	pthread_mutex_unlock(&g_mutex_array);
	return SampleResult;
}

/*复位指令发送*/
static struct LocalInfoHead reset_cmd_head =
{
		.type = 0xAACC0101,
		.length = 4
};

void send_reset_cmd()
{
	struct message *msg2reject;
	msg2reject = msg_factory_produce(g_local_mbx_id, 0);
	msg2reject->ops->set_data(msg2reject, &reset_cmd_head, sizeof(reset_cmd_head), NULL, 0);
	mailbox_post(msg2reject);
}

/*测试安装位置*/
static struct LocalInfoHead test_pos_head =
{
		.type = 0xAACC0103,
		.length = 4 + 4
};

void send_test_pos(__u32 item)
{
	struct message *msg2reject;
	static __u32 s_item;
	__u16 isr_val;

	//根据项目进行中断开关
	if(item == 0)
	{
		isr_val = FPGA_READ16(fpga_base, FPGA_CLR_REG);
		FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val|0x02);

		isr_val = FPGA_READ16(fpga_base, FPGA_ISRCLR_REG1);
		FPGA_WRITE16(fpga_base, FPGA_ISRCLR_REG1, isr_val|0x06);
		isr_val = FPGA_READ16(fpga_base, FPGA_ISRCLR_REG2);
		FPGA_WRITE16(fpga_base, FPGA_ISRCLR_REG2, isr_val|0x06);

		s_item = item;
		msg2reject = msg_factory_produce(g_local_mbx_id, 0);
		msg2reject->ops->set_data(msg2reject, &test_pos_head, sizeof(test_pos_head), NULL, 0);
		msg2reject->ops->set_data(msg2reject, &s_item, 4, NULL, 0);
		mailbox_post(msg2reject);
	}
	else if(item == 1)
	{
		isr_val = FPGA_READ16(fpga_base, FPGA_CLR_REG);
		FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val&0xFFFD);

		isr_val = FPGA_READ16(fpga_base, FPGA_ISRCLR_REG1);
		FPGA_WRITE16(fpga_base, FPGA_ISRCLR_REG1, isr_val|0x06);
		isr_val = FPGA_READ16(fpga_base, FPGA_ISRCLR_REG2);
		FPGA_WRITE16(fpga_base, FPGA_ISRCLR_REG2, isr_val|0x06);

		s_item = item;
		msg2reject = msg_factory_produce(g_local_mbx_id, 0);
		msg2reject->ops->set_data(msg2reject, &test_pos_head, sizeof(test_pos_head), NULL, 0);
		msg2reject->ops->set_data(msg2reject, &s_item, 4, NULL, 0);
		mailbox_post(msg2reject);
	}
	else
	{
		isr_val = FPGA_READ16(fpga_base, FPGA_CLR_REG);
		FPGA_WRITE16(fpga_base, FPGA_CLR_REG, isr_val|0x02);
		s_item = item;
		msg2reject = msg_factory_produce(g_local_mbx_id, 0);
		msg2reject->ops->set_data(msg2reject, &test_pos_head, sizeof(test_pos_head), NULL, 0);
		msg2reject->ops->set_data(msg2reject, &s_item, 4, NULL, 0);
		mailbox_post(msg2reject);
	}
}


/*******************主进程接收协议********************/

/*握手应答*/
static void recv_shack_response(__u8 *param)
{
	struct MainBoardVersion *pVersion;
	pVersion = sc_get_main_board_version(NULL);
	strcpy(pVersion->kick_ver, param);
}

/*复位应答*/
static void recv_reset_response()
{
	sem_post(&gsem_reject_reset);
}

/*安装位置测试报告*/
static void recv_pos_report(__u32 *param)
{
	dis_info.item = *param;
	dis_info.length = *(param + 1);
	send_length_info(&dis_info, sizeof(dis_info));
}

void local_cmd_analyze(Clt_CMD_STR* CltCmd)
{
	if(CltCmd->cmdword == 0xAACC0200)
	{
		recv_shack_response(CltCmd->param);
	}
	if(CltCmd->cmdword == 0xAACC0201)
	{
		recv_reset_response();
	}
	else if(CltCmd->cmdword == 0xAACC0204)
	{
		recv_pos_report((__u32 *)CltCmd->param);
	}
	else
		;
}






