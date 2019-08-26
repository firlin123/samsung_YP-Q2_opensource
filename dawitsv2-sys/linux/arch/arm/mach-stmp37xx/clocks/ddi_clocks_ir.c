/////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2008 SigmaTel, Inc.
//
//! \file ddi_clocks_ir.c
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
    bool     bPllUngated = false;


    //--------------------------------------------------------------------------
    // Error checks
    //--------------------------------------------------------------------------
    {
        //----------------------------------------------------------------------
        // Is requested frequency zero?
        //----------------------------------------------------------------------
        if((u32ReqIrFreq == 0) || (u32ReqIrovFreq == 0))
        {
            return ERROR_DDI_CLOCKS_ZERO_FREQ_REQUEST;
        }

        // We get freq and this checks dividers.  Need to fix this...
        #if 0
        //----------------------------------------------------------------------
        // Is requested IR frequency in range?
        //----------------------------------------------------------------------
        if((u32ReqIrFreq > MAX_IR_DIV) || (u32ReqIrFreq < MIN_IR_DIV))
        {
            return ERROR_DDI_CLOCKS_GENERAL;    
        }

        //----------------------------------------------------------------------
        // Is requested IROV frequency in range?
        //----------------------------------------------------------------------
        if((u32ReqIrovFreq > MAX_IROV_DIV) || (u32ReqIrovFreq < MIN_IROV_DIV))
        {
            return ERROR_DDI_CLOCKS_GENERAL;    
        }
        #endif
    }

    //--------------------------------------------------------------------------
    // Use the PLL for frequencies greater than 24Mhz
    //--------------------------------------------------------------------------
    if(u32ReqIrovFreq > MIN_PLL_KHZ)
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
        // Read the ref_io frequency.
        //----------------------------------------------------------------------
        u32RefIoFreq = ddi_clocks_GetPllRefClkFreq(PLL_REF_IO);


        //----------------------------------------------------------------------
        // Calculate the divider values
        //----------------------------------------------------------------------
        if((rtn = ddi_clocks_CalculatePfdAndIntDiv(&u32ReqIrovFreq,
                                                   &u32RefIoFreq, 
                                                   &u32IrovDiv,
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
        // IR clock is divided from the IROV clock 
        //----------------------------------------------------------------------
        u32IrDiv = u32ReqIrovFreq/u32ReqIrFreq;
        u32ReqIrFreq = u32ReqIrovFreq/u32IrDiv;


        //----------------------------------------------------------------------        
        // Determine the transition the clock will do.  Since the new frequency
        // requires the PLL, it just matters what source the clock is 
        // currently using.
        //----------------------------------------------------------------------
        if(ddi_clocks_GetBypassRefIoIr())
        {
            ddi_clocks_TransIrClkXtalToPll(u32IrDiv,u32IrovDiv,u32PhaseFracDiv,bChangeRefIo);
        }
        else
        {
            ddi_clocks_TransIrClkPllToPll(u32IrDiv,u32IrovDiv,u32PhaseFracDiv,bChangeRefIo);
        }        


        //----------------------------------------------------------------------
        // Update globals.
        //----------------------------------------------------------------------
        g_CurrentClockFreq.IrClk = u32ReqIrFreq;
        g_CurrentClockFreq.IrovClk = u32ReqIrovFreq;
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
        if((rtn = ddi_clocks_CalculateRefXtalDiv(&u32ReqIrovFreq,&u32IrovDiv)) != SUCCESS)
            return rtn;


        //----------------------------------------------------------------------
        // IR clock is divided from the IROV clock 
        //----------------------------------------------------------------------
        u32IrDiv = u32ReqIrovFreq/u32ReqIrFreq;
        u32ReqIrFreq = u32ReqIrovFreq/u32IrDiv;

        
        //----------------------------------------------------------------------        
        // Determine the transition the clock will do.  Since the new frequency
        // requires the crystal, it just matters what source the clock is 
        // currently using.
        //----------------------------------------------------------------------
        if(ddi_clocks_GetBypassRefIoIr())
        {
            ddi_clocks_TransIrClkXtalToXtal(u32IrDiv,u32IrovDiv);
        }
        else
        {
            ddi_clocks_TransIrClkPllToXtal(u32IrDiv,u32IrovDiv);
        }


        //----------------------------------------------------------------------
        // Update globals.
        //----------------------------------------------------------------------
        g_CurrentClockFreq.IrClk = u32ReqIrFreq;
        g_CurrentClockFreq.IrovClk = u32ReqIrovFreq;
    }

    //--------------------------------------------------------------------------
    // Return the new IR and IROV clock frequencies and the success status.
    //--------------------------------------------------------------------------
    *u32IrFreq_kHz = u32ReqIrFreq;
    *u32IrovFreq_kHz = u32ReqIrovFreq;
    return SUCCESS;

}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the IrClk frequency in kHz
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
//! \brief Returns the IrovClk frequency in kHz
//!
//! \fntype Function
//!
//! \retval IrovClk frequency in kHz
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetIrovClk(void)
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


/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

