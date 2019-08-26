/** 
 * @file        stmp37xx_fmtuner_si4703.c
 * @brief       Implementation for SI4703 FM Tuner driver
 */
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/fs.h>

#include <asm/uaccess.h>

#include <asm-arm/arch-stmp37xx/digctl.h> /* 2008.11.05: for check option */

#include <asm-arm/arch-stmp37xx/stmp37xx_fmtuner_ioctl.h>
#include "stmp37xx_sw_i2c.h"
#include "stmp37xx_fmtuner_si4703.h"
#include "stmp37xx_fmtuner_si4703_reg.h"
/* by jinho.lim */
#include <asm-arm/arch-stmp37xx/pinctrl.h>
#include <asm-arm/arch-stmp37xx/gpio.h>

#if defined(__hppa__) || \
    defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
    (defined(__MIPS__) && defined(__MISPEB__)) || \
    defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
    defined(__sparc__)
#define ENDIAN_BIG 4321
#else
#define ENDIAN_LITTLE 1234
#endif

#define FM_REG_DATA_COUNT 16

/********************************************************
 * ioctl code for supporting stmp36xx
 ********************************************************/
enum {
	SET_FREQUENCY = 0,
	GET_FREQUENCY,
	SET_RSSI_LEVEL,
	Check_Freq_Status,
	SET_REGION,
	MANUAL_FREQ_DOWN,
	MANUAL_FREQ_UP,
	SET_VOL,
	SET_MUTE,
	CHECK_MONO_ST,
	FM_POWER_ON,
	AUTO_SEARCH_UP,
	AUTO_SEARCH_DOWN,
	CHECK_RDS_MSG,
	GET_RDS_BUFFER,
	SET_LIMITED_VOL,
	GET_RSSI_LEVEL = 20,
	GET_AFC_RAIL,
	FM_SET_SPACING,
	FM_913_IS_POWER_ON = 33, /* 2009.06.15: added for verifying power on */
	FM_913_RETRY_POWER_ON = 34 /* 2009.06.15: added for verifying power on */
};

/********************************************************
 * Macro definition
 ********************************************************/
#define USE_CRYSTAL

#define tSRST 100
#define tHRST 100

#define USING_STC_INT HW_SYSCONFIG1.B.STCIEN && HW_SYSCONFIG1.B.GPIO2
#define USING_RDS_INT HW_SYSCONFIG1.B.RDSIEN && HW_SYSCONFIG1.B.GPIO2

/* DCDC mode selection { */
/* Default mode: PWM: 1.5M */
//#define DCDC_MODE_PWM
//#define DCDC_MODE_PFM
//#define DCDC_CLK_15M
//#define DCDC_CLK_750K
/* DCDC mode selection } */

#define FMTUNER_MAJOR	240
#define FM_RSSI_MIN 0
#define FM_RSSI_MAX 127
#define FM_RSSI_DEF 23

#define FM_TIMEOUT_I2C 200000 /* Time out: 200ms */
#define FM_TIMEOUT_TUNE 100000
#define FM_TIMEOUT_SEEK 2000000

#define FM_BAND_OTHERS   0
#define FM_BAND_JAPANNW  2
#define FM_BAND_JAPAN    1
#define FM_DE_KOREAUSA   0
#define FM_DE_DEFAULT    1

#define FM_VOLUME_RANGE  30
/********************************************************
 * enums
 ********************************************************/
typedef enum fm_region_type_t {
	FM_REGION_EUROPE = 0,
	FM_REGION_OTHERS
} en_region;

typedef enum fm_contry_type_t {
	FM_CONTRY_KOREA_USA = 0,
	FM_CONTRY_JAPAN,
	FM_CONTRY_OTHERS
} en_contry;

typedef struct fm_area_code_t {
	en_region region;
	en_contry contry;
} area_code;

/* For U4 ioctl */
enum {
	REGION_KOREA = 0,
	REGION_JAPAN,
	REGION_WORLD
};
/* For SI4721 RX/TX module */
typedef enum power_mode_t {
	POWER_UP_RX = 0, //POWER_UP_IN_FUNC_FMRX,
	POWER_UP_TX = 2  //POWER_UP_IN_FUNC_FMTX
} en_power_mode;

enum { /* Indicate seek/volume control direction */
	GO_DOWN = 0x00,
	GO_UP
};

/********************************************************
 * variables
 ********************************************************/
/* For area specific information */
typedef struct fm_area_spec_t {
	UC band;
	UC space;
	UC de;
	UC rds;
	UC stc;
} fm_area_spec;

fm_area_spec area_spec = {
	.band  = 0,  /* 87.5 ~ 108mhz */
	.space = 1,  /* 100khz spacing */
	.de    = 0,  /* 75 us USA */
	.rds   = 1,  /* RDS enable */
	.stc   = 1   /* STC enable */
};

static area_code default_area = { 
	.region = FM_REGION_OTHERS,
	.contry = FM_CONTRY_KOREA_USA
};

static en_power_mode power_mode;

static UC default_volume = 15;      /* Max for Si4703 FM tuner */
static US default_frequency = 9990;

static US FM_FREQUENCY_MAX = 10800; /* KOREA/USA */
static US FM_FREQUENCY_MIN = 8750;  /* KOREA/USA */
static UC FM_FREQUENCY_STEP = 10;   /* KOREA/USA 100khz step */
static US FM_RSSI_THRESHOLD = FM_RSSI_DEF;
static UC FM_MUTE_STATUS = 1; /* 1: mute off, 0: mute */

static UC frequency_status = 0;  /* add for Check_Freq_Status */

static UC is_tuner_on = false;
static UC is_initialized = false;
static UC is_tunning = false;

static UC open_counter = 0;

wait_queue_head_t fm_wait_q;

static UC is_fr;
static UC mute_option;
static version_inf_t version_info;

static const UC volume_scale = 1; /* 2008.11.27: for controling output level */

const unsigned char volume_table[31] =
{ 
	124, 
	114, 104, 94, /* -5db */
	84, 78,  72,  66,  /* -6db */
	60,  56,  52,  48, 44,  40,  36, /* -2db */  
	32,  30,  28, /* -1db */
	26,  24,  22,  20,  18,  16,
	14,  12,  10,  8,   6,   4,
	2
};

/********************************************************
 * Local functions declearation
 ********************************************************/
