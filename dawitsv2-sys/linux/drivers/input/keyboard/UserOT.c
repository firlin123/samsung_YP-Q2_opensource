// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// This file is specifically generated for

// OTLib 3.2 is customized with the following options:
// * Interrupt Mode of operation (Attention Line)
// * Software I2C on the host processor
// * Button  Features on OneTouch
// * TimeOut For the Attention Line 
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Copyright (c) 2006-2007 Synaptics, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of 
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to use, 
// copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
// Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
// SOFTWARE.
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Include all the header files you want to include here
// ***********************************************************************
// NOTE: User defined header file: Please add/ modify for your application
// ***********************************************************************
//#include <stdio.h> 

// ***************************************
// NOTE: OTLib header file: Do Not Modify
// ***************************************
// OTLib specific header files
#include "SynaOT.h"
#include "UserOT.h"
#include <linux/kernel.h>
#include <asm-arm/arch-stmp37xx/gpio.h>
#include <linux/delay.h>
#include <asm/arch/37xx/regs.h>
#include <asm/arch/37xx/regsdigctl.h>
#include <asm/arch/37xx/regslradc.h>
#include <asm/arch/37xx/regspinctrl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>    
#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>    
#include <linux/fcntl.h>   
#include <linux/sched.h>
#include <linux/fs.h>    
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/arch/irqs.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/arch/stmp37xx_pm.h>
#include <asm/arch/digctl.h>
#include <asm/arch/hardware.h>
#include <asm/arch/usb_common.h>

//#include "stmp37xx_sw_touchpad_i2c.h"

// ==========================================================================
// * OTLib Host Processor Specific (Hardware Abstract Layer (HAL) functions *
// ==========================================================================
// Prototype defintions 

// *************************************************************************************
// The User has to write the following HAL functions:
//
// 1) void OT_HAL_delay_10th_usec(OT_U16 tenth_microsec);
//			// Delay in units of 1/10th of microseconds (100 nano seconds)
//
// 2) void OT_HAL_SDA_IN_MODE(void);
//			// Set SDA (I2C Data) Line to Input Mode
//
// 3) void OT_HAL_SCL_IN_MODE(void);
//			// Set SCL (I2C Clock) Line to Input Mode 		
//
// 4) void OT_HAL_SDA_OUT_MODE(void);
//			// Set SDA (I2C Data) Line to Output Mode
//
// 5) void OT_HAL_SCL_OUT_MODE(void);
//			// Set SCL (I2C Clock) Line to Output Mode
//
// 6) void OT_HAL_I2C_SDA_LO(void);
//			// Set SDA (I2C Data) Line to Low (logic "0")
//
// 7) void OT_HAL_I2C_SCL_LO(void);
//			// Set SCL (I2C Clock) Line to Low (logic "0")	
//
// 8) OT_U8 OT_HAL_BitRead_I2C_SDA(void);
//			// Read the state of SDA (I2C Data) Line
//
// 9) OT_U8 OT_HAL_BitRead_I2C_SCL(void);
//			// Read the state of SCL (I2C Clock) Line	
//
// 10) OT_U8 OT_HAL_BitRead_ATTN(void);
//			//	Read the state of the Attention (ATTN) line
//	
// NOTE: Do not change the function names. Just add to the body of the HAL functions
// **************************************************************************************

 
#define BYTE unsigned char
 
/************************************************
 * Macro definition
 ************************************************/
#define PIN_SCL BANK0_PIN18
#define PIN_SDA BANK0_PIN9
#define PIN_INTR BANK0_PIN8
 
#define TOUCHPAD_I2C_SCL_GPIO        stmp37xx_gpio_set_af(PIN_SCL, GPIO_AF3)
#define TOUCHPAD_I2C_SCL_OUT         stmp37xx_gpio_set_dir(PIN_SCL, GPIO_DIR_OUT)
#define TOUCHPAD_I2C_SCL_IN          stmp37xx_gpio_set_dir(PIN_SCL, GPIO_DIR_IN)
#define TOUCHPAD_I2C_SCL_HIGH        stmp37xx_gpio_set_level(PIN_SCL, GPIO_VOL_33)
#define TOUCHPAD_I2C_SCL_LOW         stmp37xx_gpio_set_level(PIN_SCL, GPIO_VOL_18)
 
#define GET_TOUCHPAD_SCL             stmp37xx_gpio_get_level(PIN_SCL)

