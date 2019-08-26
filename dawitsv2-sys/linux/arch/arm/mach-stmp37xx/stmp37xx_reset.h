#ifndef STMP37XX_TOUCHPAD_MEL_H
#define STMP37XX_TOUCHPAD_MEL_H

#define MELFAS_ADDR_FEEDBACK_FLAG                0x00
#define MELFAS_ADDR_LED_ONOFF                    0x01
#define MELFAS_ADDR_COMMAND                      0x02
#define MELFAS_ADDR_COMMAND_PARAMETER            0x03
#define MELFAS_ADDR_KEY_STATUS                   0x04
#define MELFAS_ADDR_KEY_VALUE                    0x05
#define MELFAS_ADDR_HW_VERSION                   0x06
#define MELFAS_ADDR_FW_VERSION                   0x0A
#define MELFAS_ADDR_INTENSITY_TSP                0x0E

#define FIRMWARE_VERSION                          0x00

#define MELFAS_COMMAND_ENTER_SLEEPMODE           0xE1
#define MELFAS_COMMAND_ENTER_TESTMODE            0xE2
#define MELFAS_COMMAND_ENTER_WORKMODE            0xE3
#define MELFAS_COMMAND_ENTER_JIGMODE             0xE4
#define MELFAS_COMMAND_SENSITIVITY               0x51
#define MELFAS_COMMAND_PALMMODE                  0xA5

#define MELFAS_CONTACTED_STATUS                  0x81
#define MELFAS_UNCONTACTED_STATUS                0x01

#define MELFAS_SENSITIVITY_HIGH                  0x01
#define MELFAS_SENSITIVITY_MIDDLE                0x02
#define MELFAS_SENSITIVITY_LOW                   0x03

#define MELFAS_PALM_FUNC_SET                      0x01 //not supported, 20080829
#define MELFAS_PALM_FUNC_RELEASE                 

#define MELFAS_FEEDBACK_FLAG_CLR                0x00
#define MELFAS_FEEDBACK_FLAG_SET                0x01

#define POWER_KEY	1
#define USER_KEY	3

#endif
