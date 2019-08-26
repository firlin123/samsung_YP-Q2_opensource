////////////////////////////////////////////////////////////////////////////////
//
// Filename: nand_common.c
//
// Description: Implementation file for various commonly useful GPMI code
//              for use when manipulating NAND devices.
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

#include "nand_common.h"


//------------------------------------------------------------------------------
// private constants

#define DMAREQ_TIMEOUT          10000
#define CMDEND_TIMEOUT          10000
#define WAIT_FOR_READY_TIMEOUT  10000


//------------------------------------------------------------------------------
// function definitions

void gpmi_nand_protect()
{
    TPRINTF(TP_MIN, ("enabling write protection for all nand devices\n"));
    BF_CLR(GPMI_CTRL1, DEV_RESET);
}


void gpmi_nand_unprotect()
{
    TPRINTF(TP_MIN, ("disabling write protection for all nand devices\n"));
    BF_SET(GPMI_CTRL1, DEV_RESET);
}


void gpmi_nand8_pio_config(reg8_t address_setup, 
                           reg8_t data_setup, 
                           reg8_t data_hold, 
                           reg16_t busy_timeout)
{
    // Setup pin timing parameters: ADRESS_SETUP, DATA_SETUP, and DATA_HOLD.
    // (Note that these are in units of GPMICLK cycles.)
    BF_CS3(GPMI_TIMING0, 
           ADDRESS_SETUP, address_setup, 
           DATA_SETUP, data_setup,
           DATA_HOLD, data_hold);

    // Setup device busy timeout.
    BW_GPMI_TIMING1_DEVICE_BUSY_TIMEOUT(busy_timeout);
    
    // Put GPMI in NAND mode, disable device reset, and make certain
    // IRQRDY polarity is active high.
    HW_GPMI_CTRL1_WR(BV_FLD(GPMI_CTRL1, GPMI_MODE, NAND) |
                     BV_FLD(GPMI_CTRL1, DEV_RESET, DISABLED) |
                     BV_FLD(GPMI_CTRL1, ATA_IRQRDY_POLARITY, ACTIVEHIGH));
}


gpmi_err_t gpmi_nand8_pio_write_cle(unsigned channel, 
                                    reg8_t cle, 
                                    gpmi_bool_t lock)
{
    gpmi_err_t err = 0;

    hw_gpmi_debug_t  debug;
    hw_gpmi_ctrl0_t  ctrl0;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_write_cle: "
                     "channel = %d, "
                     "cle = 0x%x, "
                     "lock = %d\n",
                     channel, cle, lock));

    // Capture value of debug.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    
    // Setup a one byte write command to specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BF_GPMI_CTRL0_LOCK_CS(lock) |
               BF_GPMI_CTRL0_XFER_COUNT(1) |
               BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    // Poll for dmareq to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_DMAREQ(channel), 
                         ~debug.U & GPMI_DEBUG_DMAREQ(channel), 
                         DMAREQ_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for dmareq\n\n"));
        err++;
    }

    // Write the data byte.
    HW_GPMI_DATA_WR(cle);

    // Poll for cmd end to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_CMD_END(channel), 
                         ~debug.U & GPMI_DEBUG_CMD_END(channel), 
                         CMDEND_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for cmd end\n\n"));
        err++;
    }

    return err;
}


gpmi_err_t gpmi_nand8_pio_write_cle_ale8(unsigned channel, 
                                         reg8_t cle, 
                                         reg8_t ale,
                                         gpmi_bool_t lock)
{
    gpmi_err_t err = 0;

    hw_gpmi_debug_t debug;
    hw_gpmi_ctrl0_t ctrl0;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_write_cle_ale8: "
                     "channel = %d, "
                     "cle = 0x%x, "
                     "ale = 0x%x, "
                     "lock = %d\n",
                     channel, cle, ale, lock));

    // Capture value of debug.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    
    // Setup a two byte write command to specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BF_GPMI_CTRL0_LOCK_CS(lock) |
               BF_GPMI_CTRL0_XFER_COUNT(2) |
               BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, ENABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    // Poll for dmareq to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_DMAREQ(channel), 
                         ~debug.U & GPMI_DEBUG_DMAREQ(channel), 
                         DMAREQ_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for dmareq\n\n"));
        err++;
    }

    // Write cle followed by ale.
    HW_GPMI_DATA_WR((ale << 8) | cle);

    // Poll for cmd end to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_CMD_END(channel), 
                         ~debug.U & GPMI_DEBUG_CMD_END(channel), 
                         CMDEND_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for cmd end\n\n"));
        err++;
    }

    return err;
}


