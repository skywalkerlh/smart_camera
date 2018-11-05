#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <linux/types.h>

#include "msg_factory.h"
#include "buf_factory.h"
#include "sys_conf.h"
#include "fpga.h"
#include "lvds_context.h"
#include "video_context.h"
#include "net.h"
#include "dsp_context.h"
#include "local_socket.h"
#include "debug.h"
#include "sys_log.h"
#include "udisk.h"
#include "upstate.h"
#include "ui_socket.h"
#include "key_file.h"
#include "led.h"


void sys_env_init(void)
{
	sigset_t bset, oset;

	/* 进程屏蔽SIGIO信号 */
	sigemptyset(&bset);
	sigaddset(&bset, SIGIO);
	if (pthread_sigmask(SIG_BLOCK, &bset, &oset) != 0)
		perror("pthread_sigmask");

	/* 进程屏蔽SIGPIPE信号 */
	signal(SIGPIPE, SIG_IGN);

	/* 创建消息工厂（管理线程通信的消息）*/
	create_msg_factory();

	/* 创建缓存工厂（128个缓存区，每个16KB。存储所有通讯协议信息）*/
	create_buf_factory(128, 1024<<4);

}


void sys_conf_init(void)
{
	/* 加载系统配置文件 */
	sc_conf_load();

	/* 初始化fpga通信模块  */
	fpga_context_init();

	/* 系统模块初始化  */
	sc_system_init();

}

void sys_module_init(void)
{

	/* 初始化日志模块 */
	log_context_init();

	/* 初始化LVDS通信模块  */
	lvds_context_init();

	/* 视频通道初始化 ，应先于dsp_context_init调用 */
	video_context_init();

	/* 初始化DSP管理模块 */
	dsp_context_init();

	/* 初始化踢废进程通讯模块 */
	local_socket_init();

	/* 初始化u盘模块 （不用）*/
//	udisk_context_init();

	/* 初始化统计上传模块 */
	upstate_context_init();

	/* 初始化ui进程通讯模块 */
	ui_context_init();

	/* 初始化led模块 */
	led_context_init();

	/* 初始化网络通信模块  */
	net_context_init();
}




int main(int argc,char **argv)
{
	pid_t pid;
	__s32 status;

	//系统环境初始化
	sys_env_init();

	//系统配置初始化
	sys_conf_init();

	//系统功能模块初始化
	sys_module_init();


#if 1
	//用子进程启动剔除进程
	if((pid = fork())<0)
	{
		status = -1;
		ERROR("fork");
	}
	else if(pid == 0)
	{
		if( execl( "./reject_process",NULL) < 0)
		{
			perror("execv error ");
			exit(1);
		}
		else
		{
			exit(0);
		}
	}
	else
	{
		;
	}
#endif

	/* 系统与相机的握手  */
	sc_handshake_init(argc);

	while(1)
	{
		sleep(1);
	}
	return 0;
}
