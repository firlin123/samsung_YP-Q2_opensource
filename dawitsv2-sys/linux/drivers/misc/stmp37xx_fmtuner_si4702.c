/* \file stmp36xx_fmtuner.c
 * \brief stmp36xx fmtuner read/write
 * \author Choi Yong Joon <yj1017.choi@samsung.com>
 * \version $Revision: 1.56 $
 * \date $Date: 2008/06/23 01:32:03 $
 *
 * This file is \a FMTuner read/write function using i2c in stmp36xx.
 */
 
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/delay.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>

#include <asm/uaccess.h>

#include <linux/dma-mapping.h>
#include <asm-arm/delay.h>
#include <asm-arm/arch-stmp37xx/37xx/regsi2c.h>
#include <asm-arm/arch-stmp37xx/37xx/regsicoll.h>
#include <asm-arm/arch-stmp37xx/37xx/regsapbx.h>
#include <asm-arm/arch-stmp37xx/37xx/regsdigctl.h>
#include <asm-arm/arch-stmp37xx/37xx/regspinctrl.h>
#include <asm-arm/arch-stmp37xx/37xx/regsaudioout.h>
#include <asm-arm/arch-stmp37xx/37xx/regsaudioin.h>

#include <asm-arm/arch-stmp37xx/37xx/regs.h>

#include "stmp37xx_fmtuner_si4702.h"
#include "stmp37xx_i2c.h"

#include <linux/proc_fs.h>

#define FMTUNER_MAJOR	240 			//device file major number
#define I2C_FMTUNER_ADDR	0x10   //(Si4702 I2C address = 0x10 << 1)

#define FMTUNER_WRITE	0
#define FMTUNER_READ	1

#define I2C_FMTUNER_WR		0x00
#define I2C_FMTUNER_RD		0x01

#define FMTUNER_DATA_SIZE	5
#define RESET_TIMEOUT		10

#define copy_to_user(a,b,c)          memcpy_tofs(a,b,c)
#define memcpy_tofs memcpy

typedef enum {
	
	SET_FREQUENCY=0,
	GET_FREQUENCY,
	SET_RSSI_LEVEL,
	Check_Freq_Status,
	SET_REGION,
	MANUAL_FREQ_DOWN,	//test sequence
	MANUAL_FREQ_UP,
	SET_VOL,
	CHECK_MONO_ST,
	FM_POWER_ON,		//2007.10.05 added.
	AUTO_SEARCH_UP,
	AUTO_SEARCH_DOWN,
	CHECK_RDS_MSG,		//2008.03.17
	GET_RDS_BUFFER,				//use "copy_to_user"
	SET_LIMITED_VOL,

	GET_RSSI_LEVEL = 20,	//to test
	GET_AFC_RAIL,
};

typedef enum {
	REGION_KOREA = 0,
	REGION_JAPAN,
	REGION_WORLD
};

typedef unsigned int UI;
typedef signed int SI;
typedef unsigned long UL;
typedef unsigned char UC;
typedef unsigned short US;

#define FM_MAGIC               0xCA
#define FM_SET_REGION          _IOW(FM_MAGIC, 0, void *)
#define FM_STEP_UP             _IO(FM_MAGIC, 1)
#define FM_STEP_DOWN           _IO(FM_MAGIC, 2)
#define FM_AUTO_UP             _IO(FM_MAGIC, 3)
#define FM_AUTO_DOWN           _IO(FM_MAGIC, 4)
#define FM_SET_FREQUENCY       _IOW(FM_MAGIC, 5, UI)
#define FM_GET_FREQUENCY       _IOR(FM_MAGIC, 6, UI)
#define FM_SET_VOLUME          _IOW(FM_MAGIC, 7, UI)
#define FM_SET_RSSI            _IOW(FM_MAGIC, 8, UI)
#define FM_GET_RDS_DATA        _IOR(FM_MAGIC, 9, UC *)
#define FM_RX_POWER_UP         _IO(FM_MAGIC, 10)
#define FM_TX_POWER_UP         _IO(FM_MAGIC, 11)
#define FM_SET_CONFIGURATION   _IOW(FM_MAGIC, 12, US)
#define FM_IS_TUNED            _IO(FM_MAGIC, 13)
#define FM_SET_FREQUENCY_STEP  _IOW(FM_MAGIC, 14, UC)


/********** Variables ***********/

/* dma buffers to hold i2c command string for slave addres+W
   and the second command, a slave address+R */
static unsigned char *FMTuner_write_command_buffer = NULL;		// Virtual Address
static unsigned char *FMTuner_read_command_buffer = NULL;		// Virtual Address
static dma_addr_t write_command_buffer_start_p = 0; 	// Physical Address
static dma_addr_t read_command_buffer_start_p = 0;		// Physical Address

/* DMA read/write buffer for reading/writing to the eeprom */
static unsigned char *FMTuner_data_write_buffer = NULL; 		// Virtual Address
static unsigned char *FMTuner_data_read_buffer = NULL;			// Virtual Address
static dma_addr_t write_buffer_start_p = 0; 			// Physical Address
static dma_addr_t read_buffer_start_p = 0;				// Physical Address

static unsigned long ulFrequency = 9990; //FM frequency
static unsigned short ulChann; //FM channel information



static unsigned long FM_MAX_FREQUENCY = 10800;
static unsigned long FM_MIN_FREQUENCY = 8750;
//static unsigned long FM_FREQUENCY_SCALE = 100000;
static unsigned long FM_FREQUENCY_SCALE = 10;

static unsigned short tuned_flag = 0;

static unsigned short setted_region = REGION_WORLD;

static union Si470X_Register SI470X;
static unsigned char SI47XX[16];

static unsigned short usRSSI_LEVEL;
static unsigned int uiFreq_status;

static unsigned int iFreq_RSSI;
static unsigned int iFreq_AFC;

unsigned char LimitedVol=FALSE;
unsigned char PowerOnDone=FALSE;
	

enum {
	MUTE_OFF = 0,
	MUTE_ON
};

enum{
	SEEK_DOWN=0,
	SEEK_UP=1
};

static unsigned char fm_volume=30; //max : 30, min : 0

const unsigned char Volume_Table[31]=
{ 
	0x1e, 
	0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14,
	0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a,
	0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
};

	unsigned char si4700_reg_data[32];	
/********** Structure Definitions **********/

/**
 * Structure for DMA Command
 */
struct stmp37xx_dma_cmd
{
	reg32_t *cmd_ptr;	/**< dma command \a virtual address pointer */
	reg32_t cmd_ptr_p;	/**< dma command \a physical address pointer */
};
/**
 * FMTuner DMA Command Chain Address
 */
static struct stmp37xx_dma_cmd FMTuner_dma_cmd1_s = {
	cmd_ptr 	:	(reg32_t *)NULL,
	cmd_ptr_p	:	0
};
static struct stmp37xx_dma_cmd FMTuner_dma_cmd2_s = {
	cmd_ptr 	:	(reg32_t *)NULL,
	cmd_ptr_p	:	0
};
static struct stmp37xx_dma_cmd FMTuner_dma_cmd3_s = {
	cmd_ptr 	:	(reg32_t *)NULL,
	cmd_ptr_p	:	0
};
static struct stmp37xx_dma_cmd FMTuner_dma_cmd4_s = {
	cmd_ptr 	:	(reg32_t *)NULL,
	cmd_ptr_p	:	0
};

/********** Variables **********/

/** 
 * Read Command DMA channel = CMD3,4
 */

static reg32_t	I2C_DMA_CMD4[4] =
{
	/* WD0: Link to next DMA descriptor */
	0x00000000,
	//(reg32_t)  0,
		
	(BF_APBX_CHn_CMD_XFER_COUNT(NUMBER_BYTES_TO_READ) | // simulation convenience
	 BF_APBX_CHn_CMD_SEMAPHORE(1)	 |
	 BF_APBX_CHn_CMD_CMDWORDS(1)	 |
	 BF_APBX_CHn_CMD_WAIT4ENDCMD(1)|
	 BF_APBX_CHn_CMD_CHAIN(0)		 |			  // last command
	 BV_FLD(APBX_CHn_CMD, COMMAND, DMA_WRITE)),

	/* WD2: Buffer address */
	(reg32_t) NULL, 						// Data to DMA	
	//(reg32_t) &FMTuner_command_buffer[0],
	
	BF_I2C_CTRL0_SEND_NAK_ON_LAST(BV_I2C_CTRL0_SEND_NAK_ON_LAST__NAK_IT) |
	BF_I2C_CTRL0_POST_SEND_STOP(BV_I2C_CTRL0_POST_SEND_STOP__SEND_STOP)|
	BF_I2C_CTRL0_MASTER_MODE(BV_I2C_CTRL0_MASTER_MODE__MASTER)			 |
	BF_I2C_CTRL0_DIRECTION(BV_I2C_CTRL0_DIRECTION__RECEIVE) 			 |
	BF_I2C_CTRL0_XFER_COUNT(NUMBER_BYTES_TO_READ)	 // simulation convenience
};

/**
 * send the eeprom address with a READ bit turned on 
 */
