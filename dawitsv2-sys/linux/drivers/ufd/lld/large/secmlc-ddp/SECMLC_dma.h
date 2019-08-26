/* 
 * \file K9HBG08U1M_dma.h
 *
 * \brief Declaration of various common dma related structure for K9HBG08U1M nand flash
 *
 * Based on Sigmatel validation code. 
 * Linux port is done by Samsung Electronics 
 * 
 * 2005 (C) Samsung Electronics. 
 * 2005 (C) Zeen Information Technologies, Inc. 
 * 
 * \author    Heechul Yun <heechul.yun@samsung.com>
 * \author    Yung Hyun Bae <yhbae@zeen.snu.ac.kr> 
 * \version   $Revision: 1.5 $ 
 * \date      $Date: 2007/09/10 12:06:00 $
 * 
 */ 

/*****
Zeen revision history

[2005/06/30] yhbae
	- add data structures and function declarations for group operations
	  (brute-force modification)

*****/

#ifndef _SECMLC_DMA_H_
#define _SECMLC_DMA_H_


#include "SECMLC_common.h"
#include <asm/memory.h>
#include <asm/arch/37xx/regsecc8.h>

//------------------------------------------------------------------------------
// 'Serial Read' operation : <00> <addr> <30> <wait> <data>

// dma chain structure
struct _SECMLC_dma_serial_read_t
{
	// descriptor sequence
	apbh_dma_gpmi3_t  tx_cle1_addr_dma;
	apbh_dma_gpmi3_t  tx_cle2_dma;
	apbh_dma_gpmi3_t  wait_dma;
	apbh_dma_t        sense_dma;
	apbh_dma_gpmi3_t  rx_data_dma;
	apbh_dma_gpmi3_t  rx_data2_dma;
	apbh_dma_gpmi3_t  disable_ecc_dma;

	// buffer for 'tx_cle1_addr_dma'
	union
	{
		reg8_t  tx_cle1_addr_buf[1 + ROW_SIZE + COL_SIZE];
		struct
		{
			volatile reg8_t tx_cle1;
			SECMLC_address_t tx_addr;
		}__attribute__ ((packed)); // hcyun - packed.. 
	};

	// buffer for 'tx_cle2_dma'
	union
	{
		reg8_t  tx_cle2_buf[1];
		struct
		{
			volatile reg8_t tx_cle2;
		}__attribute__ ((packed));
	};
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_serial_read_t SECMLC_dma_serial_read_t;

// build the dma chain structure for a 'Serial Read' operation
void  SECMLC_build_dma_serial_read(SECMLC_dma_serial_read_t* chain, 
                                       unsigned cs,
                                       reg32_t offset,
                                       reg16_t size,
                                       void* data_buf_paddr,
                                       void* aux_buf_paddr,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success);

//------------------------------------------------------------------------------
// 'Random Read' operation :  <05> <col> <e0> <data>

// dma chain structure
struct _SECMLC_dma_random_read_t
{
	// descriptor sequence
	apbh_dma_gpmi1_t  tx_cle1_addr_dma;
	apbh_dma_gpmi1_t  tx_cle2_dma;
	apbh_dma_gpmi1_t  rx_data_dma;

	// buffer for 'tx_cle1_addr_dma'
	union
	{
		reg8_t  tx_cle1_addr_buf[1 + COL_SIZE];
		struct
		{
			volatile reg8_t tx_cle1;
			SECMLC_col_t tx_col;
		}__attribute__ ((packed)); // hcyun - packed.. 
	};
	
