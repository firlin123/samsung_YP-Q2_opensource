
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/proc_fs.h>

#define CPU_FREQ_MIN	 1000
#define CPU_FREQ_MAX	360000
#define VDDA_SS		2100 //MAX = 2100
typedef int RtStatus_t;

#include "clocks/ddi_clocks.h"
#include "clocks/hw_clocks.h"
#include "clocks/ddi_emi.h"
#include "power/ddi_power.h"


/* TODO: Add support for SDRAM timing changes */

static int proc_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *buf = page;
	char *next = buf;
	unsigned size = count;
	int t;

	if (off != 0)
		return 0;

	/* basic device status */
	t = scnprintf(next, size,
				  "PCLK %d HCLK %d HCLKSLOW %d EMICLK %d GPMICLK %d XCLK %d\n"
				  "SSPCLK %d IRCLK %d IROVCLK %d PIX %d\n"
				  "[Bypass]\n"
				  "cpu %d emi %d pix %d iossp %d iogpmi %d ioir %d\n"
				  "MaxPllRef %d\n"
				  "vddd %d vdda %d vddio %d\n",
				  ddi_clocks_GetPclk(), ddi_clocks_GetHclk(), ddi_clocks_GetHclkSlow(),
				  ddi_clocks_GetEmiClk(), ddi_clocks_GetGpmiClk(), ddi_clocks_GetXclk(), 
				  ddi_clocks_GetSspClk(), ddi_clocks_GetIrClk(), ddi_clocks_GetIrovClk(),
				  ddi_clocks_GetPixClk(),
				  ddi_clocks_GetBypassRefCpu(), ddi_clocks_GetBypassRefEmi(),
				  ddi_clocks_GetBypassRefPix(),ddi_clocks_GetBypassRefIoSsp(),
				  ddi_clocks_GetBypassRefIoGpmi(), ddi_clocks_GetBypassRefIoIr(), 
				  ddi_clocks_GetMaxPllRefFreq(),
				  ddi_power_GetVddd(), ddi_power_GetVdda(), ddi_power_GetVddio());

	size -= t;
	next += t;

	*eof = 1;
	return count - size;
}

#define isdigit(c) (c >= '0' && c <= '9')
__inline static int atoi( char *s)
{
	int i = 0;
	while (isdigit(*s))
	  i = i*10 + *(s++) - '0';
	return i;
}

static ssize_t proc_write (struct file * file, const char * buf, 
	unsigned long count, void *data)
{
	char cmd0[64], cmd1[64], cmd2[64]; 
	unsigned value, value2;

	sscanf(buf, "%s %s %s", cmd0, cmd1, cmd2);
	
	if(!strcmp(cmd0, "hclkdiv")) {
		unsigned pclk;
		value = atoi(cmd1);
		pclk = ddi_clocks_GetPclk();
		ddi_clocks_SetPclkHclk(&pclk, value);
	}
	else if (!strcmp(cmd0, "gpmiclk")) {
		value = atoi(cmd1);
		ddi_clocks_SetGpmiClk(&value, 1);
	}
	else if (!strcmp(cmd0, "xclk")) {
		value = atoi(cmd1);
		ddi_clocks_SetXclk(&value);
	}
	else if (!strcmp(cmd0, "pixclk")) {
		value = atoi(cmd1);
		ddi_clocks_SetPixClk(&value);
	}
	else if (!strcmp(cmd0, "vddd")) {
		value = atoi(cmd1);
		value2 = atoi(cmd2);
		ddi_power_SetVddd(value, value2);
	}
	else if (!strcmp(cmd0, "vdda")) {
		value = atoi(cmd1);
		value2 = atoi(cmd2);
		ddi_power_SetVdda(value, value2);
	}
	else if (!strcmp(cmd0, "vddio")) {
		value = atoi(cmd1);
		value2 = atoi(cmd2);
		ddi_power_SetVddio(value, value2);
	}

	return count; 
}

int stmp37xx_verify_speed(struct cpufreq_policy *policy)
{
	if (policy->cpu)
		return -EINVAL;

	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
					 policy->cpuinfo.max_freq);

	policy->min = policy->min;
	policy->max = policy->max;
	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
					 policy->cpuinfo.max_freq);
	return 0;
}

unsigned int stmp37xx_getspeed(unsigned int cpu)
{
	return ddi_clocks_GetPclk();
}

#if 0
typedef struct {
	unsigned pclk;
	unsigned hclkdiv;
	unsigned emiclk;
	unsigned gpimclk;
	unsigned xclk;

	unsigned vddd;
	unsigned vddd_bo;
#if 0
	unsigned vddio;
	unsigned vddio_bo;
	unsigned vdda;
	unsigned vdda_bo;
#endif
} freq_table;

freq_table ftable[] = 
{
	{ 24000, 1, 133000, 24000, 24000, 1450, 1350 },
	{ 24000, 1, 133000, 24000, 24000, 1450, 1350 },
};
#endif
extern RtStatus_t ddi_emi_ChangeClockFrequency(int EmiClockSetting);

