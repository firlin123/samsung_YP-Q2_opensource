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
/*  This file implements global definitions of Linux RFS.               */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Dong-Hee Lee                                               */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fm_global.h                                               */
/*  PURPOSE : Header file for global definitions of Linux RFS           */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - Header file for global definitions of Linux RFS including...      */
/*    . Source Code Configurations                                      */
/*    . Flash Memory Characteristics                                    */
/*    . Global Constants and Type Definitions                           */
/*    . Global Variables and Functions Declarations                     */
/*                                                                      */
/*  - Primitive data types are redefined here and should be redefined   */
/*    to meet their sizes for each platform. Developers who uses        */
/*    Linux RFS are recommended to use these types instead of           */
/*    primitive data types given by each OS platform for portability.   */
/*                                                                      */
/*    Example for Linux                                                 */
/*        typedef char                INT8;     // 1 byte signed int    */
/*        typedef short               INT16;    // 2 byte signed int    */
/*        typedef int                 INT32;    // 4 byte signed int    */
/*        typedef long long           INT64;    // 8 byte signed int    */
/*                                                                      */
/*        typedef unsigned char       UINT8;    // 1 byte unsigned int  */
/*        typedef unsigned short      UINT16;   // 2 byte unsigned int  */
/*        typedef unsigned int        UINT32;   // 4 byte unsigned int  */
/*        typedef unsigned long long  UINT64;   // 8 byte unsigned int  */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Dong-Hee Lee] : first writing                         */
/*  - 01/10/2003 [Joosun Hahn]  : style modified                        */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _FM_GLOBAL_H
#define _FM_GLOBAL_H

/*======================================================================*/
/*                                                                      */
/*                    RFS GLOBAL CONFIGURATIONS                         */
/*                  (CHANGE THIS PART IF REQUIRED)                      */
/*                                                                      */
/*======================================================================*/

/*----------------------------------------------------------------------*/
/*  Target OS Platform (select one of the followings)                   */
/*----------------------------------------------------------------------*/

#define ZFLASH_LINUX            1       // Z-Flash for Linux
//#define ZFLASH_WINDOWS          1       // Z-Flash for Windows
//#define ZFLASH_SMDK2410         1       // Z-Flash for NonOS-SMDK2410
//#define ZFLASH_DVS              1       // Z-Flash for NonOS-DVS

/*----------------------------------------------------------------------*/
/*  Execution Environment                                               */
/*----------------------------------------------------------------------*/

/* runs as an OS (kernel) component? */

#ifdef __KERNEL__
  #define ZFLASH_KERNEL           1
#endif

/* runs as a BLeX (Boot Loader eXpress) component? */

#if defined(ZFLASH_LINUX) && !defined(ZFLASH_KERNEL)
  #define ZFLASH_BLEX             1
#endif

/* runs as an application? */

#if !defined(ZFLASH_LINUX) || \
   (!defined(ZFLASH_KERNEL) && !defined(ZFLASH_BLEX))
  #define ZFLASH_APP              1
#endif

/* flash memory & device driver */

#if defined(ZFLASH_WINDOWS) || (defined(ZFLASH_LINUX) && defined(ZFLASH_APP))
  #define ZFLASH_EMUL             1     // emulate flash memory
  #ifdef ZFLASH_EMUL
//#define CRASH_TEST              1     // test Flash Memory crash condition
//#define MEM_SAVE                1     // save to memory (not disk)
//#define FM_LINUX_TIMING         1     // emulate the timing of Flash Memory
//#define FM_PERFMEASURE          1
  #endif
#endif

/* runs as a demo version? (limited # of mount counts) */

//#define ZFLASH_DEMO             1

/*======================================================================*/
/*                                                                      */
/*                       CONSTANT DEFINITIONS                           */
/*                                                                      */
/*======================================================================*/

/*----------------------------------------------------------------------*/
/*  Well-Known Constants (DO NOT CHANGE THIS PART !!)                   */
/*----------------------------------------------------------------------*/

#ifndef TRUE
#define TRUE                    1
#endif
#ifndef FALSE
#define FALSE                   0
#endif
#ifndef OK
#define OK                      0
#endif
#ifndef NULL
#define NULL                    0
#endif

#define SECTOR_SIZE             512
#define SECTOR_BITS             9
#define SECTOR_MASK             (SECTOR_SIZE - 1)

