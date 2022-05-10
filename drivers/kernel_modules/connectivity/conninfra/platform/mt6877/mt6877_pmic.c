// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <connectivity_build_in_adapter.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <pmic_api_buck.h>
#include <upmu_common.h>

#include "consys_hw.h"
#include "pmic_mng.h"
#include "mt6877_pos.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
enum vcn13_state {
	vcn13_1_3v = 0,
	vcn13_1_32v = 1,
	vcn13_1_37v = 2,
};
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
static atomic_t g_voltage_change_status = ATOMIC_INIT(0);
static struct timer_list g_voltage_change_timer;

static struct regulator *reg_VCN13;
static struct regulator *reg_VCN18;
static struct regulator *reg_VCN33_1_BT;
static struct regulator *reg_VCN33_1_WIFI;
static struct regulator *reg_VCN33_2_WIFI;

static struct conninfra_dev_cb* g_dev_cb;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static int consys_plt_pmic_get_from_dts_mt6877(struct platform_device*, struct conninfra_dev_cb*);

static int consys_plt_pmic_common_power_ctrl_mt6877(unsigned int);
static int consys_plt_pmic_common_power_low_power_mode_mt6877(unsigned int);
static int consys_plt_pmic_wifi_power_ctrl_mt6877(unsigned int);
static int consys_plt_pmic_bt_power_ctrl_mt6877(unsigned int);
static int consys_plt_pmic_gps_power_ctrl_mt6877(unsigned int);
static int consys_plt_pmic_fm_power_ctrl_mt6877(unsigned int);
/* VCN33_1 is enable when BT or Wi-Fi is on */
static int consys_pmic_vcn33_1_power_ctl_mt6877(bool, struct regulator*);
/* VCN33_2 is enable when Wi-Fi is on */
static int consys_pmic_vcn33_2_power_ctl_mt6877(bool);

static int consys_plt_pmic_raise_voltage_mt6877(unsigned int, bool, bool);
static void consys_plt_pmic_raise_voltage_timer_handler_mt6877(unsigned long);

const struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6877 = {
	.consys_pmic_get_from_dts = consys_plt_pmic_get_from_dts_mt6877,
	/* vcn 18 */
	.consys_pmic_common_power_ctrl = consys_plt_pmic_common_power_ctrl_mt6877,
	.consys_pmic_common_power_low_power_mode = consys_plt_pmic_common_power_low_power_mode_mt6877,
	.consys_pmic_wifi_power_ctrl = consys_plt_pmic_wifi_power_ctrl_mt6877,
	.consys_pmic_bt_power_ctrl = consys_plt_pmic_bt_power_ctrl_mt6877,
	.consys_pmic_gps_power_ctrl = consys_plt_pmic_gps_power_ctrl_mt6877,
	.consys_pmic_fm_power_ctrl = consys_plt_pmic_fm_power_ctrl_mt6877,
	.consys_pmic_raise_voltage = consys_plt_pmic_raise_voltage_mt6877,
};

int consys_plt_pmic_get_from_dts_mt6877(struct platform_device *pdev, struct conninfra_dev_cb* dev_cb)
{
	g_dev_cb = dev_cb;
	reg_VCN13 = devm_regulator_get_optional(&pdev->dev, "vcn13");
	if (!reg_VCN13)
		pr_err("Regulator_get VCN_13 fail\n");
	reg_VCN18 = regulator_get(&pdev->dev, "vcn18");
	if (!reg_VCN18)
		pr_err("Regulator_get VCN_18 fail\n");
	reg_VCN33_1_BT = regulator_get(&pdev->dev, "vcn33_1_bt");
	if (!reg_VCN33_1_BT)
		pr_err("Regulator_get VCN33_1_BT fail\n");
	reg_VCN33_1_WIFI = regulator_get(&pdev->dev, "vcn33_1_wifi");
	if (!reg_VCN33_1_WIFI)
		pr_err("Regulator_get VCN33_1_WIFI fail\n");
	reg_VCN33_2_WIFI = regulator_get(&pdev->dev, "vcn33_2_wifi");
	if (!reg_VCN33_2_WIFI)
		pr_err("Regulator_get VCN33_WIFI fail\n");

	init_timer(&g_voltage_change_timer);
	g_voltage_change_timer.function = consys_plt_pmic_raise_voltage_timer_handler_mt6877;
	return 0;
}

