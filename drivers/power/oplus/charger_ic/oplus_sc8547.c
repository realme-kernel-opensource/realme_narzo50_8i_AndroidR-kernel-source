/*
 * SC8547 battery charging driver
*/

#define pr_fmt(fmt)	"[sc8547] %s: " fmt, __func__

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/err.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/debugfs.h>
#include <linux/bitops.h>
#include <linux/math64.h>
//#include <linux/wakelock.h>
#include <linux/proc_fs.h>

#include <trace/events/sched.h>
#include<linux/ktime.h>
#include <linux/pm_qos.h>
#include "../oplus_vooc.h"
#include "../oplus_gauge.h"
#include "../oplus_charger.h"
#include "oplus_sc8547.h"
#include "vooc_base.h"
#include "oplus_vooc_20031.h"

typedef enum {
	ADC_VBUS,
	ADC_VAC,
	ADC_VOUT,
	ADC_VBAT,
	ADC_IBAT,
	ADC_IBUS,
	ADC_TDIE,
	ADC_MAX_NUM,
}ADC_CH;

static struct sc8547 *__sc;

int voocphy_log_level = 2;

#define SC8547_DEVICE_ID		0x66

#define sc_alert(fmt, ...)	\
do {					\
	if (voocphy_log_level <= 4)	\
		printk(KERN_ERR "[sc8547:]:%s:" fmt, __func__, ##__VA_ARGS__);\
} while(0);

#define sc_err(fmt, ...)	\
do {					\
	if (voocphy_log_level <= 3)	\
		printk(KERN_ERR "[sc8547:]:%s:" fmt, __func__, ##__VA_ARGS__);\
} while(0);

#define sc_info(fmt, ...)	\
do {						\
	if (voocphy_log_level <= 2)	\
		printk(KERN_INFO "[sc8547:]:%s:" fmt, __func__, ##__VA_ARGS__);\
} while(0);

#define sc_dbg(fmt, ...)	\
do {					\
	if (voocphy_log_level <= 1)	\
		printk(KERN_DEBUG "[sc8547:]:%s:" fmt, __func__, ##__VA_ARGS__);\
} while(0);

extern int oplus_chg_get_ui_soc(void);
extern int oplus_chg_get_chg_temperature(void);
extern int oplus_chg_get_charger_voltage(void);
BattMngr_err_code_e vooc_ap_event_handle(unsigned long data);	//lkl need modify
BattMngr_err_code_e vooc_vol_event_handle(unsigned long data);
BattMngr_err_code_e vooc_safe_event_handle(unsigned long data);
BattMngr_err_code_e vooc_monitor_start(unsigned long data);
BattMngr_err_code_e vooc_monitor_stop(unsigned long data);

BattMngr_err_code_e vooc_temp_event_handle(unsigned long data);
BattMngr_err_code_e vooc_discon_event_handle(unsigned long data);
BattMngr_err_code_e vooc_btb_event_handle(unsigned long data);
void oplus_reset_fastchg_after_usbout(void);
BattMngr_err_code_e vooc_chg_out_check_event_handle(unsigned long data);
BattMngr_err_code_e vooc_curr_event_handle(unsigned long data);
BattMngr_err_code_e oplus_vooc_reset_voocphy(void);
void oplus_vooc_handle_voocphy_status(struct work_struct *work);
bool oplus_vooc_wake_notify_fastchg_work(void);
bool oplus_vooc_wake_voocphy_service_work(VOOCPHY_REQUEST_TYPE request);
extern bool oplus_vooc_wake_monitor_work(void);
enum hrtimer_restart vooc_monitor_base_timer(struct hrtimer* time);
void oplus_vooc_set_status_and_notify_ap(FASTCHG_STATUS fastchg_notify_status);
static void oplus_vooc_check_charger_out(void);
extern bool oplus_chg_stats(void);
BattMngr_err_code_e vooc_fastchg_check_process_handle(unsigned long enable);
BattMngr_err_code_e vooc_commu_process_handle(unsigned long enable);
extern void cpuboost_charge_event(int flag);
extern bool oplus_is_power_off_charging(struct oplus_vooc_chip *chip);
int oplus_voocphy_hw_setting(CHG_PUMP_SETTING_REASON reason);
static int oplus_voocphy_set_txbuff(u16 val);
static int oplus_voocphy_set_predata(u16 val);
int oplus_voocphy_get_adapter_request_info(void);
static int oplus_voocphy_chg_enable(void);
extern bool oplus_voocphy_is_fast_chging(void);
extern void oplus_vooc_switch_fast_chg(void);
BOOLEAN oplus_vooc_check_fastchg_real_allow(void);
extern int oplus_vooc_get_vooc_switch_val(void);
static int oplus_voocphy_direct_chg_enable(u8 *data);
static int oplus_voocphy_chg_enable(void);
static int oplus_voocphy_direct_chg_enable(u8 *data);
int oplus_voocphy_get_switch_gpio_val(void);
void oplus_voocphy_update_chg_data(void);
extern int ppm_sys_boost_min_cpu_freq_set(int freq_min, int freq_mid, int freq_max, unsigned int clear_time);
extern int ppm_sys_boost_min_cpu_freq_clear(void);
extern bool get_ppm_freq_info(void);
void oplus_voocphy_wake_modify_cpufeq_work(int flag);
extern bool oplus_chg_check_pd_svooc_adapater(void);

/*hongzhenglong@ODM.HQ.BSP.CHG 2020/04/19 add for bringing up 18W&33W*/
extern int is_spaceb_hc_project(void);
bool fast_chg_full = false;


struct sc8547_cfg {
	bool bat_ovp_disable;

	bool vdrop_ovp_disable;

	int bat_ovp_th;
	int bat_ocp_th;

	bool bus_ovp_disable;
	bool bus_ocp_disable;
	bool bus_ucp_disable;

	int bus_ovp_th;
	int bus_ocp_th;

	int ac_ovp_th;

	int sense_r_mohm;
};

struct sc8547 {
	struct device *dev;
	struct i2c_client *client;

	int irq_gpio;
	int irq;

	//add for uart and cp
	int switch1_gpio;		//gpio 47
	struct pinctrl *pinctrl;
	struct pinctrl_state *charging_inter_active;
	struct pinctrl_state *charging_inter_sleep;

	struct pinctrl_state *charger_gpio_sw_ctrl2_high;
	struct pinctrl_state *charger_gpio_sw_ctrl2_low;

	struct mutex data_lock;
	struct mutex i2c_rw_lock;
	struct mutex charging_disable_lock;
	struct mutex irq_complete;

	bool irq_waiting;
	bool irq_disabled;
	bool resume_completed;

	bool batt_present;
	bool vbus_present;

	bool usb_present;
	bool charge_enabled;	/* Register bit status */

	int  vbus_error;
	int charger_mode;
	int direct_charge;

	/* ADC reading */
	int vbat_volt;
	int vbus_volt;
	int vout_volt;
	int vac_volt;

	int ibat_curr;
	int ibus_curr;

	int die_temp;

	/* alarm/fault status */
	bool bat_ovp_fault;
	bool bat_ocp_fault;
	bool bus_ovp_fault;
	bool bus_ocp_fault;

	bool therm_shutdown_flag;
	bool therm_shutdown_stat;

	struct sc8547_cfg *cfg;

	int skip_writes;
	int skip_reads;

	struct sc8547_platform_data *platform_data;

	struct delayed_work irq_work;

	//struct dentry *debug_root;

	struct power_supply_desc psy_desc;
	struct power_supply_config psy_cfg;
	struct power_supply *fc2_psy;


	//adpter request info
	u8 vooc_rxdata;
	u8 vooc_flag;
	struct wakeup_source ws;
};

/*0~50*/
OPLUS_SVOOC_BATT_SYS_CURVES svooc_batt_curves_by_tmprange_soc0_2_50[BATT_SYS_MAX] = {
	[BATT_SYS_CURVE_TEMP0_2_TEMP45] = {
		.batt_sys_curve = {
			{VBATT_4180_MV, VBATT_1500_MA, false},
			{VBATT_4430_MV, VBATT_1000_MA, true},
		},
		.sys_curv_num = 2,
	},
	[BATT_SYS_CURVE_TEMP45_2_TEMP115] = {
		.batt_sys_curve = {
			{VBATT_4180_MV, VBATT_2000_MA, false,},
			{VBATT_4420_MV, VBATT_1500_MA, false,},
			{VBATT_4430_MV, VBATT_1000_MA, true,},
		},
		.sys_curv_num = 3,
	},
	[BATT_SYS_CURVE_TEMP115_2_TEMP160] = {
		.batt_sys_curve = {
			{VBATT_4450_MV, VBATT_2000_MA, false,},
			{VBATT_4470_MV, VBATT_1500_MA, false,},
			{VBATT_4480_MV, VBATT_1000_MA, true,},
		},
		.sys_curv_num = 3,
	},
	[BATT_SYS_CURVE_TEMP160_2_TEMP430] = {
		.batt_sys_curve = {
			{VBATT_4200_MV, VBATT_3000_MA, false,	(10 * 60 * 1000)/VOOC_VOL_EVENT_TIME},	//7min
			{VBATT_4450_MV, VBATT_2500_MA, false,	(10 * 60 * 1000)/VOOC_VOL_EVENT_TIME},	//13min
			{VBATT_4450_MV, VBATT_2250_MA, false,	},
			{VBATT_4450_MV, VBATT_2000_MA, false,	},
			{VBATT_4470_MV, VBATT_1500_MA, false,	},
			{VBATT_4480_MV, VBATT_1000_MA, true,	},
		},
		.sys_curv_num = 6,
	},
};

/*50~75*/
OPLUS_SVOOC_BATT_SYS_CURVES svooc_batt_curves_by_tmprange_soc50_2_75[BATT_SYS_MAX] = {
	[BATT_SYS_CURVE_TEMP0_2_TEMP45] = {
		.batt_sys_curve = {
			{VBATT_4180_MV, VBATT_1500_MA, false,},
			{VBATT_4430_MV, VBATT_1000_MA, true,},
		},
		.sys_curv_num = 2,
	},
	[BATT_SYS_CURVE_TEMP45_2_TEMP115] = {
		.batt_sys_curve = {
			{VBATT_4180_MV, VBATT_2000_MA, false,},
			{VBATT_4420_MV, VBATT_1500_MA, false,},
			{VBATT_4430_MV, VBATT_1000_MA, true,},
		},
		.sys_curv_num = 3,
	},
	[BATT_SYS_CURVE_TEMP115_2_TEMP160] = {
		.batt_sys_curve = {
			{VBATT_4450_MV, VBATT_2000_MA, false,},
			{VBATT_4470_MV, VBATT_1500_MA, false,},
			{VBATT_4480_MV, VBATT_1000_MA, true,},
		},
		.sys_curv_num = 3,
	},
	[BATT_SYS_CURVE_TEMP160_2_TEMP430] = {
		.batt_sys_curve = {
			{VBATT_4450_MV, VBATT_2500_MA, false,	(13 * 60 * 1000)/VOOC_VOL_EVENT_TIME},
			{VBATT_4450_MV, VBATT_2250_MA, false,},
			{VBATT_4450_MV, VBATT_2000_MA, false,},
			{VBATT_4470_MV, VBATT_1500_MA, false,},
			{VBATT_4480_MV, VBATT_1000_MA, true},
		},
		.sys_curv_num = 5,
	},
};


/*75~85*/
OPLUS_SVOOC_BATT_SYS_CURVES svooc_batt_curves_by_tmprange_soc75_2_85[BATT_SYS_MAX] = {
	[BATT_SYS_CURVE_TEMP0_2_TEMP45] = {
		.batt_sys_curve = {
			{VBATT_4430_MV, VBATT_1000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP45_2_TEMP115] = {
		.batt_sys_curve = {
			{VBATT_4430_MV, VBATT_1000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP115_2_TEMP160] = {
		.batt_sys_curve = {
			{VBATT_4480_MV, VBATT_1000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP160_2_TEMP430] = {
		.batt_sys_curve = {
			{VBATT_4470_MV, VBATT_1500_MA, false},
			{VBATT_4480_MV, VBATT_1000_MA, true},
		},
		.sys_curv_num = 2,
	},
};


/*85~90*/
OPLUS_SVOOC_BATT_SYS_CURVES svooc_batt_curves_by_tmprange_soc85_2_90[BATT_SYS_MAX] = {
	[BATT_SYS_CURVE_TEMP0_2_TEMP45] = {
		.batt_sys_curve = {
			{VBATT_4430_MV, VBATT_1000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP45_2_TEMP115] = {
		.batt_sys_curve = {
			{VBATT_4430_MV, VBATT_1000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP115_2_TEMP160] = {
		.batt_sys_curve = {
			{VBATT_4480_MV, VBATT_1000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP160_2_TEMP430] = {
		.batt_sys_curve = {
			{VBATT_4480_MV, VBATT_1000_MA, true},
		},
		.sys_curv_num = 1,
	},
};

/*vooc curv*/
/*0~50*/
OPLUS_SVOOC_BATT_SYS_CURVES vooc_batt_curves_by_tmprange_soc0_2_50[BATT_SYS_MAX] = {
	[BATT_SYS_CURVE_TEMP0_2_TEMP45] = {
		.batt_sys_curve = {
			{VBATT_4430_MV, VBATT_2000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP45_2_TEMP115] = {
		.batt_sys_curve = {
			{VBATT_4420_MV, VBATT_3000_MA, false,},
			{VBATT_4430_MV, VBATT_2000_MA, true,},
		},
		.sys_curv_num = 2,
	},
	[BATT_SYS_CURVE_TEMP115_2_TEMP160] = {
		.batt_sys_curve = {
			{VBATT_4475_MV, VBATT_3000_MA, false,},
			{VBATT_4480_MV, VBATT_2000_MA, false,},
		},
		.sys_curv_num = 2,
	},
	[BATT_SYS_CURVE_TEMP160_2_TEMP430] = {
		.batt_sys_curve = {
			{VBATT_4475_MV, VBATT_3000_MA, false,},	//7min
			{VBATT_4480_MV, VBATT_2000_MA, true,},	//13min
		},
		.sys_curv_num = 2,
	},
};

/*50~75*/
OPLUS_SVOOC_BATT_SYS_CURVES vooc_batt_curves_by_tmprange_soc50_2_75[BATT_SYS_MAX] = {
	[BATT_SYS_CURVE_TEMP0_2_TEMP45] = {
		.batt_sys_curve = {
			{VBATT_4430_MV, VBATT_2000_MA, true,},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP45_2_TEMP115] = {
		.batt_sys_curve = {
			{VBATT_4420_MV, VBATT_3000_MA, false,},
			{VBATT_4430_MV, VBATT_2000_MA, true,},
		},
		.sys_curv_num = 2,
	},
	[BATT_SYS_CURVE_TEMP115_2_TEMP160] = {
		.batt_sys_curve = {
			{VBATT_4475_MV, VBATT_3000_MA, false,},
			{VBATT_4480_MV, VBATT_2000_MA, true,},
		},
		.sys_curv_num = 2,
	},
	[BATT_SYS_CURVE_TEMP160_2_TEMP430] = {
		.batt_sys_curve = {
			{VBATT_4475_MV, VBATT_3000_MA, false,},
			{VBATT_4480_MV, VBATT_2000_MA, true,},	//4600mA
		},
		.sys_curv_num = 2,
	},
};

/*75~85*/
OPLUS_SVOOC_BATT_SYS_CURVES vooc_batt_curves_by_tmprange_soc75_2_85[BATT_SYS_MAX] = {
	[BATT_SYS_CURVE_TEMP0_2_TEMP45] = {
		.batt_sys_curve = {
			{VBATT_4430_MV, VBATT_2000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP45_2_TEMP115] = {
		.batt_sys_curve = {
			{VBATT_4430_MV, VBATT_2000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP115_2_TEMP160] = {
		.batt_sys_curve = {
			{VBATT_4480_MV, VBATT_2000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP160_2_TEMP430] = {
		.batt_sys_curve = {
			{VBATT_4475_MV, VBATT_3000_MA, false},
			{VBATT_4480_MV, VBATT_2000_MA, true},
		},
		.sys_curv_num = 2,
	},
};


/*85~90*/
OPLUS_SVOOC_BATT_SYS_CURVES vooc_batt_curves_by_tmprange_soc85_2_90[BATT_SYS_MAX] = {
	[BATT_SYS_CURVE_TEMP0_2_TEMP45] = {
		.batt_sys_curve = {
			{VBATT_4430_MV, VBATT_2000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP45_2_TEMP115] = {
		.batt_sys_curve = {
			{VBATT_4430_MV, VBATT_2000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP115_2_TEMP160] = {
		.batt_sys_curve = {
			{VBATT_4480_MV, VBATT_2000_MA, true},
		},
		.sys_curv_num = 1,
	},
	[BATT_SYS_CURVE_TEMP160_2_TEMP430] = {
		.batt_sys_curve = {
			{VBATT_4480_MV, VBATT_2000_MA, true},
		},
		.sys_curv_num = 1,
	},
};

UINT8 svooc_batt_sys_curve[BATT_SYS_ARRAY][7] = {
{0, 0, 1, 1, 1, 1, 0},// 3000mA   //!!! be sure to change BATT_PWD_CURR_THD1
{0, 0, 0, 0, 0, 0, 1},// 3414mV   //!!! be sure to change BATT_PWD_VOL_THD1
{0, 0, 1, 0, 1, 0, 0},// 2000mA
{1, 1, 1, 0, 0, 1, 0},// 4550mV
{0, 0, 0, 1, 0, 1, 0},// 1000mA
{1, 1, 1, 0, 0, 1, 0},// 4550mV------Remember to modify 'BATT_CMP_OVER_THD' together
};

UINT8 vooc_batt_sys_curve[BATT_SYS_ARRAY][7] = {
{0, 1, 0, 1, 1, 0, 1},// 4500mA   //!!! be sure to change BATT_PWD_CURR_THD1
{0, 0, 0, 0, 0, 0, 1},// 4604mV   //!!! be sure to change BATT_PWD_VOL_THD1
{0, 0, 1, 0, 1, 0, 0},// 2000mA
{1, 1, 1, 0, 0, 1, 0},// 4550mV
{0, 0, 0, 1, 0, 1, 0},// 1000mA
{1, 1, 1, 0, 0, 1, 0},// 4550mV------Remember to modify 'BATT_CMP_OVER_THD' together
};

vooc_manager * oplus_vooc_mg = NULL;
struct pm_qos_request pm_qos_req;

ktime_t calltime, rettime;

/************************************************************************/
static int sc8547_hw_setting(CHG_PUMP_SETTING_REASON reason);
static int sc8547_init_device(struct sc8547 *sc);
static int sc8547_svooc_hw_setting(struct sc8547 *sc);
int sc8547_init_vooc(void);
static s32 sc8547_get_adapter_request_info(void);
static void sc8547_update_chg_data(void);
static int sc8547_direct_chg_enable(u8 *data);
void sc8547_send_handshake_seq(void);

#define OPLUS_PDSVOOC_ADATER_ID	0x12
bool __attribute__((weak)) oplus_chg_check_pd_svooc_adapater(void)
{
	return false;
}

int oplus_get_fastchg_adapter_ask_cmd_copy(void)
{
	return oplus_vooc_mg->fastchg_adapter_ask_cmd_copy;
}

void oplus_reset_fastchg_adapter_ask_cmd_copy(void)
{
	oplus_vooc_mg->fastchg_adapter_ask_cmd_copy = 0;
}

void oplus_voocphy_pm_qos_update(int new_value)
{
	static int last_value = 0;

	if (!pm_qos_request_active(&pm_qos_req)) {
		sc_info("oplus_voocphy_pm_qos_update 1!");
		pm_qos_add_request(&pm_qos_req, PM_QOS_CPU_DMA_LATENCY, new_value);
	} else {
		sc_info("oplus_voocphy_pm_qos_update 2!");
		pm_qos_update_request(&pm_qos_req, new_value);
	}

	if (last_value != new_value) {
		last_value = new_value;

		if (new_value ==  PM_QOS_DEFAULT_VALUE) {
			sc_info("oplus_voocphy_pm_qos_update PM_QOS_DEFAULT_VALUE \n");
		} else {
			sc_info("oplus_voocphy_pm_qos_update value =%d \n", new_value);
		}
	}
}

void voocphy_cpufreq_init(void)
{
#ifdef CONFIG_OPLUS_CHARGER_MTK
	sc_info("%s \n", __func__);
	atomic_set(&oplus_vooc_mg->voocphy_freq_state, 1);
	ppm_sys_boost_min_cpu_freq_set(oplus_vooc_mg->voocphy_freq_mincore, oplus_vooc_mg->voocphy_freq_midcore, oplus_vooc_mg->voocphy_freq_maxcore, 15000);
#else
	cpuboost_charge_event(CPU_CHG_FREQ_STAT_UP);
#endif
}

void voocphy_cpufreq_release(void)
{
#ifdef CONFIG_OPLUS_CHARGER_MTK
	sc_info("%s RESET_DELAY_30S\n", __func__);
	atomic_set(&oplus_vooc_mg->voocphy_freq_state, 0);
	ppm_sys_boost_min_cpu_freq_clear();
#else
	cpuboost_charge_event(CPU_CHG_FREQ_STAT_AUTO);
#endif
}
EXPORT_SYMBOL(voocphy_cpufreq_release);

void  voocphy_cpufreq_update(int flag)
{
	if (flag == CPU_CHG_FREQ_STAT_UP) {
#ifdef CONFIG_OPLUS_CHARGER_MTK
		sc_info("%s CPU_CHG_FREQ_STAT_UP\n", __func__);
		atomic_set(&oplus_vooc_mg->voocphy_freq_state, 1);
		oplus_voocphy_wake_modify_cpufeq_work(CPU_CHG_FREQ_STAT_UP);
#else
		cpuboost_charge_event(CPU_CHG_FREQ_STAT_UP);
#endif
	} else {
#ifdef CONFIG_OPLUS_CHARGER_MTK
		sc_info("%s CPU_CHG_FREQ_STAT_AUTO\n", __func__);
		if (atomic_read(&oplus_vooc_mg->voocphy_freq_state) == 0)
			return;

		atomic_set(&oplus_vooc_mg->voocphy_freq_state, 0);
		oplus_voocphy_wake_modify_cpufeq_work(CPU_CHG_FREQ_STAT_AUTO);
#else
		cpuboost_charge_event(CPU_CHG_FREQ_STAT_AUTO);
#endif
	}
}

static int __sc8547_read_byte(struct sc8547 *sc, u8 reg, u8 *data)
{
	s32 ret;
	//sc_dbg(" rm mutex enter!\n");
	ret = i2c_smbus_read_byte_data(sc->client, reg);
	if (ret < 0) {
		if(oplus_vooc_mg) {
			oplus_vooc_mg->voocphy_iic_err = 1;
		}
		sc_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		return ret;
	}

	*data = (u8) ret;
	//sc_dbg(" exit!\n");
	return 0;
}

static int __sc8547_write_byte(struct sc8547 *sc, int reg, u8 val)
{
	s32 ret;

	//sc_dbg(" enter!\n");
	ret = i2c_smbus_write_byte_data(sc->client, reg, val);
	if (ret < 0) {
		if(oplus_vooc_mg) {
			oplus_vooc_mg->voocphy_iic_err = 1;
		}
		sc_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
		       val, reg, ret);
		return ret;
	}
	//sc_dbg(" exit!\n");
	return 0;
}

static int sc8547_read_byte(struct sc8547 *sc, u8 reg, u8 *data)
{
	int ret;

	if (sc->skip_reads) {
		*data = 0;
		return 0;
	}

	mutex_lock(&sc->i2c_rw_lock);
	ret = __sc8547_read_byte(sc, reg, data);
	mutex_unlock(&sc->i2c_rw_lock);

	return ret;
}

static int sc8547_write_byte(struct sc8547 *sc, u8 reg, u8 data)
{
	int ret;

	if (sc->skip_writes)
		return 0;

	mutex_lock(&sc->i2c_rw_lock);
	ret = __sc8547_write_byte(sc, reg, data);
	mutex_unlock(&sc->i2c_rw_lock);

	return ret;
}


static int sc8547_update_bits(struct sc8547*sc, u8 reg,
				    u8 mask, u8 data)
{
	int ret;
	u8 tmp;

	mutex_lock(&sc->i2c_rw_lock);
	ret = __sc8547_read_byte(sc, reg, &tmp);
	if (ret) {
		sc_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= data & mask;

	ret = __sc8547_write_byte(sc, reg, tmp);
	if (ret)
		sc_err("Failed: reg=%02X, ret=%d\n", reg, ret);
out:
	mutex_unlock(&sc->i2c_rw_lock);
	return ret;
}

static s32 sc8547_read_word(struct sc8547 *sc, u8 reg)
{
	s32 ret;

	mutex_lock(&sc->i2c_rw_lock);
	ret = i2c_smbus_read_word_data(sc->client, reg);
	if (ret < 0) {
		if(oplus_vooc_mg) {
			oplus_vooc_mg->voocphy_iic_err = 1;
		}
		sc_err("i2c read word fail: can't read reg:0x%02X \n", reg);
		mutex_unlock(&sc->i2c_rw_lock);
		return ret;
	}
	mutex_unlock(&sc->i2c_rw_lock);
	return ret;
}

static s32 sc8547_write_word(struct sc8547 *sc, u8 reg, u16 val)
{
	s32 ret;

	mutex_lock(&sc->i2c_rw_lock);
	ret = i2c_smbus_write_word_data(sc->client, reg, val);
	if (ret < 0) {
		if(oplus_vooc_mg) {
			oplus_vooc_mg->voocphy_iic_err = 1;
		}
		sc_err("i2c write word fail: can't write 0x%02X to reg:0x%02X \n", val, reg);
		mutex_unlock(&sc->i2c_rw_lock);
		return ret;
	}
	mutex_unlock(&sc->i2c_rw_lock);
	return 0;
}

static s32 sc8547_set_predata(u16 val)
{
	s32 ret;
	if (!__sc) {
		sc_err("failed: __sc is null\n");
		return -1;
	}

	//predata
	ret = sc8547_write_word(__sc, SC8547_REG_31, val);
	if (ret < 0) {
		sc_err("failed: write predata\n");
		return -1;
	}
	sc_info("write predata 0x%0x\n", val);
	return ret;
}

static s32 sc8547_set_txbuff(u16 val)
{
	s32 ret;
	if (!__sc) {
		sc_err("failed: __sc is null\n");
		return -1;
	}

	//txbuff
	ret = sc8547_write_word(__sc, SC8547_REG_2C, val);
	if (ret < 0) {
		sc_err("failed: write txbuff\n");
		return -1;
	}
	//sc_dbg("write txbuff 0x%0x\n", val);
	return ret;
}

#define GET_VBAT_PREDATA_DEFAULT (0x64 << SC8547_DATA_H_SHIFT) | 0x02;
#define EXEC_TIME_THR	1500
extern struct oplus_chg_chip* oplus_get_oplus_chip(void);
static int oplus_voocphy_send_mesg(void)
{
	u16 val, ret;
	ktime_t delta;
	unsigned long long duration;
	struct oplus_chg_chip *chip = oplus_get_oplus_chip();

	if (!oplus_vooc_mg || !chip) {
		sc_err("failed: oplus_vooc_mg is null\n");
		return -1;
	}

	//write txbuff //0x2c
	val = (oplus_vooc_mg->voocphy_tx_buff[0] << SC8547_DATA_H_SHIFT) | oplus_vooc_mg->voocphy_tx_buff[1];
	ret = oplus_voocphy_set_txbuff(val);
	if (ret < 0) {
		sc_err("failed: oplus_voocphy_send_mesg txbuff\n");
		return -1;
	}

	//end time
	rettime = ktime_get();
	delta = ktime_sub(rettime,calltime);
	duration = (unsigned long long) ktime_to_ns(delta) >> 10;
	if (duration >= EXEC_TIME_THR) {
		//note exception for systrace
		oplus_vooc_mg->irq_tx_timeout_num++;
		sc_err("dur time %lld usecs\n", duration);
	}

	sc_dbg("write txbuff 0x%0x usecs %lld\n", val, duration);
	//write predata
	if (oplus_vooc_mg->fastchg_adapter_ask_cmd == VOOC_CMD_GET_BATT_VOL) {
		//val = GET_VBAT_PREDATA_DEFAULT;
		ret = oplus_voocphy_set_predata(val);
		if (ret < 0) {
			sc_err("failed: oplus_voocphy_send_mesg predata\n");
			return -1;
		}
		sc_dbg("write predata 0x%0x\n", val);

		//stop fastchg check
		oplus_vooc_mg->fastchg_err_commu = false;
		oplus_vooc_mg->fastchg_to_warm = false;
		chip->voocphy.fastchg_to_warm = false;
	}

	return 0;
}

static s32 sc8547_get_adapter_request_info(void)
{
	s32 data;
	if (!__sc || !oplus_vooc_mg) {
		sc_err("__sc or oplus_vooc_mg is null\n");
		return -1;
	}

	data = sc8547_read_word(__sc, SC8547_REG_2E);

	if (data < 0) {
		sc_err("sc8547_read_word faile\n");
		return -1;
	}

	VOOCPHY_DATA16_SPLIT(data, __sc->vooc_rxdata , __sc->vooc_flag);
	oplus_vooc_mg->vooc_flag = __sc->vooc_flag;
	sc_info("data, vooc_flag, vooc_rxdata: 0x%0x 0x%0x 0x%0x\n", data, __sc->vooc_flag, __sc->vooc_rxdata);

	return 0;
}

int sc8547_get_cp_vbat(void)
{
	u8 data_block[2] = {0};
	int cp_vbat = 0;

	if (!__sc) {
		sc_err("Failed\n");
		return -1;
	}

	i2c_smbus_read_i2c_block_data(__sc->client, SC8547_REG_1B, 2, data_block);
	cp_vbat = ((data_block[0] << 8) | data_block[1])*125 / 100;
	return cp_vbat;
}

int sc8547_set_adc_enable(bool enable)
{
	if(!__sc) {
		sc_err("Failed\n");
		return -1;
	}

	if(enable)
		return sc8547_write_byte(__sc, SC8547_REG_11, 0x80);
	else
		return sc8547_write_byte(__sc, SC8547_REG_11, 0x00);
}

int sc8547_get_adc_enable(u8 *data)
{
	int ret = 0;

	if(!__sc) {
		sc_err("Failed\n");
		return -1;
	}

	ret = sc8547_read_byte(__sc, SC8547_REG_11, data);
	if(ret < 0) {
		sc_err("SC8547_REG_11\n");
		return -1;
	}

	*data = *data >> 7;

	return ret;
}

static void sc8547_update_chg_data(void)
{
	u8 data_block[4] = {0};
	int i = 0;
	u8 data = 0;
	s32 ret = 0;

	/*int_flag*/
	sc8547_read_byte(__sc, SC8547_REG_0F, &data);
	oplus_vooc_mg->int_flag = data;

	/*parse data_block for improving time of interrupt*/
	ret = i2c_smbus_read_i2c_block_data(__sc->client, SC8547_REG_19, 4, data_block);
	if (ret < 0) {
		if(oplus_vooc_mg) {
			oplus_vooc_mg->voocphy_iic_err = 1;
		}
		sc_err("sc8547_update_chg_data error \n");
	}
	for (i=0; i<4; i++) {
		pr_info("read vsys vbat data_block[%d] = %u\n", i, data_block[i]);
	}
	oplus_vooc_mg->cp_vsys = ((data_block[0] << 8) | data_block[1])*125 / 100;
	oplus_vooc_mg->bq27541_vbatt = ((data_block[2] << 8) | data_block[3])*125 / 100;

	memset(data_block, 0 , sizeof(u8) * 4);
	i2c_smbus_read_i2c_block_data(__sc->client, SC8547_REG_13, 4, data_block);
	/*parse data_block for improving time of interrupt*/
	for (i=0; i<4; i++) {
		sc_info("data_block[%d] = %u\n", i, data_block[i])
	}
	oplus_vooc_mg->cp_ichg = ((data_block[0] << 8) | data_block[1])*1875 / 1000;
	oplus_vooc_mg->cp_vbus = ((data_block[2] << 8) | data_block[3])*375 / 100;
	sc_info("cp_ichg = %d cp_vbus = %d, cp_vsys = %d int_flag = %d",
		oplus_vooc_mg->cp_ichg, oplus_vooc_mg->cp_vbus, oplus_vooc_mg->cp_vsys, oplus_vooc_mg->int_flag);
}

/*********************************************************************/
static int sc8547_reg_reset(struct sc8547 *sc, bool enable)
{
	int ret;
	u8 val;
	if (enable)
		val = SC8547_RESET_REG;
	else
		val = SC8547_NO_REG_RESET;

	val <<= SC8547_REG_RESET_SHIFT;

	ret = sc8547_update_bits(sc, SC8547_REG_07,
				SC8547_REG_RESET_MASK, val);

	return ret;
}


static int sc8547_direct_chg_enable(u8 *data)
{
	int ret = 0;
	if (!__sc) {
		sc_err("Failed\n");
		return -1;
	}
	ret = sc8547_read_byte(__sc, SC8547_REG_07, data);
	if (ret < 0) {
		sc_err("SC8547_REG_07\n");
		return -1;
	}
	*data = *data >> 7;

	return ret;
}

static int sc8547_write_txbuff_error(void)
{
	if (!oplus_vooc_mg) {
		sc_err("oplus_vooc_mg null\n");
		return 0;
	}

	return oplus_vooc_mg->vooc_flag & (1 << 3);
}

int oplus_write_txbuff_error(void)
{
	return sc8547_write_txbuff_error();
}

struct irqinfo {
	int mask;
	char except_info[30];
	int mark_except;
};

//int flag
#define VOUT_OVP_FLAG_MASK	BIT(7)
#define VBAT_OVP_FLAG_MASK	BIT(6)
#define IBAT_OCP_FLAG_MASK	BIT(5)
#define VBUS_OVP_FLAG_MASK	BIT(4)
#define IBUS_OCP_FLAG_MASK	BIT(3)
#define IBUS_UCP_FLAG_MASK	BIT(2)
#define ADAPTER_INSERT_FLAG_MASK	BIT(1)
#define VBAT_INSERT_FLAG_MASK	BIT(0)

#define IRQ_EVNET_NUM	8
struct irqinfo int_flag[IRQ_EVNET_NUM] = {
	{VOUT_OVP_FLAG_MASK, "VOUT_OVP", 0},
	{VBAT_OVP_FLAG_MASK, "VBAT_OVP", 1},
	{IBAT_OCP_FLAG_MASK, "IBAT_OCP", 1},
	{VBUS_OVP_FLAG_MASK, "VBUS_OVP", 1},
	{IBUS_OCP_FLAG_MASK, "IBUS_OCP", 1},
	{IBUS_UCP_FLAG_MASK, "IBUS_UCP", 1},
	{ADAPTER_INSERT_FLAG_MASK, "ADAPTER_INSERT", 1},
	{VBAT_INSERT_FLAG_MASK,   "VBAT_INSERT", 1},
};


struct irqinfo vooc_flag[IRQ_EVNET_NUM] = {
	{PULSE_FILTERED_STAT_MASK	, "PULSE_FILTERED_STAT", 1},
	{NINTH_CLK_ERR_FLAG_MASK	, "NINTH_CLK_ERR_FLAG", 1},
	{TXSEQ_DONE_FLAG_MASK		, "TXSEQ_DONE_FLAG", 0},
	{ERR_TRANS_DET_FLAG_MASK	, "ERR_TRANS_DET_FLAG", 1},
	{TXDATA_WR_FAIL_FLAG_MASK	, "TXDATA_WR_FAIL_FLAG", 1},
	{RX_START_FLAG_MASK			, "RX_START_FLAG", 0},
	{RXDATA_DONE_FLAG_MASK		, "RXDATA_DONE_FLAG", 0},
	{TXDATA_DONE_FALG_MASK		, "TXDATA_DONE_FALG", 0},
};

#define CHARGER_UP_CPU_FREQ	2000000	//1.8 x 1000 x 1000
int base_cpufreq_for_chg = CHARGER_UP_CPU_FREQ;
int cpu_freq_stratehy = CPU_CHG_FREQ_STAT_AUTO;
EXPORT_SYMBOL(base_cpufreq_for_chg);
EXPORT_SYMBOL(cpu_freq_stratehy);

void sc8547_dump_reg_in_err_issue(void)
{
	int i = 0, p = 0;
	//u8 value[DUMP_REG_CNT] = {0};
	struct oplus_chg_chip *chip = oplus_get_oplus_chip();

	if(!chip) {
		sc_err( "!!!!! oplus_chg_chip *chip NULL");
		return;
	}

	for( i = 0; i < 37; i++) {
		p = p + 1;
		sc8547_read_byte(__sc, i, &chip->voocphy.reg_dump[p]);
	}
	for( i = 0; i < 9; i++) {
		p = p + 1;
		sc8547_read_byte(__sc, 43 + i, &chip->voocphy.reg_dump[p]);
	}
	p = p + 1;
	sc8547_read_byte(__sc, SC8547_REG_36, &chip->voocphy.reg_dump[p]);
	p = p + 1;
	sc8547_read_byte(__sc, SC8547_REG_3A, &chip->voocphy.reg_dump[p]);
	sc_err( "p[%d], ", p);

	///memcpy(chip->voocphy.reg_dump, value, DUMP_REG_CNT);
	return;
}

void opchg_voocphy_dump_reg(void)
{
	sc8547_dump_reg_in_err_issue();
	return;
}

int oplus_vooc_print_dbg_info(void)
{
	int i = 0;
	u8 value = 0;
	struct oplus_chg_chip *chip = oplus_get_oplus_chip();

	if(!chip) {
		sc_err( "!!!!! oplus_chg_chip *chip NULL");
		return 0;
	}

	//print irq event
	for (i = 0; i < IRQ_EVNET_NUM; i++) {
		if ((vooc_flag[i].mask & oplus_vooc_mg->vooc_flag) && vooc_flag[i].mark_except) {
			printk("vooc happened %s\n", vooc_flag[i].except_info);
			goto chg_exception;
		}
	}

	if (oplus_vooc_mg) {
		if (oplus_vooc_mg->vops && oplus_vooc_mg->vops->print_cp_int_exception) {
			if (oplus_vooc_mg->vops->print_cp_int_exception(oplus_vooc_mg)) {
				goto chg_exception;
			}
		} else {
			for (i = 0; i < IRQ_EVNET_NUM; i++) {
				if ((int_flag[i].mask & oplus_vooc_mg->int_flag) && int_flag[i].mark_except) {
					printk("cp int happened %s\n", int_flag[i].except_info);
					if(int_flag[i].mask != VOUT_OVP_FLAG_MASK
						&& int_flag[i].mask != ADAPTER_INSERT_FLAG_MASK
						&& int_flag[i].mask != VBAT_INSERT_FLAG_MASK) {
						sc8547_dump_reg_in_err_issue();
					}
					goto chg_exception;
				}
			}
		}
	}

	if (oplus_vooc_mg->fastchg_adapter_ask_cmd != VOOC_CMD_GET_BATT_VOL) {
		goto chg_exception;
	} else {
		if ((oplus_vooc_mg->irq_total_num % 50) == 0) {		//about 2s
			goto chg_exception;
		}
	}
	return 0;

chg_exception:
	chip->voocphy.voocphy_cp_irq_flag = oplus_vooc_mg->int_flag;
	chip->voocphy.voocphy_vooc_irq_flag = oplus_vooc_mg->vooc_flag;
	chip->voocphy.disconn_pre_vbat_calc = oplus_vooc_mg->vbat_calc;
	chip->voocphy.voocphy_iic_err = oplus_vooc_mg->voocphy_iic_err;
	chip->voocphy.irq_rcvok_num += oplus_vooc_mg->irq_rcvok_num;
	chip->voocphy.ap_handle_timeout_num += oplus_vooc_mg->ap_handle_timeout_num;
	chip->voocphy.rcv_done_200ms_timeout_num += oplus_vooc_mg->rcv_done_200ms_timeout_num;
	if (oplus_vooc_mg->vops && oplus_vooc_mg->vops->get_voocphy_enable) {
		oplus_vooc_mg->vops->get_voocphy_enable(oplus_vooc_mg, &value);
	} else {
		sc8547_read_byte(__sc, SC8547_REG_2B, &value);
	}
	chip->voocphy.voocphy_enable = value;
	oplus_vooc_mg->voocphy_iic_err = 0;
	oplus_vooc_mg->vbat_calc = 0;
	oplus_vooc_mg->ap_handle_timeout_num = 0;
	oplus_vooc_mg->rcv_done_200ms_timeout_num = 0;
	oplus_vooc_mg->rcv_date_err_num = 0;


	printk(KERN_ERR "voocphydbg data[%d %d %d %d %d], status[%d, %d], init[%d, %d], set[%d, %d]"
		"comm[rcv:%d, 0x%0x, 0x%0x, %d, 0x%0x, 0x%0x reply:0x%0x 0x%0x], "
		"irqinfo[%d, %d, %d, %d, %d, %d, %d, %d] other[%d]\n",
		oplus_vooc_mg->cp_vsys, oplus_vooc_mg->cp_ichg,									//data
		oplus_vooc_mg->icharging, oplus_vooc_mg->cp_vbus,oplus_vooc_mg->bq27541_vbatt,
		oplus_vooc_mg->vooc_vbus_status, oplus_vooc_mg->vbus_vbatt,						//stutus
		oplus_vooc_mg->batt_soc_plugin, oplus_vooc_mg->batt_temp_plugin,					//init
		oplus_vooc_mg->current_expect, oplus_vooc_mg->current_max,						//setting
		oplus_vooc_mg->vooc_move_head, oplus_vooc_mg->voocphy_rx_buff,					//comm rcv
		oplus_vooc_mg->vooc_head, oplus_vooc_mg->adapter_type,
		oplus_vooc_mg->adapter_mesg, oplus_vooc_mg->fastchg_adapter_ask_cmd,
		oplus_vooc_mg->voocphy_tx_buff[0], oplus_vooc_mg->voocphy_tx_buff[1],				//comm reply
		oplus_vooc_mg->vooc_flag, oplus_vooc_mg->int_flag,oplus_vooc_mg->irq_total_num,	//irqinfo
		oplus_vooc_mg->irq_rcvok_num, oplus_vooc_mg->irq_rcverr_num, oplus_vooc_mg->irq_tx_timeout_num,
		oplus_vooc_mg->irq_tx_timeout, oplus_vooc_mg->irq_hw_timeout_num,
		base_cpufreq_for_chg);															//others

	return 0;
}

BattMngr_err_code_e  oplus_vooc_read_mesg_mask(UINT8 mask, UINT8 data, UINT8 *read_data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	*read_data = (data & mask) >> VOOC_SHIFT_FROM_MASK(mask);

	return status;
}

BattMngr_err_code_e  oplus_vooc_write_mesg_mask(UINT8 mask, UINT8 *pdata, UINT8 write_data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	write_data <<= VOOC_SHIFT_FROM_MASK(mask);
	*pdata = (*pdata & ~mask) | (write_data & mask);

	return status;
}


BattMngr_err_code_e oplus_vooc_get_mesg_from_adapter(UINT8 pmic_index)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	//int ret;
	UINT8 vooc_head = 0;
	UINT8 vooc_move_head = 0;

	//handle the frame head & adapter_data
	vooc_head = (oplus_vooc_mg->voocphy_rx_buff & VOOC_HEAD_MASK) >> VOOC_HEAD_SHIFT;

	//handle the shift frame head
	vooc_move_head = (oplus_vooc_mg->voocphy_rx_buff & VOOC_MOVE_HEAD_MASK)
					>> VOOC_MOVE_HEAD_SHIFT;

	if (vooc_head == SVOOC_HEAD || vooc_move_head == SVOOC_HEAD) {
		if (vooc_head == SVOOC_HEAD) {
			sc_dbg("SVOOC_HEAD");
		} else {
			sc_dbg("SVOOC_MOVE_HEAD");
			oplus_vooc_mg->vooc_move_head = true;
		}
		oplus_vooc_mg->vooc_head = SVOOC_INVERT_HEAD;
		oplus_vooc_mg->adapter_type = ADAPTER_SVOOC; //detect svooc adapter
	} else if (vooc_head == VOOC3_HEAD || vooc_move_head == VOOC3_HEAD) {
		if (vooc_head == VOOC3_HEAD) {
			sc_dbg("VOOC30_HEAD");
		} else {
			sc_dbg("VOOC30_MOVE_HEAD");
			oplus_vooc_mg->vooc_move_head = true;
		}
		oplus_vooc_mg->vooc_head = VOOC3_INVERT_HEAD;
		oplus_vooc_mg->adapter_type = ADAPTER_VOOC30; //detect vooc30 adapter
	} else if (vooc_head == VOOC2_HEAD ||vooc_move_head == VOOC2_HEAD) {
		if (vooc_move_head == VOOC2_HEAD) {
			sc_dbg("VOOC20_MOVE_HEAD");
			oplus_vooc_mg->vooc_move_head = true;
		}

		if (oplus_vooc_mg->adapter_model_detect_count) {
			oplus_vooc_mg->adapter_model_detect_count--; //detect three times
		} else {	//adapter_model_detect_count
			oplus_vooc_mg->adapter_type = ADAPTER_VOOC30; //detect svooc adapter
			#if	1	//def VOOC_ACCESS_CTRL_MOS
			//retry detect vooc30
			if (!oplus_vooc_mg->ask_vooc3_detect) {
				oplus_vooc_mg->ask_vooc3_detect = true;
				oplus_vooc_mg->adapter_model_detect_count =3;
				oplus_vooc_mg->fastchg_allow = false;
				oplus_vooc_mg->vooc_head = VOOC3_INVERT_HEAD; //set vooc3 head to detect
			} else {
				sc_dbg("VOOC20_HEAD");
				oplus_vooc_mg->adapter_type = ADAPTER_VOOC20; //old vooc2.0 adapter
				oplus_vooc_mg->vooc_head = VOOC2_INVERT_HEAD;
				oplus_vooc_mg->adapter_model_ver = POWER_SUPPLY_USB_SUBTYPE_VOOC;
				if (oplus_vooc_mg->fastchg_real_allow) {
					oplus_vooc_mg->fastchg_allow = true; //allow fastchg at old vooc2.0
					oplus_vooc_mg->fastchg_start = true; //fastchg_start true
					oplus_vooc_mg->fastchg_to_warm = false;
					oplus_vooc_mg->fastchg_dummy_start = false;
					oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_PRESENT);
					sc_dbg("fastchg_real_allow = true");
				} else {
					oplus_vooc_mg->fastchg_allow = false;
					oplus_vooc_mg->fastchg_start = false;
					oplus_vooc_mg->fastchg_to_warm = false;
					oplus_vooc_mg->fastchg_dummy_start = true;
					oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_DUMMY_START);
					sc_dbg("fastchg_dummy_start and reset voocphy");
				}
			}
			#else
			sc_dbg("POWER_BANK_MODE");
			oplus_vooc_mg->vooc_head = VOOC2_INVERT_HEAD;
			oplus_vooc_mg->adapter_type = ADAPTER_VOOC20; //power_bank mode
			oplus_vooc_mg->fastchg_allow = false; //power_bank mode need set fastchg_allow false
			#endif
		}
	} else {
		sc_err("unknow frame head, reset voocphy & fastchg rerun check");
		oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_ERR_COMMU);
		oplus_vooc_mg->rcv_date_err_num++;
		return BATTMNGR_EUNSUPPORTED;
	}

	if (!oplus_vooc_mg->vooc_move_head) {
		status = oplus_vooc_read_mesg_mask(VOOC_MESG_READ_MASK,
					oplus_vooc_mg->voocphy_rx_buff, &oplus_vooc_mg->adapter_mesg);
	} else {
		status = oplus_vooc_read_mesg_mask(VOOC_MESG_MOVE_READ_MASK,
					oplus_vooc_mg->voocphy_rx_buff, &oplus_vooc_mg->adapter_mesg);
	}

	if (oplus_vooc_mg->fastchg_reactive == true
		&& oplus_vooc_mg->adapter_mesg == VOOC_CMD_ASK_FASTCHG_ORNOT) {
		oplus_vooc_mg->fastchg_reactive = false;
	}

	if (oplus_vooc_mg->fastchg_reactive == true) {
		sc_err("(fastchg_dummy_start or err commu) && adapter_mesg != 0x04");
		status = BATTMNGR_EUNSUPPORTED;
	}

	sc_dbg("adapter_mesg=0x%0x", oplus_vooc_mg->adapter_mesg);
	return status;
}


BattMngr_err_code_e oplus_vooc_hardware_monitor_stop(void)
{
	//lkl need modify
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	/*disable batt adc*/

	/*disable temp adc*/

	/*disable dchg irq*/

	return status;
}


BattMngr_err_code_e oplus_vooc_reset_voocphy(void)
{

	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->reset_voocphy) {
		return oplus_vooc_mg->vops->reset_voocphy(oplus_vooc_mg);
	} else {
		//stop hardware monitor	//lkl need modify
		status = oplus_vooc_hardware_monitor_stop();
		if (status != BATTMNGR_SUCCESS)
			return BATTMNGR_EFAILED;

		//hwic config with plugout
		sc8547_write_byte(__sc, SC8547_REG_00, 0x2E);
		sc8547_write_byte(__sc, SC8547_REG_02, 0x07);
		sc8547_write_byte(__sc, SC8547_REG_04, 0x50);
		sc8547_write_byte(__sc, SC8547_REG_05, 0x28);
		sc8547_write_byte(__sc, SC8547_REG_11, 0x00);

		//turn off mos
		sc8547_write_byte(__sc, SC8547_REG_07, 0x04);

		//clear tx data
		sc8547_write_byte(__sc, SC8547_REG_2C, 0x00);
		sc8547_write_byte(__sc, SC8547_REG_2D, 0x00);

		//disable vooc phy irq
		sc8547_write_byte(__sc, SC8547_REG_30, 0x7f);

		//set D+ HiZ
		sc8547_write_byte(__sc, SC8547_REG_21, 0xc0);

		//select big bang mode

		//disable vooc
		sc8547_write_byte(__sc, SC8547_REG_2B, 0x00);
		oplus_voocphy_set_predata(0x0);
	}

	sc_info ("oplus_vooc_reset_voocphy done");

	return BATTMNGR_SUCCESS;
}


BattMngr_err_code_e oplus_vooc_reactive_voocphy(void)
{
	u8 value;
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->reactive_voocphy) {
		return oplus_vooc_mg->vops->reactive_voocphy(oplus_vooc_mg);
	} else {
		//to avoid cmd of adjust current(0x01)return error, add voocphy bit0 hold time to 800us
		oplus_voocphy_set_predata(0x0);
		sc8547_read_byte(__sc, SC8547_REG_3A, &value);
		value = value | (3 << 5);
		sc8547_write_byte(__sc, SC8547_REG_3A, value);

		//dpdm
		sc8547_write_byte(__sc, SC8547_REG_21, 0x21);
		sc8547_write_byte(__sc, SC8547_REG_22, 0x00);
		sc8547_write_byte(__sc, SC8547_REG_33, 0xD1);

		//clear tx data
		sc8547_write_byte(__sc, SC8547_REG_2C, 0x00);
		sc8547_write_byte(__sc, SC8547_REG_2D, 0x00);

		//vooc
		sc8547_write_byte(__sc, SC8547_REG_30, 0x05);
		//oplus_vooc_send_handshake_seq();
	}
	sc_info ("oplus_vooc_reactive_voocphy done");

	return BATTMNGR_SUCCESS;
}

int oplus_voocphy_clear_interrupt(void)
{
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->clear_intertupts) {
		return oplus_vooc_mg->vops->clear_intertupts(oplus_vooc_mg);
	} else {
		return 0;
	}
}

void oplus_get_soc_and_temp_with_enter_fastchg(void)
{
	int sys_curve_temp_idx = BATT_SYS_CURVE_TEMP160_2_TEMP430;
	INT32 batt_temp = oplus_vooc_mg->plug_in_batt_temp;
	oplus_vooc_mg->batt_sys_curv_found = false;

	//step1: find sys curv by soc
	if (oplus_vooc_mg->batt_soc <= 50) {
		oplus_vooc_mg->batt_soc_plugin = BATT_SOC_0_TO_50;
	} else if (oplus_vooc_mg->batt_soc <= 75) {
		oplus_vooc_mg->batt_soc_plugin = BATT_SOC_50_TO_75;
	} else if (oplus_vooc_mg->batt_soc <= 85) {
		oplus_vooc_mg->batt_soc_plugin = BATT_SOC_75_TO_85;
	} else if (oplus_vooc_mg->batt_soc <= 90) {
		oplus_vooc_mg->batt_soc_plugin = BATT_SOC_85_TO_90;
	} else {
		oplus_vooc_mg->batt_soc_plugin = BATT_SOC_90_TO_100;
	}

	//step2: find sys curv by temp range
	//update batt_temp_plugin status
	if (batt_temp < VOOC_LITTLE_COLD_TEMP) {
		sys_curve_temp_idx = BATT_SYS_CURVE_TEMP0_2_TEMP45;
		oplus_vooc_mg->batt_temp_plugin = BATT_TEMP_COOL;
		//if (oplus_vooc_mg->current_max > TEMP115_2_TEMP160_CURR) {					//need to modify
		//	oplus_vooc_mg->current_expect = TEMP115_2_TEMP160_CURR;
		//	oplus_vooc_mg->current_max = TEMP115_2_TEMP160_CURR;
		//}
		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COLD;			//need to modify
		oplus_vooc_mg->fastchg_batt_temp_status =  BATT_TEMP_LITTLE_COLD;
	} else if (batt_temp < VOOC_COOL_TEMP) {
		sys_curve_temp_idx = BATT_SYS_CURVE_TEMP45_2_TEMP115;
		oplus_vooc_mg->batt_temp_plugin = BATT_TEMP_COOL;
		//if (oplus_vooc_mg->current_max > TEMP45_2_TEMP115_CURR) {
		//	oplus_vooc_mg->current_expect = TEMP45_2_TEMP115_CURR;
		//	oplus_vooc_mg->current_max = TEMP45_2_TEMP115_CURR;
		//}
		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_COOL;
		oplus_vooc_mg->fastchg_batt_temp_status =  BATT_TEMP_COOL;
	} else if (batt_temp < VOOC_LITTLE_COOL_TEMP) {
		sys_curve_temp_idx = BATT_SYS_CURVE_TEMP115_2_TEMP160;
		oplus_vooc_mg->batt_temp_plugin = BATT_TEMP_LITTLE_COOL;
		//if (oplus_vooc_mg->current_max > TEMP115_2_TEMP160_CURR) {
		//	oplus_vooc_mg->current_expect = TEMP115_2_TEMP160_CURR;
		//	oplus_vooc_mg->current_max = TEMP115_2_TEMP160_CURR;
		//}
		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
		oplus_vooc_mg->fastchg_batt_temp_status =  BATT_TEMP_LITTLE_COOL;
	} else if (batt_temp < VOOC_NORMAL_TEMP) {
		sys_curve_temp_idx = BATT_SYS_CURVE_TEMP160_2_TEMP430;
		oplus_vooc_mg->batt_temp_plugin = BATT_TEMP_NORMAL;
		//if (oplus_vooc_mg->current_max > TEMP115_2_TEMP160_CURR) {
		//	oplus_vooc_mg->current_expect = TEMP115_2_TEMP160_CURR;
		//	oplus_vooc_mg->current_max = TEMP115_2_TEMP160_CURR;
		//}
		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_NORMAL;
		oplus_vooc_mg->fastchg_batt_temp_status =  BATT_TEMP_NORMAL;
	} else if (batt_temp < VOOC_WARM_TEMP) {
		oplus_vooc_mg->batt_temp_plugin = BATT_TEMP_WARM;
		sc_err("BATT_TEMP_WARM\n");
	} else {
		oplus_vooc_mg->batt_temp_plugin = BATT_TEMP_HIGH;
		sc_err("BATT_TEMP_HIGH\n");
	}
	oplus_vooc_mg->sys_curve_temp_idx = sys_curve_temp_idx;

	sc_err("current_max, current_expect:%d batt_soc_plugin:%d batt_temp_plugin:%d sys_curve_index:%d",
			oplus_vooc_mg->current_expect, oplus_vooc_mg->batt_soc_plugin,
			oplus_vooc_mg->batt_temp_plugin, sys_curve_temp_idx);
	return;
}

void oplus_choose_batt_sys_curve(void)
{
	int sys_curve_temp_idx = 0;
	int idx = 0;
	int convert_ibus = 0;
	OPLUS_SVOOC_BATT_SYS_CURVES *batt_sys_curv_by_tmprange = NULL;
	OPLUS_SVOOC_BATT_SYS_CURVE *batt_sys_curve = NULL;

	if (BATT_SOC_90_TO_100 != oplus_vooc_mg->batt_soc_plugin) {
		if (oplus_vooc_mg->batt_sys_curv_found == false) {
			//step1: find sys curv by soc
			if (oplus_vooc_mg->adapter_type == ADAPTER_SVOOC) {
				if (oplus_vooc_mg->batt_soc_plugin == BATT_SOC_0_TO_50) {
					oplus_vooc_mg->batt_sys_curv_by_soc = svooc_batt_curves_by_tmprange_soc0_2_50;
				} else if (oplus_vooc_mg->batt_soc_plugin == BATT_SOC_50_TO_75) {
					oplus_vooc_mg->batt_sys_curv_by_soc = svooc_batt_curves_by_tmprange_soc50_2_75;
				} else if (oplus_vooc_mg->batt_soc_plugin == BATT_SOC_75_TO_85) {
					oplus_vooc_mg->batt_sys_curv_by_soc = svooc_batt_curves_by_tmprange_soc75_2_85;
				} else if (oplus_vooc_mg->batt_soc_plugin == BATT_SOC_85_TO_90) {
					oplus_vooc_mg->batt_sys_curv_by_soc = svooc_batt_curves_by_tmprange_soc85_2_90;
				} else {
					//do nothing
				}
			} else {
				if (oplus_vooc_mg->batt_soc_plugin == BATT_SOC_0_TO_50) {
					oplus_vooc_mg->batt_sys_curv_by_soc = vooc_batt_curves_by_tmprange_soc0_2_50;
				} else if (oplus_vooc_mg->batt_soc_plugin == BATT_SOC_50_TO_75) {
					oplus_vooc_mg->batt_sys_curv_by_soc = vooc_batt_curves_by_tmprange_soc50_2_75;
				} else if (oplus_vooc_mg->batt_soc_plugin == BATT_SOC_75_TO_85) {
					oplus_vooc_mg->batt_sys_curv_by_soc = vooc_batt_curves_by_tmprange_soc75_2_85;
				} else if (oplus_vooc_mg->batt_soc_plugin == BATT_SOC_85_TO_90) {
					oplus_vooc_mg->batt_sys_curv_by_soc = vooc_batt_curves_by_tmprange_soc85_2_90;
				} else {
					//do nothing
				}
			}

			//step2: find sys curv by temp range
			sys_curve_temp_idx = oplus_vooc_mg->sys_curve_temp_idx;

			//step3: note sys curv location by temp range, for example BATT_SYS_CURVE_TEMP160_2_TEMP430
			oplus_vooc_mg->batt_sys_curv_by_tmprange = &(oplus_vooc_mg->batt_sys_curv_by_soc[sys_curve_temp_idx]);
			oplus_vooc_mg->cur_sys_curv_idx = 0;

			//step4: note init current_expect and current_max
			oplus_vooc_mg->bq27541_vbatt = sc8547_get_cp_vbat();
			batt_sys_curv_by_tmprange = oplus_vooc_mg->batt_sys_curv_by_tmprange;
			for (idx = 0; idx <= batt_sys_curv_by_tmprange->sys_curv_num - 1; idx++) {	//big -> small
				batt_sys_curve = &(batt_sys_curv_by_tmprange->batt_sys_curve[idx]);
				convert_ibus = batt_sys_curve->target_ibus * 100;
				sc_err("!batt_sys_curve[%d] bq27541_vbatt,exit,target_vbat,cp_ichg,target_ibus:[%d, %d, %d, %d, %d], \n", idx,
						oplus_vooc_mg->bq27541_vbatt, batt_sys_curve->exit, batt_sys_curve->target_vbat, oplus_vooc_mg->cp_ichg, convert_ibus);
				if (batt_sys_curve->exit == false) {
					if (oplus_vooc_mg->bq27541_vbatt <= batt_sys_curve->target_vbat) {
						oplus_vooc_mg->cur_sys_curv_idx = idx;
						oplus_vooc_mg->batt_sys_curv_found = true;
						sc_err("! found batt_sys_curve first idx[%d]\n", idx);
						break;
					}
				} else {
					//exit fastchg
					sc_err("! not found batt_sys_curve first\n");
				}
			}
			if (oplus_vooc_mg->batt_sys_curv_found) {
				sc_err("! found batt_sys_curve idx idx[%d]\n", idx);
				oplus_vooc_mg->current_expect = oplus_vooc_mg->batt_sys_curv_by_tmprange->batt_sys_curve[idx].target_ibus;
				oplus_vooc_mg->current_max = oplus_vooc_mg->current_expect;
				oplus_vooc_mg->batt_sys_curv_by_tmprange->batt_sys_curve[idx].chg_time = 0;
			} else {
				oplus_vooc_mg->current_expect = oplus_vooc_mg->batt_sys_curv_by_tmprange->batt_sys_curve[0].target_ibus;
				oplus_vooc_mg->current_max = oplus_vooc_mg->current_expect;
				oplus_vooc_mg->batt_sys_curv_by_tmprange->batt_sys_curve[0].chg_time = 0;
			}
		}
	} else {
		//do nothing
	}

	sc_err("current_max:%d current_expect:%d batt_soc_plugin:%d batt_temp_plugin:%d sys_curve_index:%d ibus:%d ",
			oplus_vooc_mg->current_max, oplus_vooc_mg->current_expect, oplus_vooc_mg->batt_soc_plugin,
			oplus_vooc_mg->batt_temp_plugin, sys_curve_temp_idx, oplus_vooc_mg->cp_ichg);
}


BattMngr_err_code_e oplus_vooc_variables_reset(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	INT32 batt_temp = 0;

	oplus_vooc_mg->voocphy_rx_buff = 0;
	oplus_vooc_mg->voocphy_tx_buff[0] = 0;
	oplus_vooc_mg->voocphy_tx_buff[1] = 0;
	if (oplus_vooc_mg->fastchg_reactive == false) {
		oplus_vooc_mg->adapter_check_ok = false;
		oplus_vooc_mg->adapter_model_detect_count = 3;
	}
	oplus_vooc_mg->adapter_ask_check_count = 3;
	oplus_vooc_mg->fastchg_adapter_ask_cmd = VOOC_CMD_UNKNOW;
	oplus_vooc_mg->vooc2_next_cmd = VOOC_CMD_ASK_FASTCHG_ORNOT;
	oplus_vooc_mg->adapter_check_cmmu_count = 0;
	oplus_vooc_mg->adapter_rand_h = 0;
	oplus_vooc_mg->adapter_rand_l = 0;
	oplus_vooc_mg->code_id_far = 0;
	oplus_vooc_mg->code_id_local = 0xFFFF;
	oplus_vooc_mg->code_id_temp_h = 0;
	oplus_vooc_mg->code_id_temp_l = 0;
	if (oplus_vooc_mg->fastchg_reactive == false) {
		oplus_vooc_mg->adapter_model_ver = 0;
	}
	oplus_vooc_mg->adapter_model_count = 0;
	oplus_vooc_mg->ask_batt_sys = 0;
	oplus_vooc_mg->current_expect = CC_VALUE_2_6C;	//lkl need modify
	oplus_vooc_mg->current_max = CC_VALUE_2_6C;
	oplus_vooc_mg->current_ap = CC_VALUE_2_6C;
	oplus_vooc_mg->current_batt_temp = CC_VALUE_2_6C;
	oplus_vooc_mg->batt_temp_plugin = BATT_TEMP_NORMAL;
	if (oplus_vooc_mg->fastchg_reactive == false) {
		oplus_vooc_mg->adapter_rand_start = false;
		oplus_vooc_mg->adapter_type = ADAPTER_SVOOC;
	}
	oplus_vooc_mg->adapter_mesg = 0;
	oplus_vooc_mg->fastchg_allow = false;
	oplus_vooc_mg->ask_vol_again = false;
	oplus_vooc_mg->ap_need_change_current = 0;
	oplus_vooc_mg->adjust_curr = ADJ_CUR_STEP_DEFAULT;
	oplus_vooc_mg->adjust_fail_cnt = 0;
	oplus_vooc_mg->pre_adapter_ask_cmd = VOOC_CMD_UNKNOW;
	oplus_vooc_mg->ignore_first_frame = true;
	oplus_vooc_mg->vooc_vbus_status = VOOC_VBUS_UNKNOW;
	if (oplus_vooc_mg->fastchg_reactive == false) {
		oplus_vooc_mg->vooc_head = VOOC_DEFAULT_HEAD;
	}
	oplus_vooc_mg->ask_vooc3_detect = false;
	oplus_vooc_mg->force_2a = false;
	oplus_vooc_mg->force_3a = false;
	oplus_vooc_mg->force_2a_flag = false;
	oplus_vooc_mg->force_3a_flag = false;
	oplus_vooc_mg->btb_temp_over = false;
	oplus_vooc_mg->btb_err_first = false;
	oplus_vooc_mg->fastchg_timeout_time = FASTCHG_TOTAL_TIME; //
	//oplus_vooc_mg->fastchg_3c_timeout_time = FASTCHG_3C_TIMEOUT;
	oplus_vooc_mg->fastchg_3c_timeout_time = FASTCHG_3000MA_TIMEOUT;
	oplus_vooc_mg->bq27541_vbatt = 0;
	oplus_vooc_mg->vbus = 0;
	oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
	oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_INIT;
	oplus_vooc_mg->fastchg_commu_stop = false;
	oplus_vooc_mg->fastchg_check_stop = false;
	oplus_vooc_mg->fastchg_monitor_stop = false;
	oplus_vooc_mg->current_pwd = oplus_vooc_mg->current_expect;
	oplus_vooc_mg->curr_pwd_count = 10;
	oplus_vooc_mg->usb_bad_connect = false;
	oplus_vooc_mg->vooc_move_head = false;
	//oplus_vooc_mg->fastchg_err_commu = false;
	oplus_vooc_mg->ask_current_first = true;
	oplus_vooc_mg->ask_bat_model_finished = false;
	oplus_vooc_mg->fastchg_need_reset = 0;
	oplus_voocphy_set_fastchg_state(OPLUS_FASTCHG_STAGE_1);
	oplus_vooc_mg->fastchg_recover_cnt = 0;
	oplus_vooc_mg->fastchg_recovering = false;

	//get initial soc
	oplus_vooc_mg->batt_soc = oplus_chg_get_ui_soc();

	if (oplus_vooc_mg->batt_fake_soc) {
		oplus_vooc_mg->batt_soc = oplus_vooc_mg->batt_fake_soc;
	}

	//get initial temp
	batt_temp = oplus_chg_get_chg_temperature();
	if (oplus_vooc_mg->batt_fake_temp) {
		batt_temp = oplus_vooc_mg->batt_fake_temp;
	}

	if (oplus_vooc_mg->batt_soc >= VOOC_LOW_SOC
			&& oplus_vooc_mg->batt_soc <= VOOC_HIGH_SOC
			&& batt_temp >= VOOC_LOW_TEMP
			&& batt_temp <= VOOC_HIGH_TEMP) {
		oplus_vooc_mg->fastchg_real_allow = true;
	} else {
		oplus_vooc_mg->fastchg_real_allow = false;
	}
	sc_dbg( "!soc:%d, temp:%d, real_allow:%d", oplus_vooc_mg->batt_soc,
		batt_temp, oplus_vooc_mg->fastchg_real_allow);

#if 0	//10v3a

#else
	if (oplus_vooc_mg->batt_soc > SOC_HIGH_LEVEL) {
		oplus_vooc_mg->current_expect = SOC75_2_SOC100_CURR;
		oplus_vooc_mg->current_max = SOC75_2_SOC100_CURR;
	} else if (oplus_vooc_mg->batt_soc > SOC_MID_LEVEL) {
		oplus_vooc_mg->current_expect = SOC50_2_SOC75_CURR;
		oplus_vooc_mg->current_max = SOC50_2_SOC75_CURR;
	}
#endif

	sc_dbg( "!batt_temp_plugin:%d, current_max:%d, current_expect:%d",
		oplus_vooc_mg->batt_temp_plugin, oplus_vooc_mg->current_max,
		oplus_vooc_mg->current_expect);

	//note exception info
	oplus_vooc_mg->vooc_flag = 0;
	oplus_vooc_mg->int_flag = 0;
	oplus_vooc_mg->irq_total_num = 0;
	oplus_vooc_mg->irq_rcverr_num = 0;
	oplus_vooc_mg->irq_tx_timeout_num = 0;
	oplus_vooc_mg->irq_tx_timeout = 0;
	oplus_vooc_mg->irq_hw_timeout_num = 0;
	//oplus_vooc_mg->irq_hw_timeout_num = 0;
	oplus_vooc_mg->ap_handle_timeout_num = 0;
	oplus_vooc_mg->rcv_done_200ms_timeout_num = 0;
	oplus_vooc_mg->rcv_date_err_num = 0;
	oplus_vooc_mg->irq_rcvok_num = 0;

	fast_chg_full = false;
	oplus_vooc_mg->user_set_exit_fastchg = false;
	//default vooc head as svooc
	status = oplus_vooc_write_mesg_mask(VOOC_INVERT_HEAD_MASK,
				&oplus_vooc_mg->voocphy_tx_buff[0], oplus_vooc_mg->vooc_head);

	//clear tx_reg
	//PmSchgVooc_SetVoocTxData(PMIC_INDEX_3, 0X00, 0x00);

	return status;
}


void oplus_vooc_reset_variables(void)
{
	if (oplus_vooc_mg)
		oplus_vooc_variables_reset();
	return;
}

BattMngr_err_code_e oplus_vooc_handle_ask_fastchg_ornot_cmd()
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	switch (oplus_vooc_mg->adapter_type) {
		case ADAPTER_VOOC20 :
			if (oplus_vooc_mg->vooc2_next_cmd != VOOC_CMD_ASK_FASTCHG_ORNOT) {
				sc_err("copycat adapter");
				//reset voocphy
				status = oplus_vooc_reset_voocphy();
			}

			//reset vooc2_next_cmd
			if (oplus_vooc_mg->fastchg_allow) {
				oplus_vooc_mg->vooc2_next_cmd = VOOC_CMD_IS_VUBS_OK;
			} else {
				oplus_vooc_mg->vooc2_next_cmd = VOOC_CMD_ASK_FASTCHG_ORNOT;
			}

			//set bit3 as fastchg_allow & set circuit R
			status = oplus_vooc_write_mesg_mask(TX0_DET_BIT3_TO_BIT7_MASK,
						&oplus_vooc_mg->voocphy_tx_buff[0], VOOC2_CIRCUIT_R_L_DEF
						| oplus_vooc_mg->fastchg_allow);
			status = oplus_vooc_write_mesg_mask(TX1_DET_BIT0_TO_BIT1_MASK,
						&oplus_vooc_mg->voocphy_tx_buff[1], VOOC2_CIRCUIT_R_H_DEF);
			sc_dbg("VOOC20 handle ask_fastchg_ornot");
			break;
		case ADAPTER_VOOC30 :	//lkl need modify
			//set bit3 as fastchg_allow & set circuit R
			status = oplus_vooc_write_mesg_mask(TX0_DET_BIT3_TO_BIT7_MASK,
						&oplus_vooc_mg->voocphy_tx_buff[0], NO_VOOC3_CIRCUIT_R_L_DEF
						| oplus_vooc_mg->fastchg_allow);
			status = oplus_vooc_write_mesg_mask(TX1_DET_BIT0_TO_BIT1_MASK,
						&oplus_vooc_mg->voocphy_tx_buff[1], NO_VOOC3_CIRCUIT_R_H_DEF);
			sc_dbg("VOOC30 handle ask_fastchg_ornot");
			break;
		case ADAPTER_SVOOC :
			//set bit3 as fastchg_allow & set circuit R
			status = oplus_vooc_write_mesg_mask(TX0_DET_BIT3_TO_BIT7_MASK,
						&oplus_vooc_mg->voocphy_tx_buff[0], NO_VOOC2_CIRCUIT_R_L_DEF
						| oplus_vooc_mg->fastchg_allow);
			status = oplus_vooc_write_mesg_mask(TX1_DET_BIT0_TO_BIT1_MASK,
						&oplus_vooc_mg->voocphy_tx_buff[1], NO_VOOC2_CIRCUIT_R_H_DEF);
			sc_dbg("SVOOC handle ask_fastchg_ornot");
			break;
		default:
			sc_err("should not go to here");
			break;
	}

	//for vooc3.0  and svooc adapter checksum
	oplus_vooc_mg->adapter_rand_start = true;
	oplus_vooc_mg->adapter_rand_h = 0x78; //temp set 0x78

	return status;
}



BattMngr_err_code_e oplus_vooc_calcucate_identification_code()
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	UINT8 code_id_temp_l = 0, code_id_temp_h = 0;

	oplus_vooc_mg->code_id_local = ((oplus_vooc_mg->adapter_rand_h << 9)
								|(oplus_vooc_mg->adapter_rand_l << 1));

	//set present need send mesg
	code_id_temp_l = ((((oplus_vooc_mg->adapter_rand_l >> 6) & 0x1) << TX0_DET_BIT0_SHIFT)
					| (((oplus_vooc_mg->adapter_rand_l >> 4) & 0x1) << TX0_DET_BIT1_SHIFT)
					| (((oplus_vooc_mg->adapter_rand_h >> 6) & 0x1) << TX0_DET_BIT2_SHIFT)
					| ((oplus_vooc_mg->adapter_rand_l & 0x1) << TX0_DET_BIT3_SHIFT)
					| ((oplus_vooc_mg->adapter_rand_h & 0x1) << TX0_DET_BIT4_SHIFT));
	code_id_temp_h = ((((oplus_vooc_mg->adapter_rand_h >> 4) & 0x1) << TX1_DET_BIT0_SHIFT)
					| (((oplus_vooc_mg->adapter_rand_l >> 2) & 0x1) << TX1_DET_BIT1_SHIFT));
	status = oplus_vooc_write_mesg_mask(TX0_DET_BIT3_TO_BIT7_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[0], code_id_temp_l);
	status |= oplus_vooc_write_mesg_mask(TX1_DET_BIT0_TO_BIT1_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[1], code_id_temp_h);

	//save next time need send mesg
	oplus_vooc_mg->code_id_temp_l = ((((oplus_vooc_mg->adapter_rand_l >> 3) & 0x1))
					| (((oplus_vooc_mg->adapter_rand_h >> 5) & 0x1) << TX0_DET_BIT1_SHIFT)
					| (((oplus_vooc_mg->adapter_rand_l >> 5) & 0x1) << TX0_DET_BIT2_SHIFT)
					| (((oplus_vooc_mg->adapter_rand_h >> 1) & 0x1) << TX0_DET_BIT3_SHIFT)
					| (((oplus_vooc_mg->adapter_rand_h >> 2) & 0x1) << TX0_DET_BIT4_SHIFT));

	oplus_vooc_mg->code_id_temp_h = ((((oplus_vooc_mg->adapter_rand_l >> 1) & 0x1))
					| (((oplus_vooc_mg->adapter_rand_h >> 3) & 0x1) << TX1_DET_BIT1_SHIFT));

	sc_dbg("present_code_id_temp_l:%0x, present_code_id_temp_h: %0x, \
					next_code_id_temp_l:%0x, next_code_id_temp_h:%0x", code_id_temp_l,
					code_id_temp_h, oplus_vooc_mg->code_id_temp_l, oplus_vooc_mg->code_id_temp_h);
	return status;
}


BattMngr_err_code_e oplus_vooc_handle_identification_cmd()
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	//choose response the identification cmd
	if (oplus_vooc_mg->adapter_ask_check_count == 0
		||oplus_vooc_mg->adapter_rand_start == false)
		return status;

	oplus_vooc_mg->adapter_ask_check_count--; //single vooc the idenficaition should not over 3 times
	oplus_vooc_mg->adapter_check_cmmu_count = ADAPTER_CHECK_COMMU_TIMES;
	oplus_vooc_mg->adapter_rand_l = 0x56;   //lizhijie add for tmep debug

	//next adapter ask cmd should be adapater_check_commu_process
	oplus_vooc_mg->fastchg_adapter_ask_cmd = VOOC_CMD_ADAPTER_CHECK_COMMU_PROCESS;

	//calculate the identification code that need send to adapter
	status = oplus_vooc_calcucate_identification_code();

	return status;
}



BattMngr_err_code_e oplus_vooc_handle_adapter_check_commu_process_cmd()
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	if (oplus_vooc_mg->adapter_check_cmmu_count == 5) { //send before save rand information
		status = oplus_vooc_write_mesg_mask(TX0_DET_BIT3_TO_BIT7_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[0], oplus_vooc_mg->code_id_temp_l);
		status = oplus_vooc_write_mesg_mask(TX1_DET_BIT0_TO_BIT1_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[1], oplus_vooc_mg->code_id_temp_h);
	} else if (oplus_vooc_mg->adapter_check_cmmu_count == 4) {
		oplus_vooc_mg->code_id_far = oplus_vooc_mg->adapter_mesg << 12;
		status = oplus_vooc_write_mesg_mask(TX0_DET_BIT5_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);

	} else if (oplus_vooc_mg->adapter_check_cmmu_count == 3) {
		oplus_vooc_mg->code_id_far = oplus_vooc_mg->code_id_far
					| (oplus_vooc_mg->adapter_mesg << 8);
		status = oplus_vooc_write_mesg_mask(TX0_DET_BIT5_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);

	} else if (oplus_vooc_mg->adapter_check_cmmu_count== 2) {
		oplus_vooc_mg->code_id_far = oplus_vooc_mg->code_id_far
					| (oplus_vooc_mg->adapter_mesg << 4);
		status = oplus_vooc_write_mesg_mask(TX0_DET_BIT5_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);

	} else if (oplus_vooc_mg->adapter_check_cmmu_count == 1) {
		oplus_vooc_mg->code_id_far = oplus_vooc_mg->code_id_far |oplus_vooc_mg->adapter_mesg;
		oplus_vooc_mg->code_id_far &= 0xfefe;
		sc_dbg( "!!!!code_id_far: 0x%0x, code_id_local",
					oplus_vooc_mg->code_id_far, oplus_vooc_mg->code_id_local);
		if (oplus_vooc_mg->code_id_far == oplus_vooc_mg->code_id_local) {
			oplus_vooc_mg->adapter_check_ok = true;
			sc_dbg( "adapter check ok");
		} else {

		}
		status = oplus_vooc_write_mesg_mask(TX0_DET_BIT5_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);
		oplus_vooc_mg->fastchg_adapter_ask_cmd = VOOC_CMD_UNKNOW; //reset cmd type
	} else {
		sc_err( "should not go here");
	}

	return status;
}


BattMngr_err_code_e oplus_vooc_handle_tell_model_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	oplus_vooc_mg->adapter_model_count = ADAPTER_MODEL_TIMES;

	//next adapter ask cmd should be adapater_tell_model_process
	oplus_vooc_mg->fastchg_adapter_ask_cmd = VOOC_CMD_TELL_MODEL_PROCESS;

	status = oplus_vooc_write_mesg_mask(TX0_DET_BIT4_MASK,
				&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);
	status = oplus_vooc_write_mesg_mask(TX0_DET_BIT6_MASK,
				&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);

	return status;
}

BattMngr_err_code_e oplus_vooc_handle_tell_model_process_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	if (oplus_vooc_mg->adapter_model_count == 2) {
		oplus_vooc_mg->adapter_model_ver = (oplus_vooc_mg->adapter_mesg << 4);
		status = oplus_vooc_write_mesg_mask(TX0_DET_BIT5_MASK,
				&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);
	} else if (oplus_vooc_mg->adapter_model_count == 1) {
		status = oplus_vooc_write_mesg_mask(TX0_DET_BIT5_MASK,
				&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);
		oplus_vooc_mg->adapter_model_ver = oplus_vooc_mg->adapter_model_ver
				| oplus_vooc_mg->adapter_mesg;
		//adapter id bit7 is need to clear
		oplus_vooc_mg->adapter_model_ver &= 0x7F;
		sc_dbg( "adapter_model_ver:0x%0x",
				oplus_vooc_mg->adapter_model_ver );
		oplus_vooc_mg->fastchg_adapter_ask_cmd = VOOC_CMD_UNKNOW; //reset cmd type
	} else {
		sc_err( "should not go here");
	}

	return status;
}


BattMngr_err_code_e oplus_vooc_handle_ask_bat_model_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	oplus_vooc_mg->ask_batt_sys = BATT_SYS_ARRAY + 1;

	//next adapter ask cmd should be ask_batt_sys_process
	oplus_vooc_mg->fastchg_adapter_ask_cmd = VOOC_CMD_ASK_BATT_SYS_PROCESS;

	status = oplus_vooc_write_mesg_mask(TX0_DET_BIT3_TO_BIT6_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], BATT_SYS_VOL_CUR_PAIR);

	return status;
}

BattMngr_err_code_e oplus_vooc_handle_ask_bat_model_process_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	UINT8 temp_data_l = 0;
	UINT8 temp_data_h = 0;

	if (oplus_vooc_mg->ask_batt_sys) {
		if (oplus_vooc_mg->adapter_type == ADAPTER_SVOOC) {  //svooc battery curve
			temp_data_l = (svooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][0] << 0)
							| (svooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][1] << 1)
							| (svooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][2] << 2)
							| (svooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][3] << 3)
							| (svooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][4] << 4);

			temp_data_h = (svooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][5] << 0)
							| (svooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][6] << 1);
		} else if (oplus_vooc_mg->adapter_type == ADAPTER_VOOC30) {	//for cp
			temp_data_l = (vooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][0] << 0)
							| (vooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][1] << 1)
							| (vooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][2] << 2)
							| (vooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][3] << 3)
							| (vooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][4] << 4);

			temp_data_h = (vooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][5] << 0)
							| (vooc_batt_sys_curve[oplus_vooc_mg->ask_batt_sys - 1][6] << 1);
		} else {
			sc_err( "error adapter type when write batt_sys_curve");
		}

		oplus_vooc_write_mesg_mask(TX0_DET_BIT3_TO_BIT7_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[0], temp_data_l);
		oplus_vooc_write_mesg_mask(TX1_DET_BIT0_TO_BIT1_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[1], temp_data_h);
		if (oplus_vooc_mg->ask_batt_sys == 1) {
			if (oplus_vooc_mg->fastchg_real_allow) {
				oplus_vooc_mg->fastchg_allow = true; //batt allow dchg
				oplus_vooc_mg->fastchg_start = true; //fastchg_start true
				oplus_vooc_mg->fastchg_to_warm = false;
				oplus_vooc_mg->fastchg_dummy_start = false;
				oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_PRESENT);
				sc_dbg( "fastchg_real_allow = true");
			} else {
				oplus_vooc_mg->fastchg_allow = false;
				oplus_vooc_mg->fastchg_start = false;
				oplus_vooc_mg->fastchg_to_warm = false;
				oplus_vooc_mg->fastchg_dummy_start = true;
				oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_DUMMY_START);
				sc_dbg( "fastchg_dummy_start and reset voocphy");
			}
			oplus_vooc_mg->ask_bat_model_finished = true;
			oplus_vooc_mg->fastchg_adapter_ask_cmd = VOOC_CMD_UNKNOW; //reset cmd
		}
	} else {
		sc_err( "should not go here");
	}

	return status;
}

