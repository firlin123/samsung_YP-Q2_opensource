////////////////////////////////////////////////////////////////////////////////
//
// Filename: regsrtc.h
//
// Description: PIO Registers for RTC interface
//
// Xml Revision: 1.75
//
// Template revision: 3790
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) SigmaTel, Inc. Unpublished
//
// SigmaTel, Inc.
// Proprietary & Confidential
//
// This source code and the algorithms implemented therein constitute
// confidential information and may compromise trade secrets of SigmaTel, Inc.
// or its associates, and any unauthorized use thereof is prohibited.
//
////////////////////////////////////////////////////////////////////////////////
//
// WARNING!  THIS FILE IS AUTOMATICALLY GENERATED FROM XML.
//                DO NOT MODIFY THIS FILE DIRECTLY.
//
////////////////////////////////////////////////////////////////////////////////
//
// The following naming conventions are followed in this file.
//      XX_<module>_<regname>_<field>
//
// XX specifies the define / macro class
//      HW pertains to a register
//      BM indicates a Bit Mask
//      BF indicates a Bit Field macro
//
// <module> is the hardware module name which can be any of the following...
//      USB20 (Note when there is more than one copy of a given module, the
//      module name includes a number starting from 0 for the first instance
//      of that module)
//
// <regname> is the specific register within that module
//
// <field> is the specific bitfield within that <module>_<register>
//
// We also define the following...
//      hw_<module>_<regname>_t is typedef of anonymous union
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _REGSRTC_H
#define _REGSRTC_H  1

#include "regs.h"

#ifndef REGS_RTC_BASE
#define REGS_RTC_BASE (REGS_BASE + 0x0005C000)
#endif

////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_CTRL - Real-Time Clock Control Register
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        unsigned ALARM_IRQ_EN          :  1;
        unsigned ONEMSEC_IRQ_EN        :  1;
        unsigned ALARM_IRQ             :  1;
        unsigned ONEMSEC_IRQ           :  1;
        unsigned WATCHDOGEN            :  1;
        unsigned FORCE_UPDATE          :  1;
        unsigned SUPPRESS_COPY2ANALOG  :  1;
        unsigned RSVD0                 : 23;
        unsigned CLKGATE               :  1;
        unsigned SFTRST                :  1;
    } B;
} hw_rtc_ctrl_t;
#endif


//
// constants & macros for entire HW_RTC_CTRL register
//

#define HW_RTC_CTRL_ADDR      (REGS_RTC_BASE + 0x00000000)
#define HW_RTC_CTRL_SET_ADDR  (REGS_RTC_BASE + 0x00000004)
#define HW_RTC_CTRL_CLR_ADDR  (REGS_RTC_BASE + 0x00000008)
#define HW_RTC_CTRL_TOG_ADDR  (REGS_RTC_BASE + 0x0000000C)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_CTRL           (*(volatile hw_rtc_ctrl_t *) HW_RTC_CTRL_ADDR)
#define HW_RTC_CTRL_RD()      (HW_RTC_CTRL.U)
#define HW_RTC_CTRL_WR(v)     (HW_RTC_CTRL.U = (v))
#define HW_RTC_CTRL_SET(v)    ((*(volatile reg32_t *) HW_RTC_CTRL_SET_ADDR) = (v))
#define HW_RTC_CTRL_CLR(v)    ((*(volatile reg32_t *) HW_RTC_CTRL_CLR_ADDR) = (v))
#define HW_RTC_CTRL_TOG(v)    ((*(volatile reg32_t *) HW_RTC_CTRL_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_CTRL bitfields
//

//--- Register HW_RTC_CTRL, field SFTRST

#define BP_RTC_CTRL_SFTRST      31
#define BM_RTC_CTRL_SFTRST      0x80000000

#ifndef __LANGUAGE_ASM__
#define BF_RTC_CTRL_SFTRST(v)   ((((reg32_t) v) << 31) & BM_RTC_CTRL_SFTRST)
#else
#define BF_RTC_CTRL_SFTRST(v)   (((v) << 31) & BM_RTC_CTRL_SFTRST)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_CTRL_SFTRST(v)   BF_CS1(RTC_CTRL, SFTRST, v)
#endif

//--- Register HW_RTC_CTRL, field CLKGATE

#define BP_RTC_CTRL_CLKGATE      30
#define BM_RTC_CTRL_CLKGATE      0x40000000

#define BF_RTC_CTRL_CLKGATE(v)   (((v) << 30) & BM_RTC_CTRL_CLKGATE)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_CTRL_CLKGATE(v)   BF_CS1(RTC_CTRL, CLKGATE, v)
#endif

//--- Register HW_RTC_CTRL, field SUPPRESS_COPY2ANALOG

#define BP_RTC_CTRL_SUPPRESS_COPY2ANALOG      6
#define BM_RTC_CTRL_SUPPRESS_COPY2ANALOG      0x00000040

#define BF_RTC_CTRL_SUPPRESS_COPY2ANALOG(v)   (((v) << 6) & BM_RTC_CTRL_SUPPRESS_COPY2ANALOG)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_CTRL_SUPPRESS_COPY2ANALOG(v)   BF_CS1(RTC_CTRL, SUPPRESS_COPY2ANALOG, v)
#endif

//--- Register HW_RTC_CTRL, field FORCE_UPDATE

#define BP_RTC_CTRL_FORCE_UPDATE      5
#define BM_RTC_CTRL_FORCE_UPDATE      0x00000020

#define BF_RTC_CTRL_FORCE_UPDATE(v)   (((v) << 5) & BM_RTC_CTRL_FORCE_UPDATE)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_CTRL_FORCE_UPDATE(v)   BF_CS1(RTC_CTRL, FORCE_UPDATE, v)
#endif

