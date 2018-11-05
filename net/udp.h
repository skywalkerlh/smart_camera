/*
 * udp.h
 *
 *  Created on: 2015年7月28日
 *      Author: work
 */

#ifndef UDP_H_
#define UDP_H_

extern __s32 udp_socket_create();
extern __s32 check_dhcp_state();
extern __s32 shakehand_with_pc(char *ip_addr, struct sockaddr_in *pc);

#endif /* UDP_H_ */
