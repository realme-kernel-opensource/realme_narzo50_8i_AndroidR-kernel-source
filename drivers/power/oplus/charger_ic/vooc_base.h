#ifndef VOOC_BASIC_H
#define VOOC_BASIC_H

#include <linux/workqueue.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
#include <linux/wakelock.h>
#endif
#include <linux/timer.h>
#include <linux/hrtimer.h>

#include <linux/slab.h>

typedef unsigned char			BOOLEAN;
typedef char					CHAR;
typedef signed char				INT8;
typedef unsigned char			UCHAR;
typedef unsigned char			UINT8;
typedef unsigned char			BYTE;
typedef short					SHORT;
typedef signed short			INT16;
typedef unsigned short			USHORT;
typedef unsigned short			UINT16;
typedef unsigned short			WORD;
typedef int						INT;
typedef signed int				INT32;
typedef unsigned int			UINT;
typedef unsigned int			UINT32;
typedef long					LONG;
typedef unsigned long			ULONG;
typedef unsigned long			DWORD;
typedef long long				LONGLONG;
typedef long long				LONG64;
typedef unsigned long long		ULONGLONG;
typedef unsigned long long		DWORDLONG;
typedef unsigned long long		ULONG64;
typedef unsigned long long		DWORD64;
typedef unsigned long long		UINT64;
typedef unsigned short			WCHAR;
typedef int						WCHAR_T;
typedef long long      INT64;

typedef enum
{
	VOOC_VBUS_UNKNOW, 	//unknow adata vbus status
	VOOC_VBUS_NORMAL, 	//vooc vbus in normal status
	VOOC_VBUS_HIGH,		 //vooc vbus in high status
	VOOC_VBUS_LOW,		//vooc vbus in low status
	VOOC_VBUS_INVALID,	//vooc vbus in invalid status
}VOOC_VBUS_STATUS;


typedef enum
{
	BATT_TEMP_HIGH,
	BATT_TEMP_WARM,
	BATT_TEMP_NORMAL,
	BATT_TEMP_LITTLE_COOL,
	BATT_TEMP_COOL,
	BATT_TEMP_LITTLE_COLD,
	BATT_TEMP_COLD,
	BATT_TEMP_REMOVE,
}BATT_TEMP_PLUGIN_STATUS;

typedef enum
{
	BATT_SOC_0_TO_50,
	BATT_SOC_50_TO_75,
	BATT_SOC_75_TO_85,
	BATT_SOC_85_TO_90,
	BATT_SOC_90_TO_100,
}BATT_SOC_PLUGIN_STATUS;

typedef enum _FASTCHG_STATUS
{
	FAST_NOTIFY_UNKNOW,
	FAST_NOTIFY_PRESENT,
	FAST_NOTIFY_ONGOING,
	FAST_NOTIFY_ABSENT,
	FAST_NOTIFY_FULL,
	FAST_NOTIFY_BAD_CONNECTED,
	FAST_NOTIFY_BATT_TEMP_OVER,
	FAST_NOTIFY_BTB_TEMP_OVER,
	FAST_NOTIFY_DUMMY_START,
	FAST_NOTIFY_ADAPTER_COPYCAT,
	FAST_NOTIFY_ERR_COMMU,
	FAST_NOTIFY_USER_EXIT_FASTCHG,
}FASTCHG_STATUS;


typedef enum
{
    BATTMNGR_SUCCESS,
    /*Level: Warning          0x1XX*/
    BATTMNGR_WARN_NO_OP     = 0x100,            /*Warn: No Operation*/
    /*Level: Error            0x2XX*/
    BATTMNGR_EFAILED        = 0x200,            /*Error: Operation Failed*/
    BATTMNGR_EUNSUPPORTED,                      /*Error: Not Supported*/
    BATTMNGR_EINVALID,                          /*Error: Invalid Argument*/
    BATTMNGR_EINVALID_FP,                       /*Error: Invalid Function Pointer*/
    BATTMNGR_ENOTALLOWED,                       /*Error: Operation Not Allowed*/
    BATTMNGR_EMEM,                              /*Error: Not Enough Memory to Perform Operation*/
    BATTMNGR_EBADPTR,                           /*Error: NULL Pointer*/
    BATTMNGR_ETESTMODE,                         /*Error: Test Mode*/
    BATTMNGR_EFATAL          = -1               /*Error: Fatal*/
}BattMngr_err_code_e;


