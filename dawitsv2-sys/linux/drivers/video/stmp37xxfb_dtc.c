/** 
 * @file        stmp37xxfb_dtc.c
 * @brief       implementation for LCD driver IC
 */
#include <linux/delay.h>
#include "stmp37xxfb_controller.h"
#include "stmp37xxfb_panel.h"
#include <linux/module.h>

static unsigned long backlight_level = 5;
static int bInited = 1;
static int bRotated = 0;
static int bFliped = 0;

spinlock_t backlight_lock = SPIN_LOCK_UNLOCKED;

static const unsigned long brightness_table[] = {
	0, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15
};
/**
 * @brief        check option for LCD
 */
static unsigned int detect_lcd_type(void)
{
	/* Option of BANK2_PIN03 */
	HW_PINCTRL_MUXSEL4_SET(BM_PINCTRL_MUXSEL4_BANK2_PIN03);
	HW_PINCTRL_DOE2_CLR(1 << 0x03);

	return ((HW_PINCTRL_DIN2_RD() & (0x01 << 0x03)) >> 0x03);
}
/**
 * @brief        adjust LCD flip
 */
void lcd_panel_flip (int flip)
{
	if (!bInited || flip == bFliped) {
		return;
	}

	/* By leeth, for new LCD at 20090502 */
	if (detect_lcd_type()) {
 		write_register(0x0050, 0x0000); 
 		write_register(0x0051, 0x00EF); 
 		write_register(0x0052, 0x0000); 
 		write_register(0x0053, 0x013F); 
	}
	
	if (flip) {
		/* By leeth, fixed a vertical line moved problem */
		/* in old LCD at 20090502 */
		if(!detect_lcd_type()) {
			write_register(0x20, 0x00EF);
		}
		write_register(0x03, 0x1020);
	} else {
		/* By leeth, fixed a vertical line moved problem */
		/* in old LCD at 20090502 */
		if(!detect_lcd_type()) {
			write_register(0x20, 0x0000);
		}
		write_register(0x03, 0x1030);
	}
	write_command_16(0x22);
	bFliped = flip;
}
/**
 * @brief        adjust LCD rotation
 */
void lcd_panel_rotation (int rotate)
{
	if (!bInited || rotate == bRotated) {
		return;
	}

	/* By leeth, for new LCD at 20090502 */
	if (detect_lcd_type()) {
 		write_register(0x0050, 0x0000); 
 		write_register(0x0051, 0x00EF); 
 		write_register(0x0052, 0x0000); 
 		write_register(0x0053, 0x013F); 
	}
	
	switch(rotate) {
	case 90:
		/* By leeth, fixed a vertical line moved problem */
		/* in old LCD at 20090502 */
		if(!detect_lcd_type()) {
			write_register(0x20, 0x0000);
			write_register(0x21, 0x013F);
		}
		write_register(0x03, 0x1018);
		break;
	case 180:
		/* By leeth, fixed a vertical line moved problem */
		/* in old LCD at 20090502 */
		if(!detect_lcd_type()) {
			write_register(0x20, 0x00EF);
			write_register(0x21, 0x013F);
		}
		write_register(0x03, 0x1000);
		break;
	case 270:
		/* By leeth, fixed a vertical line moved problem */
		/* in old LCD at 20090502 */
		if(!detect_lcd_type()) {
			write_register(0x20, 0x00EF);
			write_register(0x21, 0x0000);
		}
		write_register(0x03, 0x1028);
		break;
	case 0:
		/* By leeth, fixed a vertical line moved problem */
		/* in old LCD at 20090502 */
		if(!detect_lcd_type()) {
			write_register(0x20, 0x0000);
			write_register(0x21, 0x0000);
		}
		write_register(0x03, 0x1030);
		break;
	}
	write_command_16(0x22);
	bRotated = rotate;
}
/**
 * @brief        initializes LCD
 */
