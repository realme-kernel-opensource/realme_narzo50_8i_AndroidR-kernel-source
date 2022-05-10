// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "oplus_charger.h"
#include "oplus_gauge.h"
#include "oplus_debug_info.h"
#include "oplus_voocphy.h"

t_voocphy_log_buf *g_voocphy_log_buf = NULL;

int init_voocphy_log_buf(void)
{
	if(!g_voocphy_log_buf) {
		g_voocphy_log_buf = kzalloc(sizeof(t_voocphy_log_buf), GFP_KERNEL);
		chg_err("sizeof(t_voocphy_log_buf) [%d] sizeof(t_voocphy_info)[%d]", sizeof(t_voocphy_log_buf), sizeof(t_voocphy_info));
		if (g_voocphy_log_buf == NULL) {
			chg_err("voocphy_log_buf kzalloc error");
			return -1;
		}
	} else {
		memset(g_voocphy_log_buf->voocphy_info_buf, 0, VOOCPHY_LOG_BUF_LEN);
	}
	g_voocphy_log_buf->point = 0;
	return 0;
}

int write_info_to_voocphy_log_buf(t_voocphy_info *voocphy)
{
	int curr_point = 0;
	if (voocphy == NULL || g_voocphy_log_buf == NULL) {
		chg_err("voocphy || g_voocphy_log_buf is NULL");
		return -1;
	}
	if (g_voocphy_log_buf->point >= VOOCPHY_LOG_BUF_LEN) {
		chg_err("g_voocphy_log_buf->point[%d] >= VOOCPHY_LOG_BUF_LEN", g_voocphy_log_buf->point);
		return -1;
	}
	curr_point = g_voocphy_log_buf->point;
	//g_voocphy_log_buf->voocphy_info_buf[curr_point]. = 
	memcpy(&g_voocphy_log_buf->voocphy_info_buf[curr_point], voocphy, sizeof(t_voocphy_info));
	curr_point = g_voocphy_log_buf->point++;

	voocphy->kernel_time = 0;
	voocphy->recv_adapter_msg = 0;
	voocphy->irq_took_time = 0;
	voocphy->send_msg = 0;
	return 0;
}

int print_voocphy_log_buf(void)
{
	int curr_point = 0;
	if (g_voocphy_log_buf == NULL) {
		//chg_err("voocphy || g_voocphy_log_buf is NULL");
		return -1;
	}
	if (g_voocphy_log_buf->point <= 0) {
		chg_err("g_voocphy_log_buf->point[%d]", g_voocphy_log_buf->point);
		return -1;
	}

	do {
		chg_err("g_voocphy_log_buf[%d, %d, %d, 0x%x, 0x%x, 0x%x]", curr_point,
			g_voocphy_log_buf->voocphy_info_buf[curr_point].kernel_time,
			g_voocphy_log_buf->voocphy_info_buf[curr_point].irq_took_time,
			g_voocphy_log_buf->voocphy_info_buf[curr_point].recv_adapter_msg,
			g_voocphy_log_buf->voocphy_info_buf[curr_point].send_msg,
			g_voocphy_log_buf->voocphy_info_buf[curr_point].predata);
		curr_point++;
	} while(curr_point < g_voocphy_log_buf->point);
	init_voocphy_log_buf();

	return 0;
}