typedef enum
{
    STATUS_SUCCESS,
    /*Level: Warning    0x1XX*/
    STATUS_WARN_NO_OP = 0x100,
    /*Level: Error      0x2XX*/
    STATUS_ERROR_UNSUCCESSFUL = 0x200,
    STATUS_ERROR_NOT_SUPPORTED,
    STATUS_ERROR_INVALID_ARGUMENT,
    STATUS_ERROR_INVALID_FUNCTION_POINTER,
} PMSTATUS;


typedef enum
{
	ADAPTER_VOOC20,
	ADAPTER_VOOC30,
	ADAPTER_SVOOC,
}VOOC_ADAPTER_TYPE;

typedef enum {
  POWER_SUPPLY_USB_SUBTYPE_UNKNOWN,
  POWER_SUPPLY_USB_SUBTYPE_VOOC,
  POWER_SUPPLY_USB_SUBTYPE_SVOOC,
  POWER_SUPPLY_USB_SUBTYPE_PD,
  POWER_SUPPLY_USB_SUBTYPE_QC
} power_supply_usb_subtype_e;



typedef enum _VOOC_THREAD_TIMER_TYPE
{
    //VOOC_THREAD_TIMER_HKTIMER,
    VOOC_THREAD_TIMER_AP,
    VOOC_THREAD_TIMER_CURR,
    VOOC_THREAD_TIMER_SOC,
    VOOC_THREAD_TIMER_VOL,
    VOOC_THREAD_TIMER_TEMP,
    VOOC_THREAD_TIMER_BTB,
    VOOC_THREAD_TIMER_SAFE,
    VOOC_THREAD_TIMER_COMMU,
    VOOC_THREAD_TIMER_DISCON,
    VOOC_THREAD_TIMER_FASTCHG_CHECK,
    VOOC_THREAD_TIMER_CHG_OUT_CHECK,
    VOOC_THREAD_TIMER_TEST_CHECK,
    VOOC_THREAD_TIMER_INVALID,
    VOOC_THREAD_TIMER_MAX = VOOC_THREAD_TIMER_INVALID
}VOOC_THREAD_TIMER_TYPE;

/*********************************************************************
##############		part of fastchg monitor    ################
**********************************************************************/
typedef enum _VOOC_EVENT_TYPE
{
	/*General Event*/
	VOOC_EVENT_EXIT,

	/*Main Charger-Specific Event*/
	VOOC_EVENT_AP_TIMEOUT,                   	/* event of ap monitor timeout 6s one time */
	VOOC_EVENT_CURR_TIMEOUT,              	/* event of current monitor timeout 1s one time*/
	VOOC_EVENT_SOC_TIMEOUT,            		/* event of soc monitor timeout 2s one time*/
	VOOC_EVENT_VOL_TIMEOUT,       			/* event of voltage monitor timeout 500ms one time */
	VOOC_EVENT_TEMP_TIMEOUT,                	/* event of temp monitor timeout 500ms one time*/
	VOOC_EVENT_BTB_TIMEOUT,			 	/* event of btb monitor timeout 800ms one time*/
	VOOC_EVENT_SAFE_TIMEOUT,			 	/* event of safe monitor timeout 100ms one time*/
	VOOC_EVENT_COMMU_TIMEOUT,			/*event of commu timeout 10ms one time*/
	VOOC_EVENT_MONITOR_START,			/*event of start vooc monitor*/
	VOOC_EVENT_VBUS_VBATT_GET,			/*event get vbus & vbatt when ask vbus-vbatt*/
	VOOC_EVENT_DISCON_TIMEOUT,			/*event of avoid disconnect of fastchg_to_normal or warm*/
	VOOC_EVENT_FASTCHG_CHECK_TIMEOUT,	/*event of fastchg check when dummy or tempover*/
	VOOC_EVENT_CHG_OUT_CHECK_TIMEOUT, /*event of avoid full or temp over disconnect chg out check*/
	VOOC_EVENT_TEST_CHECK_TIMEOUT,	/*event of vote latency check for vooc*/

	VOOC_EVENT_INVALID
}VOOC_EVENT_TYPE;


