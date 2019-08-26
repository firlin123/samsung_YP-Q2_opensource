/* 
 * \file SECMLC_dma.c
 *
 * \brief Implement various common dma related functions for SECMLC nand flash
 *
 * Based on Sigmatel validation code. 
 * Linux port is done by Samsung Electronics
 * 
 * 2005 (C) Samsung Electronic
 * 2005 (C) Zeen Information Technologies, Inc. 
 * 
 * \author    Heechul Yun <heechul.yun@samsung.com>
 * \author    Yung Hyun Bae <yhbae@zeen.snu.ac.kr> 
 * \version   $Revision: 1.5 $ 
 * \date      $Date: 2007/09/12 03:41:19 $
 * 
 */ 

/*****
Zeen revision history

[2005/06/30] yhbae
	- add functions for group operations
	  (brute-force modification)

[2005/07/05] yhbae
	- modify the chip selection parameter in all build_dma functions
	  for chip bank selection features
	  stmp36xx allows only ce0 and ce1 for NAND, so (chip0, chip1) and
	  (chip2, chip3) are selected by bank selection via GPIO pin.
*****/

#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <asm/memory.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>

#include "fm_global.h"
#include "fd_physical.h"
#include "SECMLC_dma.h"

//------------------------------------------------------------------------------
// common structure. 

#define SECMLC_READ_ID_SIZE   2
#define SECMLC_RESET_DEVICE_SIZE  1
#define SECMLC_READ_STATUS_SIZE  1
#define SECMLC_STATUS_SIZE  1
#define SECMLC_CHIP1_STATUS_SIZE  1
#define SECMLC_CHIP2_STATUS_SIZE  1
#define SECMLC_ID_SIZE  5

static const reg8_t SECMLC_READ_ID_COMMAND[SECMLC_READ_ID_SIZE] =
{
	FLASH_READ_ID,
	0x00
};

static const reg8_t SECMLC_RESET_DEVICE_COMMAND[SECMLC_RESET_DEVICE_SIZE] =
{
	FLASH_RESET
};


static const reg8_t SECMLC_READ_STATUS_COMMAND[SECMLC_READ_STATUS_SIZE] =
{
	FLASH_READ_STATUS
};

const reg8_t SECMLC_CHIP1_STATUS_COMMAND[SECMLC_CHIP1_STATUS_SIZE] =
{
	FLASH_CHIP1_STATUS /* 0xf1 */ 
};


const reg8_t SECMLC_CHIP2_STATUS_COMMAND[SECMLC_CHIP2_STATUS_SIZE] =
{
	FLASH_CHIP2_STATUS /* 0xf2 */ 
};


//------------------------------------------------------------------------------
// 'Read' operation : <00> <addr> <30> <wait> <data> 
void  SECMLC_build_dma_serial_read(SECMLC_dma_serial_read_t* chain, 
                                       unsigned cs,
                                       reg32_t offset,
                                       reg16_t size,
                                       void* data_buf_paddr,
                                       void* aux_buf_paddr,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success)
{
    reg16_t spare_per_sector = flash_spec->SpareSize >> BITS[flash_spec->SectorsPerPage];
    reg16_t total_size = 0;
    int use_two_rx_chain = 0;
    
    total_size = flash_spec->PageSize;
    if (spare_per_sector == 16) {
        // 4-bit ECC
        if (flash_spec->SectorsPerPage > 4) {
            use_two_rx_chain = 1;
            total_size >>= 1;
        }
    }
    else {
        // 8-bit ECC
    }

#if 0
    printk("(size=%d, total_size=%d, aux_size=%d, spare_per_sector=%d, tx_cle1_addr_dma=%08x, tx_cle2_dma=%08x, wait_dma=%08x, sense_dma=%08x, rx_data_dma=%08x, rx_data2_dma=%08x, disable_ecc_dma=%08x, success=%08x)", 
            size, total_size, aux_size, spare_per_sector, &chain->tx_cle1_addr_dma, &chain->tx_cle2_dma, &chain->wait_dma, &chain->sense_dma, &chain->rx_data_dma, &chain->rx_data2_dma, &chain->disable_ecc_dma, success);
#endif

	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // must be locked to preserve atomic transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to maintain CS between parts of transmit chain
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));
    chain->tx_cle1_addr_dma.gpmi_compare.U = 0x0;
    chain->tx_cle1_addr_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
	
	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // remain locked here as well...
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	
	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
	    (BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock to allow interleave during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(0));

	chain->sense_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->rx_data_dma));
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // unlock to allow interleave after this descriptor
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
	
	if (use_two_rx_chain) {
	    chain->rx_data_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)&(chain->rx_data2_dma));
	}
	else {
	    chain->rx_data_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)&(chain->disable_ecc_dma));
	}
	chain->rx_data_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(6) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // unlock after read sequence to allow interleave
		 // between primitive nand operations
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->rx_data_dma.bar = 0x0;
	chain->rx_data_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(total_size));
    chain->rx_data_dma.gpmi_compare.U = 0x0;
    if (spare_per_sector == 16) {
        chain->rx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, DECODE_4_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
    }
    else {
        chain->rx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, DECODE_8_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
    }
    chain->rx_data_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);
    chain->rx_data_dma.gpmi_data_ptr.U = (reg32_t)data_buf_paddr;
    chain->rx_data_dma.gpmi_aux_ptr.U = (reg32_t)aux_buf_paddr;
    
    if (use_two_rx_chain) {
        chain->rx_data2_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)&(chain->disable_ecc_dma));
    	chain->rx_data2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
    		 BF_APBH_CHn_CMD_CMDWORDS(6) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // unlock after read sequence to allow interleave
    		 // between primitive nand operations
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
    	chain->rx_data2_dma.bar = 0x0;
    	chain->rx_data2_dma.gpmi_ctrl0.U = 
    		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ) |
    		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
    		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
    		 BF_GPMI_CTRL0_CS(cs) |
    		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
    		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
    		 BF_GPMI_CTRL0_XFER_COUNT(total_size));
        chain->rx_data2_dma.gpmi_compare.U = 0x0;
        if (spare_per_sector == 16) {
            chain->rx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, DECODE_4_BIT) |
                                                 BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                 BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
        }
        else {
            chain->rx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, DECODE_8_BIT) |
                                                 BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                 BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
        }
        chain->rx_data2_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);
        chain->rx_data2_dma.gpmi_data_ptr.U = (reg32_t)(data_buf_paddr + 512*4);
        chain->rx_data2_dma.gpmi_aux_ptr.U = (reg32_t)aux_buf_paddr;
    }
		 
    chain->disable_ecc_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)success);
	chain->disable_ecc_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock after read sequence to allow interleave
		 // between primitive nand operations
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->disable_ecc_dma.bar = 0x0;
	chain->disable_ecc_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, /*READ*/WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(0));
    chain->disable_ecc_dma.gpmi_compare.U = 0x0;
    chain->disable_ecc_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
    
	chain->tx_cle1 = FLASH_READ1;
	SECMLC_offset_to_address(&chain->tx_addr, offset);
	chain->tx_cle2 = FLASH_READ1_SECOND;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}



//------------------------------------------------------------------------------
// 'Random Read' operation  : <05> <col> <e0> <data> 

void  SECMLC_build_dma_random_read(SECMLC_dma_random_read_t* chain, 
                                       unsigned cs,
				       int area,  /* main or spare */ 
                                       reg32_t offset,
                                       reg16_t size,
                                       void*  buffer_paddr, 
                                       apbh_dma_t* success)
{ 
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // must be locked to preserve atomic transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to maintain CS between parts of transmit chain
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + COL_SIZE));
	
	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->rx_data_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // remain locked here as well...
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	
	chain->rx_data_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)success);
	chain->rx_data_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 // unlock after read sequence to allow interleave
		 // between primitive nand operations
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_WRITE));
	chain->rx_data_dma.bar = buffer_paddr;
	chain->rx_data_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_XFER_COUNT(size));
	
	chain->tx_cle1 = FLASH_RANDOM_OUTPUT;
	SECMLC_offset_to_col(&chain->tx_col, offset);
	chain->tx_cle2 = FLASH_RANDOM_OUTPUT_SECOND;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}

