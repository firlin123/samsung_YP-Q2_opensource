/** 
 * @file        stmp37xx_fmtuner_si4721.c
 * @brief       Implementation for SI4721 FM Tuner driver
 */
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/fs.h>

#include <asm/uaccess.h>

#include "stmp37xx_fmtuner_si4721.h"
#include "stmp37xx_sw_i2c.h"

/********************************************************
 * Macro definition
 ********************************************************/
/* DCDC mode selection { */
/* Default mode: PWM: 1.5M */
#define DCDC_MODE_PWM
//#define DCDC_MODE_PFM
#define DCDC_CLK_15M
//#define DCDC_CLK_750K
/* DCDC mode selection } */

#define FMTUNER_MAJOR	240
#define FM_RSSI_MIN 0
#define FM_RSSI_MAX 127
#define FM_RSSI_DEF 20
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

typedef enum power_mode_t {
	POWER_UP_RX = POWER_UP_IN_FUNC_FMRX,
	POWER_UP_TX = POWER_UP_IN_FUNC_FMTX
} en_power_mode;

enum {
	GO_DOWN = 0x00,
	GO_UP
};

/********************************************************
 * variables
 ********************************************************/
/* for tune status bit { */
typedef struct rx_tune_status_t {
	UC STC;     /* Seek/Tune complete */ 
	UC BLTF;    /* Band limit indicator while auto search */
	UC AFCRL;   /* Automatic frequency control */
	UC Valid;   /* Valid channel */
	US Freq;    /* Tuned frequency */
	UC RSSI;    /* RSSI level */
	UC ASNR;    /* SNR */
	US AntCap;  /* Antenna tuning capacitor */ 
} rx_tune_status;

typedef struct tx_tune_status_t {
	UC STC;     /* Seek/Tune complete */ 
	UC Voltage; /* TX voltage */
	US Freq;    /* Tuned frequency */
	US AntCap;  /* Antenna tuning capacitor */
	UC RNL;     /* Received noise level */
} tx_tune_status;

typedef union tune_status_t {
	rx_tune_status rx;
	tx_tune_status tx;
} tune_status;

/* for tune status bit } */
static en_power_mode power_mode;
static tune_status status;

static UC command_data[8] = {0};
static UC reply_data[13] = {0};
static UC rds_data[12] = {0};

static UC default_volume = 63;      /* Max for Si4721 FM tuner */
static UC default_tx_voltage = 115; /* Max for Si4721 FM tuner */
static US default_frequency = 9990;

static SI is_tunning = false;

static area_code default_area = { 
	.region = FM_REGION_OTHERS,
	.contry = FM_CONTRY_KOREA_USA
};

static UC tuner_is_on = false;

static US FM_FREQUENCY_MAX = 10800; /* KOREA/USA */
static US FM_FREQUENCY_MIN = 8750;  /* KOREA/USA */
static US FM_FREQUENCY_STEP = 10;   /* KOREA/USA 100khz step */

static US FM_RSSI_THRESHOLD = FM_RSSI_DEF;

/********************************************************
 * Local functions declearation
 ********************************************************/
/* Local functions */
static void si4721_enable(void);
static void si4721_disable(void);

static SI si4721_wait_for_cts(void);
static SC si4721_command(UC cmd_size, UC *cmd, UC reply_size, UC *reply);
static SC si4721_set_property(US property, US value);
static void si4721_configure(void);
static SC si4721_rx_tune(US frequency);
static SC si4721_tx_tune(US frequency);
static SC si4721_get_interrupt_status(void);
static SC si4721_get_rx_tune_status(UC cancel, UC intack);
static SC si4721_get_tx_tune_status(UC intack);
static SC si4721_rx_mute(UC mute);
static SC si4721_seek_all(UC direction);
static SC si4721_tx_power(UC voltage);
/* For ioctl */
static SC si4721_power_up(en_power_mode power_mode);
static SC si4721_power_down(void);
static SI si4721_set_configuration(US frequency);
static void si4721_set_region(area_code *area);
static SI si4721_set_frequency(US frequency);
static SI stmp3750_step_updown(UC direction);
static SI stmp3750_auto_updown(UC direction);
static SI stmp3750_get_rds(UC *buffer);
/* File operations */
static SI stmp37xx_fmtuner_open(struct inode *inode, struct file *file);
static SI stmp37xx_fmtuner_release(struct inode *inode, struct file *file);
static SI stmp37xx_fmtuner_ioctl(struct inode *inode, struct file *filp, UI cmd, UL arg);

