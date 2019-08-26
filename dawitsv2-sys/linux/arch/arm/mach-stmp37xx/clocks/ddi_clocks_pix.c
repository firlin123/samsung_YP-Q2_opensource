/////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2008 SigmaTel, Inc.
//
//! \file ddi_clocks_pix.c
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
//! \brief Set the Pix (LCDIF) clock
//!
//! \fntype Function
//!
//! \param[in] u32Freq = Requested clock frequency in kHz
//!
//! \param[out] u32Freq = Actual frequency the clock was set to
//!
//! \retval SUCCESS
//! \retval ERROR_DDI_CLOCKS_INVALID_PIXCLK_FREQ - frequency requested out of range
//!
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetPixClk(uint32_t* u32Freq_kHz)
{
    RtStatus_t rtn;
    uint32_t u32ReqFreq = *u32Freq_kHz;
    uint32_t u32RefPixFreq;
    uint32_t u32IntDiv;
    uint32_t u32PhaseFracDiv;
    bool     bStrict = TRUE;
    bool     bPllUngated = false;

    //--------------------------------------------------------------------------
    // Error Check
    //--------------------------------------------------------------------------
    {
        //----------------------------------------------------------------------
        // Check the requested frequency bounds.  
        //----------------------------------------------------------------------
        if(u32ReqFreq > MAX_PIXCLK || u32ReqFreq < MIN_PIXCLK)
            return ERROR_DDI_CLOCKS_INVALID_PIXCLK_FREQ;
    }


    //--------------------------------------------------------------------------
    // If the Strict setting is requested, check if the frequency can be 
    // achieved with the XTAL and the integer divider.
    //--------------------------------------------------------------------------
    if(bStrict)
    {

        //----------------------------------------------------------------------
        // Exact frequency requested.  Now check if XTAL can get it.  
        //----------------------------------------------------------------------
        uint32_t u32XtalFreq = u32ReqFreq;
        uint32_t u32XtalDiv;
        if(ddi_clocks_CalculateRefXtalDiv(&u32XtalFreq,&u32XtalDiv) == SUCCESS)                    
        {
            
            //------------------------------------------------------------------
            // Is the calculated speed exactly the requested frequency?
            //------------------------------------------------------------------
            if(u32XtalFreq == u32ReqFreq)
            {
                //--------------------------------------------------------------
                // We will use the XTAL to provide the clock.  It will be exact.
                //--------------------------------------------------------------
                bStrict = FALSE;
            }
        }
    }   


    //--------------------------------------------------------------------------
    // Use the PLL for frequencies greater than 24MHz or if we need to be as
    // close as possible to the requested speed and the crystal will not be 
    // exact.
    //--------------------------------------------------------------------------
    if(u32ReqFreq > MIN_PLL_KHZ || bStrict)
    {
        //----------------------------------------------------------------------
        // Check if ref_pix is gated.
        //----------------------------------------------------------------------        
        if(hw_clkctrl_GetPfdRefPixGate())
        {
            //------------------------------------------------------------------
            // Yes it was.  Ungate it because the PLL is going to be used.  Also
            // save the fact the PLL was ungated by the driver in case there is
            // an error.
            //------------------------------------------------------------------ 
            hw_clkctrl_SetPfdRefPixGate(false);
            bPllUngated = true;
        }

        //----------------------------------------------------------------------
        // Calculate the divider values
        //----------------------------------------------------------------------
        u32RefPixFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_PIX);

        if((rtn = ddi_clocks_CalculatePfdAndIntDiv(&u32ReqFreq,
                                                   &u32RefPixFreq, 
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
                // ref_pix was ungated by this function, but there was an
                // error, so return the gate to its previous state.
                //--------------------------------------------------------------
                hw_clkctrl_SetPfdRefPixGate(true);
            }
            return rtn;
        }
    
        
        //----------------------------------------------------------------------        
        // Determine the transition the clock will do.  Since the new frequency
        // requires the PLL, it just matters what source the clock is 
        // currently using.
        //----------------------------------------------------------------------
        if(ddi_clocks_GetBypassRefPix())
        {
            ddi_clocks_TransPixClkXtalToPll(u32IntDiv,u32PhaseFracDiv);
        }
        else
        {
            ddi_clocks_TransPixClkPllToPll(u32IntDiv,u32PhaseFracDiv);
        }     
    

        //----------------------------------------------------------------------
        // Update globals
        //----------------------------------------------------------------------
        g_CurrentClockFreq.PixClk = u32ReqFreq;
        g_CurrentClockFreq.ref_pix = u32RefPixFreq;
    }
    
    //--------------------------------------------------------------------------    
    // Use the 24MHz crystal.
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
        if(ddi_clocks_GetBypassRefPix())
        {
            ddi_clocks_TransPixClkXtalToXtal(u32IntDiv,u32PhaseFracDiv);
        }
        else
        {
            ddi_clocks_TransPixClkPllToXtal(u32IntDiv,u32PhaseFracDiv);
        }

        //----------------------------------------------------------------------
        // Update globals
        //----------------------------------------------------------------------
        g_CurrentClockFreq.PixClk = u32ReqFreq;

    }

    //----------------------------------------------------------------------
    // Return the real frequency we were able to set the clock to.
    //----------------------------------------------------------------------
    *u32Freq_kHz = u32ReqFreq;
    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return the Pix (LCDIF) clock frequency in kHz.
//!
//! \fntype Function
//!
//! \retval Pix clock frequency in kHz
//!
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetPixClk(void)
{
    uint32_t ClkFreq;

    //--------------------------------------------------------------------------
    // If the clock is gated, then the frequency must be 0.                 
    //--------------------------------------------------------------------------
    if(HW_CLKCTRL_PIX.B.CLKGATE)
    {
        return 0;
    }

    //--------------------------------------------------------------------------
    // Determine the clock source.  Bypassing means we bypass the PLL.  All 
    // frequencies will be calculated in kHz. 
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetPixClkBypass())
    {
        //----------------------------------------------------------------------
        // Start with 24MHz
        //----------------------------------------------------------------------
        ClkFreq = XTAL_24MHZ_IN_KHZ;
    }

    else
    {

        //----------------------------------------------------------------------
        // Start with the ref_pix frequency.
        //----------------------------------------------------------------------
        ClkFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_PIX);
    }

    //--------------------------------------------------------------------------
    // Divide by the integer clock divider.
    //--------------------------------------------------------------------------
    ClkFreq /= HW_CLKCTRL_PIX.B.DIV;


    return ClkFreq;
}

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

