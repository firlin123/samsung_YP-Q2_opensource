////////////////////////////////////////////////////////////////////////////////
//! \addtogroup ddi_power
//! @{
//
// Copyright(C) 2005 SigmaTel, Inc.
//
//! \file ddi_power_handoff.c
//! \brief Implementation file for the power driver 5V to DCDC and 
//! DCDC to 5V hanoffs.
//!
////////////////////////////////////////////////////////////////////////////////
//   Includes and external references
////////////////////////////////////////////////////////////////////////////////
#include "ddi_power_common.h"

////////////////////////////////////////////////////////////////////////////////
// Globals & Variables
////////////////////////////////////////////////////////////////////////////////
static bool ddi_power_bDcdcOnDuring5v = false;
ddi_power_PowerHandoff_t   g_ddi_power_Handoff;

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// DCDC and 5V Handoff
////////////////////////////////////////////////////////////////////////////////

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
void ddi_power_Enable5VoltsToBatteryHandoff(void)
{

//	printk("ddi_power_Enable5VoltsToBatteryHandoff\n");
    //--------------------------------------------------------------------------    
    // Enable detection of 5V removal (unplug)
    //--------------------------------------------------------------------------
#if 0 //disable dhsong
    hw_power_Enable5vUnplugDetect(TRUE);
#endif

    //--------------------------------------------------------------------------
    // Enable automatic transition to DCDC
    //--------------------------------------------------------------------------
    
    hw_power_EnableAutoDcdcTransfer(TRUE);
    //hw_power_EnableAutoDcdcTransfer(FALSE); //dhsong
}


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
void ddi_power_Execute5VoltsToBatteryHandoff(void)
{

	//printk("<pm>ddi_power_Execute5VoltsToBatteryHandoff\n");
    //--------------------------------------------------------------------------
    // VDDD has different configurations depending on the battery type and 
    // battery level.  
    //--------------------------------------------------------------------------
    
	//----------------------------------------------------------------------
	// For LiIon battery, we will use the DCDC to power VDDD.
	//----------------------------------------------------------------------        

	hw_power_SetVdddPowerSource(HW_POWER_DCDC_LINREG_READY);

	//--------------------------------------------------------------------------
    // Power VDDA and VDDIO from the DCDC.  
    //--------------------------------------------------------------------------

    hw_power_SetVddaPowerSource(HW_POWER_DCDC_LINREG_READY);
    hw_power_SetVddioPowerSource(HW_POWER_DCDC_LINREG_ON);

    //--------------------------------------------------------------------------    
    // Disable hardware power down when 5V is inserted or removed
    //--------------------------------------------------------------------------

    hw_power_DisableAutoHardwarePowerdown(TRUE);

    //--------------------------------------------------------------------------
    // Re-enable the battery brownout interrupt in case it was disabled.
    //--------------------------------------------------------------------------

    hw_power_EnableBatteryBrownoutInterrupt(TRUE);
}

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
void ddi_power_EnableBatteryTo5VoltsHandoff(void)
{
//	printk("ddi_power_EnableBatteryTo5VoltsHandoff\n");


    //--------------------------------------------------------------------------
    // Enable 5V plug-in detection
    //--------------------------------------------------------------------------
#if 0 //disable dhsong
    hw_power_Enable5vPlugInDetect(TRUE);
#endif
    //--------------------------------------------------------------------------
    // Force current from 5V to be zero by disabling its entry source.
    //--------------------------------------------------------------------------
    hw_power_DisableVddioLinearRegulator(true);

    //--------------------------------------------------------------------------
    // Allow DCDC be to active when 5V is present.
    //--------------------------------------------------------------------------
    hw_power_EnableDcdc(true);

    //hw_power_EnableAutoDcdcTransfer(FALSE); //dhsong
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Transfers the  power source from battery to 5V.
//!
//! \fntype Function
//!
//! This function handles the transitions on each of the power rails necessary
//! to power the chip from the 5V power supply when it was previously powered
//! from the battery power supply.
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_ExecuteBatteryTo5VoltsHandoff(void)
{

	//printk("<pm>ddi_power_ExecuteBatteryTo5VoltsHandoff\n");
    //----------------------------------------------------------------------
    // Disable or Enable the DCDC during 5V connections.
    //----------------------------------------------------------------------
    //hw_power_EnableDcdc(false);
    //hw_power_EnableDcdc(true); //add for always enable dcdc, dhsong

#if 0
    //----------------------------------------------------------------------
    // Use the linear regulators to power the chip.  
    //----------------------------------------------------------------------
    hw_power_SetVdddPowerSource(HW_POWER_LINREG_DCDC_READY);
    hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_READY);
    hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_READY);
