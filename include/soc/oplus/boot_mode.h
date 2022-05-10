/************************************************************************************
** File: - android\kernel\arch\arm\mach-msm\include\mach\oplus_boot.h
** Copyright (C), 2008-2012, OPLUS Mobile Comm Corp., Ltd
** 
** Description:  
**     change define of boot_mode here for other place to use it
** Version: 1.0 
** --------------------------- Revision History: --------------------------------
** 	<author>	<data>			<desc>
** tong.han@BasicDrv.TP&LCD 11/01/2014 add this file
************************************************************************************/
#ifndef _OPLUS_BOOT_H
#define _OPLUS_BOOT_H
#include <soc/oplus/boot_mode_types.h>
//Ke.Li@ROM.Security, KernelEvent, 2019-9-30, remove the "static" flag for kernel kevent
extern int get_boot_mode(void);
//Fuchun.Liao@Mobile.BSP.CHG 2016-01-14 add for charge
extern bool qpnp_is_power_off_charging(void);
//PengNan@SW.BSP add for detect charger when reboot 2016-04-22
extern bool qpnp_is_charger_reboot(void);
#endif
