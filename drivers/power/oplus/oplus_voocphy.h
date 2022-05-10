/* SPDX-License-Identifier: GPL-2.0-only  */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef _OPLUS_VOOCPHY_H_
#define _OPLUS_VOOCPHY_H_

#define VOOCPHY_LOG_BUF_LEN 1024


typedef struct _t_voocphy_info{
	int kernel_time;
	int irq_took_time;
	int recv_adapter_msg;
	unsigned char fastchg_adapter_ask_cmd;
	int predata;
	int send_msg;
}t_voocphy_info;

typedef struct _t_voocphy_log_buf{
	t_voocphy_info voocphy_info_buf[VOOCPHY_LOG_BUF_LEN];
	int point;
}t_voocphy_log_buf;

int init_voocphy_log_buf(void);
int write_info_to_voocphy_log_buf(t_voocphy_info *voocphy);
int print_voocphy_log_buf(void);

#endif /* _OPLUS_VOOCPHY_H_ */