gpmi_err_t gpmi_nand8_pio_write_ale8(unsigned channel, 
                                     reg8_t ale,
                                     gpmi_bool_t lock)
{
    gpmi_err_t err = 0;

    hw_gpmi_debug_t debug;
    hw_gpmi_ctrl0_t ctrl0;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_write_ale8: "
                     "channel = %d, "
                     "ale = 0x%x, "
                     "lock = %d\n",
                     channel, ale, lock));

    // Capture value of debug.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    
    // Setup a two byte write command to specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BF_GPMI_CTRL0_LOCK_CS(lock) |
               BF_GPMI_CTRL0_XFER_COUNT(1) |
               BV_FLD(GPMI_CTRL0, ADDRESS, NAND_ALE) |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    // Poll for dmareq to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_DMAREQ(channel), 
                         ~debug.U & GPMI_DEBUG_DMAREQ(channel), 
                         DMAREQ_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for dmareq\n\n"));
        err++;
    }

    // Write cle followed by ale.
    HW_GPMI_DATA_WR(ale);

    // Poll for cmd end to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_CMD_END(channel), 
                         ~debug.U & GPMI_DEBUG_CMD_END(channel), 
                         CMDEND_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for cmd end\n\n"));
        err++;
    }

    return err;
}


gpmi_err_t gpmi_nand8_pio_write_ale16(unsigned channel, 
                                      reg16_t ale,
                                      gpmi_bool_t lock)
{
    gpmi_err_t err = 0;

    hw_gpmi_debug_t debug;
    hw_gpmi_ctrl0_t ctrl0;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_write_ale16: "
                     "channel = %d, "
                     "ale = 0x%x, "
                     "lock = %d\n",
                     channel, ale, lock));

    // Capture value of debug.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    
    // Setup a two byte write command to specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BF_GPMI_CTRL0_LOCK_CS(lock) |
               BF_GPMI_CTRL0_XFER_COUNT(2) |
               BV_FLD(GPMI_CTRL0, ADDRESS, NAND_ALE) |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    // Poll for dmareq to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_DMAREQ(channel), 
                         ~debug.U & GPMI_DEBUG_DMAREQ(channel), 
                         DMAREQ_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for dmareq\n\n"));
        err++;
    }

    // Write cle followed by ale.
    HW_GPMI_DATA_WR(ale);

    // Poll for cmd end to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_CMD_END(channel), 
                         ~debug.U & GPMI_DEBUG_CMD_END(channel), 
                         CMDEND_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for cmd end\n\n"));
        err++;
    }

    return err;
}


gpmi_err_t gpmi_nand8_pio_write_ale32(unsigned channel, 
                                      reg32_t ale,
                                      gpmi_bool_t lock)
{
    gpmi_err_t err = 0;

    hw_gpmi_debug_t debug;
    hw_gpmi_ctrl0_t ctrl0;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_write_ale32: "
                     "channel = %d, "
                     "ale = 0x%x, "
                     "lock = %d\n",
                     channel, ale, lock));

    // Capture value of debug.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    
    // Setup a two byte write command to specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BF_GPMI_CTRL0_LOCK_CS(lock) |
               BF_GPMI_CTRL0_XFER_COUNT(4) |
               BV_FLD(GPMI_CTRL0, ADDRESS, NAND_ALE) |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    // Poll for dmareq to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_DMAREQ(channel), 
                         ~debug.U & GPMI_DEBUG_DMAREQ(channel), 
                         DMAREQ_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for dmareq\n\n"));
        err++;
    }

    // Write cle followed by ale.
    HW_GPMI_DATA_WR(ale);

    // Poll for cmd end to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_CMD_END(channel), 
                         ~debug.U & GPMI_DEBUG_CMD_END(channel), 
                         CMDEND_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for cmd end\n\n"));
        err++;
    }

    return err;
}


