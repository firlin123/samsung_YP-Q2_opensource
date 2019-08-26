///////////////////////////////////////////////////////////////////////////////
//! \addtogroup ddi_emi
//! @{
//
// Copyright (c) 2004-2005 SigmaTel, Inc.
//
//! \file    ddi_emi.h
//! \brief   Definitions and prototypes for EMI DDI API
//
//! The EMI DDI API contains a set functions to for change EMI clock
//! speeds and configuring the correct EMI parameters for SDRAM
//! communication
//! \todo [PUBS] Add definitions for TBDs in this file. High priority.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef DDI_EMI_H
#define DDI_EMI_H

//#include "../include/types.h"
#include "hw_emi.h"
//#include "registers\regsemi.h"
//#include "registers\regspinctrl.h"
//#include "registers\regsclkctrl.h"

//------------------------------------------------------------------------------
// definitions
//------------------------------------------------------------------------------
#define DCC_RESYNC_NO  0
#define DCC_RESYNC_YES 1

// these are the fixed TLB entries that will be used by the EMI driver.  Currently,
// there is no multiclient API in place for reserving and release these TLB entries
// so they are being hard coded for now.

#define DDI_EMI_TLB_TEXT_LOCK_ELEMENT1 1
#define DDI_EMI_TLB_TEXT_LOCK_ELEMENT2 2
#define DDI_EMI_TLB_TEXT_LOCK_ELEMENT3 3

#define DDI_EMI_TLB_BSS_LOCK_ELEMENT1 4
#define DDI_EMI_TLB_BSS_LOCK_ELEMENT2 5

// Assuming a 4k page size, this should match the number of elements above
// this is hacky for now and will be fixed later when an TLB lockdown
// API is in place.
#define MAXIMUM_EMI_TEXT_SIZE_DESIRED 0x3000
#define MAXIMUM_EMI_BSS_SIZE_DESIRED 0x3000


//------------------------------------------------------------------------------
// DDI EMI data structure
//------------------------------------------------------------------------------

typedef struct ddi_emi_vars_tag{

    hw_emi_ClockState_t EmiClkSpeedState;
    hw_emi_MemType_t MemType;
    bool bAutoMemorySelfRefreshModeEnabled;
    bool bAutoMemoryClockGateModeEnabled;
    bool bStaticMemorySelfRefreshModeEnabled;
    bool bStaticMemoryClockGateModeEnabled;

}ddi_emi_vars_t;

/*
typedef struct ddi_emi_DramConfig{
    uint16_t twr_cycles;
    uint16_t trrd_picoseconds;
    uint16
*/


//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
RtStatus_t ddi_emi_LockDownMemory(void);

RtStatus_t ddi_emi_PreOsInit(hw_emi_MemType_t,
    hw_emi_ChipSelectMask_t ChipSelectMask,
        hw_emi_TotalDramSize_t TotalDramSize);

//void hw_emi_ChgEmiClkCrossMode(hw_emi_ClockState_t emi_freq/*uint32_t dcc_resync_enable*/);

RtStatus_t ddi_emi_EnterDramSelfRefreshMode(void);
RtStatus_t ddi_emi_LeaveDramSelfRefreshMode(void);

RtStatus_t ddi_emi_WaitForRefEmiRefXtalAndDccResyncNotBusy(void);
RtStatus_t ddi_emi_WaitForRefXtalAndDccResyncNotBusy(void);
RtStatus_t ddi_emi_WaitForRefEmiAndDccResyncNotBusy(void);

/*
void hw_emi_StartDramController(
        uint32_t simmemsel,
        hw_emi_ClockState_t emi_clk_freq,
        TEmiClkDelay emi_clk_delay,
        TPinVoltage pin_voltage,
        TPinDrive pin_drive_addr,
        TPinDrive pin_drive_data,
        TPinDrive pin_drive_ce,
        TPinDrive pin_drive_clk,
        TPinDrive pin_drive_ctrl,
        dram_init_fcn fcn,
        TEmiDevice device);
*/

RtStatus_t ddi_emi_EnterStaticSelfRefreshMode(void);
RtStatus_t ddi_emi_ExitStaticSelfRefreshMode(void);

void ddi_emi_PrepareForDramSelfRefreshMode(void);
RtStatus_t ddi_emi_ChangeClockFrequency (int emiclk_new);
RtStatus_t ddi_emi_OsDisabledChangeClockFrequency(hw_emi_ClockState_t EmiClockSetting);
RtStatus_t ddi_emi_PrepareControllerForClockSpeedTransition(hw_emi_ClockState_t EmiClockSetting);
hw_emi_ClockState_t ddi_emi_GetCurrentSpeedState(void);

void ddi_emi_EnterAutoMemoryClockGateMode(void);
void ddi_emi_ExitAutoMemoryClockGateMode(void);
void ddi_emi_EnterAutoMemorySelfRefreshMode(void);
void ddi_emi_ExitAutoMemorySelfRefreshMode(void);

void ddi_emi_start_function_for_locating(void);
void ddi_emi_end_function_for_locating(void);
int stmp37xx_enter_idle (int notused);
#endif
