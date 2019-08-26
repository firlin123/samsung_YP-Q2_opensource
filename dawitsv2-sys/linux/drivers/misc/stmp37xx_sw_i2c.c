/** 
 * @file        stmp37xx_sw_i2c.c
 * @brief       Implementation of I2C protocol for SI4703 FM Tuner
 */
#include <linux/kernel.h>
#include <asm-arm/arch-stmp37xx/gpio.h>
#include <linux/delay.h>

#include <asm-arm/arch-stmp37xx/37xx/regsdigctl.h>

#include "stmp37xx_sw_i2c.h"

#define BYTE unsigned char

/************************************************
 * Macro definition
 ************************************************/
#define PIN_SCL BANK2_PIN5
#define PIN_SDA BANK2_PIN6

#define FM_I2C_SCL_GPIO        stmp37xx_gpio_set_af(PIN_SCL, GPIO_AF3)
#define FM_I2C_SCL_OUT         stmp37xx_gpio_set_dir(PIN_SCL, GPIO_DIR_OUT)
#define FM_I2C_SCL_IN          stmp37xx_gpio_set_dir(PIN_SCL, GPIO_DIR_IN)
#define FM_I2C_SCL_HIGH        stmp37xx_gpio_set_level(PIN_SCL, GPIO_VOL_33)
#define FM_I2C_SCL_LOW         stmp37xx_gpio_set_level(PIN_SCL, GPIO_VOL_18)

#define FM_I2C_SDA_GPIO        stmp37xx_gpio_set_af(PIN_SDA, GPIO_AF3)
#define FM_I2C_SDA_OUT         stmp37xx_gpio_set_dir(PIN_SDA, GPIO_DIR_OUT)
#define FM_I2C_SDA_IN          stmp37xx_gpio_set_dir(PIN_SDA, GPIO_DIR_IN)
#define FM_I2C_SDA_HIGH        stmp37xx_gpio_set_level(PIN_SDA, GPIO_VOL_33)
#define FM_I2C_SDA_LOW         stmp37xx_gpio_set_level(PIN_SDA, GPIO_VOL_18)

#define GET_FM_SDA             stmp37xx_gpio_get_level(PIN_SDA)

#define FM_I2C_BYTE_DELAY      fm_udelay(6) //fm_udelay(3)
#define FM_I2C_CLOCK_DELAY     fm_udelay(4) //fm_udelay(2)

/************************************************
 * Global variabe definition
 ************************************************/
unsigned char FM_CHIPID;
/************************************************
 * Local function declearation
 ************************************************/
static void SW_I2C_START(void);
static void SW_I2C_SEND_ID(unsigned char ID, unsigned char RW);
static char SW_I2C_ACK(void);
static void SW_I2C_send_ACK(char N_ACK);
static void SW_I2C_sendDATA(unsigned char data_out);
static BYTE SW_I2C_readDATA(void);
static void SW_I2C_stop(void);
/**
 * @brief         Set pin mux for I2C communication
 */
void sw_i2c_init(unsigned char address){
	FM_CHIPID = address;

	FM_I2C_SCL_GPIO;
	FM_I2C_SDA_GPIO;

	FM_I2C_SCL_OUT;
	FM_I2C_SDA_OUT;

	FM_I2C_SCL_LOW;
	FM_I2C_SDA_LOW;
}
/**
 * @brief         Restore pin mux
 */
void sw_i2c_term(void) {
	FM_I2C_SCL_LOW;
	FM_I2C_SDA_LOW;

	FM_I2C_SCL_IN;
	FM_I2C_SDA_IN;
}
/**
 * @brief         Set start condition
 */
static void SW_I2C_START(void) {
	/* Start condition */
	FM_I2C_SDA_OUT;	

	FM_I2C_SCL_HIGH;

	FM_I2C_SDA_HIGH;
		
	FM_I2C_CLOCK_DELAY;
	
	FM_I2C_SDA_LOW;	

	FM_I2C_CLOCK_DELAY;

	FM_I2C_SCL_LOW;		

	FM_I2C_CLOCK_DELAY;
}
/**
 * @brief         Send device ID
 */
