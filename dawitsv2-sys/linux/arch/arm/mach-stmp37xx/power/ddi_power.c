////////////////////////////////////////////////////////////////////////////////
//! \addtogroup ddi_power
//! @{
//
// Copyright(C) 2005 SigmaTel, Inc.
//
//! \file ddi_power.c
//! \brief Implementation file for the power driver.
//!
//!
////////////////////////////////////////////////////////////////////////////////
//   Includes and external references
////////////////////////////////////////////////////////////////////////////////
#include "ddi_power_common.h"

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////
//static bool bEnableVdddSafetyLimits = true;
static bool bEnableVddaSafetyLimits = true;
static bool bEnableVddioSafetyLimits = true;
static bool b5vPowerSourceValid = false;
static bool bForceDcdcPowerSource = false;

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////
uint32_t hw_digctl_GetCurrentTime(void)
{
    return HW_DIGCTL_MICROSECONDS_RD();
}

/////////////////////////////////////////////////////////////////////////////////
//! See hw_digctl.h for details
/////////////////////////////////////////////////////////////////////////////////
bool hw_digctl_CheckTimeOut(uint32_t StartTime, uint32_t TimeOut)
{
    uint32_t    CurTime, EndTime;
    bool        bTimeOut;

    CurTime = HW_DIGCTL_MICROSECONDS_RD();
    EndTime = StartTime + TimeOut;

    if ( StartTime <= EndTime)
    {
        bTimeOut = ((CurTime >= StartTime) && (CurTime < EndTime))? FALSE : TRUE;
    }
    else
    {
        bTimeOut = ((CurTime >= StartTime) || (CurTime < EndTime))? FALSE : TRUE;
    }

    return bTimeOut;
}

