#include <linux/delay.h>
#include "stmp37xxfb_controller.h"
#include "stmp37xxfb_panel.h"
#include <linux/module.h>

#if defined CONFIG_FB_STMP37XX_TOPSON
static int backlight_level;
static int bInited = 0;
static int bRotated = 0;

void lcd_panel_rotation (int rotate)
{
	if (!bInited || rotate == bRotated)
		return;

	if (rotate)
		write_register(0x11, 0x6800);	//AM=0, ID=00
	else
	 	write_register(0x11, 0x6818);	//AM=1, ID=01

	write_command_16(0x22);
	bRotated = rotate;
}

void lcd_panel_init (void)
{
	unsigned i;
	static const unsigned short seq[] = {
		0x00, 0x0001,	// Start oscillation
		0x03, 0xa8a4,	// Power Control 1 - 262k
		0x0c, 0x0000,	// Power Control 2
		0x0d, 0x030c,	// Power Control 3
		0x0e, 0x2d00,	// Power Control 4
		0x1e, 0x00b0,	// Power Control 5
		0x01, 0x693f,	// Driver output control - (262k?)
		0x02, 0x0e00,	// LCD driving waveform control, WSYNC mode 0
		0x10, 0x0000,	// Sleep mode disabled
		// Application note is wrong for this register
		// 0x11, 0x4030,	// Entry mode - 262k
		// This is the place to change rotation mode
#if LCD_WIDTH == 320
		0x11, 0x6818,	// Entry mode - 65k, AM=1, ID=01, window
#else
		0x11, 0x6800,	// Entry mode - 65k, AM=0, ID=00
#endif
		0x05, 0x0000,	// Compare register
		0x06, 0x0000,	// Compare register
		0x16, 0xef1c,	// Horizontal porch (240 lines)
		0x17, 0x0003,	// Vertical porch
		0x07, 0x0233,	// Display control - (262k?)
		0x0b, 0x0000,	// Frame cycle control
		0x0f, 0x0000,	// Gate scan position
		0x41, 0x0000,	// Vertical scroll control
		0x42, 0x0000,	// Vertical scroll control
		0x48, 0x0000,	// 1st screen driving position
		0x49, 0x013f,	// 1st screen driving position // !!
		0x4a, 0x0000,	// 2nd screen driving position
		0x4b, 0x0000,	// 2nd screen driving position
		// Horizontal data update window.
		0x44, 0xef00,	// Horizontal ram address position (239)
		// Vertical data update window.
		0x45, 0x0000,	// Window vertical RAM address
		0x46, 0x013f,	// Window vertical RAM address = 319
		// Initial data address. Due to our 90 degree rotation
		// this is the bottom left of our update window when
		// viewing the panel in portait mode.
		0x4e, 0x0000,	// Initial X address
#if LCD_WIDTH == 320
		0x4f, LCD_WIDTH - 1,	// Initial Y address
#else
		0x4f, 0,	// Initial Y address
#endif
		0x30, 0x0707,	// Gamma control
		0x31, 0x0204,	// Gamma control
		0x32, 0x0204,	// Gamma control
		0x33, 0x0502,	// Gamma control
		0x34, 0x0507,	// Gamma control
		0x35, 0x0204,	// Gamma control
		0x36, 0x0204,	// Gamma control
		0x37, 0x0502,	// Gamma control
		0x3a, 0x0302,	// Gamma control
		0x3b, 0x0302,	// Gamma control
		0x23, 0x0000,	// Ram write data mask = 0
		0x24, 0x0000,	// Ram write data mask = 0
	};
	
	for(i = 0; i < sizeof(seq) / sizeof(seq[0]); i += 2)
		write_register(seq[i], seq[i + 1]);

	// Set register index to 0x22 to write screen data
	write_command_16(0x22);
	bInited = 1;
	// That setup a 240x320 window, rotated so we can output 320x240.
}

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
#if LCD_WIDTH == 320
		0x4f, LCD_WIDTH - 1,	// Initial Y address
#else
		0x4f, 0,	// Initial Y address
#endif
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
#ifdef USE_OLD_GPIO
	// Set PWM lines to GPIOs
	HW_PINCTRL_DOUT2_CLR(1 << 2);	
	HW_PINCTRL_DOE2_SET(1 << 2);
	HW_PINCTRL_MUXSEL4_SET(3 << 4);
#else
	stmp37xx_config_pin(ARMA_GPIO_LCD_BACKLIGHT);
#endif
	backlight_level = 100;
	lcd_bl_power(1);
	return 0;
}

int lcd_bl_power (int on)
{
	if (on) {
		lcd_bl_control(lcd_bl_getlevel());
	} else {
#ifdef USE_OLD_GPIO
		// Send shutdown: hold line low for > 2 ms
		HW_PINCTRL_DOUT2_CLR(1 << 2);
#else
		stmp37xx_gpio_set_level(ARMA_GPIO_LCD_BACKLIGHT, 0);
#endif
		mdelay(3);
	}
	return 0;
}

int lcd_bl_control (unsigned level)
{
	// See http://www.allegromicro.com/en/Products/Part_Numbers/8435/8435.pdf for details
	unsigned i;
	const unsigned max_level = 11;

	// level 0 - 100%
	if (level > 100)
		level = 100;
	backlight_level = level;

	/* level 0 - 11: Off, 5%, 10%, 20%, 30%, 40%, 50%, 60%, 70%, 80%, 90%, 100% */
	level = level * max_level / 100;
	
#ifdef USE_OLD_GPIO
	// Send shutdown: hold line low for > 2 ms
	HW_PINCTRL_DOUT2_CLR(1 << 2);
	mdelay(3);
	
	// If we've been asked to shutdown (level == 0) we don't need to do anything else
	if (level == 0) 
		return 0;
	
	// Send init: high for > 100 us
	HW_PINCTRL_DOUT2_SET(1 << 2);
	udelay(150);
	
	// Send one pulse for each change in level - starting at 100%
	for (i = level; i < max_level; i++)
	{
		// Send pulse
		HW_PINCTRL_DOUT2_CLR(1 << 2);
		udelay(5);
		HW_PINCTRL_DOUT2_SET(1 << 2);
		udelay(5);
	}
#else
	/* ARMA37 only */
	stmp37xx_gpio_set_level(ARMA_GPIO_LCD_BACKLIGHT, 0);
	if (level == 0) 
		return 0;

	mdelay(3);

	stmp37xx_gpio_set_level(ARMA_GPIO_LCD_BACKLIGHT, 1);
	udelay(150);

	for (i = level; i < max_level; i++) {
		// Send pulse
		stmp37xx_gpio_set_level(ARMA_GPIO_LCD_BACKLIGHT, 0);
		udelay(5);
		stmp37xx_gpio_set_level(ARMA_GPIO_LCD_BACKLIGHT, 1);
		udelay(5);
	}
#endif /* !USE_OLD_GPIO */
	
	return 0;
}

int lcd_bl_getlevel (void)
{
	return backlight_level;
}

#endif