BattMngr_err_code_e oplus_vooc_vbus_vbatt_detect(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	INT32 vbus_vbatt = 0;

	vbus_vbatt = oplus_vooc_mg->vbus = oplus_vooc_mg->cp_vbus;

	if (oplus_vooc_mg->adapter_type == ADAPTER_SVOOC)
		vbus_vbatt = oplus_vooc_mg->vbus - oplus_vooc_mg->bq27541_vbatt * 2; //doubule batt need
	else
		vbus_vbatt = oplus_vooc_mg->vbus - oplus_vooc_mg->bq27541_vbatt;     //single batt need

	if (vbus_vbatt  < 100) {
		oplus_vooc_mg->vooc_vbus_status = VOOC_VBUS_LOW;
	} else if (vbus_vbatt > 200) {
		oplus_vooc_mg->vooc_vbus_status = VOOC_VBUS_HIGH;
	} else {
		oplus_vooc_mg->vooc_vbus_status = VOOC_VBUS_NORMAL;
	}
	oplus_vooc_mg->vbus_vbatt = vbus_vbatt;

	sc_dbg( "!!vbus=%d, vbatt=%d, vbus_vbatt=%d, vooc_vbus_status=%d",
			oplus_vooc_mg->vbus, oplus_vooc_mg->bq27541_vbatt, vbus_vbatt, oplus_vooc_mg->vooc_vbus_status);
	return status;
}