static reg32_t	I2C_DMA_CMD3[4] =
{
	/* WD0: Link to next DMA descriptor */
	0x00000000,
	//(reg32_t)  I2C_DMA_CMD4,
	
	(BF_APBX_CHn_CMD_XFER_COUNT(1) |
	 BF_APBX_CHn_CMD_SEMAPHORE(0)  |
	 BF_APBX_CHn_CMD_CMDWORDS(1)   |
	 BF_APBX_CHn_CMD_WAIT4ENDCMD(1)|
	 BF_APBX_CHn_CMD_CHAIN(1)	   |
	 BV_FLD(APBX_CHn_CMD, COMMAND, DMA_READ)),

	/* WD2: Buffer address */
	(reg32_t) NULL, 						// Data to DMA	
	//(reg32_t) &FMTuner_command_buffer[3], // i2c read
	
	BF_I2C_CTRL0_RETAIN_CLOCK(BV_I2C_CTRL0_RETAIN_CLOCK__HOLD_LOW)|
	BF_I2C_CTRL0_PRE_SEND_START(BV_I2C_CTRL0_PRE_SEND_START__SEND_START)|
	BF_I2C_CTRL0_MASTER_MODE(BV_I2C_CTRL0_MASTER_MODE__MASTER)			|
	BF_I2C_CTRL0_DIRECTION(BV_I2C_CTRL0_DIRECTION__TRANSMIT)			|
	BF_I2C_CTRL0_XFER_COUNT(1) 
};

/**
 * Wriet Command DMA channel = CMD1,2
 * write 64 bytes to the eeprom 
 */
static reg32_t	I2C_DMA_CMD2[4] =
{
	/* WD0: Link to next DMA descriptor */
	0x00000000,
	//(reg32_t)  0,

	/* WD1: DMA Channel Command */	
	(BF_APBX_CHn_CMD_XFER_COUNT(NUMBER_BYTES_TO_WRITE ) |
	 BF_APBX_CHn_CMD_SEMAPHORE(1)						|
	 BF_APBX_CHn_CMD_CMDWORDS(1)						|
	 BF_APBX_CHn_CMD_WAIT4ENDCMD(1) 					|
	 BF_APBX_CHn_CMD_CHAIN(0)							|
	 BV_FLD(APBX_CHn_CMD, COMMAND, DMA_READ)),

	/* WD2: Buffer address */	
	(reg32_t) NULL,  // Data to DMA 
	//(reg32_t) &FMTuner_command_buffer[0],

	/* WD3: PIO words */	
	BF_I2C_CTRL0_POST_SEND_STOP(BV_I2C_CTRL0_POST_SEND_STOP__SEND_STOP) |
	BF_I2C_CTRL0_MASTER_MODE(BV_I2C_CTRL0_MASTER_MODE__MASTER)			 |
	BF_I2C_CTRL0_DIRECTION(BV_I2C_CTRL0_DIRECTION__TRANSMIT)			 |
	BF_I2C_CTRL0_XFER_COUNT(NUMBER_BYTES_TO_WRITE )
};

/**
 * write 1 bytes to the FMTuner in a single dma transfer  pre start and post end 
 */
static reg32_t	I2C_DMA_CMD1[4] =
{
	/* WD0: Link to next DMA descriptor */
	0x00000000,
	//(reg32_t)  I2C_DMA_CMD2,

	/* WD1: DMA Channel Command */
	(BF_APBX_CHn_CMD_XFER_COUNT(1) |		// slave addr
	 BF_APBX_CHn_CMD_CMDWORDS(1)   |
	 BF_APBX_CHn_CMD_WAIT4ENDCMD(1)|
	 BF_APBX_CHn_CMD_CHAIN(1)	   |
	 BV_FLD(APBX_CHn_CMD, COMMAND, DMA_READ)),

	/* WD2: Buffer address */
	(reg32_t) NULL, 						// Data to DMA
	//(reg32_t) &FMTuner_command_buffer[0], // i2c write	

	/* WD3: PIO words */
	BF_I2C_CTRL0_PRE_SEND_START(BV_I2C_CTRL0_PRE_SEND_START__SEND_START) |
	BF_I2C_CTRL0_RETAIN_CLOCK(BV_I2C_CTRL0_RETAIN_CLOCK__HOLD_LOW)		 |
	BF_I2C_CTRL0_MASTER_MODE(BV_I2C_CTRL0_MASTER_MODE__MASTER)			 |
	BF_I2C_CTRL0_DIRECTION(BV_I2C_CTRL0_DIRECTION__TRANSMIT)			 |
	BF_I2C_CTRL0_XFER_COUNT(1)				// slave addr
};

static ssize_t fmtuner_dma_addr_init(void);
static ssize_t fmtuner_dma_addr_exit(void);
static void stmp3750_i2c_write(void);
static void stmp3750_i2c_read(void);
static void stmp3750_manual_frequp(void);
static void stmp3750_manual_freqdown(void);
static unsigned int stmp3750_set_frequency(unsigned long frequency, unsigned char Mute_state);
static void stmp3750_set_region(unsigned short region_indicator);
static unsigned long stmp3750_get_frequency(void);
static unsigned short stmp3750_get_sig_ifcount(void);
static int stmp3750_fmtuner_open(struct inode *inode, struct file *file);
static int stmp3750_fmtuner_release(struct inode *inode, struct file *file);
static unsigned int stmp3750_fmtuner_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
static int stmp3750_fmtuner_init(unsigned long addr);
static int stmp3750_fmtuner_exit(struct inode *inode, struct file *file);
static void stmp3750_fmtuner_reg_init(void);

static fm_call_back dsp_callback;


static int set_pin_gpio_val(int bank, int pin, int val)
{
  volatile unsigned *addr;

  addr = (unsigned *)(HW_PINCTRL_DOUT0_ADDR + (bank << 8));

  if(val ==0) {
    *(addr+2)=1 << pin;
  } else {
    *(addr+1)=1 << pin;
  }
  
  return 0;
}

static int get_pin_gpio_val(int bank, int pin)
{

  volatile unsigned *addr;
  int val;
 
  addr = (unsigned *)(HW_PINCTRL_DIN0_ADDR + (bank << 8));

  val = (*addr) & (1 << pin);

  if(val != 0) return 1;
  else return 0;

  return 0;
}


int set_pin_func(int bank, int pin, int func)
{
#if 0
  volatile unsigned *addr;

  if(bank < 0 || bank >= 4 ||
     pin < 0 || pin >= 32 ||
     func < 0 || func >= 4 )
    {
      printk("[PINCTRL] error in range\n");
    }
  
  addr = (unsigned *) (HW_PINCTRL_MUXSEL0_ADDR + (bank * 0x100) + ((pin >> 4) * 0x10));
  
  *(addr+2)=0x03 << ((pin %16) << 1);
  *(addr+1)=func << ((pin %16) << 1);
  
#else
  stmp37xx_gpio_set_af(pin_GPIO(bank,  pin), func); 
#endif
  return 0;
}

#define GPIO_I2C

#ifdef GPIO_I2C
static void SW_I2C_start(void)
{

	// Generate Start signal....
	
	FM_I2C_SDA_OUT;	

	FM_I2C_SCL_HIGH;

	FM_I2C_SDA_HIGH;
		
	FM_I2C_CLOCK_DELAY;
	
	FM_I2C_SDA_LOW;	

	FM_I2C_CLOCK_DELAY;

	FM_I2C_SCL_LOW;		

	FM_I2C_CLOCK_DELAY;
}

static void SW_I2C_sendID_RW(BYTE ID, BYTE RW)
{

	char i;
	BYTE temp;

	// make sure SDA port output
	// and set SDA & SCL low


	// Generate ID signal and RW signal....
	// step 1 : combine 7bit ID and 1bit RW.
	if(RW == 0)
		temp = (ID<<1) & 0xFE;
	else
		temp = (ID<<1) | 0x01;

	// step 2 : send serialized data...
	for(i=8;i>0;i--)
	{
		if((temp>>(i-1) & 0x01) == 0)		// when bit 1 is zero('0').
			FM_I2C_SDA_LOW;	
		else				// when bit 1 is '1'.
			FM_I2C_SDA_HIGH;
		
		FM_I2C_CLOCK_DELAY;

		FM_I2C_SCL_HIGH;	

		FM_I2C_CLOCK_DELAY;
		
		FM_I2C_SCL_LOW;	

	}

	// make both line to low('0').
	FM_I2C_SDA_LOW;

}

static char SW_I2C_ACK()
{

	BYTE wait_cnt= 0;
	BYTE ack_check = 0;
	
	// to catch ACK/NACK signal...make SDATA to input port

	FM_I2C_SDA_IN;

	FM_I2C_SCL_HIGH;	

	FM_I2C_CLOCK_DELAY;
	
	ack_check = GET_FM_SDA;		
	
	FM_I2C_SCL_LOW;

	FM_I2C_CLOCK_DELAY;

	while(1)
	{

		if(!ack_check)		
			break;			
		else
			wait_cnt++;

		ack_check = GET_FM_SDA;

		if(wait_cnt >= 120)
		{
			FM_I2C_SDA_OUT;

			FM_I2C_CLOCK_DELAY;
				
			FM_I2C_SDA_LOW;

			FM_I2C_CLOCK_DELAY;
				
			FM_I2C_SCL_LOW;
			
			FM_I2C_CLOCK_DELAY;	
			
			return -1;
		}
	
	}

	// make SDATA to output port
	FM_I2C_SDA_OUT;
	
	FM_I2C_SDA_LOW;

	return 1;
}

static void SW_I2C_send_ACK(BOOL  N_ACK)
{

	// HOST should have to send ACK or NACK signal 
	// and make SDATA to output port

	if(N_ACK == 0)				// in case of NACK
		FM_I2C_SDA_HIGH;
	else						// in case of ACK.
		FM_I2C_SDA_LOW;


	FM_I2C_SCL_HIGH;

	FM_I2C_CLOCK_DELAY;

	FM_I2C_SCL_LOW;	

	FM_I2C_CLOCK_DELAY;

	// make low both of SCLK and SDATA line ....
	FM_I2C_SDA_LOW;
	

}

