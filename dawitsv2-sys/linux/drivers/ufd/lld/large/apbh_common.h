////////////////////////////////////////////////////////////////////////////////
// Filename: apbh_common.h
//
// Description: Include file for various commonly useful APBH dma code.
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) SigmaTel, Inc. Unpublished
//
// SigmaTel, Inc.
// Proprietary & Confidential
//
// This source code and the algorithms implemented therein constitute
// confidential information and may compromise trade secrets of SigmaTel, Inc.
// or its associates, and any unauthorized use thereof is prohibited.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef APBH_COMMON_H
#define APBH_COMMON_H

#include <linux/completion.h>
#include <linux/time.h>

#include <asm/arch/37xx/regsapbh.h>
//#include <asm/arch/37xx/hw_dma.h>

#include <asm/arch/hardware.h> 

#define APBH_GPMI_CS(channel)    (channel & 0x3)

#define APBH_CHANNEL_MASK(channel) (0x1 << (channel & 0x7))
#define APBH_IRQ(channel_mask) ((channel_mask & 0xff) << BP_APBH_CTRL1_CH0_CMDCMPLT_IRQ)

#define APBH_SEMA_MAX 0xFF

//#define stmp_virt_to_phys(vaddr)	virt_to_phys(vaddr)
//#define stmp_phys_to_virt(paddr)	phys_to_virt(paddr)

#include <asm/cacheflush.h>

//#define rfs_clean_dcache_range(b, e)	clean_dcache_area((void *)b, (int)(e-b))
//#define rfs_invalidate_dcache_range(b, e)
#define rfs_clean_dcache_range(b, e)	dmac_clean_range((void *)b, (void *)e)
#define rfs_invalidate_dcache_range(b, e) dmac_inv_range((void *)b, (void *)e)

#define invalidate_dcache_range(b, e)
#define flush_dcache_range(b, e)

extern inline unsigned get_usec(void)
{
	return HW_DIGCTL_MICROSECONDS_RD();
}

extern inline unsigned get_usec_elapsed(unsigned start, unsigned end)
{
#if 0
    if (start <= end) {
        return (end - start);
    }
    else {
        return (999999 - start + 1 + end);
    }
#endif

    return(end - start);
}

void tprintf_apbh_channel_registers(unsigned verbosity, unsigned channel);
void tprintf_apbh_buffer(unsigned verbosity, const void* buffer, unsigned bytes);
void tprintf_apbh_descriptor(unsigned verbosity, const void*  descriptor, unsigned pio_count);
void tprintf_apbh_chain(unsigned verbosity, const void*  chain);


#define USE_LLD_ISR        0

#if USE_LLD_ISR

typedef struct { 
	struct completion status[8]; /* 0-ECC, 4-chip1, 5-chip2 */
} lld_device_t; 

extern lld_device_t lld_dev;

#endif  /* USE_LLD_ISR */ 

enum
{
	APBH_SUCCESS         = 0,
	APBH_FAILURE         = 1,
	APBH_TIMEOUT         = 2,
	APBH_NAND_PROTECTED  = 3,
	APBH_NAND_BUSY       = 4,
	APBH_NAND_FAIL       = 5,
	APBH_IRQ_TIMEOUT     = 6,
	APBH_MISSING_IRQ     = 7
};


#define APBH_DMA_WORDS  3

#define APBH_DMA_PIO1_WORDS   (APBH_DMA_WORDS + 1)
#define APBH_DMA_PIO2_WORDS   (APBH_DMA_WORDS + 2)
#define APBH_DMA_PIO3_WORDS   (APBH_DMA_WORDS + 3)
#define APBH_DMA_PIO4_WORDS   (APBH_DMA_WORDS + 4)
#define APBH_DMA_PIO5_WORDS   (APBH_DMA_WORDS + 5)
#define APBH_DMA_PIO6_WORDS   (APBH_DMA_WORDS + 6)
#define APBH_DMA_PIO7_WORDS   (APBH_DMA_WORDS + 7)
#define APBH_DMA_PIO8_WORDS   (APBH_DMA_WORDS + 8)
#define APBH_DMA_PIO9_WORDS   (APBH_DMA_WORDS + 9)
#define APBH_DMA_PIO10_WORDS  (APBH_DMA_WORDS + 10)
#define APBH_DMA_PIO11_WORDS  (APBH_DMA_WORDS + 11)
#define APBH_DMA_PIO12_WORDS  (APBH_DMA_WORDS + 12)
#define APBH_DMA_PIO13_WORDS  (APBH_DMA_WORDS + 13)
#define APBH_DMA_PIO14_WORDS  (APBH_DMA_WORDS + 14)
#define APBH_DMA_PIO15_WORDS  (APBH_DMA_WORDS + 15)


