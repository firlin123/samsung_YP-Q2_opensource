////////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_power
//! @{
//
// Copyright (c) 2004-2005 SigmaTel, Inc.
//
//! \file hw_power.h
//! \brief Header file for power hardware API.
//!
//! Contains all header information and prototypes for power hardware API.
//!
//! \see hw_power.c
////////////////////////////////////////////////////////////////////////////////
#ifndef __HW_POWER_H
#define __HW_POWER_H

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
//#include "hw/lradc/hw_lradc.h"
#include "hw_power_translation.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! \brief The inverse of 6.25e-3 used in the DCFUNCV calculations
#define DCFUNCV_CONST_160 160
//! \brief Constant value for 1000 used in the DCFUNCV calculations
#define DCFUNCV_CONST_1000 1000
//! \brief Maximum possible value for DCFUNC bitfield
#define DCFUNCV_MAX_VALUE 1023

//! \brief Base voltage to start battery calculations for LiIon
#define BATT_BRWNOUT_LIION_BASE_MV 2800
//! \brief Constant to help with determining whether to round up or not during calculation
#define BATT_BRWNOUT_LIION_CEILING_OFFSET_MV 39
//! \brief Number of mV to add if rounding up in LiIon mode
#define BATT_BRWNOUT_LIION_LEVEL_STEP_MV 40
//! \brief Constant value to be calculated by preprocessing
#define BATT_BRWNOUT_LIION_EQN_CONST (BATT_BRWNOUT_LIION_BASE_MV - BATT_BRWNOUT_LIION_CEILING_OFFSET_MV)
//! \brief Base voltage to start battery calculations for Alkaline/NiMH
#define BATT_BRWNOUT_ALKAL_BASE_MV 800
//! \brief Constant to help with determining whether to round up or not during calculation
#define BATT_BRWNOUT_ALKAL_CEILING_OFFSET_MV 19
//! \brief Number of mV to add if rounding up in Alkaline/NiMH mode
#define BATT_BRWNOUT_ALKAL_LEVEL_STEP_MV 20
//! \brief Constant value to be calculated by preprocessing
#define BATT_BRWNOUT_ALKAL_EQN_CONST (BATT_BRWNOUT_ALKAL_BASE_MV - BATT_BRWNOUT_ALKAL_CEILING_OFFSET_MV)

//! \brief Constant value for 8mV steps used in battery translation
#define BATT_VOLTAGE_8_MV 8

//! \brief Register key value to allow writes to chip reset register
#define POWERDOWN_KEY 0x3e77

//! \brief The first LRADC channel to use
#define DIE_TEMP_CHAN_0 LRADC_CH2
//! \brief The second LRADC channel to use
#define DIE_TEMP_CHAN_1 LRADC_CH3

////////////////////////////////////////////////////////////////////////////////
// Enumerates
////////////////////////////////////////////////////////////////////////////////

//! \brief Important voltage values for VDDD, VDDA, and VDDIO
typedef enum _hw_power_VoltageValues_t 
{
    //! \brief Size of voltage steps in mV.
    VOLTAGE_STEP_MV     = 25,
    //! \brief Minimum brownout offset from target voltage.
    BO_MIN_OFFSET_MV    = 25,
    //! \brief Maximum brownout offset from target voltage.
    BO_MAX_OFFSET_MV    = VOLTAGE_STEP_MV * 7,

    //! \brief Number of steps between target and brownout
    VDDD_MARGIN_STEPS   = 4,
    //! \brief Minimum margin between VDDD target and brownout
    VDDD_TO_BO_MARGIN  = (VDDD_MARGIN_STEPS * VOLTAGE_STEP_MV),
    //! \brief Default VDDD voltage at power on.
    VDDD_DEFAULT_MV    = 1200,
    //! \brief Default VDDD BO level at power on.
    VDDD_DEFAULT_BO    = (VDDD_DEFAULT_MV - VDDD_TO_BO_MARGIN),
    //! \brief Absolute minimum VDDD voltage.
    VDDD_ABS_MIN_MV    = 800,
    //! \brief Absolute maximum VDDD voltage.
    VDDD_ABS_MAX_MV    = 1575,
    //! \brief Maximum safe VDDD voltage.
    VDDD_SAFE_MAX_MV   = 1550,
    //! \brief Minimum safe VDDD voltage.
    VDDD_SAFE_MIN_MV   = (VDDD_ABS_MIN_MV + BO_MIN_OFFSET_MV),
    //! \brief Maximum voltage for alkaline battery
    VDDD_ALKALINE_MAX_MV = 1300,
    //! \brief Minimum possible VDDD brownout voltage
    VDDD_BO_MIN_MV     = VDDD_ABS_MIN_MV,
    //! \brief Maximum possible VDDD brownout voltage
    VDDD_BO_MAX_MV     = (VDDD_SAFE_MAX_MV - VDDD_TO_BO_MARGIN), 
    //! \brief Base VDDD voltage for calculations.  Equivalent to register setting 0x0.
    VDDD_BASE_MV       = VDDD_ABS_MIN_MV,

    //! \brief Number of steps between target and brownout
    VDDA_MARGIN_STEPS   = 4,
    //! \brief Minimum margin between VDDA target and brownout
    VDDA_TO_BO_MARGIN  = (VDDA_MARGIN_STEPS * VOLTAGE_STEP_MV),
    //! \brief Default VDDA voltage at power on.
    VDDA_DEFAULT_MV    = 1750,  //modify for sound driver, 2100, dhsong
    //! \brief Default VDDA BO level at power on.
    VDDA_DEFAULT_BO    = (VDDA_DEFAULT_MV - VDDA_TO_BO_MARGIN),
    //! \brief Absolute minimum VDDA voltage.
    VDDA_ABS_MIN_MV    = 1500,
    //! \brief Absolute maximum VDDA voltage.
    VDDA_ABS_MAX_MV    = 2275,
    //! \brief Maximum safe VDDA voltage.
    VDDA_SAFE_MAX_MV   = 1950, //modify for sound driver, 2100, dhsong
    //! \brief Minimum safe VDDA voltage.
    VDDA_SAFE_MIN_MV   = (VDDA_ABS_MIN_MV + BO_MIN_OFFSET_MV),
    //! \brief Minimum possible VDDA brownout voltage
    VDDA_BO_MIN_MV     = VDDA_ABS_MIN_MV,
    //! \brief Maximum possible VDDA brownout voltage
    VDDA_BO_MAX_MV     = (VDDA_SAFE_MAX_MV - VDDA_TO_BO_MARGIN), 
    //! \brief Base VDDA voltage for calculations.  Equivalent to register setting 0x0.
    VDDA_BASE_MV       = VDDA_ABS_MIN_MV,

    //! \brief Number of steps between target and brownout
    VDDIO_MARGIN_STEPS  = 4,
    //! \brief Minimum margin between VDDIO target and brownout
    VDDIO_TO_BO_MARGIN  = (VDDIO_MARGIN_STEPS * VOLTAGE_STEP_MV),
    //! \brief Default VDDIO voltage at power on.
    VDDIO_DEFAULT_MV    = 3100,
    //! \brief Default VDDIO BO level at power on.
    VDDIO_DEFAULT_BO    = (VDDIO_DEFAULT_MV - VDDIO_TO_BO_MARGIN),
    //! \brief Absolute minimum VDDIO voltage.
    VDDIO_ABS_MIN_MV    = 2800,
    //! \brief Absolute maximum VDDIO voltage.
    VDDIO_ABS_MAX_MV    = 3575,
    //! \brief Maximum safe VDDIO voltage.
    VDDIO_SAFE_MAX_MV   = 3575,
    //! \brief Minimum safe VDDIO voltage.
    VDDIO_SAFE_MIN_MV   = (VDDIO_ABS_MIN_MV + BO_MIN_OFFSET_MV),
    //! \brief Minimum possible VDDIO brownout voltage
    VDDIO_BO_MIN_MV     = VDDIO_ABS_MIN_MV,
    //! \brief Maximum possible VDDIO brownout voltage
    VDDIO_BO_MAX_MV     = (VDDIO_SAFE_MAX_MV - VDDIO_TO_BO_MARGIN), 
    //! \brief Base VDDIO voltage for calculations.  Equivalent to register setting 0x0.
    VDDIO_BASE_MV       = VDDIO_ABS_MIN_MV,
} hw_power_VoltageValues_t;