static void SW_I2C_sendDATA(BYTE data_out)
{

	char i;

	// Generate ID signal and RW signal....
	// step 1 : send serialized data...
	for(i=8;i>0;i--)
	{

		if((data_out>>(i-1) & 0x01) == 0)		// when bit 1 is zero('0').
			FM_I2C_SDA_LOW;//IOA = (IOA & ~(bmSDATA));// | bmSCLK));
		else				// when bit 1 is '1'.
			FM_I2C_SDA_HIGH;//IOA = (IOA | bmSDATA);// & ~bmSCLK;


		FM_I2C_SCL_HIGH;

		FM_I2C_CLOCK_DELAY;

		FM_I2C_SCL_LOW;

		FM_I2C_CLOCK_DELAY;

	}

	// make both line to low('0').
	FM_I2C_SDA_LOW;
}

static BYTE SW_I2C_readDATA()
{

	BYTE i;
	BYTE read_data = 0;

	FM_I2C_SDA_IN;
	
	FM_I2C_BYTE_DELAY;
	
	// Generate ID signal and RW signal....
	// step 1 : send serialized data...
	for(i=0;i<8;i++)
	{
		// make SCLK line high...
		FM_I2C_SCL_HIGH;
		
		FM_I2C_CLOCK_DELAY;

		if(!GET_FM_SDA)
			read_data = read_data | (read_data & ~0x01);
		else				// when bit 1 is '1'.
			read_data = read_data | 0x01;

		if(i<7)
			read_data = read_data << 1;
	
		FM_I2C_SCL_LOW;

		FM_I2C_CLOCK_DELAY;
	}
	
	FM_I2C_SDA_OUT;

	// make both line to low('0').
	FM_I2C_SDA_LOW;
	
	return read_data;
}

static void SW_I2C_stop()
{

	// Generate Stop signal....
//	FM_I2C_SDA_OUT;
	
	// step 1 : make high SCLK line first...
//	FM_I2C_SDA_LOW;
	
	FM_I2C_SCL_HIGH;
	
	FM_I2C_CLOCK_DELAY;

	FM_I2C_SDA_HIGH;
}

static char SW_I2C_READ( BYTE length, BYTE *data_read) 
{
	BYTE temp;

	//	printk("[%s]\n", __func__);
	
	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Step by Step description of I2C Read Transfer 1 or more BYTEs.
	// Step 1: Send START Signal.
	SW_I2C_start();
	FM_I2C_BYTE_DELAY;

	// Step 2: Write Device Address and direction = 0.
	SW_I2C_sendID_RW((BYTE)FM_CHIPID, (BYTE)(I2C_READ));
	FM_I2C_BYTE_DELAY;

	// Step 3: Wait for DONE = 1. And after that check If BERR = 1 or ACK = 0, terminate by setting STOP = 1.
	if(SW_I2C_ACK() != 1)		// When ACK is '0' to STOP process...
	{
		printk("Chip ID write: read mode error\n");	
		SW_I2C_stop();
		return -1;
	}
	FM_I2C_BYTE_DELAY;
#if 0	
	// Step 4: Write Data Address to I2DAT.
	SW_I2C_sendDATA(addr);
	FM_I2C_BYTE_DELAY;
	if(SW_I2C_ACK() != 1)		// When ACK is '0' to STOP process...
	{
		printk("address write error\n");			
		SW_I2C_stop();
		return -1;
	}
	FM_I2C_BYTE_DELAY;
	// Step 5: Send START = 1.  <<-------- Repeated Start
	SW_I2C_start();
	FM_I2C_BYTE_DELAY;
	

	// Step 6: Write Peripheral Address and direction = 1 to I2DAT.
	SW_I2C_sendID_RW((BYTE)CHIPID, (BYTE)I2C_READ);
	FM_I2C_BYTE_DELAY;
	// Step 7: Wait for DONE = 1. If BERR = 1 or ACK = 0, terminate by setting STOP = 1.

	if(SW_I2C_ACK() != 1)
	{
		printk("Chip ID write: read mode error\n");			
		SW_I2C_stop();
		return -1;
	}
	FM_I2C_BYTE_DELAY;
#endif	
	for(temp = 0; temp< length; temp++)
	{
		// Step 8: Read the data from I2DAT. With LASTRD = 1, this initiates the final byte read on the bus.
		// At this step Host must set ACK to indicate Read successful to Device when multi byte transfers.

		data_read[temp] = SW_I2C_readDATA();
	
		FM_I2C_BYTE_DELAY;
	
		if((temp == (length - 1)))
			SW_I2C_send_ACK(0);		// Set LASTRD = 1
		else
			SW_I2C_send_ACK(1);		// Set LASTRD = 1

	}

	FM_I2C_BYTE_DELAY;
	// Step 12: Set STOP = 1.
	SW_I2C_stop();
	///////////////////////////////////////////////////////////////////////////////////////////////////
	return 1;

}

static char SW_I2C_WRITE(BYTE length, BYTE *data_write) 
{

	BYTE temp;

	//	printk("[%s]\n", __func__);
	
	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Step by Step description of I2C Read Transfer 1 BYTE.
	// Step 1: Send START = 1.
	FM_I2C_BYTE_DELAY;
	
	SW_I2C_start();
	
	FM_I2C_BYTE_DELAY;

	// Step 2: Write Device Address and direction = 0.
	SW_I2C_sendID_RW((BYTE)FM_CHIPID, (BYTE)(I2C_WRITE));
	FM_I2C_BYTE_DELAY;
	// Step 3: Wait for DONE = 1. If BERR = 1 or ACK = 0, terminate by setting STOP = 1.
	if(SW_I2C_ACK() != 1)
	{
		SW_I2C_stop();
		printk("Chip ID write: write mode error\n");			
		return -1;
	}
	FM_I2C_BYTE_DELAY;
#if 0	
	// Step 4: Write Data Address to I2DAT.
	SW_I2C_sendDATA(addr);
	FM_I2C_BYTE_DELAY;	
	if(SW_I2C_ACK() != 1)
	{
		SW_I2C_stop();


		return -1;
	}
	FM_I2C_BYTE_DELAY;
#endif

	// Step 5: Write data to I2DAT.
	if( data_write != NULL)
	for(temp = 0; temp< length; temp++)
	{
		SW_I2C_sendDATA(data_write[temp]);

		// Step 6: Wait for DONE = 1. If BERR = 1, terminate by setting STOP = 1.
		if(SW_I2C_ACK() != 1)
		{
			SW_I2C_stop();

			return -1;
		}

		//_nop_();	// Delay for a cycle...
	}
	FM_I2C_BYTE_DELAY;
	// Step 7: Set STOP = 1.
	SW_I2C_stop();

	FM_I2C_BYTE_DELAY;
	
	return 1;
}

void ResetSi47XX_2w(void)
{
	SET_TUNER_RESET;
	RST_HIGH;
	DELAY(3);
	
/*************************************************************************************/
#if 0
	// To let low condion SDIO
	set_pin_func(3,18,GPIO_MODE); /* I2C SCL - hcyun */ 
	set_pin_gpio_mode(3, 18, GPIO_OUT);		
	set_pin_gpio_val(3, 18, 0);

	FM_I2C_SCL_PIN_GPIO;
	FM_I2C_SCL_PIN_OUT;

#endif	
/*************************************************************************************/
	set_pin_func(2,6,3); /* I2C SCL - hcyun */ 
	stmp37xx_gpio_set_dir(pin_GPIO(2,6), 1);//GPIO_DIR_OUT);
	//set_pin_gpio_val(2, 6, 0);
	stmp37xx_gpio_set_level(pin_GPIO(2,6), 0);

	FM_I2C_SCL_PIN_GPIO;
	FM_I2C_SCL_PIN_OUT;


	DELAY(1);
	RST_LOW;
	DELAY(3);
	FM_I2C_SCL_HIGH;
	//	SET_TUNER_RESET;
	RST_HIGH;
	DELAY(1);
}


unsigned char OperationSi47XX_2w(T_OPERA_MODE operation, unsigned char *data, unsigned char numBytes)
{
	unsigned char ret;

	if(operation == FM_WRITE)
	{
	
		if(SW_I2C_WRITE(numBytes, data) == 1)
			ret = 0;
		else
			ret = 1;
	}
	else
	{
		if(SW_I2C_READ(numBytes, data) == 1)
			ret = 0;
		else
			ret = 1;
	}
	return ret;
}





#else

void ResetSi47XX_2w(void)
{
	SET_TUNER_RESET;
	RST_HIGH;
	DELAY(1);
	RST_LOW;
	DELAY(1);

/*************************************************************************************/
// To let low condion SDIO
#if 0
	set_pin_func(3,18,GPIO_MODE); /* I2C SCL - hcyun */ 
	set_pin_gpio_mode(3, 18, GPIO_OUT);		
	set_pin_gpio_val(3, 18, 0);
	
	FM_I2C_SCL_PIN_GPIO;
	FM_I2C_SCL_PIN_OUT;

	DELAY(1);
#endif
/*************************************************************************************/

	set_pin_func(2,6,3); /* I2C SCL - hcyun */ 
	stmp37xx_gpio_set_dir(pin_GPIO(2,6), 1);//GPIO_DIR_OUT);
	stmp37xx_gpio_set_level(pin_GPIO(2,6), 0);

	FM_I2C_SCL_PIN_GPIO;
	FM_I2C_SCL_PIN_OUT;

	DELAY(1);

//	SET_TUNER_RESET;
	RST_HIGH;
	DELAY(1);
	i2c_dma_init();	//Set I2C enable...

}


