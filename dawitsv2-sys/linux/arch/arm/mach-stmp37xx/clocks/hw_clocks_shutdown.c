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
//! \brief Resets the digital registers to their default state.
//!
//! \fntype Function
//!
//! The DCDC and power module will not be reset.  All the digital sections
//! of the chip will be reset.
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_ResetDigital(void)
{
    BF_SET(CLKCTRL_RESET,DIG);
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Resets the entire chip.  Same as power on reset.
//!
//! \fntype Function
//!
//! The entire chip will reset.
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_ResetChip(void)
{
    BF_SET(CLKCTRL_RESET,CHIP);
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
