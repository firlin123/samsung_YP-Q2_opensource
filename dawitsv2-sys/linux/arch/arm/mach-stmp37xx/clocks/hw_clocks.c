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
// PClk
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Sets divider for the crystal reference clock (ref_xtal)
//!
//! \fntype Function
//!
//! This field controls the divider connected to the crystal reference clock
//! that drives the CLK_P domain when bypass is selected.  This divides the
//! ref_xtal clock.  The divider can be an integer or fractional divider.
//!
//! \param[in] u32Div - Integer or fractional divide value.
//! \param[in] bDivFracEn - TRUE to specify u32Div as a fractional divider. FALSE for integer
//!
//! \retval SUCCESS - Call was successful.
//! \return ERROR_HW_CLKCTRL_DIV_BY_ZERO - u32Div should not be zero
//!
//! \notes The divider is set to divide by 1 at power-on reset. Do NOT divide by 0.
//! \notes HW_CLKCTRL_CPU.B.DIV_XTAL is used to avoid a clear/set
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPclkRefXtalDiv(uint32_t u32Div, bool bDivFracEn)
{
    // Enable fractional divide, else enable integer divide
    if( bDivFracEn )
        BF_SET(CLKCTRL_CPU,DIV_XTAL_FRAC_EN);
    else
        BF_CLR(CLKCTRL_CPU,DIV_XTAL_FRAC_EN);

    // Set divider
    HW_CLKCTRL_CPU.B.DIV_XTAL = u32Div;
    return SUCCESS;

}

/////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Returns the current divider for the crystal reference clock (ref_xtal)
//!
//! \fntype Function
//!
//! \retval Current divider value
//!
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPclkRefXtalDiv(void)
{
    return HW_CLKCTRL_CPU.B.DIV_XTAL;
}

