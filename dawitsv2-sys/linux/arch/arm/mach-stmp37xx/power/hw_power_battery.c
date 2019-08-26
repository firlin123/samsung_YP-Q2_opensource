////////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_power
//! @{
//
// Copyright (c) 2004 - 2007 SigmaTel, Inc.
//
//! \file hw_power_battery.c
//! \brief Contains hardware API for power peripheral related to battery.
//! \version 0.1
//! \date 03/2007
//!
//! This file contains the hardware accessing functions for the on-chip
//! power peripheral.
//!
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes and external references
////////////////////////////////////////////////////////////////////////////////
#include "hw_power_common.h"

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_SetBatteryBrownoutValue(int16_t i16BatteryBrownout_mV)
{
    int16_t     i16BrownoutLevel;

    // Calculate battery brownout level
    switch (hw_power_GetBatteryMode())
    {
        //case HW_POWER_BATT_MODE_LIION_SINGLE_CONVERTOR:
        case HW_POWER_BATT_MODE_0:
            i16BrownoutLevel  = i16BatteryBrownout_mV - BATT_BRWNOUT_LIION_EQN_CONST;
            i16BrownoutLevel /= BATT_BRWNOUT_LIION_LEVEL_STEP_MV;
            break;

        //case HW_POWER_BATT_MODE_SINGLE_ALKALINE_NIMH:
        case HW_POWER_BATT_MODE_1:
            i16BrownoutLevel  = i16BatteryBrownout_mV - BATT_BRWNOUT_ALKAL_EQN_CONST;
            i16BrownoutLevel /= BATT_BRWNOUT_ALKAL_LEVEL_STEP_MV;
            break;
        default:
            return ERROR_HW_POWER_INVALID_BATT_MODE;
    }

    // Do a check to make sure nothing went wrong.
    if (i16BrownoutLevel <= 0x0f)
    {
        //Write the battery brownout level
        BF_WR(POWER_BATTMONITOR, BRWNOUT_LVL, i16BrownoutLevel);
    }
    else
    {
       return ERROR_HW_POWER_INVALID_INPUT_PARAM;
    }
    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetBatteryBrownoutValue(void)
{
    uint16_t    u16BatteryBrownoutLevel;

    // Get battery brownout level
    u16BatteryBrownoutLevel = HW_POWER_BATTMONITOR.B.BRWNOUT_LVL;

    // Calculate battery brownout level
    switch (hw_power_GetBatteryMode())
    {
        case HW_POWER_BATT_MODE_0:
        //case HW_POWER_BATT_MODE_LIION_DUAL_CONVERTOR:
            u16BatteryBrownoutLevel *= BATT_BRWNOUT_LIION_LEVEL_STEP_MV;
            u16BatteryBrownoutLevel += BATT_BRWNOUT_LIION_BASE_MV;
            break;
        case HW_POWER_BATT_MODE_1:
        //case HW_POWER_BATT_MODE_SINGLE_ALKALINE_NIMH:
            u16BatteryBrownoutLevel *= BATT_BRWNOUT_ALKAL_LEVEL_STEP_MV;
            u16BatteryBrownoutLevel += BATT_BRWNOUT_ALKAL_BASE_MV;
            break;
        default:
            u16BatteryBrownoutLevel = 0;
            break;
    }
    return u16BatteryBrownoutLevel;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetBatteryVoltage(void)
{
    uint16_t    u16BattVolt;

    // Get the raw result of battery measurement
    u16BattVolt = HW_POWER_BATTMONITOR.B.BATT_VAL;

    // Adjust for 8-mV LSB resolution and return
    return (u16BattVolt * BATT_VOLTAGE_8_MV);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetBatteryMonitorVoltage(uint16_t u16BattVolt)
{
    uint16_t u16BattValue;

    // Adjust for 8-mV LSB resolution
    u16BattValue = u16BattVolt/BATT_VOLTAGE_8_MV;

    // Write to register
    BF_WR(POWER_BATTMONITOR, BATT_VAL, u16BattValue);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_SetMaxBatteryChargeCurrent(uint16_t u16Current)
{
    uint16_t   u16OldSetting;
    uint16_t   u16NewSetting;
    uint16_t   u16ToggleMask;

    // Get the old setting.
    u16OldSetting = BF_RD(POWER_CHARGE, BATTCHRG_I);

    // Convert the new threshold into a setting.
    u16NewSetting = hw_power_ConvertCurrentToSetting(u16Current);

    // Compute the toggle mask.
    u16ToggleMask = u16OldSetting ^ u16NewSetting;

    // Write to the toggle register.
    HW_POWER_CHARGE_TOG(u16ToggleMask << BP_POWER_CHARGE_BATTCHRG_I);

    // Tell the caller what current we're set at now.
    return(hw_power_ConvertSettingToCurrent(u16NewSetting));
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t  hw_power_SetBatteryChargeCurrentThreshold(uint16_t u16Threshold)
{
    uint16_t   u16OldSetting;
    uint16_t   u16NewSetting;
    uint16_t   u16ToggleMask;

    //--------------------------------------------------------------------------
    // See hw_power_SetMaxBatteryChargeCurrent for an explanation of why we're
    // using the toggle register here.
    //
    // Since this function doesn't have any major hardware effect, we could use
    // the usual macros for writing to this bit field. But, for the sake of
    // parallel construction and any potentially odd effects on the status bit,
    // we use the toggle register in the same way as ddi_bc_hwSetMaxCurrent.
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // The threshold hardware can't express as large a range as the max
    // current setting, but we can use the same functions as long as we add an
    // extra check here.
    //
    // Thresholds larger than 180mA can't be expressed.
    //--------------------------------------------------------------------------

    if (u16Threshold > 180) u16Threshold = 180;

    ////////////////////////////////////////
    // Create the mask
    ////////////////////////////////////////

    // Get the old setting.
    u16OldSetting = BF_RD(POWER_CHARGE, STOP_ILIMIT);

    // Convert the new threshold into a setting.
    u16NewSetting = hw_power_ConvertCurrentToSetting(u16Threshold);

    // Compute the toggle mask.
    u16ToggleMask = u16OldSetting ^ u16NewSetting;

    // Shift to the correct bit position
    u16ToggleMask = BF_POWER_CHARGE_STOP_ILIMIT(u16ToggleMask);


    /////////////////////////////////////////
    // Write to the register
    /////////////////////////////////////////

    // Write to the toggle register.
    HW_POWER_CHARGE_TOG(u16ToggleMask);

    // Tell the caller what current we're set at now.
    return(hw_power_ConvertSettingToCurrent(u16NewSetting));
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetBatteryChargeCurrentThreshold(void)
{
    uint16_t u16Threshold;

    u16Threshold = BF_RD(POWER_CHARGE, STOP_ILIMIT);

    return hw_power_ConvertSettingToCurrent(u16Threshold);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetMaxBatteryChargeCurrent(void)
{
    uint8_t u8Bits;

    // Get the raw data from register
    u8Bits = BF_RD(POWER_CHARGE, BATTCHRG_I);

    // Translate raw data to current (in mA) and return it
    return hw_power_ConvertSettingToCurrent(u8Bits);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_GetDieTemperature(int16_t * pi16Low, int16_t * pi16High)
{
#if 0
    int16_t i16High, i16Low;
    uint16_t u16Reading;

    // Temperature constant
    #define TEMP_READING_ERROR_MARGIN 5
    #define KELVIN_TO_CELSIUS_CONST 273

    // Get the reading in Kelvins
    u16Reading = hw_lradc_MeasureInternalDieTemperature(DIE_TEMP_CHAN_0, DIE_TEMP_CHAN_1);

    // Adjust for error margin
    i16High = u16Reading + TEMP_READING_ERROR_MARGIN;
    i16Low  = u16Reading - TEMP_READING_ERROR_MARGIN;

    // Convert to Celsius
    i16High -= KELVIN_TO_CELSIUS_CONST;
    i16Low  -= KELVIN_TO_CELSIUS_CONST;

    // Return the results
    *pi16High = i16High;
    *pi16Low  = i16Low;
#endif	
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}


