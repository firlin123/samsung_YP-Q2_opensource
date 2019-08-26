/* \file stmp36xx_fmtuner.h
 * \brief stmp36xx fmtuner read/write
 * \author Choi Yong Joon <yj1017.choi@samsung.com>
 * \version $Revision: 1.18 $
 * \date $Date: 2008/05/09 10:23:50 $
 *
 * This file is \a FMTuner read/write function using i2c in stmp36xx.
 */

#ifndef STMP37XX_FMTUNER_SI4072_H
#define STMP37XX_FMTUNER_SI4072_H

/********** Defines **********/
#define I2C_CHANNEL_NUMBER			3
#define NUMBER_BYTES_TO_WRITE		12

#define NUMBER_BYTES_TO_READ		12

#define FMINTERMEDIATE_FREQ 				225000	// FM intermediate frequency
#define FMCLK						32768  //FM reference frequency
#define FMCAL_VAL					8192  // = FMCLK/4

#define FM_MAX_FREQUENCY_US				10800
#define FM_MIN_FREQUENCY_US				8750
#define FM_FREQUENCY_SCALE_US 				10

#define FM_MAX_FREQUENCY_JAPAN				10800
#define FM_MIN_FREQUENCY_JAPAN				7600
#define FM_FREQUENCY_SCALE_JAPAN			1

#define FM_MAX_FREQUENCY_OTHER				10800
#define FM_MIN_FREQUENCY_OTHER				8750
#define FM_FREQUENCY_SCALE_OTHER			5

#define FM_TUNER_REG_SPACE	32

#define TRUE		1
#define FALSE	0
enum
{
	SS_FM_OPEN,
	SS_FM_RELEASE,
};
typedef int ss_fm_request_t;

enum
{
	SS_FM_DSP_DEV,	    /* USB device */
};
typedef int ss_fm_dev_t;

typedef void (*fm_call_back)(ss_fm_request_t rqst);

typedef enum OPERA_MODE {
    FM_READ = 1,
    FM_WRITE = 2
} T_OPERA_MODE;

typedef enum ERROR_OP {
    OK = 1,
    I2C_ERROR = 2,
    LOOP_EXP_ERROR = 3,
    ERROR = 4,
    END_OF_SEEK = 5,
    IN_VALID_CHANNEL = 6
} T_ERROR_OP;

#ifdef CONFIG_STMP37XX_SI4702
extern void ss_fm_register(ss_fm_dev_t type, fm_call_back callback);
#else
#define ss_fm_register(type, callback)
#endif

/********************************************************************************************/
/*         pjdragon si47XX																		*/


enum {
  GPIO_IN = 0,
  GPIO_OUT
};

enum {
  GPIO_LOW = 0,
  GPIO_HIGH
};

#define GPIO_MODE  3

typedef	unsigned char BYTE;

typedef unsigned char BOOL;

#define FM_CHIPID	0x10
#define I2C_WRITE		0x00
#define I2C_READ		0x01

#define DURATION_INIT_1	1 
#define DURATION_INIT_2	1 

#define POWER_SETTLING  600        // make sure the delay is 110ms

/* #define SET_TUNER_RESET	set_pin_gpio_mode(0, 10, GPIO_OUT); */
/* #define SET_TUNER_SERACH	set_pin_gpio_mode(0, 11, GPIO_IN); */

/* #define RST_LOW   set_pin_gpio_val(0, 10, 0)                 //	set reset PIN to low 		 */
/* #define RST_HIGH  set_pin_gpio_val(0, 10, 1)	             // set reset PIN to high  */

/* #define IS_MUTEP_ON		get_pin_gpio_val(1, 22) ? TRUE : FALSE		//high actice GPIO 1.22	 */
/* #define MUTEP_ON		set_pin_gpio_val(1, 22, 1)		 */
/* #define MUTEP_OFF		set_pin_gpio_val(1, 22, 0)		 */

#if 1

#define SET_TUNER_RESET   stmp37xx_gpio_set_dir(pin_GPIO(0,10), GPIO_OUT)
#define SET_TUNER_SERACH stmp37xx_gpio_set_dir(pin_GPIO(0,11), GPIO_IN)

#define RST_HIGH   stmp37xx_gpio_set_level(pin_GPIO(0,10), GPIO_HIGH)
#define RST_LOW   stmp37xx_gpio_set_level(pin_GPIO(0,10), GPIO_LOW)

#define MUTEP_ON stmp37xx_gpio_set_level(pin_GPIO(1,22), GPIO_HIGH)
#define MUTEP_OFF stmp37xx_gpio_set_level(pin_GPIO(1,22), GPIO_LOW)
#define IS_MUTEP_ON stmp37xx_gpio_get_level(pin_GPIO(1,22)) ? TRUE : FALSE