/**
 * @brief         enable fm tuner module by setting pins and reset tuner  
 */
static void si4721_enable(void)
{
	/* Pin setup { */
	stmp37xx_gpio_set_level(BANK0_PIN10, GPIO_VOL_33);
	mdelay(1);
	
	stmp37xx_gpio_set_dir(BANK0_PIN10, GPIO_DIR_OUT);
	stmp37xx_gpio_set_dir(BANK0_PIN11, GPIO_DIR_OUT);

	stmp37xx_gpio_set_level(BANK0_PIN11, GPIO_VOL_18);

	mdelay(1);
	/* Pin setup } */
	
	/* Reset condition { */
	RESET_LOW;
	udelay(100);
	RESET_HIGH;
	udelay(30);
	/* Reset condition } */
}
/**
 * @brief         disable fm tuner module
 */
static void si4721_disable(void)
{
	stmp37xx_gpio_set_dir(BANK0_PIN10, GPIO_DIR_IN);
	stmp37xx_gpio_set_dir(BANK0_PIN11, GPIO_DIR_IN);	

	RESET_LOW;
}
/**
 * @brief         wait for Clear to Send timming
 * @return        notifies error
 */
static SI si4721_wait_for_cts(void)
{
	US i = 1000;
	UC cts_status;

	sw_i2c_read(1, &cts_status);

	while (!(cts_status & CTS) && --i) {
		sw_i2c_read(1, &cts_status);
		udelay(500);
	}

	/* For notifing error { */
	if (!(cts_status & CTS) && i == 0) {
		return RETURN_NG;
	}
	/* For notifing error } */

	return RETURN_OK;
}
/**
 * @brief         sends a command and returns the reply dat
 * @param         cmd_size: byte size of command and it's parameter
 * @param         cmd: byte stream that stores command and parameter
 * @param         reply_size: byte size of reply stream
 * @param         reply: reply data stream
 * @return        notifies error
 */
static SC si4721_command(UC cmd_size, UC *cmd, UC reply_size, UC *reply)
{
	if (si4721_wait_for_cts() == RETURN_NG) {
		return RETURN_NG;
	}

	sw_i2c_write(cmd_size, cmd);

	if (si4721_wait_for_cts() == RETURN_NG) {
		return RETURN_NG;
	}

	if (reply_size) {
		sw_i2c_read(reply_size, reply);
	}

	return RETURN_OK;
}
/**
 * @brief         set the property to the given valu
 * @param         property: ID of property
 * @param         value: value of the property
 * @return        notifies error
 */
static SC si4721_set_property(US property, US value)
{
	command_data[0] = SET_PROPERTY;
	
	command_data[1] = 0;  /* Reserved section */
	
	/* Put the property number in the third and fourth bytes. */
	command_data[2] = (UC)(property >> 8);
	command_data[3] = (UC)(property & 0x00FF);
	
	/* Put the property value in the fifth and sixth bytes. */
	command_data[4] = (UC)(value >> 8);
	command_data[5] = (UC)(value & 0x00FF);
	
	//udelay(300); /* tCTS for SET_PROPERTY command */
	mdelay(10); /* tCOMP */
	return si4721_command(6, command_data, 0, NULL);
}
/**
 * @brief         set configurations
 */
