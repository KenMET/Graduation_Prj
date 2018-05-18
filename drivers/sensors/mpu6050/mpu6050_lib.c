/***********************************************************************************
 * mpu6050 lib for exynos4412
 *
 * Copyright (C) 2018 Ken International Ltd.
 *	Liu Beiming <Ken_processor@outlook.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
***********************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>

#include <plat/exynos4.h>
#include <plat/gpio-cfg.h>

#include <mach/gpio.h>
#include <mach/regs-gpio.h>

#include <asm/io.h>

#include "mpu6050_lib.h"
#include "dmpKey.h"
#include "dmpmap.h"

int mpu_i2c_read(struct mpu6050_device *mpu6050, u8 addr, u8 reg, u8 len, u8 *value)
{
	u8 txbuf[1] = { reg };
	u8 rxbuf[BUF_SIZE] = { 0 };
	struct i2c_msg msgs[] = {
			{
					.addr	= addr, 
					.flags	= 0,	
					.len	= 1,
					.buf	= txbuf,
			},
			{
					.addr	= addr,
					.flags	= I2C_M_RD,
					.len	= len,
					.buf	= rxbuf,
			},
	};
	int ret, k;
	ret = i2c_transfer(mpu6050->client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0)
	{
		mpu_log("read reg (0x%02x) error, %d\n", reg, ret);
	}
	else
	{
		for(k=0; k<len; k++)
			*(value+k) = rxbuf[k];
	}
	return ret;
}

int mpu_i2c_write(struct mpu6050_device *mpu6050, unsigned char addr, unsigned char reg, unsigned char len, unsigned char *value)  
{
	u8 txbuf[BUF_SIZE] = { 0 };
	struct i2c_msg msgs[] = {
			{
					.addr	= addr, 
					.flags	= 0,	
					.len	= len + 1,
					.buf	= txbuf,
			},
	};
	int ret, k;

	txbuf[0] = reg;
	for(k=0; k<len; k++)
		txbuf[k+1] = *(value+k);
	
	ret = i2c_transfer(mpu6050->client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0)
	{
		mpu_log("write reg (0x%02x) error, %d\n", reg, ret);
	}
	return ret;

}

int mpu_read_mem(struct mpu6050_device *mpu6050, unsigned short mem_addr, unsigned short length, unsigned char *data)
{
	int ret = -EINVAL;
	unsigned char tmp[2];

	if (!data)
		return -1;
	if (!mpu6050->chip_cfg->sensors)
		return -1;

	tmp[0] = (unsigned char)(mem_addr >> 8);
	tmp[1] = (unsigned char)(mem_addr & 0xFF);

	/* Check bank boundaries. */
	if (tmp[1] + length > DMP_MEM_BLANK_SIZE)
		return -1;

	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_BANK_SEL, 2, tmp);
	if (ret < 0) {	
		mpu_log("write bank_sel not ok");	
		return ret;  
	}

	ret = mpu_i2c_read(mpu6050, mpu6050->dev_id, MPU_MEM_R_W, length, data);
	if (ret < 0) {	
		mpu_log("read mem_r_w not ok");	
		return ret;  
	}

	return 0;
}

int mpu_write_mem(struct mpu6050_device *mpu6050, unsigned short mem_addr, unsigned short length,
        unsigned char *data)
{
	int ret = -EINVAL;
	unsigned char tmp[2];

	if (!data)
		return -1;
	if (!mpu6050->chip_cfg->sensors)
		return -1;

	tmp[0] = (unsigned char)(mem_addr >> 8);
	tmp[1] = (unsigned char)(mem_addr & 0xFF);

	/* Check bank boundaries. */
	if (tmp[1] + length > DMP_MEM_BLANK_SIZE)
		return -1;

	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_BANK_SEL, 2, tmp);
	if (ret < 0) {	
		mpu_log("write bank_sel not ok");	
		return ret;  
	}

	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_MEM_R_W, length, data);
	if (ret < 0) {	
		mpu_log("write mem_r_w not ok");	
		return ret;  
	}

	return 0;
}

int mpu_set_gyro_fsr(struct mpu6050_device *mpu6050, unsigned short fsr)
{
	int ret = -EINVAL;
	unsigned char data;

	if (!(mpu6050->chip_cfg->sensors))
		return -1;

	switch (fsr) {
		case 250:	data = INV_FSR_250DPS << 3;		break;
		case 500:	data = INV_FSR_500DPS << 3;		break;
		case 1000:	data = INV_FSR_1000DPS << 3;	break;
		case 2000:	data = INV_FSR_2000DPS << 3;	break;
		default:	return -1;
	}

	if (mpu6050->chip_cfg->gyro_fsr == (data >> 3))
		return 0;

	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_GYRO_CONFIG, 1, &data);
	if (ret < 0) {	
		mpu_log("write gyro_cfg not ok");	
		return ret;  
	}

	mpu6050->chip_cfg->gyro_fsr = data >> 3;
	return 0;
}

int mpu_set_accel_fsr(struct mpu6050_device *mpu6050, unsigned char fsr)
{
	int ret = -EINVAL;
	unsigned char data;

	if (!(mpu6050->chip_cfg->sensors))
		return -1;

	switch (fsr) {
		case 2:		data = INV_FSR_2G << 3;		break;
		case 4:		data = INV_FSR_4G << 3;		break;
		case 8:		data = INV_FSR_8G << 3;		break;
		case 16:		data = INV_FSR_16G << 3;	break;
		default:	return -1;
	}

	if (mpu6050->chip_cfg->accel_fsr == (data >> 3))
		return 0;

	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_ACCEL_CONFIG, 1, &data);
	if (ret < 0) {	
		mpu_log("write gyro_cfg not ok");	
		return ret;  
	}

	mpu6050->chip_cfg->accel_fsr = data >> 3;

	return 0;
}

int mpu_set_lpf(struct mpu6050_device *mpu6050, unsigned short lpf)
{
	int ret = -EINVAL;
	unsigned char data;

	if (!(mpu6050->chip_cfg->sensors))
		return -1;

	if (lpf >= 188)				data = INV_FILTER_188HZ;
	else if (lpf >= 98)			data = INV_FILTER_98HZ;
	else if (lpf >= 42)			data = INV_FILTER_42HZ;
	else if (lpf >= 20)			data = INV_FILTER_20HZ;
	else if (lpf >= 10)			data = INV_FILTER_10HZ;
	else									data = INV_FILTER_5HZ;

	if (mpu6050->chip_cfg->lpf == data)
		return 0;

	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_LPF, 1, &data);
	if (ret < 0) {	
		mpu_log("write lpf not ok");	
		return ret;  
	}

	mpu6050->chip_cfg->lpf = data;
	return 0;
}

