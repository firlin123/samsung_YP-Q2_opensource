#include <linux/kernel.h>
#include <asm-arm/arch-stmp37xx/gpio.h>
#include <linux/delay.h>

#include "stmp37xx_sw_touchpad_i2c.h"

#define BYTE unsigned char

/************************************************
 * Macro definition
 ************************************************/
#define PIN_SCL BANK0_PIN18
#define PIN_SDA BANK0_PIN9

#define TOUCHPAD_I2C_SCL_GPIO        stmp37xx_gpio_set_af(PIN_SCL, GPIO_AF3)
#define TOUCHPAD_I2C_SCL_OUT         stmp37xx_gpio_set_dir(PIN_SCL, GPIO_DIR_OUT)
#define TOUCHPAD_I2C_SCL_IN          stmp37xx_gpio_set_dir(PIN_SCL, GPIO_DIR_IN)
#define TOUCHPAD_I2C_SCL_HIGH        stmp37xx_gpio_set_level(PIN_SCL, GPIO_VOL_33)
#define TOUCHPAD_I2C_SCL_LOW         stmp37xx_gpio_set_level(PIN_SCL, GPIO_VOL_18)

#define TOUCHPAD_I2C_SDA_GPIO        stmp37xx_gpio_set_af(PIN_SDA, GPIO_AF3)
#define TOUCHPAD_I2C_SDA_OUT         stmp37xx_gpio_set_dir(PIN_SDA, GPIO_DIR_OUT)
#define TOUCHPAD_I2C_SDA_IN          stmp37xx_gpio_set_dir(PIN_SDA, GPIO_DIR_IN)
#define TOUCHPAD_I2C_SDA_HIGH        stmp37xx_gpio_set_level(PIN_SDA, GPIO_VOL_33)
#define TOUCHPAD_I2C_SDA_LOW         stmp37xx_gpio_set_level(PIN_SDA, GPIO_VOL_18)

#define GET_TOUCHPAD_SDA             stmp37xx_gpio_get_level(PIN_SDA)

//#define TOUCHPAD_I2C_BYTE_DELAY      udelay(3)
//#define TOUCHPAD_I2C_CLOCK_DELAY     udelay(2)

#ifdef FAST_ISR_VER
#define TOUCHPAD_I2C_BYTE_DELAY      udelay(30) //dhsong, 20081006,
#define TOUCHPAD_I2C_CLOCK_DELAY     udelay(1)
#else
#define TOUCHPAD_I2C_BYTE_DELAY      udelay(100) //dhsong
#define TOUCHPAD_I2C_CLOCK_DELAY     udelay(10)
#endif

/************************************************
 * Global variabe definition
 ************************************************/
unsigned char TOUCHPAD_CHIPID;
/************************************************
 * Local function declearation
 ************************************************/
static void SW_TOUCHPAD_I2C_START(void);
static void SW_TOUCHPAD_I2C_SEND_ID(unsigned char ID, unsigned char RW);
static char SW_TOUCHPAD_I2C_ACK(void);
static void SW_TOUCHPAD_I2C_send_ACK(char N_ACK);
static void SW_TOUCHPAD_I2C_sendDATA(unsigned char data_out);
static BYTE SW_TOUCHPAD_I2C_readDATA(void);
static void SW_TOUCHPAD_I2C_stop(void);

void sw_touchpad_i2c_init(unsigned char address){
	TOUCHPAD_CHIPID = address;

	TOUCHPAD_I2C_SCL_GPIO;
	TOUCHPAD_I2C_SDA_GPIO;

	TOUCHPAD_I2C_SCL_OUT;
	TOUCHPAD_I2C_SDA_OUT;

	TOUCHPAD_I2C_SCL_LOW;
	TOUCHPAD_I2C_SDA_LOW;
}

void sw_touchpad_i2c_term(void) {
	TOUCHPAD_I2C_SCL_LOW;
	TOUCHPAD_I2C_SDA_LOW;

	TOUCHPAD_I2C_SCL_IN;
	TOUCHPAD_I2C_SDA_IN;
}

static void SW_TOUCHPAD_I2C_START(void) {
	/* Start condition */
	TOUCHPAD_I2C_SDA_OUT;	

	TOUCHPAD_I2C_SCL_HIGH;

	TOUCHPAD_I2C_SDA_HIGH;
		
	TOUCHPAD_I2C_CLOCK_DELAY;
	
	TOUCHPAD_I2C_SDA_LOW;	

	TOUCHPAD_I2C_CLOCK_DELAY;

	TOUCHPAD_I2C_SCL_LOW;		

	TOUCHPAD_I2C_CLOCK_DELAY;
}