//--- Register HW_RTC_CTRL, field WATCHDOGEN

#define BP_RTC_CTRL_WATCHDOGEN      4
#define BM_RTC_CTRL_WATCHDOGEN      0x00000010

#define BF_RTC_CTRL_WATCHDOGEN(v)   (((v) << 4) & BM_RTC_CTRL_WATCHDOGEN)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_CTRL_WATCHDOGEN(v)   BF_CS1(RTC_CTRL, WATCHDOGEN, v)
#endif

//--- Register HW_RTC_CTRL, field ONEMSEC_IRQ

#define BP_RTC_CTRL_ONEMSEC_IRQ      3
#define BM_RTC_CTRL_ONEMSEC_IRQ      0x00000008

#define BF_RTC_CTRL_ONEMSEC_IRQ(v)   (((v) << 3) & BM_RTC_CTRL_ONEMSEC_IRQ)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_CTRL_ONEMSEC_IRQ(v)   BF_CS1(RTC_CTRL, ONEMSEC_IRQ, v)
#endif

//--- Register HW_RTC_CTRL, field ALARM_IRQ

#define BP_RTC_CTRL_ALARM_IRQ      2
#define BM_RTC_CTRL_ALARM_IRQ      0x00000004

#define BF_RTC_CTRL_ALARM_IRQ(v)   (((v) << 2) & BM_RTC_CTRL_ALARM_IRQ)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_CTRL_ALARM_IRQ(v)   BF_CS1(RTC_CTRL, ALARM_IRQ, v)
#endif

//--- Register HW_RTC_CTRL, field ONEMSEC_IRQ_EN

#define BP_RTC_CTRL_ONEMSEC_IRQ_EN      1
#define BM_RTC_CTRL_ONEMSEC_IRQ_EN      0x00000002

#define BF_RTC_CTRL_ONEMSEC_IRQ_EN(v)   (((v) << 1) & BM_RTC_CTRL_ONEMSEC_IRQ_EN)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_CTRL_ONEMSEC_IRQ_EN(v)   BF_CS1(RTC_CTRL, ONEMSEC_IRQ_EN, v)
#endif

//--- Register HW_RTC_CTRL, field ALARM_IRQ_EN

#define BP_RTC_CTRL_ALARM_IRQ_EN      0
#define BM_RTC_CTRL_ALARM_IRQ_EN      0x00000001

#define BF_RTC_CTRL_ALARM_IRQ_EN(v)   (((v) << 0) & BM_RTC_CTRL_ALARM_IRQ_EN)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_CTRL_ALARM_IRQ_EN(v)   BF_CS1(RTC_CTRL, ALARM_IRQ_EN, v)
#endif


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_STAT - Real-Time Clock Status Register
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg8_t   RSVD0;
        reg8_t   NEW_REGS;
        reg8_t   STALE_REGS;
        unsigned RSVD1              :  3;
        unsigned XTAL32768_PRESENT  :  1;
        unsigned XTAL32000_PRESENT  :  1;
        unsigned WATCHDOG_PRESENT   :  1;
        unsigned ALARM_PRESENT      :  1;
        unsigned RTC_PRESENT        :  1;
    } B;
} hw_rtc_stat_t;
#endif


//
// constants & macros for entire HW_RTC_STAT register
//

#define HW_RTC_STAT_ADDR      (REGS_RTC_BASE + 0x00000010)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_STAT           (*(volatile hw_rtc_stat_t *) HW_RTC_STAT_ADDR)
#define HW_RTC_STAT_RD()      (HW_RTC_STAT.U)
#endif


//
// constants & macros for individual HW_RTC_STAT bitfields
//

//--- Register HW_RTC_STAT, field RTC_PRESENT

#define BP_RTC_STAT_RTC_PRESENT      31
#define BM_RTC_STAT_RTC_PRESENT      0x80000000

#ifndef __LANGUAGE_ASM__
#define BF_RTC_STAT_RTC_PRESENT(v)   ((((reg32_t) v) << 31) & BM_RTC_STAT_RTC_PRESENT)
#else
#define BF_RTC_STAT_RTC_PRESENT(v)   (((v) << 31) & BM_RTC_STAT_RTC_PRESENT)
#endif

//--- Register HW_RTC_STAT, field ALARM_PRESENT

#define BP_RTC_STAT_ALARM_PRESENT      30
#define BM_RTC_STAT_ALARM_PRESENT      0x40000000

#define BF_RTC_STAT_ALARM_PRESENT(v)   (((v) << 30) & BM_RTC_STAT_ALARM_PRESENT)

//--- Register HW_RTC_STAT, field WATCHDOG_PRESENT

#define BP_RTC_STAT_WATCHDOG_PRESENT      29
#define BM_RTC_STAT_WATCHDOG_PRESENT      0x20000000

#define BF_RTC_STAT_WATCHDOG_PRESENT(v)   (((v) << 29) & BM_RTC_STAT_WATCHDOG_PRESENT)

//--- Register HW_RTC_STAT, field XTAL32000_PRESENT

#define BP_RTC_STAT_XTAL32000_PRESENT      28
#define BM_RTC_STAT_XTAL32000_PRESENT      0x10000000

#define BF_RTC_STAT_XTAL32000_PRESENT(v)   (((v) << 28) & BM_RTC_STAT_XTAL32000_PRESENT)

//--- Register HW_RTC_STAT, field XTAL32768_PRESENT

#define BP_RTC_STAT_XTAL32768_PRESENT      27
#define BM_RTC_STAT_XTAL32768_PRESENT      0x08000000

