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
// Enumerates
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
// Code
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate the 24MHz clock to the UART
//!
//! \fntype Function
//!
//! \param[in] bClkGate = TRUE to gate the clock, FALSE to ungate
//!
/////////////////////////////////////////////////////////////////////////////////
void ddi_clocks_SetUartClkGate(bool bClkGate)
{
    hw_clkctrl_SetXtalClkGate(UART_CLK_GATE,bClkGate);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate the 24MHz clock to the digital filter
//!
//! \fntype Function
//!
//! \param[in] bClkGate = TRUE to gate the clock, FALSE to ungate
//!
/////////////////////////////////////////////////////////////////////////////////
void ddi_clocks_SetDigFiltClkGate(bool bClkGate)
{
    hw_clkctrl_SetXtalClkGate(FILT_CLK24M_GATE,bClkGate);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate the 24MHz clock to the PWM
//!
//! \fntype Function
//!
//! \param[in] bClkGate = TRUE to gate the clock, FALSE to ungate
//!
/////////////////////////////////////////////////////////////////////////////////
void ddi_clocks_SetPwmClkGate(bool bClkGate)
{
    hw_clkctrl_SetXtalClkGate(PWM_CLK24M_GATE,bClkGate);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate the 24MHz clock to the DRI
//!
//! \fntype Function
//!
//! \param[in] bClkGate = TRUE to gate the clock, FALSE to ungate
//!
/////////////////////////////////////////////////////////////////////////////////
void ddi_clocks_SetDriClkGate(bool bClkGate)
{
    hw_clkctrl_SetXtalClkGate(DRI_CLK24M_GATE,bClkGate);
}
    
/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate the 1MHz clock to the digital control
//!
//! \fntype Function
//!
//! \param[in] bClkGate = TRUE to gate the clock, FALSE to ungate
//!
/////////////////////////////////////////////////////////////////////////////////
void ddi_clocks_SetDigCtrlClkGate(bool bClkGate)
{
    hw_clkctrl_SetXtalClkGate(DIGCTRL_CLK1M_GATE,bClkGate);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate the 32kHz clock to the timer/rotary encoder block
//!
//! \fntype Function
//!
//! \param[in] bClkGate = TRUE to gate the clock, FALSE to ungate
//!
/////////////////////////////////////////////////////////////////////////////////
void ddi_clocks_SetTimrotClkGate(bool bClkGate)
{
    hw_clkctrl_SetXtalClkGate(TIMROT_CLK32K_GATE,bClkGate);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Bypass the reference clock from the PLL for the ref_cpu 
//!
//! \fntype Function
//!
//! \param[in] bBypass = TRUE to bypass PLL, use 24MHz; FALSE use PLL signal
//!
//! \retval SUCCESS - bypass operatiom successful
//! \retval ERROR_ - HW level error
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_BypassRefCpu(bool bBypass)
{
    return hw_clkctrl_SetClkBypass(BYPASS_CPU,bBypass);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Bypass the reference clock from the PLL for the ref_emi 
//!
//! \fntype Function
//!
//! \param[in] bBypass = TRUE to bypass PLL, use 24MHz; FALSE use PLL signal
//!
//! \retval SUCCESS - bypass operatiom successful
//! \retval ERROR_ - HW level error
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_BypassRefEmi(bool bBypass)
{
    return hw_clkctrl_SetClkBypass(BYPASS_EMI,bBypass);
}
   
/////////////////////////////////////////////////////////////////////////////////
//! \brief Bypass the reference clock from the PLL for the ref_io signal to the SSP
//!
//! \fntype Function
//!
//! \param[in] bBypass = TRUE to bypass PLL, use 24MHz, FALSE use PLL signal
//!
//! \retval SUCCESS - bypass operatiom successful
//! \retval ERROR_ - HW level error
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_BypassRefIoSsp(bool bBypass)
{
    return hw_clkctrl_SetClkBypass(BYPASS_SSP,bBypass);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Bypass the reference clock from the PLL for the ref_io signal to the GPMI
//!
//! \fntype Function
//!
//! \param[in] bBypass = TRUE to bypass PLL, use 24MHz, FALSE use PLL signal
//!
//! \retval SUCCESS - bypass operatiom successful
//! \retval ERROR_ - HW level error
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_BypassRefIoGpmi(bool bBypass)
{
    return hw_clkctrl_SetClkBypass(BYPASS_GPMI,bBypass);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Bypass the reference clock from the PLL for the ref_io signal to the IR
//!
//! \fntype Function
//!
//! \param[in] bBypass = TRUE to bypass PLL, use 24MHz, FALSE use PLL signal
//!
//! \retval SUCCESS - bypass operatiom successful
//! \retval ERROR_ - HW level error
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_BypassRefIoIr(bool bBypass)
{
    return hw_clkctrl_SetClkBypass(BYPASS_IR,bBypass);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Bypass the reference clock from the PLL for the ref_pix signal to the LCD
//!
//! \fntype Function
//!
//! \param[in] bBypass = TRUE to bypass PLL, use 24MHz, FALSE use PLL signal
//!
//! \retval SUCCESS - bypass operatiom successful
//! \retval ERROR_ - HW level error
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_BypassRefPix(bool bBypass)
{
    return hw_clkctrl_SetClkBypass(BYPASS_PIX,bBypass);
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status of ref_cpu signal to Pclk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_cpu bypassed, ref_xtal is source clock
//! \retval FALSE - ref_cpu is source clock
/////////////////////////////////////////////////////////////////////////////////
bool ddi_clocks_GetBypassRefCpu(void)
{
    return hw_clkctrl_GetPclkBypass();
}
                                    
/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status of ref_emi signal
//!
//! \fntype Function
//!
//! \retval TRUE - ref_emi bypassed, ref_xtal is source clock
//! \retval FALSE - ref_emi is source clock
/////////////////////////////////////////////////////////////////////////////////
bool ddi_clocks_GetBypassRefEmi(void)
{
    return hw_clkctrl_GetEmiClkBypass();
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status of ref_pix signal
//!
//! \fntype Function
//!
//! \retval TRUE - ref_pix bypassed, ref_xtal is source clock
//! \retval FALSE - ref_pix is source clock
/////////////////////////////////////////////////////////////////////////////////
bool ddi_clocks_GetBypassRefPix(void)
{
    return hw_clkctrl_GetPixClkBypass();
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status of ref_io signal for SspClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_io bypassed, ref_xtal is source clock
//! \retval FALSE - ref_io is source clock
/////////////////////////////////////////////////////////////////////////////////
bool ddi_clocks_GetBypassRefIoSsp(void)
{
    return hw_clkctrl_GetSspClkBypass();
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status of ref_io signal for GpmiClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_io bypassed, ref_xtal is source clock
//! \retval FALSE - ref_io is source clock
/////////////////////////////////////////////////////////////////////////////////
bool ddi_clocks_GetBypassRefIoGpmi(void)
{
    return hw_clkctrl_GetGpmiClkBypass();
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status of ref_io signal for IrClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_io bypassed, ref_xtal is source clock
//! \retval FALSE - ref_io is source clock
/////////////////////////////////////////////////////////////////////////////////
bool ddi_clocks_GetBypassRefIoIr(void)
{
    return hw_clkctrl_GetIrClkBypass();
}

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

