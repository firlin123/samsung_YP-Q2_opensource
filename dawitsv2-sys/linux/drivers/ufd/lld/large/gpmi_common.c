////////////////////////////////////////////////////////////////////////////////
//
// Filename: gpmi_common.c
//
// Description: Implementation file for various commonly useful GPMI code.
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

#include "gpmi_common.h"
#include <asm/arch/37xx/regsdigctl.h>
//#include <asm/arch/clocks.h>

#if (!HW_BRAZOU)
#include <asm/arch/37xx/regspinctrl.h>
#endif

#if HW_BRAZO
#include <asm/arch/37xx/regsbrazoio.h>
#endif

#include <asm/arch/digctl.h> 

//------------------------------------------------------------------------------
// constants

#if 1



#define GPMI_RESETN_PAD_MUX   1	//xxxx
#define GPMI_RESETN_PAD_LSB   12
#define GPMI_RESETN_PAD_FUNC  0

#define GPMI_RDN_PAD_MUX      1
#define GPMI_RDN_PAD_LSB      18
#define GPMI_RDN_PAD_FUNC     0

#define GPMI_WRN_PAD_MUX      1
#define GPMI_WRN_PAD_LSB     16
#define GPMI_WRN_PAD_FUNC     0

#define GPMI_CE0N_PAD_MUX     4		//xxxx
#define GPMI_CE0N_PAD_LSB     30
#define GPMI_CE0N_PAD_FUNC    1

#define GPMI_CE1N_PAD_MUX     4		//xxxx
#define GPMI_CE1N_PAD_LSB     28
#define GPMI_CE1N_PAD_FUNC    1

/* CE2, CE3 changes for TB1 - hcyun */ 
#define GPMI_CE2N_PAD_MUX     0		//xxxx  
#define GPMI_CE2N_PAD_LSB     28 
#define GPMI_CE2N_PAD_FUNC    2

#define GPMI_CE3N_PAD_MUX     0 		//xxxx
#define GPMI_CE3N_PAD_LSB     30 
#define GPMI_CE3N_PAD_FUNC    2  



#define GPMI_RDY0_PAD_MUX     1
#define GPMI_RDY0_PAD_LSB     6
#define GPMI_RDY0_PAD_FUNC    0

#define GPMI_RDY1_PAD_MUX     1
#define GPMI_RDY1_PAD_LSB     14
#define GPMI_RDY1_PAD_FUNC    0

#define GPMI_RDY2_PAD_MUX     1
#define GPMI_RDY2_PAD_LSB     8
#define GPMI_RDY2_PAD_FUNC    0

#define GPMI_RDY3_PAD_MUX     1
#define GPMI_RDY3_PAD_LSB     10
#define GPMI_RDY3_PAD_FUNC    0

#define GPMI_ADDR0_PAD_MUX    1
#define GPMI_ADDR0_PAD_LSB   0
#define GPMI_ADDR0_PAD_FUNC   0

#define GPMI_ADDR1_PAD_MUX    1
#define GPMI_ADDR1_PAD_LSB   2
#define GPMI_ADDR1_PAD_FUNC   0

#define GPMI_ADDR2_PAD_MUX    1
#define GPMI_ADDR2_PAD_LSB   4
#define GPMI_ADDR2_PAD_FUNC   0

#else
#define GPMI_RESETN_PAD_MUX   3	//xxxx
#define GPMI_RESETN_PAD_LSB   8
#define GPMI_RESETN_PAD_FUNC  0

#define GPMI_RDN_PAD_MUX      1
#define GPMI_RDN_PAD_LSB      2
#define GPMI_RDN_PAD_FUNC     0

#define GPMI_WRN_PAD_MUX      1
#define GPMI_WRN_PAD_LSB     10
#define GPMI_WRN_PAD_FUNC     0

#define GPMI_CE0N_PAD_MUX     6		//xxxx
#define GPMI_CE0N_PAD_LSB     0
#define GPMI_CE0N_PAD_FUNC    1