static void si4721_configure(void)
{
	if (power_mode == POWER_UP_RX) {
		/* Enable the RDS and STC interrupt  */
		si4721_set_property(GPO_IEN, GPO_IEN_STCIEN_MASK | GPO_IEN_RDSIEN_MASK);
		
		/* Set SNR/RSSI thresholds */
		si4721_set_property(FM_SEEK_TUNE_SNR_THRESHOLD, 3);
		si4721_set_property(FM_SEEK_TUNE_RSSI_THRESHOLD, FM_RSSI_THRESHOLD);
		
		/*  Turn off the mute */
		si4721_set_property(RX_HARD_MUTE, 0);
		/* Set the volume to the passed value */
		si4721_set_property(RX_VOLUME, (SI) default_volume & RX_VOLUME_MASK);
	} else {
		/* Enable the STC interrupt  */
		si4721_set_property(GPO_IEN, GPO_IEN_STCIEN_MASK);
		si4721_set_property(TX_COMPONENT_ENABLE, 
				    TX_COMPONENT_ENABLE_PILOT_MASK |
				    TX_COMPONENT_ENABLE_LMR_MASK |  /* Left minus right */
				    TX_COMPONENT_ENABLE_RDS_MASK);
		
		/* The following properties setup the deviation of the different components */
		si4721_set_property(TX_AUDIO_DEVIATION, 6550);
		si4721_set_property(TX_PILOT_DEVIATION,  750);
		si4721_set_property(TX_RDS_DEVIATION,    200);

		/* Set the preemphasis to 75us */
		si4721_set_property(TX_PREEMPHASIS, TX_PREEMPHASIS_75US);
		
	}
}
/**
 * @brief         tunning FM with given frequency
 * @param         frequency: frequency for tunning
 * @return        notifies error
 */
static SC si4721_rx_tune(US frequency) 
{
	int ret;

	command_data[0] = FM_TUNE_FREQ;

	command_data[1] = 0;  /* Reserved section */

	/* Put the frequency in the second and third bytes. */
	command_data[2] = (UC)(frequency >> 8);
	command_data[3] = (UC)(frequency & 0x00FF);
	
	/* Set the antenna calibration value. */
	command_data[4] = (UC) 0;   /* Auto */
	
	ret = si4721_command(5, command_data, 1, reply_data);

	//udelay(300);
	mdelay(60); /* tCOMP */
	return ret;
}
/**
 * @brief         tunning TX with given frequency
 * @param         frequency: frequency for tunning
 * @return        notifies error
 */
static SC si4721_tx_tune(US frequency) 
{
	int ret;

	command_data[0] = TX_TUNE_FREQ;

	command_data[1] = 0;  /* Reserved section */

	/* Put the frequency in the second and third bytes. */
	command_data[2] = (UC)(frequency >> 8);
	command_data[3] = (UC)(frequency & 0x00FF);
	
	ret = si4721_command(4, command_data, 1, reply_data);

	udelay(300);

	return ret;
}
/**
 * @brief         read interrupt status
 * @return        bit stream for interrupt status
 */
static SC si4721_get_interrupt_status(void) 
{
	SC error;

	command_data[0] = GET_INT_STATUS;

	error = si4721_command(1, command_data, 1, reply_data);

	if (error == RETURN_NG) {
		reply_data[0] = 0x01;
	}

	return reply_data[0];
}
/**
 * @brief         read RX tune statu
 * @param         cnacel: cancel seek
 * @param         intack: clear seek/tune interrupt
 * @return        notifies error
 */
static SC si4721_get_rx_tune_status(UC cancel, UC intack)
{
	SC error;

	command_data[0] = FM_TUNE_STATUS;

	command_data[1] = 0;  /* Reserved section */

	if (cancel) {  /* Cancel seek */
		command_data[1] |= FM_TUNE_STATUS_IN_CANCEL;
	}
	if (intack) {  /* Seek/Tune interrupt clear */
		command_data[1] |= FM_TUNE_STATUS_IN_INTACK;
	}

	error = si4721_command(2, command_data, 8, reply_data);

	/* Parse the results */
	status.rx.STC    = !!(reply_data[0] & STCINT);
	status.rx.BLTF   = !!(reply_data[1] & FM_TUNE_STATUS_OUT_BTLF);
	status.rx.AFCRL  = !!(reply_data[1] & FM_TUNE_STATUS_OUT_AFCRL);
	status.rx.Valid  = !!(reply_data[1] & FM_TUNE_STATUS_OUT_VALID);
	status.rx.Freq   = ((US) reply_data[2] << 8) | (US) reply_data[3];
	status.rx.RSSI   = reply_data[4];
	status.rx.ASNR   = reply_data[5];
	status.rx.AntCap = reply_data[7];

	udelay(300);

	return error;
}
/**
 * @brief         read TX tune statu
 * @param         intack: clear seek/tune interrupt
 * @return        notifies error
 */