	// buffer for 'tx_cle2_dma'
	union
	{
		reg8_t  tx_cle2_buf[1];
		struct
		{
			volatile reg8_t tx_cle2;
		}__attribute__ ((packed));
	};
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_random_read_t SECMLC_dma_random_read_t;

// build the dma chain structure for a 'Random Read' operation
void  SECMLC_build_dma_random_read(SECMLC_dma_random_read_t* chain, 
                                       unsigned cs,
				       int area,       /* main or spare */ 
                                       reg32_t offset, /* column offset */ 
                                       reg16_t size,
                                       void* buffer_paddr,
                                       apbh_dma_t* success);



//------------------------------------------------------------------------------
// 'Read Id' operation : <90> <00> <output> 

// dma chain structure
struct _SECMLC_dma_read_id_t
{
	// descriptor sequence
	apbh_dma_gpmi1_t  tx_dma;
	apbh_dma_gpmi1_t  rx_dma;
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_read_id_t SECMLC_dma_read_id_t;

// build the dma chain structure for a 'Read Id' operation
void  SECMLC_build_dma_read_id(SECMLC_dma_read_id_t* chain, 
                                   unsigned cs,
                                   void* buffer,
                                   apbh_dma_t* success);



//------------------------------------------------------------------------------
// 'Reset Device' operation : <ff> <wait>

// dma chain structure
struct _SECMLC_dma_reset_device_t
{
	// descriptor sequence
	apbh_dma_gpmi1_t  tx_dma;
	apbh_dma_gpmi1_t  wait_dma;
	apbh_dma_t        sense_dma;
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_reset_device_t SECMLC_dma_reset_device_t;

// build the dma chain structure for a 'Read Id' operation
void  SECMLC_build_dma_reset_device(SECMLC_dma_reset_device_t* chain, 
                                        unsigned cs,
                                        apbh_dma_t* timeout,
                                        apbh_dma_t* success);



//------------------------------------------------------------------------------
// 'Serial Program' operation : <80> <addr> <data> <10> <wait> 

// dma chain structure
struct _SECMLC_dma_serial_program_t
{
	// descriptor sequence
	apbh_dma_gpmi3_t  tx_cle1_addr_dma;
	apbh_dma_gpmi3_t  tx_data_dma;
	apbh_dma_gpmi3_t  tx_aux_dma;
	apbh_dma_gpmi3_t  tx_data2_dma;
	apbh_dma_gpmi3_t  tx_aux2_dma;
	apbh_dma_gpmi3_t  tx_cle2_dma;
	apbh_dma_gpmi3_t  wait_dma;
	apbh_dma_t        sense_dma;
	
	// buffer for 'tx_cle1_addr_dma'
	union
	{
		reg8_t  tx_cle1_addr_buf[1 + ROW_SIZE + COL_SIZE];
		struct
		{
			volatile reg8_t tx_cle1;
			SECMLC_address_t tx_addr;
		} __attribute__ ((packed)); // hcyun - packed.. 
	};

	// buffer for 'tx_cle2_dma'
	union
	{
		reg8_t  tx_cle2_buf[1];
		struct
		{
			volatile reg8_t tx_cle2;
		} __attribute__ ((packed));
	};
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_serial_program_t SECMLC_dma_serial_program_t;

// build the dma chain structure for a 'Serial Program' operation
void  SECMLC_build_dma_serial_program(SECMLC_dma_serial_program_t* chain, 
                                          unsigned cs,
                                          reg32_t offset,
                                          reg16_t size,
                                          const void* data_buf_paddr,
                                          const void* aux_buf_paddr,
                                          apbh_dma_t* timeout,
                                          apbh_dma_t* success);

// build the dma chain structure for a '2-Plane or 4-Plane Page Program Phase-1' operation
void  SECMLC_build_dma_serial_program_group1(SECMLC_dma_serial_program_t* chain, 
                                          unsigned cs,
                                          reg32_t offset,
                                          reg16_t size,
                                          const void* data_buf_paddr,
                                          const void* aux_buf_paddr,
                                          apbh_dma_t* timeout,
                                          apbh_dma_t* success);

// build the dma chain structure for a '2-Plane or 4-Plane Page Program Phase-2' operation
void  SECMLC_build_dma_serial_program_group2(SECMLC_dma_serial_program_t* chain, 
                                          unsigned cs,
                                          reg32_t offset,
                                          reg16_t size,
                                          const void* data_buf_paddr,
                                          const void* aux_buf_paddr,
                                          apbh_dma_t* timeout,
                                          apbh_dma_t* success);

// build the dma chain structure for a '4-Plane Page Program Phase-3' operation
void  SECMLC_build_dma_serial_program_group3(SECMLC_dma_serial_program_t* chain, 
                                          unsigned cs,
                                          reg32_t offset,
                                          reg16_t size,
                                          const void* data_buf_paddr,
                                          const void* aux_buf_paddr,
                                          apbh_dma_t* timeout,
                                          apbh_dma_t* success);

// build the dma chain structure for a '4-Plane Page Program Phase-4' operation
void  SECMLC_build_dma_serial_program_group4(SECMLC_dma_serial_program_t* chain, 
                                          unsigned cs,
                                          reg32_t offset,
                                          reg16_t size,
                                          const void* data_buf_paddr,
                                          const void* aux_buf_paddr,
                                          apbh_dma_t* timeout,
                                          apbh_dma_t* success);
                                          

//------------------------------------------------------------------------------
// 'Copy-back Read' operation : <00> <addr> <35> <wait tR> <data>

// dma chain structure
struct _SECMLC_dma_copyback_read_t
{
	// descriptor sequence
	apbh_dma_gpmi3_t  tx_cle1_addr_dma;
	apbh_dma_gpmi3_t  tx_cle2_dma;
	apbh_dma_gpmi3_t  wait_dma;
	apbh_dma_t        sense_dma;
	apbh_dma_gpmi3_t  rx_data_dma;
	apbh_dma_gpmi3_t  rx_data2_dma;
	apbh_dma_gpmi3_t  disable_ecc_dma;

