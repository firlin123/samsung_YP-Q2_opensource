/*
 *  linux/arch/arm/mach-stmp37xx/dma.c
 *
 *  Copyright (C) 2005-2006 Sigmatel Inc
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
 * Sun Feb 9, SeonKon Choi <bushi@mizi.com>
 *  - initial
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/dmapool.h>
#include <asm/page.h>
#include <asm/arch/dma.h>

unsigned long dma_map_used = 0;

#define STMP37XX_DMA_POOL_ITEM_SIZE sizeof(stmp37xx_dma_command_t)
#define STMP37XX_DMA_POOL_ALIGNMENT (8)

static struct stmp37xx_dma_map {
	int id;
	const int irq;
	const int bus;
	const int ch;
	void (*handler)(int id, unsigned int state, void *data);
	void *data;
} maps[] = {
	[DMA_LCDIF] = {
		.irq = IRQ_LCDIF_DMA,
		.bus = STMP37XX_BUS_APBH,
		.ch = 0,
	},
	[DMA_SSP1] = {
		.irq = IRQ_SSP1_DMA,
		.bus = STMP37XX_BUS_APBH,
		.ch = 1,
	},
	[DMA_SSP2] = {
		.irq = IRQ_SSP2_DMA,
		.bus = STMP37XX_BUS_APBH,
		.ch = 2,
	},
	[DMA_NAND0] = {
		.irq = IRQ_GPMI_DMA, // alternative
		.bus = STMP37XX_BUS_APBH,
		.ch = 4,
	},
	[DMA_NAND1] = {
		.irq = IRQ_GPMI_DMA, // shared
		.bus = STMP37XX_BUS_APBH,
		.ch = 5,
	},
	[DMA_NAND2] = {
		.irq = IRQ_GPMI_DMA, // shared
		.bus = STMP37XX_BUS_APBH,
		.ch = 6,
	},
	[DMA_NAND3] = {
		.irq = IRQ_GPMI_DMA, // shared
		.bus = STMP37XX_BUS_APBH,
		.ch = 7,
	},
	[DMA_ADC] = {
		.irq = IRQ_ADC_DMA,
		.bus = STMP37XX_BUS_APBX,
		.ch = 0,
	},
	[DMA_DAC] = {
		.irq = IRQ_DAC_DMA,
		.bus = STMP37XX_BUS_APBX,
		.ch = 1,
	},
	[DMA_SPDIF] = {
		.irq = IRQ_SPDIF_DMA, // alternative
		.bus = STMP37XX_BUS_APBX,
		.ch = 2,
	},
	[DMA_SAIF2] = {
		.irq = IRQ_SPDIF_DMA, // alternative
		.bus = STMP37XX_BUS_APBX,
		.ch = 2, // alternative
	},
	[DMA_I2C] = {
		.irq = IRQ_I2C_DMA,
		.bus = STMP37XX_BUS_APBX,
		.ch = 3,
	},
	[DMA_SAIF1] = {
		.irq = IRQ_SAIF1_DMA,
		.bus = STMP37XX_BUS_APBX,
		.ch = 4,
	},
	[DMA_DRI] = {
		.irq = IRQ_DRI_DMA,
		.bus = STMP37XX_BUS_APBX,
		.ch = 5,
	},
	[DMA_UART_RX] = {
		.irq = IRQ_UARTAPP_RX_DMA, // alternative
		.bus = STMP37XX_BUS_APBX,
		.ch = 6,
	},
	[DMA_IRDA_RX] = {
		.irq = IRQ_UARTAPP_RX_DMA, // alternative
		.bus = STMP37XX_BUS_APBX,
		.ch = 6,
	},
	[DMA_UART_TX] = {
		.irq = IRQ_UARTAPP_TX_DMA, // alternative
		.bus = STMP37XX_BUS_APBX,
		.ch = 7,
	},
	[DMA_IRDA_TX] = {
		.irq = IRQ_UARTAPP_TX_DMA, // alternative
		.bus = STMP37XX_BUS_APBX,
		.ch = 7,
	},
};

int stmp37xx_dma_user_init(struct device *dev, stmp37xx_dma_user_t* user)
{
    /* Create a pool to allocate dma commands from */
    user->pool = dma_pool_create(user->name, dev, STMP37XX_DMA_POOL_ITEM_SIZE,
				 STMP37XX_DMA_POOL_ALIGNMENT, PAGE_SIZE);

    if (user->pool == NULL)
    {
	BUG();
	return -1;
    }

    return 1;
}

