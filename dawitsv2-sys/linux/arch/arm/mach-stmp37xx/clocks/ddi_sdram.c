/////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2005 SigmaTel, Inc.
//
//! \file ddi_sdram.c
//
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
//  Includes and external references
/////////////////////////////////////////////////////////////////////////////////

//#include <math.h>
#include "../include/types.h"
#include "../include/error.h"

#if 0
#ifndef STMP37xx //stub
#include <hw\hw_clocks.h>
#include <hw\hw_digctl.h>
#include "hw/emi/hw_emi.h"
#include <hw\hw_core.h>
#include <hw\mmu.h>
#include <registers\regsemi.h>
#include <registers\regsclkctrl.h>
#endif
#endif

#define DDI_CLOCKS_EMI_READY_TIMEOUT    1000


//
// refresh time & number of cycles come from SDRAM specification
//
#define	HW_EMIDRAMTIME_REFRESH_COUNTER(refresh_time, numberOfcycles) ((int)(62.5 * numberOfcycles / refresh_time))

//#pragma ghs section text=".ocram.text"   /// MUST BE IN OCRAM !!!!


/////////////////////////////////////////////////////////////////////////////////
//
//! \brief Set the EMICLK Divider, in a "safe" context.
//!
//! \fntype Function
//!
//! The Stmp36xx EMICLK Divider should not be changed while SDRAM (and maybe NOR)
//! is being accessed. Therefore, the current SDK (>= 4.2x) requires that the divider
//! not be changed after application startup (i.e. don't change it after
//! enabling multi-threading and Interrupt Service Routines and DMA activity, if
//! any of those might possible access SDRAM). Further, it requires that the code
//! that sets the divider (this routine) does not reference SDRAM; thus, this routine
//! is forced to reside in OCRAM.
//!
//! Therefore, this routine is the ONLY way one should change the EMICLK Divider,
//! and you should only call this routine when you know there will be no background
//! or interrupting accesses to SDRAM. The typical place to call this routine is from
//! app_PreTxInit(), which is specific to each application.
//!
//! It is important to call this routine in EVERY application that uses the EMI.
//! The Power Management Interface (PMI) typically assumes a specific value for the
//! EMICLK Divider, and its State Table is generated, based on that assumption.
//! Using the wrong divider can result in errors, such as SDRAM data corruptions.
//!
//! Just assuming the default bootup divider value (=1) is not sufficient, because the
//! path to startup (i.e. how the application was loaded and booted) can alter
//! the divider. For example, booting the application from NAND (involving ROM (with
//! potential patches) and probably a bootmanager) will usually result
//! in a different initial value for the divider then will loading and starting
//! from the JTAG debugger.
//!
//! \param[in] The new EMICLK Divider
//!
//! \retval <SUCCESS    If no error has occurred>
//! \retval
//!
//! \pre No background activities will use the EMI (i.e. no interrupts or DMA accessign SDRAM or NOR).
//!
//! \note A call to ddi_clocks_SetSDRAMTimings() should typically follow the call to this routine.
//
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetEmiClkDiv(uint16_t iNewDiv)
{

    return SUCCESS;
   
}


/////////////////////////////////////////////////////////////////////////////////
//
//! \brief Setup SDRAM for optimal 24Mhz timings
//!
//! \fntype Function
//!
//
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_SetupSDRAMTimings( void )
{
    return SUCCESS ;
}
//#pragma ghs section

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