int mpu_set_int_latched(struct mpu6050_device *mpu6050, unsigned char enable)
{
	int ret = -EINVAL;
	unsigned char tmp;
	if (mpu6050->chip_cfg->latched_int == enable)
		return 0;

	if (enable)
		tmp = BIT_LATCH_EN | BIT_ANY_RD_CLR;
	else
		tmp = 0;

	if (mpu6050->chip_cfg->bypass_mode)
		tmp |= BIT_BYPASS_EN;
	if (mpu6050->chip_cfg->active_low_int)
		tmp |= BIT_ACTL;

	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_INT_PIN_CONFIG, 1, &tmp);
	if (ret < 0) {	
		mpu_log("write int_pin_cfg not ok");	
		return ret;  
	}

	mpu6050->chip_cfg->latched_int = enable;
	return 0;
}

int mpu_set_int_status(struct mpu6050_device *mpu6050, unsigned char enable)
{
	int ret = -EINVAL;
	unsigned char tmp;

	if (mpu6050->chip_cfg->dmp_on) {
		if (enable)
			tmp = BIT_DMP_INT_EN;
		else
			tmp = 0x00;

		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_INT_EN, 1, &tmp);
		if (ret < 0) {	
			mpu_log("write int_enable not ok");	
			return ret;  
		}

		mpu6050->chip_cfg->int_enable = tmp;
	}else {
		if (!mpu6050->chip_cfg->sensors)
			return -1;
		if (enable && mpu6050->chip_cfg->int_enable)
			return 0;
		if (enable)
			tmp = BIT_DATA_RDY_EN;
		else
			tmp = 0x00;

		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_INT_EN, 1, &tmp);
		if (ret < 0) {	
			mpu_log("write int_enable not ok");	
			return ret;  
		}

		mpu6050->chip_cfg->int_enable = tmp;
	}
	return 0;
}

int mpu_set_bypass(struct mpu6050_device *mpu6050, unsigned char bypass_on)
{
	int ret = -EINVAL;
	unsigned char tmp;

	if (mpu6050->chip_cfg->bypass_mode == bypass_on)
		return 0;

	if (bypass_on) {
		ret = mpu_i2c_read(mpu6050, mpu6050->dev_id, MPU_USER_CTRL, 1, &tmp);
		if (ret < 0) {	
			mpu_log("read user_ctrl not ok");	
			return ret;  
		}
		tmp &= ~BIT_AUX_IF_EN;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_USER_CTRL, 1, &tmp);
		if (ret < 0) {	
			mpu_log("write user_ctrl not ok");	
			return ret;  
		}

		msleep(3);

		tmp = BIT_BYPASS_EN;
		if (mpu6050->chip_cfg->active_low_int)
			tmp |= BIT_ACTL;
		if (mpu6050->chip_cfg->latched_int)
			tmp |= BIT_LATCH_EN | BIT_ANY_RD_CLR;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_INT_PIN_CONFIG, 1, &tmp);
		if (ret < 0) {	
			mpu_log("write int_pin_cfg not ok");	
			return ret;  
		}
	} else {
		/* Enable I2C master mode if compass is being used. */
		ret = mpu_i2c_read(mpu6050, mpu6050->dev_id, MPU_USER_CTRL, 1, &tmp);
		if (ret < 0) {	
			mpu_log("read user_ctrl not ok");	
			return ret;  
		}
		if (mpu6050->chip_cfg->sensors & INV_XYZ_COMPASS)
			tmp |= BIT_AUX_IF_EN;
		else
			tmp &= ~BIT_AUX_IF_EN;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_USER_CTRL, 1, &tmp);
		if (ret < 0) {	
			mpu_log("write user_ctrl not ok");	
			return ret;  
		}

		msleep(3);

		if (mpu6050->chip_cfg->active_low_int)
			tmp = BIT_ACTL;
		else
			tmp = 0;
		if (mpu6050->chip_cfg->latched_int)
			tmp |= BIT_LATCH_EN | BIT_ANY_RD_CLR;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_INT_PIN_CONFIG, 1, &tmp);
		if (ret < 0) {	
			mpu_log("write int_pin_cfg not ok");	
			return ret;  
		}
	}
	mpu6050->chip_cfg->bypass_mode = bypass_on;

	return 0;
}

int mpu_set_sample_rate(struct mpu6050_device *mpu6050, unsigned short rate)
{
	int ret = -EINVAL;
	unsigned char data;

	if (!(mpu6050->chip_cfg->sensors))
		return -1;

	if (mpu6050->chip_cfg->dmp_on)
		return -1;
	else {
		if (mpu6050->chip_cfg->lp_accel_mode) {
			if (rate && (rate <= 40)) {
				/* Just stay in low-power accel mode. */
				mpu_lp_accel_mode(mpu6050, rate);
				return 0;
			}
			/* Requested rate exceeds the allowed frequencies in LP accel mode,
			* switch back to full-power mode.
			*/
			mpu_lp_accel_mode(mpu6050, 0);
		}
		if (rate < 4)
			rate = 4;
		else if (rate > 1000)
			rate = 1000;

		data = 1000 / rate - 1;

		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_RATE_DIV, 1, &data);
		if (ret < 0) {	
			mpu_log("write rate_div not ok");	
			return ret;  
		}

		mpu6050->chip_cfg->sample_rate = 1000 / (1 + data);

		/* Automatically set LPF to 1/2 sampling rate. */
		mpu_set_lpf(mpu6050, mpu6050->chip_cfg->sample_rate >> 1);
		return 0;
	}
}

int mpu_set_sensors(struct mpu6050_device *mpu6050, unsigned char sensors)
{
	int ret = -EINVAL;
	unsigned char data;

	if (sensors & INV_XYZ_GYRO)
		data = INV_CLK_PLL;
	else if (sensors)
		data = 0;
	else
		data = BIT_SLEEP;
	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_PWR_MGMT1, 1, &data);
	if (ret < 0) {	
		mpu_log("write pwr_mgmt_1 not ok");	
		return ret;  
	}

	mpu6050->chip_cfg->clk_src = data & ~BIT_SLEEP;

	data = 0;
	if (!(sensors & INV_X_GYRO))
		data |= BIT_STBY_XG;
	if (!(sensors & INV_Y_GYRO))
		data |= BIT_STBY_YG;
	if (!(sensors & INV_Z_GYRO))
		data |= BIT_STBY_ZG;
	if (!(sensors & INV_XYZ_ACCEL))
		data |= BIT_STBY_XYZA;
	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_PWR_MGMT2, 1, &data);
	if (ret < 0) {
		mpu6050->chip_cfg->sensors = 0;
		mpu_log("write pwr_mgmt_2 not ok");	
		return ret;  
	}

	if (sensors && (sensors != INV_XYZ_ACCEL))
		mpu_set_int_latched(mpu6050, 0);/* Latched interrupts only used in LP accel mode. */

	mpu6050->chip_cfg->sensors = sensors;
	mpu6050->chip_cfg->lp_accel_mode = 0;
	mdelay(50);
	return 0;
}

