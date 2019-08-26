/******************************************************************************
                           MCUBEWORKS VIDEO DECODER
           Copyright (c) 2006 McubeWorks, Inc. All rights reserved.

This source code was developed by McubeWorks(MCW) based on Licensed Technology
from Microsoft under "Windows Media Components Interim Product Agreement"
(Agreement Number: 6143887113) between MCW and Microsoft and is strictly limited 
for use on SigmaTel's ARM9E-based products only to ensure the quality standards 
of the Developed Technology which is defined both in the "Windows Media Components
Interim Product Agreement" (Agreement Number: 6143887113) between MCW and
Microsoft and "Windows Media Components Interim Product Agreement" Agreement
Number: 6143171155) between SigmaTel and Microsoft. McubeWorks' Developed Technology
is company confidential information and the property of MCW, and can not be
reproduced or disclosed in any form without written authorization of MCW while
Microsoft reserves all right, title and interest in and to the Licensed
Technology.

Those intending to use this software illegally or for other purposes
are advised that it infringes MCW's copyright and shall have full responsibility
of any relevant claim. MCW has no liability for any claim of infringement in use
of a superceded or altered release of this software and in use of this software
combined with other products than Sigmatel's ARM9E-based products.
This copyright notice must be included in all copies or derivative works.
******************************************************************************/

#ifndef _MCW_WMV9_DECODER_
#define _MCW_WMV9_DECODER_

#include "mcw_sigmatel_viddec.h"

// @ Intialization, Input
//  Structure to get Sequence Header information
typedef struct
{
   unsigned char*      pbySeqInfo;            // I   :Base pointer of a codec specific configuration data
   unsigned long       dwSeqHdrSize;          // I   :Byte size appended to the configuration data
   int                 iMultiResDisabled;     // I/O :flag to inform whether multi-resolution or/and range reduction are used
} WMV9DEC_MCW_INIT_IN;

// @ Intialization, Output
//  Structure to delicate bitstream information
typedef struct
{
   unsigned long       uRcvNumFrames;         // O :the number of frames in this bitstream
   long                dwProfile;             // O :profile for this bitstream
   int                 iDelayAFrm;            // O :flag whether one delay is when B picture is involved in this bitstream
   int                 iErrorCode;            // O :error code about initialization step 
} WMV9DEC_MCW_INIT_OUT;

// @ Decoding, Input
//  Structure to delicate delay information for display
typedef struct
{
   int                 iDisplayOnly;          // I :only display when any B picture is involved in this bitstream
} WMV9DEC_MCW_DECFRM_IN;

//=============================================================================
// FUNCTIONS: API DEFINITION
//=============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// Get the version of library
FUNCTYPE void     MCW_SIGMATEL_WMV9DEC_GetVersion(
    VIDDEC_MCW_VERSION            *o_pSCodecVersion
);

// Returns the number of Memory Blocks which delivers Decoder Memory to the decoder library.
FUNCTYPE int      MCW_SIGMATEL_WMV9DEC_GetNumMemTablesForAlloc(void);

// Return the size of each Memory Block
FUNCTYPE int      MCW_SIGMATEL_WMV9DEC_GetMemRequirement(
    VIDDEC_MCW_INIT_IN            *i_pSInitIn,
    VIDDEC_MCW_MEM_REQ            *o_pSMemReq
);

// Parses Sequence Header Information
FUNCTYPE int      MCW_SIGMATEL_WMV9DEC_ParseVidConfig(
    VIDDEC_MCW_INIT_IN            *i_pSInitIn,
    VIDDEC_MCW_INIT_OUT           *o_pSInitOut
);

// Initializes decoder with sequence header and initializes all the internal memory pointers using pstMemReq->pvBase
FUNCTYPE void *   MCW_SIGMATEL_WMV9DEC_Init(
    VIDDEC_MCW_MEM_REQ            *i_pSMemReq,
    VIDDEC_MCW_INIT_IN            *i_pSInitIn,
    VIDDEC_MCW_INIT_OUT           *o_pSInitOut
);

// Decodes one Access Unit (or Frame) bitstream into an YUV frame
FUNCTYPE int      MCW_SIGMATEL_WMV9DEC_DecodeFrame(
    void                          *i_pvCodecHandle,
    VIDDEC_MCW_DECFRM_IN          *i_pSDecFrmIn,
    VIDDEC_MCW_DECFRM_OUT         *o_pSDecFrmOut
);

// Delivers base address of Memory Blocks, which must be allocated by caller and delivered to the decoder library.
FUNCTYPE int  MCW_SIGMATEL_WMV9DEC_GetMemBaseForFree(
    void                          *i_pvCodecHandle,
    VIDDEC_MCW_MEM_REQ            *o_pSMemReq
);

#ifdef __cplusplus
}
#endif

#endif