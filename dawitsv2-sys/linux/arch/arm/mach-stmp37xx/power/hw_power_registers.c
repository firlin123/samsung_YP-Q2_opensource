
////////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_power
//! @{
//
// Copyright (c) 2004 - 2007 SigmaTel, Inc.
//
//! \file hw_power_registers.c
//! \brief Contains hardware register API for power peripheral.
//! \version 0.1
//! \date 03/2007
//!
//! This file contains the hardware accessing functions for direct access
//! to the hardware power registers
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
void hw_power_SetPowerClkGate(bool bGate)
{
    // Gate/Ungate the clock to the power block
    if(bGate)
    {
        BF_SET(POWER_CTRL, CLKGATE);
    }
    else
    {
        BF_CLR(POWER_CTRL, CLKGATE);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetPowerClkGate(void)
{
    return HW_POWER_CTRL.B.CLKGATE;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVbusValidThresh(hw_power_VbusValidThresh_t eThresh)
{
    BF_WR(POWER_5VCTRL, VBUSVALID_TRSH, eThresh);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableVbusValid5vDetect(bool bEnable)
{
    if(bEnable)
    {
        BF_SET(POWER_5VCTRL, VBUSVALID_5VDETECT);
    }
    else
    {
        BF_CLR(POWER_5VCTRL, VBUSVALID_5VDETECT);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisableAutoHardwarePowerdown(bool bDisable)
{
    if(bDisable)
    {
        BF_CLR(POWER_5VCTRL, PWDN_5VBRNOUT);
    }
    else
    {
        BF_SET(POWER_5VCTRL, PWDN_5VBRNOUT);
    }
}

void hw_power_DisableBrownoutPowerdown(void)
{
    BF_CLR(POWER_BATTMONITOR, PWDN_BATTBRNOUT);
}

void hw_power_Disable5vBrownoutPowerdown(void)
{
    BF_CLR(POWER_5VCTRL, PWDN_5VBRNOUT);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDcdc(bool bEnable)
{
    if(bEnable)
        BF_SET(POWER_5VCTRL, ENABLE_DCDC);
    else
        BF_CLR(POWER_5VCTRL, ENABLE_DCDC);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetEnableDcdc(void)
{
    return HW_POWER_5VCTRL.B.ENABLE_DCDC;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableAutoDcdcTransfer(bool bEnable)
{
    if(bEnable)
    {
        BF_SET(POWER_5VCTRL, DCDC_XFER);
    }
    else
    {
        BF_CLR(POWER_5VCTRL, DCDC_XFER);
    }
}


////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableCurrentLimit(bool bEnable)
{
    if(bEnable)
    {
        BF_SET(POWER_5VCTRL, ENABLE_ILIMIT);
    }
    else
    {
        BF_CLR(POWER_5VCTRL, ENABLE_ILIMIT);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
hw_power_BiasCurrentSource_t hw_power_GetBiasCurrentSource(void)
{
    return (hw_power_BiasCurrentSource_t) HW_POWER_CHARGE.B.USE_EXTERN_R;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetBiasCurrentSource(hw_power_BiasCurrentSource_t eSource)
{
    switch(eSource)
    {
        case HW_POWER_INTERNAL_BIAS_CURRENT:
            BF_SET(POWER_CHARGE, USE_EXTERN_R);
        break;

        case HW_POWER_EXTERNAL_BIAS_CURRENT:
            BF_CLR(POWER_CHARGE, USE_EXTERN_R);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetPowerDownBatteryCharger(void)
{
    return BF_RD(POWER_CHARGE, PWD_BATTCHRG);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetPowerDownBatteryCharger(bool bPowerDown)
{
    if(bPowerDown)
    {
        BF_SET(POWER_CHARGE, PWD_BATTCHRG);
    }
    else
    {
        BF_CLR(POWER_CHARGE, PWD_BATTCHRG);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVdddLinRegOffset(hw_power_LinRegOffsetStep_t eOffset)
{
    BF_WR(POWER_VDDDCTRL,LINREG_OFFSET,eOffset);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddaLinRegOffset(hw_power_LinRegOffsetStep_t eOffset)
{
    BF_WR(POWER_VDDACTRL,LINREG_OFFSET,eOffset);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetVddioLinRegOffset(hw_power_LinRegOffsetStep_t eOffset)
{
    BF_WR(POWER_VDDIOCTRL,LINREG_OFFSET,eOffset);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
hw_power_LinRegOffsetStep_t hw_power_GetVdddLinRegOffset(void)
{
    return (hw_power_LinRegOffsetStep_t) HW_POWER_VDDDCTRL.B.LINREG_OFFSET;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
hw_power_LinRegOffsetStep_t hw_power_GetVddaLinRegOffset(void)
{
    return (hw_power_LinRegOffsetStep_t) HW_POWER_VDDACTRL.B.LINREG_OFFSET;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
hw_power_LinRegOffsetStep_t hw_power_GetVddioLinRegOffset(void)
{
    return (hw_power_LinRegOffsetStep_t) HW_POWER_VDDIOCTRL.B.LINREG_OFFSET;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableRcScale(hw_power_RcScaleLevels_t eLevel)
{
    BF_WR(POWER_LOOPCTRL, EN_RCSCALE, eLevel);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetBattChrgPresentFlag(void)
{
    return HW_POWER_STS.B.BATT_CHRG_PRESENT;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
hw_power_BatteryMode_t hw_power_GetBatteryMode(void)
{
    return (hw_power_BatteryMode_t) HW_POWER_STS.B.MODE;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
bool hw_power_GetBatteryChargingStatus(void)
{
    return HW_POWER_STS.B.CHRGSTS;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_PowerDown(void)
{
    //--------------------------------------------------------------------------
    // Make sure the power down bit is not disabled.  Just key so PWD_OFF
    // will be written with zero.
    //--------------------------------------------------------------------------
    HW_POWER_RESET_WR(POWERDOWN_KEY << 16);

    //--------------------------------------------------------------------------
    // Set the PWD bit to shut off the power.
    //--------------------------------------------------------------------------
    HW_POWER_RESET_WR((POWERDOWN_KEY << 16) | BM_POWER_RESET_PWD);

	//----------------------------------------------------------------------
	// You may begin to feel a little sleepy.
	//----------------------------------------------------------------------
	Loop: goto Loop;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_DisablePowerDown(bool bDisable)
{
    if(bDisable)
    {
        //----------------------------------------------------------------------
        // Key and mask for PWD_OFF to set PWD_OFF.
        //----------------------------------------------------------------------
        HW_POWER_RESET_WR((POWERDOWN_KEY << 16) | BM_POWER_RESET_PWD_OFF);
    }
    else
    {
        //----------------------------------------------------------------------
        // Just key so PWD_OFF will be written with zero.
        //----------------------------------------------------------------------
        HW_POWER_RESET_WR(POWERDOWN_KEY << 16);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableVdddLinRegFromBatt(bool bEnable)
{
    if(bEnable)
    {
        BF_SET(POWER_VDDDCTRL, LINREG_FROM_BATT);
    }
    else
    {
        BF_CLR(POWER_VDDDCTRL, LINREG_FROM_BATT);
    }
}


////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetPosLimitBoost(uint16_t u16Limit)
{
    BF_WR(POWER_DCLIMITS, POSLIMIT_BOOST, u16Limit);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetPosLimitBuck(uint16_t u16Limit)
{
    BF_WR(POWER_DCLIMITS, POSLIMIT_BUCK, u16Limit);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetNegLimit(uint16_t u16Limit)
{
    BF_WR(POWER_DCLIMITS, NEGLIMIT, u16Limit);
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableDoubleFets(bool bEnable)
{
    if(bEnable)
    {
        BF_SET(POWER_MINPWR, DOUBLE_FETS);
    }
    else
    {
        BF_CLR(POWER_MINPWR, DOUBLE_FETS);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_EnableHalfFets(bool bEnable)
{
    if(bEnable)
    {
        BF_SET(POWER_MINPWR, HALF_FETS);
    }
    else
    {
        BF_CLR(POWER_MINPWR, HALF_FETS);
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetLoopCtrlDcC(uint16_t u16Value)
{
    BF_WR(POWER_LOOPCTRL, DC_C, u16Value);
}

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
//!
////////////////////////////////////////////////////////////////////////////////
void hw_power_SetDcdcClkFreq(uint16_t u16Freq, bool bUsePll)
{
    uint16_t Setting;

    //--------------------------------------------------------------------------
    // Convert the decimal frequency value to the register setting.  The default
    // value will be 24MHz.
    //--------------------------------------------------------------------------
    if(u16Freq == 19200)
    {
        Setting = 0x3;
    }
    else if(u16Freq == 20000)
    {
        Setting = 0x1;
    }
    else
    {
        Setting = 0x2;
    }

    //--------------------------------------------------------------------------
    // Set the new PLL frequency first even if we are using the XTAL.  This will
    // synchronize the frequencies.  Then use the bUsePll variable to set the
    // SEL_PLLCLK field.  A TRUE value will set a one in the register field.
    //--------------------------------------------------------------------------
    BF_WR(POWER_MISC, FREQSEL, Setting);
    BF_WR(POWER_MISC, SEL_PLLCLK, bUsePll);

}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

