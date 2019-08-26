////////////////////////////////////////////////////////////////////////////////
//! \addtogroup ddi_power
//! @{
//
// Copyright (c) 2004-2005 SigmaTel, Inc.
//
//! \file ddi_power.h
//! \brief Contains header data for the Power Supply Device Driver Interface.
//! \todo [PUBS] Add definitions for TBDs in this file. Medium priority.
////////////////////////////////////////////////////////////////////////////////
#ifndef _DDI_POWER_H
#define _DDI_POWER_H

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include "ddi_power_translation.h"

#if 0
// This is only here because of the dependency in the USb driver.  When that is
// cleaned up, we can remove this.  
#include "hw/lradc/hw_lradc.h"
// This is only here because of the dependency in the USb driver.  When that is
// cleaned up, we can remove this.  
//#include "registers/regspower.h"
// This is only here because of the dependency in the infobar.  When that is
// cleaned up, we can remove this.  
#endif
#include "hw_power.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! \brief TBD
#define DDI_PWR_HANDOFF_DEBOUNCE_TICKS  3
#define DDI_POWER_HANDOFF_DEBOUNCE_TICKS 3
//! \brief TBD
#define DDI_PWR_DCDC_READY_TIMEOUT      20000 //20 milliseconds
#define DDI_PWR_LINREG_READY_TIMEOUT      1000 //20 milliseconds

// 36xx enums

//! \brief Battery modes
typedef enum _ddi_power_BatteryMode_t
{
// 37xx battery modes
	//! \brief LiIon battery powers the player
    DDI_POWER_BATT_MODE_LIION           = 0,
	//! \brief Alkaline/NiMH battery powers the player
    DDI_POWER_BATT_MODE_ALKALINE_NIMH   = 1,

// 36xx battery modes
	//! \brief 36xx batt mode
    DDI_POWER_BATT_MODE_0                      = 0,
	//! \brief 36xx batt mode
    DDI_POWER_BATT_MODE_LIION_DUAL_CONVERTOR   = 0,
	//! \brief 36xx batt mode
    DDI_POWER_BATT_MODE_1                      = 1,
	//! \brief 36xx batt mode
    DDI_POWER_BATT_MODE_LIION_SINGLE_INDUCTOR  = 1,
	//! \brief 36xx batt mode
    DDI_POWER_BATT_MODE_2                      = 2,
	//! \brief 36xx batt mode
    DDI_POWER_BATT_MODE_SERIES_AA_AAA          = 2,
	//! \brief 36xx batt mode
    DDI_POWER_BATT_MODE_DUAL_ALKALINE_NIMH     = 2,
	//! \brief 36xx batt mode
    DDI_POWER_BATT_MODE_3                      = 3,
	//! \brief 36xx batt mode
    DDI_POWER_BATT_MODE_SINGLE_AA              = 3,
	//! \brief 36xx batt mode
    DDI_POWER_BATT_MODE_SINGLE_ALKALINE_NIMH   = 3
} ddi_power_BatteryMode_t;

//! \brief Available sources for bias currents
typedef enum _ddi_power_BiasCurrentSource_t
{
 	//! \brief Use external resistor to generate bias current
    DDI_POWER_EXTERNAL_BIAS_CURRENT = 0x0,
 	//! \brief Use internal resistor to generate bias current
    DDI_POWER_INTERNAL_BIAS_CURRENT = 0x1
} ddi_power_BiasCurrentSource_t; 

//! \brief Possible power sources for each power supply
typedef enum _ddi_power_PowerSource_t
{
	//! \brief Powered by linear regulator.
    DDI_POWER_LINEAR_REGULATOR,
	//! \brief Powered by DCDC converter.
    DDI_POWER_DCDC
}ddi_power_PowerSource_t;