typedef enum _VOOCPHY_REQUEST_TYPE
{
	/*General Event*/
	VOOCPHY_REQUEST_RESET,
	VOOCPHY_REQUEST_UPDATE_DATA,
	VOOCPHY_REQUEST_INVALID
}VOOCPHY_REQUEST_TYPE;

typedef enum _CHG_PUMP_SETTING_REASON
{
	/*General Event*/
	SETTING_REASON_PROBE,
	SETTING_REASON_RESET,
	SETTING_REASON_SVOOC,
	SETTING_REASON_VOOC,
	SETTING_REASON_5V2A
}CHG_PUMP_SETTING_REASON;

#define VOOC_THREAD_STK_SIZE                         0x800
#define VOOC_WAIT_EVENT                              (1 << VOOC_EVENT_INVALID) - 1
#define VOOC_AP_EVENT_TIME				6000
#define VOOC_CURR_EVENT_TIME				1000
#define VOOC_SOC_EVENT_TIME				10
#define VOOC_VOL_EVENT_TIME				500
#define VOOC_TEMP_EVENT_TIME				500
#define VOOC_BTB_EVENT_TIME				800
#define VOOC_BTB_OVER_EVENT_TIME			10000
#define VOOC_SAFE_EVENT_TIME				100
#define VOOC_COMMU_EVENT_TIME			200
#define VOOC_DISCON_EVENT_TIME			350
#define VOOC_FASTCHG_CHECK_TIME			5000
#define VOOC_CHG_OUT_CHECK_TIME			3000
#define VOOC_TEST_CHECK_TIME				20
#define VOOC_BASE_TIME_STEP				5

// Convert Event enum to bitmask of event
#define VOOC_EVENTTYPE_TO_EVENT(x) 					(1 << x)
#define VOOC_EVENT_EXIT_MASK							0x01
#define VOOC_EVENT_AP_TIMEOUT_MASK					0x02
#define VOOC_EVENT_CURR_TIMEOUT_MASK				0x04
#define VOOC_EVENT_SOC_TIMEOUT_MASK					0x08
#define VOOC_EVENT_VOL_TIMEOUT_MASK					0x10
#define VOOC_EVENT_TEMP_TIMEOUT_MASK				0x20
#define VOOC_EVENT_BTB_TIMEOUT_MASK					0x40
#define VOOC_EVENT_SAFE_TIMEOUT_MASK				0x80
#define VOOC_EVENT_COMMU_TIMEOUT_MASK				0x100
#define VOOC_EVENT_MONITOR_START_MASK				0x200
#define VOOC_EVENT_VBUS_VBATT_GET_MASK				0X400
#define VOOC_EVENT_DISCON_TIMEOUT_MASK				0X800
#define VOOC_EVENT_FASTCHG_CHECK_TIMEOUT_MASK		0X1000
#define VOOC_EVENT_CHG_OUT_CHECK_TIMEOUT_MASK		0X2000
#define VOOC_EVENT_TEST_TIMEOUT_MASK				0X4000



#define VBATT_4180_MV	4180
#define VBATT_4200_MV	4200
#define VBATT_4204_MV	4204
#define VBATT_4420_MV	4420
#define VBATT_4430_MV	4430
#define VBATT_4450_MV	4450
#define VBATT_4454_MV	4454
#define VBATT_4470_MV	4470
#define VBATT_4474_MV	4474
#define VBATT_4475_MV	4475
#define VBATT_4480_MV	4480
#define VBATT_4485_MV	4485

