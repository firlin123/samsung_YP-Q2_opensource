
#ifndef _DDI_POWER_COMMON_H
#define _DDI_POWER_COMMON_H

#include "../include/types.h"
#include "../include/error.h"                  // Common SigmaTel Error Codes
#include "hw_power.h"
#include "ddi_power.h"       // Driver API
#include "ddi_power_errordefs.h"

#ifdef __KERNEL__
 #include <asm/hardware.h>
 #include <linux/delay.h>
#else
 #include "linux_regs.h"
#endif

#endif  // _DDI_POWER_COMMON_H
