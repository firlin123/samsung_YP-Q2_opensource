#include <../include/types.h>
#include <../include/error.h>
//#include <registers\regsdigctl.h>
//#include <hw\digctl\hw_digctl.h>

#include "hw_emi.h"
#include "hw_clocks.h"
#include "hw_dram_settings.h"


//#pragma ghs section text=".ocram.text"    /// MUST BE IN OCRAM !!!!

static uint8_t hw_emi_unused_var_for_memory_location;

uint8_t * emi_block_startaddress(void)
{
    // this does nothing.  Just used for memory location.
    return &hw_emi_unused_var_for_memory_location;
}


//* Function Specification *****************************************************
//!
//! \brief Initialize Pin Mux to enable all EMI pins
//!
//! This function sets each of the Pin Mux select registers to enable all the
//! EMI associated pins. In addition, it also set the voltage level and drive
//! strength as specified for each of the EMI pins. This routine only sets/clears
//! the bits necessary for those pins associated with EMI. No other Pin Mux
//! settings are changed.
//!
//! \param[in] pin_voltage    - Pin voltage assigned to each EMI pin.
//! \param[in] pin_drive_addr - Pin drive strength (mA) assigned to EMI address pins.
//! \param[in] pin_drive_data - Pin drive strength (mA) assigned to EMI data pins.
//! \param[in] pin_drive_ce   - Pin drive strength (mA) assigned to EMI chip select pins.
//! \param[in] pin_drive_ctrl - Pin drive strength (mA) assigned to EMI control pins.
//! \param[in] pin_drive_clk  - Pin drive strength (mA) assigned to EMI clock pins.
//!
//******************************************************************************
#ifndef __KERNEL__
void hw_emi_ConfigureEmiPins(
        TPinVoltage pin_voltage,
        TPinDrive pin_drive_addr,
        TPinDrive pin_drive_data,
        TPinDrive pin_drive_ce,
        TPinDrive pin_drive_clk,
        TPinDrive pin_drive_ctrl)
{
    //-------------------------------------------------------------------------
    // Enable the Pinmux by clearing the Soft Reset and Clock Gate
    //-------------------------------------------------------------------------
    HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE);

