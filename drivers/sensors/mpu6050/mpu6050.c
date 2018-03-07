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


struct mpu6050_device *mpu6050;

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

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -EINVAL;
	dev_t devno = MKDEV(MPU_MAJOR, MPU_MINOR);

	printk(KERN_ALERT "mpu6050 driver  probe!\n");
	
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
	printk(KERN_ALERT "mpu6050 driver remove!\n");
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