#define GPMI_CE1N_PAD_MUX     6		//xxxx
#define GPMI_CE1N_PAD_LSB     2
#define GPMI_CE1N_PAD_FUNC    1

/* CE2, CE3 changes for TB1 - hcyun */ 
#define GPMI_CE2N_PAD_MUX     0		//xxxx  
#define GPMI_CE2N_PAD_LSB     28 
#define GPMI_CE2N_PAD_FUNC    2

#define GPMI_CE3N_PAD_MUX     0 		//xxxx
#define GPMI_CE3N_PAD_LSB     30 
#define GPMI_CE3N_PAD_FUNC    2  

#define GPMI_RDY0_PAD_MUX     1
#define GPMI_RDY0_PAD_LSB     4
#define GPMI_RDY0_PAD_FUNC    0

#define GPMI_RDY1_PAD_MUX     1
#define GPMI_RDY1_PAD_LSB     0
#define GPMI_RDY1_PAD_FUNC    0

#define GPMI_RDY2_PAD_MUX     1
#define GPMI_RDY2_PAD_LSB     8
#define GPMI_RDY2_PAD_FUNC    0

#define GPMI_RDY3_PAD_MUX     1
#define GPMI_RDY3_PAD_LSB     6
#define GPMI_RDY3_PAD_FUNC    0

//#define GPMI_ADDR0_PAD_MUX    7
#define GPMI_ADDR0_PAD_MUX    1
#define GPMI_ADDR0_PAD_LSB   12
#define GPMI_ADDR0_PAD_FUNC   0

//#define GPMI_ADDR1_PAD_MUX    7
#define GPMI_ADDR1_PAD_MUX    1
#define GPMI_ADDR1_PAD_LSB   14
#define GPMI_ADDR1_PAD_FUNC   0

//#define GPMI_ADDR2_PAD_MUX    7
#define GPMI_ADDR2_PAD_MUX    1
#define GPMI_ADDR2_PAD_LSB   16
#define GPMI_ADDR2_PAD_FUNC   0

#endif

#define GPMI_DATA00_PAD_MUX   0
#define GPMI_DATA00_PAD_LSB   0
#define GPMI_DATA00_PAD_FUNC  0

#define GPMI_DATA01_PAD_MUX   0
#define GPMI_DATA01_PAD_LSB   2
#define GPMI_DATA01_PAD_FUNC  0

#define GPMI_DATA02_PAD_MUX   0
#define GPMI_DATA02_PAD_LSB   4
#define GPMI_DATA02_PAD_FUNC  0

#define GPMI_DATA03_PAD_MUX   0
#define GPMI_DATA03_PAD_LSB   6
#define GPMI_DATA03_PAD_FUNC  0

#define GPMI_DATA04_PAD_MUX   0
#define GPMI_DATA04_PAD_LSB   8
#define GPMI_DATA04_PAD_FUNC  0

#define GPMI_DATA05_PAD_MUX   0
#define GPMI_DATA05_PAD_LSB  10
#define GPMI_DATA05_PAD_FUNC  0

#define GPMI_DATA06_PAD_MUX   0
#define GPMI_DATA06_PAD_LSB  12
#define GPMI_DATA06_PAD_FUNC  0

#define GPMI_DATA07_PAD_MUX   0
#define GPMI_DATA07_PAD_LSB  14
#define GPMI_DATA07_PAD_FUNC  0

#define GPMI_DATA08_PAD_MUX   0
#define GPMI_DATA08_PAD_LSB  16
#define GPMI_DATA08_PAD_FUNC  0

#define GPMI_DATA09_PAD_MUX   0
#define GPMI_DATA09_PAD_LSB  18
#define GPMI_DATA09_PAD_FUNC  0

#define GPMI_DATA10_PAD_MUX   0
#define GPMI_DATA10_PAD_LSB  20
#define GPMI_DATA10_PAD_FUNC  0

#define GPMI_DATA11_PAD_MUX   0
#define GPMI_DATA11_PAD_LSB  22
#define GPMI_DATA11_PAD_FUNC  0