BattMngr_err_code_e oplus_vooc_handle_is_vbus_ok_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	if (oplus_vooc_mg->adapter_type == ADAPTER_VOOC20) {
		if (oplus_vooc_mg->vooc2_next_cmd != VOOC_CMD_IS_VUBS_OK) {
			status = oplus_vooc_reset_voocphy();
		} else {
			oplus_vooc_write_mesg_mask(TX0_DET_BIT3_MASK,
				&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);
		}
	}

	//detect vbus status
	status = oplus_vooc_vbus_vbatt_detect();
	if (oplus_vooc_mg->vooc_vbus_status == VOOC_VBUS_HIGH) {  //vbus high
		oplus_vooc_write_mesg_mask(TX0_DET_BIT4_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);
	} else if (oplus_vooc_mg->vooc_vbus_status == VOOC_VBUS_NORMAL) {  //vbus ok
		oplus_vooc_write_mesg_mask(TX0_DET_BIT4_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);
		oplus_vooc_write_mesg_mask(TX0_DET_BIT5_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);
	} else {   //vbus low
		oplus_vooc_write_mesg_mask(TX0_DET_BIT5_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], BIT_ACTIVE);
	}

	if (oplus_vooc_mg->vooc_vbus_status == VOOC_VBUS_NORMAL) { //vbus-vbatt = ok
		oplus_voocphy_chg_enable();
	}

	if (oplus_vooc_mg->vooc_vbus_status == VOOC_VBUS_NORMAL) {
		switch (oplus_vooc_mg->adapter_type) {
			case ADAPTER_VOOC20 :
				sc_err("ADAPTER_VOOC20\n");
				oplus_vooc_mg->vooc2_next_cmd = VOOC_CMD_ASK_CURRENT_LEVEL;
				vooc_monitor_start(0);
				//oplus_vooc_hardware_monitor_start();
				break;
			case ADAPTER_VOOC30 :
			case ADAPTER_SVOOC :
				vooc_monitor_start(0);
				//oplus_vooc_hardware_monitor_start();
				break;
			default:
				sc_dbg( " should not go to here");
				break;
		}
	}

	return status;
}

extern int mt6370_suspend_charger(bool suspend);

void voocphy_request_fastchg_curve_init(void);

BattMngr_err_code_e oplus_vooc_handle_ask_current_level_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	//UINT8 temp_data_l = 0, temp_data_h = 0;
	if (oplus_vooc_mg->ask_current_first) {
		voocphy_request_fastchg_curve_init();
		oplus_vooc_mg->ask_current_first = false;
	}

	if (oplus_vooc_mg->adapter_type == ADAPTER_VOOC20) {
		if (oplus_vooc_mg->vooc2_next_cmd != VOOC_CMD_ASK_CURRENT_LEVEL) {
			status = oplus_vooc_reset_voocphy();
		} else {
			 //if (oplus_vooc_mg->batt_soc <= VOOC_SOC_HIGH) { // soc_high is not
			 if (1 < 0) {	//10v3a design is not need to go here
				if (oplus_vooc_mg->batt_temp_plugin == BATT_TEMP_NORMAL) { //16-44?
					//set current 3.75A
					oplus_vooc_write_mesg_mask(TX1_DET_BIT0_MASK,
						&oplus_vooc_mg->voocphy_tx_buff[1], BIT_ACTIVE); //bit 8
					oplus_vooc_write_mesg_mask(TX1_DET_BIT1_MASK,
						&oplus_vooc_mg->voocphy_tx_buff[1], BIT_ACTIVE); //bit 9
				} else if (oplus_vooc_mg->batt_temp_plugin == BATT_TEMP_LITTLE_COOL) { //12-16
					//set current 3.5A
					oplus_vooc_write_mesg_mask(TX1_DET_BIT0_MASK,
						&oplus_vooc_mg->voocphy_tx_buff[1], BIT_ACTIVE); //bit 8
				} else {
					//set current 3.0A do nothing, tx_buff keep default zero.
				}
			 } else {
				//do nothing, tx_buff keep default zero.
				//set current 3.0A
			}
			oplus_vooc_mg->vooc2_next_cmd = VOOC_CMD_GET_BATT_VOL; //vooc2_next_cmd;
		}
	}else { //svooc&vooc30
		oplus_vooc_write_mesg_mask(TX0_DET_BIT3_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], (oplus_vooc_mg->current_expect >> 6) & 0x1);
		oplus_vooc_write_mesg_mask(TX0_DET_BIT4_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], (oplus_vooc_mg->current_expect >> 5) & 0x1);
		oplus_vooc_write_mesg_mask(TX0_DET_BIT5_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], (oplus_vooc_mg->current_expect >> 4) & 0x1);
		oplus_vooc_write_mesg_mask(TX0_DET_BIT6_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], (oplus_vooc_mg->current_expect >> 3) & 0x1);
		oplus_vooc_write_mesg_mask(TX0_DET_BIT7_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], (oplus_vooc_mg->current_expect >> 2) & 0x1);
		oplus_vooc_write_mesg_mask(TX1_DET_BIT0_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[1], (oplus_vooc_mg->current_expect >> 1) & 0x1);
		oplus_vooc_write_mesg_mask(TX1_DET_BIT1_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[1], (oplus_vooc_mg->current_expect >> 0) & 0x1);

		oplus_vooc_mg->ap_need_change_current = 0; //clear need change flag
		oplus_vooc_mg->current_pwd = oplus_vooc_mg->current_expect * 100 + 300;
		oplus_vooc_mg->curr_pwd_count = 10;
		if (oplus_vooc_mg->adjust_curr == ADJ_CUR_STEP_REPLY_VBATT) {	//step1
			oplus_vooc_mg->adjust_curr = ADJ_CUR_STEP_SET_CURR_DONE;		//step2
		}
		sc_dbg( "!! set expect_current:%d", oplus_vooc_mg->current_expect);
	}
	return status;
}

