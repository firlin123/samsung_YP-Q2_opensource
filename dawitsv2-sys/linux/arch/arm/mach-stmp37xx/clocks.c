////////////////////////////////////////////////////////////////////////////////
//
// Filename: clocks.c
//
// Description: Implementation of common clocking-related library functions.
//
// To Do: 
//
//   * assert that the input parameters are within acceptable ranges
//   * add critical section protection where necessary
//   * add polling timeout behavior
//   * add error checking (especially for polling timeouts)
//   * add XTAL clocks gating
//   * perhaps add OCRAMCLK management (Matt H. doesn't think it should be used)
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

/* 
 * Ported to Linux and added proc interface
 * 
 * (C) 2006 Samsung Electronics 
 * - heechul.yun@samsung.com 
 */ 

#include <linux/config.h>
#include <linux/kernel.h> 
#include <linux/module.h>

#include <linux/delay.h> 
#include <linux/init.h> 

#include <linux/fs.h>
#include <linux/proc_fs.h>

#include <linux/miscdevice.h>

#include <asm/arch/clocks.h>
#include <asm/arch/pm_msg.h>
#include <asm/arch/stmp36xx_power.h>

#include <asm/arch/regs/regsdigctl.h>
#include <asm/arch/regs/regsclkctrl.h>
#include <asm/arch/regs/regsemi.h>
#include <asm/arch/usb_common.h>

#include <asm/arch/regs/regsusbphy.h> 

#if 1 //copy from 36, dhsong
#include <asm/arch/37xx/regs.h>

#endif
#define CLKCTRL_DEBUG 

#undef PDEBUG             /* undef it, just in case */
#ifdef CLKCTRL_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) //printk( KERN_DEBUG "[DBG_CLK]: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

// -----------------------------------------------------------------------------
// declarations of private functions used in this file; DO NOT EXPORT

static inline clocks_err_t clocks_enable_pll(const clocks_mode_t*  new_clocks_mode);
static inline clocks_err_t clocks_disable_pll(const clocks_mode_t*  new_clocks_mode);
static inline clocks_err_t clocks_accelerate_pll(const clocks_mode_t*  new_clocks_mode);
static inline clocks_err_t clocks_decelerate_pll(const clocks_mode_t*  new_clocks_mode);

static inline clocks_err_t clocks_poll_pll_locked(void);
static inline clocks_err_t clocks_poll_bus_clocks_ready(void);

static inline clocks_err_t clocks_manage_pll(unsigned new_pll_freq);
static inline clocks_err_t clocks_manage_xbusclk(unsigned new_clk_div);
static inline clocks_err_t clocks_manage_spdifclk(unsigned new_clk_div);
static inline clocks_err_t clocks_manage_usbclk(unsigned new_pll_freq, unsigned old_pll_freq, unsigned usb_enable);
static inline clocks_err_t clocks_manage_irclk(unsigned new_irov_div, unsigned new_ir_div, unsigned wait_if_needed);
static inline clocks_err_t clocks_manage_sspclk(unsigned new_clk_div, unsigned wait_if_needed);
static inline clocks_err_t clocks_manage_gpmiclk(unsigned new_clk_div, unsigned wait_if_needed);
static inline clocks_err_t clocks_manage_emiclk(unsigned new_clk_div, unsigned wait_if_needed);
static inline clocks_err_t clocks_manage_hbusclk(unsigned new_clk_div, unsigned new_hbusclk_autoslow_rate, unsigned wait_if_needed);
static inline clocks_err_t clocks_manage_cpuclk(unsigned new_clk_div, unsigned new_cpuclk_interrupt_wait, unsigned wait_if_needed);

// -----------------------------------------------------------------------------
//Joseph added function here
clocks_err_t set_clk_mp3_opt(bool set);
clocks_err_t set_clk_idle(bool set);

static void set_debug_msg(int level);

// -----------------------------------------------------------------------------
// preprocessor definition of XBUSCLK divider that will always be safe for all
// allowed frequencies of HBUSCLK (i.e. HBUSCLK < 100 KHz <= XBUSCLK)

#define SAFE_XBUSCLK_DIV 240

/* minor number of misc device */
#define STMP36xx_CLOCKS_MINOR 69

// -----------------------------------------------------------------------------
// declarations of constant structs representing named clock modes
/* TODO : new style of structure init -> i.e) .pll : 480, .. */ 
clocks_mode_t  CLOCKS_MODE_INIT = 
{
	0, // pll_freq
	0, // cpuclk_interrupt_wait
	0, // hbusclk_autoslow_rate
	1, // cpuclk_div
	1, // hbusclk_div
	1, // emiclk_div
	1, // gpmiclk_div
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	8,  // xbusclk_div

	lpj : 0
};

clocks_mode_t  CLOCKS_MODE_IDLE = 
{
	0, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	1, // cpuclk_div
	1, // hbusclk_div
	1, // emiclk_div
	1, // gpmiclk_div
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	8,  // xbusclk_div

	lpj : 0
};

// joseph change this to init value becuase 24M
/* 24/24/24 */ 
clocks_mode_t  CLOCKS_MODE_MP3 = 
{
	0, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	1, // cpuclk_div
	1, // hbusclk_div
	1, // emiclk_div
	1, // gpmiclk_div
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	8,  // xbusclk_div

	lpj : 0
};

clocks_mode_t  CLOCKS_MODE_FM = 
{
	0, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	1, // cpuclk_div
	1, // hbusclk_div
	1, // emiclk_div
	1, // gpmiclk_div
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	8,  // xbusclk_div
	lpj : 0
};

/* 80/80/80 */ 
clocks_mode_t  CLOCKS_MODE_MP3_DNSE = 
{
	240, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	3, // cpuclk_div   (pll/n)  = 80Mhz. 
	1, // hbusclk_div  (cpu/n)  = 80Mhz
	1, // emiclk_div   (hclk/n) = 80Mhz
	4, // gpmiclk_div  (pll/n)  = 60Mhz. 16.6ns 
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	8,  // xbusclk_div
	
	lpj : 0
};

/* 48/48/48 */ 
clocks_mode_t  CLOCKS_MODE_FM_DNSE = 
{
	240, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	5, // cpuclk_div   (pll/n)  = 48Mhz. 
	1, // hbusclk_div  (cpu/n)  = 48Mhz
	1, // emiclk_div   (hclk/n) = 48Mhz
	4, // gpmiclk_div  (pll/n)  = 60Mhz. 16.6ns 
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	8,  // xbusclk_div
	
	lpj : 0
};

/* 40/40/40 */ 
clocks_mode_t  CLOCKS_MODE_WMA = 
{
	240, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	6, // cpuclk_div   (pll/n)  = 40Mhz
	1, // hbusclk_div  (cpu/n)  = 40Mhz 
	1, // emiclk_div   (hclk/n)
	4, // gpmiclk_div  (pll/n)  = 60Mhz. 16ns 
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	8,  // xbusclk_div

	lpj : 0
};

/* 120/60/60 */
clocks_mode_t  CLOCKS_MODE_WMA_DNSE = 
{
	240, // pll_freq 
	1, // cpuclk_interrupt_wait 
	8, // hbusclk_autoslow_rate
	2, // cpuclk_div   (pll/n)  = 120Mhz
	2, // hbusclk_div  (cpu/n)  = 60Mhz
	1, // emiclk_div   (hclk/n) = 60Mhz
	4, // gpmiclk_div  (pll/n)  = 60Mhz
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	8,  // xbusclk_div

	lpj : 0
};


/* 80/40/40 */ 
clocks_mode_t  CLOCKS_MODE_OGG = 
{
	240, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	3, // cpuclk_div   (pll/n) = 80Mhz
	2, // hbusclk_div  (cpu/n) = 40Mhz
	1, // emiclk_div   (hclk/n)= 40Mhz
	4, // gpmiclk_div  (pll/n) = 60Mhz
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	8,  // xbusclk_div

	lpj : 0
};

/* 150/75/75 */
clocks_mode_t  CLOCKS_MODE_OGG_DNSE = 
{
	300, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	2, // cpuclk_div   (pll/n) = 150Mhz
	2, // hbusclk_div  (cpu/n) = 75Mhz
	1, // emiclk_div   (hclk/n)= 75Mhz
	4, // gpmiclk_div  (pll/n) = 75Mhz
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	8,  // xbusclk_div

	lpj : 0
};

clocks_mode_t  CLOCKS_MODE_MAXCPU = 
{
	480, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	3, // cpuclk_div   (pll/n)
	3, // hbusclk_div  (cpu/n)   
	1, // emiclk_div   (hclk/n)
	4, // gpmiclk_div  (pll/n)  = 120Mhz
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	1,  // xbusclk_div

	lpj : 400384
};

clocks_mode_t  CLOCKS_MODE_MAXBUS = 
{
	480, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	4, // cpuclk_div   (pll/n)
	2, // hbusclk_div  (cpu/n)   
	1, // emiclk_div   (hclk/n)
	4, // gpmiclk_div  (pll/n)  = 120Mhz
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	1,  // xbusclk_div

	lpj : 0 /* 300032 */ 
};

// 200,100,100 (200/66/66)
clocks_mode_t  CLOCKS_MODE_MAXPERF = 
{
	400, // pll_freq
	1, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	2, // cpuclk_div
	2, // hbusclk_div
	1, // emiclk_div
	4, // gpmiclk_div          = 100Mhz
 	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	0, // usb_enable
	1,  // xbusclk_div

	lpj : 500736 /* 375040 */ 
};


clocks_mode_t  CLOCKS_MODE_MAXNAND = 
{
	480, // pll_freq
	0, // cpuclk_interrupt_wait
	8, // hbusclk_autoslow_rate
	5, // cpuclk_div   (pll/n) = 96
	1, // hbusclk_div  (cpu/n) = 96 
	2, // emiclk_div   (hclk/n)= 48
	4, // gpmiclk_div  (pll/n) = 120Mhz
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	1, // usb_enable
	1, // xbusclk_div

	lpj : 0
};

clocks_mode_t  CLOCKS_MODE_MAXUSBCPU = 
{
	480, // pll_freq
	0, // cpuclk_interrupt_wait
	0, // hbusclk_autoslow_rate
	3, // cpuclk_div   (pll/n)
	2, // hbusclk_div  (cpu/n)	 
	1, // emiclk_div   (hclk/n)
	4, // gpmiclk_div  (pll/n) = 120Mhz
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	1, // usb_enable
	1,	// xbusclk_div

	lpj : 0
};


clocks_mode_t  CLOCKS_MODE_MAXUSBBUS = 
{
	480, // pll_freq
	0, // cpuclk_interrupt_wait
	0, // hbusclk_autoslow_rate
	3, // cpuclk_div   (pll/n)
	2, // hbusclk_div  (cpu/n)	
	1, // emiclk_div   (hclk/n)
	4, // gpmiclk_div  (pll/n) = 120Mhz
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	1, // usb_enable
	1,	// xbusclk_div

	lpj : 0
};