// FM SCLK: 3.17 => 2.5
// FM SDAT: 3.18 => 2.6

#define FM_I2C_SDA_PIN_GPIO		set_pin_func(2,6,GPIO_MODE);
#define FM_I2C_SDA_PIN_DIR		stmp37xx_gpio_set_dir(pin_GPIO(2, 6), GPIO_OUT) //set_pin_gpio_mode(2, 6, 1)			


#define FM_I2C_SDA_OUT	stmp37xx_gpio_set_dir(pin_GPIO(2,6),GPIO_OUT) 	//HW_PINCTRL_DOE3_SET(0x1 << 18)
#define FM_I2C_SDA_IN	stmp37xx_gpio_set_dir(pin_GPIO(2,6),GPIO_IN)  //HW_PINCTRL_DOE3_CLR(0x1 << 18)
#define FM_I2C_SDA_LOW  stmp37xx_gpio_set_level(pin_GPIO(2,6),GPIO_LOW) //HW_PINCTRL_DOUT3_CLR(0x1 << 18)	
#define FM_I2C_SDA_HIGH	stmp37xx_gpio_set_level(pin_GPIO(2,6),GPIO_HIGH) //HW_PINCTRL_DOUT3_SET(0x1 << 18)	

#define GET_FM_SDA		get_pin_gpio_val(2,6)//stmp37xx_gpio_get_level(pin_GPIO(2, 6)) //get_pin_gpio_val(2, 6) 	

#define FM_I2C_SCL_PIN_GPIO		set_pin_func(2,5,GPIO_MODE) 		// 3.17
#define FM_I2C_SCL_PIN_OUT		stmp37xx_gpio_set_dir(pin_GPIO(2, 5), GPIO_OUT)  //set_pin_gpio_mode(2, 5, 1)			

#define FM_I2C_SCL_LOW	              stmp37xx_gpio_set_level(pin_GPIO(2,5),GPIO_LOW)
#define FM_I2C_SCL_HIGH	              stmp37xx_gpio_set_level(pin_GPIO(2,5),GPIO_HIGH)

#else

#define SET_TUNER_RESET   stmp37xx_gpio_set_dir(pin_GPIO(0,10), 1)
#define SET_TUNER_SERACH stmp37xx_gpio_set_dir(pin_GPIO(0,11), 0)//GPIO_DIR_IN); 
#define RST_HIGH   set_pin_gpio_val(0,10, 1)
#define RST_LOW   set_pin_gpio_val(0,10, 0)
#define MUTEP_ON set_pin_gpio_val(1,22, 1) // 0 26
#define MUTEP_OFF set_pin_gpio_val(1,22, 0) // 0 26
#define IS_MUTEP_ON get_pin_gpio_val(1,22) ? TRUE : FALSE //0 26

// FM SCLK: 3.17 => 2.5
// FM SDAT: 3.18 => 2.6

#define FM_I2C_SDA_PIN_GPIO		set_pin_func(2,6,3);
#define FM_I2C_SDA_PIN_DIR		stmp37xx_gpio_set_dir(pin_GPIO(2, 6), 1) //set_pin_gpio_mode(2, 6, 1)			


#define FM_I2C_SDA_OUT	stmp37xx_gpio_set_dir(pin_GPIO(2,6),1)
#define FM_I2C_SDA_IN	stmp37xx_gpio_set_dir(pin_GPIO(2,6),0)
#define FM_I2C_SDA_LOW   set_pin_gpio_val(2,6,0)	
#define FM_I2C_SDA_HIGH	set_pin_gpio_val(2,6,1) 

#define GET_FM_SDA		get_pin_gpio_val(2, 6) //get_pin_gpio_val(2, 6) 	

#define FM_I2C_SCL_PIN_GPIO		set_pin_func(2,5,3) 		// 3.17
#define FM_I2C_SCL_PIN_OUT		stmp37xx_gpio_set_dir(pin_GPIO(2, 5), 1)  //set_pin_gpio_mode(2, 5, 1)			

#define FM_I2C_SCL_LOW	              set_pin_gpio_val(2,5,0)//HW_PINCTRL_DOUT3_CLR(0x1 << 17)//set_pin_gpio_val(1, 21, 0)    
#define FM_I2C_SCL_HIGH	              set_pin_gpio_val(2,5,1)//HW_PINCTRL_DOUT3_SET(0x1 << 17)//set_pin_gpio_val(1, 21, 1)    


#endif


#define FM_I2C_BYTE_DELAY		udelay(3)
#define FM_I2C_CLOCK_DELAY		udelay(2)



#define DELAY(DURATION)		mdelay(DURATION)



