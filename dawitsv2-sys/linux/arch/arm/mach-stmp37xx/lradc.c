/*  linux/arch/arm/mach-stmp37xx/lradc.c
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
 * Tue Feb 11, SeonKon Choi <bushi@mizi.com>
 *  - initial
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

#include <asm/hardware.h>
#include <asm/arch/lradc.h>

#define LRADC_SLOT_MAX (8)
#define LRADC_DELAY_MAX (4)

static spinlock_t lradc_spin = SPIN_LOCK_UNLOCKED;
static unsigned long slot_used = 0xffffff00;
static unsigned long delay_used = 0xfffffff0;

static struct lradc_map_s {
	const int slot;
	const int irq;
	int ch;
	struct lradc_ops *ops;
	unsigned int latest_result;
	unsigned int latest_result_raw;
	unsigned int latest_jiffy;
} lradc_maps[LRADC_SLOT_MAX] = {
	[0] = {.slot = 0, .irq = IRQ_LRADC_CH0, .ch = -1, .ops = NULL},
	[1] = {.slot = 1, .irq = IRQ_LRADC_CH1, .ch = -1, .ops = NULL},
	[2] = {.slot = 2, .irq = IRQ_LRADC_CH2, .ch = -1, .ops = NULL},
	[3] = {.slot = 3, .irq = IRQ_LRADC_CH3, .ch = -1, .ops = NULL},
	[4] = {.slot = 4, .irq = IRQ_LRADC_CH4, .ch = -1, .ops = NULL},
	[5] = {.slot = 5, .irq = IRQ_LRADC_CH5, .ch = -1, .ops = NULL},
	[6] = {.slot = 6, .irq = IRQ_LRADC_CH6, .ch = -1, .ops = NULL},
	[7] = {.slot = 7, .irq = IRQ_LRADC_CH7, .ch = -1, .ops = NULL},
};

static inline void __lradc_enable_slot(int slot)
{
	/* IRQ_EN */
	HW_LRADC_CTRL1_SET(1 << (slot + 16));
}

static inline void __lradc_disable_slot(int slot)
{
	/* IRQ_EN */
	HW_LRADC_CTRL1_CLR(1 << (slot + 16));
}

static inline void __lradc_startadc_slot(int slot)
{
	/* SCHEDULE */
	HW_LRADC_CTRL0_SET(1<<slot);
}

static inline void __lradc_get_result(struct lradc_map_s *map)
{
	int slot = map->slot;
	unsigned int result;
	struct lradc_ops *ops = map->ops;

	result = *(volatile unsigned int*)(HW_LRADC_CHn_ADDR(slot));
	map->latest_result = (result & 0x3ffff) / (ops->num_of_samples + 1);
	map->latest_result_raw = result;
	map->latest_jiffy = jiffies;
}

static inline int __lradc_init_callback(int slot)
{
	struct lradc_map_s *map = &lradc_maps[slot];
	struct lradc_ops *ops = map->ops;
	int ret = 0;

	if (ops && ops->init)
		ret = ops->init(map->slot, map->ch, ops->private_data);
	return ret;
}

static inline void __lradc_deinit_callback(int slot)
{
	struct lradc_map_s *map = &lradc_maps[slot];
	struct lradc_ops *ops = map->ops;

	if (ops && ops->deinit)
		ops->deinit(map->slot, map->ch, ops->private_data);
	return;
}

static inline int __lradc_find_slot(int channel)
{
	int i = 0;

	for (i = 0; i < LRADC_SLOT_MAX; i++) {
		if (lradc_maps[i].ch == channel)
			return i;
	}
	printk("%s(): there is no slot assigned with channel %d\n",
			__func__, channel);
	return -EINVAL;
}

static inline struct lradc_map_s *__lradc_find_map(int channel)
{
	int i = 0;

	for (i = 0; i < LRADC_SLOT_MAX; i++) {
		if (lradc_maps[i].ch == channel)
			return &lradc_maps[i];
	}
	printk("%s(): there is no slot assigned with channel %d\n",
			__func__, channel);
	return NULL;
}

int result_lradc(int channel, unsigned int *result, unsigned long *jiffy)
{
	unsigned long flags;
	struct lradc_map_s *map;
	int ret = -ENOENT;

	spin_lock_irqsave(&lradc_spin, flags);
	map = __lradc_find_map(channel);
	if (map) {
		ret = 0;
		*result = map->latest_result;
		*jiffy = map->latest_jiffy;
//			printk("result = %d\n", *result);
//			printk("jiffy = %d\n", jiffy);
	}
	spin_unlock_irqrestore(&lradc_spin, flags);
	return ret;
};