// Bank-0 EMI pins are needed for NOR flash only. These pins conflict with
// booting from NAND flash. This function needs to be reworked to account for
// this conflict. For now, just comment out the NOR flash pins.
#if 0
    //-------------------------------------------------------------------------
    // Bank-0 of the Pinmux contains EMI pins at 8-18 and 21-22.
    // First, set the voltage and drive strength of these pins as specified.
    // Second, set the pinmux value to 0x01 to enable the EMI connection.
    //-------------------------------------------------------------------------

    // Configure Bank-0 Pins 8-15 voltage and drive strength
    HW_PINCTRL_DRIVE1_CLR(
            BM_PINCTRL_DRIVE1_BANK0_PIN08_V | BM_PINCTRL_DRIVE1_BANK0_PIN08_MA |
            BM_PINCTRL_DRIVE1_BANK0_PIN09_V | BM_PINCTRL_DRIVE1_BANK0_PIN09_MA |
            BM_PINCTRL_DRIVE1_BANK0_PIN10_V | BM_PINCTRL_DRIVE1_BANK0_PIN10_MA |
            BM_PINCTRL_DRIVE1_BANK0_PIN11_V | BM_PINCTRL_DRIVE1_BANK0_PIN11_MA |
            BM_PINCTRL_DRIVE1_BANK0_PIN12_V | BM_PINCTRL_DRIVE1_BANK0_PIN12_MA |
            BM_PINCTRL_DRIVE1_BANK0_PIN13_V | BM_PINCTRL_DRIVE1_BANK0_PIN13_MA |
            BM_PINCTRL_DRIVE1_BANK0_PIN14_V | BM_PINCTRL_DRIVE1_BANK0_PIN14_MA |
            BM_PINCTRL_DRIVE1_BANK0_PIN15_V | BM_PINCTRL_DRIVE1_BANK0_PIN15_MA);

    HW_PINCTRL_DRIVE1_SET(
            BF_PINCTRL_DRIVE1_BANK0_PIN08_V(pin_voltage) | BF_PINCTRL_DRIVE1_BANK0_PIN08_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE1_BANK0_PIN09_V(pin_voltage) | BF_PINCTRL_DRIVE1_BANK0_PIN09_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE1_BANK0_PIN10_V(pin_voltage) | BF_PINCTRL_DRIVE1_BANK0_PIN10_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE1_BANK0_PIN11_V(pin_voltage) | BF_PINCTRL_DRIVE1_BANK0_PIN11_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE1_BANK0_PIN12_V(pin_voltage) | BF_PINCTRL_DRIVE1_BANK0_PIN12_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE1_BANK0_PIN13_V(pin_voltage) | BF_PINCTRL_DRIVE1_BANK0_PIN13_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE1_BANK0_PIN14_V(pin_voltage) | BF_PINCTRL_DRIVE1_BANK0_PIN14_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE1_BANK0_PIN15_V(pin_voltage) | BF_PINCTRL_DRIVE1_BANK0_PIN15_MA(pin_drive_addr));

    // Configure Bank-0 Pins 16-18 and 21-22 voltage and drive strength
    HW_PINCTRL_DRIVE2_CLR(
            BM_PINCTRL_DRIVE2_BANK0_PIN16_V | BM_PINCTRL_DRIVE2_BANK0_PIN16_MA |
            BM_PINCTRL_DRIVE2_BANK0_PIN17_V | BM_PINCTRL_DRIVE2_BANK0_PIN17_MA |
            BM_PINCTRL_DRIVE2_BANK0_PIN18_V | BM_PINCTRL_DRIVE2_BANK0_PIN18_MA |
            BM_PINCTRL_DRIVE2_BANK0_PIN21_V | BM_PINCTRL_DRIVE2_BANK0_PIN21_MA |
            BM_PINCTRL_DRIVE2_BANK0_PIN22_V | BM_PINCTRL_DRIVE2_BANK0_PIN22_MA);

    HW_PINCTRL_DRIVE2_SET(
            BF_PINCTRL_DRIVE2_BANK0_PIN16_V(pin_voltage) | BF_PINCTRL_DRIVE2_BANK0_PIN16_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE2_BANK0_PIN17_V(pin_voltage) | BF_PINCTRL_DRIVE2_BANK0_PIN17_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE2_BANK0_PIN18_V(pin_voltage) | BF_PINCTRL_DRIVE2_BANK0_PIN18_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE2_BANK0_PIN21_V(pin_voltage) | BF_PINCTRL_DRIVE2_BANK0_PIN21_MA(pin_drive_ctrl) |
            BF_PINCTRL_DRIVE2_BANK0_PIN22_V(pin_voltage) | BF_PINCTRL_DRIVE2_BANK0_PIN22_MA(pin_drive_ctrl));

    // Configure Bank-0 Pins 8-15 as EMI pins
    HW_PINCTRL_MUXSEL0_CLR(
            BM_PINCTRL_MUXSEL0_BANK0_PIN08 |
            BM_PINCTRL_MUXSEL0_BANK0_PIN09 |
            BM_PINCTRL_MUXSEL0_BANK0_PIN10 |
            BM_PINCTRL_MUXSEL0_BANK0_PIN11 |
            BM_PINCTRL_MUXSEL0_BANK0_PIN12 |
            BM_PINCTRL_MUXSEL0_BANK0_PIN13 |
            BM_PINCTRL_MUXSEL0_BANK0_PIN14 |
            BM_PINCTRL_MUXSEL0_BANK0_PIN15);

    HW_PINCTRL_MUXSEL0_SET(
            BF_PINCTRL_MUXSEL0_BANK0_PIN08(1) |
            BF_PINCTRL_MUXSEL0_BANK0_PIN09(1) |
            BF_PINCTRL_MUXSEL0_BANK0_PIN10(1) |
            BF_PINCTRL_MUXSEL0_BANK0_PIN11(1) |
            BF_PINCTRL_MUXSEL0_BANK0_PIN12(1) |
            BF_PINCTRL_MUXSEL0_BANK0_PIN13(1) |
            BF_PINCTRL_MUXSEL0_BANK0_PIN14(1) |
            BF_PINCTRL_MUXSEL0_BANK0_PIN15(1));

    // Configure Bank-0 Pins 16-18 and 21-22 as EMI pins
    HW_PINCTRL_MUXSEL1_CLR(
            BM_PINCTRL_MUXSEL1_BANK0_PIN16 |
            BM_PINCTRL_MUXSEL1_BANK0_PIN17 |
            BM_PINCTRL_MUXSEL1_BANK0_PIN18 |
            BM_PINCTRL_MUXSEL1_BANK0_PIN21 |
            BM_PINCTRL_MUXSEL1_BANK0_PIN22);

    HW_PINCTRL_MUXSEL1_SET(
            BF_PINCTRL_MUXSEL1_BANK0_PIN16(1) |
            BF_PINCTRL_MUXSEL1_BANK0_PIN17(1) |
            BF_PINCTRL_MUXSEL1_BANK0_PIN18(1) |
            BF_PINCTRL_MUXSEL1_BANK0_PIN21(1) |
            BF_PINCTRL_MUXSEL1_BANK0_PIN22(1));
