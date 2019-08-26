////////////////////////////////////////////////////////////////////////////////
//
// Filename: nand_common.h
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

#ifndef _NAND_COMMON_H_
#define _NAND_COMMON_H_

#include "fm_global.h"
#include "fd_if.h"
#include "gpmi_common.h"

//------------------------------------------------------------------------------
// constants

#define NAND_SECTOR_DATA_SIZE  512

#define NAND_SECTOR_RSI_SIZE     1
#define NAND_SECTOR_BA_SIZE      2
#define NAND_SECTOR_UNUSED_SIZE  2
#define NAND_SECTOR_CHKSUM_SIZE  1
#define NAND_SECTOR_BSM_SIZE     1
#define NAND_SECTOR_ECC_SIZE     9

#define NAND_SECTOR_RA_SIZE (NAND_SECTOR_RSI_SIZE + \
                             NAND_SECTOR_BA_SIZE + \
                             NAND_SECTOR_UNUSED_SIZE + \
                             NAND_SECTOR_CHKSUM_SIZE + \
                             NAND_SECTOR_BSM_SIZE + \
                             NAND_SECTOR_ECC_SIZE)

#define NAND_SECTOR_SIZE (NAND_SECTOR_DATA_SIZE + \
                          NAND_SECTOR_RA_SIZE)

struct _nand_sector_t
{
    reg8_t   data[NAND_SECTOR_DATA_SIZE];
    reg8_t   rsi;
    reg16_t  ba;
    reg16_t  unused;
    reg8_t   chksum;
    reg8_t   bsm;
    reg8_t   ecc[NAND_SECTOR_ECC_SIZE];
};

typedef struct _nand_sector_t nand_sector_t;

struct _nand_info_t
{
    u8      id[5];                      /* device ID read from NAND flash;
                                           1st ID data: maker code
                                           2nd ID data: device code 
                                           3rd, 4th, 5th ID data: see below */

    /* breakdown info for 3rd ID data */
    u8      internal_chip_number;       /* 1, 2, 4, or 8 */
    u8      cell_type;                  /* 2, 4, 8, or 16-level cell */
    u8      num_simul_prog_pages;       /* 1, 2, 4, or 8 */
    u8      interleave_support;         /* 0: no, 1: yes */
    u8      cache_prog_support;         /* 0: no, 1: yes */
    
    /* breakdown info for 4th ID data */
    u16     page_size;                  /* in bytes */
    u32     block_size;                 /* in bytes */
    u8      spare_size_in_512;          /* 8 or 16 bytes */
    u8      organization;               /* 8 or 16-bit I/O lines */
    u8      serial_access_min;          /* in ns */
    
    /* breakdown info for 5th ID data */
    u8      plane_number;               /* 1, 2, 4, or 8 */
    u32     plane_size;                 /* in bytes */
};

typedef struct _nand_info_t nand_info_t;

extern FLASH_SPEC *flash_spec;

//------------------------------------------------------------------------------
// functions

extern void gpmi_nand_protect(void);

extern void gpmi_nand_unprotect(void);

extern void gpmi_nand8_pio_config(reg8_t address_setup, reg8_t data_setup, reg8_t data_hold, reg16_t busy_timeout);

extern gpmi_err_t gpmi_nand8_pio_write_cle(unsigned channel, reg8_t cle, gpmi_bool_t lock);

extern gpmi_err_t gpmi_nand8_pio_write_cle_ale8(unsigned channel, reg8_t cle, reg8_t ale, gpmi_bool_t lock);

extern gpmi_err_t gpmi_nand8_pio_write_ale8(unsigned channel, reg8_t ale, gpmi_bool_t lock);
extern gpmi_err_t gpmi_nand8_pio_write_ale16(unsigned channel, reg16_t ale, gpmi_bool_t lock);
extern gpmi_err_t gpmi_nand8_pio_write_ale32(unsigned channel, reg32_t ale, gpmi_bool_t lock);

extern gpmi_err_t gpmi_nand8_pio_write_datum(unsigned channel, reg8_t datum);
extern gpmi_err_t gpmi_nand8_pio_write_data(unsigned channel, reg8_t* data, reg16_t count);

extern gpmi_err_t gpmi_nand8_pio_read_datum(unsigned channel, reg8_t* datum);
extern gpmi_err_t gpmi_nand8_pio_read_data(unsigned channel, reg8_t* data, reg16_t count);

extern gpmi_err_t gpmi_nand8_pio_read_and_compare8(unsigned channel, reg8_t mask, reg8_t ref);
extern gpmi_err_t gpmi_nand8_pio_read_and_compare16(unsigned channel, reg16_t mask, reg16_t ref);

extern gpmi_err_t gpmi_nand8_pio_wait_for_ready(unsigned channel);


#endif // !_NAND_COMMON_H_

////////////////////////////////////////////////////////////////////////////////
//
// $Log: nand_common.h,v $
// Revision 1.2  2007/08/16 06:01:09  hcyun
// [ADDED] secmlc ddp/mono support (2k/4k support)
// - hcyun
//
// Revision 1.1  2005/05/15 23:01:38  hcyun
// lots of cleanup... not yet compiled..
//
// - hcyun
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
// Revision 1.3  2004/11/30 18:20:58  ttoelkes
// providing semantic shell around enabling/disabling of nand write protect
//
// Revision 1.2  2004/09/29 17:26:39  ttoelkes
// code updates in preparation for simultaneous read on four nand devices
//
// Revision 1.1  2004/09/26 19:51:10  ttoelkes
// reorganizing NAND-specific half of GPMI code base
//
// Revision 1.4  2004/09/24 21:50:42  ttoelkes
// adding i2c unit test directory and auto-generated reset test
//
// Revision 1.3  2004/09/24 20:03:46  ttoelkes
// tried to make test fail; didn't succeed
//
// Revision 1.2  2004/09/03 16:02:55  ttoelkes
// eliding extraneous long comment that accidentally got checked in yesterday; fixing default verbosity
//
// Revision 1.1  2004/09/02 21:50:05  ttoelkes
//
////////////////////////////////////////////////////////////////////////////////