//! \brief Available alkaline optimization states
typedef enum _ddi_power_AlkalineState_t
{
    //! \brief Vddd source is DCDC converter
    DDI_POWER_ALKALINE_DCDC,
    //! \brief Vddd source is linear regulator from Vdda.
    DDI_POWER_ALKALINE_LINREG_VDDA,
    //! \brief Vddd source is linear regulator from the battery.
    DDI_POWER_ALKALINE_LINREG_BATT,
    //! \brief Vddd source is linear regulator from the battery, and
    //! alkaline charge is enabled.  
    DDI_POWER_ALKALINE_LINREG_ALKA_CHARGE,
    //! \brief Default state is DDI_POWER_ALKALINE_LINREG_VDDA
    DDI_POWER_ALKALINE_DEFAULT = DDI_POWER_ALKALINE_LINREG_VDDA
} ddi_power_AlkalineState_t;

//! \brief Available 5V detection methods.
typedef enum _ddi_power_5vDetection_t
{
    //! \brief VBUSVALID will be used for 5V/USB detection.
    DDI_POWER_VBUSVALID,
    //! \brief VDD5V_GT_VDDIO will be used for 5V/USB detection.
    DDI_POWER_VDD5V_GT_VDDIO
} ddi_power_5vDetection_t;

//! \brief typedef for the power event message generator routine
typedef void ddi_power_MsgHandler_t(void);

////////////////////////////////////////////////////////////////////////////////
// Structures
////////////////////////////////////////////////////////////////////////////////
typedef struct _ddi_power_InitValues_t
{
    uint32_t                u32BatterySamplingInterval;
    ddi_power_5vDetection_t e5vDetection;
} ddi_power_InitValues_t;

//! \brief Holds the callback functions related to 5V-battery handoffs.
typedef struct _ddi_power_PowerHandoff_t
{
    //! \brief Function called when a handoff starts.
    ddi_power_MsgHandler_t *pFxnHandoffStartCallback;
    //! \brief Function called when a handoff ends.
    ddi_power_MsgHandler_t *pFxnHandoffEndCallback;
    //! \brief Function called when handoff is a battery-to-5V transition
    ddi_power_MsgHandler_t *pFxnHandoffTo5VoltCallback;
    //! \brief Function called when handoff is a 5V-to-battery transition
    ddi_power_MsgHandler_t *pFxnHandoffToBatteryCallback;
}ddi_power_PowerHandoff_t;

////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////
extern ddi_power_PowerHandoff_t   g_ddi_power_Handoff;

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

//Power supply driver public API.
////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Initializes the power driver
//!
//! \fntype Function
//! This function initializes the power-supply hardware.
//!
//! \param[in]  InitValues Structure with initial values for the power driver.
//!
//! \return     SUCCESS No error
//!             Others  Error
//!
//! \internal
//! \see To view the function definition, see ddi_power_init.c.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_Init(ddi_power_InitValues_t* pInitValues);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Initializes the power handoff handler
//!
//! \fntype Function
//! This function sets up battery-5V handoff, installs the VDD5V interrupt
//! handler, creates a TX application timer to handle the 5V-battery handoff
//! debounce, and calls the callback functions:
//!     pFnxHandoffStartCallback on a handoff event detected,
//!     pFnxHandoffEndCallback on the handoff debounce end,
//!     pFxnHandoffTo5VoltCallback on the handoff-to-5V debounce end,
//!     pFxnHandoffToBatteryCallback on the handoff-to-battery debounce end.
//!
//! \param[in]  pFnxHandoffStartCallback        Pointer to the handoff-start callback function
//! \param[in]  pFnxHandoffEndCallback          Pointer to the handoff-end callback function
//! \param[in]  pFxnHandoffTo5VoltCallback      Pointer to the send-handoff-to-5V message function
//! \param[in]  pFxnHandoffToBatteryCallback    Pointer to the send-handoff-to-battery message function
//! \param[in]  u32HandoffDebounce              Battery-5V handoff debounce time in ticks
//!
//! \return     SUCCESS                         No error
//!             Others                          Error
//!
//! \note This function runs in ThreadX environment
//!
//! \internal
//! \see To view the function definition, see ddi_power_init.c.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_HandoffInit(ddi_power_MsgHandler_t  *pFnxHandoffStartCallback,
                                  ddi_power_MsgHandler_t  *pFnxHandoffEndCallback,
                                  ddi_power_MsgHandler_t  *pFxnHandoffTo5VoltCallback,
                                  ddi_power_MsgHandler_t  *pFxnHandoffToBatteryCallback,
                                  uint32_t                u32HandoffDebounce,
                                  uint32_t                battery_check);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Set the VDDD and its brownout voltages.