//! \brief Threshold values for Vbus valid comparator
typedef enum _hw_power_VbusValidThresh_t
{
    //! \brief 4.40V threshold on insertion, 4.21V threshold on removal
    VBUS_VALID_THRESH_4400_4210 = 0,
    //! \brief 4.17V threshold on insertion, 4.00V threshold on removal
    VBUS_VALID_THRESH_4170_4000 = 1,
    //! \brief 2.50V threshold on insertion, 2.45V threshold on removal
    VBUS_VALID_THRESH_2500_2450 = 2,
    //! \brief 4.73V threshold on insertion, 4.48V threshold on removal
    VBUS_VALID_THRESH_4730_4480 = 3,
    //! \brief Maximum threshold value for the register setting
    VBUS_VALID_THRESH_MAX       = 3,

    //! \brief Use under normal operating conditions.
    VBUS_VALID_THRESH_NORMAL = VBUS_VALID_THRESH_4400_4210,
    //! \brief Use when a lower than normal threshold is needed.
    VBUS_VALID_THRESH_LOW    = VBUS_VALID_THRESH_4170_4000,
    //! \brief Use when a higher than normal threshold is needed.
    VBUS_VALID_THRESH_HIGH   = VBUS_VALID_THRESH_4730_4480,
    //! \brief Use only for testing, or under SigmaTel guidance.
    VBUS_VALID_THRESH_TEST   = VBUS_VALID_THRESH_2500_2450
} hw_power_VbusValidThresh_t;

//! \brief Battery mode enumeration
typedef enum _hw_power_BatteryMode_t
{
	//! \brief Battery type is LiIon
    HW_POWER_BATT_MODE_LIION  = 0,    
	//! \brief Batter type is Alkaline or NiMH
    HW_POWER_BATT_MODE_ALKALINE_NIMH = 1,

// We will remove these later.  They are redundant or obsolete.  
	//! \brief 36xx batt mode
    HW_POWER_BATT_MODE_0                      = 0,
	//! \brief 36xx batt mode
    HW_POWER_BATT_MODE_LIION_DUAL_CONVERTOR   = 0,
	//! \brief 36xx batt mode
    HW_POWER_BATT_MODE_1                      = 1,
	//! \brief 36xx batt mode
    HW_POWER_BATT_MODE_LIION_SINGLE_INDUCTOR  = 1,
	//! \brief 36xx batt mode
    HW_POWER_BATT_MODE_2                      = 2,
	//! \brief 36xx batt mode
    HW_POWER_BATT_MODE_SERIES_AA_AAA          = 2,
	//! \brief 36xx batt mode
    HW_POWER_BATT_MODE_DUAL_ALKALINE_NIMH     = 2,
	//! \brief 36xx batt mode
    HW_POWER_BATT_MODE_3                      = 3,
	//! \brief 36xx batt mode
    HW_POWER_BATT_MODE_SINGLE_AA              = 3,
	//! \brief 36xx batt mode
    HW_POWER_BATT_MODE_SINGLE_ALKALINE_NIMH   = 3,
	//! \brief 36xx batt mode
    HW_PWR_BATT_MODE_0                      = 0,
	//! \brief 36xx batt mode
    HW_PWR_BATT_MODE_LIION_DUAL_CONVERTOR   = 0,
	//! \brief 36xx batt mode
    HW_PWR_BATT_MODE_1                      = 1,
	//! \brief 36xx batt mode
    HW_PWR_BATT_MODE_LIION_SINGLE_INDUCTOR  = 1,
	//! \brief 36xx batt mode
    HW_PWR_BATT_MODE_2                      = 2,
	//! \brief 36xx batt mode
    HW_PWR_BATT_MODE_SERIES_AA_AAA          = 2,
	//! \brief 36xx batt mode
    HW_PWR_BATT_MODE_DUAL_ALKALINE_NIMH     = 2,
	//! \brief 36xx batt mode
    HW_PWR_BATT_MODE_3                      = 3,
	//! \brief 36xx batt mode
    HW_PWR_BATT_MODE_SINGLE_AA              = 3,
	//! \brief 36xx batt mode
    HW_PWR_BATT_MODE_SINGLE_ALKALINE_NIMH   = 3
} hw_power_BatteryMode_t;

//! \brief Possible power sources for each power supply
typedef enum _hw_power_PowerSource_t
{
	//! \brief Powered by linear regulator.  DCDC output is gated off and 
    //! the linreg output is equal to the target.
    HW_POWER_LINREG_DCDC_OFF,
    //! \brief Powered by linear regulator.  DCDC output is not gated off
    //! and is ready for the automatic hardware transistion after a 5V
    //! event.  The converters are not enabled when 5V is present. LinReg output
    //! is 25mV below target.  
    HW_POWER_LINREG_DCDC_READY,
	//! \brief Powered by DCDC converter and the LinReg is on. LinReg output
    //! is 25mV below target.
    HW_POWER_DCDC_LINREG_ON,
	//! \brief Powered by DCDC converter and the LinReg is off. LinReg output
    //! is 25mV below target.  
    HW_POWER_DCDC_LINREG_OFF,
    //! \brief Powered by DCDC converter and the LinReg is ready for the
    //! automatic hardware transfer.  The LinReg output is not enabled and depends
    //! on the 5V presence to enable the LinRegs.  LinReg offset is 25mV below target.  
    HW_POWER_DCDC_LINREG_READY,
    //! \brief Unknown configuration.  This is an error.
    HW_POWER_UNKNOWN_SOURCE,
	//! \brief 36xx enum
    HW_POWER_DCDC1 = 1,
	//! \brief 36xx enum
    HW_POWER_DCDC2 = 2,
    //! \brief For the PMI constants.  To be removed.  
    HW_POWER_LINEAR_REGULATOR = HW_POWER_LINREG_DCDC_OFF
    
 
}hw_power_PowerSource_t;

//! \brief Selects the source of the bias current
typedef enum _hw_power_BiasCurrentSource_t
{
 	//! \brief Use external resistor to generate bias current.
    HW_POWER_EXTERNAL_BIAS_CURRENT = 0x0,
 	//! \brief Use internal resistor to generate bias current.
    HW_POWER_INTERNAL_BIAS_CURRENT = 0x1
} hw_power_BiasCurrentSource_t;

//! \brief Battery charging current magnitudes converted to register settings.  
typedef enum _hw_power_BattChargeCurrentMag_t
{
 	//! \brief Current magniude 400mA
    HW_POWER_CURRENT_MAG_400 = 0x20,
 	//! \brief Current magniude 200mA
    HW_POWER_CURRENT_MAG_200 = 0x10,
 	//! \brief Current magniude 100mA
    HW_POWER_CURRENT_MAG_100 = 0x08,
 	//! \brief Current magniude 50mA
    HW_POWER_CURRENT_MAG_50  = 0x04,
 	//! \brief Current magniude 20mA
    HW_POWER_CURRENT_MAG_20  = 0x02,
 	//! \brief Current magniude 10mA
    HW_POWER_CURRENT_MAG_10  = 0x01
} hw_power_BattChargeCurrentMag_t;

