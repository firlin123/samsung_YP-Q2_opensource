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

#ifndef __STMP37XX_BATTERY_H 
#define __STMP37XX_BATTERY_H

#include <asm/types.h>

/********************************************************
*                                                       *
* Definition of STMP37XX_BATTERY                        *
*                                                       *
*********************************************************/

/* Major & Minor No.*/
#define MAJOR_NUMBER		 253
#define MINOR_NUMBER  		 68

/* IOCTL Command */
#define BATT_SET_CHARGING	 10
#define BATT_LEVEL_CHECK	 20
#define BATT_COMPLETE_CHECK	 30
#define BATT_CHARGING_CHECK  	 40
#define BATT_CHARGE_MA_CHECK 	 50

/* Return Value */
#define BATT_SUCCESS		 0
#define BATT_FAILED		 1

/* Battery Voltage Value */
#define MAX_BAT_THRESHOLD		523 /* 4.20V */
#define TOO_LOW_BATTLEVEL 		422 /* 3.39V */
#define BATTERY_CHARGING_COMP	520 /* 4.18V */
#define POWER_DOWN 			416 /* 3.35V */

/* By leeth, level3_high value is changed from 3.89V to 3.85V at 20090502 */
#if 1
#define level4_high 		487 /* 3.85V + (1.3% accuracy) * (about 10% margine) */
#define level4_low 		480
/* It is calculated by adc value of 3.89 and 3.77 */
#define level3_high 		479	/* 3.85V */
#else
/* By leeth, for cellular charger at 20090430 */
#if 1
#define level4_high 		492 /* 3.89V + (1.3% accuracy) * (about 10% margine) */
#else
#define level4_high 		520 /* 4.18V */ //add dhsong for full charge event //523 /* 4.20V */
#endif
#define level4_low 		485
#define level3_high 		484	/* 3.89V */
#endif
#define level3_low		470
#define level2_high		469	/* 3.77V */
#define level2_low		462
#define level1_high		461 /* 3.71V */
/* 2009.06.30: modify the voltage for low battery status { */
/* #define level1_low 		455 /\* 3.66V *\/ */
/* #define level0_high		454 /\* 3.65V *\/ */
#define level1_low 		448 /* 3.60V */
#define level0_high		447 /* 3.59V */
/* 2009.06.30: modify the voltage for low battery status } */
#define level0_low 		422 /* 3.40V */

/* Timer Interval Sec.*/
#define BATT_REFRESH_INTERVAL_L 100	/* 1   sec */
#define BATT_REFRESH_INTERVAL_S   5	/* 50 msec */

/* battery level event status */
#define EVENT_BATTERY_LEVEL_0 0x00
#define EVENT_BATTERY_LEVEL_1 0x01
#define EVENT_BATTERY_LEVEL_2 0x02
#define EVENT_BATTERY_LEVEL_3 0x03
#define EVENT_BATTERY_LEVEL_4 0x04
#define EVENT_BATTERY_LEVEL_5 0x05

/* battery 5v src event status */
//#define EVENT_BATTERY_5V_SRC_NONE 0x06
#define EVENT_BATTERY_5V_SRC_AC_NONE 0x05
#define EVENT_BATTERY_5V_SRC_USB_NONE 0x06
#define EVENT_BATTERY_5V_SRC_AC 0x07
#define EVENT_BATTERY_5V_SRC_USB 0x08

/* battery 5v charging status event status */
#define EVENT_BATTERY_5V_CHARGING_ON 0x09
#define EVENT_BATTERY_5V_CHARGING_OFF 0x10
#define EVENT_BATTERY_5V_CHARGING_COMPLETE 0x11

/* Charging Command Register Value */
enum SS_CHG_CMD
{
	SS_CHG_OFF = 0x00,
	SS_CHG_50  = 0x04,
	SS_CHG_100 = 0x08,
	SS_CHG_150 = 0x0C,
	SS_CHG_200 = 0x10,
	SS_CHG_250 = 0x14,
	SS_CHG_300 = 0x18,
	SS_CHG_350 = 0x1C,
	SS_CHG_400 = 0x20,
	SS_CHG_450 = 0x24
};

