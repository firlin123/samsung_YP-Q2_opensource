 /* $Id: stmp37xx_battery.c, v0.7 2008/08/13 BK.Koo Exp $ */
/***********************************************************************
* \file stmp37xx_battery.c
* \brief stmp37xx battery relation.
* \author Koo ByoungKi <byeonggi.gu@partner.samsung.com>
* \version $Revision: v0.7 $
* \date $Date: 2008/08/13 $
*
* This file is a battery functions using Timer in stmp37xx.
************************************************************************/

/********************************************************
*                                                       *
* Include Header File of stmp37xx_battery.c             *
*                                                       *
*********************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/smp_lock.h>
#include <linux/spinlock.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <asm/arch/stmp37xx.h>
#include <asm/arch/stmp37xx_pm.h>
#include <asm/arch/lradc.h>
#include "stmp37xx_battery.h"

/********************************************************
*                                                       *
* Global Variable of STMP37XX-Battery.c                 *
*                                                       *
*********************************************************/
static int battery_reopen = 0; /* device reopen busy process */
static int isChanged_source = 0;
static int isChanged_level = 0;
static int isChanged_status = 0;

/********************************************************
*                                                       *
* Function of STMP37XX_BATTERY                          *
*                                                       *
*********************************************************/


static int battery_open(struct inode *inode, struct file *filp)
{

	if(battery_reopen) return -EBUSY;

	battery_reopen = 1;

	printk("battery Driver open\n");
	return BATT_SUCCESS;
}

static int battery_release(struct inode *inode, struct file *filp)
{
	battery_reopen = 0;

	printk("battery Driver close\n");	
	return BATT_SUCCESS;
}

static int battery_ioctl(struct inode *inode, struct file *file, unsigned int cmd,unsigned int nCharge)
{
	switch(cmd)
	{
		case BATT_SET_CHARGING:
			set_battery_charging(nCharge,0x04);
		break;

		case BATT_LEVEL_CHECK:
			return battery_state.nBattLevel;
		break;

		case BATT_COMPLETE_CHECK:
			return battery_state.nBattChareComp;
		break;

		case BATT_CHARGING_CHECK:
			return battery_state.u16BattStatus;
		break;

		case BATT_CHARGE_MA_CHECK:
			return get_max_battery_charge_current();
		break;
	}

	printk("battery Driver ioctl\n");
	return BATT_SUCCESS;
}

static ssize_t battery_read(struct file *filp, char *buf, size_t len, loff_t *ptr)
{
	int length, ret;

	if(!(filp->f_flags & O_NONBLOCK))
	{
		wait_event_interruptible(poll_wait_queue, (event_changed == 1));
	}

	if(len < sizeof(battery_event_status))
		length = len;
	else
		length = sizeof(battery_event_status);

	if (copy_to_user (buf, (void *)&battery_event_status, length))
		ret = -EFAULT;
	else
		ret = length;

	event_changed = 0;

	return ret;	
}

static unsigned int battery_poll(struct file *filp, poll_table *wait)
{
	int mask = 0;

	poll_wait(filp, &poll_wait_queue, wait);

	if(event_changed) mask = POLLIN;

	return mask;
}

static void send_battery_event(int level, int source, int status)
{
	battery_event_status.level    = level;
	battery_event_status.source = source;
	battery_event_status.status  = status;

	printk("battery level = [0x%x], source = [0x%x], status = [0x%x]\n", level, source, status);

	event_changed = 1;
	wake_up_interruptible(&poll_wait_queue);
}