/* Local functions */
static void si4703_enable(void);
static void si4703_disable(void);
static void si4703_set_sysconfig(void);
static SC si4703_rx_tune(US frequency);
/* For ioctl */
static SC si4703_power_up(en_power_mode power_mode);
static SC si4703_power_down(void);
static SI si4703_set_configuration(void);
static void si4703_set_region(area_code *area);
static SI si4703_set_frequency(US frequency);
static SI si4703_step_updown(UC direction);
static SI si4703_auto_updown(UC direction);
static SI stmp37xx_set_volume(UC level);
static SC si4703_set_mute(UC mute);
static SI si4703_get_rds(UC *buffer);
static SI si4703_set_spacing(unsigned short spacing);
/* File operations */
static SI fmtuner_si4703_open(struct inode *inode, struct file *file);
static SI fmtuner_si4703_release(struct inode *inode, struct file *file);
static SI fmtuner_si4703_ioctl(struct inode *inode, struct file *filp, UI cmd, UL arg);

/* by jinho.lim */
void onHWmuteForFM(void)
{
	int bank = 0;
	int pin = 21;

	BW_AUDIOOUT_HPVOL_MUTE(0x01);				
	
	if(mute_option == 1) { /* 2008.11.05: check hw option, rotarya == 0 && rotaryb == 0*/
		bank = 0;
		pin = 12;
	}

	stmp37xx_gpio_set_af(pin_GPIO(bank,  pin), GPIO_MODE); //3); //muxsel set gpio func
	stmp37xx_gpio_set_dir(pin_GPIO(bank,  pin), GPIO_DIR_OUT); //output
	stmp37xx_gpio_set_level(pin_GPIO(bank,  pin), 0); //low
	printk("=====================> onHWmuteFM\n");
}
/* by jinho.lim */
void offHWmuteForFM(void)
{
	/* for Q2 - BANK0 21 - MUX Reg1 10/11 : high(mute off), low(mute on) */
	int bank = 0;
	int pin = 21;

	BW_AUDIOOUT_HPVOL_MUTE(0x00);				

	BW_AUDIOOUT_HPVOL_VOL_LEFT(volume_table[default_volume] + volume_scale);
	BW_AUDIOOUT_HPVOL_VOL_RIGHT(volume_table[default_volume] + volume_scale);

	if(mute_option == 1) { /* 2008.11.05: check hw option, rotarya == 0 && rotaryb == 0*/
		bank = 0;
		pin = 12;
	}

	stmp37xx_gpio_set_af(pin_GPIO(bank,  pin), GPIO_MODE); //3); //muxsel set gpio func
	stmp37xx_gpio_set_dir(pin_GPIO(bank,  pin), GPIO_DIR_OUT); //output
	stmp37xx_gpio_set_level(pin_GPIO(bank,  pin), 1); //high
	printk("=====================> offHWmuteFM\n");
}
/**
 * @brief         enable fm tuner module by setting pins and reset tuner  
 */
static void si4703_enable(void)
{
	RESET_HIGH;
	mdelay(3);
	/* Pin setup { */

	stmp37xx_gpio_set_af(BANK0_PIN10, 3);
	stmp37xx_gpio_set_af(BANK0_PIN11, 3);

	stmp37xx_gpio_set_level(BANK0_PIN10, GPIO_VOL_33);
	mdelay(1);

	stmp37xx_gpio_set_dir(BANK0_PIN10, GPIO_DIR_OUT);
	stmp37xx_gpio_set_dir(BANK0_PIN11, GPIO_DIR_OUT);

	stmp37xx_gpio_set_level(BANK0_PIN11, GPIO_VOL_18);
	mdelay(1);
	/* Pin setup } */

	/* Reset condition { */
	RESET_LOW;
	mdelay(3);//fm_udelay(tSRST);
	RESET_HIGH;
	mdelay(1);//fm_udelay(tHRST);
	/* Reset condition } */

	stmp37xx_gpio_set_dir(BANK0_PIN11, GPIO_DIR_IN);
}
/**
 * @brief         disable fm tuner module
 */
static void si4703_disable(void)
{
	stmp37xx_gpio_set_dir(BANK0_PIN10, GPIO_DIR_OUT);
	stmp37xx_gpio_set_dir(BANK0_PIN11, GPIO_DIR_OUT);	

	RESET_LOW;
}
/**
 * @brief         swap lo/hi byte
 */
static void swap(UC *data, UC length)
{
	UC orig;
	UC i;

#ifdef ENDIAN_LITTLE
	for(i = 0; i < length; i += 2) {
		orig = data[i];
		data[i] = data[i + 1];
		data[i + 1] = orig;
	}
#endif
}
/**
 * @brief         write data to register
 */
static SC si4703_reg_write(UC *reg_data, UC length)
{
	SC ret = RETURN_OK;
	UL begin = fm_get_current();

	swap(reg_data, length);  /* swap lo/hi byte for command stream */

	do {
		ret = sw_i2c_write(length, reg_data);
		
		if (fm_get_elasped(begin) > FM_TIMEOUT_I2C) {
			ret = RETURN_NG;
			break;
		}
	} while (ret < 0);

	swap(reg_data, length);  /* swap lo/hi byte for command stream */

	return ret;
}
/**
 * @brief         read data from register
 */
static SC si4703_reg_read(UC *reg_data, UC length)
{
	SC ret = RETURN_OK;
	UL begin = fm_get_current();

	do {
		ret = sw_i2c_read(length, reg_data);

		if (fm_get_elasped(begin) > FM_TIMEOUT_I2C) {
			ret = RETURN_NG;
			break;
		}
	} while (ret < 0);

	swap(reg_data, length);  /* swap lo/hi byte for command stream */

	return ret;
}
/**
 * @brief         configures tuner for tunning
 */
static void si4703_set_sysconfig(void) 
{
	/* Configure GPIO1 */
	/* =>Clear the bits. Comment remaining lines if GPIO1 is to remain unused. */
	HW_POWERCFG.B.DMUTE = 1;//0;
	HW_POWERCFG.B.ENABLE = 1;
	HW_POWERCFG.B.DISABLE = 0;
	HW_POWERCFG.B.RDSM = 0;

	HW_CHANNEL.U = 0; /* Not required before tunning */

	HW_SYSCONFIG1.B.STCIEN = area_spec.stc;
	HW_SYSCONFIG1.B.RDSIEN = area_spec.rds;
	HW_SYSCONFIG1.B.RDS = area_spec.rds;
	HW_SYSCONFIG1.B.DE = area_spec.de;
	HW_SYSCONFIG1.B.GPIO3 = 0;
	HW_SYSCONFIG1.B.GPIO2 = 1;
	HW_SYSCONFIG1.B.GPIO1 = 0;

	HW_SYSCONFIG2.B.SEEKTH = FM_RSSI_THRESHOLD;
	HW_SYSCONFIG2.B.VOLUME = is_fr ? 0x08 : 0x0C; /* 0x1F :0db , 0x0B: -8db*/ /* 0x08: for France, 0x0C: others */
	HW_SYSCONFIG2.B.BAND = area_spec.band;
	HW_SYSCONFIG2.B.SPACE = area_spec.space;
}
/**
 * @brief         calculates channel with given frequency
 * @return        channel value
 */