//------------------------------------------------------------------------------
// 'Read Id' operation : <90> <00> <output> 

void  SECMLC_build_dma_read_id(SECMLC_dma_read_id_t* chain, 
                                   unsigned cs,
                                   void*  buffer_paddr,
                                   apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->rx_dma));
	chain->tx_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(SECMLC_READ_ID_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep locked between two halves of 'Read Id'
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_dma.bar = (void *)stmp_virt_to_phys((void *)&SECMLC_READ_ID_COMMAND);
	chain->tx_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // maintain CS between two halves of 'Read Id'
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(SECMLC_READ_ID_SIZE));
	
	chain->rx_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->rx_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(SECMLC_ID_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 // unlock after this descriptor to allow interleaving
		 // between primitive nand operation chains
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_WRITE));
	chain->rx_dma.bar = buffer_paddr; 
	chain->rx_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_XFER_COUNT(SECMLC_ID_SIZE));
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}



//------------------------------------------------------------------------------
// 'Reset Device' operation : <ff> <wait>

void  SECMLC_build_dma_reset_device(SECMLC_dma_reset_device_t* chain, 
                                        unsigned cs,
                                        apbh_dma_t* timeout,
                                        apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(SECMLC_RESET_DEVICE_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep locked until wait for ready is set up
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_dma.bar = (void *)stmp_virt_to_phys((void *)&SECMLC_RESET_DEVICE_COMMAND);
	chain->tx_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 //BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(SECMLC_RESET_DEVICE_SIZE));
	
	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 // unlock to allow interleaving between primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Serial Program' operation : <80> <addr> <data> <10> <wait> 

void  SECMLC_build_dma_serial_program(SECMLC_dma_serial_program_t* chain, 
                                          unsigned cs,
                                          reg32_t offset,
                                          reg16_t size,
                                          const void* data_buf_paddr,
                                          const void* aux_buf_paddr,
                                          apbh_dma_t* timeout,
                                          apbh_dma_t* success)
{
    reg16_t spare_per_sector = flash_spec->SpareSize >> BITS[flash_spec->SectorsPerPage];
    reg16_t aux_size = 0, total_size = 0;
    int use_two_tx_chain = 0;

    total_size = flash_spec->PageSize;
    if (spare_per_sector == 16) {
        // 4-bit ECC
        aux_size = total_size - ((512 + 9) << BITS[flash_spec->SectorsPerPage]) - 9;
        if (flash_spec->SectorsPerPage > 4) {
            use_two_tx_chain = 1;
            size >>= 1;
            total_size >>= 1;
            aux_size = ((aux_size + 9) >> 1) - 9;
        }
    }
    else {
        // 8-bit ECC
        aux_size = total_size - ((512 + 18) << BITS[flash_spec->SectorsPerPage]) - 9;
    }

#if 0
    printk("(size=%d, total_size=%d, aux_size=%d, tx_cle1_addr_dma=%08x, tx_data_dma=%08x, tx_aux_dma=%08x, tx_data2_dma=%08x, tx_aux2_dma=%08x, tx_cle2_dma=%08x, wait_dma=%08x, sense_dma=%08x, success=%08x)", 
            size, total_size, aux_size, &chain->tx_cle1_addr_dma, &chain->tx_data_dma, &chain->tx_aux_dma, &chain->tx_data2_dma, &chain->tx_aux2_dma, &chain->tx_cle2_dma, &chain->wait_dma, &chain->sense_dma, success);
#endif

	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_data_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep channel locked during transmit phase of primitive
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to keep chip select locked during transmit as well
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));
    chain->tx_cle1_addr_dma.gpmi_compare.U = 0x0;
    chain->tx_cle1_addr_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
	
	chain->tx_data_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_aux_dma));
	chain->tx_data_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
		 BF_APBH_CHn_CMD_CMDWORDS(4) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // channel remains locked
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_data_dma.bar = (void *)data_buf_paddr;
	chain->tx_data_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // chip select remains locked
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(size + aux_size));
    chain->tx_data_dma.gpmi_compare.U = 0x0;
    if (spare_per_sector == 16) {
        chain->tx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
    }
    else {
        chain->tx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_8_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
    }
    chain->tx_data_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);

    if (use_two_tx_chain) {		 
        chain->tx_aux_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_data2_dma));
    }
    else {
        chain->tx_aux_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
    }
	chain->tx_aux_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(aux_size) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // channel remains locked
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_aux_dma.bar = (void *)aux_buf_paddr;
	
	if (use_two_tx_chain) {
	    chain->tx_data2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_aux2_dma));
    	chain->tx_data2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
    		 BF_APBH_CHn_CMD_CMDWORDS(4) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // channel remains locked
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
    	chain->tx_data2_dma.bar = (void *)(data_buf_paddr + 512*4);
    	chain->tx_data2_dma.gpmi_ctrl0.U = 
    		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
    		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
    		 // chip select remains locked
    		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
    		 BF_GPMI_CTRL0_CS(cs) |
    		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
    		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
    		 BF_GPMI_CTRL0_XFER_COUNT(size + aux_size));
        chain->tx_data2_dma.gpmi_compare.U = 0x0;
        if (spare_per_sector == 16) {
            chain->tx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |
                                                BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
        }
        else {
            chain->tx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_8_BIT) |
                                                BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
        }
        chain->tx_data2_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);
    
        chain->tx_aux2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
    	chain->tx_aux2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(aux_size) |
    		 BF_APBH_CHn_CMD_CMDWORDS(0) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // channel remains locked
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
    	chain->tx_aux2_dma.bar = (void *)aux_buf_paddr;
	}
	
	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep locked until wait for ready set up
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
    chain->tx_cle2_dma.gpmi_compare.U = 0x0;
    chain->tx_cle2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);

	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock channel to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(0));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // unlock channel to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
	
	chain->tx_cle1 = FLASH_PAGE_PROGRAM;
	SECMLC_offset_to_address(&chain->tx_addr, offset);
	chain->tx_cle2 = FLASH_PAGE_PROGRAM_SECOND;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// '2-Plane or 4-Plane Page Program Phase-1' operation : <80> <addr> <data> <11> <wait for ready> 