void lcd_panel_init (void)
{
	if(detect_lcd_type()) 	{
		msleep(10);

		write_register(0x00FF, 0x0001);           
		write_register(0x00F3, 0x0008);           
		write_command_16(0x00F3);
		/* READ SIGNAL HIGH */  

		/* sRST already performed in previous stage */
		write_register(0x0001, 0x0100);
		write_register(0x0002, 0x0700);
		write_register(0x0003, 0x1030);
		write_register(0x0007, 0x0121);
		write_register(0x0008, 0x0302);
		write_register(0x0009, 0x0200);
		write_register(0x000A, 0x0000);
		
		write_register(0x0010, 0x0790);
		write_register(0x0011, 0x0005);
		write_register(0x0012, 0x0000);
		write_register(0x0013, 0x0000);
		msleep(50);
		write_register(0x0010, 0x1290);
		msleep(50);
		write_register(0x0011, 0x0212);
		msleep(50);
		write_register(0x0012, 0x008A);
		write_register(0x0013, 0x0F00);
		write_register(0x0029, 0x0008);
		msleep(50);
				
		/* Gamma control { */
		write_register(0x0030, 0x0200);
		write_register(0x0031, 0x0407);
		write_register(0x0032, 0x0103);
		write_register(0x0035, 0x0003);
		write_register(0x0036, 0x0C04);
		
		write_register(0x0037, 0x0200);
		write_register(0x0038, 0x0406);
		write_register(0x0039, 0x0204);
		write_register(0x003C, 0x0301);
		write_register(0x003D, 0x0E02);
		/* Gamma control } */
		
		write_register(0x0050, 0x0000);
		write_register(0x0051, 0x00EF);
		write_register(0x0052, 0x0000);
		write_register(0x0053, 0x013F);
		write_register(0x0060, 0xA700);
		write_register(0x0061, 0x0001);
		
		write_register(0x0090, 0x002A);
		write_register(0x0092, 0x0100);
		
		write_register(0x0009, 0x0000);
		write_register(0x0007, 0x0133);
		/* Set register index to 0x22 to write screen data */
		//write_register(0x0022, 0x0000);
		/* Inserted by Park YoungMIn 4/.26 */
		write_command_16(0x22);
		
		mdelay(50);
	} else {
		msleep(10);
		/* sRST already performed in previous stage */
		write_register(0x0007, 0x0000);
		write_register(0x0012, 0x0000);
		msleep(15);
		
		write_command_16(0x0000);
		write_command_16(0x0000);
		write_command_16(0x0000);
		write_command_16(0x0000);
		
		write_register(0x00A4, 0x0001);
		msleep(15);
		
		write_register(0x0090, 0x0013);
		write_register(0x0060, 0x2700);
		write_register(0x0008, 0x0505);
		
		/* Gamma control { */
		write_register(0x0030, 0x0407);
		write_register(0x0031, 0x0404);
		write_register(0x0032, 0x0301);
		write_register(0x0033, 0x0303);
		write_register(0x0034, 0x0202);
//JS-Edit --> 2009.05.11.
//		write_register(0x0035, 0x0405);
		write_register(0x0035, 0x0401);
//JS-Edit <--
		write_register(0x0036, 0x131F);
		write_register(0x0037, 0x0706);
		write_register(0x0038, 0x0503);
		write_register(0x0039, 0x0702);
		write_register(0x003A, 0x0303);
		write_register(0x003B, 0x0303);
//JS-Edit --> 2009.05.11.
//		write_register(0x003C, 0x0404);
		write_register(0x003C, 0x0401);
//JS-Edit <--		
		write_register(0x003D, 0x1A1F);
		/* Gamma control } */
		
		/* Power supply on sequence */
		write_register(0x0007, 0x0001);
		write_register(0x0017, 0x0001);
		write_register(0x0019, 0x0000);
		
		write_register(0x0010, 0x77F0);
		msleep(200); /* Added 2009.02.03 */
		write_register(0x0011, 0x0136);
		msleep(60);
		
		write_register(0x0012, 0x019A);
//JS-Edit --> 2009.05.11.
//		write_register(0x0013, 0x0400);
//		write_register(0x0029, 0x0007);
		write_register(0x0013, 0x0900);
		write_register(0x0029, 0x0003);
//JS-Edit <--
		msleep(40);

//JS-Edit --> 2009.05.11.	
//		write_register(0x0012, 0x01BA);
		write_register(0x0012, 0x01BB);
//JS-Edit <--
		msleep(120); /* Recommanded by DTC */
		
		write_register(0x0001, 0x0100);
		write_register(0x0002, 0x0300);
		write_register(0x0003, 0x1030);
		write_register(0x0004, 0x0000);
		write_register(0x0009, 0x0000);
		write_register(0x000A, 0x0008);
		write_register(0x000C, 0x0003);
		write_register(0x000D, 0x0140);
		
		write_register(0x0050, 0x0000);
		write_register(0x0051, 0x00EF);
		write_register(0x0052, 0x0000);
		write_register(0x0053, 0x013F);
		
		write_register(0x0061, 0x0001);
		write_register(0x0092, 0x0100);
		write_register(0x0093, 0x0001);
		msleep(15);
		
		write_register(0x0007, 0x0021);
		msleep(1);
		write_register(0x0010, 0x77F0);
		msleep(200);
		write_register(0x0011, 0x0132);
		msleep(60);
		write_register(0x0007, 0x0061);
		msleep(25);
		
		write_register(0x0007, 0x0173);

		/* Set register index to 0x22 to write screen data */
		write_command_16(0x22);
	}
	
	bInited = 1;
	bFliped = 0;
	bRotated = 0;
	/* That setup a 240x320 window, rotated so we can output 320x240. */
}
/**
 * @brief        terminates  LCD
 */