static US si4703_get_channel(US frequency)
{
	US bottom;
	US channel;

	if (area_spec.band == 1) { /* In case of JAPAN: 76 ~ 108 */
		bottom = 7600;
	} else {
		bottom = 8750;
	}

	if (frequency < bottom) {
		frequency = bottom;
	}
	/* channel = (FREQUCNCY - FREQUENCY_MIN) / SPACING */
	channel = (frequency - bottom) / FM_FREQUENCY_STEP; 

	return channel;
}
/**
 * @Brief         wait until the seeking/tunning is completed
 * @return        notifies error
 */
#include <linux/sched.h>
void fm_usleep(UL timeout)
{
	//set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(timeout);
}
static void si4703_wait_for_stc(UL timeout) 
{
	UL begin = fm_get_current();

	while (WAIT_STC_INTERRUPT && (timeout > fm_get_elasped(begin))) {
		fm_usleep(1);
	}  /* Wait for interrupt to clear the bit */
}
/**
 * @Brief         tunes with passed frequency
 * @return        notifies error
 */
static SC si4703_rx_tune(US frequency)
{
	US channel;
	int ret = RETURN_OK;

	/* Determine if frequency contains the channel number or 10kHz frequency and convert it. */
	if (frequency >= 7600) {
		channel = si4703_get_channel(frequency);
	} else {
		channel = frequency;
	}

#ifdef DCDC_MODE_PFM  	/* Set DCDC mode */
	HW_POWER_MINPWR_SET(BM_POWER_MINPWR_EN_DC_PFM);
	HW_POWER_LOOPCTRL_SET(BM_POWER_LOOPCTRL_HYST_SIGN); /*Set HYST_SIGN to high */
#else
#endif
#ifdef DCDC_CLK_750K
	HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK); 
#else
#endif
	HW_POWERCFG.B.ENABLE = 1;
	HW_POWERCFG.B.DMUTE = 1;//FM_MUTE_STATUS;
	HW_POWERCFG.B.SEEK = 0;
	HW_POWERCFG.B.DSMUTE = 1;
	
	HW_CHANNEL.B.CHAN = channel;
	HW_CHANNEL.B.TUNE = 1;
	
	HW_SYSCONFIG1.B.STCIEN = area_spec.stc;
	HW_SYSCONFIG1.B.RDSIEN = area_spec.rds;
	HW_SYSCONFIG1.B.RDS = area_spec.rds;
	HW_SYSCONFIG1.B.GPIO2 = 1;
	HW_SYSCONFIG1.B.DE = area_spec.de;
	HW_SYSCONFIG1.B.AGCD = 0;

	HW_SYSCONFIG2.B.SPACE = area_spec.space;
	HW_SYSCONFIG2.B.BAND = area_spec.band;
	HW_SYSCONFIG2.B.SEEKTH = FM_RSSI_THRESHOLD;

	ret = si4703_reg_write((UC *) &si4703_reg.POWERCFG, 8);
	if (ret == RETURN_NG) {
		return ret;
	}

	/* Wait for STC bit */
	mdelay(2);
	/* If using interrupts, wait for the interrupt before continuing */
	if (USING_STC_INT) {
		si4703_wait_for_stc(FM_TIMEOUT_TUNE);
	}

	/* Fixed for avoiding side: waiting till 60ms before turning off the tune */
	//mdelay(60); /* Only needed for reading STC */

	HW_CHANNEL.B.TUNE = 0;

	ret = si4703_reg_write((UC *) &si4703_reg.POWERCFG, 4);
	if (ret == RETURN_NG) {
		return ret;
	}

	/* Wait for STC bit is cleaned */
	mdelay(5);
	if (WAIT_STC_INTERRUPT == 0) {
		//printk("STC interrupt state error[%d]\n", WAIT_STC_INTERRUPT);
		mdelay(20);
	}

	/* Wait for stc bit to be set */
	do {
		ret = si4703_reg_read((UC *) &si4703_reg.STATUSRSSI, 4);
	} while (HW_STATUSRSSI.B.STC && ret >= 0);

	/* add for Check_Freq_Status that is ioctl for stmp36xx { */
	if (ret >= 0 && (HW_STATUSRSSI.B.RSSI >= FM_RSSI_THRESHOLD)) {
	        frequency_status = 1;
	} else {
	        frequency_status = 0;
	}

	if(HW_STATUSRSSI.B.AFCRL) {
		frequency_status = 0;
	}
	//printk("afcrl = %d, rssi = %d \n", HW_STATUSRSSI.B.AFCRL, HW_STATUSRSSI.B.RSSI);
	/* add for Check_Freq_Status that is ioctl for stmp36xx } */

	default_frequency = FM_FREQUENCY_MIN + HW_READCHAN.B.READCHAN * FM_FREQUENCY_STEP;

	if (ret == RETURN_NG) {
		return ret;
	}

#ifdef DCDC_CLK_750K
	HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC_HALFCLK); 
#else
#endif
#ifdef DCDC_MODE_PFM  	/* Set DCDC mode */
	HW_POWER_LOOPCTRL_CLR(BM_POWER_LOOPCTRL_HYST_SIGN); /*Set HYST_SIGN to low */
	HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_EN_DC_PFM);
#else
#endif
	return HW_STATUSRSSI.B.RSSI;
}
/**
 * @Brief         set rssi level
 */
static void si4703_set_rssi(US rssi)
{
	if (rssi < FM_RSSI_MIN + 1 || rssi >  FM_RSSI_MAX) {
		rssi = FM_RSSI_DEF;
	}

	FM_RSSI_THRESHOLD = rssi - 10;
	HW_SYSCONFIG2.B.SEEKTH = FM_RSSI_THRESHOLD;
}
/**
 * @brief         power up the SI4703 RX module
 * @param         mode: power up mode, RX or TX (RX only)
 * @return        notifies error
 */