#endif //NOR flash

    //-------------------------------------------------------------------------
    // Bank-1 of the Pinmux does not contain any EMI pins so no changes
    // are made to MUXSEL2 or MUXSEL3 registers
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // Bank-2 of the Pinmux contains EMI pins at 9-31.
    // First, set the voltage and drive strength of these pins as specified.
    // Second, set the pinmux value to 0x0 to enable the EMI connection.
    //-------------------------------------------------------------------------

// Bank-2 Pins 14,15 (emi_ce2n, emi_ce3n) conflict with booting from NAND flash.
// For now comment out these pins.

    // Configure Bank-2 Pins 9-15 voltage and drive strength
    HW_PINCTRL_DRIVE9_CLR(
            BM_PINCTRL_DRIVE9_BANK2_PIN09_V | BM_PINCTRL_DRIVE9_BANK2_PIN09_MA |
            BM_PINCTRL_DRIVE9_BANK2_PIN10_V | BM_PINCTRL_DRIVE9_BANK2_PIN10_MA |
            BM_PINCTRL_DRIVE9_BANK2_PIN11_V | BM_PINCTRL_DRIVE9_BANK2_PIN11_MA |
            BM_PINCTRL_DRIVE9_BANK2_PIN12_V | BM_PINCTRL_DRIVE9_BANK2_PIN12_MA |
            BM_PINCTRL_DRIVE9_BANK2_PIN13_V | BM_PINCTRL_DRIVE9_BANK2_PIN13_MA
            );
//             BM_PINCTRL_DRIVE9_BANK2_PIN14_V | BM_PINCTRL_DRIVE9_BANK2_PIN14_MA |
//             BM_PINCTRL_DRIVE9_BANK2_PIN15_V | BM_PINCTRL_DRIVE9_BANK2_PIN15_MA);

    HW_PINCTRL_DRIVE9_SET(
            BF_PINCTRL_DRIVE9_BANK2_PIN09_V(pin_voltage) | BF_PINCTRL_DRIVE9_BANK2_PIN09_MA(pin_drive_clk)  |
            BF_PINCTRL_DRIVE9_BANK2_PIN10_V(pin_voltage) | BF_PINCTRL_DRIVE9_BANK2_PIN10_MA(pin_drive_ctrl) |
            BF_PINCTRL_DRIVE9_BANK2_PIN11_V(pin_voltage) | BF_PINCTRL_DRIVE9_BANK2_PIN11_MA(pin_drive_ctrl) |
            BF_PINCTRL_DRIVE9_BANK2_PIN12_V(pin_voltage) | BF_PINCTRL_DRIVE9_BANK2_PIN12_MA(pin_drive_ce)   |
            BF_PINCTRL_DRIVE9_BANK2_PIN13_V(pin_voltage) | BF_PINCTRL_DRIVE9_BANK2_PIN13_MA(pin_drive_ce)
            );