	// buffer for 'tx_cle1_addr_dma'
	union
	{
		reg8_t  tx_cle1_addr_buf[1 + ROW_SIZE + COL_SIZE];
		struct
		{
			volatile reg8_t tx_cle1;
			SECMLC_address_t tx_addr;
		}__attribute__ ((packed)); // hcyun - packed.. 
	};

	// buffer for 'tx_cle2_dma'
	union
	{
		reg8_t  tx_cle2_buf[1];
		struct
		{
			volatile reg8_t tx_cle2;
		}__attribute__ ((packed));
	};
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_copyback_read_t SECMLC_dma_copyback_read_t;

// build the dma chain structure for a 'Copy-back Read' operation
void  SECMLC_build_dma_copyback_read(SECMLC_dma_copyback_read_t* chain, 
                                       unsigned cs,
                                       reg32_t offset,
                                       reg16_t size,
                                       void* data_buf_paddr,
                                       void* aux_buf_paddr,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success);


//------------------------------------------------------------------------------
// 'Two-Plane Copy-back Read' operation : <60> <row> <60> <row> <35> <wait tR>
//                                        <00> <addr> <05> <col> <e0> <data>
//                                        <00> <addr> <05> <col> <e0> <data>

// dma chain structure 
struct _SECMLC_dma_copyback_read_group1_t
{
	// descriptor sequence
	apbh_dma_gpmi1_t  tx_cle1_row1_dma;
	apbh_dma_gpmi1_t  tx_cle1_row2_dma;
	apbh_dma_gpmi1_t  tx_cle2_dma;
	apbh_dma_gpmi1_t  wait_dma;
	apbh_dma_t        sense_dma;
	
	// buffer for 'tx_cle1_row1_dma'
	union
	{
		reg8_t  tx_cle1_row1_buf[1 + ROW_SIZE]; 
		struct
		{
			volatile reg8_t tx_cle11;
			SECMLC_row_t tx_row1;
		} __attribute__((packed));
	};

	// buffer for 'tx_cle1_row2_dma'
	union
	{
		reg8_t  tx_cle1_row2_buf[1 + ROW_SIZE];
		struct
		{
			volatile reg8_t tx_cle12;
			SECMLC_row_t tx_row2;
		} __attribute__((packed));
	};

	// buffer for 'tx_cle2_dma'
	union
	{
		reg8_t  tx_cle2_buf[1];
		struct
		{
			volatile reg8_t tx_cle2;
		} __attribute__((packed));
	};
};

struct _SECMLC_dma_copyback_read_group2_t
{
	// descriptor sequence
	apbh_dma_gpmi1_t  tx_cle1_addr_dma;
	apbh_dma_gpmi1_t  tx_cle1_col_dma;
	apbh_dma_gpmi1_t  tx_cle2_dma;
	apbh_dma_gpmi1_t  rx_data_dma;

	// buffer for 'tx_cle1_addr_dma'
	union
	{
	    reg8_t  tx_cle1_addr_buf[1 + ROW_SIZE + COL_SIZE];
	    struct
	    {
	        volatile reg8_t tx_cle11;
	        SECMLC_address_t tx_addr;
	    } __attribute__((packed));
	};
	