static SC si4703_power_up(en_power_mode power_mode)
{
	SC ret =  RETURN_OK;

	if(is_tuner_on) {
		printk("%s already power is on\n",__func__);
		return ret;
	}
	/* Enables crystal OSC */
#ifdef USE_CRYSTAL
	HW_POWERCFG.B.ENABLE = 1;
	HW_POWERCFG.B.DISABLE = 1;
	HW_POWERCFG.B.DMUTE = 1;//0;

	HW_TEST1.B.XOSCEN = 1;
	HW_TEST1.B.AHIZEN = 0;

	ret = si4703_reg_write((UC *) &si4703_reg.POWERCFG, 12); /* 2 ~ 7 */
#endif

	if (ret < RETURN_OK) {
		return ret;
	}

	is_tuner_on = true;

#if 0 /* Read tuner id for testing */
{
	UC i, j;
	UC enable_shadow[FM_REG_DATA_COUNT * 2];
	unsigned short si4703_reg_data[FM_REG_DATA_COUNT];
	US *penable_shadow;

	mdelay(100);

	penable_shadow = (US *) &enable_shadow[0];

	/* Initialize shadow registers by reading from the part */
	/* Starts at register 0Ah and wraps back around till it reaches 09h:REG_BOOT_CONFIG */
	ret = si4703_reg_read(&enable_shadow[0], FM_REG_DATA_COUNT << 1);

	for(i = 0, j = 0; i < FM_REG_DATA_COUNT; i++, j += 2) {
		si4703_reg_data[(0x0A + i) & 0x0F] = penable_shadow[i];
		printk("[0x%x] %d \t\t[0x%x] %d \n",si4703_reg_data[(0x0A + i) & 0x0F], (0x0A + i) & 0x0F, penable_shadow[i], i);
	}
}
#endif 
	return ret;
}
/**
 * @brief         power down the SI4703 RX/TX module
 * @return        notifies error
 */
static SC si4703_power_down(void)
{
	SC ret = RETURN_OK;

	is_initialized = false;

	if (is_tuner_on) {
		if (is_tunning == true) {
			printk("%s now tunning....waiting event....\n", __func__);
			wait_event_interruptible(fm_wait_q, is_tunning == false);
		}

        	HW_POWERCFG.U = 0;
		HW_POWERCFG.B.DISABLE = 1;
		HW_POWERCFG.B.DMUTE = 1;//0;

		ret = si4703_reg_write((UC *) &si4703_reg.POWERCFG, 2);

		is_tuner_on = false;
		wake_up_interruptible(&fm_wait_q);
	}
	return ret;
}
/**
 * @brief         set region specific info
 */
static void si4703_set_region(area_code *area)
{
	/* Depending on the country, set the de-emphasis, band, and space settings */
	/* Also optionally enable RDS for countries that support it */
	switch (area->contry) {
        case FM_CONTRY_KOREA_USA:
		area_spec.band = FM_BAND_OTHERS;
		area_spec.space = 1;
		area_spec.de = FM_DE_KOREAUSA;
		area_spec.rds = 1;
		FM_FREQUENCY_STEP = 10;
		FM_FREQUENCY_MIN = 8750;
		break;
        case FM_CONTRY_JAPAN:
		area_spec.band = FM_BAND_JAPAN;
		area_spec.space = 1;
		area_spec.de = FM_DE_DEFAULT;
		area_spec.rds = 0;
		FM_FREQUENCY_STEP = 5;
		FM_FREQUENCY_MIN = 7600;
		break;
        case FM_CONTRY_OTHERS:
        default:
		area_spec.band = FM_BAND_OTHERS;
		area_spec.space = 2;
		area_spec.de = FM_DE_DEFAULT;
		area_spec.rds = 1;
		FM_FREQUENCY_STEP = 10;
		FM_FREQUENCY_MIN = 8750;
		break;
	}

	HW_SYSCONFIG2.B.BAND = area_spec.band;
	HW_SYSCONFIG2.B.SPACE = area_spec.space;
	HW_SYSCONFIG1.B.DE = area_spec.de;
	HW_SYSCONFIG1.B.RDS = area_spec.rds;
}
/**
 * @brief         set region specific info: FM_SET_CONFIGURATION
 */
static SI si4703_set_configuration()
{
	SI ret = RETURN_OK;
	
	if (is_initialized == true) {
		return ret;
	}

	is_initialized = true;

	si4703_set_region(&default_area);
	si4703_set_sysconfig();
	si4703_set_rssi(FM_RSSI_THRESHOLD);

	ret = si4703_reg_write((UC *) &si4703_reg.POWERCFG, 8);

	return ret;
}
/**
 * @brief         sets system frequency and then tunning: FM_SET_FREQUENCY
 */
static SI si4703_set_frequency(US frequency)
{
	SI ret = RETURN_OK;

	if (is_tunning == true) {
		printk("%s now tunning....waiting event....\n", __func__);
		wait_event_interruptible(fm_wait_q, is_tunning == false);
	}

	is_tunning = true;

	default_frequency = frequency;

	if (power_mode == POWER_UP_RX) {
		ret = si4703_rx_tune(frequency);
	}

	is_tunning = false;
	wake_up_interruptible(&fm_wait_q);
	return ret;
}
/**
 * @brief         gets current tunned frequency
 */
static SI si4703_get_frequency(void)
{
	return default_frequency;
}
/**
 * @brief         change frequency and tunning with fixed step
 */
static SI si4703_step_updown(UC direction) 
{
	if (direction == GO_UP) {
		default_frequency += FM_FREQUENCY_STEP;
	} else {
		default_frequency -= FM_FREQUENCY_STEP;
	}	
	if (default_frequency > FM_FREQUENCY_MAX) {
		default_frequency = FM_FREQUENCY_MIN;
	}
	if (default_frequency < FM_FREQUENCY_MIN) {
		default_frequency = FM_FREQUENCY_MAX;
	}

	return si4703_set_frequency(default_frequency);
}
/**
 * @brief         search available channel automatically
 */
static SI si4703_auto_updown(UC direction)
{
	SI ret = RETURN_OK;
	SI seek;
	US tmp;

	if (is_tunning == true) {
		printk("%s now tunning.... waiting event....\n", __func__);
		wait_event_interruptible(fm_wait_q, is_tunning == false);
	}

	is_tunning = true;

#ifdef DCDC_MODE_PFM  	/* Set DCDC mode */
	HW_POWER_MINPWR_SET(BM_POWER_MINPWR_EN_DC_PFM); 
	HW_POWER_LOOPCTRL_SET(BM_POWER_LOOPCTRL_HYST_SIGN); /*Set HYST_SIGN to high */
#else
#endif
#ifdef DCDC_CLK_750K
	HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK); 