#define BF_RTC_STAT_XTAL32768_PRESENT(v)   (((v) << 27) & BM_RTC_STAT_XTAL32768_PRESENT)

//--- Register HW_RTC_STAT, field STALE_REGS

#define BP_RTC_STAT_STALE_REGS      16
#define BM_RTC_STAT_STALE_REGS      0x00FF0000

#define BF_RTC_STAT_STALE_REGS(v)   (((v) << 16) & BM_RTC_STAT_STALE_REGS)

//--- Register HW_RTC_STAT, field NEW_REGS

#define BP_RTC_STAT_NEW_REGS      8
#define BM_RTC_STAT_NEW_REGS      0x0000FF00

#define BF_RTC_STAT_NEW_REGS(v)   (((v) << 8) & BM_RTC_STAT_NEW_REGS)


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_MILLISECONDS - Real-Time Clock Milliseconds Counter
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg32_t  COUNT;
    } B;
} hw_rtc_milliseconds_t;
#endif


//
// constants & macros for entire HW_RTC_MILLISECONDS register
//

#define HW_RTC_MILLISECONDS_ADDR      (REGS_RTC_BASE + 0x00000020)
#define HW_RTC_MILLISECONDS_SET_ADDR  (REGS_RTC_BASE + 0x00000024)
#define HW_RTC_MILLISECONDS_CLR_ADDR  (REGS_RTC_BASE + 0x00000028)
#define HW_RTC_MILLISECONDS_TOG_ADDR  (REGS_RTC_BASE + 0x0000002C)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_MILLISECONDS           (*(volatile hw_rtc_milliseconds_t *) HW_RTC_MILLISECONDS_ADDR)
#define HW_RTC_MILLISECONDS_RD()      (HW_RTC_MILLISECONDS.U)
#define HW_RTC_MILLISECONDS_WR(v)     (HW_RTC_MILLISECONDS.U = (v))
#define HW_RTC_MILLISECONDS_SET(v)    ((*(volatile reg32_t *) HW_RTC_MILLISECONDS_SET_ADDR) = (v))
#define HW_RTC_MILLISECONDS_CLR(v)    ((*(volatile reg32_t *) HW_RTC_MILLISECONDS_CLR_ADDR) = (v))
#define HW_RTC_MILLISECONDS_TOG(v)    ((*(volatile reg32_t *) HW_RTC_MILLISECONDS_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_MILLISECONDS bitfields
//

//--- Register HW_RTC_MILLISECONDS, field COUNT

#define BP_RTC_MILLISECONDS_COUNT      0
#define BM_RTC_MILLISECONDS_COUNT      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_RTC_MILLISECONDS_COUNT(v)   ((reg32_t) v)
#else
#define BF_RTC_MILLISECONDS_COUNT(v)   (v)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_MILLISECONDS_COUNT(v)   (HW_RTC_MILLISECONDS.B.COUNT = (v))
#endif


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_SECONDS - Real-Time Clock Seconds Counter
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg32_t  COUNT;
    } B;
} hw_rtc_seconds_t;
#endif


//
// constants & macros for entire HW_RTC_SECONDS register
//

#define HW_RTC_SECONDS_ADDR      (REGS_RTC_BASE + 0x00000030)
#define HW_RTC_SECONDS_SET_ADDR  (REGS_RTC_BASE + 0x00000034)
#define HW_RTC_SECONDS_CLR_ADDR  (REGS_RTC_BASE + 0x00000038)
#define HW_RTC_SECONDS_TOG_ADDR  (REGS_RTC_BASE + 0x0000003C)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_SECONDS           (*(volatile hw_rtc_seconds_t *) HW_RTC_SECONDS_ADDR)
#define HW_RTC_SECONDS_RD()      (HW_RTC_SECONDS.U)
#define HW_RTC_SECONDS_WR(v)     (HW_RTC_SECONDS.U = (v))
#define HW_RTC_SECONDS_SET(v)    ((*(volatile reg32_t *) HW_RTC_SECONDS_SET_ADDR) = (v))
#define HW_RTC_SECONDS_CLR(v)    ((*(volatile reg32_t *) HW_RTC_SECONDS_CLR_ADDR) = (v))
#define HW_RTC_SECONDS_TOG(v)    ((*(volatile reg32_t *) HW_RTC_SECONDS_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_SECONDS bitfields
//

//--- Register HW_RTC_SECONDS, field COUNT

#define BP_RTC_SECONDS_COUNT      0
#define BM_RTC_SECONDS_COUNT      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_RTC_SECONDS_COUNT(v)   ((reg32_t) v)
#else
#define BF_RTC_SECONDS_COUNT(v)   (v)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_SECONDS_COUNT(v)   (HW_RTC_SECONDS.B.COUNT = (v))
#endif


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_ALARM - Real-Time Clock Alarm Register
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg32_t  VALUE;
    } B;
} hw_rtc_alarm_t;
#endif


//
// constants & macros for entire HW_RTC_ALARM register
//

#define HW_RTC_ALARM_ADDR      (REGS_RTC_BASE + 0x00000040)
#define HW_RTC_ALARM_SET_ADDR  (REGS_RTC_BASE + 0x00000044)
#define HW_RTC_ALARM_CLR_ADDR  (REGS_RTC_BASE + 0x00000048)
#define HW_RTC_ALARM_TOG_ADDR  (REGS_RTC_BASE + 0x0000004C)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_ALARM           (*(volatile hw_rtc_alarm_t *) HW_RTC_ALARM_ADDR)
#define HW_RTC_ALARM_RD()      (HW_RTC_ALARM.U)
#define HW_RTC_ALARM_WR(v)     (HW_RTC_ALARM.U = (v))
#define HW_RTC_ALARM_SET(v)    ((*(volatile reg32_t *) HW_RTC_ALARM_SET_ADDR) = (v))
#define HW_RTC_ALARM_CLR(v)    ((*(volatile reg32_t *) HW_RTC_ALARM_CLR_ADDR) = (v))
#define HW_RTC_ALARM_TOG(v)    ((*(volatile reg32_t *) HW_RTC_ALARM_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_ALARM bitfields
//