#define TOUCHPAD_I2C_SDA_GPIO        stmp37xx_gpio_set_af(PIN_SDA, GPIO_AF3)
#define TOUCHPAD_I2C_SDA_OUT         stmp37xx_gpio_set_dir(PIN_SDA, GPIO_DIR_OUT)
#define TOUCHPAD_I2C_SDA_IN          stmp37xx_gpio_set_dir(PIN_SDA, GPIO_DIR_IN)
#define TOUCHPAD_I2C_SDA_HIGH        stmp37xx_gpio_set_level(PIN_SDA, GPIO_VOL_33)
#define TOUCHPAD_I2C_SDA_LOW         stmp37xx_gpio_set_level(PIN_SDA, GPIO_VOL_18)
 
#define GET_TOUCHPAD_SDA             stmp37xx_gpio_get_level(PIN_SDA)
#define GET_TOUCHPAD_INTR            stmp37xx_gpio_get_level(PIN_INTR)
 
#define TOUCHPAD_I2C_BYTE_DELAY      udelay(3)
#define TOUCHPAD_I2C_CLOCK_DELAY     udelay(2)

void gpio_init(void)
{
        TOUCHPAD_I2C_SCL_GPIO;
        TOUCHPAD_I2C_SDA_GPIO;
	
	//HW_PINCTRL_PULL0_SET(0x1 << 8);	
	//HW_PINCTRL_PULL0_SET(0x1 << 9);	
	//HW_PINCTRL_PULL0_SET(0x1 << 18);	
	//printk("HW_PINCTRL_PULL0 = 0x%08x\n", HW_PINCTRL_PULL0);

        TOUCHPAD_I2C_SCL_OUT;
        TOUCHPAD_I2C_SDA_OUT;

        TOUCHPAD_I2C_SCL_LOW;
        TOUCHPAD_I2C_SDA_LOW;
}

// Delay in units of 1/10th of microseconds (100 nano seconds)
void OT_HAL_delay_10th_usec(OT_U16 tenth_microsec)
{	
	udelay(tenth_microsec);
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************

}

// Set SDA (I2C Data) Line to Input Mode
void OT_HAL_SDA_IN_MODE(void)
{
	TOUCHPAD_I2C_SDA_IN;
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************

}

// Set SCL (I2C Clock) Line to Input Mode 		
void OT_HAL_SCL_IN_MODE(void)
{
	TOUCHPAD_I2C_SCL_IN;
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************

}

// Set SDA (I2C Data) Line to Output Mode
void OT_HAL_SDA_OUT_MODE(void)
{
	TOUCHPAD_I2C_SDA_OUT;
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************

}

// Set SCL (I2C Clock) Line to Output Mode
void OT_HAL_SCL_OUT_MODE(void)
{
	TOUCHPAD_I2C_SCL_OUT;
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************

}	

// Set SDA (I2C Data) Line to Low (logic "0")
void OT_HAL_I2C_SDA_LO(void)
{
	TOUCHPAD_I2C_SDA_LOW;
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************

}

// Set SCL (I2C Clock) Line to Low (logic "0")	
void OT_HAL_I2C_SCL_LO(void)
{
	TOUCHPAD_I2C_SCL_LOW;
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************

}	

// Read the state of SDA (I2C Data) Line
OT_U8 OT_HAL_BitRead_I2C_SDA(void)
{
	unsigned char read_data = 0;

                if(!GET_TOUCHPAD_SDA) {
			//printk("get_data=0x%08x\n", GET_TOUCHPAD_SDA);
                        read_data = read_data | (read_data & ~0x01);
		}
                else {
			//printk("get_data=0x%08x\n", GET_TOUCHPAD_SDA);
                        read_data = read_data | 0x01;
		}

                //if(i < 7)
                //        read_data = read_data << 1;
		//printk("read_data=0x%08x\n", read_data);

	return read_data;
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************
}

// Read the state of SCL (I2C Clock) Line	
OT_U8 OT_HAL_BitRead_I2C_SCL(void)
{
	return (GET_TOUCHPAD_SCL >> 18); //pin18	
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************

}

//	Read the state of the Attention (ATTN) line
OT_U8 OT_HAL_BitRead_ATTN(void)			
{
	return GET_TOUCHPAD_INTR;	
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************

}

