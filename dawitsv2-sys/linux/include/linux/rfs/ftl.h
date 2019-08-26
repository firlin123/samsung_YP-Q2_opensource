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
/*  This file implements the Flash Translation Layer (FTL).             */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Dong-Hee Lee                                               */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : ftl.h                                                     */
/*  PURPOSE : Header file for Flash Translation Layer (FTL)             */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - NAND Flash FTL core for Linux Robust FAT File System              */
/*  - Page group re-mapping scheme is used                              */
/*  - Log-based data write method is used                               */
/*  - Supports both of small block & large block flash memory devices   */
/*  - Supports multi-plane operations                                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Dong-Hee Lee] : first writing                         */
/*  - 01/10/2003 [Joosun Hahn]  : ported to linux-2.4.18-rmk7-pxa1      */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _FTL_H
#define _FTL_H

#include "fm_global.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Non-Configurable)                     */
/*----------------------------------------------------------------------*/

/* there are two FTL implementations; which one to use? */

#define CONFIG_FTL_4K           1

#define ZEEN_DEVELOPMENT        0

/* return values of FTL APIs */

#define FTL_SUCCESS             0
#define FTL_CONFIGERR           1
#define FTL_DATAALIGNERR        2
#define FTL_FORMATERR           3
#define FTL_NOTOPENED           4
#define FTL_INVALIDSECTOR       5
#define FTL_MEDIAERR            6
#define FTL_READERR             7
#define FTL_WRITEERR            8

#define FTL_ERROR               -1

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

typedef struct {
    UINT32  NumSector;
    UINT32  BlockSize;
    UINT32  PageSize;
} FTL_STAT_T;

/*----------------------------------------------------------------------*/
/*  Global Function Declarations                                        */
/*----------------------------------------------------------------------*/

/* FTL API functions */

INT32 ftl_format(UINT32 dev);
INT32 ftl_open(UINT32 dev) ;
INT32 ftl_close(UINT32 dev);
INT32 ftl_read_sectors(UINT32 dev, UINT32 sectorno, UINT8 *buf, UINT32 num_sectors);
INT32 ftl_write_sectors(UINT32 dev, UINT32 sectorno, UINT8 *buf, UINT32 num_sectors);
INT32 ftl_stat(UINT32 dev, FTL_STAT_T *stat);
INT32 ftl_mapdestroy(UINT32 dev, UINT32 sectorno, UINT32 num_sectors);
INT32 ftl_flush(UINT32 dev);
INT32 ftl_sync(UINT32 dev);
INT32 ftl_idle(UINT32 dev);
INT32 ftl_transaction_begin(UINT32 dev);
INT32 ftl_format_blkmap(UINT32 dev);
INT32 ftl_scan_logblock(UINT32 dev);

#endif /* _FTL_H */

/* end of ftl.h */