#else   //20081229, add to control vddio, vdda with linear regulator when 5v is connected, dhsong
	hw_power_SetVdddPowerSource(HW_POWER_DCDC_LINREG_READY);
        hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_READY);
        hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_READY);
	//add for always using dcdc, dhsong
        //hw_power_SetVdddPowerSource(HW_POWER_DCDC_LINREG_READY);
        //hw_power_SetVddaPowerSource(HW_POWER_DCDC_LINREG_READY);
        //hw_power_SetVddioPowerSource(HW_POWER_DCDC_LINREG_ON);
#endif


    

    //--------------------------------------------------------------------------
    // Disable hardware power down when 5V is inserted or removed
    //--------------------------------------------------------------------------

    hw_power_DisableAutoHardwarePowerdown(TRUE);
}




#if 0 // not used
////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Configures what state the DCDCs will be in during a 5V connection
//!
//! \fntype Function
//!
//! Configures what state the DCDCs will be in during a 5V connection
//! 0 = ON, nonzero = off
//!
////////////////////////////////////////////////////////////////////////////////
void  ddi_power_LeaveDcdcEnabledDuring5v(bool state)
{
    ddi_power_bDcdcOnDuring5v = state;    
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Requests what state the DCDCs will be in during a 5V connection
//!
//! \fntype Function
//!
//! Requests what state the DCDCs will be in during a 5V connection
//!
////////////////////////////////////////////////////////////////////////////////
bool  ddi_power_IsDcdcEnabledDuring5v(void)
{
	//----------------------------------------------------------------------
	//  Let the application decide for LiIon batteries                                                                                    
	//----------------------------------------------------------------------    

	return ddi_power_bDcdcOnDuring5v;
    
}
#endif // not used



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
void ddi_power_Suspend5Volts(void)
{
    //--------------------------------------------------------------------------
    // Go to suspend mode only if 5V connection is present.  
    //--------------------------------------------------------------------------

    if(hw_power_Get5vPresentFlag())
    {
        
        //----------------------------------------------------------------------
        // Don't switch to battery if we are too low or a battery is not
        // present.  
        //----------------------------------------------------------------------
        // TODO: this all needs to move to os_pmi.
        #define MIN_SAFE_BATTERY_VOLTAGE 3000
        if(hw_power_GetBatteryVoltage() >= MIN_SAFE_BATTERY_VOLTAGE)
        {
            //------------------------------------------------------------------
            // We are in 5 Volt mode and need to switch to DCDC power.  Our DCDC 
            // will be active while 5V is present.
            //------------------------------------------------------------------

            hw_power_EnableDcdc(TRUE);


            //------------------------------------------------------------------
            // Switch all the power supplies to use DCDC.
            //------------------------------------------------------------------

            hw_power_SetVdddPowerSource(HW_POWER_DCDC_LINREG_ON);
            hw_power_SetVddaPowerSource(HW_POWER_DCDC_LINREG_ON);
            hw_power_SetVddioPowerSource(HW_POWER_DCDC_LINREG_ON);


            //------------------------------------------------------------------
            // Limit the current pulled through the 5V connection.  Also disable 
            // the VDDIO linear regulator because it is the entry point for 
            // current that comes from the 5V connection.
            //------------------------------------------------------------------

            hw_power_EnableCurrentLimit(TRUE);
            hw_power_DisableVddioLinearRegulator(TRUE);
       }
        
    }
    else
    {
        //--------------------------------------------------------------------------
        // USB 5V is not present.  
        // When the usb cable is pulled, the data pins are removed first
        // and usb suspend is entered so this function is called, then 5V is lost 
        // when the longer 5V pin is also disconnected. Don't halt here for that case.
        //--------------------------------------------------------------------------
    }
    
}

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
void ddi_power_Unsuspend5Volts(void)
{
    //--------------------------------------------------------------------------
    // We can only unsuspend if 5V connection is present.  
    //--------------------------------------------------------------------------

    if(hw_power_Get5vPresentFlag())
    {
        //----------------------------------------------------------------------
        // Transition the player from Battery power to 5 Volt power.  Prepare
        // the player for a 5V-to-Battery handoff.
        //----------------------------------------------------------------------
        ddi_power_ExecuteBatteryTo5VoltsHandoff();
        ddi_power_Enable5VoltsToBatteryHandoff();


        //----------------------------------------------------------------------
        // We are in DCDC mode with 5V connected.  We need to switch back to being
        // powered from the 5V connection.  Enable the VDDIO linear regulator and
        // disable the current limit.
        //----------------------------------------------------------------------
        hw_power_EnableCurrentLimit(FALSE);
        hw_power_DisableVddioLinearRegulator(FALSE);

    }    

    //--------------------------------------------------------------------------
    // If we are here, 5V was not present and this fucntion should not have 
    // been called, or the 5V detection is not working properly.  
    //--------------------------------------------------------------------------
    
    else
    {
        //SystemHalt();
    }
    
}

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief 5V-battery handoff DPC callback function
//!
//! \fntype Function
//! This function will set up the next power handoff,
//! call either the handoff-to-5V callback function which may send out the
//! handoff-to-5V event message if 5V is plugged-in or the handoff-to-battery
//! callback function which may send out the handoff-to-battery event message if
//! the 5V is unplugged. And then it'll call the handoff-end callback function
//! which can reenable the ladder button driver. Inthe end, it enables the 5V
//! interrupt before it returns.
//!
//! \param[in] input       Not used
//!
//! \return None
//!
//! \note This function is only called through a DPC callback function.  If called
//! directly, it must be synchronized with a locking mechanism since the handoff
//! code is not reentrant.  The DPC message queue takes care of this, but not if
//! the function is called directly.  
//!
////////////////////////////////////////////////////////////////////////////////
void ddi_power_Handoff(uint32_t input)
{
    //--------------------------------------------------------------------------
    // Determine if 5V or battery powers the chip.
    //--------------------------------------------------------------------------
    if (ddi_power_Get5vPresent())
    {
        
        //----------------------------------------------------------------------
        // 5V is present, but may not be available for use.  Check now.
        //----------------------------------------------------------------------
        if(ddi_power_5vPowerSourceValid())
        {
            //------------------------------------------------------------------
            // 5V is present and valid for use.  Perform a battery to 5V 
            // handoff, and prepare the chip for a 5V to battery handoff.
            //------------------------------------------------------------------
            /* HOTFIX:: by leeth, except for VDDA, enable Power rail brownout at 20090721 */
            if(input)
            {
                ddi_power_ExecuteBatteryTo5VoltsHandoff();
            }
            else
            {
                hw_power_SetVdddPowerSource(HW_POWER_LINREG_DCDC_READY);
                hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_READY);
                hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_READY);
                hw_power_DisableAutoHardwarePowerdown(TRUE);
            }
            ddi_power_Enable5VoltsToBatteryHandoff();

            //------------------------------------------------------------------
            // Call the handoff to 5V callback function for the application.  
            // (This could send notification of a battery-to-5V handoff to 
            // the system.)
            //------------------------------------------------------------------
            if (g_ddi_power_Handoff.pFxnHandoffTo5VoltCallback != NULL)
                g_ddi_power_Handoff.pFxnHandoffTo5VoltCallback();

        }

        else
        {
            //------------------------------------------------------------------
            // 5V is present, but not valid for use.  Stay on battery power
            // and perform a 5V to battery handoff in case only the 5V validity
            // changed.  Since 5V is still present, prepare the chip for a 5V
            // removal.
            //------------------------------------------------------------------
            ddi_power_Execute5VoltsToBatteryHandoff();
            ddi_power_Enable5VoltsToBatteryHandoff();

            //------------------------------------------------------------------
            // Call the handoff to 5V callback function for the application.  
            // (This could send notification of a 5V-to-battery handoff to the 
            // system.)
            //------------------------------------------------------------------
            if ( g_ddi_power_Handoff.pFxnHandoffToBatteryCallback != NULL)
                g_ddi_power_Handoff.pFxnHandoffToBatteryCallback();

        }

    }

    else
    {
        //----------------------------------------------------------------------
        // For battery power, perform the 5V-to-battery handoff and setup the
        // chip for a possible battery-to-5V handoff.
        //----------------------------------------------------------------------
        ddi_power_Execute5VoltsToBatteryHandoff();
        ddi_power_EnableBatteryTo5VoltsHandoff();


        //----------------------------------------------------------------------
        // Invalidate the 5V source.  The next 5V source is unknown.
        //----------------------------------------------------------------------
        ddi_power_Validate5vPowerSource(false);

        //----------------------------------------------------------------------
        // Call the handoff to 5V callback function for the application.  (This
        // could send notification of a 5V-to-battery handoff to the system.)
        //----------------------------------------------------------------------
        if ( g_ddi_power_Handoff.pFxnHandoffToBatteryCallback != NULL)
            g_ddi_power_Handoff.pFxnHandoffToBatteryCallback();
    }


    //--------------------------------------------------------------------------
    // Call the end of handoff callback function.  (This could send notification
    // that the handoff is complete, maybe through the EOI module.)
    //--------------------------------------------------------------------------
    if (g_ddi_power_Handoff.pFxnHandoffEndCallback != NULL)
        g_ddi_power_Handoff.pFxnHandoffEndCallback();


    //--------------------------------------------------------------------------
    // Reenable the 5V interrupt.
    //--------------------------------------------------------------------------
#if 0 //disable dhsong
    hw_power_Clear5vInterrupt();
#endif
    //hw_power_Enable5vInterrupt(TRUE); //disable 20090105
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