clocks_mode_t  CLOCKS_MODE_USBLOWPOWER = 
{
	480, // pll_freq
	0, // cpuclk_interrupt_wait
	0, // hbusclk_autoslow_rate
	20, // cpuclk_div   (pll/n)
	1, // hbusclk_div  (cpu/n)	 
	1, // emiclk_div   (hclk/n)
	20, // gpmiclk_div  (pll/n) = 24Mhz
	0, // sspclk_div
	0, // irov_div
	0, // ir_div
	0, // spdifclk_div
	1, // usb_enable
	1,	// xbusclk_div
	
	lpj : 0
};

// -----------------------------------------------------------------------------
// declarations of private static variable containing last clock mode

static clocks_mode_t  old_clocks;

static int current_clock_run_level;
clock_context_t clock_context;
static int mod_usecount;
static bool debug_msg = 0;

// -----------------------------------------------------------------------------
// linux related stuff. 

struct semaphore clk_sem; 

static int clkctrl_read_procmem(char *buf, char **start, off_t offset,
                int count, int *eof, void *data)
{
	int len = 0; 
	
	len += sprintf(buf + len, "\n[CLKCTRL] Current clock mode settings..\n"); 

	if (down_interruptible (&clk_sem))
		return -ERESTARTSYS;	

	len += tprintf_clocks_mode(buf + len, &old_clocks); 
	
	len += sprintf(buf + len, "DRAMCTRL: 0x%x\n", HW_EMIDRAMCTRL_RD()); 
	len += sprintf(buf + len, "  auto_clk_gate = %d\n", HW_EMIDRAMCTRL.AUTO_EMICLK_GATE ); 
	len += sprintf(buf + len, "  precharge     = %d\n", HW_EMIDRAMCTRL.PRECHARGE ); 

	up (&clk_sem);

	*eof = 1; 
	return len; 
}


/*
 * @brief: clock control function
 *
 * Currently, this can do very preliminary control 
 * 
 * @param: 
 * @return: 
 */

static ssize_t clkctrl_write_procmem(struct file * file, const char * buf, 
		unsigned long count, void *data)
{
	char cmd[80]; 
	int val, val2, val3; 
	clocks_mode_t new_clocks; 
	unsigned long flags; 

	sscanf(buf, "%s %d %d %d", cmd, &val, &val2, &val3); 

	PDEBUG("%s %d %d %d\n", cmd, val, val2, val3); 
//	get_usec_elapsed_from_prev_call(); 

        if (down_interruptible (&clk_sem))
                return -ERESTARTSYS;	

	local_irq_save(flags);

	/* cpu, hbus, xbus, emi, gpmi */ 
	if ( !strcmp(cmd, "mode") ) { 
		switch ( val ) {
		case 8:
			set_clocks_mode(&CLOCKS_MODE_MAXUSBCPU); 
			break;
		case 7:
			set_clocks_mode(&CLOCKS_MODE_MAXNAND);
			break; 
		case 6:
			set_clocks_mode(&CLOCKS_MODE_OGG);
			break; 
		case 5:
			set_clocks_mode(&CLOCKS_MODE_WMA);
			break; 
		case 4:
			set_clocks_mode(&CLOCKS_MODE_MP3);
			break; 
		case 3: 
			set_clocks_mode(&CLOCKS_MODE_MAXPERF);
			break;
		case 2: 
			set_clocks_mode(&CLOCKS_MODE_MAXBUS);
			break;
		case 1: 
			set_clocks_mode(&CLOCKS_MODE_MAXCPU);
			break;
		case 0:
			set_clocks_mode(&CLOCKS_MODE_INIT);
			break;
		}

	}
	else if(!strcmp(cmd, "OPM_LEVEL"))	apply_clk_policy(val);
	else if(!strcmp(cmd, "maxemi") ) {
		if ( val == 0 ) {
			CLOCKS_MODE_MAXPERF.hbusclk_div = 3;   // 200/66/66
			CLOCKS_MODE_MAXUSBBUS.cpuclk_div = 4;  // 120/60/60
		} else {
			CLOCKS_MODE_MAXPERF.hbusclk_div = 2;   // 200/100/100
			CLOCKS_MODE_MAXUSBBUS.cpuclk_div = 3;  // 160/80/80
		}
	}
	else if(!strcmp(cmd, "debug"))	{
		set_debug_msg(val);

		printk("set debug xclkdiv = 1\n"); 
		CLOCKS_MODE_MP3.xbusclk_div = 1; 
		CLOCKS_MODE_WMA.xbusclk_div = 1; 
		CLOCKS_MODE_OGG.xbusclk_div = 1; 
		CLOCKS_MODE_MP3_DNSE.xbusclk_div = 1; 
		CLOCKS_MODE_WMA_DNSE.xbusclk_div = 1; 
		CLOCKS_MODE_OGG_DNSE.xbusclk_div = 1; 
	}
	else if(!strcmp(cmd, "autoemi" )) {
		if ( val == 0 ) { 
			HW_EMIDRAMCTRL_CLR(BM_EMIDRAMCTRL_AUTO_EMICLK_GATE);
		} else {
			HW_EMIDRAMCTRL_SET(BM_EMIDRAMCTRL_AUTO_EMICLK_GATE);
		}
	} else if (!strcmp(cmd, "precharge" )) { 
		if ( val == 0 ) { 
			HW_EMIDRAMCTRL_CLR(BM_EMIDRAMCTRL_PRECHARGE);
		} else {
			HW_EMIDRAMCTRL_SET(BM_EMIDRAMCTRL_PRECHARGE);
		}
	} else if (!strcmp(cmd, "usboff")) { /* hcyun */ 
		BF_CLR(CLKCTRL_UTMICLKCTRL, UTMI_CLK120M_GATE);
		BF_CLR(DIGCTL_CTRL, USB_CLKGATE);
		BF_CLR(USBPHY_CTRL,SFTRST);
		while(BF_RD(USBPHY_CTRL,SFTRST)==1);
		BF_SET(DIGCTL_CTRL, USB_CLKGATE);
		BF_SET(CLKCTRL_UTMICLKCTRL, UTMI_CLK120M_GATE);
	} else {
		memcpy(&new_clocks, &old_clocks, sizeof(clocks_mode_t)); 

		if ( !strcmp(cmd, "cpu") )
			new_clocks.cpuclk_div = val; 
		else if ( !strcmp(cmd, "pll") )
			new_clocks.pll_freq = val; 
		else if ( !strcmp(cmd, "hbus") ) 
			new_clocks.hbusclk_div = val; 
		else if ( !strcmp(cmd, "xbus") ) 
			new_clocks.xbusclk_div = val; 
		else if ( !strcmp(cmd, "emi") ) 
			new_clocks.emiclk_div = val; 
		else if ( !strcmp(cmd, "gpmi") ) 
			new_clocks.gpmiclk_div = val; 
		else if ( !strcmp(cmd, "usb") ) 
			new_clocks.usb_enable = val; 
		else if ( !strcmp(cmd, "intr") ) 
			new_clocks.cpuclk_interrupt_wait = val; 
		else if ( !strcmp(cmd, "slow") ) 
			new_clocks.hbusclk_autoslow_rate = val; 
		else if ( !strcmp(cmd, "apbx") ) {
			if ( val == 0 ) {
				printk("clear\n");
				HW_CLKCTRL_HBUSCLKCTRL_CLR(BF_CLKCTRL_HBUSCLKCTRL_APBXDMA_BUSY_FAST(1));
			} else if ( val == 1 ) {
				printk("set\n");
				HW_CLKCTRL_HBUSCLKCTRL_SET(BF_CLKCTRL_HBUSCLKCTRL_APBXDMA_BUSY_FAST(1));
			} 
			printk("apbx_busy_fast = %d\n", 
			       HW_CLKCTRL_HBUSCLKCTRL.APBXDMA_BUSY_FAST); 
		}
			

		else printk("[CLKCTRL] Unknown command..\n"); 

		set_clocks_mode(&new_clocks); 
	}

	local_irq_restore(flags);

	PDEBUG("[CLKCTRL] clock map is changed \n"); 

        up (&clk_sem);
//	printk ("[TIME] CLK time = %u usec\n", get_usec_elapsed_from_prev_call()); 

	return count; 
}



// -----------------------------------------------------------------------------
// definition of public function for initializing clocks mode; this function
// should be called before using 'get_clocks_mode()' or 'set_clocks_mode()'
// in order to ensure correct operation

clocks_err_t init_clocks_mode(void)
{
    const hw_clkctrl_pllctrl0_t     pllctrl0     = HW_CLKCTRL_PLLCTRL0;
    const hw_clkctrl_cpuclkctrl_t   cpuclkctrl   = HW_CLKCTRL_CPUCLKCTRL;
    const hw_clkctrl_hbusclkctrl_t  hbusclkctrl  = HW_CLKCTRL_HBUSCLKCTRL;
    const hw_clkctrl_xbusclkctrl_t  xbusclkctrl  = HW_CLKCTRL_XBUSCLKCTRL;
    const hw_clkctrl_sspclkctrl_t   sspclkctrl   = HW_CLKCTRL_SSPCLKCTRL;
    const hw_clkctrl_gpmiclkctrl_t  gpmiclkctrl  = HW_CLKCTRL_GPMICLKCTRL;
    const hw_clkctrl_spdifclkctrl_t spdifclkctrl = HW_CLKCTRL_SPDIFCLKCTRL;
    const hw_clkctrl_emiclkctrl_t   emiclkctrl   = HW_CLKCTRL_EMICLKCTRL;
    const hw_clkctrl_irclkctrl_t    irclkctrl    = HW_CLKCTRL_IRCLKCTRL;
    const hw_clkctrl_utmiclkctrl_t  utmiclkctrl  = HW_CLKCTRL_UTMICLKCTRL;

    // words[0]
    old_clocks.pll_freq = pllctrl0.POWER ? pllctrl0.FREQ : 0;
    old_clocks.cpuclk_interrupt_wait = cpuclkctrl.INTERRUPT_WAIT;
    if (hbusclkctrl.AUTO_SLOW_MODE)
    {
        switch (hbusclkctrl.SLOW_DIV)
        {
            case 0x0: old_clocks.hbusclk_autoslow_rate = 1; break;
            case 0x1: old_clocks.hbusclk_autoslow_rate = 2; break;
            case 0x2: old_clocks.hbusclk_autoslow_rate = 4; break;
            case 0x3: old_clocks.hbusclk_autoslow_rate = 8; break;
        }
    }
    else
    {
        old_clocks.hbusclk_autoslow_rate = 0;
    }

    // joseph change this because of divide by 0 problem.
    // words[1]
    old_clocks.cpuclk_div = cpuclkctrl.DIV;
    old_clocks.hbusclk_div = hbusclkctrl.DIV;
    old_clocks.xbusclk_div = xbusclkctrl.DIV;

    // words[2]
    old_clocks.gpmiclk_div = gpmiclkctrl.CLKGATE ? 1 : gpmiclkctrl.DIV;
    old_clocks.sspclk_div = sspclkctrl.CLKGATE ? 1 : sspclkctrl.DIV;

    // words[3]
    if (irclkctrl.CLKGATE)
    {
        old_clocks.irov_div = 4;
        old_clocks.ir_div = 4;
    }
    else if (irclkctrl.AUTO_DIV)
    {
        old_clocks.irov_div = 4;
        old_clocks.ir_div = 4;
    }
    else
    {
        old_clocks.irov_div = irclkctrl.IROV_DIV;
        old_clocks.ir_div = irclkctrl.IR_DIV;
    }

    // words[4]
    old_clocks.emiclk_div = emiclkctrl.CLKGATE ? 1: emiclkctrl.DIV;
    old_clocks.usb_enable = (pllctrl0.EN_USB_CLKS && 
                             !utmiclkctrl.UTMI_CLK120M_GATE && 
                             !utmiclkctrl.UTMI_CLK30M_GATE);
    old_clocks.spdifclk_div = spdifclkctrl.CLKGATE ? 1 : spdifclkctrl.DIV;

#if 0
    {
	    char buf[1024]; 
	    printk("Current clock mode settings\n"); 
	    printk("----------------------------\n"); 
	    tprintf_clocks_mode(buf, &old_clocks); 
	    printk("%s\n", buf); 
    }
#endif 

    return CLOCKS_ERR_SUCCESS;
}


