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

#include "mpu6050.h"


#define	INPUT_SYSTEM_SUPPORT

static struct proc_dir_entry *mpu_proc_entry;
struct mpu6050_device *mpu6050;


static int mpu_open(struct inode *inode, struct file *filp)
{
	mpu_log("%s, line = %d\n", __FUNCTION__, __LINE__);
    return 0;
}

static int mpu_release(struct inode *inode, struct file *filp)
{
	mpu_log("%s, line = %d\n", __FUNCTION__, __LINE__);
    return 0;
}

static long mpu_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mpu6050_event mpu_ev; 
  
    switch(cmd) {  
    	case 2:
			mpu_ev.q1 = mpu6050->mpu_ev->q1;
			mpu_ev.q2 = mpu6050->mpu_ev->q2;
			mpu_ev.q3 = mpu6050->mpu_ev->q3;
			mpu_ev.q4 = mpu6050->mpu_ev->q4;
			break;
		default:
			mpu_log("ioctl unkown cmd\n");
			break;
	}
	if (copy_to_user((void *)arg, &mpu_ev, sizeof(struct mpu6050_event)))  
        return -EFAULT;
	
	return 0;
}

static unsigned int mpu_poll(struct file *file, struct poll_table_struct *poll_tab)
{
	
	return 0;
}


static struct file_operations mpu6050_fops = {
    .owner			= THIS_MODULE,
    .open			= mpu_open,
    .release		= mpu_release,
    .unlocked_ioctl = mpu_ioctl,
    .poll			= mpu_poll,
};

static irqreturn_t mpu6050_irq(int irq, void *handle)
{
	struct mpu6050_device *mpu = handle;

	//printk("%s, line = %d\n", __FUNCTION__, __LINE__);

	disable_irq_nosync(mpu->irq);
	//queue_delayed_work(&mpu->work, msecs_to_jiffies(mpu->poll_delay));
	schedule_delayed_work(&mpu->work, MPU_DELAY_WORK_INTERVAL);

	return IRQ_HANDLED;
}

static void mpu6050_free_irq(struct mpu6050_device *mpu)
{
	free_irq(mpu->irq, mpu);
	if (cancel_delayed_work_sync(&mpu->work)) {
		/*
		 * Work was pending, therefore we need to enable
		 * IRQ here to balance the disable_irq() done in the
		 * interrupt handler.
		 */
		enable_irq(mpu->irq);
	}
}

static void mpu_work(struct work_struct *work)
{
	struct mpu6050_device *mpu =
			container_of(to_delayed_work(work), struct mpu6050_device, work);
	struct mpu6050_event *mpu_ev = mpu->mpu_ev;
	struct input_dev *input = mpu->input;
	static int test = 0;
	
	if(mpu_ev == NULL || input == NULL)
		goto not_init_yet;
	
	//printk("%s, line = %d\n", __FUNCTION__, __LINE__);
	
	memset(mpu_ev,0,sizeof(struct mpu6050_event));
	
	mpu_read_values(mpu, mpu_ev);

#ifdef INPUT_SYSTEM_SUPPORT
	input_report_abs(input, ABS_X, mpu_ev->q1);
	input_report_abs(input, ABS_Y, mpu_ev->q2);
	input_report_abs(input, ABS_Z, mpu_ev->q3);
	input_report_abs(input, ABS_RX, mpu_ev->q4);

	test = !test;
	input_report_key(input, BTN_TOUCH, test);
	
	input_sync(input);
#endif


not_init_yet:
	enable_irq(mpu->irq);
}

