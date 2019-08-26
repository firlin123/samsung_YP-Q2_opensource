/*
 *  linux/include/asm-arm/arch-stmp37xx/stmp37xx.h
 *
 *  Copyright (C) 2005 Sigmatel Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __ASM_ARCH_STMP37XX_H
#define __ASM_ARCH_STMP37XX_H

#if defined(__ASSEMBLER__) && !defined(__LANGUAGE_ASM__)
# define __LANGUAGE_ASM__	1
#endif

/* Catch other includes that redefine BUSY, COPY or ABORT - this messes with our structures! */
#ifdef BUSY
# define OLD_BUSY BUSY
# undef BUSY
#endif
#ifdef COPY
# define OLD_COPY COPY
# undef COPY
#endif
#ifdef ABORT
# define OLD_ABORT ABORT
# undef ABORT
#endif

#include "37xx/regsapbh.h"
#include "37xx/regsapbx.h"
#include "37xx/regsaudioin.h"
#include "37xx/regsaudioout.h"
#include "37xx/regsclkctrl.h"
#include "37xx/regsdcp.h"
#include "37xx/regsdigctl.h"
#include "37xx/regsdram.h"
#include "37xx/regsdri.h"
#include "37xx/regsecc8.h"
#include "37xx/regsemi.h"
#include "37xx/regsgpiomon.h"
#include "37xx/regsgpmi.h"
#include "37xx/regsi2c.h"
#include "37xx/regsicoll.h"
#include "37xx/regsir.h"
#include "37xx/regslcdif.h"
#include "37xx/regslradc.h"
#include "37xx/regsocotp.h"
#include "37xx/regspinctrl.h"
#include "37xx/regspower.h"
#include "37xx/regspwm.h"
#include "37xx/regsrtc.h"
#include "37xx/regssaif.h"
#include "37xx/regsspdif.h"
#include "37xx/regsssp.h"
#include "37xx/regstimrot.h"
#include "37xx/regsuartapp.h"
#include "37xx/regsuartdbg.h"
#include "37xx/regsusbctrl.h"
#include "37xx/regsusbphy.h"

/* Restore definition of BUSY, COPY, ABORT */
#ifdef OLD_BUSY
# define BUSY OLD_BUSY
# undef OLD_BUSY
#endif
#ifdef OLD_COPY
# define COPY OLD_COPY
# undef OLD_COPY
#endif
#ifdef OLD_ABORT
# define ABORT OLD_ABORT
# undef OLD_ABORT
#endif

#endif /* __ASM_ARCH_STMP37XX_H */
