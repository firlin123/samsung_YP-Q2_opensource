/////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2008 SigmaTel, Inc.
//
//! \file ddi_clocks_ssp.c
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
//! \brief Sets the SSP clock to the given frequency
//!
//! \fntype Function
//!
//! This function will set the SSP clock divider and possibly the ref_io clock
//! If bChangeRefIo is TRUE, the ref_io clock will be changed so the SSP clock
//! frequency will be as close as possible to the requested frequency.  The ref_io
//! PFD and the SSP clock divider will possibly change.  When bChangeRefIo is FALSE,
//! the function will use the current ref_io speed and only change the SSP clock 
//! divider to get as close as possible to the requested frequency.
//!
//! \param[in] u32Freq - The requested SSP clock frequency
//! \param[in] bChangeRefIo - TRUE to allow ref_io clock to change.  
//!                           FALSE to use current ref_io
//!
//! \param[out] u32Freq - The actual frequency the SSP clock was set to
//!
//! \retval ERROR_DDI_CLOCKS_GENERAL - Do not request zero clock frequencies
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetSspClk(uint32_t* u32Freq_kHz, bool bChangeRefIo)
{
    RtStatus_t rtn;
    uint32_t u32ReqFreq = *u32Freq_kHz;
    uint32_t u32RefIoFreq;
    uint32_t u32IntDiv;
    uint32_t u32PhaseFracDiv;
    bool     bPllUngated = false;

    //--------------------------------------------------------------------------
    // Error checks
    //--------------------------------------------------------------------------
    {

        //----------------------------------------------------------------------
        // Is requested frequency zero?
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
        // Check if ref_io is ungated first.
        //----------------------------------------------------------------------
        if(hw_clkctrl_GetPfdRefIoGate())
        {
            //------------------------------------------------------------------
            // No, ungate it because it will probably be ungated.
            //------------------------------------------------------------------
            hw_clkctrl_SetPfdRefIoGate(false);
            bPllUngated = true;
        }


        //----------------------------------------------------------------------
        // Read the ref_io clock frequency.
        //----------------------------------------------------------------------
        u32RefIoFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_IO);

        
        //----------------------------------------------------------------------
        // Calculate the divider values.
        //----------------------------------------------------------------------
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
        // Determine the transition the clock will do.  Since the new frequency
        // requires the PLL, it just matters what source the clock is 
        // currently using.
        //----------------------------------------------------------------------
        if(ddi_clocks_GetBypassRefIoSsp())
        {
            ddi_clocks_TransSspClkXtalToPll(u32IntDiv,u32PhaseFracDiv,bChangeRefIo);        
        }
        else
        {
            ddi_clocks_TransSspClkPllToPll(u32IntDiv,u32PhaseFracDiv,bChangeRefIo);        
        }


        //----------------------------------------------------------------------
        // Update globals.
        //----------------------------------------------------------------------
        g_CurrentClockFreq.SspClk = u32ReqFreq;
        g_CurrentClockFreq.ref_io = u32RefIoFreq;         

    }


    //--------------------------------------------------------------------------
    // Use the crystal for frequencies less than or equal to 24MHz.
    //--------------------------------------------------------------------------
    else
    {
        //----------------------------------------------------------------------
        // Calculate the divider and frequency.  
        //----------------------------------------------------------------------
        if((rtn = ddi_clocks_CalculateRefXtalDiv(&u32ReqFreq,&u32IntDiv)) != SUCCESS)
            return rtn;


        //----------------------------------------------------------------------        
        // Determine the transition the clock will do.  Since the new frequency
        // requires the crystal, it just matters what source the clock is 
        // currently using.
        //----------------------------------------------------------------------
        if(ddi_clocks_GetBypassRefIoSsp())
        {
            ddi_clocks_TransSspClkXtalToXtal(u32IntDiv,u32PhaseFracDiv);
        }
        else
        {
            ddi_clocks_TransSspClkPllToXtal(u32IntDiv,u32PhaseFracDiv);
        }

        //----------------------------------------------------------------------
        // Update globals.
        //----------------------------------------------------------------------
        g_CurrentClockFreq.SspClk = u32ReqFreq;
    }


    //----------------------------------------------------------------------
    // Return the real frequency we were able to set the clock to.
    //----------------------------------------------------------------------
    *u32Freq_kHz = u32ReqFreq;
    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gets the SspClk frequency
//!
//! \fntype Function
//!
//! This function returns the currect SSP clock frequency in kHz.
//!
//! \retval SspClk frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetSspClk(void)
{
    uint32_t ClkFreq;

    //--------------------------------------------------------------------------
    // If the clock is gated, then the frequency must be 0.                 
    //--------------------------------------------------------------------------
    if(HW_CLKCTRL_SSP.B.CLKGATE)
    {
        return 0;
    }

    //--------------------------------------------------------------------------
    // Determine the clock source.  Bypassing means we bypass the PLL.  All 
    // frequencies will be calculated in kHz. 
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetSspClkBypass())

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
    ClkFreq /= HW_CLKCTRL_SSP.B.DIV;


    return ClkFreq;
}


/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

