/***********************************************************************************
 * mpu6050 driver for exynos4412
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
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>

#include "mpu6050.h"

#define BUF_SIZE	20

struct mpu6050_device *mpu6050;

const struct hw_s hw={
  0x68,	 //addr
  1024,	 //max_fifo
  118,	 //num_reg
  340,	 //temp_sens
  -521,	 //temp_offset
  256	 //bank_size
};

const struct gyro_reg_s reg = {
0x75,  //who_am_i
0x19,  //rate_div
0x1A,  //lpf
0x0C,  //prod_id
0x6A,  //user_ctrl
0x23,  //fifo_en
0x1B,  //gyro_cfg
0x1C,  //accel_cfg
0x1F,  // motion_thr
0x20,  // motion_dur
0x72,  // fifo_count_h
0x74,  // fifo_r_w
0x43,  // raw_gyro
0x3B,  // raw_accel
0x41,  // temp
0x38,  // int_enable
0x39,  //  dmp_int_status
0x3A,  //  int_status
0x6B,  // pwr_mgmt_1
0x6C,  // pwr_mgmt_2
0x37,  // int_pin_cfg
0x6F,  // mem_r_w
0x06,  // accel_offs
0x24,  // i2c_mst
0x6D,  // bank_sel
0x6E,  // mem_start_addr
0x70   // prgm_start_h
};

const struct test_s test={
32768/250,		 //gyro_sens
32768/16,		 //	accel_sens
0,				 //	reg_rate_div
1,				//	reg_lpf
0,				 //	reg_gyro_fsr
0x18,			//	reg_accel_fsr
50,				//	wait_ms
5,				//	packet_thresh
};


static struct gyro_state_s st={
  &reg,
  &hw,
  {0},
  &test
};



int mpu6050_i2c_read(u8 addr, u8 reg, u8 len, u8 *value)
{
	u8 txbuf[1] = { reg };
	u8 rxbuf[BUF_SIZE] = { 0 };
	struct i2c_msg msgs[] = {
			{
					.addr	= addr, 
					.flags	= 0,	
					.len	= len,
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
		printk("read reg (0x%02x) error, %d\n", reg, ret);
	}
	else
	{
		for(k=0; k<len; k++)
			*(value+k) = rxbuf[k];
	}
	return ret;
}

  
int mpu6050_i2c_write(unsigned char addr, unsigned char reg, unsigned char len, unsigned char *value)  
{
	u8 txbuf[BUF_SIZE] = { 0 };
	struct i2c_msg msgs[] = {
			{
					.addr	= addr, 
					.flags	= 0,	
					.len	= len,
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
		printk("write reg (0x%02x) error, %d\n", reg, ret);
	}
	return ret;
}


/*
 *  @brief      Set the gyro full-scale range.
 *  @param[in]  fsr Desired full-scale range.
 *  @return     0 if successful.
 */
int mpu_set_gyro_fsr(unsigned short fsr)
{
	int ret = -EINVAL;
    unsigned char data;

    if (!(st.chip_cfg.sensors))
        return -1;

    switch (fsr) {
	    case 250:	data = INV_FSR_250DPS << 3;		break;
	    case 500:	data = INV_FSR_500DPS << 3;		break;
	    case 1000:	data = INV_FSR_1000DPS << 3;	break;
	    case 2000:	data = INV_FSR_2000DPS << 3;	break;
	    default:	return -1;
    }

    if (st.chip_cfg.gyro_fsr == (data >> 3))
        return 0;
	
	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->gyro_cfg, 1, &data);
	if (ret < 0) {	
		printk("write gyro_cfg not ok");	
		return ret;  
	}

    st.chip_cfg.gyro_fsr = data >> 3;
    return 0;
}

/**
 *  @brief      Set the accel full-scale range.
 *  @param[in]  fsr Desired full-scale range.
 *  @return     0 if successful.
 */
int mpu_set_accel_fsr(unsigned char fsr)
{
	int ret = -EINVAL;
    unsigned char data;

    if (!(st.chip_cfg.sensors))
        return -1;

    switch (fsr) {
	    case 2:		data = INV_FSR_2G << 3;		break;
	    case 4:		data = INV_FSR_4G << 3;		break;
	    case 8:		data = INV_FSR_8G << 3;		break;
	    case 16:	data = INV_FSR_16G << 3;	break;
	    default:	return -1;
    }

    if (st.chip_cfg.accel_fsr == (data >> 3))
        return 0;

	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->accel_cfg, 1, &data);
	if (ret < 0) {	
		printk("write gyro_cfg not ok");	
		return ret;  
	}
	
    st.chip_cfg.accel_fsr = data >> 3;
	
    return 0;
}

