////////////////////////////////////////////////////////////////////////////////
//
// Filename: gpmi_common.h
//
// Description: Include file for various commonly useful GPMI code.
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

#ifndef _GPMI_COMMON_H_
#define _GPMI_COMMON_H_

#include <linux/kernel.h>
#include <linux/mm.h> 
#include <linux/slab.h>

#include <asm/arch/hardware.h> 
#include <asm/arch/37xx/regsgpmi.h>

#include "print.h" 

//------------------------------------------------------------------------------
// types

typedef unsigned gpmi_err_t;
typedef unsigned gpmi_bool_t;

typedef reg8_t gpmi_cs_t;

#define GPMI_CS_0  ((gpmi_cs_t) 0x1)
#define GPMI_CS_1  ((gpmi_cs_t) 0x2)
#define GPMI_CS_2  ((gpmi_cs_t) 0x4)
#define GPMI_CS_3  ((gpmi_cs_t) 0x8)

#define GPMI_CS_RDY_MASK(cs)  (0x1 << cs)

typedef reg16_t gpmi_dev_t;

#define GPMI_DEV_ATA_CS0        ((gpmi_dev_t) 0x0001)
#define GPMI_DEV_K9F1G08U0M_CS0 ((gpmi_dev_t) 0x0008)
#define GPMI_DEV_KM29U128_CS1   ((gpmi_dev_t) 0x0080)


struct _gpmi_mem_sel_t
{
    gpmi_cs_t   mem_cs;
    gpmi_dev_t  mem_dev;
};

typedef struct _gpmi_mem_sel_t   gpmi_mem_sel_t;
typedef struct _gpmi_mem_sel_t*  p_gpmi_mem_sel_t;


//------------------------------------------------------------------------------
// constants

#define GPMI_PROBECFG_BUS  3
#define GPMI_PROBECFG_PADS 4
#define GPMI_PROBECFG  GPMI_PROBECFG_PADS


//------------------------------------------------------------------------------
// macros

// used for specifying dma channel given gpmi cs
#define GPMI_APBH_CHANNEL(cs)  (4 + (cs & 0x3))

// used for indexing HW_DEBUG bitfields by channel
#define GPMI_DEBUG_READY(channel)           (0x1 << (BP_GPMI_DEBUG_READY0 + channel))
#define GPMI_DEBUG_WAIT_FOR_READY(channel)  (0x1 << (BP_GPMI_DEBUG_WAIT_FOR_READY_END0 + channel))
#define GPMI_DEBUG_SENSE(channel)           (0x1 << (BP_GPMI_DEBUG_SENSE0 + channel))
#define GPMI_DEBUG_DMAREQ(channel)          (0x1 << (BP_GPMI_DEBUG_DMAREQ0 + channel))
#define GPMI_DEBUG_CMD_END(channel)         (0x1 << (BP_GPMI_DEBUG_CMD_END + channel))

#define GPMI_TPRINTF_DEBUG(verbosity, debug) gpmi_tprintf_debug(0, debug)


//------------------------------------------------------------------------------
// functions

#ifndef reg32_t  // hcyun 
// #warning define reg32_t
   #define reg32_t unsigned int 
#endif 

extern void gpmi_enable(gpmi_bool_t use_16_bit_data, reg32_t cs_rdy_mask);
extern void gpmi_disable(void);

extern void gpmi_pad_enable(gpmi_bool_t use_16_bit_data, reg32_t cs_rdy_mask);


extern hw_gpmi_debug_t gpmi_tprintf_debug(unsigned verbosity, hw_gpmi_debug_t debug);

extern unsigned gpmi_poll_debug(reg32_t mask, reg32_t match, unsigned timeout);


#endif // !_GPMI_COMMON_H_

