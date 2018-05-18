/***********************************************************************************
 * encoder driver for exynos4412
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
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include "linux/timer.h"
#include "linux/jiffies.h"

#include <plat/exynos4.h>
#include <plat/gpio-cfg.h>

#include <mach/gpio.h>
#include <mach/regs-gpio.h>

#include <asm/io.h>

#include "encode.h"

#define encoder_order_XYZ
//#define encoder_order_YZX
//#define encoder_order_ZXY


static struct proc_dir_entry *encoder_proc_entry;
struct encode_device *encoder;
struct timer_list demo_timer;


static void encode_work(struct work_struct *work)
{
	struct encode_device *encode =
			container_of(to_delayed_work(work), struct encode_device, work);
	struct input_dev *input = encode->input;
	static int test = 0;
	
	if(input == NULL)
		return ;
	

	input_report_abs(input, ABS_X, encode->pause);
	input_report_abs(input, ABS_Y, encode->rand);

	test = !test;
	input_report_key(input, BTN_TOUCH, test);
	
	input_sync(input);
}


static irqreturn_t encode_x_irq(int irq, void *handle)
{
#ifdef encoder_order_XYZ
	if(gpio_get_value(ENCODER_PIN_Y))
	{
		encoder->now_way = 0;
		encoder->pause--;
	}
	else
	{
		encoder->now_way = 1;
		encoder->pause++;
	}
#endif
#ifdef encoder_order_YZX
	if(!encoder->now_way)
		encoder->rand--;
	else
		encoder->rand++;
#endif

	return IRQ_HANDLED;
}

static irqreturn_t encode_y_irq(int irq, void *handle)
{
#ifdef encoder_order_YZX
	if(gpio_get_value(ENCODER_PIN_Z))
	{
		encoder->now_way = 0;
		encoder->pause--;
	}
	else
	{
		encoder->now_way = 1;
		encoder->pause++;
	}
#endif
#ifdef encoder_order_ZXY
	if(!encoder->now_way)
		encoder->rand--;
	else
		encoder->rand++;
#endif
	return IRQ_HANDLED;
}

static irqreturn_t encode_z_irq(int irq, void *handle)
{
#ifdef encoder_order_ZXY
	if(gpio_get_value(ENCODER_PIN_X))
	{
		encoder->now_way = 0;
		encoder->pause--;
	}
	else
	{
		encoder->now_way = 1;
		encoder->pause++;
	}
#endif
#ifdef encoder_order_XYZ
	if(!encoder->now_way)
		encoder->rand--;
	else
		encoder->rand++;
#endif
	return IRQ_HANDLED;
}


static void encode_timer_func(unsigned long data)
{
	schedule_delayed_work(&encoder->work, ENCODER_DELAY_WORK_INTERVAL);
	mod_timer(&demo_timer,jiffies + HZ / 2);
}



static int encode_probe(struct platform_device *pdev)
{
	int err = -EINVAL;
	struct encode_device *encode_ohm;
	struct input_dev *input_dev;
	
	encode_log("encode driver probe\n");

	encode_ohm = kzalloc(sizeof(struct encode_device), GFP_KERNEL);  
	if (encode_ohm == NULL) {  
		err = -ENODEV;	 
		dev_err(&pdev->dev, "probe kzalloc failed\n");
		goto kzalloc_failed;
	}
	encoder = encode_ohm;
	encode_ohm->pause = 0;

	
	err = gpio_request_one(ENCODER_PIN_X, GPIOF_IN, "Encode_X_IRQ");
	if (err) {
		printk(KERN_ERR "failed to request Encode_X_IRQ pin\n");
		goto gpio_request_failed;
	}
	err = gpio_request_one(ENCODER_PIN_Y, GPIOF_IN, "Encode_Y_IRQ");
	if (err) {
		printk(KERN_ERR "failed to request Encode_Y_IRQ pin\n");
		goto gpio_request_failed;
	}
	err = gpio_request_one(ENCODER_PIN_Z, GPIOF_IN, "Encode_Z_IRQ");
	if (err) {
		printk(KERN_ERR "failed to request Encode_Z_IRQ pin\n");
		goto gpio_request_failed;
	}

	s3c_gpio_cfgpin(ENCODER_PIN_X, S3C_GPIO_SFN(0xF));
	s3c_gpio_cfgpin(ENCODER_PIN_Y, S3C_GPIO_SFN(0xF));
	s3c_gpio_cfgpin(ENCODER_PIN_Z, S3C_GPIO_SFN(0xF));
	
	s3c_gpio_setpull(ENCODER_PIN_X, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(ENCODER_PIN_Y, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(ENCODER_PIN_Z, S3C_GPIO_PULL_NONE);
	
	gpio_free(ENCODER_PIN_X);
	gpio_free(ENCODER_PIN_Y);
	gpio_free(ENCODER_PIN_Z);


	err = request_irq(ENCODER_IRQ_X, encode_x_irq, IRQ_TYPE_EDGE_RISING,
				pdev->dev.driver->name, NULL);
	if (err) {
		printk(KERN_ERR "failed to request Encode_X_IRQ\n");
		return -1;
	}
	err = request_irq(ENCODER_IRQ_Y, encode_y_irq, IRQ_TYPE_EDGE_RISING,
				pdev->dev.driver->name, NULL);
	if (err) {
		printk(KERN_ERR "failed to request Encode_Y_IRQ\n");
		return -1;
	}
	err = request_irq(ENCODER_IRQ_Z, encode_z_irq, IRQ_TYPE_EDGE_RISING,
				pdev->dev.driver->name, NULL);
	if (err) {
		printk(KERN_ERR "failed to request Encode_Z_IRQ\n");
		return -1;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		goto err_free_input_mem;
	}
	encode_ohm->input = input_dev;
	input_dev->name = "Encoder Quaternion";

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);


	input_set_abs_params(input_dev, ABS_X, -2147438647, 2147438646, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, -2147438647, 2147438646, 0, 0);

	err = input_register_device(input_dev);
	if (err) {
		encode_log("input device register failed\n");
		goto register_input_failed;
	}

	INIT_DELAYED_WORK(&encode_ohm->work, encode_work);

	setup_timer(&demo_timer, encode_timer_func, (unsigned long)"demo_timer!");
	demo_timer.expires = jiffies + 1*HZ;
	add_timer(&demo_timer);


	return 0;
	
register_input_failed:

err_free_input_mem:

gpio_request_failed:
	kfree(encoder);
kzalloc_failed:
	return err;

}

static int encode_remove (struct platform_device *pdev)
{	
	encode_log("encode remove!\n");	
	misc_deregister(pdev);	
	del_timer(&demo_timer);
	return 0;
}

static int encode_suspend (struct platform_device *pdev, pm_message_t state)
{	
	encode_log("encode suspend:power off!\n");	
	return 0;
}

static int encode_resume (struct platform_device *pdev)
{	
	encode_log("encode resume:power on!\n");	
	return 0;
}


static struct platform_driver encode_driver = {	
	.probe = encode_probe,	
	.remove = __devexit_p(encode_remove),	
	.suspend = encode_suspend,	
	.resume = encode_resume,	
	.driver = {		
		.name = DRIVER_NAME,		
		.owner = THIS_MODULE,	
	},
};


static int __init encode_init(void)
{
	encode_log("encode driver init\n");
	return platform_driver_register(&encode_driver);
}
static void __exit encode_exit(void)
{
	encode_log("encode driver exit\n");
	platform_driver_unregister(&encode_driver);
	del_timer(&demo_timer);
} 

late_initcall(encode_init);
module_exit(encode_exit);

MODULE_AUTHOR("Ken");
MODULE_DESCRIPTION("encode driver demo");
MODULE_LICENSE("GPL");
