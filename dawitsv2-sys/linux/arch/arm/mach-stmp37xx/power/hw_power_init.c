////////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_power
//! @{
//
// Copyright (c) 2004 - 2007 SigmaTel, Inc.
//
//! \file hw_power_init.c
//! \brief Contains init function for the hw power layer
//! \version 0.1
//! \date 04/2007
//!
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes and external references
////////////////////////////////////////////////////////////////////////////////
#include "hw_power_common.h"

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Code							  
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_Init(void)
{
    RtStatus_t rtn = SUCCESS;
    
    //--------------------------------------------------------------------------
    // Make the clock registers accessible by ungating power block
    //--------------------------------------------------------------------------
    BF_CLR(POWER_CTRL, CLKGATE);

    //--------------------------------------------------------------------------
    // Improve efficieny and reduce transient ripple
    //--------------------------------------------------------------------------
    
    {
        //----------------------------------------------------------------------
        // Change stepping voltage only after the diff control loop
        // has toggled as well.
        //----------------------------------------------------------------------
        BF_SET(POWER_LOOPCTRL, TOGGLE_DIF);

        //----------------------------------------------------------------------
        // Enable the commom mode and differential mode hysterisis
        //----------------------------------------------------------------------
        BF_SET(POWER_LOOPCTRL, EN_CM_HYST);
        BF_SET(POWER_LOOPCTRL, EN_DF_HYST);
        


        //----------------------------------------------------------------------
        // To run with a lower battery voltage, adjust the duty 
        // cycle positive limit
        //----------------------------------------------------------------------
        BF_WR(POWER_DCLIMITS, POSLIMIT_BUCK, 0x30);


       
    }

    //--------------------------------------------------------------------------
    // Done.
    //--------------------------------------------------------------------------

    return rtn;
}

RtStatus_t hw_power_InitBatteryMonitor (uint32_t u32SampleInterval)
{
    RtStatus_t rtn;

	// 
	// BATT conversion using DELAY channel 3
	// 
	BF_WR(LRADC_CONVERSION, SCALE_FACTOR, 2);
	// Enable the automatic update mode of BATT_VALUE field in HW_POWER_MONITOR register
	BF_SET(LRADC_CONVERSION, AUTOMATIC);

	// Disable the divide-by-two of a LRADC channel
	BF_CLRV(LRADC_CTRL2, DIVIDE_BY_TWO, (1 << 7));
	// Clear the accumulator & NUM_SAMPLES
	HW_LRADC_CHn_CLR(7, 0xFFFFFFFF);
	// Disable the accumulation of a LRADC channel
	BF_CLRn(LRADC_CHn, 7, ACCUMULATE);

	// schedule a conversion before the setting up of the delay channel
	// so the user can have a good value right after the function returns
	BF_SETV(LRADC_CTRL0, SCHEDULE, (1 << 7));

	// Setup the trigger loop forever,
	BF_SETVn(LRADC_DELAYn, 3, TRIGGER_LRADCS, (1<<7));
	BF_SETVn(LRADC_DELAYn, 3, TRIGGER_DELAYS, (1<<3));

	BF_WRn(LRADC_DELAYn, 3, LOOP_COUNT, 0);
	BF_WRn(LRADC_DELAYn, 3, DELAY, u32SampleInterval);	// 0.5*N msec on 2khz

	// Clear the accumulator & NUM_SAMPLES
	HW_LRADC_CHn_CLR(7, 0xFFFFFFFF);
	// Kick off the LRADC battery measurement
	BF_SETn(LRADC_DELAYn, 3, KICK);

    //--------------------------------------------------------------------------
    // Update the DCFUNCV register for battery adjustment
    //--------------------------------------------------------------------------
    hw_power_UpdateDcFuncvVddd();
    hw_power_UpdateDcFuncvVddio();


    //--------------------------------------------------------------------------
    // Finally enable the battery adjust
    //--------------------------------------------------------------------------
    BF_SET(POWER_BATTMONITOR,EN_BATADJ);
                   
    //--------------------------------------------------------------------------
    // 4X increase to transient load response.  Enable this after the 
    // battery voltage is correct.      
    //--------------------------------------------------------------------------
	//----------------------------------------------------------------------
	// The following settings give optimal power supply capability and 
	// efficiency.  Extreme loads will need HALF_FETS cleared and 
	// possibly DOUBLE_FETS set.  The below setting are probably also 
	// the best for alkaline mode also but more characterization is 
	// needed to know for sure.
	//----------------------------------------------------------------------
	
	//----------------------------------------------------------------------
	// Increase the RCSCALE_THRESHOLD
	//----------------------------------------------------------------------
	BF_SET(POWER_LOOPCTRL, RCSCALE_THRESH);
	
	//----------------------------------------------------------------------
	// Increase the RCSCALE level for quick DCDC response to dynamic load
	//----------------------------------------------------------------------            
	hw_power_EnableRcScale(HW_POWER_RCSCALE_8X_INCR);

	
	//----------------------------------------------------------------------
	// Enable half fets for increase efficiency.
	//----------------------------------------------------------------------
	hw_power_EnableHalfFets(TRUE);
        
        
    return SUCCESS;
}

