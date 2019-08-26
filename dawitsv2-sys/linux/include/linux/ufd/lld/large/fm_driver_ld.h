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
/*  This file implements a Device Driver for NAND flash memory.         */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Younghyun Bae                                              */
/*  @author  Joosun Hahn                                                */
/*  @author  Sung-Kwan Kim                                              */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fm_driver_ld.h                                            */
/*  PURPOSE : Header file for Flash Device Driver                       */
/*            for small block (2K-byte page) direct NAND flash          */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - Supports NAND flash memory having the following features:         */
/*    . K9F1G08U0M                                                      */
/*    . small block (2K-byte page)                                      */
/*    . 16-bit operation                                                */
/*  - Assumes that NAND flash memory is controlled by the memory        */
/*    controller in the S3C2410 MCU.                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2002 [Younghyun Bae] : first writing                        */
/*  - 01/10/2003 [Joosun Hahn]   : ported to linux-2.4.18-rmk7-pxa1     */
/*  - 01/01/2004 [Sung-Kwan Kim] : re-organized & trimmed               */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _FM_DRIVER_LD_H
#define _FM_DRIVER_LD_H

/*----------------------------------------------------------------------*/
/*  Flash Memory Configurations                                         */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  Global Function Declarations                                        */
/*----------------------------------------------------------------------*/

INT32  FLD_Init             (void);
INT32  FLD_Open             (UINT16 chip);
INT32  FLD_Close            (UINT16 chip);
INT32  FLD_Read_Page        (UINT16 chip, UINT32 block, UINT16 page,
                             UINT16 sector_offset, UINT16 num_sectors,
                             UINT8 *dbuf, UINT8 *sbuf);
INT32  FLD_Write_Page       (UINT16 chip, UINT32 block, UINT16 page,
                             UINT16 sector_offset, UINT16 num_sectors,
                             UINT8 *dbuf, UINT8 *sbuf, BOOL is_last);
INT32  FLD_Copy_Back        (UINT16 chip, UINT32 src_block, UINT16 src_page,
                             UINT32 dest_block, UINT16 dest_page);
INT32  FLD_Erase            (UINT16 chip, UINT32 block);
INT32  FLD_Sync             (UINT16 chip);
BOOL   FLD_IsBadBlock       (UINT16 chip, UINT32 block);
INT32  FLD_Read_ID          (UINT16 chip, UINT8 *maker, UINT8 *dev_code);

INT32  FLD_Erase_Group      (UINT16 chip, UINT32 *block, INT32 *flag);

#endif /* _FM_DRIVER_LD_H */

/* end of fm_driver_ld.h */