#else
#endif

	tmp = HW_POWERCFG.U;
	HW_POWERCFG.U = 0;

	HW_POWERCFG.B.ENABLE = 1;
	HW_POWERCFG.B.DMUTE = 1;//FM_MUTE_STATUS;
	HW_POWERCFG.B.SEEK = 1;
	HW_POWERCFG.B.SEEKUP = direction;
	HW_POWERCFG.B.SKMODE = 1;
	HW_POWERCFG.B.DSMUTE = 1;

	HW_CHANNEL.U = 0;  /* Not required for auto tunning */

	HW_SYSCONFIG3.U = 0;
	HW_SYSCONFIG3.B.SKCNT = 0;//8;//8;
	HW_SYSCONFIG3.B.SKSNR = 0;//6;//12;

	//printk("%s seek thershold = %d\n", __func__, FM_RSSI_THRESHOLD);
	
	HW_SYSCONFIG2.B.SEEKTH = FM_RSSI_THRESHOLD;
	
	HW_SYSCONFIG1.B.GPIO2 = 1;
	HW_SYSCONFIG1.B.AGCD = 0;
	HW_SYSCONFIG1.B.RDSIEN = 0;//area_spec.rds;
	HW_SYSCONFIG1.B.STCIEN = area_spec.stc;
	HW_SYSCONFIG1.B.RDS = area_spec.rds;
	HW_SYSCONFIG1.B.DE = area_spec.de;

	ret = si4703_reg_write((UC *) &si4703_reg.POWERCFG, 10);

	if (ret == RETURN_NG) {
		wake_up_interruptible(&fm_wait_q);
		return ret;
	}

#if 1 /* polling interrupt status */
	/* If using interrupts, wait for the interrupt before continuing */
	mdelay(2);
	if (USING_STC_INT) {
		si4703_wait_for_stc(FM_TIMEOUT_SEEK);
	}
	ret = si4703_reg_read((UC *) &si4703_reg.STATUSRSSI, 2);
#else /* Read fm status */
	seek = 0;
	do {
		seek++;
		ret = si4703_reg_read((UC *) &si4703_reg.STATUSRSSI, 2);
		mdelay(1);
		if(seek > 0xff) {
			break;
		}
       	} while ((HW_STATUSRSSI.B.STC == 1) && (ret >= 0));
	printk("SF_BL = %d retry = %d\n", HW_STATUSRSSI.B.SF_BL, seek);
#endif
	seek = ~HW_STATUSRSSI.B.SF_BL & 0x01;

	mdelay(5);

	HW_POWERCFG.B.SEEK = 0;

	ret = si4703_reg_write((UC *) &si4703_reg.POWERCFG, 2);
	if (ret == RETURN_NG) {
		wake_up_interruptible(&fm_wait_q);
		return ret;
	}

	/* Wait for STC bit is cleaned */
	mdelay(2);
	if (WAIT_STC_INTERRUPT == 0) {
		//printk("STC interrupt state error[%d]\n", WAIT_STC_INTERRUPT);
		mdelay(20);
	}

	/* Wait for stc bit to be set */
	do {
		ret = si4703_reg_read((UC *) &si4703_reg.STATUSRSSI, 4);
	} while (HW_STATUSRSSI.B.STC && (ret >= 0));
	/* add for Check_Freq_Status { */

	/* add for Check_Freq_Status that is ioctl for stmp36xx { */
	if (ret >= 0 && (HW_STATUSRSSI.B.RSSI >= FM_RSSI_THRESHOLD)) {
	        frequency_status = 1;
	} else {
	        frequency_status = 0;
	}

	if(HW_STATUSRSSI.B.AFCRL || seek != 1) {
		frequency_status = 0;
	}
	/* add for Check_Freq_Status } */

	default_frequency = FM_FREQUENCY_MIN + HW_READCHAN.B.READCHAN * FM_FREQUENCY_STEP;
	//printk("afcrl = %d, rssi = %d SF_BL = %d freq = %d\n", HW_STATUSRSSI.B.AFCRL, HW_STATUSRSSI.B.RSSI, seek, default_frequency);
	
	tmp = HW_POWERCFG.U;  /* Restore default value for next operation */

#ifdef DCDC_CLK_750K
	HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC_HALFCLK); 
#else
#endif
#ifdef DCDC_MODE_PFM  	/* Set DCDC mode */
	HW_POWER_LOOPCTRL_CLR(BM_POWER_LOOPCTRL_HYST_SIGN); /*Set HYST_SIGN to low */
	HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_EN_DC_PFM);
#else
#endif
	is_tunning = false;
	wake_up_interruptible(&fm_wait_q);

	if (ret == RETURN_NG) {
		return ret;
	}

	return seek;
}
/**
 * @brief         set H/W mute
 */
static SC si4703_set_mute(UC mute)
{
	US tmp;
	SC ret = RETURN_OK;

	tmp = HW_POWERCFG.U;

	FM_MUTE_STATUS = (~mute) & 0x01;

	/* 2008.11.26: removed for avoiding tuner initialization fail in the MUTE state */
	/* HW_POWERCFG.B.ENABLE = 1; */
	/* HW_POWERCFG.B.DMUTE = FM_MUTE_STATUS; */
	/* ret = si4703_reg_write((UC *) &si4703_reg.POWERCFG, 2); */

	HW_POWERCFG.U = tmp;

	return ret;
}
/**
 * @brief         set volume level
 */
static SI stmp37xx_set_volume(UC level)
{
	UC data;

	default_volume = level;
	
	if (default_volume > FM_VOLUME_RANGE) {
		default_volume = FM_VOLUME_RANGE;
	}

	data = volume_table[default_volume];
	data += volume_scale;
	
	BW_AUDIOOUT_HPVOL_VOL_LEFT(data);
	BW_AUDIOOUT_HPVOL_VOL_RIGHT(data);

	if (FM_MUTE_STATUS == 0) {
		si4703_set_mute(FM_MUTE_STATUS);
		offHWmuteForFM();
	}

	return default_volume;
}
/**
 * @brief         get RDS data
 */