/* Threshold Command Register Value */
enum SS_THRESHOLD_CMD
{
	SS_THRESHOLD_OFF = 0x00,
	SS_THRESHOLD_50  = 0x01,
	SS_THRESHOLD_100 = 0x01,
	SS_THRESHOLD_150 = 0x02,
	SS_THRESHOLD_200 = 0x02,
	SS_THRESHOLD_250 = 0x03,
	SS_THRESHOLD_300 = 0x03,
	SS_THRESHOLD_350 = 0x04,
	SS_THRESHOLD_400 = 0x04,
	SS_THRESHOLD_450 = 0x04
};

/* Battery Driver Control & Status Struct*/
typedef struct
{
	int			nBattLevel; // Battery Level
	int			nSetmA;
	int			nBattChareComp; // Charging Complete
	uint16_t	u16BattStatus; // Charging Status
	uint16_t	u16Batt5V; // 5V Connect Status

	uint16_t	u16BattVolt; // Battery Voltage
	uint16_t	u16BattVoltAvr; // Battery Voltage Average
	uint16_t	u16BattVoltAvrCnt; // Battery Average Accumulate Count

	uint16_t    u16BattVoltAvr_Old;// Old Battery Voltage Average
	uint16_t    u16BattVoltAvr_Now;// Now Battery Voltage Average
} battery_struct_t;

/* 
* This array maps bit numbers to current increments, as used in the register
* fields HW_POWER_CHARGE.STOP_ILIMIT and HW_POWER_CHARGE.BATTCHRG_I.
* Bit: |0|  |1|  |2|  |3|  |4|  |5|
*/
static const uint16_t current_per_bit[] = {  10,  20,  50, 100, 200, 400 };

/* Battery Event Structure */
struct battery_event
{
	volatile unsigned short level;
	volatile unsigned short source;
	volatile unsigned short status;	
};


/********************************************************
*                                                       *
* Function Prototype of STMP37XX_BATTERY                *
*                                                       *
*********************************************************/
//==============================
/* file_operations Function */
//==============================
static int battery_open(struct inode *inode,struct file *filp);
static int battery_release(struct inode *inode,struct file *filp);
static int battery_ioctl(struct inode *inode, struct file *file,
							unsigned int cmd, unsigned int nCharge);
static ssize_t battery_read(struct file *filp, char *buf, size_t len, loff_t *ptr);
static unsigned int battery_poll(struct file *filp, poll_table *wait);

//==============================
/* User Define Function */
//==============================
// Batter Timer Handler
void battery_handler(unsigned long arg);
// Set Battery Charging
int	 set_battery_charging(unsigned int voltage, unsigned int threshold);
// PowerMamagement CallBack Command Function
void set_charging_cmd(ss_pm_request_t event);
// Get Charge Ampere
uint16_t get_max_battery_charge_current(void);
// Convert Register Value
uint16_t convert_setting_to_current(uint16_t u16Setting);
// Set Stepping Charging 
void set_stepping_charging(int level);
// Send Battery Event
static void send_battery_event(int level, int source, int status);
// wait_queue_head_t poll_wait_queue;	/* for poll */
static DECLARE_WAIT_QUEUE_HEAD(poll_wait_queue);
// select battery level
int select_battery_level(int nBatteryValue);

void hw_power_powerdown(void);
int dynimic_charging(void);
int dynimic_charging_insert(void);

//==============================
/* Define Struct */
//==============================
/* Battery Structure Initialize */
static battery_struct_t battery_state;
/* Battery Timer Structure */
static struct timer_list battery_timer;
/* file_operations Structure */
static struct file_operations battery_fops = {
	read:		battery_read, 
	poll:		battery_poll, 
	open:		battery_open,
	ioctl:		battery_ioctl,
	release:	battery_release
};
/* misc device Structure */
static struct miscdevice battery_misc = {
	minor : MINOR_NUMBER,
	name  : "misc/battery_event",
	fops  : &battery_fops,
};
/* Battery Status Structure */ 
static volatile struct battery_event battery_event_status = {5, 0, 0};

//==============================
/* Define Variable */
//==============================
/* Event Changed */
static volatile int event_changed = 0;

#endif // STMP37XX-BATTERY_H
