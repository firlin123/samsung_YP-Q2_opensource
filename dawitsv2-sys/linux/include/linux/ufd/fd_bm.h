/************************************************************************/
/*                                                                      */
/*  Copyright (c) 2004 Flash Planning Group, Samsung Electronics, Inc.  */
/*  Copyright (c) 2004 Zeen Information Technologies, Inc.              */
/*  All right reserved.                                                 */
/*                                                                      */
/*  This software is the confidential and proprietary information of    */
/*  Samsung Electronics, Inc. and Zeen Information Technologies, Inc.   */
/*  ("Confidential Information"). You shall not disclose such           */
/*  confidential information and shall use it only in accordance with   */
/*  the terms of the license agreement you entered into with one of     */
/*  the above copyright holders.                                        */
/*                                                                      */
/************************************************************************/
/*  This file implements the Flash Device Bad Block Management Layer.   */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Sung-Kwan Kim                                              */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fd_bm.h                                                   */
/*  PURPOSE : Header file for Flash Device Bad Block Management Layer   */
/*            (BM)                                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - This module's functionality is officially called DLBM             */
/*    (Driver-Level Bad Block Management).                              */
/*  - To enable/disable DLBM, define the 'USE_DLBM' macro in this file  */
/*    appropriately.                                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 2.0)                                          */
/*                                                                      */
/*  - 01/12/2003 [Sung-Kwan Kim] : First writing                        */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _FD_BM_H
#define _FD_BM_H

#include "fd_if.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Configurable)                         */
/*----------------------------------------------------------------------*/

#define USE_DLBM                1       /* 1: use DLBM function (dafault)
                                           0: don't use DLBM function */

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Non-Configurable)                     */
/*----------------------------------------------------------------------*/

#ifdef ZFLASH_EMUL
#undef  USE_DLBM
#define USE_DLBM                0
#endif

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  External Variable Declarations                                      */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  External Function Declarations                                      */
/*----------------------------------------------------------------------*/

#if USE_DLBM

extern INT32  BM_Init(void);
extern UINT32 BM_CalcBmAreaNumBlocks(UINT16 chip_id);
extern INT32  BM_Open(UINT16 chip_id);
extern INT32  BM_Format(UINT16 chip_id, BOOL forced);
extern BOOL   BM_isFormatted(UINT16 chip_id);
extern INT32  BM_GetNumBadBlocks(UINT16 chip_id);

extern INT32  BM_SwapWriteBadBlock(UINT16 chip_id, UINT32 block, UINT16 page);
extern INT32  BM_SwapEraseBadBlock(UINT16 chip_id, UINT32 block);
extern UINT32 BM_GetSwappingBlock(UINT16 chip_id, UINT32 block);
extern UINT32 BM_GetOriginalBlock(UINT16 chip_id, UINT32 block);

#endif /* USE_DLBM */
#endif /* _FD_BM_H */
