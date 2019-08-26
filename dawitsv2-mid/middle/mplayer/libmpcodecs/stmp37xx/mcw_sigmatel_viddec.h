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

/*****************************************************************************
 FUNCTYPE
 Definition for the library type.

 Usage example:
 // 1-1. to make or use the static library, define nothing
 // 1-2. to use(import) the dynamic link library, define as
 // #define FUNCTYPE  __declspec (dllimport)
 // 1-3. to make(export) the dynamic link library, define as
 // #define FUNCTYPE  __declspec (dllexport)
 // 2. and include this header file
*****************************************************************************/
#ifndef FUNCTYPE
#define FUNCTYPE
#endif

//=====================  NULL & RETURN TYPE Definition ======================
#if !defined(NULL)
      #define NULL                                  0
#endif

#define RET_SUCCESS                                 0
#define RET_FAIL                                   -1


//====== Structure Definition =================================================
// "I" denotes that the member is used as input,
// "O" for output and "I/O" for both input and output

// This structure is defined to get library version.
typedef struct {
    int               iMajor;                 // O : Major version number
    int               iMinor;                 // O : Minor version number
    int               iPatch;                 // O : Patch version number
} VIDDEC_MCW_VERSION;

// This structure is defined to get/deliver required decoder memory information from/to library
typedef struct
{
    void              *pvBase;                // I :Base address of a allocated pointer
    unsigned long     uSize;                  // O :Size in unit of sizeof(char)
} VIDDEC_MCW_MEM_REQ;

// @ Intialization, Input
typedef struct
{
    void              *pvInitInSpecific;      // I :Codec specific input
    unsigned char     *(*papbyFrmBuffer)[3];  // I :Frame pointers for a reconstructed, a previous, and so on
    unsigned int      uDecodableMaxWidth;     // I :Image width
                                              //    This must not be smaller than the actual size of a reconsructed frame
    unsigned int      uDecodableMaxHeight;    // I :Image height
                                              //    This must not be smaller than the actual size of a reconsructed frame
    unsigned int      uNumFrmBuffer;          // I :The number of frame buffers
} VIDDEC_MCW_INIT_IN;

// @ Initialization,  Output
typedef struct
{
    void              *pvInitOutSpecific;     // O :Codec specific output
    unsigned int      uDispWidth;             // O :Display width known from configuration info.
    unsigned int      uDispHeight;            // O :Display height known from configuration info.
} VIDDEC_MCW_INIT_OUT;

// @ Decoding, Input
typedef struct
{
    void              *pvDecFrmInSpecific;    // I :Codec specific input
    unsigned char     *pbyFrmBStream;         // I :Base pointer of a compressed bitsream of a frame
    long              lFrmBStreamLeng;        // I :Byte-size of a compressed bitsream delivered by pbyFrmBStream
    int               iFrmDispLevel;          // I :Parameter to determine the rule to output "iFrmNotDisp"
    int               iErrorFirstPos;         // I :Position of the first error in the bitstream in byte ('-1'=NoError)
    int               iErrorBrokenPred;       // I :Flag to let a decoder know that prediction is broken by some errors
} VIDDEC_MCW_DECFRM_IN;

// @ Decoding, Output
typedef struct
{
    void              *pvDecFrmOutSpecific;   // O :Codec specific output
    unsigned char     *apbyFrmRecon[3];       // O :Reconstructed frame pointers (YUV 4:2:0 planar format)
    unsigned long     dwFrmTimeDiffX90000;    // O :(Temporal difference in sec * 90000) from the last decoded frame (If '0', ignore this.)
    int               iFrmIsKeyFrame;         // O :Flag to tell if this frame is a key frame or not. [0]Dependent frame(P/B) | [1]Syncable key frame (I/IDR)
    int               iFrmNotDisplay;         // O :Flag to order that this frame should not be displayed
    unsigned int      uDispWidth;             // O :Display width known from configuration info
    unsigned int      uDispHeight;            // O :Display height known from configuration info
    unsigned int      uMemStride;             // O :Stride of memory which is the byte-distance to the next row (Usually, uMemStride = uDispWidth)
} VIDDEC_MCW_DECFRM_OUT;