void  SECMLC_build_dma_serial_program_group1(SECMLC_dma_serial_program_t* chain, 
                                          unsigned cs,
                                          reg32_t offset,
                                          reg16_t size,
                                          const void* data_buf_paddr,
                                          const void* aux_buf_paddr,
                                          apbh_dma_t* timeout,
                                          apbh_dma_t* success)
{
    reg16_t spare_per_sector = flash_spec->SpareSize >> BITS[flash_spec->SectorsPerPage];
    reg16_t aux_size = 0, total_size = 0;
    int use_two_tx_chain = 0;

    total_size = flash_spec->PageSize;
    if (spare_per_sector == 16) {
        // 4-bit ECC
        aux_size = total_size - ((512 + 9) << BITS[flash_spec->SectorsPerPage]) - 9;
        if (flash_spec->SectorsPerPage > 4) {
            use_two_tx_chain = 1;
            size >>= 1;
            total_size >>= 1;
            aux_size = ((aux_size + 9) >> 1) - 9;
        }
    }
    else {
        // 8-bit ECC
        aux_size = total_size - ((512 + 18) << BITS[flash_spec->SectorsPerPage]) - 9;
    }

#if 0
    printk("(size=%d, total_size=%d, aux_size=%d, tx_cle1_addr_dma=%08x, tx_data_dma=%08x, tx_aux_dma=%08x, tx_data2_dma=%08x, tx_aux2_dma=%08x, tx_cle2_dma=%08x, wait_dma=%08x, sense_dma=%08x, success=%08x)", 
            size, total_size, aux_size, &chain->tx_cle1_addr_dma, &chain->tx_data_dma, &chain->tx_aux_dma, &chain->tx_data2_dma, &chain->tx_aux2_dma, &chain->tx_cle2_dma, &chain->wait_dma, &chain->sense_dma, success);
#endif

	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_data_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep channel locked during transmit phase of primitive
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to keep chip select locked during transmit as well
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));
    chain->tx_cle1_addr_dma.gpmi_compare.U = 0x0;
    chain->tx_cle1_addr_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
	
	chain->tx_data_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_aux_dma));
	chain->tx_data_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
		 BF_APBH_CHn_CMD_CMDWORDS(4) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // channel remains locked
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_data_dma.bar = (void *)data_buf_paddr;
	chain->tx_data_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // chip select remains locked
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(size + aux_size));
    chain->tx_data_dma.gpmi_compare.U = 0x0;
    if (spare_per_sector == 16) {
        chain->tx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
    }
    else {
        chain->tx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_8_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
    }
    chain->tx_data_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);

    if (use_two_tx_chain) {		 
        chain->tx_aux_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_data2_dma));
    }
    else {
        chain->tx_aux_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
    }
	chain->tx_aux_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(aux_size) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // channel remains locked
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_aux_dma.bar = (void *)aux_buf_paddr;
	
	if (use_two_tx_chain) {
	    chain->tx_data2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_aux2_dma));
    	chain->tx_data2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
    		 BF_APBH_CHn_CMD_CMDWORDS(4) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // channel remains locked
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
    	chain->tx_data2_dma.bar = (void *)(data_buf_paddr + 512*4);
    	chain->tx_data2_dma.gpmi_ctrl0.U = 
    		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
    		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
    		 // chip select remains locked
    		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
    		 BF_GPMI_CTRL0_CS(cs) |
    		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
    		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
    		 BF_GPMI_CTRL0_XFER_COUNT(size + aux_size));
        chain->tx_data2_dma.gpmi_compare.U = 0x0;
        if (spare_per_sector == 16) {
            chain->tx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |
                                                BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
        }
        else {
            chain->tx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_8_BIT) |
                                                BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
        }
        chain->tx_data2_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);
    
        chain->tx_aux2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
    	chain->tx_aux2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(aux_size) |
    		 BF_APBH_CHn_CMD_CMDWORDS(0) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // channel remains locked
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
    	chain->tx_aux2_dma.bar = (void *)aux_buf_paddr;
	}
	
	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep locked until wait for ready set up
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
    chain->tx_cle2_dma.gpmi_compare.U = 0x0;
    chain->tx_cle2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);

	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock channel to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(0));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // unlock channel to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
	
	chain->tx_cle1 = FLASH_PAGE_PROGRAM_GROUP;
	SECMLC_offset_to_address(&chain->tx_addr, offset);
	chain->tx_cle2 = FLASH_PAGE_PROGRAM_GROUP_CLE2;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// '2-Plane or 4-Plane Page Program Phase-2' operation : <81> <addr> <data> <10> <wait> 

void  SECMLC_build_dma_serial_program_group2(SECMLC_dma_serial_program_t* chain, 
                                          unsigned cs,
                                          reg32_t offset,
                                          reg16_t size,
                                          const void* data_buf_paddr,
                                          const void* aux_buf_paddr,
                                          apbh_dma_t* timeout,
                                          apbh_dma_t* success)
{
    reg16_t spare_per_sector = flash_spec->SpareSize >> BITS[flash_spec->SectorsPerPage];
    reg16_t aux_size = 0, total_size = 0;
    int use_two_tx_chain = 0;

    total_size = flash_spec->PageSize;
    if (spare_per_sector == 16) {
        // 4-bit ECC
        aux_size = total_size - ((512 + 9) << BITS[flash_spec->SectorsPerPage]) - 9;
        if (flash_spec->SectorsPerPage > 4) {
            use_two_tx_chain = 1;
            size >>= 1;
            total_size >>= 1;
            aux_size = ((aux_size + 9) >> 1) - 9;
        }
    }
    else {
        // 8-bit ECC
        aux_size = total_size - ((512 + 18) << BITS[flash_spec->SectorsPerPage]) - 9;
    }

#if 0
    printk("(size=%d, total_size=%d, aux_size=%d, tx_cle1_addr_dma=%08x, tx_data_dma=%08x, tx_aux_dma=%08x, tx_data2_dma=%08x, tx_aux2_dma=%08x, tx_cle2_dma=%08x, wait_dma=%08x, sense_dma=%08x, success=%08x)", 
            size, total_size, aux_size, &chain->tx_cle1_addr_dma, &chain->tx_data_dma, &chain->tx_aux_dma, &chain->tx_data2_dma, &chain->tx_aux2_dma, &chain->tx_cle2_dma, &chain->wait_dma, &chain->sense_dma, success);
#endif

	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_data_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep channel locked during transmit phase of primitive
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to keep chip select locked during transmit as well
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));
    chain->tx_cle1_addr_dma.gpmi_compare.U = 0x0;
    chain->tx_cle1_addr_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
	
	chain->tx_data_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_aux_dma));
	chain->tx_data_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
		 BF_APBH_CHn_CMD_CMDWORDS(4) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // channel remains locked
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_data_dma.bar = (void *)data_buf_paddr;
	chain->tx_data_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // chip select remains locked
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(size + aux_size));
    chain->tx_data_dma.gpmi_compare.U = 0x0;
    if (spare_per_sector == 16) {
        chain->tx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
    }
    else {
        chain->tx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_8_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
    }
    chain->tx_data_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);

    if (use_two_tx_chain) {		 
        chain->tx_aux_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_data2_dma));
    }
    else {
        chain->tx_aux_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
    }
	chain->tx_aux_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(aux_size) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // channel remains locked
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_aux_dma.bar = (void *)aux_buf_paddr;
	
	if (use_two_tx_chain) {
	    chain->tx_data2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_aux2_dma));
    	chain->tx_data2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
    		 BF_APBH_CHn_CMD_CMDWORDS(4) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // channel remains locked
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
    	chain->tx_data2_dma.bar = (void *)(data_buf_paddr + 512*4);
    	chain->tx_data2_dma.gpmi_ctrl0.U = 
    		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
    		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
    		 // chip select remains locked
    		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
    		 BF_GPMI_CTRL0_CS(cs) |
    		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
    		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
    		 BF_GPMI_CTRL0_XFER_COUNT(size + aux_size));
        chain->tx_data2_dma.gpmi_compare.U = 0x0;
        if (spare_per_sector == 16) {
            chain->tx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |
                                                BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
        }
        else {
            chain->tx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_8_BIT) |
                                                BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
        }
        chain->tx_data2_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);
    
        chain->tx_aux2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
    	chain->tx_aux2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(aux_size) |
    		 BF_APBH_CHn_CMD_CMDWORDS(0) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // channel remains locked
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
    	chain->tx_aux2_dma.bar = (void *)aux_buf_paddr;
	}
	
	if (flash_spec->NumPlanes == 2) {
	    chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	}
	else {
	    chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)success);
	}
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep locked until wait for ready set up
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
    chain->tx_cle2_dma.gpmi_compare.U = 0x0;
    chain->tx_cle2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);

    if (flash_spec->NumPlanes == 2) {
    	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
    	chain->wait_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
    		 BF_APBH_CHn_CMD_CMDWORDS(1) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
    		 // unlock channel to allow interleaving during wait for ready
    		 BF_APBH_CHn_CMD_NANDLOCK(0) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
    	chain->wait_dma.bar = 0x0;
    	chain->wait_dma.gpmi_ctrl0.U = 
    		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
    		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
    		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
    		 BF_GPMI_CTRL0_CS(cs) |
    		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
    		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
    		 BF_GPMI_CTRL0_XFER_COUNT(0));
    	
    	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
    	chain->sense_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
    		 BF_APBH_CHn_CMD_CMDWORDS(0) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // unlock channel to allow interleaving between nand primitives
    		 BF_APBH_CHn_CMD_NANDLOCK(0) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
    	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
    }
	
	chain->tx_cle1 = FLASH_PAGE_PROGRAM_GROUP_CLE3;
	SECMLC_offset_to_address(&chain->tx_addr, offset);
	chain->tx_cle2 = FLASH_PAGE_PROGRAM_GROUP_CLE4;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// '4-Plane Page Program Phase-3' operation : <80> <addr> <data> <11> <wait for ready> 