//             BF_PINCTRL_DRIVE9_BANK2_PIN14_V(pin_voltage) | BF_PINCTRL_DRIVE9_BANK2_PIN14_MA(pin_drive_ce)   |
//             BF_PINCTRL_DRIVE9_BANK2_PIN15_V(pin_voltage) | BF_PINCTRL_DRIVE9_BANK2_PIN15_MA(pin_drive_ce));

    // Configure Bank-2 Pins 16-23 voltage and drive strength
    HW_PINCTRL_DRIVE10_CLR(
            BM_PINCTRL_DRIVE10_BANK2_PIN16_V | BM_PINCTRL_DRIVE10_BANK2_PIN16_MA |
            BM_PINCTRL_DRIVE10_BANK2_PIN17_V | BM_PINCTRL_DRIVE10_BANK2_PIN17_MA |
            BM_PINCTRL_DRIVE10_BANK2_PIN18_V | BM_PINCTRL_DRIVE10_BANK2_PIN18_MA |
            BM_PINCTRL_DRIVE10_BANK2_PIN19_V | BM_PINCTRL_DRIVE10_BANK2_PIN19_MA |
            BM_PINCTRL_DRIVE10_BANK2_PIN20_V | BM_PINCTRL_DRIVE10_BANK2_PIN20_MA |
            BM_PINCTRL_DRIVE10_BANK2_PIN21_V | BM_PINCTRL_DRIVE10_BANK2_PIN21_MA |
            BM_PINCTRL_DRIVE10_BANK2_PIN22_V | BM_PINCTRL_DRIVE10_BANK2_PIN22_MA |
            BM_PINCTRL_DRIVE10_BANK2_PIN23_V | BM_PINCTRL_DRIVE10_BANK2_PIN23_MA);

    HW_PINCTRL_DRIVE10_SET(
            BF_PINCTRL_DRIVE10_BANK2_PIN16_V(pin_voltage) | BF_PINCTRL_DRIVE10_BANK2_PIN16_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE10_BANK2_PIN17_V(pin_voltage) | BF_PINCTRL_DRIVE10_BANK2_PIN17_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE10_BANK2_PIN18_V(pin_voltage) | BF_PINCTRL_DRIVE10_BANK2_PIN18_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE10_BANK2_PIN19_V(pin_voltage) | BF_PINCTRL_DRIVE10_BANK2_PIN19_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE10_BANK2_PIN20_V(pin_voltage) | BF_PINCTRL_DRIVE10_BANK2_PIN20_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE10_BANK2_PIN21_V(pin_voltage) | BF_PINCTRL_DRIVE10_BANK2_PIN21_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE10_BANK2_PIN22_V(pin_voltage) | BF_PINCTRL_DRIVE10_BANK2_PIN22_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE10_BANK2_PIN23_V(pin_voltage) | BF_PINCTRL_DRIVE10_BANK2_PIN23_MA(pin_drive_addr));

    // Configure Bank-2 Pins 24-31 voltage and drive strength
    HW_PINCTRL_DRIVE11_CLR(
            BM_PINCTRL_DRIVE11_BANK2_PIN24_V | BM_PINCTRL_DRIVE11_BANK2_PIN24_MA |
            BM_PINCTRL_DRIVE11_BANK2_PIN25_V | BM_PINCTRL_DRIVE11_BANK2_PIN25_MA |
            BM_PINCTRL_DRIVE11_BANK2_PIN26_V | BM_PINCTRL_DRIVE11_BANK2_PIN26_MA |
            BM_PINCTRL_DRIVE11_BANK2_PIN27_V | BM_PINCTRL_DRIVE11_BANK2_PIN27_MA |
            BM_PINCTRL_DRIVE11_BANK2_PIN28_V | BM_PINCTRL_DRIVE11_BANK2_PIN28_MA |
            BM_PINCTRL_DRIVE11_BANK2_PIN29_V | BM_PINCTRL_DRIVE11_BANK2_PIN29_MA |
            BM_PINCTRL_DRIVE11_BANK2_PIN30_V | BM_PINCTRL_DRIVE11_BANK2_PIN30_MA |
            BM_PINCTRL_DRIVE11_BANK2_PIN31_V | BM_PINCTRL_DRIVE11_BANK2_PIN31_MA);

    HW_PINCTRL_DRIVE11_SET(
            BF_PINCTRL_DRIVE11_BANK2_PIN24_V(pin_voltage) | BF_PINCTRL_DRIVE11_BANK2_PIN24_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE11_BANK2_PIN25_V(pin_voltage) | BF_PINCTRL_DRIVE11_BANK2_PIN25_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE11_BANK2_PIN26_V(pin_voltage) | BF_PINCTRL_DRIVE11_BANK2_PIN26_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE11_BANK2_PIN27_V(pin_voltage) | BF_PINCTRL_DRIVE11_BANK2_PIN27_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE11_BANK2_PIN28_V(pin_voltage) | BF_PINCTRL_DRIVE11_BANK2_PIN28_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE11_BANK2_PIN29_V(pin_voltage) | BF_PINCTRL_DRIVE11_BANK2_PIN29_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE11_BANK2_PIN30_V(pin_voltage) | BF_PINCTRL_DRIVE11_BANK2_PIN30_MA(pin_drive_addr) |
            BF_PINCTRL_DRIVE11_BANK2_PIN31_V(pin_voltage) | BF_PINCTRL_DRIVE11_BANK2_PIN31_MA(pin_drive_ctrl));

    // Configure Bank-2 Pins 9-15 as EMI pins
    HW_PINCTRL_MUXSEL4_CLR(
            BM_PINCTRL_MUXSEL4_BANK2_PIN09 |
            BM_PINCTRL_MUXSEL4_BANK2_PIN10 |
            BM_PINCTRL_MUXSEL4_BANK2_PIN11 |
            BM_PINCTRL_MUXSEL4_BANK2_PIN12 |
            BM_PINCTRL_MUXSEL4_BANK2_PIN13
            );
