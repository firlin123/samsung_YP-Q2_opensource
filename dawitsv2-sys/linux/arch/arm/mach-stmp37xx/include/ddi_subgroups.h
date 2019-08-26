///////////////////////////////////////////////////////////////////////////////
//! \addtogroup common
//! @{
//
// Copyright (c) 2004-2007 SigmaTel, Inc.
//
//! \file ddi_errordefs.h
//! \brief Contains error codes for the DDI library
//! 
///////////////////////////////////////////////////////////////////////////////
#ifndef _DDI_SUBGROUPS_H
#define _DDI_SUBGROUPS_H

#include "../include/groups.h"

//commented out codes are ones that are not being used at all right now.

#define DDI_UART_DEBUG_GROUP      (DDI_GROUP|0x00000000)
#define DDI_LED_GROUP             (DDI_GROUP|0x00001000)
#define DDI_TIMER_GROUP           (DDI_GROUP|0x00002000)
#define DDI_PWM_OUTPUT_GROUP      (DDI_GROUP|0x00003000)
#define DDI_UARTAPP_GROUP         (DDI_GROUP|0x00004000)
#define DDI_ETM_GROUP             (DDI_GROUP|0x00005000)
#define DDI_SSP_GROUP             (DDI_GROUP|0x00006000)
#define DDI_I2C_GROUP             (DDI_GROUP|0x00007000)
#define DDI_LDL_GROUP             (DDI_GROUP|0x00008000)
#define DDI_USB_GROUP             (DDI_GROUP|0x0000A000)
#define DDI_LCDIF_GROUP           (DDI_GROUP|0x0000B000)
#define DDI_ADC_GROUP             (DDI_GROUP|0x0000D000)
#define DDI_RTC_GROUP             (DDI_GROUP|0x0000E000)
#define DDI_ALARM_GROUP           (DDI_GROUP|0x0000F000)
#define DDI_FM_TUNER_GROUP        (DDI_GROUP|0x00011000)
#define DDI_LRADC_GROUP           (DDI_GROUP|0x00012000)
#define DDI_GPIO_GROUP            (DDI_GROUP|0x00013000)
#define DDI_DISPLAY_GROUP         (DDI_GROUP|0x00015000)
#define DDI_PSWITCH_GROUP         (DDI_GROUP|0x00016000)
#define DDI_BCM_GROUP             (DDI_GROUP|0x00017000)
#define DDI_DRI_GROUP             (DDI_GROUP|0x00018000)
#define DDI_CLOCKS_GROUP          (DDI_GROUP|0x00019000)
#define DDI_MEDIABUFMGR_GROUP     (DDI_GROUP|0x0001f000)
#define DDI_NAND_GROUP            (DDI_GROUP|0x00020000)
#define DDI_MMC_GROUP             (DDI_GROUP|0x00021000)
#define DDI_DCP_GROUP             (DDI_GROUP|0x00022000)
#define DDI_POWER_GROUP           (DDI_GROUP|0x00022000)

#endif//_DDI_SUBGROUPS_H

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