static SC si4721_get_tx_tune_status(UC intack)
{
	SC error;

	command_data[0] = TX_TUNE_STATUS;
	
	command_data[1] = 0;  /* Reserved section */

	if (intack) {  /* Seek/Tune interrupt clear */
		command_data[1] |= TX_TUNE_STATUS_IN_INTACK;
	}

	error = si4721_command(2, command_data, 8, reply_data);

	/* Parse the results */
	status.tx.STC    = !!(reply_data[0] & STCINT);
	status.tx.Freq   = ((US)reply_data[2] << 8) | (US)reply_data[3];
	status.tx.Voltage= reply_data[5];
	status.tx.AntCap = reply_data[6];   
	status.tx.RNL    = reply_data[7];

	udelay(300);

	return error;
}
/**
 * @Brief         set mute for RX
 * @param         mute: mute or not
 * @return        notifies error
 */
static SC si4721_rx_mute(UC mute) 
{
	return si4721_set_property(RX_HARD_MUTE, (RX_HARD_MUTE_RMUTE_MASK | RX_HARD_MUTE_LMUTE_MASK) * mute);
}
/**
 * @Brief         wait until the seek/tune is completed
 * @return        notifies error
 */
__inline static void si4721_wait_for_stc(void) 
{

	US i = 1000;

	while (WAIT_STC_INTERRUPT && --i) {
		udelay(1);
	}   /* Wait for interrupt to clear the bit */
	
	mdelay(5);
	
	while (!(si4721_get_interrupt_status() & STCINT));  /* Wait for interrupt to clear the bit */
}
/**
 * @brief         seek valid channel automatically with given direction
 * @param         direction: indicates direction for search
 * @return        notifies error
 */
static SC si4721_seek_all(UC direction) 
{
	SC error;

	direction <<= 3; /* FM_SEEK_START_IN_SEEKUP == 0x08 */

	command_data[0] = FM_SEEK_START;
	
	command_data[1] = 0;  /* Reserved section */
	command_data[1] |= FM_SEEK_START_IN_SEEKUP & direction;
	command_data[1] |= FM_SEEK_START_IN_WRAP;  /* Don't reports BLTF */

	error = si4721_command(2, command_data, 1, reply_data);
	
	if (error == RETURN_NG) {
		return error;
	}
	
	si4721_wait_for_stc(); /* Waiting STC */

	error = si4721_get_rx_tune_status(0, 1);

	if (error == RETURN_NG) {
		return error;
	}

	return status.rx.BLTF;
}
/**
 * @brief         set ouput power level for TX
 * @param         voltage: set output power level
 * @return        notifies error
 */
static SC si4721_tx_power(UC voltage) 
{
	command_data[0] = TX_TUNE_POWER;
	
	command_data[1] = 0;  /* Reserved section */
	command_data[2] = 0;  /* Reserved section */

	command_data[3] = voltage;  /* Set TX voltage */

	/* Set the chip to automatically calculate the antenna cap. */
	command_data[4] = 0;

	return si4721_command(5, command_data, 1, reply_data);
}
/**
 * @brief         power up the SI4721 RX/TX module
 * @param         mode: power up mode, RX or TX
 * @return        notifies error
 */