//--- Register HW_RTC_ALARM, field VALUE

#define BP_RTC_ALARM_VALUE      0
#define BM_RTC_ALARM_VALUE      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_RTC_ALARM_VALUE(v)   ((reg32_t) v)
#else
#define BF_RTC_ALARM_VALUE(v)   (v)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_ALARM_VALUE(v)   (HW_RTC_ALARM.B.VALUE = (v))
#endif


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_WATCHDOG - Watchdog Timer Register
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg32_t  COUNT;
    } B;
} hw_rtc_watchdog_t;
#endif


//
// constants & macros for entire HW_RTC_WATCHDOG register
//

#define HW_RTC_WATCHDOG_ADDR      (REGS_RTC_BASE + 0x00000050)
#define HW_RTC_WATCHDOG_SET_ADDR  (REGS_RTC_BASE + 0x00000054)
#define HW_RTC_WATCHDOG_CLR_ADDR  (REGS_RTC_BASE + 0x00000058)
#define HW_RTC_WATCHDOG_TOG_ADDR  (REGS_RTC_BASE + 0x0000005C)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_WATCHDOG           (*(volatile hw_rtc_watchdog_t *) HW_RTC_WATCHDOG_ADDR)
#define HW_RTC_WATCHDOG_RD()      (HW_RTC_WATCHDOG.U)
#define HW_RTC_WATCHDOG_WR(v)     (HW_RTC_WATCHDOG.U = (v))
#define HW_RTC_WATCHDOG_SET(v)    ((*(volatile reg32_t *) HW_RTC_WATCHDOG_SET_ADDR) = (v))
#define HW_RTC_WATCHDOG_CLR(v)    ((*(volatile reg32_t *) HW_RTC_WATCHDOG_CLR_ADDR) = (v))
#define HW_RTC_WATCHDOG_TOG(v)    ((*(volatile reg32_t *) HW_RTC_WATCHDOG_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_WATCHDOG bitfields
//

//--- Register HW_RTC_WATCHDOG, field COUNT

#define BP_RTC_WATCHDOG_COUNT      0
#define BM_RTC_WATCHDOG_COUNT      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_RTC_WATCHDOG_COUNT(v)   ((reg32_t) v)
#else
#define BF_RTC_WATCHDOG_COUNT(v)   (v)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_WATCHDOG_COUNT(v)   (HW_RTC_WATCHDOG.B.COUNT = (v))
#endif


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_PERSISTENT0 - Persistent State Register 0
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        unsigned CLOCKSOURCE      :  1;
        unsigned ALARM_WAKE_EN    :  1;
        unsigned ALARM_EN         :  1;
        unsigned LCK_SECS         :  1;
        unsigned XTAL24MHZ_PWRUP  :  1;
        unsigned XTAL32KHZ_PWRUP  :  1;
        unsigned XTAL32_FREQ      :  1;
        unsigned ALARM_WAKE       :  1;
        unsigned MSEC_RES         :  5;
        unsigned DISABLE_XTALOK   :  1;
        unsigned LOWERBIAS        :  2;
        unsigned DISABLE_PSWITCH  :  1;
        unsigned AUTO_RESTART     :  1;
        unsigned SPARE_ANALOG     : 14;
    } B;
} hw_rtc_persistent0_t;
#endif


//
// constants & macros for entire HW_RTC_PERSISTENT0 register
//

#define HW_RTC_PERSISTENT0_ADDR      (REGS_RTC_BASE + 0x00000060)
#define HW_RTC_PERSISTENT0_SET_ADDR  (REGS_RTC_BASE + 0x00000064)
#define HW_RTC_PERSISTENT0_CLR_ADDR  (REGS_RTC_BASE + 0x00000068)
#define HW_RTC_PERSISTENT0_TOG_ADDR  (REGS_RTC_BASE + 0x0000006C)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_PERSISTENT0           (*(volatile hw_rtc_persistent0_t *) HW_RTC_PERSISTENT0_ADDR)
#define HW_RTC_PERSISTENT0_RD()      (HW_RTC_PERSISTENT0.U)
#define HW_RTC_PERSISTENT0_WR(v)     (HW_RTC_PERSISTENT0.U = (v))
#define HW_RTC_PERSISTENT0_SET(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT0_SET_ADDR) = (v))
#define HW_RTC_PERSISTENT0_CLR(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT0_CLR_ADDR) = (v))
#define HW_RTC_PERSISTENT0_TOG(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT0_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_PERSISTENT0 bitfields
//

//--- Register HW_RTC_PERSISTENT0, field SPARE_ANALOG

#define BP_RTC_PERSISTENT0_SPARE_ANALOG      18
#define BM_RTC_PERSISTENT0_SPARE_ANALOG      0xFFFC0000

#ifndef __LANGUAGE_ASM__
#define BF_RTC_PERSISTENT0_SPARE_ANALOG(v)   ((((reg32_t) v) << 18) & BM_RTC_PERSISTENT0_SPARE_ANALOG)
#else
#define BF_RTC_PERSISTENT0_SPARE_ANALOG(v)   (((v) << 18) & BM_RTC_PERSISTENT0_SPARE_ANALOG)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_SPARE_ANALOG(v)   BF_CS1(RTC_PERSISTENT0, SPARE_ANALOG, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field AUTO_RESTART