//             BM_PINCTRL_MUXSEL4_BANK2_PIN14 |
//             BM_PINCTRL_MUXSEL4_BANK2_PIN15);

    // Configure Bank-2 Pins 16-31 as EMI pins
    HW_PINCTRL_MUXSEL5_CLR(
            BM_PINCTRL_MUXSEL5_BANK2_PIN16 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN17 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN18 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN19 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN20 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN21 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN22 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN23 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN24 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN25 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN26 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN27 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN28 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN29 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN30 |
            BM_PINCTRL_MUXSEL5_BANK2_PIN31);

    //-------------------------------------------------------------------------
    // Bank-3 of the Pinmux contains EMI pins at 0-21.
    // First, set the voltage and drive strength of these pins as specified.
    // Second, set the pinmux value to 0x0 to enable the EMI connection
    //-------------------------------------------------------------------------

    // Configure Bank-3 Pins 0-7 voltage and drive strength
    HW_PINCTRL_DRIVE12_CLR(
            BM_PINCTRL_DRIVE12_BANK3_PIN00_V | BM_PINCTRL_DRIVE12_BANK3_PIN00_MA |
            BM_PINCTRL_DRIVE12_BANK3_PIN01_V | BM_PINCTRL_DRIVE12_BANK3_PIN01_MA |
            BM_PINCTRL_DRIVE12_BANK3_PIN02_V | BM_PINCTRL_DRIVE12_BANK3_PIN02_MA |
            BM_PINCTRL_DRIVE12_BANK3_PIN03_V | BM_PINCTRL_DRIVE12_BANK3_PIN03_MA |
            BM_PINCTRL_DRIVE12_BANK3_PIN04_V | BM_PINCTRL_DRIVE12_BANK3_PIN04_MA |
            BM_PINCTRL_DRIVE12_BANK3_PIN05_V | BM_PINCTRL_DRIVE12_BANK3_PIN05_MA |
            BM_PINCTRL_DRIVE12_BANK3_PIN06_V | BM_PINCTRL_DRIVE12_BANK3_PIN06_MA |
            BM_PINCTRL_DRIVE12_BANK3_PIN07_V | BM_PINCTRL_DRIVE12_BANK3_PIN07_MA);

    HW_PINCTRL_DRIVE12_SET(
            BF_PINCTRL_DRIVE12_BANK3_PIN00_V(pin_voltage) | BF_PINCTRL_DRIVE12_BANK3_PIN00_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE12_BANK3_PIN01_V(pin_voltage) | BF_PINCTRL_DRIVE12_BANK3_PIN01_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE12_BANK3_PIN02_V(pin_voltage) | BF_PINCTRL_DRIVE12_BANK3_PIN02_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE12_BANK3_PIN03_V(pin_voltage) | BF_PINCTRL_DRIVE12_BANK3_PIN03_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE12_BANK3_PIN04_V(pin_voltage) | BF_PINCTRL_DRIVE12_BANK3_PIN04_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE12_BANK3_PIN05_V(pin_voltage) | BF_PINCTRL_DRIVE12_BANK3_PIN05_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE12_BANK3_PIN06_V(pin_voltage) | BF_PINCTRL_DRIVE12_BANK3_PIN06_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE12_BANK3_PIN07_V(pin_voltage) | BF_PINCTRL_DRIVE12_BANK3_PIN07_MA(pin_drive_data));

    // Configure Bank-3 Pins 8-15 voltage and drive strength
    HW_PINCTRL_DRIVE13_CLR(
            BM_PINCTRL_DRIVE13_BANK3_PIN08_V | BM_PINCTRL_DRIVE13_BANK3_PIN08_MA |
            BM_PINCTRL_DRIVE13_BANK3_PIN09_V | BM_PINCTRL_DRIVE13_BANK3_PIN09_MA |
            BM_PINCTRL_DRIVE13_BANK3_PIN10_V | BM_PINCTRL_DRIVE13_BANK3_PIN10_MA |
            BM_PINCTRL_DRIVE13_BANK3_PIN11_V | BM_PINCTRL_DRIVE13_BANK3_PIN11_MA |
            BM_PINCTRL_DRIVE13_BANK3_PIN12_V | BM_PINCTRL_DRIVE13_BANK3_PIN12_MA |
            BM_PINCTRL_DRIVE13_BANK3_PIN13_V | BM_PINCTRL_DRIVE13_BANK3_PIN13_MA |
            BM_PINCTRL_DRIVE13_BANK3_PIN14_V | BM_PINCTRL_DRIVE13_BANK3_PIN14_MA |
            BM_PINCTRL_DRIVE13_BANK3_PIN15_V | BM_PINCTRL_DRIVE13_BANK3_PIN15_MA);

    HW_PINCTRL_DRIVE13_SET(
            BF_PINCTRL_DRIVE13_BANK3_PIN08_V(pin_voltage) | BF_PINCTRL_DRIVE13_BANK3_PIN08_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE13_BANK3_PIN09_V(pin_voltage) | BF_PINCTRL_DRIVE13_BANK3_PIN09_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE13_BANK3_PIN10_V(pin_voltage) | BF_PINCTRL_DRIVE13_BANK3_PIN10_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE13_BANK3_PIN11_V(pin_voltage) | BF_PINCTRL_DRIVE13_BANK3_PIN11_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE13_BANK3_PIN12_V(pin_voltage) | BF_PINCTRL_DRIVE13_BANK3_PIN12_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE13_BANK3_PIN13_V(pin_voltage) | BF_PINCTRL_DRIVE13_BANK3_PIN13_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE13_BANK3_PIN14_V(pin_voltage) | BF_PINCTRL_DRIVE13_BANK3_PIN14_MA(pin_drive_data) |
            BF_PINCTRL_DRIVE13_BANK3_PIN15_V(pin_voltage) | BF_PINCTRL_DRIVE13_BANK3_PIN15_MA(pin_drive_data));

    // Configure Bank-3 Pins 16-21 voltage and drive strength
    HW_PINCTRL_DRIVE14_CLR(
            BM_PINCTRL_DRIVE14_BANK3_PIN16_V | BM_PINCTRL_DRIVE14_BANK3_PIN16_MA |
            BM_PINCTRL_DRIVE14_BANK3_PIN17_V | BM_PINCTRL_DRIVE14_BANK3_PIN17_MA |
            BM_PINCTRL_DRIVE14_BANK3_PIN18_V | BM_PINCTRL_DRIVE14_BANK3_PIN18_MA |
            BM_PINCTRL_DRIVE14_BANK3_PIN19_V | BM_PINCTRL_DRIVE14_BANK3_PIN19_MA |
            BM_PINCTRL_DRIVE14_BANK3_PIN20_V | BM_PINCTRL_DRIVE14_BANK3_PIN20_MA |
            BM_PINCTRL_DRIVE14_BANK3_PIN21_V | BM_PINCTRL_DRIVE14_BANK3_PIN21_MA);

    HW_PINCTRL_DRIVE14_SET(
            BF_PINCTRL_DRIVE14_BANK3_PIN16_V(pin_voltage) | BF_PINCTRL_DRIVE14_BANK3_PIN16_MA(pin_drive_ctrl) |
            BF_PINCTRL_DRIVE14_BANK3_PIN17_V(pin_voltage) | BF_PINCTRL_DRIVE14_BANK3_PIN17_MA(pin_drive_ctrl) |
            BF_PINCTRL_DRIVE14_BANK3_PIN18_V(pin_voltage) | BF_PINCTRL_DRIVE14_BANK3_PIN18_MA(pin_drive_ctrl) |
            BF_PINCTRL_DRIVE14_BANK3_PIN19_V(pin_voltage) | BF_PINCTRL_DRIVE14_BANK3_PIN19_MA(pin_drive_ctrl) |
            BF_PINCTRL_DRIVE14_BANK3_PIN20_V(pin_voltage) | BF_PINCTRL_DRIVE14_BANK3_PIN20_MA(pin_drive_clk)  |
            BF_PINCTRL_DRIVE14_BANK3_PIN21_V(pin_voltage) | BF_PINCTRL_DRIVE14_BANK3_PIN21_MA(pin_drive_clk));

    // Configure Bank-3 Pins 0-15 as EMI pins
    HW_PINCTRL_MUXSEL6_CLR(
            BM_PINCTRL_MUXSEL6_BANK3_PIN00 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN01 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN02 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN03 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN04 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN05 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN06 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN07 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN08 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN09 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN10 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN11 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN12 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN13 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN14 |
            BM_PINCTRL_MUXSEL6_BANK3_PIN15);

    // Configure Bank-3 Pins 16-21 as EMI pins
    HW_PINCTRL_MUXSEL7_CLR(
            BM_PINCTRL_MUXSEL7_BANK3_PIN16 |
            BM_PINCTRL_MUXSEL7_BANK3_PIN17 |
            BM_PINCTRL_MUXSEL7_BANK3_PIN18 |
            BM_PINCTRL_MUXSEL7_BANK3_PIN19 |
            BM_PINCTRL_MUXSEL7_BANK3_PIN20 |
            BM_PINCTRL_MUXSEL7_BANK3_PIN21);
}

