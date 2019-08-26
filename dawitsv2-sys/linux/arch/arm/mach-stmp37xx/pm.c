/*
 * pm.c
 * STMP37xx Power management driver for Samsung
 *
 * Copyright (C) 2008 Mooji Semiconductor
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
 * 
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <asm/arch/irqs.h>
#include <asm/arch/stmp37xx_pm.h>
#include <asm/arch/ocram.h>
#include <asm/arch/lradc.h>
#include <asm/cacheflush.h>

#include "include/error.h"
#include "clocks/ddi_clocks.h"
#include "clocks/hw_clocks.h"
#include "clocks/ddi_emi.h"
#include "power/ddi_power.h"


#define BATT_SAMPLING_INTERVAL	200		// 100msec (2kHz timer)
#define BATT_BO_LEVEL			4		// 2.89V (2.73V + 0.04 * BATT_BO_LEVEL)
//#define DEFAULT_VDDA	2100 //FIXME, dhsong
#define DEFAULT_VDDA	1750 //FIXME, dhsong

/* minor number of misc device */
#define	STMP_PM_MINOR		69

#define USB_PM		1
#define ADAPTER_PM	0

#define MAX_CLOCK	320000
//#define DEBUG

#ifdef DEBUG
 #define PDEBUG(fmt, args...) printk(fmt , ## args)
#else
 #define PDEBUG(fmt, args...) do {} while(0)
#endif

#define LOW_ANAL_CUR
//#define SEND_CONN_EVT_AFTER_DISCONN_EVT ////add 1205 to do not send connection event 2 times successively

#if 1 //add to check usb or adapter with lradc
#define DEV_NAME	"usb_dp_check"
//#define USB_LRADC
#define USB_DP_ADC_CH			LRADC_CH_USB_DN
#define USB_DP_LOOPS_PER_SAMPLE	        (0)
#define USB_DP_SAMPLES_PER_SEC	        (1) /* 10 interrupts per 1 second */

#define USB_DP_FREQ \
        (10 / ((USB_DP_LOOPS_PER_SAMPLE+1) * USB_DP_SAMPLES_PER_SEC))

#if USB_DP_LOOPS_PER_SAMPLE > 0
# define USB_DP_ADC_ACC (1)
#else
# define USB_DP_ADC_ACC (0)
#endif
static int adc_delay_slot = -1;
static int usb_dp_check = -1;
static int usb_check_count = 1;
static int vdd5v_timer_count = 1;
static int nand_flash_check = -1;
static int lpj = 0;
static int enable_HclkSlow = true;

static int usb_dp_adc_init(int slot, int channel, void *data);
static void usb_lradc_init();
static void usb_dp_check_handler(int slot, int channel, void *data);
static void usb_dp_adc_handler(int slot, int channel, void *data);
#endif //#if 1 //add to check usb or adapter with lradc

static struct lradc_ops usb_dp_ops = {
        .init           = usb_dp_adc_init,
#ifdef USB_LRADC
        .handler        = usb_dp_adc_handler,
#else
        .handler        = usb_dp_check_handler,
#endif
        .num_of_samples = USB_DP_LOOPS_PER_SAMPLE,
};

typedef struct _pm_mode
{
	char		*name;
	/* PCLK */
	unsigned	pclk;
	bool		pclk_intr_wait;

	/* HCLK */
	unsigned	hclkdiv;
	bool		hclkslowenable;
	/*SLOW_DIV_BY1, SLOW_DIV_BY2, SLOW_DIV_BY4, SLOW_DIV_BY8, SLOW_DIV_BY16, SLOW_DIV_BY32 */
	unsigned	hclkslowdiv;

	unsigned	emiclk;
	unsigned	gpmiclk;
	unsigned	xclk;
	unsigned	pixclk;
	unsigned	sspclk;
	unsigned	irclk;
	unsigned	irovclk;

	unsigned	vddd, vddd_bo;
	unsigned	vdda, vdda_bo;
	unsigned	vddio, vddio_bo;
} pm_mode_t;

struct stmp37xx_pm_dev {
	stmp37xx_pm_callback callback;
	struct list_head entry;
	int type;
};

static LIST_HEAD(pm_list);

static int usecount = 0;
static int pm_cur = -1;
static int usb_or_adapter = -1;
static int usb_disconn_check = -1;
static int get_5v_present_flag = 0;
static unsigned prev_event = SS_PM_NOT_USED_EVNET;
static unsigned int pinctrl_muxsel[8] = {0};
static unsigned int pinctrl_drive[15] = {0};
static unsigned int pinctrl_doe[3] = {0};

unsigned char *emi_block_end = emi_block_startaddress + 0x00001000; //4K

static struct timer_list timer_vdd5v;
static struct timer_list timer_handoff;
static struct timer_list timer_usb_check;

static spinlock_t pm_event_lock = SPIN_LOCK_UNLOCKED;

static int pm_enter_idle (void);
static int set_gpio_idle(void);
static int set_gpio_wakeup(void);
static int mem_gpio_setting(void);
int is_USB_connected(void);
int is_adapter_connected(void);
int set_battery_charging_2(unsigned int voltage, unsigned int threshold);
int set_pm_mode_stable( int pm_opm_state );


#include "pm_table.c"

#define EMI_CLOCK_CHANGE(clk) \
	pm_run_inocram(emi_block_startaddress, emi_block_endddress, ddi_emi_ChangeClockFrequency, clk);


static int print_pm_info (char *buf, int size)
{
	return scnprintf(buf, size,
			"---------------------------------------------------------------\n"
			" [OPM_LEVEL] %s(%d)\n"
			" [CLOCK]\n"
			"  PCLK %d HCLK %d HCLKSLOW %d EMICLK %d GPMICLK %d XCLK %d\n"
			"  SSPCLK %d IRCLK %d IROVCLK %d PIX %d\n"
			"  PllRef %d \n"
			//"  PllRef %d HclkSlow %d\n"
			"  [BYPASS]\n"
			"   cpu %d emi %d ssp %d gpmi %d pix %d ir %d\n"
			" [POWER]\n"
			"  vddd  %d bo %d src %d\n"
			"  vdda  %d bo %d src %d\n"
			"  vddio %d bo %d src %d\n"
			" [HW_POWER_REGS]\n"
			"  CTRL     %08x 5VCTRL   %08x MINPWR      %08x\n"
			"  VDDDCTRL %08x VDDACTRL %08x VDDIOCTRL   %08x\n"
			"  DCFUNCV  %08x MISC     %08x DCLIMITS    %08x\n"
			"  LOOPCTRL %08x STS      %08x BATTMONITOR %08x\n"
			"---------------------------------------------------------------\n",
			pm_table[pm_cur].name, pm_cur,
			
			ddi_clocks_GetPclk(), ddi_clocks_GetHclk(), ddi_clocks_GetHclkSlow(),
			ddi_clocks_GetEmiClk(), ddi_clocks_GetGpmiClk(), ddi_clocks_GetXclk(), 
			ddi_clocks_GetSspClk(), ddi_clocks_GetIrClk(), ddi_clocks_GetIrovClk(),
			ddi_clocks_GetPixClk(),
			ddi_clocks_GetPllStatus() ? ddi_clocks_GetMaxPllRefFreq() : 0, 
			//ddi_clocks_GetPllStatus() ? ddi_clocks_GetMaxPllRefFreq() : 0, enable_HclkSlow, //20081231, to select enable/disable hclkslow
			
			ddi_clocks_GetBypassRefCpu(), ddi_clocks_GetBypassRefEmi(),
			ddi_clocks_GetBypassRefIoSsp(), ddi_clocks_GetBypassRefIoGpmi(), 
			ddi_clocks_GetBypassRefPix(), ddi_clocks_GetBypassRefIoIr(),
			
			ddi_power_GetVddd(), ddi_power_GetVdddBrownout(), hw_power_GetVdddPowerSource(), 
			ddi_power_GetVdda(), ddi_power_GetVddaBrownout(), hw_power_GetVddaPowerSource(), 
			ddi_power_GetVddio(), ddi_power_GetVddioBrownout(), hw_power_GetVddioPowerSource(),
			
			HW_POWER_CTRL_RD(), HW_POWER_5VCTRL_RD(), HW_POWER_MINPWR_RD(),
			HW_POWER_VDDDCTRL_RD(), HW_POWER_VDDACTRL_RD(), HW_POWER_VDDIOCTRL_RD(),
			HW_POWER_DCFUNCV_RD(), HW_POWER_MISC_RD(), HW_POWER_DCLIMITS_RD(),
			HW_POWER_LOOPCTRL_RD(), HW_POWER_STS_RD(), HW_POWER_BATTMONITOR_RD()
		);
}

typedef int (*ocram_func_t)(int);

static int pm_run_inocram (void *block_start, void *block_end, int (*function)(int), int param)
{
	unsigned long flags;
	unsigned long sp_save;
	unsigned long sp_new = OCRAM_RUN_STACK_START_VIRT;
	int ret;

	// calc. ocram function address
	ocram_func_t call = (ocram_func_t)((unsigned long)function-(unsigned long)block_start 
									  + OCRAM_RUN_CODE_START_VIRT);

	PDEBUG("calc used ocram = %d Byte\n ", block_end - block_start );
	PDEBUG("@@@@@ Enter OCRAM Function @@@@@\n %p %p\n", function, call);

	/* save current stack */
	asm("mov %0, sp" : "=r" (sp_save));
	//PDEBUG("%lx\n", sp_save);

	/* interrupt disable */
	local_irq_save(flags);

	/* set new stack in ocram */
	asm("mov sp, %0" : : "r" (sp_new));

	/* copy code blocks to ocram */
	memcpy((void*)OCRAM_RUN_CODE_START_VIRT, block_start, block_end-block_start);
	flush_cache_all();

	/* call ocram function */
	ret = (call)(param);

	/* restore saved stack */
	asm("mov sp, %0" : : "r" (sp_save));

	/* enable interrupt */
	local_irq_restore(flags);

	PDEBUG("@@@@@ Exit OCRAM Function @@@@@\n");
	return ret;
}

static void search_pvt (const unsigned pvt[][3], int pvt_size, unsigned clk, unsigned *vddd, unsigned *vddd_bo)
{
	int i;
	for (i = 0; i < pvt_size; i++) {
		if (clk <= pvt[i][0]) {
			*vddd    = max(*vddd, pvt[i][1]);
			*vddd_bo = max(*vddd_bo, pvt[i][2]);
			return;
		}
	}
	/* use maximum power if not found */
	*vddd    = max(*vddd, pvt[pvt_size-1][1]);
	*vddd_bo = max(*vddd_bo,pvt[pvt_size-1][2]);
}

static void adjust_vddd (pm_mode_t *pm)
{
	unsigned vddd = 0, vddd_bo = 0;

	/* set vddd & vdd_bo forcefully? */
	/* !!!CAUTION!!! do not check validity here */
	if (pm->vddd || pm->vddd_bo)
		return;

	search_pvt(pvt_pclk, ARRAY_SIZE(pvt_pclk), pm->pclk, &vddd, &vddd_bo);
	search_pvt(pvt_hclk, ARRAY_SIZE(pvt_hclk), pm->pclk/pm->hclkdiv, &vddd, &vddd_bo);
	search_pvt(pvt_emiclk, ARRAY_SIZE(pvt_emiclk), pm->emiclk, &vddd, &vddd_bo);

	/* update pm data*/
	pm->vddd = vddd;
	pm->vddd_bo = vddd_bo;
}

static void adjust_vdda (pm_mode_t *pm)
{
	/* !!!CAUTION!!! do not check validity here */
	if (pm->vdda || pm->vdda_bo)
		return;

	pm->vdda = VDDA_DEFAULT_MV;
	pm->vdda_bo = VDDA_DEFAULT_BO;
}

static void adjust_vddio (pm_mode_t *pm)
{
	/* !!!CAUTION!!! do not check validity here */
	if (pm->vddio || pm->vddio_bo)
		return;

	pm->vddio = VDDIO_DEFAULT_MV;
	pm->vddio_bo = VDDIO_SAFE_MIN_MV;
}

