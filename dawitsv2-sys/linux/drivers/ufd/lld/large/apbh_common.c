////////////////////////////////////////////////////////////////////////////////
// Filename: apbh_common.c
//
// Description: Implementation file for various commonly useful APBH dma code.
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

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <asm/memory.h> 
#include <asm/io.h>
#include <asm/arch/37xx/regsdigctl.h>
#include <asm/arch/37xx/regsgpmi.h>

#include <asm/arch/hardware.h>

#include <linux/rfs/fm_global.h>

#include "apbh_common.h"
#include "print.h" 

apbh_dma_t*  APBH_SUCCESS_DMA;
apbh_dma_t*  APBH_FAILURE_DMA;
apbh_dma_t*  APBH_TIMEOUT_DMA;

#if DEBUG_SEMA 
static int apbh_channel[8]; 
#endif 

unsigned char* init_apbh_descriptors(unsigned char *buf, int left)
{
	unsigned char *ptr; 
	int used; 

	ptr = buf; 

	APBH_SUCCESS_DMA = (apbh_dma_t *)ptr; 
	APBH_SUCCESS_DMA->nxt = (apbh_dma_t *)stmp_virt_to_phys(APBH_SUCCESS_DMA);
	APBH_SUCCESS_DMA->cmd = (hw_apbh_chn_cmd_t) 
		((reg32_t) (BF_APBH_CHn_CMD_IRQONCMPLT(1) | BF_APBH_CHn_CMD_CHAIN(1) | 
			    BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));

	APBH_SUCCESS_DMA->bar = (void *) APBH_SUCCESS;
	ptr += sizeof(*APBH_SUCCESS_DMA); 

	APBH_FAILURE_DMA = (apbh_dma_t *)ptr; 
	APBH_FAILURE_DMA->nxt = (apbh_dma_t *)stmp_virt_to_phys(APBH_FAILURE_DMA);
	APBH_FAILURE_DMA->cmd = (hw_apbh_chn_cmd_t) 
		((reg32_t) (BF_APBH_CHn_CMD_IRQONCMPLT(1) | BF_APBH_CHn_CMD_CHAIN(1) | 
			    BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
	APBH_FAILURE_DMA->bar = (void *) APBH_NAND_FAIL; /* 5 - check */ 
	ptr += sizeof(*APBH_FAILURE_DMA); 

	APBH_TIMEOUT_DMA = (apbh_dma_t *)ptr; 
	APBH_TIMEOUT_DMA->nxt = (apbh_dma_t *)stmp_virt_to_phys(APBH_TIMEOUT_DMA);
	APBH_TIMEOUT_DMA->cmd = (hw_apbh_chn_cmd_t) 
		((reg32_t) (BF_APBH_CHn_CMD_IRQONCMPLT(1) | BF_APBH_CHn_CMD_CHAIN(1) | 
			    BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER)));
	APBH_TIMEOUT_DMA->bar = (void *) APBH_TIMEOUT;
	ptr += sizeof(*APBH_TIMEOUT_DMA); 

	used = (int)(ptr - buf); 

	if ( used > left ) {
		printk("%s: ERROR: Out of descriptor buffer memory : need %d but %d left\n", 
		       __FUNCTION__, used, left);
	}

	rfs_clean_dcache_range((unsigned)buf, (unsigned)ptr); 
	return ptr; 
}


void apbh_setup_channel(unsigned channel, apbh_dma_t* chain)
{
	unsigned channel_mask = APBH_CHANNEL_MASK(channel); 

	// Reset dma channel. 
	BF_WR(APBH_CTRL0, RESET_CHANNEL, channel_mask); 
	
	// Wait for the reset to complete
    while (HW_APBH_CTRL0.B.RESET_CHANNEL & channel_mask);

	// Clear IRQ flag.
	HW_APBH_CTRL1_CLR(APBH_IRQ(channel_mask)); 

	// Initialize chain pointer.
	BF_WRn(APBH_CHn_NXTCMDAR, channel, CMD_ADDR, (reg32_t) stmp_virt_to_phys(chain));
}


void apbh_start_channel(unsigned channel, unsigned semaphore_increment)
{
#if USE_LLD_ISR 
	init_completion(&(lld_dev.status[channel]));	
#endif 

#if DEBUG_SEMA
	apbh_channel[channel] = 1; 
#endif 
	// Start the chain by incrementing the semaphore.
	BF_WRn(APBH_CHn_SEMA, channel, INCREMENT_SEMA, semaphore_increment);
}
 
unsigned apbh_decode_dma_exit(unsigned channel)
{
	int ret; 
	extern lld_stat_t lld_stat; 

	// Check exit code of dma chain (located in BAR) 
	ret = BF_RDn(APBH_CHn_BAR, channel, ADDRESS);
	
	switch ( ret ) { 
	case APBH_SUCCESS: /* 0 - okay */ 
		break; 
	case APBH_NAND_FAIL:
		PDEBUG("ERROR: ABPH[%d] failed operation\n", channel);
		lld_stat.nand_fail ++;
		break;
	case APBH_TIMEOUT:
		printk("ERROR: APBH[%d] timed out\n", channel); 
		lld_stat.timeout ++;
		break; 
	case APBH_NAND_PROTECTED:
		printk("ERROR: ABPH[%d] write protected\n", channel);
		break;
	default:
		printk("ERROR: APBH[%d] exited with unexpected code 0x%x\n", 
		       channel, ret);
		break;
	}; 
	
	return ret; 
}



/* wait PHORE become zero ensuring that current DMA chain excuted.. */ 
unsigned apbh_poll_channel(unsigned channel, unsigned timeout)
{
	int phore = 0;
	int retry = 0; 

#if DEBUG_SEMA
	if ( apbh_channel[channel] == 0 ) {
		printk("%s: channel(%d) is not started\n", __FUNCTION__, channel); 
		while(1);
	}
#endif 

	while ( retry <= timeout / 10 ) {
		if ((phore = BF_RDn(APBH_CHn_SEMA, channel, PHORE)) == 0 ) {
			return 0; 
		}
		retry ++; 
		udelay(10);
	}

	printk("%s: timeout for %d us. phore = %d\n", __FUNCTION__, timeout, phore);
	while(1);

	return APBH_TIMEOUT; 
}


/*   wait IRQ. confirm completion of DMA chain */ 
unsigned apbh_wait_channel(unsigned channel, unsigned timeout)
{
	unsigned irq_mask = APBH_CHANNEL_MASK(channel);
	unsigned start, duration;
	unsigned curar, phore;
	unsigned got_irq; 

	duration = 0;  
	start = get_usec();

#if USE_LLD_ISR 
	wait_for_completion(&(lld_dev.status[channel]));

	return apbh_decode_dma_exit(channel); 
#else /* !USE_LLD_ISR */ 

#if DEBUG_SEMA
	if ( unlikely(apbh_channel[channel] == 0) ) {
		printk("%s: channel(%d) is not started\n", __FUNCTION__, channel); 
		while(1);
	} else {
		apbh_channel[channel] = 0; 
	}
#endif 
	// Poll for all IRQs being raised at the end of their chains.
	while ( duration <= timeout ) {

		duration = get_usec_elapsed(start, get_usec()); 
		got_irq = ((HW_APBH_CTRL1_RD() & APBH_IRQ(irq_mask)) == APBH_IRQ(irq_mask));
		if ( got_irq ) { 
			signed char decrement = -1; 
			// read the current semaphore
			phore = BF_RDn(APBH_CHn_SEMA, channel, PHORE);

			if ( phore != 1 ) { /* failed before last dma chain */ 
				decrement = - (char)phore;
				printk("%s: phore(%d) decrement(%x)\n", 
				       __FUNCTION__, phore, decrement); 
			}

			// clear the semaphore to stop the end-of-chain loop
			BF_WRn(APBH_CHn_SEMA, channel, INCREMENT_SEMA, (unsigned)decrement);

			return apbh_decode_dma_exit(channel); 
		} 
		udelay(5);
	}
	
	curar = stmp_phys_to_virt(HW_APBH_CHn_CURCMDAR_RD(channel));
	phore = BF_RDn(APBH_CHn_SEMA, channel, PHORE);

	if ( curar == (unsigned)APBH_SUCCESS_DMA || curar == (unsigned)APBH_FAILURE_DMA || 
	     curar == (unsigned)APBH_TIMEOUT_DMA  ) 
	{ /* missing irq */
		extern lld_stat_t lld_stat; 
		lld_stat.missing_irq ++; 
		return apbh_decode_dma_exit(channel); 
	} else {
	    unsigned nxtar = stmp_phys_to_virt(HW_APBH_CHn_NXTCMDAR_RD(channel));
		printk("%s: timeout channel=%d, timeout=%d (%08x), duration=%d (%08x), start=%d (%08x, %08x), phore=%d, curar=0x%08x, nxtar=0x%08x\n\n", 
		       __FUNCTION__, channel, timeout, timeout, duration, duration, start, start, get_usec(), phore, curar, nxtar); 
		
		printk("HW_APBH_CH%d_DEBUG1 register value:\n", channel);
		printk("REQ = %d\n", HW_APBH_CHn_DEBUG1(channel).B.REQ);
		printk("BURST = %d\n", HW_APBH_CHn_DEBUG1(channel).B.BURST);
		printk("KICK = %d\n", HW_APBH_CHn_DEBUG1(channel).B.KICK);
		printk("END = %d\n", HW_APBH_CHn_DEBUG1(channel).B.END);
		printk("SENSE = %d\n", HW_APBH_CHn_DEBUG1(channel).B.SENSE);
		printk("READY = %d\n", HW_APBH_CHn_DEBUG1(channel).B.READY);
		printk("LOCK = %d\n", HW_APBH_CHn_DEBUG1(channel).B.LOCK);
		printk("NEXTCMDADDRVALID = %d\n", HW_APBH_CHn_DEBUG1(channel).B.NEXTCMDADDRVALID);
		printk("RD_FIFO_EMPTY = %d\n", HW_APBH_CHn_DEBUG1(channel).B.RD_FIFO_EMPTY);
		printk("RD_FIFO_FULL = %d\n", HW_APBH_CHn_DEBUG1(channel).B.RD_FIFO_FULL);
		printk("WR_FIFO_EMPTY = %d\n", HW_APBH_CHn_DEBUG1(channel).B.WR_FIFO_EMPTY);
		printk("WR_FIFO_FULL = %d\n", HW_APBH_CHn_DEBUG1(channel).B.WR_FIFO_FULL);
		printk("STATEMACHINE = 0x%02x\n\n", HW_APBH_CHn_DEBUG1(channel).B.STATEMACHINE);
		
		printk("HW_APBH_CH%d_DEBUG2 register value:\n", channel);
		printk("AHB_BYTES = %d\n", HW_APBH_CHn_DEBUG2(channel).B.AHB_BYTES);
		printk("APB_BYTES = %d\n", HW_APBH_CHn_DEBUG2(channel).B.APB_BYTES);
		
		printk("System halted.\n\n\n");
		dump_stack();
		while(1);
		
		return APBH_IRQ_TIMEOUT; 
	}
#endif /* USE_LLD_ISR */

}


////////////////////////////////////////////////////////////////////////////////
//
// $Log: apbh_common.c,v $
// Revision 1.23  2006/07/06 05:16:12  hcyun
// debug
//
// Revision 1.22  2006/05/02 11:05:00  hcyun
// from main z-proj source
//
// Revision 1.22  2006/03/07 11:42:18  hcyun
// ..
//
// Revision 1.21  2005/11/15 06:37:11  hcyun
// handle when PHORE is not 1 (meaning it's not at the end of DMA chain)
//
//
// - hcyun
//
// Revision 1.20  2005/11/08 04:18:01  hcyun
// remove warning..
// chain itself to avoid missing irq problem.
//
// - hcyun
//
// Revision 1.19  2005/11/05 23:30:58  hcyun
// add prev_ecc structure to check previous ecc location.
//
// pipeline option is now located in the fd_physical.h
//
// - hcyun
//
// Revision 1.18  2005/11/03 02:28:46  hcyun
// support SRAM descriptor
// Improved missing IRQ handling
// Now used by HWECC also.
//
// - hcyun
//
// Revision 1.16  2005/10/13 00:17:33  zzinho
// added by heechul in USA
//
// Revision 1.15  2005/10/07 01:49:52  hcyun
// remove IRQ error messages.
//
// - hcyun
//
// Revision 1.14  2005/09/28 07:35:41  hcyun
// support interrupt
//
// - hcyun
//
// Revision 1.13  2005/09/15 00:26:14  hcyun
// reset handling
// missing irq handling improved..
//
// - hcyun
//
// Revision 1.12  2005/08/29 05:07:21  hcyun
// Stable new rfs (?)
//
// - single plane
// - hwecc
// - sw-copyback
//
// - err_recover() bug fix (handling previous write operation)
// - FLM_Read_Page_Group() bug fix
//
// Revision 1.11  2005/07/21 05:44:15  hcyun
// merged yhbae's addition - mainly test code, plus bank selection code (currently it has a bug)
//
// 2-plane operation is not enabled in the flash_spec structure.
//
// - hcyun
//
// Revision 1.10  2005/07/18 12:37:16  hcyun
// 1GB, 1plane, SW_COPYBACK, NO_ECC,
//
// - hcyun
//
// Revision 1.8  2005/07/04 13:12:38  hcyun
// possible bug??.. apbh poll/wait fixed..
//
// - hcyun
//
// Revision 1.7  2005/06/29 09:51:34  hcyun
// flash_sync() error handling position changed..
// apbh_poll_channel() return value is changed.
//
//
// - hcyun
//
// Revision 1.6  2005/06/08 11:31:17  hcyun
// Critical bug fix of serial ri program: add IRQONCOMPLETE on the dma chain. Now, it works exactly at expected speed..
//
// - hcyun
//
// Revision 1.5  2005/06/06 15:39:40  hcyun  
// now timeout is handled using JIFFY counter.. instead of using udelay()
//
// - hcyun
//
// Revision 1.4  2005/05/26 20:57:17  hcyun
// ..
//
// - hcyun
//
// Revision 1.3  2005/05/18 03:37:35  hcyun
// remove AHBP register dump.
//
// - hcyun
//
// Revision 1.2  2005/05/15 23:01:38  hcyun
// lots of cleanup... not yet compiled..
//
// - hcyun
//
// Revision 1.1  2005/05/14 07:07:56  hcyun
// copy-back, cache-program, random-input, random-output added.
// fm_driver for stmp36xx added
//
// Revision 1.4  2005/05/11 04:11:02  hcyun
// - cache flush added for serial program
// - removed entire cache flush code of runtest.c
// - check status is added..
//
// Revision 1.3  2005/05/08 01:49:13  hcyun
// - First working NAND flash low level API set.
//   Read-Id, Read, Write, Erase seems to working..
//
// Revision 1.2  2005/05/06 21:31:01  hcyun
// - DMA Read ID is working...
// - descriptors : kmalloc/kfree with flush_cache_range()
// - buffers : consistent_alloc/free
//
// TODO: fixing other dma functions..
//
// Revision 1.1  2005/05/05 01:05:37  hcyun
// - Started using stmp_virt_to_phys.. not yet complete..
// -------------------------------------------------------------------------
// Revision 1.1.1.1  2005/03/25 14:17:50  ttoelkes
// pulling in Multi-based bringup code piecewise
//
// Revision 1.12  2004/11/30 00:24:19  ttoelkes
// added some useful library code to for APBH work
//
// Revision 1.11  2004/11/01 20:30:52  ttoelkes
// fixing issue with test which is causing breaks in simulation
//
// Revision 1.10  2004/10/11 21:32:31  ttoelkes
// added TPRINTF macro/function pair to dump an APBH channel's registers
//
// Revision 1.9  2004/09/29 20:58:32  ttoelkes
// checkpoint
//
// Revision 1.8  2004/09/24 22:11:51  ttoelkes
// changed standard APBH terminal descriptors back to const
//
// Revision 1.7  2004/09/24 22:01:45  ttoelkes
// checkpoint with no build errors
//
// Revision 1.6  2004/09/24 18:22:48  ttoelkes
// checkpointing new devlopment while it still compiles; not known to actually work
//
// Revision 1.5  2004/08/03 22:57:54  ttoelkes
// migrated APBH and MEMCPY tests to use new header format
//
// Revision 1.4  2004/07/29 00:20:39  ttoelkes
// made buffer code byte-oriented rather than word-oriented
//
// Revision 1.3  2004/07/20 23:33:52  ttoelkes
// checking in changes to test case that appear to be breaking MEMCPY or APBH
//
// Revision 1.2  2004/07/20 22:32:47  ttoelkes
// checkpointing memcpy test work
//
// Revision 1.1  2004/07/20 15:42:13  ttoelkes
// checkpointing memcpy_dma test development
//
////////////////////////////////////////////////////////////////////////////////