static void SW_I2C_SEND_ID(BYTE ID, BYTE RW)
{
	char i;
	BYTE temp;
	
	/* Combine 7bit ID and 1bit RW. */
	if(RW == 0)
		temp = (ID << 1) & 0xFE;
	else
		temp = (ID << 1) | 0x01;
	
	/* Send serialized data... */
	for(i = 8; i > 0; i--) {
		if((temp >> (i - 1) & 0x01) == 0)
			FM_I2C_SDA_LOW;	
		else
			FM_I2C_SDA_HIGH;
		
		FM_I2C_CLOCK_DELAY;
		
		FM_I2C_SCL_HIGH;	
		
		FM_I2C_CLOCK_DELAY;
		FM_I2C_SCL_LOW;	
	}

	FM_I2C_SDA_LOW;
}
/**
 * @brief         Get ack
 */
static char SW_I2C_ACK()
{
	BYTE wait_cnt= 0;
	BYTE ack_check = 0;
	
	FM_I2C_SDA_IN;

	FM_I2C_SCL_HIGH;	

	FM_I2C_CLOCK_DELAY;
	
	ack_check = GET_FM_SDA;	

	FM_I2C_SCL_LOW;

	FM_I2C_CLOCK_DELAY;

	while(1) {
		if(!ack_check)		
			break;			
		else
			wait_cnt++;

		ack_check = GET_FM_SDA;

		if(wait_cnt >= 120) {
			FM_I2C_SDA_OUT;

			FM_I2C_CLOCK_DELAY;
				
			FM_I2C_SDA_LOW;

			FM_I2C_CLOCK_DELAY;
				
			FM_I2C_SCL_LOW;
			
			FM_I2C_CLOCK_DELAY;	
			
			return -1;
		}

	}

	FM_I2C_SDA_OUT;
	
	FM_I2C_SDA_LOW;

	return 1;
}
/**
 * @brief         Send ack
 */
static void SW_I2C_send_ACK(char N_ACK)
{
	/* HOST should have to send ACK or NACK signal and make SDATA to output port */
	if(N_ACK == 0)
		FM_I2C_SDA_HIGH;
	else
		FM_I2C_SDA_LOW;

	FM_I2C_SCL_HIGH;

	FM_I2C_CLOCK_DELAY;

	FM_I2C_SCL_LOW;	

	FM_I2C_CLOCK_DELAY;

	FM_I2C_SDA_LOW;
}
/**
 * @brief         Send data
 */
static void SW_I2C_sendDATA(BYTE data_out)
{
	char i;

	for(i = 8; i > 0; i--) {
		if((data_out >> (i - 1) & 0x01) == 0)
			FM_I2C_SDA_LOW;
		else
			FM_I2C_SDA_HIGH;

		/* Needed to avoid ack error from device */
		FM_I2C_CLOCK_DELAY; 

		FM_I2C_SCL_HIGH;

		FM_I2C_CLOCK_DELAY;

		FM_I2C_SCL_LOW;

		FM_I2C_CLOCK_DELAY;
	}

	FM_I2C_SDA_LOW;
}
/**
 * @brief         Read data
 */
static BYTE SW_I2C_readDATA()
{
	BYTE i;
	BYTE read_data = 0;

	FM_I2C_SDA_IN;
	
	FM_I2C_BYTE_DELAY;
	
	for(i=0 ; i < 8; i++) {
		FM_I2C_SCL_HIGH;
		
		FM_I2C_CLOCK_DELAY;

		if(!GET_FM_SDA)
			read_data = read_data | (read_data & ~0x01);
		else
			read_data = read_data | 0x01;

		if(i < 7)
			read_data = read_data << 1;
	
		FM_I2C_SCL_LOW;

		FM_I2C_CLOCK_DELAY;
	}
	
	FM_I2C_SDA_OUT;

	FM_I2C_SDA_LOW;

	return read_data;
}
/**
 * @brief         Stop I2C
 */
