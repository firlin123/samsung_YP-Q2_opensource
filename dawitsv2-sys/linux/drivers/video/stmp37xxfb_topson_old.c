#include <linux/delay.h>
#include "stmp37xxfb_controller.h"
#include "stmp37xxfb_panel.h"
#include <linux/module.h>

#ifdef CONFIG_FB_STMP37XX_TOPSON_OLD

#define LCD_BL_PWM_CHANNEL_NO  2
#define LCD_BL_PWM_MAXDURATION 80


//! Driver output control register of the SSD1289
typedef union _DriverOutputControl 
{
    uint16_t V;
    struct {
        uint16_t MUX:9;
        uint8_t TB:1;
        uint8_t SM:1;
        uint8_t BGR:1;
        uint8_t CAD:1;
        uint8_t REV:1;
        uint8_t RL:1;
        uint8_t Reserved:1;
    } B;
} SSD1289DriverOutputControl;

//! Entry mode register of the SSD1289
typedef union _EntryMode 
{
    __u16 V;
    struct {
        __u8 LG:3;
        __u8 AM:1;
        __u8 ID:2;
        __u8 TY:2;
        __u8 DMode:2;
        __u8 WMode:1;
        __u8 OEDef:1;
        __u8 TRANS:1;
        __u8 DFM:2;
        __u8 VSMode:1;
    } B;
} SSD1289EntryMode;

extern void pwm_change (int channel, uint32_t freq, int brightness);
extern void pwm_on (int channel);
extern void pwm_off (int channel);

static int backlight_level;
static int bInited = 0;
static int bRotated = 0;
static int bFliped = 0;

/* Entry mode - 65k, AM=0, ID=11 */
static SSD1289EntryMode entry_mode = { 0x6830 };
/* Driver output control - SM0 TB1 RL0 */
static SSD1289DriverOutputControl output_control = { 0x2b3f };

void lcd_panel_flip (int flip)
{
	if (!bInited || flip == bFliped)
		return;

	if (flip) {
		output_control.B.TB = 1;
		output_control.B.RL = 1;
	}
	else {
		output_control.B.TB = 1;
		output_control.B.RL = 0;
	}
	write_register(0x01, output_control.V);
	write_command_16(0x22);
	bFliped = flip;
}

void lcd_panel_rotation (int rotate)
{
	if (!bInited || rotate == bRotated)
		return;

	if (rotate) {
		entry_mode.B.ID = 3;
		entry_mode.B.AM = 0;
	}
	else {
		entry_mode.B.ID = 2;
		entry_mode.B.AM = 1;
	}
	write_register(0x11, entry_mode.V);
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
		//0x01, 0x693f,	// Driver output control - SM0 TB1 RL1
		0x01, 0x2b3f,	// Driver output control - SM0 TB1 RL0
		//0x02, 0x0e00,	// LCD driving waveform control, WSYNC mode 0
		0x02, 0x0600,	// LCD driving waveform control, WSYNC mode 0
		0x10, 0x0000,	// Sleep mode disabled

		// Application note is wrong for this register
		// 0x11, 0x4030,	// Entry mode - 262k
		// This is the place to change rotation mode
		0x11, 0x6830,	// Entry mode - 65k, AM=0, ID=11
		//0x11, 0x6818,	// Entry mode - 65k, AM=1, ID=01, window
		//0x11, 0x6830,	// Entry mode - 65k, AM=0, ID=11

		0x05, 0x0000,	// Compare register
		0x06, 0x0000,	// Compare register
		0x16, 0xef1c, 	// Horizontal porch (240 lines)
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
		0x4f, 0x0000,	// Initial Y address
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

	bInited = 1;
	// Set register index to 0x22 to write screen data
	write_command_16(0x22);
	// That setup a 240x320 window, rotated so we can output 320x240.
}

void lcd_panel_power (int on)
{
	#if 0
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
	#else
	if (on)
	{
		write_register(0x10, 0x0000);
	}
	else
	{
		write_register(0x10, 0x0001);
	}
	#endif
}

int lcd_bl_init (void)
{
	backlight_level = 100;
	lcd_bl_power(1);
	return 0;
}


int lcd_bl_power (int on)
{
	if (on) {
		pwm_on(LCD_BL_PWM_CHANNEL_NO);
		lcd_bl_control(lcd_bl_getlevel());
	} else {
		pwm_off(LCD_BL_PWM_CHANNEL_NO);
	}
	return 0;
}

/* level 0-100% */
int lcd_bl_control (unsigned level)
{
	if (level > 100) {
		level = 100;
	}
	backlight_level = level;
	pwm_change(LCD_BL_PWM_CHANNEL_NO, 120*1000, backlight_level * LCD_BL_PWM_MAXDURATION / 100);
	return 0;
}

int lcd_bl_getlevel (void)
{
	return backlight_level;
}

#endif

