/////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2008 SigmaTel, Inc.
//
//! \file ddi_clocks_gpmi.c
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
// Enumerates
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
// Code
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the GPMI clock to the given frequency
//!
//! \fntype Function
//!
//! This function will set the GPMI clock divider and possibly the ref_io clock
//! If bChangeRefIo is TRUE, the ref_io clock will be changed so the GPMI clock
//! frequency will be as close as possible to the requested frequency.  The ref_io
//! PFD and the GPMI clock divider will possibly change.  When bChangeRefIo is 
//! FALSE, the function will use the current ref_io speed and only change the 
//! GPMI clock divider to get as close as possible to the requested frequency.
//!
//! \param[in] u32Freq The requested GPMI clock frequency.
//! \param[in] bChangeRefIo TRUE to allow ref_io clock to change, FALSE to 
//! use current ref_io
//!
//! \param[out] u32Freq The actual frequency the GPMI clock was set to.
//!
//! \retval ERROR_DDI_CLOCKS_GENERAL - Do not request zero clock frequencies.
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetGpmiClk(uint32_t* u32Freq_kHz, bool bChangeRefIo)
{
    RtStatus_t rtn;
    uint32_t u32ReqFreq = *u32Freq_kHz;
    uint32_t u32RefIoFreq;
    uint32_t u32IntDiv,u32PhaseFracDiv;
    bool     bPllUngated = false;

    //--------------------------------------------------------------------------
    // Error checks
    //--------------------------------------------------------------------------
    {
        //----------------------------------------------------------------------    
        // Driver currently does not accept frequency requests of zero.  This 
        // may change in the future and be the way a clock is turned off.
        //----------------------------------------------------------------------
        if(u32ReqFreq == 0)
        {
            return ERROR_DDI_CLOCKS_ZERO_FREQ_REQUEST;
        }
    }

    //--------------------------------------------------------------------------
    // Use the PLL for frequencies greater than 24MHz.
    //--------------------------------------------------------------------------
    if(u32ReqFreq > MIN_PLL_KHZ)
    {
        //----------------------------------------------------------------------
        // Check if ref_io is gated.
        //----------------------------------------------------------------------
        if(hw_clkctrl_GetPfdRefIoGate())
        {
            //------------------------------------------------------------------
            // Yes it was.  Ungate it because the PLL is going to be used.  Also
            // save the fact the PLL was ungated by the driver in case there is
            // an error.
            //------------------------------------------------------------------
            hw_clkctrl_SetPfdRefIoGate(false);
            bPllUngated = true;
        }


        //----------------------------------------------------------------------
        // Use the PLL for frequencies greater than 24MHz.  Calculate the 
        // PFD and integer dividers.  
        //----------------------------------------------------------------------
        u32RefIoFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_IO);
        
        if((rtn = ddi_clocks_CalculatePfdAndIntDiv(&u32ReqFreq,
                                                   &u32RefIoFreq, 
                                                   &u32IntDiv,
                                                   &u32PhaseFracDiv,
                                                   bChangeRefIo)) != SUCCESS)
        {
            //------------------------------------------------------------------
            // If here, there was an error calculating the dividers.
            //------------------------------------------------------------------
            if(bPllUngated)
            {
                //--------------------------------------------------------------
                // ref_io was ungated by this function, but there was an
                // error, so return the gate to its previous state.
                //--------------------------------------------------------------
                hw_clkctrl_SetPfdRefIoGate(true);
            }
            return rtn;
        }

        //----------------------------------------------------------------------
        // Determine the transistion to use.  We know we need to use PLL for
        // the new frequency.  We'll check the PLL bypass to see if we are 
        // currently using the PLL.  
        //----------------------------------------------------------------------
        if(hw_clkctrl_GetGpmiClkBypass())
        {
            //------------------------------------------------------------------
            // Transistion from crystal to PLL.  
            //------------------------------------------------------------------
            ddi_clocks_TransGpmiClkXtalToPll(u32IntDiv,
                                             u32PhaseFracDiv,
                                             bChangeRefIo);
        }
        else
        {
            //------------------------------------------------------------------
            // Transistion PLL to PLL.  
            //------------------------------------------------------------------
            ddi_clocks_TransGpmiClkPllToPll(u32IntDiv,
                                            u32PhaseFracDiv,
                                            bChangeRefIo);        
        }

        //----------------------------------------------------------------------
        // Update globals
        //----------------------------------------------------------------------
        g_CurrentClockFreq.GpmiClk = u32ReqFreq;
        g_CurrentClockFreq.ref_io = u32RefIoFreq;

    }

    else
    {
        //----------------------------------------------------------------------
        // Use the 24MHz crystal as the source.  Calculate the divider 
        // and frequency.
        //----------------------------------------------------------------------
        if((rtn = ddi_clocks_CalculateRefXtalDiv(&u32ReqFreq,
                                                 &u32IntDiv)) != SUCCESS)
        {
            return rtn;
        }

        //----------------------------------------------------------------------
        // We know we need to use the crystal for the new frequency.  Check the
        // PLL bypass to see if we are currently using the PLL and use this
        // to decide which transistion to use.  
        //----------------------------------------------------------------------
        if(hw_clkctrl_GetGpmiClkBypass())
        {
            //------------------------------------------------------------------
            // Transition crystal to crystal.
            //------------------------------------------------------------------
            ddi_clocks_TransGpmiClkXtalToXtal(u32IntDiv, u32PhaseFracDiv);
        }
        else
        {
            //------------------------------------------------------------------
            // Transistion PLl to crystal.  
            //------------------------------------------------------------------
            ddi_clocks_TransGpmiClkPllToXtal(u32IntDiv, u32PhaseFracDiv);
        }

        //----------------------------------------------------------------------
        // Update globals
        //----------------------------------------------------------------------
        g_CurrentClockFreq.GpmiClk = u32ReqFreq;
    }

    //----------------------------------------------------------------------
    // Return the real frequency we were able to set the clock to.
    //----------------------------------------------------------------------
    *u32Freq_kHz = u32ReqFreq;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the GPMI clock frequency in kHz
