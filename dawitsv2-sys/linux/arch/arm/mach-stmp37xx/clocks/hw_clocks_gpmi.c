/////////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_clocks
//! @{
//
// Copyright(C) 2007 SigmaTel, Inc.
//
//! \file hw_clocks_gpmi.c
//! \brief Routines for GPMI clock.  Placed in their own file for code paging.
//!
//! \date October 2007
//!
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
//  Includes and external references
/////////////////////////////////////////////////////////////////////////////////

#include "hw_clocks_common.h"

/////////////////////////////////////////////////////////////////////////////////
// Externs
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Code
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate GpmiClk
//! 
//! \fntype Function                        
//!
//! CLK_GPMI Gate. If set to 1, CLK_GPMI is gated off. 0:CLK_GPMI is not gated. 
//! When this bit is modified, or when it is high, the DIV field should 
//! not change its value. The DIV field can change ONLY when this clock
//! gate bit field is low.
//!
//! \param[in] bClkGate TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetGpmiClkGate(bool bClkGate)
{
    //--------------------------------------------------------------------------
    // Gate or ungated the GPMI clock
    //--------------------------------------------------------------------------
    if(bClkGate)
        BF_SET(CLKCTRL_GPMI,CLKGATE);
    else
        BF_CLR(CLKCTRL_GPMI,CLKGATE);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set GpmiClk divider
//! 
//! \fntype Function                        
//!
//! The GPMI clock frequency is determined by dividing the selected reference 
//! clock (ref_xtal or ref_io) by the value in this bit field. This field 
//! can be programmed with a new value only when CLKGATE = 0.
//!
//! \param[in] u32Div Divider in range 1 to 1023
//! \param[in] bDivFracEn TRUE to use fractional divide, FALSE to use integer divide
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - GpmiClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - upstream ref clock is gated
//! \retval SUCCESS - GpmiClk divider set
//! 
//! \notes The divider is set to divide by 1 at power-on reset. Do NOT divide by 0.
//! \notes HW_CLKCTRL_GPMI.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero. 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetGpmiClkDiv(uint32_t u32Div, bool bDivFracEn)
{
    //--------------------------------------------------------------------------
    // Error checks
    //--------------------------------------------------------------------------
    {
        //----------------------------------------------------------------------
        // Cannot set divider to zero.
        //----------------------------------------------------------------------
        if( u32Div == 0 )
            return ERROR_HW_CLKCTRL_DIV_BY_ZERO;

        //----------------------------------------------------------------------
        // Return if busy with another divider change.
        //----------------------------------------------------------------------
        if( HW_CLKCTRL_GPMI.B.BUSY )
            return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

        //----------------------------------------------------------------------
        // Only change DIV_FRAC_EN and DIV when CLKGATE is clear.
        //----------------------------------------------------------------------
        if(HW_CLKCTRL_GPMI.B.CLKGATE)
            return ERROR_HW_CLKCTRL_CLK_GATED;

        //----------------------------------------------------------------------
        // Ensure upstream reference clock gate is ungated if using ref_io.
        //----------------------------------------------------------------------
        if(HW_CLKCTRL_FRAC.B.CLKGATECPU && !HW_CLKCTRL_CLKSEQ.B.BYPASS_GPMI)
            return ERROR_HW_CLKCTRL_REF_CLK_GATED;
    }

    //--------------------------------------------------------------------------
    // Passed error checks.  Now set the divider.
    //--------------------------------------------------------------------------
    {
        //----------------------------------------------------------------------
        // Set the divider type.
        //----------------------------------------------------------------------        
        if( bDivFracEn )
        {
            //------------------------------------------------------------------
            // Use the divider as a fractional divider.
            //------------------------------------------------------------------
            BF_SET(CLKCTRL_GPMI,DIV_FRAC_EN);
        }
        else    
        {
            //------------------------------------------------------------------
            // Use the divider as an integer divider.
            //------------------------------------------------------------------
            BF_CLR(CLKCTRL_GPMI,DIV_FRAC_EN);
        }
    
        //----------------------------------------------------------------------
        // Set divider value.
        //----------------------------------------------------------------------
        HW_CLKCTRL_GPMI.B.DIV = u32Div;
    }

    //--------------------------------------------------------------------------
    // Everything set successfully.  
    //--------------------------------------------------------------------------
    return SUCCESS;    

}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get GpmiClk divider
//! 
//! \fntype Function                        
//!
//! Returns the GPMI clock divider value
//!
//! \retval Current GPMI clock divider value
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetGpmiClkDiv(void)
{
    return HW_CLKCTRL_GPMI.B.DIV;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate PLL output through PFD for ref_io
//! 
//! \fntype Function                        
//!
//! IO Clock Gate. If set to 1, the IO fractional divider clock
//! (reference PLL ref_io) is off (power savings). 0: IO
//! fractional divider clock is enabled.
//!
//! \param[in] bClkGate TRUE to gate, FALSE to ungate 
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPfdRefIoGate(bool bClkGate)
{
    //--------------------------------------------------------------------------
    // Gate or ungate the ref_io clock
    //--------------------------------------------------------------------------
    if(bClkGate)
        BF_SET(CLKCTRL_FRAC,CLKGATEIO);
    else
        BF_CLR(CLKCTRL_FRAC,CLKGATEIO);

    //--------------------------------------------------------------------------
    // Wait for the clock to settle.
    //--------------------------------------------------------------------------
    while(HW_CLKCTRL_GPMI.B.BUSY);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the gate status for ref_io in the PFD
//! 
//! \fntype Function                        
//! 
//! \retval - TRUE if gated, FALSE if ungated
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdRefIoGate(void)
{
    return HW_CLKCTRL_FRAC.B.CLKGATEIO;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PFD for ref_io
//! 
//! \fntype Function                        
//!
//! This field controls the IO clocks fractional divider. The
//! resulting frequency shall be 480 * (18/IOFRAC) where
//! IOFRAC = 18-35.
//!
//! \param[in] u32Div Divider in range 18 to 35
//! 
//! \notes Divider = 18 will disable PFD circuit to save power since 18/18 is 
//! \notes equal to 1.  The output will be 480MHz.
//!
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - can't change divider when clock is gated
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPfdRefIoDiv(uint32_t u32Div)
{
    //--------------------------------------------------------------------------
    // Don't change divider if clock is gated
    //--------------------------------------------------------------------------
    if(HW_CLKCTRL_FRAC.B.CLKGATEIO)
        return ERROR_HW_CLKCTRL_REF_CLK_GATED;
    
    //--------------------------------------------------------------------------
    // Set divider for IO clock PFD
    //--------------------------------------------------------------------------
    HW_CLKCTRL_FRAC.B.IOFRAC = u32Div;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return divider value for ref_io
//! 
//! \fntype Function                        
//! 
//! \retval - divider value in range 18 - 35
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPfdRefIoDiv(void)
{
    return HW_CLKCTRL_FRAC.B.IOFRAC;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for GpmiClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_xtal generates GpmiClk
//! \retval FALSE - ref_io generates GpmiClk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetGpmiClkBypass(void)
{
    return HW_CLKCTRL_CLKSEQ.B.BYPASS_GPMI;
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