/**
 *  @brief      Set digital low pass filter.
 *  The following LPF settings are supported: 188, 98, 42, 20, 10, 5.
 *  @param[in]  lpf Desired LPF setting.
 *  @return     0 if successful.
 */
int mpu_set_lpf(unsigned short lpf)
{
	int ret = -EINVAL;
    unsigned char data;

    if (!(st.chip_cfg.sensors))
        return -1;

    if (lpf >= 188)
        data = INV_FILTER_188HZ;
    else if (lpf >= 98)
        data = INV_FILTER_98HZ;
    else if (lpf >= 42)
        data = INV_FILTER_42HZ;
    else if (lpf >= 20)
        data = INV_FILTER_20HZ;
    else if (lpf >= 10)
        data = INV_FILTER_10HZ;
    else
        data = INV_FILTER_5HZ;

    if (st.chip_cfg.lpf == data)
        return 0;

	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->lpf, 1, &data);
	if (ret < 0) {	
		printk("write lpf not ok");	
		return ret;  
	}

    st.chip_cfg.lpf = data;
    return 0;
}

/**
 *  @brief      Enable latched interrupts.
 *  Any MPU register will clear the interrupt.
 *  @param[in]  enable  1 to enable, 0 to disable.
 *  @return     0 if successful.
 */
int mpu_set_int_latched(unsigned char enable)
{
	int ret = -EINVAL;
    unsigned char tmp;
    if (st.chip_cfg.latched_int == enable)
        return 0;

    if (enable)
        tmp = BIT_LATCH_EN | BIT_ANY_RD_CLR;
    else
        tmp = 0;
    if (st.chip_cfg.bypass_mode)
        tmp |= BIT_BYPASS_EN;
    if (st.chip_cfg.active_low_int)
        tmp |= BIT_ACTL;

	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->int_pin_cfg, 1, &tmp);
	if (ret < 0) {	
		printk("write int_pin_cfg not ok");	
		return ret;  
	}
	
    st.chip_cfg.latched_int = enable;
    return 0;
}

/**
 *  @brief      Enable/disable data ready interrupt.
 *  If the DMP is on, the DMP interrupt is enabled. Otherwise, the data ready
 *  interrupt is used.
 *  @param[in]  enable      1 to enable interrupt.
 *  @return     0 if successful.
 */
static int set_int_enable(unsigned char enable)
{
	int ret = -EINVAL;
    unsigned char tmp;

    if (st.chip_cfg.dmp_on) {
        if (enable)
            tmp = BIT_DMP_INT_EN;
        else
            tmp = 0x00;

		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->int_enable, 1, &tmp);
		if (ret < 0) {	
			printk("write int_enable not ok");	
			return ret;  
		}
	
        st.chip_cfg.int_enable = tmp;
    } else {
        if (!st.chip_cfg.sensors)
            return -1;
        if (enable && st.chip_cfg.int_enable)
            return 0;
        if (enable)
            tmp = BIT_DATA_RDY_EN;
        else
            tmp = 0x00;
		
        ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->int_enable, 1, &tmp);
		if (ret < 0) {	
			printk("write int_enable not ok");	
			return ret;  
		}
		
        st.chip_cfg.int_enable = tmp;
    }
    return 0;
}

/**
 *  @brief  Reset FIFO read/write pointers.
 *  @return 0 if successful.
 */