#define GPMI_DATA12_PAD_MUX   0
#define GPMI_DATA12_PAD_LSB  24
#define GPMI_DATA12_PAD_FUNC  0

#define GPMI_DATA13_PAD_MUX   0
#define GPMI_DATA13_PAD_LSB  26
#define GPMI_DATA13_PAD_FUNC  0

#define GPMI_DATA14_PAD_MUX   0
#define GPMI_DATA14_PAD_LSB  28
#define GPMI_DATA14_PAD_FUNC  0

#define GPMI_DATA15_PAD_MUX   0
#define GPMI_DATA15_PAD_LSB  30
#define GPMI_DATA15_PAD_FUNC  0


//------------------------------------------------------------------------------
// functions

#define GPMI_PINCTRL_SETUP(clr, set, name) \
    ((clr[GPMI_##name##_PAD_MUX] |= 0x3 << GPMI_##name##_PAD_LSB), \
     (set[GPMI_##name##_PAD_MUX] |= GPMI_##name##_PAD_FUNC << GPMI_##name##_PAD_LSB))  

#define GPMI_PINCTRL_UPDATE(i, clr, set) \
    ((*((reg32_t*) (HW_PINCTRL_MUXSEL0_CLR_ADDR + (0x10 * (i % 2)) + (0x100 * (i / 2)))) = clr), \
     (*((reg32_t*) (HW_PINCTRL_MUXSEL0_SET_ADDR + (0x10 * (i % 2)) + (0x100 * (i / 2)))) = set))


//------------------------------------------------------------------------------
// functions

void gpmi_enable(gpmi_bool_t use_16_bit_data, 
                 reg32_t cs_rdy_mask)
{

#if (HW_BRAZO && !SIMULATION)
    printk("%s: BRAZO Power on...\n", __FUNCTION__ ); 
    // Turn on peripheral power.
    BW_BRAZOIOCSR_PWRUP3_3V(1);
    BW_BRAZOIOCSR_PWRUP5V(1);
#endif

#if HW_3600
    printk("%s: E3600 Power on...\n", __FUNCTION__ ); 
#endif 

    // Bring GPMI out of soft reset and release clock gate.
    BF_CS2(GPMI_CTRL0, SFTRST, 0, CLKGATE, 0);

#if (!HW_BRAZOU)
    // Configure all of the pads that will be used for GPMI.
    gpmi_pad_enable(use_16_bit_data, cs_rdy_mask);
#endif
}


void gpmi_pad_enable(gpmi_bool_t use_16_bit_data, 
                     reg32_t cs_rdy_mask)
{
    // Only BrazoU doesn't have pinctrl support...
 
#if (!HW_BRAZOU)

    reg32_t mux_clr[8];
    reg32_t mux_set[8];
    int index;
    int board_option=0;

/*
	get_hw_option_type() return value
	if(rotaryb == 0x0 && rotarya == 0x0)              return 1;
	else if(rotaryb == 0x0 && rotarya == 0x1)      return 2;
       else if(rotaryb == 0x1 && rotarya == 0x0)      return 3;
       else if(rotaryb == 0x1 && rotarya == 0x1)      return 4;
        return 5;
*/
	board_option = get_hw_option_type();
#if VERBOSE
	printk("<UFD> %s: board_option =%d\n", __FUNCTION__, board_option ); 
#endif

    // Initialize clr and set arrays for PINCTRL MUX registers.
    for (index = 0; index < 8; index++)
    {
        mux_clr[index] = mux_set[index] = 0;
    }

    // Wake up PINCTRL for GPMI use (i.e. bring out of reset and clkgate).
    BF_CS2(PINCTRL_CTRL, SFTRST, 0, CLKGATE, 0);

    // Collect clr/set values for MUX registers.  Yes, this is tedious...
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, RESETN);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, RDN);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, WRN);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, ADDR0);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, ADDR1);
    //    GPMI_PINCTRL_SETUP(mux_clr, mux_set, ADDR2); - not used in NAND 
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA00);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA01);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA02);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA03);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA04);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA05);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA06);
    GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA07);

    if (cs_rdy_mask & 0x1)
    {
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, CE0N);
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, RDY0);    
    }

    if (cs_rdy_mask & 0x2)
    {
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, CE1N);
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, RDY1);    
    }

    if (cs_rdy_mask & 0x4)
    {
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, CE2N);    
	if(board_option == 1) {  // Option 1 => New Board option_rotary = 00
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, RDY2);
	}
    }

    if (cs_rdy_mask & 0x8)
    {
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, CE3N);
	if(board_option == 1) {  // Option 1 => New Board option_rotary = 00
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, RDY3);    
	}
    }

    if (use_16_bit_data)
    {
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA08);
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA09);
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA10);
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA11);
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA12);
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA13);
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA14);
        GPMI_PINCTRL_SETUP(mux_clr, mux_set, DATA15);
    }

    // Apply the clr/set values to each MUX that needs configured.
    for (index = 0; index < 8; index++)
    {
        if (mux_clr[index])
        {
	// TPRINTF(TP_MIN, ("HW_PINCTRL_MUXSEL%d_CLR(0x%x);\n", index, mux_clr[index]));
	// TPRINTF(TP_MIN, ("HW_PINCTRL_MUXSEL%d_SET(0x%x);\n", index, mux_set[index]));
            GPMI_PINCTRL_UPDATE(index, mux_clr[index], mux_set[index]);
        }
    }

