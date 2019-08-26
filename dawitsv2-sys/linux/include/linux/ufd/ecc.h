/************************************************************************/
/*                                                                      */
/*  Copyright (c) 2004 Flash Planning Group, Samsung Electronics, Inc.  */
/*  All right reserved.                                                 */
/*                                                                      */
/*  This software is the confidential and proprietary information of    */
/*  Samsung Electronics, Inc. ("Confidential Information"). You shall   */
/*  not disclose such confidential information and shall use it only    */
/*  in accordance with the terms of the license agreement you entered   */
/*  into with Samsung Electronics.                                      */
/*                                                                      */
/************************************************************************/
/*  This file implements ECC (Error Correction Code) handling.          */
/*                                                                      */
/*  @author  Young Gon Kim                                              */
/*  @author  Jong Baek Jang                                             */
/*  @author  Jae Bum Lee                                                */
/*  @author  Hak-Yong Lee                                               */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : ecc.h                                                     */
/*  PURPOSE : Header file for handling ECC                              */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Young Gon Kim] : First writing                        */
/*  - 01/12/2003 [Sung-Kwan Kim] : Trimmed & Adapted for RFS project    */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _ECC_H
#define _ECC_H

/************************************************************************/
/* ECC Comparison Results                                               */
/************************************************************************/

enum ECC_COMPARE_RESULT {
    ECC_NO_ERROR                = 0,        /* no error */
    ECC_CORRECTABLE_ERROR       = 1,        /* one bit data error */
    ECC_ECC_ERROR               = 2,        /* one bit ECC error */
    ECC_UNCORRECTABLE_ERROR     = 3         /* uncorrectable error */
};

/************************************************************************/
/* Exported Functions                                                   */
/************************************************************************/

/* for 512 byte page */

extern void     make_ecc_512byte    (UINT8 *ecc_buf, UINT8 *data_buf);
extern INT32    compare_ecc_512byte (UINT8 *ecc_data1, UINT8 *ecc_data2, 
                                     UINT8 *page_data, INT32 *offset, 
                                     UINT8 *corrected);

/* for 256 word page */

extern void     make_ecc_256word    (UINT16 *ecc_buf, UINT16 *data_buf);
extern INT32    compare_ecc_256word (UINT8 *ecc_data1, UINT8 *ecc_data2, 
                                     UINT16 *page_data, INT32 *offset, 
                                     UINT16 *corrected);

/************************************************************************/
/* Utility Functions and Macros                                         */
/************************************************************************/

#ifdef DEBUG_ECC
#define ECC_DBG_PRINT(x)        PRINT(x)
#else
#define ECC_DBG_PRINT(x)
#endif /* DEBUG_ECC */

#endif /* _ECC_H */