int mpu_reset_fifo(void)
{
	int ret = -EINVAL;
    unsigned char data;

    if (!(st.chip_cfg.sensors))
        return -1;

    data = 0;
	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->int_enable, 1, &data);
	if (ret < 0) {	
		printk("write int_enable not ok");	
		return ret;  
	}
	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->fifo_en, 1, &data);
	if (ret < 0) {	
		printk("write fifo_en not ok");	
		return ret;  
	}
	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->user_ctrl, 1, &data);
	if (ret < 0) {	
		printk("write user_ctrl not ok");	
		return ret;  
	}

    if (st.chip_cfg.dmp_on) {
        data = BIT_FIFO_RST | BIT_DMP_RST;
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->user_ctrl, 1, &data);
		if (ret < 0) {	
			printk("write user_ctrl not ok");	
			return ret;  
		}
		
        msleep(50);
		
        data = BIT_DMP_EN | BIT_FIFO_EN;
        if (st.chip_cfg.sensors & INV_XYZ_COMPASS)
            data |= BIT_AUX_IF_EN;
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->user_ctrl, 1, &data);
		if (ret < 0) {	
			printk("write user_ctrl not ok");	
			return ret;  
		}
		
        if (st.chip_cfg.int_enable)
            data = BIT_DMP_INT_EN;
        else
            data = 0;
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->int_enable, 1, &data);
		if (ret < 0) {	
			printk("write int_enable not ok");	
			return ret;  
		}

        data = 0;
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->fifo_en, 1, &data);
		if (ret < 0) {	
			printk("write int_enable not ok");	
			return ret;  
		}
    } else {
        data = BIT_FIFO_RST;
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->user_ctrl, 1, &data);
		if (ret < 0) {	
			printk("write user_ctrl not ok");	
			return ret;  
		}
		
        if (st.chip_cfg.bypass_mode || !(st.chip_cfg.sensors & INV_XYZ_COMPASS))
            data = BIT_FIFO_EN;
        else
            data = BIT_FIFO_EN | BIT_AUX_IF_EN;
        ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->user_ctrl, 1, &data);
		if (ret < 0) {	
			printk("write user_ctrl not ok");	
			return ret;  
		}
		
        msleep(50);
        if (st.chip_cfg.int_enable)
            data = BIT_DATA_RDY_EN;
        else
            data = 0;
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->int_enable, 1, &data);
		if (ret < 0) {	
			printk("write int_enable not ok");	
			return ret;  
		}
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->fifo_en, 1, &st.chip_cfg.fifo_enable);
		if (ret < 0) {	
			printk("write fifo_en not ok");	
			return ret;  
		}

    }
    return 0;
}

/**
 *  @brief      Set device to bypass mode.
 *  @param[in]  bypass_on   1 to enable bypass mode.
 *  @return     0 if successful.
 */
int mpu_set_bypass(unsigned char bypass_on)
{
	int ret = -EINVAL;
    unsigned char tmp;

    if (st.chip_cfg.bypass_mode == bypass_on)
        return 0;

    if (bypass_on) {
		ret = mpu6050_i2c_read(mpu6050->dev_id, st.reg->user_ctrl, 1, &tmp);
		if (ret < 0) {	
			printk("read user_ctrl not ok");	
			return ret;  
		}
        tmp &= ~BIT_AUX_IF_EN;
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->user_ctrl, 1, &tmp);
		if (ret < 0) {	
			printk("write user_ctrl not ok");	
			return ret;  
		}
		
        msleep(3);
		
        tmp = BIT_BYPASS_EN;
        if (st.chip_cfg.active_low_int)
            tmp |= BIT_ACTL;
        if (st.chip_cfg.latched_int)
            tmp |= BIT_LATCH_EN | BIT_ANY_RD_CLR;
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->int_pin_cfg, 1, &tmp);
		if (ret < 0) {	
			printk("write int_pin_cfg not ok");	
			return ret;  
		}
    } else {
        /* Enable I2C master mode if compass is being used. */
		ret = mpu6050_i2c_read(mpu6050->dev_id, st.reg->user_ctrl, 1, &tmp);
		if (ret < 0) {	
			printk("read user_ctrl not ok");	
			return ret;  
		}
        if (st.chip_cfg.sensors & INV_XYZ_COMPASS)
            tmp |= BIT_AUX_IF_EN;
        else
            tmp &= ~BIT_AUX_IF_EN;
        ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->user_ctrl, 1, &tmp);
		if (ret < 0) {	
			printk("write user_ctrl not ok");	
			return ret;  
		}
		
        msleep(3);
		
        if (st.chip_cfg.active_low_int)
            tmp = BIT_ACTL;
        else
            tmp = 0;
        if (st.chip_cfg.latched_int)
            tmp |= BIT_LATCH_EN | BIT_ANY_RD_CLR;
        ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->int_pin_cfg, 1, &tmp);
		if (ret < 0) {	
			printk("write int_pin_cfg not ok");	
			return ret;  
		}
    }
    st.chip_cfg.bypass_mode = bypass_on;
    return 0;
}

