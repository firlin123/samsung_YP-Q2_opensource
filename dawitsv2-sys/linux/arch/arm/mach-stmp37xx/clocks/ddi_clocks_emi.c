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


/////////////////////////////////////////////////////////////////////////////////
//! 
//! \brief Sets the EmiClk frequencies 
//!
//! \fntype Function
//!
//! This function will set the EmiClk frequency as close as possible to the requested
//! frequency.  The 24MHz crystal will be used for speeds lower than 24MHz. For
//! speeds greater than 24Mhz, the ref_emi clock from the PLL will be used to
//! generate the clock.  The ref_emi's PFD, and the EmiClk's dividers may all be changed.
//!
//! \param[in] pu32ReqFreq - requested EmiClk frequency
//!
//! \param[out] pu32ReqFreq - actual frequency EmiClk was set to
//!
//! \retval ERROR_DDI_CLOCKS_INVALID_EMICLK_FREQ - EmiClk range exceeded
//! \retval SUCCESS 
/////////////////////////////////////////////////////////////////////////////////
#if 0 // not used in linux
RtStatus_t ddi_clocks_SetEmiClk(uint32_t* pu32ReqFreq_kHz)
{
    RtStatus_t rtn;
    uint32_t u32IntDiv;
    uint32_t u32PhaseFracDiv;
    uint32_t u32ReqFreq = *pu32ReqFreq_kHz;
    uint32_t u32RefEmiFreq;
    bool     bPllUngated = false;

    //--------------------------------------------------------------------------
    // Error checks
    //--------------------------------------------------------------------------
    {

        //----------------------------------------------------------------------
    	// Check ReqFreq bounds
        //----------------------------------------------------------------------
	    if(u32ReqFreq > MAX_EMICLK || u32ReqFreq < MIN_EMICLK)
        {
            return ERROR_DDI_CLOCKS_INVALID_EMICLK_FREQ;
	    }

    }


    //--------------------------------------------------------------------------
    // Use the PLL for frequencies greater than 24Mhz
    //--------------------------------------------------------------------------
    if(u32ReqFreq > MIN_PLL_KHZ)
    {

        //----------------------------------------------------------------------
        // Check if ref_emi is gated.
        //----------------------------------------------------------------------
        if(hw_clkctrl_GetPfdRefEmiGate())
        {
            //------------------------------------------------------------------
            // Yes it was.  Ungate it because the PLL is going to be used.  Also
            // save the fact the PLL was ungated by the driver in case there is
            // an error.
            //------------------------------------------------------------------
            hw_clkctrl_SetPfdRefEmiGate(false);
            bPllUngated = true;
        }


        //----------------------------------------------------------------------
        // Read the ref_emi frequency.
        //----------------------------------------------------------------------
        u32RefEmiFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_EMI);


        //----------------------------------------------------------------------
        // Calculate the divider values
        //----------------------------------------------------------------------
        if((rtn = ddi_clocks_CalculatePfdAndIntDiv(&u32ReqFreq,
                                                   &u32RefEmiFreq, 
                                                   &u32IntDiv,
                                                   &u32PhaseFracDiv,
                                                   TRUE)) != SUCCESS)
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
                hw_clkctrl_SetPfdRefEmiGate(true);
            }

            //------------------------------------------------------------------
            // Return the error from the divider calcualtion.
            //------------------------------------------------------------------
            return rtn;
        }

        //----------------------------------------------------------------------        
        // Determine the transition the clock will do.  Since the new frequency
        // requires the PLL, it just matters what source the clock is 
        // currently using.
        //----------------------------------------------------------------------        
        if(ddi_clocks_GetBypassRefEmi())
        {
            if((rtn = ddi_clocks_TransEmiClkXtalToPll(u32IntDiv,u32PhaseFracDiv)) != SUCCESS)
                return rtn;
        }
        else
        {
            if((rtn = ddi_clocks_TransEmiClkPllToPll(u32IntDiv,u32PhaseFracDiv)) != SUCCESS)
                return rtn;
        }

        //----------------------------------------------------------------------        
        // Update globals
        //----------------------------------------------------------------------        
        g_CurrentClockFreq.EmiClk = u32ReqFreq;
        g_CurrentClockFreq.ref_emi = u32RefEmiFreq;
    }


    //--------------------------------------------------------------------------
    // Use the 24Mhz crystal for frequencies less than or equal to 24MHz  
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
        if(ddi_clocks_GetBypassRefEmi())
        {
            if((rtn = ddi_clocks_TransEmiClkXtalToXtal(u32IntDiv,u32PhaseFracDiv)) != SUCCESS)
                return rtn;
        }
        else
        {
            if((rtn = ddi_clocks_TransEmiClkPllToXtal(u32IntDiv,u32PhaseFracDiv)) != SUCCESS)
                return rtn;                   
        }

        //----------------------------------------------------------------------
        // Update globals
        //----------------------------------------------------------------------
        g_CurrentClockFreq.EmiClk = u32ReqFreq;
    }

    //--------------------------------------------------------------------------
    // Return the new EMI clock frequency and the success status.
    //--------------------------------------------------------------------------
    *pu32ReqFreq_kHz = u32ReqFreq;
    return SUCCESS;
}
#endif // not used in linux

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gets the EmiCk frequency
//!
//! \fntype Function
//!
//! \retval EmiClk frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetEmiClk(void)
{
    uint32_t ClkFreq;

    //--------------------------------------------------------------------------
    // Determine the clock source.  Bypassing means we bypass the PLL.  All 
    // frequencies will be calculated in kHz. 
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetEmiClkBypass())

    {
        //----------------------------------------------------------------------
        // If the clock is gated, then the frequency must be 0.                 
        //----------------------------------------------------------------------
        if(HW_CLKCTRL_EMI.B.CLKGATE)
        {
            return 0;
        }

        //----------------------------------------------------------------------
        // Start with 24MHz
        //----------------------------------------------------------------------
        ClkFreq = XTAL_24MHZ_IN_KHZ;

        //----------------------------------------------------------------------
        // Divide by crystal divider.
        //----------------------------------------------------------------------
        ClkFreq /= HW_CLKCTRL_EMI.B.DIV_XTAL;
    }

    else
    {

        //----------------------------------------------------------------------
        // Start with the ref_emi frequency.
        //----------------------------------------------------------------------
        ClkFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_EMI);

        //----------------------------------------------------------------------
        // Divide by ref_emi divider. 
        //----------------------------------------------------------------------
        ClkFreq /= HW_CLKCTRL_EMI.B.DIV_EMI;

    }

    return ClkFreq;
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Sets flag to keep PLL power on.
//!
//! \fntype Function
//!
//! \retval EmiClk frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
void ddi_clocks_KeepPllPowered(bool bPowered)
{
    //--------------------------------------------------------------------------
    // The application can choose to keep the PLL on even if current clock
    // frequencies do not require it to generate their clocks.  
    //--------------------------------------------------------------------------
    bKeepPllPowered = bPowered;

    //--------------------------------------------------------------------------
    // Turn on the PLL if it was requested on.  It the request is to turn off
    // the PLL, the clock driver must turn it off because other clocks may
    // be depending on it now.  
    //--------------------------------------------------------------------------        
    if(bPowered)
    {
        hw_clkctrl_PowerPll(TRUE);    
    }        
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Sets flag to keep PLL power on.
//!
//! \fntype Function
//!
//! \retval EmiClk frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
bool ddi_clocks_PllPoweredForApp(void)
{
    //--------------------------------------------------------------------------
    // The application can choose to keep the PLL on even if current clock
    // frequencies do not require it to generate their clocks.  
    //--------------------------------------------------------------------------
    return bKeepPllPowered;    
}
/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

