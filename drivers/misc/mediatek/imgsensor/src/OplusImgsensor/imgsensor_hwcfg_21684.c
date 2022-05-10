/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef __IMGSENSOR_HWCFG_21684_CTRL_H__
#define __IMGSENSOR_HWCFG_21684_CTRL_H__
#include "imgsensor_hwcfg_custom.h"

struct IMGSENSOR_SENSOR_LIST
    gimgsensor_sensor_list_21684[MAX_NUM_OF_SUPPORT_SENSOR] = {
#if defined(OV64B40_MIPI_RAW)
{OV64B40_SENSOR_ID, SENSOR_DRVNAME_OV64B40_MIPI_RAW, OV64B40_MIPI_RAW_SensorInit},
#endif
#if defined(S5KJN1_MIPI_RAW)
	{S5KJN1_SENSOR_ID, SENSOR_DRVNAME_S5KJN1_MIPI_RAW, S5KJN1_MIPI_RAW_SensorInit},
#endif
#if defined(HI1634Q_MIPI_RAW)
	{HI1634Q_SENSOR_ID, SENSOR_DRVNAME_HI1634Q_MIPI_RAW, HI1634Q_MIPI_RAW_SensorInit},
#endif
#if defined(S5K3P9SPSUNNY_MIPI_RAW)
	{S5K3P9SPSUNNY_SENSOR_ID, SENSOR_DRVNAME_S5K3P9SPSUNNY_MIPI_RAW, S5K3P9SPSUNNY_MIPI_RAW_SensorInit},
#endif
#if defined(GC02M1HLT_MIPI_RAW)
	{GC02M1HLT_SENSOR_ID, SENSOR_DRVNAME_GC02M1HLT_MIPI_RAW, GC02M1HLT_MIPI_RAW_SensorInit},
#endif
#if defined(GC02M1SHINE_MIPI_RAW)
	{GC02M1SHINE_SENSOR_ID, SENSOR_DRVNAME_GC02M1SHINE_MIPI_RAW, GC02M1SHINE_MIPI_RAW_SensorInit},
#endif
#if defined(GC02M1BSY_MIPI_RAW)
	{GC02M1BSY_SENSOR_ID, SENSOR_DRVNAME_GC02M1BSY_MIPI_RAW, GC02M1BSY_MIPI_RAW_SensorInit},
#endif

#if defined(OV02B1B_MIPI_MONO)
	{OV02B1B_SENSOR_ID, SENSOR_DRVNAME_OV02B1B_MIPI_MONO, OV02B1B_MIPI_MONO_SensorInit},
#endif
    /* ADD sensor driver before this line */
    {0, {0}, NULL}, /* end of list */
};

