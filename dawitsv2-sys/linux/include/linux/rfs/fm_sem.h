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
/*  This file implements the semaphore functions for Linux RFS.         */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Dong-Hee Lee                                               */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fm_sem.h                                                  */
/*  PURPOSE : Header file for Semaphore Functions for Linux RFS         */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Dong-Hee Lee] : first writing                         */
/*  - 01/10/2003 [Joosun Hahn]  : added SYSTEM_SEMAPHORE for Linux      */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _FM_SEM_H
#define _FM_SEM_H

#include "fm_global.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

#define NO_SEMAPHORE        0            /* do not use semaphore */
#define SYSTEM_SEMAPHORE    1            /* use system semaphore */
#define SELF_SEMAPHORE      2            /* use private semaphore */

/* select one of the above semaphore methods for RFS semaphore */

#define RFS_SEMAPHORE       SYSTEM_SEMAPHORE

#if !defined(ZFLASH_LINUX) || !defined(ZFLASH_KERNEL)
#undef  RFS_SEMAPHORE
#define RFS_SEMAPHORE       NO_SEMAPHORE
#endif

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  External Function Declarations                                      */
/*----------------------------------------------------------------------*/

#if (RFS_SEMAPHORE == NO_SEMAPHORE)

#define SM_P()
#define SM_V()
#define INIT_SM()
#define QUERY_SM_INIT()         (TRUE)

#else

INT32 SM_P(void);
INT32 SM_V(void);
INT32 INIT_SM(void);
INT32 QUERY_SM_INIT(void);

#endif
#endif /* _FM_SEM_H */

/* end of fm_sem.h */
