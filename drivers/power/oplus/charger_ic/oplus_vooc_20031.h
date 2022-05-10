/*===========================================================================
* Copyright (c) 2020 OPLUS Technologies Incorporated. All Rights Reserved.
* OPLUS Confidential and Proprietary
*
*$DateTime: 2020/08/03 20:38:45 $
*$Author: lizhijie@BSP.CHG.Basic
=============================================================================
EDIT HISTORY

when           who     what, where, why
--------       ---     -----------------------------------------------------------
03/08/2020     lizhijie    Added support for VOOC_PHY
=============================================================================*/

//#include "battmngr_plat_vooc.h"

#define VOOC_DEFAULT_HEAD			SVOOC_INVERT_HEAD

/* 1.R_DET */
#define R_DET_BIT4                              1//0
#define R_DET_BIT5                              1
#define R_DET_BIT6                              0
#define R_DET_BIT7                              0
#define R_DET_BIT8                              0
#define R_DET_BIT9                              0


#define VOOC2_CIRCUIT_R_H_DEF	((R_DET_BIT9 << TX1_DET_BIT1_SHIFT) | (R_DET_BIT8 << TX1_DET_BIT0_SHIFT))
#define VOOC2_CIRCUIT_R_L_DEF ((R_DET_BIT5 << TX0_DET_BIT2_SHIFT) | (R_DET_BIT6 << TX0_DET_BIT3_SHIFT) \
									| (R_DET_BIT7 << TX0_DET_BIT4_SHIFT))
#define NO_VOOC2_CIRCUIT_R_H_DEF	((R_DET_BIT9 << TX1_DET_BIT1_SHIFT) | (R_DET_BIT8 << TX1_DET_BIT0_SHIFT))
#define NO_VOOC2_CIRCUIT_R_L_DEF ((R_DET_BIT4 << TX0_DET_BIT1_SHIFT) | (R_DET_BIT5 << TX0_DET_BIT2_SHIFT) \
									| (R_DET_BIT6 << TX0_DET_BIT3_SHIFT) | (R_DET_BIT7 << TX0_DET_BIT4_SHIFT))
#define BURR_VOOC2_CIRCUIT_R	((R_DET_BIT4 << 4) | (R_DET_BIT5 << 3) | (R_DET_BIT6 << 2) \
									|(R_DET_BIT7 << 1) | (R_DET_BIT8))

#define R_DET_BIT_VOOC30_4                              0
#define R_DET_BIT_VOOC30_5                              1
#define R_DET_BIT_VOOC30_6                              1
#define R_DET_BIT_VOOC30_7                              0
#define R_DET_BIT_VOOC30_8                              0
#define R_DET_BIT_VOOC30_9                              0
#define NO_VOOC3_CIRCUIT_R_H_DEF	((R_DET_BIT_VOOC30_9 << TX1_DET_BIT1_SHIFT) | (R_DET_BIT_VOOC30_8 << TX1_DET_BIT0_SHIFT))
#define NO_VOOC3_CIRCUIT_R_L_DEF ((R_DET_BIT_VOOC30_4 << TX0_DET_BIT1_SHIFT) | (R_DET_BIT_VOOC30_5 << TX0_DET_BIT2_SHIFT) \
									| (R_DET_BIT_VOOC30_6 << TX0_DET_BIT3_SHIFT) | (R_DET_BIT_VOOC30_7 << TX0_DET_BIT4_SHIFT))


/* 2.batt sys array*/
#define BATT_SYS_ARRAY 0x6
#define BATT_SYS_VOL_CUR_PAIR 0X0C

/* 3.allow fastchg temp range and soc rang*/
#define VOOC_LOW_TEMP						0
#define VOOC_HIGH_TEMP						430
#define VOOC_LOW_SOC						0
#define VOOC_HIGH_SOC						90