void set_charging_cmd(ss_pm_request_t event)
{
	battery_state.u16BattVolt		= 0;
	battery_state.u16BattVoltAvr    = 0;
	battery_state.u16BattVoltAvrCnt = 0;

	switch(event)
	{
		case SS_PM_USB_INSERTED:
			//printk("batt_dev =====> usb connected\n" );
			dynimic_charging_insert();
			battery_state.u16Batt5V = EVENT_BATTERY_5V_SRC_USB;
			isChanged_source = 1;
			break;

		case SS_PM_USB_REMOVED:
			//printk("batt_dev =====> usb disconnected\n" );
			set_battery_charging(SS_CHG_OFF,0x04);
			battery_state.u16Batt5V = EVENT_BATTERY_5V_SRC_USB_NONE;
			battery_state.nBattChareComp = 0;
			battery_state.nSetmA = 0;
			isChanged_source = 1;
			BW_POWER_5VCTRL_VBUSVALID_TRSH(0);
			break;

		case SS_PM_5V_INSERTED:
			printk("<batt_dev> =====> 5V connected\n" );
			dynimic_charging_insert();
			battery_state.u16Batt5V = EVENT_BATTERY_5V_SRC_AC;
			isChanged_source = 1;
			break;

		case SS_PM_5V_REMOVED:
			printk("<batt_dev> =====> 5V disconnected\n" );
			set_battery_charging(SS_CHG_OFF,0x04);
			battery_state.u16Batt5V = EVENT_BATTERY_5V_SRC_AC_NONE;
			battery_state.nBattChareComp = 0;
			battery_state.nSetmA = 0;
			isChanged_source = 1;
			BW_POWER_5VCTRL_VBUSVALID_TRSH(0);
			break;

		case SS_PM_IDLE: 
			break;

		case SS_PM_SET_WAKEUP:
			break;
	}
}