//#if __GNUC__ == 3 && __GNUC_MINOR__ >= 3
#define DPRINTK(format, args...)					\
do {									\
       printk("%s[%d]: " format "\n", __func__, __LINE__, ##args);	\
} while (0)
//#endif

#define assert(expr) (expr)?((void)0):panic("Assertion failed at %s:%d \n",__FUNCTION__,__LINE__)

/* Min/Max macro */

#define MIN(a, b)               ((a < b) ? a : b)
#define MAX(a, b)               ((a > b) ? a : b)

/*----------------------------------------------------------------------*/
/*  Global Sizes (CHANGE THIS PART IF REQUIRED)                         */
/*----------------------------------------------------------------------*/

#define MAX_DRIVE               2       // MAX_FTL_DEVICE * MAX_VOLUME
#define MAX_VOLUME              4       // should NOT be greater than 4
#define MAX_FTL_DEVICE          2       // max # of FTL block devices

/*======================================================================*/
/*                                                                      */
/*                         TYPE DEFINITIONS                             */
/*                  (CHANGE THIS PART IF REQUIRED)                      */
/*                                                                      */
/*======================================================================*/

/* type definitions for primitive types;
   these should be re-defined to meet its size for each OS platform;
   these should be used instead of primitive types for portability... */

typedef char                    INT8;   // 1 byte signed integer
typedef short                   INT16;  // 2 byte signed integer
typedef int                     INT32;  // 4 byte signed integer

typedef unsigned char           BOOL;

typedef unsigned char           UINT8;  // 1 byte unsigned integer
typedef unsigned short          UINT16; // 2 byte unsigned integer
typedef unsigned int            UINT32; // 4 byte unsigned integer

typedef long long               INT64;  // 8 byte signed integer
typedef unsigned long long      UINT64; // 8 byte unsigned integer

/*======================================================================*/
/*                                                                      */
/*        LIBRARY FUNCTION DECLARATIONS -- WELL-KNOWN FUNCTIONS         */
/*                  (CHANGE THIS PART IF REQUIRED)                      */
/*                                                                      */
/*======================================================================*/

/*----------------------------------------------------------------------*/
/*  Heap Allocation Macros & Functions                                  */
/*----------------------------------------------------------------------*/

#define USE_SYSTEM_HEAP_FUNC            1

#if (USE_SYSTEM_HEAP_FUNC == 1)

/* macro for the system malloc() function */

#ifdef ZFLASH_APP
#include <stdlib.h>
#define SYS_MALLOC(size)                malloc(size)
#define SYS_FREE(mem)                   free(mem)

#else
#if defined(ZFLASH_LINUX)
#include <linux/mm.h>
#include <linux/slab.h>
#define SYS_MALLOC(size)                kmalloc(size, GFP_ATOMIC)
#define SYS_FREE(mem)                   kfree(mem)
#endif
#endif

/* macro for malloc() */

#define MALLOC(size)                    SYS_MALLOC(size)
#define FREE(mem)                       do {                                \
                                            if (mem != NULL) SYS_FREE(mem); \
                                        } while(0)
                                        
#else /* (USE_SYSTEM_HEAP_FUNC == 0) */

void   *__malloc(UINT32 size);
void    __free(void *mem);

/* macro for malloc() */

#define MALLOC(size)                    __malloc(size)
#define FREE(mem)                       do {                                \
                                            if (mem != NULL) __free(mem);   \
                                        } while(0)

#endif /* (USE_SYSTEM_HEAP_FUNC == 1) */

/* CALLOC: malloc and memclr */

void   *CALLOC(UINT32 size);

/*----------------------------------------------------------------------*/
/*  Memory Manipulation Macros & Functions                              */
/*----------------------------------------------------------------------*/

#define USE_SYSTEM_MEMORY_FUNC          1

#ifdef USE_SYSTEM_MEMORY_FUNC
  #if defined(ZFLASH_KERNEL)
    #include <linux/string.h>
  #elif defined(ZFLASH_APP)
    #include <string.h>
  #endif
  #define MEMSET(mem, value, size)        memset(mem, value, size)
  #define MEMCPY(dest, src, size)         memcpy(dest, src, size)
  #define MEMCMP(mem1, mem2, size)        memcmp(mem1, mem2, size)
#else
  void  __memset(void *mem, UINT8 value, UINT16 size);
  void  __memcpy(void *dest, void far *src, UINT16 size);
  INT32 __memcmp(void *mem1, void *mem2, UINT16 size);

  #define MEMSET(mem, value, size)        __memset(mem, value, size)
  #define MEMCPY(dest, src, size)         __memcpy(dest, src, size)
  #define MEMCMP(mem1, mem2, size)        __memcmp(mem1, mem2, size)
#endif /* USE_SYSTEM_MEMORY_FUNC */ 

/*----------------------------------------------------------------------*/
/*  String Manipulation Macros & Functions                              */
/*----------------------------------------------------------------------*/