int mpu_set_dmp_state(struct mpu6050_device *mpu6050, unsigned char enable)
{
	int ret = -EINVAL;
	unsigned char tmp;
	if (mpu6050->chip_cfg->dmp_on == enable)
		return 0;

	if (enable) {
		if (!mpu6050->chip_cfg->dmp_loaded)
			return -1;
		/* Disable data ready interrupt. */
		mpu_set_int_status(mpu6050, 0);
		/* Disable bypass mode. */
		mpu_set_bypass(mpu6050, 0);
		/* Keep constant sample rate, FIFO rate controlled by dmp-> */
		mpu_set_sample_rate(mpu6050, mpu6050->chip_cfg->dmp_sample_rate);
		/* Remove FIFO elements. */
		tmp = 0;

		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_FIFO_EN, 1, &tmp);
		if (ret < 0) {	
			mpu_log("write fifo_en not ok");	
			return ret;  
		}

		mpu6050->chip_cfg->dmp_on = 1;
		/* Enable DMP interrupt. */
		mpu_set_int_status(mpu6050, 1);
		mpu_reset_fifo(mpu6050);
	} else {
		/* Disable DMP interrupt. */
		mpu_set_int_status(mpu6050, 0);
		/* Restore FIFO settings. */
		tmp = mpu6050->chip_cfg->fifo_enable;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_FIFO_EN, 1, &tmp);
		if (ret < 0) {	
			mpu_log("write fifo_en not ok");	
			return ret;  
		}
		mpu6050->chip_cfg->dmp_on = 0;
		mpu_reset_fifo(mpu6050);
	}
	return 0;
}

int mpu_get_id(struct mpu6050_device *mpu6050)
{
	int ret = -EINVAL;
	unsigned char val[1] = { 0 };

	ret = mpu_i2c_read(mpu6050, MPU_ADDR_AD0_LOW, MPU_WHO_AM_I, 1, val);
	if (ret < 0){  
		mpu_log("MPU_ADDR_AD0_LOW read not ok");
		ret = mpu_i2c_read(mpu6050, MPU_ADDR_AD0_HIGH, MPU_WHO_AM_I, 1, val);
		if (ret < 0){
			mpu_log("both addr read not ok"); 
			return ret; 
		} 
    }
	return val[0];
}

int mpu_get_gyro_fsr(struct mpu6050_device *mpu6050, unsigned short *fsr)
{
	switch (mpu6050->chip_cfg->gyro_fsr) {
		case INV_FSR_250DPS:
			fsr[0] = 250;
			break;
		case INV_FSR_500DPS:
			fsr[0] = 500;
			break;
		case INV_FSR_1000DPS:
			fsr[0] = 1000;
			break;
		case INV_FSR_2000DPS:
			fsr[0] = 2000;
			break;
		default:
			fsr[0] = 0;
			break;
	}
	return 0;
}

int mpu_get_accel_fsr(struct mpu6050_device *mpu6050, unsigned char *fsr)
{
	switch (mpu6050->chip_cfg->accel_fsr) {
		case INV_FSR_2G:
			fsr[0] = 2;
			break;
		case INV_FSR_4G:
			fsr[0] = 4;
			break;
		case INV_FSR_8G:
			fsr[0] = 8;
			break;
		case INV_FSR_16G:
			fsr[0] = 16;
			break;
		default:
			return -1;
	}
	if (mpu6050->chip_cfg->accel_half)
		fsr[0] <<= 1;
	return 0;
}

int mpu_get_lpf(struct mpu6050_device *mpu6050, unsigned short *lpf)
{
	switch (mpu6050->chip_cfg->lpf) {
		case INV_FILTER_188HZ:
			lpf[0] = 188;
			break;
		case INV_FILTER_98HZ:
			lpf[0] = 98;
			break;
		case INV_FILTER_42HZ:
			lpf[0] = 42;
			break;
		case INV_FILTER_20HZ:
			lpf[0] = 20;
			break;
		case INV_FILTER_10HZ:
			lpf[0] = 10;
			break;
		case INV_FILTER_5HZ:
			lpf[0] = 5;
			break;
		case INV_FILTER_256HZ_NOLPF2:
		case INV_FILTER_2100HZ_NOLPF:
		default:
			lpf[0] = 0;
			break;
	}
	return 0;
}

int mpu_get_sample_rate(struct mpu6050_device *mpu6050, unsigned short *rate)
{
	if (mpu6050->chip_cfg->dmp_on)
		return -1;
	else
		rate[0] = mpu6050->chip_cfg->sample_rate;
	return 0;
}

int mpu_get_fifo_config(struct mpu6050_device *mpu6050, unsigned char *sensors)
{
	sensors[0] = mpu6050->chip_cfg->fifo_enable;
	return 0;
}

int mpu_reset_fifo(struct mpu6050_device *mpu6050)
{
	int ret = -EINVAL;
	unsigned char data;

	if (!(mpu6050->chip_cfg->sensors))
		return -1;

	data = 0;
	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_INT_EN, 1, &data);
	if (ret < 0) {	
		mpu_log("write int_enable not ok");	
		return ret;  
	}
	
	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_FIFO_EN, 1, &data);
	if (ret < 0) {	
		mpu_log("write fifo_en not ok");	
		return ret;  
	}
	
	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_USER_CTRL, 1, &data);
	if (ret < 0) {	
		mpu_log("write user_ctrl not ok");	
		return ret;  
	}

	if (mpu6050->chip_cfg->dmp_on) {
		data = BIT_FIFO_RST | BIT_DMP_RST;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_USER_CTRL, 1, &data);
		if (ret < 0) {	
			mpu_log("write user_ctrl not ok");	
			return ret;  
		}

		msleep(50);

		data = BIT_DMP_EN | BIT_FIFO_EN;
		if (mpu6050->chip_cfg->sensors & INV_XYZ_COMPASS)
			data |= BIT_AUX_IF_EN;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_USER_CTRL, 1, &data);
		if (ret < 0) {	
			mpu_log("write user_ctrl not ok");	
			return ret;  
		}

		if (mpu6050->chip_cfg->int_enable)
			data = BIT_DMP_INT_EN;
		else
			data = 0;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_INT_EN, 1, &data);
		if (ret < 0) {	
			mpu_log("write int_enable not ok");	
			return ret;  
		}

		data = 0;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_FIFO_EN, 1, &data);
		if (ret < 0) {	
			mpu_log("write int_enable not ok");	
			return ret;  
		}
	}else {
		data = BIT_FIFO_RST;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_USER_CTRL, 1, &data);
		if (ret < 0) {	
			mpu_log("write user_ctrl not ok");	
			return ret;  
		}

		if (mpu6050->chip_cfg->bypass_mode || !(mpu6050->chip_cfg->sensors & INV_XYZ_COMPASS))
			data = BIT_FIFO_EN;
		else
			data = BIT_FIFO_EN | BIT_AUX_IF_EN;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_USER_CTRL, 1, &data);
		if (ret < 0) {	
			mpu_log("write user_ctrl not ok");	
			return ret;  
		}

		msleep(50);
		if (mpu6050->chip_cfg->int_enable)
			data = BIT_DATA_RDY_EN;
		else
			data = 0;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_INT_EN, 1, &data);
		if (ret < 0) {	
			mpu_log("write int_enable not ok");	
			return ret;  
		}
		
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_FIFO_EN, 1, &mpu6050->chip_cfg->fifo_enable);
		if (ret < 0) {	
			mpu_log("write fifo_en not ok");	
			return ret;  
		}
	}
	return 0;
}

