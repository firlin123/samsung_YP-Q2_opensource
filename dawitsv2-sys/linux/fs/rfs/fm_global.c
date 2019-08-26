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
/*  This file implements common code and some utility functions.        */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Dong-Hee Lee                                               */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fm_global.c                                               */
/*  PURPOSE : Common Codes for Linux Robust FAT Flash File System       */
/*            (Some Utility Functions)                                  */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Dong-Hee Lee] : first writing                         */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#include "fm_global.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/compatmac.h>

#ifdef ZFLASH_APP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef ZFLASH_WINDOWS
#include <sys/types.h>
#endif
#endif

/*----------------------------------------------------------------------*/
/*  Global Variable Definitions                                         */
/*----------------------------------------------------------------------*/

/* a look-up table for translating 'a number of 2^n' into 'n' */

#if 0 /* defined in fd_global.c - hcyun */ 
UINT8 BITS[257] = {
                  /*   0,  1,  2,  3,  4,  5,  6,  7,  8,  9 */

    /*   0 ..   9 */   0,  0,  1,  1,  2,  2,  2,  2,  3,  3,  
    /*  10 ..  19 */   3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  
    /*  20 ..  29 */   4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  
    /*  30 ..  39 */   4,  4,  5,  5,  5,  5,  5,  5,  5,  5,  
    /*  40 ..  49 */   5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  
    /*  50 ..  59 */   5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  
    /*  60 ..  69 */   5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  
    /*  70 ..  79 */   6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  
    /*  80 ..  89 */   6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  
    /*  90 ..  99 */   6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  
    /* 100 .. 109 */   6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  
    /* 110 .. 119 */   6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  
    /* 120 .. 129 */   6,  6,  6,  6,  6,  6,  6,  6,  7,  7,
    
    /* 130 .. 139 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 140 .. 149 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 150 .. 159 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 160 .. 169 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 170 .. 179 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 180 .. 189 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 190 .. 199 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 200 .. 209 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 210 .. 219 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 220 .. 229 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 230 .. 239 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 240 .. 249 */   7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
    /* 250 .. 259 */   7,  7,  7,  7,  7,  7,  8
};
#endif 

#ifdef DEBUG_ZFLASH
/* set the global debugging message output level
    DL1 level 1: error msgs
    DL2 level 2: important msgs
    DL3 level 3: detailed msgs */
UINT32 DebugLevelMask = DL2;

/* debug msg header strings by debug categories */
INT8 *DMsgHeader[] = {
    "FFS",
    "FTL",
    "FM_DRV",
    "FM_EMUL",
    "FM_SEM"
};

#ifdef ZFLASH_WINDOWS
CRITICAL_SECTION    printf_critical_section;
INT32               printf_critical_section_inited = 0;
#endif
#endif /* DEBUG_ZFLASH */


/*======================================================================*/
/*                                                                      */
/*        LIBRARY FUNCTION DEFINITIONS -- WELL-KNOWN FUNCTIONS          */
/*                                                                      */
/*======================================================================*/

/*----------------------------------------------------------------------*/
/*  Memory Manipulation Functions                                       */
/*  (defined if no system memory functions are available)               */
/*----------------------------------------------------------------------*/

#ifndef USE_SYSTEM_MEMORY_FUNC

void __memset(void *mem, UINT8 value, UINT16 size)
{
    UINT8 *m = (UINT8 *) mem;
    UINT8 *last = m + size - 1;
    UINT16 value2 = ((UINT16) value << 8) | ((UINT16) value);
    
    *m = value;
    m = (UINT8 *)((UINT16)(m + 1) & 0xfffe);
    
    while (m < last) {
        *((UINT16 *) m) = value2;
        m += 2;
    }
    
    *last = value;
}

void __memcpy(void *dest, void far *src, UINT16 size)
{
    UINT16 i = 0, word_limit;
    UINT8 *d, *s;

    if ((UINT32) src > 0x10000) {

        /* far byte copy */
        for (; i < size; i++) {
            *((UINT8 *) dest + i) = *((UINT8 far *) src + i);
        }
        return;
    }

    /* prepare for near copy */    
    d = (UINT8 *) dest;
    s = (UINT8 *)((UINT16)((UINT32)src));

    if ((((UINT16) d ^ (UINT16) s) & 0x0001) == 0) {

        /* near byte copy for not-word-aligned bytes */
        for (; i < ((UINT16) s & 0x0001); i++) {
            d[i] = s[i];
        }

        word_limit = ((size - i) & 0xfffe);

        /* near word copy */
        for (; i < word_limit; i += 2) {
            *((UINT16 *)(d + i)) = *((UINT16 *)(s + i));
        }
    }

    /* near byte copy for remaining bytes */
    for (; i < size; i++) {
        d[i] = s[i];
    }
}

INT32 __memcmp(void *mem1, void *mem2, UINT16 size)
{
    UINT16 i;
    
    for (i = 0; i < size; i++) {
        if (*((UINT8 *) mem1 + i) > *((UINT8 *) mem2 + i)) return(1);
        else if (*((UINT8 *) mem1 + i) < *((UINT8 *) mem2 + i)) return(-1);
    }
    
    return(0);
}

