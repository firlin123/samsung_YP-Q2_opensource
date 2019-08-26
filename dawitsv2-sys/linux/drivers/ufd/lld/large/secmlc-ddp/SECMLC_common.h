/* 
 * \file SECMLC_common.h 
 *
 * \brief generic SECMLC timing constants, commands, address structions 
 *
 * Based on Sigmatel validation code. 
 * Linux port is done by Samsung Electronics 
 * 
 * 2005 (C) Samsung Electronics. 
 * 
 * \author Heechul Yun <heechul.yun@samsung.com>
 * \version $Revision: 1.3 $ 
 * \date $Date: 2007/08/16 07:49:10 $
 * 
 */ 

#ifndef _SECMLC_COMMON_H_
#define _SECMLC_COMMON_H_


#include "../nand_common.h"
#include "../gpmi_dma.h"
#include "../print.h"
#include "../sec_nand.h"

#define ROW_SIZE 3  /**< 128M < size < 32GB (assume 1 page = 2KB. for 4KB page, max 64GB */ 
#define COL_SIZE 2 

#define USE_16GBIT_NAND 1  /* 33bit address is required. discard lower 4 bits.  */

//------------------------------------------------------------------------------
// device addressing

struct _SECMLC_col_t
{
	volatile reg8_t byte1;
	volatile reg8_t byte2;
}__attribute__((packed));

typedef struct _SECMLC_col_t SECMLC_col_t;


struct _SECMLC_row_t
{
	volatile reg8_t byte1;
	volatile reg8_t byte2;
	volatile reg8_t byte3;
	
} __attribute__((packed));

typedef struct _SECMLC_row_t SECMLC_row_t;


struct _SECMLC_address_t
{
    SECMLC_col_t col;
    SECMLC_row_t row;
} __attribute__((packed));

typedef struct _SECMLC_address_t SECMLC_address_t;


void SECMLC_init_addr         (int row_shift );
void SECMLC_offset_to_address (SECMLC_address_t *address, reg32_t offset);
void SECMLC_offset_to_col     (SECMLC_col_t *col, reg32_t offset);
void SECMLC_offset_to_row     (SECMLC_row_t *row, reg32_t offset);

#endif // !_SECMLC_COMMON_H_
