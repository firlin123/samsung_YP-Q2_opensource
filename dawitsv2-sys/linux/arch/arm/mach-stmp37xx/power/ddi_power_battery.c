////////////////////////////////////////////////////////////////////////////////
//! \addtogroup ddi_power
//! @{
//
// Copyright(C) 2005 SigmaTel, Inc.
//
//! \file ddi_power_battery.c
//! \brief Implementation file for the power driver battery charger.
//!
////////////////////////////////////////////////////////////////////////////////
//   Includes and external references
////////////////////////////////////////////////////////////////////////////////
#include "ddi_power_common.h"

////////////////////////////////////////////////////////////////////////////////
// Globals & Variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Battery Charger Status                                                                        
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_GetBatteryMode
//!
//! \brief 
////////////////////////////////////////////////////////////////////////////////
ddi_power_BatteryMode_t ddi_power_GetBatteryMode(void)
{
    return (ddi_power_BatteryMode_t) hw_power_GetBatteryMode();
}

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_GetBatteryChargerEnabled
//!
//! \brief 
////////////////////////////////////////////////////////////////////////////////
bool ddi_power_GetBatteryChargerEnabled(void)
{
    return hw_power_GetBattChrgPresentFlag();
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Report if the charger hardware power is on.
//!
//! \fntype Function
//!
//! This function reports if the charger hardware power is on.
//!
//! \retval  Zero if the charger hardware is not powered. Non-zero otherwise.
//!
//! Note that the bit we're looking at is named PWD_BATTCHRG. The "PWD"
//! stands for "power down". Thus, when the bit is set, the battery charger
//! hardware is POWERED DOWN.
////////////////////////////////////////////////////////////////////////////////
bool ddi_power_GetChargerPowered(void)
{
    // Call into the hw layer.
    return !hw_power_GetPowerDownBatteryCharger();
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Turn the charging hardware on or off.
//!
//! \fntype Function
//!
//! This function turns the charging hardware on or off.
//!
//! \param[in]  on  Indicates whether the charging hardware should be on or off.
//!
//! Note that the bit we're looking at is named PWD_BATTCHRG. The "PWD"
//! stands for "power down". Thus, when the bit is set, the battery charger
//! hardware is POWERED DOWN.
////////////////////////////////////////////////////////////////////////////////
void ddi_power_SetChargerPowered(bool bPowerOn)
{
    // Hit the battery charge power switch.
    hw_power_SetPowerDownBatteryCharger(!bPowerOn);  
}

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
int  ddi_power_GetChargeStatus(void)
{
    return hw_power_GetBatteryChargingStatus();
}

////////////////////////////////////////////////////////////////////////////////
// Battery Voltage
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Report the voltage across the battery.
//!
//! \fntype Function
//!
//! This function reports the voltage across the battery.
//!
//! \retval The voltage across the battery, in mV.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t  ddi_power_GetBattery(void)
{
    // Should return a value in range ~3000 - 4200 mV
    return hw_power_GetBatteryVoltage();
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Report the voltage across the battery.
//!
//! \fntype Function
//!
//! This function reports the voltage across the battery.
//!
//! \retval The voltage across the battery, in mV.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t  ddi_power_GetBatteryBrownout(void)
{
    return hw_power_GetBatteryBrownoutValue();
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Set battery brownout level
//!
//! \fntype     Reentrant Function
//!
//! This function sets the battery brownout level in millivolt. It transforms the
//! input brownout value from millivolts to the hardware register bit field value
//! taking the ceiling value in the calculation.
//!
//! \param[in]  u16BattBrownout_mV      Battery battery brownout level in mV
//!
//! \return     SUCCESS
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_SetBatteryBrownout(uint16_t  u16BattBrownout_mV)
{
    return hw_power_SetBatteryBrownoutValue(u16BattBrownout_mV);
}

////////////////////////////////////////////////////////////////////////////////
// Currents
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_SetBiasCurrentSource
//!
//! \brief 
////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_power_SetBiasCurrentSource(ddi_power_BiasCurrentSource_t eSource)
{
    switch(eSource)
    {
        case DDI_POWER_INTERNAL_BIAS_CURRENT:
            hw_power_SetBiasCurrentSource(HW_POWER_INTERNAL_BIAS_CURRENT);
        break;

        case DDI_POWER_EXTERNAL_BIAS_CURRENT:
            hw_power_SetBiasCurrentSource(HW_POWER_EXTERNAL_BIAS_CURRENT);
        break;

        default:
            return ERROR_DDI_POWER_INVALID_PARAM;
    }
    
    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_GetBiasCurrentSource
//!
//! \brief 
////////////////////////////////////////////////////////////////////////////////
ddi_power_BiasCurrentSource_t ddi_power_GetBiasCurrentSource(void)
{
    return (ddi_power_BiasCurrentSource_t) hw_power_GetBiasCurrentSource();
}

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_SetMaxBatteryChargeCurrent
//!
//! \brief 
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_SetMaxBatteryChargeCurrent(uint16_t u16MaxCur)
{
    return hw_power_SetMaxBatteryChargeCurrent(u16MaxCur);
}

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_GetMaxBatteryChargeCurrent
//!
//! \brief 
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetMaxBatteryChargeCurrent(void)
{
    return hw_power_GetMaxBatteryChargeCurrent();
}

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_GetMaxChargeCurrent
//!
//! \brief 
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_SetBatteryChargeCurrentThreshold(uint16_t u16Thresh)
{
    return hw_power_SetBatteryChargeCurrentThreshold(u16Thresh);
}

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_GetBatteryChargeCurrentThreshold
//!
//! \brief 
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetBatteryChargeCurrentThreshold(void)
{
    return hw_power_GetBatteryChargeCurrentThreshold();
}



////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Manually update the battery voltage monitor with an offset
//!
//!
//! \fntype Function
//!
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_EnableSoftwareBatteryVoltageUpdate(bool bEnable, int16_t iOffset)
{
	// MOOJI : disabled for now, to check
#if 0
    if(bEnable)
    {   
        //--------------------------------------------------------------------------
        // We need to disable the automatic hardware updating first
        //--------------------------------------------------------------------------

        hw_lradc_DisableAutomaticBatteryUpdate();

        //--------------------------------------------------------------------------
        // Set up the timer that will periodically update the battery voltage
        //--------------------------------------------------------------------------

        // Set up timer.
    }
    else
    {
        //--------------------------------------------------------------------------
        // We are going back to DCDC mode.  We need the LRADC's calculated battery
        // voltage. 
        //--------------------------------------------------------------------------

        hw_lradc_EnableAutomaticBatteryUpdate();
    }
#endif
}


void ddi_power_SoftwareBatteryVoltageUpdate(int16_t iOffset)
{
    uint16_t Batt, NewBatt;
    
    //--------------------------------------------------------------------------
    // Read the true battery voltage and calculate the offseted voltage we are
    // going to write.
    //--------------------------------------------------------------------------
    
    Batt = ddi_power_GetBattery();
    NewBatt = Batt + iOffset;    


    //--------------------------------------------------------------------------
    // Make sure we keep a reasonable minimum.
    //--------------------------------------------------------------------------

    if(NewBatt < 1100)
    {
        NewBatt = 1100;
    }

    //--------------------------------------------------------------------------
    // Write the voltage.
    //--------------------------------------------------------------------------

    hw_power_SetBatteryMonitorVoltage(NewBatt);
}

////////////////////////////////////////////////////////////////////////////////
// Conversion
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Compute the actual current expressible in the hardware.
//!
//! \fntype Function
//!
//! Given a desired current, this function computes the actual current
//! expressible in the hardware.
//!
//! Note that the hardware has a minimum resolution of 10mA and a maximum
//! expressible value of 780mA (see the data sheet for details). If the given
//! current cannot be expressed exactly, then the largest expressible smaller
//! value will be used.
//!
//! \param[in]  u16Current  The current of interest.
//!
//! \retval  The corresponding current in mA.
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t  ddi_power_ExpressibleCurrent(uint16_t u16Current)
{
    // Return the result.
    return(hw_power_ConvertSettingToCurrent(hw_power_ConvertCurrentToSetting(u16Current)));

}

void ddi_power_SetBatteryBrownoutLevel (uint8_t u8Code)
{
    //--------------------------------------------------------------------------
    // Open the clock gate for the power supply registers. It might already be
    // open but, just in case...
    //--------------------------------------------------------------------------

    BF_CLR(POWER_CTRL, CLKGATE);

    //--------------------------------------------------------------------------
    // Disable the hardware from powering itself down on or interrupting the CPU
    // on a battery brown out. We don't want any of that to happen while we
    // change the voltage threshold.
    //--------------------------------------------------------------------------

    BF_CLR(POWER_BATTMONITOR, PWDN_BATTBRNOUT);
    BF_CLR(POWER_CTRL, ENIRQBATT_BO);

    //--------------------------------------------------------------------------
    // Clear any battery brown out interrupts we may have observed in the past.
    //--------------------------------------------------------------------------

    BF_CLR(POWER_CTRL, BATT_BO_IRQ);

    //--------------------------------------------------------------------------
    // Set the voltage level for a brownout.
    //--------------------------------------------------------------------------

    BF_WR(POWER_BATTMONITOR, BRWNOUT_LVL, u8Code);

    //--------------------------------------------------------------------------
    // Enable the hardware to power itself down on a battery brown out.
    //--------------------------------------------------------------------------

    BF_SET(POWER_BATTMONITOR, PWDN_BATTBRNOUT);
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