struct IMGSENSOR_HW_CFG imgsensor_custom_config_21684[] = {
	{
		IMGSENSOR_SENSOR_IDX_MAIN,
		IMGSENSOR_I2C_DEV_0,
		{
			{IMGSENSOR_HW_PIN_MCLK,  IMGSENSOR_HW_ID_MCLK},
			{IMGSENSOR_HW_PIN_AVDD,  IMGSENSOR_HW_ID_WL2864},
			{IMGSENSOR_HW_PIN_DOVDD, IMGSENSOR_HW_ID_WL2864},
			{IMGSENSOR_HW_PIN_DVDD,  IMGSENSOR_HW_ID_WL2864},
			{IMGSENSOR_HW_PIN_AFVDD, IMGSENSOR_HW_ID_WL2864},
			{IMGSENSOR_HW_PIN_RST,   IMGSENSOR_HW_ID_GPIO},
			{IMGSENSOR_HW_PIN_NONE,  IMGSENSOR_HW_ID_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB,
		IMGSENSOR_I2C_DEV_1,
		{
			{IMGSENSOR_HW_PIN_MCLK,  IMGSENSOR_HW_ID_MCLK},
			{IMGSENSOR_HW_PIN_AVDD,  IMGSENSOR_HW_ID_WL2864},
			{IMGSENSOR_HW_PIN_DOVDD, IMGSENSOR_HW_ID_WL2864},
			{IMGSENSOR_HW_PIN_DVDD,  IMGSENSOR_HW_ID_WL2864},
			{IMGSENSOR_HW_PIN_RST,   IMGSENSOR_HW_ID_GPIO},
			{IMGSENSOR_HW_PIN_NONE, IMGSENSOR_HW_ID_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_MAIN2,
		IMGSENSOR_I2C_DEV_1,
		{

			{IMGSENSOR_HW_PIN_MCLK,  IMGSENSOR_HW_ID_MCLK},
 			{IMGSENSOR_HW_PIN_AVDD,  IMGSENSOR_HW_ID_WL2864},
 			{IMGSENSOR_HW_PIN_DOVDD, IMGSENSOR_HW_ID_WL2864},
			{IMGSENSOR_HW_PIN_RST,   IMGSENSOR_HW_ID_GPIO},
#ifdef MIPI_SWITCH
			{IMGSENSOR_HW_PIN_MIPI_SWITCH_EN, IMGSENSOR_HW_ID_GPIO},
			{IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL,
				IMGSENSOR_HW_ID_GPIO},
#endif
			{IMGSENSOR_HW_PIN_NONE,  IMGSENSOR_HW_ID_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB2,
		IMGSENSOR_I2C_DEV_3,
		{
			{IMGSENSOR_HW_PIN_MCLK,  IMGSENSOR_HW_ID_MCLK},
			{IMGSENSOR_HW_PIN_AVDD,  IMGSENSOR_HW_ID_WL2864},
			{IMGSENSOR_HW_PIN_DOVDD, IMGSENSOR_HW_ID_WL2864},
			{IMGSENSOR_HW_PIN_DVDD,  IMGSENSOR_HW_ID_NONE},
			{IMGSENSOR_HW_PIN_PDN,   IMGSENSOR_HW_ID_NONE},
			{IMGSENSOR_HW_PIN_RST,   IMGSENSOR_HW_ID_GPIO},
			{IMGSENSOR_HW_PIN_NONE,  IMGSENSOR_HW_ID_NONE},
		},
	},
    {IMGSENSOR_SENSOR_IDX_NONE}
};


struct IMGSENSOR_HW_POWER_SEQ sensor_power_sequence_21684[] = {
#if defined(OV64B40_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV64B40_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1100, 1},
			{DOVDD, Vol_1800, 1},
			{AFVDD, Vol_2800, 2},
			{SensorMCLK, Vol_High, 0},
			{RST, Vol_High, 5},
		},
	},
#endif

#if defined(S5KJN1_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5KJN1_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1050, 1},
			{AVDD, Vol_2800, 5},
			{AFVDD, Vol_2800, 0},
			{RST, Vol_High, 2},
			{SensorMCLK, Vol_High, 2}
		},
	},
#endif
#if defined(HI1634Q_MIPI_RAW)
	{
		SENSOR_DRVNAME_HI1634Q_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 1},
			{SensorMCLK, Vol_High, 5},
			{RST, Vol_High, 5},
		},
	},
#endif

#if defined(S5K3P9SPSUNNY_MIPI_RAW)
		{
			SENSOR_DRVNAME_S5K3P9SPSUNNY_MIPI_RAW,
			{
				{RST, Vol_Low, 1},
				{AVDD, Vol_2800, 0},
				{DVDD, Vol_1100, 0},
				{DOVDD, Vol_1800, 1},
				{RST, Vol_High, 2},
				{SensorMCLK, Vol_High, 1},

			},
		},
#endif

#if defined(GC02M1HLT_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC02M1HLT_MIPI_RAW,
		{
			{RST, Vol_Low, 3},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{RST, Vol_High, 5},
			{SensorMCLK, Vol_High, 3},
		},
	},
#endif

#if defined(GC02M1SHINE_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC02M1SHINE_MIPI_RAW,
		{
			{RST, Vol_Low, 3},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{RST, Vol_High, 5},
			{SensorMCLK, Vol_High, 3},
		},
	},
#endif

#if defined(GC02M1BSY_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC02M1BSY_MIPI_RAW,
		{
			{RST, Vol_Low, 3},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{RST, Vol_High, 5},
			{SensorMCLK, Vol_High, 3},
		},
	},
#endif

#if defined(OV02B1B_MIPI_MONO)
	{
		SENSOR_DRVNAME_OV02B1B_MIPI_MONO,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 6},
			{SensorMCLK, Vol_High, 5},
			{RST, Vol_High, 2}
		},
	},
#endif
    /* add new sensor before this line */
    {NULL,},
};
#endif