// -----------------------------------------------------------------------------
// definition of public function for getting clocks mode

clocks_err_t get_clocks_mode(clocks_mode_t*  return_clocks_mode)
{

    memcpy(return_clocks_mode, &old_clocks, sizeof(clocks_mode_t)); 

#if 0 
    for (index = 0; index < CLOCKS_MODE_WORDS; index++)
    {
        return_clocks_mode->words[index] = old_clocks.words[index];
    }
#endif 
    return CLOCKS_ERR_SUCCESS;
}


// -----------------------------------------------------------------------------
// definition of public function for setting clocks mode
#define HLIM_EMICLOCK_BOOST   1

#define USE_NAND_TIMING_CHANGE 0

#if USE_NAND_TIMING_CHANGE
#include <asm/arch/regs/regsgpmi.h>
static inline void gpmi_change_timing(int tas, int tds, int tdh)
{
	PDEBUG("set gpmi timing: (tas,tds,tdh) = (%d,%d,%d)\n", tas,tds,tdh); 
	BF_CS3(GPMI_TIMING0, 
	       ADDRESS_SETUP, tas,
	       DATA_SETUP, tds,
	       DATA_HOLD, tdh);
}
#endif 

#if (!HW_3600)

clocks_err_t set_clocks_mode(const clocks_mode_t* new_clocks)
{
    // We have no PLL in Brazo, so everything becomes simpler.

    // Adjust the XBUSCLK divider first.
    clocks_manage_xbusclk(new_clocks->xbusclk_div);
    
    // Then, adjust all PLL-based dividers.
    clocks_manage_irclk(new_clocks->irov_div, new_clocks->ir_div, 0);
    clocks_manage_spdifclk(new_clocks->spdifclk_div);
    clocks_manage_sspclk(new_clocks->sspclk_div, 0);
    clocks_manage_gpmiclk(new_clocks->gpmiclk_div, 0);
    clocks_manage_emiclk(new_clocks->emiclk_div, 0);
    clocks_manage_hbusclk(new_clocks->hbusclk_div, new_clocks->hbusclk_autoslow_rate, 0);
    clocks_manage_cpuclk(new_clocks->cpuclk_div, new_clocks->cpuclk_interrupt_wait, 0);

    // Finally, manage the USB clocking (Carroll, is this correct for Brazo?)
    clocks_manage_usbclk(480, 480, new_clocks->usb_enable);

    return CLOCKS_ERR_SUCCESS;
}


#else // HW_3600
 
clocks_err_t set_clocks_mode(const clocks_mode_t* new_clocks)
{

    //Disable interrup here

    
    unsigned old_pll_freq = old_clocks.pll_freq;
    unsigned new_pll_freq = new_clocks->pll_freq;

    unsigned pll_enable = (new_pll_freq && !old_pll_freq);
    unsigned pll_disable = (!new_pll_freq && old_pll_freq);
    unsigned pll_rising  = (new_pll_freq > old_pll_freq);
    unsigned pll_falling = (new_pll_freq < old_pll_freq);

    //@@@ joseph add adjust core voltage function.
    int do_adj_voltage;
    int new_vddd;
    int old_vddd;
#if HLIM_EMICLOCK_BOOST
    //@@@ H.Lim add adjust io voltage function like a vddd adjust.
    int do_adj_voltageio;
    int new_vddio;
    int old_vddio;
#endif
    if(new_clocks->usb_enable)
    	new_vddd =get_vddd_value_from_clk(new_clocks->pll_freq/new_clocks->cpuclk_div, 
    				new_clocks->pll_freq/new_clocks->cpuclk_div/new_clocks->hbusclk_div,true);
    else
    	new_vddd =get_vddd_value_from_clk(new_clocks->pll_freq/new_clocks->cpuclk_div, 
    				new_clocks->pll_freq/new_clocks->cpuclk_div/new_clocks->hbusclk_div,false);
    if(old_clocks.usb_enable)
    	old_vddd =  get_vddd_value_from_clk(old_clocks.pll_freq/old_clocks.cpuclk_div,
    				old_clocks.pll_freq/old_clocks.cpuclk_div/old_clocks.hbusclk_div,true);
    else
    	old_vddd =  get_vddd_value_from_clk(old_clocks.pll_freq/old_clocks.cpuclk_div,
    				old_clocks.pll_freq/old_clocks.cpuclk_div/old_clocks.hbusclk_div,false);
// H.Lim CheckPoint
#if HLIM_EMICLOCK_BOOST
    /* set vddio from emiclock boost or not */
	new_vddio = get_vddio_value_from_clk(new_clocks->pll_freq/new_clocks->cpuclk_div,
					     new_clocks->hbusclk_div*new_clocks->emiclk_div,
					     new_clocks->usb_enable);
    	old_vddio = get_vddio_value_from_clk(old_clocks.pll_freq/old_clocks.cpuclk_div, 
					     old_clocks.hbusclk_div*old_clocks.emiclk_div,
					     old_clocks.usb_enable);
#endif


// H.Lim CheckPoint
#if HLIM_EMICLOCK_BOOST
    if(new_vddio < 0)
    {
	    printk("VDDIO clock setting value is wrong so setting default IO Voltage.\n"); 
	    return CLOCKS_ERR_FAILURE;
    }
    if(new_vddio > old_vddio) {
	    do_adj_voltageio = 1; 
	    set_io_level(new_vddio); 
    }
    else if(new_vddio < old_vddio) { 
	    do_adj_voltageio = 0; 
    } 
    else {
	    do_adj_voltageio = 1; 
	    // Skip io Level Setting.
    }	    
#endif
    
    if(new_vddd < 0){
	    printk("clock setting value is wrong.\n"); 
	    return CLOCKS_ERR_FAILURE;
    }
    if(new_vddd > old_vddd) {
	    do_adj_voltage = 1; 
	    set_core_level(new_vddd);
    }
    else if(new_vddd < old_vddd) { 
	    do_adj_voltage = 0;
    } else {
	    do_adj_voltage = 1;	
    }

    //@@@ joseph add adjust core voltage function end.
     
    if (pll_enable) PDEBUG("pll_enable = %d\n", pll_enable); 
    if (pll_disable) PDEBUG("pll_disable = %d\n", pll_disable); 
    if (pll_rising) PDEBUG("pll_rising = %d\n", pll_rising); 
    if (pll_falling) PDEBUG("pll_falling = %d\n", pll_falling); 

    if(debug_msg != false)
    {
	    if(new_clocks->pll_freq == 0)
	    {
	    	//PDEBUG("\tPLL=%d, CPU=%d, HBUS=%d, EMI=%d, GPMI=%d, USB=%d\n", 
	    	   printk(KERN_INFO "\tPLL=%d, CPU=%d, HBUS=%d, EMI=%d, GPMI=%d, USB=%d\n", 
		   24, 24 /new_clocks->cpuclk_div, 24 /new_clocks->cpuclk_div/new_clocks->hbusclk_div, 
		   24 /new_clocks->cpuclk_div/new_clocks->hbusclk_div/new_clocks->emiclk_div,
		   24 /new_clocks->gpmiclk_div, new_clocks->usb_enable ); 
	    }
	    else
	    {
	    	   //PDEBUG("\tPLL=%d, CPU=%d, HBUS=%d, EMI=%d, GPMI=%d USB=%d\n", 
	 	   printk(KERN_INFO "\tPLL=%d, CPU=%d, HBUS=%d, EMI=%d, GPMI=%d USB=%d\n", 
		   new_clocks->pll_freq, 
		   new_clocks->pll_freq/new_clocks->cpuclk_div, 
		   new_clocks->pll_freq/new_clocks->cpuclk_div/new_clocks->hbusclk_div, 
		   new_clocks->pll_freq/new_clocks->cpuclk_div/new_clocks->hbusclk_div/new_clocks->emiclk_div,
		   new_clocks->pll_freq/new_clocks->gpmiclk_div, 
		   new_clocks->usb_enable); 
	    }
    }
    
    if ( new_clocks->lpj ) { 
	    loops_per_jiffy = new_clocks->lpj; 
	    old_clocks.lpj  = new_clocks->lpj; 
    } else { /* compute lpj based on previous lpj & clock settings */ 
	    unsigned old_cpu_clk; 
	    unsigned new_cpu_clk; 

	    if ( pll_enable ) {
		    old_cpu_clk = 24; 
		    new_cpu_clk = new_clocks->pll_freq / new_clocks->cpuclk_div;
	    } else if ( pll_disable ) {
		    old_cpu_clk = old_clocks.pll_freq / old_clocks.cpuclk_div; 
		    new_cpu_clk = 24;
	    } else {

	    	    if((!old_clocks.pll_freq)&&(!new_clocks->pll_freq))
	    	    {
			    old_cpu_clk = 24 / old_clocks.cpuclk_div; 
			    new_cpu_clk = 24 / new_clocks->cpuclk_div;
	    	    }
	    	    else
	    	    {		    	    
			    old_cpu_clk = old_clocks.pll_freq / old_clocks.cpuclk_div; 
			    new_cpu_clk = new_clocks->pll_freq / new_clocks->cpuclk_div;
	    	    }
	    }

	    loops_per_jiffy = (loops_per_jiffy * new_cpu_clk) / old_cpu_clk; 
	    
	    new_clocks->lpj = old_clocks.lpj = loops_per_jiffy;
    }
    PDEBUG("LPJ is changed to %ld\n", loops_per_jiffy); 

    // Adjust the XBUSCLK divider first.
    clocks_manage_xbusclk(new_clocks->xbusclk_div);
	    
    clocks_manage_pll(new_clocks->pll_freq); 

    // Then, adjust all PLL-based dividers.
    clocks_manage_irclk(new_clocks->irov_div, new_clocks->ir_div, 0);
    clocks_manage_spdifclk(new_clocks->spdifclk_div);
    clocks_manage_sspclk(new_clocks->sspclk_div, 0);
    clocks_manage_gpmiclk(new_clocks->gpmiclk_div, 0);
    clocks_manage_usbclk(new_pll_freq, old_pll_freq, new_clocks->usb_enable); // FIX
    
 retry:
    clocks_manage_emiclk(new_clocks->emiclk_div, 0);
    clocks_manage_hbusclk(new_clocks->hbusclk_div, new_clocks->hbusclk_autoslow_rate, 0);
    clocks_manage_cpuclk(new_clocks->cpuclk_div, new_clocks->cpuclk_interrupt_wait, 0);
    
    if ( (new_clocks->cpuclk_div != BF_RD(CLKCTRL_CPUCLKCTRL, DIV)) ||
	 (new_clocks->hbusclk_div != BF_RD(CLKCTRL_HBUSCLKCTRL, DIV)) ||
	 (new_clocks->emiclk_div != BF_RD(CLKCTRL_EMICLKCTRL, DIV))) 
    {
	    goto retry; 
    }

    // original place of USB clock management.. 


    //change core voltage after clock setting!!	
    if(!do_adj_voltage)	set_core_level(new_vddd);
#if HLIM_EMICLOCK_BOOST
    if(!do_adj_voltageio) set_io_level(new_vddio);  
#endif
    return CLOCKS_ERR_SUCCESS;
}