////////////////////////////////////////////////////////////////////////////////
//
// $Log: gpmi_common.h,v $
// Revision 1.1  2005/05/14 07:07:56  hcyun
// copy-back, cache-program, random-input, random-output added.
// fm_driver for stmp36xx added
//
// Revision 1.3  2005/05/10 22:21:00  hcyun
// - Clock Control for GPMI : this must be moved to general clock control code
//   and it should be accessible through user space.
//
// - Currently only works for CS 0 (This is a h/w problem??? )
//
// - TODO:
//   generic clk control
//   full nand erase/program check status chain
//
// Revision 1.2  2005/05/06 21:31:01  hcyun
// - DMA Read ID is working...
// - descriptors : kmalloc/kfree with flush_cache_range()
// - buffers : consistent_alloc/free
//
// TODO: fixing other dma functions..
//
// Revision 1.1  2005/05/05 01:05:37  hcyun
// - Started using virt_to_phys.. not yet complete..
//
// Revision 1.34  2005/02/08 17:10:32  ttoelkes
// updating 'clocks' library
//
// Revision 1.33  2005/01/28 02:28:32  ttoelkes
// added basic CLKCTRL manipulation to common gpmi_enable() function
//
// Revision 1.32  2004/09/29 17:26:39  ttoelkes
// code updates in preparation for simultaneous read on four nand devices
//
// Revision 1.31  2004/09/26 20:01:29  ttoelkes
// saving state of dev in midst of directory reorg
//
// Revision 1.30  2004/09/09 22:35:56  ttoelkes
// sketching some new library code; should still be inert at this point
//
// Revision 1.29  2004/09/03 20:14:18  ttoelkes
// put GPMI_PROBECFG back in public constants section in header
//
// Revision 1.28  2004/09/03 19:05:38  ttoelkes
// implemented full pad enable; still needs to be optimized
//
// Revision 1.27  2004/09/03 16:02:55  ttoelkes
// eliding extraneous long comment that accidentally got checked in yesterday; fixing default verbosity
//
// Revision 1.26  2004/09/02 21:50:05  ttoelkes
//
// Revision 1.25  2004/08/24 00:22:08  ttoelkes
// updated header and many of the tests to reflect recent GPMI register reorganization
//
// Revision 1.24  2004/08/18 23:16:57  ttoelkes
// fixed 'gpmi_enable' so it really enables all of the necessary pads
//
// Revision 1.23  2004/08/12 15:40:43  ttoelkes
// added KM29U128/read_id test to the regression suite...finally
//
// Revision 1.22  2004/08/12 15:29:18  ttoelkes
// K9F1G08U0M read_status and read_id tests compile
//
// Revision 1.21  2004/08/11 22:43:44  ttoelkes
// work in progress
//
// Revision 1.20  2004/08/11 21:50:28  ttoelkes
// significant chunk of the way to getting GPMI tests converted
//
// Revision 1.18  2004/07/21 19:17:36  ttoelkes
// updated test while looking at it for testing broken bitfile
//
// Revision 1.17  2004/07/20 15:42:13  ttoelkes
// checkpointing memcpy_dma test development
//
// Revision 1.16  2004/07/14 18:54:53  ttoelkes
// bringing recent work on test code up to date
//
// Revision 1.15  2004/06/15 22:02:35  ttoelkes
// updated gpmi references for new register naming convention
//
// Revision 1.14  2004/06/15 21:20:49  ttoelkes
// checkpoint from GPMI test development
//
// Revision 1.13  2004/06/15 15:06:46  ttoelkes
// checkpointing work on read_id test
//
// Revision 1.12  2004/06/14 23:14:09  ttoelkes
// fixed minor typo
//
// Revision 1.11  2004/06/14 04:02:35  ttoelkes
// added constants for GPMI probe_mux configs
//
// Revision 1.10  2004/06/11 23:31:02  ttoelkes
// abstracting command blocks into common library functions
//
// Revision 1.9  2004/06/11 15:50:53  ttoelkes
// finished adding err flags reporting
//
// Revision 1.8  2004/06/08 13:36:32  ttoelkes
// modified debug0 print statement so the function call overhead disappears if the verbosity level is too low
//
// Revision 1.7  2004/06/07 20:16:42  ttoelkes
// Read Status test passes in simulation on both HW models for these two nand flash models
//
// Revision 1.6  2004/06/04 22:58:37  ttoelkes
// device reset test works and passes; read status test works and fails; read id test in progress
//
// Revision 1.5  2004/06/04 19:29:49  ttoelkes
// four GPMI tests building; two of them even work
//
// Revision 1.4  2004/06/03 22:21:47  ttoelkes
// added code looking towards immediately imminent tests on emulator
//
// Revision 1.3  2004/06/03 21:29:33  ttoelkes
// delivered working device reset test; added more common infrastructure; fixed build for read id test
//
// Revision 1.2  2004/06/02 20:46:24  ttoelkes
// checkpointing recent GPMI work
//
// Revision 1.1  2004/06/01 05:09:44  dpadgett
// Add validation files
//
////////////////////////////////////////////////////////////////////////////////
