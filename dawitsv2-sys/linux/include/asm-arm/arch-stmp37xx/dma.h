/*
 *  linux/include/asm-arm/arch-stmp37xx/dma.h
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
#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H

#include <linux/device.h>

#define MAX_DMA_ADDRESS		0xffffffff

/* external */
#define MAX_DMA_CHANNELS	0

/* internal */
#define DMA_LCDIF	(0)
#define DMA_SSP1	(1)
#define DMA_SSP2	(2)
#define DMA_NAND0	(3)
#define DMA_NAND1	(4)
#define DMA_NAND2	(5)
#define DMA_NAND3	(6)
#define DMA_ADC		(7)
#define DMA_DAC		(8)
#define DMA_SPDIF	(9)
#define DMA_SAIF2	(10)
#define DMA_I2C		(11)
#define DMA_SAIF1	(12)
#define DMA_DRI		(13)
#define DMA_UART_RX	(14)
#define DMA_IRDA_RX	(15)
#define DMA_UART_TX	(16)
#define DMA_IRDA_TX	(17)

#define STMP37XX_DMA_ERROR    (1)
#define STMP37XX_DMA_COMPLETE (2)

#if !defined (MAX_PIO_WORDS)
#define MAX_PIO_WORDS   (15)
#endif

#define STMP37XX_BUS_APBH	1
#define STMP37XX_BUS_APBX	2

typedef struct
{
    unsigned int next;
    unsigned int cmd;
    unsigned int buf_ptr;
    unsigned int pio_words[MAX_PIO_WORDS];
} stmp37xx_dma_command_t;

struct stmp37xx_dma_descriptor
{
    stmp37xx_dma_command_t *command;
    dma_addr_t handle;

    /* The virtual address of the buffer pointer */
    void* virtual_buf_ptr;
    /* The next descriptor in a the DMA chain (optional) */
    struct stmp37xx_dma_descriptor* next_descr;
};
typedef struct stmp37xx_dma_descriptor stmp37xx_dma_descriptor_t;

/* Each user should provide to the DMA the following filled with their
 * name.  This is to allocate a different pool to each user.
 */
typedef struct
{
    char* name;
    struct dma_pool* pool;
} stmp37xx_dma_user_t;

int stmp37xx_dma_user_init(struct device *, stmp37xx_dma_user_t *);
void stmp37xx_dma_user_destroy(stmp37xx_dma_user_t *);
int stmp37xx_dma_allocate_command(stmp37xx_dma_user_t *, stmp37xx_dma_descriptor_t *);
void stmp37xx_dma_free_command(stmp37xx_dma_user_t *, stmp37xx_dma_descriptor_t *);
int stmp37xx_dma_reset_channel (int id);

int stmp37xx_dma_go(int id, stmp37xx_dma_descriptor_t *, unsigned int);
int stmp37xx_dma_running(int id);

int __must_check stmp37xx_request_dma(int id, char *,
		void (*handler)(int id, unsigned int, void *), void *);
void stmp37xx_free_dma(int id);


/****--- Circular DMA chain ---****/

/* This struct represents a handle to a circular DMA chain instance.
 *
 * Each command in the chain is associated with a single state.  There
 * exist three states:
 *
 * [free  state]:  The  DMA  channel  will  not  process  the  command
 * (i.e. the semaphore count is not high enough to touch the command).
 *
 * [active state]:  The command is  within reach of the  DMA channel's
 * semaphore count and is expected to be processed.
 *
 * [coocked state]: The command has been served by the DMA channel.
 *
 * The commands of the chain are  initialy set to the free state.  The
 * state transition diagram is as follows:
 *
 * free   -> active  [circ_advance_active]
 * active -> cooked  [circ_addvance_cooked]
 * cooked -> free    [circ_advacne_free]
 *   any  -> free    [circ_clear_chain]
 *
 * So, in real  life you will fill in your free  buffers with data (if
 * it is a DMA-tx command),  advance your commands from free to active
 * and when the DMA interrupt happens then you will advance the active
 * buffers to cooked, process their buffers (if it has been a DMA-recv
 * command)  and then  advance the  cooked buffers  to the  free state
 * again etc.
 *
 * Please note  this chain  is not protected  by any  mutual exclusion
 * mechanism.
 */
typedef struct {
    unsigned total_count;
    stmp37xx_dma_descriptor_t* chain;
    
    unsigned free_index;
    unsigned free_count;
    unsigned active_index;
    unsigned active_count;
    unsigned cooked_index;
    unsigned cooked_count;

	int id;
    int bus;
    unsigned channel;
} circular_dma_chain_t;


/* Make a circular dma chain  with the specified amount of items.  All
 * the commands are initialised to the free state.
 */
bool make_circular_dma_chain( stmp37xx_dma_user_t* user, circular_dma_chain_t* chain,
			      stmp37xx_dma_descriptor_t descriptors[], unsigned items );

/* Set all comands in the free state */
void circ_clear_chain( circular_dma_chain_t* chain );

/* Advance 'count' command from cooked to free. There must be at least
 * as many commands in the cooked state
 */
void circ_advance_free( circular_dma_chain_t* chain, unsigned count );

/* Advance  'count' commands  from free  to active,  thus  making them
 * available  for DMA  processing.  There  must be  at  least as  many
 * command in the free state as 'count'.
 */
void circ_advance_active( circular_dma_chain_t* chain, unsigned count );
    
/* Advance all DMA processed  commands from active to cooked.  Returns
 * the number of cooked commands advanced.
 */
unsigned circ_advance_cooked( circular_dma_chain_t* chain );


/* Get the first of the free commands sequence */
stmp37xx_dma_descriptor_t* circ_get_free_head( const circular_dma_chain_t* chain );

/* Get the head of the cooked command sequence
 */
stmp37xx_dma_descriptor_t* circ_get_cooked_head( const circular_dma_chain_t* chain );

/* Print the circular chain */
void circ_print_chain( const circular_dma_chain_t* chain, int verbose );

#endif /* _ASM_ARCH_DMA_H */

