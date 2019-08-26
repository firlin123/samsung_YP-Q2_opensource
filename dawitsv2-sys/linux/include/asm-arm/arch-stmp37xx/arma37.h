#ifndef __ASM_ARCH_ARMA37_H__
#define __ASM_ARCH_ARMA37_H__

#include <asm/arch/stmp37xx.h>
#include <asm/arch/gpio.h>

/*
 * Revision Value
 */
#define CONFIG_ARMA37_REV_01_VAL        0x10    /* REV 0.1 */

/*
 * define revision number macro and
 * include suitable header(arma37_0*.h) according to revision number
 */
#if defined(CONFIG_ARMA37_REV_01)
# define CONFIG_ARMA37_REV      CONFIG_ARMA_REV_01_VAL
# define CONFIG_ARMA37_REV_DESC "MIZI ARMA37 Revision 0.1"
# include "arma37_01.h"
#endif

#endif /* __ASM_ARCH_ARMA37_H__ */