typedef union
{
	struct
	{
		unsigned short  PN				:4;
		unsigned short  MFGID			:12;
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_00H_DeviceID;

typedef union
{
	struct
	{
		unsigned short  REV			:6;
		unsigned short  DEV			:4;
		unsigned short  Firmware		:6;
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_01H_ChipID;

typedef union
{
	struct
	{
		unsigned short  DSMUTE		:1;
		unsigned short  DMUTE			:1;
		unsigned short  MONO			:1;
		unsigned short  Reserve0		:1;
		unsigned short  RDSM			:1;
		unsigned short  SKMODE		:1;
		unsigned short  SEEKUP		:1;
		unsigned short  SEEK			:1;
		
		unsigned short  Reserve1		:1;
		unsigned short  DISABLE		:1;
		unsigned short  Reserve2		:5;
		unsigned short  ENABLE		:1;
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_02H_PowerCFG;

typedef union
{
	struct
	{
		unsigned short  TUNE			:1;
		unsigned short  Reserve0		:5;
		unsigned short  CHAN			:10;
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_03H_Channel;

typedef union
{
	struct
	{
		unsigned short  RDSIEN		:1;
		unsigned short  STCIEN			:1;
		unsigned short  Reserve0		:1;
		unsigned short  RDS			:1;
		unsigned short  DE				:1;
		unsigned short  AGCD			:1;
		unsigned short  Reserve1		:2;
		unsigned short  BLNDADJ		:2;
		unsigned short  GPIO3			:2;
		unsigned short  GPIO2			:2;
		unsigned short  GPIO1			:2;
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_04H_Sysconfig1;

typedef union
{
	struct
	{
		unsigned short  SEEKTH		:8;
		unsigned short  BAND			:2;
		unsigned short  SPACE			:2;		
		unsigned short  VOLUME		:4;
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_05H_Sysconfig2;

typedef union
{
	struct
	{
		unsigned short  SMUTER		:2;
		unsigned short  SMUTEA		:2;
		unsigned short  Reserve0		:2;
		unsigned short  RDSPRF		:1;		
		unsigned short  VOLEXT		:1;				
		unsigned short  SKSNR			:4;				
		unsigned short  SKCNT			:4;
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_06H_Sysconfig3;

typedef union
{
	struct
	{
		unsigned short  XOSCEN		:1;
		unsigned short  AHIZEN		:1;
		unsigned short  Reserve0		:10;
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_07H_TEST1;

typedef union
{
	struct
	{
		unsigned short  Reserve0;
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_08H_TEST2;

typedef union
{
	struct
	{
		unsigned short  Reserve0;
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_09H_BootConfig;

typedef union
{
	struct
	{
		unsigned short  RDSR			:1;
		unsigned short  STC			:1;
		unsigned short  SFBL			:1;
		unsigned short  AFCRL			:1;		
		unsigned short  FDSS2			:1;				
		unsigned short  BLERA			:2;				
		unsigned short  ST				:1;
		unsigned short  RSSI			:8;		
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_0AH_StatusRSSI;

typedef union
{
	struct
	{
		unsigned short  BLERB			:2;
		unsigned short  BLERC			:2;
		unsigned short  BLERD			:2;
		unsigned short  READCAN		:10;		
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_0BH_ReadChan;

typedef union
{
	struct
	{
		unsigned short  RDSA;
		
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_0CH_RDSA;

typedef union
{
	struct
	{
		unsigned short  RDS;
		
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_0DH_RDSB;

typedef union
{
	struct
	{
		unsigned short  RDS;
		
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_0EH_RDSC;

typedef union
{
	struct
	{
		unsigned short  RDS;
		
	}__attribute__ ((packed))B;
	unsigned short I;
}__attribute__ ((packed))Si470X_0FH_RDSD;


union Si470X_Register{
	struct{
		Si470X_00H_DeviceID 			B0;								
		Si470X_01H_ChipID 			B1;										
		Si470X_02H_PowerCFG 			B2;								
		Si470X_03H_Channel 			B3;								
		Si470X_04H_Sysconfig1 		B4;								
		Si470X_05H_Sysconfig2 		B5;								
		Si470X_06H_Sysconfig3 		B6;								
		Si470X_07H_TEST1	 		B7;								
		Si470X_08H_TEST2	 		B8;								
		Si470X_09H_BootConfig	 		B9;										
		Si470X_0AH_StatusRSSI 		B10;								
		Si470X_0BH_ReadChan	 		B11;										
		Si470X_0CH_RDSA		 		B12;										
		Si470X_0DH_RDSB		 		B13;										
		Si470X_0EH_RDSC		 		B14;												
		Si470X_0FH_RDSD		 		B15;														
		}__attribute__ ((packed))ADDR;
	unsigned short SI470X_D[16];
}__attribute__ ((packed));


/********************************************************************************************/




#endif