int mpu_configure_fifo(struct mpu6050_device *mpu6050, unsigned char sensors)
{
	unsigned char prev;
	int result = 0;

	/* Compass data isn't going into the FIFO. Stop trying. */
	sensors &= ~INV_XYZ_COMPASS;

	if (mpu6050->chip_cfg->dmp_on)
		return 0;
	else {
		if (!(mpu6050->chip_cfg->sensors))
			return -1;
		prev = mpu6050->chip_cfg->fifo_enable;
		mpu6050->chip_cfg->fifo_enable = sensors & mpu6050->chip_cfg->sensors;
		if (mpu6050->chip_cfg->fifo_enable != sensors)
			result = -1;		/* You're not getting what you asked for. Some sensors are asleep.*/
		else
			result = 0;
		if (sensors || mpu6050->chip_cfg->lp_accel_mode)
			mpu_set_int_status(mpu6050, 1);
		else
			mpu_set_int_status(mpu6050, 0);
		if (sensors) {
			if (mpu_reset_fifo(mpu6050)) {
				mpu6050->chip_cfg->fifo_enable = prev;
				return -1;
			}
		}
	}

	return result;
}

int mpu_lp_accel_mode(struct mpu6050_device *mpu6050, unsigned char rate)
{
	int ret = -EINVAL;
	unsigned char tmp[2];

	if (rate > 40)
		return -1;

	if (!rate) {
		mpu_set_int_latched(mpu6050, 0);
		tmp[0] = 0;
		tmp[1] = BIT_STBY_XYZG;
		ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_PWR_MGMT1, 2, tmp);
		if (ret < 0) {	
			mpu_log("write pwr_mgmt_1 not ok");	
			return ret;  
		}

		mpu6050->chip_cfg->lp_accel_mode = 0;
		return 0;
	}
	/* For LP accel, we automatically configure the hardware to produce latched
	* interrupts. In LP accel mode, the hardware cycles into sleep mode before
	* it gets a chance to deassert the interrupt pin; therefore, we shift this
	* responsibility over to the MCU.
	* Any register read will clear the interrupt.
	*/
	mpu_set_int_latched(mpu6050, 1);

	tmp[0] = BIT_LPA_CYCLE;
	if (rate == 1) {
		tmp[1] = INV_LPA_1_25HZ;
		mpu_set_lpf(mpu6050, 5);
	}else if (rate <= 5) {
		tmp[1] = INV_LPA_5HZ;
		mpu_set_lpf(mpu6050, 5);
	}else if (rate <= 20) {
		tmp[1] = INV_LPA_20HZ;
		mpu_set_lpf(mpu6050, 10);
	}else {
		tmp[1] = INV_LPA_40HZ;
		mpu_set_lpf(mpu6050, 20);
	}
	tmp[1] = (tmp[1] << 6) | BIT_STBY_XYZG;

	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_PWR_MGMT1, 2, tmp);
	if(ret < 0) {	
		mpu_log("write pwr_mgmt_1 not ok");	
		return ret;  
	}

	mpu6050->chip_cfg->sensors = INV_XYZ_ACCEL;
	mpu6050->chip_cfg->clk_src = 0;
	mpu6050->chip_cfg->lp_accel_mode = 1;
	mpu_configure_fifo(mpu6050, 0);

	return 0;
}

int mpu_load_firmware(struct mpu6050_device *mpu6050, unsigned short length, const unsigned char *firmware,
    unsigned short start_addr, unsigned short sample_rate)
{
	int ret = -EINVAL;
	unsigned short ii;
	unsigned short this_write;
	/* Must divide evenly into DMP_MEM_BLANK_SIZE to avoid bank crossings. */
#define LOAD_CHUNK  (16)
	unsigned char cur[LOAD_CHUNK], tmp[2];

	if (mpu6050->chip_cfg->dmp_loaded)
		return -1;	/* DMP should only be loaded once. */

	if (!firmware)
		return -1;
	for (ii = 0; ii < length; ii += this_write) {
		this_write = min(LOAD_CHUNK, length - ii);
		if (mpu_write_mem(mpu6050, ii, this_write, (unsigned char*)&firmware[ii]))
			return -1;
		if (mpu_read_mem(mpu6050, ii, this_write, cur))
			return -1;
		if (memcmp(firmware+ii, cur, this_write))
			return -2;
	}

	/* Set program start address. */
	tmp[0] = start_addr >> 8;
	tmp[1] = start_addr & 0xFF;
	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_PRGM_START_H, 2, tmp);
	if (ret < 0) {
		mpu6050->chip_cfg->sensors = 0;
		mpu_log("write prgm_start_h not ok");	
		return ret;  
	}

	mpu6050->chip_cfg->dmp_loaded = 1;
	mpu6050->chip_cfg->dmp_sample_rate = sample_rate;
	return 0;
}

unsigned short mpu_row_to_scale(const signed char *row)
{
	if (row[0] > 0)					return 0;
	else if (row[0] < 0)			return 4;
	else if (row[1] > 0)			return 1;
	else if (row[1] < 0)			return 5;
	else if (row[2] > 0)			return 2;
	else if (row[2] < 0)			return 6;
	else										return 7;      // error
}

unsigned short mpu_orientation_matrix_to_scalar(const signed char *mtx)
{
	unsigned short scalar;

	/*XYZ  010_001_000 Identity Matrix
		XZY  001_010_000
		YXZ  010_000_001
		YZX  000_010_001
		ZXY  001_000_010
		ZYX  000_001_010	*/

	scalar = mpu_row_to_scale(mtx);
	scalar |= mpu_row_to_scale(mtx + 3) << 3;
	scalar |= mpu_row_to_scale(mtx + 6) << 6;

	return scalar;
}

int dmp_set_fifo_rate(struct mpu6050_device *mpu6050, unsigned short rate)
{
	const unsigned char regs_end[12] = {DINAFE, DINAF2, DINAAB,
		0xc4, DINAAA, DINAF1, DINADF, DINADF, 0xBB, 0xAF, DINADF, DINADF};
	unsigned short div;
	unsigned char tmp[8];

	if (rate > DMP_SAMPLE_RATE)
		return -1;
	div = DMP_SAMPLE_RATE / rate - 1;
	tmp[0] = (unsigned char)((div >> 8) & 0xFF);
	tmp[1] = (unsigned char)(div & 0xFF);
	if (mpu_write_mem(mpu6050, D_0_22, 2, tmp))
		return -1;
	if (mpu_write_mem(mpu6050, CFG_6, 12, (unsigned char*)regs_end))
		return -1;

	mpu6050->dmp->fifo_rate = rate;
	return 0;
}


