// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "consys_reg_mng.h"
#include "consys_reg_util.h"
#include "mt6877_consys_reg.h"
#include "mt6877_consys_reg_offset.h"


static int consys_reg_init(struct platform_device *pdev);
static int consys_reg_deinit(void);
static int consys_check_reg_readable(void);
static int consys_is_consys_reg(unsigned int addr);
static int consys_is_bus_hang(void);

struct consys_base_addr conn_reg;

struct consys_reg_mng_ops g_dev_consys_reg_ops_mt6877 = {
	.consys_reg_mng_init = consys_reg_init,
	.consys_reg_mng_deinit = consys_reg_deinit,
	.consys_reg_mng_check_reable = consys_check_reg_readable,
	.consys_reg_mng_is_bus_hang = consys_is_bus_hang,
	.consys_reg_mng_is_consys_reg = consys_is_consys_reg,

};


static const char* consys_base_addr_index_to_str[CONSYS_BASE_ADDR_MAX] = {
	"conn_infra_rgu",
	"conn_infra_cfg",
	"conn_infra_clkgen_on_top",
	"conn_infra_bus_cr",
	"conn_host_csr_top",
	"infracfg_ao",
	"GPIO",
	"spm",
	"top_rgu",
	"conn_wt_slp_ctl_reg",
	"conn_infra_sysram_sw_cr",
	"conn_afe_ctl",
	"conn_semaphore",
	"conn_rf_spi_mst_reg",
	"IOCFG_RT",
	"conn_therm_ctl",
	"conn_bcrm_on",
};

struct consys_base_addr* get_conn_reg_base_addr()
{
	return &conn_reg;
}

int consys_reg_get_reg_symbol_num(void)
{
	return CONSYS_BASE_ADDR_MAX;
}

int consys_is_consys_reg(unsigned int addr)
{
	if (addr >= 0x18000000 && addr < 0x19000000)
		return 1;
	return 0;
}

static void consys_bus_hang_dump_a(void)
{
	unsigned int r1, r2;

	/* AP2CONN_INFRA ON
	 * 1. Check ap2conn gals sleep protect status
	 * 	- 0x1000_1220 [13] / 0x1000_1228 [13](rx/tx)
	 * 	(sleep protect enable ready)
	 * 	both of them should be 1'b0 ?(CR at ap side)
	 */
	r1 = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 13));
	r2 = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, (0x1 << 13));
	pr_info("[CONN_BUS_A][0x%08x][0x%08x]", r1, r2);
}

static void consys_bus_hang_dump_b(void)
{
}

static void consys_bus_hang_dump_c_host_csr(void)
{
}

static void consys_bus_hang_dump_c(void)
{
	consys_bus_hang_dump_c_host_csr();
}

static int consys_is_bus_hang(void)
{
	unsigned int r1, r2;
	unsigned int ret = 0;

	consys_bus_hang_dump_a();
	/* AP2CONN_INFRA ON
	 * 1. Check ap2conn gals sleep protect status
	 * 	- 0x1000_1220 [13] / 0x1000_1228 [13](rx/tx)
	 * 	(sleep protect enable ready)
	 * 	both of them should be 1'b0 ?(CR at ap side)
	 */
	r1 = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 13));
	r2 = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, (0x1 << 13));
	if (r1)
		ret = CONNINFRA_AP2CONN_RX_SLP_PROT_ERR;
	if (r2)
		ret = CONNINFRA_AP2CONN_TX_SLP_PROT_ERR;
	if (ret)
		return ret;
	consys_bus_hang_dump_b();

	/* AP2CONN_INFRA OFF
	 * 1.Check "AP2CONN_INFRA ON step is ok"
	 * 2. Check conn_infra off bus clock
	 * 	- write 0x1 to 0x1806_0000[0], reset clock detect
	 * 	- 0x1806_0000[2]: conn_infra off bus clock (should be 1'b1 if clock exist)
	 * 	- 0x1806_0000[1]: osc clock (should be 1'b1 if clock exist)
	 * 3. Read conn_infra IP version
	 * 	- Read 0x1800_1000 = 0x02060000 
	 * 4 Check conn_infra off domain bus hang irq status
	 * 	- 0x1806_014C[0], should be 1'b0, or means conn_infra off bus might hang
	 */
	CONSYS_SET_BIT(CONN_HOST_CSR_TOP_BUS_MCU_STAT_ADDR, (0x1 << 0));
	r1 = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_BUS_MCU_STAT_ADDR, ((0x1 << 2) || (0x1 << 1)));
	r2 = CONSYS_REG_READ(CONN_CFG_IP_VERSION_ADDR);
	if (r1 != 0x6 || r2 != CONN_HW_VER) {
		consys_bus_hang_dump_c_host_csr();
		return 0;
	}
	r1 = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_CONN_INFRA_ON_BUS_TIMEOUT_IRQ_ADDR, (0x1 << 0));
	if (r1 != 0x0) {
		pr_err("conninfra off bus might hang cirq=[0x%08x]", r1);
		consys_bus_hang_dump_c();
		return CONNINFRA_INFRA_BUS_HANG_IRQ;
	}

	consys_bus_hang_dump_c();
	return 0;
}