//! \brief Linear regulator offset values
typedef enum _hw_power_LinRegOffsetStep_t
{
	//! \brief No offset.  Linear regualator output equals target.
    HW_POWER_LINREG_OFFSET_NO_STEP    = 0,
	//! \brief Linear regulator is one 25mV step above the target.  
    HW_POWER_LINREG_OFFSET_STEP_ABOVE = 1,
	//! \brief Linear regulator is one 25mV step below the target.  
    HW_POWER_LINREG_OFFSET_STEP_BELOW = 2,
	//! \brief Max offset. Linear regulator is one 25mV step above the target.  
    HW_POWER_LINREG_OFFSET_MAX        = 3,
    //! \brief No step is used for LinReg mode.
    HW_POWER_LINREG_OFFSET_LINREG_MODE = HW_POWER_LINREG_OFFSET_NO_STEP,
    //! \brief No step is used for LinReg mode.
    HW_POWER_LINREG_OFFSET_DCDC_MODE = HW_POWER_LINREG_OFFSET_STEP_BELOW

} hw_power_LinRegOffsetStep_t;

//! \brief Possible RC Scale values to increase transient load response
typedef enum _hw_power_RcScaleLevels_t
{
	//! \brief Use nominal response 
    HW_POWER_RCSCALE_DISABLED   = 0,
	//! \brief Increase response by factor of 2
    HW_POWER_RCSCALE_2X_INCR    = 1,
	//! \brief Increase response by factor of 4
    HW_POWER_RCSCALE_4X_INCR    = 2,
	//! \brief Increase response by factor of 8
    HW_POWER_RCSCALE_8X_INCR    = 3
} hw_power_RcScaleLevels_t;

//! \brief Interrupt polarities for power hardware
typedef enum _hw_power_InterruptPolarity_t
{
    //! \brief Generate interrupt when 5V is disconnected
    HW_POWER_CHECK_5V_DISCONNECTED      = 0,
    //! \brief Generate interrupt when 5V is connected
    HW_POWER_CHECK_5V_CONNECTED         = 1,
    //! \brief Generate interrupt when linear regulators are stable
    HW_POWER_CHECK_LINEAR_REGULATORS_OK = 1,
    //! \brief Generate interrupt when Pswitch goes high
    HW_POWER_CHECK_PSWITCH_HIGH         = 1,
    //! \brief Generate interrupt when Pswitch goes low
    HW_POWER_CHECK_PSWITCH_LOW          = 0
} hw_power_InterruptPolarity_t;

//! \brief Possible sources for Pswitch IRQ source
typedef enum _hw_power_PswitchIrqSource_t
{
    //! \brief Use bit 0 for Pswitch source
    HW_POWER_STS_PSWITCH_BIT_0,
    //! \brief Use bit 1 for Pswitch source
    HW_POWER_STS_PSWITCH_BIT_1
} hw_power_PswitchIrqSource_t;

