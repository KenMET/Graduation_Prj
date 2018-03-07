/***********************************************************************************
 * mpu6050 head file for exynos4412
 *
 * Copyright (C) 2018 Ken International Ltd.
 *	Liu Beiming <Ken_processor@outlook.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
***********************************************************************************/


#ifndef __MPU6050_H__
#define __MPU6050_H__

#define SMPLRT_DIV      0x19  
#define CONFIG          0x1A  
#define GYRO_CONFIG     0x1B  
#define ACCEL_CONFIG    0x1C  
#define ACCEL_XOUT_H    0x3B  
#define ACCEL_XOUT_L    0x3C  
#define ACCEL_YOUT_H    0x3D  
#define ACCEL_YOUT_L    0x3E  
#define ACCEL_ZOUT_H    0x3F  
#define ACCEL_ZOUT_L    0x40  
#define TEMP_OUT_H      0x41  
#define TEMP_OUT_L      0x42  
#define GYRO_XOUT_H     0x43  
#define GYRO_XOUT_L     0x44  
#define GYRO_YOUT_H     0x45  
#define GYRO_YOUT_L     0x46  
#define GYRO_ZOUT_H     0x47  
#define GYRO_ZOUT_L     0x48  
#define PWR_MGMT_1      0x6B

#define MPU_NAME		"mpu6050"
#define MPU_MAJOR		500
#define MPU_MINOR		0


struct mpu6050_device{
	struct cdev	cdev;
	struct i2c_client *client;
};


#endif

