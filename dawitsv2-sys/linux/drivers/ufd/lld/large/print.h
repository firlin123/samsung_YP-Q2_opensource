/*
 * Debug and conversion macros
 *
 * 2005 (c) Samsung Electronics
 *
 * @author    Heechul Yun <heechul.yun@samsung.com>
 */
                                                                                

#ifndef PRINT_H_ 
#define PRINT_H_

#define TP_MIN 1
#define TP_MED 2 

#include <linux/kernel.h> 
#include <linux/mm.h> 
#include <asm/memory.h> 

#define TPRINTF(x,y)	printk y 

#define DEBUG 

#define DEBUG_CHAIN 0
#define DEBUG_DATA  0
#define DEBUG_SEMA  1

#ifdef DEBUG 
  #define ASSERT(x) (x)?((void)0):panic("Assertion failed at %s:%d \n",__FUNCTION__,__LINE__)
#else
  #define ASSERT(x) ((void)0)
#endif 

extern volatile int lld_debug; 

#undef PDEBUG             /* undef it, just in case */

#ifdef DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) if (lld_debug) printk( "lld: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) if (lld_debug) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif


#endif /* PRINT_H_ */ 

