////////////////////////////////////////////////////////////////////////////////
//
// Filename: clocks.h
//
// Description: Declarations of common clocking-related library functions.
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

#ifndef _CLOCKS_H_
#define _CLOCKS_H_

#include <asm/arch/hardware.h> 

#if !defined(HW_3600) && !defined(HW_BRAZO)
  #error "HW_3600 or HW_BRAZO must be defined..." 
#endif 

#include <asm/arch/regs/regs.h>

struct _clocks_mode
{
        // words[0]
        reg16_t  pll_freq;
        reg8_t   cpuclk_interrupt_wait;
        reg8_t   hbusclk_autoslow_rate;
        
        // words[1]
        reg16_t  cpuclk_div;
        reg8_t   hbusclk_div;
        reg8_t   emiclk_div;
        
        // words[2]
        reg16_t  gpmiclk_div;
        reg16_t  sspclk_div;

        // words[3]
        reg16_t  irov_div;
        reg16_t  ir_div;

        // words[4]
        reg8_t   spdifclk_div;
        reg8_t   usb_enable;
        reg16_t  xbusclk_div;
	    
	// words [5]??
	unsigned long lpj; // - hcyun 

};

typedef struct _clocks_mode clocks_mode_t;


extern clocks_mode_t  CLOCKS_MODE_INIT;
extern clocks_mode_t  CLOCKS_MODE_MP3;
extern clocks_mode_t  CLOCKS_MODE_IDLE;
extern clocks_mode_t  CLOCKS_MODE_MAXBUS;
extern clocks_mode_t  CLOCKS_MODE_MAXCPU;
extern clocks_mode_t  CLOCKS_MODE_MAXPERF;
extern clocks_mode_t  CLOCKS_MODE_MAXNAND;
extern clocks_mode_t  CLOCKS_MODE_MAXUSBCPU;
extern clocks_mode_t  CLOCKS_MODE_MAXUSBBUS;
extern clocks_mode_t  CLOCKS_MODE_USBLOWPOWER;

enum _clocks_err_enum
{
    CLOCKS_ERR_SUCCESS = 0,
    CLOCKS_ERR_FAILURE = -1,
    CLOCKS_ERR_NO_CLKCTRL = -2
};

typedef enum _clocks_err_enum clocks_err_t;

typedef struct 
{
	unsigned int  CLOCK_HBUS_TRAFFIC_FAST;
	unsigned int  CLOCK_HBUS_EMI_BUSY_FAST;
	unsigned int  CLOCK_EMI_DRAM_PRECHARGE;
	unsigned int  CLOCK_EMI_AUTO_EMICLK_GATE;
	
} clock_context_t;

/* definitions of ioctl command */
#define STMP36XX_OPM_LEVEL _IOW('c', 0, unsigned long)

clocks_err_t init_clocks_mode(void);

clocks_err_t get_clocks_mode(clocks_mode_t*  return_clocks_mode);

clocks_err_t set_clocks_mode(const clocks_mode_t*  new_clocks_mode);

clocks_err_t apply_clk_policy(int mode);

int tprintf_clocks_mode(char *buf, const clocks_mode_t* clocks_mode); 

#endif // _CLOCKS_H_


////////////////////////////////////////////////////////////////////////////////
//
// $Log: clocks.h,v $
// Revision 1.15  2006/12/13 09:51:10  hcyun
// if you set debug enable, the serial now will not break
//
// - hcyun
//
// Revision 1.14  2005/12/29 09:36:13  joseph
// removed Auto_slow_down
//
// Revision 1.13  2005/12/29 07:23:20  biglow
// - add usb mode
//
// Revision 1.12  2005/11/22 13:34:07  biglow
// - for delete warning message at compiling
//
// Revision 1.11  2005/11/14 07:57:02  joseph
// extern idle mode
//
// Revision 1.10  2005/10/26 08:03:21  biglow
// - patch for locking during USB enable state
//
// Revision 1.9  2005/10/05 11:57:03  joseph
// add state flags struct
//
// Revision 1.8  2005/10/05 00:52:11  biglow
// - optimize with clocks
//
// Revision 1.7  2005/09/30 11:38:44  hcyun
// remove journalling
// add MAXNAND mode
//
// - hcyun
//
// Revision 1.6  2005/09/29 01:38:22  biglow
// - add USB clock mode
//
// Revision 1.5  2005/09/15 12:21:09  biglow
// - add USB mode
//
// Revision 1.4  2005/08/06 02:16:24  hcyun
// added lpj
//
// - hcyun
//
// Revision 1.3  2005/07/27 05:25:42  hcyun
// mode setting /proc/clkctrl
//
// $ echo "mode [0|1|2|3] > /proc/clkctrl"
//
//
// - hcyun
//
// Revision 1.2  2005/05/17 15:16:40  hcyun
// global h/w slection definition (HW_BRAZO, HW_3600)
//
// - hcyun
//
// Revision 1.1  2005/05/12 19:04:24  hcyun
// clock control header.
//
// - hcyun
//
// Revision 1.2  2005/05/10 22:21:00  hcyun
// - Clock Control for GPMI : this must be moved to general clock control code
//   and it should be accessible through user space.
//
// - Currently only works for CS 0 (This is a h/w problem??? )
//
// - TODO:
//   generic clk control
//   full nand erase/program check status chain
//
// Revision 1.1  2005/05/05 01:05:37  hcyun
// - Started using virt_to_phys.. not yet complete..
//
// Revision 1.4  2005/02/28 17:43:10  ttoelkes
// significant modification to clocking API
//
// Revision 1.3  2005/02/21 03:23:59  ttoelkes
// major changes to 'clocks' api
//
// Revision 1.2  2005/02/08 17:10:32  ttoelkes
// updating 'clocks' library
//
// Revision 1.1  2005/01/28 02:25:17  ttoelkes
// promoted clock functions to common lib
//
////////////////////////////////////////////////////////////////////////////////