// Application Example Prototype defintions 
//void 	Buttons_Process_Example(void);


//
// Function Name: main
// Functionality : This is the main entry function.
//
 
OT_U8 pOneTouchDataRegs[OT_NUM_DATA_REG_BYTES]; // OneTouch Data Registers
OT_U8 Interrupt_Flag = OT_FALSE;

#if 0
OT_U8 touchpad_read()
{

	OT_U8 Result;
	
	gpio_init();

	OT_Init_BitBang_I2C();			// Do not remove this. 

	if( OT_Init()==OT_FAILURE )
	{
		printk("OT_Init fail\n");	
		// put your initializataion code here
	}		

}
#endif



OT_U8 OT_touchpad_init(void) //main(void)
{
	OT_U8 Result;
	
	gpio_init();

// ********************************************************
// ADD code HERE to initialize host and attention lines *
// ********************************************************	

		// Initialise the Host Processor
		// Initialize the Attention Line as input

// *************************************************************
// The USER writes code to initialize host and attention lines *
// *************************************************************		
//	printk("%s, %d\n", __FILE__, __LINE__);	

	// Initialize I2C data and Clock line 
	OT_Init_BitBang_I2C();			// Do not remove this. 

//	printk("%s, %d\n", __FILE__, __LINE__);	
		
	//  Initialize OneTouch Device by writing to the Config Registers

	// If the initialization fails, either retry to initialize for a specific number
	// of times or display error in initialization
	if( OT_Init()==OT_FAILURE )
	{
		//printk("OT_Init fail\n");	
		return OT_FAILURE;
		// put your initializataion code here
	}		
//	printk("%s, %d\n", __FILE__, __LINE__);	

#if 0
	// main loop -- Your application main loop goes here	
	while(1) 
	{
			
		if(Interrupt_Flag == OT_TRUE) 	// If Interrupt Flag is set
		{
			Interrupt_Flag = OT_FALSE;

			// Read the OT Data Registers
			if(OT_ReadDataReg(pOneTouchDataRegs) == OT_SUCCESS)
			{
	
 				// Invoke the Button function, if no error	
				Buttons_Process_Example();
				
			}	// end of OT_ReadDataReg() - if
		} 
	}	// end of while(1)
#endif

} // end of main()

// Interrupt Service Routine
// Interrupt from the Attention Line

void AttnIntHandler(void)
{
	Interrupt_Flag = OT_TRUE;		
}


// wait for attention line to be Asserted with timeout 

// If this function is not implemented then the 
// default function with no timing control is used
	
OT_U8 OT_Poll_Attn_Line_TimeOut(void)
{
	OT_U32 bCounter=0;
	
	// wait until attention line is asserted or timer timeout
	do{
		bCounter++;
	}
	while( ( (OT_HAL_BitRead_ATTN() ==1) & (bCounter < OT_TIMEOUT)) );
	
	if(OT_HAL_BitRead_ATTN()== 0)						
		return OT_SUCCESS;
	
	return OT_FAILURE;			
}

// Button Function to do button processing
void Buttons_Process_Example(void)
{

// Write your application specific button processing here

// Uncomment this to use with Category 1A board.

// Example Button function where it toggles the LED on Category-1A board when a
// button is pressed

// Button mapping is as follows:
// pOneTouchDataRegs[0] = Buttons [15:8]
// pOneTouchDataRegs[1] = Buttons [7:0]

// For Category 1A sensor module
// Top left = Button-15
// Top right = Button-12
// Bottom left = Button-7
// Bottom right = Button-4

static OT_U8  pButtonBuffer[4]={0x00, 0x0E, 0x1F, 0x1F};

	if(pOneTouchDataRegs[0+OT_BUTTON_OFFSET] &0x80)
		pButtonBuffer[3] ^= 0x02;

	if(pOneTouchDataRegs[0+OT_BUTTON_OFFSET] &0x10)
		pButtonBuffer[3] ^= 0x04;		

	if(pOneTouchDataRegs[1+OT_BUTTON_OFFSET] &0x80)
		pButtonBuffer[3] ^= 0x08;	

	if(pOneTouchDataRegs[1+OT_BUTTON_OFFSET] &0x10)
		pButtonBuffer[3] ^= 0x10;			
		
	OT_WriteI2C(OT_ADDR, pButtonBuffer, 4);
	
	OT_Set_ReadAddress();	
}




