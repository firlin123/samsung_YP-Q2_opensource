/*
 * linux/drivers/input/keyboard/arma37_analog_button.c
 *
 *  Copyright (C) 2008 MIZI Research, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Tue Feb 12, SeonKon Choi <bushi@mizi.com>
 *  - initial
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/input.h>

#include <mizi/key.h>

#include <asm/hardware.h>
#include <asm/arch/lradc.h>
#include <asm-arm/arch-stmp37xx/lradc.h>

#define DEV_NAME "A.Button"

#define MAX_VDDIO_ADC (3500)
#define PRESS_THRESHOLD_ADC (3400)
//#define STMP37XX

//#define BUTTON_ADC_CH		LRADC_CH_0
#define BUTTON_ADC_CH		LRADC_CH_1 //add dhsong 
#define BUTTON_LOOPS_PER_SAMPLE	(0)
#define BUTTON_SAMPLES_PER_SEC	(10) /* 10 interrupts per 1 second */

#define BUTTON_FREQ \
	(2000 / ((BUTTON_LOOPS_PER_SAMPLE+1) * BUTTON_SAMPLES_PER_SEC))

#if BUTTON_LOOPS_PER_SAMPLE > 0
# define BUTTON_ADC_ACC (1)
#else
# define BUTTON_ADC_ACC (0)
#endif

//#define BUTTON_MAX_NUM (11)
#define BUTTON_MAX_NUM (7) //dhsong