static SI si4703_get_rds(UC *buffer) 
{
	UC program_type;
	SI tmp;

	if (is_tunning == true) {
		printk("%s now tunning.... waiting event\n", __func__);
		wait_event_interruptible(fm_wait_q, is_tunning == false);
	}

	is_tunning = true;
	
	//si4703_reg_read((UC *) &si4703_reg.RDSA, 12);
	si4703_reg_read((UC *) &si4703_reg.STATUSRSSI, 12);

	/* Restore byte order for RDS data */
	swap((UC *) &si4703_reg.RDSA, 8);

	tmp = copy_to_user((char *)buffer, (char *) &si4703_reg.STATUSRSSI, 12);  /* for avoiding compiler warning */
	program_type = (si4703_reg.RDSB.U & 0x3E0) >> 5;

	is_tunning = false;
	wake_up_interruptible(&fm_wait_q);

	return program_type;
}
/**
 * @brief         set spacing
 */
static SI si4703_set_spacing(unsigned short spacing) 
{

	switch(spacing) {
	case 50:
		area_spec.space = 0x02;
	case 200:
		area_spec.space = 0x00;
		break;
	case 100:
	default:
		area_spec.space = 0x01;
		break;
	}

	FM_FREQUENCY_STEP = spacing / 10;

	return area_spec.space;
}
/**
 * @brief         defence code for SI4702 913 LOT.
 * @date          2009.06.15
 */
static int si4702_check_power_on(void)
{
	UC i, j;
	UC enable_shadow[FM_REG_DATA_COUNT * 2];
	unsigned short si4703_reg_data[FM_REG_DATA_COUNT];
	US *penable_shadow;
	unsigned short firmware_rev;
	int ret;
	
	penable_shadow = (US *) &enable_shadow[0];

	/* Initialize shadow registers by reading from the part */
	/* Starts at register 0Ah and wraps back around till it reaches 09h:REG_BOOT_CONFIG */
	ret = si4703_reg_read(&enable_shadow[0], FM_REG_DATA_COUNT * 2);

	for(i = 0, j = 0; i < FM_REG_DATA_COUNT; i++, j += 2) {
		si4703_reg_data[(0x0A + i) & 0x0F] = penable_shadow[i];
	}
	
	firmware_rev = ((si4703_reg_chipid *) &si4703_reg_data[1])->B.FIRMWARE;
	printk("<FM> FIRMWARE = 0x%x\n", firmware_rev);

	if (firmware_rev == 0){
		ret = -EPERM;
	} else {
		ret = 1;
	}

	return ret;
}
/**
 * @brief         defence code for SI4702 913 LOT.
 *                retry power up.
 * @date          2009.06.15
 */
static void si4702_retry_power_on(void)
{
	si4703_disable();
	mdelay(1);
	si4703_enable();
	mdelay(1);

	is_tuner_on = false;
	si4703_power_up(POWER_UP_RX);
}
/**
 * @brief         initialize buses and then, reset SI4703
 * @return        notifies error
 */
static int fmtuner_si4703_open(struct inode *inode, struct file *file) 
{
	int ret = RETURN_OK;

	//set_pm_mode(SS_CLOCK_LEVEL16); //dhsong

	/* Protect from multiple call */
	if (open_counter > 0) {
		ret = RETURN_NG;
		return ret;
	}
	
	open_counter = 1;

	/* Check system version */
	get_sw_version(&version_info);

	is_fr = 0;
	if(strcmp(version_info.nation, "FR") == 0) {
		is_fr = 1;
	}

	mute_option = get_hw_option_type();

	printk("%s option = %d\n", __func__, mute_option);
	
	/* by jinho.lim */
	onHWmuteForFM();
	
	BW_AUDIOOUT_HPVOL_SELECT(0x1);	/* Set audio path line-in */

#ifdef DCDC_MODE_PFM  	/* Set DCDC mode */
	//HW_POWER_MINPWR.B.EN_DC_PFM = 1;
#else
#endif
#ifdef DCDC_CLK_750K
	//HW_POWER_MINPWR.B.DC_HALFCLK = 1;
#else
#endif
	//printk("%s DCDC mode PFM = %d, HALFCLK = %d\n", __func__, HW_POWER_MINPWR.B.EN_DC_PFM, HW_POWER_MINPWR.B.DC_HALFCLK);

	sw_i2c_init(FM_SI4703_CHIPID);
	si4703_enable();

	printk("SI4703 FM module is opened \n"); 
	return ret;
}
/**
 * @brief         power down SI4703 and terminate buses
 * @return        notifies error
 */
static int fmtuner_si4703_release(struct inode *inode, struct file *file)
{
	int ret = RETURN_OK;

	/* by jinho.lim */
	onHWmuteForFM();

	HW_AUDIOOUT_HPVOL_SET(0x00000000);

	si4703_power_down();

#ifdef USE_CRYSTAL
	HW_TEST1.B.XOSCEN = 0;
	HW_TEST1.B.AHIZEN = 0;
	HW_TEST1.U |= 0x0100;
	ret = si4703_reg_write((UC *) &si4703_reg.POWERCFG, 12); /* 2 ~ 7 */
#endif

	si4703_disable();

	sw_i2c_term();

	printk("SI4703 FM module is closed\n");

#ifdef DCDC_MODE_PFM  	/* Restore DCDC mode */
	//HW_POWER_MINPWR.B.EN_DC_PFM = 0;
#else
#endif
#ifdef DCDC_CLK_750K
	//HW_POWER_MINPWR.B.DC_HALFCLK = 0;
#else
#endif
	FM_MUTE_STATUS = 1; /* 2008.12.18: set mute status to off */

	open_counter = 0;

	return ret;
}
/**
 * @brief         ioctl operation
 * @return        notifies error
 */
