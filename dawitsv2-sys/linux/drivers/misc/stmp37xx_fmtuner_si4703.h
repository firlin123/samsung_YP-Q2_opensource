#ifndef STMP37XX_FMTUNER_SI4703_H
#define STMP37XX_FMTUNER_SI4703_H

/********************************************************
 * Macro definition
 *******************************************************/
#define FM_SI4703_CHIPID 0x10
#define FM_SI4703_DEVID 0x1242

#define SET_TUNER_RESET   stmp37xx_gpio_set_dir(BANK0_PIN10, GPIO_DIR_OUT)
#define SET_TUNER_SEARCH stmp37xx_gpio_set_dir(BANK0_PIN11, GPIO_DIR_IN)

#define RESET_HIGH   stmp37xx_gpio_set_level(BANK0_PIN10, GPIO_VOL_33)
#define RESET_LOW   stmp37xx_gpio_set_level(BANK0_PIN10, GPIO_VOL_18)

#define MUTEP_ON stmp37xx_gpio_set_level(BANK1_PIN22, GPIO_VOL_33)
#define MUTEP_OFF stmp37xx_gpio_set_level(BANK1_PIN22, GPIO_VOL_18)
#define IS_MUTEP_ON stmp37xx_gpio_get_level(BANK1_PIN22) ? TRUE : FALSE

#define WAIT_STC_INTERRUPT stmp37xx_gpio_get_level(BANK0_PIN11)

/* Powerup delay in milliseconds */
#define POWERUP_TIME 500 

#define RETURN_OK 0
#define RETURN_NG -1
#define TRUE 1
#define FALSE 0

/********************************************************
 * Data type definition
 ********************************************************/
typedef unsigned char UC;
typedef signed char SC;
typedef unsigned short US;
typedef signed short SS;
typedef unsigned int UI;
typedef signed int SI;
typedef unsigned long UL;
typedef signed long SL;

/********************************************************
 * ERROR definition
 ********************************************************/
typedef signed long FM_ERROR;
#define FM_ERROR_BASE        0x01000000
#define FM_ERROR_HW          FM_ERROR_BASE + 0x01
#define FM_ERROR_PARAM       FM_ERROR_HW + 0x01
#define FM_ERROR_STATUS      FM_ERROR_PARAM + 0x01

#endif
