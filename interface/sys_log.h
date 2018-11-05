/*
 * sys_log.h
 *
 *  Created on: 2015年9月16日
 *      Author: work
 */

#ifndef SYS_LOG_H_
#define SYS_LOG_H_

void log_builder(void* log_data);
void logging_tsk(void);
void log_context_init();

#endif /* SYS_LOG_H_ */
