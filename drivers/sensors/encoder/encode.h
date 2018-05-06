/***********************************************************************************
 * encoder head file for exynos4412
 *
 * Copyright (C) 2018 Ken International Ltd.
 *	Liu Beiming <Ken_processor@outlook.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
***********************************************************************************/


#ifndef __ENCODER_H__
#define __ENCODER_H__


#define encode_log(...)		printk(KERN_ALERT __VA_ARGS__)


#define DRIVER_NAME "Encoder"

#define	ENCODER_PIN_X	EXYNOS4_GPX1(6)
#define	ENCODER_PIN_Y	EXYNOS4_GPX3(0)
#define	ENCODER_PIN_Z	EXYNOS4_GPX2(6)

#define	ENCODER_IRQ_X	IRQ_EINT(14)
#define	ENCODER_IRQ_Y	IRQ_EINT(24)
#define	ENCODER_IRQ_Z	IRQ_EINT(22)

struct encode_device{
	int pause;
	struct input_dev *input;
};



#endif