static SC si4721_power_up(en_power_mode mode)
{
	SC error;

	power_mode = mode;

	si4721_power_down();

	command_data[0] = POWER_UP;

	/* POWER_UP_IN_FUNC_FMRX or POWER_UP_IN_FUNC_FMTX */
	command_data[1] = power_mode;
	//command_data[1] |= POWER_UP_IN_XOSCEN;  /* Enable crystal osc */
        command_data[1] |= POWER_UP_IN_GPO2OEN; /* For determine RDS timming */
	//command_data[1] |= POWER_UP_IN_FUNC_QUERY;

	/* POWER_UP_IN_OPMODE_RX_ANALOG(0x05) or POWER_UP_IN_OPMODE_TX_ANALOG(0x50) */
	command_data[2] = POWER_UP_IN_OPMODE_RX_ANALOG << (power_mode * 2); 

	error = si4721_command(3, command_data, 8, reply_data);

	//mdelay(POWERUP_TIME);  /* wait for si4721 to powerup */

	tuner_is_on = true;

	if ((command_data[1] & POWER_UP_IN_FUNC_QUERY) == POWER_UP_IN_FUNC_QUERY ) {
		printk("[Part Number \"%d\"]\n", reply_data[1]);
		printk("[FW Major Rev \"%d\"]\n", reply_data[2]);
		printk("[FW Minor Rev \"%d\"]\n", reply_data[3]);
		printk("[reserved \"%d\"]\n", reply_data[4]);
		printk("[reserved \"%d\"]\n", reply_data[5]);
		printk("[Chip rev \"%d\"]\n", reply_data[6]);
		printk("[Library ID \"%d\"]\n", reply_data[7]);
	}
	return error;
}
/**
 * @brief         power down the SI4721 RX/TX module
 * @return        notifies error
 */
static SC si4721_power_down(void) 
{
	SC error = RETURN_OK;

	if (tuner_is_on) {	
		command_data[0] = POWER_DOWN;
		error = si4721_command(1, command_data, 0, NULL);
		tuner_is_on = false;
	}

	return error;
}
/**
 * @brief         sets several configuration and then tune RX/TX with give frequency
 * @param         frequency: the frequency to be tuned
 * @return        notifies error
 */
static SI si4721_set_configuration(US frequency) 
{
	si4721_configure();

	if (power_mode == POWER_UP_RX) {
		si4721_set_region(&default_area);
	}

	return si4721_set_frequency(frequency);
}
/**
 * @brief         sets region information
 * @param         region: the region to be set
 * @return        notifies error
 */