static int adjust_clocks (unsigned target_freq)
{
	int hclkdiv;
	unsigned emiclk, gpmiclk, xclk;

	/* todos */
	if (target_freq > 300000) {
		hclkdiv = 2;
	}
	else {
		hclkdiv = 1;
	}

	xclk = 24*target_freq/(320);
	emiclk = 96*target_freq/(320);
	//gpmiclk = 24*target_freq/(320);
	gpmiclk = 24000; //add dhsong

	#if 1
	// temporary !!hack!!
	if (ddi_clocks_GetEmiClk() > 24000)
	{
		// emi always needs pll
		ddi_clocks_ClockUsingPll(EMICLK, 1);
	}
	#endif

	ddi_clocks_SetPclkHclk(&target_freq, hclkdiv);
	ddi_clocks_SetXclk(&xclk);
	ddi_clocks_SetGpmiClk(&gpmiclk, 1);

    //ddi_clocks_SetEmiClk(&emiclk);
	//ddi_emi_ChangeClockFrequency(24); //EMI_CLK_24MHz);

#if 1
	if (target_freq < 200000) {
		//--------------------------------------------------------------------------
		// Enable interrupt wait to gate unused CPU clocks to save power.  
		//--------------------------------------------------------------------------
		hw_clkctrl_SetPclkInterruptWait(true);    
		//--------------------------------------------------------------------------
		// Select conditions to use HCLK auto-slow.
		//--------------------------------------------------------------------------
		hw_clkctrl_EnableHclkModuleAutoSlow(APBHDMA_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(APBXDMA_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(TRAFFIC_JAM_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(TRAFFIC_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(CPU_DATA_AUTOSLOW,true);
		hw_clkctrl_EnableHclkModuleAutoSlow(CPU_INSTR_AUTOSLOW,true);

		//--------------------------------------------------------------------------
		// Set the HCLK auto-slow divider to 32.  
		//--------------------------------------------------------------------------
		hw_clocks_SetHclkAutoSlowDivider(SLOW_DIV_BY32);

		//--------------------------------------------------------------------------
		// Enable HCLK auto-slow.
		//--------------------------------------------------------------------------
		hw_clocks_EnableHclkAutoSlow(true);
	}
	else 
#endif
	{
		hw_clkctrl_SetPclkInterruptWait(false);
		hw_clocks_EnableHclkAutoSlow(false);
	}

	/* todos */
	return 0;
}

static int adjust_powers (unsigned target_freq)
{
	/* todos */
	if (target_freq < 100000) {
		ddi_power_SetVddd(1050, 975);
		//ddi_power_SetVddio(1050, 975);
	}
	else if (target_freq < 200000) {
		ddi_power_SetVddd(1200, 1100);
	}
	else if (target_freq < 260000) {
		ddi_power_SetVddd(1300, 1200);
	}
	else {
		ddi_power_SetVddd(1450, 1350);
	}
	return 0;
}

static int stmp37xx_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	struct cpufreq_freqs freqs;
	int ret = 0;

	freqs.old = stmp37xx_getspeed(0);
	freqs.new = target_freq;
	freqs.cpu = 0;

	//printk("old %d new %d target %d\n", freqs.old, freqs.new, target_freq);

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	// clock down
	if (freqs.old > freqs.new) {
		adjust_clocks(freqs.new);
		adjust_powers(freqs.new);
	}
	// clock up
	else {
		adjust_powers(freqs.new);
		adjust_clocks(freqs.new);
	}

	//printk("old %d new %d target %d\n", freqs.old, freqs.new, target_freq);
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	return ret;
}

static int __init stmp37xx_cpu_init(struct cpufreq_policy *policy)
{
	if (policy->cpu != 0)
		return -EINVAL;

	policy->cur = policy->min = policy->max = stmp37xx_getspeed(0);
	policy->cpuinfo.min_freq = CPU_FREQ_MIN;
	policy->cpuinfo.max_freq = CPU_FREQ_MAX;
	policy->cpuinfo.transition_latency = CPUFREQ_ETERNAL;
	return 0;
}

static struct cpufreq_driver stmp37xx_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= stmp37xx_verify_speed,
	.target		= stmp37xx_target,
	.get		= stmp37xx_getspeed,
	.init		= stmp37xx_cpu_init,
	.name		= "stmp37xx",
};

static int __init stmp37xx_cpufreq_init(void)
{
	struct proc_dir_entry *proc_entry;

	/* FIXME::HOTFIX:: for temperary, VDDA is fixed 1.950V at 20080625 */
	ddi_power_SetVdda(VDDA_SS, VDDA_SS - VDDA_TO_BO_MARGIN); //dhsong
	//hw_power_SetVddaValue(VDDA_SS);

	proc_entry = create_proc_entry("freq", S_IWUSR | S_IRUGO, NULL);
	proc_entry->read_proc = proc_read;
	proc_entry->write_proc = proc_write;
	proc_entry->data = NULL; 	

	return cpufreq_register_driver(&stmp37xx_driver);
}

arch_initcall(stmp37xx_cpufreq_init);
