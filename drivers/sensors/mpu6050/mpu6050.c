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
#include <linux/platform_device.h>

#define MPU_NAME	"mpu6050"

static struct i2c_client *this_client;

static int mpu6050_open(struct file *filp)
{
    return 0;
}

static int mpu6050_release(struct file *filp)
{
    return 0;
}


static ssize_t mpu6050_write(struct file *filp, char __user *buffer, size_t count, loff_t *ppos)
{
	int ret = 0;

	return ret;
}

static ssize_t mpu6050_read(struct file *filp, char __user *buffer, size_t count, loff_t *ppos)
{
	int ret = 0;

	return ret;

}

static struct file_operations mpu6050_ops = {
    .owner		= THIS_MODULE,
    .open		= mpu6050_open,
    .release	= mpu6050_release,
    .write		= mpu6050_write,
    .read		= mpu6050_read,
};

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -EINVAL;
	printk(KERN_ALERT "mpu6050 driver probe!\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{		
		err = -ENODEV;		
		goto exit_check_functionality_failed;	

	}
	
	return 0;

exit_check_functionality_failed:
	dev_err(&client->dev, "probe mpu6050 failed , %d\n", err);
	return err;
}

static int __devexit mpu6050_remove(struct i2c_client *client)
{
	printk(KERN_ALERT "mpu6050 driver remove!\n");
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
	printk(KERN_ALERT "mpu6050 driver init1!flag:%d\n", flag);
	
	return flag;
}
static void __exit mpu6050_exit(void)
{
	printk(KERN_ALERT "mpu6050 driver exit\n");
	i2c_del_driver(&mpu6050_driver);
} 

module_init(mpu6050_init);
module_exit(mpu6050_exit);
MODULE_AUTHOR("Ken");
MODULE_DESCRIPTION("MPU6050 driver demo");
MODULE_LICENSE("GPL");