/**
 *  @brief      Select which sensors are pushed to FIFO.
 *  @e sensors can contain a combination of the following flags:
 *  \n INV_X_GYRO, INV_Y_GYRO, INV_Z_GYRO
 *  \n INV_XYZ_GYRO
 *  \n INV_XYZ_ACCEL
 *  @param[in]  sensors Mask of sensors to push to FIFO.
 *  @return     0 if successful.
 */
int mpu_configure_fifo(unsigned char sensors)
{
    unsigned char prev;
    int result = 0;

    /* Compass data isn't going into the FIFO. Stop trying. */
    sensors &= ~INV_XYZ_COMPASS;

    if (st.chip_cfg.dmp_on)
        return 0;
    else {
        if (!(st.chip_cfg.sensors))
            return -1;
        prev = st.chip_cfg.fifo_enable;
        st.chip_cfg.fifo_enable = sensors & st.chip_cfg.sensors;
        if (st.chip_cfg.fifo_enable != sensors)
            /* You're not getting what you asked for. Some sensors are
             * asleep.
             */
            result = -1;
        else
            result = 0;
        if (sensors || st.chip_cfg.lp_accel_mode)
            set_int_enable(1);
        else
            set_int_enable(0);
        if (sensors) {
            if (mpu_reset_fifo()) {
                st.chip_cfg.fifo_enable = prev;
                return -1;
            }
        }
    }

    return result;
}


/**
 *  @brief      Enter low-power accel-only mode.
 *  In low-power accel mode, the chip goes to sleep and only wakes up to sample
 *  the accelerometer at one of the following frequencies:
 *  \n MPU6050: 1.25Hz, 5Hz, 20Hz, 40Hz
 *  \n MPU6500: 1.25Hz, 2.5Hz, 5Hz, 10Hz, 20Hz, 40Hz, 80Hz, 160Hz, 320Hz, 640Hz
 *  \n If the requested rate is not one listed above, the device will be set to
 *  the next highest rate. Requesting a rate above the maximum supported
 *  frequency will result in an error.
 *  \n To select a fractional wake-up frequency, round down the value passed to
 *  @e rate.
 *  @param[in]  rate        Minimum sampling rate, or zero to disable LP
 *                          accel mode.
 *  @return     0 if successful.
 */
int mpu_lp_accel_mode(unsigned char rate)
{
	int ret = -EINVAL;
    unsigned char tmp[2];

    if (rate > 40)
        return -1;

    if (!rate) {
        mpu_set_int_latched(0);
        tmp[0] = 0;
        tmp[1] = BIT_STBY_XYZG;
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->pwr_mgmt_1, 2, tmp);
		if (ret < 0) {	
			printk("write pwr_mgmt_1 not ok");	
			return ret;  
		}

        st.chip_cfg.lp_accel_mode = 0;
        return 0;
    }
    /* For LP accel, we automatically configure the hardware to produce latched
     * interrupts. In LP accel mode, the hardware cycles into sleep mode before
     * it gets a chance to deassert the interrupt pin; therefore, we shift this
     * responsibility over to the MCU.
     *
     * Any register read will clear the interrupt.
     */
    mpu_set_int_latched(1);

    tmp[0] = BIT_LPA_CYCLE;
    if (rate == 1) {
        tmp[1] = INV_LPA_1_25HZ;
        mpu_set_lpf(5);
    } else if (rate <= 5) {
        tmp[1] = INV_LPA_5HZ;
        mpu_set_lpf(5);
    } else if (rate <= 20) {
        tmp[1] = INV_LPA_20HZ;
        mpu_set_lpf(10);
    } else {
        tmp[1] = INV_LPA_40HZ;
        mpu_set_lpf(20);
    }
    tmp[1] = (tmp[1] << 6) | BIT_STBY_XYZG;

	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->pwr_mgmt_1, 2, tmp);
	if (ret < 0) {	
		printk("write pwr_mgmt_1 not ok");	
		return ret;  
	}

    st.chip_cfg.sensors = INV_XYZ_ACCEL;
    st.chip_cfg.clk_src = 0;
    st.chip_cfg.lp_accel_mode = 1;
    mpu_configure_fifo(0);

    return 0;
}

/**
 *  @brief      Set sampling rate.
 *  Sampling rate must be between 4Hz and 1kHz.
 *  @param[in]  rate    Desired sampling rate (Hz).
 *  @return     0 if successful.
 */