unsigned char OperationSi47XX_2w(T_OPERA_MODE operation, unsigned char *data, unsigned char numBytes)
{
	FMTuner_write_command_buffer[0] = (0x10 << 1) | I2C_FMTUNER_WR;
	FMTuner_read_command_buffer[0] = (0x10 << 1) | I2C_FMTUNER_RD;
 

	if(operation == FM_WRITE)
	{

		memcpy(FMTuner_data_write_buffer, data, numBytes);
		udelay(1000);
		stmp3750_i2c_write();
	}
	else
	{
		stmp3750_i2c_read();
		memcpy(data, FMTuner_data_read_buffer, numBytes);
		udelay(1000);
	}
	return 0;
}


#endif



//#define	FM_START_DC1CLK_PWM_15M_FIX	
//#define	FM_START_DC1CLK_750K_FIX
//#define USE_DC1CLK_750KHZ 1

//#define PWM_MODE
//#define PFM_MODE
//#define DC15M_CLK
//#define DC750K_CLK

#include <asm-arm/arch-stmp37xx/37xx/regspower.h>

T_ERROR_OP OSC_TUNR_ON(void)
{
//	unsigned char Si47XX_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si47XX_power_up[] = {0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00};
	//unsigned char Si47XX_power_up[] = {0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned short  loop_counter = 0;

//	SI470X.ADDR.B2.B.DISABLE = 1;
//	SI470X.ADDR.B2.B.ENABLE = 1;
//	SI470X.ADDR.B7.B.XOSCEN = 1;

//	SI47XX[5]=0x41;

//	SI47XX[14]=0x80;


	printk("[%s]\n",__func__);

	do
	{
		error_ind = OperationSi47XX_2w(FM_WRITE, &(Si47XX_power_up[0]), 12);

		if(error_ind)
			loop_counter++;
		else
			break;
	
	}while(loop_counter < 100);	


	if(loop_counter >= 100)
	{
		printk("stmp37xx_FMTuner : OSC_TURN_ON error\n");
  		return LOOP_EXP_ERROR;
	}

	return OK;
}

T_ERROR_OP Si4702_Power_Off(void)
{
	unsigned char error_ind = 0;
	unsigned char Si47XX_power_down[] = {0x00,0x41,0x00,0x00,0x40,0x04,0x0c,0x1e,0x00,0x00,0x00,0x00};
	unsigned short  loop_counter = 0;
//	fmtuner_dma_addr_exit();


	FMTuner_dma_cmd2_s.cmd_ptr[1] = (I2C_DMA_CMD2[1] & 0x0000ffff) |BF_APBX_CHn_CMD_XFER_COUNT(8);
	FMTuner_dma_cmd2_s.cmd_ptr[3]=  (I2C_DMA_CMD2[3] & 0xffff0000) |BF_I2C_CTRL0_XFER_COUNT(8);

//	fmtuner_dma_addr_init();

	do
	{
		error_ind = OperationSi47XX_2w(FM_WRITE, &(Si47XX_power_down[0]), 10);

		if(error_ind)
			loop_counter++;
		else
			break;
	
	}while(loop_counter < 100);	


	if(loop_counter >= 100)
	{
		printk("stmp37xx_FMTuner : TUNER_POWER_OFF error\n");
  		return LOOP_EXP_ERROR;
	}

	
	return OK;
}

T_ERROR_OP Si4702_Power_Up(void)
{
	unsigned char error_ind = 0;
	unsigned char Si47XX_power_up[] = {0x00,0x01,0x00,0x00,0x40,0x04,0x0c,0x1e,0x00,0x00,0x00,0x00};
	unsigned short  loop_counter = 0;

	FMTuner_dma_cmd2_s.cmd_ptr[1] = (I2C_DMA_CMD2[1] & 0x0000ffff) |BF_APBX_CHn_CMD_XFER_COUNT(8);
	FMTuner_dma_cmd2_s.cmd_ptr[3]=  (I2C_DMA_CMD2[3] & 0xffff0000) |BF_I2C_CTRL0_XFER_COUNT(8);

	do
	{
		error_ind = OperationSi47XX_2w(FM_WRITE, &(Si47XX_power_up[0]), 8);

		if(error_ind)
			loop_counter++;
		else
			break;
	
	}while(loop_counter < 100);	


	if(loop_counter >= 100)
	{
		printk("stmp37xx_FMTuner : TUNER_POWER_UP error\n");
  		return LOOP_EXP_ERROR;
	}
	DELAY(90);	

	return OK;
}

T_ERROR_OP Si4702_MUTE_OFF(void)
{
//	unsigned char Si47XX_mute_on[] = {0x00,0x01};
	unsigned char Si47XX_mute_off[] = {0xC0,0x01};
	unsigned char error_ind = 0;
	unsigned short loop_counter = 0;

//	error_ind = OperationSi47XX_2w(FM_WRITE, &(Si47XX_mute_off[0]), 2);		


	do
	{
		error_ind = OperationSi47XX_2w(FM_WRITE, &(Si47XX_mute_off[0]), 2);		


		if(error_ind)
			loop_counter++;
		else
			break;
	
	}while(loop_counter < 100);	


	if(loop_counter >= 100)
	{
		printk("stmp37xx_FMTuner : TUNER_POWER_UP error\n");
  		return LOOP_EXP_ERROR;
	}

	return OK;
}


	
T_ERROR_OP Si4702_FM_Tune_Freq(unsigned long channel_freq, unsigned char channel_space)
{
	unsigned short freq_reg_data, loop_counter = 0;
//	unsigned char si4700_reg_data[32];	
	unsigned char error_ind = 0, freq_step;
//	unsigned char si4700_channel_start_tune[] = {0x00,0x01,0x80,0xca,0x40,0x04,0x0a,0x1e,0x00,0x00,0x00,0x00};	//107.7MHz
//	unsigned char si4700_channel_stop_tune[] =  {0x00,0x01,0x00,0xca,0x40,0x04,0x0a,0x1e,0x00,0x00,0x00,0x00};	

	unsigned char si4700_channel_start_tune[] = {0x00,0x01,0x80,0xca,0xD0,0x04,0x0a,0x1d,0x00,0x00,0x00,0x00};	//107.7MHz
	unsigned char si4700_channel_stop_tune[] =  {0x00,0x01,0x00,0xca,0xD0,0x04,0x0a,0x1d,0x00,0x00,0x00,0x00};	

	unsigned int rtn;
	unsigned long initfreq;

	printk("[%s]\n",__func__); 

//normal  fm vol : B
//france fm vol: 7
	//set region
	switch(channel_space)
	{
		default:
		case REGION_WORLD:
			if(LimitedVol == TRUE)
				si4700_channel_start_tune[7] = 0x27;
			else
				si4700_channel_start_tune[7] = 0x2b;				
			freq_step = 50;
			initfreq = FM_MIN_FREQUENCY_OTHER;
			break;

		case REGION_JAPAN:
			if(LimitedVol == TRUE)
				si4700_channel_start_tune[7] = 0x57;
			else
				si4700_channel_start_tune[7] = 0x5b;				
			freq_step = 100;			
			initfreq = FM_MIN_FREQUENCY_JAPAN;			
			break;

		case REGION_KOREA:
			if(LimitedVol == TRUE)
				si4700_channel_start_tune[7] = 0x17;
			else
				si4700_channel_start_tune[7] = 0x1b;				
			freq_step = 100;			
			initfreq = FM_MIN_FREQUENCY_US;			
			break;
	}

	//set tune bit

	si4700_channel_start_tune[6] = usRSSI_LEVEL;
	si4700_channel_stop_tune[6] = usRSSI_LEVEL;
	
	freq_reg_data = (unsigned short)((channel_freq - initfreq)/(freq_step/10));
		
	si4700_channel_start_tune[3] = freq_reg_data & 0xff; 
	si4700_channel_start_tune[2] = (si4700_channel_start_tune[2] & 0xfc) | (freq_reg_data >> 8);



//	SI470X.ADDR.B3.B.CHAN = freq_reg_data;
	// Set tune
	do
	{
		error_ind = OperationSi47XX_2w(FM_WRITE, &(si4700_channel_start_tune[0]), 8);

		if(error_ind)
			loop_counter++;
		else
			break;
	
	}while(loop_counter < 0xff);	


	if(loop_counter >= 0xff)
	{
		printk("write tune error\n");
  		return I2C_ERROR;
	}

	loop_counter = 0;
	
	//Check stc bit.	after tune, stc bit is high
	do
	{

		DELAY(1);
//		udelay(100);

		loop_counter++;
		
	}
	while((get_pin_gpio_val(0, 11) == 1) && (loop_counter < 0xfff));		
	//while((stmp37xx_gpio_get_level(pin_GPIO(0, 11)) == 1) && (loop_counter < 0xfff));

	if(loop_counter >= 0xff)
	{
		printk("stmp37xx_FMTuner : LOOP_EXP_ERROR_set_tune\n");
  		return LOOP_EXP_ERROR;
	}


	loop_counter = 0;

	DELAY(70);

	loop_counter = 0;
	
	//clear tune bit
	si4700_channel_start_tune[2] = si4700_channel_start_tune[2] & 0x7f;


	do
	{
		error_ind = OperationSi47XX_2w(FM_WRITE, &(si4700_channel_start_tune[0]), 4);

		if(error_ind)
			loop_counter++;
		else
			break;

	}while(loop_counter < 100);	


	if(loop_counter >= 100)
	{
		printk("write turn clear bit error\n");
  		return I2C_ERROR;
	}
	
#if 1	
	//This is unstable fm module. 	
	//	if(stmp37xx_gpio_get_level(pin_GPIO(0, 11)) == 0)
	if(get_pin_gpio_val(0, 11) != 1)
	{
	  printk("GPIO low state error[%d]\n", get_pin_gpio_val(0,11));
	  return I2C_ERROR;		
	}
#endif	

	//Check stc bit "0"
	do
	{	
		//	error_ind = OperationSi47XX_2w(FM_READ, &(si4700_reg_data[0]), 4);
		while(OperationSi47XX_2w(FM_READ, &(si4700_reg_data[0]), 4))
/*
		if(error_ind)
		{
			printk("check clear turn bit error\n");
			
			return I2C_ERROR;	
		}
*/		
		loop_counter++;
	}
	while(((si4700_reg_data[0]&0x40) != 0) && (loop_counter < 0xff));		
	
	if(loop_counter >= 0xff)
	{
		printk("stmp37xx_FMTuner : LOOP_EXP_ERROR_clear_STC\n");
		
	   	return LOOP_EXP_ERROR;
	}

	mdelay(10);
#if 1
	{
		unsigned char i,j=0;
		
		{

			do
			{
				error_ind = OperationSi47XX_2w(FM_READ, &(si4700_reg_data[0]), 2);

				if(error_ind)
					loop_counter++;
				else
					break;
				
				mdelay(1);
			
			}while(loop_counter < 100);	


			if(loop_counter >= 100)
			{
				printk("read rssi error\n");
		  		return I2C_ERROR;
			}

			
			iFreq_RSSI = (unsigned int) si4700_reg_data[1];
			
			iFreq_AFC = (unsigned int)(si4700_reg_data[0] & 0x10) ; 
			
			if(!((si4700_reg_data[1] >= usRSSI_LEVEL)&&(!(si4700_reg_data[0] & 0x10))))
			{
				j++;
			}
		}
		
		if(j==0) uiFreq_status =TRUE; else uiFreq_status =FALSE;
 		
	}
#endif


//	Si4702_MUTE_OFF();
	
	return OK;

}