#define trace_oplus_tp_sched_change_ux(x, y)
BattMngr_err_code_e oplus_non_vooc2_handle_get_batt_vol_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	UINT32 vbatt = 0;
	UINT8 data_temp_l = 0, data_temp_h = 0;
	UINT32 vbatt1;

	sc_dbg( "oplus_non_vooc2_handle_get_batt_vol_cmd start ");

	if (oplus_vooc_mg->btb_temp_over) {
		if (!oplus_vooc_mg->btb_err_first) { //1st->b'1100011:4394mv , 2nd->b'0001000:3484mv
			data_temp_l = 0x03; //bit 0 0011
			data_temp_h = 0x3; //bit 11
			oplus_vooc_mg->btb_err_first = true;
		} else {
			data_temp_l = 0x08; //bit 0 1000
			data_temp_h = 0x0; //bit 00
			oplus_vooc_mg->btb_err_first = false;
		}
	} else if (oplus_vooc_mg->ap_need_change_current && oplus_vooc_mg->adjust_fail_cnt <= 3) {
		data_temp_l = 0x1f; //bit 1 1111
		data_temp_h = 0x3; //bit 11
		oplus_vooc_mg->ap_need_change_current--;
		voocphy_cpufreq_update(CPU_CHG_FREQ_STAT_UP);
		oplus_voocphy_pm_qos_update(400);
		trace_oplus_tp_sched_change_ux(1, task_cpu(current));
		trace_oplus_tp_sched_change_ux(0, task_cpu(current));
		oplus_vooc_mg->adjust_curr = ADJ_CUR_STEP_REPLY_VBATT;
		sc_info( " ap_need_change_current adjust_fail_cnt[%d]\n", oplus_vooc_mg->adjust_fail_cnt);
	} else {
		/*calculate R*/
		if (oplus_vooc_mg->adapter_type == ADAPTER_SVOOC) {	//lkl need modify
			vbatt1 = (oplus_vooc_mg->cp_vbus/2) - (signed)(oplus_vooc_mg->cp_vsys - oplus_vooc_mg->bq27541_vbatt)/4 - (oplus_vooc_mg->cp_ichg * 75)/1000;
		} else {
			vbatt1 = oplus_vooc_mg->bq27541_vbatt;
		}

		if (vbatt1 < VBATT_BASE_FOR_ADAPTER) {
			vbatt = 0;
		} else {
			vbatt = (vbatt1 - VBATT_BASE_FOR_ADAPTER) / VBATT_DIV_FOR_ADAPTER;
		}

		data_temp_l =  ((vbatt >> 6) & 0x1) | (((vbatt >> 5) & 0x1) << 1)
						| (((vbatt >> 4) & 0x1) << 2) | (((vbatt >> 3) & 0x1) << 3)
						| (((vbatt >> 2) & 0x1) << 4);
		data_temp_h = ((vbatt >> 1) & 0x1) | (((vbatt >> 0) & 0x1) << 1);
	}
	oplus_vooc_mg->vbat_calc = vbatt;

	sc_info( "bq27541_vbatt=%d, vbatt=0x%0x, \
			data_temp_l=0x%0x, data_temp_h=0x%0x cp:%d %d %d %d",
			oplus_vooc_mg->bq27541_vbatt, vbatt,data_temp_l, data_temp_h, oplus_vooc_mg->cp_vsys,
			oplus_vooc_mg->cp_ichg,oplus_vooc_mg->cp_vbus, vbatt1);

	oplus_vooc_write_mesg_mask(TX0_DET_BIT3_TO_BIT7_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[0], data_temp_l);
	oplus_vooc_write_mesg_mask(TX1_DET_BIT0_TO_BIT1_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[1], data_temp_h);
	return status;
}



BattMngr_err_code_e oplus_vooc2_handle_get_batt_vol_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	UINT32 vbatt = 0;
	UINT8 data_temp_l = 0, data_temp_h = 0;

	if (oplus_vooc_mg->vooc2_next_cmd != VOOC_CMD_GET_BATT_VOL)
		status = oplus_vooc_reset_voocphy();

	oplus_vooc_write_mesg_mask(TX0_DET_BIT3_MASK,
			&oplus_vooc_mg->voocphy_tx_buff[0], oplus_vooc_mg->fastchg_allow); //need refresh bit3

	if (oplus_vooc_mg->batt_soc > VOOC_SOC_HIGH &&  !oplus_vooc_mg->ask_vol_again) {
		data_temp_l = oplus_vooc_mg->fastchg_allow | 0x0e;
		data_temp_h = 0x01;
		oplus_vooc_mg->ask_vol_again = true;
	} else if (oplus_vooc_mg->bq27541_vbatt > 3391) {

		 vbatt = (oplus_vooc_mg->bq27541_vbatt - 3392) / 16;		//+40mv
		 if(vbatt > 61)	 {	// b'111110,vbatt >= 4384mv
			vbatt = 61;
		 }
		 if (oplus_vooc_mg->force_2a && (!oplus_vooc_mg->force_2a_flag)
		 	&& (vbatt < FORCE2A_VBATT_REPORT)) {
			vbatt = FORCE2A_VBATT_REPORT;		//adapter act it as 4332mv
			oplus_vooc_mg->force_2a_flag = true;
		 } else if (oplus_vooc_mg->force_3a && (!oplus_vooc_mg->force_3a_flag)
		 	&& (vbatt < FORCE3A_VBATT_REPORT)) {
			vbatt = FORCE3A_VBATT_REPORT;		//adapter act it as 4316mv
			oplus_vooc_mg->force_3a_flag = true;
		 }
		 data_temp_l = (((vbatt >> 5) & 0x1) << 1) | (((vbatt >> 4) & 0x1) << 2)
		 				| (((vbatt >> 3) & 0x1) << 3) | (((vbatt >> 2) & 0x1) << 4)
		 				| oplus_vooc_mg->fastchg_allow;
		 data_temp_h = ((vbatt & 0x1) << 1) | ((vbatt >> 1) & 0x1);

		//reply adapter when btb_temp_over
		 if (oplus_vooc_mg->btb_temp_over) {
			if (!oplus_vooc_mg->btb_err_first) {
				data_temp_l = oplus_vooc_mg->fastchg_allow | 0x1e;
				data_temp_h = 0x01;
				oplus_vooc_mg->btb_err_first = true;
			} else {
				data_temp_l = oplus_vooc_mg->fastchg_allow | 0x10;
				data_temp_h = 0x02;
				oplus_vooc_mg->btb_err_first = false;
			}
		 }
	} else if (oplus_vooc_mg->bq27541_vbatt < 3000) {
		data_temp_l = 0;
		sc_dbg( "vbatt too low at vooc2 mode");
	} else {
		data_temp_l |= oplus_vooc_mg->fastchg_allow;
	}
	oplus_vooc_mg->vbat_calc = vbatt;

	sc_dbg( "data_temp_l=0x%0x, data_temp_h=0x%0x",
				data_temp_l, data_temp_h);
	oplus_vooc_write_mesg_mask(TX0_DET_BIT3_TO_BIT7_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[0], data_temp_l);
	oplus_vooc_write_mesg_mask(TX1_DET_BIT0_TO_BIT1_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[1], data_temp_h);

	oplus_vooc_mg->vooc2_next_cmd = VOOC_CMD_GET_BATT_VOL;

	return status;
}


BattMngr_err_code_e oplus_vooc_handle_get_batt_vol_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	switch (oplus_vooc_mg->adapter_type) {
		case ADAPTER_VOOC30 :
		case ADAPTER_SVOOC :
			status = oplus_non_vooc2_handle_get_batt_vol_cmd();
			break;
		case ADAPTER_VOOC20 :
			status = oplus_vooc2_handle_get_batt_vol_cmd();
			break;
		default :
			sc_err( " should not go to here");
			break;
	}

	return status;
}


BattMngr_err_code_e oplus_vooc_handle_null_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	//status = oplus_vooc_reset_voocphy();
	sc_err( " null cmd");

	return status;
}

BattMngr_err_code_e oplus_vooc_handle_tell_usb_bad_cmd(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	struct oplus_chg_chip *chip = oplus_get_oplus_chip();

	if(!chip) {
		sc_err( "!!!!! oplus_chg_chip *chip NULL");
		return status;
	}
	//status = oplus_vooc_reset_voocphy();
	oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_BAD_CONNECTED);
	oplus_vooc_mg->usb_bad_connect = true;
	oplus_vooc_mg->adapter_check_ok = 0;
	oplus_vooc_mg->adapter_model_ver = 0;
	oplus_vooc_mg->ask_batt_sys = 0;
	oplus_vooc_mg->adapter_check_cmmu_count = 0;
	chip->voocphy.r_state = 1;

	sc_err( " usb bad connect");
	return status;
}


#define USB_ICL_0_MA 0
BattMngr_err_code_e oplus_vooc_handle_ask_ap_status(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	//read vbus&vbatt need suspend charger

	return status;
}

BattMngr_err_code_e oplus_vooc_reply_adapter_mesg(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	//memset set tx_buffer head & tx_buffer data
	status = oplus_vooc_write_mesg_mask(TX0_DET_BIT3_TO_BIT7_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[0], 0);
	status |= oplus_vooc_write_mesg_mask(TX1_DET_BIT0_TO_BIT1_MASK,
					&oplus_vooc_mg->voocphy_tx_buff[1], 0);
	status = oplus_vooc_write_mesg_mask(VOOC_INVERT_HEAD_MASK,
				&oplus_vooc_mg->voocphy_tx_buff[0], oplus_vooc_mg->vooc_head);

	//handle special status
	if ((oplus_vooc_mg->fastchg_adapter_ask_cmd != VOOC_CMD_ADAPTER_CHECK_COMMU_PROCESS)
		&& (oplus_vooc_mg->fastchg_adapter_ask_cmd != VOOC_CMD_ASK_BATT_SYS_PROCESS)
		&& (oplus_vooc_mg->fastchg_adapter_ask_cmd != VOOC_CMD_TELL_MODEL_PROCESS)) {
		oplus_vooc_mg->fastchg_adapter_ask_cmd = oplus_vooc_mg->adapter_mesg;
	}

	sc_err("rx::0x%0x adapter_ask:0x%0x",oplus_vooc_mg->voocphy_rx_buff, oplus_vooc_mg->fastchg_adapter_ask_cmd);
	oplus_vooc_mg->fastchg_adapter_ask_cmd_copy = oplus_vooc_mg->fastchg_adapter_ask_cmd;

	//handle the adaper request mesg
	switch (oplus_vooc_mg->fastchg_adapter_ask_cmd) {
		case VOOC_CMD_ASK_FASTCHG_ORNOT:
			status = oplus_vooc_handle_ask_fastchg_ornot_cmd();
			break;
		case VOOC_CMD_IDENTIFICATION:
			status = oplus_vooc_handle_identification_cmd();
			break;
		case VOOC_CMD_ADAPTER_CHECK_COMMU_PROCESS:
			status = oplus_vooc_handle_adapter_check_commu_process_cmd();
			break;
		case VOOC_CMD_TELL_MODEL:
			status = oplus_vooc_handle_tell_model_cmd();
			break;
		case VOOC_CMD_TELL_MODEL_PROCESS:
			status = oplus_vooc_handle_tell_model_process_cmd();
			break;
		case VOOC_CMD_ASK_BAT_MODEL:
			status = oplus_vooc_handle_ask_bat_model_cmd();
			break;
		case VOOC_CMD_ASK_BATT_SYS_PROCESS:
			status = oplus_vooc_handle_ask_bat_model_process_cmd();
			break;
		case VOOC_CMD_IS_VUBS_OK:
			status = oplus_vooc_handle_is_vbus_ok_cmd();
			break;
		case VOOC_CMD_ASK_CURRENT_LEVEL:
			status = oplus_vooc_handle_ask_current_level_cmd();
			break;
		case VOOC_CMD_GET_BATT_VOL:
			status = oplus_vooc_handle_get_batt_vol_cmd();
			break;
		case VOOC_CMD_NULL:
			status = oplus_vooc_handle_null_cmd();
			break;
		case VOOC_CMD_TELL_USB_BAD:
			status = oplus_vooc_handle_tell_usb_bad_cmd();
			break;
		case VOOC_CMD_ASK_AP_STATUS :
			status = oplus_vooc_handle_ask_ap_status();
			break;
		default:
			oplus_vooc_mg->rcv_date_err_num++;
			sc_err("cmd not support, default handle");
			break;
	}

	return status;
}

void oplus_vooc_convert_tx_bit0_to_bit9(void)
{
	UINT8 i = 0;
	UINT16 src_data = 0, des_data = 0, temp_data = 0;

	//pDeviceContext->usb_power_supply_properties[USB_POWER_SUPPLY_TX_BUFF] = oplus_vooc_mg->voocphy_tx_buff[0];

	src_data = oplus_vooc_mg->voocphy_tx_buff[1] & TX1_DET_BIT0_TO_BIT1_MASK;
	src_data = (src_data << 8) | (oplus_vooc_mg->voocphy_tx_buff[0]);


	for (i=0; i<10; i++) {
		temp_data = (src_data >> i) & 0x1;
		des_data = des_data | (temp_data << (10 - i - 1));
	}

	oplus_vooc_mg->voocphy_tx_buff[1] = (des_data  >> 8) & TX1_DET_BIT0_TO_BIT1_MASK;
	oplus_vooc_mg->voocphy_tx_buff[0] = (des_data & 0xff);
};

void oplus_vooc_convert_tx_bit0_to_bit9_for_move_model(void)
{
	UINT8 i = 0;
	UINT16 src_data = 0, des_data = 0, temp_data = 0;

	src_data = oplus_vooc_mg->voocphy_tx_buff[1] & TX1_DET_BIT0_TO_BIT1_MASK;
	src_data = (src_data << 8) | (oplus_vooc_mg->voocphy_tx_buff[0]);


	for (i=0; i<10; i++) {
		temp_data = (src_data >> i) & 0x1;
		des_data = des_data | (temp_data << (10 - i - 1));
	}

	des_data = (des_data >> 1);
	oplus_vooc_mg->voocphy_tx_buff[1] = (des_data  >> 8) & TX1_DET_BIT0_TO_BIT1_MASK;
	oplus_vooc_mg->voocphy_tx_buff[0] = (des_data & 0xff);
};


BattMngr_err_code_e oplus_vooc_send_mesg_to_adapter(UINT8 pmic_index)
{
	int ret;
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	if (oplus_vooc_mg->adapter_check_cmmu_count) {
		oplus_vooc_mg->adapter_check_cmmu_count--;
	} else if (oplus_vooc_mg->adapter_model_count) {
		oplus_vooc_mg->adapter_model_count--;
	} else if (oplus_vooc_mg->ask_batt_sys) {
		oplus_vooc_mg->ask_batt_sys--;
		sc_dbg(" sc_dbg %d\n", oplus_vooc_mg->ask_batt_sys);
	}

	sc_dbg("before convert tx_buff[0]=0x%0x, tx_buff[1]=0x%0x",
				oplus_vooc_mg->voocphy_tx_buff[0], oplus_vooc_mg->voocphy_tx_buff[1]);

	//convert the tx_buff
	if (!oplus_vooc_mg->vooc_move_head) {
		oplus_vooc_convert_tx_bit0_to_bit9();
	} else {
		oplus_vooc_convert_tx_bit0_to_bit9_for_move_model();
	}
	oplus_vooc_mg->vooc_move_head = false;

	sc_err("!tx_buff[0]=0x%0x, tx_buff[1]=0x%0x 0x%0x %d",
				oplus_vooc_mg->voocphy_tx_buff[0], oplus_vooc_mg->voocphy_tx_buff[1], oplus_vooc_mg->fastchg_adapter_ask_cmd, oplus_vooc_mg->adjust_curr);

	//send tx_buff 10 bits to adapter
	ret = oplus_voocphy_send_mesg();
	//deal error when fastchg adjust current write txbuff
	if (VOOC_CMD_ASK_CURRENT_LEVEL == oplus_vooc_mg->fastchg_adapter_ask_cmd && ADJ_CUR_STEP_SET_CURR_DONE == oplus_vooc_mg->adjust_curr) {
		sc_err("adjust curr ADJ_CUR_STEP_SET_CURR_DONE\n");
	} else {
		if (VOOC_CMD_GET_BATT_VOL == oplus_vooc_mg->fastchg_adapter_ask_cmd && ADJ_CUR_STEP_DEFAULT == oplus_vooc_mg->adjust_curr) {
			voocphy_cpufreq_update(CPU_CHG_FREQ_STAT_AUTO);
			sc_dbg("voocphy_cpufreq_update ADJ_CUR_STEP_DEFAULT\n");
			//trace_oplus_tp_sched_change_ux(1, task_cpu(current));
			//trace_oplus_tp_sched_change_ux(0, task_cpu(current));
		}
	}

	if (ret < 0)
		status =  BATTMNGR_EFATAL;
	else
		status =  BATTMNGR_SUCCESS;

	return status;
}



BattMngr_err_code_e oplus_vooc_adapter_commu_with_voocphy(UINT8 pmic_index)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	//get data from adapter
	status = oplus_vooc_get_mesg_from_adapter(pmic_index);
	if (status != BATTMNGR_SUCCESS)
	{
		sc_err(" oplus_vooc_get_mesg_from_adapter fail");
		goto EndError;
	}

	//calculate mesg for voocphy need reply to adapter
	status = oplus_vooc_reply_adapter_mesg();
	if (status != BATTMNGR_SUCCESS)
	{
		sc_err(" oplus_vooc_process_mesg fail");
		goto EndError;
	}

	//send data to adapter
	status = oplus_vooc_send_mesg_to_adapter(pmic_index);
	if (status != BATTMNGR_SUCCESS)
	{
		sc_err(" oplus_vooc_send_mesg_to_adapter fail");
	}

EndError:
	return status;
}

BattMngr_err_code_e oplus_vooc_variables_init(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	oplus_vooc_mg->voocphy_rx_buff = 0;
	oplus_vooc_mg->voocphy_tx_buff[0] = 0;
	oplus_vooc_mg->voocphy_tx_buff[1] = 0;
	oplus_vooc_mg->adapter_check_ok = false;
	oplus_vooc_mg->adapter_ask_check_count = 3;
	oplus_vooc_mg->adapter_model_detect_count = 3;
	oplus_vooc_mg->fastchg_adapter_ask_cmd = VOOC_CMD_UNKNOW;
	oplus_vooc_mg->vooc2_next_cmd = VOOC_CMD_ASK_FASTCHG_ORNOT;
	oplus_vooc_mg->adapter_check_cmmu_count = 0;
	oplus_vooc_mg->adapter_rand_h = 0;
	oplus_vooc_mg->adapter_rand_l = 0;
	oplus_vooc_mg->code_id_far = 0;
	oplus_vooc_mg->code_id_local = 0xFFFF;
	oplus_vooc_mg->code_id_temp_h = 0;
	oplus_vooc_mg->code_id_temp_l = 0;
	oplus_vooc_mg->adapter_model_ver = 0;
	oplus_vooc_mg->adapter_model_count = 0;
	oplus_vooc_mg->ask_batt_sys = 0;
	oplus_vooc_mg->current_expect = CC_VALUE_2_6C;
	oplus_vooc_mg->current_max = CC_VALUE_2_6C;
	oplus_vooc_mg->current_ap = CC_VALUE_2_6C;
	oplus_vooc_mg->current_batt_temp = CC_VALUE_2_6C;
	oplus_vooc_mg->batt_temp_plugin = BATT_TEMP_NORMAL;
	oplus_vooc_mg->adapter_rand_start = false;
	oplus_vooc_mg->adapter_type = ADAPTER_SVOOC;
	oplus_vooc_mg->adapter_mesg = 0;
	oplus_vooc_mg->fastchg_allow = false;
	oplus_vooc_mg->ask_vol_again = false;
	oplus_vooc_mg->ap_need_change_current = 0;
	oplus_vooc_mg->adjust_curr = ADJ_CUR_STEP_DEFAULT;
	oplus_vooc_mg->adjust_fail_cnt = 0;
	oplus_vooc_mg->pre_adapter_ask_cmd = VOOC_CMD_UNKNOW;
	oplus_vooc_mg->ignore_first_frame = true;
	oplus_vooc_mg->vooc_vbus_status = VOOC_VBUS_UNKNOW;
	oplus_vooc_mg->vooc_head = VOOC_DEFAULT_HEAD;
	oplus_vooc_mg->ask_vooc3_detect = false;
	oplus_vooc_mg->force_2a = false;
	oplus_vooc_mg->force_3a = false;
	oplus_vooc_mg->force_2a_flag = false;
	oplus_vooc_mg->force_3a_flag = false;
	oplus_vooc_mg->btb_temp_over = false;
	oplus_vooc_mg->btb_err_first = false;
	oplus_vooc_mg->fastchg_notify_status = FAST_NOTIFY_UNKNOW;
	oplus_vooc_mg->ap_request_current = 30;
	oplus_vooc_mg->fastchg_timeout_time = FASTCHG_TOTAL_TIME; //
	//oplus_vooc_mg->fastchg_3c_timeout_time = FASTCHG_3C_TIMEOUT;
	oplus_vooc_mg->fastchg_3c_timeout_time = FASTCHG_3000MA_TIMEOUT;
	oplus_vooc_mg->bq27541_vbatt = 0;
	oplus_vooc_mg->vbus = 0;
	oplus_vooc_mg->batt_soc = 50;
	oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
	oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_INIT;
	oplus_vooc_mg->fastchg_start = false;
	oplus_vooc_mg->fastchg_to_normal = false;
	oplus_vooc_mg->fastchg_to_warm = false;
	oplus_vooc_mg->fastchg_dummy_start = false;
	oplus_vooc_mg->fastchg_real_allow = false;
	oplus_vooc_mg->fastchg_commu_stop = false;
	oplus_vooc_mg->fastchg_check_stop = false;
	oplus_vooc_mg->fastchg_monitor_stop = false;
	oplus_vooc_mg->current_pwd = oplus_vooc_mg->current_expect;
	oplus_vooc_mg->curr_pwd_count = 10;
	oplus_vooc_mg->usb_bad_connect = false;
	oplus_vooc_mg->vooc_move_head = false;
	oplus_vooc_mg->fastchg_err_commu = false;
	oplus_vooc_mg->fastchg_reactive = false;
	oplus_vooc_mg->ask_bat_model_finished = false;
	oplus_voocphy_set_user_exit_fastchg(false);
	oplus_vooc_mg->fastchg_need_reset = 0;
	oplus_vooc_mg->fastchg_recover_cnt = 0;
	oplus_vooc_mg->fastchg_recovering = false;
	oplus_vooc_mg->chrpmp_id = CHGPMP_ID_INVAL;

	//note exception info
	oplus_vooc_mg->vooc_flag = 0;
	oplus_vooc_mg->int_flag = 0;
	oplus_vooc_mg->irq_total_num = 0;
	oplus_vooc_mg->irq_rcvok_num = 0;
	oplus_vooc_mg->irq_rcverr_num = 0;
	oplus_vooc_mg->irq_tx_timeout_num = 0;
	oplus_vooc_mg->irq_tx_timeout = 0;
	oplus_vooc_mg->irq_hw_timeout_num = 0;
	oplus_vooc_mg->ap_handle_timeout_num = 0;
	oplus_vooc_mg->rcv_done_200ms_timeout_num = 0;
	oplus_vooc_mg->rcv_date_err_num = 0;

	//default vooc head as svooc
	status = oplus_vooc_write_mesg_mask(VOOC_INVERT_HEAD_MASK,
				&oplus_vooc_mg->voocphy_tx_buff[0], oplus_vooc_mg->vooc_head);
	//clear tx_reg		//lkl need modify
	//PmSchgVooc_SetVoocTxData(PMIC_INDEX_3, 0X00, 0x00);

	return status;
}

static void vooc_monitor_timer_init (void)
{
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_AP].expires = VOOC_AP_EVENT_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_AP].data = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_AP].mornitor_hdler = vooc_ap_event_handle;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_AP].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_AP].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_AP].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CURR].expires = VOOC_CURR_EVENT_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CURR].data = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CURR].mornitor_hdler = vooc_curr_event_handle;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CURR].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CURR].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CURR].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SOC].expires = VOOC_SOC_EVENT_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SOC].data = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SOC].mornitor_hdler = NULL;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SOC].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SOC].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SOC].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_VOL].expires = VOOC_VOL_EVENT_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_VOL].data = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_VOL].mornitor_hdler = vooc_vol_event_handle;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_VOL].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_VOL].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_VOL].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEMP].expires = VOOC_TEMP_EVENT_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEMP].data = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEMP].mornitor_hdler = vooc_temp_event_handle;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEMP].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEMP].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEMP].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_BTB].expires = VOOC_BTB_EVENT_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_BTB].data = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_BTB].mornitor_hdler = vooc_btb_event_handle;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_BTB].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_BTB].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_BTB].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SAFE].expires = VOOC_SAFE_EVENT_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SAFE].data = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SAFE].mornitor_hdler = vooc_safe_event_handle;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SAFE].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SAFE].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_SAFE].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_COMMU].expires = VOOC_COMMU_EVENT_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_COMMU].data = true;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_COMMU].mornitor_hdler = vooc_commu_process_handle;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_COMMU].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_COMMU].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_COMMU].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_DISCON].expires = VOOC_DISCON_EVENT_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_DISCON].data = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_DISCON].mornitor_hdler = NULL;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_DISCON].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_DISCON].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_DISCON].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_FASTCHG_CHECK].expires = VOOC_FASTCHG_CHECK_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_FASTCHG_CHECK].data = true;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_FASTCHG_CHECK].mornitor_hdler = vooc_fastchg_check_process_handle;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_FASTCHG_CHECK].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_FASTCHG_CHECK].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_FASTCHG_CHECK].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CHG_OUT_CHECK].expires = VOOC_CHG_OUT_CHECK_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CHG_OUT_CHECK].data = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CHG_OUT_CHECK].mornitor_hdler = vooc_chg_out_check_event_handle;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CHG_OUT_CHECK].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CHG_OUT_CHECK].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_CHG_OUT_CHECK].timeout = false;

	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEST_CHECK].expires = VOOC_TEST_CHECK_TIME / VOOC_BASE_TIME_STEP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEST_CHECK].data = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEST_CHECK].mornitor_hdler = NULL;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEST_CHECK].status = VOOC_MONITOR_STOP;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEST_CHECK].cnt = 0;
	oplus_vooc_mg->mornitor_evt[VOOC_THREAD_TIMER_TEST_CHECK].timeout = false;

	//init hrtimer
	hrtimer_init(&oplus_vooc_mg->monitor_btimer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
    oplus_vooc_mg->monitor_btimer.function = vooc_monitor_base_timer;

	sc_info( "vooc: create timers successfully");
}

BattMngr_err_code_e vooc_monitor_timer_start(VOOC_THREAD_TIMER_TYPE timer_id, LONGLONG duration_in_ms)
{
    if( (timer_id >= VOOC_THREAD_TIMER_INVALID) || (duration_in_ms < 0))
		return BATTMNGR_EFATAL;

	if (!oplus_vooc_mg) {
		sc_err( "oplus_vooc_mg is null");
		return BATTMNGR_EFATAL;
	}

	oplus_vooc_mg->mornitor_evt[timer_id].cnt = 0;
	oplus_vooc_mg->mornitor_evt[timer_id].timeout = false;
	oplus_vooc_mg->mornitor_evt[timer_id].status = VOOC_MONITOR_START;

	sc_dbg("timerid %ul\n", timer_id);

    return BATTMNGR_SUCCESS;
}

BattMngr_err_code_e vooc_monitor_timer_stop(VOOC_THREAD_TIMER_TYPE timer_id, LONGLONG duration_in_ms)
{
    if( (timer_id >= VOOC_THREAD_TIMER_INVALID) || (duration_in_ms < 0))
		return BATTMNGR_EFATAL;

	if (!oplus_vooc_mg) {
		sc_dbg( "oplus_vooc_mg is null");
		return BATTMNGR_EFATAL;
	}

	oplus_vooc_mg->mornitor_evt[timer_id].cnt = 0;
	oplus_vooc_mg->mornitor_evt[timer_id].timeout = false;
	oplus_vooc_mg->mornitor_evt[timer_id].status = VOOC_MONITOR_STOP;
	sc_err("timerid %d\n", timer_id);

    return BATTMNGR_SUCCESS;
}


void oplus_basetimer_monitor_start(void)
{
	if (oplus_vooc_mg && !hrtimer_active(&oplus_vooc_mg->monitor_btimer)) {
		oplus_vooc_mg->moniotr_kt = ktime_set(0,5000000);// 0s	5000000ns	= 5ms
		hrtimer_start(&oplus_vooc_mg->monitor_btimer,oplus_vooc_mg->moniotr_kt, HRTIMER_MODE_REL);
		sc_info("finihed\n");
		//dump_stack();
	}
}

void oplus_basetimer_monitor_stop(void)
{
	if (oplus_vooc_mg && hrtimer_active(&oplus_vooc_mg->monitor_btimer)) {
		hrtimer_cancel(&oplus_vooc_mg->monitor_btimer);
		sc_err("finihed");
		//dump_stack();
	}
	vooc_monitor_timer_stop(VOOC_THREAD_TIMER_COMMU, VOOC_COMMU_EVENT_TIME);
}


void oplus_vooc_switch_mode(int mode);
extern void oplus_chg_set_charger_type_unknown(void);
extern void oplus_chg_suspend_charger(void);
void oplus_voocphy_turn_off_fastchg(void)
{
	//struct oplus_vooc_chip *chip = g_vooc_chip;
	struct oplus_chg_chip *chg = oplus_get_oplus_chip();
	if (!chg) {
		chg_err("chg is null\n");
		return;
	}
	if (oplus_chg_get_voocphy_support()) {
		oplus_chg_set_chargerid_switch_val(0);
		oplus_vooc_switch_mode(NORMAL_CHARGER_MODE);

		//chip->allow_reading = true;
		chg->voocphy.fastchg_start= false;
		chg->voocphy.fastchg_to_normal = false;
		chg->voocphy.fastchg_to_warm = false;
		chg->voocphy.fastchg_ing = false;
		//chg->voocphy.btb_temp_over = false;
		chg->voocphy.fastchg_dummy_start = false;
		chg->voocphy.fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;

		oplus_chg_clear_chargerid_info();
		//oplus_vooc_del_watchdog_timer(chip);
		oplus_chg_set_charger_type_unknown();
		oplus_chg_wake_update_work();
		//oplus_vooc_set_awake(chip, false);
		oplus_chg_suspend_charger();
		chg_err("oplus_voocphy_turn_off_fastchg\n");
	}
	return;
}

