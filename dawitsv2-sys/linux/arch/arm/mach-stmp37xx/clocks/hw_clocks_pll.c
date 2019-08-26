/////////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_clocks
//! @{
//
// Copyright(C) 2007 SigmaTel, Inc.
//
//! \file hw_clocks_pfd.c
//! \brief PFD hardware interface routines.
//!
//! \date June 2007
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
//! \brief Gate/ungate PLL output through PFD for ref_pix
//!
//! \fntype Function
//!
//! PIX Clock Gate. If set to 1, the PIX fractional divider
//! clock (reference PLL ref_pix) is off (power savings). 0:
//! PIX fractional divider clock is enabled.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPfdRefPixGate(bool bClkGate)
{
    // Gate or ungate the ref_pix clock
    if(bClkGate)
        BF_SET(CLKCTRL_FRAC,CLKGATEPIX);
    else
        BF_CLR(CLKCTRL_FRAC,CLKGATEPIX);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the gate status for ref_pix in the PFD
//!
//! \fntype Function
//!
//! \retval - TRUE if gated, FALSE if ungated
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdRefPixGate(void)
{
    return HW_CLKCTRL_FRAC.B.CLKGATEPIX;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PFD for ref_pix
//!
//! \fntype Function
//!
//! This field controls the PIX clock fractional divider. The
//! resulting frequency shall be 480 * (18/PIXFRAC) where
//! PIXFRAC = 18-35.
//!
//! \param[in] u32Div - Divider in range 18 to 35
//!
//! \notes Divider = 18 will disable PFD circuit to save power since 18/18 is
//! \notes equal to 1.  The output will be 480MHz.
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - can't change divider when clock is gated
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPfdRefPixDiv(uint32_t u32Div)
{
    // Don't change divider if clock is gated
    if(HW_CLKCTRL_FRAC.B.CLKGATEPIX)
        return ERROR_HW_CLKCTRL_REF_CLK_GATED;

    // Set divider for PIX clock PFD
    HW_CLKCTRL_FRAC.B.PIXFRAC = u32Div;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return divider value for ref_pix
//!
//! \fntype Function
//!
//! \retval - divider value in range 18 - 35
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPfdRefPixDiv(void)
{
    return HW_CLKCTRL_FRAC.B.PIXFRAC;
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate PLL output through PFD for ref_emi
//!
//! \fntype Function
//!
//! EMI Clock Gate. If set to 1, the EMI fractional divider
//! clock (reference PLL ref_emi) is off (power savings). 0:
//! EMI fractional divider clock is enabled.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPfdRefEmiGate(bool bClkGate)
{
    // Gate or ungate the ref_emi clock
    if(bClkGate)
        BF_SET(CLKCTRL_FRAC,CLKGATEEMI);
    else
        BF_CLR(CLKCTRL_FRAC,CLKGATEEMI);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the gate status for ref_emi in the PFD
//!
//! \fntype Function
//!
//! \retval - TRUE if gated, FALSE if ungated
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdRefEmiGate(void)
{
    return HW_CLKCTRL_FRAC.B.CLKGATEEMI;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PFD for ref_emi
//!
//! \fntype Function
//!
//! This field controls the EMI clock fractional divider. The
//! resulting frequency shall be 480 * (18/EMIFRAC) where
//! EMIFRAC = 18-35.
//!
//! \param[in] u32Div - Divider in range 18 to 35
//!
//! \notes Divider = 18 will disable PFD circuit to save power since 18/18 is
//! \notes equal to 1.  The output will be 480MHz.
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - can't change divider when clock is gated
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////

RtStatus_t hw_clkctrl_SetPfdRefEmiDiv(uint32_t u32Div)
{
    // Don't change divider if clock is gated
    if(HW_CLKCTRL_FRAC.B.CLKGATEEMI)
        return ERROR_HW_CLKCTRL_REF_CLK_GATED;

    // Set divider for EMI clock PFD
    HW_CLKCTRL_FRAC.B.EMIFRAC = u32Div;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return divider value for ref_emi
//!
//! \fntype Function
//!
//! \retval - divider value in range 18 - 35
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPfdRefEmiDiv(void)
{
    return HW_CLKCTRL_FRAC.B.EMIFRAC;
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate PLL output through PFD for ref_cpu
//!
//! \fntype Function
//!
//! CPU Clock Gate. If set to 1, the CPU fractional divider
//! clock (reference PLL ref_cpu) is off (power savings). 0:
//! CPU fractional divider clock is enabled.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPfdRefCpuGate(bool bClkGate)
{
    // Gate or ungate the ref_cpu clock
    if(bClkGate)
        BF_SET(CLKCTRL_FRAC,CLKGATECPU);
    else
        BF_CLR(CLKCTRL_FRAC,CLKGATECPU);
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the gate status for ref_cpu in the PFD
//!
//! \fntype Function
//!
//! \retval - TRUE if gated, FALSE if ungated
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdRefCpuGate(void)
{
    return HW_CLKCTRL_FRAC.B.CLKGATECPU;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PFD for ref_cpu
//!
//! \fntype Function
//!
//! This field controls the CPU clock fractional divider. The
//! resulting frequency shall be 480 * (18/CPUFRAC) where
//! CPUFRAC = 18-35.
//!
//! \param[in] u32Div - Divider in range 18 to 35
//!
//! \notes Divider = 18 will disable PFD circuit to save power since 18/18 is
//! \notes equal to 1.  The output will be 480MHz.
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - can't change divider when clock is gated
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////

RtStatus_t hw_clkctrl_SetPfdRefCpuDiv(uint32_t u32Div)
{
    // Don't change divider if clock is gated
    if(HW_CLKCTRL_FRAC.B.CLKGATECPU)
        return ERROR_HW_CLKCTRL_REF_CLK_GATED;

    // Set divider for CPU clock PFD
    HW_CLKCTRL_FRAC.B.CPUFRAC = u32Div;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return divider value for ref_cpu
//!
//! \fntype Function
//!
//! \retval - divider value in range 18 - 35
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPfdRefCpuDiv(void)
{
    return HW_CLKCTRL_FRAC.B.CPUFRAC;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get status of fractional divider transition for ref_io
//!
//! \retval Divider transition for ref_io
//!
//! \notes This read-only bitfield is for DIAGNOSTIC PURPOSES ONLY since the
//! \notes fractional divide should become stable quickly enough that this
//! \notes field will never need to be used by either device driver or
//! \notes application code. The value inverts when the new programmed fractional
//! \notes divide value has taken effect. Read this bit, program the new value,
//! \notes and when this bit inverts, the phase divider clock output is stable.
//! \notes Note that the value will not invert when the fractional divider
//! \notes is taken out of or placed into clock-gated state.
//!
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdFracDivIoStable(void)
{
    return HW_CLKCTRL_FRAC.B.IO_STABLE;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get status of fractional divider transition for ref_pix
//!
//! \retval Divider transition for ref_pix
//!
//! \notes This read-only bitfield is for DIAGNOSTIC PURPOSES ONLY since the
//! \notes fractional divide should become stable quickly enough that this
//! \notes field will never need to be used by either device driver or
//! \notes application code. The value inverts when the new programmed fractional
//! \notes divide value has taken effect. Read this bit, program the new value,
//! \notes and when this bit inverts, the phase divider clock output is stable.
//! \notes Note that the value will not invert when the fractional divider
//! \notes is taken out of or placed into clock-gated state.
//!
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdFracDivPixStable(void)
{
    return HW_CLKCTRL_FRAC.B.PIX_STABLE;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get status of fractional divider transition for ref_emi
//!
//! \retval Divider transition for ref_emi
//!
//! \notes This read-only bitfield is for DIAGNOSTIC PURPOSES ONLY since the
//! \notes fractional divide should become stable quickly enough that this
//! \notes field will never need to be used by either device driver or
//! \notes application code. The value inverts when the new programmed fractional
//! \notes divide value has taken effect. Read this bit, program the new value,
//! \notes and when this bit inverts, the phase divider clock output is stable.
//! \notes Note that the value will not invert when the fractional divider
//! \notes is taken out of or placed into clock-gated state.
//!
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdFracDivEmiStable(void)
{
    return HW_CLKCTRL_FRAC.B.EMI_STABLE;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get status of fractional divider transition for ref_cpu
//!
//! \retval Divider transition for ref_cpu
//!
//! \notes This read-only bitfield is for DIAGNOSTIC PURPOSES ONLY since the
//! \notes fractional divide should become stable quickly enough that this
//! \notes field will never need to be used by either device driver or
//! \notes application code. The value inverts when the new programmed fractional
//! \notes divide value has taken effect. Read this bit, program the new value,
//! \notes and when this bit inverts, the phase divider clock output is stable.
//! \notes Note that the value will not invert when the fractional divider
//! \notes is taken out of or placed into clock-gated state.
//!
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdFracDivCpuStable(void)
{
    return HW_CLKCTRL_FRAC.B.CPU_STABLE;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Enable/disable PLL outputs for USB PHY
//!
//! \fntype Function
//!
//! 0: 8-phase PLL outputs for USB PHY are powered
//! down. If set to 1, 8-phase PLL outputs for USB PHY
//! are powered up. Additionally, the UTMICLK120_GATE
//! and UTMICLK30_GATE must be deasserted in the
//! UTMI phy to enable USB operation.
//!
//! \param[in] bEnableUsbClks - TRUE to enable, FALSE to disable
//!
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_EnableUsbClks(bool bEnableUsbClks)
{
    // Enable or disable 8-phase PLL outputs for USB PHY
    if( bEnableUsbClks )
        BF_SET(CLKCTRL_PLLCTRL0,EN_USB_CLKS);
    else
        BF_CLR(CLKCTRL_PLLCTRL0,EN_USB_CLKS);

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Power the PLL
//!
//! \fntype Function
//!
//! PLL Power On (0 = PLL off; 1 = PLL On). Allow 10 us
//! after turning the PLL on before using the PLL as a
//! clock source. This is the time the PLL takes to lock to 480 MHz.
//!
//! \param[in] bPowerOn - TRUE to power, FALSE to unpower
//!
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_PowerPll(bool bPowerOn)
{
    // Power on PLL
    if(bPowerOn)
        BF_SET(CLKCTRL_PLLCTRL0,POWER);
    else
        BF_CLR(CLKCTRL_PLLCTRL0,POWER);

    // We may also be able to wait on HW_CLKCTRL_PLLCTRL1::LOCK.  When it becomes zero?
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Return PLL lock status
//!
//! \fntype Function
//!
//! \retval - PLL lock status
//!
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPllLock(void)
{
    return HW_CLKCTRL_PLLCTRL1.B.LOCK;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return PLL lock count
//!
//! \fntype Function
//!
//! \retval - PLL lock count
//!//////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPllLockCount(void)
{
    return HW_CLKCTRL_PLLCTRL1.B.LOCK_COUNT;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Force PLL lock
//!
//! \fntype Function
//!
//! \param[in] bForceLock - RESERVED
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetForceLock(bool bForceLock)
{
    if( bForceLock )
        BF_SET(CLKCTRL_PLLCTRL1,FORCE_LOCK);
    else
        BF_CLR(CLKCTRL_PLLCTRL1,FORCE_LOCK);
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

