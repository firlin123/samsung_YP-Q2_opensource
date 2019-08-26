////////////////////////////////////////////////////////////////////////////////
//! \addtogroup ddi_power
//! @{
//
// Copyright (c) 2004-2005 SigmaTel, Inc.
//
//! \file ddi_power.h
//! \brief Contains header data for the Power Supply Device Driver Interface.
//! \todo [PUBS] Add definitions for TBDs in this file. Medium priority.
////////////////////////////////////////////////////////////////////////////////
#ifndef _DDI_POWER_TRANSLATION_H
#define _DDI_POWER_TRANSLATION_H

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////
//! \brief TBD
typedef struct _ddi_pwr_Voltage_t
{
    //! \brief TBD
    uint16_t    u16Level_mVolts;
    //! \brief TBD
    uint16_t    u16Brownout_mVolts;
} ddi_pwr_Voltage_t;

// Old power translation functions
RtStatus_t ddi_pwr_GetVddd(ddi_pwr_Voltage_t* Volt);
void ddi_pwr_SetVddio(uint16_t Volt, uint16_t Bo);
RtStatus_t ddi_pwr_GetBattery(ddi_pwr_Voltage_t* Batt);
RtStatus_t ddi_pwr_PowerDown(void);
bool ddi_power_Get5vPresentFlag(void);
void ddi_pwr_Init(uint32_t Unused);
void ddi_pwr_WarmRestart(void);
void ddi_pwr_LeaveDcdcEnabledDuring5v(bool Enable);
typedef void ddi_pwr_MsgHandler_t(void);
RtStatus_t ddi_pwr_HandoffInit(ddi_pwr_MsgHandler_t    *pFxnHandoffStartCallback,
                                ddi_pwr_MsgHandler_t    *pFxnHandoffEndCallback,
                                ddi_pwr_MsgHandler_t    *pFxnHandoffTo5VoltCallback,
                                ddi_pwr_MsgHandler_t    *pFxnHandoffToBatteryCallback,
                                uint32_t                u32HandoffDebounce);

#endif // _DDI_POWER_H

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
