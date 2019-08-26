////////////////////////////////////////////////////////////////////////////////
// Filename: gpmi_dma.c
//
// Description: Implementation file for various commonly useful GPMI dma code.
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

#include "apbh_common.h"

const apbh_dma_t  NAND_PROTECTED_DMA =
{
    (apbh_dma_t*) 0x0,
    (hw_apbh_chn_cmd_t) ((reg32_t) (BF_APBH_CHn_CMD_IRQONCMPLT(1) | 
                                    BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER))),
    (void *) APBH_NAND_PROTECTED
};


const apbh_dma_t  NAND_BUSY_DMA =
{
    (apbh_dma_t*) 0x0,
    (hw_apbh_chn_cmd_t) ((reg32_t) (BF_APBH_CHn_CMD_IRQONCMPLT(1) | 
                                    BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER))),
    (void *) APBH_NAND_BUSY
};


const apbh_dma_t  NAND_FAIL_DMA =
{
    (apbh_dma_t*) 0x0,
    (hw_apbh_chn_cmd_t) ((reg32_t) (BF_APBH_CHn_CMD_IRQONCMPLT(1) | 
                                    BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER))),
    (void *) APBH_NAND_FAIL
};


////////////////////////////////////////////////////////////////////////////////
//
// $Log: gpmi_dma.c,v $
// Revision 1.2  2005/05/15 23:01:38  hcyun
// lots of cleanup... not yet compiled..
//
// - hcyun
//
// Revision 1.1  2005/05/14 07:07:56  hcyun
// copy-back, cache-program, random-input, random-output added.
// fm_driver for stmp36xx added
//
// Revision 1.1  2005/05/05 01:05:37  hcyun
// - Started using virt_to_phys.. not yet complete..
//
// Revision 1.2  2004/10/25 19:00:01  ttoelkes
// checkpoint
//
// Revision 1.1  2004/09/29 20:27:48  ttoelkes
// adding implementation file for const objects defined in "gpmi_dma.h"
//
////////////////////////////////////////////////////////////////////////////////
