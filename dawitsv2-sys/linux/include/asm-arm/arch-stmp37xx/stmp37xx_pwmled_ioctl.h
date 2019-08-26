#ifndef PWMLED_IOCTL_H
#define PWMLED_IOCTL_H

/********************************************************
 * IOCTL definition
 ********************************************************/
#define PWMLED_MAGIC         0xAA
#define PWMLED_ON            _IO(PWMLED_MAGIC, 0)
#define PWMLED_OFF           _IO(PWMLED_MAGIC, 1)
#define PWMLED_BL_LEVEL      _IOW(PWMLED_MAGIC, 2, UC *)

#endif