struct _apbh_dma_t;

struct _apbh_dma_pio1_t;
struct _apbh_dma_pio2_t;
struct _apbh_dma_pio3_t;
struct _apbh_dma_pio4_t;
struct _apbh_dma_pio5_t;
struct _apbh_dma_pio6_t;
struct _apbh_dma_pio7_t;
struct _apbh_dma_pio8_t;
struct _apbh_dma_pio9_t;
struct _apbh_dma_pio10_t;
struct _apbh_dma_pio11_t;
struct _apbh_dma_pio12_t;
struct _apbh_dma_pio13_t;
struct _apbh_dma_pio14_t;
struct _apbh_dma_pio15_t;

typedef struct _apbh_dma_t apbh_dma_t;

typedef struct _apbh_dma_pio1_t apbh_dma_pio1_t;
typedef struct _apbh_dma_pio2_t apbh_dma_pio2_t;
typedef struct _apbh_dma_pio3_t apbh_dma_pio3_t;
typedef struct _apbh_dma_pio4_t apbh_dma_pio4_t;
typedef struct _apbh_dma_pio5_t apbh_dma_pio5_t;
typedef struct _apbh_dma_pio6_t apbh_dma_pio6_t;
typedef struct _apbh_dma_pio7_t apbh_dma_pio7_t;
typedef struct _apbh_dma_pio8_t apbh_dma_pio8_t;
typedef struct _apbh_dma_pio9_t apbh_dma_pio9_t;
typedef struct _apbh_dma_pio10_t apbh_dma_pio10_t;
typedef struct _apbh_dma_pio11_t apbh_dma_pio11_t;
typedef struct _apbh_dma_pio12_t apbh_dma_pio12_t;
typedef struct _apbh_dma_pio13_t apbh_dma_pio13_t;
typedef struct _apbh_dma_pio14_t apbh_dma_pio14_t;
typedef struct _apbh_dma_pio15_t apbh_dma_pio15_t;


struct _apbh_dma_t
{
    volatile apbh_dma_t*           nxt;
    volatile hw_apbh_chn_cmd_t     cmd;
    volatile void*                 bar;
};


struct _apbh_dma_pio1_t
{
    volatile apbh_dma_t*           nxt;
    volatile hw_apbh_chn_cmd_t     cmd;
    volatile void*                 bar;
    volatile reg32_t               pio[1];
};



extern apbh_dma_t*  APBH_SUCCESS_DMA;
extern apbh_dma_t*  APBH_FAILURE_DMA;
extern apbh_dma_t*  APBH_TIMEOUT_DMA;


unsigned char* init_apbh_descriptors(unsigned char *buf, int left);

void apbh_setup_channel(unsigned apbh_channel, apbh_dma_t* chain);
void apbh_start_channel(unsigned apbh_channel, unsigned semaphore_increment);
unsigned apbh_poll_channel(unsigned apbh_channel, unsigned timeout);
unsigned apbh_wait_channel(unsigned apbh_channel, unsigned timeout);
unsigned apbh_wait_irqs(unsigned irq_mask, unsigned timeout);


struct _apbh_statistic {
	int max_poll;
	int min_poll;

	int max_wait;
	int min_wait; 
};

typedef struct _apbh_statistic apbh_statistic_t;
extern apbh_statistic_t apbh_statistic;

void apbh_init_statistic(void);
void apbh_print_statistic(void);

#endif // APBH_COMMON_H

/* 
  $Log: apbh_common.h,v $
  Revision 1.11  2005/11/08 04:18:01  hcyun
  remove warning..
  chain itself to avoid missing irq problem.

  - hcyun

  Revision 1.10  2005/11/03 02:28:46  hcyun
  support SRAM descriptor
  Improved missing IRQ handling
  Now used by HWECC also.

  - hcyun

  Revision 1.9  2005/09/28 07:35:41  hcyun
  support interrupt

  - hcyun

  Revision 1.8  2005/09/15 00:26:14  hcyun
  reset handling
  missing irq handling improved..

  - hcyun

  Revision 1.7  2005/08/20 00:58:10  biglow
  - update rfs which is worked fib fixed chip only.

  Revision 1.6  2005/07/18 12:37:16  hcyun
  1GB, 1plane, SW_COPYBACK, NO_ECC,

  - hcyun

  Revision 1.4  2005/07/04 13:12:38  hcyun
  possible bug??.. apbh poll/wait fixed..

  - hcyun

  Revision 1.3  2005/05/15 23:11:51  hcyun
  ..

  - hcyun

*/ 
  