static SI fmtuner_si4703_ioctl(struct inode *inode, struct file *filp, UI cmd, UL arg)
{
	SI ret = RETURN_OK;
	UL val;
	area_code *area_p;

	/* Contol codes for stmp36xx API based application { */
	if (cmd < 0xCA00) {
		switch (cmd) {
		case FM_POWER_ON:
			ret = si4703_power_up(POWER_UP_RX);
			break;
		case SET_REGION:
			/* To avoid warning message from compiler */
			if (get_user(val, (UI *) arg)) {
				return -EINVAL;
			}
			switch (val) {
			case REGION_KOREA:
				area_spec.band = FM_BAND_OTHERS;
				area_spec.space = 1;
				area_spec.de = FM_DE_KOREAUSA;
				area_spec.rds = 1;
				FM_FREQUENCY_STEP = 10;
				FM_FREQUENCY_MIN = 8750;
				break;
			case REGION_JAPAN:
				area_spec.band = FM_BAND_JAPAN;
				area_spec.space = 1;
				area_spec.de = FM_DE_DEFAULT;
				area_spec.rds = 0;
				FM_FREQUENCY_STEP = 10;
				FM_FREQUENCY_MIN = 7600;
				break;
			case REGION_WORLD:
			default:
				area_spec.band = FM_BAND_OTHERS;
				area_spec.space = 2;
				area_spec.de = FM_DE_DEFAULT;
				area_spec.rds = 1;
				FM_FREQUENCY_STEP = 5;
				FM_FREQUENCY_MIN = 8750;
				break;
			}
			ret = RETURN_OK;
			break;
		case SET_FREQUENCY:
			if (get_user(val, (SI *) arg)) {
				return -EINVAL;
			}

			if (is_initialized != true) {
				is_initialized = true;
				
				HW_SYSCONFIG2.B.BAND = area_spec.band;
				HW_SYSCONFIG2.B.SPACE = area_spec.space;
				HW_SYSCONFIG1.B.DE = area_spec.de;
				HW_SYSCONFIG1.B.RDS = area_spec.rds;
				
				si4703_set_sysconfig();
				si4703_set_rssi(FM_RSSI_THRESHOLD);
				
				ret = si4703_reg_write((UC *) &si4703_reg.POWERCFG, 8);
			}
			
			default_frequency = val;
			
			ret = si4703_set_frequency(default_frequency);
			break;
		case GET_FREQUENCY:
			put_user(si4703_get_frequency(), (long *) arg);
			ret = RETURN_OK;
			break;
		case MANUAL_FREQ_UP:
			ret = si4703_step_updown(GO_UP);
			break;
		case MANUAL_FREQ_DOWN:
			ret = si4703_step_updown(GO_DOWN);
			break;
		case AUTO_SEARCH_UP:
			ret = si4703_auto_updown(GO_UP);
			break;
		case AUTO_SEARCH_DOWN:
			ret = si4703_auto_updown(GO_DOWN);
			break;
		case SET_VOL:
		        if (get_user(val, (UC *) arg)) {
				return -EINVAL;
			}
			if (val == 0) {
				onHWmuteForFM();
				FM_MUTE_STATUS = 0;
				if(is_tuner_on) {
					if(default_volume != 0) {
						ret = si4703_set_mute(1);
					}
				}
				default_volume = 0;
			} else {
				ret = stmp37xx_set_volume(val);
			}
			break;
		case SET_MUTE:
			if (get_user(val, (UC *) arg)) {
				return -EINVAL;
			}
			/* by jinho.lim */
			if (val == 1) {
				onHWmuteForFM();
			}

			if(is_tuner_on) {
				ret = si4703_set_mute(val);
			} else {
				FM_MUTE_STATUS = (~val) & 0x01;
			}

			if(val == 0) {
				offHWmuteForFM();
			}

			break;
		case SET_RSSI_LEVEL:
			if (get_user(val, (UC *) arg)) {
				return -EINVAL;
			}
			si4703_set_rssi(val);
			ret = RETURN_OK;
			break;
		case GET_RSSI_LEVEL:
			/* add follow code into si4703_get_rssi */
			put_user(HW_STATUSRSSI.B.RSSI, (UC *) arg);
			ret = RETURN_OK;
			break;
		case GET_RDS_BUFFER:
			/* For avoiding compiler warning */
			ret = copy_to_user((char *)arg, (char *) &si4703_reg.STATUSRSSI, 12);
			break;
		case CHECK_RDS_MSG:
			if(HW_SYSCONFIG1.B.RDSIEN == false) {
				return 0;
			}
			if (is_tunning == true) {
				printk("%s now tunning.... waiting event\n", __func__);
				wait_event_interruptible(fm_wait_q, is_tunning == false);
			}
			
			is_tunning = true;
			
			si4703_reg_read((UC *) &si4703_reg.STATUSRSSI, 12);
			
			ret = HW_STATUSRSSI.B.RDSR | HW_STATUSRSSI.B.RDSS;
			/* 2008.12.11: removed for skipping STC interrupt */
			/* if(stmp37xx_gpio_get_level(BANK0_PIN11) == 0) { */
			/* 	ret = 1; */
			/* } */
			if(ret == 1) {
				/* Restore byte order for RDS data */
				swap((UC *) &si4703_reg.RDSA, 8);
			}

			is_tunning = false;
			wake_up_interruptible(&fm_wait_q);
			break;
		case CHECK_MONO_ST:
			ret = 1;
			break;
		case Check_Freq_Status:
			ret = frequency_status;
			break;
		case FM_SET_SPACING:
			if (get_user(val, (UC *) arg)) {
				return -EINVAL;
			}
			si4703_set_spacing(val);
			break;
		case FM_913_IS_POWER_ON:
			ret = si4702_check_power_on();
			break;
		case FM_913_RETRY_POWER_ON:
			printk("<FM> retry power on\n");
			si4702_retry_power_on();
			break;
		default:
			printk("no old ioctl %x\n", cmd);
			break;
		}
		return ret;
	}
	/* Contol codes for stmp36xx API based application } */

	/* New common FM tuner ioctl */
	switch (cmd) {
	case FM_RX_POWER_UP:
	case FM_TX_POWER_UP:
		ret = si4703_power_up(POWER_UP_RX);
		break;
	case FM_POWER_DOWN:
		ret = si4703_power_down();
		break;
	case FM_SET_CONFIGURATION:
		if (get_user(val, (SI *) arg)) {
			return -EINVAL;
		}
		ret = si4703_set_configuration();
		if (ret > RETURN_NG) {
			ret = si4703_set_frequency(val);
		}
		break;
	case FM_SET_REGION:
		area_p = (area_code *) arg;

		/* To avoid warning message from compiler */
		ret = copy_from_user(&default_area, area_p, sizeof(area_code));
		si4703_set_region(&default_area);
		ret = RETURN_OK;
		break;
	case FM_SET_FREQUENCY:
		if (get_user(val, (SI *) arg)) {
			return -EINVAL;
		}
		ret = si4703_set_configuration();
		default_frequency = val;
		ret = si4703_set_frequency(default_frequency);
		break;
	case FM_GET_FREQUENCY:
		put_user(si4703_get_frequency(), (long *) arg);
		ret = RETURN_OK;
		break;
	case FM_STEP_UP:
		ret = si4703_step_updown(GO_UP);
		break;
	case FM_STEP_DOWN:
		ret = si4703_step_updown(GO_DOWN);
		break;
	case FM_AUTO_UP:
		ret = si4703_auto_updown(GO_UP);
		break;
	case FM_AUTO_DOWN:
		ret = si4703_auto_updown(GO_DOWN);
		break;
	case FM_SET_VOLUME:
		if (get_user(val, (UC *) arg)) {
			return -EINVAL;
		}
		ret = stmp37xx_set_volume(val);
		break;
	case FM_SET_RSSI:
		if (get_user(val, (UC *) arg)) {
			return -EINVAL;
		}
		si4703_set_rssi(val);
		ret = RETURN_OK;
		break;
	case FM_GET_RSSI:
		put_user(HW_STATUSRSSI.B.RSSI, (UC *) arg);
		ret = RETURN_OK;
		break;
	case FM_SET_MUTE:
		if (get_user(val, (UC *) arg)) {
			return -EINVAL;
		}
		ret = si4703_set_mute(val);
		break;
	case CHECK_RDS_MSG:
		if (stmp37xx_gpio_get_level(BANK0_PIN11)) {
			ret = 1;
		} else {
			ret = 0;
		}
		break;
	case FM_GET_RDS_DATA:
		if (stmp37xx_gpio_get_level(BANK0_PIN11)) {
			ret = si4703_get_rds((UC *) arg);
		} else {
			ret = 0;
		}
		break;
	default:
		printk("no matched ioctl %x\n", cmd);
		break;
	}
	
	return ret;
}
static struct file_operations fmtuner03_fops = {
	.owner		= THIS_MODULE,
	.open		= fmtuner_si4703_open,
	.release	= fmtuner_si4703_release,
	.ioctl		= fmtuner_si4703_ioctl,
};