RtStatus_t hw_power_InitPowerSupplies(void)
{
    RtStatus_t Status;

    //----------------------------------------------------------------------
    // Make sure the power supplies are configured for their power sources.
    // This sets the LINREG_OFFSET field correctly for each power supply.  
    //---------------------------------------------------------------------

    if(hw_power_Get5vPresentFlag())
    {
        //------------------------------------------------------------------
        // For 5V connection, use LinRegs.
        //------------------------------------------------------------------
        if((Status = hw_power_SetVdddPowerSource(HW_POWER_LINREG_DCDC_OFF)) != SUCCESS)
            return Status;
        if((Status = hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_OFF)) != SUCCESS)
            return Status;
        if((Status = hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_OFF)) != SUCCESS)
            return Status;
    }
    
    else
    {
        //------------------------------------------------------------------
        // For LiIon battery, all the rails can start off as DCDC mode.
        //------------------------------------------------------------------
        if((Status = hw_power_SetVdddPowerSource(HW_POWER_DCDC_LINREG_READY)) != SUCCESS)
            return Status;
        if((Status = hw_power_SetVddaPowerSource(HW_POWER_DCDC_LINREG_READY)) != SUCCESS)
            return Status;
        if((Status = hw_power_SetVddioPowerSource(HW_POWER_DCDC_LINREG_READY)) != SUCCESS)
            return Status;
    }
    //--------------------------------------------------------------------------
    // Done.
    //--------------------------------------------------------------------------
    return SUCCESS;
}

#if 0 // not use in linux
RtStatus_t hw_power_InitFiq(void)
{
    //--------------------------------------------------------------------------
    // Clear the brownout interrupts.
    //--------------------------------------------------------------------------

    hw_power_ClearVdddBrownoutInterrupt();
    hw_power_ClearVddaBrownoutInterrupt();
    hw_power_ClearVddioBrownoutInterrupt();
    hw_power_ClearBatteryBrownoutInterrupt();

    //--------------------------------------------------------------------------
    // Enable the power supply to assert brownout interrupts.
    //--------------------------------------------------------------------------

    hw_power_EnableBatteryBrownoutInterrupt(TRUE);
    hw_power_EnableVdddBrownoutInterrupt(TRUE);
    hw_power_EnableVddaBrownoutInterrupt(TRUE);
    hw_power_EnableVddioBrownoutInterrupt(TRUE);

    //--------------------------------------------------------------------------
    // We can handle brownouts now.  Don't allow the chip to power itself off
    // if it experiences a brownout.  We'll manage it in software.
    //--------------------------------------------------------------------------
    
    hw_power_DisableBrownoutPowerdown();

    return SUCCESS;

}
#endif // not used in linux

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

