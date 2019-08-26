/*
 *  linux/arch/arm/mach-stmp37xx/lradc-vddio.c
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

#include <asm/hardware.h>
#include <asm/arch/lradc.h>

#define to_private(x) (void*)(x)
#define to_delay(x) (int)(x)

#define VDDIO_LOOPS_PER_SAMPLE (10)
#define VDDIO_SAMPLES_PER_SEC  (10) /* 10 interrupts per 1 second */

#define VDDIO_FREQ \
	(2000 / ((VDDIO_LOOPS_PER_SAMPLE+1) * VDDIO_SAMPLES_PER_SEC))

#if VDDIO_LOOPS_PER_SAMPLE > 0
# define VDDIO_ADC_ACC (1)
#else
# define VDDIO_ADC_ACC (0)
#endif

int vddio_adc_init(int slot, int channel, void *data)
{
	int delay_slot = to_delay(data);

//	printk("%s(): %d %d\n", __func__, slot, channel);

	config_lradc(channel, 0, VDDIO_ADC_ACC, VDDIO_LOOPS_PER_SAMPLE);
	config_lradc_delay(delay_slot, VDDIO_LOOPS_PER_SAMPLE, VDDIO_FREQ);
	return 0;
}

void vddio_adc_handler(int slot, int channel, void *data)
{
	int delay_slot = to_delay(data);

#if 0
	unsigned int result;
	unsigned long result_jiffy;
	unsigned int ret = result_lradc(channel, &result, &result_jiffy);

	/* interrupt context */

	printk("%d: %s(%lu): slot %d, channel %d, (%d)0x%03x at %lu\n",
			ret, __func__, jiffies, slot, channel,
			result, result, result_jiffy);
#endif
	
	/* re-config */
	vddio_adc_init(slot, channel, data);

	/* re-start */
	start_lradc_delay(delay_slot);
}

static struct lradc_ops vddio_ops = {
	.init           = vddio_adc_init,
	.handler        = vddio_adc_handler,
	.num_of_samples = VDDIO_LOOPS_PER_SAMPLE,
};

static int __init vddio_adc_devinit(void)
{
	int ret;

	printk("%s::%s()\n", __FILE__, __func__);

	ret = request_lradc(LRADC_CH_VDDIO, "VDDIO", &vddio_ops);
	if (ret < 0) {
		printk("%s(): request_lradc() fail(%d)\n", __func__, ret);
		return -EINVAL;
	}	

	ret = request_lradc_delay();
	if (ret < 0) {
		printk("%s(): request_lradc_delay() fail(%d)\n", __func__, ret);
		free_lradc(LRADC_CH_VDDIO, &vddio_ops);
		return -EINVAL;
	}

	assign_lradc_delay(0, ret, LRADC_CH_VDDIO);
	vddio_ops.private_data = to_private(ret);

	enable_lradc(LRADC_CH_VDDIO);

	start_lradc_delay(ret);

	return 0;
}

device_initcall(vddio_adc_devinit);