int raw_result_lradc(int channel, unsigned int *result, unsigned long *jiffy)
{
	unsigned long flags;
	struct lradc_map_s *map;
	int ret = -ENOENT;

	spin_lock_irqsave(&lradc_spin, flags);
	map = __lradc_find_map(channel);
	if (map) {
		ret = 0;
		*result = map->latest_result_raw;
		*jiffy = map->latest_jiffy;
	}
	spin_unlock_irqrestore(&lradc_spin, flags);
	return ret;
};

void enable_lradc(int channel)
{
	unsigned long flags;
	int slot;

	spin_lock_irqsave(&lradc_spin, flags);
	slot = __lradc_find_slot(channel);
	if (slot >= 0) {
		__lradc_enable_slot(slot);
		__lradc_init_callback(slot);
	}
	spin_unlock_irqrestore(&lradc_spin, flags);
}

void disable_lradc(int channel)
{
	unsigned long flags;
	int slot;

	spin_lock_irqsave(&lradc_spin, flags);
	slot = __lradc_find_slot(channel);
	if (slot >= 0) {
		__lradc_disable_slot(slot);
		__lradc_deinit_callback(slot);
	}
	spin_unlock_irqrestore(&lradc_spin, flags);
}

void start_lradc(int channel)
{
	unsigned long flags;
	int slot;

	spin_lock_irqsave(&lradc_spin, flags);
	slot = __lradc_find_slot(channel);
	if (slot >= 0)
		__lradc_startadc_slot(slot);
	spin_unlock_irqrestore(&lradc_spin, flags);
}

int config_lradc(int channel, int divide_by_two, int acc, int num_sample)
{
	int ret = 0, slot = -1;

	spin_lock(&lradc_spin);

	slot = __lradc_find_slot(channel);
	if (slot < 0) {
		ret = slot;
		goto out_config_lradc;
	}

	/* DIVIDE_BY_TWO */
	if (divide_by_two > 0) {
		HW_LRADC_CTRL2_SET((1 << slot) << 24);
	} else if (divide_by_two == 0) {
		HW_LRADC_CTRL2_CLR((1 << slot) << 24);
	} else {
		/* do nothing */
	}

	/* ACCUMULATE */
	if (acc > 0) {
		HW_LRADC_CHn_SET(slot, 1<<29);
	} else if (acc == 0) {
		HW_LRADC_CHn_CLR(slot, 1<<29);
	} else {
		/* do nothing */
	}

	/* NUM_SAMPLES */
	if (num_sample >= 0) {
		HW_LRADC_CHn_CLR(slot, 0x1f<<24);
		HW_LRADC_CHn_SET(slot, (num_sample & 0x1f)<<24);
	} else {
		/* do nothins */
	}

	/* VALUE */
	HW_LRADC_CHn_CLR(slot, 0x3ffff);

 out_config_lradc:
	spin_unlock(&lradc_spin);
	return ret;
}

void reset_lradc(int channel)
{
	/* ToDo */
	config_lradc(channel, 0, 0, 0);
}


static irqreturn_t lradc_isr(int irq, void *dev_id)
{
	struct lradc_map_s *map = dev_id;
	struct lradc_ops *ops = map->ops;
	int slot = map->slot;

	__lradc_get_result(map);

	if (ops && ops->handler)
		ops->handler(slot, map->ch, ops->private_data);

	/* clear pending */
	HW_LRADC_CTRL1_CLR(1 << slot);

	return IRQ_HANDLED;
}

int request_lradc(int channel, const char *name, struct lradc_ops *client_ops)
{
	int ret, slot = -1;
	struct lradc_map_s *map;

	spin_lock(&lradc_spin);
	if (channel == LRADC_CH_VDDIO) {
		slot = 6;
	} else
	if (channel == LRADC_CH_BATTERY) {
		slot = 7;
	} else {
		unsigned long tmp;
		tmp = slot_used;
		slot_used |= 0xc0;
		slot = ffz(slot_used);
		slot_used = tmp;
	}

	if (slot < 0) {
		spin_unlock(&lradc_spin);
		return -ENOSPC;
	}

	if (__test_and_set_bit(slot, &slot_used)) {
		spin_unlock(&lradc_spin);
		return -ENOSPC;
	}

	map = &lradc_maps[slot];
	map->ch = channel;
	map->ops = client_ops;

	__lradc_disable_slot(slot);

	spin_unlock(&lradc_spin);

	ret = request_irq(map->irq, lradc_isr, IRQF_DISABLED, name, map);
	if (ret < 0) {
		map->ch = -1;
		map->ops = NULL;
		clear_bit(slot, &slot_used);
		return ret;
	}

	/* assign slot with given channel */
	HW_LRADC_CTRL4_CLR(0xf << (slot * 4));
	HW_LRADC_CTRL4_SET((channel & 0xf) << (slot * 4));

	return 0;
}

