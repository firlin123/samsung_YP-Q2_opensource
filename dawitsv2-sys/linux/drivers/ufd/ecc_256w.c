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
/*  FILE    : ecc_256w.c                                                */
/*  PURPOSE : Code for handling ECC (especially for 256 words data)     */
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

#include "fm_global.h"
#include "fd_if.h"
#include "fd_physical.h"
#include "ecc.h"

#if (ZFLASH_EMUL || USE_SMALL_DIRECT16_NAND || USE_LARGE_DIRECT16_NAND)
#if (ECC_METHOD == SW_ECC)
/************************************************************************/
/*                                                                      */
/* NAME                                                                 */
/*        make_ecc_256word                                              */
/* DESCRIPTION                                                          */
/*        This function generates 22 bit ECC for 256 word data.         */
/* PARAMETERS                                                           */
/*        ecc_buf       the location where ECC should be stored         */
/*        data_buf      given data                                      */
/* RETURN VALUES                                                        */
/*        Nothing                                                       */
/*                                                                      */
/************************************************************************/
void make_ecc_256word(UINT16 *ecc_buf, UINT16 *data_buf)
{
    UINT32  iIndex;

    UINT32  iLsum, iParity;

    UINT32  iParityLo;
    UINT32  iParityHi;

    UINT8   iInvLow, iInvHigh;
    UINT8   iLow, iHigh;

    UINT16  iP1_1, iP1_2, iP2_1, iP2_2, iP4_1, iP4_2, iP8_1, iP8_2;
    UINT16  iP16_1, iP16_2, iP32_1, iP32_2, iP64_1, iP64_2, iP128_1, iP128_2;
    UINT16  iP256_1, iP256_2, iP512_1, iP512_2, iP1024_1, iP1024_2, iP2048_1, iP2048_2;

    UINT32  *temp_datal = (UINT32 *)data_buf;

    iParityLo = iParityHi = iLsum = 0;

    // bit position [0 ~ 63]
    for (iIndex = 0; iIndex < (256/4); iIndex++ ) {
        iParityLo = iParityLo ^ (*(temp_datal + iIndex * 2));
        iParityHi = iParityHi ^ (*(temp_datal + iIndex * 2 + 1));
    }

    iParity = iParityLo ^ iParityHi;
    for (iIndex = 0; iIndex < 32; iIndex++)
    {
        iLsum ^= ((iParity >> iIndex) & 0x1);    
    }

    iParity = iParityLo ^ iParityHi;        // bit0
    iParity &= 0x55555555;

    iParity = (iParity >> 16) ^ iParity;    
    iParity = (iParity >> 8) ^ iParity;     
    iParity = (iParity >> 4) ^ iParity;     
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iLow = (iParity & 0x1);

    iParity = iParityLo ^ iParityHi;        // bit1
    iParity &= 0x33333333;

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iLow |= ((iParity & 0x1) << 1);

    iParity = iParityLo ^ iParityHi;        // bit2
    iParity &= 0x0F0F0F0F;

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iLow |= ((iParity & 0x1) << 2);

    iParity = iParityLo ^ iParityHi;        // bit3
    iParity &= 0x00FF00FF;

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iLow |= ((iParity & 0x1) << 3);

    iParity = iParityLo ^ iParityHi;        // bit4
    iParity &= 0x0000FFFF;

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iLow |= ((iParity & 0x1) << 4);

    iParity = iParityLo;                    // bit5

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iLow |= ((iParity & 0x1) << 5);
    iLow &= 0x3f;

    if (iLsum == 0) 
        iInvLow = iLow;
    else 
        iInvLow = ~iLow & 0x3f;

    iParityLo = iParityHi = iLsum = 0;

    // word position [0 ~ 63]
    for (iIndex = 0; iIndex < (256/4); iIndex++)
    {
        UINT32 iDataLo, iDataHi;

        iDataLo = (*(temp_datal + iIndex * 2));
        iDataHi = (*(temp_datal + iIndex * 2 + 1));
    
        iDataLo = iDataLo ^ iDataHi;

        iDataLo = (iDataLo >> 16) ^ iDataLo;
        iDataLo = (iDataLo >> 8) ^ iDataLo;
        iDataLo = (iDataLo >> 4) ^ iDataLo;
        iDataLo = (iDataLo >> 2) ^ iDataLo;
        iDataLo = (iDataLo >> 1) ^ iDataLo;

        if (iIndex < 32)
            iParityLo |= ((iDataLo & 0x01) << iIndex);
        else
            iParityHi |= ((iDataLo & 0x01) << (iIndex - 32));

        iLsum ^= (iDataLo & 0x1);
    }

    iParity = iParityLo ^ iParityHi;        // bit0
    iParity &= 0x55555555;

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iHigh = (iParity & 0x1);

    iParity = iParityLo ^ iParityHi;        // bit1
    iParity &= 0x33333333;

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iHigh |= ((iParity & 0x1) << 1);

    iParity = iParityLo ^ iParityHi;        // bit2
    iParity &= 0x0F0F0F0F;

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iHigh |= ((iParity & 0x1) << 2);

    iParity = iParityLo ^ iParityHi;        // bit3
    iParity &= 0x00FF00FF;

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iHigh |= ((iParity & 0x1) << 3);

    iParity = iParityLo ^ iParityHi;        // bit4
    iParity &= 0x0000FFFF;

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iHigh |= ((iParity & 0x1) << 4);

    iParity = iParityLo;                    // bit5

    iParity = (iParity >> 16) ^ iParity;
    iParity = (iParity >> 8) ^ iParity;
    iParity = (iParity >> 4) ^ iParity;
    iParity = (iParity >> 2) ^ iParity;
    iParity = (iParity >> 1) ^ iParity;

    iHigh |= ((iParity & 0x1) << 5);
    iHigh &= 0x3f;

    if (iLsum == 0) 
        iInvHigh = iHigh;
    else 
        iInvHigh = ~iHigh & 0x3f;


    iP1_2 = (iLow & 0x1);
    iP1_1 = (iInvLow & 0x1) << 1;
    iP2_2 = (iLow & 0x2) << 1;
    iP2_1 = (iInvLow & 0x2) << 2;
    iP4_2 = (iLow & 0x4) << 2;
    iP4_1 = (iInvLow & 0x4) << 3;

    iP8_2 = (iLow & 0x8) << 3;
    iP8_1 = (iInvLow & 0x8) << 4;
    iP16_2 = (iLow & 0x10) >> 4;
    iP16_1 = (iInvLow & 0x10) >> 3;
    iP32_2 = (iLow & 0x20) >> 3;
    iP32_1 = (iInvLow & 0x20) >> 2;

    iP64_2 = (iHigh & 0x1) << 4;
    iP64_1 = (iInvHigh & 0x1) << 5;
    
    iP128_2 = (iHigh & 0x2) << 5;
    iP128_1 = (iInvHigh & 0x2) << 6;
    iP256_2 = (iHigh & 0x4) << 6;
    iP256_1 = (iInvHigh & 0x4) << 7;
    iP512_2 = (iHigh & 0x8) << 7;
    iP512_1 = (iInvHigh & 0x8) << 8;
    
    iP1024_2 = (iHigh & 0x10) << 8;
    iP1024_1 = (iInvHigh & 0x10) << 9;
    iP2048_2 = (iHigh & 0x20) << 9;
    iP2048_1 = (iInvHigh & 0x20) << 10;

    *(ecc_buf + 0) = ~( iP2048_1|iP2048_2|iP1024_1|iP1024_2|iP512_1|iP512_2|iP256_1|iP256_2
              |iP128_1|iP128_2|iP64_1|iP64_2|iP32_1|iP32_2|iP16_1|iP16_2);

    *(ecc_buf + 1) = 0xFF00 | ~( iP8_1|iP8_2|iP4_1|iP4_2|iP2_1|iP2_2|iP1_1|iP1_2);
}
#endif /* (ECC_METHOD == SW_ECC) */