void oplus_vooc_handle_voocphy_status(struct work_struct *work)
{
	struct oplus_chg_chip *chip = oplus_get_oplus_chip();
	int intval = 0;

	if (!chip) {
		printk(KERN_ERR "!!!chip null, voocphy non handle status: [%d]\n", intval);
		return;
	}

	intval = oplus_vooc_mg->fastchg_notify_status;

	//if (vooc_status ^ intval) {
	if (true) {
		printk(KERN_ERR "!!![voocphy] vooc status: [%d]\n", intval);
		if (intval == FAST_NOTIFY_PRESENT) {
			chip->voocphy.fastchg_start = true;		//lkl need to avoid guage data
			chip->voocphy.disconn_pre_charger_type = CHARGER_SUBTYPE_FASTCHG_SVOOC;
			chip->voocphy.fastchg_to_warm = false;
			chip->voocphy.fastchg_dummy_start = false;
			chip->voocphy.fastchg_to_normal = false;
			chip->voocphy.fastchg_ing = false;
			//chip->voocphy.fast_chg_type = ((intval >> 8) & 0xFF);
			chip->voocphy.fast_chg_type = oplus_vooc_mg->adapter_model_ver;
			printk(KERN_ERR "!!![voocphy] fastchg start: [%d], adapter version: [0x%0x]\n",
				chip->voocphy.fastchg_start, chip->voocphy.fast_chg_type);
			oplus_chg_wake_update_work();
		} else if (intval == FAST_NOTIFY_DUMMY_START) {
			chip->voocphy.fastchg_start = false;
			chip->voocphy.fastchg_to_warm = false;
			chip->voocphy.fastchg_dummy_start = true;
			chip->voocphy.fastchg_to_normal = false;
			chip->voocphy.fastchg_ing = false;
			//chip->voocphy.fast_chg_type = ((intval >> 8) & 0xFF);
			chip->voocphy.fast_chg_type = oplus_vooc_mg->adapter_model_ver;
			printk(KERN_ERR "!!![voocphy] fastchg dummy start: [%d], adapter version: [0x%0x]\n",
					chip->voocphy.fastchg_dummy_start, chip->voocphy.fast_chg_type);
			oplus_chg_wake_update_work();
		} else if ((intval & 0xFF) == FAST_NOTIFY_ONGOING) {
			chip->voocphy.fastchg_ing = true;
			printk(KERN_ERR "!!!! voocphy fastchg ongoing\n");
		} else if (intval == FAST_NOTIFY_FULL || intval == FAST_NOTIFY_BAD_CONNECTED) {
			//switch usbswitch and ctrl2
			//dump_stack();
			printk(KERN_ERR "%s\r\n", intval == FAST_NOTIFY_FULL ? "FAST_NOTIFY_FULL" : "FAST_NOTIFY_BAD_CONNECTED");
			oplus_chg_set_chargerid_switch_val(0);
			oplus_vooc_switch_mode(NORMAL_CHARGER_MODE);

			//note flags
			fast_chg_full = true;
			chip->voocphy.fastchg_to_normal = true;
			usleep_range(350000, 350000);
			chip->voocphy.fastchg_start = false;
			chip->voocphy.fastchg_to_warm = false;
			chip->voocphy.fastchg_dummy_start = false;
			chip->voocphy.fastchg_ing = false;
			//allow reading = ture
			printk(KERN_ERR "!!![voocphy] fastchg to normal bf: [%d]\n",
				chip->voocphy.fastchg_to_normal);

			//check fastchg out
			oplus_chg_set_charger_type_unknown();
			oplus_vooc_check_charger_out();

			//wake update work
			printk(KERN_ERR "!!![voocphy] fastchg to normal af: [%d]\n",
				chip->voocphy.fastchg_to_normal);
			oplus_chg_wake_update_work();
		} else if (intval == FAST_NOTIFY_BATT_TEMP_OVER) {
			//switch usbswitch and ctrl2
			printk(KERN_ERR "FAST_NOTIFY_BATT_TEMP_OVER\r\n");
			oplus_chg_set_chargerid_switch_val(0);
			oplus_vooc_switch_mode(NORMAL_CHARGER_MODE);

			chip->voocphy.fastchg_to_warm = true;
			usleep_range(350000, 350000);
			chip->voocphy.fastchg_start = false;
			chip->voocphy.fastchg_dummy_start = false;
			chip->voocphy.fastchg_to_normal = false;
			chip->voocphy.fastchg_ing = false;

			printk(KERN_ERR "!!![voocphy] fastchg to warm bf: [%d]\n",
					chip->voocphy.fastchg_to_warm);

			//check fastchg out and temp recover
			//vooc_monitor_timer_start(VOOC_THREAD_TIMER_FASTCHG_CHECK, VOOC_FASTCHG_CHECK_TIME);
			oplus_chg_set_charger_type_unknown();
			oplus_vooc_check_charger_out();

			printk(KERN_ERR "!!![voocphy] fastchg to warm af: [%d]\n",
					chip->voocphy.fastchg_to_warm);
			oplus_chg_wake_update_work();
		} else if (intval == FAST_NOTIFY_ERR_COMMU) {
			chip->voocphy.fastchg_start = false;
			chip->voocphy.fastchg_to_warm = false;
			chip->voocphy.fastchg_dummy_start = false;
			chip->voocphy.fastchg_to_normal = false;
			chip->voocphy.fastchg_ing = false;
			chip->voocphy.fast_chg_type = 0;
			oplus_chg_set_charger_type_unknown();
			oplus_chg_wake_update_work();
			printk(KERN_ERR "!!![voocphy] fastchg err commu: [%d]\n", intval);
		} else if (intval == FAST_NOTIFY_USER_EXIT_FASTCHG) {
			//switch usbswitch and ctrl2
			printk(KERN_ERR "FAST_NOTIFY_USER_EXIT_FASTCHG\r\n");
			oplus_chg_set_chargerid_switch_val(0);
			oplus_vooc_switch_mode(NORMAL_CHARGER_MODE);

			oplus_voocphy_set_user_exit_fastchg(true);
			usleep_range(350000, 350000);
			chip->voocphy.fastchg_start = false;
			chip->voocphy.fastchg_dummy_start = true;
			chip->voocphy.fastchg_to_normal = false;
			chip->voocphy.fastchg_ing = false;

			printk(KERN_ERR "!!![voocphy] user_exit_fastchg bf: [%d]\n",
					oplus_vooc_mg->user_exit_fastchg);

			//check fastchg out and temp recover
			//vooc_monitor_timer_start(VOOC_THREAD_TIMER_FASTCHG_CHECK, VOOC_FASTCHG_CHECK_TIME);
			oplus_chg_set_charger_type_unknown();
			oplus_vooc_check_charger_out();

			printk(KERN_ERR "!!![voocphy] user_exit_fastchg af: [%d]\n",
					oplus_vooc_mg->user_exit_fastchg);
			oplus_chg_wake_update_work();
		} else {
			printk(KERN_ERR "!!![voocphy] non handle status: [%d]\n", intval);
		}
	}
}

bool oplus_vooc_wake_notify_fastchg_work(void)
{
	return schedule_delayed_work(&oplus_vooc_mg->notify_fastchg_work, 0);
}


void oplus_vooc_set_status_and_notify_ap(FASTCHG_STATUS fastchg_notify_status)
{
	UINT8 dchg_enable_status = 0;
	struct oplus_chg_chip *chip = oplus_get_oplus_chip();
	sc_err( "monitor fastchg_notify:%d", fastchg_notify_status);

	switch (fastchg_notify_status) {
		case FAST_NOTIFY_PRESENT:
		case FAST_NOTIFY_DUMMY_START:
		oplus_vooc_mg->fastchg_notify_status = fastchg_notify_status;	//lkl need modify(add)
			if (fastchg_notify_status == FAST_NOTIFY_DUMMY_START) {
				//start fastchg_check timer
				sc_err( "VOOC_THREAD_TIMER_FASTCHG_CHECK start for FAST_NOTIFY_DUMMY_START:%d", fastchg_notify_status);
				vooc_monitor_timer_start(VOOC_THREAD_TIMER_FASTCHG_CHECK, VOOC_FASTCHG_CHECK_TIME);

				//reset vooc_phy
				oplus_vooc_reset_voocphy();
				//oplus_vooc_wake_voocphy_service_work(VOOCPHY_REQUEST_RESET);

				//stop voocphy commu with adapter time
				vooc_commu_process_handle(false);
				voocphy_cpufreq_update(CPU_CHG_FREQ_STAT_AUTO);
			}
			break;
		case FAST_NOTIFY_ONGOING:
			oplus_vooc_mg->fastchg_notify_status = fastchg_notify_status;
			break;
		case FAST_NOTIFY_FULL:
		case FAST_NOTIFY_BATT_TEMP_OVER:
		case FAST_NOTIFY_BAD_CONNECTED:
		case FAST_NOTIFY_USER_EXIT_FASTCHG:
			chip->voocphy.ap_disconn_issue = fastchg_notify_status;
			vooc_monitor_timer_start(VOOC_THREAD_TIMER_DISCON, VOOC_DISCON_EVENT_TIME);
			oplus_vooc_mg->fastchg_notify_status = fastchg_notify_status;

			//stop monitor thread
			vooc_monitor_stop(0);

			if (fastchg_notify_status == FAST_NOTIFY_BATT_TEMP_OVER) {
				//start fastchg_check timer
				//oplus_vooc_mg->fastchg_to_warm = true;
				//chip->voocphy.fastchg_to_warm = true;
				sc_err( "VOOC_THREAD_TIMER_FASTCHG_CHECK start for FAST_NOTIFY_BATT_TEMP_OVER:%d", fastchg_notify_status);
				vooc_monitor_timer_start(VOOC_THREAD_TIMER_FASTCHG_CHECK, VOOC_FASTCHG_CHECK_TIME);
			}

			/*if (fastchg_notify_status == FAST_NOTIFY_USER_EXIT_FASTCHG) {
				oplus_voocphy_set_user_exit_fastchg(true);
				oplus_vooc_mg->fastchg_dummy_start = true;
				chip->voocphy.fastchg_dummy_start = true;
			}*/

			//reset vooc_phy
			oplus_vooc_reset_voocphy();

			//stop voocphy commu with adapter time
			vooc_commu_process_handle(false);		//lkl need modify
			break;
		case FAST_NOTIFY_ERR_COMMU:
			oplus_vooc_mg->fastchg_notify_status = fastchg_notify_status;
			chip->voocphy.ap_disconn_issue = fastchg_notify_status;
			chip->voocphy.rcv_date_err_num += oplus_vooc_mg->rcv_date_err_num;
			oplus_vooc_mg->rcv_date_err_num = 0;

			/*if dchg enable, then reset_voocphy will create pluginout, so wo need not run
			5s fastchg check, if not we need run the 5s thread to recovery the vooc*/
			//PmSchgVooc_GetDirectChgEnable(PMIC_INDEX_3, &dchg_enable_status);
			oplus_voocphy_direct_chg_enable(&dchg_enable_status);
			if (oplus_vooc_mg->fastchg_err_commu == false && ((oplus_vooc_mg->adapter_type == ADAPTER_SVOOC && dchg_enable_status == 0)
				||(oplus_vooc_mg->adapter_type == ADAPTER_VOOC20 && oplus_vooc_mg->fastchg_allow))) {
				sc_err("!VOOC_THREAD_TIMER_FASTCHG_CHECK abnormal status, should run fastchg check");
				vooc_monitor_timer_start(VOOC_THREAD_TIMER_FASTCHG_CHECK, VOOC_FASTCHG_CHECK_TIME);
				oplus_vooc_mg->fastchg_err_commu = true;
				oplus_reset_fastchg_after_usbout();
				oplus_vooc_mg->fastchg_start = false;
				chip->voocphy.fastchg_start = oplus_vooc_mg->fastchg_start;
			} else {
				oplus_vooc_mg->fastchg_start = false;
				chip->voocphy.fastchg_start = oplus_vooc_mg->fastchg_start;
				oplus_reset_fastchg_after_usbout();
			}

			//reset vooc_phy
			oplus_vooc_reset_voocphy();

			//stop voocphy commu with adapter time
			vooc_commu_process_handle(false);

			//stop monitor thread
			vooc_monitor_stop(0);

			oplus_chg_set_charger_type_unknown();
			break;
		default:
			/*handle other error status*/
			oplus_vooc_mg->fastchg_start = false;
			chip->voocphy.fastchg_start = oplus_vooc_mg->fastchg_start;
			oplus_reset_fastchg_after_usbout();

			//reset vooc_phy
			oplus_vooc_reset_voocphy();

			//stop voocphy commu with adapter time
			vooc_commu_process_handle(false);

			//stop monitor thread
			vooc_monitor_stop(0);
			break;
	}
	if (fastchg_notify_status != FAST_NOTIFY_ONGOING
		&& fastchg_notify_status != FAST_NOTIFY_PRESENT) {
		voocphy_cpufreq_update(CPU_CHG_FREQ_STAT_AUTO);
	}
	//pmic_glink_send_power_supply_notification(VOOC_STATUS_GET_REQ);
	oplus_vooc_wake_notify_fastchg_work();
}


void sc8547_send_handshake_seq(void)
{
	sc_info("fastchg start signal\n");
	sc8547_write_byte(__sc, SC8547_REG_2B, 0x81);
}

bool oplus_voocphy_user_exit_fastchg(void)
{
	if (oplus_vooc_mg) {
		sc_info("user 1 %s fastchg\n", oplus_vooc_mg->user_exit_fastchg == true ? "not allow" : "allow");
		return oplus_vooc_mg->user_exit_fastchg;
	} else {
		sc_info("user 2 %s fastchg\n", "not allow");
		return false;
	}
}

bool oplus_voocphy_set_user_exit_fastchg(unsigned char exit)
{
	if (oplus_vooc_mg) {
		sc_err("exit %d\n", exit);
		oplus_vooc_mg->user_exit_fastchg = exit;
	}
	return true;
}

bool oplus_voocphy_set_fastchg_state(unsigned char fastchg_state)
{
	if (oplus_vooc_mg) {
		//sc_err("fastchg_state %d\n", fastchg_state);
		oplus_vooc_mg->fastchg_stage = fastchg_state;
	}
	return true;
}

unsigned char oplus_voocphy_get_fastchg_state(void)
{
	if (!oplus_vooc_mg) {
		sc_err("fastchg_state %d\n", oplus_vooc_mg->fastchg_stage);
		return OPLUS_FASTCHG_STAGE_1;
	}

	sc_info("fastchg_state %d\n", oplus_vooc_mg->fastchg_stage);
	return oplus_vooc_mg->fastchg_stage;
}

extern int oplus_chg_get_curr_time_ms(unsigned long *time_ms);
bool oplus_voocphy_stage_check(void)
{

	unsigned long cur_chg_time = 0;
	if (oplus_voocphy_get_fastchg_state() == OPLUS_FASTCHG_STAGE_2) {
		sc_info("torch return for OPLUS_FASTCHG_STAGE_2\n");
		return false;
	}

	if (oplus_voocphy_get_fastchg_state() == OPLUS_FASTCHG_STAGE_1)
	{
		if (oplus_voocphy_user_exit_fastchg()) {
			sc_info("torch return for user_exit_fastchg \n");
			return false;
		} else if (oplus_vooc_mg->fastchg_recovering){
			oplus_chg_get_curr_time_ms(&cur_chg_time);
			cur_chg_time = cur_chg_time/1000;
			if (cur_chg_time - oplus_vooc_mg->fastchg_recover_cnt >= 30) {
				/*fastchg_recovering need to check num of calling*/
				sc_info("torch return for fastchg_recovering1  \n");
				oplus_vooc_mg->fastchg_recovering = false;
				oplus_vooc_mg->fastchg_recover_cnt = 0;
				goto stage_check;
			} else {
				sc_info("torch return for fastchg_recovering2  \n");
				return false;
			}
		} else {
			sc_info("do nothing for [%d %d]\n", oplus_vooc_mg->fastchg_recovering, oplus_vooc_mg->fastchg_recover_cnt);
			goto stage_check;
		}
	}
stage_check:
	return true;
}

void oplus_notify_voocphy(int event)
{
	bool plugIn = false;
	struct oplus_chg_chip *chip = oplus_get_oplus_chip();
	unsigned long cur_chg_time = 0;
	sc_err(" start %s flash led %d, %d\n", event ? "open" : "close", oplus_vooc_mg->user_exit_fastchg, oplus_vooc_mg->fastchg_recovering);
	if (1 == event) {				//exit fastchg
		if (oplus_chg_stats() && (oplus_voocphy_get_fastchg_state() == OPLUS_FASTCHG_STAGE_2)) {
			oplus_vooc_mg->user_set_exit_fastchg = true;
		}
		if (oplus_voocphy_get_fastchg_state() == OPLUS_FASTCHG_STAGE_2) {
 			oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_USER_EXIT_FASTCHG);
		} else if (oplus_vooc_mg->fastchg_recovering == false){
			//plugIn = oplus_chg_stats();
			oplus_chg_wake_update_work();
			sc_err("is normal adapter plugIn = %d\n", plugIn);
		} else {
			oplus_vooc_mg->fastchg_recovering = false;
			oplus_voocphy_set_user_exit_fastchg(true) ;
		}
	} else if (0 == event/* && chip->charger_type == POWER_SUPPLY_TYPE_USB_DCP*/) {		//recover
		oplus_vooc_mg->user_set_exit_fastchg = false;
		chip->voocphy.fastchg_to_normal = false;
		oplus_voocphy_set_user_exit_fastchg(false) ;
		oplus_voocphy_set_fastchg_state(OPLUS_FASTCHG_STAGE_1);
		oplus_vooc_mg->fastchg_recover_cnt = 0;
		plugIn = oplus_chg_stats();
		if (!plugIn) {
			oplus_vooc_mg->fastchg_recovering = false;
			return;
		}
		oplus_vooc_mg->fastchg_recovering = true;
		//vooc_monitor_timer_start(VOOC_THREAD_TIMER_FASTCHG_CHECK, VOOC_FASTCHG_CHECK_TIME);
		oplus_chg_get_curr_time_ms(&cur_chg_time);
		oplus_vooc_mg->fastchg_recover_cnt = cur_chg_time/1000;

		sc_err("VOOC_THREAD_TIMER_FASTCHG_CHECK plugIn %d charger_type %d %d switch %d %d\n",
			plugIn, chip->charger_type, chip->chg_ops->get_charger_type(),
			chip->chg_ops->get_chargerid_switch_val(), oplus_vooc_get_vooc_switch_val());
		if (plugIn && chip->charger_type == POWER_SUPPLY_TYPE_USB_DCP) {
			oplus_chg_wake_update_work();
		}
	}
	sc_err(" end %s flash led %d\n", event ? "open" : "close", oplus_vooc_mg->user_exit_fastchg);
}

UINT8 oplus_vooc_set_fastchg_current(UINT8 max_curr, UINT8 ap_curr, UINT8 temp_curr)
{
	UINT8 current_expect_temp = oplus_vooc_mg->current_expect;

	sc_err( "max_curr[%d]   ap_curr[%d]   temp_curr[%d]",
								max_curr, ap_curr, temp_curr);

	oplus_vooc_mg->current_ap = ap_curr; //update the valid current_ap
	oplus_vooc_mg->current_batt_temp = temp_curr; //update the final temp_curr

	if (max_curr > oplus_vooc_mg->current_max)
		oplus_vooc_mg->current_max = max_curr;

	if (ap_curr < temp_curr)
		temp_curr = ap_curr;

	if (oplus_vooc_mg->current_max > temp_curr)
		oplus_vooc_mg->current_expect = temp_curr;
	else
		oplus_vooc_mg->current_expect = oplus_vooc_mg->current_max;

	if (current_expect_temp != oplus_vooc_mg->current_expect) {
		sc_err( "current_expect[%d --> %d]",
					current_expect_temp, oplus_vooc_mg->current_expect);
		return 5;
	}

	return 0;
}

enum hrtimer_restart vooc_monitor_base_timer(struct hrtimer* time)
{
	int i = 0;

	//cont for all evnents
	for (i = 0; i < MONITOR_EVENT_NUM; i++) {
		if (oplus_vooc_mg->mornitor_evt[i].status == VOOC_MONITOR_START) {
			oplus_vooc_mg->mornitor_evt[i].cnt++;
			sc_dbg( "timex %d [%d %d] %d", i, oplus_vooc_mg->mornitor_evt[i].cnt, oplus_vooc_mg->mornitor_evt[i].expires, oplus_vooc_mg->fastchg_monitor_stop);
			if (oplus_vooc_mg->mornitor_evt[i].cnt == oplus_vooc_mg->mornitor_evt[i].expires) {
				oplus_vooc_mg->mornitor_evt[i].timeout = true;
				oplus_vooc_mg->mornitor_evt[i].cnt = 0;
				oplus_vooc_mg->mornitor_evt[i].status = VOOC_MONITOR_STOP;
				//notify monitor event
				oplus_vooc_wake_monitor_work();
			}
		}
	}
	hrtimer_forward(&oplus_vooc_mg->monitor_btimer,oplus_vooc_mg->monitor_btimer.base->get_time(),oplus_vooc_mg->moniotr_kt);

	return HRTIMER_RESTART;
}

void vooc_monitor_process_events(struct work_struct *work)
{
	int evt_i = 0;
	unsigned long data = 0;

	//event loop
	for (evt_i = 0; evt_i < MONITOR_EVENT_NUM; evt_i++) {
		sc_dbg(" [%d %d]\n", evt_i, oplus_vooc_mg->mornitor_evt[evt_i].timeout);
		if (oplus_vooc_mg->mornitor_evt[evt_i].timeout) {
			oplus_vooc_mg->mornitor_evt[evt_i].timeout = false;		//avoid repeating run monitor event
			data = oplus_vooc_mg->mornitor_evt[evt_i].data;
			if (oplus_vooc_mg->mornitor_evt[evt_i].mornitor_hdler)
				oplus_vooc_mg->mornitor_evt[evt_i].mornitor_hdler(data);
		}
	}

	return;
}

extern int oplus_ap_current_notify_voocphy(void);
BattMngr_err_code_e vooc_ap_event_handle(unsigned long data)	//lkl need modify
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	UINT8 ap_current = 0;
	static BOOLEAN ignore_first_monitor = true;

	if (oplus_vooc_mg->fastchg_monitor_stop == true) {
		sc_err( "vooc_ap_event_handle ignore");
		return status;
	}
	if ((oplus_vooc_mg->fastchg_notify_status & 0XFF) == FAST_NOTIFY_PRESENT) {
		ignore_first_monitor = true;
	}

	oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_ONGOING);

	oplus_vooc_mg->ap_request_current = oplus_ap_current_notify_voocphy();
	sc_err( "ignore_first_monitor %d %u\n", ignore_first_monitor, oplus_vooc_mg->ap_request_current);
	if (!oplus_vooc_mg->btb_temp_over) {   //btb temp is normal
		if (oplus_vooc_mg->ap_request_current >= AP_CURRENT_REQUEST_MIN
			&& oplus_vooc_mg->ap_request_current <= AP_CURRENT_REQUEST_MAX) {
			ap_current = oplus_vooc_mg->ap_request_current;
		} else {
			ap_current = CC_VALUE_2_6C;
		}
		oplus_vooc_mg->current_ap = ap_current;
		if ((!oplus_vooc_mg->ap_need_change_current)
			&& (ignore_first_monitor == false)) {
			oplus_vooc_mg->ap_need_change_current
				= oplus_vooc_set_fastchg_current(oplus_vooc_mg->current_max,
					ap_current, oplus_vooc_mg->current_batt_temp);
			sc_dbg( "ap_need_change_current %d\n", oplus_vooc_mg->ap_need_change_current);
		} else {
			ignore_first_monitor = false;
		}

		status = vooc_monitor_timer_start(VOOC_THREAD_TIMER_AP, VOOC_AP_EVENT_TIME);
	}

	return status;
}

static UINT8 oplus_vooc_set_current_1_temp_normal_range(INT32 batt_temp)
{
	UINT8 ret = 0;
	static UINT8 vooc_strategy_change_count = 0;

	oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_NORMAL;
	if ((oplus_vooc_mg->fastchg_notify_status & 0xFF) == FAST_NOTIFY_PRESENT){
		vooc_strategy_change_count = 0;
	}

	switch (oplus_vooc_mg->fastchg_batt_temp_status) {
		case BAT_TEMP_NATURAL :
			if (batt_temp > VOOC_STRATEGY1_BATT_HIGH_TEMP0) {	//385
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
				ret = VOOC_STRATEGY1_HIGH_CURRENT0;				//6A
			} else if (batt_temp > VOOC_LITTLE_COOL_TEMP) {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;				//6A
			} else if (batt_temp > VOOC_COOL_TEMP) {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LITTLE_COOL_LOW;
				ret = VOOC_NORMAL_TO_LITTLE_COOL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_COOL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_COOL;
			}
			break;
		case BAT_TEMP_HIGH0 :
			if (batt_temp > VOOC_STRATEGY1_BATT_HIGH_TEMP1) {	//395
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = VOOC_STRATEGY1_HIGH_CURRENT1;				//6A
			} else if (batt_temp < VOOC_STRATEGY1_BATT_LOW_TEMP0) {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LOW0;
				ret = VOOC_STRATEGY1_LOW_CURRENT0;				//6A
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
				ret = VOOC_STRATEGY1_HIGH_CURRENT0;				//6A
			}
			break;
		case BAT_TEMP_HIGH1 :
			if (batt_temp > VOOC_STRATEGY1_BATT_HIGH_TEMP2) {	//420
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = VOOC_STRATEGY1_HIGH_CURRENT2;				//4A
			} else if (batt_temp < VOOC_STRATEGY1_BATT_LOW_TEMP1) {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LOW1;
				ret = VOOC_STRATEGY1_LOW_CURRENT1;
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = VOOC_STRATEGY1_HIGH_CURRENT1;				//6A
			}
			break;
		case BAT_TEMP_HIGH2:
			if (batt_temp > VOOC_BATT_OVER_HIGH_TEMP) {			//440
				vooc_strategy_change_count++;
				ret = oplus_vooc_mg->current_expect;
				if (vooc_strategy_change_count >= VOOC_TEMP_OVER_COUNTS) {
					vooc_strategy_change_count = 0;
					sc_err( "[vooc_monitor] temp_over:%d", batt_temp);
			 		oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_BATT_TEMP_OVER);
				}
			}else if (batt_temp < VOOC_STRATEGY1_BATT_LOW_TEMP2) {	//430
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LOW2;
				ret = VOOC_STRATEGY1_LOW_CURRENT2;					//4A
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = VOOC_STRATEGY1_HIGH_CURRENT2;					//4A
				vooc_strategy_change_count = 0;
			}
			break;
		case BAT_TEMP_LOW0:
			if (batt_temp > VOOC_STRATEGY1_BATT_HIGH_TEMP0) {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
				ret = VOOC_STRATEGY1_HIGH_CURRENT0;
			} else if (batt_temp < VOOC_LITTLE_COOL_TEMP) { //<16
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LITTLE_COOL_LOW;
				ret = VOOC_NORMAL_TO_LITTLE_COOL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
			} else if (batt_temp < VOOC_STRATEGY_BATT_NATRUE_TEMP) { //383
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;					//6A
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LOW0;
				ret = VOOC_STRATEGY1_LOW_CURRENT0;					//6A
			}
			break;
		case BAT_TEMP_LOW1:
			if (batt_temp > VOOC_STRATEGY1_BATT_HIGH_TEMP1) {		//395
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = VOOC_STRATEGY1_HIGH_CURRENT1;					//4A
			} else if (batt_temp < VOOC_STRATEGY1_BATT_LOW_TEMP0) {	//385
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LOW0;
				ret = VOOC_STRATEGY1_LOW_CURRENT0;					//6A
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LOW1;
				ret = VOOC_STRATEGY1_LOW_CURRENT1;					//6A
			}
			break;
		case BAT_TEMP_LOW2:
			if (batt_temp > VOOC_STRATEGY1_BATT_HIGH_TEMP2) {		//420
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = VOOC_STRATEGY1_HIGH_CURRENT2;					//2A
			} else if (batt_temp < VOOC_STRATEGY1_BATT_LOW_TEMP1) {	//410
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LOW1;
				ret = VOOC_STRATEGY1_LOW_CURRENT1;					//6A
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LOW2;
				ret = VOOC_STRATEGY1_LOW_CURRENT2;					//4A
			}
			break;
		default:
			ret = VOOC_STRATEGY_NORMAL_CURRENT;
			sc_err( "[vooc_monitor] should not go here");
			break;
	}

	sc_err( "[vooc_monitor][below] batt_temp =%d, normal_status = %d ret = %d",
			batt_temp, oplus_vooc_mg->fastchg_batt_temp_status, ret);
	return ret;
}