#define	VBATT_750_MA	8
#define	VBATT_1000_MA	10
#define	VBATT_1250_MA	13
#define	VBATT_1500_MA	15
#define	VBATT_1750_MA	18
#define VBATT_2000_MA	20
#define VBATT_2250_MA	23
#define	VBATT_2500_MA	25
#define VBATT_2600_MA	26
#define VBATT_2750_MA	27
#define VBATT_3000_MA	30
#define VBATT_3250_MA	33
#define VBATT_3800_MA	38
#define VBATT_4000_MA	40
#define VBATT_4600_MA	46
#define VBATT_5000_MA	50
#define VBATT_6000_MA	60


#define VOOC_DIS	0x0
#define VOOC_EN		0x1
#define VOOC_MODE_BB   0x0
#define VOOC_MODE_PHY 0x1
#define VOOC_DP_OE_HIZ 0x0
#define VOOC_DP_OE_EN  0x1
#define VOOC_DP_BB_0    0x0
#define VOOC_DP_BB_1    0x1

#define VOOC_COMM_ERR_2 		0x20
#define VOOC_COMM_ERR_1 		0x10
#define VOOC_VBATT_OV_ANA		0x08
#define VOOC_VBATT_OV_ADC		0x04
#define VOOC_IBATT_OC_ADC		0x02
#define VOOC_VDROP_OV_ADC	0x01

#define VOOC_JEITA_HOT_ADC	0x10
#define VOOC_JEITA_COLD_ADC	0x08
#define VOOC_CONN_ADC			0x04
#define VOOC_SMB_ADC			0x02
#define VOOC_WLS_ADC			0x01

#define VOOC_PLUS_COUNTS_MAX  25

#define VOOCPHY_TX_LEN 2
#define VOOC_MESG_READ_MASK 0X1E
#define VOOC_MESG_MOVE_READ_MASK 0X0F

#define BIT_ACTIVE 0x1
#define BIT_SLEEP 0x0

#define RX_DET_BIT0_MASK		0x1
#define RX_DET_BIT1_MASK		0x2
#define RX_DET_BIT2_MASK		0x4
#define RX_DET_BIT3_MASK		0x8
#define RX_DET_BIT4_MASK		0x10
#define RX_DET_BIT5_MASK		0x20
#define RX_DET_BIT6_MASK		0x40
#define RX_DET_BIT7_MASK		0x80

#define VOOC2_HEAD					0x05
#define VOOC3_HEAD					0x03
#define SVOOC_HEAD					0x04
#define VOOC2_INVERT_HEAD			0x05
#define VOOC3_INVERT_HEAD			0x06
#define SVOOC_INVERT_HEAD			0x01
#define VOOC_HEAD_MASK			0xe0
#define VOOC_INVERT_HEAD_MASK 	0x07
#define VOOC_HEAD_SHIFT			0x05
#define VOOC_MOVE_HEAD_MASK		0x70
#define VOOC_MOVE_HEAD_SHIFT		0x04

#define  TX0_DET_BIT3_TO_BIT6_MASK 0x78
#define TX0_DET_BIT3_TO_BIT7_MASK 0xf8
#define TX1_DET_BIT0_TO_BIT1_MASK 0x3
#define TX0_DET_BIT0_MASK 	0x1
#define TX0_DEF_BIT1_MASK	0x2
#define TX0_DET_BIT2_MASK	0x4
#define TX0_DET_BIT3_MASK	0x8
#define TX0_DET_BIT4_MASK 	0x10
#define TX0_DET_BIT5_MASK	0x20
#define TX0_DET_BIT6_MASK	0x40
#define TX0_DET_BIT7_MASK	0x80
#define TX1_DET_BIT0_MASK	0x1
#define TX1_DET_BIT1_MASK      0x2
#define TX0_DET_BIT0_SHIFT 0x0
#define TX0_DET_BIT1_SHIFT 0x1
#define TX0_DET_BIT2_SHIFT 0x2
#define TX0_DET_BIT3_SHIFT 0x3
#define TX0_DET_BIT4_SHIFT 0x4
#define TX0_DET_BIT5_SHIFT 0x5
#define TX0_DET_BIT6_SHIFT 0x6
#define TX0_DET_BIT7_SHIFT 0x7
#define TX1_DET_BIT0_SHIFT 0x0
#define TX1_DET_BIT1_SHIFT 0x1

