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

#include <linux/cdev.h>
#include "mpu_macro_var.h"

#define mpu_log(...)		printk(KERN_ALERT __VA_ARGS__)

#define BUF_SIZE			32

#define MPU_ADDR_AD0_LOW	0X68
#define MPU_ADDR_AD0_HIGH	0X69

#define MPU_NAME			"mpu6050"
#define	MPU_MAJOR			500
#define	MPU_MINOR			0

#define	MPU_DELAY_WORK_INTERVAL	1


struct hw_s {
    unsigned short max_fifo;
    unsigned char num_reg;
    unsigned short temp_sens;
    short temp_offset;
    unsigned short bank_size;
};


/* Cached chip configuration data.
 * TODO: A lot of these can be handled with a bitmask.
 */
struct chip_cfg_s {
    /* Matches gyro_cfg >> 3 & 0x03 */
    unsigned char gyro_fsr;
    /* Matches accel_cfg >> 3 & 0x03 */
    unsigned char accel_fsr;
    /* Enabled sensors. Uses same masks as fifo_en, NOT pwr_mgmt_2. */
    unsigned char sensors;
    /* Matches config register. */
    unsigned char lpf;
    unsigned char clk_src;
    /* Sample rate, NOT rate divider. */
    unsigned short sample_rate;
    /* Matches fifo_en register. */
    unsigned char fifo_enable;
    /* Matches int enable register. */
    unsigned char int_enable;
    /* 1 if devices on auxiliary I2C bus appear on the primary. */
    unsigned char bypass_mode;
    /* 1 if half-sensitivity.
     * NOTE: This doesn't belong here, but everything else in hw_s is const,
     * and this allows us to save some precious RAM.
     */
    unsigned char accel_half;
    /* 1 if device in low-power accel-only mode. */
    unsigned char lp_accel_mode;
    /* 1 if interrupts are only triggered on motion events. */
    unsigned char int_motion_only;
    /* 1 for active low interrupts. */
    unsigned char active_low_int;
    /* 1 for latched interrupts. */
    unsigned char latched_int;
    /* 1 if DMP is enabled. */
    unsigned char dmp_on;
    /* Ensures that DMP will only be loaded once. */
    unsigned char dmp_loaded;
    /* Sampling rate used when DMP is enabled. */
    unsigned short dmp_sample_rate;
};

struct dmp_s {
    void (*tap_cb)(unsigned char count, unsigned char direction);
    void (*android_orient_cb)(unsigned char orientation);
    unsigned short orient;
    unsigned short feature_mask;
    unsigned short fifo_rate;
    unsigned char packet_length;
};

struct mpu6050_event {
	int	q1;
	int	q2;
	int q3;
	int q4;
};

struct mpu6050_platform_data {
	u16	model;				
	u16	x_plate_ohms;
	u16	max_rt; /* max. resistance above which samples are ignored */
	unsigned long poll_delay; /* delay (in ms) after pen-down event
				     before polling starts */
	unsigned long poll_period; /* time (in ms) between samples */
	int	fuzzx; /* fuzz factor for X, Y and pressure axes */
	int	fuzzy;
	int	fuzzz;

	int	(*get_pendown_state)(void);
	void	(*clear_penirq)(void);		/* If needed, clear 2nd level
						   interrupt source */
	int	(*init_platform_hw)(void);
	void	(*exit_platform_hw)(void);
};


struct mpu6050_device{
	struct cdev	cdev;
	struct class *cdev_class;
	struct i2c_client *client;
	struct chip_cfg_s *chip_cfg;
	struct dmp_s *dmp;
	struct hw_s *hw;
	struct delayed_work	work;
	struct input_dev *input;
	struct mpu6050_event *mpu_ev;
	unsigned char dev_id;
	char phys[32];
	int irq;
	u16 model;
	unsigned long poll_delay;
	unsigned long poll_period;


};

int mpu_i2c_read(unsigned char addr, unsigned char reg, unsigned char len, unsigned char *value);
int mpu_i2c_write(unsigned char addr, unsigned char reg, unsigned char len, unsigned char *value);
int mpu_read_mem(unsigned short mem_addr, unsigned short length, unsigned char *data);
int mpu_write_mem(unsigned short mem_addr, unsigned short length,unsigned char *data);
int mpu_set_gyro_fsr(unsigned short fsr);
int mpu_set_accel_fsr(unsigned char fsr);
int mpu_set_lpf(unsigned short lpf);
int mpu_set_int_latched(unsigned char enable);
int mpu_set_int_status(unsigned char enable);
int mpu_set_bypass(unsigned char bypass_on);
int mpu_set_sample_rate(unsigned short rate);
int mpu_set_sensors(unsigned char sensors);
int mpu_set_dmp_state(unsigned char enable);
int mpu_get_id(void);
int mpu_get_accel_fsr(unsigned char *fsr);
int mpu_get_lpf(unsigned short *lpf);
int mpu_get_sample_rate(unsigned short *rate);
int mpu_reset_fifo(void);
int mpu_configure_fifo(unsigned char sensors);
int mpu_lp_accel_mode(unsigned char rate);
int mpu_load_firmware(unsigned short length, const unsigned char *firmware, unsigned short start_addr, unsigned short sample_rate);
int dmp_set_fifo_rate(unsigned short rate);
int dmp_set_orientation(unsigned short orient);
int dmp_set_tap_thresh(unsigned char axis, unsigned short thresh);
int dmp_set_tap_axes(unsigned char axis);
int dmp_set_tap_count(unsigned char min_taps);
int dmp_set_tap_time(unsigned short time);
int dmp_set_tap_time_multi(unsigned short time);
int dmp_set_shake_reject_thresh(long sf, unsigned short thresh);
int dmp_set_shake_reject_time(unsigned short time);
int dmp_set_shake_reject_timeout(unsigned short time);
int dmp_set_gyro_cal_status(unsigned char enable);
int dmp_set_lp_quat_status(unsigned char enable);
int dmp_set_6x_lp_quat_status(unsigned char enable);
int dmp_set_feature_status(unsigned short mask);
int dmp_load_motion_driver_firmware(void);
unsigned short mpu_row_to_scale(const signed char *row);
unsigned short mpu_orientation_matrix_to_scalar(const signed char *mtx);



#endif