void stmp37xx_dma_user_destroy(stmp37xx_dma_user_t* user)
{
    dma_pool_destroy(user->pool);
}

static inline void __stmp37xx_dma_reset_channel(int bus, int channel)
{
	unsigned int bits = (1<<channel);
	
	switch(bus) {
	case STMP37XX_BUS_APBH:
		/* Make sure block is out of reset */
		HW_APBH_CTRL0_CLR(BM_APBH_CTRL0_CLKGATE | BM_APBH_CTRL0_SFTRST);
		/* Reset channel and wait for it to complete */
		BW_APBH_CTRL0_RESET_CHANNEL(bits);

		while (HW_APBH_CTRL0.B.RESET_CHANNEL & bits)
			barrier();

		break;

	case STMP37XX_BUS_APBX:
		/* Make sure block is out of reset */
		HW_APBX_CTRL0_CLR(BM_APBX_CTRL0_CLKGATE | BM_APBX_CTRL0_SFTRST);
		/* Reset channel and wait for it to complete */
		BW_APBX_CTRL0_RESET_CHANNEL(bits);

		while (HW_APBX_CTRL0.B.RESET_CHANNEL & bits)
			barrier();

		break;

	default:
		BUG();
	}
}

int stmp37xx_dma_reset_channel(int id)
{
	struct stmp37xx_dma_map *map;

	if (id >= ARRAY_SIZE(maps))
		return -ENOENT;

	map = &maps[id];
	__stmp37xx_dma_reset_channel(map->bus, map->ch);
	return 0;
}

int stmp37xx_dma_allocate_command(stmp37xx_dma_user_t* user, stmp37xx_dma_descriptor_t *descriptor)
{
    dma_addr_t handle = 0;

    /* Allocate memory for a command from the buffer */
    descriptor->command = dma_pool_alloc(user->pool, GFP_KERNEL, &handle);
    /* Check it worked */
    if (!descriptor->command) {
        return -EINVAL;
    }

    /* Set the handle (physical address) in the descriptor */
    descriptor->handle = handle;

    return 0;
}

void stmp37xx_dma_free_command(stmp37xx_dma_user_t* user, stmp37xx_dma_descriptor_t *descriptor)
{
    /* Initialise descriptor so we're not tempted to use it */
    descriptor->command = 0;
    descriptor->handle = 0;
    descriptor->virtual_buf_ptr = 0;
    descriptor->next_descr = 0;
    
    /* Return the command memory to the pool */
    dma_pool_free(user->pool, descriptor->command, descriptor->handle);
}

