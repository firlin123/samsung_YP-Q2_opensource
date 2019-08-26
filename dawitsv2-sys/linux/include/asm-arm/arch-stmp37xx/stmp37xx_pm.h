#ifndef STMP37XXPM_IOCTL_H
#define STMP37XXPM_IOCTL_H

#include <linux/ioctl.h>
#include <asm/arch/hardware.h>
#include <asm/arch/37xx/regs.h>

#define STMP37XX_MDELAY(m)	stmp37xx_mdelay_func(m)
#define STMP37XX_UDELAY(u)	stmp37xx_udelay_func(u)

enum {
	STMP37XX_PM_EVENT_5V_CONNECTED,
	STMP37XX_PM_EVENT_5V_DISCONNECTED,
	///add for suspend/resume, dhsong
	STMP37XX_PM_EVENT_IDLE,
	STMP37XX_PM_EVENT_WAKE_UP,
};

#if 1
/* added by jinho.lim : clock level define 
LEVEL 1		24/24/24
LEVEL 2 	40/40/40
LEVEL 3 	60/60/60
LEVEL 4 	80/40/40
LEVEL 5 	80/80/80
LEVEL 6 	100/100/100
LEVEL 7 	120/60/60
LEVEL 8 	140/70/70
LEVEL 9 	160/80/80
LEVEL 10	180/90/90
LEVEL 11	200/100/100
LEVEL 12	220/110/110
LEVEL 13	240/120/120
LEVEL 14	260/130/130
LEVEL 15	280/140/133
LEVEL 16	300/150/133
*/

enum {
	SS_POWER_OFF,	// 0
	SS_POWER_ON,
	SS_IDLE,
	
	SS_CLOCK_LEVEL1, //3
	SS_CLOCK_LEVEL2,
	SS_CLOCK_LEVEL3,
	SS_CLOCK_LEVEL4,
	SS_CLOCK_LEVEL5,
	SS_CLOCK_LEVEL6,
	SS_CLOCK_LEVEL7,
	SS_CLOCK_LEVEL8, //10
	SS_CLOCK_LEVEL9,
	SS_CLOCK_LEVEL10,
	SS_CLOCK_LEVEL11,
	SS_CLOCK_LEVEL12,
	SS_CLOCK_LEVEL13,
	SS_CLOCK_LEVEL14,
	SS_CLOCK_LEVEL15,
	SS_CLOCK_LEVEL16, //18
	SS_CLOCK_STABLE,
	SS_CLOCK_LEVEL17, //add 20090122 for fmt

	SS_MAX_CPU = 30,
	SS_MAX_PERF,
};

/*
enum {
	SS_POWER_OFF,	// 0
	SS_POWER_ON,
	SS_IDLE,
	SS_MP3,
	SS_MP3_DNSE,
	SS_MP3_DNSE_SPEED, //5
	
	SS_WMA = 8,			
	SS_WMA_DNSE,
	SS_WMA_DNSE_SPEED, //10

	SS_OGG = 13,
	SS_OGG_DNSE,
	SS_OGG_DNSE_SPEED, //15

	SS_FM = 18,
	SS_FM_DNSE,		
	SS_RECORDING,
	SS_AVI,
	SS_AVI_DNSE, //22

	SS_WMV = 25,
	SS_WMV_DNSE,	// 24

	SS_MAX_CPU = 30,
	SS_MAX_PERF
};
*/
#else
enum {
        SS_POWER_OFF,   // 0
        SS_POWER_ON,
        SS_IDLE,
        SS_MP3,
        SS_MP3_DNSE,
        SS_WMA,                 // 5
        SS_WMA_DNSE,
        SS_OGG,
        SS_OGG_DNSE,
        SS_FM,
        SS_FM_DNSE,             // 10
        SS_RECORDING,
        SS_AVI,
        SS_AVI_DNSE,
        SS_WMV,
        SS_WMV_DNSE,    // 15

        SS_MAX_CPU = 20,
        SS_MAX_PERF
};
#endif

#if 1 //copy from stmp36xx_power.h
enum _power_err_enum
{
    POWER_ERR_SUCCESS = 0,
    POWER_ERR_FAILURE = -1
};
 
typedef enum _power_err_enum power_err_t;
 
typedef struct 
{
        volatile bool  POWER_5V_ADAPTOR;
        volatile bool  POWER_USB_CABLE;
        volatile bool  POWER_IDLE_STATUS;
} power_context_t;

enum VDDCTLVALUE
{
        VDDDIO_BO = 0,
        VDDIO_TRG,
        VDDD_BO,
        VDDD_TRG,
        VDD_MAX
};
 
// valid only for LINREG_OFFSET = 1
 
