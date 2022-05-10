# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux Kernel SOC specific device drivers.
#

obj-$(CONFIG_ARCH_ACTIONS)	+= actions/
obj-$(CONFIG_ARCH_AT91)		+= atmel/
obj-y				+= bcm/
obj-$(CONFIG_ARCH_DOVE)		+= dove/
obj-$(CONFIG_MACH_DOVE)		+= dove/
obj-y				+= fsl/
obj-y				+= mediatek/
obj-$(CONFIG_ARCH_MXC)		+= imx/
obj-$(CONFIG_SOC_XWAY)		+= lantiq/
obj-y						+= mediatek/
obj-$(CONFIG_ARCH_MESON)	+= amlogic/
obj-$(CONFIG_ARCH_QCOM)		+= qcom/
obj-y				+= renesas/
obj-$(CONFIG_ARCH_ROCKCHIP)	+= rockchip/
obj-$(CONFIG_SOC_SAMSUNG)	+= samsung/
obj-$(CONFIG_ARCH_SUNXI)	+= sunxi/
obj-$(CONFIG_ARCH_TEGRA)	+= tegra/
obj-$(CONFIG_SOC_TI)		+= ti/
obj-$(CONFIG_ARCH_U8500)	+= ux500/
obj-$(CONFIG_PLAT_VERSATILE)	+= versatile/
obj-$(CONFIG_ARCH_ZX)		+= zte/
obj-y               += oplus/oplus/
#ifdef OPLUS_FEATURE_TP_BASIC
#obj-$(CONFIG_OPLUS_ARCH_FILE)	+= oplus/oplus/
#endif /*OPLUS_FEATURE_TP_BASIC*/

#ifdef OPLUS_FEATURE_TP_BASIC
#obj-$(CONFIG_OPLUS_ARCH_FILE)    += oplus/oplus/
#endif /*OPLUS_FEATURE_TP_BASIC*/

#ifdef OPLUS_FEATURE_IOMONITOR
obj-$(CONFIG_IOMONITOR)    +=  oplus/iomonitor/
#endif /*OPLUS_FEATURE_IOMONITOR*/

#ifdef OPLUS_FEATURE_TP_BASIC
#wen.luo@BSP.Kernel.Stability 2020/03/25 add for oplus kernel system
obj-$(CONFIG_OPLUS_SYSTEM_KERNEL)	+= oplus/system/
#endif /* OPLUS_FEATURE_TP_BASIC */

#ifdef OPLUS_FEATURE_HEALTHINFO
obj-y     +=  oplus/healthinfo/
#endif /* OPLUS_FEATURE_HEALTHINFO */

#ifdef OPLUS_FEATURE_LOWMEM_DBG
obj-$(CONFIG_OPLUS_FEATURE_LOWMEM_DBG)	+=  oplus/lowmem_dbg/
#endif /* OPLUS_FEATURE_LOWMEM_DBG */

#ifdef OPLUS_FEATURE_NFC_CONSOFT
obj-$(CONFIG_OPLUS_NFC)     +=  oplus/oplus_nfc/
#endif /* OPLUS_FEATURE_NFC_CONSOFT */

#Add for NandSwap
obj-$(CONFIG_NANDSWAP)		+= oplus/oplus_nandswap/

# wangcheng@ODM.Multimedia.LCD  2021/05/01 add for devinfo, begin.
#ifdef CONFIG_OPLUS_DISPLAY_MTK6781
obj-y +=  oplus/device_info/
#endif
# wangcheng@ODM.Multimedia.LCD  2021/05/01 add for devinfo, end.

obj-y +=  oplus/oplus_custom/