static void si4721_set_region(area_code *area) 
{
	/* Set frequency ragne and step */
	switch(area->region) {
	case FM_REGION_EUROPE:
		/* FM_FREQUENCY_MAX = 10800; */
		/* FM_FREQUENCY_MIN = 9990; */
		/* FM_FREQUENCY_STEP = 10; */
		if (area->contry != FM_CONTRY_KOREA_USA) {
			//FM_FREQUENCY_STEP = 10;
		}
		break;
	default:
		if (area->contry == FM_CONTRY_JAPAN) {
			FM_FREQUENCY_MAX = 9000;
			FM_FREQUENCY_MIN = 7600;
			//FM_FREQUENCY_STEP = 5;
		}
		break;
	}

	/* Set contry information */
	switch(area->contry) {
	case FM_CONTRY_JAPAN:
		si4721_set_property(FM_RDS_CONFIG, 0);              /* Disable RDS */
		si4721_set_property(FM_DEEMPHASIS, FM_DEEMPH_50US); /* Deemphasis timming */
		si4721_set_property(FM_SEEK_BAND_BOTTOM, 7600);     /* 76 MHz Bottom */
		si4721_set_property(FM_SEEK_BAND_TOP, 9000);        /* 90 MHz Top */
		si4721_set_property(FM_SEEK_FREQ_SPACING, 10);      /* 100 kHz Spacing */
		break;
	case FM_CONTRY_OTHERS:
		si4721_set_property(FM_RDS_INTERRUPT_SOURCE, 
				    FM_RDS_INTERRUPT_SOURCE_SYNCFOUND_MASK); /* RDS Interrupt */
		
		/* Enable the RDS and allow all blocks so we can compute the error rate later. */
		si4721_set_property(FM_RDS_CONFIG,
				    FM_RDS_CONFIG_RDSEN_MASK |
				    (3 << FM_RDS_CONFIG_BLETHA_SHFT) |  /* Block Error Threshold BLOCKA. */
				    (3 << FM_RDS_CONFIG_BLETHB_SHFT) |  /* Block Error Threshold BLOCKB. */
				    (3 << FM_RDS_CONFIG_BLETHC_SHFT) |  /* Block Error Threshold BLOCKC. */
				    (3 << FM_RDS_CONFIG_BLETHD_SHFT));  /* Block Error Threshold BLOCKD. */
		
		si4721_set_property(FM_DEEMPHASIS, FM_DEEMPH_50US); /* Deemphasis */
		/* Band is already set to 87.5-107.9MHz (Europe) */
		si4721_set_property(FM_SEEK_FREQ_SPACING, 10);      /* 100 kHz Spacing */
		break;
	case FM_CONTRY_KOREA_USA:
	default:
		si4721_set_property(FM_RDS_INTERRUPT_SOURCE, 
				    FM_RDS_INTERRUPT_SOURCE_SYNCFOUND_MASK); /* RDS Interrupt */
		
		/* Enable the RDS and allow all blocks so we can compute the error rate later. */
		si4721_set_property(FM_RDS_CONFIG,
				    FM_RDS_CONFIG_RDSEN_MASK |
				    (3 << FM_RDS_CONFIG_BLETHA_SHFT) |  /* Block Error Threshold BLOCKA. */
				    (3 << FM_RDS_CONFIG_BLETHB_SHFT) |  /* Block Error Threshold BLOCKB. */
				    (3 << FM_RDS_CONFIG_BLETHC_SHFT) |  /* Block Error Threshold BLOCKC. */
				    (3 << FM_RDS_CONFIG_BLETHD_SHFT));  /* Block Error Threshold BLOCKD. */
		
		si4721_set_property(FM_DEEMPHASIS, FM_DEEMPH_75US); /* Deemphasis */
		/* Band is already set to 87.5-107.9MHz (US) */
		/* Space is already set to 200kHz (US) */
		break;
	}
}
/**
 * @brief         sets frequency and tune
 * @param         frequency: the frequency to be set
 * @return        notifies error
 */

static SI si4721_set_frequency(US frequency) 
{
	SI error;

	if (is_tunning == true) {
		printk("now tunning....return\n");
		return RETURN_NG;
	}
	
	is_tunning = true;

	if (power_mode == POWER_UP_RX) {
		error  = si4721_rx_tune(frequency);
	} else {
		error = si4721_tx_tune(frequency);
	}

	if (error == RETURN_NG) {
		is_tunning = false;
		return error;
	}

	si4721_wait_for_stc();

	if (power_mode == POWER_UP_RX) {
		si4721_get_rx_tune_status(0, 1);
		is_tunning = false;

		return status.rx.Valid;
	} else {
		si4721_get_tx_tune_status(1);

		si4721_tx_power(default_tx_voltage); 

		si4721_wait_for_stc();

		si4721_get_tx_tune_status(1);
		
		is_tunning = false;

		return status.tx.RNL;
	}
}
/**
 * @brief         step up/down frequency with fixed scale
 * @param         direction: direction for tune
 * @return        notifies error
 */
static SI stmp3750_step_updown(UC direction) 
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

	return si4721_set_frequency(default_frequency);
}
/**
 * @brief         auto up/down search
 * @param         direction: direction for tune
 * @return        notifies error
 */
static SI stmp3750_auto_updown(UC direction) 
{
	UC band_limit;
	SI error;

	if (is_tunning == true) {
		printk("now tunning....return\n");
		return RETURN_NG;
	}
	
	is_tunning = true;

	si4721_rx_mute(true);

	band_limit = si4721_seek_all(direction);

#if 0	/* TODO: if FM_SEEK_START_IN_WRAP is set in SEEK_SATRT command, remove this state { */
	if (band_limit) {  
		if (direction == GO_UP) {
			si4721_set_frequency(FM_FREQUENCY_MIN); /* set frequency with MIN */
		} else {
			si4721_set_frequency(FM_FREQUENCY_MAX); /* set frequency with MAX */
		}
		si4721_seek_all(direction);
	}
#endif	/* if set FM_SEEK_START_IN_WRAP in SEEK_SATRT command remov this state } */

	si4721_get_rx_tune_status(0, 0); /* For get frequency */

	default_frequency = status.rx.Freq;

	error = si4721_rx_mute(false);

	is_tunning = false ;

	if (error == RETURN_NG) {
		return error;
	}

	return status.rx.Freq;
}
/**
 * @brief         get RDS data if exist
 * @param         buffer: buffer to store RDS data
 * @return        notifies error
 */