#endif

}


void gpmi_disable()
{
#if (HW_BRAZO && !SIMULATION)
    // Turn off peripheral power.
    BW_BRAZOIOCSR_PWRUP5V(0);
    BW_BRAZOIOCSR_PWRUP3_3V(0);
#endif

    // Gate clocks to GPMI.
    BW_GPMI_CTRL0_CLKGATE(1);
}


hw_gpmi_debug_t gpmi_tprintf_debug(unsigned verbosity,
                                   hw_gpmi_debug_t debug)
{
    TPRINTF(verbosity, ("HW_GPMI_DEBUG = 0x%x\n", debug.U));
    TPRINTF(verbosity, ("    READYn =               4'b%d%d%d%d\n",
                        debug.B.READY3, debug.B.READY2, debug.B.READY1, debug.B.READY0));
    TPRINTF(verbosity, ("    WAIT_FOR_READY_ENDn =  4'b%d%d%d%d\n",
                        debug.B.WAIT_FOR_READY_END3, debug.B.WAIT_FOR_READY_END2, 
                        debug.B.WAIT_FOR_READY_END1, debug.B.WAIT_FOR_READY_END0));                        
    TPRINTF(verbosity, ("    SENSEn =               4'b%d%d%d%d\n",
                        debug.B.SENSE3, debug.B.SENSE2, debug.B.SENSE1, debug.B.SENSE0));
    TPRINTF(verbosity, ("    DMAREQn =              4'b%d%d%d%d\n",
                        debug.B.DMAREQ3, debug.B.DMAREQ2, debug.B.DMAREQ1, debug.B.DMAREQ0));
    TPRINTF(verbosity, ("    CMD_END =              4'b%d%d%d%d\n",
                        (debug.B.CMD_END & 0x8) / 0x8, 
                        (debug.B.CMD_END & 0x4) / 0x8, 
                        (debug.B.CMD_END & 0x2) / 0x2, 
                        debug.B.CMD_END & 0x1));
    TPRINTF(verbosity, ("    BUSY =                 1'b%d\n", debug.B.BUSY));
    TPRINTF(verbosity, ("    PIN_STATE =            3'b%d%d%d  ", 
                        (debug.B.PIN_STATE & 0x4) / 0x4, 
                        (debug.B.PIN_STATE & 0x2) / 0x2, 
                        debug.B.PIN_STATE & 0x1));
    switch (debug.B.PIN_STATE)
    {
        case BV_GPMI_DEBUG_PIN_STATE__PSM_IDLE:
            TPRINTF(verbosity, ("(%s)\n", "PSM_IDLE"));
            break;
        case BV_GPMI_DEBUG_PIN_STATE__PSM_BYTCNT:
            TPRINTF(verbosity, ("(%s)\n", "PSM_BYTCNT"));
            break;
        case BV_GPMI_DEBUG_PIN_STATE__PSM_ADDR:
            TPRINTF(verbosity, ("(%s)\n", "PSM_ADDR"));
            break;
        case BV_GPMI_DEBUG_PIN_STATE__PSM_STALL:
            TPRINTF(verbosity, ("(%s)\n", "PSM_STALL"));
            break;
        case BV_GPMI_DEBUG_PIN_STATE__PSM_STROBE:
            TPRINTF(verbosity, ("(%s)\n", "PSM_STROBE"));
            break;
        case BV_GPMI_DEBUG_PIN_STATE__PSM_ATARDY:
            TPRINTF(verbosity, ("(%s)\n", "PSM_ATARDY"));
            break;
        case BV_GPMI_DEBUG_PIN_STATE__PSM_DHOLD:
            TPRINTF(verbosity, ("(%s)\n", "PSM_DHOLD"));
            break;
        case BV_GPMI_DEBUG_PIN_STATE__PSM_DONE:
            TPRINTF(verbosity, ("(%s)\n", "PSM_DONE"));
            break;
        default:
            TPRINTF(verbosity, ("(ERROR - invalid PIN_STATE)\n"));
    }
    TPRINTF(verbosity, ("    MAIN_STATE =           4'b%d%d%d%d ", 
                        (debug.B.MAIN_STATE & 0x8) / 0x8, 
                        (debug.B.MAIN_STATE & 0x4) / 0x4,
                        (debug.B.MAIN_STATE & 0x2) / 0x2, 
                        debug.B.MAIN_STATE & 0x1));
    switch (debug.B.MAIN_STATE)
    {
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_IDLE:
            TPRINTF(verbosity, ("(%s)\n", "MSM_IDLE"));
            break;
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_BYTCNT:
            TPRINTF(verbosity, ("(%s)\n", "MSM_BYTCNT"));
            break;
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_WAITFE:
            TPRINTF(verbosity, ("(%s)\n", "MSM_WAITFE"));
            break;
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_WAITFR:
            TPRINTF(verbosity, ("(%s)\n", "MSM_WAITFR"));
            break;
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_DMAREQ:
            TPRINTF(verbosity, ("(%s)\n", "MSM_DMAREQ"));
            break;
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_DMAACK:
            TPRINTF(verbosity, ("(%s)\n", "MSM_DMAACK"));
            break;
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_WAITFF:
            TPRINTF(verbosity, ("(%s)\n", "MSM_WAITFF"));
            break;
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_LDFIFO:
            TPRINTF(verbosity, ("(%s)\n", "MSM_LDFIFO"));
            break;
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_LDDMAR:
            TPRINTF(verbosity, ("(%s)\n", "MSM_LDDMAR"));
            break;
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_RDCMP:
            TPRINTF(verbosity, ("(%s)\n", "MSM_RDCMP"));
            break;
        case BV_GPMI_DEBUG_MAIN_STATE__MSM_DONE:
            TPRINTF(verbosity, ("(%s)\n", "MSM_DONE"));
            break;
        default:
            TPRINTF(verbosity, ("(ERROR - invalid MAIN_STATE)\n"));
    }

    return debug;
}