////////////////////////////////////////////////////////////////////////////////
// VDDD
////////////////////////////////////////////////////////////////////////////////

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
//! will not return until the output VDDD stable.
//!
//! \param[in]  u16Vddd_mV              Vddd voltage in millivolts
//! \param[in]  u16VdddBrownout_mV      Vddd brownout in millivolts
//!
//! \return SUCCESS.
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_SetVddd(uint16_t  u16NewTarget, uint16_t  u16NewBrownout)
{
    uint16_t    u16CurrentTarget;
    RtStatus_t  rtn = SUCCESS;
    bool        bPoweredByLinReg;
    //hw_audioout_refctrl_t tempRefCtrlReg;


    //--------------------------------------------------------------------------
    // Limit inputs and get the current target level
    //--------------------------------------------------------------------------

    // Apply ceiling and floor limits to Vddd and BO
    rtn = ddi_power_LimitVdddAndBo(&u16NewTarget, &u16NewBrownout);

    // Convert the brownout as millivolt value to an offset from the target
    u16NewBrownout = u16NewTarget - u16NewBrownout;

    // Get the current target value
    u16CurrentTarget = hw_power_GetVdddValue();

    // Determine if this rail is powered by linear regulator.
#if 1
    if(hw_power_GetVdddPowerSource() == HW_POWER_LINREG_DCDC_READY ||
       hw_power_GetVdddPowerSource() == HW_POWER_LINREG_DCDC_OFF )
#else

#endif
    {
        bPoweredByLinReg = TRUE;
	//printk("%s, %d, \n", __FILE__, __LINE__);
    }
    else
    {
	//printk("%s, %d, \n", __FILE__, __LINE__);
        bPoweredByLinReg = FALSE;
    }


    //--------------------------------------------------------------------------
    // Voltage and brownouts need to be changed in specific order.
    //
    // Because the brownout is now an offset of the target, as the target
    // steps up or down, the brownout voltage will follow.  This causes a
    // problem where the brownout will be one step too close when the target
    // raised because the brownout changes instantly, but the output voltage
    // ramps up.
    //
    //  Target does this              ____
    //                               /
    //                          ____/
    //  but brownout does this       _____
    //                              |
    //                          ____|
    //
    //  We are concerned about the time under the ramp when the brownout
    //  is too close.
    //--------------------------------------------------------------------------

    if(u16NewTarget > u16CurrentTarget)
    {
	//printk("%s, %d, \n", __FILE__, __LINE__);
        // Temporarily change the rail's brownout.
        if(bPoweredByLinReg)
        {
            // Disable detection if powered by linear regulator.  This avoids
            // the problem where the brownout level reaches its new value before
            // the target does.
            hw_power_EnableVdddBrownoutInterrupt(FALSE);
        }
        else
        {
            // Temporarily use the maximum offset for DCDC transitions.  DCDC
            // transitions step the target and the brownout offset in 25mV
            // increments so there is not a risk of an inverted brownout level.
            hw_power_SetVdddBrownoutValue(BO_MAX_OFFSET_MV);
        }

        // Now change target
        hw_power_SetVdddValue(u16NewTarget);

        // Wait for Vddd to become stable
        ddi_power_WaitForVdddStable();

        // Clear the interrupt in case it occured.
        if(bPoweredByLinReg)
        {
            hw_power_ClearVdddBrownoutInterrupt();
            hw_power_EnableVdddBrownoutInterrupt(TRUE);
        }

        // Set the real brownout offset.
        hw_power_SetVdddBrownoutValue(u16NewBrownout);

    }
    else
    {
        // Change target
        hw_power_SetVdddValue(u16NewTarget);

        // Wait for Vddd to become stable
        ddi_power_WaitForVdddStable();

        // Set brownout
        hw_power_SetVdddBrownoutValue(u16NewBrownout);
    }


    //--------------------------------------------------------------------------
    // Update DCDC FuncV values
    //--------------------------------------------------------------------------

    // Update Vddd
    hw_power_UpdateDcFuncvVddd();

    // Update Vddio
    hw_power_UpdateDcFuncvVddio();

#if 0
    // Adjust according to Table 1135. "Audio Reference Control Settings"
    tempRefCtrlReg.U = HW_AUDIOOUT_REFCTRL_RD();
    if(u16NewTarget < 1700)
    {
        tempRefCtrlReg.B.RAISE_REF = 0;
    }
    else
    {
        tempRefCtrlReg.B.RAISE_REF = 1;
    }
    HW_AUDIOOUT_REFCTRL_WR(tempRefCtrlReg.U);
#endif

    return rtn;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDD voltage
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present values of the VDDD voltage in millivolts
//!
//! \param[in]  none
//!
//! \return     VDDD voltage in millivolts
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVddd(void)
{

    ///////////////////////////////////////////////
    // Read the converted register value
    ///////////////////////////////////////////////

    return hw_power_GetVdddValue();
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDD brownout level voltage
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value of VDDD brownout in millivolts
//!
//! \param[in]  none
//!
//! \return     VDDD brownout voltage in millivolts
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVdddBrownout(void)
{
    uint16_t VdddBoOffset_mV;
    uint16_t Vddd_mV;
    uint16_t VdddBo;

    //--------------------------------------------------------------------------
    // Read the target and brownout register values.
    //--------------------------------------------------------------------------

    Vddd_mV = hw_power_GetVdddValue();
    VdddBoOffset_mV = hw_power_GetVdddBrownoutValue();


    //--------------------------------------------------------------------------
    // The brownout level is the difference between the target and the offset.
    //--------------------------------------------------------------------------

    VdddBo = Vddd_mV - VdddBoOffset_mV;

    return VdddBo;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief      Waits on status bit signal or timeout for Vddd stability
//!
//! \fntype     Non-reentrant Function
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_WaitForVdddStable(void)
{
    uint32_t u32StartTime;

    // Wait for VDDD stable
    if (hw_power_GetVdddPowerSource()!=HW_POWER_LINREG_DCDC_OFF)
    {//It's DCDC operation, wait for DCDC stable

        // Wait for DCDC output stable
        u32StartTime = hw_digctl_GetCurrentTime();

        while ( !hw_power_CheckDcdcTransitionDone() &&
				!hw_digctl_CheckTimeOut(u32StartTime, DDI_PWR_DCDC_READY_TIMEOUT) );
     }
    else
    {
        u32StartTime = hw_digctl_GetCurrentTime();
        while ( !hw_digctl_CheckTimeOut(u32StartTime, DDI_PWR_LINREG_READY_TIMEOUT) );
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See ddi_power.h for details.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_power_LimitVdddAndBo(uint16_t *pu16Vddd_mV, uint16_t *pu16Bo_mV)
{
    uint16_t    u16Vddd_mV = *pu16Vddd_mV;
    uint16_t    u16Bo_mV = *pu16Bo_mV;
    RtStatus_t  rtn = SUCCESS;

    //////////////////////////////////////////
    // Check Vddd limits
    //////////////////////////////////////////

	// Make sure Vddd is not above the safe voltage
	if (u16Vddd_mV > VDDD_SAFE_MAX_MV)
	{
		u16Vddd_mV = VDDD_SAFE_MAX_MV;
		rtn = ERROR_DDI_POWER_VDDD_PARAM_ADJUSTED;
	}
        // Make sure Vddd is not below the safe voltage
    if (u16Vddd_mV < VDDD_SAFE_MIN_MV)
    {
        u16Vddd_mV = VDDD_SAFE_MIN_MV;
        rtn = ERROR_DDI_POWER_VDDD_PARAM_ADJUSTED;
    }

    //////////////////////////////////////////
    // Check Vddd brownout limits
    //////////////////////////////////////////

    // Make sure there's at least a margin of difference between Vddd and Bo
    if (VDDD_TO_BO_MARGIN > (u16Vddd_mV - u16Bo_mV))
    {
        u16Bo_mV = u16Vddd_mV - VDDD_TO_BO_MARGIN;
        rtn = ERROR_DDI_POWER_VDDD_PARAM_ADJUSTED;
    }

    // Make sure the brownout value does not exceed the maximum allowed
    // by the system.
    if ((u16Vddd_mV - u16Bo_mV) > BO_MAX_OFFSET_MV)
    {
        u16Bo_mV = u16Vddd_mV - BO_MAX_OFFSET_MV;
        rtn = ERROR_DDI_POWER_VDDIO_PARAM_ADJUSTED;
    }

    // Give the results back to the caller.
    *pu16Vddd_mV = u16Vddd_mV;
    *pu16Bo_mV = u16Bo_mV;
    return rtn;
}

////////////////////////////////////////////////////////////////////////////////
// VDDIO
////////////////////////////////////////////////////////////////////////////////

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
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_SetVddio(uint16_t u16NewTarget, uint16_t u16NewBrownout)
{
    uint16_t    u16CurrentTarget;
    RtStatus_t  rtn = SUCCESS;
    bool        bPoweredByLinReg;

    //--------------------------------------------------------------------------
    // Limit inputs and get the current BO level
    //--------------------------------------------------------------------------
    // Apply ceiling and floor limits to Vddio and BO
    rtn = ddi_power_LimitVddioAndBo(&u16NewTarget, &u16NewBrownout);

    // Convert the brownout as millivolt value to an offset from the target
    u16NewBrownout = u16NewTarget - u16NewBrownout;

    // Get the current target value
    u16CurrentTarget = hw_power_GetVddioValue();

    // Determine if this rail is powered by linear regulator.
    if(hw_power_GetVddioPowerSource() == HW_POWER_LINREG_DCDC_READY ||
       hw_power_GetVddioPowerSource() == HW_POWER_LINREG_DCDC_OFF )
    {
        bPoweredByLinReg = TRUE;
    }
    else
    {
        bPoweredByLinReg = FALSE;
    }

    //--------------------------------------------------------------------------
    // Voltage and brownouts need to be changed in specific order .
    // Because the brownout is now an offset of the target, as the target
    // steps up or down, the brownout voltage will follow.  This causes a
    // problem where the brownout will be one step too close when the target
    // raised because the brownout changes instantly, but the output voltage
    // ramps up.
    //
    // Target does this              ____
    //                              /
    //                         ____/
    // but brownout does this       _____
    //                             |
    //                         ____|
    //
    //  We are concerned about the time under the ramp when the brownout
    //  is too close.
    //--------------------------------------------------------------------------

    if(u16NewTarget > u16CurrentTarget)
    {
        // Temporarily change the rail's brownout.
        if(bPoweredByLinReg)
        {
            // Disable detection if powered by linear regulator.  This avoids
            // the problem where the brownout level reaches its new value before
            // the target does.
            hw_power_EnableVddioBrownoutInterrupt(FALSE);
        }
        else
        {
            // Temporarily use the maximum offset for DCDC transitions.  DCDC
            // transitions step the target and the brownout offset in 25mV
            // increments so there is not a risk of an inverted brownout level.
            hw_power_SetVddioBrownoutValue(BO_MAX_OFFSET_MV);
        }

        // Now change target
        hw_power_SetVddioValue(u16NewTarget);

        // Wait for Vddio to become stable
        ddi_power_WaitForVddioStable();

        // Clear the interrupt in case it occured.
        if(bPoweredByLinReg)
        {
            hw_power_ClearVddioBrownoutInterrupt();
            hw_power_EnableVddioBrownoutInterrupt(TRUE);
        }

        // Set the real brownout offset.
        hw_power_SetVddioBrownoutValue(u16NewBrownout);

    }
    else
    {
        // Change target
        hw_power_SetVddioValue(u16NewTarget);

        // Wait for Vddio to become stable
        ddi_power_WaitForVddioStable();

        // Set brownout
        hw_power_SetVddioBrownoutValue(u16NewBrownout);
    }

    //--------------------------------------------------------------------------
    // Update DCDC FuncV values
    //--------------------------------------------------------------------------

    // Update Vddio
    hw_power_UpdateDcFuncvVddd();

    // Update Vddio
    hw_power_UpdateDcFuncvVddio();

    return rtn;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDIO voltage
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present values of the VDDIO voltage in millivolts
//!
//! \param[in]  none
//!
//! \return     VDDIO voltage in millivolts
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVddio(void)
{
    //--------------------------------------------------------------------------
    // Return the converted register value.
    //--------------------------------------------------------------------------

    return hw_power_GetVddioValue();
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDIO brownout level voltage
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value of VDDIO brownout in millivolts
//!
//! \param[in]  none
//!
//! \return     VDDIO brownout voltage in millivolts
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVddioBrownout(void)
{
    uint16_t VddioBoOffset_mV;
    uint16_t Vddio_mV;
    uint16_t VddioBo;

    //--------------------------------------------------------------------------
    // Read the target and brownout register value
    //--------------------------------------------------------------------------

    Vddio_mV = hw_power_GetVddioValue();
    VddioBoOffset_mV = hw_power_GetVddioBrownoutValue();


    //--------------------------------------------------------------------------
    // The brownout level is the difference between the target and the offset.
    //--------------------------------------------------------------------------

    VddioBo = Vddio_mV - VddioBoOffset_mV;

    return VddioBo;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief      Waits on status bit signal or timeout for Vddio stability
//!
//! \fntype     Non-reentrant Function
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_WaitForVddioStable(void)
{
    uint32_t u32StartTime;

    // Wait for VDDIO stable
    if (hw_power_GetVddioPowerSource()!=HW_POWER_LINREG_DCDC_OFF)
    {//It's DCDC operation, wait for DCDC stable

        // Wait for DCDC output stable
        u32StartTime = hw_digctl_GetCurrentTime();
        while ( !hw_power_CheckDcdcTransitionDone() &&
                !hw_digctl_CheckTimeOut(u32StartTime, DDI_PWR_DCDC_READY_TIMEOUT) );
     }
    else
    {
        u32StartTime = hw_digctl_GetCurrentTime();
        while ( !hw_digctl_CheckTimeOut(u32StartTime, DDI_PWR_LINREG_READY_TIMEOUT) );
    }
}
////////////////////////////////////////////////////////////////////////////////
//! See ddi_power.h for details.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_power_LimitVddioAndBo(uint16_t *pu16Vddio_mV, uint16_t *pu16Bo_mV)
{
    uint16_t    u16Trg_mV = *pu16Vddio_mV;
    uint16_t    u16Bo_mV = *pu16Bo_mV;
    RtStatus_t  rtn = SUCCESS;

    //--------------------------------------------------------------------------
    // Check Vddio limits. Use different limits depending on whether we are
    // checking safety limits or register limits.
    //--------------------------------------------------------------------------
    if(bEnableVddioSafetyLimits)
    {
        //----------------------------------------------------------------------
        // Make sure Vddio is not above the safe voltage
        //----------------------------------------------------------------------
        if (u16Trg_mV > VDDIO_SAFE_MAX_MV)
        {
            u16Trg_mV = VDDIO_SAFE_MAX_MV;
            rtn = ERROR_DDI_POWER_VDDIO_PARAM_ADJUSTED;
        }

        //----------------------------------------------------------------------
        // Make sure Vddio is not below the safe voltage
        //----------------------------------------------------------------------
        if (u16Trg_mV < VDDIO_SAFE_MIN_MV)
        {
            u16Trg_mV = VDDIO_SAFE_MIN_MV;
            rtn = ERROR_DDI_POWER_VDDIO_PARAM_ADJUSTED;
        }
    }


    //--------------------------------------------------------------------------
    // Check Vddio brownout limits
    //--------------------------------------------------------------------------
    {
        //----------------------------------------------------------------------
        // Make sure there's at least a margin of difference between Vddio
        // and the brownout level.
        //----------------------------------------------------------------------
        if (BO_MIN_OFFSET_MV > (u16Trg_mV - u16Bo_mV))
        {
            u16Bo_mV = u16Trg_mV - BO_MIN_OFFSET_MV;
            rtn = ERROR_DDI_POWER_VDDIO_PARAM_ADJUSTED;
        }

        //----------------------------------------------------------------------
        // Make sure the brownout value does not exceed the maximum allowed
        // by the system.
        //----------------------------------------------------------------------
        if ((u16Trg_mV - u16Bo_mV) > BO_MAX_OFFSET_MV)
        {
            u16Bo_mV = u16Trg_mV - BO_MAX_OFFSET_MV;
            rtn = ERROR_DDI_POWER_VDDIO_PARAM_ADJUSTED;
        }

    }

    //--------------------------------------------------------------------------
    // Give the results back to the caller and return.
    //--------------------------------------------------------------------------
    *pu16Vddio_mV = u16Trg_mV;
    *pu16Bo_mV = u16Bo_mV;
    return rtn;
}

void ddi_power_EnableVddioSafetyLimits(bool bEnable)
{
    bEnableVddioSafetyLimits = bEnable;
}

////////////////////////////////////////////////////////////////////////////////
// VDDA
////////////////////////////////////////////////////////////////////////////////

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
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_SetVdda(uint16_t  u16NewTarget, uint16_t  u16NewBrownout)
{
    uint16_t    u16CurrentTarget;
    RtStatus_t  rtn = SUCCESS;
    bool        bPoweredByLinReg;

    //--------------------------------------------------------------------------
    // Limit inputs and get the current BO level
    //--------------------------------------------------------------------------

    // Apply ceiling and floor limits to Vdda and BO
    //rtn = ddi_power_LimitVddaAndBo(&u16NewTarget, &u16NewBrownout); //disable to set max vdda val(2.1V)

    // Convert the brownout as millivolt value to an offset from the target
    u16NewBrownout = u16NewTarget - u16NewBrownout;

    // Get the current target value
    u16CurrentTarget = hw_power_GetVddaValue();

    // Determine if this rail is powered by linear regulator.
    if(hw_power_GetVddaPowerSource() == HW_POWER_LINREG_DCDC_READY ||
       hw_power_GetVddaPowerSource() == HW_POWER_LINREG_DCDC_OFF )
    {
        bPoweredByLinReg = TRUE;
    }
    else
    {
        bPoweredByLinReg = FALSE;
    }

    //--------------------------------------------------------------------------
    // Voltage and brownouts need to be changed in specific order.
    // Because the brownout is now an offset of the target, as the target
    // steps up or down, the brownout voltage will follow.  This causes a
    // problem where the brownout will be one step too close when the target
    // raised because the brownout changes instantly, but the output voltage
    // ramps up.
    //
    // Target does this              ____
    //                              /
    //                         ____/
    // but brownout does this       _____
    //                             |
    //                         ____|
    //
    // We are concerned about the time under the ramp when the
    // brownout is too close.
    //--------------------------------------------------------------------------

    if(u16NewTarget > u16CurrentTarget)
    {

        // Temporarily change the rail's brownout.
        if(bPoweredByLinReg)
        {
            // Disable detection if powered by linear regulator.  This avoids
            // the problem where the brownout level reaches its new value before
            // the target does.
            hw_power_EnableVddaBrownoutInterrupt(FALSE);
        }
        else
        {
            // Temporarily use the maximum offset for DCDC transitions.  DCDC
            // transitions step the target and the brownout offset in 25mV
            // increments so there is not a risk of an inverted brownout level.
            hw_power_SetVddaBrownoutValue(BO_MAX_OFFSET_MV);
        }

        // Now change target
        hw_power_SetVddaValue(u16NewTarget);

        // Wait for Vdda to become stable
        ddi_power_WaitForVddaStable();

        // Clear the interrupt in case it occured.
        if(bPoweredByLinReg)
        {
            hw_power_ClearVddaBrownoutInterrupt();
            hw_power_EnableVddaBrownoutInterrupt(TRUE);
        }

        // Set the real brownout offset.
        hw_power_SetVddaBrownoutValue(u16NewBrownout);

    }
    else
    {
        // Change target
        hw_power_SetVddaValue(u16NewTarget);

        // Wait for Vdda to become stable
        ddi_power_WaitForVddaStable();

        // Set brownout
        hw_power_SetVddaBrownoutValue(u16NewBrownout);
    }

    //--------------------------------------------------------------------------
    // Update DCDC FuncV values
    //--------------------------------------------------------------------------

    // Update Vdda
    hw_power_UpdateDcFuncvVddd();

    // Update Vdda
    hw_power_UpdateDcFuncvVddio();

    return rtn;
}
EXPORT_SYMBOL(ddi_power_SetVdda);
////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDA voltage
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present values of the VDDA voltage in millivolts
//!
//! \param[in]  none
//!
//! \return     VDDA voltage in millivolts
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVdda(void)
{
    //--------------------------------------------------------------------------
    // Return the converted register value.
    //--------------------------------------------------------------------------

    return hw_power_GetVddaValue();
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Read the VDDA brownout level voltage
//!
//! \fntype     Reentrant Function
//!
//! This function returns the present value of VDDA brownout in millivolts
//!
//! \param[in]  none
//!
//! \return     VDDA brownout voltage in millivolts
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_GetVddaBrownout(void)
{
    uint16_t VddaBoOffset_mV;
    uint16_t Vdda_mV;
    uint16_t VddaBo;

    //--------------------------------------------------------------------------
    // Read the target and brownout register values.
    //--------------------------------------------------------------------------

    Vdda_mV = hw_power_GetVddaValue();
    VddaBoOffset_mV = hw_power_GetVddaBrownoutValue();


    //--------------------------------------------------------------------------
    // The brownout level is the difference between the target and the offset.
    //--------------------------------------------------------------------------

    VddaBo = Vdda_mV - VddaBoOffset_mV;

    return VddaBo;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief      Waits on status bit signal or timeout for Vdda stability
//!
//! \fntype     Non-reentrant Function
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_WaitForVddaStable(void)
{
    uint32_t u32StartTime;

    // Wait for VDDA stable
    if (hw_power_GetVddaPowerSource()!=HW_POWER_LINREG_DCDC_OFF)
    {//It's DCDC operation, wait for DCDC stable

        // Wait for DCDC output stable
        u32StartTime = hw_digctl_GetCurrentTime();
        while ( !hw_power_CheckDcdcTransitionDone() &&
                !hw_digctl_CheckTimeOut(u32StartTime, DDI_PWR_DCDC_READY_TIMEOUT) );
     }
    else
    {
        u32StartTime = hw_digctl_GetCurrentTime();
        while ( !hw_digctl_CheckTimeOut(u32StartTime, DDI_PWR_LINREG_READY_TIMEOUT) );
    }
}


////////////////////////////////////////////////////////////////////////////////
//! See ddi_power.h for details.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_power_LimitVddaAndBo(uint16_t *pu16Vdda_mV, uint16_t *pu16Bo_mV)
{
    uint16_t    u16Trg_mV = *pu16Vdda_mV;
    uint16_t    u16Bo_mV = *pu16Bo_mV;
    RtStatus_t  rtn = SUCCESS;

    //--------------------------------------------------------------------------
    // Check Vdda limits. Use different limits depending on whether we are
    // checking safety limits or register limits.
    //--------------------------------------------------------------------------
    if(bEnableVddaSafetyLimits)
    {
        //----------------------------------------------------------------------
        // Make sure Vdda is not above the safe voltage
        //----------------------------------------------------------------------
        if (u16Trg_mV > VDDA_SAFE_MAX_MV)
        {
            u16Trg_mV = VDDA_SAFE_MAX_MV;
            rtn = ERROR_DDI_POWER_VDDA_PARAM_ADJUSTED;
        }

        //----------------------------------------------------------------------
        // Make sure Vdda is not below the safe voltage
        //----------------------------------------------------------------------
        if (u16Trg_mV < VDDA_SAFE_MIN_MV)
        {
            u16Trg_mV = VDDA_SAFE_MIN_MV;
            rtn = ERROR_DDI_POWER_VDDA_PARAM_ADJUSTED;
        }
    }


    //--------------------------------------------------------------------------
    // Check Vdda brownout limits
    //--------------------------------------------------------------------------
    {
        //----------------------------------------------------------------------
        // Make sure there's at least a margin of difference between Vdda
        // and the brownout level.
        //----------------------------------------------------------------------
        if (BO_MIN_OFFSET_MV > (u16Trg_mV - u16Bo_mV))
        {
            u16Bo_mV = u16Trg_mV - BO_MIN_OFFSET_MV;
            rtn = ERROR_DDI_POWER_VDDA_PARAM_ADJUSTED;
        }

        //----------------------------------------------------------------------
        // Make sure the brownout value does not exceed the maximum allowed
        // by the system.
        //----------------------------------------------------------------------
        if ((u16Trg_mV - u16Bo_mV) > BO_MAX_OFFSET_MV)
        {
            u16Bo_mV = u16Trg_mV - BO_MAX_OFFSET_MV;
            rtn = ERROR_DDI_POWER_VDDIO_PARAM_ADJUSTED;
        }

    }

    //--------------------------------------------------------------------------
    // Give the results back to the caller and return.
    //--------------------------------------------------------------------------
    *pu16Vdda_mV = u16Trg_mV;
    *pu16Bo_mV = u16Bo_mV;
    return rtn;
}

void ddi_power_EnableVddaSafetyLimits(bool bEnable)
{
    bEnableVddaSafetyLimits = bEnable;
}

////////////////////////////////////////////////////////////////////////////////
// Power and reset functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Power down the chip without clearing the persistent bits
//!
//! \fntype Function
//!
//! This function shuts off the power without clearing the persistent bits.
//!
////////////////////////////////////////////////////////////////////////////////
void  ddi_power_PowerDown(void)
{
    // Good bye.
    hw_power_PowerDown();
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Power down the chip with clearing the persistent bits
//!
//! \fntype Function
//!
//! This function shuts off the power to the chip and clears the analog portion as well
//!
//!
////////////////////////////////////////////////////////////////////////////////
void  ddi_power_ColdPowerDown(void)
{
    // TODO: implement cold power down if needed
    // For now, we can just do the normal power down
    hw_power_PowerDown();
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Reboot the chip
//!
//! \fntype Function
//!
//! This function resets all the non-power module digital registers and reboots
//! the CPU.
//!
////////////////////////////////////////////////////////////////////////////////
void  ddi_power_ColdReboot(void)
{
    // TODO: implement if needed
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Restart the chip, keeping persistent bits intact
//!
//! \fntype Function
//!
//! This function resets all but the power registers and
//! most power register values are changed to their
//! default values
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_WarmRestart(void)
{
    // Do not use this function for 37xx.  
    // Use the framework's shutdown.
#if 0	
    SystemHalt();
#endif	
}

////////////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Increments the brownout offset by one step
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_IncrementBrownoutOffset(uint16_t u16Offset)
{
    return u16Offset + VOLTAGE_STEP_MV;
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Decrements the brownout offset by one step
//!
////////////////////////////////////////////////////////////////////////////////
uint16_t ddi_power_DecrementBrownoutOffset(uint16_t u16Offset)
{
    return u16Offset - VOLTAGE_STEP_MV;
}

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_UpdateDcFuncvVddd
//!
//! \brief
////////////////////////////////////////////////////////////////////////////////
void ddi_power_UpdateDcFuncvVddd(void)
{
    //TODO: put Transient loading options here...
    hw_power_UpdateDcFuncvVddd();
}

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_UpdateDcFuncvVddio
//!
//! \brief
////////////////////////////////////////////////////////////////////////////////
void ddi_power_UpdateDcFuncvVddio(void)
{
    //TODO: put Transient loading options here...
    hw_power_UpdateDcFuncvVddio();
}

////////////////////////////////////////////////////////////////////////////////
//! Name: ddi_power_Get5VPresent
//!
//! \brief
////////////////////////////////////////////////////////////////////////////////
bool ddi_power_Get5vPresent(void)
{
    return hw_power_Get5vPresentFlag();
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Checks if the 5V source is available for use.  
//!
//! This function returns the status of the 5V power source.  To be valid, 
//! the 5V source must be present, and the application must have validated
//! the 5V source as an available source for use.  Also, the DCDC must not
//! be forced as the chip's power source.
////////////////////////////////////////////////////////////////////////////////
bool ddi_power_5vPowerSourceValid(void)
{
    return b5vPowerSourceValid && !bForceDcdcPowerSource;
}

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
void ddi_power_Validate5vPowerSource(bool bValidSource)
{
    b5vPowerSourceValid = bValidSource;
}                                      

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
void ddi_power_ForceDcdcPowerSource(bool bForceDcdc)
{
    bForceDcdcPowerSource = bForceDcdc;
}                                      


////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Report on the die temperature.
//!
//! \fntype Function
//!
//! This function reports on the die temperature.
//!
//! \param[out]  pLow   The low  end of the temperature range.
//! \param[out]  pHigh  The high end of the temperature range.
//!
////////////////////////////////////////////////////////////////////////////////
void  ddi_power_GetDieTemp(int16_t * pLow, int16_t * pHigh)
{
    hw_power_GetDieTemperature(pLow,pHigh);
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Checks to see if the DCDC has been manually enabled
//!
//! \fntype Function
//!
//! \retval  true if DCDC is ON, false if DCDC is OFF.
//!
////////////////////////////////////////////////////////////////////////////////
bool ddi_power_IsDcdcOn(void)
{
    return hw_power_GetEnableDcdc();
}

/*
RtStatus_t ddi_power_SetPowerSources(ddi_power_Source_t eSource)
{
    switch(eSource)
    {
        case DDI_POWER_SOURCE_ALKALINE:

        break;

        case DDI_POWER_SOURCE_LIION:

        break;

        case DDI_POWER_SOURCE_5V:
    }
}
*/
////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}