void battery_handler(unsigned long arg)
{
	static int	nfirstcnt = 0;
	static int	nLevel_pre = 0;
	static int	nCharging_pre = 0;
	static int	nPowerDownCnt = 0;
	
	nLevel_pre 	  = battery_state.nBattLevel;
	nCharging_pre = battery_state.u16BattStatus;
	
	/* Get the raw result of battery measurement */
	battery_state.u16BattVolt = HW_POWER_BATTMONITOR.B.BATT_VAL;
	
	/* Battery Average Count ==> 8 */
	if(battery_state.u16BattVoltAvrCnt < 8){
		battery_state.u16BattVoltAvrCnt++;
		battery_state.u16BattVoltAvr+=battery_state.u16BattVolt;
		
		/* Set Timer */
		battery_timer.expires=jiffies+BATT_REFRESH_INTERVAL_L;
	}else{
		
		/* First Old Voltage Setting */
		if(!nfirstcnt){
			battery_state.u16BattVoltAvr_Old = battery_state.u16BattVoltAvr / 8;
			nfirstcnt++;
		}
		
		/* Now Current Voltage Setting */
		battery_state.u16BattVoltAvr_Now = battery_state.u16BattVoltAvr / 8;
		
		/* Voltage Average Setting */
		battery_state.u16BattVoltAvr = (battery_state.u16BattVoltAvr_Old + battery_state.u16BattVoltAvr_Now)/2;
		
		/* Battery Level Decision */
		if((battery_state.nBattLevel == 3) && (battery_state.u16BattVoltAvr > level4_high))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_4;
		else if((battery_state.nBattLevel == 4) && (battery_state.u16BattVoltAvr < level4_low))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_3;
		else if((battery_state.nBattLevel == 2) && (battery_state.u16BattVoltAvr > level3_high))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_3;
		else if((battery_state.nBattLevel == 3) && (battery_state.u16BattVoltAvr < level3_low))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_2;
		else if((battery_state.nBattLevel == 1) && (battery_state.u16BattVoltAvr > level2_high))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_2;
		else if((battery_state.nBattLevel == 2) && (battery_state.u16BattVoltAvr < level2_low))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_1;
		else if((battery_state.nBattLevel == 0) && (battery_state.u16BattVoltAvr > level1_high))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_1;
		else if((battery_state.nBattLevel == 1) && (battery_state.u16BattVoltAvr < level1_low))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_0;
		
		/* Old voltage Setting */
		battery_state.u16BattVoltAvr_Old = battery_state.u16BattVoltAvr;
		
		/* Initialize Voltage Value */
		battery_state.u16BattVoltAvrCnt= 0;
		battery_state.u16BattVoltAvr = 0;
		
		/* Timer Set */
		battery_timer.expires = jiffies+BATT_REFRESH_INTERVAL_S;
	}
	
	/* Timer Refresh */
	add_timer(&battery_timer);	
	
	/* Charging Status Flag */
	if(HW_POWER_CHARGE.B.PWD_BATTCHRG == 0){
		if((battery_state.u16BattStatus != EVENT_BATTERY_5V_CHARGING_COMPLETE) ||
		   (battery_state.u16BattVoltAvr_Now <= BATTERY_CHARGING_COMP)) { /* 2008.12.31: modify condition */
			battery_state.u16BattStatus = EVENT_BATTERY_5V_CHARGING_ON;
		}
	}else{
		battery_state.u16BattStatus = EVENT_BATTERY_5V_CHARGING_OFF;
	}
	
	/* Charging Complete Flag */
	if((battery_state.u16BattVoltAvr_Now > BATTERY_CHARGING_COMP) && 
	   (battery_state.u16BattStatus == EVENT_BATTERY_5V_CHARGING_ON) && 
	   (HW_POWER_STS.B.CHRGSTS == 0) && //){ 	/* 2008.12.30: modify condition */
	   (battery_state.nBattLevel == EVENT_BATTERY_LEVEL_4) ){ /* add dhsong for full charge event */
		battery_state.nBattChareComp = 1;
		battery_state.u16BattStatus = EVENT_BATTERY_5V_CHARGING_COMPLETE;
		//battery_state.nBattLevel = EVENT_BATTERY_LEVEL_4; //set batt level to 4 when batt is full charged, add dhsong		
	}
	
	if(nCharging_pre != battery_state.u16BattStatus) isChanged_status = 1;
	else  isChanged_status = 0;
	
	if(nLevel_pre != battery_state.nBattLevel) isChanged_level = 1;
	else  isChanged_level = 0;
	
	if(isChanged_level || isChanged_status || isChanged_source){
		send_battery_event(battery_state.nBattLevel, battery_state.u16Batt5V, battery_state.u16BattStatus);
		isChanged_level = 0;
		isChanged_status = 0;
		isChanged_source = 0;
	}

	/* Level 0 ==> power down */
	if(!battery_state.nBattLevel && (battery_state.u16BattStatus == EVENT_BATTERY_5V_CHARGING_OFF) && (nPowerDownCnt < 70)) 
		nPowerDownCnt++;
	else if(!battery_state.nBattLevel && (battery_state.u16BattStatus == EVENT_BATTERY_5V_CHARGING_OFF) && (nPowerDownCnt == 70)) 
		hw_power_powerdown();

	/* Dynamic Charging*/
	if((battery_state.u16Batt5V == 7) || (battery_state.u16Batt5V == 8))dynimic_charging();

#if 0
	/* Battery Debug Rtn */
	printk("======================\n");
	printk(" BATTMON_BATT_VAL=> %d\n", battery_state.u16BattVolt);
	printk(" BattAvrCnt    ===> %d\n", battery_state.u16BattVoltAvrCnt);
	printk(" BattAvr       ===> %d\n", battery_state.u16BattVoltAvr);
	printk(" Battery Level ===> %d\n", battery_state.nBattLevel);
	printk(" nChargingComp ===> %d\n", battery_state.nBattChareComp);
	printk(" Batt_Avr_Old  ===> %d\n", battery_state.u16BattVoltAvr_Old);
	printk(" Batt_Avr_Now  ===> %d\n", battery_state.u16BattVoltAvr_Now);
	printk("======================\n\n\n");
#endif	
}