	// buffer for 'tx_cle1_col_dma'
	union
	{
	    reg8_t  tx_cle1_col_buf[1 + COL_SIZE];
	    struct
	    {
	        volatile reg8_t tx_cle12;
	        SECMLC_col_t tx_col;
	    } __attribute__((packed));
	};
	
	// buffer for 'tx_cle2_dma'
	union
	{
	    reg8_t  tx_cle2_buf[1];
	    struct
	    {
	        volatile reg8_t tx_cle2;
	    } __attribute__((packed));
	};
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_copyback_read_group1_t SECMLC_dma_copyback_read_group1_t;
typedef struct _SECMLC_dma_copyback_read_group2_t SECMLC_dma_copyback_read_group2_t;

void  SECMLC_build_dma_copyback_read_group1(SECMLC_dma_copyback_read_group1_t* chain, 
                                       unsigned cs,
                                       unsigned* offset,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success);

void  SECMLC_build_dma_copyback_read_group2(SECMLC_dma_copyback_read_group2_t* chain, 
                                       unsigned cs,
                                       reg32_t offset,
                                       reg16_t size,
                                       void* buffer,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success);


//------------------------------------------------------------------------------
// 'Copy-back Program' operation : <85> <addr> <10> <wait> 

// dma chain structure
struct _SECMLC_dma_copyback_program_t
{
	// descriptor sequence
	apbh_dma_gpmi3_t  tx_cle1_addr_dma;
	apbh_dma_gpmi3_t  tx_cle2_dma;
	apbh_dma_gpmi3_t  wait_dma;
	apbh_dma_t        sense_dma;

	// buffer for 'tx_cle1_addr_dma'
	union
	{
		reg8_t  tx_cle1_addr_buf[1 + ROW_SIZE + COL_SIZE];
		struct
		{
			volatile reg8_t tx_cle1;
			SECMLC_address_t tx_addr;
		} __attribute__ ((packed));
	};
	
	// buffer for 'tx_cle2_dma'
	union
	{
		reg8_t  tx_cle2_buf[1];
		struct
		{
			volatile reg8_t tx_cle2;
		}__attribute__ ((packed));
	};
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_copyback_program_t SECMLC_dma_copyback_program_t;

// build the dma chain structure for a 'Copyback Program' operation
void  SECMLC_build_dma_copyback_program(SECMLC_dma_copyback_program_t* chain, 
					    unsigned cs,
					    reg32_t offset,
					    apbh_dma_t* timeout,
					    apbh_dma_t* success);

// build the dma chain structure for a '2-Plane Copyback Program Phase-1' operation
// <85> <addr> <11> <wait tDBSY>
void  SECMLC_build_dma_copyback_program_group1(SECMLC_dma_copyback_program_t* chain, 
					    unsigned cs,
					    reg32_t offset,
					    apbh_dma_t* timeout,
					    apbh_dma_t* success);

// build the dma chain structure for a '2-Plane Copyback Program Phase-2' operation
// <81> <addr> <10> <wait tPROG>
void  SECMLC_build_dma_copyback_program_group2(SECMLC_dma_copyback_program_t* chain, 
					    unsigned cs,
					    reg32_t offset,
					    apbh_dma_t* timeout,
					    apbh_dma_t* success);


//------------------------------------------------------------------------------
// 'Block Erase' operation : <60> <row> <d0> <wait tBERS>

// dma chain structure 
struct _SECMLC_dma_block_erase_t
{
	// descriptor sequence
	apbh_dma_gpmi1_t  tx_cle1_row_dma;
	apbh_dma_gpmi1_t  tx_cle2_dma;
	apbh_dma_gpmi1_t  wait_dma;
	apbh_dma_t        sense_dma;
	
	// buffer for 'tx_cle1_row_dma'
	union
	{
		reg8_t  tx_cle1_row_buf[1 + ROW_SIZE]; // hcyun 5 -> 3 
		struct
		{
			volatile reg8_t tx_cle1;
			SECMLC_row_t tx_row;
		} __attribute__((packed)); // hcyun 
	};