gpmi_err_t gpmi_nand8_pio_write_datum(unsigned channel, 
                                      reg8_t datum)
{
    gpmi_err_t err = 0;

    hw_gpmi_debug_t debug;
    hw_gpmi_ctrl0_t ctrl0;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_write_datum: "
                     "channel = %d, "
                     "datum = 0x%x\n",
                     channel, datum));

    // Capture value of debug.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    
    // Setup a multi-byte write command to specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
               BF_GPMI_CTRL0_XFER_COUNT(1) |
               BV_GPMI_CTRL0_ADDRESS__NAND_DATA |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    // Poll for dmareq to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_DMAREQ(channel), 
                         ~debug.U & GPMI_DEBUG_DMAREQ(channel), 
                         DMAREQ_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for dmareq\n\n"));
        err++;
    }

    // Write the data.
    HW_GPMI_DATA_WR(datum);

    // Poll for cmd end to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_CMD_END(channel), 
                         ~debug.U & GPMI_DEBUG_CMD_END(channel), 
                         CMDEND_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for cmd end\n\n"));
        err++;
    }

    return err;
}


gpmi_err_t gpmi_nand8_pio_write_data(unsigned channel, 
                                     reg8_t* data, 
                                     reg16_t count)
{
    gpmi_err_t err = 0;

    hw_gpmi_debug_t debug;
    hw_gpmi_ctrl0_t ctrl0;

    unsigned index;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_write_data: "
                     "channel = %d, "
                     "data = 0x%x, "
                     "count = %d\n",
                     channel, (unsigned)data, count));

    // Capture value of debug.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    
    // Setup a multi-byte write command to specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
               BF_GPMI_CTRL0_XFER_COUNT(count) |
               BV_GPMI_CTRL0_ADDRESS__NAND_DATA |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    for (index = 0; index < count; index++)
    {
        // Poll for dmareq to toggle.
        if (!gpmi_poll_debug(GPMI_DEBUG_DMAREQ(channel), 
                             ~debug.U & GPMI_DEBUG_DMAREQ(channel), 
                             DMAREQ_TIMEOUT))
        {
            TPRINTF(TP_MED, ("\nERROR: timed out polling for dmareq\n\n"));
            err++;
        }

        // Write the data.
        HW_GPMI_DATA_WR(data[index]);
    }

    // Poll for cmd end to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_CMD_END(channel), 
                         ~debug.U & GPMI_DEBUG_CMD_END(channel), 
                         CMDEND_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for cmd end\n\n"));
        err++;
    }

    return err;
}


gpmi_err_t gpmi_nand8_pio_read_datum(unsigned channel, 
                                     reg8_t* datum)
{
    gpmi_err_t err = 0;

    hw_gpmi_debug_t debug;
    hw_gpmi_ctrl0_t ctrl0;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_read_datum: "
                     "channel = %d, "
                     "datum = ??\n",
                     channel));

    // Capture value of debug.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    
    // Setup a wait for ready command to specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
               BF_GPMI_CTRL0_XFER_COUNT(1) |
               BV_GPMI_CTRL0_ADDRESS__NAND_DATA |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    // Poll for dmareq to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_DMAREQ(channel), 
                         ~debug.U & GPMI_DEBUG_DMAREQ(channel), 
                         DMAREQ_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for dmareq\n\n"));
        err++;
    }

    // Read the data returned by the device.
    *datum = (reg8_t) HW_GPMI_DATA_RD();

    // Poll for cmd end to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_CMD_END(channel), 
                         ~debug.U & GPMI_DEBUG_CMD_END(channel), 
                         CMDEND_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for cmd end\n\n"));
        err++;
    }

    return err;
}


