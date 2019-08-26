///////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_emi
//! @{
//
// Copyright (c) 2004-2008 SigmaTel, Inc.
//
//! \file    hw_emi.h
//! \brief   Definitions and prototypes for External Memory Interface (EMI) HW API
//!
//! The EMI HW API contains a set of defines, typedefs, and functions for
//! manipulating the STMP37xx EMI block.
//! EMI IC types include SDRAM, Mobile SDRAM, and Mobile DDR RAM.
//! This API assumes the following:
//! - The EMI HW API enables/configures its pins via HW_PINCTRL block.
//! - Device contention is managed external to the EMI HW API.
//! - Calls into EMI HW API will not occur while executing from an EMI device.
//! Apr8'08 : EMI (parallel) NOR flash interface not supported by this
//! EMI driver although some products in the STMP37xx series have the HW.
//! STMP3780 (Hua Shan) and its follow-on IC do not have parallel NOR interfaces.
//! STMP37xx <= STMP3710 & STMP3770 do not have an EMI, and they use On-Chip RAM only. 
//! \todo [PUBS] Add definitions for TBDs in this file. High priority.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef HW_EMI_H
#define HW_EMI_H

//#include <../include/types.h>
#include "hw_emi_errordefs.h"
#ifdef __KERNEL__
 #include <asm/hardware.h>
#else
 #include "linux_regs.h"
#endif

//#include <registers\regsemi.h>
//#include <registers\regspinctrl.h>
//#include <registers\regsclkctrl.h>



//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef void (*dram_init_fcn)(void);

typedef enum
{
    EMI_CLK_OFF = 0,
    EMI_CLK_6MHz = 6,
    EMI_CLK_24MHz = 24,
    EMI_CLK_48MHz = 48,
    EMI_CLK_60MHz = 60,
    EMI_CLK_96MHz = 96,
    EMI_CLK_120MHz = 120,
    EMI_CLK_133MHz = 133
} hw_emi_ClockState_t;


typedef enum
{
    EMI_2MB_DRAM = 2,
    EMI_4MB_DRAM = 4,
    EMI_8MB_DRAM = 8,
    EMI_16MB_DRAM = 16,
    EMI_32MB_DRAM = 32,
    EMI_64MB_DRAM = 64,
    EMI_128MB_DRAM = 128,
    EMI_256MB_DRAM = 256,
    EMI_512MB_DRAM = 512,

} hw_emi_TotalDramSize_t;



typedef enum
{
    PIN_VOLTAGE_1pt8V = 0,
    PIN_VOLTAGE_3pt3V = 1,
} TPinVoltage;

typedef enum
{
    PIN_DRIVE_4mA  = 0,
    PIN_DRIVE_8mA  = 1,
    PIN_DRIVE_12mA = 2
} TPinDrive;


//! \brief TBD
typedef enum
{
    //! \brief TBD
    CE0 = 1,
    CE1 = 2,
    CE2 = 4,
    CE3 = 8

} hw_emi_ChipSelectMask_t;

//! \brief TBD
typedef enum _hw_emi_MemType_t
{
    //! \brief TBD
    EMI_DEV_SDRAM,
    //! \brief TBD
    EMI_DEV_MOBILE_SDRAM,
    //! \brief TBD
    EMI_DEV_MOBILE_DDR,
    //! \brief TBD
    EMI_DEV_NOR
} hw_emi_MemType_t;


void hw_emi_ConfigureEmiPins(
        TPinVoltage pin_voltage,
        TPinDrive pin_drive_addr,
        TPinDrive pin_drive_data,
        TPinDrive pin_drive_ce,
        TPinDrive pin_drive_clk,
        TPinDrive pin_drive_ctrl);

uint8_t * emi_block_startaddress(void);
void hw_emi_DisableEmiPadKeepers(void);
void hw_emi_EnableDramSelfRefresh(bool bEnable);
void hw_emi_PrepareControllerForNewFrequency(hw_emi_ClockState_t EmiClockSetting,
    hw_emi_MemType_t MemType);

void hw_emi_ClearReset(void);
bool hw_emi_IsDramSupported(void);
bool hw_emi_IsControllerHalted(void);
void hw_emi_start_function_for_locating(void);
void emi_block_endddress(void);

void hw_emi_SetAutoMemorySelfRefreshIdleCounterTimeoutCycles(uint16_t CounterResetValue);
void hw_emi_SetMemorySelfRefeshAutoFlag(bool bEnable);
void hw_emi_EnterMemorySelfRefreshMode(bool bOnOff);


void hw_emi_SetAutoMemoryClockGateIdleCounterTimeoutCycles(uint16_t CounterResetValue);
void hw_emi_SetMemoryClockGateAutoFlag(bool bEnable);
void hw_emi_EnterMemoryClockGateMode(bool bOnOff);

#endif //#ifndef HW_EMI_H

///////////////////////////////////////////////////////////////////////////////
// End of file
///////////////////////////////////////////////////////////////////////////////
//! @}
