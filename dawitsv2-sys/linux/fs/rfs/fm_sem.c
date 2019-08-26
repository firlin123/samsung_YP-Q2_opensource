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
/*  FILE    : fm_sem.c                                                  */
/*  PURPOSE : Semaphore Functions for Linux RFS                         */
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

#include "fm_global.h"
#include "fm_sem.h"

#if (RFS_SEMAPHORE == SELF_SEMAPHORE)
/*======================================================================*/
/*                                                                      */
/*                      SELF-SEMAPHORE DEFINITIONS                      */
/*                                                                      */
/*======================================================================*/

#ifdef ZFLASH_APP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef ZFLASH_WIDNOWS
#include <sys/types.h>
#endif
#endif

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

#define SM_GLOBAL       0
#define SM_LOCAL        1
#define SM_PRIOR        2
#define SM_FIFO         4
#define SM_BOUNDED      8
#define SM_WAIT         16
#define SM_NOWAIT       32

#define SM_ERROR        -1
#define SM_SUCCESS      0

#define NUMSEMSLOTS     128

/* DMSG() macro - Debugging */
#ifdef DEBUG_FM_SEM
#define DMSG(DebugLevel, fmt_and_args)  \
        _DMSG(DMSG_FM_SEM, DebugLevel, fmt_and_args)
#else
#define DMSG(DebugLevel, fmt_and_args) 
#endif

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

typedef UINT32 perror_t;
typedef UINT32 psem_t;
typedef INT8   *pname_t;
typedef INT32  pcount_t;
typedef UINT32 pbitfield_t;
typedef INT32  int32;
typedef UINT32 pnode_t;

/*----------------------------------------------------------------------*/
/*  Local Variable Definitions                                          */
/*----------------------------------------------------------------------*/

static psem_t   smid = -1;
static INT32 sm_initialized = FALSE;

static struct {
    INT32       used;
    INT8        name[5];
    pbitfield_t flag;
    pcount_t    init_value;
    pcount_t    cur_value;
} semslot[NUMSEMSLOTS];

/*----------------------------------------------------------------------*/
/*  Local Function Declarations                                         */
/*----------------------------------------------------------------------*/

static void sm_init(void);
static perror_t sm_create(pname_t name, pcount_t count, 
                   pbitfield_t flags, psem_t *smid);
static perror_t sm_delete(psem_t smid);
static perror_t sm_ident(pname_t name, pnode_t node, psem_t *smid);
static perror_t sm_p(psem_t smid, pbitfield_t flags, int32 timeout);
static perror_t sm_v(psem_t smid);

/*----------------------------------------------------------------------*/
/*  Function Definitions                                                */
/*----------------------------------------------------------------------*/

INT32 SM_P(void){
    return(sm_p(smid, SM_WAIT, 0));
} /* end of SM_P */

INT32 SM_V(void) {
    return(sm_v(smid));
} /* end of SM_V */

INT32 INIT_SM(void) {
    if (sm_create("NFFS", 1, SM_FIFO, &smid)) {
        DMSG(DL2, ("NFFS semaphore fail\n"));
        return(-1);
    }
    return(0);
} /* end of INIT_SM */

INT32 QUERY_SM_INIT(void) {
    if (smid == -1) return(FALSE);
    return(TRUE);
} /* end of QUERY_SM_INIT */

static void sm_init(void)
{
    INT32 i, j;

    for (i = 0; i < NUMSEMSLOTS; i++) {
        semslot[i].used = FALSE;
        for (j = 0; j < 5; j++) semslot[i].name[j] = 0x00;
        semslot[i].flag = 0;
        semslot[i].init_value = semslot[i].cur_value = 0;
    }
    sm_initialized = TRUE;
}

static perror_t sm_create(pname_t name, pcount_t count, pbitfield_t flags, 
        psem_t *smid)
{
    INT8 *ch;
    INT32 i, j;

    if (!sm_initialized) sm_init();

    for (i = 0; (i < NUMSEMSLOTS) && (semslot[i].used == TRUE); i++) ;
    if (i >= NUMSEMSLOTS) return(SM_ERROR);

    semslot[i].used = TRUE;
    for (j = 0; j < 5; j++) semslot[i].name[j] = 0x00;
    if (name) {
        for (j = 0, ch = name; (*ch) && (j < 4); j++, ch++)
            semslot[i].name[j] = *ch;
    }
    semslot[i].flag = flags;
    semslot[i].init_value = semslot[i].cur_value = count;
    *smid = i;

    return(SM_SUCCESS);
}

static perror_t sm_delete(psem_t smid)
{
    if (!sm_initialized) sm_init();
    semslot[smid].used = FALSE;

    return(SM_SUCCESS);
}