void free_lradc(int channel, struct lradc_ops *client_ops)
{
	unsigned long flags;
	struct lradc_map_s *map;

	spin_lock_irqsave(&lradc_spin, flags);

	map = __lradc_find_map(channel);
	if (!map) {
		printk("%s(): there is no slot for channel %d\n",
				__func__, channel);
		goto out_free_lradc;
	}

	if (!__test_and_clear_bit(map->slot, &slot_used)) {
		printk("%s(): slot %d was not occupied\n",
				__func__, map->slot);
		goto out_free_lradc;
	}

	if (map->ops != client_ops) {
		printk("%s(): not matched", __func__);
		goto out_free_lradc;
	}

	__lradc_disable_slot(map->slot);
	free_irq(map->irq, map);
	map->ch = -1;
	map->ops = NULL;

 out_free_lradc:
	spin_unlock_irqrestore(&lradc_spin, flags);
}

int request_lradc_delay(void)
{
	int index = ffz(delay_used);

	if (index < 0)
		return -ENOSPC;

	set_bit(index, &delay_used);

	return index;
}

void free_lradc_delay(int delay_slot)
{
	if (delay_slot < 4)
		clear_bit(delay_slot, &delay_used);
}

void assign_lradc_delay(int type, int delay_slot, int id)
{
	unsigned long flags, addr;
	unsigned int slot;

	spin_lock_irqsave(&lradc_spin, flags);

	addr = HW_LRADC_DELAYn_ADDR(delay_slot);
	*(volatile unsigned int*)(addr + 0x8) = 0xff << 24;
	*(volatile unsigned int*)(addr + 0x8) = 0xf << 16;

	switch (type) {
	case 0: /* delay */
		slot = __lradc_find_slot(id);
		if (slot < 0) {
			goto out_assign_lradc_delay;
		}
		*(volatile unsigned int*)(addr + 0x4) =
				(1 << (slot & 0xff)) << 24;
		break;
	case 1: /* trigger */
		*(volatile unsigned int*)(addr + 0x4) = (1 << (id & 0xf)) << 16;
		break;
	}

 out_assign_lradc_delay:
	spin_unlock_irqrestore(&lradc_spin, flags);
}

void config_lradc_delay(int delay_slot, int loop_count, int delay)
{
	spin_lock(&lradc_spin);

	if (loop_count >= 0) {
		HW_LRADC_DELAYn_CLR(delay_slot, (0x1f << 11));
		HW_LRADC_DELAYn_SET(delay_slot, ((loop_count & 0x1f) << 11));
	} else {
		/* do nothing */
	}

	if (delay >= 0) {
		HW_LRADC_DELAYn_CLR(delay_slot, (0x7ff));
		HW_LRADC_DELAYn_SET(delay_slot, (delay & 0x7ff));
	} else {
		/* do nothing */
	}

	spin_unlock(&lradc_spin);
}

void reset_lradc_delay(int delay_slot)
{
	config_lradc_delay(delay_slot, 0, 0);
}
void start_lradc_delay(int delay_slot)
{
	/* KICK */
	HW_LRADC_DELAYn_SET(delay_slot, (1<<20));
}

EXPORT_SYMBOL_GPL(enable_lradc);
EXPORT_SYMBOL_GPL(disable_lradc);
EXPORT_SYMBOL_GPL(request_lradc);
EXPORT_SYMBOL_GPL(free_lradc);
EXPORT_SYMBOL_GPL(config_lradc);
EXPORT_SYMBOL_GPL(reset_lradc);
EXPORT_SYMBOL_GPL(start_lradc);
EXPORT_SYMBOL_GPL(result_lradc);
EXPORT_SYMBOL_GPL(raw_result_lradc);

EXPORT_SYMBOL_GPL(request_lradc_delay);
EXPORT_SYMBOL_GPL(free_lradc_delay);
EXPORT_SYMBOL_GPL(assign_lradc_delay);
EXPORT_SYMBOL_GPL(config_lradc_delay);
EXPORT_SYMBOL_GPL(reset_lradc_delay);
EXPORT_SYMBOL_GPL(start_lradc_delay);

static int __init stmp37xx_lradc_core_init(void)
{
	// Clear the Soft Reset for normal operation
	//BF_CLR(LRADC_CTRL0, SFTRST);
	HW_LRADC_CTRL0_SET(BM_LRADC_CTRL0_SFTRST);
	HW_LRADC_CTRL0_CLR(BM_LRADC_CTRL0_SFTRST | BM_LRADC_CTRL0_SFTRST);

	/* FIXME: register clock source */
	// Clear the Clock Gate for normal operation
	BF_CLR(LRADC_CTRL0, CLKGATE);      // use bitfield clear macro

#if defined(CONFIG_MACH_ARMA37)
	// Disable the onchip ground ref of LRADC conversions
	BF_CLR( LRADC_CTRL0, ONCHIP_GROUNDREF);
	// Set LRADC conversion clock frequency
	BF_WR(LRADC_CTRL3, CYCLE_TIME, 0); /* 6MHz */
#endif

	return 0;
}

core_initcall(stmp37xx_lradc_core_init);