int mpu_set_sample_rate(unsigned short rate)
{
	int ret = -EINVAL;
    unsigned char data;

    if (!(st.chip_cfg.sensors))
        return -1;

    if (st.chip_cfg.dmp_on)
        return -1;
    else {
        if (st.chip_cfg.lp_accel_mode) {
            if (rate && (rate <= 40)) {
                /* Just stay in low-power accel mode. */
                mpu_lp_accel_mode(rate);
                return 0;
            }
            /* Requested rate exceeds the allowed frequencies in LP accel mode,
             * switch back to full-power mode.
             */
            mpu_lp_accel_mode(0);
        }
        if (rate < 4)
            rate = 4;
        else if (rate > 1000)
            rate = 1000;

        data = 1000 / rate - 1;
		
		ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->rate_div, 1, &data);
		if (ret < 0) {	
			printk("write rate_div not ok");	
			return ret;  
		}

        st.chip_cfg.sample_rate = 1000 / (1 + data);

        /* Automatically set LPF to 1/2 sampling rate. */
        mpu_set_lpf(st.chip_cfg.sample_rate >> 1);
        return 0;
    }
}

/**
 *  @brief      Turn specific sensors on/off.
 *  @e sensors can contain a combination of the following flags:
 *  \n INV_X_GYRO, INV_Y_GYRO, INV_Z_GYRO
 *  \n INV_XYZ_GYRO
 *  \n INV_XYZ_ACCEL
 *  \n INV_XYZ_COMPASS
 *  @param[in]  sensors    Mask of sensors to wake.
 *  @return     0 if successful.
 */
int mpu_set_sensors(unsigned char sensors)
{
	int ret = -EINVAL;
    unsigned char data;

    if (sensors & INV_XYZ_GYRO)
        data = INV_CLK_PLL;
    else if (sensors)
        data = 0;
    else
        data = BIT_SLEEP;
	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->pwr_mgmt_1, 1, &data);
	if (ret < 0) {	
		printk("write pwr_mgmt_1 not ok");	
		return ret;  
	}
	
    st.chip_cfg.clk_src = data & ~BIT_SLEEP;

    data = 0;
    if (!(sensors & INV_X_GYRO))
        data |= BIT_STBY_XG;
    if (!(sensors & INV_Y_GYRO))
        data |= BIT_STBY_YG;
    if (!(sensors & INV_Z_GYRO))
        data |= BIT_STBY_ZG;
    if (!(sensors & INV_XYZ_ACCEL))
        data |= BIT_STBY_XYZA;
	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->pwr_mgmt_2, 1, &data);
	if (ret < 0) {
		st.chip_cfg.sensors = 0;
		printk("write pwr_mgmt_2 not ok");	
		return ret;  
	}

    if (sensors && (sensors != INV_XYZ_ACCEL))
        /* Latched interrupts only used in LP accel mode. */
        mpu_set_int_latched(0);

    st.chip_cfg.sensors = sensors;
    st.chip_cfg.lp_accel_mode = 0;
    mdelay(50);
    return 0;
}


