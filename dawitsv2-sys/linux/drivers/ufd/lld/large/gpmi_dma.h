////////////////////////////////////////////////////////////////////////////////
//
// Filename: gpmi_dma.h
//
// Description: Include file for various commonly useful code for dma with GPMI.
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

#ifndef _GPMI_DMA_H_
#define _GPMI_DMA_H_

#include "gpmi_common.h"
#include "apbh_common.h"


//------------------------------------------------------------------------------
// types

struct _apbh_dma_gpmi1_t
{
    volatile apbh_dma_t*                   nxt;
    volatile hw_apbh_chn_cmd_t             cmd;
    volatile void*                         bar;
    union
    {
        struct
        {
            volatile hw_gpmi_ctrl0_t       gpmi_ctrl0;
        };
        reg32_t                            pio[1];
    };
};


struct _apbh_dma_gpmi2_t
{
    volatile apbh_dma_t*                   nxt;
    volatile hw_apbh_chn_cmd_t             cmd;
    volatile void*                         bar;
    union
    {
        struct
        {
            volatile hw_gpmi_ctrl0_t       gpmi_ctrl0;
            volatile hw_gpmi_compare_t     gpmi_compare;
        };
        volatile reg32_t                   pio[2];
    };
};


struct _apbh_dma_gpmi3_t
{
    volatile apbh_dma_t*                   nxt;
    volatile hw_apbh_chn_cmd_t             cmd;
    volatile void*                         bar;
    union
    {
        struct
        {
            volatile hw_gpmi_ctrl0_t       gpmi_ctrl0;
            volatile hw_gpmi_compare_t     gpmi_compare;
            volatile hw_gpmi_eccctrl_t     gpmi_eccctrl;
            volatile hw_gpmi_ecccount_t    gpmi_ecccount;
            volatile hw_gpmi_payload_t     gpmi_data_ptr;
            volatile hw_gpmi_auxiliary_t   gpmi_aux_ptr;
        };
        volatile reg32_t                   pio[6];
    };
};


typedef struct _apbh_dma_gpmi1_t apbh_dma_gpmi1_t;
typedef struct _apbh_dma_gpmi2_t apbh_dma_gpmi2_t;
typedef struct _apbh_dma_gpmi3_t apbh_dma_gpmi3_t;

extern const apbh_dma_t NAND_PROTECTED_DMA;
extern const apbh_dma_t NAND_FAIL_DMA;
extern const apbh_dma_t NAND_BUSY_DMA;


#endif // !_GPMI_DMA_H_

////////////////////////////////////////////////////////////////////////////////
//
// $Log: gpmi_dma.h,v $
// Revision 1.3  2005/07/18 12:37:16  hcyun
// 1GB, 1plane, SW_COPYBACK, NO_ECC,
//
// - hcyun
//
// Revision 1.1  2005/05/15 23:01:38  hcyun
// lots of cleanup... not yet compiled..
//
// - hcyun
//
// Revision 1.1  2005/05/05 01:05:37  hcyun
// - Started using virt_to_phys.. not yet complete..
//
// Revision 1.4  2005/01/31 17:51:57  ttoelkes
// fixed pio register ordering bugs in larger descriptor types
//
// Revision 1.3  2004/10/25 19:00:01  ttoelkes
// checkpoint
//
// Revision 1.2  2004/09/29 20:58:32  ttoelkes
// checkpoint
//
// Revision 1.1  2004/09/24 18:22:49  ttoelkes
// checkpointing new devlopment while it still compiles; not known to actually work
//
////////////////////////////////////////////////////////////////////////////////