void  SECMLC_build_dma_serial_program_group3(SECMLC_dma_serial_program_t* chain, 
                                          unsigned cs,
                                          reg32_t offset,
                                          reg16_t size,
                                          const void* data_buf_paddr,
                                          const void* aux_buf_paddr,
                                          apbh_dma_t* timeout,
                                          apbh_dma_t* success)
{
    reg16_t spare_per_sector = flash_spec->SpareSize >> BITS[flash_spec->SectorsPerPage];
    reg16_t aux_size = 0, total_size = 0;
    int use_two_tx_chain = 0;

    total_size = flash_spec->PageSize;
    if (spare_per_sector == 16) {
        // 4-bit ECC
        aux_size = total_size - ((512 + 9) << BITS[flash_spec->SectorsPerPage]) - 9;
        if (flash_spec->SectorsPerPage > 4) {
            use_two_tx_chain = 1;
            size >>= 1;
            total_size >>= 1;
            aux_size = ((aux_size + 9) >> 1) - 9;
        }
    }
    else {
        // 8-bit ECC
        aux_size = total_size - ((512 + 18) << BITS[flash_spec->SectorsPerPage]) - 9;
    }

#if 0
    printk("(size=%d, total_size=%d, aux_size=%d, tx_cle1_addr_dma=%08x, tx_data_dma=%08x, tx_aux_dma=%08x, tx_data2_dma=%08x, tx_aux2_dma=%08x, tx_cle2_dma=%08x, wait_dma=%08x, sense_dma=%08x, success=%08x)", 
            size, total_size, aux_size, &chain->tx_cle1_addr_dma, &chain->tx_data_dma, &chain->tx_aux_dma, &chain->tx_data2_dma, &chain->tx_aux2_dma, &chain->tx_cle2_dma, &chain->wait_dma, &chain->sense_dma, success);
#endif

	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_data_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep channel locked during transmit phase of primitive
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to keep chip select locked during transmit as well
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));
    chain->tx_cle1_addr_dma.gpmi_compare.U = 0x0;
    chain->tx_cle1_addr_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
	
	chain->tx_data_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_aux_dma));
	chain->tx_data_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
		 BF_APBH_CHn_CMD_CMDWORDS(4) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // channel remains locked
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_data_dma.bar = (void *)data_buf_paddr;
	chain->tx_data_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // chip select remains locked
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(size + aux_size));
    chain->tx_data_dma.gpmi_compare.U = 0x0;
    if (spare_per_sector == 16) {
        chain->tx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
    }
    else {
        chain->tx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_8_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
    }
    chain->tx_data_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);

    if (use_two_tx_chain) {		 
        chain->tx_aux_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_data2_dma));
    }
    else {
        chain->tx_aux_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
    }
	chain->tx_aux_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(aux_size) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // channel remains locked
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_aux_dma.bar = (void *)aux_buf_paddr;
	
	if (use_two_tx_chain) {
	    chain->tx_data2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_aux2_dma));
    	chain->tx_data2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
    		 BF_APBH_CHn_CMD_CMDWORDS(4) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // channel remains locked
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
    	chain->tx_data2_dma.bar = (void *)(data_buf_paddr + 512*4);
    	chain->tx_data2_dma.gpmi_ctrl0.U = 
    		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
    		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
    		 // chip select remains locked
    		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
    		 BF_GPMI_CTRL0_CS(cs) |
    		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
    		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
    		 BF_GPMI_CTRL0_XFER_COUNT(size + aux_size));
        chain->tx_data2_dma.gpmi_compare.U = 0x0;
        if (spare_per_sector == 16) {
            chain->tx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |
                                                BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
        }
        else {
            chain->tx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_8_BIT) |
                                                BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
        }
        chain->tx_data2_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);
    
        chain->tx_aux2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
    	chain->tx_aux2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(aux_size) |
    		 BF_APBH_CHn_CMD_CMDWORDS(0) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // channel remains locked
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
    	chain->tx_aux2_dma.bar = (void *)aux_buf_paddr;
	}
	
	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)success);
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep locked until wait for ready set up
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
    chain->tx_cle2_dma.gpmi_compare.U = 0x0;
    chain->tx_cle2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
	
	chain->tx_cle1 = FLASH_PAGE_PROGRAM_GROUP;
	SECMLC_offset_to_address(&chain->tx_addr, offset);
	chain->tx_cle2 = FLASH_PAGE_PROGRAM_GROUP_CLE2;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// '4-Plane Page Program Phase-4' operation : <81> <addr> <data> <10> <wait> 

void  SECMLC_build_dma_serial_program_group4(SECMLC_dma_serial_program_t* chain, 
                                          unsigned cs,
                                          reg32_t offset,
                                          reg16_t size,
                                          const void* data_buf_paddr,
                                          const void* aux_buf_paddr,
                                          apbh_dma_t* timeout,
                                          apbh_dma_t* success)
{
    reg16_t spare_per_sector = flash_spec->SpareSize >> BITS[flash_spec->SectorsPerPage];
    reg16_t aux_size = 0, total_size = 0;
    int use_two_tx_chain = 0;

    total_size = flash_spec->PageSize;
    if (spare_per_sector == 16) {
        // 4-bit ECC
        aux_size = total_size - ((512 + 9) << BITS[flash_spec->SectorsPerPage]) - 9;
        if (flash_spec->SectorsPerPage > 4) {
            use_two_tx_chain = 1;
            size >>= 1;
            total_size >>= 1;
            aux_size = ((aux_size + 9) >> 1) - 9;
        }
    }
    else {
        // 8-bit ECC
        aux_size = total_size - ((512 + 18) << BITS[flash_spec->SectorsPerPage]) - 9;
    }

#if 0
    printk("(size=%d, total_size=%d, aux_size=%d, tx_cle1_addr_dma=%08x, tx_data_dma=%08x, tx_aux_dma=%08x, tx_data2_dma=%08x, tx_aux2_dma=%08x, tx_cle2_dma=%08x, wait_dma=%08x, sense_dma=%08x, success=%08x)", 
            size, total_size, aux_size, &chain->tx_cle1_addr_dma, &chain->tx_data_dma, &chain->tx_aux_dma, &chain->tx_data2_dma, &chain->tx_aux2_dma, &chain->tx_cle2_dma, &chain->wait_dma, &chain->sense_dma, success);
#endif

	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_data_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep channel locked during transmit phase of primitive
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to keep chip select locked during transmit as well
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));
    chain->tx_cle1_addr_dma.gpmi_compare.U = 0x0;
    chain->tx_cle1_addr_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
	
	chain->tx_data_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_aux_dma));
	chain->tx_data_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
		 BF_APBH_CHn_CMD_CMDWORDS(4) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // channel remains locked
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_data_dma.bar = (void *)data_buf_paddr;
	chain->tx_data_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // chip select remains locked
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(size + aux_size));
    chain->tx_data_dma.gpmi_compare.U = 0x0;
    if (spare_per_sector == 16) {
        chain->tx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
    }
    else {
        chain->tx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_8_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
    }
    chain->tx_data_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);

    if (use_two_tx_chain) {		 
        chain->tx_aux_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_data2_dma));
    }
    else {
        chain->tx_aux_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
    }
	chain->tx_aux_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(aux_size) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // channel remains locked
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_aux_dma.bar = (void *)aux_buf_paddr;
	
	if (use_two_tx_chain) {
	    chain->tx_data2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_aux2_dma));
    	chain->tx_data2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
    		 BF_APBH_CHn_CMD_CMDWORDS(4) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // channel remains locked
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
    	chain->tx_data2_dma.bar = (void *)(data_buf_paddr + 512*4);
    	chain->tx_data2_dma.gpmi_ctrl0.U = 
    		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
    		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
    		 // chip select remains locked
    		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
    		 BF_GPMI_CTRL0_CS(cs) |
    		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
    		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
    		 BF_GPMI_CTRL0_XFER_COUNT(size + aux_size));
        chain->tx_data2_dma.gpmi_compare.U = 0x0;
        if (spare_per_sector == 16) {
            chain->tx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |
                                                BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
        }
        else {
            chain->tx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_8_BIT) |
                                                BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
        }
        chain->tx_data2_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);
    
        chain->tx_aux2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
    	chain->tx_aux2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(aux_size) |
    		 BF_APBH_CHn_CMD_CMDWORDS(0) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // channel remains locked
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
    	chain->tx_aux2_dma.bar = (void *)aux_buf_paddr;
	}
	
	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep locked until wait for ready set up
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
    chain->tx_cle2_dma.gpmi_compare.U = 0x0;
    chain->tx_cle2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);

	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock channel to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(0));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // unlock channel to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
	
	chain->tx_cle1 = FLASH_PAGE_PROGRAM_GROUP_CLE3;
	SECMLC_offset_to_address(&chain->tx_addr, offset);
	chain->tx_cle2 = FLASH_PAGE_PROGRAM_GROUP_CLE4;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Copy-back Read' operation : <00> <addr> <35> <wait tR> <data>