/* 4.setting fastchg temperature range*/
#define BATT_TEMP_OVER_L						45
#define BATT_TEMP_OVER_H						440

/* 5.setting up overcurrent protection Point */
#define VOOC_OVER_CURRENT_VALUE                      7500

/* 6.low current term condition */
#define LOW_CURR_FULL_T1						120
#define LOW_CURR_FULL_T2						350
#define LOW_CURR_FULL_T3						430

#define RANGE1_LOW_CURR1						-10000
#define RANGE1_LOW_CURR2						1100
#define RANGE1_LOW_CURR3						1000
#define RANGE1_LOW_CURR4						900
#define RANGE1_LOW_CURR5						800

#define RANGE1_FULL_VBAT1						4480
#define RANGE1_FULL_VBAT2						4475
#define RANGE1_FULL_VBAT3						4460
#define RANGE1_FULL_VBAT4						4450
#define RANGE1_FULL_VBAT5						4450

#define RANGE2_LOW_CURR1						-10000
#define RANGE2_LOW_CURR2						1200
#define RANGE2_LOW_CURR3						1100
#define RANGE2_LOW_CURR4						1000
#define RANGE2_LOW_CURR5						900

#define RANGE2_FULL_VBAT1						4480
#define RANGE2_FULL_VBAT2						4475
#define RANGE2_FULL_VBAT3						4460
#define RANGE2_FULL_VBAT4						4450
#define RANGE2_FULL_VBAT5						4450
#define LOW_CURRENT_TERM_LO                     900
#define LOW_CURRENT_TERM_VBAT_LO                4450
#define LOW_CURRENT_TERM_HI                     1000
#define LOW_CURRENT_TERM_VBAT_HI                4470

/* 7.adjust current according to battery voltage */
#define CC_VALUE_3_0C                           65
#define CC_VALUE_2_9C                           55
#define CC_VALUE_2_8C                           50
#define CC_VALUE_2_7C                           40
#define CC_VALUE_2_6C							30
#define CC_VALUE_2_5_0C                         20
#define CC_VALUE_2_4C							10


#define CC_2_95C_THD							4220
#define CC_VALUE_2_95C							55

#define CC_2_5C_THD								4224
#define CC_VALUE_2_5C							50

#define CC_2_0C_THD								4300
#define CC_VALUE_2_0C							CC_VALUE_3_0C//No effect

#define CC_LOWTEMP_1_1C_THD						4200
#define CC_LOWTEMP_VALUE_1_1C					CC_VALUE_3_0C//No effect

#define CC_LOWTEMP_1_0C_THD						4450
#define CC_LOWTEMP_VALUE_1_0C					CC_VALUE_3_0C//No effect

/* 8.adapter copydat vol & curr thd*/
#define BATT_PWD_CURR_THD1						4300
#define BATT_PWD_VOL_THD1						4304

/* 9.Battery Charging Full Voltage */
#define BAT_FULL_1TIME_THD                      (VBATT_4480_MV + 10)	//4510
#define BAT_FULL_NTIME_THD                      VBATT_4480_MV	//4500

#define BAT_FULL2_1TIME_THD                     4510
#define BAT_FULL2_NTIME_THD                     4500

/*littile cold temp range set charger current*/
#define TEMP0_2_TEMP45_FULL_VOLTAGE			4430


/* 10. cool temp range set charger current */
#define TEMP45_2_TEMP115_FULL_VOLTAGE			4430
#define TEMP45_2_TEMP115_CURR					30
#define TEMP45_2_TEMP115_2A_VOLTAGE				4180
#define TEMP45_2_TEMP115_2A						15

/* 11. little cool temp range set charger current */
#define TEMP115_2_TEMP160_CURR					50

/* 12. according soc set charger current*/
#define SOC75_2_SOC100_CURR						20
#define SOC50_2_SOC75_CURR						45
#define SOC_HIGH_LEVEL							75
#define SOC_MID_LEVEL							50
#define VOOC_SOC_HIGH							75