//* Function Specification *****************************************************
//!
//! \brief Disable Bus Keepers on EMI Pins
//!
//! This function disables the internal bus keepers on the EMI pins. This is only
//! necessary when connecting a Mobile DDR device to the DRAM controller.
//!
//******************************************************************************
void hw_emi_DisableEmiPadKeepers(void)
{
    // Enable the Pinmux by clearing the Soft Reset and Clock Gate
    HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE);

    // Disable the internal bus-keeper pins associated with EMI.
    HW_PINCTRL_PULL3_SET(

            BM_PINCTRL_PULL3_BANK3_PIN15 |
            BM_PINCTRL_PULL3_BANK3_PIN14 |
            BM_PINCTRL_PULL3_BANK3_PIN13 |
            BM_PINCTRL_PULL3_BANK3_PIN12 |
            BM_PINCTRL_PULL3_BANK3_PIN11 |
            BM_PINCTRL_PULL3_BANK3_PIN10 |
            BM_PINCTRL_PULL3_BANK3_PIN09 |
            BM_PINCTRL_PULL3_BANK3_PIN08 |
            BM_PINCTRL_PULL3_BANK3_PIN07 |
            BM_PINCTRL_PULL3_BANK3_PIN06 |
            BM_PINCTRL_PULL3_BANK3_PIN05 |
            BM_PINCTRL_PULL3_BANK3_PIN04 |
            BM_PINCTRL_PULL3_BANK3_PIN03 |
            BM_PINCTRL_PULL3_BANK3_PIN02 |
            BM_PINCTRL_PULL3_BANK3_PIN01 |
            BM_PINCTRL_PULL3_BANK3_PIN00);

