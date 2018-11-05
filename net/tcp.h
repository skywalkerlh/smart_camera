/*
 * tcp.h
 *
 *  Created on: 2015年7月28日
 *      Author: work
 */

#ifndef TCP_H_
#define TCP_H_

__s32 tcp_client_create_socket();
__s32 client_connect_server(__s8 *ip_addr);
__s32 check_link_status();
__s32 client_recv_data(void *buf, __u32 len);
__s32 client_send_data(void *buf, __u32 len);
__s32 client_break_socket();

#endif /* TCP_H_ */