static UINT8 oplus_vooc_set_current_2_temp_normal_range(INT32 batt_temp)
{
	UINT8 ret = 0;
	static UINT8 vooc_strategy_change_count = 0;

	oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_NORMAL;
	if ((oplus_vooc_mg->fastchg_notify_status & 0xFF) == FAST_NOTIFY_PRESENT){
		vooc_strategy_change_count = 0;
	}

	switch (oplus_vooc_mg->fastchg_batt_temp_status) {
		case BAT_TEMP_NATURAL :
			if (batt_temp > VOOC_STRATEGY2_BATT_UP_TEMP1) {				//385
				if (oplus_is_power_off_charging(NULL) && batt_temp > VOOC_STRATEGY2_BATT_UP_TEMP5) {			//420
					oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
					ret = VOOC_STRATEGY2_HIGH2_CURRENT; 					//4A
				} else {
					oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
					ret = VOOC_STRATEGY2_HIGH0_CURRENT; 					//6A
				}
			} else if (batt_temp > VOOC_LITTLE_COOL_TEMP) {				//160
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;						//6A
			} else if (batt_temp > VOOC_COOL_TEMP) {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LITTLE_COOL_LOW;
				ret = VOOC_NORMAL_TO_LITTLE_COOL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_COOL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_COOL;
			}
			break;
		case BAT_TEMP_HIGH0:
			if (batt_temp > VOOC_STRATEGY2_BATT_UP_TEMP3) {				//395
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = VOOC_STRATEGY2_HIGH1_CURRENT;						//6A
			} else if (batt_temp < VOOC_LITTLE_COOL_TEMP) { 			//<16
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LITTLE_COOL_LOW;
				ret = VOOC_NORMAL_TO_LITTLE_COOL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
			} else if (batt_temp < VOOC_STRATEGY_BATT_NATRUE_TEMP) {	//383
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;						//6A
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
				ret = VOOC_STRATEGY2_HIGH0_CURRENT;						//6A
			}
			break;
		case BAT_TEMP_HIGH1:
			if (batt_temp > VOOC_STRATEGY2_BATT_UP_TEMP5) {				//420
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = VOOC_STRATEGY2_HIGH2_CURRENT;						//4A
			} else if (batt_temp < VOOC_STRATEGY2_BATT_UP_DOWN_TEMP7) {	//385
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
				ret = VOOC_STRATEGY2_HIGH0_CURRENT;						//6A
			} else if (batt_temp < VOOC_LITTLE_COOL_TEMP) { 			//<16
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LITTLE_COOL_LOW;
				ret = VOOC_NORMAL_TO_LITTLE_COOL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = VOOC_STRATEGY2_HIGH1_CURRENT;						//6A
			}
			break;
		case BAT_TEMP_HIGH2:
			if (batt_temp > VOOC_STRATEGY2_BATT_UP_TEMP6) {				//430
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH3;
				ret = VOOC_STRATEGY2_HIGH3_CURRENT;						//4A
			} else if (batt_temp < VOOC_STRATEGY2_BATT_UP_DOWN_TEMP2){	//410
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = VOOC_STRATEGY2_HIGH1_CURRENT;						//6A
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = VOOC_STRATEGY2_HIGH2_CURRENT;						//4A
			}
			break;
		case BAT_TEMP_HIGH3:
			if (batt_temp > VOOC_BATT_OVER_HIGH_TEMP) {					//440
				vooc_strategy_change_count++;
				ret = oplus_vooc_mg->current_expect;
				if (vooc_strategy_change_count >= VOOC_TEMP_OVER_COUNTS) {
					vooc_strategy_change_count = 0;
					sc_dbg( "[vooc_monitor] temp_over:%d", batt_temp);
			 		oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_BATT_TEMP_OVER);
				}
			}else if (batt_temp < VOOC_STRATEGY2_BATT_UP_DOWN_TEMP4) {	//430
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = VOOC_STRATEGY2_HIGH2_CURRENT;						//4A
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_HIGH3;
				ret = VOOC_STRATEGY2_HIGH3_CURRENT;						//4A
				vooc_strategy_change_count = 0;
			}
			break;
		default:
			ret = VOOC_STRATEGY_NORMAL_CURRENT;
			sc_dbg( "[vooc_monitor] should not go here");
			break;
	}

	sc_dbg( "[vooc_monitor][up] batt_temp =%d, normal_status = %d",
			batt_temp, oplus_vooc_mg->fastchg_batt_temp_status);
	return ret;
}

static UINT8 oplus_vooc_set_current_temp_little_cool_range(INT32 batt_temp)
{
	UINT8 ret = 0;

	switch (oplus_vooc_mg->fastchg_batt_temp_status) {
		case BAT_TEMP_LITTLE_COOL:
			if (batt_temp < VOOC_COOL_TEMP) {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_COOL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_COOL;
				//need set current_max when down to 5-12C
				if (oplus_vooc_mg->current_max > TEMP45_2_TEMP115_CURR) {
					oplus_vooc_mg->current_max = TEMP45_2_TEMP115_CURR;
					oplus_vooc_mg->batt_temp_plugin = BATT_TEMP_COOL;
					sc_dbg( "[vooc_monitor] goto 5-12 C logic");
				}
			} else if (batt_temp > VOOC_LITTLE_COOL_TEMP) {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_NORMAL;
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LITTLE_COOL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
			}
			break;
		case BAT_TEMP_LITTLE_COOL_LOW:
			if (batt_temp < VOOC_COOL_TEMP) {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_COOL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_COOL;
				//need set current_max when down to 5-12C
				if (oplus_vooc_mg->current_max > TEMP45_2_TEMP115_CURR) {
					oplus_vooc_mg->current_max = TEMP45_2_TEMP115_CURR;
					oplus_vooc_mg->batt_temp_plugin = BATT_TEMP_COOL;
					sc_dbg( "[vooc_monitor] goto 5-12 C logic");
				}
			} else if(batt_temp > VOOC_LITTLE_COOL_TO_NORMAL_TEMP){
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
				ret = VOOC_STRATEGY_NORMAL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_NORMAL;
			} else {
				oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LITTLE_COOL_LOW;
				ret = VOOC_NORMAL_TO_LITTLE_COOL_CURRENT;
				oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
			}
			break;
		default:
			ret = VOOC_STRATEGY_NORMAL_CURRENT;
			sc_dbg( "[vooc_monitor] should not go here");
			break;
	}

	return ret;
}

static UINT8 oplus_vooc_set_current_temp_cool_range(INT32 batt_temp)
{
	UINT8 ret = 0;
	static UINT8 vooc_strategy_change_count = 0;

	if ((oplus_vooc_mg->fastchg_notify_status & 0XFF) == FAST_NOTIFY_PRESENT){
		vooc_strategy_change_count = 0;
	}

	if (batt_temp < VOOC_BATT_OVER_LOW_TEMP) {
		vooc_strategy_change_count++;
		ret = oplus_vooc_mg->current_expect;
		if (vooc_strategy_change_count >= VOOC_TEMP_OVER_COUNTS) {
			vooc_strategy_change_count = 0;
			sc_dbg( "[vooc_monitor] temp_over:%d", batt_temp);
			 		oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_BATT_TEMP_OVER);
		}
	}else if (batt_temp < VOOC_COOL_TEMP) {
		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_COOL;
		oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_COOL;
		ret = VOOC_STRATEGY_NORMAL_CURRENT;
	} else if (batt_temp < VOOC_LITTLE_COOL_TEMP) {
		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
		oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LITTLE_COOL;
		ret = VOOC_STRATEGY_NORMAL_CURRENT;
	} else {
		oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
		ret = VOOC_STRATEGY_NORMAL_CURRENT;
		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_NORMAL;
		vooc_strategy_change_count = 0;
	}

	return ret;
}


static BattMngr_err_code_e oplus_vooc_set_fastchg_curr_below_logic_batt_temp(INT32 batt_temp)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	UINT8 temp_curr = 0;

	if (oplus_vooc_mg->vooc_temp_cur_range == FASTCHG_TEMP_RANGE_INIT) {
		if (batt_temp < VOOC_COOL_TEMP) {				//12 cool

			oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_COOL;
			oplus_vooc_mg->fastchg_batt_temp_status =  BAT_TEMP_COOL;
		} else if (batt_temp < VOOC_LITTLE_COOL_TEMP) {	//16 little cool
			oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
			oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LITTLE_COOL;
		} else {										//>16	normal
			oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_NORMAL;
			oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
		}
	}

	switch (oplus_vooc_mg->vooc_temp_cur_range) {
		case FASTCHG_TEMP_RANGE_NORMAL :
			temp_curr = oplus_vooc_set_current_1_temp_normal_range(batt_temp);
			break;
		case FASTCHG_TEMP_RANGE_LITTLE_COOL :
			temp_curr = oplus_vooc_set_current_temp_little_cool_range(batt_temp);
			break;
		case FASTCHG_TEMP_RANGE_COOL :
			temp_curr = oplus_vooc_set_current_temp_cool_range(batt_temp);
			break;
		default :
			temp_curr = VOOC_STRATEGY_NORMAL_CURRENT;
			sc_dbg( "[vooc_monitor] should not go here");
			break;
	}
	sc_dbg( "[vooc_monitor][below] temp_curr = %d, batt_temp = %d, \
			temp_status = %d, temp_range = %d", temp_curr, batt_temp,
			oplus_vooc_mg->fastchg_batt_temp_status, oplus_vooc_mg->vooc_temp_cur_range);

	if (temp_curr > 30){
		temp_curr = 30;
	}

	if (!oplus_vooc_mg->ap_need_change_current) {
		oplus_vooc_mg->ap_need_change_current = oplus_vooc_set_fastchg_current(
			oplus_vooc_mg->current_max, oplus_vooc_mg->current_ap, temp_curr);
	}

	return status;
}

static BattMngr_err_code_e oplus_vooc_set_fastchg_curr_up_logic_batt_temp(INT32 batt_temp)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	UINT8 temp_curr = 0;

	if (oplus_vooc_mg->vooc_temp_cur_range == FASTCHG_TEMP_RANGE_INIT) {
		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_NORMAL;
		oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
	}

	switch (oplus_vooc_mg->vooc_temp_cur_range) {
		case FASTCHG_TEMP_RANGE_NORMAL :
			temp_curr = oplus_vooc_set_current_2_temp_normal_range(batt_temp);
			break;
		case FASTCHG_TEMP_RANGE_LITTLE_COOL :
			temp_curr = oplus_vooc_set_current_temp_little_cool_range(batt_temp);
			break;
		case FASTCHG_TEMP_RANGE_COOL :
			temp_curr = oplus_vooc_set_current_temp_cool_range(batt_temp);
			break;
		default:
			temp_curr = VOOC_STRATEGY_NORMAL_CURRENT;
			sc_dbg( "[vooc_monitor] should not go here");
			break;
	}

	sc_dbg( "[vooc_monitor][up] temp_curr = %d, batt_temp = %d, \
			temp_status = %d, temp_range = %d", temp_curr, batt_temp,
			oplus_vooc_mg->fastchg_batt_temp_status, oplus_vooc_mg->vooc_temp_cur_range);

	if (temp_curr > 30){
		temp_curr = 30;
	}

	if (!oplus_vooc_mg->ap_need_change_current) {
		oplus_vooc_mg->ap_need_change_current = oplus_vooc_set_fastchg_current(
			oplus_vooc_mg->current_max, oplus_vooc_mg->current_ap, temp_curr);
	}

	return status;
}




BOOLEAN oplus_vooc_btb_and_usb_temp_detect(void)
{
	BOOLEAN detect_over = false;
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	INT32 btb_temp =0, usb_temp = 0;
	static UINT8 temp_over_count = 0;

	if ((oplus_vooc_mg->fastchg_notify_status & 0XFF)== FAST_NOTIFY_PRESENT) {
		temp_over_count = 0;
	}

	//status = battman_adc_read(BATTMAN_ADC_IDX_BATT_THERM_C_PU, &btb_temp);
	//status |= battman_adc_read(BATTMAN_ADC_IDX_CONN_THERM_C_PM_03, &usb_temp);
	//lkl need modify
	btb_temp = 25;
	usb_temp = 25;

	if (status != BATTMNGR_SUCCESS) {
		sc_dbg( "get btb and usb temp error");
		return detect_over;
	}

	sc_dbg( "btb_temp: %d, usb_temp: %d", btb_temp, usb_temp);
	if (btb_temp >= 80 ||usb_temp >= 80) {
		temp_over_count++;
		if (temp_over_count > 9) {
			detect_over = true;
			sc_dbg( "btb_and_usb temp over");
		}
	} else {
		temp_over_count = 0;
	}

	return detect_over;
}

BattMngr_err_code_e vooc_btb_event_handle(unsigned long data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	BOOLEAN btb_detect = false;
	static BOOLEAN btb_err_report = false;

	sc_dbg( "vooc_btb_event_handle");
	if (oplus_vooc_mg->fastchg_monitor_stop == true) {
		sc_dbg( "vooc_btb_event_handle ignore");
		return status;
	}

	btb_detect = oplus_vooc_btb_and_usb_temp_detect();
	if ((oplus_vooc_mg->fastchg_notify_status & 0XFF) == FAST_NOTIFY_PRESENT) {
		btb_err_report = false;
	}

	if(!oplus_vooc_mg->btb_temp_over) {
		if (btb_detect) {
			//PmSchgVooc_SetDirectChgEnable(PMIC_INDEX_3, false, false);
			//lkl need modify
			oplus_vooc_mg->btb_temp_over = true;
		}
		status = vooc_monitor_timer_start(VOOC_THREAD_TIMER_BTB, VOOC_BTB_EVENT_TIME);
	} else {
		if (!btb_err_report) {
			btb_err_report = true;
			oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_BTB_TEMP_OVER);
		}
		status = vooc_monitor_timer_start(VOOC_THREAD_TIMER_BTB, VOOC_BTB_OVER_EVENT_TIME);
	}

	return status;
}

#define DELAY_TEMP_MONITOR_COUNTS		2
BattMngr_err_code_e vooc_temp_event_handle(unsigned long data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	INT32 batt_temp = 0;
	static UINT8 fastchg_logic_select= 1;
	static BOOLEAN batt_temp_first_detect = false;
	static BOOLEAN allow_temp_monitor = false;
	static UINT8 delay_temp_count = 0;

	if (oplus_vooc_mg->fastchg_monitor_stop == true) {
		sc_err( "vooc_temp_event_handle ignore");
		return status;
	}

	batt_temp = oplus_chg_get_chg_temperature();
	if (oplus_vooc_mg->batt_fake_temp) {
		batt_temp = oplus_vooc_mg->batt_fake_temp;
	}
	if ((oplus_vooc_mg->fastchg_notify_status & 0XFF) == FAST_NOTIFY_PRESENT) {
		fastchg_logic_select = 1;
		batt_temp_first_detect = true;
		delay_temp_count = 0;
		allow_temp_monitor = false;
	}

	if (delay_temp_count < DELAY_TEMP_MONITOR_COUNTS) {
		delay_temp_count++;
		allow_temp_monitor = false;
	} else {
		allow_temp_monitor = true;
		delay_temp_count = DELAY_TEMP_MONITOR_COUNTS;
	}

	if (batt_temp_first_detect) {
		if (batt_temp < VOOC_LOGIC_SELECT_BATT_TEMP) {
			fastchg_logic_select = 1;
		} else {
			fastchg_logic_select = 2;
		}
		batt_temp_first_detect = false;
	}
	sc_err( "!batt_temp = %d %d %d %d %d\n", batt_temp, allow_temp_monitor, fastchg_logic_select,
		oplus_vooc_mg->vooc_temp_cur_range, oplus_vooc_mg->fastchg_batt_temp_status);

	if ((!oplus_vooc_mg->btb_temp_over) && (allow_temp_monitor == true)) {
		if (fastchg_logic_select == 1) {
			status = oplus_vooc_set_fastchg_curr_below_logic_batt_temp(batt_temp);
		} else {
			status = oplus_vooc_set_fastchg_curr_up_logic_batt_temp(batt_temp);
		}
	}

	if (batt_temp < VOOC_COOL_TEMP) {				//12 cool

		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_COOL;
		oplus_vooc_mg->fastchg_batt_temp_status =  BAT_TEMP_COOL;
	} else if (batt_temp < VOOC_LITTLE_COOL_TEMP) {	//16 little cool
		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_LITTLE_COOL;
		oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_LITTLE_COOL;
	} else {										//>16	normal
		oplus_vooc_mg->vooc_temp_cur_range = FASTCHG_TEMP_RANGE_NORMAL;
		oplus_vooc_mg->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
	}

	if (batt_temp >= 440 || batt_temp <= 0) {
		oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_BATT_TEMP_OVER);
	}


	status = vooc_monitor_timer_start(VOOC_THREAD_TIMER_TEMP, VOOC_TEMP_EVENT_TIME);

	return status;
}

/*
	cur1:5~12
	cur2:12~16
	cur3:16~43
*/
void voocphy_request_fastchg_curve_init(void)
{
	oplus_choose_batt_sys_curve();
}

#define TARGET_VOL_OFFSET_THR	250
void voocphy_request_fastchg_curv(void)
{
	static int cc_cnt = 0;
	int idx = 0;
	int convert_ibus = 0;
	OPLUS_SVOOC_BATT_SYS_CURVES *batt_sys_curv_by_tmprange = oplus_vooc_mg->batt_sys_curv_by_tmprange;
	OPLUS_SVOOC_BATT_SYS_CURVE *batt_sys_curve = NULL;
	OPLUS_SVOOC_BATT_SYS_CURVE *batt_sys_curve_next = NULL;

	if (!oplus_vooc_mg || !batt_sys_curv_by_tmprange) {
		sc_err("oplus_vooc_mg or batt_sys_curv_by_tmprange is NULL pointer!!!\n");
		return;
	}

	if (!oplus_vooc_mg->batt_sys_curv_found) {	/*first enter and find index*/
		//use default chg
		idx = oplus_vooc_mg->cur_sys_curv_idx;
		sc_err("!fastchg curv1 [%d %d %d]\n", oplus_vooc_mg->current_max, (&(batt_sys_curv_by_tmprange->batt_sys_curve[idx]))->target_ibus, (&(batt_sys_curv_by_tmprange->batt_sys_curve[idx]))->target_vbat);
	} else {	/*adjust current*/
		idx = oplus_vooc_mg->cur_sys_curv_idx;
		if (idx >= BATT_SYS_ROW_MAX) {
			sc_err("idx out of bound of array!!!!\n");
			return;
		}
		batt_sys_curve = &(batt_sys_curv_by_tmprange->batt_sys_curve[idx]);
		if(!batt_sys_curve) {
			sc_err("batt_sys_curve is a NULL pointer!!!!\n");
			return;
		}
		batt_sys_curve->chg_time++;
		if (batt_sys_curve->exit == false) {
			sc_err("!fastchg curv2 [%d %d %d %d %d %d %d %d %d]\n", cc_cnt, idx, batt_sys_curve->exit,
				batt_sys_curve->target_time, batt_sys_curve->chg_time,
				oplus_vooc_mg->bq27541_vbatt, oplus_vooc_mg->current_max, 
				(&(batt_sys_curv_by_tmprange->batt_sys_curve[idx]))->target_ibus, 
				(&(batt_sys_curv_by_tmprange->batt_sys_curve[idx]))->target_vbat);
			convert_ibus = batt_sys_curve->target_ibus * 100;
			if ((oplus_vooc_mg->bq27541_vbatt > batt_sys_curve->target_vbat			//switch by vol_thr
				&& oplus_vooc_mg->cp_ichg < convert_ibus + TARGET_VOL_OFFSET_THR)
				|| (batt_sys_curve->target_time > 0 && batt_sys_curve->chg_time >= batt_sys_curve->target_time)		//switch by chg time
				) {
				cc_cnt++;
				if (cc_cnt > 3) {
					if (batt_sys_curve->exit == false) {
						if (batt_sys_curve->chg_time >= batt_sys_curve->target_time) {
							sc_err("switch fastchg curv by chgtime[%d, %d]\n", batt_sys_curve->chg_time, batt_sys_curve->target_time);
						}
						batt_sys_curve->chg_time = 0;
						sc_err("!bf switch fastchg curv [%d %d %d]\n", oplus_vooc_mg->current_max, (&(batt_sys_curv_by_tmprange->batt_sys_curve[idx]))->target_ibus, (&(batt_sys_curv_by_tmprange->batt_sys_curve[idx]))->target_vbat);
						batt_sys_curve_next = &(batt_sys_curv_by_tmprange->batt_sys_curve[idx+1]);
						if(!batt_sys_curve_next) {
							sc_err("batt_sys_curve is a NULL pointer!!!!\n");
							return;
						}
						oplus_vooc_mg->cur_sys_curv_idx += 1;
						cc_cnt = 0;
						oplus_vooc_mg->current_max = oplus_vooc_mg->current_max > batt_sys_curve_next->target_ibus ? batt_sys_curve_next->target_ibus : oplus_vooc_mg->current_max;
						sc_err("!af switch fastchg curv [%d %d %d]\n", oplus_vooc_mg->current_max, batt_sys_curve_next->target_ibus, batt_sys_curve_next->target_vbat);
					}
				}
			} else {
				cc_cnt = 0;
			}
		} else {
			//exit fastchg
			sc_err("! exit fastchg\n");
		}
	}

	sc_err( "curv info [%d %d %d %d]\n", oplus_vooc_mg->bq27541_vbatt, oplus_vooc_mg->cp_ichg, oplus_vooc_mg->current_max, cc_cnt);
}


BattMngr_err_code_e vooc_vol_event_handle(unsigned long data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	static UINT8 fast_full_count = 0;

	if (oplus_vooc_mg->fastchg_monitor_stop == true) {
		sc_err( "vooc_vol_event_handle ignore");
		return status;
	}

	sc_err( "[vooc_vol_event_handle] vbatt: %d",oplus_vooc_mg->bq27541_vbatt);
	if ((oplus_vooc_mg->fastchg_notify_status & 0xFF) == FAST_NOTIFY_PRESENT)
		fast_full_count = 0;

	if (!oplus_vooc_mg->btb_temp_over) {
		voocphy_request_fastchg_curv();
		//set above calculate max current
		if (!oplus_vooc_mg->ap_need_change_current)
			oplus_vooc_mg->ap_need_change_current
				= oplus_vooc_set_fastchg_current(oplus_vooc_mg->current_max,
				oplus_vooc_mg->current_ap, oplus_vooc_mg->current_batt_temp);

		//notify full at some condition
		if ((oplus_vooc_mg->batt_temp_plugin == BATT_TEMP_LITTLE_COLD			//0-5 chg to 4430mV
			&& oplus_vooc_mg->bq27541_vbatt > TEMP0_2_TEMP45_FULL_VOLTAGE)
			|| (oplus_vooc_mg->batt_temp_plugin == BATT_TEMP_COOL				//5-12 chg to 4430mV
			&& oplus_vooc_mg->bq27541_vbatt > TEMP45_2_TEMP115_FULL_VOLTAGE)
			|| (oplus_vooc_mg->bq27541_vbatt > BAT_FULL_1TIME_THD)) {			//4510mV
			sc_err( "vbatt 1time fastchg full: %d",oplus_vooc_mg->bq27541_vbatt);
			oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_FULL);
		} else if (oplus_vooc_mg->bq27541_vbatt > BAT_FULL_NTIME_THD) {			//4480mV
			fast_full_count++;
			if (fast_full_count > 5) {
				sc_err( "vbatt ntime fastchg full: %d",oplus_vooc_mg->bq27541_vbatt);
				oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_FULL);
			}
		}

		status = vooc_monitor_timer_start(VOOC_THREAD_TIMER_VOL, VOOC_VOL_EVENT_TIME);
	}

	return status;
}

BattMngr_err_code_e vooc_safe_event_handle(unsigned long data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	if (oplus_vooc_mg->fastchg_monitor_stop == true) {
		sc_dbg( "vooc_safe_event_handle ignore");
		return status;
	}

	if (oplus_vooc_mg->fastchg_timeout_time)
		oplus_vooc_mg->fastchg_timeout_time--;
	if (oplus_vooc_mg->fastchg_3c_timeout_time)
		oplus_vooc_mg->fastchg_3c_timeout_time--;

	if (oplus_vooc_mg->fastchg_timeout_time == 0
		&& (!oplus_vooc_mg->btb_temp_over)) {
		oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_FULL);
	}

	if (oplus_vooc_mg->usb_bad_connect) {
		oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_BAD_CONNECTED);
		//reset vooc_phy
		oplus_vooc_reset_voocphy();
	}
	status = vooc_monitor_timer_start(VOOC_THREAD_TIMER_SAFE, VOOC_SAFE_EVENT_TIME);

	return status;
}



BattMngr_err_code_e vooc_discon_event_handle(unsigned long data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	oplus_vooc_mg->fastchg_start = false; //fastchg_start need be reset false;
	if (oplus_vooc_mg->fastchg_notify_status == FAST_NOTIFY_FULL || oplus_vooc_mg->fastchg_notify_status == FAST_NOTIFY_BAD_CONNECTED) {
		oplus_vooc_mg->fastchg_to_normal = true;
	} else if (oplus_vooc_mg->fastchg_notify_status == FAST_NOTIFY_BATT_TEMP_OVER) {
		oplus_vooc_mg->fastchg_to_warm = true;
	} else if (oplus_vooc_mg->fastchg_notify_status == FAST_NOTIFY_USER_EXIT_FASTCHG) {
	}

	sc_dbg ("!!![vooc_discon_event_handle] [%d %d %d]",
		oplus_vooc_mg->fastchg_to_normal, oplus_vooc_mg->fastchg_to_warm, oplus_vooc_mg->user_exit_fastchg);


	//code below moved to function oplus_vooc_handle_voocphy_status
	//timer of check charger out time should been started
	//status = vooc_monitor_timer_start(
	//		VOOC_THREAD_TIMER_CHG_OUT_CHECK, VOOC_CHG_OUT_CHECK_TIME);
	return status;
}

//extern 	void opchg_ui_soc_decimal_reset(void);
void oplus_reset_fastchg_after_usbout(void)
{
	struct oplus_chg_chip *chip = oplus_get_oplus_chip();
	int vbatt = 0;

	if (!chip) {
		sc_err ("chip is null");
		return;
	}

	if (oplus_vooc_mg->fastchg_start == false) {
		oplus_vooc_mg->fastchg_notify_status = FAST_NOTIFY_UNKNOW;
		sc_err ("fastchg_start");
	}

	vbatt = sc8547_get_cp_vbat();
	if (vbatt <= 4300 || fast_chg_full == false) {
		oplus_vooc_mg->fastchg_to_normal = false;
		oplus_vooc_mg->fastchg_to_warm = false;
		oplus_vooc_mg->fastchg_dummy_start = false;
		oplus_vooc_mg->fastchg_reactive = false;
		if (oplus_vooc_mg->user_set_exit_fastchg == false) {
			oplus_voocphy_set_user_exit_fastchg(false);
		}
		oplus_voocphy_set_fastchg_state(OPLUS_FASTCHG_STAGE_1);
		//opchg_ui_soc_decimal_reset();
	}

	chip->voocphy.fastchg_to_normal = oplus_vooc_mg->fastchg_to_normal;
	chip->voocphy.fastchg_to_warm = oplus_vooc_mg->fastchg_to_warm;
	chip->voocphy.fastchg_dummy_start = oplus_vooc_mg->fastchg_dummy_start;
	chip->voocphy.fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
	oplus_basetimer_monitor_stop();
	oplus_vooc_mg->fastchg_recovering = false;
	if (opchg_get_vooc_start_fg() == true) {
		opchg_voocphy_dump_reg();
	}
	voocphy_cpufreq_update(CPU_CHG_FREQ_STAT_AUTO);
	if (pm_qos_request_active(&pm_qos_req)) {
		pm_qos_update_request(&pm_qos_req, PM_QOS_DEFAULT_VALUE);
		sc_err("pm_qos_remove_request after usbout");
	}
	//update the vooc_status
	//pmic_glink_send_power_supply_notification(VOOC_STATUS_GET_REQ);
	//oplus_vooc_wake_notify_fastchg_work();	//lkl need modify
	sc_info("oplus_reset_fastchg_after_usbout");
}

static void oplus_check_chg_out_work_func(void);
BattMngr_err_code_e vooc_chg_out_check_event_handle(unsigned long data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	oplus_check_chg_out_work_func();

	return status;
}

extern void oplus_chg_clear_chargerid_info(void);
static void oplus_check_chg_out_work_func(void)
{
	INT32 chg_vol = 0;
	struct oplus_chg_chip *chip = oplus_get_oplus_chip();

	//porting check_charger_out_work_func && reset_fastchg_after_usbout from oplus_vooc.c
	chg_vol = oplus_chg_get_charger_voltage();
	if (chg_vol >= 0 && chg_vol < 2000) {
		if (oplus_vooc_mg->fastchg_start == false || chip->voocphy.fastchg_start == false) {
			oplus_vooc_mg->fastchg_notify_status = FAST_NOTIFY_UNKNOW;
		}
		oplus_vooc_mg->fastchg_to_normal = false;
		oplus_vooc_mg->fastchg_to_warm = false;
		oplus_vooc_mg->fastchg_dummy_start = false;
		oplus_vooc_mg->fastchg_reactive = false;
		oplus_voocphy_set_user_exit_fastchg(false);

		chip->voocphy.fastchg_to_normal = oplus_vooc_mg->fastchg_to_normal;
		chip->voocphy.fastchg_to_warm = oplus_vooc_mg->fastchg_to_warm;
		chip->voocphy.fastchg_dummy_start = oplus_vooc_mg->fastchg_dummy_start;

		oplus_chg_clear_chargerid_info();
		oplus_basetimer_monitor_stop();

		sc_err("chg_vol = %d %d %d\n", chg_vol, oplus_vooc_mg->fastchg_start, chip->voocphy.fastchg_start);
		oplus_chg_wake_update_work();
	}
	vooc_monitor_timer_stop(VOOC_THREAD_TIMER_CHG_OUT_CHECK, VOOC_CHG_OUT_CHECK_TIME);
	sc_err("notify chg work chg_vol = %d\n", chg_vol);
}


static void oplus_vooc_check_charger_out(void)
{
	sc_err("call\n");
	vooc_monitor_timer_start(VOOC_THREAD_TIMER_CHG_OUT_CHECK, VOOC_CHG_OUT_CHECK_TIME);
}

BOOLEAN oplus_vooc_check_fastchg_real_allow(void)
{
	UINT16 batt_soc = 0;
	INT32 batt_temp = 0;
	BOOLEAN ret = false;

	batt_soc = oplus_chg_get_ui_soc();			// get batt soc
	if (oplus_vooc_mg->batt_fake_soc) {
		batt_soc = oplus_vooc_mg->batt_fake_soc;
	}	
	//batt_soc /= 100;
	batt_temp = oplus_chg_get_chg_temperature(); //get batt temp
	if (oplus_vooc_mg->batt_fake_temp) {
		batt_temp = oplus_vooc_mg->batt_fake_temp;
	}


	if (batt_soc >= VOOC_LOW_SOC && batt_soc <= VOOC_HIGH_SOC
			&& batt_temp >= VOOC_LOW_TEMP && batt_temp < VOOC_HIGH_TEMP)
		ret = true;
	else
		ret = false;

	sc_dbg("!!![oplus_vooc_check_fastchg_real_allow] \
						ret:%d, temp:%d, soc:%d", ret, batt_temp, batt_soc);
	return ret;
}
bool voocphy_get_real_fastchg_allow(void)
{
	if(!oplus_vooc_mg) {
		return false;
	} else {
		oplus_vooc_mg->fastchg_real_allow = oplus_vooc_check_fastchg_real_allow();
		return oplus_vooc_mg->fastchg_real_allow;
	}
}



extern bool oplus_voocphy_get_fastchg_to_warm(void);
BattMngr_err_code_e vooc_fastchg_check_process_handle(unsigned long enable)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	BOOLEAN bPlugIn = false;

#if 0
	status = BattMngr_WaitLockAcquire(&(oplus_vooc_mg->voocphy_process_wait_lock), NULL);
	if (status != BATTMNGR_SUCCESS){
		sc_dbg( "get process wait lock fail");
		return status;
	}
