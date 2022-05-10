/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#ifndef __CAM_CAL_LIST_H
#define __CAM_CAL_LIST_H
#include <linux/i2c.h>

#define DEFAULT_MAX_EEPROM_SIZE_8K 0x2000

typedef unsigned int (*cam_cal_cmd_func) (struct i2c_client *client,
	unsigned int addr, unsigned char *data, unsigned int size);

/* zhaopengfei@ODM.Camera.Drv 20210622 for s5kjn1 crosstalk data*/
struct stCAM_CAL_FUNC_STRUCT {
        unsigned int sensorID;
        unsigned int slaveID;
        cam_cal_cmd_func readCamCalData;
};


struct stCAM_CAL_LIST_STRUCT {
	unsigned int sensorID;
	unsigned int slaveID;
	cam_cal_cmd_func readCamCalData;
	unsigned int maxEepromSize;
	cam_cal_cmd_func writeCamCalData;
};


unsigned int cam_cal_get_sensor_list
		(struct stCAM_CAL_LIST_STRUCT **ppCamcalList);

/* zhaopengfei@ODM.Camera.Drv 20210622 for s5kjn1 crosstalk data*/
unsigned int cam_cal_get_func_list(struct stCAM_CAL_FUNC_STRUCT **ppCamcalFuncList);


#endif				/* __CAM_CAL_LIST_H */
