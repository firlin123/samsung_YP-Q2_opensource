
#ifndef _DDI_CLOCKS_COMMON_H
#define _DDI_CLOCKS_COMMON_H

#include "../include/types.h"
#include "../include/error.h"

#include "hw_clocks.h"
#include "ddi_clocks.h"
#include "ddi_clocks_errordefs.h"
#include "ddi_clocks_internal.h"

#ifdef __KERNEL__
 #include <asm/hardware.h>
 #include <linux/delay.h>
#else
 #include "linux_regs.h"
#endif

#endif  // _DDI_CLOCKS_COMMON_H