void  SECMLC_build_dma_copyback_read(SECMLC_dma_copyback_read_t* chain, 
                                       unsigned cs,
                                       reg32_t offset,
                                       reg16_t size,
                                       void* data_buf_paddr,
                                       void* aux_buf_paddr,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success)
{
    reg16_t spare_per_sector = flash_spec->SpareSize >> BITS[flash_spec->SectorsPerPage];
    reg16_t total_size = 0;
    int use_two_rx_chain = 0;
    
    total_size = flash_spec->PageSize;
    if (spare_per_sector == 16) {
        // 4-bit ECC
        if (flash_spec->SectorsPerPage > 4) {
            use_two_rx_chain = 1;
            total_size >>= 1;
        }
    }
    else {
        // 8-bit ECC
    }

#if 0
    printk("(size=%d, total_size=%d, aux_size=%d, spare_per_sector=%d, tx_cle1_addr_dma=%08x, tx_cle2_dma=%08x, wait_dma=%08x, sense_dma=%08x, rx_data_dma=%08x, rx_data2_dma=%08x, disable_ecc_dma=%08x, success=%08x)", 
            size, total_size, aux_size, spare_per_sector, &chain->tx_cle1_addr_dma, &chain->tx_cle2_dma, &chain->wait_dma, &chain->sense_dma, &chain->rx_data_dma, &chain->rx_data2_dma, &chain->disable_ecc_dma, success);
#endif

	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // must be locked to preserve atomic transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to maintain CS between parts of transmit chain
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));
    chain->tx_cle1_addr_dma.gpmi_compare.U = 0x0;
    chain->tx_cle1_addr_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
	
	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // remain locked here as well...
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	
	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
	    (BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock to allow interleave during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(0));

	chain->sense_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->rx_data_dma));
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // unlock to allow interleave after this descriptor
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
	
	if (use_two_rx_chain) {
	    chain->rx_data_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)&(chain->rx_data2_dma));
	}
	else {
	    chain->rx_data_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)&(chain->disable_ecc_dma));
	}
	chain->rx_data_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(6) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // unlock after read sequence to allow interleave
		 // between primitive nand operations
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->rx_data_dma.bar = 0x0;
	chain->rx_data_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(total_size));
    chain->rx_data_dma.gpmi_compare.U = 0x0;
    if (spare_per_sector == 16) {
        chain->rx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, DECODE_4_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
    }
    else {
        chain->rx_data_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, DECODE_8_BIT) |
                                            BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                            BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
    }
    chain->rx_data_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);
    chain->rx_data_dma.gpmi_data_ptr.U = (reg32_t)data_buf_paddr;
    chain->rx_data_dma.gpmi_aux_ptr.U = (reg32_t)aux_buf_paddr;
    
    if (use_two_rx_chain) {
        chain->rx_data2_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)&(chain->disable_ecc_dma));
    	chain->rx_data2_dma.cmd.U = 
    		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
    		 BF_APBH_CHn_CMD_CMDWORDS(6) |
    		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
    		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
    		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
    		 // unlock after read sequence to allow interleave
    		 // between primitive nand operations
    		 BF_APBH_CHn_CMD_NANDLOCK(1) |
    		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
    		 BF_APBH_CHn_CMD_CHAIN(1) |
    		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
    	chain->rx_data2_dma.bar = 0x0;
    	chain->rx_data2_dma.gpmi_ctrl0.U = 
    		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ) |
    		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
    		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
    		 BF_GPMI_CTRL0_CS(cs) |
    		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
    		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
    		 BF_GPMI_CTRL0_XFER_COUNT(total_size));
        chain->rx_data2_dma.gpmi_compare.U = 0x0;
        if (spare_per_sector == 16) {
            chain->rx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, DECODE_4_BIT) |
                                                 BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                 BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);
        }
        else {
            chain->rx_data2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ECC_CMD, DECODE_8_BIT) |
                                                 BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE) |
                                                 BF_GPMI_ECCCTRL_BUFFER_MASK(0x1FF);
        }
        chain->rx_data2_dma.gpmi_ecccount.U = BF_GPMI_ECCCOUNT_COUNT(total_size);
        chain->rx_data2_dma.gpmi_data_ptr.U = (reg32_t)(data_buf_paddr + 512*4);
        chain->rx_data2_dma.gpmi_aux_ptr.U = (reg32_t)aux_buf_paddr;
    }
		 
    chain->disable_ecc_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)success);
	chain->disable_ecc_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock after read sequence to allow interleave
		 // between primitive nand operations
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->disable_ecc_dma.bar = 0x0;
	chain->disable_ecc_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, /*READ*/WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(0));
    chain->disable_ecc_dma.gpmi_compare.U = 0x0;
    chain->disable_ecc_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
    
	chain->tx_cle1 = FLASH_READ2;
	SECMLC_offset_to_address(&chain->tx_addr, offset);
	chain->tx_cle2 = FLASH_READ2_SECOND;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Copy-back Read Group' operation : <60> <row> <60> <row> <35> <wait tR>
//                                    <00> <addr> <05> <col> <e0> <data>
//                                    <00> <addr> <05> <col> <e0> <data>

// First phase : <60> <row> <60> <row> <35> <wait tR>

void  SECMLC_build_dma_copyback_read_group1(SECMLC_dma_copyback_read_group1_t* chain, 
                                       unsigned cs,
                                       unsigned* offset,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_row1_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle1_row2_dma));
	chain->tx_cle1_row1_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // must be locked to preserve atomic transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_row1_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_row1_buf);
	chain->tx_cle1_row1_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to maintain CS between parts of transmit chain
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE));

    chain->tx_cle1_row2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_row2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // must be locked to preserve atomic transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_row2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_row2_buf);
	chain->tx_cle1_row2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to maintain CS between parts of transmit chain
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE));

	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // remain locked here as well...
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));

	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock to allow interleave during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs));

	chain->sense_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 // unlock to allow interleave after this descriptor
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);

/*
	chain->sense_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |		 
		 // unlock to allow interleave after this descriptor
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
*/
	
	chain->tx_cle11 = FLASH_READ_GROUP2;
#if !USE_16GBIT_NAND
	SECMLC_offset_to_row(&chain->tx_row1, offset[0] & (0x10010000 << 4));
#else
    SECMLC_offset_to_row(&chain->tx_row1, offset[0] & 0x10010000);
#endif
	chain->tx_cle12 = FLASH_READ_GROUP2_CLE2;
	SECMLC_offset_to_row(&chain->tx_row2, offset[1]);
	chain->tx_cle2 = FLASH_READ_GROUP2_CLE3;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


// Second phase : <00> <addr> <05> <col> <e0> <data>

void  SECMLC_build_dma_copyback_read_group2(SECMLC_dma_copyback_read_group2_t* chain, 
                                       unsigned cs,
                                       reg32_t offset,
                                       reg16_t size,
                                       void* buffer_paddr,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle1_col_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // must be locked to preserve atomic transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to maintain CS between parts of transmit chain
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));

    chain->tx_cle1_col_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_col_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // must be locked to preserve atomic transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_col_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_col_buf);
	chain->tx_cle1_col_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to maintain CS between parts of transmit chain
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + COL_SIZE));

	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->rx_data_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // remain locked here as well...
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));

    chain->rx_data_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)success);
    //chain->rx_data_dma.nxt = (apbh_dma_t*)stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->rx_data_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(size) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 // unlock to allow interleave after this descriptor
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_WRITE));
	chain->rx_data_dma.bar = buffer_paddr;
	chain->rx_data_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_XFER_COUNT(size));

/*
	chain->sense_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |		 
		 // unlock to allow interleave after this descriptor
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
*/
	
	chain->tx_cle11 = FLASH_READ2;
#if !USE_16GBIT_NAND
	SECMLC_offset_to_address(&chain->tx_addr, offset & (0x10010000 << 4));