/* modified by jinho.lim as stmp36xx power supply spec */
enum _dcdc_core_step
{
        CORE_1_280 = 0x08,
        CORE_1_312 = 0x09,
        CORE_1_344 = 0x0A,
        CORE_1_376 = 0x0B,
        CORE_1_408 = 0x0C,
        CORE_1_440 = 0x0D,
        CORE_1_472 = 0x0E,
        CORE_1_504 = 0x0F,
        CORE_1_536 = 0x10,
        CORE_1_568 = 0x11,
        CORE_1_600 = 0x12,
        CORE_1_632 = 0x13,
        CORE_1_664 = 0x14,
        CORE_1_696 = 0x15,
        CORE_1_728 = 0x16,
        CORE_1_760 = 0x17,
        CORE_1_792 = 0x18,
        CORE_1_824 = 0x19,
        CORE_1_856 = 0x1A,
        CORE_1_888 = 0x1B,
        CORE_1_920 = 0x1C,
        CORE_1_952 = 0x1D,
        CORE_2_108 = 0x1E,
        CORE_2_200 = 0x1F
};

enum _dcdc_io_step
{
        IO_2_049 = 0x00,
        IO_2_113 = 0x01,
        IO_2_177 = 0x02,
        IO_2_241 = 0x03,
        IO_2_305 = 0x04,
        IO_2_369 = 0x05,
        IO_2_433 = 0x06,
        IO_2_497 = 0x07,
        IO_2_561 = 0x08,
        IO_2_625 = 0x09,
        IO_2_689 = 0x0A,
        IO_2_753 = 0x0B,
        IO_2_817 = 0x0C,
        IO_2_881 = 0x0D,
        IO_2_945 = 0x0E,
        IO_3_009 = 0x0F,
        IO_3_073 = 0x10,
        IO_3_137 = 0x11,
        IO_3_201 = 0x12,
        IO_3_265 = 0x13,
        IO_3_329 = 0x14,
        IO_3_393 = 0x15,
        IO_3_457 = 0x16,
        IO_3_521 = 0x17,
        IO_3_585 = 0x18,
        IO_3_649 = 0x19,
        IO_3_713 = 0x1A,
        IO_3_777 = 0x1B,
        IO_3_841 = 0x1C,
        IO_3_905 = 0x1D,
        IO_3_969 = 0x1E,
        IO_4_033 = 0x1F
};

enum
{
        SS_PM_IDLE,
        SS_PM_RESUME,
        SS_PM_SAVE_STATE,
        SS_PM_SET_WAKEUP,
        SS_PM_USB_INSERTED,
        SS_PM_USB_REMOVED,
        SS_PM_5V_INSERTED,
        SS_PM_5V_REMOVED,
        /* For USB event by Lee*/
        SS_PM_USB_ENABLED,
        SS_PM_USB_DISABLED,
        /* For LCD event by Lee*/
        SS_PM_LCD_ON,
        SS_PM_LCD_OFF,
        SS_PM_LOW_BATT,
        /* For Safe, connect USB by Lee*/
        SS_PM_USB_UNCONFIG,
        SS_PM_USB_CONFIG,
        SS_PM_CHARGE_UP,
        SS_PM_CHARGE_DOWN,
        SS_PM_DCDC_ON,
        SS_PM_CHARGE_CUTOFF,
        SS_PM_PWRKEY_PRESSED,
        SS_PM_PWRKEY_RELEASED,
        /* For Backlight event by Lee*/
        SS_PM_BL_MAX,
        SS_PM_BL_MID,
        SS_PM_BL_MIN,
        /* By leeth, For extra another event such as transferring, ... at 20080118 */
        SS_PM_USB_SUSPEND,
        SS_PM_USB_IDLE,
        SS_PM_USB_TRANSFER,

	SS_PM_NOT_USED_EVNET
};
typedef int ss_pm_request_t;
#endif //copy from stmp36xx_power.h

enum
{
        SS_PM_USB_DEV,      
        SS_PM_EVT_DEV,
        SS_PM_CHG_DEV,
        SS_PM_SND_DEV,
        SS_PM_RTC_DEV,
        SS_PM_BUT_DEV,
        SS_PM_LCD_DEV,       
        SS_PM_BATT_DEV,       
        SS_PM_TOUCHPAD_DEV,       
        SS_PM_ALL_DEV,       
        SS_PM_EAR_DEV,       
        SS_PM_TEST_DEV       
};
typedef int ss_pm_dev_t;


#if defined __KERNEL__
typedef int (*stmp37xx_pm_callback) (int event);

//int set_pm_mode (int pm_target);
int ss_pm_register (ss_pm_dev_t type, stmp37xx_pm_callback callback);
int pm_send_event (int event, int device);
void hw_power_SetVddaValue(uint16_t u16Vdda_mV);
int stmp37xx_mdelay_func(int sec);
int stmp37xx_udelay_func(int sec);
int ddi_power_SetVdda(uint16_t  u16NewTarget, uint16_t  u16NewBrownout);
int Enable_HclkSlow(int select);
int is_USB_connected(void);
int is_adapter_connected(void);
#endif

#define OPM_LEVEL _IOW('c', 0, unsigned long)

#endif //STMP37XXPM_IOCTL_H