#define USE_SYSTEM_STRING_FUNC          1

#ifdef USE_SYSTEM_STRING_FUNC
  #ifdef ZFLASH_APP
    #include <string.h>
  #endif
  #define STRCPY(dest, src)               strcpy(dest, src)
  #define STRNCPY(dest, src, n)           strncpy(dest, src, n)
  #define STRCMP(str1, str2)              strcmp(str1, str2)
  #define STRNCMP(str1, str2, n)          strncmp(str1, str2, n)
  #define STRLEN(str)                     strlen(str)
  #define STRCHR(str, chr)                strchr(str, chr)
  #define STRRCHR(str, chr)               strrchr(str, chr)
#endif

/*----------------------------------------------------------------------*/
/*  Time Delay Macros & Functions                                       */
/*----------------------------------------------------------------------*/

#ifdef ZFLASH_KERNEL
  #include <linux/delay.h>
  //#define DELAY_IN_USEC(usecs)            udelay(usecs)
  #define DELAY_IN_USEC(usecs)
#else
  #define DELAY_IN_USEC(usecs)
#endif

/*======================================================================*/
/*                                                                      */
/*       LIBRARY FUNCTION DECLARATIONS -- OTHER UTILITY FUNCTIONS       */
/*                    (DO NOT CHANGE THIS PART !!)                      */
/*                                                                      */
/*======================================================================*/

/*----------------------------------------------------------------------*/
/*  Bitmap Manipulation Functions                                       */
/*----------------------------------------------------------------------*/

void    Bitmap_set_all(UINT8 *bitmap, INT32 mapsize);
void    Bitmap_clear_all(UINT8 *bitmap, INT32 mapsize);
INT32   Bitmap_test(UINT8 *bitmap, INT32 i);
void    Bitmap_set(UINT8 *bitmap, INT32 i);
void    Bitmap_clear(UINT8 *bitmap, INT32 i);
void    Bitmap_nbits_set(UINT8 *bitmap, INT32 offset, INT32 nbits);
void    Bitmap_nbits_clear(UINT8 *bitmap, INT32 offset, INT32 nbits);

/*----------------------------------------------------------------------*/
/*  List Manipulation Functions                                         */
/*----------------------------------------------------------------------*/

struct D_LIST_T {
    struct D_LIST_T *next, *prev;
};

void    listout(struct D_LIST_T *bp);
void    push_to_mru(struct D_LIST_T *bp, struct D_LIST_T *list);
void    push_to_lru(struct D_LIST_T *bp, struct D_LIST_T *list);
void    move_to_mru(struct D_LIST_T *bp, struct D_LIST_T *lrulist);
void    move_to_lru(struct D_LIST_T *bp, struct D_LIST_T *lrulist);

/*----------------------------------------------------------------------*/
/*  Miscellaneous Library Functions                                     */
/*----------------------------------------------------------------------*/

void    my_itoa(INT8 *buf, INT32 v);
INT32   my_log2(UINT32 v);

#ifdef ZFLASH_APP
  extern UINT32 rand_count;
  UINT32  my_rand(void);
#endif

/*======================================================================*/
/*                                                                      */
/*                    GLOBAL VARIABLE DECLARATIONS                      */
/*                                                                      */
/*======================================================================*/

extern UINT8 BITS[257];               // lookup table for log of base 2

/*======================================================================*/
/*                                                                      */
/*                    DEFINITIONS FOR DEBUGGING                         */
/*                  (CHANGE THIS PART IF REQUIRED)                      */
/*                                                                      */
/*======================================================================*/

// #define DEBUG_ZFLASH                 // global debug ON/OFF switch

/* debug ON/OFF definitions for each module */
#define VERBOSE			0

#ifdef DEBUG_ZFLASH
#define DEBUG_FFS               1     // debug for FFS module 
#define DEBUG_FTL               1     // debug for FTL module 
#define DEBUG_FM_DRV            1     // debug for device driver module
#define DEBUG_FM_EMUL           1     // debug for device emulator module
#define DEBUG_FM_SEM            1     // debug for semaphore module
#endif /* DEBUG_ZFLASH */ 

/* debug categories & levels */

enum DEBUG_CATEGORY {
    DMSG_FFS                  = 0,    // debug msgs for FFS module
    DMSG_FTL                  = 1,    // debug msgs for FTL module
    DMSG_FM_DRV               = 2,    // debug msgs for device driver module
    DMSG_FM_EMUL              = 3,    // debug msgs for device emulator module
    DMSG_FM_SEM               = 4     // debug msgs for semaphore module
};

enum DEBUG_LEVEL {
    DL1                       = 1,    // level 1: error msgs
    DL2                       = 2,    // level 2: important msgs
    DL3                       = 3     // level 3: detailed msgs
};