int dmp_set_orientation(struct mpu6050_device *mpu6050, unsigned short orient)
{
	unsigned char gyro_regs[3], accel_regs[3];
	const unsigned char gyro_axes[3] = {DINA4C, DINACD, DINA6C};
	const unsigned char accel_axes[3] = {DINA0C, DINAC9, DINA2C};
	const unsigned char gyro_sign[3] = {DINA36, DINA56, DINA76};
	const unsigned char accel_sign[3] = {DINA26, DINA46, DINA66};

	gyro_regs[0] = gyro_axes[orient & 3];
	gyro_regs[1] = gyro_axes[(orient >> 3) & 3];
	gyro_regs[2] = gyro_axes[(orient >> 6) & 3];
	accel_regs[0] = accel_axes[orient & 3];
	accel_regs[1] = accel_axes[(orient >> 3) & 3];
	accel_regs[2] = accel_axes[(orient >> 6) & 3];

	/* Chip-to-body, axes only. */
	if (mpu_write_mem(mpu6050, FCFG_1, 3, gyro_regs))
		return -1;
	if (mpu_write_mem(mpu6050, FCFG_2, 3, accel_regs))
		return -1;

	memcpy(gyro_regs, gyro_sign, 3);
	memcpy(accel_regs, accel_sign, 3);
	if (orient & 4) {
		gyro_regs[0] |= 1;
		accel_regs[0] |= 1;
	}
	if (orient & 0x20) {
		gyro_regs[1] |= 1;
		accel_regs[1] |= 1;
	}
	if (orient & 0x100) {
		gyro_regs[2] |= 1;
		accel_regs[2] |= 1;
	}

	/* Chip-to-body, sign only. */
	if (mpu_write_mem(mpu6050, FCFG_3, 3, gyro_regs))
		return -1;
	if (mpu_write_mem(mpu6050, FCFG_7, 3, accel_regs))
		return -1;
	mpu6050->dmp->orient = orient;
	return 0;
}

int dmp_set_tap_thresh(struct mpu6050_device *mpu6050, unsigned char axis, unsigned short thresh)
{
	unsigned char tmp[4], accel_fsr;
	unsigned int scaled_thresh;
	unsigned int dmp_thresh, dmp_thresh_2;
	if (!(axis & TAP_XYZ) || thresh > 1600)
		return -1;

	scaled_thresh = ((unsigned int)thresh * 1000)  / DMP_SAMPLE_RATE;

	mpu_get_accel_fsr(mpu6050, &accel_fsr);
	switch (accel_fsr) {
		case 2:
			dmp_thresh = (scaled_thresh * 16384);
			dmp_thresh_2 = (scaled_thresh * 12288);
			break;
		case 4:
			dmp_thresh = (scaled_thresh * 8192);
			dmp_thresh_2 = (scaled_thresh * 6144);
			break;
		case 8:
			dmp_thresh = (scaled_thresh * 4096);
			dmp_thresh_2 = (scaled_thresh * 3072);
			break;
		case 16:
			dmp_thresh = (scaled_thresh * 2048);
			dmp_thresh_2 = (scaled_thresh * 1536);
			break;
		default:
			return -1;
	}
	dmp_thresh /= 1000;
	dmp_thresh_2 /= 1000;
	tmp[0] = (unsigned char)(dmp_thresh >> 8);
	tmp[1] = (unsigned char)(dmp_thresh & 0xFF);
	tmp[2] = (unsigned char)(dmp_thresh_2 >> 8);
	tmp[3] = (unsigned char)(dmp_thresh_2 & 0xFF);

	if (axis & TAP_X) {
		if (mpu_write_mem(mpu6050, DMP_TAP_THX, 2, tmp))
			return -1;
		if (mpu_write_mem(mpu6050, D_1_36, 2, tmp+2))
			return -1;
	}
	if (axis & TAP_Y) {
		if (mpu_write_mem(mpu6050, DMP_TAP_THY, 2, tmp))
			return -1;
		if (mpu_write_mem(mpu6050, D_1_40, 2, tmp+2))
			return -1;
	}
	if (axis & TAP_Z) {
		if (mpu_write_mem(mpu6050, DMP_TAP_THZ, 2, tmp))
			return -1;
		if (mpu_write_mem(mpu6050, D_1_44, 2, tmp+2))
			return -1;
	}
	return 0;
}

int dmp_set_tap_axes(struct mpu6050_device *mpu6050, unsigned char axis)
{
	unsigned char tmp = 0;

	if (axis & TAP_X)
		tmp |= 0x30;
	if (axis & TAP_Y)
		tmp |= 0x0C;
	if (axis & TAP_Z)
		tmp |= 0x03;
	return mpu_write_mem(mpu6050, D_1_72, 1, &tmp);
}

int dmp_set_tap_count(struct mpu6050_device *mpu6050, unsigned char min_taps)
{
	unsigned char tmp;

	if (min_taps < 1)
		min_taps = 1;
	else if (min_taps > 4)
		min_taps = 4;

	tmp = min_taps - 1;
	return mpu_write_mem(mpu6050, D_1_79, 1, &tmp);
}

int dmp_set_tap_time(struct mpu6050_device *mpu6050, unsigned short time)
{
	unsigned short dmp_time;
	unsigned char tmp[2];

	dmp_time = time / (1000 / DMP_SAMPLE_RATE);
	tmp[0] = (unsigned char)(dmp_time >> 8);
	tmp[1] = (unsigned char)(dmp_time & 0xFF);
	return mpu_write_mem(mpu6050, DMP_TAPW_MIN, 2, tmp);
}

int dmp_set_tap_time_multi(struct mpu6050_device *mpu6050, unsigned short time)
{
	unsigned short dmp_time;
	unsigned char tmp[2];

	dmp_time = time / (1000 / DMP_SAMPLE_RATE);
	tmp[0] = (unsigned char)(dmp_time >> 8);
	tmp[1] = (unsigned char)(dmp_time & 0xFF);
	return mpu_write_mem(mpu6050, D_1_218, 2, tmp);
}

int dmp_set_shake_reject_thresh(struct mpu6050_device *mpu6050, long sf, unsigned short thresh)
{
	unsigned char tmp[4];
	long thresh_scaled = sf / 1000 * thresh;
	tmp[0] = (unsigned char)(((long)thresh_scaled >> 24) & 0xFF);
	tmp[1] = (unsigned char)(((long)thresh_scaled >> 16) & 0xFF);
	tmp[2] = (unsigned char)(((long)thresh_scaled >> 8) & 0xFF);
	tmp[3] = (unsigned char)((long)thresh_scaled & 0xFF);
	return mpu_write_mem(mpu6050, D_1_92, 4, tmp);
}

int dmp_set_shake_reject_time(struct mpu6050_device *mpu6050, unsigned short time)
{
	unsigned char tmp[2];

	time /= (1000 / DMP_SAMPLE_RATE);
	tmp[0] = time >> 8;
	tmp[1] = time & 0xFF;
	return mpu_write_mem(mpu6050, D_1_90,2,tmp);
}

int dmp_set_shake_reject_timeout(struct mpu6050_device *mpu6050, unsigned short time)
{
	unsigned char tmp[2];

	time /= (1000 / DMP_SAMPLE_RATE);
	tmp[0] = time >> 8;
	tmp[1] = time & 0xFF;
	return mpu_write_mem(mpu6050, D_1_88,2,tmp);
}