#define VOOC_CMD_UNKNOW			0xff
#define VOOC_CMD_NULL				0x00
#define VOOC_CMD_GET_BATT_VOL       0x01
#define VOOC_CMD_IS_VUBS_OK         0x02
#define VOOC_CMD_ASK_CURRENT_LEVEL  0x03
#define VOOC_CMD_ASK_FASTCHG_ORNOT  0x04
#define VOOC_CMD_TELL_MODEL         0x05
#define VOOC_CMD_ASK_POWER_BANK    0x06
#define VOOC_CMD_TELL_FW_VERSION    0x07
#define VOOC_CMD_ASK_UPDATE_ORNOT   0x08
#define VOOC_CMD_TELL_USB_BAD       0x09
#define VOOC_CMD_ASK_AP_STATUS      0x0A
#define VOOC_CMD_ASK_CUR_UPDN       0x0B
#define VOOC_CMD_IDENTIFICATION     0x0C
#define VOOC_CMD_ASK_BAT_MODEL      0x0D
#define VOOC_CMD_ADAPTER_CHECK_COMMU_PROCESS 0xF1
#define VOOC_CMD_TELL_MODEL_PROCESS 0xF2
#define VOOC_CMD_ASK_BATT_SYS_PROCESS 0xF3

#define OPLUS_IS_VUBS_OK_PREDATA_VOOC20	((u16)0xa002)
#define OPLUS_IS_VUBS_OK_PREDATA_VOOC30	((u16)0xa001)
#define OPLUS_IS_VUBS_OK_PREDATA_SVOOC	((u16)0x2002)

#define ADAPTER_CHECK_COMMU_TIMES 0x6 //adapter verify need commu 6 times
#define ADAPTER_MODEL_TIMES 0x3 //adapter model need by 3 times

#define VBATT_BASE_FOR_ADAPTER                  3404
#define VBATT_DIV_FOR_ADAPTER                   10

#define VOOC_RX_RECEIVED_STATUS	0x03
#define VOOC_RX_STARTED_STATUS  	0X01
#define VOOC_TX_TRANSMI_STATUS	0x04

#define VOOC_SHIFT_FROM_MASK(x) ((x & 0x01) ? 0 : \
                               (x & 0x02) ? 1 : \
                               (x & 0x04) ? 2 : \
                               (x & 0x08) ? 3 : \
                               (x & 0x10) ? 4 : \
                               (x & 0x20) ? 5 : \
                               (x & 0x40) ? 6 : \
                               (x & 0x80) ? 7 : \
                               (x & 0x100) ? 8 : \
                               (x & 0x200) ? 9 : \
                               (x & 0x400) ? 10 : \
                               (x & 0x800) ? 11 : 0)

/*
	* example regs
	* data_l: low addr reg		0x2e
	* data_h: high addr reg		0x2f
*/
#define SC8547_DATA_MASK	0xff
#define SC8547_DATA_H_SHIFT	0x08

#define VOOCPHY_DATA16_SPLIT(data_src, data_l, data_h)	\
do {				\
	data_l = (data_src) & SC8547_DATA_MASK;	\
	data_h = ((data_src) >> SC8547_DATA_H_SHIFT) & SC8547_DATA_MASK;	\
} while(0);

#define MONITOR_EVENT_NUM	VOOC_THREAD_TIMER_INVALID
#define VOOC_MONITOR_START	1
#define VOOC_MONITOR_STOP	0

#define ADJ_CUR_STEP_DEFAULT		0
#define ADJ_CUR_STEP_REPLY_VBATT	1
#define ADJ_CUR_STEP_SET_CURR_DONE	2