int mpu_sensor_init(void)
{
    unsigned char rev;
    /* Reset device. */

	int ret = -EINVAL;
	unsigned char val[6] = { 0 };
	val[0] = 0x80;
	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->pwr_mgmt_1, 1, val);
	if (ret < 0) {	
		printk("write PWR_MGMT_1 not ok");	
		return ret;  
	}

    msleep(100);
    /* Wake up chip. */
    val[0] = 0x00;
	ret = mpu6050_i2c_write(mpu6050->dev_id, st.reg->pwr_mgmt_1, 1, val);
	if (ret < 0) {	
		printk("write PWR_MGMT_1 not ok");	
		return ret;  
	}
		
    /* Check product revision. */

	ret = mpu6050_i2c_read(mpu6050->dev_id, st.reg->accel_offs, 6, val);
	if (ret < 0) {	
		printk("read accel_offs not ok");	
		return ret;  
	}
	
		
    rev = ((val[5] & 0x01) << 2) | ((val[3] & 0x01) << 1) |
        (val[1] & 0x01);

    if (rev) {
        /* Congrats, these parts are better. */
        if (rev == 1)
            st.chip_cfg.accel_half = 1;
        else if (rev == 2)
            st.chip_cfg.accel_half = 0;
        else {
          
            return -1;
        }
    } else {
    	ret = mpu6050_i2c_read(mpu6050->dev_id, st.reg->prod_id, 1, val);
        if (ret < 0) {	
			printk("read prod_id not ok");	
			return ret;  
		}
        rev = val[0] & 0x0F;
        if (!rev) {
            return -1;
        } else if (rev == 4) {
            st.chip_cfg.accel_half = 1;
        } else
            st.chip_cfg.accel_half = 0;
    }

    /* Set to invalid values to ensure no I2C writes are skipped. */
    st.chip_cfg.sensors = 0xFF;
    st.chip_cfg.gyro_fsr = 0xFF;
    st.chip_cfg.accel_fsr = 0xFF;
    st.chip_cfg.lpf = 0xFF;
    st.chip_cfg.sample_rate = 0xFFFF;
    st.chip_cfg.fifo_enable = 0xFF;
    st.chip_cfg.bypass_mode = 0xFF;

    /* mpu_set_sensors always preserves this setting. */
    st.chip_cfg.clk_src = INV_CLK_PLL;
    /* Handled in next call to mpu_set_bypass. */
    st.chip_cfg.active_low_int = 0;
    st.chip_cfg.latched_int = 0;
    st.chip_cfg.int_motion_only = 0;
    st.chip_cfg.lp_accel_mode = 0;
    memset(&st.chip_cfg.cache, 0, sizeof(st.chip_cfg.cache));
    st.chip_cfg.dmp_on = 0;
    st.chip_cfg.dmp_loaded = 0;
    st.chip_cfg.dmp_sample_rate = 0;

    if (mpu_set_gyro_fsr(2000))
        return -1;
    if (mpu_set_accel_fsr(2))
        return -1;
    if (mpu_set_lpf(42))
        return -1;
    if (mpu_set_sample_rate(50))
        return -1;
    if (mpu_configure_fifo(0))
        return -1;

    /* Already disabled by setup_compass. */
    if (mpu_set_bypass(0))
        return -1;

	mpu_log("mpu initialization complete......\n ");		//mpu initialization complete	 	  

	if(!mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL))		//mpu_set_sensor
		mpu_log("mpu_set_sensor complete ......\n");
	else
		mpu_log("mpu_set_sensor come across error ......\n");

	if(!mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL))	//mpu_configure_fifo
		mpu_log("mpu_configure_fifo complete ......\n");
	else
		mpu_log("mpu_configure_fifo come across error ......\n");

	if(!mpu_set_sample_rate(DEFAULT_MPU_HZ))				//mpu_set_sample_rate
		mpu_log("mpu_set_sample_rate complete ......\n");
	else
		mpu_log("mpu_set_sample_rate error ......\n");


/*

if(!dmp_load_motion_driver_firmware())					//dmp_load_motion_driver_firmvare
	mpu_log("dmp_load_motion_driver_firmware complete ......\n");
else
	mpu_log("dmp_load_motion_driver_firmware come across error ......\n");

if(!dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation)))	  //dmp_set_orientation
	mpu_log("dmp_set_orientation complete ......\n");
else
	mpu_log("dmp_set_orientation come across error ......\n");

if(!dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |
DMP_FEATURE_GYRO_CAL))								 //dmp_enable_feature
	mpu_log("dmp_enable_feature complete ......\n");
else
	mpu_log("dmp_enable_feature come across error ......\n");

if(!dmp_set_fifo_rate(DEFAULT_MPU_HZ))					 //dmp_set_fifo_rate
	mpu_log("dmp_set_fifo_rate complete ......\n");
else
	mpu_log("dmp_set_fifo_rate come across error ......\n");

//run_self_test();		

if(!mpu_set_dmp_state(1))
	mpu_log("mpu_set_dmp_state complete ......\n");
else
	mpu_log("mpu_set_dmp_state come across error ......\n");

*/
















	
    return 0;
}


static int mpu6050_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int mpu6050_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long mpu6050_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations mpu6050_fops = {
    .owner			= THIS_MODULE,
    .open			= mpu6050_open,
    .release		= mpu6050_release,
    .unlocked_ioctl = mpu6050_ioctl,
};