void lcd_panel_term(void)
{
	if(detect_lcd_type()) 	{
		/* Display off */
		write_register(0x0007, 0x0020);

		/* Power off */
		write_register(0x0010, 0x0002);
		write_register(0x0012, 0x0000);
		write_register(0x0010, 0x0000);
	} else {
		/* Display off */
		udelay(1000);
		write_register(0x0007, 0x0072);
		msleep(40);
		write_register(0x0007, 0x0001);
		msleep(40);
		write_register(0x0007, 0x0000);
		msleep(40);

		/* Power off */
		write_register(0x0010, 0x0080);
		write_register(0x0011, 0x0108);
		write_register(0x0012, 0x0000);
		msleep(40);
		write_register(0x0010, 0x0000);

		/* Set sleep mode */
		/* write_register(0x0010, 0x0002); */
	}

	bInited = 0;
	bFliped = 0;
	bRotated = 0;
}
/**
 * @brief        controls panel power
 */
void lcd_panel_power (int on)
{
	unsigned i;
	static const unsigned short seq[] = {
		0x44, 0xef00,	// Horizontal ram address position (239)
		// Vertical data update window.
		0x45, 0x0000,	// Window vertical RAM address
		0x46, 0x013f,	// Window vertical RAM address = 319
		// Initial data address. Due to our 90 degree rotation
		// this is the bottom left of our update window when
		// viewing the panel in portait mode.
		0x4e, 0x0000,	// Initial X address
		0x4f, 0x0000,	// Initial Y address
	};
	if (on)
	{
		for(i = 0; i < sizeof(seq) / sizeof(seq[0]); i += 2)
			write_register(seq[i], seq[i + 1]);
		// display on
		write_register(0x07, 0x0233);
		// Set register index to 0x22 to write screen data
		write_command_16(0x22);
	}
	else
	{
		// display off
		write_register(0x07, 0x0231);
	}
}

int lcd_bl_init (void)
{
	//backlight_level = 5;
	lcd_bl_power(1);
	return 0;
}


int lcd_bl_power (int on)
{
	if (on) {
		//lcd_bl_control(lcd_bl_getlevel());
		lcd_bl_on_off(1);
	} else {
		lcd_bl_on_off(0);
	}

	return 0;
}

int lcd_bl_on_off(unsigned char on)
{
	if (on) {
		lcd_bl_control(backlight_level);
	} else {
		stmp37xx_gpio_set_af(BANK2_PIN13, 3);
		stmp37xx_gpio_set_dir(BANK2_PIN13, 1);
		stmp37xx_gpio_set_level(BANK2_PIN13, on);
	}
	return 0;
}
/* Charging pump: AAT3151 */
#define AAT3151_BASE 16
enum /* Register index */
{
	REG_D1D4 = AAT3151_BASE + 0x01, 
	REG_D1D3, 
	REG_D4, 
	REG_MAX, 
	REG_LOW, 
};

enum /* Data for REG_MAX register */
{
	MAX20MA = 0x01, 
	MAX30MA, 
	MAX15MA, 
	LCMODE, 
};
/**
 * @brief        controls charging pump with gpio
 */
static void aat3151_write(unsigned long pulse_count)
{
	int i;

	for(i = 0; i < pulse_count; i++) {
		stmp37xx_gpio_set_level(BANK2_PIN13, 0);
		udelay(25);
		stmp37xx_gpio_set_level(BANK2_PIN13, 1);
		udelay(25);
	}
}
/**
 * @brief        send address and data
 */
static void aat3151_set(unsigned long address, unsigned long data)
{
	/* unsigned long flags; */

	/* local_irq_save(flags); */
	aat3151_write(address);                 /* send register address */
	udelay(500);                            /* Latch timming */
	aat3151_write(data);                    /* send data */
	udelay(500);                            /* Latch timming */
	/* local_irq_restore(flags); */
}
/**
 * @brief        controls the level of backlight
 */
int lcd_bl_control(unsigned long level)
{
	unsigned long reg_data;
	unsigned long flags;

	if (level > 10 || level < 0) {
		level = 10;
	}

	spin_lock_irqsave(&backlight_lock, flags);

	backlight_level = level;
	reg_data = 16 - brightness_table[backlight_level];

	if(reg_data == 16) {
		aat3151_set(REG_LOW, 12); /* D1-D3: 2ma, D4: 0ma */
		aat3151_set(REG_MAX, LCMODE);
	} else {
		aat3151_set(REG_MAX, MAX15MA);
		aat3151_set(REG_D1D4, reg_data);
	}

	spin_unlock_irqrestore(&backlight_lock, flags);

	return 0;
}
/**
 * @brief        notifies current backlight level
 */
int lcd_bl_getlevel (void)
{
	return backlight_level;
}
/**
 * @brief        set backlight level
 */
void lcd_bl_setlevel (unsigned long arg)
{
	if (arg > 10 || arg < 0) {
		arg = 10;
	}
	backlight_level = arg;
}