typedef BattMngr_err_code_e (*vooc_mornitor_hdler_t)(unsigned long data);
typedef struct vooc_monitor_event 
{	
	int status;
	int cnt;
	bool timeout;
	int expires;
	unsigned long data;
	vooc_mornitor_hdler_t mornitor_hdler;
} vooc_monitor_event_t;

typedef struct {
	UINT32 target_vbat;
	UINT32 target_ibus;
	BOOLEAN exit;
	UINT32 target_time;
	UINT32 chg_time;
	UINT32 max_ibus_thr;
	UINT32 min_ibus_thr;
} OPLUS_SVOOC_BATT_SYS_CURVE;

#define BATT_SYS_ROW_MAX        8
#define BATT_SYS_COL_MAX        7
#define BATT_SYS_MAX        4

typedef struct {
	OPLUS_SVOOC_BATT_SYS_CURVE batt_sys_curve[BATT_SYS_ROW_MAX];
	UINT8 sys_curv_num;
} OPLUS_SVOOC_BATT_SYS_CURVES;

enum {
	BATT_SYS_CURVE_TEMP0_2_TEMP45   ,
	BATT_SYS_CURVE_TEMP45_2_TEMP115 ,
	BATT_SYS_CURVE_TEMP115_2_TEMP160,
	BATT_SYS_CURVE_TEMP160_2_TEMP430,
	//BATT_SYS_CURVE_TEMP115_2_TEMP160,
	//BATT_SYS_CURVE_TEMP160_2_TEMP250,
	//BATT_SYS_CURVE_TEMP160_2_TEMP250,
	//BATT_SYS_CURVE_TEMP250_2_TEMP430,
	BATT_SYS_CURVE_MAX,
};

typedef enum _VOOCPHY_CHRPMP_ID {
	CHGPMP_ID_SC8547,
	CHGPMP_ID_HL7138,
	CHGPMP_ID_INVAL,
} VOOCPHY_CHRPMP_ID;