static void SW_TOUCHPAD_I2C_SEND_ID(BYTE ID, BYTE RW)
{
	char i;
	BYTE temp;
	
	/* Combine 7bit ID and 1bit RW. */
	if(RW == 0) {
		temp = (ID << 1) & 0xFE;
		//temp = (ID) & 0xFE; //dhsong
		//printk("write temp = 0x%08x\n\n", temp); //dhsong
	}
	else {
		temp = (ID << 1) | 0x01;
		//temp = ID | 0x01; //dhsong
		//printk("read temp = 0x%08x\n\n", temp); //dhsong
	}
	
	/* Send serialized data... */
	for(i = 8; i > 0; i--) {
		if((temp >> (i - 1) & 0x01) == 0)
			TOUCHPAD_I2C_SDA_LOW;	
		else
			TOUCHPAD_I2C_SDA_HIGH;
		
		TOUCHPAD_I2C_CLOCK_DELAY;
		
		TOUCHPAD_I2C_SCL_HIGH;	
		
		TOUCHPAD_I2C_CLOCK_DELAY;
		TOUCHPAD_I2C_SCL_LOW;	
	}

	TOUCHPAD_I2C_SDA_LOW;
}

static char SW_TOUCHPAD_I2C_ACK()
{
	BYTE wait_cnt= 0;
	BYTE ack_check = 0;
	
	TOUCHPAD_I2C_SDA_IN;

	TOUCHPAD_I2C_SCL_HIGH;	

	TOUCHPAD_I2C_CLOCK_DELAY;
	
	ack_check = GET_TOUCHPAD_SDA;	

	TOUCHPAD_I2C_SCL_LOW;

	TOUCHPAD_I2C_CLOCK_DELAY;

	while(1) {
		if(!ack_check)		
			break;			
		else
			wait_cnt++;

		ack_check = GET_TOUCHPAD_SDA;

		if(wait_cnt >= 120) {
			TOUCHPAD_I2C_SDA_OUT;

			TOUCHPAD_I2C_CLOCK_DELAY;
				
			TOUCHPAD_I2C_SDA_LOW;

			TOUCHPAD_I2C_CLOCK_DELAY;
				
			TOUCHPAD_I2C_SCL_LOW;
			
			TOUCHPAD_I2C_CLOCK_DELAY;	
			
			return -1;
		}

	}

	TOUCHPAD_I2C_SDA_OUT;
	
	TOUCHPAD_I2C_SDA_LOW;

	return 1;
}

static void SW_TOUCHPAD_I2C_send_ACK(char N_ACK)
{
	/* HOST should have to send ACK or NACK signal and make SDATA to output port */

	if(N_ACK == 0)
		TOUCHPAD_I2C_SDA_HIGH;
	else
		TOUCHPAD_I2C_SDA_LOW;

	TOUCHPAD_I2C_SCL_HIGH;

	TOUCHPAD_I2C_CLOCK_DELAY;

	TOUCHPAD_I2C_SCL_LOW;	

	TOUCHPAD_I2C_CLOCK_DELAY;

	TOUCHPAD_I2C_SDA_LOW;
}

static void SW_TOUCHPAD_I2C_sendDATA(BYTE data_out)
{
	char i;

	for(i = 8; i > 0; i--) {
		if((data_out >> (i - 1) & 0x01) == 0)
			TOUCHPAD_I2C_SDA_LOW;
		else
			TOUCHPAD_I2C_SDA_HIGH;

		/* Needed to avoid ack error from device */
		TOUCHPAD_I2C_CLOCK_DELAY; 

		TOUCHPAD_I2C_SCL_HIGH;

		TOUCHPAD_I2C_CLOCK_DELAY;

		TOUCHPAD_I2C_SCL_LOW;

		TOUCHPAD_I2C_CLOCK_DELAY;
	}

	TOUCHPAD_I2C_SDA_LOW;
}