#else
    SECMLC_offset_to_address(&chain->tx_addr, offset & 0x10010000);
#endif
	chain->tx_cle12 = FLASH_RANDOM_OUTPUT;
	SECMLC_offset_to_col(&chain->tx_col, offset);
	chain->tx_cle2 = FLASH_RANDOM_OUTPUT_SECOND;

	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Copy-back Program' operation : <85> <addr> <10> <wait tPROG> 

void  SECMLC_build_dma_copyback_program(SECMLC_dma_copyback_program_t* chain, 
					    unsigned cs,
					    reg32_t offset,
					    apbh_dma_t* timeout,
					    apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep channel locked during transmit phase of primitive
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to keep chip select locked during transmit as well
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));
    chain->tx_cle1_addr_dma.gpmi_compare.U = 0x0;
    chain->tx_cle1_addr_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);

	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep locked until wait for ready set up
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
    chain->tx_cle2_dma.gpmi_compare.U = 0x0;
    chain->tx_cle2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);

	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock channel to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(0));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // unlock channel to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);

	chain->tx_cle1 = FLASH_COPY_BACK;
	SECMLC_offset_to_address(&chain->tx_addr, offset);
	chain->tx_cle2 = FLASH_COPY_BACK_SECOND;

	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// '2-Plane Copy-back Program Phase-1' operation : <85> <addr> <11> <wait tDBSY> 

void  SECMLC_build_dma_copyback_program_group1(SECMLC_dma_copyback_program_t* chain, 
					    unsigned cs,
					    reg32_t offset,
					    apbh_dma_t* timeout,
					    apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep channel locked during transmit phase of primitive
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to keep chip select locked during transmit as well
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));
    chain->tx_cle1_addr_dma.gpmi_compare.U = 0x0;
    chain->tx_cle1_addr_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);

	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep locked until wait for ready set up
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
    chain->tx_cle2_dma.gpmi_compare.U = 0x0;
    chain->tx_cle2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);

	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock channel to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(0));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // unlock channel to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);

	chain->tx_cle1 = FLASH_COPY_BACK_GROUP;
#if !USE_16GBIT_NAND
	SECMLC_offset_to_address(&chain->tx_addr, offset & (0x10010000 << 4));
#else
    SECMLC_offset_to_address(&chain->tx_addr, offset & 0x10010000);
#endif
	chain->tx_cle2 = FLASH_COPY_BACK_GROUP_CLE2;

	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// '2-Plane Copy-back Program Phase-2' operation : <81> <addr> <10> <wait tPROG> 

void  SECMLC_build_dma_copyback_program_group2(SECMLC_dma_copyback_program_t* chain, 
					    unsigned cs,
					    reg32_t offset,
					    apbh_dma_t* timeout,
					    apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_addr_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_addr_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE + COL_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep channel locked during transmit phase of primitive
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_addr_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_addr_buf);
	chain->tx_cle1_addr_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // need to keep chip select locked during transmit as well
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE + COL_SIZE));
    chain->tx_cle1_addr_dma.gpmi_compare.U = 0x0;
    chain->tx_cle1_addr_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);

	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // keep locked until wait for ready set up
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
    chain->tx_cle2_dma.gpmi_compare.U = 0x0;
    chain->tx_cle2_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);

	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock channel to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(0) |
		 BF_GPMI_CTRL0_XFER_COUNT(0));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(0) |
		 BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(0) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(0) |
		 // unlock channel to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_IRQONCMPLT(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
	
	chain->tx_cle1 = FLASH_COPY_BACK_GROUP_CLE3;
#if !USE_16GBIT_NAND
	SECMLC_offset_to_address(&chain->tx_addr, offset & (0x1FFFFE00 << 4));
#else
    SECMLC_offset_to_address(&chain->tx_addr, offset & 0x1FFFFE00);
#endif
	chain->tx_cle2 = FLASH_COPY_BACK_GROUP_CLE4;

	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Block Erase' operation : <60> <row> <d0> <wait tBERS>

void  SECMLC_build_dma_block_erase(SECMLC_dma_block_erase_t* chain, 
                                       unsigned cs,
                                       unsigned offset,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_row_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_row_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep channel locked during transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_row_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_row_buf);
	chain->tx_cle1_row_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked during transmit 
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE));
	
	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // channel must remain locked until wait for ready setup
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	
	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock channel to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 // unlock channel to allow interleaving between primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
	
	chain->tx_cle1 = FLASH_BLOCK_ERASE;
	SECMLC_offset_to_row(&chain->tx_row, offset);
	chain->tx_cle2 = FLASH_BLOCK_ERASE_SECOND;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Two-Plane Block Erase' operation : <60> <row> <60> <row> <d0> <wait tBERS>

void  SECMLC_build_dma_block_erase_group(SECMLC_dma_block_erase_group_t* chain, 
                                       unsigned cs,
                                       unsigned* offset,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_row1_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle1_row2_dma));
	chain->tx_cle1_row1_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep channel locked during transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_row1_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_row1_buf);
	chain->tx_cle1_row1_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked during transmit 
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE));

	chain->tx_cle1_row2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_row2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep channel locked during transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_row2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_row2_buf);
	chain->tx_cle1_row2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked during transmit 
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE));

	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // channel must remain locked until wait for ready setup
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	
	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock channel to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 // unlock channel to allow interleaving between primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
	
	chain->tx_cle11 = FLASH_BLOCK_ERASE_CLE1;
	SECMLC_offset_to_row(&chain->tx_row1, offset[0]);
	chain->tx_cle12 = FLASH_BLOCK_ERASE_CLE1;
	SECMLC_offset_to_row(&chain->tx_row2, offset[1]);
	chain->tx_cle2 = FLASH_BLOCK_ERASE_CLE2;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// '4-Plane Block Erase1' operation : <60> <row> <60> <row> <d0>

void  SECMLC_build_dma_block_erase_group1(SECMLC_dma_block_erase_group_t* chain, 
                                       unsigned cs,
                                       unsigned* offset,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_row1_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle1_row2_dma));
	chain->tx_cle1_row1_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep channel locked during transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_row1_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_row1_buf);
	chain->tx_cle1_row1_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked during transmit 
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE));

	chain->tx_cle1_row2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_row2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep channel locked during transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_row2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_row2_buf);
	chain->tx_cle1_row2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked during transmit 
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE));

	chain->tx_cle2_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // channel must remain locked until wait for ready setup
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
/*	
	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock channel to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 // unlock channel to allow interleaving between primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
*/	
	chain->tx_cle11 = FLASH_BLOCK_ERASE_CLE1;
	SECMLC_offset_to_row(&chain->tx_row1, offset[0]);
	chain->tx_cle12 = FLASH_BLOCK_ERASE_CLE1;
	SECMLC_offset_to_row(&chain->tx_row2, offset[1]);
	chain->tx_cle2 = FLASH_BLOCK_ERASE_CLE2;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// '4-Plane Block Erase2' operation : <60> <row> <60> <row> <d0> <wait tBERS>

void  SECMLC_build_dma_block_erase_group2(SECMLC_dma_block_erase_group_t* chain, 
                                       unsigned cs,
                                       unsigned* offset,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_cle1_row1_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle1_row2_dma));
	chain->tx_cle1_row1_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep channel locked during transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_row1_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_row1_buf);
	chain->tx_cle1_row1_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked during transmit 
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE));

	chain->tx_cle1_row2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->tx_cle2_dma));
	chain->tx_cle1_row2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1 + ROW_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep channel locked during transmit sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle1_row2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle1_row2_buf);
	chain->tx_cle1_row2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked during transmit 
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_ADDRESS_INCREMENT(1) |
		 BF_GPMI_CTRL0_XFER_COUNT(1 + ROW_SIZE));

	chain->tx_cle2_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->wait_dma));
	chain->tx_cle2_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(1) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // channel must remain locked until wait for ready setup
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_cle2_dma.bar = (void *)stmp_virt_to_phys((void *)chain->tx_cle2_buf);
	chain->tx_cle2_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	
	chain->wait_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->wait_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_NANDWAIT4READY(1) |
		 // unlock channel to allow interleaving during wait for ready
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->wait_dma.bar = 0x0;
	chain->wait_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(0) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 // unlock channel to allow interleaving between primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)timeout);
	
	chain->tx_cle11 = FLASH_BLOCK_ERASE_CLE1;
	SECMLC_offset_to_row(&chain->tx_row1, offset[0]);
	chain->tx_cle12 = FLASH_BLOCK_ERASE_CLE1;
	SECMLC_offset_to_row(&chain->tx_row2, offset[1]);
	chain->tx_cle2 = FLASH_BLOCK_ERASE_CLE2;
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Read Status' operation : <70> <out> 