#define BP_RTC_PERSISTENT0_AUTO_RESTART      17
#define BM_RTC_PERSISTENT0_AUTO_RESTART      0x00020000

#define BF_RTC_PERSISTENT0_AUTO_RESTART(v)   (((v) << 17) & BM_RTC_PERSISTENT0_AUTO_RESTART)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_AUTO_RESTART(v)   BF_CS1(RTC_PERSISTENT0, AUTO_RESTART, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field DISABLE_PSWITCH

#define BP_RTC_PERSISTENT0_DISABLE_PSWITCH      16
#define BM_RTC_PERSISTENT0_DISABLE_PSWITCH      0x00010000

#define BF_RTC_PERSISTENT0_DISABLE_PSWITCH(v)   (((v) << 16) & BM_RTC_PERSISTENT0_DISABLE_PSWITCH)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_DISABLE_PSWITCH(v)   BF_CS1(RTC_PERSISTENT0, DISABLE_PSWITCH, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field LOWERBIAS

#define BP_RTC_PERSISTENT0_LOWERBIAS      14
#define BM_RTC_PERSISTENT0_LOWERBIAS      0x0000C000

#define BF_RTC_PERSISTENT0_LOWERBIAS(v)   (((v) << 14) & BM_RTC_PERSISTENT0_LOWERBIAS)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_LOWERBIAS(v)   BF_CS1(RTC_PERSISTENT0, LOWERBIAS, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field DISABLE_XTALOK

#define BP_RTC_PERSISTENT0_DISABLE_XTALOK      13
#define BM_RTC_PERSISTENT0_DISABLE_XTALOK      0x00002000

#define BF_RTC_PERSISTENT0_DISABLE_XTALOK(v)   (((v) << 13) & BM_RTC_PERSISTENT0_DISABLE_XTALOK)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_DISABLE_XTALOK(v)   BF_CS1(RTC_PERSISTENT0, DISABLE_XTALOK, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field MSEC_RES

#define BP_RTC_PERSISTENT0_MSEC_RES      8
#define BM_RTC_PERSISTENT0_MSEC_RES      0x00001F00

#define BF_RTC_PERSISTENT0_MSEC_RES(v)   (((v) << 8) & BM_RTC_PERSISTENT0_MSEC_RES)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_MSEC_RES(v)   BF_CS1(RTC_PERSISTENT0, MSEC_RES, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field ALARM_WAKE

#define BP_RTC_PERSISTENT0_ALARM_WAKE      7
#define BM_RTC_PERSISTENT0_ALARM_WAKE      0x00000080

#define BF_RTC_PERSISTENT0_ALARM_WAKE(v)   (((v) << 7) & BM_RTC_PERSISTENT0_ALARM_WAKE)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_ALARM_WAKE(v)   BF_CS1(RTC_PERSISTENT0, ALARM_WAKE, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field XTAL32_FREQ

#define BP_RTC_PERSISTENT0_XTAL32_FREQ      6
#define BM_RTC_PERSISTENT0_XTAL32_FREQ      0x00000040

#define BF_RTC_PERSISTENT0_XTAL32_FREQ(v)   (((v) << 6) & BM_RTC_PERSISTENT0_XTAL32_FREQ)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_XTAL32_FREQ(v)   BF_CS1(RTC_PERSISTENT0, XTAL32_FREQ, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field XTAL32KHZ_PWRUP

#define BP_RTC_PERSISTENT0_XTAL32KHZ_PWRUP      5
#define BM_RTC_PERSISTENT0_XTAL32KHZ_PWRUP      0x00000020

#define BF_RTC_PERSISTENT0_XTAL32KHZ_PWRUP(v)   (((v) << 5) & BM_RTC_PERSISTENT0_XTAL32KHZ_PWRUP)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_XTAL32KHZ_PWRUP(v)   BF_CS1(RTC_PERSISTENT0, XTAL32KHZ_PWRUP, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field XTAL24MHZ_PWRUP

#define BP_RTC_PERSISTENT0_XTAL24MHZ_PWRUP      4
#define BM_RTC_PERSISTENT0_XTAL24MHZ_PWRUP      0x00000010

#define BF_RTC_PERSISTENT0_XTAL24MHZ_PWRUP(v)   (((v) << 4) & BM_RTC_PERSISTENT0_XTAL24MHZ_PWRUP)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_XTAL24MHZ_PWRUP(v)   BF_CS1(RTC_PERSISTENT0, XTAL24MHZ_PWRUP, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field LCK_SECS

#define BP_RTC_PERSISTENT0_LCK_SECS      3
#define BM_RTC_PERSISTENT0_LCK_SECS      0x00000008

#define BF_RTC_PERSISTENT0_LCK_SECS(v)   (((v) << 3) & BM_RTC_PERSISTENT0_LCK_SECS)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_LCK_SECS(v)   BF_CS1(RTC_PERSISTENT0, LCK_SECS, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field ALARM_EN

#define BP_RTC_PERSISTENT0_ALARM_EN      2
#define BM_RTC_PERSISTENT0_ALARM_EN      0x00000004

#define BF_RTC_PERSISTENT0_ALARM_EN(v)   (((v) << 2) & BM_RTC_PERSISTENT0_ALARM_EN)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_ALARM_EN(v)   BF_CS1(RTC_PERSISTENT0, ALARM_EN, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field ALARM_WAKE_EN

#define BP_RTC_PERSISTENT0_ALARM_WAKE_EN      1
#define BM_RTC_PERSISTENT0_ALARM_WAKE_EN      0x00000002

