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
#ifndef _DDI_ERRORDEFS_H
#define _DDI_ERRORDEFS_H

#include "../include/errordefs.h"
#include "../include/ddi_subgroups.h"

//commented out codes are ones that are not being used at all right now.

#define ERROR_DDI_UART_DEBUG_GROUP      (ERROR_MASK |DDI_UART_DEBUG_GROUP)
#define ERROR_DDI_UARTAPP_GROUP         (ERROR_MASK |DDI_UARTAPP_GROUP)
#define ERROR_DDI_LED_GROUP             (ERROR_MASK |DDI_LED_GROUP)
#define ERROR_DDI_TIMER_GROUP           (ERROR_MASK |DDI_TIMER_GROUP)
#define ERROR_DDI_PWM_OUTPUT_GROUP      (ERROR_MASK |DDI_PWM_OUTPUT_GROUP)
#define ERROR_DDI_ETM_GROUP				(ERROR_MASK |DDI_ETM_GROUP)
#define ERROR_DDI_SSP_GROUP             (ERROR_MASK |DDI_SSP_GROUP)
#define ERROR_DDI_I2C_GROUP             (ERROR_MASK |DDI_I2C_GROUP)
#define ERROR_DDI_LDL_GROUP             (ERROR_MASK |DDI_LDL_GROUP)
#define ERROR_DDI_USB_GROUP             (ERROR_MASK |DDI_USB_GROUP)
#define ERROR_DDI_LCDIF_GROUP           (ERROR_MASK |DDI_LCDIF_GROUP)
#define ERROR_DDI_ADC_GROUP             (ERROR_MASK |DDI_ADC_GROUP)
#define ERROR_DDI_RTC_GROUP             (ERROR_MASK |DDI_RTC_GROUP)
#define ERROR_DDI_ALARM_GROUP           (ERROR_MASK |DDI_ALARM_GROUP)
#define ERROR_DDI_FM_TUNER_GROUP        (ERROR_MASK |DDI_FM_TUNER_GROUP)
#define ERROR_DDI_LRADC_GROUP           (ERROR_MASK |DDI_LRADC_GROUP)
#define ERROR_DDI_GPIO_GROUP            (ERROR_MASK |DDI_GPIO_GROUP)
#define ERROR_DDI_DISPLAY_GROUP         (ERROR_MASK |DDI_DISPLAY_GROUP)
#define ERROR_DDI_PSWITCH_GROUP         (ERROR_MASK |DDI_PSWITCH_GROUP)
#define ERROR_DDI_BCM_GROUP             (ERROR_MASK |DDI_BCM_GROUP)
#define ERROR_DDI_DRI_GROUP             (ERROR_MASK |DDI_DRI_GROUP)
#define ERROR_DDI_CLOCKS_GROUP          (ERROR_MASK |DDI_CLOCKS_GROUP)
#define ERROR_DDI_MEDIABUFMGR_GROUP     (ERROR_MASK |DDI_MEDIABUFMGR_GROUP)
#define ERROR_DDI_NAND_GROUP            (ERROR_MASK |DDI_NAND_GROUP)
#define ERROR_DDI_MMC_GROUP             (ERROR_MASK |DDI_MMC_GROUP)
#define ERROR_DDI_DCP_GROUP             (ERROR_MASK |DDI_DCP_GROUP)
#define ERROR_DDI_POWER_GROUP           (ERROR_MASK |DDI_POWER_GROUP)

#endif//_DDI_ERRORDEFS_H 

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}