typedef struct _vooc_manager
{
   UINT8 voocphy_rx_buff;
   UINT8 voocphy_tx_buff[VOOCPHY_TX_LEN];
   UINT8 adapter_mesg;
   UINT8 adapter_model_detect_count;
   UINT8 fastchg_adapter_ask_cmd;
   UINT8 pre_adapter_ask_cmd;
   UINT8 vooc2_next_cmd;
   UINT8 adapter_ask_check_count;
   UINT8 adapter_check_cmmu_count;
   UINT8 adapter_rand_h; //adapter checksum high byte
   UINT8 adapter_rand_l;  //adapter checksum low byte
   UINT16 code_id_local; //identification code at voocphy
   UINT16 code_id_far;	 //identification code from adapter
   UINT16 batt_soc;

   UINT16 plug_in_batt_soc;	//lkl need modify
   UINT16 plug_in_batt_temp;	//lkl need modify
   
   UINT8  code_id_temp_l; //identification code temp save
   UINT8  code_id_temp_h; 
   UINT8 adapter_model_ver;
   UINT8 adapter_model_count; //obtain adapter_model need times
   UINT8 ask_batt_sys; //batt_sys
   UINT8 current_expect;
   UINT8 current_max;
   UINT8 current_ap;
   UINT8 current_batt_temp;
   UINT8 ap_need_change_current;
   UINT8 adjust_curr;
   UINT8 adjust_fail_cnt;
   UINT8 ap_request_current;
   UINT8 vooc_head; 	   //vooc_frame head
   UINT32 fastchg_batt_temp_status; //batt temp status
   UINT32 vooc_temp_cur_range;	   // range of current vooc temp
   UINT32 fastchg_timeout_time; //fastchg_total time
   UINT32 fastchg_3c_timeout_time; // fastchg_3c_total_time
   UINT32 bq27541_vbatt;		   // batt voltage
   UINT32 bq27541_pre_vbatt;
   UINT32 cp_vbus;
   UINT32 cp_vsys;
   UINT32 cp_ichg;
   UINT32 icharging;
   UINT32 ask_current_first;
   UINT32 adapter_ask_first;
   UINT32 vbus; 					   // vbus voltage
   UINT32 vbat_calc;
   int voocphy_iic_err;
   int vbus_adjust_cnt;
   int fastchg_adapter_ask_cmd_copy;
   UINT32 current_pwd;			   //copycat adapter current thd
   UINT32 curr_pwd_count;		   //count for	ccopycat adapter is ornot

   BATT_TEMP_PLUGIN_STATUS batt_temp_plugin; //batt_temp at plugin
   BATT_SOC_PLUGIN_STATUS batt_soc_plugin; //batt_soc at plugin
   BOOLEAN adapter_rand_start; //adapter checksum need start;
   BOOLEAN adapter_check_ok;
   BOOLEAN fastchg_allow;
   BOOLEAN ask_bat_model_finished;
   BOOLEAN ask_vol_again;
   BOOLEAN ignore_first_frame;
   BOOLEAN ask_vooc3_detect;
   BOOLEAN force_2a;
   BOOLEAN force_2a_flag;
   BOOLEAN force_3a;
   BOOLEAN force_3a_flag;
   BOOLEAN btb_temp_over;
   BOOLEAN btb_err_first;
   BOOLEAN usb_bad_connect;
   BOOLEAN fastchg_dummy_start;
   BOOLEAN fastchg_start;
   BOOLEAN fastchg_to_normal;
   BOOLEAN fastchg_to_warm;
   BOOLEAN fastchg_err_commu;
   BOOLEAN fastchg_reactive;
   BOOLEAN fastchg_real_allow;
   BOOLEAN fastchg_commu_stop;
   BOOLEAN fastchg_check_stop;
   BOOLEAN fastchg_monitor_stop;
   BOOLEAN vooc_move_head;
   //crash crash_func;

   BOOLEAN user_set_exit_fastchg;
   BOOLEAN user_exit_fastchg;
   BOOLEAN fastchg_stage;
   BOOLEAN fastchg_need_reset;
   BOOLEAN fastchg_recovering;
   UINT32 fastchg_recover_cnt;

   VOOC_VBUS_STATUS vooc_vbus_status;
   INT32 vbus_vbatt;
   VOOC_ADAPTER_TYPE adapter_type;
   UINT32 fastchg_notify_status;

   //BATTMNGR_SYS_PWRMODE_VOTE_HANDLE_TYPE hPwrModeVote;

   //BATTMNGR_WAITLOCK voocphy_process_wait_lock; // for voocphy process wait lock
   //BATTMNGR_WAITLOCK voocphy_commu_wait_lock; //for voocphy commu wait lock
   //BATTMNGR_WAITLOCK voocphy_config_wait_lock; //for voocphy commu wait lock
   
   struct hrtimer monitor_btimer;	//monitor base timer
   ktime_t moniotr_kt;
   
   VOOCPHY_REQUEST_TYPE voocphy_request;
   struct delayed_work voocphy_service_work;
   struct delayed_work notify_fastchg_work;
   struct delayed_work monitor_work;
   struct delayed_work modify_cpufeq_work;
   atomic_t  voocphy_freq_state;
   int voocphy_freq_mincore;
   int voocphy_freq_midcore;
   int voocphy_freq_maxcore;
   int voocphy_current_change_timeout;

   //batt sys curv
   BOOLEAN batt_sys_curv_found;
   OPLUS_SVOOC_BATT_SYS_CURVES *batt_sys_curv_by_soc;
   OPLUS_SVOOC_BATT_SYS_CURVES *batt_sys_curv_by_tmprange;
   UINT8 cur_sys_curv_idx;
   int sys_curve_temp_idx;

   vooc_monitor_event_t mornitor_evt[MONITOR_EVENT_NUM];

	//record debug info
	int irq_total_num;
	int ap_handle_timeout_num;
	int rcv_done_200ms_timeout_num;
	int rcv_date_err_num;
	int irq_rcvok_num;
	int irq_rcverr_num;
	int vooc_flag;
	int int_flag;
	int irq_tx_timeout_num;
	int irq_tx_timeout;
	int irq_hw_timeout_num;

	int batt_fake_temp;
	int batt_fake_soc;

	VOOCPHY_CHRPMP_ID chrpmp_id;
	struct oplus_voocphy_operations *vops;
	
	struct i2c_client *client;
	struct device *dev;
	struct oplus_voocphy_operations *ops;

	int irq_gpio;
	int irq;

	int switch1_gpio;
	struct pinctrl *pinctrl;
	struct pinctrl_state *charging_inter_active;
	struct pinctrl_state *charging_inter_sleep;
    struct pinctrl_state *slave_charging_inter_default;

	struct pinctrl_state *charger_gpio_sw_ctrl2_high;
	struct pinctrl_state *charger_gpio_sw_ctrl2_low;
}vooc_manager;