#ifdef CONFIG_PROC_FS
/**
 * @brief         read proc
 */
static SI fm_read_proc(char *page, char **start, off_t off, SI count, SI *eof, void *data) 
{
	SI length = 0;
	UC i;
	UC enable_shadow[FM_REG_DATA_COUNT * 2];
	unsigned short si4703_reg_data[FM_REG_DATA_COUNT];
	US *penable_shadow;

	if (off != 0) {
		return 0;
	}

	*start = page;

	/* POST: FM tuner */
	sw_i2c_init(FM_SI4703_CHIPID);
	si4703_enable();

	mdelay(5);

	penable_shadow = (US *) &enable_shadow[0];

	/* Initialize shadow registers by reading from the part */
	/* Starts at register 0Ah and wraps back around till it reaches 09h:REG_BOOT_CONFIG */
	si4703_reg_read(&enable_shadow[0], FM_REG_DATA_COUNT << 1);

	/* Reordering with register index */
	for(i = 0; i < FM_REG_DATA_COUNT; i++) {
		si4703_reg_data[(0x0A + i) & 0x0F] = penable_shadow[i];
	}

	if(si4703_reg_data[0] == FM_SI4703_DEVID) {
		length += sprintf(page + length, "1242");
		/* display fm error message through LCD */
	} else {
		length += sprintf(page + length, "0000");
	}
	si4703_disable();
	sw_i2c_term();
	
	*start = page + off;
	off = 0;
	*eof = 1;

	return length;
}
#ifndef isdigit
#define isdigit(c) (c >= '0' && c <= '9')
#endif

__inline static int atoi(char *s)
{
	int i = 0;
	while (isdigit(*s))
	  i = i * 10 + *(s++) - '0';
	return i;
}
/**
 * @brief         write proc
 */
static ssize_t fm_write_proc(struct file * file, const char * buf, UL count, void *data) 
{
	char c0[64];
	char c1[64]; 
	int value;

	static unsigned char level = 20;

	sscanf(buf, "%s %s", c0, c1);

	if (!strcmp(c0, "Freq")) {
		value = atoi(c1);
		si4703_set_frequency(value);
	}
	if (!strcmp(c0, "stepup")) {
		si4703_step_updown(GO_UP);
		printk("freq = %d\n", default_frequency);
	}
	if (!strcmp(c0, "stepdown")) {
		si4703_step_updown(GO_DOWN);
		printk("freq = %d\n", default_frequency);
	}
	if (!strcmp(c0, "powerup")) {
		si4703_power_up(POWER_UP_RX);
	}	
	if (!strcmp(c0, "powerdown")) {
		si4703_power_down();
	}	
	if (!strcmp(c0, "autoup")) {
		si4703_auto_updown(GO_UP);
	}	
	if (!strcmp(c0, "autodown")) {
		si4703_auto_updown(GO_DOWN);
	}
	if (!strcmp(c0, "muteon")) {
		si4703_set_mute(1);
	}
	if (!strcmp(c0, "muteoff")) {
		si4703_set_mute(0);
	}
	if (!strcmp(c0, "volon")) {
		level = stmp37xx_set_volume(level);
		level++;
	}
	if (!strcmp(c0, "voldn")) {
		level = stmp37xx_set_volume(level);
		level--;
	} 
	if (!strcmp(c0, "status")) {
		printk("SI4703 FM Tuner driver\n");
		printk("  RSSI: %d\n", HW_STATUSRSSI.B.RSSI);
		printk("  Freq: %d\n", default_frequency);
	}
	return count;
}
#endif
/**
 * @brief         module init function
 */
static SI __init fmtuner_si4703_init(void) 
{

#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *proc_entry;

	proc_entry = create_proc_entry("fm", S_IWUSR | S_IRUGO, NULL);
	proc_entry->read_proc = fm_read_proc;
	proc_entry->write_proc = fm_write_proc;
	proc_entry->data = NULL;
#endif

	init_waitqueue_head(&fm_wait_q);
	/* printk("<FM Tuner> %s\n", __func__); */

	return register_chrdev(FMTUNER_MAJOR, "SI4703", &fmtuner03_fops);
}
/**
 * @brief         module exit function
 */
static void __exit fmtuner_si4703_exit(void) 
{
#ifdef CONFIG_PROC_FS
	remove_proc_entry("fm", NULL);
#endif

	/* printk("<FM Tuner> %s\n", __func__); */
	unregister_chrdev(FMTUNER_MAJOR, "SI4703");
}

module_init(fmtuner_si4703_init);
module_exit(fmtuner_si4703_exit);

MODULE_DESCRIPTION("SI4703 FM Tuner RX/TX driver");
MODULE_AUTHOR("Samsung MP Group");
MODULE_LICENSE("GPL");