#define BF_RTC_PERSISTENT0_ALARM_WAKE_EN(v)   (((v) << 1) & BM_RTC_PERSISTENT0_ALARM_WAKE_EN)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_ALARM_WAKE_EN(v)   BF_CS1(RTC_PERSISTENT0, ALARM_WAKE_EN, v)
#endif

//--- Register HW_RTC_PERSISTENT0, field CLOCKSOURCE

#define BP_RTC_PERSISTENT0_CLOCKSOURCE      0
#define BM_RTC_PERSISTENT0_CLOCKSOURCE      0x00000001

#define BF_RTC_PERSISTENT0_CLOCKSOURCE(v)   (((v) << 0) & BM_RTC_PERSISTENT0_CLOCKSOURCE)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT0_CLOCKSOURCE(v)   BF_CS1(RTC_PERSISTENT0, CLOCKSOURCE, v)
#endif


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_PERSISTENT1 - Persistent State Register 1
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg32_t  GENERAL;
    } B;
} hw_rtc_persistent1_t;
#endif


//
// constants & macros for entire HW_RTC_PERSISTENT1 register
//

#define HW_RTC_PERSISTENT1_ADDR      (REGS_RTC_BASE + 0x00000070)
#define HW_RTC_PERSISTENT1_SET_ADDR  (REGS_RTC_BASE + 0x00000074)
#define HW_RTC_PERSISTENT1_CLR_ADDR  (REGS_RTC_BASE + 0x00000078)
#define HW_RTC_PERSISTENT1_TOG_ADDR  (REGS_RTC_BASE + 0x0000007C)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_PERSISTENT1           (*(volatile hw_rtc_persistent1_t *) HW_RTC_PERSISTENT1_ADDR)
#define HW_RTC_PERSISTENT1_RD()      (HW_RTC_PERSISTENT1.U)
#define HW_RTC_PERSISTENT1_WR(v)     (HW_RTC_PERSISTENT1.U = (v))
#define HW_RTC_PERSISTENT1_SET(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT1_SET_ADDR) = (v))
#define HW_RTC_PERSISTENT1_CLR(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT1_CLR_ADDR) = (v))
#define HW_RTC_PERSISTENT1_TOG(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT1_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_PERSISTENT1 bitfields
//

//--- Register HW_RTC_PERSISTENT1, field GENERAL

#define BP_RTC_PERSISTENT1_GENERAL      0
#define BM_RTC_PERSISTENT1_GENERAL      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_RTC_PERSISTENT1_GENERAL(v)   ((reg32_t) v)
#else
#define BF_RTC_PERSISTENT1_GENERAL(v)   (v)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT1_GENERAL(v)   (HW_RTC_PERSISTENT1.B.GENERAL = (v))
#endif

#define BV_RTC_PERSISTENT1_GENERAL__SPARE3                 0x80000000:0x4000
#define BV_RTC_PERSISTENT1_GENERAL__SDRAM_BOOT             0x2000
#define BV_RTC_PERSISTENT1_GENERAL__ENUMERATE_500MA_TWICE  0x1000
#define BV_RTC_PERSISTENT1_GENERAL__USB_BOOT_PLAYER_MODE   0x0800
#define BV_RTC_PERSISTENT1_GENERAL__SKIP_CHECKDISK         0x0400
#define BV_RTC_PERSISTENT1_GENERAL__USB_LOW_POWER_MODE     0x0200
#define BV_RTC_PERSISTENT1_GENERAL__OTG_HNP_BIT            0x0100
#define BV_RTC_PERSISTENT1_GENERAL__OTG_ATL_ROLE_BIT       0x0080
#define BV_RTC_PERSISTENT1_GENERAL__SDRAM_CS_HI            0x0040
#define BV_RTC_PERSISTENT1_GENERAL__SDRAM_CS_LO            0x0020
#define BV_RTC_PERSISTENT1_GENERAL__SDRAM_NDX_3            0x0010
#define BV_RTC_PERSISTENT1_GENERAL__SDRAM_NDX_2            0x0008
#define BV_RTC_PERSISTENT1_GENERAL__SDRAM_NDX_1            0x0004
#define BV_RTC_PERSISTENT1_GENERAL__SDRAM_NDX_0            0x0002
#define BV_RTC_PERSISTENT1_GENERAL__ETM_ENABLE             0x0001


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_PERSISTENT2 - Persistent State Register 2
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg32_t  GENERAL;
    } B;
} hw_rtc_persistent2_t;
#endif


//
// constants & macros for entire HW_RTC_PERSISTENT2 register
//

#define HW_RTC_PERSISTENT2_ADDR      (REGS_RTC_BASE + 0x00000080)
#define HW_RTC_PERSISTENT2_SET_ADDR  (REGS_RTC_BASE + 0x00000084)
#define HW_RTC_PERSISTENT2_CLR_ADDR  (REGS_RTC_BASE + 0x00000088)
#define HW_RTC_PERSISTENT2_TOG_ADDR  (REGS_RTC_BASE + 0x0000008C)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_PERSISTENT2           (*(volatile hw_rtc_persistent2_t *) HW_RTC_PERSISTENT2_ADDR)
#define HW_RTC_PERSISTENT2_RD()      (HW_RTC_PERSISTENT2.U)
#define HW_RTC_PERSISTENT2_WR(v)     (HW_RTC_PERSISTENT2.U = (v))
#define HW_RTC_PERSISTENT2_SET(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT2_SET_ADDR) = (v))
#define HW_RTC_PERSISTENT2_CLR(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT2_CLR_ADDR) = (v))
#define HW_RTC_PERSISTENT2_TOG(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT2_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_PERSISTENT2 bitfields
//