void  SECMLC_build_dma_read_status(SECMLC_dma_read_status_t* chain, 
                                       unsigned cs,
                                       void* buffer_paddr,
                                       apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->rx_dma));
	chain->tx_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(SECMLC_READ_STATUS_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep channel locked during read status sequence
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_dma.bar = (void *)stmp_virt_to_phys((void *)&SECMLC_READ_STATUS_COMMAND);
	chain->tx_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked between transmit and receive
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(SECMLC_READ_STATUS_SIZE));
	
	chain->rx_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->rx_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(SECMLC_STATUS_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 // unlock channel to allow interleave between primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_WRITE));
	chain->rx_dma.bar = (void *)buffer_paddr;
	chain->rx_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_XFER_COUNT(SECMLC_STATUS_SIZE));
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Check Status' operation


void  SECMLC_build_dma_check_status(SECMLC_dma_check_status_t* chain, 
                                        unsigned cs,
                                        reg16_t mask,
                                        reg16_t match,
                                        apbh_dma_t* failure,
                                        apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->cmp_dma));
	chain->tx_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(SECMLC_READ_STATUS_SIZE) |
		 //BF_APBH_CHn_CMD_CMDWORDS(3) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep locked between transmit and read&compare
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) | 
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_dma.bar = (void *)stmp_virt_to_phys((void *)&SECMLC_READ_STATUS_COMMAND);
	chain->tx_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked between transmit and read&compare
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(SECMLC_READ_STATUS_SIZE));
    //chain->tx_dma.gpmi_compare.U = 0x0;
    //chain->tx_dma.gpmi_eccctrl.U = BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);
	
	chain->cmp_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->cmp_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(2) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 // unlock to allow interleaving before sense
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->cmp_dma.bar = 0x0;
	chain->cmp_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ_AND_COMPARE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	chain->cmp_dma.gpmi_compare.U = 
		(BF_GPMI_COMPARE_MASK(mask) |
		 BF_GPMI_COMPARE_REFERENCE(match));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 // unlock to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)failure);
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Check Status for Chip-1' operation


void  SECMLC_build_dma_check_status1(SECMLC_dma_check_status_t* chain, 
                                         unsigned cs,
                                         reg16_t mask,
                                         reg16_t match,
                                         apbh_dma_t* failure,
                                         apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->cmp_dma));
	chain->tx_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(SECMLC_CHIP1_STATUS_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep locked between transmit and read&compare
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) | 
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_dma.bar = (void *)stmp_virt_to_phys((void *)&SECMLC_CHIP1_STATUS_COMMAND);
	chain->tx_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked between transmit and read&compare
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(SECMLC_CHIP1_STATUS_SIZE));
	
	chain->cmp_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->cmp_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(2) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 // unlock to allow interleaving before sense
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->cmp_dma.bar = 0x0;
	chain->cmp_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ_AND_COMPARE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	chain->cmp_dma.gpmi_compare.U = 
		(BF_GPMI_COMPARE_MASK(mask) |
		 BF_GPMI_COMPARE_REFERENCE(match));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 // unlock to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)failure);
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Check Status for Chip-2' operation


void  SECMLC_build_dma_check_status2(SECMLC_dma_check_status_t* chain, 
                                         unsigned cs,
                                         reg16_t mask,
                                         reg16_t match,
                                         apbh_dma_t* failure,
                                         apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->cmp_dma));
	chain->tx_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(SECMLC_CHIP2_STATUS_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep locked between transmit and read&compare
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) | 
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_dma.bar = (void *)stmp_virt_to_phys((void *)&SECMLC_CHIP2_STATUS_COMMAND);
	chain->tx_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked between transmit and read&compare
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(SECMLC_CHIP2_STATUS_SIZE));
	
	chain->cmp_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->cmp_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(2) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 // unlock to allow interleaving before sense
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->cmp_dma.bar = 0x0;
	chain->cmp_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ_AND_COMPARE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	chain->cmp_dma.gpmi_compare.U = 
		(BF_GPMI_COMPARE_MASK(mask) |
		 BF_GPMI_COMPARE_REFERENCE(match));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_SEMAPHORE(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 // unlock to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)failure);
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Check DBSY for Chip-1' operation


void  SECMLC_build_dma_check_dbsy1(SECMLC_dma_check_status_t* chain, 
                                       unsigned cs,
                                       reg16_t mask,
                                       reg16_t match,
                                       apbh_dma_t* failure,
                                       apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->cmp_dma));
	chain->tx_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(SECMLC_CHIP1_STATUS_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep locked between transmit and read&compare
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) | 
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_dma.bar = (void *)stmp_virt_to_phys((void *)&SECMLC_CHIP1_STATUS_COMMAND);
	chain->tx_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked between transmit and read&compare
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(SECMLC_CHIP1_STATUS_SIZE));
	
	chain->cmp_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->cmp_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(2) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 // unlock to allow interleaving before sense
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->cmp_dma.bar = 0x0;
	chain->cmp_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ_AND_COMPARE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	chain->cmp_dma.gpmi_compare.U = 
		(BF_GPMI_COMPARE_MASK(mask) |
		 BF_GPMI_COMPARE_REFERENCE(match));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CHAIN(1) |
		 // unlock to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)failure);
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


//------------------------------------------------------------------------------
// 'Check DBSY for Chip-2' operation


void  SECMLC_build_dma_check_dbsy2(SECMLC_dma_check_status_t* chain, 
                                       unsigned cs,
                                       reg16_t mask,
                                       reg16_t match,
                                       apbh_dma_t* failure,
                                       apbh_dma_t* success)
{
	// cs filtering for effective chip selection via bank selection
	// cs &= 0x01; // (cs = cs % 2)

	chain->tx_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->cmp_dma));
	chain->tx_dma.cmd.U = 
		(BF_APBH_CHn_CMD_XFER_COUNT(SECMLC_CHIP2_STATUS_SIZE) |
		 BF_APBH_CHn_CMD_CMDWORDS(1) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 // keep locked between transmit and read&compare
		 BF_APBH_CHn_CMD_NANDLOCK(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) | 
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ));
	chain->tx_dma.bar = (void *)stmp_virt_to_phys((void *)&SECMLC_CHIP2_STATUS_COMMAND);
	chain->tx_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 // keep chip select locked between transmit and read&compare
		 BF_GPMI_CTRL0_LOCK_CS(1) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE) |
		 BF_GPMI_CTRL0_XFER_COUNT(SECMLC_CHIP2_STATUS_SIZE));
	
	chain->cmp_dma.nxt = (apbh_dma_t*) stmp_virt_to_phys((void *)&(chain->sense_dma));
	chain->cmp_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CMDWORDS(2) |
		 BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
		 BF_APBH_CHn_CMD_CHAIN(1) |
		 // unlock to allow interleaving before sense
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER));
	chain->cmp_dma.bar = 0x0;
	chain->cmp_dma.gpmi_ctrl0.U = 
		(BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ_AND_COMPARE) |
		 BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT) |
		 BF_GPMI_CTRL0_CS(cs) |
		 BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA) |
		 BF_GPMI_CTRL0_XFER_COUNT(1));
	chain->cmp_dma.gpmi_compare.U = 
		(BF_GPMI_COMPARE_MASK(mask) |
		 BF_GPMI_COMPARE_REFERENCE(match));
	
	chain->sense_dma.nxt = (apbh_dma_t *)stmp_virt_to_phys((void *)success);
	chain->sense_dma.cmd.U = 
		(BF_APBH_CHn_CMD_CHAIN(1) |
		 // unlock to allow interleaving between nand primitives
		 BF_APBH_CHn_CMD_NANDLOCK(0) |
		 BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE));
	chain->sense_dma.bar = (void *)stmp_virt_to_phys((void *)failure);
	
	rfs_clean_dcache_range((unsigned int)chain, (unsigned int)chain + sizeof(*chain));
}