#endif /* !defined(USE_SYSTEM_MEMORY_FUNC) */


/*======================================================================*/
/*                                                                      */
/*       LIBRARY FUNCTION DEFINITIONS -- OTHER UTILITY FUNCTIONS        */
/*                                                                      */
/*======================================================================*/

/*----------------------------------------------------------------------*/
/*  Bitmap Manipulation Functions                                       */
/*----------------------------------------------------------------------*/

#define BITMAP_LOC(v)           ((v) >> 3)
#define BITMAP_SHIFT(v)         ((v) & 0x7)

void Bitmap_set_all(UINT8 *bitmap, INT32 mapsize)
{
    MEMSET(bitmap, 0xFF, mapsize);
}  /* end of Bitmap_set_all */

void Bitmap_clear_all(UINT8 *bitmap, INT32 mapsize)
{
    MEMSET(bitmap, 0x00, mapsize);
}  /* end of Bitmap_clear_all */

INT32 Bitmap_test(UINT8 *bitmap, INT32 i)
{
    UINT8   data;

    data = bitmap[BITMAP_LOC(i)];
    if ((data >> BITMAP_SHIFT(i)) & 0x01) return(1);
    return(0);
}  /* end of Bitmap_test */

void Bitmap_set(UINT8 *bitmap, INT32 i)
{
    bitmap[BITMAP_LOC(i)] |= (0x01 << BITMAP_SHIFT(i));
}  /* end of Bitmap_set */

void Bitmap_clear(UINT8 *bitmap, INT32 i)
{
    bitmap[BITMAP_LOC(i)] &= ~(0x01 << BITMAP_SHIFT(i));
}  /* end of Bitmap_clear */

void Bitmap_nbits_set(UINT8 *bitmap, INT32 offset, INT32 nbits)
{
    INT32   i;

    for (i = 0; i < nbits; i++) {
        Bitmap_set(bitmap, offset+i);
    }
}

void Bitmap_nbits_clear(UINT8 *bitmap, INT32 offset, INT32 nbits)
{
    INT32   i;

    for (i = 0; i < nbits; i++) {
        Bitmap_clear(bitmap, offset+i);
    }
}

/*----------------------------------------------------------------------*/
/*  List Manipulation Functions                                         */
/*----------------------------------------------------------------------*/

void listout(struct D_LIST_T *bp)
{
    bp->prev->next = bp->next;
    bp->next->prev = bp->prev;
} /* end of listout */

void push_to_mru(struct D_LIST_T *bp, struct D_LIST_T *list)
{
    bp->next = list->next;
    bp->prev = list;
    list->next->prev = bp;
    list->next = bp;
} /* end of push_to_mru */

void push_to_lru(struct D_LIST_T *bp, struct D_LIST_T *list)
{
    bp->prev = list->prev;
    bp->next = list;
    list->prev->next = bp;
    list->prev = bp;
} /* end of push_to_lru */

void move_to_mru(struct D_LIST_T *bp, struct D_LIST_T *lrulist)
{
    listout(bp);
    push_to_mru(bp, lrulist);
} /* end of move_to_mru */

void move_to_lru(struct D_LIST_T *bp, struct D_LIST_T *lrulist)
{
    listout(bp);
    push_to_lru(bp, lrulist);
} /* end of move_to_lru */


/*----------------------------------------------------------------------*/
/*  Miscellaneous Library Functions                                     */
/*----------------------------------------------------------------------*/

/* integer to ascii conversion */
void my_itoa(INT8 *buf, INT32 v)
{
    INT32 mod[10];
    INT32 i;

    for (i = 0; i < 10; i++) {
        mod[i] = (v % 10);
        v = v / 10;
        if (v == 0) break;
    }
    for (; i >= 0; i--) {
        *buf = (INT8) ('0' + mod[i]);
        buf++;
    }
    *buf = '\0';
} /* end of my_itoa */

/* value to log2(value) conversion */
INT32 my_log2(UINT32 v)
{
    UINT32 bits = 0;

    while (v > 1) {
        if (v & 0x1) return -1;
        v >>= 1;
        bits++;
    }
    return bits;
} /* end of my_log2 */

#ifdef ZFLASH_APP
UINT32 rand_count = 1;

#ifdef ZFLASH_WINDOWS
UINT32 my_rand()
{
    INT32 rand1, rand2, rand3;

    rand_count++;
    rand1 = rand();
    rand2 = rand();
    rand3 = rand();
    return (rand1+rand2+rand3)%90000;
}
#else 
UINT32 my_rand(void)
{
    srand(rand_count++);
    return(rand()%5000000);
} /* end of my_rand */
#endif
#endif


EXPORT_SYMBOL(listout);
EXPORT_SYMBOL(push_to_mru);
EXPORT_SYMBOL(push_to_lru);
EXPORT_SYMBOL(move_to_mru);
EXPORT_SYMBOL(move_to_lru);


/* end of fm_global.c */
