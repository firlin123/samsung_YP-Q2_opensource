/////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2007-2008 SigmaTel, Inc.
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
ddi_clocks_Clocks_t g_CurrentClockFreq;

/////////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Code
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! 
//! \brief Sets the Pclk and Hclk frequencies 
//!
//! \fntype Function
//!
//! This function will set the Pclk frequency as close as possible to the requested
//! frequency.  The 24MHz crystal will be used for speeds lower than 24MHz. For
//! speeds greater than 24MHz, the ref_cpu clock from the PLL will be used to
//! generate the clock.  The ref_cpu's PFD, and the Pclk's dividers may all be changed.
//! The Hclk divider will be set and its speed will be a ratio of Pclk.
//!
//! \param[in] pu32ReqFreq - requested Pclk frequency in kHz
//! \param[in] u32HclkDiv - PCLK to HCLK integer ratio
//!
//! \param[out] pu32ReqFreq - actual kHz frequency Pclk was set to
//!
//! \retval ERROR_DDI_CLOCKS_INVALID_PCLK_FREQ - Pclk range exceeded
//! \retval SUCCESS 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetPclkHclk(uint32_t* pu32ReqFreq_kHz, uint32_t u32HclkDiv)
{
    RtStatus_t rtn;
    uint32_t u32IntDiv;
    uint32_t u32PhaseFracDiv;
    uint32_t u32CurFreq;
    uint32_t u32ReqFreq = *pu32ReqFreq_kHz;
    uint32_t u32RefCpuFreq;
    bool bPerformHclkDivFirst;
    bool bPerformHclkDivLast;
    bool     bPllUngated = false;

    //--------------------------------------------------------------------------
    // Error checks
    //--------------------------------------------------------------------------
    {

        //----------------------------------------------------------------------
        // Check ReqFreq bounds.
        //----------------------------------------------------------------------
        if(u32ReqFreq > MAX_PCLK || u32ReqFreq < MIN_PCLK)
        {
            return ERROR_DDI_CLOCKS_INVALID_PCLK_FREQ;
        }
        
        //----------------------------------------------------------------------
        // Check HCLK divider.
        //----------------------------------------------------------------------
        if(u32HclkDiv < MIN_HCLK_DIV || u32HclkDiv > MAX_HCLK_DIV)
        {
            return ERROR_DDI_CLOCKS_INVALID_HCLK_DIV;
        }

    }

    //--------------------------------------------------------------------------
    // Read the current clock frequency.
    //--------------------------------------------------------------------------
    u32CurFreq = ddi_clocks_GetPclk();


    //--------------------------------------------------------------------------
    // Determine when to change HClk.  If we increase PClk, we need to change 
    // HClk first to avoid a momentary high speed HClk.
    //--------------------------------------------------------------------------
    if(u32ReqFreq > u32CurFreq)
    {
//			printk("[PM], %s, %d\n\n", __FILE__, __LINE__); //dhsong
        bPerformHclkDivFirst = true;
        bPerformHclkDivLast = false; //dhsong
    }
    else
    {
//			printk("[PM], %s, %d\n\n", __FILE__, __LINE__); //dhsong
        bPerformHclkDivLast = true;
        bPerformHclkDivFirst = false; //dhsong
    }

#if 1 //disable dhsong, system down when usb inserted
    //--------------------------------------------------------------------------
    // Change HCLK before PCLK
    //--------------------------------------------------------------------------
    if(bPerformHclkDivFirst)
    {
			//printk("[PM], %s, %d\n\n", __FILE__, __LINE__); //dhsong
        if((rtn = hw_clkctrl_SetHclkDiv(u32HclkDiv,FALSE)) != SUCCESS)
            return rtn;
    }
#endif

    //--------------------------------------------------------------------------
    // Use the PLL for frequencies greater than 24Mhz
    //--------------------------------------------------------------------------
    if(u32ReqFreq > MIN_PLL_KHZ)
    {

        //----------------------------------------------------------------------
        // Check if ref_cpu is gated.
        //----------------------------------------------------------------------
        if(hw_clkctrl_GetPfdRefCpuGate())
        {
            //------------------------------------------------------------------
            // Yes it was.  Ungate it because the PLL is going to be used.  Also
            // save the fact the PLL was ungated by the driver in case there is
            // an error.
            //------------------------------------------------------------------
            hw_clkctrl_SetPfdRefCpuGate(false);
            bPllUngated = true;
        }


        //----------------------------------------------------------------------
        // Read the ref_cpu frequency.
        //----------------------------------------------------------------------
        u32RefCpuFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_CPU);


        //----------------------------------------------------------------------
        // Calculate the divider values
        //----------------------------------------------------------------------
        if((rtn = ddi_clocks_CalculatePfdAndIntDiv(&u32ReqFreq,
                                                   &u32RefCpuFreq, 
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
                // ref_cpu was ungated by this function, but there was an
                // error, so return the gate to its previous state.
                //--------------------------------------------------------------    
                hw_clkctrl_SetPfdRefCpuGate(true);
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
        if(ddi_clocks_GetBypassRefCpu())
        {
            if((rtn = ddi_clocks_TransPclkXtalToPll(u32IntDiv,u32PhaseFracDiv)) != SUCCESS)
                return rtn;
        }
        else
        {
            if((rtn = ddi_clocks_TransPclkPllToPll(u32IntDiv,u32PhaseFracDiv)) != SUCCESS)
                return rtn;
        }

        //----------------------------------------------------------------------        
        // Update globals
        //----------------------------------------------------------------------        
        g_CurrentClockFreq.Pclk = u32ReqFreq;
        g_CurrentClockFreq.Hclk = u32ReqFreq/u32HclkDiv;
        g_CurrentClockFreq.ref_cpu = u32RefCpuFreq;
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
        if(ddi_clocks_GetBypassRefCpu())
        {
            if((rtn = ddi_clocks_TransPclkXtalToXtal(u32IntDiv,u32PhaseFracDiv)) != SUCCESS)
                return rtn;
        }
        else
        {
            if((rtn = ddi_clocks_TransPclkPllToXtal(u32IntDiv,u32PhaseFracDiv)) != SUCCESS)
                return rtn;
        }


        //----------------------------------------------------------------------
        // Update globals
        //----------------------------------------------------------------------
        g_CurrentClockFreq.Pclk = u32ReqFreq;
        g_CurrentClockFreq.Hclk = u32ReqFreq/u32HclkDiv;
    }


    //--------------------------------------------------------------------------
    // Change HCLK after PCLK.
    //--------------------------------------------------------------------------
    if(bPerformHclkDivLast)
    {
        if((rtn = hw_clkctrl_SetHclkDiv(u32HclkDiv,FALSE)) != SUCCESS)
            return rtn;
    }


    //--------------------------------------------------------------------------
    // Return the new PCLK frequency and the success status.
    //--------------------------------------------------------------------------
    *pu32ReqFreq_kHz = u32ReqFreq;
    return SUCCESS;

}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the current Pclk frequency in kHz 
//!
//! \fntype Function
//!
//! \retval current Pclk frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetPclk(void)
{
    uint32_t ClkFreq;

    //--------------------------------------------------------------------------
    // Determine the clock source.  Bypassing means we bypass the PLL.  All 
    // frequencies will be calculated in kHz. 
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetPclkBypass())
    {

        //----------------------------------------------------------------------
        // Start with 24MHz
        //----------------------------------------------------------------------
        ClkFreq = XTAL_24MHZ_IN_KHZ;

        //----------------------------------------------------------------------
        // Divide by crystal divider.
        //----------------------------------------------------------------------
        ClkFreq /= HW_CLKCTRL_CPU.B.DIV_XTAL;

    }
    else
    {

        //----------------------------------------------------------------------
        // Start with the ref_cpu frequency.
        //----------------------------------------------------------------------
        ClkFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_CPU);

        //----------------------------------------------------------------------
        // Divide by ref_cpu divider. 
        //----------------------------------------------------------------------
        ClkFreq /= HW_CLKCTRL_CPU.B.DIV_CPU;

    }

    return ClkFreq;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the current Hclk frequency in kHz 
//!
//! \fntype Function
//!
//! \retval current Hclk frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetHclk(void)
{
    uint32_t ClkFreq;

    //--------------------------------------------------------------------------
    // The HCLK source is PCLK and its frequency is a ratio of PCLK.  
    //--------------------------------------------------------------------------    
    {
        //----------------------------------------------------------------------
        // Start with the PCLK frequency.  
        //----------------------------------------------------------------------
        ClkFreq = ddi_clocks_GetPclk();

        //----------------------------------------------------------------------
        // Divide by the PCLK-to-HCLK ratio divider.
        //----------------------------------------------------------------------
        ClkFreq /= HW_CLKCTRL_HBUS.B.DIV;
    }

    return ClkFreq;

}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the current Hclk auto-slow frequency in kHz 
//!
//! \fntype Function
//!
//! \retval current Hclk auto-slow frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetHclkSlow(void)
{
    uint32_t ClkFreq;

    //--------------------------------------------------------------------------
    // The slow HCLK source is HCLK when auto-slow is enabled.  
    //--------------------------------------------------------------------------    
    {
        //----------------------------------------------------------------------
        // Start with the fast HCLK frequency.
        //----------------------------------------------------------------------
        ClkFreq = ddi_clocks_GetHclk();

        //----------------------------------------------------------------------
        // Divide by the IR divider.
        //----------------------------------------------------------------------
        // mooji: SDK bug
        // ClkFreq /= HW_CLKCTRL_HBUS.B.SLOW_DIV;
        ClkFreq >>= HW_CLKCTRL_HBUS.B.SLOW_DIV;
    }

    return ClkFreq;

}

   
/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