T_ERROR_OP Si4702_FM_Seek(unsigned  char Seek_mode, unsigned char channel_space)
{
	unsigned short freq_reg_data, loop_counter = 0;
//	unsigned char si4700_reg_data[32];	
	unsigned char error_ind = 0, freq_step;
//	unsigned char si4700_channel_start_tune[] = {0x00,0x01,0x00,0xca,0x40,0x04,0x0a,0x1e,0x00,0x48,0x00,0x00};	//107.7MHz
//	unsigned char si4700_channel_stop_tune[] =  {0x00,0x01,0x00,0xca,0x40,0x04,0x0a,0x1e,0x00,0x48,0x00,0x00};	

	unsigned char si4700_channel_start_tune[] = {0x00,0x01,0x00,0xca,0xD0,0x04,0x0a,0x1e,0x00,0x48,0x00,0x00};	//107.7MHz
	unsigned char si4700_channel_stop_tune[] =  {0x00,0x01,0x00,0xca,0xD0,0x04,0x0a,0x1e,0x00,0x48,0x00,0x00};	


	unsigned char seek_sucess = 0, valid_channel = 0;
	unsigned int rtn;
	unsigned long initfreq;


/*
	if(usRSSI_LEVEL == 10)
		si4700_channel_start_tune[9] = 0xff;
	else	 if(usRSSI_LEVEL > 10)
		si4700_channel_start_tune[9] = 0x00;
	else
		si4700_channel_start_tune[9] = 0xff;
	*/
#if USE_DC1CLK_750KHZ
//	printk(KERN_DEBUG "DC1 freq = 750Khz\n");
//	HW_POWER_MINPWR_SET(4); 		//change into PFM mode
	//HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC1_HALFCLK); 	//change into 750K
	HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK); 	//change into 750K
	//HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC2_HALFCLK); 	//change into 750K
#endif 


#ifdef PWM_MODE
	//default HW_POWER_MINPWR_CLR(2);
#endif
#ifdef PFM_MODE
	HW_POWER_MINPWR.B.EN_DC_PFM = 1;
	//HW_POWER_MINPWR_SET(BM_POWER_MINPWR_EN_DC_PFM);
#endif

#ifdef DC15M_CLK
	//default HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC_HALFCLK);
#endif
#ifdef DC750K_CLK
	HW_POWER_MINPWR.B.DC_HALFCLK = 1;
	//HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK);
#endif

//DELAY(100);
//set region
	//set region
	switch(channel_space)
	{
		default:
		case REGION_WORLD:
			if(LimitedVol == TRUE)
				si4700_channel_start_tune[7] = 0x27;
			else
				si4700_channel_start_tune[7] = 0x2c;				
			freq_step = 50;
			initfreq = FM_MIN_FREQUENCY_OTHER;
			break;

		case REGION_JAPAN:
			if(LimitedVol == TRUE)
				si4700_channel_start_tune[7] = 0x57;
			else
				si4700_channel_start_tune[7] = 0x5c;				
			freq_step = 100;			
			initfreq = FM_MIN_FREQUENCY_JAPAN;			
			break;

		case REGION_KOREA:
			if(LimitedVol == TRUE)
				si4700_channel_start_tune[7] = 0x17;
			else
				si4700_channel_start_tune[7] = 0x1c;				
			freq_step = 100;			
			initfreq = FM_MIN_FREQUENCY_US;			
			break;
	}

	
	switch(Seek_mode)
	{
		default:
		case SEEK_DOWN:
			si4700_channel_start_tune[0] = 0x45;
			break;

		case SEEK_UP:
			si4700_channel_start_tune[0] = 0x47;
			break;
	}

	si4700_channel_start_tune[6] = usRSSI_LEVEL;
	si4700_channel_stop_tune[6] = usRSSI_LEVEL;
		
	do
	{
		error_ind = OperationSi47XX_2w(FM_WRITE, &(si4700_channel_start_tune[0]), 10);

		if(error_ind)
			loop_counter++;
		else
			break;
	
	}while(loop_counter < 0xff);	


	if(loop_counter >= 0xff)
	{
		printk("write tune error\n");
  		return I2C_ERROR;
	}

	DELAY(2);

	
	//Check stc bit.	after seek, stc bit is high
/*	
	do
	{
		DELAY(1);
		loop_counter++;
		error_ind = OperationSi47XX_2w(FM_READ, &(si4700_reg_data[0]), 4);
	}while(((si4700_reg_data[0]&0x40) == 0) && (loop_counter < 5000));

*/

	do
	{
		DELAY(1);
		//while ((stmp37xx_gpio_get_level(pin_GPIO(0,11) == 1)) && (loop_counter < 5000));
	} while((get_pin_gpio_val(0, 11) == 1) && (loop_counter < 5000)); 


	if(loop_counter >= 5000)
	{
		printk("stmp37xx_FMTuner : LOOP_EXP_ERROR_seek_tune\n");
  		return LOOP_EXP_ERROR;
	}

	DELAY(100);
	
	loop_counter = 0;

	do
	{
		error_ind = OperationSi47XX_2w(READ,&(si4700_reg_data[0]), 2);
		

		if(error_ind)
		{
			DELAY(3);
			loop_counter++;
		}
		else
			break;
	
	}while(loop_counter < 0xff);	


	if(loop_counter >= 0xff)
	{
		printk("Read seek tune val error\n");
  		return I2C_ERROR;
	}



	if((si4700_reg_data[0]& 0x20) != 0)
		seek_sucess = 0;
	else
		seek_sucess = 1;

#if 1	
	if(seek_sucess)
	{
		unsigned char i,j=0;
		
		for(i=0;i<2;i++)
		{
//			OperationSi47XX_2w(FM_READ, &(si4700_reg_data[0]), 4);
				
			if(!((si4700_reg_data[1] >= usRSSI_LEVEL)&&(!(si4700_reg_data[0] & 0x10))))
			{
				j++;
			}

		}
		
		if(j==0) 
		{
			printk("seek done & valid channel\n"); 
			valid_channel =TRUE;
			rtn = OK;
			
		}
		else
		{
			printk("seek done & invalid channel\n"); 
			valid_channel =FALSE;
			rtn = IN_VALID_CHANNEL;			
		}
		
	}
	else
	{
		rtn = END_OF_SEEK;
		printk("seek fail check end of limitted\n"); 
	}
#else
	if(seek_sucess)
	{
		printk("seek done & valid channel\n"); 
		valid_channel =TRUE;
		rtn = OK;
	}
	else
	{
		rtn = END_OF_SEEK;
		printk("seek fail check end of limitted\n"); 
	}

#endif
	
	//clear seek bit
	si4700_channel_start_tune[0] = si4700_channel_start_tune[0] & 0xfe;
	
	loop_counter = 0;
	
	do
	{
	
		error_ind = OperationSi47XX_2w(FM_WRITE, &(si4700_channel_start_tune[0]), 2);
			

		if(error_ind)
			loop_counter++;
		else
			break;
	
	}while(loop_counter < 0xff);	


	if(loop_counter >= 0xff)
	{
		printk("write clear seek bit error\n");
  		return I2C_ERROR;
	}

	loop_counter = 0;
	
	DELAY(2);
	
	//Check stc bit "0"
	do {	
		/*
		  do
		  {
		  
		  error_ind = OperationSi47XX_2w(FM_READ, &(si4700_reg_data[0]), 4);
		  
		  
		  if(error_ind)
		  loop_counter++;
		  else
		  break;
		  
		  }while(loop_counter < 0xff);	
		  
		*/
		while(OperationSi47XX_2w(FM_READ, &(si4700_reg_data[0]), 4))
			
			loop_counter++;
		
		if(loop_counter >= 0xff) {
			printk("check stc bit 0 error \n");
	  		return I2C_ERROR;
		}			
		
	} while(((si4700_reg_data[0]&0x40) != 0) && (loop_counter < 0xff));		
	
	
	if(loop_counter >= 0xff)
	{
		printk("stmp37xx_FMTuner : LOOP_EXP_ERROR_clear_STC at seek\n");
	   	return LOOP_EXP_ERROR;
	}
	loop_counter = 0;
	
	 freq_reg_data = ((si4700_reg_data[2] << 8) | si4700_reg_data[3])& 0x3ff;	

	ulFrequency = initfreq + freq_reg_data*(freq_step/10);
	 
	printk("stmp3750_FMTuner_seek_freq(%d)\n",ulFrequency);
	