#endif

	bPlugIn = oplus_chg_stats();
	oplus_vooc_mg->fastchg_real_allow = oplus_vooc_check_fastchg_real_allow();

	sc_err( "fastchg_to_warm[%d %d] fastchg_dummy_start[%d] fastchg_err_commu[%d] fastchg_check_stop[%d] enable[%d] bPlugIn[%d] fastchg_real_allow[%d]\n",
		oplus_vooc_mg->fastchg_to_warm, oplus_voocphy_get_fastchg_to_warm(), oplus_vooc_mg->fastchg_dummy_start, oplus_vooc_mg->fastchg_err_commu,
		oplus_vooc_mg->fastchg_check_stop, enable, bPlugIn, oplus_vooc_mg->fastchg_real_allow);
	if ((oplus_vooc_mg->fastchg_to_warm || oplus_voocphy_get_fastchg_to_warm()
		|| oplus_vooc_mg->fastchg_dummy_start
		|| oplus_vooc_mg->fastchg_err_commu)
		&& (!oplus_vooc_mg->fastchg_check_stop)
		&& enable && bPlugIn) {
		if (oplus_vooc_mg->fastchg_real_allow == true) {
			/*
			if (oplus_vooc_mg->fastchg_to_warm == false) {
				oplus_vooc_mg->fastchg_reactive = true;
			}
			oplus_vooc_variables_reset();
			status = oplus_vooc_reactive_voocphy(); //re_active voocphy
			*/
		} else {
			sc_err( "[vooc_monitor] VOOC_THREAD_TIMER_FASTCHG_CHECK start for self fastchg check time start");
			status = vooc_monitor_timer_start(VOOC_THREAD_TIMER_FASTCHG_CHECK, VOOC_FASTCHG_CHECK_TIME);
		}
	} else {
		oplus_vooc_mg->fastchg_check_stop = true;
		status = vooc_monitor_timer_stop(VOOC_THREAD_TIMER_FASTCHG_CHECK, VOOC_FASTCHG_CHECK_TIME);
		sc_err( "[vooc_monitor] VOOC_THREAD_TIMER_FASTCHG_CHECK stop fastchg check time stop");
	}

//FASTCHG_CHECK_ERR:
	//BattMngr_WaitLockRelease(&(oplus_vooc_mg->voocphy_process_wait_lock));
	return status;
}


BattMngr_err_code_e vooc_commu_process_handle(unsigned long enable)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	//status = BattMngr_WaitLockAcquire(
	//		&(oplus_vooc_mg->voocphy_commu_wait_lock), NULL);
	if (status != BATTMNGR_SUCCESS){
		sc_dbg("get commu wait lock fail");
		return status;
	}

	if (enable && oplus_vooc_mg->fastchg_commu_stop == false) {
		sc_err("[vooc_monitor] commu timeout");
		oplus_vooc_mg->rcv_done_200ms_timeout_num++;
		oplus_vooc_mg->fastchg_start = false;
		oplus_reset_fastchg_after_usbout();
		status = vooc_monitor_stop(0);
		status |= oplus_vooc_reset_voocphy();
		oplus_chg_set_charger_type_unknown();
	} else {
		oplus_vooc_mg->fastchg_commu_stop = true;
		status = vooc_monitor_timer_stop(VOOC_THREAD_TIMER_COMMU, VOOC_COMMU_EVENT_TIME);
		sc_err("[vooc_monitor] commu time stop [%d]", oplus_vooc_mg->fastchg_notify_status);
	}
	//BattMngr_WaitLockRelease(&(oplus_vooc_mg->voocphy_commu_wait_lock));

	return status;
}

static void sc8547_set_pd_svooc_config(bool enable);
static bool sc8547_get_pd_svooc_config(void);

BattMngr_err_code_e vooc_curr_event_handle(unsigned long data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	INT32 vbat_temp_cur = 0;
	static UINT8 curr_over_count = 0;
	static UINT8 chg_curr_over_count = 0;
	static UINT8 term_curr_over_count = 0;
	static UINT8 curr_curve_pwd_count = 10;
	INT32 chg_temp = oplus_chg_get_chg_temperature();
	bool pd_svooc_status;

	if (oplus_vooc_mg->fastchg_monitor_stop == true) {
		sc_err( "vooc_curr_event_handle ignore");
		return status;
	}

	if ((oplus_vooc_mg->fastchg_notify_status & 0XFF) == FAST_NOTIFY_PRESENT) {
		curr_over_count = 0;
		chg_curr_over_count = 0;
		term_curr_over_count = 0;
		curr_curve_pwd_count = 10;
	}

	vbat_temp_cur = oplus_vooc_mg->icharging;

	sc_err( "[vooc_curr_event_handle] chg_temp: %d, current: %d, vbatt: %d btb: %d current_pwd[%d %d]",
			chg_temp, vbat_temp_cur, oplus_vooc_mg->bq27541_vbatt, oplus_vooc_mg->btb_temp_over,
			oplus_vooc_mg->curr_pwd_count, oplus_vooc_mg->current_pwd);

	pd_svooc_status = sc8547_get_pd_svooc_config();
	if (oplus_chg_check_pd_svooc_adapater()) {
		sc_err("pd_svooc_status = %d, oplus_vooc_mg->cp_ichg = %d\n", pd_svooc_status, oplus_vooc_mg->cp_ichg);
		if (pd_svooc_status == true && oplus_vooc_mg->cp_ichg > 1000) {
			sc_err("IBUS > 1A set 0x5 to 0x28!\n");
			sc8547_set_pd_svooc_config(false);
		}
	}

	if (!oplus_vooc_mg->btb_temp_over) {//non btb temp over
		if (vbat_temp_cur < -2000) { 	//BATT OUPUT 2000MA
			curr_over_count++;
			if (curr_over_count > 3) {
				sc_info("vcurr low than -2000mA\n");
				oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_ABSENT);
			}
		} else {
			curr_over_count = 0;
		}

		if (vbat_temp_cur > VOOC_OVER_CURRENT_VALUE) {
			chg_curr_over_count++;
			if (chg_curr_over_count > 7) {
				oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_BAD_CONNECTED);
			}
		} else {
			chg_curr_over_count = 0;
		}

		if (chg_temp >= LOW_CURR_FULL_T1 && chg_temp <= LOW_CURR_FULL_T2) {		/*12 - 25C*/
			if ((vbat_temp_cur <= RANGE1_LOW_CURR1 && oplus_vooc_mg->bq27541_vbatt >= RANGE1_FULL_VBAT1)
				|| (vbat_temp_cur <= RANGE1_LOW_CURR2 && oplus_vooc_mg->bq27541_vbatt >= RANGE1_FULL_VBAT2)
				|| (vbat_temp_cur <= RANGE1_LOW_CURR3 && oplus_vooc_mg->bq27541_vbatt >= RANGE1_FULL_VBAT3)
				|| (vbat_temp_cur <= RANGE1_LOW_CURR4 && oplus_vooc_mg->bq27541_vbatt >= RANGE1_FULL_VBAT4)) {
				term_curr_over_count++;
				if (term_curr_over_count > 5) {
					sc_err( "range1 lowcurr ntime fastchg full %d, ",oplus_vooc_mg->bq27541_vbatt, vbat_temp_cur);
					oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_FULL);
				}
			} else {
				term_curr_over_count = 0;
			}

		} else if (chg_temp > LOW_CURR_FULL_T2 && chg_temp <= LOW_CURR_FULL_T3){	/*25 -43C*/
			if ((vbat_temp_cur <= RANGE2_LOW_CURR1 && oplus_vooc_mg->bq27541_vbatt >= RANGE2_FULL_VBAT1)
				|| (vbat_temp_cur <= RANGE2_LOW_CURR2 && oplus_vooc_mg->bq27541_vbatt >= RANGE2_FULL_VBAT2)
				|| (vbat_temp_cur <= RANGE2_LOW_CURR3 && oplus_vooc_mg->bq27541_vbatt >= RANGE2_FULL_VBAT3)
				|| (vbat_temp_cur <= RANGE2_LOW_CURR4 && oplus_vooc_mg->bq27541_vbatt >= RANGE2_FULL_VBAT4)) {
				term_curr_over_count++;
				if (term_curr_over_count > 5) {
					sc_err( "range2 lowcurr ntime fastchg full %d, ",oplus_vooc_mg->bq27541_vbatt, vbat_temp_cur);
					oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_FULL);
				}
			} else {
				term_curr_over_count = 0;
			}
		} else {
			term_curr_over_count = 0;
		}

		if (vbat_temp_cur/2 > oplus_vooc_mg->current_pwd && oplus_vooc_mg->adapter_type == ADAPTER_SVOOC) {
			if (oplus_vooc_mg->curr_pwd_count) {
				oplus_vooc_mg->curr_pwd_count--;
			} else {
				sc_err("FAST_NOTIFY_ADAPTER_COPYCAT for over current");
				//oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_ADAPTER_COPYCAT);
			}
		}
		if ((oplus_vooc_mg->bq27541_vbatt > BATT_PWD_VOL_THD1)
			&& (vbat_temp_cur > BATT_PWD_CURR_THD1)) {
			curr_curve_pwd_count--;
			if (curr_curve_pwd_count == 0) {
				sc_err("FAST_NOTIFY_ADAPTER_COPYCAT for over vbatt && ibat[%d %d]\n", BATT_PWD_VOL_THD1, BATT_PWD_CURR_THD1);
				//oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_ADAPTER_COPYCAT);
			}
		} else {
			curr_curve_pwd_count = 10;
		}

		status = vooc_monitor_timer_start(VOOC_THREAD_TIMER_CURR, VOOC_CURR_EVENT_TIME);

	}

	return status;
}



BattMngr_err_code_e vooc_monitor_start(unsigned long data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;
	if (!oplus_vooc_mg) {
		sc_err( "oplus_vooc_mg is null");
		return BATTMNGR_EFATAL;
	}

	oplus_vooc_mg->fastchg_monitor_stop = false;
	vooc_monitor_timer_start(VOOC_THREAD_TIMER_SAFE, VOOC_SAFE_EVENT_TIME);
	//vooc_monitor_timer_start(VOOC_THREAD_TIMER_BTB, VOOC_BTB_EVENT_TIME);
	vooc_monitor_timer_start(VOOC_THREAD_TIMER_VOL, VOOC_VOL_EVENT_TIME);
	vooc_monitor_timer_start(VOOC_THREAD_TIMER_TEMP, VOOC_TEMP_EVENT_TIME);
	//vooc_monitor_timer_start(VOOC_THREAD_TIMER_SOC, VOOC_SOC_EVENT_TIME);
	vooc_monitor_timer_start(VOOC_THREAD_TIMER_CURR, VOOC_CURR_EVENT_TIME);
	vooc_monitor_timer_start(VOOC_THREAD_TIMER_AP, VOOC_AP_EVENT_TIME);

	sc_dbg( "vooc_monitor hrstart success");
	return status;
}

BattMngr_err_code_e vooc_monitor_stop(unsigned long data)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	vooc_monitor_timer_stop(VOOC_THREAD_TIMER_AP, VOOC_AP_EVENT_TIME);
	vooc_monitor_timer_stop(VOOC_THREAD_TIMER_CURR, VOOC_CURR_EVENT_TIME);
	//vooc_monitor_timer_stop(VOOC_THREAD_TIMER_SOC, VOOC_SOC_EVENT_TIME);
	vooc_monitor_timer_stop(VOOC_THREAD_TIMER_VOL, VOOC_VOL_EVENT_TIME);
	vooc_monitor_timer_stop(VOOC_THREAD_TIMER_TEMP, VOOC_TEMP_EVENT_TIME);
	//vooc_monitor_timer_stop(VOOC_THREAD_TIMER_BTB, VOOC_BTB_EVENT_TIME);
	vooc_monitor_timer_stop(VOOC_THREAD_TIMER_SAFE, VOOC_SAFE_EVENT_TIME);

	oplus_vooc_mg->fastchg_monitor_stop = true;
	sc_dbg( "vooc_monitor stop hrtimer success");
	return status;
}

void oplus_vooc_vooc_monitor_stop(void)
{
	vooc_monitor_stop(0);
}

BattMngr_err_code_e oplus_vooc_monitor_init(void)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	/* Create the vooc_monitor thread */
	vooc_monitor_timer_init();

	//timer create finished and triggered by timeout

	return status;
}


void voocphy_service(struct work_struct *work)
{
	if (!oplus_vooc_mg) {
		printk(KERN_ERR "[%s] oplus_vooc_mg null\n", __func__);
		return ;
	}

	oplus_vooc_mg->icharging = -oplus_gauge_get_batt_current();
	oplus_vooc_print_dbg_info();
}

bool oplus_vooc_wake_voocphy_service_work(VOOCPHY_REQUEST_TYPE request)
{
	oplus_vooc_mg->voocphy_request= request;
	schedule_delayed_work(&oplus_vooc_mg->voocphy_service_work, 0);
	sc_dbg("request %d \n", request);
	return true;
}

bool oplus_vooc_wake_monitor_work(void)
{
	schedule_delayed_work(&oplus_vooc_mg->monitor_work, 0);
	return true;
}

void oplus_voocphy_wake_modify_cpufeq_work(int flag)
{
	sc_dbg("%s %s\n", __func__, flag == CPU_CHG_FREQ_STAT_UP ?"request":"release");
	schedule_delayed_work(&oplus_vooc_mg->modify_cpufeq_work, 0);
}

void oplus_voocphy_modify_cpufeq_work(struct work_struct *work)
{
	if (atomic_read(&oplus_vooc_mg->voocphy_freq_state) == 1)
		ppm_sys_boost_min_cpu_freq_set(oplus_vooc_mg->voocphy_freq_mincore,
			oplus_vooc_mg->voocphy_freq_midcore,
			oplus_vooc_mg->voocphy_freq_maxcore,
			oplus_vooc_mg->voocphy_current_change_timeout);
	else
		ppm_sys_boost_min_cpu_freq_clear();
}

BattMngr_err_code_e oplus_vooc_init_res(UINT8 pmic_index)
{
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	//request oplus_vooc_mg memery
	oplus_vooc_mg = (vooc_manager* )kzalloc(sizeof(vooc_manager), GFP_KERNEL);
	if(oplus_vooc_mg == NULL)
	{
		sc_dbg("oplus_vooc_mg init fail");
		status = BATTMNGR_EMEM;
		goto EndError;
    }

	//oplus_vooc_variables_init
	status = oplus_vooc_variables_init();

	//oplus_vooc_monitor init
	status |= oplus_vooc_monitor_init();

	INIT_DELAYED_WORK(&(oplus_vooc_mg->voocphy_service_work), voocphy_service);
	INIT_DELAYED_WORK(&(oplus_vooc_mg->notify_fastchg_work), oplus_vooc_handle_voocphy_status);
	INIT_DELAYED_WORK(&(oplus_vooc_mg->monitor_work), vooc_monitor_process_events);
	INIT_DELAYED_WORK(&(oplus_vooc_mg->modify_cpufeq_work), oplus_voocphy_modify_cpufeq_work);
	atomic_set(&oplus_vooc_mg->voocphy_freq_state, 0);
EndError:
	return status;
}

/*
 * interrupt does nothing, just info event chagne, other module could get info
 * through power supply interface
 */
//extern int oplus_fastchg_switch_done(void);
#define AP_ALLOW_FASTCHG	(1 << 6)
bool oplus_vooc_ap_allow_fastchg(void)
{
	bool ap_allow_fastchg = false;
	if (oplus_vooc_mg->fastchg_adapter_ask_cmd == VOOC_CMD_ASK_FASTCHG_ORNOT) {
		ap_allow_fastchg = (oplus_vooc_mg->voocphy_tx_buff[0] & AP_ALLOW_FASTCHG);
		if (ap_allow_fastchg) {
			sc_info("fastchg_stage OPLUS_FASTCHG_STAGE_2\n");
			oplus_voocphy_set_fastchg_state(OPLUS_FASTCHG_STAGE_2);
		}
		return ap_allow_fastchg;
	}
	return false;
}

extern	void oplus_chg_disable_charge(void);

int oplus_is_vbus_ok_predata (void)
{
	if (oplus_vooc_mg->adapter_type == ADAPTER_SVOOC) {
		if (oplus_vooc_mg->ask_bat_model_finished) {
			oplus_vooc_mg->ask_bat_model_finished = false;
			oplus_voocphy_set_predata(OPLUS_IS_VUBS_OK_PREDATA_SVOOC);
		}
	} else if (oplus_vooc_mg->adapter_type == ADAPTER_VOOC30) {
		if (oplus_vooc_mg->ask_bat_model_finished) {
			oplus_vooc_mg->ask_bat_model_finished = false;
			oplus_voocphy_set_predata(OPLUS_IS_VUBS_OK_PREDATA_VOOC30);
		}
	} else if (oplus_vooc_mg->adapter_type == ADAPTER_VOOC20) {
		oplus_voocphy_set_predata(OPLUS_IS_VUBS_OK_PREDATA_VOOC20);
	} else {
		sc_err("adapter_type error !!!\n");
		return -1;
	}
	return 0;
}

extern bool oplus_chg_get_flash_led_status(void);
extern int oplus_mt6370_charging_current_write_fast(int chg_curr);
#ifdef CONFIG_TCPC_CLASS
#ifdef CONFIG_USB_PD_DIRECT_CHARGE
extern void mt_tcpm_set_direct_charge_en(bool en);
#endif
#endif

#define PD_COUNT_OVER		50
static irqreturn_t oplus_charger_interrupt(int irq, void *dev_id)
{
	struct sc8547 *sc = dev_id;
	int ret = 0;
	static bool pd_reset = 1;
	static int pd_reset_count = 0;

	u8 value1;
	BattMngr_err_code_e status = BATTMNGR_SUCCESS;

	//start time
	calltime = ktime_get();

	if (!sc)
		return IRQ_HANDLED;

	oplus_vooc_mg->irq_total_num++;

	//for flash led
	if (oplus_vooc_mg->fastchg_need_reset) {
		sc_info("fastchg_need_reset%d\n");
		oplus_vooc_mg->fastchg_need_reset = 0;
		oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_USER_EXIT_FASTCHG);
		goto handle_done;
	}

	//__pm_stay_awake(&__sc->ws);		//lkl porting
	oplus_voocphy_get_adapter_request_info();

	if (VOOC_CMD_ASK_CURRENT_LEVEL == oplus_vooc_mg->pre_adapter_ask_cmd && ADJ_CUR_STEP_SET_CURR_DONE == oplus_vooc_mg->adjust_curr) {
		if (oplus_write_txbuff_error()) {
			oplus_vooc_mg->ap_need_change_current = 5;
			oplus_vooc_mg->adjust_fail_cnt++;
			sc_err("adjust curr wtxbuff fail %d times\n", oplus_vooc_mg->adjust_fail_cnt);
		} else {
			voocphy_cpufreq_update(CPU_CHG_FREQ_STAT_AUTO);
			//trace_oplus_tp_sched_change_ux(1, task_cpu(current));
			//trace_oplus_tp_sched_change_ux(0, task_cpu(current));
			sc_err("adjust cpu to default");
			oplus_vooc_mg->adjust_fail_cnt = 0;
			oplus_vooc_mg->adjust_curr = ADJ_CUR_STEP_DEFAULT;
		}
	}
	if (__sc->vooc_flag & TXDATA_WR_FAIL_FLAG_MASK) {
		oplus_vooc_mg->ap_handle_timeout_num++;
	}

	if (__sc->vooc_flag & RXDATA_DONE_FLAG_MASK) {
		//feed soft monitor watchdog
		if (!oplus_vooc_mg->fastchg_commu_stop) {
			status = vooc_monitor_timer_stop(VOOC_THREAD_TIMER_COMMU, VOOC_COMMU_EVENT_TIME);
			if (status != BATTMNGR_SUCCESS) {
				sc_err("stop commu timer fail");
			}
			status = vooc_monitor_timer_start(VOOC_THREAD_TIMER_COMMU,VOOC_COMMU_EVENT_TIME);
			if (status != BATTMNGR_SUCCESS) {
				sc_err("restart commu timer fail");
			}
		}
	}

	if (__sc->vooc_flag == 0xF) {
		//do nothing
		sc_err("!RX_START_FLAG & TXDATA_WR_FAIL_FLAG occured do nothing");
	} else if (__sc->vooc_flag & RXDATA_DONE_FLAG_MASK) {	//rxdata recv done
		oplus_vooc_mg->irq_rcvok_num++;
		oplus_vooc_mg->voocphy_rx_buff = __sc->vooc_rxdata;
		oplus_vooc_adapter_commu_with_voocphy(0);

		/*When the adapter cannot boost the voltage normally, reset the adapter with a delay of 200ms*/
		if (oplus_vooc_mg->fastchg_adapter_ask_cmd == 0x2 && pd_reset) {
			if (oplus_vooc_mg->cp_vbus < 1000 && pd_reset_count++ > PD_COUNT_OVER) {
				pd_reset = 0;
				msleep(200);
				chg_err("pd_reset_over");
			}
		} else {
			pd_reset_count = 0;
		}

		if (oplus_vooc_ap_allow_fastchg()) {
			if (oplus_vooc_mg->fastchg_stage == OPLUS_FASTCHG_STAGE_2 && oplus_chg_get_flash_led_status()) {
				//oplus_vooc_set_status_and_notify_ap(FAST_NOTIFY_USER_EXIT_FASTCHG);
				oplus_vooc_mg->fastchg_need_reset = 1;
				sc_info("OPLUS_FASTCHG_STAGE_2 and open torch exit fastchg\n");
				goto handle_done;
			} else {
				/*oplus_chg_disable_charge();
				ret = oplus_mt6370_charging_current_write_fast(0);
				if (ret < 0) {
					sc_err("%s : oplus_mt6370_charging_current_write_fast fail\n", __func__);
				}*/
				mt6370_suspend_charger(true);
				sc_err("allow fastchg adapter type %d\n", oplus_vooc_mg->adapter_type);
			}

			//handle timeout of adapter ask cmd 0x4
			oplus_is_vbus_ok_predata();

			if (oplus_vooc_mg->adapter_type == ADAPTER_SVOOC) {
				oplus_voocphy_hw_setting(SETTING_REASON_SVOOC);
				if (oplus_chg_check_pd_svooc_adapater()) {
					sc_err("pd_svooc adapter, set config!\n");
					sc8547_set_pd_svooc_config(true);
				}
			} else if (oplus_vooc_mg->adapter_type == ADAPTER_VOOC20 || oplus_vooc_mg->adapter_type == ADAPTER_VOOC30){
				oplus_voocphy_hw_setting(SETTING_REASON_VOOC);
#ifdef CONFIG_TCPC_CLASS
#ifdef CONFIG_USB_PD_DIRECT_CHARGE
				mt_tcpm_set_direct_charge_en(true);
#endif
#endif
			} else {
				sc_err("SETTING_REASON_DEFAULT\n");
			}
		}
		oplus_vooc_mg->pre_adapter_ask_cmd = oplus_vooc_mg->fastchg_adapter_ask_cmd;
		oplus_voocphy_update_chg_data();
	}

	if (__sc->vooc_flag & RXDATA_DONE_FLAG_MASK) {
		oplus_vooc_mg->irq_hw_timeout_num++;
	}

	if (__sc->vooc_flag & RXDATA_DONE_FLAG_MASK) {
		/*move to sc8547_update_chg_data for improving time of interrupt*/
	} else {
		oplus_voocphy_pm_qos_update(400);
		sc8547_read_byte(sc, SC8547_REG_0F, &value1);
		oplus_vooc_mg->int_flag = value1;
	}
	sc_info("adapter_ask = 0x%0x, 0x%0x int_flag 0x%0x 0x%0x\n",
		oplus_vooc_mg->pre_adapter_ask_cmd, oplus_vooc_mg->fastchg_adapter_ask_cmd,
		oplus_vooc_mg->int_flag, oplus_vooc_mg->vooc_flag);
	//__pm_relax(&__sc->ws);	//lkl porting

	oplus_vooc_wake_voocphy_service_work(VOOCPHY_REQUEST_UPDATE_DATA);

handle_done:
	return IRQ_HANDLED;
}

int sc8547_init_vooc(void)
{
	u8 value;
	sc_err(" >>>>start init vooc\n");
	//sc8547_reg_reset(__sc, true);

	sc8547_reg_reset(__sc, true);
	sc8547_init_device(__sc);

	oplus_vooc_reset_variables();
	vooc_monitor_stop(0);
	vooc_monitor_timer_stop(VOOC_THREAD_TIMER_COMMU, VOOC_COMMU_EVENT_TIME);

	//to avoid cmd of adjust current(0x01)return error, add voocphy bit0 hold time to 800us
	oplus_voocphy_set_predata(0x0);
	sc8547_read_byte(__sc, SC8547_REG_3A, &value);
	value = value | (3 << 5);
	sc8547_write_byte(__sc, SC8547_REG_3A, value);
	//sc8547_read_byte(__sc, SC8547_REG_3A, &value);
	sc_err("read value %d\n", value);

	//dpdm
	sc8547_write_byte(__sc, SC8547_REG_21, 0x21);
	sc8547_write_byte(__sc, SC8547_REG_22, 0x00);
	sc8547_write_byte(__sc, SC8547_REG_33, 0xD1);

	//vooc
	sc8547_write_byte(__sc, SC8547_REG_30, 0x05);

	if (oplus_vooc_mg) {
		oplus_vooc_mg->batt_soc = oplus_chg_get_ui_soc();
		if (oplus_vooc_mg->batt_fake_soc) {
			oplus_vooc_mg->batt_soc = oplus_vooc_mg->batt_fake_soc;
		}
		oplus_vooc_mg->plug_in_batt_temp = oplus_chg_get_chg_temperature();
		if (oplus_vooc_mg->batt_fake_temp) {
			oplus_vooc_mg->plug_in_batt_temp = oplus_vooc_mg->batt_fake_temp;
		}

		if (oplus_vooc_mg->batt_soc >= VOOC_LOW_SOC
				&& oplus_vooc_mg->batt_soc <= VOOC_HIGH_SOC
				&& oplus_vooc_mg->plug_in_batt_temp >= VOOC_LOW_TEMP
				&& oplus_vooc_mg->plug_in_batt_temp <= VOOC_HIGH_TEMP) {
			oplus_vooc_mg->fastchg_real_allow = true;
		} else {
			oplus_vooc_mg->fastchg_real_allow = false;
		}

		oplus_vooc_mg->ask_current_first = true;
		oplus_vooc_mg->batt_sys_curv_found == false;
		oplus_get_soc_and_temp_with_enter_fastchg();

		sc_err("batt_soc %d, plug_in_batt_temp = %d\n", oplus_vooc_mg->batt_soc, oplus_vooc_mg->plug_in_batt_temp);
	}
	return 0;
}

static int sc8547_parse_dt(struct sc8547 *sc, struct device *dev)
{
	int ret;
	struct device_node *np = dev->of_node;

	sc->cfg = devm_kzalloc(dev, sizeof(struct sc8547_cfg),
					GFP_KERNEL);

	if (!sc->cfg)
		return -ENOMEM;

	sc->cfg->bat_ovp_disable = of_property_read_bool(np,
			"sc,sc8547,bat-ovp-disable");
	sc->cfg->vdrop_ovp_disable = of_property_read_bool(np,
			"sc,sc8547,vdrop-ovp-disable");
	sc->cfg->bus_ovp_disable = of_property_read_bool(np,
			"sc,sc8547,bus-ovp-disable");
	sc->cfg->bus_ucp_disable = of_property_read_bool(np,
			"sc,sc8547,bus-ucp-disable");
	sc->cfg->bus_ocp_disable = of_property_read_bool(np,
			"sc,sc8547,bus-ocp-disable");

	ret = of_property_read_u32(np, "sc,sc8547,bat-ovp-threshold",
			&sc->cfg->bat_ovp_th);
	if (ret) {
		sc_dbg("failed to read bat-ovp-threshold\n");
		return ret;
	}
	ret = of_property_read_u32(np, "sc,sc8547,bat-ocp-threshold",
			&sc->cfg->bat_ocp_th);
	if (ret) {
		sc_dbg("failed to read bat-ocp-threshold\n");
		return ret;
	}
	ret = of_property_read_u32(np, "sc,sc8547,ac-ovp-threshold",
			&sc->cfg->ac_ovp_th);
	if (ret) {
		sc_dbg("failed to read ac-ovp-threshold\n");
		return ret;
	}
	ret = of_property_read_u32(np, "sc,sc8547,bus-ovp-threshold",
			&sc->cfg->bus_ovp_th);
	if (ret) {
		sc_dbg("failed to read bus-ovp-threshold\n");
		return ret;
	}
	ret = of_property_read_u32(np, "sc,sc8547,bus-ocp-threshold",
			&sc->cfg->bus_ocp_th);
	if (ret) {
		sc_dbg("failed to read bus-ocp-threshold\n");
		return ret;
	}

	ret = of_property_read_u32(np, "sc,sc8547,sense-resistor-mohm",
			&sc->cfg->sense_r_mohm);
	if (ret) {
		sc_dbg("failed to read sense-resistor-mohm\n");
		return ret;
	}
	ret = of_property_read_u32(np, "qcom,voocphy_freq_mincore",
	                          &oplus_vooc_mg->voocphy_freq_mincore);
	if (ret) {
		oplus_vooc_mg->voocphy_freq_mincore = 975000;
	}
	sc_err("voocphy_freq_mincore is %d\n",&oplus_vooc_mg->voocphy_freq_mincore);

	ret = of_property_read_u32(np, "qcom,voocphy_freq_midcore",
	                          &oplus_vooc_mg->voocphy_freq_midcore);
	if (ret) {
		oplus_vooc_mg->voocphy_freq_midcore = 1275000;
	}
	sc_err("voocphy_freq_midcore is %d\n", &oplus_vooc_mg->voocphy_freq_midcore);

	ret = of_property_read_u32(np, "qcom,voocphy_freq_maxcore",
	                          &oplus_vooc_mg->voocphy_freq_maxcore);
	if (ret) {
		oplus_vooc_mg->voocphy_freq_maxcore = 1375000;
	}
	sc_err("voocphy_freq_maxcore is %d\n", &oplus_vooc_mg->voocphy_freq_maxcore);

	ret = of_property_read_u32(np, "qcom,voocphy_current_change_timeout",
	                          &oplus_vooc_mg->voocphy_current_change_timeout);
	if (ret) {
		oplus_vooc_mg->voocphy_current_change_timeout = 100;
	}
	sc_err("voocphy_current_change_timeout is %d\n", &oplus_vooc_mg->voocphy_current_change_timeout);
	return 0;
}

