/////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2005 SigmaTel, Inc.
//
//! \file ddi_clocks.c
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Includes
/////////////////////////////////////////////////////////////////////////////////
#include "ddi_clocks_common.h"

/////////////////////////////////////////////////////////////////////////////////
// Externs
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Code
/////////////////////////////////////////////////////////////////////////////////

bool ddi_clocks_PowerOffPll(void)
{

    //--------------------------------------------------------------------------
    // Check if the application wants to keep the PLL on. 
    //--------------------------------------------------------------------------
    if(bKeepPllPowered)
    {
        //----------------------------------------------------------------------
        // Leave the PLL on.
        //----------------------------------------------------------------------
        hw_clkctrl_PowerPll(TRUE);
        return FALSE;

    }

    //--------------------------------------------------------------------------
    // Check if any of the clocks are currently using the PLL.
    //--------------------------------------------------------------------------
    if(u32PllClients)
    {
        //----------------------------------------------------------------------
        // At least one clock is using the PLL.  
        //----------------------------------------------------------------------
        hw_clkctrl_PowerPll(TRUE);
        return FALSE;
    }

    //--------------------------------------------------------------------------
    // No one is using the PLL so it is safe to turn it off. 
    //--------------------------------------------------------------------------    
    hw_clkctrl_PowerPll(FALSE);


    //--------------------------------------------------------------------------
    // Enable some other power savings
    //--------------------------------------------------------------------------    
    //hw_power_EnableHalfFets(TRUE);
    //hw_power_EnableRcScale(HW_POWER_RCSCALE_DISABLED);

    return TRUE;

}

void ddi_clocks_ClockUsingPll(ddi_clocks_PllClients_t eClient, bool bPllInUse)
{
    //--------------------------------------------------------------------------
    // Set or clear the client number in our PLL user variable.  
    //--------------------------------------------------------------------------

    if(bPllInUse)
    {
        //----------------------------------------------------------------------
        // Logical OR with the client bitmask to set the bit representing 
        // the client.  
        //----------------------------------------------------------------------
        u32PllClients |= eClient;

        //----------------------------------------------------------------------
        // Remove the additional power savings because we are turning the PLL on
        // again. 
        //----------------------------------------------------------------------
        //hw_power_EnableHalfFets(FALSE);
        //hw_power_EnableRcScale(HW_POWER_RCSCALE_4X_INCR);

    }
    else
    {
        //----------------------------------------------------------------------
        // Logical AND to clear the bit representing the client with the inverted
        // client bitmask.  
        //----------------------------------------------------------------------
        u32PllClients &= ~eClient;
    }
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Allows the USB to keep the PLL powered or allow the driver to 
//! turn it off. 
//!
//! \fntype Function
//!
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_UsbPowerOnPll(bool bPowerOn)
{

    //--------------------------------------------------------------------------
    // The USB driver can either power on or power off the PLL.
    //--------------------------------------------------------------------------
    if(bPowerOn)
    {
        //----------------------------------------------------------------------
        // Let the driver know the USB needs the PLL on.  
        //----------------------------------------------------------------------
        ddi_clocks_ClockUsingPll(USB,TRUE);
    }

    else
    {
        //----------------------------------------------------------------------
        // Let the driver know the USB does not need the PLL on anymore.
        //----------------------------------------------------------------------
        ddi_clocks_ClockUsingPll(USB,FALSE);

        //----------------------------------------------------------------------
        // Try to power off the PLL.  
        //----------------------------------------------------------------------
        ddi_clocks_PowerOffPll();
    }

    return SUCCESS;
}



uint32_t ddi_clocks_GetPll(void)
{
    return 480000;
}

bool ddi_clocks_GetPllStatus(void)
{
    return HW_CLKCTRL_PLLCTRL0.B.POWER;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set the minimum PFD value for PLL dividers.
//!
//! \fntype Function
//!
//! \param[in] u16MinPfd - valid values are 18-35 inclusive.
//! \retval 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetMaxPllRefFreq(uint32_t u32MaxFreq)
{
    //--------------------------------------------------------------------------
    // Make sure the requested frequency is attainable using the PFD.
    //--------------------------------------------------------------------------

    if((u32MaxFreq <= MAX_PLL_KHZ) && (u32MaxFreq >= MIN_PLL_KHZ))
    {
        uint32_t u32Temp;
                
        //----------------------------------------------------------------------        
        // Calculate the PFD value needed to achieve the requested frequency.
        // We'll mulitply the max PLL (480MHz) by the hardware's minimum PFD (18)
        // and then divide by the requested maximum frequency to get the 
        // software's minimum PFD.  This eliminates any fractional dividing.  
        //----------------------------------------------------------------------

        u32Temp = (MAX_PLL_KHZ * MIN_PFD_VALUE)/u32MaxFreq;


        //----------------------------------------------------------------------
        // We need to round up the PFD value if divide was not exact.  Otherwise,
        // the maximum frequency we allow will be greater than the maximum 
        // frequency requested.  
        //----------------------------------------------------------------------

        if(((MAX_PLL_KHZ * MIN_PFD_VALUE)%u32MaxFreq) != 0)
        {
            u32Temp++;
        }    


        //----------------------------------------------------------------------
        // Save the PFD value for later calculations, but only if it is within 
        // the valid PFD value range.  We are done so return successfully.
        //----------------------------------------------------------------------

        if((u32Temp >= MIN_PFD_VALUE) && (u32Temp <= MAX_PFD_VALUE))
        {
            u8MinPfdValue = (uint8_t) u32Temp;
            u32MaxPllFreq = u32MaxFreq;
            return SUCCESS;
        }
    }


    //--------------------------------------------------------------------------
    // The PLL cannout output this frequency using the PFD's.  
    //--------------------------------------------------------------------------
    return ERROR_DDI_CLOCKS_INVALID_PLL_FREQ;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return the software's maximum ref_clk from PLL
//!
//! \fntype Function
//!
//! \retval Max PLL reference clock frequency in kHz.  
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetMaxPllRefFreq(void)
{
    return u32MaxPllFreq;
}

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