static SI stmp3750_get_rds(UC *buffer) 
{
	UC rds_flag;
	UC program_type;

	/* BANK0_PIN11 will high if the RDS exist */
	rds_flag = stmp37xx_gpio_get_level(BANK0_PIN11);

	if (rds_flag) {
		sw_i2c_read(12, &rds_data[0]);

		/* To avoid warning message form compiler */
		program_type = copy_to_user((UC *) buffer, rds_data, sizeof(rds_data));

		program_type = (((rds_data[6] & 0x03) << 3) | ((rds_data[7] & 0xe0) >> 5)) & 0x1f;
		printk("Get RDS buffer program type =(%d)\n", program_type);
	}
	return rds_flag;
}
/**
 * @brief         init buses and then, reset SI4721
 * @return        notifies error
 */
static int stmp37xx_fmtuner_open(struct inode *inode, struct file *file) 
{
#ifdef DCDC_MODE_PFM
	HW_POWER_MINPWR_SET(2) /* PFM */
#else
#endif
#ifdef DCDC_CLK_750K
	HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK);
#else
#endif

	sw_i2c_init(FM_SI4721_CHIPID);

	si4721_enable();

	printk("SI4721 FM module is opened \n"); 

	return RETURN_OK;
}
/**
 * @brief         power down SI4721 and terminate buses
 * @return        notifies error
 */
static int stmp37xx_fmtuner_release(struct inode *inode, struct file *file)
{
	si4721_power_down();

	si4721_disable();

	sw_i2c_term();

	tuner_is_on = false;

	printk("SI4721 FM module is closed\n");

#ifdef DCDC_MODE_PFM
	HW_POWER_MINPWR_CLR(2); /* PWM */
#else
#endif
#ifdef DCDC_CLK_750K
	HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC_HALFCLK);
#else
#endif
	return RETURN_OK;
}
/**
 * @brief         ioctl operation
 * @return        notifies error
 */
static SI stmp37xx_fmtuner_ioctl(struct inode *inode, struct file *filp, UI cmd, UL arg)
{
	UI ret = RETURN_OK;
	UL val;
	area_code *area_p;

	switch(cmd) {
	case FM_RX_POWER_UP:
		si4721_power_up(POWER_UP_RX);
		break;
	case FM_TX_POWER_UP:
		si4721_power_up(POWER_UP_TX);
		break;
	case FM_POWER_DOWN:
		si4721_power_down();
		break;
	case FM_SET_CONFIGURATION:
		if (get_user(val, (SI *) arg)) {
			return -EINVAL;
		}
		ret = si4721_set_configuration(val);
		break;
	case FM_SET_REGION:
		area_p = (area_code *) arg;
		/* To avoid warning message from compiler */
		ret = copy_from_user(&default_area, area_p, sizeof(area_code));
		si4721_set_region(&default_area);
		ret = RETURN_OK;
		break;
	case FM_SET_FREQUENCY:
		if (get_user(val, (SI *) arg)) {
			return -EINVAL;
		}
		default_frequency = val;
		ret = si4721_set_frequency(default_frequency);
		break;
	case FM_GET_FREQUENCY:
		put_user(status.rx.Freq, (long *) arg);
		ret = RETURN_OK;
		break;
	case FM_STEP_UP:
		ret = stmp3750_step_updown(GO_UP);
		break;
	case FM_STEP_DOWN:
		ret = stmp3750_step_updown(GO_DOWN);
		break;
	case FM_AUTO_UP:
		ret = stmp3750_auto_updown(GO_UP);
		break;
	case FM_AUTO_DOWN:
		ret = stmp3750_auto_updown(GO_DOWN);
		break;
	case FM_GET_RDS_DATA:
		ret = stmp3750_get_rds((UC *) arg);
		break;
	case FM_IS_TUNED:
		ret =  status.rx.Valid;
		break;
	case FM_SET_FREQUENCY_STEP:
		if (get_user(val, (UC *) arg)) {
			return -EINVAL;
		}
		FM_FREQUENCY_STEP = val;
		ret = RETURN_OK;
		break;
	case FM_SET_RSSI:
		if (get_user(val, (UC *) arg)) {
			return -EINVAL;
		}
		if (val < FM_RSSI_MIN + 1 || val >  FM_RSSI_MAX) {
			val = FM_RSSI_DEF;
		}

		FM_RSSI_THRESHOLD = val;
		ret = RETURN_OK;
		break;		
	default:
		printk("%s : no mached ioctl found\n", __func__);
		break;
	}
	return ret;
}