int oplus_irq_gpio_init(void)
{
	int rc;
	struct device_node *node = NULL;
	vooc_manager *sc = oplus_vooc_mg;

	if (!sc) {
		return -1;
	}

	node = sc->dev->of_node;

	if (!node) {
		sc_dbg("device tree node missing\n");
		return -EINVAL;
	}
	//irq_gpio
	sc->irq_gpio = of_get_named_gpio(node,
		"qcom,switch_voocphy_irq_gpio", 0);
	if (sc->irq_gpio < 0) {
		sc_dbg("sc->irq_gpio not specified\n");
	} else {
		if (gpio_is_valid(sc->irq_gpio)) {
			rc = gpio_request(sc->irq_gpio,
				"switch_voocphy_irq_gpio");
			if (rc) {
				sc_dbg("unable to request gpio [%d]\n",
					sc->irq_gpio);
			}
		}
		sc_dbg("sc->irq_gpio =%d\n", sc->irq_gpio);
	}

	//irq_num
	sc->irq = gpio_to_irq(sc->irq_gpio);
	sc_dbg("irq way1 sc->irq =%d\n",sc->irq);

	sc->irq = irq_of_parse_and_map(node, 0);
	sc_dbg("irq way2 sc->irq =%d\n",sc->irq);


	/* set voocphy pinctrl*/
	sc->pinctrl = devm_pinctrl_get(sc->dev);
	if (IS_ERR_OR_NULL(sc->pinctrl)) {
		chg_err("get pinctrl fail\n");
		return -EINVAL;
	}

	sc->charging_inter_active =
		pinctrl_lookup_state(sc->pinctrl, "charging_inter_active");
	if (IS_ERR_OR_NULL(sc->charging_inter_active)) {
		chg_err(": %d Failed to get the state pinctrl handle\n", __LINE__);
		return -EINVAL;
	}

	sc->charging_inter_sleep =
		pinctrl_lookup_state(sc->pinctrl, "charging_inter_sleep");
	if (IS_ERR_OR_NULL(sc->charging_inter_sleep)) {
		chg_err(": %d Failed to get the state pinctrl handle\n", __LINE__);
		return -EINVAL;
	}

	//irq active
	gpio_direction_input(sc->irq_gpio);
	pinctrl_select_state(sc->pinctrl, sc->charging_inter_active); /* no_PULL */

	rc = gpio_get_value(sc->irq_gpio);
	sc_dbg("irq sc->irq_gpio input =%d irq_gpio_stat = %d\n",sc->irq_gpio, rc);

	return 0;
}

static int sc8547_irq_register(struct sc8547 *sc)
{
	int ret;

	oplus_irq_gpio_init();
	if (oplus_vooc_mg->irq) {
        ret = request_threaded_irq(oplus_vooc_mg->irq, NULL,
                                   oplus_charger_interrupt,
                                   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                   "sc8547_charger_irq", sc);
		if (ret < 0) {
			sc_dbg("request irq for irq=%d failed, ret =%d\n",
							oplus_vooc_mg->irq, ret);
			return ret;
		}
		enable_irq_wake(oplus_vooc_mg->irq);
	}
	sc_dbg("request irq ok\n");

	return ret;
}

static void sc8547_set_pd_svooc_config(bool enable)
{
	int ret = 0;
	u8 reg05_data = 0;
	if (!__sc) {
		pr_err("Failed\n");
		return;
	}

	if (enable) {
		sc8547_write_byte(__sc, SC8547_REG_05, 0xA8);
		sc8547_write_byte(__sc, SC8547_REG_09, 0x13);
	} else
		sc8547_write_byte(__sc, SC8547_REG_05, 0x28);

	ret = sc8547_read_byte(__sc, SC8547_REG_05, &reg05_data);
	if (ret < 0) {
		pr_err("SC8547_REG_05\n");
		return;
	}
	pr_err("pd_svooc config SC8547_REG_05 = %d\n", reg05_data);
}

static bool sc8547_get_pd_svooc_config(void)
{
	int ret = 0;
	u8 data = 0;

	if (!__sc) {
		pr_err("Failed\n");
		return false;
	}

	ret = sc8547_read_byte(__sc, SC8547_REG_05, &data);
	if (ret < 0) {
		pr_err("SC8547_REG_05\n");
		return false;
	}

	pr_err("SC8547_REG_05 = 0x%0x\n", data);

	data = data >> 7;
	if (data == 1)
		return true;
	else
		return false;
}

static int sc8547_init_device(struct sc8547 *sc)
{
	sc8547_write_byte(sc, SC8547_REG_11, 0x0);		//ADC_CTRL:disable
	sc8547_write_byte(__sc, SC8547_REG_02, 0x7);	//
	sc8547_write_byte(__sc, SC8547_REG_04, 0x00);	//VBUS_OVP:10 2:1 or 1:1V
	sc8547_write_byte(__sc, SC8547_REG_00, 0x2E);	//VBAT_OVP:4.65V
	sc8547_write_byte(__sc, SC8547_REG_05, 0x28);	//IBUS_OCP_UCP:3.6A
	sc8547_write_byte(__sc, SC8547_REG_01, 0xbf);
	sc8547_write_byte(__sc, SC8547_REG_2B, 0x00);	//VOOC_CTRL:disable
	sc8547_update_bits(__sc, SC8547_REG_09, SC8547_IBUS_UCP_RISE_MASK_MASK, (1 << SC8547_IBUS_UCP_RISE_MASK_SHIFT));

	return 0;
}

static int sc8547_svooc_hw_setting(struct sc8547 *sc)
{
	//sc8547_write_byte(sc, SC8547_REG_00, 0x2E);
	sc8547_write_byte(__sc, SC8547_REG_02, 0x01);	//VAC_OVP:12v
	sc8547_write_byte(__sc, SC8547_REG_04, 0x50);	//VBUS_OVP:10v
	sc8547_write_byte(__sc, SC8547_REG_05, 0x28);	//IBUS_OCP_UCP:3.6A
	sc8547_write_byte(__sc, SC8547_REG_09, 0x13);	//WD:1000ms
	sc8547_write_byte(__sc, SC8547_REG_11, 0x80);	//ADC_CTRL:ADC_EN
	sc8547_write_byte(__sc, SC8547_REG_0D, 0x70);
	//8547_write_byte(__sc, SC8547_REG_2B, 0x81);		//VOOC_CTRL
	//oplus_vooc_send_handshake_seq();
	sc8547_write_byte(__sc, SC8547_REG_33, 0xd1);	//Loose_det=1
	return 0;
}

static int sc8547_vooc_hw_setting(struct sc8547 *sc)
{
	//sc8547_write_byte(sc, SC8547_REG_00, 0x2E);
	sc8547_write_byte(__sc, SC8547_REG_02, 0x07);	//VAC_OVP:
	sc8547_write_byte(__sc, SC8547_REG_04, 0x50);	//VBUS_OVP:
	sc8547_write_byte(__sc, SC8547_REG_05, 0x2c);	//IBUS_OCP_UCP:
	sc8547_write_byte(__sc, SC8547_REG_09, 0x93);	//WD:5000ms
	sc8547_write_byte(__sc, SC8547_REG_11, 0x80);	//ADC_CTRL:
	//8547_write_byte(__sc, SC8547_REG_2B, 0x81);	//VOOC_CTRL
	//oplus_vooc_send_handshake_seq();
	sc8547_write_byte(__sc, SC8547_REG_33, 0xd1);	//Loose_det
	return 0;
}


static int sc8547_5v2a_hw_setting(struct sc8547 *sc)
{
	//sc8547_write_byte(sc, SC8547_REG_00, 0x2E);
	sc8547_write_byte(__sc, SC8547_REG_02, 0x07);	//VAC_OVP:
	sc8547_write_byte(__sc, SC8547_REG_04, 0x50);	//VBUS_OVP:
	//sc8547_write_byte(__sc, SC8547_REG_05, 0x2c);	//IBUS_OCP_UCP:
	sc8547_write_byte(__sc, SC8547_REG_07, 0x04);
	sc8547_write_byte(__sc, SC8547_REG_09, 0x10);	//WD:

	sc8547_write_byte(__sc, SC8547_REG_11, 0x00);	//ADC_CTRL:
	sc8547_write_byte(__sc, SC8547_REG_2B, 0x00);	//VOOC_CTRL
	//sc8547_write_byte(__sc, SC8547_REG_31, 0x01);	//
	//sc8547_write_byte(__sc, SC8547_REG_32, 0x80);	//
	//sc8547_write_byte(__sc, SC8547_REG_33, 0xd1);	//Loose_det
	return 0;
}


static int sc8547_hw_setting(CHG_PUMP_SETTING_REASON reason)
{
	if (!__sc) {
		sc_err("__sc is null exit\n");
		return -1;
	}
	switch (reason) {
		case SETTING_REASON_PROBE:
		case SETTING_REASON_RESET:
			sc8547_init_device(__sc);
			sc_info("SETTING_REASON_RESET OR PROBE\n");
			break;
		case SETTING_REASON_SVOOC:
			sc8547_svooc_hw_setting(__sc);
			sc_info("SETTING_REASON_SVOOC\n");
			break;
		case SETTING_REASON_VOOC:
			sc8547_vooc_hw_setting(__sc);
			sc_info("SETTING_REASON_VOOC\n");
			break;
		case SETTING_REASON_5V2A:
			sc8547_5v2a_hw_setting(__sc);
			sc_info("SETTING_REASON_5V2A\n");
			break;
		default:
			sc_err("do nothing\n");
			break;
	}
	return 0;
}

static ssize_t sc8547_show_registers(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sc8547 *sc = dev_get_drvdata(dev);
	u8 addr;
	u8 val;
	u8 tmpbuf[300];
	int len;
	int idx = 0;
	int ret;

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "sc8547");
	for (addr = 0x0; addr <= 0x3C; addr++) {
		if((addr < 0x24) || (addr > 0x2B && addr < 0x33)
			|| addr == 0x36 || addr == 0x3C) {
			ret = sc8547_read_byte(sc, addr, &val);
			if (ret == 0) {
				len = snprintf(tmpbuf, PAGE_SIZE - idx,
						"Reg[%.2X] = 0x%.2x\n", addr, val);
				memcpy(&buf[idx], tmpbuf, len);
				idx += len;
			}
		}
	}

	return idx;
}

static ssize_t sc8547_store_register(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sc8547 *sc = dev_get_drvdata(dev);
	int ret;
	unsigned int reg;
	unsigned int val;

	ret = sscanf(buf, "%x %x", &reg, &val);
	if (ret == 2 && reg <= 0x3C)
		sc8547_write_byte(sc, (unsigned char)reg, (unsigned char)val);

	return count;
}


static DEVICE_ATTR(registers, 0660, sc8547_show_registers, sc8547_store_register);

static void sc8547_create_device_node(struct device *dev)
{
	device_create_file(dev, &dev_attr_registers);
}


static struct of_device_id sc8547_charger_match_table[] = {
	{
		.compatible = "sc,sc8547-standalone",
	},
	{},
};

static int sc8547_sw_ctrl2_gpio_init(void)
{
	vooc_manager *chip = oplus_vooc_mg;

	if (!chip) {
		printk(KERN_ERR "[OPLUS_CHG][%s]: oplus_chip not ready!\n", __func__);
		return -EINVAL;
	}

	chip->pinctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR_OR_NULL(chip->pinctrl)) {
		chg_err("get chargerid_switch_gpio pinctrl fail\n");
		return -EINVAL;
	}

	chip->charger_gpio_sw_ctrl2_high =
			pinctrl_lookup_state(chip->pinctrl,
			"switch1_act_switch2_act");
	if (IS_ERR_OR_NULL(chip->charger_gpio_sw_ctrl2_high)) {
		chg_err("get switch1_act_switch2_act fail\n");
		return -EINVAL;
	}

	chip->charger_gpio_sw_ctrl2_low =
			pinctrl_lookup_state(chip->pinctrl,
			"switch1_sleep_switch2_sleep");
	if (IS_ERR_OR_NULL(chip->charger_gpio_sw_ctrl2_low)) {
		chg_err("get switch1_sleep_switch2_sleep fail\n");
		return -EINVAL;
	}

	pinctrl_select_state(chip->pinctrl,
			chip->charger_gpio_sw_ctrl2_low);

	chip->slave_charging_inter_default =
	    pinctrl_lookup_state(chip->pinctrl,
	                         "slave_charging_inter_default");
	if (IS_ERR_OR_NULL(chip->slave_charging_inter_default)) {
		chg_err("get slave_charging_inter_default fail\n");
	} else {
		pinctrl_select_state(chip->pinctrl,
	                     chip->slave_charging_inter_default);
	}

	printk(KERN_ERR "[OPLUS_CHG][%s]: oplus_chip is ready!\n", __func__);
	return 0;
}

int oplus_vooc_chg_sw_ctrl2_parse_dt(void)
{
	vooc_manager *chip = oplus_vooc_mg;
	int rc = 0;
	struct device_node * node = NULL;

	if (!chip) {
		sc_dbg("chip null\n");
		return -1;
	}

	/* Parsing gpio switch gpio47*/
	node = chip->dev->of_node;
	chip->switch1_gpio = of_get_named_gpio(node,
		"qcom,charging_switch1-gpio", 0);
	if (chip->switch1_gpio < 0) {
		sc_dbg("chip->switch1_gpio not specified\n");
	} else {
		if (gpio_is_valid(chip->switch1_gpio)) {
			rc = gpio_request(chip->switch1_gpio,
				"charging-switch1-gpio");
			if (rc) {
				sc_dbg("unable to request gpio [%d]\n",
					chip->switch1_gpio);
			} else {
				rc = sc8547_sw_ctrl2_gpio_init();
				if (rc)
					chg_err("unable to init charging_sw_ctrl2-gpio:%d\n",
							chip->switch1_gpio);
			}
		}
		sc_dbg("chip->switch1_gpio =%d\n", chip->switch1_gpio);
	}

	return 0;
}

int oplus_voocphy_get_switch_gpio_val(void)
{
	vooc_manager *chip = oplus_vooc_mg;

	if (!chip) {
		sc_dbg("oplus_voocphy_get_switch_gpio_val chip null\n");
		return 0;
	}
	return gpio_get_value(chip->switch1_gpio);
}

extern void set_reg_spm_f26m_req(int val);

void opchg_voocphy_set_switch_fast_charger(void)
{
	vooc_manager *chip = oplus_vooc_mg;

	if (!chip) {
		sc_err("opchg_voocphy_set_switch_fast_charger chip null\n");
		return;
	}

	pinctrl_select_state(chip->pinctrl, chip->charger_gpio_sw_ctrl2_high);
	gpio_direction_output(chip->switch1_gpio, 1);	/* out 1*/
	//dump_stack();
	sc_err("switch switch2 %d to fast finshed\n", oplus_voocphy_get_switch_gpio_val());
	set_reg_spm_f26m_req(1);


	return;
}

void opchg_voocphy_set_switch_normal_charger(void)
{
	vooc_manager *chip = oplus_vooc_mg;

	if (!chip) {
		sc_err("opchg_voocphy_set_switch_normal_charger chip null\n");
		return;
	}

	pinctrl_select_state(chip->pinctrl, chip->charger_gpio_sw_ctrl2_low);
	if (chip->switch1_gpio > 0) {
		gpio_direction_output(chip->switch1_gpio, 0);	/* in 0*/
	}
	//dump_stack();
	sc_err("switch switch2 %d to normal finshed\n", oplus_voocphy_get_switch_gpio_val());
	set_reg_spm_f26m_req(0);


	return;
}

int oplus_voocphy_hw_setting_reset(void)
{
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->hw_setting) {
		return oplus_vooc_mg->vops->hw_setting(oplus_vooc_mg, SETTING_REASON_RESET);
	} else {
		return sc8547_hw_setting(SETTING_REASON_RESET);
	}
}

int oplus_voocphy_get_adapter_request_info(void) 
{
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->get_adapter_info) {
		return oplus_vooc_mg->vops->get_adapter_info(oplus_vooc_mg);
	} else {
		oplus_voocphy_pm_qos_update(400);
		return sc8547_get_adapter_request_info();
	}
}

void oplus_voocphy_update_chg_data(void)
{
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->update_data) {
		return oplus_vooc_mg->vops->update_data(oplus_vooc_mg);
	} else {
		oplus_voocphy_pm_qos_update(400);
		sc8547_update_chg_data();
	}
}

static int oplus_voocphy_direct_chg_enable(u8 *data)
{
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->get_chg_enable) {
		return oplus_vooc_mg->vops->get_chg_enable(oplus_vooc_mg, data);
	} else {
		return sc8547_direct_chg_enable(data);
	}
}

void oplus_vooc_send_handshake_seq(void)
{
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->send_handshake) {
		return oplus_vooc_mg->vops->send_handshake(oplus_vooc_mg);
	} else {
		sc8547_send_handshake_seq();
	}
}

int oplus_voocphy_init(void) {
	int ret = 0;

	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->init_vooc) {
		ret = oplus_vooc_mg->vops->init_vooc(oplus_vooc_mg);
	} else {
		ret = sc8547_init_vooc();
	}

	if (oplus_vooc_mg) {
		oplus_vooc_mg->batt_soc = oplus_chg_get_ui_soc();
		if (oplus_vooc_mg->batt_fake_soc) {
			oplus_vooc_mg->batt_soc = oplus_vooc_mg->batt_fake_soc;
		}
		oplus_vooc_mg->plug_in_batt_temp = oplus_chg_get_chg_temperature();
		if (oplus_vooc_mg->batt_fake_temp) {
			oplus_vooc_mg->plug_in_batt_temp = oplus_vooc_mg->batt_fake_temp;
		}

		if (oplus_vooc_mg->batt_soc >= VOOC_LOW_SOC
				&& oplus_vooc_mg->batt_soc <= VOOC_HIGH_SOC
				&& oplus_vooc_mg->plug_in_batt_temp >= VOOC_LOW_TEMP
				&& oplus_vooc_mg->plug_in_batt_temp < VOOC_HIGH_TEMP) {
			oplus_vooc_mg->fastchg_real_allow = true;
		} else {
			oplus_vooc_mg->fastchg_real_allow = false;
		}

		oplus_vooc_mg->ask_current_first = true;
		oplus_vooc_mg->batt_sys_curv_found == false;
		oplus_get_soc_and_temp_with_enter_fastchg();

		sc_err("batt_soc %d, plug_in_batt_temp = %d\n", oplus_vooc_mg->batt_soc, oplus_vooc_mg->plug_in_batt_temp);
	}
	return ret;
}

static int oplus_voocphy_set_txbuff(u16 val) {
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->set_txbuff) {
		return oplus_vooc_mg->vops->set_txbuff(oplus_vooc_mg, val);
	} else {
		oplus_voocphy_pm_qos_update(400);
		return sc8547_set_txbuff(val);
	}
}

static int oplus_voocphy_set_predata(u16 val) {
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->set_predata) {
		return oplus_vooc_mg->vops->set_predata(oplus_vooc_mg, val);
	} else {
		oplus_voocphy_pm_qos_update(400);
		return sc8547_set_predata(val);
	}
}

static int sc8547_chg_enable(void) 
{
	return sc8547_write_byte(__sc, SC8547_REG_07, 0x84);
}

static int oplus_voocphy_chg_enable(void)
{
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->set_chg_enable) {
		return oplus_vooc_mg->vops->set_chg_enable(oplus_vooc_mg);
	} else {
		return sc8547_chg_enable();
	}
}

int oplus_voocphy_hw_setting(CHG_PUMP_SETTING_REASON reason) {
	if (oplus_vooc_mg && oplus_vooc_mg->vops && oplus_vooc_mg->vops->hw_setting) {
		return oplus_vooc_mg->vops->hw_setting(oplus_vooc_mg, reason);
		return 0;
	} else {
		return sc8547_hw_setting(reason);
	}
}

void * get_vooc_manager(void)
{
	return oplus_vooc_mg;
}

static int voocphy_batt_fake_temp_show(struct seq_file *seq_filp, void *v)
{
	seq_printf(seq_filp, "%d\n", oplus_vooc_mg->batt_fake_temp);
	return 0;
}
static int voocphy_batt_fake_temp_open(struct inode *inode, struct file *file)
{
	int ret;
	ret = single_open(file, voocphy_batt_fake_temp_show, NULL);
	return ret;
}

static ssize_t voocphy_batt_fake_temp_write(struct file *filp,
		const char __user *buff, size_t len, loff_t *data)
{
	int batt_fake_temp;
	char temp[16];

	if (len > sizeof(temp)) {
		return -EINVAL;
	}
	if (copy_from_user(temp, buff, len)) {
		pr_err("voocphy_batt_fake_temp_write error.\n");
		return -EFAULT;
	}
	sscanf(temp, "%d", &batt_fake_temp);
	oplus_vooc_mg->batt_fake_temp = batt_fake_temp;
	sc_err("batt_fake_temp is %d\n", batt_fake_temp);
	
	return len;
}

static const struct file_operations voocphy_batt_fake_temp_fops = {
	.open = voocphy_batt_fake_temp_open,
	.write = voocphy_batt_fake_temp_write,
	.read = seq_read,
};


static int voocphy_batt_fake_soc_show(struct seq_file *seq_filp, void *v)
{
	seq_printf(seq_filp, "%d\n", oplus_vooc_mg->batt_fake_soc);
	return 0;
}

static int voocphy_batt_fake_soc_open(struct inode *inode, struct file *file)
{
	int ret;
	ret = single_open(file, voocphy_batt_fake_soc_show, NULL);
	return ret;
}

static ssize_t voocphy_batt_fake_soc_write(struct file *filp,
		const char __user *buff, size_t len, loff_t *data)
{
	int batt_fake_soc;
	char temp[16];

	if (len > sizeof(temp)) {
		return -EINVAL;
	}
	if (copy_from_user(temp, buff, len)) {
		pr_err("voocphy_batt_fake_soc_write error.\n");
		return -EFAULT;
	}
	sscanf(temp, "%d", &batt_fake_soc);
	oplus_vooc_mg->batt_fake_soc = batt_fake_soc;
	sc_err("batt_fake_soc is %d\n", batt_fake_soc);
	
	return len;
}

static const struct file_operations voocphy_batt_fake_soc_fops = {
	.open = voocphy_batt_fake_soc_open,
	.write = voocphy_batt_fake_soc_write,
	.read = seq_read,
};


static int voocphy_loglevel_show(struct seq_file *seq_filp, void *v)
{
	seq_printf(seq_filp, "%d\n", voocphy_log_level);
	return 0;
}
static int voocphy_loglevel_open(struct inode *inode, struct file *file)
{
	int ret;
	ret = single_open(file, voocphy_loglevel_show, NULL);
	return ret;
}

static ssize_t voocphy_loglevel_write(struct file *filp,
		const char __user *buff, size_t len, loff_t *data)
{
	char temp[16];

	if (len > sizeof(temp)) {
		return -EINVAL;
	}
	if (copy_from_user(temp, buff, len)) {
		pr_err("voocphy_loglevel_write error.\n");
		return -EFAULT;
	}
	sscanf(temp, "%d", &voocphy_log_level);
	sc_err("voocphy_log_level is %d\n", voocphy_log_level);

	return len;
}

static const struct file_operations voocphy_loglevel_fops = {
	.open = voocphy_loglevel_open,
	.write = voocphy_loglevel_write,
	.read = seq_read,
};

void init_proc_voocphy_debug(void)
{
	struct proc_dir_entry *p_batt_fake_temp = NULL;
	struct proc_dir_entry *p_batt_fake_soc = NULL;
	struct proc_dir_entry *p_batt_loglevel = NULL;

	p_batt_fake_temp = proc_create("voocphy_batt_fake_temp", 0664, NULL,
			&voocphy_batt_fake_temp_fops);
	if (!p_batt_fake_temp) {
		pr_err("proc_create voocphy_batt_fake_temp_fops fail!\n");
	}
	oplus_vooc_mg->batt_fake_temp = 0;

	p_batt_fake_soc = proc_create("voocphy_batt_fake_soc", 0664, NULL,
			&voocphy_batt_fake_soc_fops);
	if (!p_batt_fake_soc) {
		pr_err("proc_create voocphy_batt_fake_soc_fops fail!\n");
	}
	oplus_vooc_mg->batt_fake_soc = 0;

	p_batt_loglevel = proc_create("voocphy_loglevel", 0664, NULL,
			&voocphy_loglevel_fops);
	if (!p_batt_loglevel) {
		pr_err("proc_create voocphy_loglevel_fops fail!\n");
	}
}

struct voocphy_level {
	int level;
	int voocphy_level;	/*10~30==>Ibus:1000mA~3000mA*/
};

struct voocphy_level vooc_tbl[] = {
	{1, 20}, 		/*2000mA*/
	{2, 30}, 		/*3000mA*/
};

struct voocphy_level svooc_tbl[] = {
	{0, 30},		/*3000mA*/
	{1, 10}, 		/*1000mA*/
	{2, 10}, 		/*1000mA*/
	{3, 10}, 		/*1000mA*/
	{4, 15}, 		/*1500mA*/
	{5, 20}, 		/*2000mA*/
	{6, 25}, 		/*2500mA*/
	{7, 30} 		/*3000mA*/
};

bool oplus_voocphy_is_fast_chging(void) {
	if (!oplus_vooc_mg)
	{
		return false;
	}

	sc_info("fastchg_adapter_ask_cmd %d\n", oplus_vooc_mg->fastchg_adapter_ask_cmd);
	return oplus_vooc_mg->fastchg_adapter_ask_cmd == VOOC_CMD_GET_BATT_VOL;
}

int oplus_voocphy_thermal_current_to_level(int ibus)
{
	ibus = ibus/100;
	sc_err("ibus/100 = %d\n", ibus);

	return ibus;
}

int oplus_voocphy_cool_down_convert(int bf_cool_down)
{
	int af_cool_down = 0;
	int i = 0;
	int len = 0;
	struct voocphy_level *plevel_tbl = NULL;

	if (oplus_vooc_mg->adapter_type == ADAPTER_SVOOC) {
		plevel_tbl = svooc_tbl;
		len= ARRAY_SIZE(svooc_tbl);
	} else {
		plevel_tbl = vooc_tbl;
		len= ARRAY_SIZE(vooc_tbl);
	}

	for (i = 0; i < len; i++) {
		if (plevel_tbl[i].level == bf_cool_down)
		{
			af_cool_down = plevel_tbl[i].voocphy_level;
			break;
		}
	}
	sc_err("af_cool_down[%d]\n", af_cool_down);

	return af_cool_down;
}

int oplus_ap_current_notify_voocphy(void) {
	struct oplus_chg_chip *chip = oplus_get_oplus_chip();
	unsigned char screenoff_current = 30;
	unsigned char smartchg_current = 30;
	
	unsigned char ap_request_current = 0;

	if (!chip || !oplus_vooc_mg) {
		sc_err("chip or oplus_vooc_mg is null\n");
		return 0;
	}

	if (chip->led_on) {
		chip->screenoff_curr = 0;
	}

	screenoff_current = oplus_voocphy_thermal_current_to_level(chip->screenoff_curr);
	if (screenoff_current == 0 || screenoff_current > 30) {
		screenoff_current = 30;
	}

	smartchg_current = oplus_voocphy_cool_down_convert(chip->user_cool_down);
	if (smartchg_current == 0 || smartchg_current > 30) {
		smartchg_current = 30;
	}
	
	ap_request_current = smartchg_current > screenoff_current ? screenoff_current : smartchg_current;

	sc_err("creenoff_current[%u, %u] smartchg_current[%u, %u] ap_current = %u %u\n",
		chip->screenoff_curr, screenoff_current, chip->user_cool_down,
		smartchg_current, oplus_vooc_mg->ap_request_current, ap_request_current);
	return ap_request_current;
}

static int sc8547_charger_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct sc8547 *sc;
	const struct of_device_id *match;
	struct device_node *node = client->dev.of_node;
	int ret;

/*hongzhenglong@ODM.HQ.BSP.CHG 2020/04/19 add for bringing up 18W&33W*/
	if(is_spaceb_hc_project() == 2){
		return -ENODEV;
	}


	sc_err("sc8547_charger_probe enter!\n");
	sc = devm_kzalloc(&client->dev, sizeof(struct sc8547), GFP_KERNEL);
	if (!sc)
		return -ENOMEM;
	__sc = sc;

	sc->dev = &client->dev;
	sc->client = client;

	mutex_init(&sc->i2c_rw_lock);
	mutex_init(&sc->data_lock);
	mutex_init(&sc->charging_disable_lock);
	mutex_init(&sc->irq_complete);
	//wakeup_source_init(&sc->ws, "voocphy_wlock");	//lkl porting

	sc->resume_completed = true;
	sc->irq_waiting = false;

	i2c_set_clientdata(client, sc);
	sc8547_create_device_node(&(client->dev));

	match = of_match_node(sc8547_charger_match_table, node);
	if (match == NULL) {
		sc_dbg("device tree match not found!\n");
		//return -ENODEV;
	}

	//hw_init
	ret = sc8547_reg_reset(sc, true);
	if (ret) {
		goto err_1;
	}
	sc8547_init_device(sc);

	//request oplus_vooc_mg memery
	oplus_vooc_init_res(0);	
	if(oplus_vooc_mg)
	{
		oplus_vooc_mg->chrpmp_id = CHGPMP_ID_SC8547;
		oplus_vooc_mg->dev = &client->dev;
		oplus_vooc_mg->client = client;
		oplus_vooc_chg_sw_ctrl2_parse_dt();
	}

	//irq register
	ret = sc8547_irq_register(sc);
	if (ret < 0)
		goto err_1;

	ret = sc8547_parse_dt(sc, &client->dev);
	//if (ret)
	//	return -EIO;
	sc_err("sc8547_parse_dt successfully!\n");

	init_proc_voocphy_debug();
	
	sc_err("sc8547_charger_probe succesfull\n");
	return 0;

err_1:
	sc_err("sc8547 probe err_1 rm irq 12341 \n");
	return ret;
}

static void sc8547_charger_shutdown(struct i2c_client *client)
{
	if (__sc) {
		sc8547_write_byte(__sc, SC8547_REG_11, 0x00);
		sc8547_write_byte(__sc, SC8547_REG_21, 0x00);
		sc8547_write_byte(__sc, SC8547_REG_07, 0x04);
	}

	return;
}



static const struct i2c_device_id sc8547_charger_id[] = {
	{"sc8547-standalone", 0},
	{},
};

static struct i2c_driver sc8547_charger_driver = {
	.driver		= {
		.name	= "sc8547-charger",
		.owner	= THIS_MODULE,
		.of_match_table = sc8547_charger_match_table,
	},
	.id_table	= sc8547_charger_id,

	.probe		= sc8547_charger_probe,
	.shutdown	= sc8547_charger_shutdown,
};

module_i2c_driver(sc8547_charger_driver);

MODULE_DESCRIPTION("SC SC8547 Charge Pump Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Aiden-yu@southchip.com");
