/////////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_clocks
//! @{
//
// Copyright(C) 2007 SigmaTel, Inc.
//
//! \file hw_clocks.c
//! \brief Master clocks (PLL, PCLK, HCLK, XCLK) and module clocks (EMI, SSP,
//! \brief GPMI, IROV, IR, PIX, SAIF, etc..) hardware interface routines.
//!
//! \date March 2007
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
//! \brief Set clock gates for various clocks with only ref_xtal as source
//! 
//! \fntype Function                        
//!
//! Controls various fixed-rate divider clocks working off the 24.0-MHz crystal clock.
//!
//! \param[in] eClk - Clock to gate/ungate.  Valid inputs are: UART_CLK_GATE,
//!                   FILT_CLK24M_GATE, PWM_CLK24M_GATE, DRI_CLK24M_GATE,
//!                   DIGCTRL_CLK1M_GATE, TIMROT_CLK32K_GATE
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
//!
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetXtalClkGate(hw_clkctrl_xtal_gate_t eClk, bool bClkGate)
{
    // Gate or ungate the specified clock
    if(bClkGate)
        HW_CLKCTRL_XTAL_SET(eClk);
    else
        HW_CLKCTRL_XTAL_CLR(eClk);
}
     
/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate PixClk
//! 
//! \fntype Function                        
//!
//! CLK_PIX Gate. If set to 1, CLK_PIX is gated off. 0:CLK_PIX is not gated. 
//! When this bit is modified, or when it is high, the DIV field should 
//! not change its value. The DIV field can change ONLY when this clock
//! gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPixClkGate(bool bClkGate)
{
    // Gate/Ungate the PIX clock
    if(bClkGate)
        BF_SET(CLKCTRL_PIX,CLKGATE);
    else
        BF_CLR(CLKCTRL_PIX,CLKGATE);

    // Wait for the clock to settle.
    while(HW_CLKCTRL_PIX.B.BUSY);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PixClk divider
//! 
//! \fntype Function                        
//!
//! The Pixel clock frequency is determined by dividing the selected reference 
//! clock (ref_xtal or ref_pix) by the value in this bit field. This field can 
//! be programmed with a new value only when CLKGATE = 0.
//!
//! \param[in] u32Div - Divider in range 1 to 255
//! \param[in] bDivFracEn - Unused
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_INVALID_DIV_VALUE - divider exceeds max divide
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - PixClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - upstream ref clock is gated
//! \retval SUCCESS - PixClk divider set
//! 
//! \notes The divider is set to divide by 1 at power-on reset. 
//! \notes Do NOT divide by 0. Do not divide by more than 255.
//! \notes HW_CLKCTRL_PIX.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
#define MAX_CLKCTRL_PIX_DIV 255
RtStatus_t hw_clkctrl_SetPixClkDiv(uint32_t u32Div, bool bDivFracEn)
{
    ////////////////////////////////////
    // Error checks
    ////////////////////////////////////

    // Make sure we don't divide by zero
    if(u32Div == 0)
        return ERROR_HW_CLKCTRL_DIV_BY_ZERO;

    // Make sure we don't divide by an invalid value
    if(u32Div > MAX_CLKCTRL_PIX_DIV)
        return ERROR_HW_CLKCTRL_INVALID_DIV_VALUE;

    // Return if busy with another divider change
    if(HW_CLKCTRL_PIX.B.BUSY)
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Only change DIV_FRAC_EN and DIV when CLKGATE = 0
    if(HW_CLKCTRL_PIX.B.BUSY)
        return ERROR_HW_CLKCTRL_CLK_GATED;

    // Check upstream reference clock gate if ref_pix is being used
    if(HW_CLKCTRL_FRAC.B.CLKGATEPIX && !HW_CLKCTRL_CLKSEQ.B.BYPASS_PIX)
        return ERROR_HW_CLKCTRL_REF_CLK_GATED;


    ////////////////////////////////////
    // Change divider
    ////////////////////////////////////

    // Always use integer divide for PIX clock
    BF_CLR(CLKCTRL_PIX,DIV_FRAC_EN);

    // Set divider
    HW_CLKCTRL_PIX.B.DIV = u32Div;

    return SUCCESS;    

}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get PixClk divider
//! 
//! \fntype Function                        
//!
//! Returns the pix clock divider value
//!
//! \retval Current pix clock divider value
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPixClkDiv(void)
{
    return HW_CLKCTRL_PIX.B.DIV;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate SspClk
//! 
//! \fntype Function                        
//!
//! CLK_SSP Gate. If set to 1, CLK_SSP is gated off. 0:CLK_SSP is not gated. 
//! When this bit is modified, or when it is high, the DIV field should not 
//! change its value. The DIV field can change ONLY when this clock
//! gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetSspClkGate(bool bClkGate)
{
    // Gate or ungate the SSP clock
    if(bClkGate)
        BF_SET(CLKCTRL_SSP,CLKGATE);
    else
        BF_CLR(CLKCTRL_SSP,CLKGATE);

    // Wait for the clock to settle.
    while(HW_CLKCTRL_SSP.B.BUSY);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set SspClk divider
//! 
//! \fntype Function                        
//!
//! The synchronous serial port clock frequency is determined by dividing 
//! the selected reference clock (ref_xtal or ref_io) by the value in this 
//! bit field. This field can be programmed with a new value only when CLKGATE = 0.
//!
//! \param[in] u32Div - Divider in range 1 to 511
//! \param[in] bDivFracEn - TRUE to use fractional divide, FALSE to use integer divide
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - SspClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - upstream ref clock is gated
//! \retval SUCCESS - SspClk divider set
//! 
//! \notes The divider is set to divide by 1 at power-on reset. Do NOT divide by 0.
//! \notes HW_CLKCTRL_SSP.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero. 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetSspClkDiv(uint32_t u32Div, bool bDivFracEn)
{
    ////////////////////////////////////
    // Error checks
    ////////////////////////////////////

    // Make sure we don't divide by zero
    if(u32Div == 0)
        return ERROR_HW_CLKCTRL_DIV_BY_ZERO;

    // Return if busy with another divider change
    if(HW_CLKCTRL_SSP.B.BUSY)
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Only change DIV_FRAC_EN and DIV when CLKGATE = 0
    if(HW_CLKCTRL_SSP.B.CLKGATE)
        return ERROR_HW_CLKCTRL_CLK_GATED;

    // Check upstream reference clock gate
    if(HW_CLKCTRL_FRAC.B.CLKGATEIO && !HW_CLKCTRL_CLKSEQ.B.BYPASS_SSP)
        return ERROR_HW_CLKCTRL_REF_CLK_GATED;

    ////////////////////////////////////
    // Change divider
    ////////////////////////////////////

    // Enable fractional divide, else enable integer divide
    if(bDivFracEn)
        BF_SET(CLKCTRL_SSP,DIV_FRAC_EN);
    else    
        BF_CLR(CLKCTRL_SSP,DIV_FRAC_EN);            

    // Set divider
    HW_CLKCTRL_SSP.B.DIV = u32Div;
    
    return SUCCESS;    
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get SspClk divider
//! 
//! \fntype Function                        
//!
//! Returns the SSP clock divider value
//!
//! \retval Current SSP clock divider value
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetSspClkDiv(void)
{
    return HW_CLKCTRL_SSP.B.DIV;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate SpdifClk
//! 
//! \fntype Function                        
//!
//! CLK_PCMSPDIF Gate. If set to 1, CLK_PCMSPDIF is gated off. 0: CLK_PCMSPDIF 
//! is not gated. When this bit is modified, or when it is high, the DIV field should
//! not change its value. The DIV field can change ONLY when this 
//! clock gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetSpdifClkGate(bool bClkGate)
{
    // Gate or ungate the SPDIF clock
    if(bClkGate)
        BF_SET(CLKCTRL_SPDIF,CLKGATE);
    else
        BF_CLR(CLKCTRL_SPDIF,CLKGATE);
        
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set GpmiClk divider
//!
//! \notes Datasheet only has clock gate.  Definition mentions DIV field so we put 
//! \notes it in here in case datasheet changes.  bDivFracEn and u32Div are 
//! \notes ignored and left in for future consideration.
//! \notes HW_CLKCTRL_SPDIF.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetSpdifClkDiv(uint32_t u32Div, bool bDivFracEn)
{
#if 0
    ////////////////////////////////////
    // Error checks
    ////////////////////////////////////

    // Make sure we don't divide by zero
    if(u32Div == 0)
        return ERROR_HW_CLKCTRL_DIV_BY_ZERO;

    // Return if busy with another divider change
    if(HW_CLKCTRL_SPDIF.B.BUSY)
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Only change DIV_FRAC_EN and DIV when CLKGATE = 0
    if(HW_CLKCTRL_SPDIF.B.CLKGATE)
        return ERROR_HW_CLKCTRL_CLK_GATED;

    ////////////////////////////////////
    // Change gate and/or divider
    ////////////////////////////////////

    // Set divider
    HW_CLKCTRL_SPDIF.B.DIV = u32Div;
    
#endif

    return SUCCESS;        
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Allow hardware to automatically set the divide ratios.
//! 
//! \fntype Function                        
//!
//! \param[in] bEnable - TRUE to enable, FALSE to disable
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_EnableIrAutoDivide(bool bEnable)
{
    // Enable to allow hardware to automatically set the divide ratios. 
    if(bEnable)
        BF_SET(CLKCTRL_IR,AUTO_DIV);
    else
        BF_CLR(CLKCTRL_IR,AUTO_DIV);
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate IrClk and IrovClk
//! 
//! \fntype Function                        
//!
//! CLK_IR and CLK_IROV Gate. If set to 1, CLK_IR and CLK_IROV are gated off. 
//! 0: CLK_IR and CLK_IROV are not gated. When this bit is modified, or when it is
//! high, the DIV field should not change its value. The DIV field can change 
//! ONLY when this clock gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetIrClkGate(bool bClkGate)
{
    // Gate or ungate the IR and IROV clock
    if(bClkGate)
        BF_SET(CLKCTRL_IR,CLKGATE);
    else
        BF_CLR(CLKCTRL_IR,CLKGATE);

    // Wait for the clock to settle.
    while(HW_CLKCTRL_IR.B.IR_BUSY || HW_CLKCTRL_IR.B.IROV_BUSY);
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Set IrClk divider
//! 
//! \fntype Function                        
//!
//! This field controls the CLK_IR divide-ratio-2. Values between 5 and 768 
//! are valid. This divider is used in conjunction with IROV_DIV to set the 
//! final rate of CLK_IR. This field can be programmed with a new
//! value only when CLKGATE = 0.
//!
//! \param[in] u32IrDiv - Divider in range 5 to 768
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_INVALID_DIV_VALUE - divider value out of range
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - IrClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - upstream ref clock is gated
//! \retval SUCCESS - IrClk divider set
//! 
//! \notes HW_CLKCTRL_IR.B.IR_DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
#define MAX_CLKCTRL_IR_DIV      768
#define MIN_CLKCTRL_IR_DIV      5
RtStatus_t hw_clkctrl_SetIrClkDiv(uint32_t u32IrDiv)
{
    ////////////////////////////////////
    // Error checks
    ////////////////////////////////////

    // Make sure we don't divide by zero
    if(u32IrDiv == 0)
        return ERROR_HW_CLKCTRL_DIV_BY_ZERO;

    // Make sure our values are valid
    if( (u32IrDiv < MIN_CLKCTRL_IR_DIV) || (u32IrDiv > MAX_CLKCTRL_IR_DIV) )
        return ERROR_HW_CLKCTRL_INVALID_DIV_VALUE;

    // Return if busy with another divider change
    if(HW_CLKCTRL_IR.B.IR_BUSY)
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Only change DIV_FRAC_EN and DIV when CLKGATE = 0
    if(HW_CLKCTRL_IR.B.CLKGATE)
        return ERROR_HW_CLKCTRL_CLK_GATED;

    // Check upstream reference clock gate if using ref_io
    if(HW_CLKCTRL_FRAC.B.CLKGATEIO && !HW_CLKCTRL_CLKSEQ.B.BYPASS_IR)
        return ERROR_HW_CLKCTRL_REF_CLK_GATED;

    ////////////////////////////////////
    // Change gate and/or divider
    ////////////////////////////////////
    
    // Set divider for IR_DIV
    HW_CLKCTRL_IR.B.IR_DIV = u32IrDiv;

    // Wait for the clock to settle.
    while(HW_CLKCTRL_IR.B.IR_BUSY || HW_CLKCTRL_IR.B.IROV_BUSY);

    return SUCCESS;   
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Set IrovClk divider
//! 
//! \fntype Function                        
//!
//! This field controls the CLK_IR Divide ratio-1 and the CLK_IROV divide 
//! ratio. Values between 4 and 260 are valid. This divider is used in 
//! conjunction with IR_DIV to set the final rate of CLK_IR. This divider is 
//! directly used to set the rate of CLK_IROV. This field can be programmed 
//! with a new value only when CLKGATE = 0.
//!
//! \param[in] u32IrDiv - Divider in range 4 to 260
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_INVALID_DIV_VALUE - divider value out of range
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - IrClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - upstream ref clock is gated
//! \retval SUCCESS - IrClk divider set
//! 
//! \notes HW_CLKCTRL_IR.B.IROV_DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
#define MAX_CLKCTRL_IROV_DIV    260
#define MIN_CLKCTRL_IROV_DIV    4
RtStatus_t hw_clkctrl_SetIrovClkDiv(uint32_t u32IrovDiv)
{
    ////////////////////////////////////
    // Error checks
    ////////////////////////////////////

    // Make sure we don't divide by zero
    if(u32IrovDiv == 0)
        return ERROR_HW_CLKCTRL_DIV_BY_ZERO;

    // Make sure our values are valid
    if( (u32IrovDiv < MIN_CLKCTRL_IROV_DIV) || (u32IrovDiv > MAX_CLKCTRL_IROV_DIV) )
        return ERROR_HW_CLKCTRL_INVALID_DIV_VALUE;

    // Return if busy with another divider change
    if(HW_CLKCTRL_IR.B.IROV_BUSY)
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Only change DIV_FRAC_EN and DIV when CLKGATE = 0
    if(HW_CLKCTRL_IR.B.CLKGATE)
        return ERROR_HW_CLKCTRL_CLK_GATED;

    // Check upstream reference clock gate if using ref_io
    if(HW_CLKCTRL_FRAC.B.CLKGATEIO && !HW_CLKCTRL_CLKSEQ.B.BYPASS_IR)
        return ERROR_HW_CLKCTRL_REF_CLK_GATED;

    ////////////////////////////////////
    // Change gate and/or divider
    ////////////////////////////////////
    
    // Set divider for IROV_DIV
    HW_CLKCTRL_IR.B.IROV_DIV = u32IrovDiv;

    // Wait for the clock to settle.
    while(HW_CLKCTRL_IR.B.IR_BUSY || HW_CLKCTRL_IR.B.IROV_BUSY);

    return SUCCESS;   
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate SaifClk
//! 
//! \fntype Function                        
//!
//! CLK_SAIF Gate. If set to 1, CLK_SAIF is gated off. 0:CLK_SAIF is not gated. 
//! When this bit is modified, or when it is high, the DIV field should not change 
//! its value. The DIV field can change ONLY when this clock gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetSaifClkGate(bool bClkGate)
{
    // Gate or ungate the SAIF clock
    if(bClkGate)
        BF_SET(CLKCTRL_SAIF,CLKGATE);
    else
        BF_CLR(CLKCTRL_SAIF,CLKGATE);

    // Wait for the clock to settle.
    while(HW_CLKCTRL_SAIF.B.BUSY);
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Set SaifClk divider
//! 
//! \fntype Function                        
//!
//! The SAIF clock frequency is determined by dividing the selected reference 
//! clock (ref_xtal or ref_pll) by the value in this bit field. This field can 
//! be programmed with a new value only when CLKGATE = 0.
//!
//! \param[in] u32IrDiv - Divider in range 1 to 65535
//! \param[in] bDivFracEn - Unused
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - SaifClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval SUCCESS - SaifClk divider set
//!
//! \notes HW_CLKCTRL_SAIF.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetSaifClkDiv(uint32_t u32Div, bool bDivFracEn)
{
    ////////////////////////////////////
    // Error checks
    ////////////////////////////////////

    // Make sure our values are valid
    if(u32Div == 0)
        return ERROR_HW_CLKCTRL_DIV_BY_ZERO;

    // Return if busy with another divider change
    if(HW_CLKCTRL_SAIF.B.BUSY)
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Only change DIV_FRAC_EN and DIV when CLKGATE = 0
    if(HW_CLKCTRL_SAIF.B.CLKGATE)
        return ERROR_HW_CLKCTRL_CLK_GATED;    


    ////////////////////////////////////
    // Change gate and/or divider
    ////////////////////////////////////

    // Always enable fractional divide for SAIF
    BF_SET(CLKCTRL_SAIF,DIV_FRAC_EN);

    // Set divider for 
    HW_CLKCTRL_SAIF.B.DIV = u32Div;

    return SUCCESS;   

}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