	// buffer for 'tx_cle2_dma'
	union
	{
		reg8_t  tx_cle2_buf[1];
		struct
		{
			volatile reg8_t tx_cle2;
		}__attribute__ ((packed));
	};
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_block_erase_t SECMLC_dma_block_erase_t;

// build the dma chain structure for a 'Block Erase' operation
void  SECMLC_build_dma_block_erase(SECMLC_dma_block_erase_t* chain, 
                                       unsigned cs,
                                       unsigned offset,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success);


//------------------------------------------------------------------------------
// 'Two-Plane Block Erase' operation : <60> <row> <60> <row> <d0> <wait tBERS>

// dma chain structure 
struct _SECMLC_dma_block_erase_group_t
{
	// descriptor sequence
	apbh_dma_gpmi1_t  tx_cle1_row1_dma;
	apbh_dma_gpmi1_t  tx_cle1_row2_dma;
	apbh_dma_gpmi1_t  tx_cle2_dma;
	apbh_dma_gpmi1_t  wait_dma;
	apbh_dma_t        sense_dma;
	
	// buffer for 'tx_cle1_row1_dma'
	union
	{
		reg8_t  tx_cle1_row1_buf[1 + ROW_SIZE]; // hcyun 5 -> 3 
		struct
		{
			volatile reg8_t tx_cle11;
			SECMLC_row_t tx_row1;
		} __attribute__((packed)); // hcyun 
	};

	// buffer for 'tx_cle1_row2_dma'
	union
	{
		reg8_t  tx_cle1_row2_buf[1 + ROW_SIZE]; // hcyun 5 -> 3 
		struct
		{
			volatile reg8_t tx_cle12;
			SECMLC_row_t tx_row2;
		} __attribute__((packed)); // hcyun 
	};