unsigned gpmi_poll_debug(reg32_t mask, 
                         reg32_t match, 
                         unsigned timeout)
{
    hw_gpmi_debug_t debug;

    reg32_t init = HW_DIGCTL_HCLKCOUNT_RD();
    reg32_t ticks = 1;
    reg32_t now;

    TPRINTF(TP_MIN, ("gpmi_poll_debug: "
                     "mask = 0x%x, "
                     "match = 0x%x, "
                     "timeout = %d\n",
                     mask, match, timeout));

    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    while (ticks < timeout)
    {
        if ((debug.U & mask) == (match & mask))
        {
            TPRINTF(TP_MIN, ("gpmi_poll_debug: match found after %d hclk ticks\n", ticks));
            GPMI_TPRINTF_DEBUG(TP_MIN, debug);
            return ticks;
        }

        now = HW_DIGCTL_HCLKCOUNT_RD();

        if (now < init)
            ticks = ((0xFFFFFFFF - init + 1) + now);
        else
            ticks = (now - init);

        debug = HW_GPMI_DEBUG;
    }

    TPRINTF(TP_MIN, ("gpmi_poll_debug: no match found; timed out after %d hclk ticks\n", ticks));
    GPMI_TPRINTF_DEBUG(TP_MIN, debug);
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
// $Log: gpmi_common.c,v $
// Revision 1.9  2006/05/02 11:05:00  hcyun
// from main z-proj source
//
// Revision 1.9  2006/03/07 11:42:53  hcyun
// support GPMI CE2, CE3 for TB1
//
// Revision 1.8  2005/11/08 04:18:01  hcyun
// remove warning..
// chain itself to avoid missing irq problem.
//
// - hcyun
//
// Revision 1.7  2005/08/20 00:58:10  biglow
// - update rfs which is worked fib fixed chip only.
//
// Revision 1.6  2005/06/16 18:52:00  hcyun
// ..
// - hcyun
//
// Revision 1.5  2005/06/13 16:21:54  hcyun
// GPMI_A2 is not needed..
//
// - hcyun
//
// Revision 1.4  2005/06/08 13:50:25  hcyun
// minior cleanup.. GPMI enable debug message removed..
//
// - hcyun
//
// Revision 1.3  2005/06/06 15:37:47  hcyun
// remove clock control in gpmi_enable.. this is enabled by clocks.c in startup..
//
// - hcyun
//
// Revision 1.2  2005/05/15 23:01:38  hcyun
// lots of cleanup... not yet compiled..
//
// - hcyun
//
// Revision 1.5  2005/05/15 05:10:29  hcyun
// copy-back, cache-program, random-input, random-output added. and removed unnecessary functions & types. Now, it's sync with ufd's dma functions..
//
// - hcyun
//
// Revision 1.4  2005/05/11 04:11:02  hcyun
// - cache flush added for serial program
// - removed entire cache flush code of runtest.c
// - check status is added..
//
// Revision 1.3  2005/05/10 22:21:00  hcyun
// - Clock Control for GPMI : this must be moved to general clock control code
//   and it should be accessible through user space.
//
// - Currently only works for CS 0 (This is a h/w problem??? )
//
// - TODO:
//   generic clk control
//   full nand erase/program check status chain
//
// Revision 1.2  2005/05/06 21:31:01  hcyun
// - DMA Read ID is working...
// - descriptors : kmalloc/kfree with flush_cache_range()
// - buffers : consistent_alloc/free
//
// TODO: fixing other dma functions..
//
// Revision 1.1  2005/05/05 01:05:37  hcyun
// - Started using virt_to_phys.. not yet complete..
//
// Revision 1.39  2005/02/28 17:43:10  ttoelkes
// significant modification to clocking API
//
// Revision 1.38  2005/02/24 15:28:26  ttoelkes
// fixing reason nand tests are broken in brazo sims and emulation (need to ungate clock in clkctrl even in brazo)
//
// Revision 1.37  2005/02/21 03:23:59  ttoelkes
// major changes to 'clocks' api
//
// Revision 1.36  2005/02/18 20:18:22  cvance
// Fixed compile warnings for building non 3600 hardware.  #ifdef HW_3600
// should have been #if HW_3600.
//
// Revision 1.35  2005/02/08 17:10:32  ttoelkes
// updating 'clocks' library
//
// Revision 1.34  2005/01/31 18:51:25  ttoelkes
// updated standard 'gpmi_enable' function so that all GPMI tests now use
// PLL-derived GPMICLK
//
// Revision 1.33  2005/01/28 02:28:32  ttoelkes
// added basic CLKCTRL manipulation to common gpmi_enable() function
//
// Revision 1.32  2004/12/14 23:56:14  ttoelkes
// added pinmux control for GPMI_RESETN to 'gpmi_pad_enable()'
//
// Revision 1.31  2004/12/03 22:55:17  ttoelkes
// added simple comment as note
//
// Revision 1.30  2004/11/11 00:06:45  ttoelkes
// updated pinmux constants to reflect pinlist changes
//
// Revision 1.29  2004/10/12 17:43:56  ttoelkes
// short term fix to lack of pinctrl in brazo
//
// Revision 1.28  2004/10/06 04:18:07  ttoelkes
// removed reference to obsolete bitfield
//
// Revision 1.27  2004/09/29 17:26:39  ttoelkes
// code updates in preparation for simultaneous read on four nand devices
//
// Revision 1.26  2004/09/09 22:33:13  ttoelkes
// turned down verbosity of various messages
//
// Revision 1.25  2004/09/06 15:44:31  ttoelkes
// checkpointing state of gpmi tests all passing current sims
//
// Revision 1.24  2004/09/03 20:14:18  ttoelkes
// put GPMI_PROBECFG back in public constants section in header
//
// Revision 1.23  2004/09/03 19:05:38  ttoelkes
// implemented full pad enable; still needs to be optimized
//
// Revision 1.22  2004/09/03 16:02:55  ttoelkes
// eliding extraneous long comment that accidentally got checked in yesterday; fixing default verbosity
//
// Revision 1.21  2004/09/02 21:50:05  ttoelkes
//
// Revision 1.20  2004/08/30 14:34:38  ttoelkes
// tuned down timeout constants
//
// Revision 1.19  2004/08/26 17:13:23  ttoelkes
// corrected improper handling of ATA_IRQRDY_POLARITY bitfield
//
// Revision 1.18  2004/08/24 00:22:07  ttoelkes
// updated header and many of the tests to reflect recent GPMI register reorganization
//
// Revision 1.17  2004/08/18 23:16:57  ttoelkes
// fixed 'gpmi_enable' so it really enables all of the necessary pads
//
// Revision 1.16  2004/08/12 15:29:18  ttoelkes
// K9F1G08U0M read_status and read_id tests compile
//
// Revision 1.15  2004/08/11 21:50:28  ttoelkes
// significant chunk of the way to getting GPMI tests converted
//
// Revision 1.14  2004/08/10 23:53:28  ttoelkes
// partially converted to new header format pre-name changes
//
// Revision 1.13  2004/07/14 18:54:53  ttoelkes
// bringing recent work on test code up to date
//
// Revision 1.12  2004/06/18 18:54:00  ttoelkes
// Read Id test now passes for K9F1G08U0M in simulation and emulation
//
// Revision 1.11  2004/06/18 17:22:28  ttoelkes
// state of common code after getting good data back from nand in emulation
//
// Revision 1.10  2004/06/15 22:02:35  ttoelkes
// updated gpmi references for new register naming convention
//
// Revision 1.9  2004/06/15 21:20:49  ttoelkes
// checkpoint from GPMI test development
//
// Revision 1.8  2004/06/15 15:06:46  ttoelkes
// checkpointing work on read_id test
//
// Revision 1.7  2004/06/14 04:02:09  ttoelkes
// fixed debug statement oops; formatted some of the others while I'm
// at it
//
// Revision 1.6  2004/06/11 23:31:02  ttoelkes
// abstracting command blocks into common library functions
//
// Revision 1.5  2004/06/07 20:16:42  ttoelkes
// Read Status test passes in simulation on both HW models for these two nand flash models
//
// Revision 1.4  2004/06/04 22:58:37  ttoelkes
// device reset test works and passes; read status test works and fails; read id test in progress
//
// Revision 1.3  2004/06/03 21:29:33  ttoelkes
// delivered working device reset test; added more common infrastructure; fixed build for read id test
//
// Revision 1.2  2004/06/02 20:46:24  ttoelkes
// checkpointing recent GPMI work
//
// Revision 1.1  2004/06/01 05:09:44  dpadgett
// Add validation files
//
////////////////////////////////////////////////////////////////////////////////