static perror_t sm_ident(pname_t name, pnode_t node, psem_t *smid)
{
    INT32 i;

    if (!sm_initialized) sm_init();

    for (i = 0; i < NUMSEMSLOTS; i++) {
        if (semslot[i].used && !STRCMP(name, semslot[i].name)) {
            *smid = (psem_t) i;
            return(SM_SUCCESS);
        }
    }

    return(SM_ERROR);
}

static perror_t sm_p(psem_t smid, pbitfield_t flags, int32 timeout)
{
    if (!sm_initialized) sm_init();

    if (semslot[smid].cur_value <= 0) {
        DMSG(DL2, ("sm_p : token 0\n"));
        exit(1);
    } else semslot[smid].cur_value--;

    return(SM_SUCCESS);
}

static perror_t sm_v(psem_t smid)
{
    if (!sm_initialized) sm_init();

    semslot[smid].cur_value++;
    if (semslot[smid].flag & SM_BOUNDED) {
        if (semslot[smid].cur_value > semslot[smid].init_value) {
            DMSG(DL2, ("sm_v : bounded semaphore is larger than init value\n"));
            exit(1);
        }
    }

    return(SM_SUCCESS);
}

/* end of #if (RFS_SEMAPHORE == SELF_SEMAPHORE) */


#elif (RFS_SEMAPHORE == SYSTEM_SEMAPHORE) 
/*======================================================================*/
/*                                                                      */
/*            SEMAPHORE DEFINITIONS USING SYSTEM SEMAPHORE              */
/*                                                                      */
/*======================================================================*/

#if defined(ZFLASH_WINDOWS)
/*----------------------------------------------------------------------*/
/*  System Semaphore Definitions for WINDOWS                            */
/*----------------------------------------------------------------------*/

#include <windows.h>
static CRITICAL_SECTION CriticalSection;
static INT32 cs_inited = 0;

INT32 SM_P(void) {
    EnterCriticalSection(&CriticalSection);
    return(0);
}

INT32 SM_V(void) {
    LeaveCriticalSection(&CriticalSection);
    return(0);
}

INT32 INIT_SM(void) {
    InitializeCriticalSection(&CriticalSection);
    cs_inited = 1;
    return(0);
}

INT32 QUERY_SM_INIT(void) {
    if (cs_inited == 0) return(FALSE);
    return(TRUE);
}

/* end of #if defined(ZFLASH_WINDOWS) */


#elif defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
/*----------------------------------------------------------------------*/
/*  System Semaphore Definitions for LINUX                              */
/*----------------------------------------------------------------------*/

#include <asm/semaphore.h>
#include <linux/ufd/fd_if.h>

#if TIMING_RFS
#include <asm/arch/digctl.h>  

rfs_perf_t rfs_perf = { 
	{ 0xffffffff, 0, 0, 0 },
	{ 0xffffffff, 0, 0, 0 }
}; 

static unsigned start, duration;

#endif /* TIMING_RFS */ 

static DECLARE_MUTEX(z_sem);

#if TIMING_RFS
INT32 SM_P(void) {
	int ret = 0; 
	FD_UserSequence++;
	down(&z_sem);
	if ( !ret ) start = get_usec();
	// if ( ret ) { printk("SEM down failed..\n"); while(1); }
	return ret; 
}

INT32 SM_V(void) {
	up(&z_sem);
	duration = get_usec_elapsed(start, get_usec()); 
	rfs_perf.sem.min = MIN(rfs_perf.sem.min, duration); 
	rfs_perf.sem.max = MAX(rfs_perf.sem.max, duration); 
	rfs_perf.sem.cnt ++; 
	rfs_perf.sem.tot += duration; 
	return(0);
} /* end of SM_V */

#else /* TIMING_RFS */ 
INT32 SM_P(void) {
    FD_UserSequence++;
    down(&z_sem); // must be down() - hcyun 
    return 0; 
} /* end of SM_P */

INT32 SM_V(void) {
    up(&z_sem);
    return(0);
} /* end of SM_V */

#endif /* TIMING_RFS */ 

INT32 INIT_SM(void) {
    sema_init(&z_sem, 1);
    return(0);
} /* end of INIT_SM */

INT32 QUERY_SM_INIT(void) {
#if WAITQUEUE_DEBUG
    if (z_sem.__magic == (long) &(z_sem.__magic)) return(TRUE);
    return(FALSE);
#else
    return(TRUE);
#endif
} /* end of QUERY_SM_INIT */

#endif /* end of #elif defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL) */
#endif /* end of #elif (RFS_SEMAPHORE == SYSTEM_SEMAPHORE) */

/* end of fm_sem.c */
