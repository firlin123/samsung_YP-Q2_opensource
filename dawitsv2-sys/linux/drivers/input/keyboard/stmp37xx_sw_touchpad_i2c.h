#ifndef STMP37XX_SW_I2C_TOUCH_H
#define STMP37XX_SW_I2C_TOUCH_H

//#define FAST_ISR_VER //20081006, read 1 byte for improving isr processing

/************************************************
 * enum
 ************************************************/
enum {
	TOUCHPAD_I2C_WRITE = 0,
	TOUCHPAD_I2C_READ
};
/************************************************
 * Interface declearation
 ************************************************/
void sw_touchpad_i2c_init(unsigned char address);
void sw_touchpad_i2c_term(void);
char sw_touchpad_i2c_prepare_read(unsigned char length, unsigned char *data_read);
char sw_touchpad_i2c_read(unsigned char length, unsigned char *data_read);
char sw_touchpad_i2c_write(unsigned char length, unsigned char *data_write);

#endif