//!
//! \fntype     Non-reentrant Function
//!
//! This function sets the VDDD value and VDDD brownout level specified by the
//! input parameters. If the new brownout level is equal to the current setting
//! it'll only update the VDDD setting. If the new brownout level is less than
//! the current setting, it will update the VDDD brownout first and then the VDDD.
//! Otherwise, it will update the VDDD first and then the brownout. This
//! arrangement is intended to prevent from false VDDD brownout. This function
//! will not return until the output VDDD is stable.
//!
//! \param[in]  u16Vddd_mV              Vddd voltage in millivolts
//! \param[in]  u16VdddBrownout_mV      Vddd brownout in millivolts
//!
//! \return SUCCESS.
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_SetVddd(uint16_t  u16Vddd_mV, uint16_t  u16VdddBrownout_mV);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDD voltage
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value of the VDDD voltage
//!
//! \retval Present VDDD voltage in mV
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVddd(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDD brownout level
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value VDDD brownout
//!
//! \retval Present VDDD brownout voltage in mV 
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVdddBrownout(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Set the VDDA and its brownout voltages.
//!
//! \fntype     Non-reentrant Function
//!
//! This function sets the VDDA value and VDDA brownout level specified by the
//! input parameters. If the new brownout level is equal to the current setting
//! it'll only update the VDDA setting. If the new brownout level is less than
//! the current setting, it will update the VDDA brownout first and then the VDDA.
//! Otherwise, it will update the VDDA first and then the brownout. This
//! arrangement is intended to prevent from false VDDA brownout. This function
//! will not return until the output VDDA stable.
//!
//! \param[in]  u16Vdda_mV              Vdda voltage in millivolts
//! \param[in]  u16VddaBrownout_mV      Vdda brownout in millivolts
//!
//! \return SUCCESS.
//!
//! \internal
//! \see To view the function definition, see ddi_power.c.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_SetVdda(uint16_t  u16Vdda_mV, uint16_t  u16VddaBrownout_mV);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDA voltage
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value of the VDDA voltage
//!
//! \retval Present VDDA voltage in mV
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVdda(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDA brownout level
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value VDDA brownout
//!
//! \retval Present VDDA brownout voltage in mV 
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVddaBrownout(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Set the VDDIO and its brownout voltages.
//!
//! \fntype     Non-reentrant Function
//!
//! This function sets the VDDIO value and VDDIO brownout level specified by the
//! input parameters. If the new brownout level is equal to the current setting
//! it'll only update the VDDIO setting. If the new brownout level is less than
//! the current setting, it will update the VDDIO brownout first and then the VDDIO.
//! Otherwise, it will update the VDDIO first and then the brownout. This
//! arrangement is intended to prevent from false VDDIO brownout. This function
//! will not return until the output VDDIO stable.
//!
//! \param[in]  u16Vddio_mV              Vddio voltage in millivolts
//! \param[in]  u16VddioBrownout_mV      Vddio brownout in millivolts
//!
//! \return SUCCESS.
//!
//! \internal
//! \see To view the function definition, see ddi_power.c.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_SetVddio(uint16_t  u16Vddio_mV, uint16_t  u16VddioBrownout_mV);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDIO voltage
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value of the VDDIO voltage
//!
//! \retval Present VDDIO voltage in mV
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVddio(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDIO brownout level
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value VDDIO brownout
//!
//! \retval Present VDDIO brownout voltage in mV 
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVddioBrownout(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Set battery brownout level
//!
//! \fntype     Reentrant Function
//!
//! This function sets the battery brownout level in millivolt. It transforms the
//! input brownout value from millivolts to the hardware register bit field value
//! with taking the ceiling value in the calculation.
//!
//! \param[in]  u16BattBrownout_mV      Battery battery brownout level in mV
//!
//! \return     SUCCESS
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_SetBatteryBrownout(uint16_t  u16BattBrownout_mV);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief  Get the battery voltage 
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value of the battery voltage The battery 
//! value is from LRADC channel -7 battery measurement.
//!
//! \retval Present battery voltage in mV
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetBattery(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief  Get the battery voltage brownout 
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value of the battery voltage brownout.
//!
//! \retval Present battery voltage brownout in mV
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetBatteryBrownout(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Enable 5V-to-battery handoff
//!
//! \fntype Function
//!
//! This function prepares the hardware for a 5V-to-battery handoff.  It assumes
//! the current configuration is using 5V as the power source.  The 5V 
//! interrupt will be set up for a 5V removal.
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_Enable5VoltsToBatteryHandoff(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Transfers the power source from 5V to the battery.
//! 
//! \fntype Function
//!
//! This function will handle all the power rail transitions necesarry to power
//! the chip from the battery when it was previously powered from the 5V power
//! source.  
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_Execute5VoltsToBatteryHandoff(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Enable battery-to-5V handoff
//! 
//! \fntype Function
//!
//! This function sets up battery-to-5V handoff. The power switch from
//! battery to 5V is automatic. This funtion enables the 5V present detection
//! such that the 5V interrupt can be generated if it is enabled. (The interrupt
//! handler can inform software the 5V present event.) To deal with noise or
//! a high current, this function enables DCDC1/2 based on the battery mode.
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_EnableBatteryTo5VoltsHandoff(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Transfers the  power source from battery to 5V.
//!
//! \fntype Function
//!
//! This function handles the transitions on each of the power rails necessary
//! to power the chip from the 5V power supply when it was previously powered
//! from the 5V power supply.
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_ExecuteBatteryTo5VoltsHandoff(void);

RtStatus_t ddi_power_OptimizeAlkaline(bool bInductorCharge,
                                      bool bEnableDcdc,
                                      bool bEnableLinReg,
                                      bool bLinRegFromBatt);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Power down the chip without clearing the persistent bits
//!
//! \fntype Function
//!
//! This function shuts off the power without clearing the persistent bits.
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_PowerDown(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Power down the chip with clearing the persistent bits
//!
//! \fntype Function
//!
//! This function shuts off the power the chip with clearing the persistent bits.
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_ColdPowerDown(void);

///////////////////////////////////////////////////////////////////////////////
//!
//! \brief Reboot the chip
//!
//! \fntype Function
//!
//! This function resets all the non-power module digital registers and reboots
//! the CPU.
//!
//! \internal
//! \see To view the function definition, see ddi_power.c.
////////////////////////////////////////////////////////////////////////////////
void ddi_power_ColdReboot(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Restart the chip, keeping persistent bits intact
//!
//! \fntype Function
//!
//! This function sets the auto restart persistent bit and powers down the
//! chip. The auto restart will cause it to come back up immediately.
//!
//! \internal
//! \see To view the function definition, see ddi_power.c.
////////////////////////////////////////////////////////////////////////////////
void ddi_power_WarmRestart(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Configures what state the DCDCs will be in during a 5V connection
//!
//! \fntype Function
//!
//! Configures what state the DCDCs will be in during a 5V connection
//!
////////////////////////////////////////////////////////////////////////////////
void  ddi_power_LeaveDcdcEnabledDuring5v(bool state);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Requests what state the DCDCs will be in during a 5V connection
//!
//! \fntype Function
//!
//! Requests what state the DCDCs will be in during a 5V connection
//!
////////////////////////////////////////////////////////////////////////////////
bool  ddi_power_IsDcdcEnabledDuring5v(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Waits until the VDDD power supply is stable
//!
//! \fntype Function
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_WaitForVdddStable(void);
 
////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Applies floor and ceiling to target and brownout 
//!
//! \fntype Function
//!
//! \param[in] pu16Vddd_mV - proposed target voltage
//! \param[in] pu16Bo_mV - proposed brownout voltage
//!
//! \param[out] - pu16Vddd_mV - valid target voltage
//! \param[out] - pu16Bo_mV - valid brownout voltage
//!
//! \retval SUCCESS - no adjustments made to proposed voltages
//! \retval ERROR_DDI_POWER_VDDD_PARAM_ADJUSTED - adjustments made to one or both
//! \retval                                       proposed voltages
////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_power_LimitVdddAndBo(uint16_t *pu16Vddd_mV, uint16_t *pu16Bo_mV);
 
////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Waits until the VDDIO power supply is stable
//!
//! \fntype Function
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_WaitForVddioStable(void);
 
////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Applies floor and ceiling to target and brownout 
//!
//! \fntype Function
//!
//! \param[in] pu16Vddio_mV - proposed target voltage
//! \param[in] pu16Bo_mV - proposed brownout voltage
//!
//! \param[out] - pu16Vddio_mV - valid target voltage
//! \param[out] - pu16Bo_mV - valid brownout voltage
//!
//! \retval SUCCESS - no adjustments made to proposed voltages
//! \retval ERROR_DDI_POWER_VDDIO_PARAM_ADJUSTED - adjustments made to one or both
//! \retval                                       proposed voltages 
////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_power_LimitVddioAndBo(uint16_t *pu16Vddio_mV, uint16_t *pu16Bo_mV);
 
////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Waits until the VDDA power supply is stable
//!
//! \fntype Function
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_WaitForVddaStable(void);
 
////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Applies floor and ceiling to target and brownout 
//!
//! \fntype Function
//!
//! \param[in] pu16Vdda_mV - proposed target voltage
//! \param[in] pu16Bo_mV - proposed brownout voltage
//!
//! \param[out] - pu16Vdda_mV - valid target voltage
//! \param[out] - pu16Bo_mV - valid brownout voltage
//!
//! \retval SUCCESS - no adjustments made to proposed voltages
//! \retval ERROR_DDI_POWER_VDDA_PARAM_ADJUSTED - adjustments made to one or both
//! \retval                                       proposed voltages 
////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_power_LimitVddaAndBo(uint16_t *pu16Vdda_mV, uint16_t *pu16Bo_mV);
 
////////////////////////////////////////////////////////////////////////////////
//!  
//! Name: ddi_power_SetBiasCurrentSource  
//! 
//! \brief Select resistor source for bias current
//! 
//! \fntype Function                        
//!
//! \param[in] 	 eSource - DDI_POWER_INTERNAL_BIAS_CURRENT - use internal resistor to generate bias current 
//!                        DDI_POWER_EXTERNAL_BIAS_CURRENT - use external resistor to generate bias current
//! 
////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_power_SetBiasCurrentSource(ddi_power_BiasCurrentSource_t eSource);

////////////////////////////////////////////////////////////////////////////////
//!  
//! Name: ddi_power_GetBiasCurrentSource  
//! 
//! \brief Return resistor source to generate bias current
//!
//! \retval DDI_POWER_INTERNAL_BIAS_CURRENT - use internal resistor to generate bias current 
//! \retval DDI_POWER_EXTERNAL_BIAS_CURRENT - use external resistor to generate bias current
//!
////////////////////////////////////////////////////////////////////////////////
ddi_power_BiasCurrentSource_t ddi_power_GetBiasCurrentSource(void);

////////////////////////////////////////////////////////////////////////////////
//!  
//! Name: ddi_power_SetMaxBatteryChargeCurrent  
//! 
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
uint16_t ddi_power_SetMaxBatteryChargeCurrent(uint16_t u16MaxCur);

////////////////////////////////////////////////////////////////////////////////
//!  
//! Name: hw_power_GetMaxBatteryChargeCurrent  
//! 
//! \brief Returns maximum battery charge current 
//! 
//! \fntype Function                        
//!     
//! \retval Maximum battery charge current in mA
//! 
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetMaxBatteryChargeCurrent(void);

////////////////////////////////////////////////////////////////////////////////
//!  
//! Name: ddi_power_SetBatteryChargeCurrentThreshold  
//! 
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
//! \param[in] 	 u16Thresh - requested threshold current in mA. 
//!
//! \retval Actual threshold current 
//!                        
//! \notes None
//! 
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_SetBatteryChargeCurrentThreshold(uint16_t u16Thresh);
   
////////////////////////////////////////////////////////////////////////////////
//!  
//! Name: hw_power_GetBatteryChargeCurrentThreshold  
//! 
//! \brief Returns battery charge current threshold
//! 
//! \fntype Function                        
//!     
//! \retval Threshold current in mA 
//!                        
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetBatteryChargeCurrentThreshold(void);
 
////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Turn the charging hardware on or off.
//!
//! \fntype Function
//!
//! This function turns the charging hardware on or off.
//!
//! \param[in]  bPowerOn  Indicates whether the charging hardware should be on or off.
//!
//! Note that the bit we're looking at is named PWD_BATTCHRG. The "PWD"
//! stands for "power down". Thus, when the bit is set, the battery charger
//! hardware is POWERED DOWN.
////////////////////////////////////////////////////////////////////////////////
void ddi_power_SetChargerPowered(bool bPowerOn);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Report if the charger hardware power is on.
//!
//! \fntype Function
//!
//! This function reports if the charger hardware power is on.
//!
//! \retval  TRUE if the charger hardware is powered. FALSE otherwise.
//!
////////////////////////////////////////////////////////////////////////////////
bool ddi_power_GetChargerPowered(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Reports if the charging current has fallen below the threshold.
//!
//! \fntype Function
//!
//! This function reports if the charging current that the battery is accepting
//! has fallen below the threshold.
//!
//! Note that this bit is regarded by the hardware guys as very slightly
//! unreliable. They recommend that you don't believe a value of zero until
//! you've sampled it twice.
//!
//! \retval  Zero if the battery is accepting less current than indicated by the
//!          charging threshold. Non-zero otherwise.
//!
////////////////////////////////////////////////////////////////////////////////
int ddi_power_GetChargeStatus(void);
 
////////////////////////////////////////////////////////////////////////////////
//!  
//! \brief Increments the brownout offset without going above maximum 
//! 
//! \param[in] u16Offset - proposed brownout offset
//!
//! \retval Adjusted brownout offset
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_IncrementBrownoutOffset(uint16_t u16Offset);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Decrements the brownout offset without going below minimum 
//! 
//! \param[in] u16Offset - proposed brownout offset
//!
//! \retval Adjusted brownout offset
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_DecrementBrownoutOffset(uint16_t u16Offset);
 
////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_UpdateDcFuncvVddd  
//! 
//! \brief Updates the DCDC efficincy register related to VDDD
//! 
//! \fntype Function                        
//!     
//! Writes the present VDDD supply voltage to the HW_POWER_DCFUNCV register's
//! VDDD field.  This provides information used to optimize DCDC converter
//! performance 
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_UpdateDcFuncvVddd(void);
 
////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_UpdateDcFuncvVddio  
//! 
//! \brief Updates the DCDC efficincy register related to VDDIO
//! 
//! \fntype Function                        
//!     
//! Writes the present VDDIO supply voltage to the HW_POWER_DCFUNCV register's
//! VDDIO field.  This provides information used to optimize DCDC converter
//! performance
//! 
////////////////////////////////////////////////////////////////////////////////
void ddi_power_UpdateDcFuncvVddio(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns whether or not battery charging hardware is present
//! 
//! \fntype Function
//!
//! \retval TRUE if hardware is present, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
bool ddi_power_GetBatteryChargerEnabled(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns whether or not 5V is present
//! 
//! \fntype Function
//!
//! \retval TRUE if present, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
bool ddi_power_Get5vPresent(void);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Report on the die temperature.
//!
//! \fntype Function
//!
//! This function reports on the die temperature.
//!
//! \param[out]  pLow   The low  end of the temperature range.
//! \param[out]  pHigh  The high end of the temperature range.
////////////////////////////////////////////////////////////////////////////////
void  ddi_power_GetDieTemp(int16_t * pLow, int16_t * pHigh);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Converts proposed current to a valid current for the register setting
//! 
//! \fntype Function
//!
//!
//! \param[in] u16Current - Proposed current in mA
//!
//! \retval Valid current value
////////////////////////////////////////////////////////////////////////////////
uint16_t  ddi_power_ExpressibleCurrent(uint16_t u16Current);
 
////////////////////////////////////////////////////////////////////////////////
//! \brief Returns whether or not DCDC is on
//! 
//! \fntype Function
//!
//! \retval TRUE if on, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
bool ddi_power_IsDcdcOn(void);
 
////////////////////////////////////////////////////////////////////////////////
//!  
//! Name: ddi_power_GetBatteryMode  
//! 
//! \brief Returns the battery mode the chip is operating in. 
//! 
//! \fntype Function                        
//!     
//! \retval - DDI_POWER_BATT_MODE_LIION when LiIon (usually rechargable and non-removable)   
//! \retval - DDI_POWER_BATT_MODE_ALKALINE_NIMH when Alkaline or NiMH (usually AA or AAA and replacable)
//!
////////////////////////////////////////////////////////////////////////////////
ddi_power_BatteryMode_t ddi_power_GetBatteryMode(void);


////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Forces power supplies to draw power from DCDC (battery) instead
//! of the 5V connection.
//!
//! \fntype Function
//!
//! For suspend mode operation, the current draw on the 5V line must be minimzed.
//! We will switch the power rails to use the battery as the power source. Other
//! power optimizations may also occur.  
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_Suspend5Volts(void);

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Returns power rails to the normal 5V connection configuration.
//!
//! \fntype Function
//!
//! For suspend mode operation, the current draw on the 5V line must be minimzed.
//! This function returns the power supplies to their original configuration.
//! Power can be drawn from the LinReg or DCDC depending on the application's
//! decision.    
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_Unsuspend5Volts(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Checks if the 5V source is available for use.  
//!
//! This function returns the status of the 5V power source.  To be valid, 
//! the 5V source must be present, and the application must have validated
//! the 5V source as an available source for use.
////////////////////////////////////////////////////////////////////////////////
bool ddi_power_5vPowerSourceValid(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Saves the status of the 5V source.  
//!
//! This function allows the application to validate the 5V power source.
//! When valid, the source could be used to power the chip, but it does not
//! have to be used.
//!
//! \param[in] bValidSource New status for the 5V source.  true if it can be used
//! to power the chip, false if it cannot be used.  
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_Validate5vPowerSource(bool bValidSource);

////////////////////////////////////////////////////////////////////////////////
//! \brief Saves the force DCDC status.
//!
//! This function allows the caller to force the use of DCDC regardless of the
//! current power source.  When set, the chip will never use the linear 
//! regulators as a power source.
//!
//! \param[in] bForceDcdc Status of the force DCDC behavior;  true if it 
//! must be used to power the chip, false if it does not need to be used.
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_ForceDcdcPowerSource(bool bForceDcdc);

void ddi_power_SoftwareBatteryVoltageUpdate(int16_t iOffset);
void ddi_power_EnableSoftwareBatteryVoltageUpdate(bool bEnable, int16_t iOffset);
void ddi_power_Handoff(uint32_t input);

void ddi_power_SetBatteryBrownoutLevel (uint8_t u8Code);
#endif // _DDI_POWER_H

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
