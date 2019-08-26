#ifndef FMTUNER_IOCTL_H
#define FMTUNER_IOCTL_H

/********************************************************
 * IOCTL definition
 ********************************************************/
#define FM_MAGIC               0xCA
#define FM_SET_REGION          _IOW(FM_MAGIC, 0, void *)
#define FM_STEP_UP             _IO(FM_MAGIC, 1)
#define FM_STEP_DOWN           _IO(FM_MAGIC, 2)
#define FM_AUTO_UP             _IO(FM_MAGIC, 3)
#define FM_AUTO_DOWN           _IO(FM_MAGIC, 4)
#define FM_SET_FREQUENCY       _IOW(FM_MAGIC, 5, unsigned int)
#define FM_GET_FREQUENCY       _IOR(FM_MAGIC, 6, unsigned int)
#define FM_SET_VOLUME          _IOW(FM_MAGIC, 7, unsigned char)
#define FM_SET_RSSI            _IOW(FM_MAGIC, 8, unsigned short)
#define FM_GET_RDS_DATA        _IOR(FM_MAGIC, 9, unsigned char *)
#define FM_RX_POWER_UP         _IO(FM_MAGIC, 10)
#define FM_TX_POWER_UP         _IO(FM_MAGIC, 11)
#define FM_SET_CONFIGURATION   _IOW(FM_MAGIC, 12, unsigned short)
#define FM_IS_TUNED            _IO(FM_MAGIC, 13)
#define FM_SET_FREQUENCY_STEP  _IOW(FM_MAGIC, 14, unsigned char)
#define FM_POWER_DOWN          _IO(FM_MAGIC, 15)
#define FM_SET_MUTE            _IOW(FM_MAGIC, 16, unsigned char)
#define FM_GET_RSSI            _IOR(FM_MAGIC, 17, unsigned char)

#endif