/////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Sets divider for the PLL reference cpu signal (ref_cpu)
//!
//! \fntype Function
//!
//! This field controls the divider connected to the ref_cpu reference clock
//! that drives the CLK_P domain when bypass is NOT selected. For changes to
//! this field to take effect, the ref_cpu reference clock must be running.
//!
//! \param[in] u32Div - Integer divide value.
//! \param[in] bDivFracEn - Not used.  Divider will always be integer.
//!
//! \retval SUCCESS - Call was successful.
//! \return ERROR_HW_CLKCTRL_DIV_BY_ZERO - u32Div should not be zero
//! \return ERROR_HW_CLKCTRL_REF_CPU_GATED - Upstream reference clock is gated
//!
//! \notes The divider is set to divide by 1 at power-on reset. Do NOT divide by 0.
//! \notes HW_CLKCTRL_CPU.B.DIV_CPU is used to avoid a clear/set
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPclkRefCpuDiv(uint32_t u32Div, bool bDivFracEn)
{
    /////////////////////////////////////////
    // Error checks
    /////////////////////////////////////////

    // Check upstream reference clock gate
    if(HW_CLKCTRL_FRAC.B.CLKGATECPU)
        return ERROR_HW_CLKCTRL_REF_CPU_GATED;


    /////////////////////////////////////////
    // Change dividers
    /////////////////////////////////////////

    // Always use integer divide
    BF_CLR(CLKCTRL_CPU,DIV_CPU_FRAC_EN);

    // Set divider
    HW_CLKCTRL_CPU.B.DIV_CPU = u32Div;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Returns the current divider for the PLL reference cpu signal (ref_cpu)
//!
//! \fntype Function
//!
//! \retval Current divider value
//!
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPclkRefCpuDiv(void)
{
    return HW_CLKCTRL_CPU.B.DIV_CPU;
}

/////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Gates/ungates the PClk while waiting for an interrupt
//!
//! \fntype Function
//!
//! \param[in] bGatePclk - TRUE to gate PClk, FALSE to ungate
//!
//! /////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPclkInterruptWait(bool bGatePclk)
{
    // Gate or ungate Pclk while waiting for an interrupt.
    if( bGatePclk )
        BF_SET(CLKCTRL_CPU,INTERRUPT_WAIT);
    else
        BF_CLR(CLKCTRL_CPU,INTERRUPT_WAIT);
}

/////////////////////////////////////////////////////////////////////////////////
// HClk
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! \brief Enables/disables auto-slow modes for specific HClk components
//!
//! \fntype Function
//!
//! There are six components that use the HClk and can be auto-slowed.  This
//! function will enable or disable each of the components auto-slow mode.
//! Components: APBHDMA_AUTOSLOW, APBXDMA_AUTOSLOW, TRAFFIC_JAM_AUTOSLOW,
//!             TRAFFIC_AUTOSLOW, CPU_DATA_AUTOSLOW, CPU_INSTR_AUTOSLOW
//!
//! \param[in] eAutoslow - HClk component to change
//! \param[in] bEnable - TRUE to enable auto-slow mode, FALSE to disable
//!
//! \retval ERROR_HW_CLKCTRL_UNSUPPORTED_AUTOSLOW_COMPONENT - unsupported component
//! \retval SUCCESS - component's auto-slow mode set properly
//! /////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_EnableHclkModuleAutoSlow(hw_clkctrl_autoslow_t eAutoslow, bool bEnable)
{
    // Enable/disable the auto-slow mode for the selected HClk component
    switch(eAutoslow)
    {
        case APBHDMA_AUTOSLOW:
            if(bEnable)
                BF_SET(CLKCTRL_HBUS,APBHDMA_AS_ENABLE);
            else
                BF_CLR(CLKCTRL_HBUS,APBHDMA_AS_ENABLE);
        break;

        case APBXDMA_AUTOSLOW:
            if(bEnable)
                BF_SET(CLKCTRL_HBUS,APBXDMA_AS_ENABLE);
            else
                BF_CLR(CLKCTRL_HBUS,APBXDMA_AS_ENABLE);
        break;

        case TRAFFIC_JAM_AUTOSLOW:
            if(bEnable)
                BF_SET(CLKCTRL_HBUS,TRAFFIC_JAM_AS_ENABLE);
            else
                BF_CLR(CLKCTRL_HBUS,TRAFFIC_JAM_AS_ENABLE);
        break;

        case TRAFFIC_AUTOSLOW:
            if(bEnable)
                BF_SET(CLKCTRL_HBUS,TRAFFIC_AS_ENABLE);
            else
                BF_CLR(CLKCTRL_HBUS,TRAFFIC_AS_ENABLE);
        break;

        case CPU_DATA_AUTOSLOW:
            if(bEnable)
                BF_SET(CLKCTRL_HBUS,CPU_DATA_AS_ENABLE);
            else
                BF_CLR(CLKCTRL_HBUS,CPU_DATA_AS_ENABLE);
        break;

        case CPU_INSTR_AUTOSLOW:
            if(bEnable)
                BF_SET(CLKCTRL_HBUS,CPU_INSTR_AS_ENABLE);
            else
                BF_CLR(CLKCTRL_HBUS,CPU_INSTR_AS_ENABLE);
        break;

        default:
            return ERROR_HW_CLKCTRL_UNSUPPORTED_AUTOSLOW_COMPONENT;
    }

    return SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Enables auto-slow mode for HClk
//!
//! \fntype Function
//!
//! Enable CLK_H auto-slow mode. When this is set, then CLK_H will run at
//! the slow rate until one of the fast mode events has occurred.
//!
//! \param[in] bEnable - TRUE to enable auto-slow mode, FALSE to disable
//!
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_EnableHclkAutoSlow(bool bEnable)
{
    // Enable or disable Hclk Auto-slow
    if(bEnable)
        BF_SET(CLKCTRL_HBUS,AUTO_SLOW_MODE);
    else
        BF_CLR(CLKCTRL_HBUS,AUTO_SLOW_MODE);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PClk to HClk divide ratio
//!
//! \fntype Function
//!
//! HClk = PClk/u32Div
//! The HClk frequency will be the result of the PClk divided by u32Div.
//!
//! \param[in] u32Div - Ratio of PClk to HClk
//! \param[in] bDivFracEn - Enable divider to be a fractional ratio.
//!
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - HClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - u32Div should not be zero
//!
//! \notes The divider is set to divide by 1 at power-on reset. Do NOT divide by 0
//! \notes HW_CLKCTRL_HBUS.B.DIV is used to avoid a clear/set
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetHclkDiv(uint32_t u32Div, bool bDivFracEn)
{
    ////////////////////////////////////
    // Error checks
    ////////////////////////////////////

    // Return if busy
    if(HW_CLKCTRL_HBUS.B.BUSY)
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Enable or disable fractional divide for Hclk
    if(bDivFracEn)
        BF_SET(CLKCTRL_HBUS,DIV_FRAC_EN);
    else
        BF_CLR(CLKCTRL_HBUS,DIV_FRAC_EN);

    // Set the Pclk to Hclk divide ratio
			//printk("[PM], %s, %d\n\n", __FILE__, __LINE__); //dhsong
			//printk("[PM], HW_CLKCTRL_HBUS=0x%08x\n\n", HW_CLKCTRL_HBUS); //dhsong
			//printk("[PM], u32Div=0x%x\n\n", u32Div); //dhsong
    HW_CLKCTRL_HBUS.B.DIV = u32Div;
			//printk("[PM], %s, %d\n\n", __FILE__, __LINE__); //dhsong
    return SUCCESS;


}

RtStatus_t hw_clkctrl_GetHclkDiv (uint32_t *u32Div)
{
	*u32Div = HW_CLKCTRL_HBUS.B.DIV;
	return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set fast HClk to slow HClk divide ratio
//!
//! \fntype Function
//!
//! Slow mode divide ratio. Sets the ratio of CLK_H fast rate to the slow rate.
//! Valid ratios: SLOW_DIV_BY1, SLOW_DIV_BY2, SLOW_DIV_BY4,
//!               SLOW_DIV_BY8, SLOW_DIV_BY16, SLOW_DIV_BY32
//!
//! \param[in] u32Div - Ratio of PClk to HClk
//! \param[in] bDivFracEn - Enable divider to be a fractional ratio.
//!
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - HClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_INVALID_PARAM - Divide value out of range
//! \retval SUCCESS - HClk slow divide set
//!
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetHclkSlowDiv(hw_clkctrl_hclk_slow_div_t eSlowDiv)
{
    // Is HClk busy?
    if(HW_CLKCTRL_HBUS.B.BUSY)
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Slow ratio out of range?
    if(eSlowDiv > SLOW_DIV_MAX)
        return ERROR_HW_CLKCTRL_INVALID_PARAM;

    // Set the Hclk slow mode divide ratio
    BF_WR(CLKCTRL_HBUS,SLOW_DIV,eSlowDiv);

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the slow HClk divide ratio
//!
//! \fntype Function
//!
//! \retval HClk slow divide ratio
//!
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetHclkSlowDiv(void)
{
    return HW_CLKCTRL_HBUS.B.SLOW_DIV;
}

/////////////////////////////////////////////////////////////////////////////////
// XClk
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set XClk divider
//!
//! \fntype Function
//!
//! This field controls the CLK_X divide ratio. CLK_X is sourced from the
//! 24-MHz XTAL through this divider.
//!
//! \param[in] u32Div - Divider in range 1 to 1023
//! \param[in] bDivFracEn - Unused
//!
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - XClk is still transitioning
//! \notes HW_CLKCTRL_XBUS.B.DIV is used to avoid a clear/set
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetXclkDiv(uint32_t u32Div, bool bDivFracEn)
{
    // Return if busy
    if( HW_CLKCTRL_XBUS.B.BUSY )
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Enable or disable fractional divide
    if(bDivFracEn)
        BF_SET(CLKCTRL_XBUS,DIV_FRAC_EN);
    else
        BF_CLR(CLKCTRL_XBUS,DIV_FRAC_EN);

    // Set the divider
    HW_CLKCTRL_XBUS.B.DIV = u32Div;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
// EmiClk
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate EmiClk  for the ref_xtal source
//!
//! \fntype Function
//!
//! CLK_EMI crystal divider Gate. If set to 1, the EMI_CLK divider that is
//! sourced by the crystal reference clock, ref_xtal, is gated off. If set to
//! 0, CLK_EMI crystal divider is not gated
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
//!
//! \notes Only gates the ref_xtal reference clock to the XTAL divider. ref_emi
//! \notes has no gate but can be muxed.
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetEmiRefXtalClkGate(bool bClkGate)
{
    // Gate or ungate the EMI clock
    if(bClkGate)
        BF_SET(CLKCTRL_EMI,CLKGATE);
    else
        BF_CLR(CLKCTRL_EMI,CLKGATE);

    // Wait for the clock to settle.
    while(HW_CLKCTRL_EMI.B.BUSY_REF_XTAL);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set EmiClk divider for the ref_xtal source
//!
//! \fntype Function
//!
//! This field controls the divider connected to the crystal reference clock,
//! ref_xtal, that drives the CLK_EMI domain when bypass IS selected.
//!
//! \param[in] u32Div - Divider in range 1 to 15
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - EmiClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval SUCCESS - EmiClk ref_xtal divider set
//!
//! \notes HW_CLKCTRL_EMI.B.DIV_XTAL is used to avoid a clear/set
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetEmiClkRefXtalDiv(uint32_t u32Div)
{
    ////////////////////////////////////
    // Error checks
    ////////////////////////////////////

    // Return if busy with another divider change
    if(HW_CLKCTRL_EMI.B.BUSY_REF_XTAL)
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Only change DIV_FRAC_EN and DIV when CLKGATE = 0
    if(HW_CLKCTRL_EMI.B.CLKGATE)
        return ERROR_HW_CLKCTRL_CLK_GATED;


    ////////////////////////////////////
    // Change gate and/or divider
    ////////////////////////////////////

    // Set divider for DIV_XTAL
    HW_CLKCTRL_EMI.B.DIV_XTAL = u32Div;

    return SUCCESS;

}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get EmiClk divider for the ref_xtal source
//!
//! \fntype Function
//!
//! This field controls the divider connected to the crystal reference clock,
//! ref_xtal, that drives the CLK_EMI domain when bypass IS selected.
//!
//! \retval Current divider value for EmiClk's ref_xtal signal
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetEmiClkRefXtalDiv(void)
{
    return HW_CLKCTRL_EMI.B.DIV_XTAL;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set EmiClk divider for the ref_emi source
//!
//! \fntype Function
//!
//! This field controls the divider connected to the ref_emi reference clock
//! that drives the CLK_EMI domain when bypass IS NOT selected. For changes
//! to this field to take effect, the ref_emi reference clock must be running.
//!
//! \param[in] u32Div - Divider in range 1 to 63
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - EmiClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval SUCCESS - EmiClk ref_emi divider set
//!
//! \notes HW_CLKCTRL_EMI.B.DIV_EMI is used to avoid a clear/set
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetEmiClkRefEmiDiv(uint32_t u32Div)
{
    ////////////////////////////////////
    // Error checks
    ////////////////////////////////////

    // Return if busy with another divider change
    if(HW_CLKCTRL_EMI.B.BUSY_REF_EMI)
        return ERROR_HW_CLKCTRL_CLK_DIV_BUSY;

    // Check upstream reference clock gate
    if(HW_CLKCTRL_FRAC.B.CLKGATEEMI)
        return ERROR_HW_CLKCTRL_REF_CLK_GATED;


    ////////////////////////////////////
    // Change gate and/or divider
    ////////////////////////////////////

    // Set divider for DIV_EMI
    HW_CLKCTRL_EMI.B.DIV_EMI = u32Div;

    // Done
    return SUCCESS;

}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get EmiClk divider for the ref_xtal source
//!
//! \fntype Function
//!
//! This field controls the divider connected to the crystal reference clock,
//! ref_xtal, that drives the CLK_EMI domain when bypass IS selected.
//!
//! \retval Current divider value for EmiClk's ref_xtal signal
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetEmiClkRefEmiDiv(void)
{
    return HW_CLKCTRL_EMI.B.DIV_EMI;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Reserved in the datasheet.  May be used for SDRAM
//!
//! \retval Status of BUSY_DCC_RESYNC
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetBusyDccResync(void)
{
    // Return BUSY_DCC_RESYNC
    return HW_CLKCTRL_EMI.B.BUSY_DCC_RESYNC;
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Reserved in the datasheet.  May be used for SDRAM
//!
//! \param[in] bEnable - TRUE to enable DCC resync
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_EnableDccResync(bool bEnable)
{
    // Enable
    if( bEnable )
        BF_SET(CLKCTRL_EMI,DCC_RESYNC_ENABLE);
    else
        BF_CLR(CLKCTRL_EMI,DCC_RESYNC_ENABLE);
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief controls the selection of clock sources for various clock dividers.
//!
//! \fntype Function
//!
//! The PClk, EmiClk, SspClk, GpmiClk, IrClk, PixClk, and SaifClk can choose their
//! clock source from the PLL output, or bypass the PLL and use the 24MHz clock
//! signal instead.
//!
//! \param[in] - eClk - clock to bypass.  Valid inputs are: BYPASS_CPU, BYPASS_EMI,
//!              BYPASS_SSP, BYPASS_GPMI, BYPASS_IR, BYPASS_PIX, BYPASS_SAIF
//! \param[in] - bBypass - TRUE to use ref_xtal clock, FALSE to use PLL
//!
//! \retval - ERROR_HW_CLKCTRL_REF_CPU_GATED - ref_cpu is gated at PFD
//! \retval - ERROR_HW_CLKCTRL_REF_EMI_GATED - ref_emi is gated at PFD
//! \retval - ERROR_HW_CLKCTRL_REF_IO_GATED - ref_io is gated at PFD
//! \retval - ERROR_HW_CLKCTRL_REF_PIX_GATED - ref_pix is gated at PFD
//! \retval - ERROR_HW_CLKCTRL_INVALID_GATE_VALUE - gate cannot be set
//! \retval - SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetClkBypass(hw_clkctrl_bypass_clk_t eClk, bool bBypass)
{

    ////////////////////////////////
    // Check upstream clock gates
    ////////////////////////////////
    switch(eClk)
    {

        case BYPASS_CPU:
            if(bBypass)
            {
                // ref_xtal is ok.
            }
            else
            {
                // Don't change if ref_cpu is gated
                if(HW_CLKCTRL_FRAC.B.CLKGATECPU)
                    return ERROR_HW_CLKCTRL_REF_CPU_GATED;
            }
        break;

        case BYPASS_EMI:
            if(bBypass)
            {
                // Don't change if ref_xtal is gated
                if(HW_CLKCTRL_EMI.B.CLKGATE)
                    return ERROR_HW_CLKCTRL_REF_CLK_GATED;
            }
            else
            {
                // Don't change if ref_emi is gated
                if(HW_CLKCTRL_FRAC.B.CLKGATEEMI)
                    return ERROR_HW_CLKCTRL_REF_EMI_GATED;
            }
        break;

        case BYPASS_SSP:
        case BYPASS_GPMI:
        case BYPASS_IR:
            if(bBypass)
            {
                // ref_xtal is ok
            }
            else
            {
                // Don't change if ref_io is gated
                if(HW_CLKCTRL_FRAC.B.CLKGATEIO)
                    return ERROR_HW_CLKCTRL_REF_IO_GATED;
            }
        break;

        case BYPASS_PIX:
            if(bBypass)
            {
                // ref_xtal is ok
            }
            else
            {
                // Don't change if ref_pix is gated
                if(HW_CLKCTRL_FRAC.B.CLKGATEPIX)
                    return ERROR_HW_CLKCTRL_REF_PIX_GATED;
            }
        break;

        case BYPASS_SAIF:
            if(bBypass)
            {
                // Should never be set to 1
                return ERROR_HW_CLKCTRL_INVALID_GATE_VALUE;
            }
            else
            {
                // Should always be 0

            }
        break;

        default:
            return ERROR_HW_CLKCTRL_INVALID_PARAM;

    }  // end switch(eClk)


    ////////////////////////////////
    // Change the clock
    ////////////////////////////////
    if(bBypass)
        HW_CLKCTRL_CLKSEQ_SET(eClk);
    else
        HW_CLKCTRL_CLKSEQ_CLR(eClk);

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for PClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_xtal generates Pclk
//! \retval FALSE - ref_cpu generates Pclk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPclkBypass(void)
{
    return HW_CLKCTRL_CLKSEQ.B.BYPASS_CPU;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for EmiClk
//!
//! \retval TRUE - ref_xtal generates Emiclk
//! \retval FALSE - ref_emi generates Emiclk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetEmiClkBypass(void)
{
    return HW_CLKCTRL_CLKSEQ.B.BYPASS_EMI;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for SspClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_xtal generates Sspclk
//! \retval FALSE - ref_io generates Sspclk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetSspClkBypass(void)
{
    return HW_CLKCTRL_CLKSEQ.B.BYPASS_SSP;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for IrClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_xtal generates IrClk
//! \retval FALSE - ref_io generates IrClk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetIrClkBypass(void)
{
    return HW_CLKCTRL_CLKSEQ.B.BYPASS_IR;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for PixClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_xtal generates PixClk
//! \retval FALSE - ref_io generates PixClk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPixClkBypass(void)
{
    return HW_CLKCTRL_CLKSEQ.B.BYPASS_PIX;
}

bool hw_clkctrl_GetPllPowered(void)
{
    return HW_CLKCTRL_PLLCTRL0.B.POWER;
}


// 36xx functions I have to resolve
RtStatus_t hw_clocks_PowerPll(bool bPowerOn)
{
    hw_clkctrl_PowerPll(bPowerOn);
    return SUCCESS;
}

RtStatus_t hw_clocks_BypassPll(bool bBypass)
{
    return SUCCESS;
}

RtStatus_t hw_clocks_SetPllFreq(uint16_t u16_FreqHz)
{
    return SUCCESS;
}

RtStatus_t hw_clocks_SetPclkDiv(uint16_t u16_PclkDivider)
{
    return SUCCESS;
}

RtStatus_t hw_clocks_SetHclkDiv(uint16_t u16_PclkDivider)
{
    return SUCCESS;
}

RtStatus_t hw_clocks_SetHclkAutoSlowDivider(int16_t uDiv)
{
    return hw_clkctrl_SetHclkSlowDiv((hw_clkctrl_hclk_slow_div_t) uDiv);
}

RtStatus_t hw_clocks_EnableHclkAutoSlow(bool bEnable)
{
    hw_clkctrl_EnableHclkAutoSlow(bEnable);
    return SUCCESS;
}

RtStatus_t hw_clocks_SetXclkDiv(uint16_t u16_XclkDivider)
{
    return hw_clkctrl_SetXclkDiv((uint32_t)u16_XclkDivider,FALSE);
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

