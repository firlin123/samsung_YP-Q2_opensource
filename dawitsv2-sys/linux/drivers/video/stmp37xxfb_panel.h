#if !defined (STMP37XXFB_PANEL_H)
#define STMP37XXFB_PANEL_H

#if 0
# define LCD_WIDTH		320
# define LCD_HEIGHT		240
#else
# define LCD_WIDTH		240
# define LCD_HEIGHT		320
#endif

#define MAX_CHAIN_LEN		3	// Need 3 chains to cover 150KB

// LCD_CYCLE_TIME_NS
// 		82n-PIXCLK24M, 100-PIXCLK20M
#if defined CONFIG_FB_STMP37XX_CHILIN
	#define LCD_CYCLE_TIME_NS	82
	#define LCD_DATA_SETUP		2
	#define LCD_DATA_HOLD		2
	#define LCD_CMD_SETUP		3
	#define LCD_CMD_HOLD		3

#elif defined CONFIG_FB_STMP37XX_TOPSON
	#define LCD_CYCLE_TIME_NS	82
	#define LCD_DATA_SETUP		1
	#define LCD_DATA_HOLD		1
	#define LCD_CMD_SETUP		1
	#define LCD_CMD_HOLD		1

#elif defined CONFIG_FB_STMP37XX_TOPSON_OLD
	#define LCD_CYCLE_TIME_NS	82	 
	#define LCD_DATA_SETUP		1
	#define LCD_DATA_HOLD		1
	#define LCD_CMD_SETUP		1
	#define LCD_CMD_HOLD		1

#elif defined CONFIG_FB_STMP37XX_DTC
	#define LCD_CYCLE_TIME_NS	82	 
	#define LCD_DATA_SETUP		1
	#define LCD_DATA_HOLD		1
	#define LCD_CMD_SETUP		1
	#define LCD_CMD_HOLD		1
#elif defined CONFIG_FB_STMP37XX_DTC_REV2
	#define LCD_CYCLE_TIME_NS	82	 
	#define LCD_DATA_SETUP		2 /* 2: for DF-240-TC-04_Ver0.4 */
	#define LCD_DATA_HOLD		1
	#define LCD_CMD_SETUP		2 /* 2: for DF-240-TC-04_Ver0.4 */
	#define LCD_CMD_HOLD		1
#endif

void lcd_panel_init(void);
void lcd_panel_term(void);
void lcd_panel_power(int on);
void lcd_panel_rotation(int rotate);
void lcd_panel_flip(int flip);

int lcd_bl_init(void);
int lcd_bl_power(int on);
int lcd_bl_control(unsigned long level);
int lcd_bl_getlevel(void);
void lcd_bl_setlevel(unsigned long);
int lcd_bl_on_off(unsigned char on);
#endif