int dmp_set_gyro_cal_status(struct mpu6050_device *mpu6050, unsigned char enable)
{
	if (enable) {
		unsigned char regs[9] = {0xb8, 0xaa, 0xb3, 0x8d, 0xb4, 0x98, 0x0d, 0x35, 0x5d};
		return mpu_write_mem(mpu6050, CFG_MOTION_BIAS, 9, regs);
	} else {
		unsigned char regs[9] = {0xb8, 0xaa, 0xaa, 0xaa, 0xb0, 0x88, 0xc3, 0xc5, 0xc7};
		return mpu_write_mem(mpu6050, CFG_MOTION_BIAS, 9, regs);
	}
}

int dmp_set_lp_quat_status(struct mpu6050_device *mpu6050, unsigned char enable)
{
	unsigned char regs[4];
	if (enable) {
		regs[0] = DINBC0;
		regs[1] = DINBC2;
		regs[2] = DINBC4;
		regs[3] = DINBC6;
	}
	else
		memset(regs, 0x8B, 4);

	mpu_write_mem(mpu6050, CFG_LP_QUAT, 4, regs);

	return mpu_reset_fifo(mpu6050);
}

int dmp_set_6x_lp_quat_status(struct mpu6050_device *mpu6050, unsigned char enable)
{
	unsigned char regs[4];
	if (enable) {
		regs[0] = DINA20;
		regs[1] = DINA28;
		regs[2] = DINA30;
		regs[3] = DINA38;
	}else{
		memset(regs, 0xA3, 4);
	}

	mpu_write_mem(mpu6050, CFG_8, 4, regs);

	return mpu_reset_fifo(mpu6050);
}

int dmp_set_feature_status(struct mpu6050_device *mpu6050, unsigned short mask)
{
	unsigned char tmp[10];

	/* TODO: All of these settings can probably be integrated into the default
	* DMP image.
	*/
	/* Set integration scale factor. */
	tmp[0] = (unsigned char)((GYRO_SF >> 24) & 0xFF);
	tmp[1] = (unsigned char)((GYRO_SF >> 16) & 0xFF);
	tmp[2] = (unsigned char)((GYRO_SF >> 8) & 0xFF);
	tmp[3] = (unsigned char)(GYRO_SF & 0xFF);
	mpu_write_mem(mpu6050, D_0_104, 4, tmp);

	/* Send sensor data to the FIFO. */
	tmp[0] = 0xA3;
	if (mask & DMP_FEATURE_SEND_RAW_ACCEL) {
		tmp[1] = 0xC0;
		tmp[2] = 0xC8;
		tmp[3] = 0xC2;
	} else {
		tmp[1] = 0xA3;
		tmp[2] = 0xA3;
		tmp[3] = 0xA3;
	}
	if (mask & DMP_FEATURE_SEND_ANY_GYRO) {
		tmp[4] = 0xC4;
		tmp[5] = 0xCC;
		tmp[6] = 0xC6;
	} else {
		tmp[4] = 0xA3;
		tmp[5] = 0xA3;
		tmp[6] = 0xA3;
	}
	tmp[7] = 0xA3;
	tmp[8] = 0xA3;
	tmp[9] = 0xA3;
	mpu_write_mem(mpu6050, CFG_15,10,tmp);

	/* Send gesture data to the FIFO. */
	if (mask & (DMP_FEATURE_TAP | DMP_FEATURE_ANDROID_ORIENT))
		tmp[0] = DINA20;
	else
		tmp[0] = 0xD8;
	mpu_write_mem(mpu6050, CFG_27,1,tmp);

	if (mask & DMP_FEATURE_GYRO_CAL)
		dmp_set_gyro_cal_status(mpu6050, 1);
	else
		dmp_set_gyro_cal_status(mpu6050, 0);

	if (mask & DMP_FEATURE_SEND_ANY_GYRO) {
		if (mask & DMP_FEATURE_SEND_CAL_GYRO) {
			tmp[0] = 0xB2;
			tmp[1] = 0x8B;
			tmp[2] = 0xB6;
			tmp[3] = 0x9B;
		} else {
			tmp[0] = DINAC0;
			tmp[1] = DINA80;
			tmp[2] = DINAC2;
			tmp[3] = DINA90;
		}
		mpu_write_mem(mpu6050, CFG_GYRO_RAW_DATA, 4, tmp);
	}

	if (mask & DMP_FEATURE_TAP) {
		/* Enable tap. */
		tmp[0] = 0xF8;
		mpu_write_mem(mpu6050, CFG_20, 1, tmp);
		dmp_set_tap_thresh(mpu6050, TAP_XYZ, 250);
		dmp_set_tap_axes(mpu6050, TAP_XYZ);
		dmp_set_tap_count(mpu6050, 1);
		dmp_set_tap_time(mpu6050, 100);
		dmp_set_tap_time_multi(mpu6050, 500);

		dmp_set_shake_reject_thresh(mpu6050, GYRO_SF, 200);
		dmp_set_shake_reject_time(mpu6050, 40);
		dmp_set_shake_reject_timeout(mpu6050, 10);
	} else {
		tmp[0] = 0xD8;
		mpu_write_mem(mpu6050, CFG_20, 1, tmp);
	}

	if (mask & DMP_FEATURE_ANDROID_ORIENT) {
		tmp[0] = 0xD9;
	} else{
		tmp[0] = 0xD8;
	}
	mpu_write_mem(mpu6050, CFG_ANDROID_ORIENT_INT, 1, tmp);

	if (mask & DMP_FEATURE_LP_QUAT)
		dmp_set_lp_quat_status(mpu6050, 1);
	else
		dmp_set_lp_quat_status(mpu6050, 0);

	if (mask & DMP_FEATURE_6X_LP_QUAT)
		dmp_set_6x_lp_quat_status(mpu6050, 1);
	else
		dmp_set_6x_lp_quat_status(mpu6050, 0);

	/* Pedometer is always enabled. */
	mpu6050->dmp->feature_mask = mask | DMP_FEATURE_PEDOMETER;
	mpu_reset_fifo(mpu6050);

	mpu6050->dmp->packet_length = 0;
	if (mask & DMP_FEATURE_SEND_RAW_ACCEL)
		mpu6050->dmp->packet_length += 6;
	if (mask & DMP_FEATURE_SEND_ANY_GYRO)
		mpu6050->dmp->packet_length += 6;
	if (mask & (DMP_FEATURE_LP_QUAT | DMP_FEATURE_6X_LP_QUAT))
		mpu6050->dmp->packet_length += 16;
	if (mask & (DMP_FEATURE_TAP | DMP_FEATURE_ANDROID_ORIENT))
		mpu6050->dmp->packet_length += 4;

	return 0;
}

int dmp_load_motion_driver_firmware(struct mpu6050_device *mpu6050)
{
	return mpu_load_firmware(mpu6050, DMP_CODE_SIZE, dmp_memory, DMP_MEM_START_ADDR,
					DMP_SAMPLE_RATE);
}


