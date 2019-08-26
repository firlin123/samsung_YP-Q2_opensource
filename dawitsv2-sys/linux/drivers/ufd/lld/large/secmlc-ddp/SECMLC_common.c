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

#include "SECMLC_common.h"

static int ROW_SHIFT; // 12 - 2KB page, 13 - 4KB page. for 16Gbit NAND support do 
static int NAND_PAGE_MASK;
static int NAND_BLOCK_MASK;


void  SECMLC_init_addr(int row_shift )
{
	int i; 

	ROW_SHIFT = row_shift; 
	NAND_PAGE_MASK = 0; 

	for ( i = 0; i < row_shift - 8; i++ )
		NAND_PAGE_MASK |= (1 << i); 
#if VERBOSE
	printk("%s: ROW_SHIFT = %d\n\n", __FUNCTION__, ROW_SHIFT); 
#endif

}

void SECMLC_offset_to_address(SECMLC_address_t *address, reg32_t offset)
{
#if !USE_16GBIT_NAND  
	address->col.byte1 = ((reg8_t) ((offset >>  0) & 0xFF));
	address->col.byte2 = ((reg8_t) ((offset >>  8) & NAND_PAGE_MASK));
	address->row.byte1 = ((reg8_t) ((offset >> ROW_SHIFT) & 0xFF));
	address->row.byte2 = ((reg8_t) ((offset >> (ROW_SHIFT + 8)) & 0xFF));
	address->row.byte3 = ((reg8_t) ((offset >> (ROW_SHIFT + 16)) & 0x0F));
#else  /* USE_16GBIT_NAND */ 
	address->col.byte1 = ((reg8_t) ((offset <<  4) & 0xFF));
	address->col.byte2 = ((reg8_t) ((offset >>  4) & NAND_PAGE_MASK));
	address->row.byte1 = ((reg8_t) ((offset >> (ROW_SHIFT - 4)) & 0xFF));
	address->row.byte2 = ((reg8_t) ((offset >> (ROW_SHIFT + 4)) & 0xFF));
	address->row.byte3 = ((reg8_t) ((offset >> (ROW_SHIFT + 12)) & 0x0F));
#endif 

	PDEBUG(" ---> offset to address\n"); 
	PDEBUG(" col1 : 0x%02x\n", address->col.byte1); 
	PDEBUG(" col2 : 0x%02x\n", address->col.byte2); 
	PDEBUG(" row1 : 0x%02x\n", address->row.byte1); 
	PDEBUG(" row2 : 0x%02x\n", address->row.byte2); 
	PDEBUG(" row3 : 0x%02x\n", address->row.byte3); 
}


void SECMLC_offset_to_col(SECMLC_col_t *col, reg32_t offset)
{
#if !USE_16GBIT_NAND
	col->byte1 = ((reg8_t) ((offset >>  0) & 0xFF));
	col->byte2 = ((reg8_t) ((offset >>  8) & NAND_PAGE_MASK));
#else 
	col->byte1 = ((reg8_t) ((offset <<  4) & 0xFF));
	col->byte2 = ((reg8_t) ((offset >>  4) & NAND_PAGE_MASK));
#endif 
	PDEBUG(" ---> offset to col\n"); 
	PDEBUG(" col1 : 0x%02x\n", col->byte1); 
	PDEBUG(" col2 : 0x%02x\n", col->byte2); 

}


void SECMLC_offset_to_row(SECMLC_row_t *row, reg32_t offset)
{
#if !USE_16GBIT_NAND
	row->byte1 = ((reg8_t) ((offset >> ROW_SHIFT) & 0xFF));
	row->byte2 = ((reg8_t) ((offset >> (ROW_SHIFT + 8)) & 0xFF));
	row->byte3 = ((reg8_t) ((offset >> (ROW_SHIFT + 16)) & 0x0F));
#else 
	row->byte1 = ((reg8_t) ((offset >> (ROW_SHIFT - 4)) & 0xFF));
	row->byte2 = ((reg8_t) ((offset >> (ROW_SHIFT + 4)) & 0xFF));
	row->byte3 = ((reg8_t) ((offset >> (ROW_SHIFT + 12)) & 0x0F)); /* !FIXME! this must be 1F for 32Gbit */
#endif 
	PDEBUG(" ---> offset to row\n"); 
	PDEBUG(" row1 : 0x%02x\n", row->byte1); 
	PDEBUG(" row2 : 0x%02x\n", row->byte2); 
	PDEBUG(" row3 : 0x%02x\n", row->byte3); 

}