//!
//! \fntype Function
//!
//! \retval GpmiClk frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetGpmiClk(void)
{
    uint32_t ClkFreq;

    //--------------------------------------------------------------------------
    // If the clock is gated, then the frequency must be 0.                 
    //--------------------------------------------------------------------------
    if(HW_CLKCTRL_GPMI.B.CLKGATE)
    {
        return 0;
    }

    //--------------------------------------------------------------------------
    // Determine the clock source.  Bypassing means we bypass the PLL.  All 
    // frequencies will be calculated in kHz. 
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetGpmiClkBypass())

    {
        //----------------------------------------------------------------------
        // Start with 24MHz
        //----------------------------------------------------------------------
        ClkFreq = XTAL_24MHZ_IN_KHZ;
    }

    else

    {
        //----------------------------------------------------------------------
        // Start with the ref_io frequency.
        //----------------------------------------------------------------------
        ClkFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_IO);
    }


    //--------------------------------------------------------------------------
    // Divide by the integer clock divider.
    //--------------------------------------------------------------------------
    ClkFreq /= HW_CLKCTRL_GPMI.B.DIV;


    return ClkFreq;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions GpmiClk between two frequencies that both use the 24MHz crystal. 
//!
//! \fntype Function
//!
//!
//! \param[in] u32IntDiv divider for GpmiClk's ref_io divider
//! \param[in] u32PhaseFracDiv Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransGpmiClkXtalToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate the GPMI clock divider. 
    //--------------------------------------------------------------------------
    hw_clkctrl_SetGpmiClkGate(FALSE);

    //--------------------------------------------------------------------------
    // Set the GPMI integer divider.
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetGpmiClkDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;

    //--------------------------------------------------------------------------
    // Done. 
    //--------------------------------------------------------------------------
    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions GpmiClk from the 24MHz crystal to PLL
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv divider for GpmiClk's ref_io divider
//! \param[in] u32PhaseFracDiv divider value for ref_io's PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransGpmiClkXtalToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo)
{
    RtStatus_t rtn;
    
    //--------------------------------------------------------------------------
    // Ungate the PLL ref_io and GPMI clock gates.
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefIoGate(FALSE);
    hw_clkctrl_SetGpmiClkGate(FALSE);

    //--------------------------------------------------------------------------
    // Change GPMI clock divider.  
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetGpmiClkDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;

    //--------------------------------------------------------------------------
    // Change PLL ref_io divider if requested.  
    //--------------------------------------------------------------------------
    if(bChangeRefIo)
    {
        hw_clkctrl_SetPfdRefIoDiv(u32PhaseFracDiv);
    }

    //--------------------------------------------------------------------------
    // The dividers and gates are set up.  Now we can switch the bypass to
    // use the PLL.  
    //--------------------------------------------------------------------------
    if((rtn = ddi_clocks_BypassRefIoGpmi(FALSE)) != SUCCESS)
        return rtn;

    //--------------------------------------------------------------------------
    // Set flag so other clocks know this clock is using the PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(GPMICLK,TRUE);

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions GpmiClk between two frequencies that both use the PLL.
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv divider for GpmiClk's ref_io divider
//! \param[in] u32PhaseFracDiv divider value for ref_io PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransGpmiClkPllToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate both PFD and integer divider clock gates in case we are coming
    // from a GPMI off state.  
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefIoGate(FALSE);
    hw_clkctrl_SetGpmiClkGate(FALSE);

    //--------------------------------------------------------------------------
    // Change GPMI divider.
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetGpmiClkDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;

    //--------------------------------------------------------------------------
    // Change the ref_io clock if it was requested.  
    //--------------------------------------------------------------------------
    if(bChangeRefIo)
    {
        hw_clkctrl_SetPfdRefIoDiv(u32PhaseFracDiv);
    }

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions GpmiClk from the PLL to 24MHz crystal
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv divider for GpmiClk's ref_io divider
//! \param[in] u32PhaseFracDiv Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransGpmiClkPllToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;
    bool bRefIoGated = FALSE;

    //--------------------------------------------------------------------------
    // Ungate both clock gates because we can't switch the bypass if one is 
    // gated.  The ref_io gate could be set because we might be coming from
    // a powered off state for GPMI.  
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetPfdRefIoGate())
    {
        bRefIoGated = TRUE;
        hw_clkctrl_SetPfdRefIoGate(FALSE);
    }
    hw_clkctrl_SetGpmiClkGate(FALSE);

    //--------------------------------------------------------------------------
    // Change the integer divider.  
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetGpmiClkDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;
                    
    //--------------------------------------------------------------------------
    // Switch the PLL bypass to use the crystal.  
    //--------------------------------------------------------------------------
    if((rtn = ddi_clocks_BypassRefIoGpmi(TRUE)) != SUCCESS)
        return rtn;

    //--------------------------------------------------------------------------
    // Put ref_io gate back the way we found it.
    //--------------------------------------------------------------------------
    if(bRefIoGated)
    {
        hw_clkctrl_SetPfdRefIoGate(TRUE);
    }

    //--------------------------------------------------------------------------
    // Set flag so other clocks know this clock is no longer using the PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(GPMICLK,FALSE);

    //--------------------------------------------------------------------------
    // Try to power off the PLL.  
    //--------------------------------------------------------------------------
    ddi_clocks_PowerOffPll();

    return SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