int consys_plt_pmic_common_power_ctrl_mt6877(unsigned int enable)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else
	int ret;

	if (enable) {
		if (consys_is_rc_mode_enable_mt6877()) {
			/* RC mode */
			/* VCN18 */
			/*  PMRC_EN[7][6][5][4] HW_OP_EN = 1, HW_OP_CFG = 0 */
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN7, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN6, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN5, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN4, 0, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_LP, 0);
			regulator_set_voltage(reg_VCN18, 1800000, 1800000);
			ret = regulator_enable(reg_VCN18);
			if (ret)
				pr_err("Enable VCN18 fail. ret=%d\n", ret);

			/* VCN13 */
			/*  PMRC_EN[7][6][5][4] HW_OP_EN = 1, HW_OP_CFG = 0 */
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN7, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN6, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN5, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN4, 0, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN13_LP, 0);
			regulator_set_voltage(reg_VCN13, 1300000, 1300000);
			ret = regulator_enable(reg_VCN13);
			if (ret)
				pr_err("Enable VCN13 fail. ret=%d\n", ret);
		} else {
			/* Legacy mode */
			/* HW_OP_EN = 1, HW_OP_CFG = 1 */
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN0, 1, 1, HW_LP);
			/* SW_LP=0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_LP, 0);
			regulator_set_voltage(reg_VCN18, 1800000, 1800000);
			/* SW_EN=1 */
			ret = regulator_enable(reg_VCN18);
			if (ret)
				pr_err("Enable VCN18 fail. ret=%d\n", ret);

			/* HW_OP_EN = 1, HW_OP_CFG = 1 */
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN0, 1, 1, HW_LP);
			regulator_set_voltage(reg_VCN13, 1300000, 1300000);
			/* SW_LP=0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN13_LP, 0);
			/* SW_EN=1 */
			ret = regulator_enable(reg_VCN13);
			if (ret)
				pr_err("Enable VCN13 fail. ret=%d\n", ret);
		}
	} else {
		regulator_disable(reg_VCN13);
		regulator_disable(reg_VCN18);
	}
#endif
	return 0;
}

int consys_plt_pmic_common_power_low_power_mode_mt6877(unsigned int enable)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else
	if (enable) {
		/* SW_LP =1 */
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_LP, 1);
		/* SW_LP =1 */
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN13_LP, 1);
	}
#endif
	return 0;
}

int consys_plt_pmic_wifi_power_ctrl_mt6877(unsigned int enable)
{
	int ret;

	ret = consys_pmic_vcn33_1_power_ctl_mt6877(enable, reg_VCN33_1_WIFI);
	if (ret)
		pr_err("%s VCN33_1 fail\n", (enable? "Enable" : "Disable"));
	ret = consys_pmic_vcn33_2_power_ctl_mt6877(enable);
	if (ret)
		pr_err("%s VCN33_2 fail\n", (enable? "Enable" : "Disable"));
	return ret;
}

int consys_plt_pmic_bt_power_ctrl_mt6877(unsigned int enable)
{
	return consys_pmic_vcn33_1_power_ctl_mt6877(enable, reg_VCN33_1_BT);
}

int consys_plt_pmic_gps_power_ctrl_mt6877(unsigned int enable)
{
	return 0;
}

int consys_plt_pmic_fm_power_ctrl_mt6877(unsigned int enable)
{
	return 0;
}

int consys_pmic_vcn33_1_power_ctl_mt6877(bool enable, struct regulator* reg_VCN33_1)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else
	int ret;
	if (enable) {
		if (consys_is_rc_mode_enable_mt6877()) {
			/*  PMRC_EN[6][5]  HW_OP_EN = 1, HW_OP_CFG = 0  */
			KERNEL_pmic_ldo_vcn33_1_lp(SRCLKEN6, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn33_1_lp(SRCLKEN5, 0, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_LP, 0);
			regulator_set_voltage(reg_VCN33_1, 3300000, 3300000);
			/* SW_EN=0 */
			/* For RC mode, we don't have to control VCN33_1 & VCN33_2 */
		#if 0
			/* regulator_disable(reg_VCN33_1); */
		#endif
		} else {
			/* HW_OP_EN = 1, HW_OP_CFG = 0 */
			KERNEL_pmic_ldo_vcn33_1_lp(SRCLKEN0, 1, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_LP, 0);
			regulator_set_voltage(reg_VCN33_1, 3300000, 3300000);
			/* SW_EN=1 */
			ret = regulator_enable(reg_VCN33_1);
			if (ret)
				pr_err("Enable VCN33_1 fail. ret=%d\n", ret);
		}
	} else {
		if (consys_is_rc_mode_enable_mt6877()) {
			/* Do nothing */
		} else {
			regulator_disable(reg_VCN33_1);
		}
	}
#endif
	return 0;
}

