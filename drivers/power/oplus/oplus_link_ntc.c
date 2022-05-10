/**********************************************************************************
* Copyright (c)  2008-2015  zhangchao hauqin Mobile Comm Corp., Ltd
* ODM_HQ_EDIT
* Description: USB NTC 1
* Version   : 1.0
* Date      : 2019-09-17
* Author    : zhangchao_hq
* ------------------------------ Revision History: --------------------------------
* <version>         <date>              <author>                      <desc>
* Revision 1.0    2019-09-17      zhangchao_hq                 Created for USB NTC 1
***********************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <mt-plat/aee.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/uaccess.h>
//#include "mt-plat/mtk_thermal_monitor.h"
//#include "mach/mtk_thermal.h"
//#include "mtk_thermal_timer.h"
#include <linux/uidgid.h>
#include <linux/slab.h>
#if defined(CONFIG_MEDIATEK_MT6577_AUXADC)
#include <linux/iio/consumer.h>
#endif



extern struct iio_channel	*batt_id;

#if defined(CONFIG_MEDIATEK_MT6577_AUXADC)
//struct iio_channel *thermistor_ch2 = NULL;
#endif

int oplus_link_ntc_adc2_read(void)
{
	int adc_volt = 0;
	int ret = 0;
	int auxadc_voltage = 0;

	//printk("%s:enter\n", __func__);

#if defined(CONFIG_MEDIATEK_MT6577_AUXADC)
	if (batt_id)
		//oplus_linktem_gpio_enable();	//open link_temp gpio


		ret = iio_read_channel_processed(batt_id, &auxadc_voltage);

		//oplus_bat_id_tem_gpio_disable();


#endif

	printk("[%s]link_ntc_adc2 auxadc_voltage is %d\n", __func__, auxadc_voltage);
	adc_volt = auxadc_voltage * 1500 / 4096;
	printk("[%s]link_ntc_adc2 voltage is %d\n", __func__, adc_volt);

	return adc_volt;
}
#if defined(CONFIG_MEDIATEK_MT6577_AUXADC)
static int link_ntc_probe(struct platform_device *pdev)
{
	int err = 0;
	/*int ret = 0;

	printk("[%s]\n", __func__);

	if (!pdev->dev.of_node) {
		printk("[%s] Only DT based supported\n",
			__func__);
		return -ENODEV;
	}

	thermistor_ch2 = devm_kzalloc(&pdev->dev, sizeof(*thermistor_ch2),
		GFP_KERNEL);
	if (!thermistor_ch2)
		return -ENOMEM;


	thermistor_ch2 = iio_channel_get(&pdev->dev, "auxadc2-batt_id_v");
	ret = IS_ERR(thermistor_ch2);
	if (ret) {
		printk("[%s] fail to get auxadc iio ch2: %d\n",
			__func__, ret);
		return ret;
	}*/

	oplus_link_ntc_adc2_read();

	return err;
}

#ifdef CONFIG_OF
const struct of_device_id link_adc2_of_match[2] = {
	{.compatible = "mediatek,link_ntc",},
	{},
};
#endif

#define USB_NTC_NAME    "link_ntc"
static struct platform_driver link_ntc_adc2 = {
	.remove = NULL,
	.shutdown = NULL,
	.probe = link_ntc_probe,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		.name = USB_NTC_NAME,
#ifdef CONFIG_OF
		.of_match_table = link_adc2_of_match,
#endif
	},
};
#endif /*CONFIG_MEDIATEK_MT6577_AUXADC*/


static int __init link_ntc_adc2_init(void)
{
#if defined(CONFIG_MEDIATEK_MT6577_AUXADC)
	int err = 0;
#endif

	printk("[%s]\n", __func__);


#if defined(CONFIG_MEDIATEK_MT6577_AUXADC)
	err = platform_driver_register(&link_ntc_adc2);
	if (err) {
		printk("link_ntc_adc2 driver callback register failed.\n");
		return err;
	}
#endif

	return 0;
}

static void __exit link_ntc_adc2_exit(void)
{
	printk("[%s]\n", __func__);
}

module_init(link_ntc_adc2_init);
module_exit(link_ntc_adc2_exit);