int test_fun(void)
{
	int ret = -EINVAL;
	unsigned char val[1] = { 0 };

	
	ret = mpu6050_i2c_read(MPU_ADDR_AD0_LOW, WHO_AM_I, 1, val);
	if (ret < 0){  
        printk("MPU_ADDR_AD0_LOW read not ok");
		ret = mpu6050_i2c_read(MPU_ADDR_AD0_HIGH, WHO_AM_I, 1, val);
		if (ret < 0){
			printk("both addr read not ok"); 
			return ret; 
		} 
    }
	mpu6050->dev_id = val[0];
	mpu_log("WHO_AM_I:0x%x\n", mpu6050->dev_id);

	mpu_log("sensor_init:%d\n", mpu_sensor_init());

/*

	val[0] = 0x80;
	ret = mpu6050_i2c_write(mpu6050->dev_id, PWR_MGMT_1, 1, val);
	if (ret < 0) {	
		printk("write PWR_MGMT_1 not ok");	
		return ret;  
	}
	

	val[0] = 0x07;
	ret = mpu6050_i2c_write(mpu6050->dev_id, SMPLRT_DIV, 1, val);
	if (ret < 0) {	
		printk("write SMPLRT_DIV not ok");	
		return ret;  
	}

	val[0] = 0x06;
	ret = mpu6050_i2c_write(mpu6050->dev_id, CONFIG, 1, val);
	if (ret < 0) {	
		printk("write CONFIG not ok");	
		return ret;  
	}

	val[0] = 0xF8;
	ret = mpu6050_i2c_write(mpu6050->dev_id, GYRO_CONFIG, 1, val);
	if (ret < 0) {	
		printk("write GYRO_CONFIG not ok");  
		return ret;  
	}

	val[0] = 0x19;
	ret = mpu6050_i2c_write(mpu6050->dev_id, ACCEL_CONFIG, 1, val);
	if (ret < 0) {	
		printk("write ACCEL_CONFIG not ok");  
		return ret;  
	}
*/

	return 0;
}

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -EINVAL;
	dev_t devno = MKDEV(MPU_MAJOR, MPU_MINOR);

	mpu_log("mpu6050 driver  probe!\n");
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{		
		err = -ENODEV;
		dev_err(&client->dev, "probe check functionality failed\n");
		goto check_functionality_failed;	

	}
	
	mpu6050 = kzalloc(sizeof(*mpu6050), GFP_KERNEL);  
    if (mpu6050 == NULL) {  
        err = -ENODEV;	 
		dev_err(&client->dev, "probe kzalloc failed\n");
		goto kzalloc_failed;
    }

	mpu6050->client = client;

	err = register_chrdev_region(devno, 1, MPU_NAME);
	if (err < 0) {  
        err = -ENODEV;
		dev_err(&client->dev, "probe register chrdev failed\n");
		goto register_chrdev_failed;
    }

	cdev_init(&mpu6050->cdev, &mpu6050_fops);  
	mpu6050->cdev.owner = THIS_MODULE;	
	err = cdev_add(&mpu6050->cdev, devno, 1);  
	if (err < 0) {	
		dev_err(&client->dev, "failed to add device\n");  
		goto add_cdev_failed;	
	}

	test_fun();
	
	//mpu_log("mpu6050 dmp test:%d!\n", mpu_sensor_init());


	return 0;

add_cdev_failed:
	unregister_chrdev_region(devno, 1);
register_chrdev_failed:
	kfree(mpu6050);
kzalloc_failed:
check_functionality_failed:
	return err;
}

static int __devexit mpu6050_remove(struct i2c_client *client)
{
	dev_t devno = MKDEV(MPU_MAJOR, MPU_MINOR); 
	mpu_log("mpu6050 driver remove!\n");
    cdev_del(&mpu6050->cdev);  
    unregister_chrdev_region(devno, 1);  
    kfree(mpu6050);
	return 0;
}

static const struct i2c_device_id mpu6050_id[] = {
	{MPU_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, mpu6050_id);

static struct i2c_driver mpu6050_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = mpu6050_probe,
	.remove = mpu6050_remove,
	.id_table = mpu6050_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = MPU_NAME,
	},
};


static int __init mpu6050_init(void)
{
	int flag = i2c_add_driver(&mpu6050_driver);
	mpu_log("mpu6050 driver init1!flag:%d\n", flag);
	
	return flag;
}
static void __exit mpu6050_exit(void)
{
	mpu_log("mpu6050 driver exit\n");
	i2c_del_driver(&mpu6050_driver);
} 

late_initcall(mpu6050_init);
module_exit(mpu6050_exit);
MODULE_AUTHOR("Ken");
MODULE_DESCRIPTION("MPU6050 driver demo");
MODULE_LICENSE("GPL");
