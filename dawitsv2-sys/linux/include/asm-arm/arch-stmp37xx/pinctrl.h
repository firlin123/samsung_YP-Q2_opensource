/*
 * linux/include/asm/arch-stmp36xx/pinctrl.h
 *
 * This file contains stmp36xx pinctrl related code.
 *
 * Copyright (C) 2005 Samsung Electronics, Inc
 * Author : Heechul Yun <heechul.yun@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ARCH_PINCTRL_H_
#define _ARCH_PINCTRL_H_

#ifndef __KERNEL__
#error This is a kernel API
#endif

#define GPIO_OUT 1 
#define GPIO_IN  0
#define GPIO_MODE 3

extern int set_gpio_pin_func(int bank, int pin, int func); 
extern int get_gpio_pin_func(int bank, int pin); 

extern int set_pin_gpio_val(int bank, int pin, int val);
extern int get_pin_gpio_val(int bank, int pin);

extern int set_pin_gpio_mode(int bank, int pin, int mode); 

#endif /* _ARCH_PINCTRL_H_ */ 