int consys_check_reg_readable(void)
{
	unsigned int r, r1, r2;

	/* AP2CONN_INFRA ON
	 * 1. Check ap2conn gals sleep protect status
	 * 	- 0x1000_1220 [13] / 0x1000_1228 [13](rx/tx)
	 * 	(sleep protect enable ready)
	 * 	both of them should be 1'b0  (CR at ap side)
	 */
	r1 = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 13));
	r2 = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, (0x1 << 13));
	if (r1 || r2) {
		return 0;
	}

	/* AP2CONN_INFRA OFF
	 * 1.Check "AP2CONN_INFRA ON step is ok"
	 * 2. Check conn_infra off bus clock
	 * 	- write 0x1 to 0x1806_0000[0], reset clock detect
	 * 	- 0x1806_0000[2]  conn_infra off bus clock (should be 1'b1 if clock exist)
	 * 	- 0x1806_0000[1]  osc clock (should be 1'b1 if clock exist)
	 * 3. Read conn_infra IP version
	 * 	- Read 0x1800_1000 = 0x02060000 
	 * 4 Check conn_infra off domain bus hang irq status
	 * 	- 0x1806_014C[0], should be 1'b0, or means conn_infra off bus might hang
	 */
	CONSYS_SET_BIT(CONN_HOST_CSR_TOP_BUS_MCU_STAT_ADDR, (0x1 << 0));
	r = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_BUS_MCU_STAT_ADDR, ((0x1 << 2) || (0x1 << 1)));
	if (r != 0x6)
		return 0;
	if (CONSYS_REG_READ(CONN_CFG_IP_VERSION_ADDR) != CONN_HW_VER)
		return 0;
	r = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_CONN_INFRA_ON_BUS_TIMEOUT_IRQ_ADDR, (0x1 << 0));
	if (r != 0x0)
		return 0;
	return 1;
}

int consys_reg_init(struct platform_device *pdev)
{
	int ret = -1;
	struct device_node *node = NULL;
	struct consys_reg_base_addr *base_addr = NULL;
	struct resource res;
	int flag, i = 0;

	node = pdev->dev.of_node;
	pr_info("[%s] node=[%p]\n", __func__, node);
	if (node) {
		for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
			base_addr = &conn_reg.reg_base_addr[i];

			ret = of_address_to_resource(node, i, &res);
			if (ret) {
				pr_err("Get Reg Index(%d-%s) failed",
						i, consys_base_addr_index_to_str[i]);
				continue;
			}

			base_addr->phy_addr = res.start;
			base_addr->vir_addr =
				(unsigned long) of_iomap(node, i);
			of_get_address(node, i, &(base_addr->size), &flag);

			pr_info("Get Index(%d-%s) phy(0x%zx) baseAddr=(0x%zx) size=(0x%zx)",
				i, consys_base_addr_index_to_str[i], base_addr->phy_addr,
				base_addr->vir_addr, base_addr->size);
		}

	} else {
		pr_err("[%s] can't find CONSYS compatible node\n", __func__);
		return ret;
	}
	return 0;

}

static int consys_reg_deinit(void)
{
	int i = 0;

	for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
		if (conn_reg.reg_base_addr[i].vir_addr) {
			pr_info("[%d] Unmap %s (0x%zx)",
				i, consys_base_addr_index_to_str[i],
				conn_reg.reg_base_addr[i].vir_addr);
			iounmap((void __iomem*)conn_reg.reg_base_addr[i].vir_addr);
			conn_reg.reg_base_addr[i].vir_addr = 0;
		}
	}

	return 0;
}