#ifdef PRINT
#undef PRINT
#endif

#if defined(ZFLASH_LINUX) && !defined(ZFLASH_APP)
  #if defined(ZFLASH_KERNEL)
    #include <linux/kernel.h>
  #elif defined(ZFLASH_BLEX)
    #include "printk.h"
  #endif
  #define PRINT       printk

#elif defined(ZFLASH_SMDK2410)
  extern void         Uart_SendString(char *pt);
  extern void         Uart_Printf(char *fmt,...);
  #define PRINT       Uart_Printf
  #define PRINT_STR   Uart_SendString

#elif defined(ZFLASH_DVS)
  #define PRINT
#else
  #define PRINT       printf
#endif

/* debug message ouput macro */

#ifdef ZFLASH_WINDOWS
  #include <windows.h>
  extern CRITICAL_SECTION printf_critical_section;
  extern INT32            printf_critical_section_inited;
#endif

#ifdef DEBUG_ZFLASH
#ifdef ZFLASH_WINDOWS
#define __DMSG(DebugCategory, DebugLevel, fmt_and_args)                 \
    do {                                                                \
        if (DebugLevel <= DebugLevelMask) {                             \
            if (!printf_critical_section_inited) {                      \
                InitializeCriticalSection(&printf_critical_section);    \
                printf_critical_section_inited = 1;                     \
            }                                                           \
            EnterCriticalSection(&printf_critical_section);             \
            PRINT("[%s] ", DMsgHeader[DebugCategory]);                  \
            PRINT fmt_and_args;                                         \
            LeaveCriticalSection(&printf_critical_section);             \
        }                                                               \
    } while (0)

#else
#define __DMSG(DebugCategory, DebugLevel, fmt_and_args)                 \
    do {                                                                \
        if (DebugLevel <= DebugLevelMask) {                             \
            PRINT("[%s] ", DMsgHeader[DebugCategory]);                  \
            PRINT fmt_and_args;                                         \
        }                                                               \
    } while (0)
#endif

#else /* !defined(DEBUG_ZFLASH) */
#define __DMSG(DebugCategory, DebugLevel, fmt_and_args) 
#endif 

#ifdef DEBUG_ZFLASH
extern UINT32   DebugLevelMask;
extern INT8     *DMsgHeader[];
#endif


typedef struct { 
	int read_fail, erase_fail, copyback_fail, write_fail; 
	int copyback_group_fail, erase_group_fail, write_group_fail;

	int corr_ecc_err, uncorr_ecc_err; 
	int lld_desc_size; /* used descriptor buffer size */ 
	int ecc_buf_size;  /* ecc buf */
	int ecc_desc_size; 
	int apbh_desc_size; 

	int nand_fail;
	int timeout; 
	int missing_irq; 
	int corrupted_bad;

	int read;
	int read_group; 
	int write; 
	int write_group; 
	int erase; 
	int erase_group;
	int copyback; 
	int copyback_group;
	int real_copyback;
	int real_copyback_group;
} lld_stat_t; 

extern lld_stat_t lld_stat; 

typedef struct { 
	int usb_write; 
	int fm_copy_back;

	int copy_merge;
	int swap_merge;
	int general_merge; 
	
	int copy_merge_copybacks;
	int swap_merge_copybacks;
	int general_merge_copybacks;
} ftl_stat_t; 

extern ftl_stat_t __ftl_stat; 

// #define DEBUG_TIMING 

/* for performance measurement */ 
typedef struct {
	unsigned min;
	unsigned max;
	unsigned cnt; 
	unsigned tot; 
} perf_t; 


#ifdef  DEBUG_TIMING 

#define TIMING_LLD 1 
#define TIMING_RFS 0 
#define TIMING_BLK 0 
#define TIMING_FTL 1

typedef struct {
	perf_t read_page;
	perf_t erase_group; 
	perf_t write_page_group; 
} lld_perf_t; 

typedef struct { 
	perf_t sem;
	perf_t statfs; 
} rfs_perf_t; 

typedef struct { 
	perf_t req; 
	perf_t sync; 
	perf_t copyback; 
	perf_t complete_group_op; 
	perf_t wr_sectors_in_q; 
	perf_t wr_sector; 
	perf_t merge;  
} ftl_perf_t; 

extern lld_perf_t lld_perf; 
extern rfs_perf_t rfs_perf; 
extern ftl_perf_t ftl_perf; 

#endif /* DEBUG_TIMING */ 

#endif /* _FM_GLOBAL_H */

/* end of fm_global.h */

/* 
   $Log $ 
*/ 