//	Si4702_MUTE_OFF();
#if USE_DC1CLK_750KHZ
//	printk(KERN_DEBUG "DC1 freq = 1.5MHz\n");
//	HW_POWER_MINPWR_CLR(4); 		//change into PWM mode
	//HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC1_HALFCLK); 
	HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC_HALFCLK); 
	//HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC2_HALFCLK); 
#endif 


#ifdef PWM_MODE
	//default HW_POWER_MINPWR_CLR(2);
#endif
#ifdef PFM_MODE
	HW_POWER_MINPWR.B.EN_DC_PFM = 0;
	//HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_EN_DC_PFM);
#endif

#ifdef DC15M_CLK
	//default HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK);
#endif
#ifdef DC750K_CLK
	HW_POWER_MINPWR.B.DC_HALFCLK = 0;
	//HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC_HALFCLK);
#endif

	return rtn;

}



#if 0
void ss_fm_register(ss_fm_dev_t type, fm_call_back callback)
{ 
	switch (type)
	{
		case SS_FM_DSP_DEV:
			printk("ss_fm_register is called by AUDIO\n"); 
			dsp_callback = callback;
			break;
			
		default:
			break;
	}		
}
#endif

static ssize_t fmtuner_dma_addr_init(void)
{
	int i;

	/* dma chain consistent allocation */
#if 0
	FMTuner_dma_cmd1_s.cmd_ptr = (reg32_t *)consistent_alloc(GFP_KERNEL|GFP_DMA, 
		PAGE_ALIGN(4*16), (dma_addr_t)&FMTuner_dma_cmd1_s.cmd_ptr_p);
#endif

	FMTuner_dma_cmd1_s.cmd_ptr = 
	  (reg32_t *) dma_alloc_coherent(NULL,
					 PAGE_ALIGN(4 * 16), 
					 &FMTuner_dma_cmd1_s.cmd_ptr_p, GFP_DMA);


	//FMTuner_dma_cmd2_s.cmd_ptr = (reg32_t *)consistent_alloc(GFP_KERNEL|GFP_DMA, 
	//	PAGE_ALIGN(sizeof(I2C_DMA_CMD2)*4), (dma_addr_t)&FMTuner_dma_cmd2_s.cmd_ptr_p);
	//FMTuner_dma_cmd3_s.cmd_ptr = (reg32_t *)consistent_alloc(GFP_KERNEL|GFP_DMA, 
	//	PAGE_ALIGN(sizeof(I2C_DMA_CMD3)*4), (dma_addr_t)&FMTuner_dma_cmd3_s.cmd_ptr_p);
	//FMTuner_dma_cmd4_s.cmd_ptr = (reg32_t *)consistent_alloc(GFP_KERNEL|GFP_DMA, 
	//	PAGE_ALIGN(sizeof(I2C_DMA_CMD4)*4), (dma_addr_t)&FMTuner_dma_cmd4_s.cmd_ptr_p);
	FMTuner_dma_cmd2_s.cmd_ptr = FMTuner_dma_cmd1_s.cmd_ptr + 4;
	FMTuner_dma_cmd3_s.cmd_ptr = FMTuner_dma_cmd2_s.cmd_ptr +4;
	FMTuner_dma_cmd4_s.cmd_ptr = FMTuner_dma_cmd3_s.cmd_ptr + 4;
	FMTuner_dma_cmd2_s.cmd_ptr_p = FMTuner_dma_cmd1_s.cmd_ptr_p + 16;
	FMTuner_dma_cmd3_s.cmd_ptr_p = FMTuner_dma_cmd2_s.cmd_ptr_p + 16;
	FMTuner_dma_cmd4_s.cmd_ptr_p = FMTuner_dma_cmd3_s.cmd_ptr_p + 16;
	
	/* initialize FMTuner dma descriptor */
	for(i = 0 ; i < 4 ; i++) 
	{
		FMTuner_dma_cmd1_s.cmd_ptr[i] = I2C_DMA_CMD1[i];
		FMTuner_dma_cmd2_s.cmd_ptr[i] = I2C_DMA_CMD2[i];
		FMTuner_dma_cmd3_s.cmd_ptr[i] = I2C_DMA_CMD3[i];
		FMTuner_dma_cmd4_s.cmd_ptr[i] = I2C_DMA_CMD4[i];
	}
	
	FMTuner_dma_cmd1_s.cmd_ptr[0] = FMTuner_dma_cmd2_s.cmd_ptr_p;
	FMTuner_dma_cmd2_s.cmd_ptr[0] = 0;
	FMTuner_dma_cmd3_s.cmd_ptr[0] = FMTuner_dma_cmd4_s.cmd_ptr_p;
	FMTuner_dma_cmd4_s.cmd_ptr[0] = 0;

	/* buffer allocation */
#if 0	
	FMTuner_write_command_buffer = (unsigned char *)consistent_alloc(GFP_KERNEL|GFP_DMA, PAGE_ALIGN(66), 
									(dma_addr_t)&write_command_buffer_start_p);
									
#endif 
	FMTuner_write_command_buffer = 
	  (unsigned char *) dma_alloc_coherent(NULL,
					       PAGE_ALIGN(66), 
					       &write_command_buffer_start_p, GFP_DMA);
	

	//FMTuner_read_command_buffer = (unsigned char *)consistent_alloc(GFP_KERNEL|GFP_DMA, PAGE_ALIGN(1), 
	//								(dma_addr_t)&read_command_buffer_start_p);
	//FMTuner_data_write_buffer = (unsigned char *)consistent_alloc(GFP_KERNEL|GFP_DMA, PAGE_ALIGN(5), 
	//								(dma_addr_t)&write_buffer_start_p);
	//FMTuner_data_read_buffer = (unsigned char *)consistent_alloc(GFP_KERNEL|GFP_DMA, PAGE_ALIGN(5), 
	//								(dma_addr_t)&read_buffer_start_p);
	FMTuner_read_command_buffer = FMTuner_write_command_buffer + 1;
	FMTuner_data_write_buffer = FMTuner_read_command_buffer + 1;
	FMTuner_data_read_buffer = FMTuner_data_write_buffer + FM_TUNER_REG_SPACE;
	read_command_buffer_start_p = write_command_buffer_start_p + 1;
	write_buffer_start_p = read_command_buffer_start_p + 1;
	read_buffer_start_p = write_buffer_start_p + FM_TUNER_REG_SPACE;
	/* dma data buffer start address setup */
	FMTuner_dma_cmd1_s.cmd_ptr[2] = write_command_buffer_start_p;
	FMTuner_dma_cmd2_s.cmd_ptr[2] = write_buffer_start_p;
	FMTuner_dma_cmd3_s.cmd_ptr[2] = read_command_buffer_start_p;
	FMTuner_dma_cmd4_s.cmd_ptr[2] = read_buffer_start_p;
	
	return 0;

}

static ssize_t fmtuner_dma_addr_exit(void)
{

#if 0
	/* Free allcation of DMA Command */
	consistent_free((reg32_t *)FMTuner_dma_cmd1_s.cmd_ptr, PAGE_ALIGN(4*16), 
					(dma_addr_t)FMTuner_dma_cmd1_s.cmd_ptr_p);

	/* Free allcation of Frame Buffer */
	consistent_free((unsigned char *)FMTuner_write_command_buffer, PAGE_ALIGN(66), 
					(dma_addr_t)write_command_buffer_start_p);
#endif

	dma_free_coherent(NULL, 4*16, FMTuner_dma_cmd1_s.cmd_ptr, FMTuner_dma_cmd1_s.cmd_ptr_p);
	dma_free_coherent(NULL, 66, FMTuner_write_command_buffer, write_command_buffer_start_p);
	
	return 0;
}

static void stmp3750_i2c_write(void)
{
	/* Setup dma channel configuration */
	BF_WRn(APBX_CHn_NXTCMDAR, I2C_CHANNEL_NUMBER, CMD_ADDR, (reg32_t)FMTuner_dma_cmd1_s.cmd_ptr_p);
	BF_WR(APBX_CTRL1, CH3_CMDCMPLT_IRQ, 0);    //clear interrupt
	/* start the DMA command chain by incrementing semaphore*/
	BF_WRn(APBX_CHn_SEMA, I2C_CHANNEL_NUMBER, INCREMENT_SEMA, 1);

	udelay(1000);
	udelay(1000);
	udelay(1000);
	udelay(1000);
	//	udelay(5000);  //delay time is more than 1000, because safe data transfer
}
static void stmp3750_i2c_read(void)
{
	/* Setup dma channel configuration */
	BF_WRn(APBX_CHn_NXTCMDAR, I2C_CHANNEL_NUMBER, CMD_ADDR, (reg32_t)FMTuner_dma_cmd3_s.cmd_ptr_p);
	BF_WR(APBX_CTRL1, CH3_CMDCMPLT_IRQ, 0);
	/* start the DMA command chain by incrementing semaphore*/
	BF_WRn(APBX_CHn_SEMA, I2C_CHANNEL_NUMBER, INCREMENT_SEMA, 1);
	udelay(1000);
	udelay(1000);
	udelay(1000);
	udelay(1000);
	//	udelay(5000);  //delay time is more than 1000, because safe data transfer
}