////////////////////////////////////////////////////////////////////////////////
//
// $Log: SECMLC_dma.c,v $
// Revision 1.5  2007/09/12 03:41:19  hcyun
//
// zeen-rfs-070911 ???? ??? ????.
//
// - two-plane copyback read ??? unrecoverable ECC ??? ?? ???
//   ??? ?? ?? ?? ???, ?? 2?? single-plane copyback read
//   ???? ??? ???????.
//
// - RFS FAT ??? ?? ??? ???? ??? ??? ??? ???
//   ???????. (??? FAT sector read? ??? ?? ?? ???
//   ????.)
//
// - hcyun
//
// Revision 1.4  2007/09/10 12:06:00  hcyun
// copyback operation added (but not used)
//
// - hcyun
//
// Revision 1.3  2007/08/16 07:49:10  hcyun
// fix mistake..
//
// - hcyun
//
// Revision 1.1  2007/07/26 09:40:23  hcyun
// DDP v0.1
//
// Revision 1.1  2006/07/18 08:58:16  hcyun
// 8GB support (k9hbg08u1m,...)
// simultaneous support for all possible nand chip including SLC(1/2/4GB), MLC(4Gb 1/2/4GB), MLC(8Gb 1GB), MLC(8Gb 2/4/8GB)
//
// Revision 1.1  2006/05/02 11:05:00  hcyun
// from main z-proj source
//
// Revision 1.1  2006/03/27 13:17:08  hcyun
// SECMLC (MLC) support.
//
// Revision 1.9  2005/11/03 02:30:40  hcyun
// support SRAM descriptor (require SRAM map change)
// support SRAM ECC Buffer (require SRAM map change)
// support ECC Pipelining (LLD level only), about 20% read performance improvement when accessing over 4KB base
// minimize wait timing (T_OVERHEAD = 10)
//
// - hcyun
//
// Revision 1.8  2005/10/23 01:43:47  hcyun
// make address, row, col address translation more efficiently
//
// - hcyun
//
// Revision 1.7  2005/09/15 00:26:14  hcyun
// reset handling
// missing irq handling improved..
//
// - hcyun
//
// Revision 1.6  2005/08/20 00:58:10  biglow
// - update rfs which is worked fib fixed chip only.
//
// Revision 1.5  2005/07/21 05:44:15  hcyun
// merged yhbae's addition - mainly test code, plus bank selection code (currently it has a bug)
//
// 2-plane operation is not enabled in the flash_spec structure.
//
// - hcyun
//
// Revision 1.4  2005/07/05 10:36:41  hcyun
// ufd 2.0: 1 package 2GB support..
//
// - hcyun
//
// Revision 1.3  2005/06/29 02:38:01  hcyun
// bug fix on dcache_clean_area
//
// - hcyun
//
// Revision 1.2  2005/06/28 07:29:58  hcyun
// conservative timing.
// add comments.
//
// Revision 1.1  2005/06/27 07:47:17  hcyun
// first working (kernel panic due to sdram corruption??)
//
////////////////////////////////////////////////////////////////////////////////
//
// Old Log:

// Revision 1.9  2005/06/08 11:31:17  hcyun
// Critical bug fix of serial ri program: add IRQONCOMPLETE on the dma chain. Now, it works exactly at expected speed..
//
// - hcyun
//
// Revision 1.8  2005/06/06 15:32:33  hcyun
// removed support of Random input of copy_back program..
//
// - hcyun
//
// Revision 1.7  2005/06/02 19:12:00  hcyun
// flush -> clean for DMA descriptors
//
// - hcyun
//
// Revision 1.6  2005/05/27 21:11:00  hcyun
// flush/invalidate_dcache_range input parameter fixed.
//
// ECC512 code require kernel mode alignment trap handler
//
// - hcyun
//
// Revision 1.5  2005/05/26 20:57:17  hcyun
// ..
//
// - hcyun
//
// Revision 1.4  2005/05/15 22:34:21  hcyun
// remove warnings.
// address translation based on area definition (AREA_A, AREA_C)
// - hcyun
//
// Revision 1.3  2005/05/15 20:52:23  hcyun
// area type (main array, spare arry) define added.
// fixed bug. Now all the newly added commands (random read, program with random input, copy-back program, cache-program) seems to work correctly
//
// - hcyun
//
// Revision 1.5  2005/05/15 05:10:29  hcyun
// copy-back, cache-program, random-input, random-output added. and removed unnecessary functions & types. Now, it's sync with ufd's dma functions..
//
// - hcyun
//
// Revision 1.1  2005/05/14 07:07:56  hcyun
// copy-back, cache-program, random-input, random-output added.
// fm_driver for stmp36xx added
//
// Revision 1.4  2005/05/11 04:11:02  hcyun
// - cache flush added for serial program
// - removed entire cache flush code of runtest.c
// - check status is added..
//
// Revision 1.3  2005/05/08 01:49:13  hcyun
// - First working NAND flash low level API set.
//   Read-Id, Read, Write, Erase seems to working..
//
// Revision 1.2  2005/05/06 21:31:01  hcyun
// - DMA Read ID is working...
// - descriptors : kmalloc/kfree with flush_cache_range()
// - buffers : consistent_alloc/free
//
// TODO: fixing other dma functions..
//
// Revision 1.1  2005/05/05 01:05:37  hcyun
// - Started using stmp_virt_to_phys.. not yet complete..
//
// Revision 1.14  2005/01/18 19:56:15  ttoelkes
// added READ_STEP; fixed build warnings
//
// Revision 1.13  2005/01/18 19:51:38  ttoelkes
// 'erase_read_program_read' test builds properly
//
// Revision 1.12  2004/11/11 15:53:27  ttoelkes
// cross your fingers; here are my updated dma chains
//
// Revision 1.11  2004/11/10 15:30:11  ttoelkes
// updated chains for new dma state machine
//                                                                                                                                                                                                            
// Revision 1.10  2004/11/07 01:07:59  ttoelkes
// trying to make NANDLOCK behavior better
//
// Revision 1.9  2004/10/27 16:11:01  ttoelkes
// added XFER_COUNT to GPMI cmd words only in 'cmp_dma' descriptor
//
// Revision 1.8  2004/10/27 15:57:11  ttoelkes
// fixed "bug" in 'check_dma' chain (SENSE cannot occur in same
// descriptor as GPMI Read&Compare)
//
// Revision 1.7  2004/10/07 21:13:23  ttoelkes
// mad hackery on the SECMLC common library
//
// Revision 1.6  2004/09/27 22:43:06  ttoelkes
// code complete on basic dma operations for SECMLC
//
// Revision 1.5  2004/09/27 19:58:30  ttoelkes
// added basic 'Read Status' operation; fixed some naming incoherencies
//
// Revision 1.4  2004/09/27 18:54:01  ttoelkes
// completed code for 'Serial Program' operation
//
// Revision 1.3  2004/09/27 18:26:05  ttoelkes
// added base code for 'Serial Program' operation
//
// Revision 1.2  2004/09/27 16:16:43  ttoelkes
// completed 'Read Id' functionality; added 'Reset' functionality
//
// Revision 1.1  2004/09/26 19:51:09  ttoelkes
// reorganizing NAND-specific half of GPMI code base
//
////////////////////////////////////////////////////////////////////////////////
//
// Old Log:
//
// Revision 1.1  2004/09/24 18:22:49  ttoelkes
// checkpointing new devlopment while it still compiles; not known to actually work
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Filename: SECMLC_dma.c
//
// Description: Implementation file for various commonly useful code concerning
//              dma operations with the Samsung SECMLC nand flash.
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) SigmaTel, Inc. Unpublished
//
// SigmaTel, Inc.
// Proprietary & Confidential
//
// This source code and the algorithms implemented therein constitute
// confidential information and may compromise trade secrets of SigmaTel, Inc.
// or its associates, and any unauthorized use thereof is prohibited.
//
////////////////////////////////////////////////////////////////////////////////