int consys_pmic_vcn33_2_power_ctl_mt6877(bool enable)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else
	int ret;

	if (enable) {
		if (consys_is_rc_mode_enable_mt6877()) {
			/*  PMRC_EN[6]  HW_OP_EN = 1, HW_OP_CFG = 0  */
			KERNEL_pmic_ldo_vcn33_2_lp(SRCLKEN6, 0, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_2_LP, 0);
			regulator_set_voltage(reg_VCN33_2_WIFI, 3300000, 3300000);
			/* SW_EN=0 */
			/* For RC mode, we don't have to control VCN33_1 & VCN33_2 */
		#if 0
			regulator_disable(reg_VCN33_2_WIFI);
		#endif
		} else  {
			/* HW_OP_EN = 1, HW_OP_CFG = 0 */
			KERNEL_pmic_ldo_vcn33_2_lp(SRCLKEN0, 1, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_2_LP, 0);
			regulator_set_voltage(reg_VCN33_2_WIFI, 3300000, 3300000);
			/* SW_EN=1 */
			ret = regulator_enable(reg_VCN33_2_WIFI);
			if (ret)
				pr_err("Enable VCN33_2 fail. ret=%d\n", ret);
		}
	} else {
		if (consys_is_rc_mode_enable_mt6877()) {
			/* Do nothing */
		} else {
			regulator_disable(reg_VCN33_2_WIFI);
		}
	}
#endif

	return 0;
}


static void consys_raise_vcn13_vs2_voltage_mt6877(enum vcn13_state next_state)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	static enum vcn13_state curr_vcn13_state = vcn13_1_3v;

	/* no change */
	if (curr_vcn13_state == next_state) {
		pr_info("[%s] curr==next_state(%d, %d), return\n", __func__, curr_vcn13_state, next_state);
		return;
	}
	pr_info("[%s] curr_vcn13_state=%d next_state=%d\n", __func__, curr_vcn13_state, next_state);
	/* Check raise window, the duration to previous action should be 1 ms. */
	while (atomic_read(&g_voltage_change_status) == 1);
	pr_info("[%s] check down\n", __func__);
	curr_vcn13_state = next_state;

	switch (curr_vcn13_state) {
		case vcn13_1_3v:
			/* restore VCN13 to 1.3V */
			KERNEL_pmic_set_register_value(PMIC_RG_VCN13_VOCAL, 0);
			/* Restore VS2 sleep voltage to 1.35V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOSEL_SLEEP, 0x2C);
			/* clear bit 4 of VS2 VOTER then VS2 can restore to 1.35V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOTER_EN_CLR, 0x10);
			break;
		case vcn13_1_32v:
			/* Set VS2 to 1.4V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOSEL, 0x30);
			/* request VS2 to 1.4V by VS2 VOTER (use bit 4) */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOTER_EN_SET, 0x10);
			/* Restore VS2 sleep voltage to 1.35V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOSEL_SLEEP, 0x2C);
			/* Set VCN13 to 1.32V */
			KERNEL_pmic_set_register_value(PMIC_RG_VCN13_VOCAL, 0x2);
			break;
		case vcn13_1_37v:
			/* Set VS2 to 1.4625V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOSEL, 0x35);
			/* request VS2 to 1.4V by VS2 VOTER (use bit 4) */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOTER_EN_SET, 0x10);
			/* Set VS2 sleep voltage to 1.375V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOSEL_SLEEP, 0x2E);
			/* Set VCN13 to 1.37V */
			KERNEL_pmic_set_register_value(PMIC_RG_VCN13_VOCAL, 0x7);
			break;
	}
	udelay(50);

	/* start timer */
	atomic_set(&g_voltage_change_status, 1);
	mod_timer(&g_voltage_change_timer, jiffies + HZ/1000);
#endif
}

int consys_plt_pmic_raise_voltage_mt6877(unsigned int drv_type, bool raise, bool onoff)
{
	static bool bt_raise = false;
	pr_info("[%s] [drv_type(%d) raise(%d) onoff(%d)][bt_raise(%d)]\n",
		__func__, drv_type, raise, onoff, bt_raise);
	if (drv_type == 0 && onoff) {
		bt_raise = raise;
	} else {
		return 0;
	}
	if (bt_raise) {
		consys_raise_vcn13_vs2_voltage_mt6877(vcn13_1_37v);
	} else {
		consys_raise_vcn13_vs2_voltage_mt6877(vcn13_1_3v);
	}
	return 0;
}

void consys_plt_pmic_raise_voltage_timer_handler_mt6877(unsigned long data)
{
	atomic_set(&g_voltage_change_status, 0);
}

