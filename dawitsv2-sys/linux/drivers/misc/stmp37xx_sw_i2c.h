#ifndef STMP37XX_SW_I2C_H
#define STMP37XX_SW_I2C_H

/************************************************
 * enum
 ************************************************/
enum {
	I2C_WRITE = 0,
	I2C_READ
};
/************************************************
 * Interface declearation
 ************************************************/
void sw_i2c_init(unsigned char address);
void sw_i2c_term(void);
char sw_i2c_read(unsigned char length, unsigned char *data_read);
char sw_i2c_write(unsigned char length, unsigned char *data_write);
/* Tools */
unsigned long fm_get_elasped(unsigned long begin);
unsigned long fm_get_current(void);
void fm_udelay(unsigned long delay);

#define uudelay(n)							\
	(__builtin_constant_p(n) ?					\
	  ((n) > (MAX_UDELAY_MS * 1000) ? __bad_udelay() :		\
			__const_udelay((n) * ((2199023U*HZ)>>11))) :	\
	  __udelay(n))

#endif
