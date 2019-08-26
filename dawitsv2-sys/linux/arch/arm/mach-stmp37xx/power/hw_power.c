////////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_power
//! @{
//
// Copyright (c) 2004 - 2007 SigmaTel, Inc.
//
//! \file hw_power.c
//! \brief Contains hardware API for the 37xx power peripheral.
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
// VDDD
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVdddValue(uint16_t u16Vddd_mV)
{
    uint16_t u16Vddd_Set;

    // Convert mV to register setting
    u16Vddd_Set = hw_power_ConvertVdddToSetting(u16Vddd_mV);

    // Use this way of writing to the register because we want a read, modify,
    // write operation.  Using the other available macros will call a clear/set
    // which momentarily clears the target field causing the voltage to dip
    // then rise again.  That should be avoided.
    HW_POWER_VDDDCTRL.B.TRG = u16Vddd_Set;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVdddValue(void)
{
    uint16_t    u16Vddd_Set;
    uint16_t    u16Vddd_mV;

    //Read VDDD bitfiled value
    u16Vddd_Set = HW_POWER_VDDDCTRL.B.TRG;

    //  Convert to mVolts
    u16Vddd_mV = hw_power_ConvertSettingToVddd(u16Vddd_Set);

    return u16Vddd_mV;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVdddBrownoutValue(uint16_t u16VdddBoOffset_mV)
{
    uint16_t u16VdddBoOffset_Set;

    // Convert millivolts to register setting (1 step = 25mV)
    u16VdddBoOffset_Set = u16VdddBoOffset_mV/(VOLTAGE_STEP_MV);

    // Use this way of writing to the register because we want a read, modify,
    // write operation.  Using the other available macros will call a clear/set
    // which momentarily makes the brownout offset zero which may trigger a
    // false brownout.
    HW_POWER_VDDDCTRL.B.BO_OFFSET = u16VdddBoOffset_Set;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVdddBrownoutValue(void)
{
    uint16_t u16VdddBoOffset_Set;
    uint16_t u16VdddBoOffset_mV;

    // Read the VDDD brownout offset field.
    u16VdddBoOffset_Set = HW_POWER_VDDDCTRL.B.BO_OFFSET;

    // Convert setting to millivolts. (1 step = 25mV)
    u16VdddBoOffset_mV = (u16VdddBoOffset_Set * VOLTAGE_STEP_MV);

    // Return the brownout offset in mV.
    return u16VdddBoOffset_mV;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_SetVdddPowerSource(hw_power_PowerSource_t eSource)
{
    RtStatus_t rtn = SUCCESS;

    //--------------------------------------------------------------------------
    // The VDDD can use two power sources in three configurations: the Linear
    // Regulator, the DCDC with LinReg off, and the DCDC with LinReg on.
    // Each has its own configuration that must be set up to
    // prevent power rail instability.
    //--------------------------------------------------------------------------

    switch(eSource)
    {
        //----------------------------------------------------------------------
        // Power the VDDD rail from DCDC.  LinReg is off.
        //----------------------------------------------------------------------
        case HW_POWER_DCDC_LINREG_OFF:
        case HW_POWER_DCDC_LINREG_READY:

            //------------------------------------------------------------------
            // Use LinReg offset for DCDC mode.
            //------------------------------------------------------------------
            hw_power_SetVdddLinRegOffset(HW_POWER_LINREG_OFFSET_DCDC_MODE);


            //------------------------------------------------------------------
            // Turn on the VDDD DCDC output and turn off the VDDD LinReg output.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDDCTRL, DISABLE_FET);
            BF_CLR(POWER_VDDDCTRL, ENABLE_LINREG);


            //------------------------------------------------------------------
            // Make sure stepping is enabled when using DCDC.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDDCTRL, DISABLE_STEPPING);


        break;


        //----------------------------------------------------------------------
        // Power the VDDD rail from DCDC.  LinReg is on.
        //----------------------------------------------------------------------
        case HW_POWER_DCDC_LINREG_ON:

            //------------------------------------------------------------------
            // Use LinReg offset for DCDC mode.
            //------------------------------------------------------------------
            hw_power_SetVdddLinRegOffset(HW_POWER_LINREG_OFFSET_DCDC_MODE);


            //------------------------------------------------------------------
            // Turn on the VDDD DCDC output and turn on the VDDD LinReg output.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDDCTRL, DISABLE_FET);
            BF_SET(POWER_VDDDCTRL, ENABLE_LINREG);


            //------------------------------------------------------------------
            // Make sure stepping is enabled when using DCDC.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDDCTRL, DISABLE_STEPPING);


        break;


        //----------------------------------------------------------------------
        // Power the VDDD rail from the linear regulator.  The DCDC is not
        // set up to automatically power the chip if 5V is removed.  Use this
        // configuration when battery is powering the player, but we want to
        // use LinReg instead of DCDC.
        //----------------------------------------------------------------------
        case HW_POWER_LINREG_DCDC_OFF:

            //------------------------------------------------------------------
            // Use LinReg offset for LinReg mode.
            //------------------------------------------------------------------
            hw_power_SetVdddLinRegOffset(HW_POWER_LINREG_OFFSET_LINREG_MODE);


            //------------------------------------------------------------------
            // Turn on the VDDD LinReg and turn off the VDDD DCDC output.
            //------------------------------------------------------------------
            BF_SET(POWER_VDDDCTRL, ENABLE_LINREG);
            BF_SET(POWER_VDDDCTRL, DISABLE_FET);


            //------------------------------------------------------------------
            // Make sure stepping is disabled when using linear regulators.
            //------------------------------------------------------------------
            BF_SET(POWER_VDDDCTRL, DISABLE_STEPPING);


        break;


		//----------------------------------------------------------------------
        // Power the VDDD rail from the linear regulator.  The DCDC is ready
        // to automatically power the chip when 5V is removed.  Use this
        // configuration when powering from 5V.
        //----------------------------------------------------------------------
        case HW_POWER_LINREG_DCDC_READY:

            //------------------------------------------------------------------
            // Use LinReg offset for LinReg mode.
            //------------------------------------------------------------------
            hw_power_SetVdddLinRegOffset(HW_POWER_LINREG_OFFSET_DCDC_MODE);


            //------------------------------------------------------------------
            // Turn on the VDDD LinReg and turn on the VDDD DCDC output.  The
            // ENABLE_DCDC must be cleared to avoid LinReg and DCDC conflict.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDDCTRL, ENABLE_LINREG);
            BF_CLR(POWER_VDDDCTRL, DISABLE_FET);


            //------------------------------------------------------------------
            // Make sure stepping is disabled when using linear regulators.
            //------------------------------------------------------------------
            BF_SET(POWER_VDDDCTRL, DISABLE_STEPPING);


        break;


        //----------------------------------------------------------------------
        // Invalid power source option.
        //----------------------------------------------------------------------
        default:
            rtn = ERROR_HW_POWER_INVALID_INPUT_PARAM;

        break;

    }

    return rtn;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
hw_power_PowerSource_t hw_power_GetVdddPowerSource(void)
{

    //--------------------------------------------------------------------------
    // If the Vddd DCDC Converter output is disabled, LinReg must be powering Vddd.
    //--------------------------------------------------------------------------

    if(HW_POWER_VDDDCTRL.B.DISABLE_FET)
    {

        //----------------------------------------------------------------------
        // Make sure the LinReg offset is correct for this source.
        //----------------------------------------------------------------------

        if(hw_power_GetVdddLinRegOffset() == HW_POWER_LINREG_OFFSET_LINREG_MODE)
        {
            return HW_POWER_LINREG_DCDC_OFF;
        }

    }


    //--------------------------------------------------------------------------
    // If here, DCDC must be powering Vddd.  Determine if the LinReg is also on.
    //--------------------------------------------------------------------------

    if(HW_POWER_VDDDCTRL.B.ENABLE_LINREG)
    {

        //----------------------------------------------------------------------
        // The LinReg offset must be below the target if DCDC and LinRegs' are
        // on at the same time.  Otherwise, we have an invalid configuration.
        //----------------------------------------------------------------------

        if(hw_power_GetVdddLinRegOffset() == HW_POWER_LINREG_OFFSET_DCDC_MODE)
        {
            return HW_POWER_DCDC_LINREG_ON;
        }

    }

    else
    {
        //--------------------------------------------------------------------------
        // Is the LinReg offset set to power Vddd from linreg?
        //--------------------------------------------------------------------------

        if(hw_power_GetVdddLinRegOffset() == HW_POWER_LINREG_OFFSET_DCDC_MODE)
        {
            return HW_POWER_DCDC_LINREG_OFF;
        }

    }


    //--------------------------------------------------------------------------
    // If we get here, the power source is in an unknown configuration.  Most
    // likely, the LinReg offset is incorrect for the given power supply.
    //--------------------------------------------------------------------------
    return HW_POWER_UNKNOWN_SOURCE;
}

////////////////////////////////////////////////////////////////////////////////
// VDDIO
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddioValue(uint16_t u16Vddio_mV)
{
    uint16_t u16Vddio_Set;

    // Convert mV to register setting
    u16Vddio_Set = hw_power_ConvertVddioToSetting(u16Vddio_mV);

    // Use this way of writing to the register because we want a read, modify,
    // write operation.  Using the other available macros will call a clear/set
    // which momentarily clears the target field causing the voltage to dip
    // then rise again.  That should be avoided.
    HW_POWER_VDDIOCTRL.B.TRG = u16Vddio_Set;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVddioValue(void)
{
    uint16_t    u16Vddio_Set;
    uint16_t    u16Vddio_mV;

    //Read VDDIO bitfiled value
    u16Vddio_Set = HW_POWER_VDDIOCTRL.B.TRG;

    //  Convert to mVolts
    u16Vddio_mV = hw_power_ConvertSettingToVddio(u16Vddio_Set);

    return u16Vddio_mV;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddioBrownoutValue(uint16_t u16VddioBoOffset_mV)
{
    uint16_t u16VddioBoOffset_Set;

    // Convert millivolts to register setting (1 step = 25mV)
    u16VddioBoOffset_Set = u16VddioBoOffset_mV/(VOLTAGE_STEP_MV);

    // Use this way of writing to the register because we want a read, modify,
    // write operation.  Using the other available macros will call a clear/set
    // which momentarily makes the brownout offset zero which may trigger a
    // false brownout.
    HW_POWER_VDDIOCTRL.B.BO_OFFSET = u16VddioBoOffset_Set;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVddioBrownoutValue(void)
{
    uint16_t u16VddioBoOffset_Set;
    uint16_t u16VddioBoOffset_mV;

    // Read the VDDIO brownout offset field.
    u16VddioBoOffset_Set = HW_POWER_VDDIOCTRL.B.BO_OFFSET;

    // Convert setting to millivolts. (1 step = 25mV)
    u16VddioBoOffset_mV = (u16VddioBoOffset_Set * VOLTAGE_STEP_MV);

    // Return the brownout offset in mV.
    return u16VddioBoOffset_mV;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_SetVddioPowerSource(hw_power_PowerSource_t eSource)
{
    RtStatus_t rtn = SUCCESS;

    //--------------------------------------------------------------------------
    // The VDDIO can use two power sources in three configurations: the Linear
    // Regulator, the DCDC with LinReg off, and the DCDC with LinReg on.
    // Each has its own configuration that must be set up to prevent power
    // rail instability. VDDIO will only use the linear regulator when
    // 5V is present.
    //--------------------------------------------------------------------------


    switch(eSource)
    {

        //----------------------------------------------------------------------
        // Power the VDDIO rail from DCDC with LinReg off.
        //----------------------------------------------------------------------
        case HW_POWER_DCDC_LINREG_OFF:

            //------------------------------------------------------------------
            // Use LinReg offset for DCDC mode.
            //------------------------------------------------------------------
            hw_power_SetVddioLinRegOffset(HW_POWER_LINREG_OFFSET_DCDC_MODE);


            //------------------------------------------------------------------
            // Turn on the VDDIO DCDC output and turn off the LinReg output.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDIOCTRL, DISABLE_FET);
            hw_power_DisableVddioLinearRegulator(TRUE);


            //------------------------------------------------------------------
            // Make sure stepping is enabled when using DCDC.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDIOCTRL, DISABLE_STEPPING);


        break;


        //----------------------------------------------------------------------
        // Power the VDDIO rail from DCDC with LinReg on.
        //----------------------------------------------------------------------
        case HW_POWER_DCDC_LINREG_READY:
        case HW_POWER_DCDC_LINREG_ON:


            //------------------------------------------------------------------
            // Use LinReg offset for DCDC mode.
            //------------------------------------------------------------------
            hw_power_SetVddioLinRegOffset(HW_POWER_LINREG_OFFSET_DCDC_MODE);


            //------------------------------------------------------------------
            // Turn on the VDDIO DCDC output and turn on the LinReg output.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDIOCTRL, DISABLE_FET);
            hw_power_DisableVddioLinearRegulator(FALSE);


            //------------------------------------------------------------------
            // Make sure stepping is enabled when using DCDC.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDIOCTRL, DISABLE_STEPPING);


        break;


        //----------------------------------------------------------------------
        // Power the VDDIO rail from the linear regulator.
        // Assumes 5V is present.
        //----------------------------------------------------------------------
        case HW_POWER_LINREG_DCDC_OFF:

            //------------------------------------------------------------------
            // Use LinReg offset for LinReg mode.
            //------------------------------------------------------------------
            hw_power_SetVddioLinRegOffset(HW_POWER_LINREG_OFFSET_LINREG_MODE);


            //------------------------------------------------------------------
            // Turn on the VDDIO LinReg output and turn off the VDDIO DCDC output.
            //------------------------------------------------------------------
            hw_power_DisableVddioLinearRegulator(FALSE);
            BF_SET(POWER_VDDIOCTRL, DISABLE_FET);


            //------------------------------------------------------------------
            // Make sure stepping is disabled when using linear regulators.
            //------------------------------------------------------------------
            BF_SET(POWER_VDDIOCTRL, DISABLE_STEPPING);


        break;


        //----------------------------------------------------------------------
        // Power the VDDIO rail from the linear regulator.
        // Assumes 5V is present.  The DCDC is ready to take over when 5V
        // is removed.
        //----------------------------------------------------------------------
        case HW_POWER_LINREG_DCDC_READY:


            //------------------------------------------------------------------
            // Use LinReg offset for LinReg mode.
            //------------------------------------------------------------------
            hw_power_SetVddioLinRegOffset(HW_POWER_LINREG_OFFSET_DCDC_MODE);


            //------------------------------------------------------------------
            // Turn on the VDDIO LinReg output and prepare the VDDIO DCDC output.
            // ENABLE_DCDC must be cleared to prevent DCDC and LinReg conflict.
            //------------------------------------------------------------------
            hw_power_DisableVddioLinearRegulator(FALSE);
            BF_CLR(POWER_VDDIOCTRL, DISABLE_FET);


            //------------------------------------------------------------------
            // Make sure stepping is disabled when using linear regulators.
            //------------------------------------------------------------------
            BF_SET(POWER_VDDIOCTRL, DISABLE_STEPPING);


        break;


        //----------------------------------------------------------------------
        // Invalid power source option.
        //----------------------------------------------------------------------
        default:
            rtn = ERROR_HW_POWER_INVALID_INPUT_PARAM;
        break;
    }

    return rtn;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
hw_power_PowerSource_t hw_power_GetVddioPowerSource(void)
{

    //--------------------------------------------------------------------------
    // 5 Volts must be present to use VDDIO LinReg
    //--------------------------------------------------------------------------

    if(hw_power_Get5vPresentFlag())
    {
        //----------------------------------------------------------------------
        // Check VDDIO's DCDC converter output.
        //----------------------------------------------------------------------

        if(HW_POWER_VDDIOCTRL.B.DISABLE_FET)
        {

            //------------------------------------------------------------------
            // DCDC output is off so we must be using LinReg.  Check that the
            // offset if correct.
            //------------------------------------------------------------------

            if(hw_power_GetVddioLinRegOffset() == HW_POWER_LINREG_OFFSET_LINREG_MODE)
            {
                return HW_POWER_LINREG_DCDC_OFF;
            }

        }


        //----------------------------------------------------------------------
        // We have 5V and DCDC converter output enabled.  Now check if the
        // DCDC is enabled.
        //----------------------------------------------------------------------

        if(hw_power_GetEnableDcdc())
        {

            //------------------------------------------------------------------
            // DCDC is enabled.  We must be powered from DCDC with LinReg on.
            // Check the offset.
            //------------------------------------------------------------------

            if(hw_power_GetVdddLinRegOffset() == HW_POWER_LINREG_OFFSET_DCDC_MODE)
            {
                return HW_POWER_DCDC_LINREG_ON;
            }

        }

        else
        {

            //------------------------------------------------------------------
            // DCDC is not enabled.  We must be powered from LinReg.
            //------------------------------------------------------------------

            if(hw_power_GetVddioLinRegOffset() == HW_POWER_LINREG_OFFSET_LINREG_MODE)
            {
                return HW_POWER_LINREG_DCDC_OFF;
            }
        }
    }

    else
    {
        //--------------------------------------------------------------------------
        // VDDIO must be powered from DCDC with LinReg off.  Check the offset.
        //--------------------------------------------------------------------------

            if(hw_power_GetVdddLinRegOffset() == HW_POWER_LINREG_OFFSET_DCDC_MODE)
            {
                return HW_POWER_DCDC_LINREG_ON;
            }
    }


    //--------------------------------------------------------------------------
    // If we get here, the power source is in an unknown configuration.  Most
    // likely, the LinReg offset is incorrect for the given power supply.
    //--------------------------------------------------------------------------
    return HW_POWER_UNKNOWN_SOURCE;

}

////////////////////////////////////////////////////////////////////////////////
// VDDA
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddaValue(uint16_t u16Vdda_mV)
{
    uint16_t u16Vdda_Set;

    // Convert mV to register setting
    u16Vdda_Set = hw_power_ConvertVddaToSetting(u16Vdda_mV);

    // Use this way of writing to the register because we want a read, modify,
    // write operation.  Using the other available macros will call a clear/set
    // which momentarily clears the target field causing the voltage to dip
    // then rise again.  That should be avoided.
    HW_POWER_VDDACTRL.B.TRG = u16Vdda_Set;
}
EXPORT_SYMBOL(hw_power_SetVddaValue);

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVddaValue(void)
{
    uint16_t    u16Vdda_Set;
    uint16_t    u16Vdda_mV;

    //Read VDDA bitfiled value
    u16Vdda_Set = HW_POWER_VDDACTRL.B.TRG;

    //  Convert to mVolts
    u16Vdda_mV = hw_power_ConvertSettingToVdda(u16Vdda_Set);

    return u16Vdda_mV;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddaBrownoutValue(uint16_t u16VddaBoOffset_mV)
{
    uint16_t u16VddaBoOffset_Set;

    // Convert millivolts to register setting (1 step = 25mV)
    u16VddaBoOffset_Set = u16VddaBoOffset_mV/(VOLTAGE_STEP_MV);

    // Use this way of writing to the register because we want a read, modify,
    // write operation.  Using the other available macros will call a clear/set
    // which momentarily makes the brownout offset zero which may trigger a
    // false brownout.
    HW_POWER_VDDACTRL.B.BO_OFFSET = u16VddaBoOffset_Set;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
uint16_t hw_power_GetVddaBrownoutValue(void)
{
    uint16_t u16VddaBoOffset_Set;
    uint16_t u16VddaBoOffset_mV;

    // Read the VDDA brownout offset field.
    u16VddaBoOffset_Set = HW_POWER_VDDACTRL.B.BO_OFFSET;

    // Convert setting to millivolts. (1 step = 25mV)
    u16VddaBoOffset_mV = (u16VddaBoOffset_Set * VOLTAGE_STEP_MV);

    // Return the brownout offset in mV.
    return u16VddaBoOffset_mV;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_SetVddaPowerSource(hw_power_PowerSource_t eSource)
{
    RtStatus_t rtn=SUCCESS;

    //--------------------------------------------------------------------------
    // The VDDA can use two power sources: the DCDC, or the Linear Regulator.
    // Each has its own configuration that must be set up to prevent power
    // rail instability.
    //--------------------------------------------------------------------------
    switch(eSource)
    {

        //----------------------------------------------------------------------
        // Power the VDDA supply from DCDC with VDDA LinReg off.
        //----------------------------------------------------------------------
        case HW_POWER_DCDC_LINREG_OFF:
        case HW_POWER_DCDC_LINREG_READY:


            //----------------------------------------------------------------------
            // Use LinReg offset for DCDC mode.
            //----------------------------------------------------------------------
            hw_power_SetVddaLinRegOffset(HW_POWER_LINREG_OFFSET_DCDC_MODE);


            //----------------------------------------------------------------------
            // Turn on the VDDA DCDC converter output and turn off the LinReg output.
            //----------------------------------------------------------------------
            BF_CLR(POWER_VDDACTRL, DISABLE_FET);
            BF_CLR(POWER_VDDACTRL, ENABLE_LINREG);


            //------------------------------------------------------------------
            // Make sure stepping is enabled when using DCDC.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDACTRL, DISABLE_STEPPING);

        break;


        //----------------------------------------------------------------------
        // Power the VDDA supply from DCDC with VDDA LinReg off.
        //----------------------------------------------------------------------
        case HW_POWER_DCDC_LINREG_ON:


            //----------------------------------------------------------------------
            // Use LinReg offset for DCDC mode.
            //----------------------------------------------------------------------
            hw_power_SetVddaLinRegOffset(HW_POWER_LINREG_OFFSET_DCDC_MODE);


            //----------------------------------------------------------------------
            // Turn on the VDDA DCDC converter output and turn on the LinReg output.
            //----------------------------------------------------------------------
            BF_CLR(POWER_VDDACTRL, DISABLE_FET);
            BF_SET(POWER_VDDACTRL, ENABLE_LINREG);


            //------------------------------------------------------------------
            // Make sure stepping is enabled when using DCDC.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDACTRL, DISABLE_STEPPING);

        break;


        //----------------------------------------------------------------------
        // Power the VDDA supply from the linear regulator.  The DCDC output is
        // off and not ready to power the rail if 5V is removed.
        //----------------------------------------------------------------------
        case HW_POWER_LINREG_DCDC_OFF:


            //------------------------------------------------------------------
            // Use LinReg offset for LinReg mode.
            //------------------------------------------------------------------
            hw_power_SetVddaLinRegOffset(HW_POWER_LINREG_OFFSET_LINREG_MODE);


            //------------------------------------------------------------------
            // Turn on the VDDA LinReg output and turn off the VDDA DCDC output.
            //------------------------------------------------------------------
            BF_SET(POWER_VDDACTRL, ENABLE_LINREG);
            BF_SET(POWER_VDDACTRL, DISABLE_FET);


            //------------------------------------------------------------------
            // Make sure stepping is disabled when using linear regulators.
            //------------------------------------------------------------------
            BF_SET(POWER_VDDACTRL, DISABLE_STEPPING);

        break;


        //----------------------------------------------------------------------
        // Power the VDDA supply from the linear regulator.  The DCDC output is
        // ready to power the rail if 5V is removed.
        //----------------------------------------------------------------------
        case HW_POWER_LINREG_DCDC_READY:

            //------------------------------------------------------------------
            // Use LinReg offset for LinReg mode.
            //------------------------------------------------------------------
            hw_power_SetVddaLinRegOffset(HW_POWER_LINREG_OFFSET_DCDC_MODE);


            //------------------------------------------------------------------
            // Turn on the VDDA LinReg output and prepare the DCDC for transfer.
            // ENABLE_DCDC must be clear to avoid DCDC and LinReg conflict.
            //------------------------------------------------------------------
            BF_CLR(POWER_VDDACTRL, ENABLE_LINREG);
            BF_CLR(POWER_VDDACTRL, DISABLE_FET);


            //------------------------------------------------------------------
            // Make sure stepping is disabled when using linear regulators.
            //------------------------------------------------------------------
            BF_SET(POWER_VDDACTRL, DISABLE_STEPPING);

        break;


        //----------------------------------------------------------------------
        // Invalid power source option.
        //----------------------------------------------------------------------
        default:
            rtn = ERROR_HW_POWER_INVALID_INPUT_PARAM;

        break;

    }

    return rtn;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
hw_power_PowerSource_t hw_power_GetVddaPowerSource(void)
{
    //--------------------------------------------------------------------------
    // If the DCDC converter output is not enabled, we must be using LinReg.
    //--------------------------------------------------------------------------
    if(HW_POWER_VDDACTRL.B.DISABLE_FET)
    {

        //----------------------------------------------------------------------
        // Make sure the LinReg offset is correct for this source.
        //----------------------------------------------------------------------
        if(hw_power_GetVddaLinRegOffset() == HW_POWER_LINREG_OFFSET_LINREG_MODE)
        {
            return HW_POWER_LINREG_DCDC_OFF;
        }
    }


    //--------------------------------------------------------------------------
    // If here, DCDC must be powering Vdda.  Determine if the LinReg is also on.
    //--------------------------------------------------------------------------
    if(HW_POWER_VDDACTRL.B.ENABLE_LINREG)
    {

        //----------------------------------------------------------------------
        // The LinReg offset must be below the target if DCDC and LinRegs' are
        // on at the same time.  Otherwise, we have an invalid configuration.
        //----------------------------------------------------------------------
        if(hw_power_GetVddaLinRegOffset() == HW_POWER_LINREG_OFFSET_DCDC_MODE)
        {
            return HW_POWER_DCDC_LINREG_ON;
        }

    }

    else
    {
        //--------------------------------------------------------------------------
        // Is the LinReg offset set to power Vdda from linreg?
        //--------------------------------------------------------------------------
        if(hw_power_GetVddaLinRegOffset() == HW_POWER_LINREG_OFFSET_DCDC_MODE)
        {
            return HW_POWER_DCDC_LINREG_OFF;
        }

    }

    //--------------------------------------------------------------------------
    // If we get here, the power source is in an unknown configuration.  Most
    // likely, the LinReg offset is incorrect for the given power supply.
    //--------------------------------------------------------------------------
    return HW_POWER_UNKNOWN_SOURCE;

}

////////////////////////////////////////////////////////////////////////////////
// DCDC Efficiency
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_UpdateDcFuncvVddd(void)
{
    uint32_t u32Numerator, u32Denominator, u32Result;
    uint32_t u32Vddd, u32Vdda;

    //--------------------------------------------------------------------------
    // Get voltage values in mV for the rails used in the calculation.
    //--------------------------------------------------------------------------
    u32Vddd = hw_power_GetVdddValue();
    u32Vdda = hw_power_GetVddaValue();


    //--------------------------------------------------------------------------
    // Use the battery mode specific calculations.
    //--------------------------------------------------------------------------
    switch(hw_power_GetBatteryMode())
    {

        //----------------------------------------------------------------------
        // For Li-Ion Value = (VDDA-VDDD)/(6.25e-3).
        // Equivalent equation we'll use: (VDDA-VDDD)*160
        // VDDA and VDDD are in volts in the eqn, but we will
        // calculate it using mV so we don't mess with fractions.
        //----------------------------------------------------------------------
        case HW_POWER_BATT_MODE_LIION:

            //------------------------------------------------------------------
            // u32Numerator = (VDDA-VDDD)
            //------------------------------------------------------------------
            u32Numerator = u32Vdda - u32Vddd;

            //------------------------------------------------------------------
            // Dividing by 6.25e-3 is the same as multiplying by 160
            //------------------------------------------------------------------
            u32Numerator = u32Numerator * DCFUNCV_CONST_160;

            //------------------------------------------------------------------
            // Divide by 1000 to convert to register value
            //------------------------------------------------------------------
            u32Result = u32Numerator/DCFUNCV_CONST_1000;

        break;


        //----------------------------------------------------------------------
        // For Alkaline (VDDD*VDDA)/((VDDA-VDDD)*6.25e-3)
        // Equivalent equation we'll use: (VDDD*VDDA*160)/(VDDA-VDDD)
        // VDDA and VDDD are in volts in the eqn, but we will
        // calculate it using mV so we don't mess with fractions.
        //----------------------------------------------------------------------
        case HW_POWER_BATT_MODE_ALKALINE_NIMH:

            //------------------------------------------------------------------
            // u32Numerator = (VDDD*VDDA*160)
            //------------------------------------------------------------------
            u32Numerator = u32Vddd * u32Vdda * DCFUNCV_CONST_160;

            //------------------------------------------------------------------
            // u32Denominator = (VDDA-VDDD)
            //------------------------------------------------------------------
            u32Denominator = u32Vdda - u32Vddd;

            //------------------------------------------------------------------
            // Divide result by 1000 to convert to value for register
            //------------------------------------------------------------------
            u32Result = (u32Numerator/u32Denominator) / DCFUNCV_CONST_1000;

        break;
    }

    //--------------------------------------------------------------------------
    // Make sure the value does not exceed the maximum possible for a 10-bit
    // number. (1023)
    //--------------------------------------------------------------------------
    if(u32Result > DCFUNCV_MAX_VALUE)
    {
        u32Result = DCFUNCV_MAX_VALUE;
    }


    //--------------------------------------------------------------------------
    // Write the value to the register.  Use this method so we get
    // a read, modify, write operation to avoid a temporary zero from
    // the clear/set operation.
    //--------------------------------------------------------------------------
    HW_POWER_DCFUNCV.B.VDDD = u32Result;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_UpdateDcFuncvVddio(void)
{
    uint32_t u32Numerator, u32Denominator, u32Result;
    uint32_t u32Vddio, u32Vdda;

    //--------------------------------------------------------------------------
    // Get voltage values in mV for the rails used in the calculation.
    //--------------------------------------------------------------------------
    u32Vddio = hw_power_GetVddioValue();
    u32Vdda = hw_power_GetVddaValue();


    //--------------------------------------------------------------------------
    // Use the battery mode specific calculations.
    //--------------------------------------------------------------------------
    switch(hw_power_GetBatteryMode())
    {

        //----------------------------------------------------------------------
        // For Li-Ion Value = (VDDIO-VDDA)/(6.25e-3).
        // Equivalent equation we'll use: (VDDIO-VDDA)*160
        // VDDIO and VDDA are in volts in the eqn, but we will
        // calculate it using mV so we don't mess with fractions.
        //----------------------------------------------------------------------
        case HW_POWER_BATT_MODE_LIION:

            //------------------------------------------------------------------
            // u32Numerator = (VDDIO-VDDA)
            //------------------------------------------------------------------
            u32Numerator = u32Vddio - u32Vdda;

            //------------------------------------------------------------------
            // Dividing by 6.25e-3 is the same as multiplying by 160
            //------------------------------------------------------------------
            u32Numerator = u32Numerator * DCFUNCV_CONST_160;

            //------------------------------------------------------------------
            // Divide by 1000 to convert to register value
            //------------------------------------------------------------------
            u32Result = u32Numerator/DCFUNCV_CONST_1000;

        break;


        //----------------------------------------------------------------------
        // For Alkaline (VDDIO*VDDA)/((VDDIO-VDDA)*6.25e-3)
        // Equivalent equation we'll use: (VDDIO*VDDA*160)/(VDDIO-VDDA)
        // VDDIO and VDDA are in volts in the eqn, but we will
        // calculate it using mV so we don't mess with fractions.
        //----------------------------------------------------------------------
        case HW_POWER_BATT_MODE_ALKALINE_NIMH:

            //------------------------------------------------------------------
            // u32Numerator = (VDDIO*VDDA*160)
            //------------------------------------------------------------------
            u32Numerator = u32Vddio * u32Vdda * DCFUNCV_CONST_160;

            //------------------------------------------------------------------
            // u32Denominator = (VDDIO-VDDA)
            //------------------------------------------------------------------
            u32Denominator = u32Vddio - u32Vdda;

            //------------------------------------------------------------------
            // Divide result by 1000 to convert to value for register
            //------------------------------------------------------------------
            u32Result = (u32Numerator/u32Denominator) / DCFUNCV_CONST_1000;

        break;
    }

    //--------------------------------------------------------------------------
    // Make sure the value does not exceed the maximum possible for a 10-bit
    // number. (1023)
    //--------------------------------------------------------------------------
    if(u32Result > DCFUNCV_MAX_VALUE)
    {
        u32Result = DCFUNCV_MAX_VALUE;
    }


    //--------------------------------------------------------------------------
    // Write the value to the register.  Use this method so we get
    // a read, modify, write operation to avoid a temporary zero from
    // the clear/set operation.
    //--------------------------------------------------------------------------
    HW_POWER_DCFUNCV.B.VDDIO = u32Result;
}

////////////////////////////////////////////////////////////////////////////////
// Linear Regulators
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVddioLinearRegulator(bool bDisable)
{
    if(bDisable)
    {
        BF_SET(POWER_5VCTRL, ILIMIT_EQ_ZERO);
    }
    else
    {
        BF_CLR(POWER_5VCTRL, ILIMIT_EQ_ZERO);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVddaLinearRegulator(bool bDisable)
{
    if(bDisable)
    {
        BF_CLR(POWER_VDDACTRL, ENABLE_LINREG);
    }
    else
    {
        BF_SET(POWER_VDDACTRL, ENABLE_LINREG);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVdddLinearRegulator(bool bDisable)
{
    if(bDisable)
    {
        BF_CLR(POWER_VDDDCTRL, ENABLE_LINREG);
    }
    else
    {
        BF_SET(POWER_VDDDCTRL, ENABLE_LINREG);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_Disable5vLinearRegulators(bool bDisable)
{
    if(bDisable)
    {
        // Turn off Vddio LinReg
        hw_power_DisableVddioLinearRegulator(TRUE);
        // Turn off Vdda LinReg
        hw_power_DisableVddaLinearRegulator(TRUE);
        // Turn off Vddd LinReg
        hw_power_DisableVdddLinearRegulator(TRUE);
    }
    else
    {
        // Turn on Vddio Linreg
        hw_power_DisableVddioLinearRegulator(FALSE);
        // Turn on Vdda Linreg
        hw_power_DisableVddaLinearRegulator(FALSE);
        // Turn on Vddd Linreg
        hw_power_DisableVdddLinearRegulator(FALSE);
    }
}

////////////////////////////////////////////////////////////////////////////////
// DCDC/5V Connection
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVdddDcdcFet(bool bDisable)
{
    if(bDisable)
    {
        BF_SET(POWER_VDDDCTRL, DISABLE_FET);
    }
    else
    {
        BF_CLR(POWER_VDDDCTRL, DISABLE_FET);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVddaDcdcFet(bool bDisable)
{
    if(bDisable)
    {
        BF_SET(POWER_VDDACTRL, DISABLE_FET);
    }
    else
    {
        BF_CLR(POWER_VDDACTRL, DISABLE_FET);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableVddioDcdcFet(bool bDisable)
{
    if(bDisable)
    {
        BF_SET(POWER_VDDIOCTRL, DISABLE_FET);
    }
    else
    {
        BF_CLR(POWER_VDDIOCTRL, DISABLE_FET);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_ConfigureDcdcControlLoopFor5vConnection(bool bConfigureFor5vConnection)
{
//  The hardware problem on 36xx that this function fixed has not been seen on
//  37xx.  Therefore, we won't port this function unless we need it.

/*
    hw_lradc_BatteryMode_t  BattMode;
    hw_power_loopctrl_t LoopCtrlReg;

    //BattMode = (hw_lradc_BatteryMode_t)BF_RD(POWER_STS, MODE);
    BattMode = hw_power_GetBatterymode();

    if(bConfigureFor5vConnection)
    {

        // This is needed to account for transients during battery charging + running
        // off of DCDC's.

        // We may not need this anymore.  It was for a 36xx bug.
        if(BattMode==1)
        {
            LoopCtrlReg.B.DC1_R=5;
            LoopCtrlReg.B.EN_DC1_RCSCALE = 0;
            HW_POWER_LOOPCTRL.U = LoopCtrlReg.U;
        }
        else if(BattMode==0)
        {
            LoopCtrlReg.B.DC2_R=5;
            LoopCtrlReg.B.EN_DC2_RCSCALE = 0;
            HW_POWER_LOOPCTRL.U = LoopCtrlReg.U;
        }


        // enable faster battery voltage measurements.  This theoriticaly helps the DCDC work better
        // with a very dynamic Vbat level, which is what happens when you charge the battery
        // AND run off the DCDC's.

        // LRADC_DELAY_TRIGGER3 needs to be set up in a header file somewhere to get
        // consitency between this and startup.c (and possibly host_startup.c)
        // using 0 as the sample interval to trigger the LRADC measurement at a 2khz rate.

            // Setup the trigger loop forever,
        hw_lradc_SetDelayTrigger( LRADC_DELAY_TRIGGER3,         // Trigger Index
                              (1 << BATTERY_VOLTAGE_CH),  // Lradc channels
                              (1 << LRADC_DELAY_TRIGGER3),  // Restart the triggers
                              0,                // No loop count
                              0); // 0.5*N msec on 2khz
    }
    else
    {

        // Return DCDC contorl loop gain to default level to reduce power usage.
        if(BattMode==1)
        {
            LoopCtrlReg.B.DC1_R=2;
            LoopCtrlReg.B.EN_DC1_RCSCALE = 1;
            HW_POWER_LOOPCTRL.U = LoopCtrlReg.U;
        }
        else if(BattMode==0)
        {
            LoopCtrlReg.B.DC2_R=2;
            LoopCtrlReg.B.EN_DC2_RCSCALE = 1;
            HW_POWER_LOOPCTRL.U = LoopCtrlReg.U;
	}
        // Returning battery measurement frequency to initial frequency to reduce power usage

        // LRADC_DELAY_TRIGGER3 needs to be set up in a header file somewhere to get
        // consitency between this and startup.c (and possibly host_startup.c)
        // using 0 as the sample interval to trigger the LRADC measurement at a 2khz rate.

                    // Setup the trigger loop forever,
        hw_lradc_SetDelayTrigger( LRADC_DELAY_TRIGGER3,         // Trigger Index
                              (1 << BATTERY_VOLTAGE_CH),  // Lradc channels
                              (1 << LRADC_DELAY_TRIGGER3),  // Restart the triggers
                              0,                // No loop count
                              200); // 0.5*N msec on 2khz
    }
*/
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
bool hw_power_CheckDcdcTransitionDone(void)
{
    // Only need to check DCDC if a supply is sourced from it.  The LinRegs
    // will also cause the DC_OK flag to go high when transitioning from high
    // to low voltages.
    if((hw_power_GetVddioPowerSource()== HW_POWER_DCDC_LINREG_OFF) ||
       (hw_power_GetVddaPowerSource() == HW_POWER_DCDC_LINREG_OFF) ||
       (hw_power_GetVdddPowerSource() == HW_POWER_DCDC_LINREG_OFF) ||
       (hw_power_GetVddioPowerSource()== HW_POWER_DCDC_LINREG_ON) ||
       (hw_power_GetVddaPowerSource() == HW_POWER_DCDC_LINREG_ON) ||
       (hw_power_GetVdddPowerSource() == HW_POWER_DCDC_LINREG_ON) )
    {
        // Someone is using DCDC's so we need to check the transition
        if (HW_POWER_STS.B.DC_OK)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDcdcDuring5vConnection(bool bEnable)
{
    hw_power_EnableDcdc(bEnable);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDcdc1(bool bEnable)
{
    // TODO: we need to resolve this.  We should call EnableDCDC.  There is only
    // one DCDC in the 37xx.
#if 0	
    SystemHalt();
#endif	
    //hw_power_EnableDcdc(bEnable);                                                                                
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDcdc2(bool bEnable)
{
    // TODO: we need to resolve this.  We should call EnableDCDC.  There is only
    // one DCDC in the 37xx.
#if 0	
    SystemHalt();
#endif	

    //hw_power_EnableDcdc(bEnable);                                                                                
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}