int stmp37xx_dma_go(int id, stmp37xx_dma_descriptor_t *head, reg32_t semaphore)
{
	struct stmp37xx_dma_map *map;

	if (id >= ARRAY_SIZE(maps))
		return -ENOENT;

	map = &maps[id];

	switch(map->bus) {
	case STMP37XX_BUS_APBH:
		/* Set next command */
		BW_APBH_CHn_NXTCMDAR_CMD_ADDR(map->ch, (reg32_t)head->handle);
	
		/* Set counting semaphore (kicks off transfer). Assumes
		peripheral has been set up correctly */
		HW_APBH_CHn_SEMA_WR(map->ch, semaphore);
		break;

	case STMP37XX_BUS_APBX:
		/* Set next command */
		BW_APBX_CHn_NXTCMDAR_CMD_ADDR(map->ch, (reg32_t)head->handle);
	
		/* Set counting semaphore (kicks off transfer). Assumes
		peripheral has been set up correctly */
		HW_APBX_CHn_SEMA_WR(map->ch, semaphore);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

int stmp37xx_dma_running (int id)
{
	struct stmp37xx_dma_map *map;

	if (id >= ARRAY_SIZE(maps))
		return -ENOENT;

	map = &maps[id];
	switch(map->bus) {
	case STMP37XX_BUS_APBH:
		return (HW_APBH_CHn_SEMA(map->ch).B.PHORE != 0);

	case STMP37XX_BUS_APBX:
		return (HW_APBX_CHn_SEMA(map->ch).B.PHORE != 0);
	}
	return -EINVAL;
}

static inline int __stmp37xx_dma_enable_int(int bus, int channel)
{
	switch(bus) {
	case STMP37XX_BUS_APBH:
		HW_APBH_CTRL1_SET(1 << (8 + channel));
		break;

	case STMP37XX_BUS_APBX:
		HW_APBX_CTRL1_SET(1 << (8 + channel));
		break;
	
	default:
		return -EINVAL;
	}
	return 0;
}

static inline int __stmp37xx_dma_disable_int(int bus, int channel)
{
	switch(bus) {
	case STMP37XX_BUS_APBH:
		HW_APBH_CTRL1_CLR(1 << (8 + channel));
		break;

	case STMP37XX_BUS_APBX:
		HW_APBX_CTRL1_CLR(1 << (8 + channel));
		break;
	
	default:
		return -EINVAL;
	}
	return 0;
}

static inline int __stmp37xx_dma_clear_int(int bus, int channel)
{
	switch(bus) {
	case STMP37XX_BUS_APBH:
		HW_APBH_CTRL1_CLR(1 << channel);
		break;

	case STMP37XX_BUS_APBX:
		HW_APBX_CTRL1_CLR(1 << channel);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

int stmp37xx_dma_enable_interrupt(int id)
{
	struct stmp37xx_dma_map *map;

	if (id >= ARRAY_SIZE(maps))
		return -ENOENT;

	map = &maps[id];
	return __stmp37xx_dma_enable_int(map->bus, map->ch);
}

int stmp37xx_dma_disable_int(int id)
{
	struct stmp37xx_dma_map *map;

	if (id >= ARRAY_SIZE(maps))
		return -ENOENT;

	map = &maps[id];
	return __stmp37xx_dma_disable_int(map->bus, map->ch);
}

int stmp37xx_dma_clear_int(int id)
{
	struct stmp37xx_dma_map *map;

	if (id >= ARRAY_SIZE(maps))
		return -ENOENT;

	map = &maps[id];
	return __stmp37xx_dma_clear_int(map->bus, map->ch);
}

static irqreturn_t stmp37xx_dma_isr(int irq, void *dev_id)
{
	struct stmp37xx_dma_map *map = dev_id;
	unsigned int bits;
	unsigned int status = 0, mask = 0, val;
	unsigned long addr = 0;

	if (!map) {
		printk (KERN_WARNING "spurious DMA irq %d\n", irq);
		goto out_isr;
	}

	bits = 1 << map->ch;

	switch (map->bus) {
	case STMP37XX_BUS_APBH:
		addr = HW_APBH_CTRL1_ADDR;
		break;
	case STMP37XX_BUS_APBX:
		addr = HW_APBX_CTRL1_ADDR;
		break;
	default:
		printk("%s(): unknown bus\n", __func__);
		goto out_isr;
	}

	val = *(volatile unsigned int*)(addr);
	if (val & (bits << 16)) {
		/* check error */
		status = STMP37XX_DMA_ERROR;
		mask = bits << 16;
	} else if (val & bits) {
		/* check complete */
		status = STMP37XX_DMA_COMPLETE;
		mask = bits;
	}

	if (map->handler)
		map->handler(map->id, status, map->data);

	/* CLR */
	*(volatile unsigned int *)(addr + 0x8) = mask;

 out_isr:
	return IRQ_HANDLED;
}

int stmp37xx_request_dma(int id, char *name,
		void (*handler)(int id, unsigned int, void *), void *data)
{
	unsigned long flags;
	int ret;
	struct stmp37xx_dma_map *map; // = maps[channel];

	if (!name || !handler)
		return -EINVAL;

	if (id >= ARRAY_SIZE(maps))
		return -ENOENT;

	map = &maps[id];

	if (test_and_set_bit(map->ch, &dma_map_used)) {
		return -EBUSY;	
	}

	local_irq_save(flags);

	__stmp37xx_dma_disable_int(map->bus, map->ch);
	__stmp37xx_dma_reset_channel(map->bus, map->ch);
	
	map->handler = handler;
	map->data = data;

	if (map->irq == IRQ_GPMI_DMA)
		ret = request_irq(map->irq, stmp37xx_dma_isr,
				IRQF_DISABLED | IRQF_SHARED, name, map);
	else
		ret = request_irq(map->irq, stmp37xx_dma_isr,
				IRQF_DISABLED, name, map);
	if (ret) {
		__clear_bit(map->ch, &dma_map_used);
		goto out_request_dma;
	}

	__stmp37xx_dma_clear_int(map->bus, map->ch);
	__stmp37xx_dma_enable_int(map->bus, map->ch);

 out_request_dma:
	local_irq_restore(flags);
	return ret;	
}

void stmp37xx_free_dma(int id)
{
	unsigned long flags;
	struct stmp37xx_dma_map *map; // = maps[channel];

	if (id >= ARRAY_SIZE(maps))
		return;

	map = &maps[id];
	if (!test_and_clear_bit(map->ch, &dma_map_used)) {
		return;
	}
	local_irq_save(flags);
	__stmp37xx_dma_disable_int(map->bus, map->ch);
	__stmp37xx_dma_reset_channel(map->bus, map->ch);
	free_irq(map->irq, map);
	map->handler = NULL;
	map->data = NULL;
	local_irq_restore(flags);
}

static int __init stmp37xx_dma_core_init(void)
{
	int i;
	struct stmp37xx_dma_map *map; // = maps[channel];

	for (i = 0; i < ARRAY_SIZE(maps); i++) {
		map = &maps[i];
		map->id = i;
		map->handler = NULL;
		map->data = NULL;	
	}
	return 0;
}

/* Circular dma chain management
 */
bool make_circular_dma_chain( stmp37xx_dma_user_t* user, circular_dma_chain_t* chain,
			      stmp37xx_dma_descriptor_t descriptors[], unsigned items )
			      
{
    int i;

    if ( items == 0 ) return true;

	for (i = 0; i < items; i++) {
		bool error = true;
		error = stmp37xx_dma_allocate_command(user, &descriptors[i]);

		if (error) {
			BUG();
			/* Couldn't allocate the whole chain, deallocate what has been allocated */
			if ( i > 0 ) i--;
			while ( i > 0 ) {
				stmp37xx_dma_free_command(user, &descriptors[i]);
				i--;
			};
			return false;
		}

		// link 
		if ( i > 0 ) {
			descriptors[i - 1].next_descr = &descriptors[i];
			descriptors[i - 1].command->next = descriptors[i].handle;
		}
	}

    /// make list circular
    descriptors[items - 1].next_descr = &descriptors[0];
    descriptors[items - 1].command->next = descriptors[0].handle;

    chain->total_count = items;
    chain->chain = descriptors;
    chain->free_index = 0;
    chain->active_index = 0;
    chain->cooked_index = 0;        
    chain->free_count = items;
    chain->active_count = 0;
    chain->cooked_count = 0;    

    return true;
}

void circ_clear_chain( circular_dma_chain_t* chain )
{
	if (stmp37xx_dma_running(chain->id) > 0) {
		circ_print_chain( chain, 0 );
		BUG();
	}

    chain->free_index   = 0;
    chain->active_index = 0;
    chain->cooked_index = 0;        
    chain->free_count = chain->total_count;
    chain->active_count = 0;
    chain->cooked_count = 0;    
}

void circ_advance_free( circular_dma_chain_t* chain, unsigned count )
{
	if ( chain->cooked_count < count ) BUG();

	chain->cooked_count -= count;
	chain->cooked_index += count;
	chain->cooked_index %= chain->total_count;

	chain->free_count += count;
}

void circ_advance_active( circular_dma_chain_t* chain, unsigned count )
{
	if ( chain->free_count < count ) BUG();

	chain->free_count -= count;
	chain->free_index += count;
	chain->free_index %= chain->total_count;

	chain->active_count += count;


	switch ( chain->bus ) {
	case STMP37XX_BUS_APBH:
		/* Set counting semaphore (kicks off transfer). Assumes
		   peripheral has been set up correctly */
		BW_APBH_CHn_SEMA_INCREMENT_SEMA(chain->channel, count);
		break;

	case STMP37XX_BUS_APBX:
		/* Set counting semaphore (kicks off transfer). Assumes
		   peripheral has been set up correctly */
		BW_APBX_CHn_SEMA_INCREMENT_SEMA(chain->channel, count);
		break;

	default:
		BUG();
	}    
}

unsigned circ_advance_cooked( circular_dma_chain_t* chain )
{
	unsigned sem_count;
	unsigned cooked;

	switch ( chain->bus ) {
	case STMP37XX_BUS_APBH:
		sem_count = BF_RD( APBH_CHn_SEMA(chain->channel), PHORE );
		break;

	case STMP37XX_BUS_APBX:
		sem_count = BF_RD( APBX_CHn_SEMA(chain->channel), PHORE );
		break;

	default:
		BUG();
	}    

	cooked = chain->active_count - sem_count;

	chain->active_count -= cooked;
	chain->active_index += cooked;
	chain->active_index %= chain->total_count;

	chain->cooked_count += cooked;

	return cooked;
}

stmp37xx_dma_descriptor_t* circ_get_free_head( const circular_dma_chain_t* chain )
{
    return &(chain->chain[ chain->free_index ]);
}

stmp37xx_dma_descriptor_t* circ_get_cooked_head( const circular_dma_chain_t* chain )
{
    return &(chain->chain[ chain->cooked_index ]);
}

void circ_print_chain( const circular_dma_chain_t* chain, int verbose )
{
    int i;
    unsigned short sem_count = 0;
    unsigned dma_debug1;
    unsigned dma_debug2;    
    unsigned current_cmd;
    unsigned next_cmd;
    
    switch( chain->bus )
    {
    case STMP37XX_BUS_APBH:
	sem_count = BF_RD( APBH_CHn_SEMA(chain->channel), PHORE );
	current_cmd = BF_RD( APBH_CHn_CURCMDAR(chain->channel), CMD_ADDR );
	next_cmd = BF_RD( APBH_CHn_NXTCMDAR(chain->channel), CMD_ADDR );

	dma_debug1 = HW_APBH_CHn_DEBUG1_RD(chain->channel);
	dma_debug2 = HW_APBH_CHn_DEBUG2_RD(chain->channel);	
	break;
	
    case STMP37XX_BUS_APBX:
	sem_count = BF_RD( APBX_CHn_SEMA(chain->channel), PHORE );
	current_cmd = BF_RD( APBX_CHn_CURCMDAR(chain->channel), CMD_ADDR );
	next_cmd = BF_RD( APBX_CHn_NXTCMDAR(chain->channel), CMD_ADDR );

	dma_debug1 = HW_APBX_CHn_DEBUG1_RD(chain->channel);
	dma_debug2 = HW_APBX_CHn_DEBUG2_RD(chain->channel);	
	break;
	
    default:
	BUG();
    }    

    
    printk("Circular dma chain status:\n");
    printk("Bus: %u, Channel: %u, Semaphore: %u, dma debug1: 0x%x, dma debug2: 0x%x\n",
	   chain->bus, chain->channel, sem_count, dma_debug1, dma_debug2);
    printk("Current physical dma command is at 0x%x, next physical at 0x%x\n", current_cmd, next_cmd );

    for ( i = 0; i < chain->free_count; i++ )
    {
	printk("F(%u)", (chain->free_index + i) % chain->total_count);
    }

    for ( i = 0; i < chain->active_count; i++ )
    {
	printk("A(%u)", (chain->active_index + i) % chain->total_count);
    }
    for ( i = 0; i < chain->cooked_count; i++ )
    {
	printk("C(%u)", (chain->cooked_index + i) % chain->total_count);
    }

    if ( verbose )
    {
	stmp37xx_dma_descriptor_t* descr = chain->chain;
	printk("\n");
	do {
	    printk("next: 0x%8x, command: 0x%8x, buf: 0x%8x, pio[0]: 0x%8x\n",
		   descr->command->next, descr->command->cmd,
		   descr->command->buf_ptr, descr->command->pio_words[0]);
	    descr = descr->next_descr;
	} while ( descr != chain->chain );
    }
    printk("\n");
}

EXPORT_SYMBOL(stmp37xx_dma_user_init);
EXPORT_SYMBOL(stmp37xx_dma_user_destroy);
EXPORT_SYMBOL(stmp37xx_request_dma);
EXPORT_SYMBOL(stmp37xx_free_dma);
EXPORT_SYMBOL(stmp37xx_dma_allocate_command);
EXPORT_SYMBOL(stmp37xx_dma_free_command);
EXPORT_SYMBOL(stmp37xx_dma_go);
EXPORT_SYMBOL(stmp37xx_dma_running);
EXPORT_SYMBOL(stmp37xx_dma_reset_channel);

EXPORT_SYMBOL(make_circular_dma_chain);
EXPORT_SYMBOL(circ_clear_chain);
EXPORT_SYMBOL(circ_get_cooked_head);
EXPORT_SYMBOL(circ_get_free_head);
EXPORT_SYMBOL(circ_advance_free);
EXPORT_SYMBOL(circ_advance_cooked);
EXPORT_SYMBOL(circ_advance_active);


core_initcall(stmp37xx_dma_core_init);