int mpu_read_fifo_stream(struct mpu6050_device *mpu6050, unsigned short length, unsigned char *data,
    unsigned char *more)
{
	int ret = -EINVAL;
    unsigned char tmp[2];
    unsigned short fifo_count;
    if (!mpu6050->chip_cfg->dmp_on)
        return -1;
    if (!mpu6050->chip_cfg->sensors)
        return -1;

	ret = mpu_i2c_read(mpu6050, mpu6050->dev_id, MPU_FIFO_COUNT, 1, tmp);
	if (ret < 0) {	
		mpu_log("read MPU_FIFO_COUNT not ok");	
		return ret;  
	}
    fifo_count = (tmp[0] << 8) | tmp[1];
    if (fifo_count < length) {
        more[0] = 0;
        return -1;
    }
    if (fifo_count > (mpu6050->hw->max_fifo >> 1)) {
        /* FIFO is 50% full, better check overflow bit. */
		ret = mpu_i2c_read(mpu6050, mpu6050->dev_id, MPU_INT_STAT, 1, tmp);
		if (ret < 0) {	
			mpu_log("read MPU_INT_STAT not ok");	
			return ret;  
		}
        if (tmp[0] & BIT_FIFO_OVERFLOW) {
            mpu_reset_fifo(mpu6050);
            return -2;
        }
    }
	ret = mpu_i2c_read(mpu6050, mpu6050->dev_id, MPU_FIFO_R_W, length, data);
	if (ret < 0) {	
		mpu_log("read MPU_FIFO_R_W not ok");	
		return ret;  
	}
    more[0] = fifo_count / length - 1;
    return 0;
}


int dmp_read_fifo(struct mpu6050_device *mpu6050, short *gyro, short *accel, long *quat,
    unsigned long *timestamp, short *sensors, unsigned char *more)
{
    unsigned char fifo_data[MAX_PACKET_LENGTH];
    unsigned char ii = 0;

    /* TODO: sensors[0] only changes when dmp_enable_feature is called. We can
     * cache this value and save some cycles.
     */
    sensors[0] = 0;

    /* Get a packet. */
    if (mpu_read_fifo_stream(mpu6050, mpu6050->dmp->packet_length, fifo_data, more))
        return -1;

    /* Parse DMP packet. */
    if (mpu6050->dmp->feature_mask & (DMP_FEATURE_LP_QUAT | DMP_FEATURE_6X_LP_QUAT)) {
        long quat_q14[4], quat_mag_sq;

        quat[0] = ((long)fifo_data[0] << 24) | ((long)fifo_data[1] << 16) |
            ((long)fifo_data[2] << 8) | fifo_data[3];
        quat[1] = ((long)fifo_data[4] << 24) | ((long)fifo_data[5] << 16) |
            ((long)fifo_data[6] << 8) | fifo_data[7];
        quat[2] = ((long)fifo_data[8] << 24) | ((long)fifo_data[9] << 16) |
            ((long)fifo_data[10] << 8) | fifo_data[11];
        quat[3] = ((long)fifo_data[12] << 24) | ((long)fifo_data[13] << 16) |
            ((long)fifo_data[14] << 8) | fifo_data[15];
        ii += 16;
        /* We can detect a corrupted FIFO by monitoring the quaternion data and
         * ensuring that the magnitude is always normalized to one. This
         * shouldn't happen in normal operation, but if an I2C error occurs,
         * the FIFO reads might become misaligned.
         *
         * Let's start by scaling down the quaternion data to avoid long long
         * math.
         */
        quat_q14[0] = quat[0] >> 16;
        quat_q14[1] = quat[1] >> 16;
        quat_q14[2] = quat[2] >> 16;
        quat_q14[3] = quat[3] >> 16;
        quat_mag_sq = quat_q14[0] * quat_q14[0] + quat_q14[1] * quat_q14[1] +
            quat_q14[2] * quat_q14[2] + quat_q14[3] * quat_q14[3];
        if ((quat_mag_sq < QUAT_MAG_SQ_MIN) ||
            (quat_mag_sq > QUAT_MAG_SQ_MAX)) {
            /* Quaternion is outside of the acceptable threshold. */
            mpu_reset_fifo(mpu6050);
            sensors[0] = 0;
            return -1;
        }
        sensors[0] |= INV_WXYZ_QUAT;
    }

    if (mpu6050->dmp->feature_mask & DMP_FEATURE_SEND_RAW_ACCEL) {
        accel[0] = ((short)fifo_data[ii+0] << 8) | fifo_data[ii+1];
        accel[1] = ((short)fifo_data[ii+2] << 8) | fifo_data[ii+3];
        accel[2] = ((short)fifo_data[ii+4] << 8) | fifo_data[ii+5];
        ii += 6;
        sensors[0] |= INV_XYZ_ACCEL;
    }

    if (mpu6050->dmp->feature_mask & DMP_FEATURE_SEND_ANY_GYRO) {
        gyro[0] = ((short)fifo_data[ii+0] << 8) | fifo_data[ii+1];
        gyro[1] = ((short)fifo_data[ii+2] << 8) | fifo_data[ii+3];
        gyro[2] = ((short)fifo_data[ii+4] << 8) | fifo_data[ii+5];
        ii += 6;
        sensors[0] |= INV_XYZ_GYRO;
    }

    return 0;
}

void mpu_read_values(struct mpu6050_device *mpu6050, struct mpu6050_event *mpu_event)
{
	unsigned long sensor_timestamp;
	short gyro[3], accel[3], sensors;
	unsigned char more;
	long quat[4];

	dmp_read_fifo(mpu6050, gyro, accel, quat, &sensor_timestamp, &sensors, &more);	 
	if(sensors & INV_WXYZ_QUAT )
	{
		mpu_event->q1 = (int)quat[0];
		mpu_event->q2 = (int)quat[1];
		mpu_event->q3 = (int)quat[2];
		mpu_event->q4 = (int)quat[3];
		//mpu_log("q0:%ld q1:%ld q2:%ld q3:%ld", quat[0], quat[1], quat[2], quat[3]);
	}
}