//20081231, to select enable/disable hclkslow
int Enable_HclkSlow(int select)
{
	if(select == true) {
		printk("<pm> Enable hclk-slow\n");
		enable_HclkSlow = true;
		
		//--------------------------------------------------------------------------
		// Enable interrupt wait to gate unused CPU clocks to save power.  
		//--------------------------------------------------------------------------
		hw_clkctrl_SetPclkInterruptWait(true);
		//--------------------------------------------------------------------------
		// Select conditions to use HCLK auto-slow.
		//--------------------------------------------------------------------------
		hw_clkctrl_EnableHclkModuleAutoSlow(APBHDMA_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(TRAFFIC_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(CPU_DATA_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(CPU_INSTR_AUTOSLOW,true);
		#if 1
		/* !!NOTE!! try disabling below two autoslow settings if problem occurs */
		hw_clkctrl_EnableHclkModuleAutoSlow(APBXDMA_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(TRAFFIC_JAM_AUTOSLOW,true);
		#endif
	
		//--------------------------------------------------------------------------
		// Set the HCLK auto-slow divider
		//--------------------------------------------------------------------------
		//hw_clocks_SetHclkAutoSlowDivider(pm.hclkslowdiv);
		//--------------------------------------------------------------------------
		// Enable HCLK auto-slow.
		//--------------------------------------------------------------------------
		hw_clocks_EnableHclkAutoSlow(true);
	}
	else if(select == false) {
		printk("<pm> Disable hclk-slow\n");
		enable_HclkSlow = false;
	
		//--------------------------------------------------------------------------
		// Disable interrupt wait to gate unused CPU clocks to save power.  
		//--------------------------------------------------------------------------
		hw_clkctrl_SetPclkInterruptWait(false);
		//--------------------------------------------------------------------------
		// Select conditions to use HCLK auto-slow.
		//--------------------------------------------------------------------------
		hw_clkctrl_EnableHclkModuleAutoSlow(APBHDMA_AUTOSLOW,false);
		hw_clkctrl_EnableHclkModuleAutoSlow(TRAFFIC_AUTOSLOW,false);
		hw_clkctrl_EnableHclkModuleAutoSlow(CPU_DATA_AUTOSLOW,false);
		hw_clkctrl_EnableHclkModuleAutoSlow(CPU_INSTR_AUTOSLOW,false);
		#if 1
		/* !!NOTE!! try disabling below two autoslow settings if problem occurs */
		hw_clkctrl_EnableHclkModuleAutoSlow(APBXDMA_AUTOSLOW,false);
		hw_clkctrl_EnableHclkModuleAutoSlow(TRAFFIC_JAM_AUTOSLOW,false);
		#endif
	
		//--------------------------------------------------------------------------
		// Set the HCLK auto-slow divider
		//--------------------------------------------------------------------------
		//hw_clocks_SetHclkAutoSlowDivider(pm.hclkslowdiv);
		//--------------------------------------------------------------------------
		// Enable HCLK auto-slow.
		//--------------------------------------------------------------------------
		hw_clocks_EnableHclkAutoSlow(false);
	}
}
EXPORT_SYMBOL(Enable_HclkSlow);



int set_pm_mode (int pm_target)
{
	pm_mode_t pm;
	unsigned pclk_cur, vddd_cur, vdda_cur, vddio_cur;
	unsigned old_cpu_clk = 0;
	unsigned new_cpu_clk = 0;
	bool bVdddChangeBeforeClk = false;
	bool bVdddChangeAfterClk = false;
	bool bVddaChangeBeforeClk = false;
	bool bVddaChangeAfterClk = false;
	bool bVddioChangeBeforeClk = false;
	bool bVddioChangeAfterClk = false;
	bool bXclkChangeBeforePclk;
	bool bEmiclkChangeBeforePclk;

	unsigned long flags;
	//unsigned int gpio_mux[8];
	//unsigned long *stmp37xx_addr;
	//unsigned int i = 0;
	//unsigned int aaa[25];
	
	spin_lock_irqsave(&pm_event_lock, flags);

	//printk("%d, HW_POWER_5VCTRL = 0x%08x\n", __LINE__, HW_POWER_5VCTRL_RD() );
#if 1
       	if (HW_POWER_STS.B.VDD5V_GT_VDDIO) { 
		//hw_power_SetVdddPowerSource(HW_POWER_LINREG_DCDC_READY);
		//hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_READY);
		//hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_READY);

		if (pm_target == SS_IDLE) {
			printk("Adapter or USB is connected\n\n");
			return 1;
			//hw_power_SetVdddPowerSource(HW_POWER_LINREG_DCDC_READY);
			//hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_READY);
			//hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_READY);
			//hw_power_EnableDcdc(false); //add dhsong
		}
		//else {
        	//	hw_power_SetVdddPowerSource(HW_POWER_DCDC_LINREG_READY);
		//        hw_power_SetVddaPowerSource(HW_POWER_DCDC_LINREG_READY);
		//        hw_power_SetVddioPowerSource(HW_POWER_DCDC_LINREG_ON);
		//	hw_power_EnableDcdc(true); //add dhsong
		//}
	}
//	else
//		hw_power_EnableDcdc(true); //add dhsong
#endif
	

	/* check power off */
	/*TODO: sync, dhsong*/
	if (pm_target == SS_POWER_OFF)
	{
		pr_info("[PM] power off\n");
		
		//ddi_rtc_Shutdown(true);
		//while (BF_RD(POWER_STS, PSWITCH) != 0);
		hw_power_PowerDown();
	}

	/* check parameter validity */
	if (pm_cur == pm_target) {
		//pr_info("[PM] same pm mode\n"); //disable dhsong.
		return 1;
	}
	if (pm_target > SS_MAX_PERF) {
		pr_info("[PM] pm mode out of range\n");
		return 1;
	}
	/* check if table is valid*/
	if (pm_table[pm_target].pclk == 0)
	{
		pr_info("[PM] undefined pm mode\n");
		return 1;
	}

	//loops_per_jiffy = 798720;

	//printk("loops_per_jiffy = %d\n\n", loops_per_jiffy);

	if( pm_table[pm_cur].pclk <= 480000)
		old_cpu_clk = pm_table[pm_cur].pclk / HZ;
	//else old_cpu_clk = 320000 / 1000;
	//printk("old_cpu_clk = %d\n\n", old_cpu_clk);
	new_cpu_clk = pm_table[pm_target].pclk / HZ;	
	//printk("new_cpu_clk = %d\n\n", new_cpu_clk);
	//printk("HZ = %d\n\n", HZ); //HZ = 100;

	//if(pm_target != SS_CLOCK_STABLE) {
		//pr_info("<PM> set pm_mode %s(%d)\n", pm_table[pm_target].name, pm_target);
	//}

	if(pm_target == SS_IDLE) { //add, send event before idle clock, dhsong
		pm_send_event(SS_PM_IDLE, SS_PM_ALL_DEV);
		mem_gpio_setting(); //for saving power consumption, dhsong
		//disable_irq(IRQ_BATT_BRNOUT);  
		disable_irq(IRQ_VDDD_BRNOUT);  
		disable_irq(IRQ_VDDIO_BRNOUT);  
	}
		
	/* ============================================================= */
	/*  1. get(copy) pm_table entry                                  */
	/* ============================================================= */
	pm = pm_table[pm_target];

	/* ============================================================= */
	/*  2. get(adjust) powers                                        */
	/*    pm->vddd, pm->vdda, pm->vddio is adjusted                  */
	/* ============================================================= */
	vdda_cur = ddi_power_GetVdda();
	adjust_vddd(&pm);
	if( vdda_cur < VDDA_DEFAULT_MV)
		adjust_vdda(&pm);
	adjust_vddio(&pm);

	/* ============================================================= */
	/*  3. check setting sequences depending on power/hclk changes   */
	/* ============================================================= */
	pclk_cur = ddi_clocks_GetPclk();
	vddd_cur = ddi_power_GetVddd();
	vdda_cur = ddi_power_GetVdda();
	vddio_cur = ddi_power_GetVddio();

	if (pm.vddd > vddd_cur || pm.pclk > 260000) 
		/* vddd increasing */
		bVdddChangeBeforeClk = true;
	else if (pm.vddd < vddd_cur)
		/* vddd decreasing */
		bVdddChangeAfterClk = true;

	if ( pm.vdda && (pm.vdda > vdda_cur) )
		/* vdda increasing */
		bVddaChangeBeforeClk = true;
	else if ( pm.vdda && (pm.vdda < vdda_cur) )
		/* vdda decreasing */
		bVddaChangeAfterClk = true;

	if (pm.vddio > vddio_cur)
		/* vddio increasing */
		bVddioChangeBeforeClk = true;
	else if (pm.vddio < vddio_cur)
		/* vddio decreasing */
		bVddioChangeAfterClk = true;

	if (pm.pclk >= pclk_cur) {
		/* pclk increasing or same */
		bXclkChangeBeforePclk = true;
		bEmiclkChangeBeforePclk = false;
	}
	else {
		/* pclk decreasing */
		bXclkChangeBeforePclk = false;
		bEmiclkChangeBeforePclk = true;
	}
#if 1
	/* ============================================================= */
	/*  4. set voltage before clock changing                         */
	/* ============================================================= */
			if (bVdddChangeBeforeClk)
				ddi_power_SetVddd(pm.vddd, pm.vddd_bo);

			if (bVddioChangeBeforeClk)
				ddi_power_SetVddio(pm.vddio, pm.vddio_bo);

			if (bVddaChangeBeforeClk)
				ddi_power_SetVdda(pm.vdda, pm.vdda_bo);

	///FIXME: dhsong
		//hw_power_SetVddaValue(DEFAULT_VDDA); //add dhsong for sound driver
			if (bVdddChangeBeforeClk)
				ddi_power_SetVddd(pm.vddd, pm.vddd_bo);

			if (bVddioChangeBeforeClk)
				ddi_power_SetVddio(pm.vddio, pm.vddio_bo);

			if (bVddaChangeBeforeClk)
				ddi_power_SetVdda(pm.vdda, pm.vdda_bo);
#endif
	/* ============================================================= */
	/*  5. set clocks                                                */
	/* ============================================================= */

	if (bXclkChangeBeforePclk)
		ddi_clocks_SetXclk(&pm.xclk);

	if (bEmiclkChangeBeforePclk) {
		/* update emiclock only when clock change is needed */
		if (pm_table[pm_cur].emiclk != pm.emiclk)
			EMI_CLOCK_CHANGE(pm.emiclk);
			//printk("<pm> emi clock change done\n");
	}

	ddi_clocks_SetGpmiClk(&pm.gpmiclk, true);
	ddi_clocks_SetPixClk(&pm.pixclk);
	if (pm.sspclk) {
		ddi_clocks_SetSspClk(&pm.sspclk, true);
	}
	if (pm.irclk || pm.irovclk) {
		ddi_clocks_SetIrClk(&pm.irclk, &pm.irovclk, true);
	}
	ddi_clocks_SetPclkHclk(&pm.pclk, pm.hclkdiv);

	//if(enable_HclkSlow == true) { //20081231, to select enable/disable hclkslow
	if(pm_table[pm_target].pclk < 260000) { //20081231, disable hclk-slow if  pclk is above 260MHz
		//printk("<pm> Enable hclk-slow\n");
		//--------------------------------------------------------------------------
		// Enable interrupt wait to gate unused CPU clocks to save power.  
		//--------------------------------------------------------------------------
		hw_clkctrl_SetPclkInterruptWait(pm.pclk_intr_wait);
		//--------------------------------------------------------------------------
		// Select conditions to use HCLK auto-slow.
		//--------------------------------------------------------------------------
		hw_clkctrl_EnableHclkModuleAutoSlow(APBHDMA_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(TRAFFIC_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(CPU_DATA_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(CPU_INSTR_AUTOSLOW,true);
		#if 1
		/* !!NOTE!! try disabling below two autoslow settings if problem occurs */
		hw_clkctrl_EnableHclkModuleAutoSlow(APBXDMA_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(TRAFFIC_JAM_AUTOSLOW,true);
		#endif
	
		//--------------------------------------------------------------------------
		// Set the HCLK auto-slow divider
		//--------------------------------------------------------------------------
		hw_clocks_SetHclkAutoSlowDivider(pm.hclkslowdiv);
		//--------------------------------------------------------------------------
		// Enable HCLK auto-slow.
		//--------------------------------------------------------------------------
		hw_clocks_EnableHclkAutoSlow(pm.hclkslowenable);
	} //if(Enable_HclkSlow == true)
	else {
		//printk("<pm> Disable hclk-slow\n");
		//--------------------------------------------------------------------------
		// Enable interrupt wait to gate unused CPU clocks to save power.  
		//--------------------------------------------------------------------------
		hw_clkctrl_SetPclkInterruptWait(false);
		//--------------------------------------------------------------------------
		// Select conditions to use HCLK auto-slow.
		//--------------------------------------------------------------------------
		hw_clkctrl_EnableHclkModuleAutoSlow(APBHDMA_AUTOSLOW,false);
		hw_clkctrl_EnableHclkModuleAutoSlow(TRAFFIC_AUTOSLOW,false);
		hw_clkctrl_EnableHclkModuleAutoSlow(CPU_DATA_AUTOSLOW,false);
		hw_clkctrl_EnableHclkModuleAutoSlow(CPU_INSTR_AUTOSLOW,false);
		#if 1
		/* !!NOTE!! try disabling below two autoslow settings if problem occurs */
		hw_clkctrl_EnableHclkModuleAutoSlow(APBXDMA_AUTOSLOW,false);
		hw_clkctrl_EnableHclkModuleAutoSlow(TRAFFIC_JAM_AUTOSLOW,false);
		#endif
	
		//--------------------------------------------------------------------------
		// Set the HCLK auto-slow divider
		//--------------------------------------------------------------------------
		hw_clocks_SetHclkAutoSlowDivider(0);
		//--------------------------------------------------------------------------
		// Enable HCLK auto-slow.
		//--------------------------------------------------------------------------
		hw_clocks_EnableHclkAutoSlow(false);
		// n.y.lee: should be removed
		//printk("<pm> Enable hclk-slow disabled\n");
	}


	if (!bXclkChangeBeforePclk) {
		// n.y.lee: should be removed
		//printk("<pm> setXclk\n");
		ddi_clocks_SetXclk(&pm.xclk);
		//printk("<pm> setXclk done\n");
	}
	if (!bEmiclkChangeBeforePclk) {
		/* update emiclock only when clock change is needed */
		if (pm_table[pm_cur].emiclk != pm.emiclk) {
			// n.y.lee: should be removed
			//printk("<pm> emi clock change\n");
			EMI_CLOCK_CHANGE(pm.emiclk);
			//printk("<pm> emi clock change done\n");
		}
	}

	/* !!hack!! to make ddi_clocks driver happy */
	//printk("<pm> clock using pll\n");
	ddi_clocks_ClockUsingPll(EMICLK, (pm.emiclk > MIN_PLL_KHZ));
	//printk("<pm> clock using pll done\n");
	//printk("<pm> power off pll\n");
	ddi_clocks_PowerOffPll();
	//printk("<pm> power off pll done\n");
	/* !!hack!! */
#if 1
	/* ============================================================= */
	/*  6. set voltage after clock changing                          */
	/* ============================================================= */
	// n.y.lee: should be removed
	//printk("<pm> set voltage after clock changing\n");

			if (bVdddChangeAfterClk)
				ddi_power_SetVddd(pm.vddd, pm.vddd_bo);

			if (bVddioChangeAfterClk)
				ddi_power_SetVddio(pm.vddio, pm.vddio_bo);

			if (bVddaChangeAfterClk)
				ddi_power_SetVdda(pm.vdda, pm.vdda_bo);

	///FIXME: dhsong
		//hw_power_SetVddaValue(DEFAULT_VDDA); //add dhsong for sound driver
			if (bVdddChangeAfterClk)
				ddi_power_SetVddd(pm.vddd, pm.vddd_bo);

			if (bVddioChangeAfterClk)
				ddi_power_SetVddio(pm.vddio, pm.vddio_bo);

			if (bVddaChangeAfterClk)
				ddi_power_SetVdda(pm.vdda, pm.vdda_bo);
#endif

	pm_cur = pm_target;
#ifdef DEBUG
	/* print out cur clock & power  */
	{
		char status[1024];
		print_pm_info(status, sizeof(status));
		PDEBUG(status);
	}
#endif

	if (pm_target == SS_IDLE) {
		// n.y.lee: should be removed
		//printk("<pm> target idle\n");
		//pr_info("[PM] power idle\n");
		return pm_enter_idle();
	}
#if 0
       	if (HW_POWER_STS.B.VDD5V_GT_VDDIO) { 
		//hw_power_SetVdddPowerSource(HW_POWER_LINREG_DCDC_READY);
		//hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_READY);
		//hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_READY);

		if (pm_target == SS_POWER_ON) {
			//hw_power_SetVdddPowerSource(HW_POWER_LINREG_DCDC_READY);
			//hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_READY);
			//hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_READY);
			hw_power_EnableDcdc(true); //add dhsong
		}
		else {
        	//	hw_power_SetVdddPowerSource(HW_POWER_DCDC_LINREG_READY);
		//      hw_power_SetVddaPowerSource(HW_POWER_DCDC_LINREG_READY);
		//        hw_power_SetVddioPowerSource(HW_POWER_DCDC_LINREG_ON);
			hw_power_EnableDcdc(false); //add dhsong
		}
	}
#endif
	loops_per_jiffy = (lpj * new_cpu_clk) / (MAX_CLOCK/HZ);
	//loops_per_jiffy = (loops_per_jiffy * new_cpu_clk) / old_cpu_clk; 

	//loops_per_jiffy = lpj/(new_cpu_clk/HZ);

	//loops_per_jiffy = (loops_per_jiffy * new_cpu_clk) / 320;

	//if(pm_target != SS_IDLE)
	//	loops_per_jiffy = (loops_per_jiffy * new_cpu_clk) / old_cpu_clk; 
	//printk("loops_per_jiffy = %d\n\n", loops_per_jiffy);

	//printk("%d, HW_POWER_5VCTRL = 0x%08x\n", __LINE__, HW_POWER_5VCTRL_RD() ); //add dhsong
	// n.y.lee: should be removed
	//printk("<pm> done with loop jiffy = %d\n", loops_per_jiffy);

	spin_unlock_irqrestore(&pm_event_lock, flags);

	return 0;
}
//EXPORT_SYMBOL(set_pm_mode);

int set_pm_mode_stable( int pm_opm_state )
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&pm_event_lock, flags);
	
	//if( pm_table[pm_opm_state].pclk > 24000 ) { //if pclk > 24M	
	//if( pm_table[pm_cur].pclk == 24000 && pm_table[pm_opm_state].pclk > 24000 ) {  	
	if( ( (pm_table[pm_cur].pclk <= pm_table[SS_CLOCK_STABLE].pclk) &&  //(if cur pclk <= 80M && if cur emiclk <=80M) &&
	    (pm_table[pm_cur].emiclk <= pm_table[SS_CLOCK_STABLE].emiclk) )&& 
	    ( (pm_table[pm_opm_state].pclk > pm_table[SS_CLOCK_STABLE].pclk) || //(if new pclk > 80M || if new emiclk > 80M)
	    (pm_table[pm_opm_state].emiclk > pm_table[SS_CLOCK_STABLE].emiclk) ) ) { 
		
		ret = set_pm_mode(SS_CLOCK_STABLE);
		/* FIXME:: By leeth, for more stable clock changing at 20090518 */
		ret = set_pm_mode(SS_CLOCK_LEVEL9);
		ret = set_pm_mode(SS_CLOCK_LEVEL11);
	}

	ret = set_pm_mode(pm_opm_state);

	spin_unlock_irqrestore(&pm_event_lock, flags);

	return ret;
}


int stmp37xx_mdelay_func(int sec) 
{     
        uint32_t cur_time=0;
        uint32_t end_time=0;
        uint32_t result=0;
 
        cur_time = HW_RTC_MILLISECONDS_RD(); //HW_DIGCTL_MICROSECONDS_RD();
     
        while(true) {
                end_time = HW_RTC_MILLISECONDS_RD(); //HW_DIGCTL_MICROSECONDS_RD();
		if(end_time < cur_time) {
			result = (end_time + 0xFFFFFFFF) - cur_time; //modify 20090219 
		} 
		else result = end_time - cur_time;

                if ( result > sec) 
                        break;
        }    
        return 1;
}
EXPORT_SYMBOL(stmp37xx_mdelay_func);
 
int stmp37xx_udelay_func(int sec) 
{     
        uint32_t cur_time=0;
        uint32_t end_time=0;
        uint32_t result=0;
 
        cur_time = HW_DIGCTL_MICROSECONDS_RD();
     
        while(true) {
                end_time = HW_DIGCTL_MICROSECONDS_RD();
		if(end_time < cur_time) {
			result = (end_time + 0xFFFFFFFF) - cur_time; //modify 20090219 
		} 
		else result = end_time - cur_time;

                if ( result > sec) 
                        break;
        }    
        return 1;
}
EXPORT_SYMBOL(stmp37xx_udelay_func);

static int set_gpio_idle()
{
	unsigned long default_values = 0xffffffff;
	unsigned long set_values;

#if 1
        //all gpio pin set gpio func
	//set_values = default_values & ~(0x1<<31) & ~(0x1<<30) & ~(0x1<<29) & ~(0x1<<28); //to except specific bit
        HW_PINCTRL_MUXSEL0_SET(default_values);
        HW_PINCTRL_MUXSEL1_SET(default_values);
        HW_PINCTRL_MUXSEL2_SET(default_values);
        HW_PINCTRL_MUXSEL3_SET(default_values);
        //HW_PINCTRL_MUXSEL4_SET(0xffffffff);
        //HW_PINCTRL_MUXSEL5_SET(0xffffffff);
        //HW_PINCTRL_MUXSEL6_SET(0xffffffff);
        //HW_PINCTRL_MUXSEL7_SET(0xffffffff);

	//printk("HW_PINCTRL_MUXSEL0_RD = 0x%08x\n\n", HW_PINCTRL_MUXSEL0_RD() );
	//printk("HW_PINCTRL_MUXSEL1_RD = 0x%08x\n\n", HW_PINCTRL_MUXSEL1_RD() );
	//printk("HW_PINCTRL_MUXSEL2_RD = 0x%08x\n\n", HW_PINCTRL_MUXSEL2_RD() );
	//printk("HW_PINCTRL_MUXSEL3_RD = 0x%08x\n\n", HW_PINCTRL_MUXSEL3_RD() );
#endif

	HW_PINCTRL_DOE0_CLR(0xffffffff);
	HW_PINCTRL_DOE1_CLR(0xffffffff);
	HW_PINCTRL_DOE2_CLR(0xffffffff);

#if 1
	//all gpio drive pin set zero
	HW_PINCTRL_DRIVE0_CLR(0xffffffff);
	HW_PINCTRL_DRIVE1_CLR(0xffffffff);
	HW_PINCTRL_DRIVE2_CLR(0xffffffff);
	HW_PINCTRL_DRIVE3_CLR(0xffffffff);
	HW_PINCTRL_DRIVE4_CLR(0xffffffff);
	HW_PINCTRL_DRIVE5_CLR(0xffffffff);
	HW_PINCTRL_DRIVE6_CLR(0xffffffff);
	HW_PINCTRL_DRIVE7_CLR(0xffffffff);
	HW_PINCTRL_DRIVE8_CLR(0xffffffff);
	HW_PINCTRL_DRIVE9_CLR(0xffffffff);
	HW_PINCTRL_DRIVE10_CLR(0xffffffff);
	HW_PINCTRL_DRIVE11_CLR(0xffffffff);
	HW_PINCTRL_DRIVE12_CLR(0xffffffff);
	HW_PINCTRL_DRIVE13_CLR(0xffffffff);
	HW_PINCTRL_DRIVE14_CLR(0xffffffff);
#endif

	return 0;
}

static int set_gpio_wakeup()
{
	unsigned int i = 0;
	unsigned long *test_addr;

	//for setting gpio drive pin 
	for(i = 0; i < 15; i++) { //for gpio drive setting
		if(i == 0)
			test_addr = &HW_PINCTRL_DRIVE0; 
		
		//pinctrl_drive[i] = *test_addr;
		*test_addr = pinctrl_drive[i];

		//printk("test_addr = 0x%08x\n", test_addr );
		//printk("pinctrl_drive[%d] = 0x%08x\n", i, pinctrl_drive[i] );

		test_addr += 0x4; 
	}
#if 1
	//for setting gpio func pin
	for(i = 0; i < 8; i++) { 
		if(i == 0)
			test_addr = &HW_PINCTRL_MUXSEL0; 
		
		//pinctrl_muxsel[i] = *test_addr;
		*test_addr = pinctrl_muxsel[i];

		//printk("test_addr = 0x%08x\n", test_addr );
		//printk("pinctrl_muxsel[%d] = 0x%08x\n", i, pinctrl_muxsel[i] );

		test_addr += 0x4; 

		//sscanf(&i, "HW_PINCTRL_MUXSEL%d", stmp37xx_addr);
		//printk("stmp37xx_addr = 0x%08x\n", *stmp37xx_addr );
		//pinctrl_muxsel[i] = *stmp37xx_addr; 	
		//printk("HW_PINCTRL_MUXSEL0_RD = 0x%08x\n\n", HW_PINCTRL_MUXSEL0_RD() );
	}
#endif

	//for setting gpio direction pin 
	for(i = 0; i < 3; i++) { //for gpio drive setting
		if(i == 0)
			test_addr = &HW_PINCTRL_DOE0; 
		
		//pinctrl_doe[i] = *test_addr;
		*test_addr = pinctrl_doe[i];

		//printk("test_addr = 0x%08x\n", test_addr );
		//printk("pinctrl_doe[%d] = 0x%08x\n", i, pinctrl_doe[i] );

		test_addr += 0x4; 
 #if 0
		if(i==0)
			printk("HW_PINCTRL_DOE0 = 0x%08x\n", HW_PINCTRL_DOE0 );
		if(i==1)
			printk("HW_PINCTRL_DOE1 = 0x%08x\n", HW_PINCTRL_DOE1 );
		if(i==2)
			printk("HW_PINCTRL_DOE2 = 0x%08x\n", HW_PINCTRL_DOE2 );
 #endif
	}

	return 0;
}

static int mem_gpio_setting()
{
	unsigned int i = 0;
	unsigned long *test_addr;
	//unsigned long *stmp37xx_addr;

	//for memory gpio func pin
	for(i = 0; i < 8; i++) { 
 #if 0
		if(i==0)
			printk("HW_PINCTRL_MUXSEL0= 0x%08x\n", HW_PINCTRL_MUXSEL0_RD() );
		if(i==1)
			printk("HW_PINCTRL_MUXSEL1= 0x%08x\n", HW_PINCTRL_MUXSEL1_RD() );
		if(i==2)
			printk("HW_PINCTRL_MUXSEL2= 0x%08x\n", HW_PINCTRL_MUXSEL2_RD() );
		if(i==3)
			printk("HW_PINCTRL_MUXSEL3= 0x%08x\n", HW_PINCTRL_MUXSEL3_RD() );
		if(i==4)
			printk("HW_PINCTRL_MUXSEL4= 0x%08x\n", HW_PINCTRL_MUXSEL4_RD() );
		if(i==5)
			printk("HW_PINCTRL_MUXSEL5= 0x%08x\n", HW_PINCTRL_MUXSEL5_RD() );
		if(i==6)
			printk("HW_PINCTRL_MUXSEL6= 0x%08x\n", HW_PINCTRL_MUXSEL6_RD() );
		if(i==7)
			printk("HW_PINCTRL_MUXSEL7= 0x%08x\n", HW_PINCTRL_MUXSEL7_RD() );
 #endif		
		if(i == 0)
			test_addr = &HW_PINCTRL_MUXSEL0; 
		
		pinctrl_muxsel[i] = *test_addr;

		//printk("test_addr = 0x%08x\n", test_addr );
		//printk("pinctrl_muxsel[%d] = 0x%08x\n", i, pinctrl_muxsel[i] );

		test_addr += 0x4; 

		//sscanf(&i, "HW_PINCTRL_MUXSEL%d", stmp37xx_addr);
		//printk("stmp37xx_addr = 0x%08x\n", *stmp37xx_addr );
		//pinctrl_muxsel[i] = *stmp37xx_addr; 	
		//printk("HW_PINCTRL_MUXSEL0_RD = 0x%08x\n\n", HW_PINCTRL_MUXSEL0_RD() );
	}

	//for memory gpio direction pin 
	for(i = 0; i < 3; i++) { //for gpio drive setting
 #if 0
		if(i==0)
			printk("HW_PINCTRL_DOE0 = 0x%08x\n", HW_PINCTRL_DOE0 );
		if(i==1)
			printk("HW_PINCTRL_DOE1 = 0x%08x\n", HW_PINCTRL_DOE1 );
		if(i==2)
			printk("HW_PINCTRL_DOE2 = 0x%08x\n", HW_PINCTRL_DOE2 );
 #endif

		if(i == 0)
			test_addr = &HW_PINCTRL_DOE0; 
		
		pinctrl_doe[i] = *test_addr;

		//printk("test_addr = 0x%08x\n", test_addr );
		//printk("pinctrl_doe[%d] = 0x%08x\n", i, pinctrl_doe[i] );

		test_addr += 0x4; 
	}

	//for memory gpio drive pin 
	for(i = 0; i < 15; i++) { //for gpio drive setting
		if(i == 0)
			test_addr = &HW_PINCTRL_DRIVE0; 
		
		pinctrl_drive[i] = *test_addr;

		//printk("test_addr = 0x%08x\n", test_addr );
		//printk("pinctrl_drive[%d] = 0x%08x\n", i, pinctrl_drive[i] );

		test_addr += 0x4; 
	}

	return 0;
}

static int pm_enter_idle (void)
{
	unsigned int start_time = 0;
	unsigned int end_time = 0;
	
	//pm_send_event(SS_PM_IDLE, SS_PM_ALL_DEV); //disable, dhsong

        //HW_LCDIF_CTRL1_CLR(0x1); //lcd reset low
        //printk("HW_LCDIF_CTRL1=0x%08x\n", HW_LCDIF_CTRL1);

#ifdef LOW_ANAL_CUR
	// lower the analog current
	BF_CLR(AUDIOIN_CTRL,SFTRST);
	BF_CLR(AUDIOIN_CTRL,CLKGATE);
	
	BF_CLR(AUDIOOUT_CTRL,SFTRST);
	BF_CLR(AUDIOOUT_CTRL,CLKGATE);
	
	BF_SET(AUDIOOUT_REFCTRL, XTAL_BGR_BIAS);
	HW_AUDIOOUT_REFCTRL.B.BIAS_CTRL = 1;
	BF_SET(AUDIOOUT_REFCTRL, LOW_PWR);
	
	BF_SET(AUDIOIN_CTRL,CLKGATE);
	BF_SET(AUDIOOUT_CTRL,CLKGATE);   
	
	while(HW_AUDIOOUT_CTRL.B.CLKGATE==0);    
#endif	
	// USB clock dance.  This doesn't appear to lower
	// power at all.  Maybe ROM already performs this
	// dance.
	BF_CS2(USBPHY_CTRL,SFTRST,0,CLKGATE,0);
	BF_CS1(USBPHY_CTRL,CLKGATE,1);

	//if(HW_POWER_STS.B.VDD5V_GT_VDDIO)
	//{
	//	BF_CS1(POWER_MINPWR,USB_I_SUSPEND,1);
	//	BF_CS1(POWER_MINPWR,VBG_OFF,1);
	//	BF_CS1(POWER_MINPWR,DC_STOPCLK,1);
	//	BF_CS1(POWER_VDDDCTRL,DISABLE_STEPPING,0x1);
	//}
	//else
	{

#ifdef LOW_ANAL_CUR
		// lower the analog current
		BF_CLR(AUDIOIN_CTRL,SFTRST);
		BF_CLR(AUDIOIN_CTRL,CLKGATE);
		
		BF_CLR(AUDIOOUT_CTRL,SFTRST);
		BF_CLR(AUDIOOUT_CTRL,CLKGATE);
		BF_SET(AUDIOOUT_REFCTRL, VDDXTAL_TO_VDDD);
		BF_SET(AUDIOOUT_REFCTRL, XTAL_BGR_BIAS);
		BF_WR(AUDIOOUT_REFCTRL, BIAS_CTRL, 1);
		BF_SET(AUDIOOUT_REFCTRL, LOW_PWR);
		BF_SET(AUDIOIN_CTRL,CLKGATE);
		BF_SET(AUDIOOUT_CTRL,CLKGATE);
#endif		
		// Optimize the gain settings
		HW_POWER_LOOPCTRL.B.EN_RCSCALE = 0; // might be worth 5-10uA??
		HW_POWER_LOOPCTRL.B.DC_R = 2;
		
		// half the fets
		BF_SET(POWER_MINPWR, HALF_FETS);
		
#if 1 //if 0, disable PFM setting, dhsong
		// Not sure why but these functionalities
		// cause major inefficiencies in PFM mode
		BF_CLR(POWER_LOOPCTRL, CM_HYST_THRESH);
		BF_CLR(POWER_LOOPCTRL, EN_CM_HYST);
		BF_CLR(POWER_LOOPCTRL, EN_DF_HYST);
		
		// enable PFM
		BW_POWER_LOOPCTRL_HYST_SIGN(1);
		BF_SET(POWER_MINPWR,EN_DC_PFM);
#endif
	}

	//BF_SET(POWER_MINPWR, DC_HALFCLK); //set PWM or PFM = 750KHz, dhsong

	//BW_CLKCTRL_XTAL_UART_CLK_GATE(1);

	//set_gpio_idle(); //add for saving power consumption, dhsong

	pm_run_inocram(emi_block_startaddress, emi_block_endddress, stmp37xx_enter_idle, 0);

	//set_gpio_wakeup(); //add for saving power consumption, dhsong

	//BW_CLKCTRL_XTAL_UART_CLK_GATE(0);

	//if (HW_POWER_STS.B.VDD5V_GT_VDDIO)
	//{
#ifdef LOW_ANAL_CUR
	//	BF_CLR(AUDIOOUT_REFCTRL, XTAL_BGR_BIAS);
	//	HW_AUDIOOUT_REFCTRL.B.BIAS_CTRL = 0;
	//	BF_CLR(AUDIOOUT_REFCTRL, LOW_PWR);
#endif
	//	BF_CS1(POWER_MINPWR,DC_STOPCLK,0);
	//	BF_CS1(POWER_MINPWR,VBG_OFF,0);
	//	BF_CS1(POWER_MINPWR,USB_I_SUSPEND,0);
	//}
	//else
	{
#if 1 //if 0, disable PFM setting, dhsong
		// disable PFM
		BF_CLR(POWER_MINPWR,EN_DC_PFM);
		BW_POWER_LOOPCTRL_HYST_SIGN(0);
		
		// Not sure why but these functionalities
		// cause major inefficiencies in PFM mode
		BF_SET(POWER_LOOPCTRL, EN_CM_HYST);
		BF_SET(POWER_LOOPCTRL, EN_DF_HYST);
		BF_CLR(POWER_LOOPCTRL, CM_HYST_THRESH);
#endif

		// Optimize the gain settings
		HW_POWER_LOOPCTRL.B.EN_RCSCALE = 3;
		HW_POWER_LOOPCTRL.B.DC_R = 2;
	}
	//BF_CLR(POWER_MINPWR, DC_HALFCLK); //set PWM or PFM = 1.5MHz, dhsong

        ddi_clocks_SetGpmiClk(&pm_table[SS_CLOCK_LEVEL8].gpmiclk, true);
        ddi_clocks_SetPclkHclk(&pm_table[SS_CLOCK_LEVEL8].pclk, 1);

	//start_time = HW_DIGCTL_MICROSECONDS_RD(); 	
	pm_send_event(SS_PM_SET_WAKEUP, SS_PM_ALL_DEV);
	//end_time = HW_DIGCTL_MICROSECONDS_RD(); 	
	//printk("time = %d uA\n", end_time - start_time);

	//delay_func_2();

	//printk("HW_POWER_STS.B.PSWITCH = 0x%08x\n", HW_POWER_STS.B.PSWITCH);
        if (HW_POWER_STS.B.PSWITCH == 0 && HW_POWER_STS.B.VDD5V_GT_VDDIO==0) { //if pswitch = 0 and 5V is not inserted 
		ddi_clocks_SetGpmiClk(&pm_table[SS_IDLE].gpmiclk, true);
                ddi_clocks_SetPclkHclk(&pm_table[SS_IDLE].pclk, 1);
		pm_send_event(SS_PM_IDLE, SS_PM_ALL_DEV);
                //printk("enter idle mode again\n");
		return pm_enter_idle();
	}
	else { 
#if 0
        	if (HW_POWER_STS.B.VDD5V_GT_VDDIO) {
			hw_power_EnableDcdc(false);
		}
		else 
			hw_power_EnableDcdc(true);
#endif
		//enable_irq(IRQ_BATT_BRNOUT);  
		enable_irq(IRQ_VDDD_BRNOUT);  
		enable_irq(IRQ_VDDIO_BRNOUT);  
		return set_pm_mode_stable(SS_POWER_ON); //disable, app set power-on mode, dhsong 
	}

	return 0;
}


#ifdef CONFIG_PROC_FS
static int proc_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *buf = page;
	char *next = buf;
	unsigned size = count;
	int t;

	if (off != 0)
		return 0;

	/* print out current clock & power  */
	t = print_pm_info(next, size);

	size -= t;
	next += t;

	*eof = 1;
	return count - size;
}

static ssize_t proc_write (struct file * file, const char * buf, unsigned long count, void *data)
{
	char cmd[64];
	unsigned value1, value2;
		unsigned long flags;

	sscanf(buf, "%s %u %u", cmd, &value1, &value2);
		spin_lock_irqsave(&pm_event_lock, flags);

	if (!strcmp(cmd, "opm_level")) {
		if (set_pm_mode_stable(value1))
			return count;
	}
	else if (!strcmp(cmd, "pclk")) {
		unsigned hclkdiv;
		hw_clkctrl_GetHclkDiv(&hclkdiv);
		ddi_clocks_SetPclkHclk(&value1, hclkdiv);
	}
	else if(!strcmp(cmd, "hclkdiv")) {
		unsigned pclk = ddi_clocks_GetPclk();
		ddi_clocks_SetPclkHclk(&pclk, value1);
	}
	else if(!strcmp(cmd, "emiclk")) {
		EMI_CLOCK_CHANGE(value1);
	}
	else if (!strcmp(cmd, "gpmiclk")) {
		ddi_clocks_SetGpmiClk(&value1, 1);
	}
	else if (!strcmp(cmd, "xclk")) {
		ddi_clocks_SetXclk(&value1);
	}
	else if (!strcmp(cmd, "pixclk")) {
		ddi_clocks_SetPixClk(&value1);
	}
	else if (!strcmp(cmd, "vddd")) {
		ddi_power_SetVddd(value1, value2);
	}
	else if (!strcmp(cmd, "vdda")) {
		ddi_power_SetVdda(value1, value2);
	}
	else if (!strcmp(cmd, "vddio")) {
		ddi_power_SetVddio(value1, value2);
	}
#if 0
	//20081231, to select enable/disable hclkslow
	else if (!strcmp(cmd, "hclk_slow")) {
		Enable_HclkSlow(value1);
	}
#endif
	        spin_unlock_irqrestore(&pm_event_lock, flags);

#ifdef DEBUG
	{
		char status[1024];
		/* print out current clock & power  */
		print_pm_info(status, sizeof(status));
		PDEBUG(status);
	}
#endif
	return count; 
}
#endif

static int pm_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;

	switch (cmd) {
	case OPM_LEVEL:
		ret = set_pm_mode_stable(arg);
		break;
	}
	return ret;
}

int ss_pm_register (ss_pm_dev_t type, stmp37xx_pm_callback callback)
{
	struct stmp37xx_pm_dev *dev = kzalloc(sizeof(struct stmp37xx_pm_dev), GFP_KERNEL);
	if (dev) {
		unsigned long flags;
		dev->callback = callback;

		spin_lock_irqsave(&pm_event_lock, flags);

		list_add(&dev->entry, &pm_list);
		dev->type = type;
		PDEBUG("dev->type = %d, type=%d\n\n",dev->type, type); 
	        spin_unlock_irqrestore(&pm_event_lock, flags);
		return 0;
	}
	return 1;
}
EXPORT_SYMBOL(ss_pm_register);

//int pm_send_event (int event, void *data)
int pm_send_event (ss_pm_request_t event, ss_pm_dev_t dev_name)
{
	struct list_head *entry;
	unsigned long flags;
#if 0
	if(prev_event != event)
		prev_event = event;
	else
		return 0; 
#endif
	PDEBUG("PM_send_event %d\n", event);

	//spin_lock_irqsave(&pm_event_lock, flags);

	if(dev_name == SS_PM_BUT_DEV) {
		//printk("send event to only Nand flash driver \n\n\n");
		entry = pm_list.next;
		while (entry != &pm_list) {
			struct stmp37xx_pm_dev *dev = list_entry(entry, struct stmp37xx_pm_dev, entry);
			if (dev->callback) {
				if (dev->type == dev_name) {
					PDEBUG(" %p\n", dev->callback);
					(*dev->callback)(event);
				}
			}
			entry = entry->next;
		}
	}

 	else if(dev_name == SS_PM_ALL_DEV) { //send event to all device
		entry = pm_list.next;
		while (entry != &pm_list) {
			struct stmp37xx_pm_dev *dev = list_entry(entry, struct stmp37xx_pm_dev, entry);
			if (dev->callback) {
				//if (dev->type != SS_PM_SND_DEV) {
					PDEBUG(" %p\n", dev->callback);
					(*dev->callback)(event);
				//}
			}
			entry = entry->next;
		}
	}

	//spin_unlock_irqrestore(&pm_event_lock, flags);

	return 0;
}
EXPORT_SYMBOL(pm_send_event);


static irqreturn_t bo_interrupt (int irq_num, void* dev_idp)
{
	if (irq_num == IRQ_BATT_BRNOUT) {
		if (!hw_power_Get5vPresentFlag()) { //if 5V is not inserted
			pr_info("BATT B/O\n");
			hw_power_ClearBatteryBrownoutInterrupt();
			hw_power_PowerDown();
		}
	}
	else {
		uint32_t  u32BoStsBits;

		//----------------------------------------------------------------------
		// Loop for specified time to debounce the brownout status bits.
		//----------------------------------------------------------------------    
		udelay(10);
		u32BoStsBits = HW_POWER_STS_RD();

		if (u32BoStsBits & (BM_POWER_STS_VDDD_BO | BM_POWER_STS_VDDA_BO | BM_POWER_STS_VDDIO_BO))
		{
			//----------------------------------------------------------------------
			// If control arrives here, either VDDD, VDDA, or VDDIO are drooping. 
			// There's no telling what this is, but it could be something like a 
			// short circuit. We need to shut down.
			//
			// Construct the bit pattern that unlocks access to the power reset
			// register. All writes to this register must include this bit pattern.
			// Any write that doesn't include this bit pattern is ignored.
			//----------------------------------------------------------------------
			if (!hw_power_Get5vPresentFlag()) { ////if 5V is not inserted, dhsong
				pr_info("%s B/O interrupt\n", (char*)dev_idp);
#if 1 // add dhsong	
				hw_power_ClearVdddBrownoutInterrupt();
				hw_power_ClearVddaBrownoutInterrupt();
				hw_power_ClearVddioBrownoutInterrupt();
#endif
				/* HOTFIX:: by leeth, except for VDDA, enable Power rail brownout at 20090721 */
				if(u32BoStsBits & (BM_POWER_STS_VDDD_BO | BM_POWER_STS_VDDIO_BO))
					hw_power_PowerDown();
			}
		}
		else
		{
			//----------------------------------------------------------------------
			// This was a false brownout.  We need to clear the brownout IRQ bits
			// and return.  
			//----------------------------------------------------------------------
			hw_power_ClearVdddBrownoutInterrupt();
			hw_power_ClearVddaBrownoutInterrupt();
			hw_power_ClearVddioBrownoutInterrupt();
		}
	}
	return IRQ_HANDLED;
}

/* HOTFIX:: by leeth, except for VDDA, enable Power rail brownout at 20090721 */
extern int is_good_battery(void);

static void handoff_timer (unsigned long data)
{
	/* check 5v flag again since 5v may be off at the time*/
	if (hw_power_Get5vPresentFlag()) {
		PDEBUG("DCDC->VDD5V\n");
		
		ddi_power_Validate5vPowerSource(true);

	//HW_POWER_5VCTRL_CLR(0x1<<2); //always linreg vddio enable, dhsong
        //HW_POWER_VDDDCTRL_SET(0x1<<21); //always linreg vddd enable, dhsong 
        //HW_POWER_VDDDCTRL_SET(0x1<<22); //always linreg vddd enable, dhsong 
        //HW_POWER_VDDACTRL_SET(0x1<<17); //always linreg vdda enable, dhsong

		//PDEBUG("%d, HW_POWER_5VCTRL = 0x%08x\n", __LINE__, HW_POWER_5VCTRL_RD() );
		//PDEBUG("%d, HW_POWER_VDDIOCTRL = 0x%08x\n", __LINE__, HW_POWER_VDDIOCTRL_RD() );
		//PDEBUG("%d, HW_POWER_VDDDCTRL = 0x%08x\n", __LINE__, HW_POWER_VDDDCTRL_RD() );
		//PDEBUG("%d, HW_POWER_VDDACTRL = 0x%08x\n", __LINE__, HW_POWER_VDDACTRL_RD() );
	}
	else {
		/* in this case, disconnect event is missing */
		/* send disconnect event */
		//pm_send_event(SS_PM_USB_REMOVED, SS_PM_ALL_DEV);

		ddi_power_Validate5vPowerSource(false);

	//HW_POWER_5VCTRL_SET(0x1<<2); //always linreg vddio enable, dhsong
        //HW_POWER_VDDDCTRL_CLR(0x1<<21); //always linreg vddd enable, dhsong 
        //HW_POWER_VDDDCTRL_SET(0x1<<22); //always linreg vddd enable, dhsong 
        //HW_POWER_VDDACTRL_CLR(0x1<<17); //always linreg vdda enable, dhsong

		//PDEBUG("%d, HW_POWER_5VCTRL = 0x%08x\n", __LINE__, HW_POWER_5VCTRL_RD() );
	}
	ddi_power_Handoff(is_good_battery());
}

bool USB_Adapter_IsConnected (void)
{
	bool bRtn = -1;
	bool bClockWasGated= false;
 
	// Check if the PHY's clock is gated. When we're done, we'll want to put it
	// back the way we found it.
	bClockWasGated = HW_USBPHY_CTRL.B.CLKGATE;
 
	// Clear the soft reset
	HW_USBPHY_CTRL_CLR(BM_USBPHY_CTRL_SFTRST);
 
	// Ungate the PHY's clock.
	HW_USBPHY_CTRL_CLR(BM_USBPHY_CTRL_CLKGATE);
 
	// Override ARC control of the 15k pulldowns.
	HW_USBPHY_DEBUG_SET(BM_USBPHY_DEBUG_ENHSTPULLDOWN);
 
	// Disable the 15k pulldowns.
	// Necessary in device mode; the host/hub will have the 15k pulldowns.
	HW_USBPHY_DEBUG_CLR(BM_USBPHY_DEBUG_HSTPULLDOWN);
 
	// Enable the 200k pullups for host detection.
	HW_USBPHY_CTRL_SET(BM_USBPHY_CTRL_ENDEVPLUGINDETECT);
 
	//#warning hw_usbphy_IsConnected - wait for pullup settle delay is NOT simulated.  Verify usec counter!
	//udelay(250);
	udelay(300);
	//mdelay(10);
	// Check for connectivity.
	bRtn = (HW_USBPHY_STATUS.B.DEVPLUGIN_STATUS ? true : false );
#if 0
	printk("<pm> USB_DEVPLUGIN_STATUS = %d\n", HW_USBPHY_STATUS.B.DEVPLUGIN_STATUS);
#else
	udelay(1000);	
#endif
 
	// Disable the 200k pullups.
	HW_USBPHY_CTRL_CLR(BM_USBPHY_CTRL_ENDEVPLUGINDETECT);
 
	// Re-gate the clock, if necessary.
	if(bClockWasGated)
	{
	    HW_USBPHY_CTRL_SET(BM_USBPHY_CTRL_CLKGATE);
	}
	// Return connection status.
	return bRtn;
}

int is_USB_connected(void)
{
	//printk("is_USB_connected, usb_or_adapter = %d\n\n", usb_or_adapter);

	if(usb_or_adapter == USB_PM)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(is_USB_connected);

int is_adapter_connected(void)
{
	//printk("is_adapter_connected, usb_or_adapter = %d\n\n", usb_or_adapter);

	if(usb_or_adapter == ADAPTER_PM)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(is_adapter_connected);


static void usb_check_timer (unsigned long data)
{
	//printk("usb_check_timer\n\n");


	
	

	mod_timer(&timer_usb_check, jiffies + (1000*HZ/1000)); //1sec
}

static void vdd5v_timer (unsigned long data)
{
	int temp_check; 

	/* 5v conneccted */
	if (hw_power_Get5vPresentFlag()) {

#if 1 //add dhsong
 #ifdef USB_LRADC
        	disable_lradc(USB_DP_ADC_CH);
        	enable_lradc(USB_DP_ADC_CH);
	        start_lradc_delay(adc_delay_slot);
 #else 
		#ifdef SEND_CONN_EVT_AFTER_DISCONN_EVT ////add 1205 to do not send connection event 2 times successively
		if( usb_or_adapter == -1) {
			if( USB_Adapter_IsConnected() ) {
				
				usb_or_adapter = USB_PM;
				printk("[PM] USB connected\n");
				pm_send_event(SS_PM_USB_INSERTED, SS_PM_ALL_DEV);
			}
			else {	
				usb_or_adapter = ADAPTER_PM;
				printk("[PM] Adapter connected\n");
				pm_send_event(SS_PM_5V_INSERTED, SS_PM_ALL_DEV);
			}
		}
		else { //for only send usb_event to nand flash driver
			if( USB_Adapter_IsConnected() ) {
				nand_flash_check = 1;
				printk("[PM] send usb_inserted_event to nand flash driver\n");
				pm_send_event(SS_PM_USB_INSERTED, SS_PM_BUT_DEV);
			}
		}
		#else //always send event when 5v is connected
	#if 0
		if( USB_Adapter_IsConnected() ) { //if usb
			usb_or_adapter = USB_PM;
			usb_disconn_check = 0; //add 1206
			printk("[PM] USB connected\n");
			pm_send_event(SS_PM_USB_INSERTED, SS_PM_ALL_DEV);
		}
		else { //if adapter	
			usb_or_adapter = ADAPTER_PM;
			printk("[PM] Adapter connected\n");
			pm_send_event(SS_PM_5V_INSERTED, SS_PM_ALL_DEV);
		}
	#else //to check usb dp line n times, final func
		temp_check = USB_Adapter_IsConnected();	
		if( temp_check == USB_PM ) { //if usb
#if 0 //disable 20081223 
			if(usb_disconn_check == 0) { //if usb connected state
				//if connection event is sent without disconnection event, first disconn event is sent
				//to resolve problem that user connect usb fast 2 times
				printk("[PM] send USB disconnected event before sending usb connnetction event\n");
				pm_send_event(SS_PM_USB_REMOVED, SS_PM_ALL_DEV); //add dhsong
			}
#endif

		#if 0
			usb_or_adapter = USB_PM;
			//disable 20081223, usb_disconn_check = 0; //add 1206, if 0 = usb conn, if 1 = usb disconn
			printk("[PM] USB connected\n");
			pm_send_event(SS_PM_USB_INSERTED, SS_PM_ALL_DEV);

			/* enable handoff timer */
			del_timer(&timer_handoff);
			mod_timer(&timer_handoff, jiffies + (1000*HZ/1000)); //1sec
			//ddi_power_Handoff(0);
		#else //to resolve the problem that usb event is occured when some china market adapter is connected 
        		disable_lradc(USB_DP_ADC_CH);
	        	enable_lradc(USB_DP_ADC_CH);
		        start_lradc_delay(adc_delay_slot);
		#endif
			
	
		}
		else if( temp_check == ADAPTER_PM ) {
			if( vdd5v_timer_count < 7 ) { //check usb dp line n times
				vdd5v_timer_count ++;
				del_timer(&timer_vdd5v); // 
				mod_timer(&timer_vdd5v, jiffies + (50*HZ/1000)); // 
			}
			else { //we decide that adapter is connected after checking usb dp line n times
				vdd5v_timer_count = 1;
				//del_timer(&timer_vddrv);				
				usb_or_adapter = ADAPTER_PM;
				printk("<PM> Adapter connected\n");
				pm_send_event(SS_PM_5V_INSERTED, SS_PM_ALL_DEV);

				/* enable handoff timer */
				del_timer(&timer_handoff);
				mod_timer(&timer_handoff, jiffies + (1000*HZ/1000)); //1sec
				//ddi_power_Handoff(0);
			}
		}
	#endif
		#endif //#ifdef SEND_CONN_EVT_AFTER_DISCONN_EVT
 #endif //ifdef USB_LRADC
#endif
	} //if (hw_power_Get5vPresentFlag())
	/* 5v disconnected */
	else {
		vdd5v_timer_count = 1;
#if 1 //add dhsong
		if( usb_or_adapter == USB_PM ) {
			printk("<PM> USB disconnected\n");
			pm_send_event(SS_PM_USB_REMOVED, SS_PM_ALL_DEV);
			//disable 20081223,  usb_disconn_check = 1; //1206
		}
		else if( usb_or_adapter == ADAPTER_PM ) {	
#if 0 //disable 20081223
			if (usb_disconn_check == 0) { //add 1206, to resolve problem that is occured when usb conn->adapter conn->adapter disconn 
				printk("[PM] first USB disconnected\n");
				pm_send_event(SS_PM_USB_REMOVED, SS_PM_ALL_DEV); //add dhsong
				usb_disconn_check = 1;
			}
#endif
			printk("<PM> Adapter disconnected\n");
			pm_send_event(SS_PM_5V_REMOVED, SS_PM_ALL_DEV);
		}
		usb_or_adapter = -1; 

		#ifdef SEND_CONN_EVT_AFTER_DISCONN_EVT ////add 1205 to do not send connection event 2 times successively 
		if(nand_flash_check == 1) {	
			printk("<PM> send usb_removed_event to nand flash driver\n");
			pm_send_event(SS_PM_USB_REMOVED, SS_PM_BUT_DEV);
			nand_flash_check = -1;
		}
		#endif
#endif
		//printk("[PM} USB, adapter disconnected\n");

		/* hand off start*/
		PDEBUG("VDD5V->DCDC\n");
		del_timer(&timer_handoff);
		mod_timer(&timer_handoff, jiffies + (1000*HZ/1000)); //1sec
		//ddi_power_Validate5vPowerSource(false);
		//ddi_power_Handoff(0);
		//PDEBUG("%d, HW_POWER_5VCTRL = 0x%08x\n", __LINE__, HW_POWER_5VCTRL_RD() );
		//PDEBUG("%d, HW_POWER_VDDIOCTRL = 0x%08x\n", __LINE__, HW_POWER_VDDIOCTRL_RD() );
		//PDEBUG("%d, HW_POWER_VDDDCTRL = 0x%08x\n", __LINE__, HW_POWER_VDDDCTRL_RD() );
		//PDEBUG("%d, HW_POWER_VDDACTRL = 0x%08x\n", __LINE__, HW_POWER_VDDACTRL_RD() );

		#if 0
		os_pmi_RefreshVoltageSettings();
		#endif
	}
}

static irqreturn_t vdd5v_interrupt (int irq_num, void* dev_idp)
{
	hw_power_Enable5vInterrupt(false);

	PDEBUG("<pm>vdd5v_interrupt\n");
	printk("<pm>vdd5v_interrupt\n");

#if 1 //add dhsong
	if(hw_power_Get5vPresentFlag())
	{
		/* 20081230, first setting dcdc on before battery to 5v handoff */
                hw_power_SetVddaPowerSource(HW_POWER_DCDC_LINREG_READY);
                hw_power_SetVddioPowerSource(HW_POWER_DCDC_LINREG_ON); 


		hw_power_Enable5vUnplugDetect(true);
	
		hw_power_Clear5vInterrupt();

		del_timer(&timer_vdd5v);
		//mod_timer(&timer_vdd5v, jiffies + (30*HZ/1000)); //30ms
		mod_timer(&timer_vdd5v, jiffies + (500*HZ/1000)); //500ms, add dhsong

		del_timer(&timer_handoff);
		hw_power_Enable5vInterrupt(true);
	}

	else
	{
		hw_power_Enable5vPlugInDetect(true);

		hw_power_Clear5vInterrupt();

		del_timer(&timer_vdd5v);
		mod_timer(&timer_vdd5v, jiffies + (30*HZ/1000)); //30ms

		del_timer(&timer_handoff);
		hw_power_Enable5vInterrupt(true);
	}
#endif

#if 1
	;
#else //for IF test, dhsong
	del_timer(&timer_vdd5v);

	if (hw_power_Get5vPresentFlag() ) { //if 5v is inserted
		mod_timer(&timer_vdd5v, jiffies + (500*HZ/1000)); //500ms, add dhsong
	}
	else if (!hw_power_Get5vPresentFlag()) { //if 5v is removed
		mod_timer(&timer_vdd5v, jiffies + (3000*HZ/1000)); //3sec, add dhsong
	}
#endif
    	//hw_power_Clear5vInterrupt();
        //hw_power_Enable5vInterrupt(true); 

	return IRQ_HANDLED;
}

void __init SetVbgAdjSetup (void)
{
    uint8_t    u8Value=0;
    // This code will have issues with the 37XX family if checks are not made
    // for HW_DIGCTL_CHIPID.B.PRODUCT_CODE
    switch (HW_DIGCTL_CHIPID.B.REVISION1)
    {
	case 0: //HW_3700_TA1:
	case 1: //HW_3700_TA2:
	case 2: //HW_3700_TA3:
	case 6: //HW_3700_TA4:
        u8Value = 0x0;  // TA2..TA4. TA4 has 0 value requested by E.S.since only value tested.
        break;
	case 7: //HW_3700_TA5:
	case 8: //HW_3700_TA6:
    /* Datasheet says:
      'Small adjustment for VBG value. Will affect ALL reference voltages.
       Expected to be used to tweak final Li-Ion charge voltage.
       000=Nominal. 001=+0.3%. 010=+0.6%. 011=0.85%. 100=-0.3%. 101=-0.6%. 110=-0.9%. 111=-1.2%.
       Note that, while this bit is located in the DAC address space, since it
       controls both DAC and ADC functions, it is not reset by the AUDIOOUT's
       SFTRST bit. It is reset by a power-on reset only.'
       JLN Oct 12 '07: IC design mgr E.S. said 010 binary (= +0.6%) looks good on
       the tester and is desired for ta5 on this 3 bit field. */
        u8Value = (uint8_t)((HW_OCOTP_SWCAP.U >> 16) & 0x07);

        // If the OTP bits has not been programmed
        // then use the default of 2
        if (u8Value == 0)
        {
            u8Value = 0x2;  // TA5
        }
        break;
    };

	BF_CLR(AUDIOOUT_REFCTRL,VBG_ADJ);
	BF_SETV(AUDIOOUT_REFCTRL,VBG_ADJ,u8Value);
}

static int pm_open (struct inode *inode, struct file * filp)
{
	if (usecount != 0)
	{
		pr_err("PM device cannot be opened!!\n");
		return -EAGAIN;
	}
	usecount = 1;
	return 0;
}

static int pm_release (struct inode *inode, struct file *filp)
{
	usecount = 0;
	return 0;
}

static struct file_operations pm_fops = {
	.ioctl		= pm_ioctl,
	.open		= pm_open,
	.release	= pm_release
};

static struct miscdevice pm_misc = {
	.minor	= STMP_PM_MINOR,
	.name	= "misc/pm",
	.fops	= &pm_fops
};

#if 0 //disable temp, dhsong
static int __init pm_check_5v (void) //check once booting 
{
#if 0
	// 5v interrupt is missing when powered by 5v
	// check 5v flag and send 5v event here
	if (hw_power_Get5vPresentFlag()) {
		PDEBUG("late_initcall - vdd5v connected\n");

		pm_send_event(SS_PM_USB_INSERTED, SS_PM_ALL_DEV);
	}
#endif
	printk("check_5v func \n");
	/* 5v conneccted */
	if (hw_power_Get5vPresentFlag()) {
#if 1 //add dhsong
 #ifdef USB_LRADC
        	disable_lradc(USB_DP_ADC_CH);

        	enable_lradc(USB_DP_ADC_CH);
	        start_lradc_delay(adc_delay_slot);
 #else 
		if( USB_Adapter_IsConnected() ) {
			usb_or_adapter = USB_PM;
			usb_disconn_check = 0; //add 1206
			printk("[PM] USB connected\n");
			pm_send_event(SS_PM_USB_INSERTED, SS_PM_ALL_DEV);
		}
		else {	
			usb_or_adapter = ADAPTER_PM;
			printk("[PM] Adapter connected\n");
			pm_send_event(SS_PM_5V_INSERTED, SS_PM_ALL_DEV);
		}
 #endif
#endif
		/* enable handoff timer */
		del_timer(&timer_handoff);
		mod_timer(&timer_handoff, jiffies + (3000*HZ/1000));
	}
	/* 5v disconnected */
	else {
#if 0 //add dhsong
		if( usb_or_adapter == USB_PM ) {
			printk("[PM] USB disconnected\n");
			pm_send_event(SS_PM_USB_REMOVED, SS_PM_ALL_DEV);
		}
		else if( usb_or_adapter == ADAPTER_PM ) {	
			printk("[PM] Adapter disconnected\n");
			pm_send_event(SS_PM_5V_REMOVED, SS_PM_ALL_DEV);
		}
		//printk("[PM} USB, adapter disconnected\n");


		/* hand off start*/
		PDEBUG("VDD5V->DCDC\n");
		ddi_power_Validate5vPowerSource(false);
		ddi_power_Handoff(0);
#endif

		#if 0
		os_pmi_RefreshVoltageSettings();
		#endif
	}
	return 0;
}
#endif

int init_check_battery(void)
{
        int i;

        //is_battery = -1;
        //setBattCharging(SS_CHG_OFF);
        set_battery_charging_2(0x00, 0x00); //SS_CHG_OFF, SS_THRESHOLD_OFF  

        //usb_charging_level = 0;

        for(i=0; i < 100; i++)
                udelay(1000);

        /* Clear SFTRST, Turn off clock gating. */
        BF_CS2(LRADC_CTRL0, SFTRST, 0, CLKGATE, 0);

        /* Clear the accumulator & NUM_SAMPLES */
        HW_LRADC_CHn_CLR(7, 0xFFFFFFFF);

        BF_CS2(LRADC_CONVERSION, AUTOMATIC, 1, SCALE_FACTOR, 2);

        /* turn on delay channel 2 loop forever, wait 10 milliseconds trigger lradc channel 7 */
        HW_LRADC_DELAYn_WR(2,
                                   (BF_LRADC_DELAYn_TRIGGER_LRADCS(0x80)        |               // channel 7
                                        BF_LRADC_DELAYn_TRIGGER_DELAYS(0x4)    |                // restart delay channel 2 each time
                                        BF_LRADC_DELAYn_LOOP_COUNT(0x0)            |            // no loop count
                                        BF_LRADC_DELAYn_DELAY(0x14) )                                   // 10 msec on 2khz //0xC8 : 100msec   0x14 : 10msec
                                        );
        HW_LRADC_DELAYn_SET(2, BM_LRADC_DELAYn_KICK);                                   // start the whole thing

        //I don't know why I have to this. But, it was really hard to find this should set for getting battery value.
        HW_LRADC_DELAYn_SET(1, BM_LRADC_DELAYn_KICK);                                   // start the whole thing

        /* LRADC channel 7 is adapted DIVIDE_BY_TWO because of voltage boundary problem */
        HW_LRADC_CTRL2_SET(0x80000000);

        return 0;
}



int set_battery_charging_2(unsigned int voltage, unsigned int threshold)
{
        unsigned int power_battchrg_value;
        
        HW_POWER_CTRL_CLR(BM_POWER_CTRL_CLKGATE);
        
        power_battchrg_value = HW_POWER_CHARGE_RD();
        power_battchrg_value &= ~(BM_POWER_CHARGE_BATTCHRG_I|BM_POWER_CHARGE_STOP_ILIMIT);
        
        if(voltage == 0){
                power_battchrg_value |= BM_POWER_CHARGE_PWD_BATTCHRG;
        }       
        else{
                power_battchrg_value |= BF_POWER_CHARGE_BATTCHRG_I(voltage);
                // Setting Current Threshold to 50 mA 
                power_battchrg_value |= BF_POWER_CHARGE_STOP_ILIMIT(threshold);
                power_battchrg_value &= ~BM_POWER_CHARGE_PWD_BATTCHRG;
        }       
        
        //write to charging register
        HW_POWER_CHARGE_WR(power_battchrg_value);
        
        return 1; //BATT_SUCCESS;
}       

int check_batt_status_2(void)
{
        int i;
        long tot_value = 0, batt_ch7conversionValue = 0;
        int batt_stat, batt_average = 0, num_check = 0;
        int init_batt_cnt = 0;
 
        do
        {
batt_loop:
//                for(i = 0; i < 10; i++) //10ms
//                        udelay(1000);
                /* Reading Battery Value */
                batt_ch7conversionValue = HW_POWER_BATTMONITOR.B.BATT_VAL;
 
                BF_CLRn(LRADC_CHn, 7, VALUE);
                HW_LRADC_CTRL0_WR(0x80);  // schedule ch6, ch7 fix for TB1
 
                init_batt_cnt++;
 
                //printk("[Battery check] %d batterylevel is %d\n\n\n", init_batt_cnt, batt_ch7conversionValue);
 
                if(init_batt_cnt > 10) //ITER_LOW_BATT_CHECK)
                        break;
 
                if(batt_ch7conversionValue == 0)
                        goto batt_loop;
 
                num_check++;
 
                tot_value += batt_ch7conversionValue;
 
                if(num_check % 10)
                        goto batt_loop;
                else
                        batt_average = tot_value / 10;
 
        }while(batt_ch7conversionValue == 0);
 
        /* By leeth, Change Threshold detecting No-Battery State at 20061011 */
        /* By leeth, check less 3.1V at 20060913 */
#if 1
        if(batt_average < 415 )//TOO_LOW_BATTLEVEL)
        {
                batt_stat = -1;
        }
        else if(batt_average < 475)//BATT_EXIST_THRESHOLD)
        {
                batt_stat = 0;
        }
        else if(batt_average < 490) //NO_BATT_THRESHOLD) /* hot fix by Lee */
        {
                batt_stat = 1;
        }
        else
        {
                batt_stat = 2;
        }
#endif

#if 1 //delay is applied 
        printk("<pm> batterylevel is %d ,batt_average = %d\n", batt_stat, batt_average);
#else
	udelay(1500);
#endif
 
        return batt_stat;
        //return batt_average;
}

/* HOTFIX:: by leeth, except for VDDA, enable Power rail brownout at 20090721 */
int is_battery = 0;

/* HOTFIX:: by leeth, except for VDDA, enable Power rail brownout at 20090721 */
int is_good_battery(void)
{
	return is_battery;
}

int stmp37xx_check_battery_2(void)
{
        int First_checked_level = 0;
        int Second_checked_level = 0;
        int icheckedlevel = 0;
        int i = 0;
 
        set_battery_charging_2(0x00, 0x00); //SS_CHG_OFF, SS_THRESHOLD_OFF  
	mdelay(30);
	//HW_POWER_CHARGE_CLR(0x1 << 16);	

        //set_battery_charging_2(0x24, 0x04); //SS_CHG_450, SS_THRESHOLD_450 
        //for(i = 0; i < 10; i++) //10 = 10ms
        //        udelay(1000);
        First_checked_level = check_batt_status_2();
#if 0
        printk("[Battery check] First_checked_level is %d\n\n", First_checked_level);
 
        if (First_checked_level < 400) {
                printk("There is no battery or battery is abnormal state \n");
                is_battery = 0;
                return is_battery;
        }
#endif
        /* H.Lim - The definition of check level on this time is different with nandboot_main. */
        //if(icheckedlevel <= 0)
        //{
        set_battery_charging_2(0x08, 0x01); //SS_CHG_100, SS_THRESHOLD_100 
 
        for(i = 0; i < 10; i++) //10 = 10ms
                udelay(1000);
 
        Second_checked_level = check_batt_status_2();
        //printk("[Battery check] Second_checked_level is %d\n\n", Second_checked_level);
#if 0
        if ( (Second_checked_level - First_checked_level) > 100 || (First_checked_level - Second_checked_level) > 100) {
                printk("After charging, There is no battery\n");
                is_battery = 0;
        }
        else {
                printk("After charging, There is battery\n");
                is_battery = 1;
        }
#endif 
 
#if 1
	 if(Second_checked_level <= 0) {

                if(check_batt_status_2() > 1)
                {
                        is_battery = 0;
                        printk("[Battery_Mon] after charging, there is no battery\n");
                }
                else
                {
                        is_battery = 1;
                        printk("[Battery_Mon] after charging, there is battery\n");
                }
                //set_battery_charging_2(0x00, 0x00); //SS_CHG_OFF, SS_THRESHOLD_OFF  
        }
        else
        {
                is_battery = 1;
                printk("[Battery_Mon] normal battery state\n");
        }
#endif

        //set_battery_charging_2(0x00, 0x00); //SS_CHG_OFF, SS_THRESHOLD_OFF  
	HW_POWER_CHARGE_CLR(0x1 << 16);	
        //return 0;
        return is_battery;
}

static int usb_dp_adc_init(int slot, int channel, void *data)
{
        config_lradc(channel, 1, USB_DP_ADC_ACC, USB_DP_LOOPS_PER_SAMPLE);
        config_lradc_delay(adc_delay_slot, USB_DP_LOOPS_PER_SAMPLE, USB_DP_FREQ);
 
        return 0;
}

static void usb_dp_check_handler(int slot, int channel, void *data)
{
        int current_status;
        volatile int earDet;
        unsigned int  this_value;
        unsigned long  this_jiffy;
        int ret, i = 0;
        int usb_ret = 0;
        int check_buf_num = 3;
        int check_buf[check_buf_num];
	int usb_check_thrs = 1500;

	HW_USBPHY_CTRL_SET(0x1 << 13); //set data_on_lradc bit to 1

        ret = result_lradc(channel, &this_value, &this_jiffy);
        if (ret < 0) {
                printk("%s(): result_lradc\n", __func__);
                return;
        }

	PDEBUG("<pm>usb_dn_lradc_val = %d\n", this_value);
	

	if(usb_check_count == 2) {	
		HW_USBPHY_CTRL_CLR(0x1 << 13); //clr data_on_lradc bit to 1
	
		if( this_value < usb_check_thrs ) {
			usb_or_adapter = USB_PM;
                        //disable 20081223, usb_disconn_check = 0; //add 1206, if 0 = usb conn, if 1 = usb disconn
                        printk("<PM> USB connected\n");
                        pm_send_event(SS_PM_USB_INSERTED, SS_PM_ALL_DEV);

			usb_check_count = 0;
	        	disable_lradc(USB_DP_ADC_CH);

                        /* enable handoff timer */
                        del_timer(&timer_handoff);
                        mod_timer(&timer_handoff, jiffies + (1000*HZ/1000)); //1sec
                        //ddi_power_Handoff(0);			

		}
		else if (this_value >= usb_check_thrs) {   
			usb_or_adapter = ADAPTER_PM;
                        printk("<PM> Adapter connected\n");
                        pm_send_event(SS_PM_5V_INSERTED, SS_PM_ALL_DEV);

			usb_check_count = 0;
	        	disable_lradc(USB_DP_ADC_CH);

                        /* enable handoff timer */
                        del_timer(&timer_handoff);
                        mod_timer(&timer_handoff, jiffies + (1000*HZ/1000)); //1sec
                        //ddi_power_Handoff(0);

		}
#if 1
		else { 
			printk("<PM> There is an error in USB or Adapter check routine \n");
			usb_check_count = 0;
			disable_lradc(USB_DP_ADC_CH);
		}
#endif
	}
	usb_check_count++;
	
        usb_dp_adc_init(slot, channel, 0);
        start_lradc_delay(adc_delay_slot);
}

static void usb_dp_adc_handler(int slot, int channel, void *data)
{
        int current_status;
        volatile int earDet;
        unsigned int  this_value;
        unsigned long  this_jiffy;
        int ret, i = 0;
        int usb_ret = 0;
        int check_buf_num = 3;
        int check_buf[check_buf_num];

	usb_or_adapter = -1;

	usb_ret = USB_Adapter_IsConnected();
		
	HW_USBPHY_CTRL_SET(0x1 << 13); //set data_on_lradc bit to 1

        ret = result_lradc(channel, &this_value, &this_jiffy);
        if (ret < 0) {
                printk("%s(): result_lradc\n", __func__);
                return;
        }
	printk("%d: usb_ret = %d\n", __LINE__, usb_ret);
	printk("%d: thi_value = %d\n", __LINE__, this_value);
	

if(usb_check_count == 3) {	

	if( this_value > 0 && this_value < 100 ) {
		usb_or_adapter = USB_PM;
		printk("%d: usb_or_adapter = %d\n", __LINE__, usb_or_adapter );
		printk("[PM] USB connected\n");
		pm_send_event(SS_PM_USB_INSERTED, SS_PM_ALL_DEV);
		usb_check_count = 0;
        	disable_lradc(USB_DP_ADC_CH);
	}
	else if (usb_ret == 0 && this_value > 0 && this_value >= 3000) {   
		usb_or_adapter = ADAPTER_PM;
		printk("%d: usb_or_adapter = %d\n", __LINE__, usb_or_adapter );
		printk("[PM] Adapter connected\n");
		pm_send_event(SS_PM_5V_INSERTED, SS_PM_ALL_DEV);
		usb_check_count = 0;
        	disable_lradc(USB_DP_ADC_CH);
	}
	else { 
		printk("[PM] Adapter or usb is not connected\n");
		usb_check_count = 0;
		disable_lradc(USB_DP_ADC_CH);
	}
}
	usb_check_count++;

        usb_dp_adc_init(slot, channel, 0);
        start_lradc_delay(adc_delay_slot);
}


static void usb_lradc_init()
{
	int ret = -1;

//#ifdef USB_LRADC //add to check usb or adapter
        ret = request_lradc(USB_DP_ADC_CH, DEV_NAME, &usb_dp_ops);
        if (ret < 0) {
                printk("%s(): request_lradc() fail(%d)\n", __func__, ret);
                return -EINVAL;
        }

        ret = request_lradc_delay();
        if (ret < 0) {
                printk("%s(): request_lradc_delay() fail(%d)\n", __func__, ret);
                free_lradc(USB_DP_ADC_CH, &usb_dp_ops);
                return -EINVAL;
        }

        adc_delay_slot = ret;
        assign_lradc_delay(0, ret, USB_DP_ADC_CH);


        disable_lradc(USB_DP_ADC_CH);
        //enable_lradc(USB_DP_ADC_CH);
        //start_lradc_delay(adc_delay_slot);
//#endif
}

static int __init pm_core_init (void)
{
	int ret = 0;
	int batt_check = 0;
	ddi_power_InitValues_t init;

	/* By leeth, we only boot up over 4.4V of external VBUS at 20090711 */
	if(HW_POWER_STS.B.VDD5V_GT_VDDIO == 1) {
		BW_POWER_5VCTRL_VBUSVALID_TRSH(0);
		BW_POWER_STS_VBUSVALID(0);
		udelay(1000);
		if(HW_POWER_STS.B.VBUSVALID == 0) {
			hw_power_PowerDown();
		}
	}

	lpj = loops_per_jiffy;
	PDEBUG("%d, lpj = %d\n", __LINE__, lpj );

	//HW_POWER_5VCTRL_SET(0x1); //enable dcdc
	PDEBUG("%d, HW_POWER_5VCTRL = 0x%08x\n", __LINE__, HW_POWER_5VCTRL_RD() );
#if 0 /// add charge   
        ///FIXME, dhsong
        HW_POWER_CTRL_CLR(BM_POWER_CTRL_CLKGATE); //30 bit
        HW_POWER_CHARGE_CLR(0x1 << 16); //PWD_BATTCHRG Power UP(set 0)
        HW_POWER_CHARGE_SET(0x1 << 10 ); //STOP_ILIMIT set 50mA
            
        HW_POWER_CHARGE_SET(0x1<<4 | 0x1<<2); //BATTCHRG_I set 250mA
        printk("HW_POWER_CHARGE = 0x%08x\n", HW_POWER_CHARGE ); //BATTCHRG_I set 250mA
        printk("Default Charging start\n");
	//udelay(10);
#endif    

	/* ============================================================= */
	/*  POWER INIT                                                   */
	/*    - init hw, vdd5v detection, batt monitoring                */
	/* ============================================================= */
    init.u32BatterySamplingInterval = BATT_SAMPLING_INTERVAL;
	init.e5vDetection = DDI_POWER_VDD5V_GT_VDDIO;
	ddi_power_Init(&init);

	//BF_SET(POWER_MINPWR, DC_HALFCLK); //set PWM or PFM = 750KHz, dhsong
	//BF_CLR(POWER_MINPWR, DC_HALFCLK); //set PWM or PFM = 1.5MHz, dhsong

	// Disable Power Down to improve EMI.  Can only be used 
	// for release builds because it prevent JTAG debugging
	hw_power_DisablePowerDown(true);

	BF_WR(DIGCTL_ARMCACHE, CACHE_SS, 0x03);

	SetVbgAdjSetup();

	/* ============================================================= */
	/*  BATT B/O level                                               */
	/* ============================================================= */
	ddi_power_SetBatteryBrownoutLevel(BATT_BO_LEVEL);


    	if (hw_power_Get5vPresentFlag()) {
		batt_check = stmp37xx_check_battery_2();
	}

	usb_lradc_init();

	/* ============================================================= */
	/*  DCDC HANDOFF SETUP                                           */
	/* ============================================================= */
	/* timer setup */
	setup_timer(&timer_vdd5v, vdd5v_timer, 0);
	setup_timer(&timer_handoff, handoff_timer, 0);
	//setup_timer(&timer_usb_check, usb_check_timer, 0);
	/* handoff init */
	ddi_power_HandoffInit(NULL, NULL, NULL, NULL, 0, 0); //batt_check); //default batt_check = 0

	HW_POWER_VDDDCTRL_SET(0x2<<16); //LINREG_OFFSET = 0x2, dhsong 
	HW_POWER_VDDACTRL_SET(0x2<<12); //LINREG_OFFSET = 0x2, dhsong
	HW_POWER_VDDIOCTRL_SET(0x2<<12); //LINREG_OFFSET = 0x2, dhsong

	//printk("%d, HW_POWER_5VCTRL = 0x%08x\n", __LINE__, HW_POWER_5VCTRL_RD() );
	//printk("%d, HW_POWER_VDDDCTRL = 0x%08x\n", __LINE__, HW_POWER_VDDDCTRL_RD() );
	//printk("%d, HW_POWER_VDDACTRL = 0x%08x\n", __LINE__, HW_POWER_VDDACTRL_RD() );
	//printk("%d, HW_POWER_VDDIOCTRL = 0x%08x\n", __LINE__, HW_POWER_VDDIOCTRL_RD() );

	/* ============================================================= */
	/*  IRQ INIT                                                     */
	/*   - VDD5V, B/O(batt,vddd,vdda,vddio)                          */
	/* ============================================================= */
	/* vdd5v irq register */
	ret = request_irq(IRQ_VDD5V, vdd5v_interrupt , 0, "vdd5v", NULL);
	if (ret != 0) {
		pr_err("Cannot register interrupt vdd5v (err=%d)\n", ret);
		return ret;
	}
	/* enble vdd5v interrupt */
	hw_power_Clear5vInterrupt();
	hw_power_Enable5vInterrupt(true);

	//if (!hw_power_Get5vPresentFlag()) { //if 5V is not inserted
		/* brownout interrupt register */
		ret = request_irq(IRQ_BATT_BRNOUT, bo_interrupt , 0, "bo_batt", NULL);
		if (ret != 0) {
			pr_err("Cannot register interrupt bo_batt (err=%d)\n", ret);
			return ret;
		}
		ret = request_irq(IRQ_VDDD_BRNOUT, bo_interrupt , 0, "bo_vddd", "VDDD");
		if (ret != 0) {
			pr_err("Cannot register interrupt bo_vddd (err=%d)\n", ret);
			return ret;
		}
		ret = request_irq(IRQ_VDDIO_BRNOUT, bo_interrupt , 0, "bo_vddio", "VDDIO");
		if (ret != 0) {
			pr_err("Cannot register interrupt bo_vddio (err=%d)\n", ret);
			return ret;
		}
#if 1
		ret = request_irq(IRQ_VDD18_BRNOUT, bo_interrupt , 0, "bo_vdda", "VDDA");
		if (ret != 0) {
			pr_err("Cannot register interrupt bo_vdda (err=%d)\n", ret);
			return ret;
		}
#endif
	//}

	//--------------------------------------------------------------------------
	// Clear the brownout interrupts.
	//--------------------------------------------------------------------------
	hw_power_ClearBatteryBrownoutInterrupt();
	hw_power_ClearVdddBrownoutInterrupt();
	hw_power_ClearVddaBrownoutInterrupt();
	hw_power_ClearVddioBrownoutInterrupt();

	//--------------------------------------------------------------------------
	// Enable the power supply to assert brownout interrupts.
	//--------------------------------------------------------------------------
	hw_power_EnableBatteryBrownoutInterrupt(true);	
	hw_power_EnableVdddBrownoutInterrupt(true);
	hw_power_EnableVddaBrownoutInterrupt(true);
	hw_power_EnableVddioBrownoutInterrupt(true);

	//--------------------------------------------------------------------------
	// We can handle brownouts now.  Don't allow the chip to power itself off
	// if it experiences a brownout.  We'll manage it in software.
	//--------------------------------------------------------------------------
	hw_power_DisableBrownoutPowerdown();

	//ddi_power_SetVddio(VDDIO_DEFAULT_MV, VDDIO_SAFE_MIN_MV);

	/* ============================================================= */
	/*  set initial pm mode to POWER_ON                              */
	/* ============================================================= */
	//printk("%d, HW_POWER_MINPWR = 0x%08x\n", __LINE__, HW_POWER_MINPWR_RD() );

#if 1
    	if (hw_power_Get5vPresentFlag()) {
		//HW_POWER_5VCTRL_SET(0x1);
		//batt_check = stmp37xx_check_battery_2();
		ddi_power_HandoffInit(NULL, NULL, NULL, NULL, 1, batt_check);
	}
	//printk("%d, HW_POWER_5VCTRL = 0x%08x\n", __LINE__, HW_POWER_5VCTRL_RD() );
#endif

	ret = set_pm_mode_stable(SS_POWER_ON);

	


	return ret;
}


static int __init pm_init (void)
{
	int ret;
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *proc_entry;

	proc_entry = create_proc_entry("pm", S_IWUSR | S_IRUGO, NULL);
	proc_entry->read_proc = proc_read;
	proc_entry->write_proc = proc_write;
	proc_entry->data = NULL; 	
	
#endif

	/* By leeth, we only boot up over 4.4V of external VBUS at 20090711 */
	if(HW_POWER_STS.B.VDD5V_GT_VDDIO == 1) {
		BW_POWER_5VCTRL_VBUSVALID_TRSH(0);
		BW_POWER_STS_VBUSVALID(0);
		udelay(1000);
		if(HW_POWER_STS.B.VBUSVALID == 0) {
			hw_power_PowerDown();
		}
	}

	PDEBUG("%d, HW_POWER_5VCTRL = 0x%08x\n", __LINE__, HW_POWER_5VCTRL_RD() );

	/* PM: register as misc device  */
	ret = misc_register(&pm_misc);
	if (ret != 0) {
		pr_err("Cannot register device /dev/%s (err=%d)\n", pm_misc.name, ret);
		return -EFAULT;
	}


	return ret;
}

core_initcall(pm_core_init);
module_init(pm_init);
//late_initcall(pm_check_5v); //disable temp