static void SW_I2C_stop()
{
	/* Generate Stop signal */
	FM_I2C_SCL_HIGH;
	
	FM_I2C_CLOCK_DELAY;

	FM_I2C_SDA_HIGH;
}
/**
 * @brief         Read data API
 */
char sw_i2c_read( BYTE length, BYTE *data_read) 
{
	BYTE temp;

	/* Step 1: Send START Signal. */
	SW_I2C_START();
	FM_I2C_BYTE_DELAY;

	/* Step 2: Write Device Address and direction = 0. */
	SW_I2C_SEND_ID((BYTE)FM_CHIPID, (BYTE)(I2C_READ));
	FM_I2C_BYTE_DELAY;

	/* Step 3: Wait for DONE = 1. And after that check If BERR = 1 or ACK = 0, terminate by setting STOP = 1. */
	if(SW_I2C_ACK() != 1) {  /* When ACK is '0' to STOP process. */
		printk("Chip ID write: read mode error\n");	
		SW_I2C_stop();
		return -1;
	}
	FM_I2C_BYTE_DELAY;

	for(temp = 0; temp< length; temp++) {
		/* Step 4: Read the data from I2DAT. */
		data_read[temp] = SW_I2C_readDATA();

		FM_I2C_BYTE_DELAY;
	
		if((temp == (length - 1)))
			SW_I2C_send_ACK(0);
		else
			SW_I2C_send_ACK(1);

	}

	FM_I2C_BYTE_DELAY;
	
	/* Step 4: Set STOP = 1. */
	SW_I2C_stop();

	return 1;
}
/**
 * @brief         Write data API
 */
char sw_i2c_write(BYTE length, BYTE *data_write) 
{
	BYTE temp;

	/* Step 1: Send START = 1. */
	FM_I2C_BYTE_DELAY;
	SW_I2C_START();
	FM_I2C_BYTE_DELAY;

	/* Step 2: Write Device Address and direction = 0. */
	SW_I2C_SEND_ID((BYTE)FM_CHIPID, (BYTE)(I2C_WRITE));
	FM_I2C_BYTE_DELAY;
	/* Step 3: Wait for DONE = 1. If BERR = 1 or ACK = 0, terminate by setting STOP = 1. */
	if(SW_I2C_ACK() != 1) {
		SW_I2C_stop();
		printk("Chip ID write: write mode error\n");			
		return -1;
	}
	FM_I2C_BYTE_DELAY;

	/* Step 4: Write data to I2DAT. */

	if( data_write != NULL)
	for(temp = 0; temp< length; temp++) {
		SW_I2C_sendDATA(data_write[temp]);
		/* Step 5: Wait for DONE = 1. If BERR = 1, terminate by setting STOP = 1. */
		if(SW_I2C_ACK() != 1) {
			SW_I2C_stop();

			return -1;
		}

	}
	FM_I2C_BYTE_DELAY;
	/* Step 6: Set STOP = 1. */
	SW_I2C_stop();

	FM_I2C_BYTE_DELAY;

	return 1;
}
/**
 * @brief         get elasped time as micro second
 */
unsigned long fm_get_elasped(unsigned long begin)
{
	unsigned long ret = 0;
	unsigned long end = HW_DIGCTL_MICROSECONDS_RD();

	if (begin > end) {
		ret = 0xFFFFFFFF - begin + end;
	} else {
		ret = end - begin;
	}
	return ret;
}
/**
 * @brief         get current time as micro second after boot
 */
unsigned long fm_get_current(void)
{
	return HW_DIGCTL_MICROSECONDS_RD();
}
/**
 * @brief         set dealy with ms
 */
void fm_udelay(unsigned long delay)
{
        unsigned long cur = 0;
        unsigned long end = 0;
 
        cur = fm_get_current();
     
        while(true) {
                end = fm_get_elasped(cur);
                if(end >= delay) {
                        break;
		}
        }    
}