int set_battery_charging(unsigned int voltage, unsigned int threshold)
{
	unsigned power_battchrg_value;

	HW_POWER_CTRL_CLR(BM_POWER_CTRL_CLKGATE);

	power_battchrg_value = HW_POWER_CHARGE_RD();
	power_battchrg_value &= ~(BM_POWER_CHARGE_BATTCHRG_I|BM_POWER_CHARGE_STOP_ILIMIT);
	
	if(voltage == 0){
		power_battchrg_value |= BM_POWER_CHARGE_PWD_BATTCHRG; 
	}
	else{
		power_battchrg_value |= BF_POWER_CHARGE_BATTCHRG_I(voltage); 
		/* Setting Current Threshold to 50 mA  */
		power_battchrg_value |= BF_POWER_CHARGE_STOP_ILIMIT(threshold);
		power_battchrg_value &= ~BM_POWER_CHARGE_PWD_BATTCHRG;
	}

	/* write to charging register */
	HW_POWER_CHARGE_WR(power_battchrg_value);

	return BATT_SUCCESS;
}

#ifdef CONFIG_PROC_FS
static int battery_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data) 
{
	int length = 0;
	
	if (off != 0) {
		return 0;
	}

	*start = page;

	length += sprintf(page, "======================\n");
	length += sprintf(page + length, " BATTMON_BATT_VAL=> %d\n", battery_state.u16BattVolt);
	length += sprintf(page + length, " BattAvrCnt    ===> %d\n", battery_state.u16BattVoltAvrCnt);
	length += sprintf(page + length, " BattAvr       ===> %d\n", battery_state.u16BattVoltAvr);
	length += sprintf(page + length, " Battery Level ===> %d\n", battery_state.nBattLevel);
	length += sprintf(page + length, " nChargingComp ===> %d\n", battery_state.nBattChareComp);
	length += sprintf(page + length, " Batt_Avr_Old  ===> %d\n", battery_state.u16BattVoltAvr_Old);
	length += sprintf(page + length, " Batt_Avr_Now  ===> %d\n", battery_state.u16BattVoltAvr_Now);
	length += sprintf(page + length, "======================\n\n\n");


	*start = page + off;
	off = 0;
	*eof = 1;

	return length;
}

/*static int battery_write_proc(struct file * file, const char * buf, UL count, void *data) 
{
	return count;
}*/
#endif
 
int __init battery_init(void)
{
	/* Register Device */
	/* register_chrdev( MAJOR_NUMBER, "battery_dev", &battery_fops); */
	if (misc_register(&battery_misc) != 0){
		printk(KERN_ERR "Cannot register device /dev/misc/%s\n", battery_misc.name);
		return -EFAULT;
	}

	/* Battery Status Structure Initialize*/
	battery_state.nBattLevel	  = 4; /* Battery Level */
	battery_state.nBattChareComp      = 0; /* Charging Complete */
	battery_state.u16BattStatus	  = 0; /* Charging Status */
	battery_state.u16Batt5V	          = 0; /* 5V Connect Status */
	battery_state.u16BattVolt         = 0; /* Battery Voltage */
	battery_state.u16BattVoltAvr      = 0; /* Battery Voltage Average */
	battery_state.u16BattVoltAvrCnt   = 0; /* Battery Average Accumulate Count */
	battery_state.u16BattVoltAvr_Old  = 0; /* Old Battery Voltage Average */

	/* choice battery level */
	/* battery_state.nBattLevel = select_battery_level(HW_POWER_BATTMONITOR.B.BATT_VAL); */

	/* Initialize battery timer structure */
	init_timer(&battery_timer);
	battery_timer.expires=jiffies+BATT_REFRESH_INTERVAL_S;
	battery_timer.function=battery_handler;
	battery_timer.data=1;

	//HW_POWER_CHARGE.B.PWD_BATTCHRG = 1; //power down pwd_battchrg, add 20090108, dhsong

	/* register battery timer */
	add_timer(&battery_timer);

	/*PowerManagement Callback Function Registration*/
	ss_pm_register(SS_PM_BATT_DEV, set_charging_cmd);

#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *proc_entry;

	proc_entry = create_proc_entry("batt", S_IWUSR | S_IRUGO, NULL);
	proc_entry->read_proc = battery_read_proc;
//	proc_entry->write_proc = battery_write_proc;
	proc_entry->data = NULL;
#endif	

	printk("[BATTERY] battery driver loaded sucessfully\n");
	return BATT_SUCCESS;
}