#endif // HW_3600


// -----------------------------------------------------------------------------
// definition of public function for printfing clock mode

int tprintf_clocks_mode(char *buf, const clocks_mode_t* clocks_mode)
{
    int len = 0;
    if (clocks_mode->pll_freq)
    {
	len += sprintf(buf + len, "pll_freq = %d\n", clocks_mode->pll_freq);
    }
    else
    {
        len += sprintf(buf + len,"pll disabled\n");
    }

    len += sprintf(buf + len, "cpuclk_div = %d\n", clocks_mode->cpuclk_div);
    len += sprintf(buf + len, "hbusclk_div = %d\n", clocks_mode->hbusclk_div);
    len += sprintf(buf + len, "xbusclk_div = %d\n", clocks_mode->xbusclk_div);

    if (clocks_mode->emiclk_div)
    {
	len += sprintf(buf + len, "emiclk_div = %d\n", clocks_mode->emiclk_div);
    }
    else
    {
        len += sprintf(buf + len, "emiclk disabled\n");
    }

    if (clocks_mode->gpmiclk_div)
    {
        len += sprintf(buf + len, "gpmiclk_div = %d\n", clocks_mode->gpmiclk_div);
    }
    else
    {
        len += sprintf(buf + len, "gpmiclk disabled\n");
    }

    if (clocks_mode->sspclk_div)
    {
        len += sprintf(buf + len, "sspclk_div = %d\n", clocks_mode->sspclk_div);
    }
    else
    {
        len += sprintf(buf + len, "sspclk disabled\n");
    }

    if (clocks_mode->spdifclk_div)
    {
        len += sprintf(buf + len, "spdifclk_div = %d\n", clocks_mode->spdifclk_div);
    }
    else
    {
        len += sprintf(buf + len, "spdifclk disabled\n");
    }

    len += sprintf(buf + len, "irov_div = %d\n", clocks_mode->irov_div);
    len += sprintf(buf + len, "ir_div = %d\n", clocks_mode->ir_div);
    len += sprintf(buf + len, "usb_enable = %d\n", clocks_mode->usb_enable);
    len += sprintf(buf + len, "hbusclk_autoslow_rate = %d\n", clocks_mode->hbusclk_autoslow_rate);
	len += sprintf(buf + len, "cpuclk_interrupt_wait = %d\n", HW_CLKCTRL_CPUCLKCTRL.INTERRUPT_WAIT ); 
    //len += sprintf(buf + len, "cpuclk_interrupt_wait = %d\n", clocks_mode->cpuclk_interrupt_wait);
    return len; 
}


// =============================================================================
// =============================================================================

// -----------------------------------------------------------------------------
// definitions of higher-level private functions used to implement 'clocks_config'