static struct file_operations fmtuner_fops = {
	.owner		= THIS_MODULE,
	.open		= stmp37xx_fmtuner_open,
	.release	= stmp37xx_fmtuner_release,
	.ioctl		= stmp37xx_fmtuner_ioctl,
};

#ifdef CONFIG_PROC_FS
/**
 * @brief         read proc
 */
static SI fm_read_proc(char *page, char **start, off_t off, SI count, SI *eof, void *data) 
{
	SI length = 0;
	
	if (off != 0) {
		return 0;
	}

	*start = page;

	length += sprintf(page, "SI4721 FM Tuner driver\n");

	if (power_mode == POWER_UP_RX) {
		length += sprintf(page + length, "  AFCRL: %d\n", status.rx.AFCRL);
		length += sprintf(page + length, "  Valid: %d\n", status.rx.Valid);
		length += sprintf(page + length, "  Freq: %d\n", status.rx.Freq);
		length += sprintf(page + length, "  RSSI: %d\n", status.rx.RSSI);
		length += sprintf(page + length, "  ASNR: %d\n", status.rx.ASNR);
		length += sprintf(page + length, "  AntCap: %d\n", status.rx.AntCap);
	} else {
		length += sprintf(page + length, "  STC: %d\n", status.tx.STC);
		length += sprintf(page + length, "  Voltage: %d\n", status.tx.Voltage);
		length += sprintf(page + length, "  Freq: %d\n", status.tx.Freq);
		length += sprintf(page + length, "  AntCap: %d\n", status.tx.AntCap);
		length += sprintf(page + length, "  RNL: %d\n", status.tx.RNL);
	}

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
	  i = i*10 + *(s++) - '0';
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

	sscanf(buf, "%s %s", c0, c1);

	if (!strcmp(c0, "Freq")) {
		value = atoi(c1);
		si4721_set_frequency(value);
	}

	return count;
}
#endif
/**
 * @brief         module init function
 */
static SI __init stmp37xx_fmtuner_si4721_init(void) 
{
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *proc_entry;

	proc_entry = create_proc_entry("fm", S_IWUSR | S_IRUGO, NULL);
	proc_entry->read_proc = fm_read_proc;
	proc_entry->write_proc = fm_write_proc;
	proc_entry->data = NULL;
#endif	
	printk("stmp37xx_FMTuner : %s\n", __func__);

	return register_chrdev(FMTUNER_MAJOR, "Si4721", &fmtuner_fops);
}
/**
 * @brief         module exit function
 */
static void __exit stmp37xx_fmtuner_si4721_exit(void) 
{
#ifdef CONFIG_PROC_FS
	remove_proc_entry("fm", NULL);
#endif

	printk("stmp37xx_FMTuner : %s\n", __func__);

	unregister_chrdev(FMTUNER_MAJOR, "Si4721");
}

module_init(stmp37xx_fmtuner_si4721_init);
module_exit(stmp37xx_fmtuner_si4721_exit);

MODULE_DESCRIPTION("SI4721 FM Tuner RX/TX driver");
MODULE_AUTHOR("Samsung MP Group");
MODULE_LICENSE("GPL");