static void stmp3750_manual_frequp(void)
{
	//printk("stmp3750_fmtuner_frequp() is start!!! \n");
	unsigned char data[5];
	unsigned short newFreq;
	
	ulFrequency += FM_FREQUENCY_SCALE;

	if(ulFrequency>FM_MAX_FREQUENCY)
	{
		ulFrequency = FM_MIN_FREQUENCY;
	}

	stmp3750_set_frequency(ulFrequency, 0);

	printk("manual_frequp()\n");
}

static void stmp3750_manual_freqdown(void)
{
	//printk("stmp3750_fmtuner_freqdown() is start!!! \n");
	unsigned char data[5];
	unsigned short newFreq;
	
	ulFrequency -= FM_FREQUENCY_SCALE;

	if(ulFrequency<FM_MIN_FREQUENCY)
	{
		ulFrequency = FM_MAX_FREQUENCY;
	}

	stmp3750_set_frequency(ulFrequency, 0);

	printk("manual_freqdown()\n");
}
static void stmp3750_set_region(unsigned short region_indicator)
{
	if(region_indicator == REGION_KOREA)
	{
		setted_region = REGION_WORLD;
	//	SI470X.ADDR.B5.B.BAND = 0;
	//	SI470X.ADDR.B5.B.SPACE = 2;
		FM_MAX_FREQUENCY = FM_MAX_FREQUENCY_US;
		FM_MIN_FREQUENCY = FM_MIN_FREQUENCY_US;
		FM_FREQUENCY_SCALE = FM_FREQUENCY_SCALE_US;
		
		printk("REGION_KOREA  is setted\n");
	}
	else if(region_indicator == REGION_JAPAN)
	{
		setted_region = REGION_JAPAN;
//		SI470X.ADDR.B5.B.BAND = 1;
//		SI470X.ADDR.B5.B.SPACE = 1;
		FM_MAX_FREQUENCY = FM_MAX_FREQUENCY_JAPAN;
		FM_MIN_FREQUENCY = FM_MIN_FREQUENCY_JAPAN;
		FM_FREQUENCY_SCALE = FM_FREQUENCY_SCALE_JAPAN;
		
		printk("JAPAN region is setted\n");
	}
	else
	{
		setted_region = REGION_WORLD;
//		SI470X.ADDR.B5.B.BAND = 0;
//		SI470X.ADDR.B5.B.SPACE = 2;
		FM_MAX_FREQUENCY = FM_MAX_FREQUENCY_OTHER;
		FM_MIN_FREQUENCY = FM_MIN_FREQUENCY_OTHER;
		FM_FREQUENCY_SCALE = FM_FREQUENCY_SCALE_OTHER;
		
		printk("REGION_WORLD is setted\n");
	}
}



static unsigned int stmp3750_Check_Freq(void)
{
	unsigned int error_ind = 0,rtn = 0;
	unsigned char Si47XX_Read[] = {0x00,0x00,0x00,0x00};

	error_ind = OperationSi47XX_2w(FM_READ, &(Si47XX_Read[0]), 4);

	if((Si47XX_Read[1] >= usRSSI_LEVEL)&&(!(Si47XX_Read[0] & 0x10)))
	{
			rtn = TRUE;
	}

	if(rtn)
	{
		if((Si47XX_Read[1] >= usRSSI_LEVEL)&&(!(Si47XX_Read[0] & 0x10)))
		{
			rtn = TRUE;
		}else
			rtn = FALSE;
	}

	return rtn;

}

static void  stmp3750_set_RSSI_level(unsigned short rssi_level)
{
	usRSSI_LEVEL =  rssi_level- 10;//9;
	printk("RSSI level set by %d\n",rssi_level);
}


static  unsigned int stmp3750_set_frequency(unsigned long frequency, unsigned char Mute_state)
{
	int i;
	unsigned short newFreq;
	unsigned char data [5];
	unsigned char data_r[5];
	unsigned long ready_flag;
	T_ERROR_OP rtn = OK;

#if USE_DC1CLK_750KHZ
//	printk(KERN_DEBUG "DC1 freq = 750Khz\n");
	HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK); 
	//HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC1_HALFCLK); 
	//HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC2_HALFCLK); 
#endif 

#ifdef PWM_MODE
	//default HW_POWER_MINPWR_CLR(2);
#endif
#ifdef PFM_MODE
	HW_POWER_MINPWR.B.EN_DC_PFM = 1;
	//HW_POWER_MINPWR_SET(BM_POWER_MINPWR_EN_DC_PFM);
#endif

#ifdef DC15M_CLK
	//default HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK);
#endif
#ifdef DC750K_CLK
	HW_POWER_MINPWR.B.DC_HALFCLK = 1;
	//HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK);
#endif

//	usFreq_status = FALSE;

	if(frequency>FM_MAX_FREQUENCY)
		frequency=FM_MIN_FREQUENCY;
	else if(frequency<FM_MIN_FREQUENCY)
		frequency=FM_MAX_FREQUENCY;

	ulFrequency = frequency;	

        rtn = Si4702_FM_Tune_Freq(ulFrequency, setted_region);		//pjdragon

	Si4702_MUTE_OFF();

//	usFreq_status = stmp3750_Check_Freq();
#if USE_DC1CLK_750KHZ
//	printk(KERN_DEBUG "DC1 freq = 1.5MHz\n");
	HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC_HALFCLK); 
	//HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC1_HALFCLK); 
	//HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC2_HALFCLK); 
#endif 


#ifdef PWM_MODE
	//default HW_POWER_MINPWR_CLR(2);
#endif
#ifdef PFM_MODE
	HW_POWER_MINPWR.B.EN_DC_PFM = 0;
	//HW_POWER_MINPWR_CLR(2);
#endif

#ifdef DC15M_CLK
	//default HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK);
#endif
#ifdef DC750K_CLK
	HW_POWER_MINPWR.B.DC_HALFCLK = 0;
	//HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC_HALFCLK);
#endif

	if(rtn != OK)
	{
		printk("tune error.....!\n");
		return LOOP_EXP_ERROR;

	}else
	{
		printk("stmp3750_FMTuner_set_freq(%d)\n",ulFrequency);
		return OK;
	}
	
}


static unsigned long stmp3750_get_frequency(void)
{
	return ulFrequency;
}

unsigned int stmp3750_Check_MS(void)
{
//	unsigned char si4700_reg_data[32];	
	unsigned int ret;

	 OperationSi47XX_2w(FM_READ, &(si4700_reg_data[0]), 4);

	if(si4700_reg_data[0] & 0x01)
		ret = 1;
	else
		ret = 0;

	return ret;
}

void stmp3750_Set_Vol(unsigned char ucVol)
{
	unsigned char reg;
		
	fm_volume = ucVol;

	if(fm_volume > 30)
		fm_volume = 0;

	reg = Volume_Table[fm_volume];

	printk("stmp3650 hp registger val(%d)\n",reg);

	BW_AUDIOOUT_HPVOL_VOL_LEFT(reg);

	BW_AUDIOOUT_HPVOL_VOL_RIGHT(reg);
	
	if(ucVol)
		BW_AUDIOOUT_HPVOL_MUTE(0x0);	//unmute	
	else
		BW_AUDIOOUT_HPVOL_MUTE(0x1);	//mute
}

static void stmp3750_fmtuner_reg_init(void)
{
  SET_TUNER_RESET;	

	set_pin_func(0,11,GPIO_MODE);//GPIO_MODE); /* I2C SCL - hcyun */
	//set_pin_func(0,11,GPIO_MODE); /* I2C SCL - hcyun */

	SET_TUNER_SERACH;

	if(IS_MUTEP_ON)
		MUTEP_OFF;	
}

typedef struct
{
  char version[16];
  char nation[4];
  char model[32];
} version_inf_t;

version_inf_t tmp_version;

static int stmp3750_fmtuner_open(struct inode *inode, struct file *file)
{
	unsigned short newFreq;
	unsigned char data[5];
	version_inf_t *sw_ver ;
//	unsigned char errCount=0;

	printk("[%s]\n",__func__);


	if(!dsp_callback)	printk("dsp is not registered \n"); 
		else (dsp_callback)(SS_FM_OPEN);

	
	PowerOnDone =FALSE;	

//*********************************************************************************************
#if 0
	sw_ver = chk_sw_version();

	if (strcmp(sw_ver->nation, "FR") == 0) 
	{
		printk("[DRIVER] contry code %s volume table\n", sw_ver->nation);		
//		setSWVersion(VER_FR);
		LimitedVol = TRUE;
	} 
	else {
		printk("[DRIVER] contry code %s volume table\n", sw_ver->nation);		
//		setSWVersion(VER_COMMON);
		LimitedVol = FALSE;
	}
#endif
//*********************************************************************************************
//소스 참고하려면 stmp37xx_audio.c line 1645

	
//RETRY:

	FMTuner_dma_cmd2_s.cmd_ptr[1] = (I2C_DMA_CMD2[1] & 0x0000ffff) |BF_APBX_CHn_CMD_XFER_COUNT(NUMBER_BYTES_TO_WRITE);
	FMTuner_dma_cmd2_s.cmd_ptr[3]=  (I2C_DMA_CMD2[3] & 0xffff0000) |BF_I2C_CTRL0_XFER_COUNT(NUMBER_BYTES_TO_WRITE);

#ifdef FM_START_DC1CLK_750K_FIX
//	printk(KERN_DEBUG "DC1 freq = 750Khz\n");
	HW_POWER_MINPWR_SET(4); 		//change into PFM mode
	HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK); 
	//HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC1_HALFCLK); 
