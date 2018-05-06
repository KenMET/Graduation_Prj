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

#include <plat/exynos4.h>
#include <plat/gpio-cfg.h>

#include <mach/gpio.h>
#include <mach/regs-gpio.h>

#include <asm/io.h>

#include "encode.h"

struct encode_device *encoder;


static irqreturn_t encode_x_irq(int irq, void *handle)
{
	//encode_log("encode x_irq\n");
	if(gpio_get_value(ENCODER_PIN_Y))
		encoder->pause++;
	else
		encoder->pause--;
	return IRQ_HANDLED;
}

static irqreturn_t encode_y_irq(int irq, void *handle)
{
	encode_log("encode y_irq\n");

	return IRQ_HANDLED;
}

static irqreturn_t encode_z_irq(int irq, void *handle)
{
	int tmp = encoder->pause;
	encode_log("%d\n", tmp);

	return IRQ_HANDLED;
}



static int encode_probe(struct platform_device *pdev)
{
	int err = -EINVAL;
	struct encode_device *encode_ohm;
	
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
	//err = request_irq(ENCODER_IRQ_Y, encode_y_irq, IRQ_TYPE_EDGE_RISING,
	//			pdev->dev.driver->name, NULL);
	//if (err) {
	//	printk(KERN_ERR "failed to request Encode_Y_IRQ\n");
	//	return -1;
	//}
	err = request_irq(ENCODER_IRQ_Z, encode_z_irq, IRQ_TYPE_EDGE_RISING,
				pdev->dev.driver->name, NULL);
	if (err) {
		printk(KERN_ERR "failed to request Encode_Z_IRQ\n");
		return -1;
	}


	return 0;


gpio_request_failed:
	kfree(encoder);
kzalloc_failed:
	return err;

}

static int encode_remove (struct platform_device *pdev)
{	
	encode_log("encode remove!\n");	
	misc_deregister(pdev);	
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
} 

late_initcall(encode_init);
module_exit(encode_exit);

MODULE_AUTHOR("Ken");
MODULE_DESCRIPTION("encode driver demo");
MODULE_LICENSE("GPL");
