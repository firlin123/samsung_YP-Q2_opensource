/*****************************************************************************
*                                                                            *
*                                MP4V DECODER                                *
*                                                                            *
*   Copyright (c) 2002-2004 by Mcubeworks, Incoporated. All Rights Reserved. *
*****************************************************************************/

/******************************* FILE HEADER *********************************

    File Name       : mcw_mp4v_decoder.h
    Included files  :
    Module          : interface for MP4V decoder

    Author(s)       :
    Created         :

    Description     : interface for MP4V decoder

==============================================================================
                      Modification History (Reverse Order)
==============================================================================
    Date        Author      Location (variables) / Description
------------------------------------------------------------------------------

------------------------------------------------------------------------------
*****************************************************************************/
#ifndef __MCW_MP4VASP_DECODER_H__
#define __MCW_MP4VASP_DECODER_H__


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
 // #include "mcw_mp4v_decoder.h"
*****************************************************************************/
#ifndef FUNCTYPE
#define FUNCTYPE
#endif

//=============================================================================
// DEFINE
//=============================================================================
#define MCW_MP4V_DEC_MIN_NUM_FRAMEBUF 3

//=============================================================================
// TYPE: STRUCTURE
//=============================================================================
// "I" denotes that the member is used as input,
// "O" for output and "I/O" for both input and output

// Input of MCW_SIGMATEL_MP4VDEC_GetVersion
typedef struct {
  int               iMajor;                 // O : Major version number
  int               iMinor;                 // O : Minor version number
  int               iPatch;                 // O : Patch version number
} MCW_MP4VDEC_VERSION;

// Input of MCW_SIGMATEL_MP4VDEC_Init
// Output of MCW_SIGMATEL_MP4VDEC_GetMemRequirement / MCW_SIGMATEL_MP4VDEC_GetMemBaseForFree
typedef struct {
  unsigned int      uSize;                  // O   : Size of Memory Blocks
  void *            pvBase;                 // I/O : The base address of Memory Blocks
} MCW_MP4VDEC_MEM_REQ;

// Input of MCW_SIGMATEL_MP4VDEC_GetMemRequirement / MCW_SIGMATEL_MP4VDEC_Init / MCW_SIGMATEL_MP4VDEC_ParseVOL
typedef struct {
  int               iShortHeader;           // I : 0 : mpeg4, 1: h263
  unsigned char     *pbyStreamBase;         // I : Base pointer of VOL
  unsigned int      uStreamLength;          // I : Length of VOL stream
  unsigned char     *(*papbyFrameBuff)[3];  // I : User-allocating frame buffers for each color component Y(0), U(1), V(2)
  unsigned int      uMaxFrameWidth;         // I : The maximum decodable frame width. must be multiple of 16
  unsigned int      uMaxFrameHeight;        // I : The maximum decodable frame height. must be multiple of 16
  unsigned int      uNumFrmBuffer;          // I : Number of user-allocating frame buffers. must be larger than or equal to 3.
} MCW_MP4VDEC_INIT_IN;

// Output of MCW_SIGMATEL_MP4VDEC_Init and MCW_SIGMATEL_MP4VDEC_ParseVOL
typedef struct {
  unsigned int      uDispWidth;             // O : Display width of Y frame
  unsigned int      uDispHeight;            // O : Display height of Y frame
  int               iDelayedOut;            // O : 0(output isnot delayed), 1(output is delayed), -1(don't know yet)
  char *            pchDbgReport;           // O : User-allocating string ptr. Error message for debugging.
} MCW_MP4VDEC_INIT_OUT;

// Input of MCW_SIGMATEL_MP4VDEC_DecodeFrame
typedef struct {
  unsigned char     *pbyStreamBase;         // I : Base pointer of the compressed bitstream for a frame
  unsigned int      uStreamLength;          // I : Length of the compressed bitstream for a frame in byte
  int				        iError1stPos;           // I : Position of the first error in the bitstream in byte. -1=NoError
  int               iErrorBrokenPred;       // I : Flag to let a decoder know that prediction is broken by some errors
  int               iUsePostFilter;         // I : Flag to use post filter
} MCW_MP4VDEC_DECODE_FRM_IN;

// Output of MCW_SIGMATEL_MP4VDEC_DecodeFrame
typedef struct {
  unsigned char     *apbyFrmRecon[3];       // O : Decoded YUV frame pointer Y(0), U(1), V(2)
  unsigned long     uFrmTimeMicroSec;       // O : Display time in microsec
  int               iFrmType;               // O : I frame = 0, P frame = 1, B frame = 2, S frame = 3
  int               iFrmReliable_0_100;     // O : Index to tell how many percentage of MB's are reliable ranging 0(all damaged) to 100(all clear)
  unsigned int      uDispWidth;             // O : Display width of Y frame
  unsigned int      uDispHeight;            // O : Display height of Y frame
  unsigned int      uMemStride;             // O : Memory stride of the picture
  int               iDelayedOut;            // O : 0(output isnot delayed), 1(output is delayed), -1(don't know yet)
  char *            pchDbgReport;           // O : User-allocating string ptr. Error message for debugging.
} MCW_MP4VDEC_DECODE_FRM_OUT;

//=============================================================================
// FUNCTIONS: API DEFINITION
//=============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// used to get the version of the library. 
FUNCTYPE void     MCW_SIGMATEL_MP4VDEC_GetVersion( MCW_MP4VDEC_VERSION *o_pVersion );

// returns the number of Memory Blocks.
FUNCTYPE int      MCW_SIGMATEL_MP4VDEC_GetNumMemBlocks(void);

// outputs the size of each Memory Block
// returns the number of Memory Blocks.
FUNCTYPE int      MCW_SIGMATEL_MP4VDEC_GetMemRequirement( MCW_MP4VDEC_INIT_IN *i_pInitIn,
                                                          MCW_MP4VDEC_MEM_REQ *o_pMemReq );

// outputs information from the VOL stream.
// is called before MCW_SIGMATEL_MP4VDEC_GetMemRequirement ( )
FUNCTYPE int      MCW_SIGMATEL_MP4VDEC_ParseVOL(MCW_MP4VDEC_INIT_IN  *i_pInitIn,
                                                MCW_MP4VDEC_INIT_OUT *o_pInitOut );

// initializes the library using pMemReq->pvBase.
// decodes VOL (if pbyStreamBase isnot NULL).
// returns the main decoder handle.
FUNCTYPE void    *MCW_SIGMATEL_MP4VDEC_Init( MCW_MP4VDEC_MEM_REQ  *i_pMemReq,
                                             MCW_MP4VDEC_INIT_IN  *i_pInitIn,
                                             MCW_MP4VDEC_INIT_OUT *o_pInitOut );

// decodes VOL if VOL stream is combined with VOP1).
// decodes one frame.
// detects bitstream errors and conceal errors.
// returns comsumed bytes
FUNCTYPE int      MCW_SIGMATEL_MP4VDEC_DecodeFrame( void                       *i_pvHandle,
                                                    MCW_MP4VDEC_DECODE_FRM_IN  *i_pDecodeIn,
                                                    MCW_MP4VDEC_DECODE_FRM_OUT *o_pDecodeOut );

// outputs the address of memory blocks
FUNCTYPE int      MCW_SIGMATEL_MP4VDEC_GetMemBaseForFree( void                *i_pvHandle,
                                                          MCW_MP4VDEC_MEM_REQ *o_pMemReq );

#ifdef __cplusplus
}
#endif

#endif //#ifndef __MCW_MP4VASP_DECODER_H__
