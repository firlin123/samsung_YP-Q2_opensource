
#ifndef _HW_CLOCKS_COMMON_H
#define _HW_CLOCKS_COMMON_H

#include "../include/types.h"
#include "../include/error.h"

#include "hw_clocks.h"
#include "hw_clocks_errordefs.h"

#ifdef __KERNEL__
 #include <asm/hardware.h>
 #include <linux/delay.h>
#else
 #include "linux_regs.h"
#endif

#endif  // _HW_CLOCKS_COMMON_H