/* 13. adjust current according to battery temp*/
#define VOOC_LOGIC_SELECT_BATT_TEMP			320
#define VOOC_LITTLE_COLD_TEMP					50
#define VOOC_COOL_TEMP						120
#define VOOC_LITTLE_COOL_TEMP				160
#define VOOC_NORMAL_TEMP					430
#define VOOC_WARM_TEMP						530

#define VOOC_LITTLE_COOL_TO_NORMAL_TEMP			180
#define VOOC_STRATEGY_NORMAL_CURRENT			VBATT_3000_MA
#define VOOC_NORMAL_TO_LITTLE_COOL_CURRENT		25
#define VOOC_BATT_OVER_HIGH_TEMP				440
#define VOOC_BATT_OVER_LOW_TEMP					0

#define VOOC_STRATEGY1_BATT_HIGH_TEMP0			385
#define VOOC_STRATEGY1_BATT_HIGH_TEMP1			395
#define VOOC_STRATEGY1_BATT_HIGH_TEMP2			420
#define VOOC_STRATEGY1_BATT_LOW_TEMP2			430
#define VOOC_STRATEGY1_BATT_LOW_TEMP1			410
#define VOOC_STRATEGY1_BATT_LOW_TEMP0			385
#define VOOC_STRATEGY_BATT_NATRUE_TEMP			383

#define VOOC_STRATEGY1_HIGH_CURRENT0			30
#define VOOC_STRATEGY1_HIGH_CURRENT1			30
#define VOOC_STRATEGY1_HIGH_CURRENT2			20
#define VOOC_STRATEGY1_LOW_CURRENT2				20
#define VOOC_STRATEGY1_LOW_CURRENT1				30
#define VOOC_STRATEGY1_LOW_CURRENT0				30

#define VOOC_STRATEGY2_BATT_UP_TEMP1			385
#define VOOC_STRATEGY2_BATT_UP_DOWN_TEMP2		410
#define VOOC_STRATEGY2_BATT_UP_TEMP3			395
#define VOOC_STRATEGY2_BATT_UP_DOWN_TEMP4		430
#define VOOC_STRATEGY2_BATT_UP_TEMP5			420
#define VOOC_STRATEGY2_BATT_UP_TEMP6			430
#define VOOC_STRATEGY2_BATT_UP_DOWN_TEMP7		385
#define VOOC_STRATEGY2_HIGH0_CURRENT			30
#define VOOC_STRATEGY2_HIGH1_CURRENT			30
#define VOOC_STRATEGY2_HIGH2_CURRENT			20
#define VOOC_STRATEGY2_HIGH3_CURRENT			20
#define VOOC_TEMP_OVER_COUNTS					2

/* 14.Fast charging timeout */
#define FASTCHG_TOTAL_TIME			100000
#define FASTCHG_3C_TIMEOUT                      3600
#define FASTCHG_2_9C_TIMEOUT                    3000
#define FASTCHG_2_8C_TIMEOUT                    2100

#define FASTCHG_3000MA_TIMEOUT                    72000
#define FASTCHG_2500MA_TIMEOUT                    67800
#define FASTCHG_2000MA_TIMEOUT                    60000


/* 15. ap request current_max & current_min*/
#define AP_CURRENT_REQUEST_MAX					30
#define AP_CURRENT_REQUEST_MIN					10

/* 16. Fast charging hardware monitor arguements setup*/
#define VBATT_HW_OV_THD							(BAT_FULL_1TIME_THD + 50)
#define VBATT_HW_OV_ANA							(BAT_FULL_1TIME_THD + 50)
#define BATT_THOT_85C							0x05e5
#define CONN_THOT_85C							0x05e5

/* 17. force charger current for old vooc2.0*/
#define FORCE2A_VBATT_REPORT                    58
#define FORCE3A_VBATT_REPORT                    57