reg32_t gpmi_nand8_pio_read_data(unsigned channel, 
                                 reg8_t* result, 
                                 reg16_t count)
{
    gpmi_err_t err = 0;

    hw_gpmi_debug_t debug;
    hw_gpmi_ctrl0_t ctrl0;

    hw_gpmi_data_t data;

    unsigned index;
    unsigned byte;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_read_data: "
                     "channel = %d, "
                     "result = ??, "
                     "count = %d\n",
                     channel, count));

    // Capture value of debug.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    
    // Setup a wait for ready command to specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
               BF_GPMI_CTRL0_XFER_COUNT(count) |
               BV_GPMI_CTRL0_ADDRESS__NAND_DATA |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    index = 0;
    while (index < count)
    {
        // Poll for dmareq to toggle.
        if (!gpmi_poll_debug(GPMI_DEBUG_DMAREQ(channel), 
                             ~debug.U & GPMI_DEBUG_DMAREQ(channel), 
                             DMAREQ_TIMEOUT))
        {
            TPRINTF(TP_MED, ("\nERROR: timed out polling for dmareq\n\n"));
            err++;
            break;
        }

        // Toggle appropriate dmareq bitfield of captured debug.
        debug.U ^= GPMI_DEBUG_DMAREQ(channel);

        // Read up to 4 bytes of data returned by the device.
        data.U = HW_GPMI_DATA_RD();
        for (byte = 0; (byte < 4) && (index < count); byte++)
        {
            result[index] = (reg8_t) (0x000000FF & (data.U >> (byte * 8)));
            TPRINTF(TP_MIN, ("reading byte %d of DATA: 0x%x\n", index, result[index]));
            index++;
        }
    }

    // Poll for cmd end to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_CMD_END(channel), 
                         ~debug.U & GPMI_DEBUG_CMD_END(channel), 
                         CMDEND_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for cmd end\n\n"));
        err++;
    }

    return err;
}


gpmi_err_t gpmi_nand8_pio_read_and_compare8(unsigned channel, 
                                            reg8_t mask, 
                                            reg8_t ref)
{
    hw_gpmi_ctrl0_t ctrl0;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_read_and_compare8: "
                     "channel = %d, "
                     "mask = 0x%x, "
                     "ref = 0x%x\n",
                     channel, mask, ref));

    // Setup a read and compare command from the specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
               BF_GPMI_CTRL0_XFER_COUNT(1) |
               BV_GPMI_CTRL0_ADDRESS__NAND_DATA |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ_AND_COMPARE) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    // Return an error if sense is enabled for this channel.
    if (GPMI_DEBUG_SENSE(channel) & GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG).U)
    {
        return 1;
    }
    
    return 0;
}


gpmi_err_t gpmi_nand8_pio_read_and_compare16(unsigned channel, 
                                             reg16_t mask, 
                                             reg16_t ref)
{
    hw_gpmi_ctrl0_t ctrl0;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_read_and_compare16: "
                     "channel = %d, "
                     "mask = 0x%x, "
                     "ref = 0x%x\n",
                     channel, mask, ref));

    // Setup a read and compare command from the specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
               BF_GPMI_CTRL0_XFER_COUNT(1) |
               BV_GPMI_CTRL0_ADDRESS__NAND_DATA |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 16_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ_AND_COMPARE) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    // Return an error if sense is enabled for this channel.
    if (GPMI_DEBUG_SENSE(channel) & GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG).U)
    {
        return 1;
    }
    
    return 0;
}