//--- Register HW_RTC_PERSISTENT2, field GENERAL

#define BP_RTC_PERSISTENT2_GENERAL      0
#define BM_RTC_PERSISTENT2_GENERAL      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_RTC_PERSISTENT2_GENERAL(v)   ((reg32_t) v)
#else
#define BF_RTC_PERSISTENT2_GENERAL(v)   (v)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT2_GENERAL(v)   (HW_RTC_PERSISTENT2.B.GENERAL = (v))
#endif


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_PERSISTENT3 - Persistent State Register 3
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg32_t  GENERAL;
    } B;
} hw_rtc_persistent3_t;
#endif


//
// constants & macros for entire HW_RTC_PERSISTENT3 register
//

#define HW_RTC_PERSISTENT3_ADDR      (REGS_RTC_BASE + 0x00000090)
#define HW_RTC_PERSISTENT3_SET_ADDR  (REGS_RTC_BASE + 0x00000094)
#define HW_RTC_PERSISTENT3_CLR_ADDR  (REGS_RTC_BASE + 0x00000098)
#define HW_RTC_PERSISTENT3_TOG_ADDR  (REGS_RTC_BASE + 0x0000009C)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_PERSISTENT3           (*(volatile hw_rtc_persistent3_t *) HW_RTC_PERSISTENT3_ADDR)
#define HW_RTC_PERSISTENT3_RD()      (HW_RTC_PERSISTENT3.U)
#define HW_RTC_PERSISTENT3_WR(v)     (HW_RTC_PERSISTENT3.U = (v))
#define HW_RTC_PERSISTENT3_SET(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT3_SET_ADDR) = (v))
#define HW_RTC_PERSISTENT3_CLR(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT3_CLR_ADDR) = (v))
#define HW_RTC_PERSISTENT3_TOG(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT3_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_PERSISTENT3 bitfields
//

//--- Register HW_RTC_PERSISTENT3, field GENERAL

#define BP_RTC_PERSISTENT3_GENERAL      0
#define BM_RTC_PERSISTENT3_GENERAL      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_RTC_PERSISTENT3_GENERAL(v)   ((reg32_t) v)
#else
#define BF_RTC_PERSISTENT3_GENERAL(v)   (v)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT3_GENERAL(v)   (HW_RTC_PERSISTENT3.B.GENERAL = (v))
#endif


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_PERSISTENT4 - Persistent State Register 4
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg32_t  GENERAL;
    } B;
} hw_rtc_persistent4_t;
#endif


//
// constants & macros for entire HW_RTC_PERSISTENT4 register
//

#define HW_RTC_PERSISTENT4_ADDR      (REGS_RTC_BASE + 0x000000A0)
#define HW_RTC_PERSISTENT4_SET_ADDR  (REGS_RTC_BASE + 0x000000A4)
#define HW_RTC_PERSISTENT4_CLR_ADDR  (REGS_RTC_BASE + 0x000000A8)
#define HW_RTC_PERSISTENT4_TOG_ADDR  (REGS_RTC_BASE + 0x000000AC)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_PERSISTENT4           (*(volatile hw_rtc_persistent4_t *) HW_RTC_PERSISTENT4_ADDR)
#define HW_RTC_PERSISTENT4_RD()      (HW_RTC_PERSISTENT4.U)
#define HW_RTC_PERSISTENT4_WR(v)     (HW_RTC_PERSISTENT4.U = (v))
#define HW_RTC_PERSISTENT4_SET(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT4_SET_ADDR) = (v))
#define HW_RTC_PERSISTENT4_CLR(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT4_CLR_ADDR) = (v))
#define HW_RTC_PERSISTENT4_TOG(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT4_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_PERSISTENT4 bitfields
//

//--- Register HW_RTC_PERSISTENT4, field GENERAL

#define BP_RTC_PERSISTENT4_GENERAL      0
#define BM_RTC_PERSISTENT4_GENERAL      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_RTC_PERSISTENT4_GENERAL(v)   ((reg32_t) v)
#else
#define BF_RTC_PERSISTENT4_GENERAL(v)   (v)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT4_GENERAL(v)   (HW_RTC_PERSISTENT4.B.GENERAL = (v))
#endif


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_PERSISTENT5 - Persistent State Register 5
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg32_t  GENERAL;
    } B;
} hw_rtc_persistent5_t;
#endif


//
// constants & macros for entire HW_RTC_PERSISTENT5 register
//

#define HW_RTC_PERSISTENT5_ADDR      (REGS_RTC_BASE + 0x000000B0)
#define HW_RTC_PERSISTENT5_SET_ADDR  (REGS_RTC_BASE + 0x000000B4)
#define HW_RTC_PERSISTENT5_CLR_ADDR  (REGS_RTC_BASE + 0x000000B8)
#define HW_RTC_PERSISTENT5_TOG_ADDR  (REGS_RTC_BASE + 0x000000BC)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_PERSISTENT5           (*(volatile hw_rtc_persistent5_t *) HW_RTC_PERSISTENT5_ADDR)
#define HW_RTC_PERSISTENT5_RD()      (HW_RTC_PERSISTENT5.U)
#define HW_RTC_PERSISTENT5_WR(v)     (HW_RTC_PERSISTENT5.U = (v))
#define HW_RTC_PERSISTENT5_SET(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT5_SET_ADDR) = (v))
#define HW_RTC_PERSISTENT5_CLR(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT5_CLR_ADDR) = (v))
#define HW_RTC_PERSISTENT5_TOG(v)    ((*(volatile reg32_t *) HW_RTC_PERSISTENT5_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_PERSISTENT5 bitfields
//