int mpu_var_init(struct mpu6050_device *mpu)
{
	int err = -EINVAL;
	struct dmp_s *dmp;
	struct chip_cfg_s *chip_cfg;
	struct hw_s *hw;
	struct mpu6050_event *mpu_ev;
	
	dmp = kzalloc(sizeof(*dmp), GFP_KERNEL);
	if (dmp == NULL) {  
		err = -ENODEV;	 
		mpu_log("dmp_s kzalloc failed\n");
		goto dmp_mem_fail;
	}
	mpu->dmp = dmp;
	mpu->dmp->tap_cb = NULL;
	mpu->dmp->android_orient_cb = NULL;
	mpu->dmp->orient = 0;
	mpu->dmp->feature_mask = 0;
	mpu->dmp->fifo_rate = 0;
	mpu->dmp->packet_length = 0;

	chip_cfg = kzalloc(sizeof(*chip_cfg), GFP_KERNEL);
	if (chip_cfg == NULL) {  
		err = -ENODEV;	 
		mpu_log("chip_cfg_s kzalloc failed\n");
		goto chip_cfg_mem_fail;
	}
	mpu->chip_cfg = chip_cfg;
	/* Set to invalid values to ensure no I2C writes are skipped. */
	mpu->chip_cfg->sensors = 0xFF;
	mpu->chip_cfg->gyro_fsr = 0xFF;
	mpu->chip_cfg->accel_fsr = 0xFF;
	mpu->chip_cfg->lpf = 0xFF;
	mpu->chip_cfg->sample_rate = 0xFFFF;
	mpu->chip_cfg->fifo_enable = 0xFF;
	mpu->chip_cfg->bypass_mode = 0xFF;

	/* mpu_set_sensors always preserves this setting. */
	mpu->chip_cfg->clk_src = INV_CLK_PLL;
	/* Handled in next call to mpu_set_bypass. */
	mpu->chip_cfg->active_low_int = 0;
	mpu->chip_cfg->latched_int = 0;
	mpu->chip_cfg->int_motion_only = 0;
	mpu->chip_cfg->lp_accel_mode = 0;
	mpu->chip_cfg->dmp_on = 0;
	mpu->chip_cfg->dmp_loaded = 0;
	mpu->chip_cfg->dmp_sample_rate = 0;

	hw = kzalloc(sizeof(*hw), GFP_KERNEL);
	if (hw == NULL) {  
		err = -ENODEV;	 
		mpu_log("hw_s kzalloc failed\n");
		goto hw_mem_fail;
	}
	mpu->hw = hw;
	mpu->hw->max_fifo = 1024;
	mpu->hw->num_reg = 118;
	mpu->hw->temp_offset = -521;
	mpu->hw->temp_sens = 340;

	mpu_ev = kzalloc(sizeof(*mpu_ev), GFP_KERNEL);
	if (mpu_ev == NULL) {  
		err = -ENODEV;	 
		mpu_log("mpu6050_event kzalloc failed\n");
		goto mpu6050_event_mem_fail;
	}
	mpu->mpu_ev = mpu_ev;
	mpu->mpu_ev->q1 = 0;
	mpu->mpu_ev->q2 = 0;
	mpu->mpu_ev->q3 = 0;
	mpu->mpu_ev->q4 = 0;

	return 0;
	
	kfree(mpu_ev);
mpu6050_event_mem_fail:
	kfree(hw);
hw_mem_fail:
	kfree(chip_cfg);
chip_cfg_mem_fail:
	kfree(dmp);
dmp_mem_fail:
	return -EINVAL;
}

int mpu_dmp_init(struct mpu6050_device *mpu6050)
{
	unsigned char rev;
	/* Reset device. */

	int ret = -EINVAL;
	unsigned char val[6] = { 0 };
	val[0] = 0x80;
	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_PWR_MGMT1, 1, val);
	if (ret < 0) {	
		mpu_log("write PWR_MGMT_1 not ok");	
		return ret;  
	}

	msleep(100);
	/* Wake up chip. */
	val[0] = 0x00;
	ret = mpu_i2c_write(mpu6050, mpu6050->dev_id, MPU_PWR_MGMT1, 2, val);
	if (ret < 0) {	
		mpu_log("write PWR_MGMT_1 not ok");	
		return ret;  
	}

	/* Check product revision. */
	ret = mpu_i2c_read(mpu6050, mpu6050->dev_id, MPU_ACCEL_OFFS, 6, val);
	if (ret < 0) {	
		mpu_log("read accel_offs not ok");	
		return ret;  
	}


	rev = ((val[5] & 0x01) << 2) | ((val[3] & 0x01) << 1) |
	(val[1] & 0x01);

	if (rev) {
		/* Congrats, these parts are better. */
		if (rev == 1)
		mpu6050->chip_cfg->accel_half = 1;
		else if (rev == 2)
		mpu6050->chip_cfg->accel_half = 0;
		else
		return -1;
	} else {
		ret = mpu_i2c_read(mpu6050, mpu6050->dev_id, MPU_PROD_ID, 1, val);
		if (ret < 0) {	
			mpu_log("read prod_id not ok");	
			return ret;  
		}
		rev = val[0] & 0x0F;
		if (!rev) {
			return -1;
		} else if (rev == 4) {
			mpu6050->chip_cfg->accel_half = 1;
		} else
			mpu6050->chip_cfg->accel_half = 0;
	}

	ret = mpu_set_gyro_fsr(mpu6050, 2000);
	if (ret != 0)
		return -1;
	
	ret = mpu_set_accel_fsr(mpu6050, 2);
	if (ret != 0)
		return -1;
	
	ret = mpu_set_lpf(mpu6050, 42);
	if (ret != 0)
		return -1;
	
	ret = mpu_set_sample_rate(mpu6050, 50);
	if (ret != 0)
		return -1;
	
	ret = mpu_configure_fifo(mpu6050, 0);
	if (ret != 0)
		return -1;
	
	/* Already disabled by setup_compass. */
	ret = mpu_set_bypass(mpu6050, 0);
	if (ret != 0)
		return -1;

	 	  

	ret = mpu_set_sensors(mpu6050, INV_XYZ_GYRO | INV_XYZ_ACCEL);
	if(ret != 0)		
	{
		mpu_log("mpu_set_sensor come across error ......%d\n", ret);
		return ret;
	}
		

	ret = mpu_configure_fifo(mpu6050, INV_XYZ_GYRO | INV_XYZ_ACCEL);
	if(ret != 0)
	{
		mpu_log("mpu_configure_fifo come across error ......%d\n", ret);
		return ret;
	}

	ret = mpu_set_sample_rate(mpu6050, DEFAULT_MPU_HZ);
	if(ret != 0)
	{
		mpu_log("mpu_set_sample_rate error ......%d\n", ret);
		return ret;
	}	

	ret = dmp_load_motion_driver_firmware(mpu6050);
	if(ret != 0)
	{
		mpu_log("dmp_load_motion_driver_firmware come across error ......%d\n", ret);
		return ret;
	}

	ret = dmp_set_orientation(mpu6050, mpu_orientation_matrix_to_scalar(gyro_orientation));
	if(ret != 0)
	{
		mpu_log("dmp_set_orientation come across error ......%d\n", ret);
		return ret;
	}		

	ret = dmp_set_feature_status(mpu6050, DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
				DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | 
					DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL);
	if(ret != 0)
	{
		mpu_log("dmp_set_feature_status come across error ......%d\n", ret);
		return ret;
	}
		
	ret = dmp_set_fifo_rate(mpu6050, DEFAULT_MPU_HZ);
	if(ret != 0)
	{
		mpu_log("dmp_set_fifo_rate come across error ......%d\n", ret);
		return ret;
	}	

	ret = mpu_set_dmp_state(mpu6050, 1);
	if(ret != 0)
	{
		mpu_log("mpu_set_dmp_state come across error ......%d\n", ret);
		return ret;
	}		

	mpu_log("mpu dmp initialization complete......\n ");

	return 0;
}