	// buffer for 'tx_cle2_dma'
	union
	{
		reg8_t  tx_cle2_buf[1];
		struct
		{
			volatile reg8_t tx_cle2;
		}__attribute__ ((packed));
	};
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_block_erase_group_t SECMLC_dma_block_erase_group_t;

void  SECMLC_build_dma_block_erase_group(SECMLC_dma_block_erase_group_t* chain, 
                                       unsigned cs,
                                       unsigned* offset,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success);

// build the dma chain structure for a '4-Plane Block Erase' operation
void  SECMLC_build_dma_block_erase_group1(SECMLC_dma_block_erase_group_t* chain, 
                                       unsigned cs,
                                       unsigned* offset,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success);

// build the dma chain structure for a '4-Plane Block Erase' operation
void  SECMLC_build_dma_block_erase_group2(SECMLC_dma_block_erase_group_t* chain, 
                                       unsigned cs,
                                       unsigned* offset,
                                       apbh_dma_t* timeout,
                                       apbh_dma_t* success);


//------------------------------------------------------------------------------
// 'Read Status' operation : <70> <out>

// dma chain structure : 
struct _SECMLC_dma_read_status_t
{
	// descriptor sequence
	apbh_dma_gpmi1_t  tx_dma;
	apbh_dma_gpmi1_t  rx_dma;
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_read_status_t SECMLC_dma_read_status_t;

// build the dma chain structure for a 'Read Status' operation
void  SECMLC_build_dma_read_status(SECMLC_dma_read_status_t* chain, 
                                       unsigned cs,
                                       void* buffer,
                                       apbh_dma_t* success);

//------------------------------------------------------------------------------
// 'Check Status' operation : <70> <IO 0~7> 
//			   or <F1> <IO 0~7> for chip-1 status
//			   or <F2> <IO 0~7> for chip-2 status

// dma chain structure
struct _SECMLC_dma_check_status_t
{
	// descriptor sequence
	apbh_dma_gpmi3_t  tx_dma;
	apbh_dma_gpmi3_t  cmp_dma;
	apbh_dma_t        sense_dma;
};

// give the dma chain structure a nice type name
typedef struct _SECMLC_dma_check_status_t SECMLC_dma_check_status_t;

// build the dma chain structure for a 'Check Status (70)' operation
void  SECMLC_build_dma_check_status(SECMLC_dma_check_status_t* chain, 
                                        unsigned cs,
                                        reg16_t mask,
                                        reg16_t match,
                                        apbh_dma_t* failure,
                                        apbh_dma_t* success);

// build the dma chain structure for a 'Check Status for Chip-1 (F1)' operation
void  SECMLC_build_dma_check_status1(SECMLC_dma_check_status_t* chain, 
                                        unsigned cs,
                                        reg16_t mask,
                                        reg16_t match,
                                        apbh_dma_t* failure,
                                        apbh_dma_t* success);
                                        
// build the dma chain structure for a 'Check Status for Chip-2 (F2)' operation
void  SECMLC_build_dma_check_status2(SECMLC_dma_check_status_t* chain, 
                                        unsigned cs,
                                        reg16_t mask,
                                        reg16_t match,
                                        apbh_dma_t* failure,
                                        apbh_dma_t* success);

// build the dma chain structure for a 'Check DBSY for Chip-1 (F1)' operation
void  SECMLC_build_dma_check_dbsy1(SECMLC_dma_check_status_t* chain, 
                                       unsigned cs,
                                       reg16_t mask,
                                       reg16_t match,
                                       apbh_dma_t* failure,
                                       apbh_dma_t* success);
                                        
// build the dma chain structure for a 'Check DBSY for Chip-2 (F2)' operation
void  SECMLC_build_dma_check_dbsy2(SECMLC_dma_check_status_t* chain, 
                                       unsigned cs,
                                       reg16_t mask,
                                       reg16_t match,
                                       apbh_dma_t* failure,
                                       apbh_dma_t* success);

#endif // !_SECMLC_DMA_H_

////////////////////////////////////////////////////////////////////////////////
//
// $Log: SECMLC_dma.h,v $
// Revision 1.5  2007/09/10 12:06:00  hcyun
// copyback operation added (but not used)
//
// - hcyun
//
// Revision 1.4  2007/08/16 07:49:10  hcyun
// fix mistake..
//
// - hcyun
//
// Revision 1.2  2007/08/16 05:59:35  hcyun
// [BUGFIX] two_plane_write_group bug fix. now support 4/8/16Gbit mono die chip
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
// Revision 1.7  2005/11/03 02:30:40  hcyun
// support SRAM descriptor (require SRAM map change)
// support SRAM ECC Buffer (require SRAM map change)
// support ECC Pipelining (LLD level only), about 20% read performance improvement when accessing over 4KB base
// minimize wait timing (T_OVERHEAD = 10)
//
// - hcyun
//
// Revision 1.6  2005/07/18 12:37:16  hcyun
// 1GB, 1plane, SW_COPYBACK, NO_ECC,
//
// - hcyun
//
// Revision 1.4  2005/07/05 10:36:41  hcyun
// ufd 2.0: 1 package 2GB support..
//
// - hcyun
//
// Revision 1.2  2005/06/28 07:29:58  hcyun
// conservative timing.
// add comments.
//
// - hcyun
//
// Revision 1.1  2005/06/27 07:47:17  hcyun
// first working (kernel panic due to sdram corruption??)
//
// - hcyun
//
// Revision 1.6  2005/06/20 15:44:48  hcyun
// ROW_SHIFT : 11 -> 12
//
// - hcyun
//
// Revision 1.5  2005/06/06 15:33:26  hcyun
// removed support of Random input of copy_back program..
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
// Revision 1.4  2005/05/15 05:10:29  hcyun
// copy-back, cache-program, random-input, random-output added. and removed unnecessary functions & types. Now, it's sync with ufd's dma functions..
//
// - hcyun
//
// Revision 1.1  2005/05/14 07:07:56  hcyun
// copy-back, cache-program, random-input, random-output added.
// fm_driver for stmp36xx added
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
// - Started using virt_to_phys.. not yet complete..
//
// Revision 1.11  2005/01/18 19:51:38  ttoelkes
// 'erase_read_program_read' test builds properly
//
// Revision 1.10  2004/11/30 16:49:37  ttoelkes
// pulling 'dma_read_id' and 'read_preloaded' test functions into common lib to easily split them apart by chip enable
//
// Revision 1.9  2004/11/10 15:30:11  ttoelkes
// updated chains for new dma state machine
//
// Revision 1.8  2004/10/27 16:00:49  ttoelkes
// added a descriptor to 'Check Status' sequence for SENSE only
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
// Revision 1.2  2004/09/27 16:16:44  ttoelkes
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
// Filename: SECMLC_dma.h
//
// Description: Declaration file for various commonly useful code concerning
//              the Samsung SECMLC nand flash.
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

