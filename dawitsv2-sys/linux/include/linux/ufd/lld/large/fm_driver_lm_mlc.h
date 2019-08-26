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
/*  FILE    : fm_driver_lm.h                                            */
/*  PURPOSE : Header file for Flash Device Driver                       */
/*            for multi-planed large block NAND flash                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - Supports NAND flash memory having the following features:         */
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

/*****
revision for STMP36xx and K9WAG08U1M

[2005/6/30] yhbae
	- add group functions for two-plane operation

*****/

#ifndef _FM_DRIVER_LM_MLC_H
#define _FM_DRIVER_LM_MLC_H

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

// for 4Gb MLC 
INT32  FLM_MLC_Init             (void);
INT32  FLM_MLC_Open             (UINT16 chip);
INT32  FLM_MLC_Close            (UINT16 chip);
INT32  FLM_MLC_Read_Page        (UINT16 chip, UINT32 block, UINT16 page,
                                 UINT16 sector_offset, UINT16 num_sectors,
                                 UINT8 *dbuf, UINT8 *sbuf);
INT32  FLM_MLC_Read_Page_Group  (UINT16 chip, UINT32 *block, UINT16 page,
                                 UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                 INT32 *flag);
INT32  FLM_MLC_Write_Page       (UINT16 chip, UINT32 block, UINT16 page,
                                 UINT16 sector_offset, UINT16 num_sectors,
                                 UINT8 *dbuf, UINT8 *sbuf, BOOL is_last);
INT32  FLM_MLC_Write_Page_Group (UINT16 chip, UINT32 *block, UINT16 page,
                                 UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                 INT32 *flag, BOOL is_last);
INT32  FLM_MLC_Copy_Back        (UINT16 chip, UINT32 src_block, UINT16 src_page,
                                 UINT32 dest_block, UINT16 dest_page);
INT32  FLM_MLC_Copy_Back_Group  (UINT16 chip, 
                                 UINT32 *src_block, UINT16 *src_page, 
                                 UINT32 *dest_block, UINT16 dest_page, 
                                 INT32 *flag);
INT32  FLM_MLC_Erase            (UINT16 chip, UINT32 block);
INT32  FLM_MLC_Erase_Group      (UINT16 chip, UINT32 *block, INT32 *flag);
INT32  FLM_MLC_Sync             (UINT16 chip);
BOOL   FLM_MLC_IsBadBlock       (UINT16 chip, UINT32 block);
INT32  FLM_MLC_Read_ID          (UINT16 chip, UINT8 *maker, UINT8 *dev_code);



// for 8Gb 2/4/8GB MLC 

INT32  FLM_MLC8G_2_Init             (void);
INT32  FLM_MLC8G_2_Open             (UINT16 chip);
INT32  FLM_MLC8G_2_Close            (UINT16 chip);
INT32  FLM_MLC8G_2_Read_Page        (UINT16 chip, UINT32 block, UINT16 page,
                                     UINT16 sector_offset, UINT16 num_sectors,
                                     UINT8 *dbuf, UINT8 *sbuf);
INT32  FLM_MLC8G_2_Read_Page_Group  (UINT16 chip, UINT32 *block, UINT16 page,
                                     UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                     INT32 *flag);
INT32  FLM_MLC8G_2_Write_Page       (UINT16 chip, UINT32 block, UINT16 page,
                                     UINT16 sector_offset, UINT16 num_sectors,
                                     UINT8 *dbuf, UINT8 *sbuf, BOOL is_last);
INT32  FLM_MLC8G_2_Write_Page_Group (UINT16 chip, UINT32 *block, UINT16 page,
                                     UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                     INT32 *flag, BOOL is_last);
INT32  FLM_MLC8G_2_Copy_Back        (UINT16 chip, UINT32 src_block, UINT16 src_page,
                                     UINT32 dest_block, UINT16 dest_page);
INT32  FLM_MLC8G_2_Copy_Back_Group  (UINT16 chip, 
                                     UINT32 *src_block, UINT16 *src_page, 
                                     UINT32 *dest_block, UINT16 dest_page, 
                                     INT32 *flag);
INT32  FLM_MLC8G_2_Erase            (UINT16 chip, UINT32 block);
INT32  FLM_MLC8G_2_Erase_Group      (UINT16 chip, UINT32 *block, INT32 *flag);
INT32  FLM_MLC8G_2_Sync             (UINT16 chip);
BOOL   FLM_MLC8G_2_IsBadBlock       (UINT16 chip, UINT32 block);
INT32  FLM_MLC8G_2_Read_ID          (UINT16 chip, UINT8 *maker, UINT8 *dev_code);
BOOL   FLM_MLC8G_2_Is_Multi_OK      (UINT16 chip, UINT32 *block, INT32 *flag);

#endif /* _FM_DRIVER_LM_MLC_H */

/* end of fm_driver_lm.h */
