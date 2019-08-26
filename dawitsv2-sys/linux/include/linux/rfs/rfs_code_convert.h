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
/*  This file implements the RFS FAT file system core.                  */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Dong-Hee Lee                                               */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : rfs_code_convert.h                                        */
/*  PURPOSE : Header file for Linux Robust FAT File System Core         */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - Name Conversion Functions                                         */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Dong-Hee Lee] : first writing                         */
/*  - 01/11/2003 [Joosun Hahn]  : added FAT32 support                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _RFS_CODE_CONVERT_H
#define _RFS_CODE_CONVERT_H

#include "fm_global.h"
#include <linux/nls.h>

//#define CONFIG_RFS_NLS

/*----------------------------------------------------------------------*/
/*  External Function Declarations                                      */
/*----------------------------------------------------------------------*/

INT32  DOS_NAME_CMP(UINT8 *a, UINT8 *b);
INT32  UNICODE_NAME_CMP(UINT16 *a, UINT16 *b);
void   CONVERT_CSTRING_TO_DOS_NAME(UINT8 *dosname, UINT8 *c_string_name, UINT8 *mixed, INT32 *lossy);
void   CONVERT_DOS_NAME_TO_CSTRING(UINT8 *c_string_name, UINT8 *dosname, UINT8 sysid);
void   CONVERT_CSTRING_TO_UNICODE(UINT16 *unicode_name, UINT8 *c_string_name, struct nls_table *nls);
void   CONVERT_UNICODE_TO_CSTRING(UINT8 *c_string_name, UINT16 *unicode_name, struct nls_table *nls);

#endif /* _RFS_CODE_CONVERT_H */

/* end of rfs_code_convert.h */