static int mpu_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -EINVAL;
	struct mpu6050_device *mpu;
	struct mpu6050_platform_data *pdata = client->dev.platform_data;
	struct input_dev *input_dev;

	dev_t devno = MKDEV(MPU_MAJOR, MPU_MINOR);

	mpu_log("mpu6050 driver  probe!\n");
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{		
		err = -ENODEV;
		dev_err(&client->dev, "probe check functionality failed\n");
		goto check_functionality_failed;	

	}
	
	mpu = kzalloc(sizeof(struct mpu6050_device), GFP_KERNEL);  
	if (mpu == NULL) {  
		err = -ENODEV;	 
		dev_err(&client->dev, "probe kzalloc failed\n");
		goto kzalloc_failed;
	}
	mpu6050 = mpu;
	
	mpu->client = client;

	err = register_chrdev_region(devno, 1, MPU_NAME);
	if (err < 0) {  
        err = -ENODEV;
		dev_err(&client->dev, "probe register chrdev failed\n");
		goto register_chrdev_failed;
    }

	cdev_init(&mpu->cdev, &mpu6050_fops);  
	mpu->cdev.owner = THIS_MODULE;	
	err = cdev_add(&mpu->cdev, devno, 1);  
	if (err < 0) {	
		dev_err(&client->dev, "failed to add device\n");  
		goto add_cdev_failed;	
	}

	mpu->cdev_class = class_create(THIS_MODULE, MPU_NAME);
	if(IS_ERR(mpu->cdev_class))
	{
		dev_err(&client->dev, "failed to add device\n");  
		goto create_class_failed;	
	}
	
	device_create(mpu->cdev_class, NULL, devno, 0, MPU_NAME);

	mpu->irq = client->irq;
	INIT_DELAYED_WORK(&mpu->work, mpu_work);
	
	mpu->model			  	= pdata->model;
	mpu->poll_delay		  	= pdata->poll_delay ? : 1;
	mpu->poll_period 	  	= pdata->poll_period ? : 1;
	snprintf(mpu->phys, sizeof(mpu->phys),
		 "%s/mpu6050", dev_name(&client->dev));

	err = request_irq(mpu->irq, mpu6050_irq, IRQ_TYPE_EDGE_RISING,
			client->dev.driver->name, mpu);
	if (err < 0) {
		mpu_log("irq %d busy?\n", mpu->irq);
		goto request_irq_failed;
	}
#ifdef INPUT_SYSTEM_SUPPORT
	input_dev = input_allocate_device();
	if (!mpu || !input_dev) {
		err = -ENOMEM;
		goto err_free_input_mem;
	}
	mpu->input = input_dev;
	input_dev->name = "MPU6050 DMP Quaternion";
	input_dev->phys = mpu->phys;
	input_dev->id.bustype = BUS_I2C;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(ABS_Z, input_dev->absbit);
	set_bit(ABS_RX, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);


	input_set_abs_params(input_dev, ABS_X, -2147438647, 2147438646, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, -2147438647, 2147438646, 0, 0);
	input_set_abs_params(input_dev, ABS_Z, -2147438647, 2147438646, 0, 0);
	input_set_abs_params(input_dev, ABS_RX, -2147438647, 2147438646, 0, 0);

	err = input_register_device(input_dev);
	if (err) {
		mpu_log("input device register failed\n");
		goto register_input_failed;
	}
#endif

	mpu->dev_id = mpu_get_id(mpu);
	
	if(mpu->dev_id != MPU_ADDR_AD0_LOW && mpu->dev_id != MPU_ADDR_AD0_HIGH)
	{
		mpu_log("mpu_dev1_get_id err\n");
		goto mpu_init_failed;
	}
	mpu_log("WHO_AM_I_dev1:0x%x\n", mpu->dev_id);
	
	if(mpu_var_init(mpu) < 0)
	{
		mpu_log("mpu_dev1_var_init err\n");
		goto mpu_init_failed;
	}

	if(mpu_dmp_init(mpu) < 0)
	{
		mpu_log("mpu_dev1 dmp init err\n");
		goto dmp_init_failed;
	}

	mpu_log("mpu6050 dev1 init OK!!!!!!!!!!!!!!!!!!!!!!\n");
	return 0;

dmp_init_failed:	
	mpu_var_delet(mpu);
mpu_init_failed:
#ifdef INPUT_SYSTEM_SUPPORT
	input_unregister_device(input_dev);
#endif
register_input_failed:
	kfree(input_dev);
err_free_input_mem:
	mpu6050_free_irq(mpu);
request_irq_failed:
	class_destroy(mpu->cdev_class);
create_class_failed:
	cdev_del(&mpu->cdev);
add_cdev_failed:
	unregister_chrdev_region(devno, 1);
register_chrdev_failed:
	kfree(mpu6050);
kzalloc_failed:
check_functionality_failed:
	return err;
}

static int __devexit mpu_remove(struct i2c_client *client)
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

static struct i2c_driver mpu6050_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = mpu_probe,
	.remove = mpu_remove,
	.id_table = mpu6050_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = MPU_NAME,
	},
};

static int __init mpu_init(void)
{
	int flag = i2c_add_driver(&mpu6050_driver);
	mpu_log("mpu6050 driver init1!flag:%d\n", flag);
	
	return flag;
}
static void __exit mpu_exit(void)
{
	mpu_log("mpu6050 driver exit\n");
	i2c_del_driver(&mpu6050_driver);
} 

late_initcall(mpu_init);
module_exit(mpu_exit);
MODULE_DEVICE_TABLE(i2c, mpu6050_id);
MODULE_AUTHOR("Ken");
MODULE_DESCRIPTION("MPU6050 driver demo");
MODULE_LICENSE("GPL");