#if (ECC_METHOD == HW_ECC) || (ECC_METHOD == SW_ECC)
/************************************************************************/
/*                                                                      */
/* NAME                                                                 */
/*        compare_ecc_256word                                           */
/* DESCRIPTION                                                          */
/*        This function compares two ECCs and indicates if there is     */
/*        an error.                                                     */
/* PARAMETERS                                                           */
/*        ecc_data1     one ECC to be compared                          */
/*        ecc_data2     the other ECC to be compared                    */
/*        page_data     content of data page                            */
/*        offset        byte offset where the error occurred            */
/*        corrected     correct data for the errorneous word            */
/* RETURN VALUES                                                        */
/*        Upon successful completion, compare_ecc returns ECC_NO_ERROR. */
/*        Otherwise, corresponding error code is returned.              */
/*                                                                      */
/************************************************************************/
INT32 compare_ecc_256word(UINT8 *ecc_data1, UINT8 *ecc_data2, 
                          UINT16 *page_data, INT32 *offset, 
                          UINT16 *corrected)
{
    UINT16  *pEcc1 = (UINT16 *)ecc_data1;
    UINT16  *pEcc2 = (UINT16 *)ecc_data2;
    UINT32  iCompecc = 0, iEccsum = 0;
    UINT16  iNewvalue;
    UINT16  iFindbit;
    UINT32  iFindword;
    UINT32  iIndex;
    
    for (iIndex = 0; iIndex < 2; iIndex++)
        iCompecc |= (((~*pEcc1++) ^ (~*pEcc2++)) << iIndex * 16);
    
    for(iIndex = 0; iIndex < 24; iIndex++) {
        iEccsum += ((iCompecc >> iIndex) & 0x01);
    }

    switch (iEccsum) {
    
    case 0 : /* no error */
             /* actually not reached */
             /* if this function is not called because two ecc's are equal */
        return ECC_NO_ERROR;

    case 1 : 
        return ECC_ECC_ERROR;

    case 12 : /* one bit ECC error */
        iFindword = ((iCompecc >> 15 & 1) << 7) + ((iCompecc >> 13 & 1) << 6)
                + ((iCompecc >> 11 & 1) << 5) + ((iCompecc >> 9 & 1) << 4) 
                + ((iCompecc >> 7 & 1) << 3)  + ((iCompecc >> 5 & 1) << 2) 
                + ((iCompecc >> 3 & 1) << 1) + (iCompecc >> 1 & 1);
        
        iFindbit = ((iCompecc >> 23 & 1) << 3) + ((iCompecc >> 21 & 1) << 2) 
                + ((iCompecc >> 19 & 1) << 1) + (iCompecc >> 17 & 1);
        
        iNewvalue = (page_data[iFindword] ^ (1 << iFindbit));
        
        if (offset != NULL) {
            *offset = (INT32)iFindword;
        }

        if (corrected != NULL) {
            *corrected = iNewvalue;
        }
        FD_ECC_Corrected = TRUE;
        return ECC_CORRECTABLE_ERROR;   /* one bit data error */
    
    default : /* more than one bit ECC error */
        return ECC_UNCORRECTABLE_ERROR;

    }
}
#endif /* (ECC_METHOD == HW_ECC) || (ECC_METHOD == SW_ECC) */
#endif /* (ZFLASH_EMUL || USE_SMALL_DIRECT16_NAND || USE_LARGE_DIRECT16_NAND) */