inline clocks_err_t clocks_enable_pll(const clocks_mode_t*  new_clocks)
{
    // Power up the PLL to the specified frequency.
    clocks_manage_pll(new_clocks->pll_freq);

    // Poll until the PLL lock has been acquired.
    clocks_poll_pll_locked();

	// removed by jinho.lim due to audio distortion in playing 
    // Adjust XBUSCLK to a safe divider while in bypass before new HBUSCLK divider is ready.
    //clocks_manage_xbusclk(SAFE_XBUSCLK_DIV);
    
    // Finally, adjust the individual PLL-based clocks; no wait is ever needed.
    clocks_manage_cpuclk(new_clocks->cpuclk_div, new_clocks->cpuclk_interrupt_wait, 0);
    clocks_manage_hbusclk(new_clocks->hbusclk_div, new_clocks->hbusclk_autoslow_rate, 0);
    clocks_manage_emiclk(new_clocks->emiclk_div, 0);
    clocks_manage_irclk(new_clocks->irov_div, new_clocks->ir_div, 0);
    clocks_manage_spdifclk(new_clocks->spdifclk_div);
    clocks_manage_sspclk(new_clocks->sspclk_div, 0);
    clocks_manage_gpmiclk(new_clocks->gpmiclk_div, 0);

    // Poll until CPUCLK and HBUSCLK are ready.
    clocks_poll_bus_clocks_ready();

    // Remove the PLL bypass.
    BF_CLR(CLKCTRL_PLLCTRL0, BYPASS);

    // Adjust XBUSCLK to final desired divider.
    clocks_manage_xbusclk(new_clocks->xbusclk_div);

    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_disable_pll(const clocks_mode_t*  new_clocks)
{
	// removed by jinho.lim due to audio distortion in playing 
    // Adjust XBUSCLK to a safe divider while in bypass before new HBUSCLK divider is ready.
    //clocks_manage_xbusclk(SAFE_XBUSCLK_DIV);
    
    // Bypass the PLL and power it down.
    BF_SET(CLKCTRL_PLLCTRL0, BYPASS);
    clocks_manage_pll(new_clocks->pll_freq);
        
    // Adjust CPUCLK and  HBUSCLK dividers.
    clocks_manage_cpuclk(new_clocks->cpuclk_div, new_clocks->cpuclk_interrupt_wait, 0);
    clocks_manage_hbusclk(new_clocks->hbusclk_div, new_clocks->hbusclk_autoslow_rate, 0);

    // Poll until CPUCLK and HBUSCLK are ready.
    clocks_poll_bus_clocks_ready();

    // Adjust the XBUSCLK divider first.
    clocks_manage_xbusclk(new_clocks->xbusclk_div);

    // Finally, adjust the rest of the pll-based clocks.
    clocks_manage_irclk(new_clocks->irov_div, new_clocks->ir_div, 0);
    clocks_manage_spdifclk(new_clocks->spdifclk_div);
    clocks_manage_sspclk(new_clocks->sspclk_div, 0);
    clocks_manage_gpmiclk(new_clocks->gpmiclk_div, 0);
    clocks_manage_emiclk(new_clocks->emiclk_div, 0);

    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_accelerate_pll(const clocks_mode_t*  new_clocks)
{
    // In all of the cases where the PLL frequency is being increased,
    // we need to set the dividers first, and then set the PLL.


    // Adjust the XBUSCLK divider first.
    clocks_manage_xbusclk(new_clocks->xbusclk_div);

    // Finally, change the PLL frequency.
    clocks_manage_pll(new_clocks->pll_freq);
    
    // Finally, adjust the individual PLL-based clocks; no wait is ever needed.
    clocks_manage_cpuclk(new_clocks->cpuclk_div, new_clocks->cpuclk_interrupt_wait, 0);
    clocks_manage_hbusclk(new_clocks->hbusclk_div, new_clocks->hbusclk_autoslow_rate, 0);
    clocks_manage_emiclk(new_clocks->emiclk_div, 0);
    clocks_manage_irclk(new_clocks->irov_div, new_clocks->ir_div, 0);
    clocks_manage_spdifclk(new_clocks->spdifclk_div);
    clocks_manage_sspclk(new_clocks->sspclk_div, 0);
    clocks_manage_gpmiclk(new_clocks->gpmiclk_div, 0);

    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_decelerate_pll(const clocks_mode_t*  new_clocks)
{
    // In all of the cases where the PLL frequency is being decreased,
    // we set the PLL first and then set the dividers, employing the
    // wait for PLL lock functionality only if the divider is being
    // decreased and the clock is not being disabled.
    //
    // This is the only case in clock configuration where we may need
    // to use the WAIT_PLL_LOCK control (if decreasing clock divider).

    // Adjust the XBUSCLK divider first.
    clocks_manage_xbusclk(new_clocks->xbusclk_div);

    // Finally, change the PLL frequency.
    clocks_manage_pll(new_clocks->pll_freq);
    
    // Then, adjust the individual PLL-based clocks, waiting if needed.
    clocks_manage_cpuclk(new_clocks->cpuclk_div, new_clocks->cpuclk_interrupt_wait, 1);
    clocks_manage_hbusclk(new_clocks->hbusclk_div, new_clocks->hbusclk_autoslow_rate, 1);
    clocks_manage_emiclk(new_clocks->emiclk_div, 1);
    clocks_manage_irclk(new_clocks->irov_div, new_clocks->ir_div, 1);
    clocks_manage_spdifclk(new_clocks->spdifclk_div);
    clocks_manage_sspclk(new_clocks->sspclk_div, 1);
    clocks_manage_gpmiclk(new_clocks->gpmiclk_div, 1);

    return CLOCKS_ERR_SUCCESS;
}


// -----------------------------------------------------------------------------
// definitions of lower-level private functions used to poll for important events

inline clocks_err_t clocks_poll_pll_locked(void)
{
    // TODO: need timeout on PLL lock polling

    while (!BF_RD(CLKCTRL_PLLCTRL1, LOCK))
    {
        // pace back and forth...
    }

    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_poll_bus_clocks_ready(void)
{
    // TODO: need timeout on CPUCLK and HBUSCLK not busy

    while (BF_RD(CLKCTRL_CPUCLKCTRL, BUSY) || BF_RD(CLKCTRL_HBUSCLKCTRL, BUSY))
    {
        // twiddle thumbs...
    }

    return CLOCKS_ERR_SUCCESS;
}


// -----------------------------------------------------------------------------
// definitions of lower-level private functions used manage CLKCTRL register updates

inline clocks_err_t clocks_manage_pll(unsigned new_pll_freq)
{
	bool bPllOnNow;
	bool bPllBypassedNow;
	uint16_t uPllFreq_MHzNow;

#define PCLK_SAFE_DIVIDER 4
#define HCLK_SAFE_DIVIDER 4

	bPllOnNow = BF_RD(CLKCTRL_PLLCTRL0, POWER);
	bPllBypassedNow = BF_RD(CLKCTRL_PLLCTRL0, BYPASS);
	uPllFreq_MHzNow = BF_RD(CLKCTRL_PLLCTRL0, FREQ);

	if ( old_clocks.pll_freq == new_pll_freq ) return CLOCKS_ERR_SUCCESS; 

	if (new_pll_freq) {
		clocks_manage_cpuclk(PCLK_SAFE_DIVIDER, 0, 0);
		clocks_manage_hbusclk(HCLK_SAFE_DIVIDER, 0, 0); 

		// Turn on PLL if desired.. 
		if ( bPllOnNow == 0 ) 
			HW_CLKCTRL_PLLCTRL0.POWER = 1; 

		if ( new_pll_freq != 480 ) {
			HW_CLKCTRL_PLLCTRL0.FREQ = new_pll_freq + 4; 
			while ( !HW_CLKCTRL_PLLCTRL1.LOCK );  
		}

		HW_CLKCTRL_PLLCTRL0.FREQ = new_pll_freq; 
		while ( !HW_CLKCTRL_PLLCTRL1.LOCK );  // wait for PLL to lock. 

		// Remove the PLL bypass.
		if ( bPllBypassedNow ) 
			BF_CLR(CLKCTRL_PLLCTRL0, BYPASS);

		old_clocks.pll_freq = new_pll_freq;
	}
	else {
		// Bypass the PLL and power it down.
		if ( bPllBypassedNow == 0 ) 
			BF_SET(CLKCTRL_PLLCTRL0, BYPASS);

		// Power off
		if ( bPllOnNow ) 
			HW_CLKCTRL_PLLCTRL0.POWER = 0; 

		old_clocks.pll_freq = 0;
	}

	return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_manage_spdifclk(unsigned new_clk_div)
{
    const unsigned old_clk_div = old_clocks.spdifclk_div;

    if (new_clk_div != old_clk_div)
    {
        if (new_clk_div)
        {
            HW_CLKCTRL_SPDIFCLKCTRL_WR(BF_CLKCTRL_SPDIFCLKCTRL_DIV(new_clk_div));
        }
        else
        {
           HW_CLKCTRL_SPDIFCLKCTRL_WR(BF_CLKCTRL_SPDIFCLKCTRL_CLKGATE(1));
        }

        old_clocks.spdifclk_div = new_clk_div;
    }

    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_manage_irclk(unsigned new_irov_div, unsigned new_ir_div, unsigned wait_if_needed)
{
    const unsigned old_irov_div = old_clocks.irov_div;
    const unsigned old_ir_div = old_clocks.ir_div;

    if ((new_irov_div != old_irov_div) || (new_ir_div != old_ir_div))
    {
        // Asking to set both IROV divider or IR divider to zero is equivalent to a request to gate IRCLK.
        if (!new_irov_div && !new_ir_div)
        {
            HW_CLKCTRL_SSPCLKCTRL_WR(BF_CLKCTRL_IRCLKCTRL_CLKGATE(1));
        }
        else
        {
            // If new IROV divider and new IR divider are both within range, explicitly set the dividers.
            if ((4 <= new_irov_div) && (new_irov_div <= 130) && (5 <= new_ir_div) && (new_ir_div <= 768))
            {
                HW_CLKCTRL_SSPCLKCTRL_WR(BF_CLKCTRL_IRCLKCTRL_WAIT_PLL_LOCK(wait_if_needed) |
                                         BF_CLKCTRL_IRCLKCTRL_IROV_DIV(new_irov_div) |
                                         BF_CLKCTRL_IRCLKCTRL_IR_DIV(new_ir_div));
            }
            // Otherwise, allow the hardware to automatically set the divide ratios.
            else
            {
                HW_CLKCTRL_SSPCLKCTRL_WR(BF_CLKCTRL_IRCLKCTRL_WAIT_PLL_LOCK(wait_if_needed) |
                                         BF_CLKCTRL_IRCLKCTRL_AUTO_DIV(1));
            }
        }
        
        old_clocks.irov_div = new_irov_div;
        old_clocks.ir_div = new_ir_div;
    }

    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_manage_sspclk(unsigned new_clk_div, unsigned wait_if_needed)
{
    const unsigned old_clk_div = BF_RD(CLKCTRL_SSPCLKCTRL, DIV);

    if (new_clk_div != old_clk_div)
    {
        if (new_clk_div)
        {
            const unsigned wait = (wait_if_needed ? (old_clk_div > new_clk_div) : 0);
            HW_CLKCTRL_SSPCLKCTRL_WR(BF_CLKCTRL_SSPCLKCTRL_WAIT_PLL_LOCK(wait) |
                                     BF_CLKCTRL_SSPCLKCTRL_DIV(new_clk_div));
        }
        else
        {
            HW_CLKCTRL_SSPCLKCTRL_WR(BF_CLKCTRL_SSPCLKCTRL_CLKGATE(1));
        }
        old_clocks.sspclk_div = new_clk_div;
    }

    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_manage_gpmiclk(unsigned new_clk_div, unsigned wait_if_needed)
{
    const unsigned old_clk_div = BF_RD(CLKCTRL_GPMICLKCTRL, DIV);

    if (new_clk_div != old_clk_div)
    {
        if (new_clk_div)
        {
            const unsigned wait = (wait_if_needed ? (old_clk_div > new_clk_div) : 0);
            HW_CLKCTRL_GPMICLKCTRL_WR(BF_CLKCTRL_GPMICLKCTRL_WAIT_PLL_LOCK(wait) |
                                      BF_CLKCTRL_GPMICLKCTRL_DIV(new_clk_div));
        }
        else
        {
            HW_CLKCTRL_GPMICLKCTRL_WR(BF_CLKCTRL_GPMICLKCTRL_CLKGATE(1));
        }
        old_clocks.gpmiclk_div = new_clk_div;
    }

    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_manage_emiclk(unsigned new_clk_div, unsigned wait_if_needed)
{
    const unsigned old_clk_div = BF_RD(CLKCTRL_EMICLKCTRL, DIV);

    if (new_clk_div != old_clk_div)
    {
        if (new_clk_div)
        {
            const unsigned wait = (wait_if_needed ? (old_clk_div > new_clk_div) : 0);

			#include <asm/proc-armv/cache.h>
		    flush_cache_all();	// 2007.11.16

		    while ( BF_RD(EMISTAT, BUSY) || BF_RD(EMISTAT, WRITE_BUFFER_DATA)); 

            HW_CLKCTRL_EMICLKCTRL_WR(BF_CLKCTRL_EMICLKCTRL_WAIT_PLL_LOCK(wait) |
                                     BF_CLKCTRL_EMICLKCTRL_DIV(new_clk_div));

		    while (HW_CLKCTRL_EMICLKCTRL_RD() & BM_CLKCTRL_EMICLKCTRL_BUSY); // 2007.11.16
        }
        else
        {
            HW_CLKCTRL_EMICLKCTRL_WR(BF_CLKCTRL_EMICLKCTRL_CLKGATE(1));
        }
        old_clocks.emiclk_div = new_clk_div;
    }

    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_manage_hbusclk(unsigned new_clk_div, unsigned new_hbusclk_autoslow_rate, unsigned wait_if_needed)
{
    const unsigned old_clk_div = BF_RD(CLKCTRL_HBUSCLKCTRL, DIV);

    if ((new_clk_div != old_clk_div) || (new_hbusclk_autoslow_rate != old_clocks.hbusclk_autoslow_rate))
    {
        const unsigned wait = (wait_if_needed ? (old_clk_div > new_clk_div) : 0);
        const unsigned autoslow_enable = (new_hbusclk_autoslow_rate ? 1 : 0);
        const unsigned autoslow_div = ((new_hbusclk_autoslow_rate == 1) ? 0x0
                                       : ((new_hbusclk_autoslow_rate == 2) ? 0x1
                                          : ((new_hbusclk_autoslow_rate == 4) ? 0x2
                                             : (new_hbusclk_autoslow_rate == 8) ? 0x3 : 0xFFFFFFFF)));

        HW_CLKCTRL_HBUSCLKCTRL_WR(BF_CLKCTRL_HBUSCLKCTRL_WAIT_PLL_LOCK(wait) |
                                  BF_CLKCTRL_HBUSCLKCTRL_EMI_BUSY_FAST(1) |
                                  BF_CLKCTRL_HBUSCLKCTRL_APBHDMA_BUSY_FAST(1) |
                                  BF_CLKCTRL_HBUSCLKCTRL_APBXDMA_BUSY_FAST(0) |
                                  BF_CLKCTRL_HBUSCLKCTRL_TRAFFIC_JAM_FAST(1) |
                                  BF_CLKCTRL_HBUSCLKCTRL_TRAFFIC_FAST(1) |
                                  BF_CLKCTRL_HBUSCLKCTRL_CPU_DATA_FAST(1) |
                                  BF_CLKCTRL_HBUSCLKCTRL_CPU_INSTR_FAST(1) |
                                  BF_CLKCTRL_HBUSCLKCTRL_AUTO_SLOW_MODE(autoslow_enable) |
                                  BF_CLKCTRL_HBUSCLKCTRL_SLOW_DIV(autoslow_div) |
                                  BF_CLKCTRL_HBUSCLKCTRL_DIV(new_clk_div));
        old_clocks.hbusclk_div = new_clk_div;
        old_clocks.hbusclk_autoslow_rate = new_hbusclk_autoslow_rate;

	/* wait for HCLK ready */ 
	while ( BF_RD(CLKCTRL_HBUSCLKCTRL, BUSY ) ) ; 
    }
    
    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_manage_cpuclk(unsigned new_clk_div, unsigned new_cpuclk_interrupt_wait, unsigned wait_if_needed)
{
    const unsigned old_clk_div = BF_RD(CLKCTRL_CPUCLKCTRL, DIV);
    int chip_rev = HW_DIGCTL_CHIPID_RD() & 0xff; 
    if ( chip_rev < 0x5 ) { /* 0x5 - TB1, 0x4 - TA5 */ 
	    new_cpuclk_interrupt_wait = 0; 
    }

    if ((new_clk_div != old_clk_div) || (new_cpuclk_interrupt_wait != old_clocks.cpuclk_interrupt_wait))
    {
        const unsigned wait = (wait_if_needed ? (old_clk_div > new_clk_div) : 0);

        HW_CLKCTRL_CPUCLKCTRL_WR(BF_CLKCTRL_CPUCLKCTRL_WAIT_PLL_LOCK(wait) | 
                                 BF_CLKCTRL_CPUCLKCTRL_INTERRUPT_WAIT(new_cpuclk_interrupt_wait) |
                                 BF_CLKCTRL_CPUCLKCTRL_DIV(new_clk_div));
        old_clocks.cpuclk_div = new_clk_div;
        old_clocks.cpuclk_interrupt_wait = new_cpuclk_interrupt_wait;

	/* wait for PCLK ready */ 
	while ( BF_RD(CLKCTRL_CPUCLKCTRL, BUSY ) ) ; 
    }

    return CLOCKS_ERR_SUCCESS;
}


extern void stmp36xx_change_speed(struct uart_port *port, u_int cflag, u_int iflag, u_int quot);

inline clocks_err_t clocks_manage_xbusclk(unsigned new_clk_div)
{
    unsigned old_clk_div = BF_RD(CLKCTRL_XBUSCLKCTRL, DIV);


    if (new_clk_div != old_clk_div)
    {

        BF_WR(CLKCTRL_XBUSCLKCTRL, DIV, new_clk_div);
        old_clocks.xbusclk_div = new_clk_div;

	/* wait for XCLK ready */ 
	while ( BF_RD(CLKCTRL_XBUSCLKCTRL, BUSY ) ) ; 

	stmp36xx_change_speed(NULL, 0, 0, 0); 
    }

    return CLOCKS_ERR_SUCCESS;
}


inline clocks_err_t clocks_manage_usbclk(unsigned new_pll_freq, unsigned old_pll_freq, unsigned new_usb_enable)
{
    int old_enable = BF_RD(CLKCTRL_PLLCTRL0, EN_USB_CLKS);

    if (new_usb_enable != old_enable )
    {
        // Enable USB, if requested and pll frequency is appropriate.
        if (new_usb_enable && (new_pll_freq == 480))
        {
            // If new PLL frequency differs from old PLL frequency, we must wait until PLL locks first.
            if (new_pll_freq != old_pll_freq)
            {
                clocks_poll_pll_locked();
            }
            
            // Now we can update the USB clock control bitfields.
            BF_SET(CLKCTRL_PLLCTRL0, EN_USB_CLKS);
            HW_CLKCTRL_UTMICLKCTRL_WR(BF_CLKCTRL_UTMICLKCTRL_UTMI_CLK120M_GATE(0) |
                                      BF_CLKCTRL_UTMICLKCTRL_UTMI_CLK30M_GATE(0));
        }
        // Otherwise, disable USB.
        else
        {
            HW_CLKCTRL_UTMICLKCTRL_WR(BF_CLKCTRL_UTMICLKCTRL_UTMI_CLK120M_GATE(1) |
                                      BF_CLKCTRL_UTMICLKCTRL_UTMI_CLK30M_GATE(1));
            BF_CLR(CLKCTRL_PLLCTRL0, EN_USB_CLKS);
        }
        
        old_clocks.usb_enable = new_usb_enable;
    }

    return CLOCKS_ERR_SUCCESS;
}

static int 
stmp36xx_clocks_ioctl(struct inode *inode, struct file *filp, 
	unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned flags; 

	local_irq_save(flags);
	switch(cmd)
	{
	case STMP36XX_OPM_LEVEL:
		ret = apply_clk_policy(arg);
		break;
	}
	local_irq_restore(flags); 
	return ret;
}

static int 
stmp36xx_clocks_open(struct inode *inode, struct file * filp)
{
	if (mod_usecount != 0)
	{
		printk(KERN_ERR "clocks control device cannot be opened!!\n");
		return -EAGAIN;
	}

	mod_usecount = 1;
	
	return 0;
}

static int 
stmp36xx_clocks_release(struct inode *inode, struct file *filp)
{
	mod_usecount = 0;
	
	return 0;
}

static struct file_operations stmp36xx_clocks_fops = {
	ioctl:		stmp36xx_clocks_ioctl,
	open:		stmp36xx_clocks_open,
	release:	stmp36xx_clocks_release,
};

static struct miscdevice stmp36xx_clocks_misc = {
	minor : STMP36xx_CLOCKS_MINOR,
	name  : "stmp36xx_clocks",
	fops  : &stmp36xx_clocks_fops,
};

int __init init_clkctrl(void)
{
	struct proc_dir_entry *part_root; 

	// semaphore init
	sema_init (&clk_sem, 1);

	// Register ssftl proc entry 
	part_root = create_proc_entry("clkctrl", S_IWUSR | S_IRUGO, NULL); 
	
	part_root->read_proc = clkctrl_read_procmem; 
	part_root->write_proc = clkctrl_write_procmem; 
	part_root->data = NULL; 

	/* register misc device for clock change */
	if (misc_register(&stmp36xx_clocks_misc) != 0)
	{
		printk(KERN_ERR "Cannot register device /dev/%s\n",
			   stmp36xx_clocks_misc.name);
		return -EFAULT;
	}
	mod_usecount = 0;
	current_clock_run_level = -1;

	return 0; 
}
void __exit cleanup_clkctrl(void)
{
	misc_deregister(&stmp36xx_clocks_misc);

	remove_proc_entry("clkctrl", 0); 
}

// Joseph add functions for the Power optimization
clocks_err_t apply_clk_policy(int mode)
{
	if(is_usb_enabled())
	{
		PDEBUG("[CLKCTRL] If USB is enabled, you can't change clocks...\n");
		PDEBUG("[CLKCTRL] First of all, you should disable USB\n");
		return CLOCKS_ERR_FAILURE;
	}

	if(current_clock_run_level == mode)
	{
		PDEBUG("[CLKCTRL]Currently system is running what you are setting...\n");
		return CLOCKS_ERR_SUCCESS;
	}

	switch (mode)
	{
	case SS_MAX_CPU:
		//PDEBUG("[CLKCTRL]SS_MAX_CPU is applied\n");
		set_clocks_mode(&CLOCKS_MODE_MAXPERF);
		break;
	case SS_USB:
		//USB will handle USB itself.
		//PDEBUG("[CLKCTRL]SS_USB is applied\n");
		//set_clocks_mode(&CLOCKS_MODE_MAXUSBCPU);
		break;
	case SS_AUDIO:
		//PDEBUG("[CLKCTRL]SS_AUDIO is applied\n");
		set_clocks_mode(&CLOCKS_MODE_MP3);
		break;
	case SS_AUDIO_DNSE:
		//PDEBUG("[CLKCTRL]SS_AUDIO_DNSE is applied\n");
		set_clocks_mode(&CLOCKS_MODE_MP3_DNSE);
		break;
	case SS_WMA:
		//PDEBUG("[CLKCTRL]SS_WMA is applied\n");
		set_clocks_mode(&CLOCKS_MODE_WMA);
		break;
	case SS_WMA_DNSE:
		//PDEBUG("[CLKCTRL]SS_WMA_DNSE is applied\n");
		set_clocks_mode(&CLOCKS_MODE_WMA_DNSE);
		break;
	case SS_OGG:
		//PDEBUG("[CLKCTRL]SS_OGG is applied\n");
		set_clocks_mode(&CLOCKS_MODE_OGG);
		break;
	case SS_OGG_DNSE:
		//PDEBUG("[CLKCTRL]SS_OGG_DNSE is applied\n");
		set_clocks_mode(&CLOCKS_MODE_OGG_DNSE);
		break;
	case SS_FM: /* !!TODO!! */
		set_clocks_mode(&CLOCKS_MODE_FM); 
		break;
	case SS_FM_DNSE: /* !!TODO!! */
		set_clocks_mode(&CLOCKS_MODE_FM_DNSE); 
		break;
	case SS_RECORDING: /* !!TODO!! maximum nand perf, hcyun */ 
		set_clocks_mode(&CLOCKS_MODE_OGG); 
		break;
	case SS_IDLE:
		//PDEBUG("[CLKCTRL]SS_IDLE is applied\n");
		set_clocks_mode(&CLOCKS_MODE_IDLE);
		break;
	case SS_POWER_OFF:
		//PDEBUG("[CLKCTRL]SS_POWER_OFF is applied\n");
		break;
	default:
		printk("[CLKCTRL]Invalid clock level\n");
		break;
	}

	apply_pw_policy(mode);
	
	current_clock_run_level = mode;
	
	return CLOCKS_ERR_SUCCESS;
}

clocks_err_t set_clk_mp3_opt(bool set)
{
	if(set !=0)
	{
		//const unsigned current_clock = old_clocks.hbusclk_div;
		const hw_clkctrl_hbusclkctrl_t  hbusclkctrl  = HW_CLKCTRL_HBUSCLKCTRL;
		const hw_emidramctrl_t  emidramctrl  = HW_EMIDRAMCTRL;
		
		//printk("[CLKCTRL]Current auto slow rate is %d\n", old_clocks.hbusclk_autoslow_rate); 

		clock_context.CLOCK_HBUS_TRAFFIC_FAST = 1;
		clock_context.CLOCK_HBUS_EMI_BUSY_FAST = 1;
		clock_context.CLOCK_EMI_DRAM_PRECHARGE = 1;
		clock_context.CLOCK_EMI_AUTO_EMICLK_GATE = 1;
		
		if(!hbusclkctrl.TRAFFIC_FAST)
		{
			clock_context.CLOCK_HBUS_TRAFFIC_FAST = 0;
			HW_CLKCTRL_HBUSCLKCTRL_SET(BM_CLKCTRL_HBUSCLKCTRL_TRAFFIC_FAST);
			//printk("[CLKCTRL]TRAFFIC_FAST is on in HBUSCLK\n"); 
		}
		//else	 printk("[CLKCTRL]TRAFFIC_FAST is already on in HBUSCLK\n"); 
		
		if(!hbusclkctrl.EMI_BUSY_FAST)
		{
			clock_context.CLOCK_HBUS_EMI_BUSY_FAST = 0;
			HW_CLKCTRL_HBUSCLKCTRL_SET(BM_CLKCTRL_HBUSCLKCTRL_EMI_BUSY_FAST);
			//printk("[CLKCTRL]EMI_BUSY_FAST is on in HBUSCLK\n"); 
		}
		//else	 printk("[CLKCTRL]EMI_BUSY_FAST is already on in HBUSCLK\n"); 


		if(!emidramctrl.PRECHARGE)
		{
			clock_context.CLOCK_EMI_DRAM_PRECHARGE = 0;
			HW_EMIDRAMCTRL_SET(BM_EMIDRAMCTRL_PRECHARGE);
			//printk("[CLKCTRL]PRECHARGE is on in emidramctrl\n"); 
		}
		//else	 printk("[CLKCTRL]PRECHARGE is already on in emidramctrl\n"); 

		if(!emidramctrl.AUTO_EMICLK_GATE)
		{
			clock_context.CLOCK_EMI_AUTO_EMICLK_GATE = 0;
			HW_EMIDRAMCTRL_SET(BM_EMIDRAMCTRL_AUTO_EMICLK_GATE);
			//printk("[CLKCTRL]AUTO_EMICLK_GATE is on in emidramctrl\n"); 
		}
		//else	 printk("[CLKCTRL]AUTO_EMICLK_GATE is already on in emidramctrl\n"); 

		//test for idle: this make system down we should run on this in RAM
//		if(!emidramctrl.SELF_REFRESH)
//		{	
//			printk("[CLKCTRL]SELF_REFRESH is on in emidramctrl\n"); 
//			HW_EMIDRAMCTRL_SET(BM_EMIDRAMCTRL_SELF_REFRESH);			
//		}
//		else	 printk("[CLKCTRL]SELF_REFRESH is already on in emidramctrl\n"); 
	}
	else
	{

		if(clock_context.CLOCK_HBUS_TRAFFIC_FAST == 0)
		{
			HW_CLKCTRL_HBUSCLKCTRL_CLR(BM_CLKCTRL_HBUSCLKCTRL_TRAFFIC_FAST);
			//printk("[CLKCTRL]TRAFFIC_FAST is off in HBUSCLK\n"); 
		}
		//else	 printk("[CLKCTRL]TRAFFIC_FAST is already off in HBUSCLK\n"); 
		
		if(!clock_context.CLOCK_HBUS_EMI_BUSY_FAST)
		{
			HW_CLKCTRL_HBUSCLKCTRL_CLR(BM_CLKCTRL_HBUSCLKCTRL_EMI_BUSY_FAST);
			//printk("[CLKCTRL]EMI_BUSY_FAST is off in HBUSCLK\n"); 
		}
		//else	 printk("[CLKCTRL]EMI_BUSY_FAST is already off in HBUSCLK\n"); 


		if(!clock_context.CLOCK_EMI_DRAM_PRECHARGE)
		{
			HW_EMIDRAMCTRL_CLR(BM_EMIDRAMCTRL_PRECHARGE);
			//printk("[CLKCTRL]PRECHARGE is off in emidramctrl\n"); 
		}
		//else	 printk("[CLKCTRL]PRECHARGE is already off in emidramctrl\n"); 

		if(!clock_context.CLOCK_EMI_AUTO_EMICLK_GATE)
		{
			HW_EMIDRAMCTRL_SET(BM_EMIDRAMCTRL_AUTO_EMICLK_GATE);
			//printk("[CLKCTRL]AUTO_EMICLK_GATE is off in emidramctrl\n"); 
		}
		//else	 printk("[CLKCTRL]AUTO_EMICLK_GATE is already off in emidramctrl\n"); 

	}

	//printk("[CLKCTRL]Current auto slow rate is %d after applied\n", old_clocks.hbusclk_autoslow_rate); 
	return CLOCKS_ERR_SUCCESS; 
}


clocks_err_t set_clk_idle(bool set)
{
	if(set !=0)
	{
		//printk("[CLKCTRL]comming in set clk idle mode\n");

		set_clk_mp3_opt(true);
		//for mp3 testing 
		//************** not that much
/*		BF_CS1(CLKCTRL_XTALCLKCTRL, PWM_CLK24M_GATE, 1);
		
//		BF_CS1(CLKCTRL_XTALCLKCTRL, UART_CLK_GATE, 1);
		BF_CS1(CLKCTRL_XTALCLKCTRL, FILT_CLK24M_GATE, 1);
		BF_CS1(CLKCTRL_XTALCLKCTRL, DRI_CLK24M_GATE, 1);
		BF_CS1(CLKCTRL_XTALCLKCTRL, DIGCTRL_CLK1M_GATE, 1);
		BF_CS1(CLKCTRL_XTALCLKCTRL, TIMROT_CLK32K_GATE, 1);
		BF_CS1(CLKCTRL_XTALCLKCTRL, EXRAM_CLK16K_GATE, 1);
//		BF_CS1(CLKCTRL_XBUSCLKCTRL, DIV, 0xF0);
//		BF_CS1(CLKCTRL_CPUCLKCTRL, DIV, 0xF0);
		BF_CS1(DIGCTL_RAMCTRL, PWDN_BANKS, 0xF);

		//   BF_CS1(CLKCTRL_CPUCLKCTRL, INTERRUPT_WAIT, 1);
		//    BF_CS1(CLKCTRL_HBUSCLKCTRL, SLOW_DIV, 0x3);
*/
		//************** not that much
/*
		//This should force use to use about 100 clocks in the same cache line.
		register int delay=50; 
		__asm__  __volatile__ ( "nop" );
		__asm__  __volatile__ ( "nop" );
		__asm__  __volatile__ ( "nop" );

		HW_CLKCTRL_CPUCLKCTRL_SET(BM_CLKCTRL_CPUCLKCTRL_INTERRUPT_WAIT);

		//VERIFY that this infact is accessing a REGISTER and not a memory access¡¦ 
		//this is critical to get the CPU off the bus.
		while (--delay != 0);   

		//In the ISR >BEFORE ICOLL_LEVELACK<<<
		HW_CLKCTRL_CPUCLKCTRL_CLR(BM_CLKCTRL_CPUCLKCTRL_INTERRUPT_WAIT);

		//NOTICE THIS MUUUUUST come AFTER the interrupt_wait and NOT BEFORE, 
		//this is CRITICAL, or the CPU will hang.
		HW_ICOLL_LEVELACK_WR(1);
		*/
	}
	else
	{
		set_clk_mp3_opt(false);
	}
	return CLOCKS_ERR_SUCCESS; 		
}

void set_debug_msg(int level)
{ 
	debug_msg = level;
}

module_init(init_clkctrl);
module_exit(cleanup_clkctrl); 

////////////////////////////////////////////////////////////////////////////////
//
// $Log: clocks.c,v $
// Revision 1.94  2007/11/16 04:56:56  hcyun
//
// clean up cache and add wait until emi is not busy
//
// - hcyun
//
// Revision 1.93  2007/10/16 12:14:03  zzinho
// clock modify when DNSe on
// by jinho.lim
//
// Revision 1.92  2007/10/08 08:13:16  zzinho
// modify clock setting when applying Dnse
// by jinho.lim
//
// Revision 1.91  2007/09/19 08:31:26  zzinho
// proc read real register
// by jinho.lim
//
// Revision 1.90  2007/08/23 12:04:41  hcyun
// 200/100/100
//
// 160/80/80
//
// Revision 1.89  2007/03/21 05:19:39  hcyun
// back to original due to EMI
//
// - hcyun
//
// Revision 1.88  2007/03/14 02:58:11  hcyun
// 200/100/100 -> 200/66/66
//
// Revision 1.87  2007/03/02 08:04:13  hcyun
// bug fix...
//
// - hcyun
//
// Revision 1.86  2007/03/02 07:50:12  hcyun
// change vddio setting sequence during the clock control
//
// - hcyun
//
// Revision 1.85  2007/03/02 07:21:12  hcyun
// back to usb=120/60/60, cpu=200/66/66
//
// Revision 1.84  2007/03/02 05:40:35  hcyun
// code indentation
//
// - hcyun
//
// Revision 1.83  2007/03/02 00:08:16  hlim72
// VDDIO Setting Step is Adjusted
//
// Revision 1.82  2007/02/27 01:36:23  hlim72
// PERF       -> 200/100/100
// USBCLK -> 160/80/80
//
// Revision 1.81  2007/02/03 05:09:00  hcyun
// for max cpu clock and max usb clock test
// - hcyun
//
// Revision 1.80  2006/12/13 09:50:50  hcyun
// if you set debug enable, the serial now will not break
//
// - hcyun
//
// Revision 1.79  2006/05/02 11:11:00  hcyun
// taken from phase2 2.0.33 kernel source tree.
//
// Revision 1.84  2006/04/18 02:05:49  hcyun
// Increase OGG clock. (Jinho's requirement. for 400Kbps OGG Q10 file)
//
// - hcyun
//
// Revision 1.83  2006/04/06 08:39:39  hcyun
// Do not use interrupt wait for USB.
//
// Revision 1.82  2006/04/05 00:29:11  hcyun
// FM noise fix.. enable RC_SCALE & EN_EC1RCSCALE to improves the DCDC's responsiveness. This solve IO voltage dipping problem. (possible to reduce voltage further)
//
// Revision 1.81  2006/03/31 01:02:08  hcyun
// enable xbus divisor 8 for all audio clock mode.
//
// OGG : 80/40/40 ->  60/30/30
// OGG_DNSE : 80/80/40 -> 80/40/40
//
// Revision 1.80  2006/03/28 05:27:40  hcyun
// remove apply_mp3_opt -- Need verification
// autoslow = 8 for all audio codec
// APBX_DMA_BUSY_FAST = 0
// xbus_div = 8 is not yet applied.
//
// Revision 1.79  2006/03/22 02:23:20  hcyun
// Support INTERRUPT_WAIT feature to save power.. - hcyun
//
// Power consumption comparison on PhaseI + TB1 with serial connected.
//
// 	without INTR	with INTR
// ----------------------------------------
// 24	23.5		25.0
// 48	41.0		36.5
// 40	38.5		35.0
// 80	45.0		37.0
// 200	>100		66
//
// Revision 1.78  2006/02/14 07:57:46  biglow
// - delete code to control PLLCPNSEL and PLLV2ISEL
//
// Revision 1.77  2006/02/03 00:57:31  biglow
// - change PLLV2ISEL value
//
// Revision 1.76  2006/01/23 09:46:19  hcyun
// 60/60/60 -> 80/40/40  for stable WMA+DNSe setting
//
// Revision 1.75  2006/01/10 09:49:41  biglow
// - fix usb operation current of each condition
//
// Revision 1.74  2006/01/08 07:51:02  biglow
// - fix for USBCV in Belkin F5U221
//
// Revision 1.73  2006/01/05 23:32:21  hcyun
// lower the gpmi clock on low power usb mode
//
// Revision 1.72  2006/01/05 04:25:23  hcyun
// Safe GPMI clock setting.
//
// Revision 1.71  2005/12/31 08:14:00  biglow
// - declare usb common functions
//
// Revision 1.70  2005/12/30 09:08:21  joseph
// Turn off debug option as default...
//
// Revision 1.69  2005/12/30 09:02:53  joseph
// Audio mute fixed in idle and move GPIO setting to right before deep idle and right after deep idle.
//
// Revision 1.68  2005/12/29 07:12:07  joseph
// change idle clock to 24Mhz
//
// Revision 1.67  2005/12/29 07:09:53  biglow
// - add usb mode
//
// Revision 1.66  2005/12/22 13:14:19  joseph
// turn back to original idle clock
//
// Revision 1.65  2005/12/22 08:42:02  joseph
// Idle clock change to 24M for deep idle....
//
// Revision 1.64  2005/12/21 09:56:15  hcyun
// stable WMA + DNSe clock support..
// (need more investigation about the reason of the 260/45/45/45 Mhz clock unstable issue)
//
// Revision 1.63  2005/12/09 00:12:03  zzinho
// mp3+dnse : 48/48/48
// wma+dnse : 65/65/65
// ogg+dnse : 90/90/90
//
// Revision 1.62  2005/12/08 04:10:23  hcyun
// back to the 120Mhz..
//
// - hcyun
//
// Revision 1.61  2005/12/02 06:33:38  zzinho
// ogg_dnse 90MHz
//
// Revision 1.60  2005/12/01 06:19:09  hcyun
// protect ioctl
//
// - hcyun
//
// Revision 1.59  2005/12/01 05:51:00  hcyun
// Big change in clock control system..
//
// now using only one clock control path regadless of pll_rising, pll_falling, pll_enable or pll_disable.
//
// - hcyun
//
// Revision 1.58  2005/11/24 08:03:16  zzinho
// remove  safe divider xbusclk
//
// Revision 1.57  2005/11/22 13:38:33  biglow
// - for delete warning message at compiling
//
// Revision 1.56  2005/11/21 23:57:51  hcyun
// more safe pll enable
//
// - hcyun
//
// Revision 1.55  2005/11/16 11:07:33  joseph
// add Remove USB setclockfunction. USB will set himself..
//
// Revision 1.54  2005/11/15 00:02:19  joseph
// add debug option and comment out all other debug message
//
// Revision 1.53  2005/11/14 08:03:15  joseph
// remove debug info
//
// Revision 1.52  2005/11/14 07:55:07  joseph
// Add DNSe and USB cable bug
//
// Revision 1.51  2005/11/09 11:15:58  hcyun
// support gpmi access timing change when PLL is disabled or enabled.. This will improve mp3 mode read performance about 25% in case SRAM buffer is used.. SDRAM buffer will improve about 15%.
//
// minor cleanup to remove warning..
//
// - hcyun
//
// Revision 1.50  2005/11/09 00:12:54  zzinho
// modified by jinho.lim
// - set core level as step by step
//
// Revision 1.49  2005/10/27 09:19:46  biglow
// - fix up bug of inital state mode. If mode is -1,  first state or unkown state
//
// Revision 1.48  2005/10/27 08:56:29  biglow
// - check only usb enabled
//
// Revision 1.47  2005/10/27 07:42:48  biglow
// - check only usb enabled
//
// Revision 1.46  2005/10/27 06:23:57  hcyun
// MAXNAND -> 68/68/68
//
// Revision 1.45  2005/10/27 05:55:54  hcyun
// new clock settings for WMA, OGG, ..
//
// Revision 1.44  2005/10/27 05:22:42  biglow
// - change policy of OGG mode
//
// Revision 1.43  2005/10/26 08:04:24  biglow
// - patch for locking during USB enable state
//
// Revision 1.42  2005/10/25 07:43:29  tknoh
// wma, ogg mode clock value change
//
// Revision 1.41  2005/10/25 06:54:42  joseph
// GPMI div bug fix in WMA and OGG mode
//
// Revision 1.40  2005/10/25 06:43:01  joseph
// Add WMA case and OGG case for the runlevel
//
// Revision 1.39  2005/10/22 00:57:01  zzinho
// dos2unix
//
// Revision 1.38  2005/10/15 05:11:20  zzinho
// added by heechul in USA
//
// Revision 1.37  2005/10/15 00:33:49  joseph
// Add run level 16 for the USB
//
// Revision 1.36  2005/10/14 10:45:50  joseph
// change 200/66/66/66 to max run level
//
// Revision 1.35  2005/10/13 23:51:26  zzinho
// added by heechul in USA
//
// Revision 1.33  2005/10/12 11:21:25  joseph
// Create Idle clock mode, add spin lock irg function, changing clock div init value, fix divide 0 issue
//
// Revision 1.32  2005/10/07 05:38:13  joseph
// Disable debugging message
//
// Revision 1.31  2005/10/07 05:23:31  joseph
// MAX clock change to 200/66/66  and modify setting clock debugging message
//
// Revision 1.30  2005/10/06 01:43:03  joseph
// Run level command change
//
// Revision 1.29  2005/10/06 01:05:23  joseph
// modified apply clk policy
//
// Revision 1.28  2005/10/05 11:53:00  joseph
// apply policy to power when clock change run level
//
// Revision 1.27  2005/10/05 00:51:19  biglow
// - optimize with clocks
//
// Revision 1.26  2005/10/01 01:16:44  hcyun
// Now default clock is 400/200/100/50
//
// - hcyun
//
// Revision 1.25  2005/09/30 12:24:01  hcyun
// MAX USB clock is 480/96/96/48
//
// Revision 1.24  2005/09/30 11:32:57  hcyun
// MAXNAND clock support (mode 7)
//
//
// - hcyun
//
// Revision 1.23  2005/09/29 06:40:53  joseph
// add voltage rescaling depend on clock settings
//
// Revision 1.22  2005/09/29 02:11:48  biglow
// - patch for improving USB signal quality
//
// Revision 1.21  2005/09/29 02:09:41  hcyun
// fixed gpmi clock.
//
// - hcyun
//
// Revision 1.20  2005/09/28 07:28:10  hcyun
// MAXPERF -> 400/200/100/50
//
// PLL bug workaround is implemented... (clocks_manage_pll())
//
// - hcyun
//
// Revision 1.19  2005/09/22 06:54:13  biglow
// - change USB mode
//
// Revision 1.18  2005/09/16 00:05:08  hcyun
// removed unnecessary sspclk(MMC/SD), spdifclk(SPDIF).
// now unit chainging commands such as "cpu", "hbus"..  are correctly supported. Therefore you can change indivisual clock as you want.
//
// - hcyun
//
// Revision 1.17  2005/09/15 12:20:49  biglow
// - add USB mode
//
// Revision 1.16  2005/08/29 06:36:46  joseph
// implement level 0 to 9
//
// Revision 1.15  2005/08/29 04:56:17  hcyun
// add brazo mode (480/80/40/40)
//
// - hcyun
//
// Revision 1.14  2005/08/24 23:13:21  hcyun
// enable rfs debugging messages. (fm_global.h)
//
// - hcyun
//
// Revision 1.13  2005/08/08 02:10:32  hcyun
// OGG timing is added..
//
// - hcyun
//
// Revision 1.12  2005/08/06 00:30:11  hcyun
// GPMI clock adjustment on MP3 mode.
// loops_per_jiffy correction for proper udelay() timing on clock switching.
//
// - hcyun
//
// Revision 1.11  2005/08/03 13:54:06  joseph
// Joseph add apply mp3 optimized function for mp3 power testing
//
// Revision 1.10  2005/07/27 05:25:12  hcyun
// mode setting /proc/clkctrl
//
// $ echo "mode [0|1|2|3] > /proc/clkctrl"
//
//
// - hcyun
//
// Revision 1.9  2005/07/23 03:35:17  hcyun
// pll setting features added.. (assume that pll is already turned on..)
//
// - hcyun
//
// Revision 1.8  2005/07/21 05:39:33  hcyun
// add form factor board type
// set 160/54 as a defalt clock...
//
// Revision 1.7  2005/06/21 09:54:11  hcyun
// wait if needed...
//
// - hcyun
//
// Revision 1.6  2005/06/08 16:39:37  hcyun
// pretty
//
// - hcyun
//
// Revision 1.5  2005/06/08 14:16:27  hcyun
// set default clock speed to 120/60/60/120.
//
// - hcyun
//
// Revision 1.4  2005/06/06 17:25:29  hcyun
// MAXPOWER clock is 60/60/60 for now..
//
// - hcyun
//
// Revision 1.3  2005/05/26 20:39:28  hcyun
// pll enable..
//
// - hcyun
//
// Revision 1.2  2005/05/18 15:30:49  hcyun
// comment is added
//
// - hcyun
//
// Revision 1.1  2005/05/12 18:12:17  hcyun
// Basic clock control features. most of clock divisor can be controlled such as cpu,hbus,xbux,..., etc. It provide /proc interface "/proc/clkctrl". you can read the current clock status and also you can control clock divisor values by writing through that interface. To see the  provided command set please refer "clkctrl_write_procmem" function in the source code..
//
// This code is mostly taken from the sigmatel's validation code. I slightly modified and added proc interface to it.
//
// As the currentl engineering board seems to unstable, I didn't tested much. [FIXME].
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
// Revision 1.17  2005/03/01 10:53:06  ttoelkes
// Fixed a couple of minor issues with 'init_clocks_mode()'.
//
// Revision 1.16  2005/03/01 10:49:19  ttoelkes
// Fixed latent bug inherent in the ordering associated with applying the
// WAIT_PLL_LOCK control and changing the PLL frequency.
//
// Revision 1.15  2005/02/28 17:43:10  ttoelkes
// significant modification to clocking API
//
// Revision 1.14  2005/02/24 15:28:26  ttoelkes
// fixing reason nand tests are broken in brazo sims and emulation (need to ungate clock in clkctrl even in brazo)
//
// Revision 1.13  2005/02/21 03:23:59  ttoelkes
// major changes to 'clocks' api
//
// Revision 1.12  2005/02/16 05:56:27  ttoelkes
// fixed XBUSCLK bug created by losing file on aborted commit (grrrrr)
//
// Revision 1.11  2005/02/15 21:59:26  ttoelkes
// major revision of clock config logic based upon discussion with Matt Henson
//
// Revision 1.9  2005/02/14 15:58:41  ttoelkes
// committing clock config development
//
// Revision 1.8  2005/02/08 17:10:32  ttoelkes
// updating 'clocks' library
//
// Revision 1.7  2005/02/01 03:56:57  ttoelkes
// try it yet again...this is a bit complicated
//
// Revision 1.6  2005/02/01 03:20:31  ttoelkes
// yet another attempt at capturing the condition of stabilized clocks
//
// Revision 1.5  2005/02/01 01:07:16  ttoelkes
// actually, individual clocks can't be considered settled until both
// WAIT_FOR_PLL and BUSY have been cleared
//
// Revision 1.4  2005/02/01 00:03:36  ttoelkes
// need to wait for PLL to lock before waiting for clocks to stabilize
//
// Revision 1.3  2005/01/31 23:54:28  ttoelkes
// carelessly forgot to check the busy flag for the individual clocks
// before clearing bypass and exiting
//
// Revision 1.2  2005/01/31 17:50:21  ttoelkes
// made 'clocks_pll_enable' more robust
//
// Revision 1.1  2005/01/28 02:25:17  ttoelkes
// promoted clock functions to common lib
//
////////////////////////////////////////////////////////////////////////////////

