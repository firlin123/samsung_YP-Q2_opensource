#ifndef _HW_CLOCKS_ERRORDEFS_H
#define _HW_CLOCKS_ERRORDEFS_H

#include "../include/hw_errordefs.h"


#define ERROR_HW_PLL_GENERAL                                     (ERROR_HW_PLL_GROUP)

#define ERROR_HW_CLOCKS_GENERAL                                  (ERROR_HW_CLOCKS_GROUP)
#define ERROR_HW_CLOCKS_SET_PCLK_TIMEOUT                         (ERROR_HW_CLOCKS_GROUP + 1)
#define ERROR_HW_CLOCKS_SET_HCLK_TIMEOUT                         (ERROR_HW_CLOCKS_GROUP + 2)
#define ERROR_HW_CLOCKS_SET_XCLK_TIMEOUT                         (ERROR_HW_CLOCKS_GROUP + 3)
#define ERROR_HW_CLOCKS_SET_PLL_FREQ_TIMEOUT                     (ERROR_HW_CLOCKS_GROUP + 4)
#define ERROR_HW_CLOCKS_INVALID_AUTOSLOW_DIV                     (ERROR_HW_CLOCKS_GROUP + 5)


#define ERROR_HW_CLKCTRL_INVALID_LFR_VALUE                      (ERROR_HW_CLOCKS_GROUP)
#define ERROR_HW_CLKCTRL_INVALID_CP_VALUE                       (ERROR_HW_CLOCKS_GROUP + 0x01)
#define ERROR_HW_CLKCTRL_INVALID_DIV_VALUE                      (ERROR_HW_CLOCKS_GROUP + 0x02)
#define ERROR_HW_CLKCTRL_DIV_BY_ZERO                            (ERROR_HW_CLOCKS_GROUP + 0x03)
#define ERROR_HW_CLKCTRL_CLK_DIV_BUSY                           (ERROR_HW_CLOCKS_GROUP + 0x04)
#define ERROR_HW_CLKCTRL_CLK_GATED                              (ERROR_HW_CLOCKS_GROUP + 0x05)
#define ERROR_HW_CLKCTRL_REF_CLK_GATED                          (ERROR_HW_CLOCKS_GROUP + 0x06)
#define ERROR_HW_CLKCTRL_REF_CPU_GATED                          (ERROR_HW_CLOCKS_GROUP + 0x07)
#define ERROR_HW_CLKCTRL_REF_EMI_GATED                          (ERROR_HW_CLOCKS_GROUP + 0x08)
#define ERROR_HW_CLKCTRL_REF_IO_GATED                           (ERROR_HW_CLOCKS_GROUP + 0x09)
#define ERROR_HW_CLKCTRL_REF_PIX_GATED                          (ERROR_HW_CLOCKS_GROUP + 0x0A)
#define ERROR_HW_CLKCTRL_INVALID_GATE_VALUE                     (ERROR_HW_CLOCKS_GROUP + 0x0B)
#define ERROR_HW_CLKCTRL_INVALID_PARAM                          (ERROR_HW_CLOCKS_GROUP + 0x0C)
#define ERROR_HW_CLKCTRL_UNSUPPORTED_AUTOSLOW_COMPONENT         (ERROR_HW_CLOCKS_GROUP + 0x0D)
#endif//_HW_CLOCKS_ERRORDEFS_H 