// keepers needed to stop DQS toggling during high-Z state which causes data corruption            
            HW_PINCTRL_PULL3_CLR(
            BM_PINCTRL_PULL3_BANK3_PIN17 |
            BM_PINCTRL_PULL3_BANK3_PIN16);
}
#endif // not used in linux


void hw_emi_PrepareControllerForNewFrequency(hw_emi_ClockState_t EmiClockSetting,
    hw_emi_MemType_t MemType)
{
#if 0 //SDRAM support deleted
    if(MemType == EMI_DEV_MOBILE_SDRAM)
    {
        switch(EmiClockSetting){

            case EMI_CLK_6MHz:
                hw_dram_Init_mobile_sdram_k4m56163pg_7_5_6MHz_optimized();
                //hw_dram_Init_mobile_sdram_mt48h16m16lf_7_5_6MHz_optimized();
                break;

            case EMI_CLK_24MHz:
                hw_dram_Init_mobile_sdram_k4m56163pg_7_5_24MHz_optimized();
                //hw_dram_Init_mobile_sdram_mt48h16m16lf_7_5_24MHz_optimized();

                break;

            case EMI_CLK_48MHz:
                hw_dram_Init_mobile_sdram_k4m56163pg_7_5_48MHz_optimized();
                //hw_dram_Init_mobile_sdram_mt48h16m16lf_7_5_48MHz_optimized();
                break;

            case EMI_CLK_60MHz:
                hw_dram_Init_mobile_sdram_k4m56163pg_7_5_60MHz_optimized();
                //hw_dram_Init_mobile_sdram_mt48h16m16lf_7_5_60MHz_optimized();
                break;

            case EMI_CLK_96MHz:
                hw_dram_Init_mobile_sdram_k4m56163pg_7_5_96MHz_optimized();
                //hw_dram_Init_mobile_sdram_mt48h16m16lf_7_5_96MHz_optimized();
                break;

            case EMI_CLK_120MHz:
                hw_dram_Init_mobile_sdram_k4m56163pg_7_5_120MHz_optimized();
                //hw_dram_Init_mobile_sdram_mt48h16m16lf_7_5_120MHz_optimized();
                break;

            case EMI_CLK_133MHz:
                // 133MHz is not working yet.
                hw_dram_Init_mobile_sdram_k4m56163pg_7_5_120MHz_optimized();
                //hw_dram_Init_mobile_sdram_k4m56163pg_7_5_133MHz_optimized();
                //hw_dram_Init_mobile_sdram_mt48h16m16lf_7_5_133MHz_optimized();
                break;

            default:
                break;


       }
    }
    else
    if(MemType == EMI_DEV_SDRAM)
    {
        switch(EmiClockSetting){

            case EMI_CLK_6MHz:
                hw_dram_Init_sdram_mt48lc32m16a2_6MHz_optimized();
                break;

            case EMI_CLK_24MHz:
                hw_dram_Init_sdram_mt48lc32m16a2_24MHz_optimized();
                break;

            case EMI_CLK_48MHz:
                hw_dram_Init_sdram_mt48lc32m16a2_48MHz_optimized();
                break;

            case EMI_CLK_60MHz:
                hw_dram_Init_sdram_mt48lc32m16a2_60MHz_optimized();
                break;

            case EMI_CLK_96MHz:

                hw_dram_Init_sdram_mt48lc32m16a2_96MHz_optimized();
                break;


            case EMI_CLK_120MHz:
                hw_dram_Init_sdram_mt48lc32m16a2_120MHz_optimized();
                break;

            case EMI_CLK_133MHz:
                hw_dram_Init_sdram_mt48lc32m16a2_133MHz_optimized();
                break;


            default:
                break;


       }
    }
    else
#endif // sdram support deleted

    if(MemType == EMI_DEV_MOBILE_DDR)
    {
        switch(EmiClockSetting){

            case EMI_CLK_6MHz:
                hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_6MHz_optimized();
                break;

            case EMI_CLK_24MHz:

                hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_24MHz_optimized();
                break;

            case EMI_CLK_48MHz:
                hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_48MHz_optimized();
                break;

            case EMI_CLK_60MHz:
                hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_60MHz_optimized();
                break;

            case EMI_CLK_96MHz:
                hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_96MHz_optimized();
                break;

            case EMI_CLK_120MHz:
                hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_120MHz_optimized();
                break;

            case EMI_CLK_133MHz:
                hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_133MHz_optimized();
                break;
            default:
                break;
        }

    }
}