struct oplus_voocphy_operations {
int (*hw_setting)(vooc_manager *chip, int reason);
int (*init_vooc)(vooc_manager *chip);
int (*set_predata)(vooc_manager *chip, u16 val);
int (*set_txbuff)(vooc_manager *chip, u16 val);
int (*get_adapter_info)(vooc_manager *chip);
void (*update_data)(vooc_manager *chip);
int (*get_chg_enable)(vooc_manager *chip, u8 *data);
int (*set_chg_enable)(vooc_manager *chip);
int (*reset_voocphy)(vooc_manager *chip);
int (*reactive_voocphy)(vooc_manager *chip);
void (*set_switch_mode)(vooc_manager *chip, int mode);
void (*send_handshake)(vooc_manager *chip);
int (*clear_intertupts)(vooc_manager *chip);
int (*print_cp_int_exception)(vooc_manager *chip);
int (*get_voocphy_enable)(vooc_manager *chip, unsigned char *val);
};

//vooc flag
#define PULSE_FILTERED_STAT_MASK	BIT(7)
#define NINTH_CLK_ERR_FLAG_MASK		BIT(6)
#define TXSEQ_DONE_FLAG_MASK		BIT(5)
#define ERR_TRANS_DET_FLAG_MASK		BIT(4)
#define TXDATA_WR_FAIL_FLAG_MASK	BIT(3)
#define RX_START_FLAG_MASK			BIT(2)
#define RXDATA_DONE_FLAG_MASK		BIT(1)
#define TXDATA_DONE_FALG_MASK		BIT(0)

#define OPLUS_FASTCHG_STAGE_1   	1
#define	OPLUS_FASTCHG_STAGE_2	  	2
#define OPLUS_FASTCHG_RECOVER_TIME   	(15000/VOOC_FASTCHG_CHECK_TIME)

extern BattMngr_err_code_e oplus_vooc_init_res(UINT8 pmic_index);
extern vooc_manager * oplus_vooc_mg;
extern int oplus_irq_gpio_init(void);
extern int oplus_vooc_chg_sw_ctrl2_parse_dt(void);
extern int oplus_voocphy_get_adapter_request_info(void);
extern void oplus_vooc_set_status_and_notify_ap(FASTCHG_STATUS fastchg_notify_status);
extern int oplus_write_txbuff_error(void);
extern void cpuboost_charge_event(int flag);
extern BattMngr_err_code_e vooc_monitor_timer_stop(VOOC_THREAD_TIMER_TYPE timer_id, LONGLONG duration_in_ms);
extern BattMngr_err_code_e vooc_monitor_timer_start(VOOC_THREAD_TIMER_TYPE timer_id, LONGLONG duration_in_ms);
extern BattMngr_err_code_e oplus_vooc_adapter_commu_with_voocphy(UINT8 pmic_index);
extern bool oplus_vooc_ap_allow_fastchg(void);
extern void oplus_chg_disable_charge(void);
extern int oplus_is_vbus_ok_predata (void);
extern int oplus_voocphy_hw_setting(CHG_PUMP_SETTING_REASON reason);
extern void oplus_voocphy_update_chg_data(void);
extern bool oplus_vooc_wake_voocphy_service_work(VOOCPHY_REQUEST_TYPE request);
extern void init_proc_voocphy_debug(void);
#endif	/*VOOC_BASIC_H*/

