/////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2005 SigmaTel, Inc.
//
//! \file ddi_clocks.c
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Includes
/////////////////////////////////////////////////////////////////////////////////
#include "../include/types.h"
#include "../include/error.h"

//#include "registers/regsclkctrl.h"
#include "hw_clocks.h"
#include "ddi_clocks.h"
#include "ddi_clocks_errordefs.h"
#include "ddi_clocks_internal.h"
#include <asm/hardware.h>
/////////////////////////////////////////////////////////////////////////////////
// Externs
/////////////////////////////////////////////////////////////////////////////////
extern ddi_clocks_Clocks_t g_CurrentClockFreq;

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

    ////////////////////////////////
    // Error checks
    ////////////////////////////////

    // Is requested frequency zero?
    if(u32ReqFreq == 0)
    {
        return ERROR_DDI_CLOCKS_GENERAL;
    }


    ////////////////////////////////
    // Set the new dividers
    ////////////////////////////////
    
    // Use the PLL for frequencies greater than 24Mhz
    if(u32ReqFreq > MIN_PLL_KHZ)
    {

        u32RefIoFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_IO);
        
        // Calculate the divider values
        if((rtn = ddi_clocks_CalculatePfdAndIntDiv(&u32ReqFreq,
                                                   &u32RefIoFreq, 
                                                   &u32IntDiv,
                                                   &u32PhaseFracDiv,
                                                   bChangeRefIo)) != SUCCESS)
        {
            return rtn;
        }
        
        // Which transition do we do?
        if(ddi_clocks_GetBypassRefIoSsp())
        {
            ddi_clocks_TransSspClkXtalToPll(u32IntDiv,u32PhaseFracDiv,bChangeRefIo);        
        }
        else
        {
            ddi_clocks_TransSspClkPllToPll(u32IntDiv,u32PhaseFracDiv,bChangeRefIo);        
        }

        // Update globals
        g_CurrentClockFreq.SspClk = u32ReqFreq;
        g_CurrentClockFreq.ref_io = u32RefIoFreq;         
    }
    // Use the 24Mhz crystal.
    else
    {
        // Calculate the divider and frequency.  
        if((rtn = ddi_clocks_CalculateRefXtalDiv(&u32ReqFreq,&u32IntDiv)) != SUCCESS)
            return rtn;

        // Which transition do we do?
        if(ddi_clocks_GetBypassRefIoSsp())
        {
            ddi_clocks_TransSspClkXtalToXtal(u32IntDiv,u32PhaseFracDiv);
        }
        else
        {
            ddi_clocks_TransSspClkPllToXtal(u32IntDiv,u32PhaseFracDiv);
        }

        // Update globals
        g_CurrentClockFreq.SspClk = u32ReqFreq;
    }

    *u32Freq_kHz = u32ReqFreq;
    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gets the SspClk frequency
//!
//! \fntype Function
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
//! \brief Sets the IR and IR overdrive (IROV) clock to the given frequency
//!
//! \fntype Function
//!
//! This function will set the IR and IROV clock divider and possibly the ref_io clock
//! If bChangeRefIo is TRUE, the ref_io clock will be changed so the IR clock
//! frequency will be as close as possible to the requested frequency.  The IROV 
//! clock is divided off of the IR clock.  The ref_io PFD, IR, and IROV clock 
//! divider will possibly change.  When bChangeRefIo is FALSE, the function will 
//! use the current ref_io speed and only change the GPMI clock divider to get 
//! as close as possible to the requested frequency.
//!
//! \param[in] u32IrFreq - The requested IR clock frequency
//! \param[in] u32IrovFreq - The requested IROV clock frequency
//! \param[in] bChangeRefIo - TRUE to allow ref_io clock to change.  
//!                           FALSE to use current ref_io
//!
//! \param[out] u32IrFreq - The actual frequency the IR clock was set to
//! \param[out] u32IrovFreq - The actual frequency the IROV clock was set to
//!
//! \retval ERROR_DDI_CLOCKS_GENERAL - Do not request zero clock frequencies
//! \retval SUCCESS