gpmi_err_t gpmi_nand8_pio_wait_for_ready(unsigned channel)
{
    gpmi_err_t err = 0;

    hw_gpmi_debug_t debug;
    hw_gpmi_ctrl0_t ctrl0;

    TPRINTF(TP_MIN, ("gpmi_nand8_pio_wait_for_ready: "
                     "channel = %d\n",
                     channel));

    // Capture value of debug.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    
    // Setup a wait for ready command to specified channel.
    ctrl0.U = (BF_GPMI_CTRL0_CS(channel) |
               BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
               BF_GPMI_CTRL0_XFER_COUNT(0) |
               BF_GPMI_CTRL0_ADDRESS(0) |
               BV_FLD(GPMI_CTRL0, ADDRESS_INCREMENT, DISABLED) |
               BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
               BV_FLD(GPMI_CTRL0, UDMA, DISABLED) |
               BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
               BF_GPMI_CTRL0_RUN(1));
    TPRINTF(TP_MIN, ("HW_GPMI_CTRL0 = 0x%x\n", ctrl0.U));
    HW_GPMI_CTRL0_WR(ctrl0.U);

    // Poll for cmd end to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_CMD_END(channel), 
                         ~debug.U & GPMI_DEBUG_CMD_END(channel), 
                         WAIT_FOR_READY_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for cmd end\n\n"));
        err++;
    }

    // Poll for wait for ready to toggle.
    if (!gpmi_poll_debug(GPMI_DEBUG_WAIT_FOR_READY(channel), 
                         ~debug.U & GPMI_DEBUG_WAIT_FOR_READY(channel), 
                         WAIT_FOR_READY_TIMEOUT))
    {
        TPRINTF(TP_MED, ("\nERROR: timed out polling for wait for ready\n\n"));
        err++;
    }

    // Check sense bit for timeout.
    debug = GPMI_TPRINTF_DEBUG(TP_MIN, HW_GPMI_DEBUG);
    if (debug.U & GPMI_DEBUG_SENSE(channel))
    {
        TPRINTF(TP_MED, ("\nERROR: sense asserted after wait for ready\n\n"));
        err++;
    }

    return err;
}


////////////////////////////////////////////////////////////////////////////////
//
// $Log: nand_common.c,v $
// Revision 1.4  2005/12/22 11:14:11  hcyun
// remove warning
//
// Revision 1.3  2005/11/08 04:18:01  hcyun
// remove warning..
// chain itself to avoid missing irq problem.
//
// - hcyun
//
// Revision 1.2  2005/08/20 00:58:10  biglow
// - update rfs which is worked fib fixed chip only.
//
// Revision 1.1  2005/05/15 23:01:38  hcyun
// lots of cleanup... not yet compiled..
//
// - hcyun
//
// Revision 1.1  2005/05/05 01:05:37  hcyun
// - Started using virt_to_phys.. not yet complete..
//
// Revision 1.6  2005/02/15 23:09:58  ttoelkes
// minimizing verbosity levels
//
// Revision 1.5  2004/12/01 03:07:39  ttoelkes
// fixed polarity of write protect control
//
// Revision 1.4  2004/11/30 18:20:58  ttoelkes
// providing semantic shell around enabling/disabling of nand write protect
//
// Revision 1.3  2004/11/02 00:15:15  ttoelkes
// updated to reflect ATA_RESET being renamed to DEV_RESET
//
// Revision 1.2  2004/09/29 17:26:39  ttoelkes
// code updates in preparation for simultaneous read on four nand devices
//
// Revision 1.1  2004/09/26 19:51:10  ttoelkes
// reorganizing NAND-specific half of GPMI code base
//
// Revision 1.8  2004/09/24 18:22:49  ttoelkes
// checkpointing new devlopment while it still compiles; not known to actually work
//
// Revision 1.7  2004/09/21 16:12:23  ttoelkes
// compile bugs eliminated that were introduced while hacking initial
// implementation of Read & Compare functions
//
// Revision 1.6  2004/09/20 21:50:41  ttoelkes
// added initial implementation of bit-bang read-and-compare function
//
// Revision 1.5  2004/09/09 22:34:41  ttoelkes
// retuned timeouts for soft-dma
//
// Revision 1.4  2004/09/03 21:37:37  ttoelkes
// fixed channel assignment errors (hopefully)
//
// Revision 1.3  2004/09/03 19:05:38  ttoelkes
// implemented full pad enable; still needs to be optimized
//
// Revision 1.2  2004/09/03 16:02:55  ttoelkes
// eliding extraneous long comment that accidentally got checked in yesterday; fixing default verbosity
//
// Revision 1.1  2004/09/02 21:50:05  ttoelkes
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