//! \brief Possible 5V detection methods
typedef enum _hw_power_5vDetection_t
{
    //! \brief Use VBUSVALID comparator for detection
    HW_POWER_5V_VBUSVALID,
    //! \brief Use VDD5V_GT_VDDIO comparison for detection
    HW_POWER_5V_VDD5V_GT_VDDIO
} hw_power_5vDetection_t;

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! \brief Initializes the hw power block and battery charger
//! 
//! \fntype Function                        
//!     
//! (Description here)
//!
//!
//! \return Status of the call.
//! \retval SUCCESS                            Call was successful.
//! \retval ERROR_HW_DCDC_INVALID_INPUT_PARAM  Bad input parameter value
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_Init(void);
RtStatus_t hw_power_InitBatteryMonitor (uint32_t u32SampleInterval);
RtStatus_t hw_power_InitPowerSupplies(void);
RtStatus_t hw_power_InitFiq(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets new VDDD 
//! 
//! \fntype Function                        
//!     
//! This function converts the millivolt value passed in to a register
//! setting and writes it to the target register.  This function will also
//! update the DCDC efficiency register field(s) related to Vddd.  
//!
//! \param[in] 	 u16Vddd_mV - new voltage level in mV
//!                        
//! \notes This function does not adjust for LinReg offsets
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVdddValue(uint16_t u16Vddd_mV);

////////////////////////////////////////////////////////////////////////////////
//! \brief returns the current VDDD voltage 
//! 
//! \fntype Function                        
//!     
//! This function converts the register setting for the target and converts
//! it to a millivolt value. The millivolt value is returned.
//!
//! \retval voltage of VDDD in mV 
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVdddValue(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets new VDDD brownout voltage
//! 
//! \fntype Function                        
//!     
//! This function converts the brownout offset (as a millivolt value) 
//! to a register setting and writes it to the brownout offset field.
//!
//! \param[in] 	 u16VdddBoOffset_mV - offset from target to set brownout 
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVdddBrownoutValue(uint16_t u16VdddBoOffset_mV);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the current VDDD brownout offset
//! 
//! \fntype Function                        
//!     
//! This function converts the register setting for the brownout offset and 
//! converts it to a millivolt value. The millivolt value is returned.
//!
//! \retval current VDDD brownout offset
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVdddBrownoutValue(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the VDDD power source
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_SetVdddPowerSource(hw_power_PowerSource_t eSource);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief 
//! 
//! \fntype Function                        
//!     
//! Returns the power source of VDDD
//!
//! \param[out]
//!
//! \retval HW_POWER_LINREG_DCDC_OFF - VDDD powered by linreg
//! \retval HW_POWER_DCDC_LINREG_OFF - VDDD powered by DCDC
////////////////////////////////////////////////////////////////////////////////
hw_power_PowerSource_t hw_power_GetVdddPowerSource(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets new VDDIO
//! 
//! \fntype Function                        
//!     
//  This function converts the millivolt value passed in to a register
//  setting and writes it to the target register.  This function will also
//  update the DCDC efficiency register field(s) related to Vddio.
//!
//! \param[in] 	 u16Vddio_mV - new VDDIO in mV 
//  Notes: This function does not adjust for LinReg offsets
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddioValue(uint16_t u16Vddio_mV);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns current VDDIO voltage
//! 
//! \fntype Function                        
//!     
//  This function converts the register setting for the target and converts
//  it to a millivolt value. The millivolt value is returned.
//!
//! \param[in] 	 void 
//!
//! \param[out]
//!
//! \retval Current VDDIO voltage in mV 
//!                        
//! \notes None //  Notes: This function does not adjust for LinReg offsets
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVddioValue(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets new VDDIO brownout offset trigger
//! 
//! \fntype Function                        
//!     
//  This function converts the brownout offset (as a millivolt value) 
//  to a register setting and writes it to the brownout offset field.
//!
//! \param[in] 	 u16VddioBoOffset_mV - offset in mV
//!
//! \param[out]
//!
//! \retval 
//!                        
//! \notes This function does not adjust for LinReg offsets
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddioBrownoutValue(uint16_t u16VddioBoOffset_mV);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns current VDDIO brownout offset
//! 
//! \fntype Function                        
//!     
//! //  This function converts the register setting for the brownout offset and 
//  converts it to a millivolt value. The millivolt value is returned.
//!
//! \param[in] 	 void 
//!
//! \param[out]
//!
//! \retval Current brownout offset in mV 
//!                        
//! \notes This function does not adjust for LinReg offsets
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVddioBrownoutValue(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the VDDIO power source
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_SetVddioPowerSource(hw_power_PowerSource_t eSource);

////////////////////////////////////////////////////////////////////////////////
//! \brief Returns VDDIO power source
//! 
//! \fntype Function                        
//!     
//! Returns the power source of VDDIO
//!
//! \param[out]
//!
//! \retval HW_POWER_LINREG_DCDC_OFF - VDDIO powered by linreg
//! \retval HW_POWER_DCDC_LINREG_OFF - VDDIO powered by DCDC
////////////////////////////////////////////////////////////////////////////////
hw_power_PowerSource_t hw_power_GetVddioPowerSource(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets new VDDA
//! 
//! \fntype Function                        
//!     
//! This function converts the millivolt value passed in to a register
//  setting and writes it to the target register.  This function will also
//  update the DCDC efficiency register field(s) related to Vdda.
//!
//! \param[in] 	 u16Vdda_mV - new VDDA in mV
//!
//! \param[out]
//!
//! \retval 
//!                        
//! \notes This function does not adjust for LinReg offsets
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddaValue(uint16_t u16Vdda_mV);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns current VDDA 
//! 
//! \fntype Function                        
//!     
//! This function converts the register setting for the target and converts
//  it to a millivolt value. The millivolt value is returned.
//!
//! \param[in] 	 void 
//!
//! \param[out]
//!
//! \retval Current VDDA in mV
//!                        
//! \notes This function does not adjust for LinReg offsets
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVddaValue(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets new VDDA brownout offset
//! 
//! \fntype Function                        
//!     
//! This function converts the brownout offset (as a millivolt value) 
//  to a register setting and writes it to the brownout offset field.
//!
//! \param[in] 	 u16VddaBoOffset_mV - new VDDA brownout in mV
//!
//! \notes This function does not adjust for LinReg offsets
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddaBrownoutValue(uint16_t u16VddaBoOffset_mV);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns current VDDA brownout
//! 
//! \fntype Function                        
//!     
//! This function converts the register setting for the brownout offset and 
//  converts it to a millivolt value. The millivolt value is returned.
//!
//! \param[in] 	 void 
//!
//! \param[out]
//!
//! \retval 
//!                        
//! \notes This function does not adjust for LinReg offsets
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVddaBrownoutValue(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the VDDA power source
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_SetVddaPowerSource(hw_power_PowerSource_t eSource);

////////////////////////////////////////////////////////////////////////////////
//! \brief Returns VDDA power source
//! 
//! \fntype Function                        
//!     
//! Returns the power source of VDDA
//!
//! \param[out]
//!
//! \retval HW_POWER_LINREG_DCDC_OFF - VDDA powered by linreg
//! \retval HW_POWER_DCDC_LINREG_OFF - VDDA powered by DCDC
////////////////////////////////////////////////////////////////////////////////
hw_power_PowerSource_t hw_power_GetVddaPowerSource(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Updates the DCDC efficincy register related to VDDD
//! 
//! \fntype Function                        
//!     
//! Writes the present VDDD supply voltage to the HW_POWER_DCFUNCV register's
//! VDDD field.  This provides information used to optimize DCDC converter
//! performance
//!                      
////////////////////////////////////////////////////////////////////////////////
void hw_power_UpdateDcFuncvVddd(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Updates the DCDC efficincy register related to VDDIO
//! 
//! \fntype Function                        
//!     
//! Writes the present VDDIO supply voltage to the HW_POWER_DCFUNCV register's
//! VDDIO field.  This provides information used to optimize DCDC converter
//! performance
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_UpdateDcFuncvVddio(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Disables the VDDIO linreg 
//! 
//! \fntype Function                        
//!     
//! To disable the Vddio we set the current limit
//  of the Vddio linreg to zero.  So if bDisable is TRUE
//  we want the current limit to zero to be TRUE also.
//!
//! \param[in] 	 bDisable - TRUE to disable
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVddioLinearRegulator(bool bDisable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Disables the VDDA linreg 
//! 
//! \fntype Function                        
//!     
//! To disable the VDDA Linreg, we clear the ENABLE_LINREG bit for VDDA
//!
//! \param[in] 	 bDisable - TRUE to disable
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVddaLinearRegulator(bool bDisable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Disables the VDDD linreg 
//! 
//! \fntype Function                        
//!     
//! To disable the VDDD Linreg, we clear the ENABLE_LINREG bit for VDDD
//!
//! \param[in] 	 bDisable - TRUE to disable
//!////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVdddLinearRegulator(bool bDisable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Disable all Linear Regulators
//! 
//! \fntype Function                        
//!     
//! Disables the VDDD, VDDA, and VDDIO linear regulators
//!
//! \param[in] 	 bDisable - TRUE to disable
//!
//! \notes None
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_Disable5vLinearRegulators(bool bDisable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Enable detection of 5V presense
//! 
//! \fntype Function                        
//!     
//! Enable hardware to check for VBUS Valid signal which is set high
//! when 5V is connected
//!
//! \retval SUCCESS - 5V detection enabled successfully
//!                        
//! \notes None
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_Enable5vDetection(hw_power_5vDetection_t eDetection);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief 5V interrupt triggers on 5V insertion 
//! 
//! \fntype Function                        
//!     
//! Sets the interrupt polarity to check for 5V inserted
//!
//! \param[in] 	 bEnable - TRUE to enable 5V insertion detection 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_Enable5vPlugInDetect(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief 5V interrupt triggers on 5V removal
//! 
//! \fntype Function                        
//!     
//! Sets the interrupt polarity to check for 5V removed
//!
//! \param[in] 	 bEnable - TRUE to enable 5V removal detection
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_Enable5vUnplugDetect(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief 
//! 
//! \fntype Function                        
//!     
//! If DCDCs are enabled during battery charging, it causes the rails to dip more 
//! during heavy loads.  To counter this, we increase the gain of the DCDC control
//! loop.  Also, we increase the battery measurement frequency to account for a more
//! dynamically change Vbat level than we were are not charging.
//!
//! \param[in] 	 bConfigureFor5vConnection - TRUE to configure 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_ConfigureDcdcControlLoopFor5vConnection(bool bConfigureFor5vConnection);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief returns the status of the DCDC voltage change. 
//! 
//! \fntype Function                        
//!     
//! Checks the status of the DC_OK bit. 
//!
//! \retval TRUE - DCDC has completed the voltage change
//! \retval FALSE - DCDC still tranferring to target voltage  
//!                        
////////////////////////////////////////////////////////////////////////////////
bool hw_power_CheckDcdcTransitionDone(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Allow DCDC to be enabled while 5V is connected.
//! 
//! \fntype Function                        
//!     
//! Normally, the DCDC will turn off when a 5V connection is established.
//! This function will allow the DCDC to remain on while 5V is connected
//! to manage high power loads.
//!
//! \param[in] 	 bEnable - TRUE to enable
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDcdcDuring5vConnection(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief 36xx function to enable DCDC1.  Will enable the olny DCDC in 37xx
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDcdc1(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief 36xx function to enable DCDC2.  Will enable the olny DCDC in 37xx
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDcdc2(bool bEnable);

////////////////////////////////////////////////////////////////////////////////
//! 
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVdddDcdcFet(bool bDisable);
    
////////////////////////////////////////////////////////////////////////////////
//! 
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVddaDcdcFet(bool bDisable);

////////////////////////////////////////////////////////////////////////////////
//! 
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVddioDcdcFet(bool bDisable);


////////////////////////////////////////////////////////////////////////////////
// Battery
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets new battery brownout voltage level
//! 
//! \fntype Function                        
//!    
//! \param[in] 	 i16BatteryBrownout_mV - new battery brownout voltage in mV
//!
//! \retval ERROR_HW_POWER_INVALID_BATT_MODE - Battery mode erroe
//! \retval ERROR_HW_POWER_INVALID_INPUT_PARAM - New brownout value is out of range
//! \retval SUCCESS - New battery brownout voltage has been set
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_SetBatteryBrownoutValue(int16_t i16BatteryBrownout_mV);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns current brownout voltage in mV
//! 
//! \fntype Function                        
//!     
//! \retval Battery brownout in mV
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetBatteryBrownoutValue(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Current battery voltage in mV
//! 
//! \fntype Function                        
//!     
//! \retval Current battery voltage
//!                        
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetBatteryVoltage(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief API to set battery monitor voltage.    
//! 
//! \fntype Function                        
//!     
//! Hardware will automatically swet the battery monitor voltage when the battery 
//! charger is set up.  This is for debugging purposes. Normally, this register is 
//! automatically set by hardware.  
//!
//! \param[in] 	 u16BattVolt - new battery monitor voltage 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetBatteryMonitorVoltage(uint16_t u16BattVolt);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Set the maximum current used to charge the battery
//! 
//! \fntype Function                        
//!     
//! Set the maximum current that will be applied to the battery for charging.
//! The maximum current available is 780mA.  The current is limited to values 
//! that can be created using 400, 200, 100, 50, 20, and 10 mA.  Each value is
//! represented by a bit.  If the current value in the argument cannot be achieved,
//! the closet value below the requested value will be used.  
//!
//! \param[in] 	 u16Current - requested maximum battery charge current in mA
//!
//! \retval Actual maximum current that was set
//!                        
//! \notes We can't use the usual Clear/Set (CS) method of writing to this register
// field. If we did, then the current draw would drop to zero for a moment
// and then zoom back up to the new setting. That would be bad.
//
// Instead, we XOR the old and new settings, which gives us a mask where the
// set bits are the ones that need to change. We then write this mask to the
// toggle register and we're done.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_SetMaxBatteryChargeCurrent(uint16_t u16Current);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns maximum battery charge current 
//! 
//! \fntype Function                        
//!     
//! \retval Maximum battery charge current in mA
//!                        
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetMaxBatteryChargeCurrent(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Minimum current that will stop the batter charging current.  
//! 
//! \fntype Function                        
//!     
//! Set the minimum current that will stop the batter charging current.  When
//! the charing current reaches this value, the charger will turn off.
//! Note that the hardware has a minimum resolution of 10mA and a maximum
//! expressible value of 180mA (see the data sheet for details). If the given
//! current cannot be expressed exactly, then the largest expressible smaller
//! value will be used. The return reports the actual value that was effected. 
//!
//! \param[in] 	 u16Threshold - requested threshold current in mA. 
//!
//! \retval Actual threshold current 
//!                        
//! \notes None
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t  hw_power_SetBatteryChargeCurrentThreshold(uint16_t u16Threshold);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns battery charge current threshold
//! 
//! \fntype Function                        
//!     
//! \retval Threshold current in mA 
//!                        
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetBatteryChargeCurrentThreshold(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Internal temperature of the chip.
//! 
//! \fntype Function                        
//!     
//! Reads the LRADC internal temperature monitor for the chip.  If the temperature
//! exceeds the safe limits, battery charging will stop.  
//!
//! \param[out] 	 pi16Low 
//! \param[out] 	 pi16High 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_GetDieTemperature(int16_t * pi16Low, int16_t * pi16High);

////////////////////////////////////////////////////////////////////////////////
//! \brief Enable Pswitch interrupt
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnablePswitchInterrupt(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Clears the Pswitch interrupt status bit
//! 
//! \fntype Function                        
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_ClearPswitchInterrupt(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Selects bit 0 or bit 1 for Pswitch interrupt source 
//! 
//! \fntype Function                        
//!     
//! In HW_POWER_STS register field PSWITCH, bit 0 or 1 (bit 18 or 19 in register respectively),
//! can used to to trigger the Pswitch interrupt 
//!
//! \param[in] 	 bSource - HW_POWER_STS_PSWITCH_BIT_0 or HW_POWER_STS_PSWITCH_BIT_1  
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetPswitchInterruptSource(bool bSource);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Select Pswitch low or high to trigger interrupt
//! 
//! \fntype Function                        
//!     
//! Pswitch interrupts can be triggered when Pswitch goes high or low.  This bit
//! selects the polarity.
//!
//! \param[in] 	 bPolarity - HW_POWER_CHECK_PSWITCH_LOW or HW_POWER_CHECK_PSWITCH_HIGH 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetPswitchInterruptPolarity(bool bPolarity);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns polarity of Pswitch interrupt 
//! 
//! \fntype Function                        
//!     
//! \retval - HW_POWER_CHECK_PSWITCH_LOW - when Pswitch is low
//! \retval - HW_POWER_CHECK_PSWITCH_HIGH - when Pswitch is high
//!                        
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetPswitchInterruptPolarity(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Enable LinReg OK interrupt
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableLinregOkInterrupt(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Clears the LinReg interrupt status bit 
//! 
//! \fntype Function                        
//!     
////////////////////////////////////////////////////////////////////////////////
void hw_power_ClearLinregOkInterrupt(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Set LinReg interrupt polarity
//! 
//! \fntype Function                        
//!     
//! (Description here)
//!
//! \param[in] 	 bPolarity - HW_POWER_CHECK_5V_DISCONNECTED or 
//!                          HW_POWER_CHECK_LINEAR_REGULATORS_OK
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetLinregOkInterruptPolarity(bool bPolarity);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Return polarity for LinReg interrupt
//! 
//! \fntype Function                        
//!     
//! \retval HW_POWER_CHECK_5V_DISCONNECTED - trigger when 5V disconnected
//! \retval HW_POWER_CHECK_LINEAR_REGULATORS_OK - trigger when LinRegs OK 
//!                        
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetLinregOkInterruptPolarity(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Enable DC OK interrupt
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDcOkInterrupt(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Clears the DC OK interrupt status bit 
//! 
//! \fntype Function                        
////////////////////////////////////////////////////////////////////////////////
void hw_power_ClearDcOkInterrupt(void);
  
////////////////////////////////////////////////////////////////////////////////
//! \brief Enable Battery interrupt
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableBatteryBrownoutInterrupt(bool bEnable);
                          
////////////////////////////////////////////////////////////////////////////////
//! \brief Clears the battery interrupt status bit 
//! 
//! \fntype Function                        
////////////////////////////////////////////////////////////////////////////////
void hw_power_ClearBatteryBrownoutInterrupt(void);
                              
////////////////////////////////////////////////////////////////////////////////
//!  
//! Name: hw_power_EnableVddioBrownoutInterrupt  
//! 
//! \brief Enable Vddio brownout interrupt
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableVddioBrownoutInterrupt(bool bEnable);
                              
////////////////////////////////////////////////////////////////////////////////
//! \brief Clears the Vddio interrupt status bit 
//! 
//! \fntype Function                        
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_ClearVddioBrownoutInterrupt(void);
   
////////////////////////////////////////////////////////////////////////////////
//! \brief Enable Vdda brownout interrupt
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableVddaBrownoutInterrupt(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//!  
//! Name: hw_power_ClearVddaBrownoutInterrupt  
//! 
//! \brief Clears the Vdda interrupt status bit
//! 
//! \fntype Function                        
//!     
////////////////////////////////////////////////////////////////////////////////
void hw_power_ClearVddaBrownoutInterrupt(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Enable Vddd brownout interrupt
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableVdddBrownoutInterrupt(bool bEnable);

////////////////////////////////////////////////////////////////////////////////
//! \brief Clears the Vddd interrupt status bit
//! 
//! \fntype Function                        
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_ClearVdddBrownoutInterrupt(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Enable VbusValid interrupt
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableVbusValidInterrupt(bool bEnable);
                 
////////////////////////////////////////////////////////////////////////////////
//! \brief Clears the VbusValid interrupt status bit 
//! 
//! \fntype Function                        
//!     
////////////////////////////////////////////////////////////////////////////////
void hw_power_ClearVbusValidInterrupt(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Return polarity for VbusValid interrupt
//! 
//! \retval HW_POWER_CHECK_5V_DISCONNECTED - trigger when 5V disconnected
//! \retval HW_POWER_CHECK_5V_CONNECTED - trigger when 5V connected
//!
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetVbusValidInterruptPolarity(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Set the VbusValid interrupt polarity 
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bPolarity - HW_POWER_CHECK_5V_DISCONNECTED or
//!                          HW_POWER_CHECK_5V_CONNECTED
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVbusValidInterruptPolarity(bool bPolarity);

////////////////////////////////////////////////////////////////////////////////
//! \brief Enable VDD5V_GT_VDDIO interrupt
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableVdd5vGtVddioInterrupt(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Clears the VDD5V_GT_VDDIO interrupt status bit
//! 
//! \fntype Function                        
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_ClearVdd5vGtVddioInterrupt(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Set the VDD5V_GT_VDDIO interrupt polarity 
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bPolarity - HW_POWER_CHECK_5V_DISCONNECTED or
//!                          HW_POWER_CHECK_5V_CONNECTED
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVdd5vGtVddioInterruptPolarity(bool bPolarity);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Return polarity for VDD5V_GT_VDDIO interrupt
//! 
//! \retval HW_POWER_CHECK_5V_DISCONNECTED - trigger when 5V disconnected
//! \retval HW_POWER_CHECK_5V_CONNECTED - trigger when 5V connected
//!
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetVdd5vGtVddioInterruptPolarity(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Enable 5V interrupt
//! 
//! \fntype Function                        
//!     
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_Enable5vInterrupt(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Clears the 5V interrupt status bit
//! 
//! \fntype Function                        
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_Clear5vInterrupt(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets/Clears CLKGATE bit in POWER_CTRL 
//! 
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetPowerClkGate(bool bGate);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns CLKGATE bit in POWER_CTRL 
//! 
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetPowerClkGate(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets/Clears VBUSVALID_5VDETECT bit in POWER_5VCTRL 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVbusValidThresh(hw_power_VbusValidThresh_t eThresh);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets/Clears VBUSVALID_5VDETECT bit in POWER_5VCTRL
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableVbusValid5vDetect(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets/Clears PWDN_5VBRNOUT bit in POWER_5VCTRL
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableAutoHardwarePowerdown(bool bDisable);

////////////////////////////////////////////////////////////////////////////////
//! \brief Clears PWDN_BATTBRNOUT bit in POWER_BATTMONITOR
//!
//////////////////////////////////////////////////////////////////////////////// 
void hw_power_DisableBrownoutPowerdown(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Clears PWDN_5VBRNOUT bit in POWER_5VCTRL
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_Disable5vBrownoutPowerdown(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets/Clears ENABLE_DCDC bit in POWER_5VCTRL
//!
//! \param[in] bEnable - TRUE to set, FALSE to clear
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDcdc(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns ENABLE_DCDC in POWER_5VCTRL 
//! 
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetEnableDcdc(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets/Clears DCDC_XFER bit in POWER_5VCTRL
//!
//! \param[in] bEnable - TRUE to set, FALSE to clear
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableAutoDcdcTransfer(bool bEnable);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets/Clears ENABLE_ILIMIT bit in POWER_5VCTRL
//!
//! \param[in] bEnable - TRUE to set, FALSE to clear
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableCurrentLimit(bool bEnable);

////////////////////////////////////////////////////////////////////////////////
//! \brief Return resistor source to generate bias current
//!
//! \retval HW_POWER_INTERNAL_BIAS_CURRENT - use internal resistor to generate bias current 
//! \retval HW_POWER_EXTERNAL_BIAS_CURRENT - use external resistor to generate bias current
//!
////////////////////////////////////////////////////////////////////////////////
hw_power_BiasCurrentSource_t hw_power_GetBiasCurrentSource(void);
                   
////////////////////////////////////////////////////////////////////////////////
//! \brief Select resistor source for bias current
//! 
//! \fntype Function                        
//!
//! \param[in] 	 eSource - HW_POWER_INTERNAL_BIAS_CURRENT - use internal resistor to generate bias current 
//!                        HW_POWER_EXTERNAL_BIAS_CURRENT - use external resistor to generate bias current
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetBiasCurrentSource(hw_power_BiasCurrentSource_t eSource);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief State of battery charger circuit  
//! 
//! \fntype Function                        
//!     
//! Returns the state of the power down bit which controls the battery
//! charging circuitry.
//!
//! \retval TRUE - Battery charger circuit powered down
//! \retval FALSE - Battery charger circuit powered up
//!                        
//! \notes The bit is a power down bit so setting the bit 
//! \notes TRUE will power down the circuit
//!
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetPowerDownBatteryCharger(void);
                   
////////////////////////////////////////////////////////////////////////////////
//! \brief Control the battery charge circuit
//! 
//! \fntype Function                        
//!     
//! Powers down or powers up the battery charge circuit.  The circuit should
//! only be powered up when 5V is present.
//!
//! \param[in] 	 bPowerDown - TRUE to power down, FALSE to power up
//!
//! \notes The bit is a power down bit so setting the bit 
//! \notes TRUE will power down the circuit
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetPowerDownBatteryCharger(bool bPowerDown);
                                                              
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the linear regulator offset from target voltage
//! 
//! \fntype Function                        
//!     
//! The linear regulator for each rail can be set equal to or 25 mV above or below
//! the target voltage.  This function sets the new value of the offset.
//!
//! \param[in] eOffset - HW_POWER_LINREG_OFFSET_NO_STEP    LinReg output = target voltage
//!                      HW_POWER_LINREG_OFFSET_STEP_ABOVE LinReg output = target voltage + 25mV
//!                      HW_POWER_LINREG_OFFSET_STEP_BELOW LinReg output = target voltage - 25mV
//!                      HW_POWER_LINREG_OFFSET_MAX        LinReg output = target voltage - 25mV
//!                        
//! \notes None
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVdddLinRegOffset(hw_power_LinRegOffsetStep_t eOffset);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the linear regulator offset from target voltage
//! 
//! \fntype Function                        
//!     
//! The linear regulator for each rail can be set equal to or 25 mV above or below
//! the target voltage.  This function sets the new value of the offset.
//!
//! \param[in] eOffset - HW_POWER_LINREG_OFFSET_NO_STEP    LinReg output = target voltage
//!                      HW_POWER_LINREG_OFFSET_STEP_ABOVE LinReg output = target voltage + 25mV
//!                      HW_POWER_LINREG_OFFSET_STEP_BELOW LinReg output = target voltage - 25mV
//!                      HW_POWER_LINREG_OFFSET_MAX        LinReg output = target voltage - 25mV
//!                        
//! \notes None
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddaLinRegOffset(hw_power_LinRegOffsetStep_t eOffset);

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the linear regulator offset from target voltage
//! 
//! \fntype Function                        
//!     
//! The linear regulator for each rail can be set equal to or 25 mV above or below
//! the target voltage.  This function sets the new value of the offset.
//!
//! \param[in] eOffset - HW_POWER_LINREG_OFFSET_NO_STEP    LinReg output = target voltage
//!                      HW_POWER_LINREG_OFFSET_STEP_ABOVE LinReg output = target voltage + 25mV
//!                      HW_POWER_LINREG_OFFSET_STEP_BELOW LinReg output = target voltage - 25mV
//!                      HW_POWER_LINREG_OFFSET_MAX        LinReg output = target voltage - 25mV
//!                        
//! \notes None
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddioLinRegOffset(hw_power_LinRegOffsetStep_t eOffset);

////////////////////////////////////////////////////////////////////////////////
//! \brief Returns offset from target voltage that the LinReg is currently set to
//! 
//! \fntype Function                        
//!     
//! The linear regulator for each rail can be set equal to or 25 mV above or below
//! the target voltage.  This function returns the current value of the offset.
//!
//! \retval HW_POWER_LINREG_OFFSET_NO_STEP    LinReg output = target voltage
//! \retval HW_POWER_LINREG_OFFSET_STEP_ABOVE LinReg output = target voltage + 25mV
//! \retval HW_POWER_LINREG_OFFSET_STEP_BELOW LinReg output = target voltage - 25mV
//! \retval HW_POWER_LINREG_OFFSET_MAX        LinReg output = target voltage - 25mV
//!                        
//! \notes None
//!
////////////////////////////////////////////////////////////////////////////////
hw_power_LinRegOffsetStep_t hw_power_GetVdddLinRegOffset(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns offset from target voltage that the LinReg is currently set to
//! 
//! \fntype Function                        
//!     
//! The linear regulator for each rail can be set equal to or 25 mV above or below
//! the target voltage.  This function returns the current value of the offset.
//!
//! \retval HW_POWER_LINREG_OFFSET_NO_STEP    LinReg output = target voltage
//! \retval HW_POWER_LINREG_OFFSET_STEP_ABOVE LinReg output = target voltage + 25mV
//! \retval HW_POWER_LINREG_OFFSET_STEP_BELOW LinReg output = target voltage - 25mV
//! \retval HW_POWER_LINREG_OFFSET_MAX        LinReg output = target voltage - 25mV
//!                        
//! \notes None
//!
////////////////////////////////////////////////////////////////////////////////
hw_power_LinRegOffsetStep_t hw_power_GetVddaLinRegOffset(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns offset from target voltage that the LinReg is currently set to
//! 
//! \fntype Function                        
//!     
//! The linear regulator for each rail can be set equal to or 25 mV above or below
//! the target voltage.  This function returns the current value of the offset.
//!
//! \retval HW_POWER_LINREG_OFFSET_NO_STEP    LinReg output = target voltage
//! \retval HW_POWER_LINREG_OFFSET_STEP_ABOVE LinReg output = target voltage + 25mV
//! \retval HW_POWER_LINREG_OFFSET_STEP_BELOW LinReg output = target voltage - 25mV
//! \retval HW_POWER_LINREG_OFFSET_MAX        LinReg output = target voltage - 25mV
//!                        
//! \notes None
//!
////////////////////////////////////////////////////////////////////////////////
hw_power_LinRegOffsetStep_t hw_power_GetVddioLinRegOffset(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Change the DCDC converter response time 
//! 
//! \fntype Function                        
//!     
//! Enable analog circuit of DCDC converter to respond faster under
//! transient load conditions.
//!
//! \param[in] 	 eLevel - HW_POWER_RCSCALE_DISABLED - normal response
//!                       HW_POWER_RCSCALE_2X_INCR  - 2 times faster response
//!                       HW_POWER_RCSCALE_4X_INCR  - 4 times faster response
//!                       HW_POWER_RCSCALE_8X_INCR  - 8 times faster response
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableRcScale(hw_power_RcScaleLevels_t eLevel);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Return status of bit to detect if battery charger is present
//! 
//! \fntype Function                        
//!     
//! \retval - TRUE if charger hardware is present 
//!                        
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetBattChrgPresentFlag(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns status of 5V presence 
//! 
//! \fntype Function                        
//!     
//! Returns TRUE if 5V is detected.
//!
//! \retval - TRUE if 5V present
//!
////////////////////////////////////////////////////////////////////////////////
bool hw_power_Get5vPresentFlag(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the battery mode the chip is operating in. 
//! 
//! \fntype Function                        
//!     
//! \retval - HW_POWER_BATT_MODE_LIION when LiIon (usually rechargable and non-removable)   
//! \retval - HW_POWER_BATT_MODE_ALKALINE_NIMH when Alkaline or NiMH (usually AA or AAA and replacable)
//!
////////////////////////////////////////////////////////////////////////////////
hw_power_BatteryMode_t hw_power_GetBatteryMode(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns status of battery charger 
//! 
//! \fntype Function                        
//!     
//! If the battery charger is currently charging a battery, this bit will be TRUE
//!
//! \retval - TRUE if battery is being charged
//!                        
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetBatteryChargingStatus(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Powers down the chip
//! 
//! \fntype Function                        
//!     
//! This function will set the PWD in the HW_POWER_RESET register.  
//! The chip will be powered down.
//!
//! \notes None
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_PowerDown(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Disable the chip powerdown bit
//! 
//! \fntype Function                        
//!     
//! Optional bit to disable all paths to power off the chip except the watchdog 
//! timer. Setting this bit will be useful for preventing fast falling edges 
//! on the PSWITCH pin from resetting the chip. It may also be useful increasing 
//! system tolerance of noisy EMI environments.
//!
//! \param[in] 	 bDisable - TRUE to disable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisablePowerDown(bool bDisable);

////////////////////////////////////////////////////////////////////////////////
//! \brief Enable the inductor charge from VDDD supply in boost mode
//! 
//! \fntype Function                        
//!     
//! Enables charging of the inductor in boost mode from the VDDD supply instead
//! of ground when the battery is 200mV above the programmed VDDD voltage.
//!
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableAlkalineCharge(bool bEnable);

////////////////////////////////////////////////////////////////////////////////
//! \brief Switch VDDD regulator to the battery from VDDA
//! 
//! \fntype Function                        
//!     
//! Switches the source of the VDDD regulator to the battery from VDDA.  The
//! default source is the VDDA supply.  This bit should only be set in alkaline-
//! powered applications when the battery si greater than 100mV above the
//! programmed VDDD voltage and ENABLE_LINREG is also set. 
//!
//! \param[in] 	 bEnable - TRUE to enable 
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableVdddLinRegFromBatt(bool bEnable);

////////////////////////////////////////////////////////////////////////////////
//! \brief Convert a current in mA to a hardware setting.
//!
//! \fntype Function
//!
//! This function converts a current measurement in mA to a hardware setting
//! used by HW_POWER_BATTCHRG.STOP_ILIMIT or HW_POWER_BATTCHRG.BATTCHRG_I.
//!
//! Note that the hardware has a minimum resolution of 10mA and a maximum
//! expressible value of 780mA (see the data sheet for details). If the given
//! current cannot be expressed exactly, then the largest expressible smaller
//! value will be used.
//!
//! \param[in]  u16Current  The current of interest.
//!
//! \retval  The corresponding setting.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t  hw_power_ConvertCurrentToSetting(uint16_t u16Current);

////////////////////////////////////////////////////////////////////////////////
//! \brief Convert a hardware current setting to a value in mA.
//!
//! \fntype Function
//!
//! This function converts a setting used by HW_POWER_BATTCHRG.STOP_ILIMIT or
//! HW_POWER_BATTCHRG.BATTCHRG_I into an actual current measurement in mA.
//!
//! Note that the hardware current fields are 6 bits wide. The higher bits in
//! the 8-bit input parameter are ignored.
//!
//! \param[in]  u8Setting  A hardware current setting.
//!
//! \retval  The corresponding current in mA.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t  hw_power_ConvertSettingToCurrent(uint16_t u16Setting);

////////////////////////////////////////////////////////////////////////////////
//! \brief Convert a mV value to the corresponding VDDD voltage setting
//!
//! \fntype Function
//!
//! This function converts the mV value passed in to the VDDD voltage setting to
//! be used in HW_POWER_VDDDCTRL.B.TRG.
//!
//! Note that the hardware target voltage fields are 5 bits wide. The higher bits in
//! the 16-bit input parameter are ignored.
//!
//! \param[in]  u16Vddd  A hardware voltage setting for VDDD.
//!
//! \retval  The corresponding VDDD register setting.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_ConvertVdddToSetting(uint16_t u16Vddd);

////////////////////////////////////////////////////////////////////////////////
//! \brief Convert a VDDD target register setting to the corresponding mV value
//!
//! \fntype Function
//!
//! This function converts the VDDD voltage setting that can be used in 
//! HW_POWER_VDDDCTRL.B.TRG to the corresponding mV value  
//!
//! Note that the hardware target voltage fields are 5 bits wide. The higher bits in
//! the 16-bit input parameter are ignored.
//!
//! \param[in]  u16Vddd  A hardware voltage setting for VDDD.
//!
//! \retval  The corresponding VDDD voltage in mV.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_ConvertSettingToVddd(uint16_t u16Setting);

////////////////////////////////////////////////////////////////////////////////
//! \brief Convert a mV value to the corresponding VDDA voltage setting
//!
//! \fntype Function
//!
//! This function converts the mV value passed in to the VDDA voltage setting to
//! be used in HW_POWER_VDDACTRL.B.TRG.
//!
//! Note that the hardware target voltage fields are 5 bits wide. The higher bits in
//! the 16-bit input parameter are ignored.
//!
//! \param[in]  u16Vdda  A hardware voltage setting for VDDA.
//!
//! \retval  The corresponding VDDA register setting.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_ConvertVddaToSetting(uint16_t u16Vdda);

////////////////////////////////////////////////////////////////////////////////
//! \brief Convert a VDDA target register setting to the corresponding mV value
//!
//! \fntype Function
//!
//! This function converts the VDDA voltage setting that can be used in 
//! HW_POWER_VDDACTRL.B.TRG to the corresponding mV value  
//!
//! Note that the hardware target voltage fields are 5 bits wide. The higher bits in
//! the 16-bit input parameter are ignored.
//!
//! \param[in]  u16Vdda  A hardware voltage setting for VDDA.
//!
//! \retval  The corresponding VDDA voltage in mV.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_ConvertSettingToVdda(uint16_t u16Setting);

////////////////////////////////////////////////////////////////////////////////
//! \brief Convert a mV value to the corresponding VDDIO voltage setting
//!
//! \fntype Function
//!
//! This function converts the mV value passed in to the VDDIO voltage setting to
//! be used in HW_POWER_VDDIOCTRL.B.TRG.                           
//!
//! Note that the hardware target voltage fields are 5 bits wide. The higher bits in
//! the 16-bit input parameter are ignored.
//!
//! \param[in]  u16Vddio  A hardware voltage setting for VDDIO.
//!
//! \retval  The corresponding VDDIO register setting.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_ConvertVddioToSetting(uint16_t u16Vddio);

////////////////////////////////////////////////////////////////////////////////
//! \brief Convert a VDDIO target register setting to the corresponding mV value
//!
//! \fntype Function
//!
//! This function converts the VDDIO voltage setting that can be used in 
//! HW_POWER_VDDIOCTRL.B.TRG to the corresponding mV value  
//!
//! Note that the hardware target voltage fields are 5 bits wide. The higher bits in
//! the 16-bit input parameter are ignored.
//!
//! \param[in]  u16Vddio  A hardware voltage setting for VDDIO.
//!
//! \retval  The corresponding VDDIO voltage in mV.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_ConvertSettingToVddio(uint16_t u16Setting);

////////////////////////////////////////////////////////////////////////////////
//! \brief Set the positive limit for the boost mode duty-cycle.
//!
//! \fntype Function
//!
//! This function sets the positive limit for the duty-cycle for
//! boost mode operation.  This limits the maximum time in the duty-cycle
//! any power supply can be charged in boost mode.
//!
//! \param[in] u16Limit Boost mode positive limit.
//!
//! \retval None.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetPosLimitBoost(uint16_t u16Limit);

////////////////////////////////////////////////////////////////////////////////
//! \brief Set the positive limit for the buck mode duty-cycle.
//!
//! \fntype Function
//!
//! This function sets the positive limit for the duty-cycle for
//! buck mode operation.  This limits the maximum time in the duty-cycle
//! any power supply can be charged in buck mode.
//!
//! \param[in] u16Limit Buck mode positive limit.
//!
//! \retval None.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetPosLimitBuck(uint16_t u16Limit);

////////////////////////////////////////////////////////////////////////////////
//! \brief Set the negative limit for the DCDC duty-cycle.
//!
//! \fntype Function
//!
//! This function sets the negative limit for the duty-cycle.  This limits the
//! time any rail does not get charged in the DCDC duty-cycle.  
//!
//! \param[in] u16Limit Negative DCDC duty-cycle limit.
//!
//! \retval None.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetNegLimit(uint16_t u16Limit);

////////////////////////////////////////////////////////////////////////////////
//! \brief Enable double FETs for high load situations.
//!
//! \fntype Function
//!
//! \param[in] bEnable TRUE to enable.
//!
//! \retval None.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDoubleFets(bool bEnable);

void hw_power_EnableHalfFets(bool bEnable);

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the ratio of integral control to proportional control in the DCDC.
//!
//! \fntype Function
//!
//! \param[in] u16Value Ration of integral to proportional control
//!
//! \retval None.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetLoopCtrlDcC(uint16_t u16Value);

////////////////////////////////////////////////////////////////////////////////
//! \brief Brownout FIQ handler to check if brownouts interrupts are valid.
//!
//! \fntype FIQ handler
//!
//! Checks the brownout status bits and IRQ bits to  determine the validity of
//! the brownout IRQ.  If valid, this function shuts down the chip.  Otherwise,
//! it passes control back to the os_pmi_FiqHandler.  
//!
//! \param[in] None.
//!
//! \retval TRUE The brownout is valid and it not a false brownout generated
//! by the supplies.  The PMI FIQ needs to handle it.
//! \retval FLSE The supplies generated a false brownout and the PMI FIQ
//! can ignore handling it.  
////////////////////////////////////////////////////////////////////////////////
bool hw_power_FiqHandler(void);

////////////////////////////////////////////////////////////////////////////////
//
//! \brief Sets the register value for the new DCDC clock frequency.
//!
//! \fntype Function
//!
//! This function will convert the requested frequency to a register setting and
//! then write those values to the register.  If an invalid frequency is input,
//! the function will use a default value of 24MHz.  
//!
//! \param[in] u16Freq New DCDC frequency in kHz
//! \param[in] bUsePll TRUE if PLL is to be used as clock source. FALSE if
//! crystal will be used.
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetDcdcClkFreq(uint16_t u16Freq, bool bUsePll);

////////////////////////////////////////////////////////////////////////////////
//
//! \brief Returns the current polarity of the 5V interrupt.
//!
//! \fntype Function
//!
//! The 5V interrupt can be set to detect an insertion or removal of a 5V 
//! signal.  This function returns the current setting in hardware.
//!
//! \return true if set to detect insertion event, false if set to detect
//! a removal event. 
//!
////////////////////////////////////////////////////////////////////////////////
bool hw_power_Get5vInterruptPolarity(void);
////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
#endif // __HW_POWER_H