//--- Register HW_RTC_PERSISTENT5, field GENERAL

#define BP_RTC_PERSISTENT5_GENERAL      0
#define BM_RTC_PERSISTENT5_GENERAL      0xFFFFFFFF

#ifndef __LANGUAGE_ASM__
#define BF_RTC_PERSISTENT5_GENERAL(v)   ((reg32_t) v)
#else
#define BF_RTC_PERSISTENT5_GENERAL(v)   (v)
#endif

#ifndef __LANGUAGE_ASM__
#define BW_RTC_PERSISTENT5_GENERAL(v)   (HW_RTC_PERSISTENT5.B.GENERAL = (v))
#endif


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_DEBUG - Real-Time Clock Debug Register
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        unsigned WATCHDOG_RESET       :  1;
        unsigned WATCHDOG_RESET_MASK  :  1;
        unsigned RSVD0                : 30;
    } B;
} hw_rtc_debug_t;
#endif


//
// constants & macros for entire HW_RTC_DEBUG register
//

#define HW_RTC_DEBUG_ADDR      (REGS_RTC_BASE + 0x000000C0)
#define HW_RTC_DEBUG_SET_ADDR  (REGS_RTC_BASE + 0x000000C4)
#define HW_RTC_DEBUG_CLR_ADDR  (REGS_RTC_BASE + 0x000000C8)
#define HW_RTC_DEBUG_TOG_ADDR  (REGS_RTC_BASE + 0x000000CC)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_DEBUG           (*(volatile hw_rtc_debug_t *) HW_RTC_DEBUG_ADDR)
#define HW_RTC_DEBUG_RD()      (HW_RTC_DEBUG.U)
#define HW_RTC_DEBUG_WR(v)     (HW_RTC_DEBUG.U = (v))
#define HW_RTC_DEBUG_SET(v)    ((*(volatile reg32_t *) HW_RTC_DEBUG_SET_ADDR) = (v))
#define HW_RTC_DEBUG_CLR(v)    ((*(volatile reg32_t *) HW_RTC_DEBUG_CLR_ADDR) = (v))
#define HW_RTC_DEBUG_TOG(v)    ((*(volatile reg32_t *) HW_RTC_DEBUG_TOG_ADDR) = (v))
#endif


//
// constants & macros for individual HW_RTC_DEBUG bitfields
//

//--- Register HW_RTC_DEBUG, field WATCHDOG_RESET_MASK

#define BP_RTC_DEBUG_WATCHDOG_RESET_MASK      1
#define BM_RTC_DEBUG_WATCHDOG_RESET_MASK      0x00000002

#define BF_RTC_DEBUG_WATCHDOG_RESET_MASK(v)   (((v) << 1) & BM_RTC_DEBUG_WATCHDOG_RESET_MASK)

#ifndef __LANGUAGE_ASM__
#define BW_RTC_DEBUG_WATCHDOG_RESET_MASK(v)   BF_CS1(RTC_DEBUG, WATCHDOG_RESET_MASK, v)
#endif

//--- Register HW_RTC_DEBUG, field WATCHDOG_RESET

#define BP_RTC_DEBUG_WATCHDOG_RESET      0
#define BM_RTC_DEBUG_WATCHDOG_RESET      0x00000001

#define BF_RTC_DEBUG_WATCHDOG_RESET(v)   (((v) << 0) & BM_RTC_DEBUG_WATCHDOG_RESET)


////////////////////////////////////////////////////////////////////////////////
//// HW_RTC_VERSION - Real-Time Clock Version Register
////////////////////////////////////////////////////////////////////////////////

#ifndef __LANGUAGE_ASM__
typedef union
{
    reg32_t  U;
    struct
    {
        reg16_t  STEP;
        reg8_t   MINOR;
        reg8_t   MAJOR;
    } B;
} hw_rtc_version_t;
#endif


//
// constants & macros for entire HW_RTC_VERSION register
//

#define HW_RTC_VERSION_ADDR      (REGS_RTC_BASE + 0x000000D0)

#ifndef __LANGUAGE_ASM__
#define HW_RTC_VERSION           (*(volatile hw_rtc_version_t *) HW_RTC_VERSION_ADDR)
#define HW_RTC_VERSION_RD()      (HW_RTC_VERSION.U)
#endif


//
// constants & macros for individual HW_RTC_VERSION bitfields
//

//--- Register HW_RTC_VERSION, field MAJOR

#define BP_RTC_VERSION_MAJOR      24
#define BM_RTC_VERSION_MAJOR      0xFF000000

#ifndef __LANGUAGE_ASM__
#define BF_RTC_VERSION_MAJOR(v)   ((((reg32_t) v) << 24) & BM_RTC_VERSION_MAJOR)
#else
#define BF_RTC_VERSION_MAJOR(v)   (((v) << 24) & BM_RTC_VERSION_MAJOR)
#endif

//--- Register HW_RTC_VERSION, field MINOR

#define BP_RTC_VERSION_MINOR      16
#define BM_RTC_VERSION_MINOR      0x00FF0000

#define BF_RTC_VERSION_MINOR(v)   (((v) << 16) & BM_RTC_VERSION_MINOR)

//--- Register HW_RTC_VERSION, field STEP

#define BP_RTC_VERSION_STEP      0
#define BM_RTC_VERSION_STEP      0x0000FFFF

#define BF_RTC_VERSION_STEP(v)   (((v) << 0) & BM_RTC_VERSION_STEP)


#endif // _REGSRTC_H

////////////////////////////////////////////////////////////////////////////////