static BYTE SW_TOUCHPAD_I2C_readDATA()
{

	BYTE i;
	BYTE read_data = 0;

	TOUCHPAD_I2C_SDA_IN;
	
	//printk("SDA DOE0= 0x%08x\n", HW_PINCTRL_DOE0_RD() >> 9);
	//printk("SDA DOUT0= 0x%08x\n", HW_PINCTRL_DOUT0_RD() >> 9);
	//printk("SDA DIN0= 0x%08x\n\n", HW_PINCTRL_DIN0_RD() >> 9);
	TOUCHPAD_I2C_BYTE_DELAY;
	
	for(i=0 ; i < 8; i++) {
		TOUCHPAD_I2C_SCL_HIGH;
		
		TOUCHPAD_I2C_CLOCK_DELAY;

		if(!GET_TOUCHPAD_SDA)
			read_data = read_data | (read_data & ~0x01);
		else
			read_data = read_data | 0x01;

		if(i < 7)
			read_data = read_data << 1;
	
		TOUCHPAD_I2C_SCL_LOW;

		TOUCHPAD_I2C_CLOCK_DELAY;
	}
	
	TOUCHPAD_I2C_SDA_OUT;

//	printk("SDA DOE0= 0x%08x\n", HW_PINCTRL_DOE0_RD() >> 9);
//	printk("SDA DOUT0= 0x%08x\n", HW_PINCTRL_DOUT0_RD() >> 9);
//	printk("SDA DIN0= 0x%08x\n\n", HW_PINCTRL_DIN0_RD() >> 9);
	TOUCHPAD_I2C_SDA_LOW;

	return read_data;
}

static void SW_TOUCHPAD_I2C_stop()
{
	/* Generate Stop signal */
	TOUCHPAD_I2C_SCL_HIGH;
	
	TOUCHPAD_I2C_CLOCK_DELAY;

	TOUCHPAD_I2C_SDA_HIGH;
}

char sw_touchpad_i2c_read( BYTE length, BYTE *data_read) 
{
	BYTE temp;

	/* Step 1: Send START Signal. */
	SW_TOUCHPAD_I2C_START();
	TOUCHPAD_I2C_BYTE_DELAY;

	/* Step 2: Write Device Address and direction = 0. */
	SW_TOUCHPAD_I2C_SEND_ID((BYTE)TOUCHPAD_CHIPID, (BYTE)(TOUCHPAD_I2C_READ));
	TOUCHPAD_I2C_BYTE_DELAY;

	/* Step 3: Wait for DONE = 1. And after that check If BERR = 1 or ACK = 0, terminate by setting STOP = 1. */
	if(SW_TOUCHPAD_I2C_ACK() != 1) {  /* When ACK is '0' to STOP process. */
		printk("Chip ID write: read mode error\n");	
		SW_TOUCHPAD_I2C_stop();
		return -1;
	}
	TOUCHPAD_I2C_BYTE_DELAY;

	for(temp = 0; temp< length; temp++) {
		/* Step 4: Read the data from I2DAT. */
		data_read[temp] = SW_TOUCHPAD_I2C_readDATA();

		TOUCHPAD_I2C_BYTE_DELAY;
	
		if((temp == (length - 1)))
			SW_TOUCHPAD_I2C_send_ACK(0);
		else
			SW_TOUCHPAD_I2C_send_ACK(1);

	}

	TOUCHPAD_I2C_BYTE_DELAY;
	
	/* Step 4: Set STOP = 1. */
	SW_TOUCHPAD_I2C_stop();

	return 1;

}

char sw_touchpad_i2c_write(BYTE length, BYTE *data_write) 
{

	BYTE temp;

	/* Step 1: Send START = 1. */
	TOUCHPAD_I2C_BYTE_DELAY;
	SW_TOUCHPAD_I2C_START();
	TOUCHPAD_I2C_BYTE_DELAY;

	/* Step 2: Write Device Address and direction = 0. */
	SW_TOUCHPAD_I2C_SEND_ID((BYTE)TOUCHPAD_CHIPID, (BYTE)(TOUCHPAD_I2C_WRITE));
	TOUCHPAD_I2C_BYTE_DELAY;
	/* Step 3: Wait for DONE = 1. If BERR = 1 or ACK = 0, terminate by setting STOP = 1. */
	if(SW_TOUCHPAD_I2C_ACK() != 1) {
		SW_TOUCHPAD_I2C_stop();
		printk("Chip ID write: write mode error\n");			
		return -1;
	}
	TOUCHPAD_I2C_BYTE_DELAY;

	/* Step 4: Write data to I2DAT. */

	if( data_write != NULL)
	for(temp = 0; temp< length; temp++) {
		SW_TOUCHPAD_I2C_sendDATA(data_write[temp]);
		/* Step 5: Wait for DONE = 1. If BERR = 1, terminate by setting STOP = 1. */
		if(SW_TOUCHPAD_I2C_ACK() != 1) {
			SW_TOUCHPAD_I2C_stop();
			printk("ACK fail\n");
			return -1;
		}

	}
	TOUCHPAD_I2C_BYTE_DELAY;
	/* Step 6: Set STOP = 1. */
	SW_TOUCHPAD_I2C_stop();

	TOUCHPAD_I2C_BYTE_DELAY;

	return 1;
}