#define DEBUG
#ifdef DEBUG
# define dbg_printk(fmt, arg...)  \
        do { \
                 printk("%s(%d) " fmt, __FUNCTION__, __LINE__, ## arg); \
        } while (0)
#else
# define dbg_printk(fmt, arg...) do{}while(0)
#endif // DEBUG

#if 0
static unsigned char button_scancodes[BUTTON_MAX_NUM] = {
	MZ_SCANCODE_MENU,     MZ_SCANCODE_F11,         MZ_SCANCODE_LEFT,
	MZ_SCANCODE_F12,      MZ_SCANCODE_F10,         MZ_SCANCODE_PAGE_UP,
	MZ_SCANCODE_DOWN,     MZ_SCANCODE_PAGE_DOWN,   MZ_SCANCODE_RIGHT,
	MZ_SCANCODE_ACTION,   MZ_SCANCODE_UP 
};
static unsigned int button_adc_value[BUTTON_MAX_NUM] = {
	300, 500, 750, 1100, 1400, 1600, 2000, 2200, 2500, 2800, 3100
};
#endif
#ifdef STMP37XX //stmp37xx
static unsigned char button_scancodes[BUTTON_MAX_NUM] = {
	KEY_BACKSPACE,	KEY_RIGHT,	KEY_RESERVED,	KEY_LEFT,
	KEY_RESERVED,	KEY_UP,      	KEY_RESERVED,	KEY_DOWN,		
	KEY_RESERVED,	KEY_ENTER,	KEY_RESERVED
};
static unsigned int button_adc_value[BUTTON_MAX_NUM] = {
	0, 200, 450, 750, 1000, 1350, 1650, 1950, 2250, 2550, 2800
};
#else // Q2
static unsigned char button_scancodes[BUTTON_MAX_NUM] = {
	KEY_UP,		KEY_DOWN,	KEY_BACKSPACE,
	KEY_RIGHT,	KEY_LEFT,   	KEY_ENTER, 	KEY_RESERVED
};
static unsigned int button_adc_value[BUTTON_MAX_NUM] = {
	//0, 300, 500, 1000, 1200, 1700, 2100
	200, 550, 1000, 1200, 1700, 2100, 2800 ///dhsong
};
#endif

static struct adc_button_s {
	struct input_dev *input_dev;
	int adc_delay_slot;
	unsigned int *adc_values;
	unsigned char *scancodes;
	int last_pressed;
} adc_button = {
	.input_dev = NULL,
	.adc_delay_slot = -1,
	.last_pressed = -1,
	.adc_values = &button_adc_value[0],
	.scancodes = &button_scancodes[0],
};

static inline char *_get_button_name(int index)
{
	char *button_names[BUTTON_MAX_NUM] = {
#ifdef STMP37XX
		"MENU", "FF", "SPARE1", "REW", "SPARE2",
		"VOL+", "SPARE3", "VOL-", "SPARE4", "PLAY/PAUSE",
		"SPARE5"
#else //add dhsong 
		"VOL+", "VOL-", "BACK", "FF", "REW", "ENTER", "SELECT"

#endif
	};
	return button_names[index];
};

static int button_adc_init(int slot, int channel, void *data)
{
	struct adc_button_s *button = data;

	config_lradc(channel, 1, BUTTON_ADC_ACC, BUTTON_LOOPS_PER_SAMPLE);
	config_lradc_delay(button->adc_delay_slot,
			BUTTON_LOOPS_PER_SAMPLE, BUTTON_FREQ);
	return 0;
}

static inline void __release_pressed_key(struct adc_button_s *button)
{
	unsigned char scancode;
	int old_pressed = button->last_pressed;

	if (old_pressed >= 0) {
		scancode = button->scancodes[old_pressed];
		input_report_key(button->input_dev, scancode, 0);
		button->last_pressed = -1;
		dbg_printk("[%s] release %x\n", _get_button_name(old_pressed), scancode);
#if 0
		//if(HW_LRADC_CHn(1).TOGGLE == 1) {}//Tooggle is high.
		static unsigned long channelAverage;
		HW_LRADC_CHn_WR(1, (BF_LRADC_CHn_ACCUMULATE(1)) );
		
		printk("ch1conversionValue = 0x%x\n", HW_LRADC_CHn(1) );
 		
		BF_LRADC_CHn_NUM_SAMPLES(5);
		BF_LRADC_CHn_VALUE(0);		
		
			

		//BF_CLRn(BUTTON_ADC_CH, 0, VALUE); //reg clear,, add dhsong
		//printk("ch1conversionValue = 0x%x\n", HW_LRADC_CHn(1) );
		//BF_CLRn( HW_LRADC_CHn(1), 0, VALUE); //reg clear,, add dhsong


#endif
	}
}

static void button_adc_handler(int slot, int channel, void *data)
{
	struct adc_button_s *button = data;
	unsigned int vddio_value, this_value;
	unsigned long vddio_jiffy, this_jiffy;
	unsigned int adc;
	int new_pressed = -1, old_pressed = -1;
	unsigned char scancode;
	int ret, i = 0;

	ret = result_lradc(LRADC_CH_VDDIO, &vddio_value, &vddio_jiffy);
		//printk("ret = %d\n", ret );
	if (ret < 0) {
		printk("%s(): there is no VDDIO\n", __func__);
		return;
	}
	ret = result_lradc(channel, &this_value, &this_jiffy);
	if (ret < 0) {
		printk("%s(): ???\n", __func__);
		return;
	}
		//printk("ret = %d\n", ret );

	adc = (this_value * MAX_VDDIO_ADC) / vddio_value;

	//printk("[handler: %u %u : %u\n", vddio_value, this_value, adc);

	old_pressed = button->last_pressed;
	//dbg_printk("old_pressed = %d\n\n", old_pressed );

	if (adc < PRESS_THRESHOLD_ADC) {
		for (i = BUTTON_MAX_NUM - 1; i >= 0; i--) {
			if (adc > button->adc_values[i])
				break;
		}
		new_pressed = i;
		//dbg_printk("new_pressed = %d\n\n", new_pressed );
	}
	if (old_pressed != new_pressed) {

		__release_pressed_key(button);

		if (new_pressed >= 0) {
			scancode = button->scancodes[new_pressed];
printk("button->input_dev->name = %s\n\n",button->input_dev->name,  __FILE__, __LINE__);
			input_report_key(button->input_dev, scancode, 1);
			button->last_pressed = new_pressed;
			dbg_printk("[%s] press %x\n", _get_button_name(new_pressed), scancode);
			
			dbg_printk("adc = %d\n", adc );
	//		dbg_printk("channel = %d\n", channel);
	//		dbg_printk("vddio_value = %d\n", vddio_value);
	//		dbg_printk("vddio_jiffy = %d\n", vddio_jiffy);
	//		dbg_printk("this_value = %d\n", this_value);
	//		dbg_printk("this_jiffy = %d\n", this_jiffy);
		}
	}

	button_adc_init(slot, channel, data);
	start_lradc_delay(button->adc_delay_slot);
}

static struct lradc_ops button_ops = {
	.init           = button_adc_init,
	.handler        = button_adc_handler,
	.num_of_samples = BUTTON_LOOPS_PER_SAMPLE,
};

static int __init adc_button_prepare(struct adc_button_s *button)
{
	int ret;
	printk("%s::%s()\n", __FILE__, __func__);

	ret = request_lradc(BUTTON_ADC_CH, DEV_NAME, &button_ops);
	if (ret < 0) {
		printk("%s(): request_lradc() fail(%d)\n", __func__, ret);
		return -EINVAL;
	}	

	ret = request_lradc_delay();
	if (ret < 0) {
		printk("%s(): request_lradc_delay() fail(%d)\n", __func__, ret);
		free_lradc(BUTTON_ADC_CH, &button_ops);
		return -EINVAL;
	}

	button->adc_delay_slot = ret;
	assign_lradc_delay(0, ret, BUTTON_ADC_CH);
	button_ops.private_data = button;

	return 0;
}

static void __devexit adc_button_release(struct adc_button_s *button)
{
	printk("%s()\n", __func__);
	if (button->adc_delay_slot >= 0) {
		free_lradc_delay(button->adc_delay_slot);
		free_lradc(BUTTON_ADC_CH, &button_ops);
		button->adc_delay_slot = -1;
	}
}

static int adc_button_open(struct input_dev *dev)
{
	struct adc_button_s *button = input_get_drvdata(dev);
	printk("%s()\n", __func__);
	button->last_pressed = -1;
	enable_lradc(BUTTON_ADC_CH);
	start_lradc_delay(button->adc_delay_slot);
	return 0;
}

static void adc_button_close(struct input_dev *dev)
{
	struct adc_button_s *button = input_get_drvdata(dev);
	printk("%s()\n", __func__);
	__release_pressed_key(button);
	disable_lradc(BUTTON_ADC_CH);
}

static int __devinit adc_button_probe(struct platform_device *pdev)
{
	int ret, i;
	struct input_dev *input_dev;

	printk("%s()\n", __func__);

	input_dev = input_allocate_device();
	if (!input_dev) {
		printk("%s(): Cannot request keypad device\n", __func__);
		ret = -ENOMEM;
		goto err_alloc;
	}
	
	ret = adc_button_prepare(&adc_button);
	if (ret < 0) {
		goto err_prepare;
	}
	
	adc_button.input_dev =  input_dev;

	input_dev->name = DEV_NAME;
	input_dev->id.bustype = BUS_HOST;
	input_dev->open = adc_button_open;
	input_dev->close = adc_button_close;
	input_dev->dev.parent = &pdev->dev;
	input_dev->keycode = &button_scancodes[0];
	input_dev->keycodemax = ARRAY_SIZE(button_scancodes);

	input_dev->evbit[0] = BIT_MASK(EV_KEY);
	printk("input_dev->evbit[0] = %d, %s, %d\n\n",input_dev->evbit[0],  __FILE__, __LINE__);
	for (i = 0; i < ARRAY_SIZE(button_scancodes); i++) {
		int code = button_scancodes[i];
		set_bit(code, input_dev->keybit);
	}
	clear_bit(0, input_dev->keybit);

	input_set_drvdata(input_dev, &adc_button);
	platform_set_drvdata(pdev, &adc_button);

	ret = input_register_device(input_dev);
	if (ret)
		goto err_register;

	return 0;

 err_register:
	input_set_drvdata(input_dev, NULL);
	adc_button_release(&adc_button);
	platform_set_drvdata(pdev, NULL);

 err_prepare:
	input_free_device(input_dev);
	
 err_alloc:
	return ret;
}

static int __devexit adc_button_remove(struct platform_device *pdev)
{
	struct adc_button_s *button = platform_get_drvdata(pdev);

	printk("%s()\n", __func__);

	adc_button_release(button);

	input_unregister_device(button->input_dev);
	platform_set_drvdata(pdev, NULL);
	
	return 0;
}

#ifdef CONFIG_PM
#else
#define adc_button_suspend NULL
#define adc_button_resume NULL
#endif

static struct platform_driver adc_button_driver = {
	.probe		= adc_button_probe,
	.remove		= __devexit_p(adc_button_remove),
	.suspend	= adc_button_suspend,
	.resume		= adc_button_resume,
	.driver		= {
		.name	= DEV_NAME,
	},
};

static int __init adc_button_dev_init(void)
{
	return platform_driver_register(&adc_button_driver);
}

static void __exit adc_button_dev_exit(void)
{
	platform_driver_unregister(&adc_button_driver);
}

module_init(adc_button_dev_init);
module_exit(adc_button_dev_exit);