void __exit battery_exit(void)
{
	/* Delete Battery Timer */
	del_timer(&battery_timer);

	/* Unregister Device */
	/* unregister_chrdev( MAJOR_NUMBER, "battery_dev"); */
	misc_deregister(&battery_misc);

#ifdef CONFIG_PROC_FS
	remove_proc_entry("batt", NULL);
#endif

	printk("[BATTERY] battery driver unloaded sucessfully\n");
}

uint16_t get_max_battery_charge_current(void)
{
	uint8_t u8Bits;

	/* Get the raw data from register */
	u8Bits = BF_RD(POWER_CHARGE, BATTCHRG_I);

	return convert_setting_to_current(u8Bits);
}

uint16_t  convert_setting_to_current(uint16_t u16Setting)
{
	int       i;
	uint16_t  u16Mask;
	uint16_t  u16Current = 0;

	/* Scan across the bit field, adding in current increments. */
	u16Mask = (0x1 << 5);

	for (i = 5; i >= 0; i--, u16Mask >>= 1){
		if(u16Setting & u16Mask) u16Current += current_per_bit[i];
	}

	return(u16Current);
}

void set_stepping_charging(int nlevel)
{
	unsigned int voltage;
	unsigned int threshold;

	/* 2008.12.31: need to compensate, +60/70ma */
	switch(nlevel){
	case 0:	voltage = SS_CHG_OFF; threshold = SS_THRESHOLD_OFF; break;
	case 1:	voltage = SS_CHG_50;  threshold = 0x06; break; /* SS_THRESHOLD_50; */
	case 2:	voltage = SS_CHG_100; threshold = 0x06; break; /* SS_THRESHOLD_100; */
	case 3:	voltage = SS_CHG_150; threshold = 0x07; break; /* SS_THRESHOLD_150; */
	case 4:	voltage = SS_CHG_200; threshold = 0x07; break; /* SS_THRESHOLD_200; */
	case 5:	voltage = SS_CHG_250; threshold = 0x08; break; /* SS_THRESHOLD_250; */
	case 6: voltage = SS_CHG_300; threshold = 0x08; break; /* SS_THRESHOLD_300; */
	case 7: voltage = SS_CHG_350; threshold = 0x0A; break; /* SS_THRESHOLD_350; */
	case 8: voltage = SS_CHG_400; threshold = 0x0A; break; /* SS_THRESHOLD_400; */
	case 9: voltage = SS_CHG_450; threshold = 0x0A; break; /* SS_THRESHOLD_450; */
	default:voltage = SS_CHG_OFF; threshold = SS_THRESHOLD_OFF; break;
	}

	set_battery_charging(voltage,threshold);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
//! \brief Register key value to allow writes to chip reset register
#define POWERDOWN_KEY 0x3e77

void hw_power_powerdown(void)
{
	/* -------------------------------------------------------------------------- */
	/* Make sure the power down bit is not disabled.  Just key so PWD_OFF */
	/* will be written with zero. */
	/* -------------------------------------------------------------------------- */
	HW_POWER_RESET_WR(POWERDOWN_KEY << 16);
	
	/* -------------------------------------------------------------------------- */
	/* Set the PWD bit to shut off the power. */
	/* -------------------------------------------------------------------------- */
	HW_POWER_RESET_WR((POWERDOWN_KEY << 16) | BM_POWER_RESET_PWD);

	/* ---------------------------------------------------------------------- */
	/*  You may begin to feel a little sleepy. */
	/* ---------------------------------------------------------------------- */
	Loop: goto Loop;
}

int dynimic_charging_insert(void)
{
	int nDelayTime = 1000;

	BW_POWER_5VCTRL_OTG_PWRUP_CMPS(1);
	BW_POWER_5VCTRL_VBUSVALID_TRSH(0);
	udelay(nDelayTime);
	
	if(HW_POWER_STS.B.VBUSVALID_STATUS){
		battery_state.nSetmA = 5;
		set_stepping_charging(battery_state.nSetmA);
		return 1;
	}

	BW_POWER_5VCTRL_VBUSVALID_TRSH(1);
	udelay(nDelayTime);
	
	if(HW_POWER_STS.B.VBUSVALID_STATUS){
		battery_state.nSetmA = 2;
		set_stepping_charging(battery_state.nSetmA);
		return 2;
	}
	
	udelay(nDelayTime);

	battery_state.nSetmA = 0;
	set_stepping_charging(battery_state.nSetmA);
	return 3;
}

int dynimic_charging(void)
{
	/* 00 =  4.4 V on 5V insertion, 4.21 V on 5V removal */
	/* 01 = 4.17 V on 5V insertion,  4.0 V on 5V removal */
	
	int	nTrash,nVBusStatus;
	static int nStepCharging;
	int nMaxLevel = 5;

	nTrash			=	HW_POWER_5VCTRL.B.VBUSVALID_TRSH;
	nVBusStatus		=	HW_POWER_STS.B.VBUSVALID_STATUS;
	nStepCharging	=	battery_state.nSetmA;

#if 1
	if((nTrash == 1) && (nVBusStatus == 0)){
		/* Not Charging */
		if(battery_state.nSetmA > 0)battery_state.nSetmA--;
		set_stepping_charging(battery_state.nSetmA);
	}
	else if((nTrash == 0) && (nVBusStatus == 1)){
		if(battery_state.nSetmA < nMaxLevel)battery_state.nSetmA++;
		set_stepping_charging(battery_state.nSetmA);
	}
	else if((nTrash == 1) && (nVBusStatus == 1)){
		if(battery_state.nSetmA < nMaxLevel)battery_state.nSetmA++;
		set_stepping_charging(battery_state.nSetmA);
		if(battery_state.nSetmA > 2) BW_POWER_5VCTRL_VBUSVALID_TRSH(0);
	}
	else if((nTrash == 0) && (nVBusStatus == 0)){
		if(battery_state.nSetmA > 0)battery_state.nSetmA--;
		set_stepping_charging(battery_state.nSetmA);
		if(battery_state.nSetmA < 3) BW_POWER_5VCTRL_VBUSVALID_TRSH(1);
	}
#endif

#if 0
	printk("Charging Level = %d\n", battery_state.nSetmA);
	printk("VBUSVALID = %d\n", HW_POWER_STS.B.VBUSVALID);
	printk("VBUSVALID_STATUS = %d\n", HW_POWER_STS.B.VBUSVALID_STATUS);
	printk("OTG_PWRUP_CMPS = %d\n", HW_POWER_5VCTRL.B.OTG_PWRUP_CMPS);
	printk("VBUSVALID_5VDETECT = %d\n", HW_POWER_5VCTRL.B.VBUSVALID_5VDETECT);
	printk("VBUSVALID_TRSH = %d\n\n", HW_POWER_5VCTRL.B.VBUSVALID_TRSH);
#endif

}

int select_battery_level(int nBatteryValue)
{
	int n;

	for(n = 0; n < 5; n++){
		if((battery_state.nBattLevel == 3) && (nBatteryValue > level4_high))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_4;
		else if((battery_state.nBattLevel == 4) && (nBatteryValue < level4_low))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_3;
		else if((battery_state.nBattLevel == 2) && (nBatteryValue > level3_high))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_3;
		else if((battery_state.nBattLevel == 3) && (nBatteryValue < level3_low))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_2;
		else if((battery_state.nBattLevel == 1) && (nBatteryValue > level2_high))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_2;
		else if((battery_state.nBattLevel == 2) && (nBatteryValue < level2_low))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_1;
		else if((battery_state.nBattLevel == 0) && (nBatteryValue > level1_high))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_1;
		else if((battery_state.nBattLevel == 1) && (nBatteryValue < level1_low))
			battery_state.nBattLevel = EVENT_BATTERY_LEVEL_0;
	}

	return battery_state.nBattLevel;
}

module_init(battery_init);
module_exit(battery_exit);

