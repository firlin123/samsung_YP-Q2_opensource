#include <asm-arm/arch-stmp37xx/37xx/regsi2c.h>
#include <asm-arm/arch-stmp37xx/37xx/regsapbx.h>
#include <asm-arm/arch-stmp37xx/37xx/regsdigctl.h>
#include <asm-arm/arch-stmp37xx/37xx/regspinctrl.h>


/********** Definitions **********/

#define RESET_TIMEOUT  10


/********** Functions Declaration **********/

/**
 * Reset the I2C's DMA channel
 */
int reset_dma(void);

/**
 * Reset I2C
 */
int reset_i2c(void);

/**
 * Turn On everythign that needs to be turned on
 */
void i2c_dma_init(void);

extern int set_pin_func(int bank, int pin, int func) ;

/********** Functions Definition **********/

int reset_dma(void)
{
  	unsigned retries;
  	reg32_t reset_mask;

  	/* Reset APBX dma channels allocated to I2C */
  	reset_mask = BF_APBX_CTRL0_RESET_CHANNEL(1 << 3);
  	HW_APBX_CTRL0_SET(reset_mask);

  	/* Poll for reset to clear on channel 3 */
  	for (retries = 0; retries < RESET_TIMEOUT; retries++)
    	if ((reset_mask & HW_APBX_CTRL0_RD()) == 0)
        	break;

   	/* Clear IRQ bit */    
  	BF_WR(APBX_CTRL1, CH3_CMDCMPLT_IRQ, 0);
  	
  	return 0;
}

int reset_i2c(void)
{
   	unsigned long ulTemp;
   	
   	HW_I2C_CTRL0_SET(BM_I2C_CTRL0_SFTRST);
   	ulTemp = HW_DIGCTL_MICROSECONDS.U;
   	ulTemp += 3;  // wait 3 microsends
   	
   	while(ulTemp > HW_DIGCTL_MICROSECONDS.U);
   	HW_I2C_CTRL0_CLR(BM_I2C_CTRL0_SFTRST | BM_I2C_CTRL0_CLKGATE);
   	HW_I2C_TIMING2_WR( BF_I2C_TIMING2_BUS_FREE(0x100) | BF_I2C_TIMING2_LEADIN_COUNT(0xFE));

   	/* Clear IRQ bits */
   	HW_I2C_CTRL1_CLR(0x000000FF);   	
    
  	return 0;
}

void i2c_dma_init(void)
{
	//unsi ned int i;
  	//NTK	FW_HW_READ_CHECK(HW_I2C_CTRL0,0xC0000000,0xFFFFFFFF,0x00000001);
  	// HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE ); // unncessary 
	
	reset_i2c(); // hcyun 
	reset_dma();

#if 0 
  	HW_PINCTRL_MUXSEL7_CLR(0x0000003C);
#else // hcyun 
	set_pin_func(2,5,3); /* I2C SCL */ // new i2c: uses gpio
	set_pin_func(2,6,3); /* I2C SDA */ // new i2c: uses gpio
#endif 
 
  	/* turn off soft reset and clock gating in i2c */
  	HW_I2C_CTRL0_CLR(BM_I2C_CTRL0_SFTRST | BM_I2C_CTRL0_CLKGATE);

  	HW_I2C_TIMING2_WR(BF_I2C_TIMING2_BUS_FREE(0x100) | BF_I2C_TIMING2_LEADIN_COUNT(0xFE));
  	HW_I2C_CTRL1_SET(0x0000ff00);	//NTK	i2c interrupt enable

  	/* start DMA */
  	HW_APBX_CTRL0_CLR(BM_APBX_CTRL0_SFTRST | BM_APBX_CTRL0_CLKGATE);
}
