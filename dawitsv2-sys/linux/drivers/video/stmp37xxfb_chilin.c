#include <linux/delay.h>
#include "stmp37xxfb_controller.h"
#include "stmp37xxfb_panel.h"
#include <linux/module.h>

#ifdef CONFIG_FB_STMP37XX_CHILIN

#define LCD_BL_PWM_CHANNEL_NO  2
#define LCD_BL_PWM_MAXDURATION 70

extern void pwm_change (int channel, uint32_t freq, int brightness);
extern void pwm_on (int channel);
extern void pwm_off (int channel);

static int backlight_level;
static int bInited = 0;
static int bRotated = 0;
static int bFliped = 0;


static inline void __write_command (int reg, int cmd)
{
	write_command_16((reg<< 8) | cmd);
}

void lcd_panel_flip (int flip)
{
	if (!bInited || flip == bFliped)
		return;

	if (flip) {
		__write_command(0x01, 0x00);
	}
	else {
		__write_command(0x01, 0x80);
	}
	bFliped = flip;
}

void lcd_panel_rotation (int rotate)
{
#if 0
	if (!bInited || rotate == bRotated)
		return;

	if (rotate)
	{
		//AM-0 ADR-1 ADX-1, WAS-0
		__write_command(0x01, 0xC0);	//R1 - ADX-0(7), ADR-1(6)
		__write_command(0x05, 0x00);	//R5 - WAS-0(4), AM-1(2), 
	}
	else
	{
		//AM-1 ADR-1 ADX-0, WAS-0
		__write_command(0x01, 0x40);	//R1 - ADX-0(7), ADR-1(6)
		__write_command(0x05, 0x04);	//R5 - WAS-0(4), AM-1(2), 
	}

	bRotated = rotate;
#endif
}

void lcd_panel_init (void)
{
	unsigned i;
	static const unsigned short seq[] = {
		/* Start power-up sequence */
		0x01, 0x00,
		192,  0x00,
		0x00, 0x60,	 //R0, ddi_controller_hx8312a_r0.value,
		0x03, 0x01,
		0xff, 10000, //delay
		0x03, 0x00,	// R03
		0x2B, 0x05,	// R43
		0x28, 0x18,	// R40
		0x1A, 0x05,	// R26
		0x25, 0x05,	// R37
		0x19, 0x00,	// R25

		// System mode D0 = 1 is <RGB> and D0 = 0 <BGR>
		// RGB mode may be opposite.
		//__write_command(0xC1, 0x00);
		0xC1, 0x01,

		0x1C, 0x73,	// R28
		0x24, 0x74,	// R36

		0x18, 0xC1,	// R24
		0xff, 10000, //delay
		0x1E, 0x01,	// R30
		0x18, 0xC5,	// R24
		0x18, 0xE5,	// R24
		0xff, 60000, //delay
		0x18, 0xF5,	// R24
		0xff, 60000,

		0x1B, 0x0D,	// R27
		0x1F, 0x0E,	// R31
		0x20, 0x10,	// R32
		0x1E, 0x81,	// R30

		0xff, 10000, //delay
		/* End power-up sequence */

		0x3E, 0x01,	// R62
		0x3F, 0x3F,	// R63
		0x46, 0xEF,	// R70
		0x49, 0x01,	// R73
		0x4A, 0x3F,	// R74
		0x87, 0x39,	// R
		0x89, 0x05,	// R
		0x8D, 0x01,	// R
		0x8B, 0x3E,	// R
		0x37, 0x01,	// R
		0x90, 0x77,	// R
		0x91, 0x07,	// R
		0x92, 0x54,	// R
		0x93, 0x07,	// R
		0x95, 0x77,	// R
		0x96, 0x45,	// R
		0x98, 0x06,	// R
		0x99, 0x03,	// R

		#if 1 //VMODE 1
		0x02, 0x10,	// R2
		#else
		0x02, 0x00,	// R2
		#endif

		0x3B, 0x01,	// R

		/* Reset register 0 and 1 as the ChiLin seems to not like
		 * having other registers setup after these are setup.
		 */
		0x00,0x20, //R0
		0x1D,0x00, //0x08, R29

		0x01, 0x80,
		0x05, 0x00,
#if 0
		/* not using window mode right now */
		69, 0,					 // xmin
		70, (240-1) & 0xff,				 // xmax
		71, 0,					 // ymin
		72, 0,					 // ymin
		73, ((320-1) >> 8) & 0xff,		 // ymax
		74, (320-1) && 0xff,	 // ymax
		66, 0,					 // xaddress
		67, 0,					 // yaddress
		68, 0,					 // yaddress
#endif
	};

	for (i = 0; i < sizeof(seq) / sizeof(seq[0]); i += 2) {
		if (seq[i] == 0xFF)
			ndelay(seq[i+1]);
		else
			__write_command(seq[i], seq[i+1]);
	}

	bInited = 1;
}

void lcd_panel_power (int on)
{
	if (on)
	{
		lcd_panel_init();
	}
	else
	{
	   __write_command(0x00, 0x60);	//disp0 = 1
	   // Stop gate scanning
	   __write_command(59, 0x00);
	   __write_command(30, 0x01);
	   __write_command(27, 0x0C);
	   __write_command(24, 0xC0);
	   ndelay(10000);
	   __write_command(24, 0x00);
	   __write_command(28, 0x30);

	   __write_command(0x01, 0x01);	//oscstby = 1
	   __write_command(192, 0x80);
	}
}

int lcd_bl_init (void)
{
	backlight_level = 100;

//	HW_LRADC_CTRL0_CLR(BM_LRADC_CTRL0_SFTRST | BM_LRADC_CTRL0_CLKGATE);

	lcd_bl_power(1);
	return 0;
}

int lcd_bl_power (int on)
{
	if (on) {
		pwm_on(LCD_BL_PWM_CHANNEL_NO);
		pwm_change(LCD_BL_PWM_CHANNEL_NO, 500000, LCD_BL_PWM_MAXDURATION);

		BF_SET(PWM_CTRL, PWM2_ANA_CTRL_ENABLE);
		BF_SET(LRADC_CTRL2, BL_ENABLE);
		lcd_bl_control(lcd_bl_getlevel());

	} else {
		BF_CLR(LRADC_CTRL2, BL_ENABLE);
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

    HW_LRADC_CTRL2.B.BL_BRIGHTNESS = (0x1C * backlight_level)/100;
//	pwm_change(LCD_BL_PWM_CHANNEL_NO, 500000, backlight_level * LCD_BL_PWM_MAXDURATION / 100);

	return 0;
}

int lcd_bl_getlevel (void)
{
	return backlight_level;
}
#endif