#endif

#ifdef FM_START_DC1CLK_PWM_15M_FIX
	HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC_HALFCLK); 
	//HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC1_HALFCLK); 
#endif

	stmp3750_fmtuner_reg_init();

	ResetSi47XX_2w();

	OSC_TUNR_ON();	

	mdelay(1000); /* added for integrate to HAL */
	
	//	I2C_


	printk("FM module is opened \n"); 
	
	return 0;		
			
}

static int stmp3750_fmtuner_release(struct inode *inode, struct file *file)
{
	unsigned short newFreq;
	unsigned char data[5];

	if(!dsp_callback)	printk("dsp release is not registered \n"); 
		else (dsp_callback)(SS_FM_RELEASE);
	
	HW_AUDIOOUT_HPVOL_CLR(0x03000000);
	HW_AUDIOOUT_HPVOL_SET(0x00000000); //remove to (line in->hpvol)noise

//	Si4702_Power_Off();
	
	RST_LOW;		//To power down tuner IC

	printk("stmp3750_FMTuner_release()\n");

	// 2. I2C line disable 
	set_pin_func(2,5,3); /* I2C SCL - hcyun */ 
	//stmp37xx_config_pin(pin_GPIO(2,5));
	set_pin_func(2,6,3); /* I2C SDA - hcyun */ 
	//stmp37xx_config_pin(pin_GPIO(2,6));

#ifdef FM_START_DC1CLK_750K_FIX
//	printk(KERN_DEBUG "DC1 freq = 1.5MHz\n");
	HW_POWER_MINPWR_CLR(4); 		//change into PFM mode
	HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC_HALFCLK); 
	//HW_POWER_MINPWR_CLR(BM_POWER_MINPWR_DC1_HALFCLK); 
#endif	

#ifdef FM_START_DC1CLK_PWM_15M_FIX
	HW_POWER_MINPWR_SET(BM_POWER_MINPWR_DC_HALFCLK); 
#endif

	PowerOnDone = FALSE;
	return 0;
}

static unsigned int stmp3750_fm_poweron(void)
{
	unsigned char errCount=0;

	//Set audio path.
	//Keep order because of pop noise.

	
	mdelay(100);
	MUTEP_ON;
	
	BW_AUDIOOUT_HPVOL_MUTE(0x1);	//mute


//	stmp3750_Set_Vol(0);	

	
	BW_AUDIOOUT_HPVOL_SELECT(0x1);	//Set audio path line-in


//RETRY:	
	Si4702_Power_Up();


#if 0
	{ /* read chpi id for testing */
	  int cpID;
	  int i = 0;

	  mdelay(1000);

	  for(i = 0; i < 32; i++) {
	    si4700_reg_data[i] = 0;
	  }
	  
	  i = 0;

	  OperationSi47XX_2w(FM_READ, &(si4700_reg_data[0]), 32);

	  for(i = 0; i < 32; i++) {
	    printk("read [%d] from i2c \n", si4700_reg_data[0]);

	  }
	}
#endif

	MUTEP_OFF;
	if( OK!= Si4702_FM_Tune_Freq(ulFrequency, setted_region))
	{
		printk("stmp3750_fm_poweron failed()\n");
		
		return 1;	
	}else
	{
		printk("stmp3750_fm_poweron ()\n");
		PowerOnDone = TRUE;
		
		return 0;	
	}

}

static  unsigned int stmp3750_fmtuner_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned int ret=0;
	unsigned long val;
//	unsigned char si4700_reg_data[12];	

	switch (cmd) {
	case FM_RX_POWER_UP:
		printk("[fm_rx_power_up ]\n");
		ret = stmp3750_fm_poweron();
		stmp3750_Set_Vol(30);
		break;
	case FM_STEP_UP:
		printk("[fm_step_up ]\n");
		stmp3750_manual_frequp();
		break;
	case FM_STEP_DOWN:
		printk("[fm_step_down ]\n");
		stmp3750_manual_freqdown();
		break;
	case FM_AUTO_UP:
		printk("[fm_auto_up ]\n");
		ret = Si4702_FM_Seek((unsigned  char) SEEK_UP, setted_region);
		break;
	case FM_AUTO_DOWN:
		printk("[fm_auto_down ]\n");
		ret = Si4702_FM_Seek((unsigned  char) SEEK_DOWN, setted_region);
		break;
	case FM_GET_FREQUENCY:

		put_user(ulFrequency, (long *) arg);
		ret = 0;
		printk("[fm_get_frquency ]\n");
		//ret = stmp3750_get_frequency();
		break;
	case FM_SET_FREQUENCY:
		printk("[fm_set_frquency ]\n");
		if (get_user(val, (SI *) arg)) {
			return -EINVAL;
		}
		ret = stmp3750_set_frequency(val, 0);
		break;
	case FM_SET_REGION:
		break;
	case FM_SET_CONFIGURATION:
		printk("[fm_set_configuration ]\n");
		if (get_user(val, (SI *) arg)) {
			return -EINVAL;
		}
		ret = stmp3750_set_frequency(val, 0);
		break;
	default:
		break;
	}

#if 0

	switch(cmd)
	{
		case MANUAL_FREQ_DOWN:
			if(PowerOnDone)
				stmp3750_manual_freqdown(); //one step frequency down
			break;
			
		case MANUAL_FREQ_UP:
			if(PowerOnDone)
				stmp3750_manual_frequp(); //one step frequency up
			break;
			
		case SET_FREQUENCY:
			if(PowerOnDone)
				ret = stmp3750_set_frequency(arg, 0); //set frequency
			break;
			
		case GET_FREQUENCY:
			ret = stmp3750_get_frequency(); //get frequency
			break;

		case Check_Freq_Status:
			ret = uiFreq_status;//get frequency
			break;

		case SET_RSSI_LEVEL:
			stmp3750_set_RSSI_level((unsigned short) arg); //get frequency
			break;

		case SET_REGION:
			stmp3750_set_region((unsigned short) arg);
			break;

		case CHECK_MONO_ST:
			ret = stmp3750_Check_MS();
			break;

		case SET_VOL:
			stmp3750_Set_Vol((unsigned char) arg);
			break;

		case FM_POWER_ON:
			ret = stmp3750_fm_poweron();
			break;

		case AUTO_SEARCH_UP:
			if(PowerOnDone)
				ret = Si4702_FM_Seek((unsigned  char) SEEK_UP, setted_region);
			break;

		case AUTO_SEARCH_DOWN:
			ret = Si4702_FM_Seek((unsigned  char) SEEK_DOWN, setted_region);
			break;

		case CHECK_RDS_MSG:			// if RDS msg, return 1.
		  // ret = stmp37xx_gpio_get_level(pin_GPIO(0, 11));
		  ret =   get_pin_gpio_val(0, 11);
			if(ret)
			{
				ret = 0;

			}
			else
			{
				ret = 1;
				printk("RDS check is called \n");
			}
			
			break;

		case GET_RDS_BUFFER:
				{
			unsigned char pty;
			
			OperationSi47XX_2w(FM_READ, &(si4700_reg_data[0]), 12);
			copy_to_user((char *)arg,si4700_reg_data, 12);
			pty = (((si4700_reg_data[6]&0x03)<<3)|((si4700_reg_data[7]&0xe0)>>5))&0x1f;
			printk("Get RDS buffer pty =(%d)\n",pty);
			}
			break;
			
		case GET_RSSI_LEVEL:
			ret = -1;//iFreq_RSSI;
			//ret = iFreq_RSSI;
			break;

		case GET_AFC_RAIL:
			ret = -1;//iFreq_AFC;
			//ret = iFreq_AFC;
			break;
			
		default:
			printk("stmp3750_fmtuner_ioctl is executed by default\n");
			break;
	}
#endif
	return ret;

}

static struct file_operations fmtuner_fops = {
	.owner			= THIS_MODULE,
	.open			= stmp3750_fmtuner_open,	
	.release			= stmp3750_fmtuner_release,
	.ioctl				= stmp3750_fmtuner_ioctl,
};




/*
* FM tuner (TEA5767) is always turn on mode.
* Therefore, It must be changed to standby mode at the initialization 
* setting 7 bit of the forth data has to change the mode to standby mode
*/
static int stmp3750_fmtuner_init(unsigned long addr)
{

	int ret;
	unsigned short newFreq;
	unsigned char data[5];


	fmtuner_dma_addr_init();
	
	printk("stmp37xx_FMTuner : stmp37xx_FMTuner_init\n");
	
	ret = register_chrdev(FMTUNER_MAJOR, "Si470X", &fmtuner_fops);
	if(ret < 0) 
		return ret;
		
	return 0;
}

static int stmp3750_fmtuner_exit(struct inode *inode, struct file *file)
{
	int ret;
	printk("stmp37xx_FMTuner_exit() \n");

	fmtuner_dma_addr_exit();	

	unregister_chrdev(FMTUNER_MAJOR, "Si470X");
	//if(ret < 0) 
	//return -EINVAL;
	
	return 0;	
}

module_init(stmp3750_fmtuner_init);
module_exit(stmp3750_fmtuner_exit);