//#pragma ghs section
void hw_emi_SetAutoMemorySelfRefreshIdleCounterTimeoutCycles(uint16_t CounterResetValue)
{
    HW_DRAM_CTL29.B.LOWPOWER_EXTERNAL_CNT = CounterResetValue;
}

void hw_emi_SetMemorySelfRefeshAutoFlag(bool bEnable)
{
    if(bEnable)
        HW_DRAM_CTL16_SET(1<<9);
    else
        HW_DRAM_CTL16_CLR(1<<9);
}

void hw_emi_EnterMemorySelfRefreshMode(bool bOnOff)
{
    if(bOnOff)
        HW_DRAM_CTL16_SET(1<<17);
    else
        HW_DRAM_CTL16_CLR(1<<17);
}



void hw_emi_SetAutoMemoryClockGateIdleCounterTimeoutCycles(uint16_t CounterResetValue)
{
    HW_DRAM_CTL30.B.LOWPOWER_POWER_DOWN_CNT = CounterResetValue;
}

void hw_emi_SetMemoryClockGateAutoFlag(bool bEnable)
{
    if(bEnable)
        HW_DRAM_CTL16_SET(1<<11);
    else
        HW_DRAM_CTL16_CLR(1<<11);
}

void hw_emi_EnterMemoryClockGateMode(bool bOnOff)
{
    if(bOnOff)
        HW_DRAM_CTL16_SET(1<<19);
    else
        HW_DRAM_CTL16_CLR(1<<19);
}

void hw_emi_ClearReset(void)
{
    BF_CLR(EMI_CTRL,SFTRST);
    BF_CLR(EMI_CTRL,CLKGATE);
}


bool hw_emi_IsDramSupported(void)
{
    if (HW_EMI_STAT.B.DRAM_PRESENT)
        return true;
    else
        return false;
}

bool hw_emi_IsControllerHalted(void)
{

    return (BF_RD(EMI_STAT,DRAM_HALTED));

}