/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetIrClk(uint32_t* u32IrFreq_kHz, uint32_t* u32IrovFreq_kHz, bool bChangeRefIo)
{
    RtStatus_t rtn;
    uint32_t u32ReqIrFreq = *u32IrFreq_kHz;
    uint32_t u32ReqIrovFreq = *u32IrovFreq_kHz;
    uint32_t u32RefIoFreq;
    uint32_t u32IrDiv,u32IrovDiv,u32PhaseFracDiv;


    ////////////////////////////////
    // Error checks
    ////////////////////////////////

    // Is requested frequency zero?
    if((u32ReqIrFreq == 0) || (u32ReqIrovFreq == 0))
    {
        return ERROR_DDI_CLOCKS_GENERAL;
    }

// We get freq and this checks dividers.  Need to fix this...
#if 0
    // Is requested IR frequency in range?
    if((u32ReqIrFreq > MAX_IR_DIV) || (u32ReqIrFreq < MIN_IR_DIV))
    {
        return ERROR_DDI_CLOCKS_GENERAL;    
    }

    // Is requested IROV frequency in range?
    if((u32ReqIrovFreq > MAX_IROV_DIV) || (u32ReqIrovFreq < MIN_IROV_DIV))
    {
        return ERROR_DDI_CLOCKS_GENERAL;    
    }
#endif

    ////////////////////////////////
    // Set the new dividers
    ////////////////////////////////

    // Use the PLL for frequencies greater than 24Mhz
    if(u32ReqIrovFreq > MIN_PLL_KHZ)
    {
        u32RefIoFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_IO);

        // Calculate the divider values
        if((rtn = ddi_clocks_CalculatePfdAndIntDiv(&u32ReqIrovFreq,
                                                   &u32RefIoFreq, 
                                                   &u32IrovDiv,
                                                   &u32PhaseFracDiv,
                                                   bChangeRefIo)) != SUCCESS)
        {
            return rtn;
        }

        // IR clock is divided from the IROV clock 
        u32IrDiv = u32ReqIrovFreq/u32ReqIrFreq;
        u32ReqIrFreq = u32ReqIrovFreq/u32IrDiv;

        // Which transition do we do?
        if(ddi_clocks_GetBypassRefIoIr())
        {
            ddi_clocks_TransIrClkXtalToPll(u32IrDiv,u32IrovDiv,u32PhaseFracDiv,bChangeRefIo);
        }
        else
        {
            ddi_clocks_TransIrClkPllToPll(u32IrDiv,u32IrovDiv,u32PhaseFracDiv,bChangeRefIo);
        }        

        // Update globals
        g_CurrentClockFreq.IrClk = u32ReqIrFreq;
        g_CurrentClockFreq.IrovClk = u32ReqIrovFreq;
        g_CurrentClockFreq.ref_io = u32RefIoFreq;
    }
    // Use the 24Mhz crystal.
    else
    {
        // Calculate the divider and frequency.
        if((rtn = ddi_clocks_CalculateRefXtalDiv(&u32ReqIrovFreq,&u32IrovDiv)) != SUCCESS)
            return rtn;

         // IR clock is divided from the IROV clock 
        u32IrDiv = u32ReqIrovFreq/u32ReqIrFreq;
        u32ReqIrFreq = u32ReqIrovFreq/u32IrDiv;
        
        // Which transition do we do?
        if(ddi_clocks_GetBypassRefIoIr())
        {
            ddi_clocks_TransIrClkXtalToXtal(u32IrDiv,u32IrovDiv);
        }
        else
        {
            ddi_clocks_TransIrClkPllToXtal(u32IrDiv,u32IrovDiv);
        }

        // Update globals
        g_CurrentClockFreq.IrClk = u32ReqIrFreq;
        g_CurrentClockFreq.IrovClk = u32ReqIrovFreq;
    }

    *u32IrFreq_kHz = u32ReqIrFreq;
    *u32IrovFreq_kHz = u32ReqIrovFreq;
    return SUCCESS;

}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gets the IrClk frequency
//!
//! \fntype Function
//!
//! \retval IrClk frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetIrClk(void)
{
    uint32_t ClkFreq;

    //--------------------------------------------------------------------------
    // The IrClk source is IrovClk and its frequency is a ratio of IrovClk.
    //--------------------------------------------------------------------------    
    {
        //----------------------------------------------------------------------
        // Start with the IrovClk frequency.  
        //----------------------------------------------------------------------
        ClkFreq = ddi_clocks_GetIrovClk();

        //----------------------------------------------------------------------
        // Divide by the IR divider.
        //----------------------------------------------------------------------
        ClkFreq /= HW_CLKCTRL_IR.B.IR_DIV;
    }

    return ClkFreq;

}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gets the IrovClk frequency
//!
//! \fntype Function
//!
//! \retval IrovClk frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetIrovClk(void)
{
{
    uint32_t ClkFreq;

    //--------------------------------------------------------------------------
    // If the clock is gated, then the frequency must be 0.                 
    //--------------------------------------------------------------------------
    if(HW_CLKCTRL_IR.B.CLKGATE)
    {
        return 0;
    }

    //--------------------------------------------------------------------------
    // Determine the clock source.  Bypassing means we bypass the PLL.  All 
    // frequencies will be calculated in kHz. 
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetIrClkBypass())
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
    ClkFreq /= HW_CLKCTRL_IR.B.IROV_DIV;


    return ClkFreq;
}
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set the ref_io clock from the PLL
//!
//! \fntype Function
//!
//! \param[in] u32Freq = Requested clock frequency in kHz (247 - 480 MHz)
//! \param[in] eClk = One of four PLL clocks to change. Valid inputs are 
//!                   PLL_REF_CPU, PLL_REF_EMI, PLL_REF_PIX, or PLL_REF_IO
//!
//! \param[out] u32Freq = Actual frequency the clock was set to
//!
//! \retval SUCCESS
//! \retval ERROR_DDI_CLOCKS_INVALID_PIXCLK_FREQ - frequency requested out of range
//!
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetPllRefClk(ddi_clocks_pll_ref_clks_t eClk, uint32_t* u32Freq_kHz)
{
    uint32_t u32ReqFreq = *u32Freq_kHz;
    uint32_t u32NewFreq;
    uint32_t u32Div;

    if((u32ReqFreq > MAX_PFD_FREQ_KHZ) || (u32ReqFreq < MIN_PFD_FREQ_KHZ))
    {
        return ERROR_DDI_CLOCKS_INVALID_PLL_FREQ ;
    }    

    // DIV = (480000kHz * 18)/ReqFreq
    u32Div = PFD_CONSTANT/u32ReqFreq;
    
    // NewFreq = 480000kHz * (18/DIV)
    u32NewFreq = PFD_CONSTANT/u32Div;

    // Adjust for rounding.  We need the frequency closest to, but not 
    // surpassing the requested frequency.   
    if(u32NewFreq > u32ReqFreq)
    {
        u32Div++;
        u32NewFreq = PFD_CONSTANT/u32Div;
    }        

    // Check the PFD value we are going to set.
    if((u32Div < MIN_PFD_VALUE) || (u32Div > MAX_PFD_VALUE))
    {
        return ERROR_DDI_CLOCKS_INVALID_PFD_DIV;
    }
    
    // Set PLL ref clock
    switch(eClk)
    {
        case PLL_REF_IO:
            hw_clkctrl_SetPfdRefIoGate(FALSE);
            hw_clkctrl_SetPfdRefIoDiv(u32Div);
            g_CurrentClockFreq.ref_io = u32NewFreq;
        break;

        case PLL_REF_CPU:
        case PLL_REF_EMI:
        case PLL_REF_PIX:
        default:
            // Need to define these cases...
            return ERROR_DDI_CLOCKS_GENERAL;
    }

    *u32Freq_kHz = u32NewFreq;
    return SUCCESS;        
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set the Pix (LCDIF) clock
//!
//! \fntype Function
//!
//! \param[in] u32Freq = Requested clock frequency in kHz
//! \param[in] bStrict = TRUE - Pix Clock will always try to be exact even under 24Mhz
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

    //--------------------------------------------------------------------------
    // Error Check: Check the requested frequency bounds.  
    //--------------------------------------------------------------------------
    
    if(u32ReqFreq > MAX_PIXCLK || u32ReqFreq < MIN_PIXCLK)
        return ERROR_DDI_CLOCKS_INVALID_PIXCLK_FREQ;


    //--------------------------------------------------------------------------
    // If the Strict setting is requested, we need to check if we can achieve
    // the frequency with the XTAL and the integer divider.
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
    // Use the PLL for frequencies greater than 24Mhz or if we need to be as
    // close as possible to the requested speed.
    //--------------------------------------------------------------------------
    if(u32ReqFreq > MIN_PLL_KHZ || bStrict)
    {
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
            return rtn;
        }
        
        //----------------------------------------------------------------------
        // Transistion from XTAL to PLL or PLL to PLL
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
    // Use the 24Mhz crystal.
    //--------------------------------------------------------------------------
    else 
    {
        //----------------------------------------------------------------------        
        // Calculate the divider and frequency.  
        //----------------------------------------------------------------------
        if((rtn = ddi_clocks_CalculateRefXtalDiv(&u32ReqFreq,&u32IntDiv)) != SUCCESS)
            return rtn;

        //----------------------------------------------------------------------
        // Transistion from XTAL to XTAL or PLL to XTAL
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

    *u32Freq_kHz = u32ReqFreq;
    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the Pix (LCDIF) clock
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
//! \brief Set the SAIF clock
//!
//! \fntype Function
//!
//! \param[in] u32Freq = New frequency in kHz
//!
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetSaifClk(uint32_t* u32Freq_kHz)
{
    //uint32_t u32ReqFreq = *u32Freq_kHz;
    //uint32_t u32Div;

    // Fractional Divide Logic needed       

     return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set the PCMSPDIF clock
//!
//! \fntype Function
//!
//! \param[in] u32Freq = New frequency in kHz
//!
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetSpdifClk(uint32_t* u32Freq_kHz)
{
    // Datasheet reserved
    // Needs function definition
    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

