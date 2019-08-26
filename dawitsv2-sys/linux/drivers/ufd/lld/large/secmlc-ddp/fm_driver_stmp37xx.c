/*
 * @file drivers/ufd/lld/large/fm_driver_stmp37xx.c 
 *
 * @brief RFS ufd device driver for NAND flash. 
 *
 * 2005 (c) Samsung Electronics 
 * 2005 (C) Zeen Information Technologies, Inc. 
 * 
 * @author    Heechul Yun <heechul.yun@samsung.com> 
 * @author    Yung Hyun Bae <yhbae@zeen.snu.ac.kr> 
 * @author
 * @version   $Revision: 1.10 $
 * @date      $Date: 2007/09/12 08:32:00 $
 */ 

/*
 *  Revision history
 *                                                                      
 *      2007/06/01  hcyun  Initial 2k/4k support
 *      2007/07/01  zeen   Dynamic configuration based on id information 
 *      2007/08/16  hcyun  2-plane write bug fix (program2 -> program4)
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/compatmac.h>
#include <asm/sizes.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <asm/memory.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/irq.h>

#include <linux/init.h> 
#include <linux/fs.h>
#include <linux/proc_fs.h>

#include <asm/arch/37xx/regspinctrl.h> 
#include <asm/arch/37xx/regsecc8.h> 

#include "fm_global.h"
#include "fd_if.h"
#include "fd_physical.h"
#include "ecc.h"
#include "lld/large/fm_driver_lm_mlc.h"


#if (ECC_METHOD == HW_ECC)
#include <asm/arch/regs/regshwecc.h>
#include "../hwecc.h"
#endif
#include "../apbh_common.h"

#include "SECMLC_common.h"
#include "SECMLC_dma.h"

#include <asm/arch/37xx/regsdigctl.h>
#include <asm/arch-stmp37xx/ocram.h>
 
#if USE_LARGE_MULTI_MLC_NAND

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

static int MAX_NAND_CE_IN_GPMI = 4; 

#define GET_CS(chip)  \
        (MAX_NAND_CE_IN_GPMI == 4) ? (chip) : (chip & 0x01)
#define GPMI_APBH_CHANNEL_IN_FLM(chip) \
        (MAX_NAND_CE_IN_GPMI == 4 ) ? GPMI_APBH_CHANNEL(chip) : GPMI_APBH_CHANNEL((chip & 0x01))

// use the definitions in fd_if.h for consistent source management
#define MAX_NUM_OF_CHIPS			4	/* # of physical devices; should be 2^n */
#define MAX_NUM_OF_DMAS                         (MAX_NAND_CE_IN_GPMI)
#define MAX_BLOCKS_IN_GROUP			8	/* max # of blocks for multi-plane operation */

#define T_OVERHEAD                  200
#define SPARE_SIZE                  412 // 188

#define USE_OCRAM_DESCRIPTOR        1       /* 1 or 0 */
#define DESC_BUF_SIZE               8192    /* 2048 * 4 chips */
#define DEFAULT_DESC_SIZE           512     /* descriptors for SUCCESS & TIMEOUT dma */

//#define STMP36XX_SRAM_USB       0xe8000000
//#define STMP36XX_SRAM_RFS       0xe8001000
//#define STMP36XX_SRAM_AUDIO_USER 0xe8002000

#define LLD_DESC_BUF(chip, idx, desc)   ((desc *)get_desc_buf(chip, idx, sizeof(desc)))

/*----------------------------------------------------------------------*/
/*  Local Variable Definitions                                          */
/*----------------------------------------------------------------------*/

static int ROW_SHIFT, BLK_SHIFT; 

static nand_info_t nand_info[MAX_NUM_OF_CHIPS];

/** 
 only 1 type of flash is accepted. 
*/ 
FLASH_SPEC *flash_spec = NULL; 
UINT32 real_num_blocks = 0;     // only for SigmaTel 37xx evaluation board

/**
   nand flash timing 
*/ 
struct _nand_timing_t
{
        int tAS;
        int tDS; 
        int tDH;
        int timeout; 
 
        int tR;
        int tPROG;
        int tCBSY;
        int tBERS;
};
typedef struct _nand_timing_t nand_timing_t; 

static nand_timing_t timing = {
	// AC characteristic 
	.tAS = 1, //1,
	.tDS = 3, //3,
	.tDH = 1, //1,
	.timeout = 500000,

	// timing of flash ops 
	.tR = 25,
	.tPROG = 900,
	.tCBSY = 1,
	.tBERS = 2000
}; 

static unsigned char *g_desc_buf; 
static unsigned char *static_data_buf;
static unsigned char *static_spare_buf;

static unsigned char dma_buf[16];
static unsigned int  dma_buf_paddr;

static OP_DESC prev_op[MAX_NUM_OF_CHIPS];	// previous operation

static int ecc_active[MAX_NUM_OF_CHIPS];
static int ecc_last_chip = 0;
static apbh_dma_gpmi3_t *ecc_descriptor[MAX_NUM_OF_CHIPS];

#if USE_LLD_ISR 
volatile int gpmi_isr_ok, gpmi_isr_err; 

lld_device_t lld_dev; 
#endif 

/*----------------------------------------------------------------------*/
/*  External Variable Declarations                                      */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  External Function Declarations                                      */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  Local Function Declarations                                         */
/*----------------------------------------------------------------------*/

static int init_lld_proc(void); 
static int flash_select_channel(int chip);
static void  lld_test(UINT16 chip, UINT32 start_block, UINT32 num_blocks, UINT16 round);
static void   lld_test2(UINT16 chip, UINT32 start_block, UINT32 num_blocks, UINT16 round);

static INT32 flash_read_page(UINT16, UINT32, UINT16, UINT16, UINT16,
			     UINT8 *, UINT8 *, UINT8);
static INT32 flash_write_page(UINT16, UINT32, UINT16, UINT16, UINT16,
			      UINT8 *, UINT8 *, UINT8, BOOL);
static INT32 flash_copyback_read_page(UINT16, UINT32, UINT16, UINT8 *, UINT8 *, UINT32 *);
static INT32 flash_copyback_read_page_two_plane(UINT16, UINT32 *, UINT16 *, UINT8 **, UINT8 **);
static INT32 flash_copyback_write_page(UINT16, UINT32, UINT16);
static INT32 flash_copyback_write_page_two_plane(UINT16, UINT32 *, UINT16);
static INT32 flash_sync(UINT16, BOOL);

// static UINT8 flash_read_status(int chip); 

static inline void *
get_desc_buf(UINT16 chip, UINT16 idx, UINT32 desc_size)
{
    static unsigned char *buf_ptr[MAX_NUM_OF_CHIPS];
    unsigned char *ptr;
    
    if (idx == 0) buf_ptr[chip] = g_desc_buf + 2048 * chip;
    ptr = buf_ptr[chip];
    buf_ptr[chip] += desc_size;

#if 1
    if (buf_ptr[chip] > (g_desc_buf + 2048 * (chip + 1) - DEFAULT_DESC_SIZE)) {
        printk("%s: LLD descriptor overrun error !!!\n", __FUNCTION__);
        dump_stack();
        while (1);
    }
#endif

    return((void *)ptr);
}

/*----------------------------------------------------------------------*/
/*   ECC8 Reset Function                                                */
/*----------------------------------------------------------------------*/

#define DDI_NAND_HAL_RESET_ECC8_SFTRST_LATENCY  (2)

////////////////////////////////////////////////////////////////////////////////
//! \brief      Resets the ECC8 block
//!
//! \fntype     Non-Reentrant
//!
//! A soft reset can take multiple clocks to complete, so do NOT gate the
//! clock when setting soft reset. The reset process will gate the clock
//! automatically. Poll until this has happened before subsequently
//! clearing soft reset and clock gate.
////////////////////////////////////////////////////////////////////////////////
void ddi_nand_hal_ResetECC8(void)
{
    unsigned musecs;

    // Reset the ECC8_CTRL block.
    // Prepare for soft-reset by making sure that SFTRST is not currently
    // asserted.
    HW_ECC8_CTRL_CLR(BM_ECC8_CTRL_SFTRST);
    
    // Wait at least a microsecond for SFTRST to deassert.
    musecs = get_usec();
    while (HW_ECC8_CTRL.B.SFTRST || 
           (get_usec_elapsed(musecs, get_usec()) < DDI_NAND_HAL_RESET_ECC8_SFTRST_LATENCY));

    // Also clear CLKGATE so we can wait for its assertion below.
    HW_ECC8_CTRL_CLR(BM_ECC8_CTRL_CLKGATE);
    
    // Now soft-reset the hardware.
    HW_ECC8_CTRL_SET(BM_ECC8_CTRL_SFTRST);
    
    // Poll until clock is in the gated state before subsequently
    // clearing soft reset and clock gate.
    while (!HW_ECC8_CTRL.B.CLKGATE)
    {
        ; // busy wait
    }

    // Deassert SFTRST.
    HW_ECC8_CTRL_CLR(BM_ECC8_CTRL_SFTRST);
    
    // Wait at least a microsecond for SFTRST to deassert. In actuality, we
    // need to wait 3 GPMI clocks, but this is much easier to implement.
    musecs = get_usec();
    while (HW_ECC8_CTRL.B.SFTRST || 
           (get_usec_elapsed(musecs, get_usec()) < DDI_NAND_HAL_RESET_ECC8_SFTRST_LATENCY));

    HW_ECC8_CTRL_CLR(BM_ECC8_CTRL_CLKGATE);

    // Poll until clock is in the NON-gated state before returning.
    while (HW_ECC8_CTRL.B.CLKGATE)
    {
        ; // busy wait
    }
}

/*======================================================================*/
/*  Function Definitions                                                */
/*======================================================================*/

static int get_pbn(int vbn) 
{
	int pbn; 

	/* check if the chip support internal interleaving */ 
	if ( flash_spec->NumDiesPerCE == 1 ) 
		// no internal interleaving 
		pbn = vbn; 
	else { 
		
		if (((vbn) & 0x3) < 2)
			pbn = 0;
		else
			pbn = (real_num_blocks >> 1) - 1;
		
		pbn += (((vbn) >> 1) + ((vbn) & 0x1));
	}
	return pbn; 
}


/*----------------------------------------------------------------------*/
/*  Global Function Definitions                                         */
/*----------------------------------------------------------------------*/

#undef TIMING_LLD
#define TIMING_LLD 0

#if TIMING_LLD

lld_perf_t lld_perf = {
	{ 0xffffffff, 0, 0, 0}, // read_page
	{ 0xffffffff, 0, 0, 0}, // erase_group
	{ 0xffffffff, 0, 0, 0}, // write_page_group
}; 

#endif /* TIMING_LLD */ 

#define TEST_START_BLK 3000
#define TEST_END_BLK   3100

/*
  TODO... !!FIXME!! 

  chip 0, 1, gpmi[23]= 0 - ce0, ce1 
  chip 2, 3, gpmi[23]= 1 - ce0, ce1 
*/ 
static int flash_select_channel(int chip) 
{
	static int prev_bank = 0; 
	int cur_bank; 
	int err; 

	if ( MAX_NAND_CE_IN_GPMI == 4 ) return 0; 

	if ( chip >= 0 && chip < 2 ) 
		cur_bank = 0; 
	else if ( chip >= 2 && chip < 4 ) 
		cur_bank = 1; 
	else 
		panic("Impossible chip select %d", chip); 

	if ( prev_bank != cur_bank ) { 
		/* change bank require syncronize all previous bank transaction */ 
		//printk("bank changed from %d to %d\n", prev_bank, cur_bank); 

		if ( prev_bank == 0 ) {		// select bank 1
			err = flash_sync(0, TRUE); 
			if (err) {
				prev_op[0].Result = err;
				prev_op[0].Command |= OP_SYNC_MASK;
			}

			err = flash_sync(1, TRUE); 
			if (err) {
				prev_op[1].Result = err;
				prev_op[1].Command |= OP_SYNC_MASK;
			}

			/* set 1 to gpio1[23] */ 
			HW_PINCTRL_DOUT1_SET( 1 << 23 ); 
		} else { 					// select bank 0
			err = flash_sync(2, TRUE); 
			if (err) {
				prev_op[2].Result = err;
				prev_op[2].Command |= OP_SYNC_MASK;
			}

			err = flash_sync(3, TRUE); 
			if (err) {
				prev_op[3].Result = err;
				prev_op[3].Command |= OP_SYNC_MASK;
			}

			/* set 0 to gpio1[23] */ 
			HW_PINCTRL_DOUT1_CLR( 1 << 23 ); 
		}
		prev_bank = cur_bank;  

		udelay(5); /* wait channel is stablized.. 70ns maximum */ 
	}
	return FM_SUCCESS; 
} 


/* FLM_MLC8G_2_Init : intializes this driver module;
    registers flash memory chips driven by this driver module */
INT32 FLM_MLC8G_2_Init(void)
{
	INT32 i, err;
	FLASH_OPS flash_ops;
	
	INT32 cs_rdy_mask = 0; 
	unsigned char *ptr; 

	lld_debug = 0; 
	flash_spec = (FLASH_SPEC *)kmalloc(sizeof(FLASH_SPEC), GFP_KERNEL); 

#if 1
	// reset all the previous states.. -> this cause problem.. T_T 
	// BW_APBH_CTRL0_SFTRST(1); // reset APBH block. 
	// BW_GPMI_CTRL0_SFTRST(1); // reset the GPMI block... 
	// BW_HWECC_CTRL_SFTRST(1); // reset HWECC block 

	// reset the GPMI block... 
	HW_GPMI_CTRL0_SET(BM_GPMI_CTRL0_SFTRST);
	HW_GPMI_CTRL0_CLR(BM_GPMI_CTRL0_SFTRST | BM_GPMI_CTRL0_CLKGATE);
#endif

#if 1
    // bring APBH out of reset
    HW_APBH_CTRL0_CLR(BM_APBH_CTRL0_SFTRST);
    HW_APBH_CTRL0_CLR(BM_APBH_CTRL0_CLKGATE);
    
    // bring ECC8 out of reset
    HW_ECC8_CTRL_CLR(BM_ECC8_CTRL_SFTRST);
    HW_ECC8_CTRL_CLR(BM_ECC8_CTRL_CLKGATE);
    HW_ECC8_CTRL_CLR(BM_ECC8_CTRL_AHBM_SFTRST);
    
    // bring GPMI out of reset
    HW_GPMI_CTRL0_CLR(BM_GPMI_CTRL0_SFTRST);
    HW_GPMI_CTRL0_CLR(BM_GPMI_CTRL0_CLKGATE);
    HW_GPMI_CTRL0_CLR(BM_GPMI_CTRL1_DEV_RESET);
    
    // enable PINCTRL
    HW_PINCTRL_CTRL_WR(0x00000000);
    
    // enable GPMI through alt pin wiring
    //HW_PINCTRL_MUXSEL0_CLR(0xff000000);
    //HW_PINCTRL_MUXSEL0_SET(0xaa000000);
    HW_PINCTRL_MUXSEL0_CLR(0xf0000000);
    HW_PINCTRL_MUXSEL0_SET(0xa0000000);
    HW_PINCTRL_MUXSEL4_CLR(0xf0000000);
    HW_PINCTRL_MUXSEL4_SET(0x50000000);
    
    // enable GPMI pins
    HW_PINCTRL_MUXSEL0_CLR(0x0000ffff);
    HW_PINCTRL_MUXSEL1_CLR(0x000fffff);
#endif 

#if VERBOSE
	printk("<UFD> Initializing NAND flash\n"); 
#endif

	/* register flash operations */
	MEMSET((void *) &flash_ops, 0, sizeof (FLASH_OPS));
	flash_ops.Open = FLM_MLC8G_2_Open;
	flash_ops.Close = FLM_MLC8G_2_Close;
	flash_ops.ReadPage = FLM_MLC8G_2_Read_Page;
	flash_ops.WritePage = FLM_MLC8G_2_Write_Page;
	flash_ops.CopyBack = FLM_MLC8G_2_Copy_Back;
	flash_ops.Erase = FLM_MLC8G_2_Erase;

	//flash_ops.ReadPageGroup = FLM_MLC8G_2_Read_Page_Group;
	flash_ops.WritePageGroup = FLM_MLC8G_2_Write_Page_Group;
	flash_ops.CopyBackGroup = FLM_MLC8G_2_Copy_Back_Group;
	flash_ops.EraseGroup = FLM_MLC8G_2_Erase_Group; 

	flash_ops.ReadDeviceCode = FLM_MLC8G_2_Read_ID;
	flash_ops.IsBadBlock = FLM_MLC8G_2_IsBadBlock;
	flash_ops.IsMultiOK = FLM_MLC8G_2_Is_Multi_OK;
	flash_ops.Sync = FLM_MLC8G_2_Sync;

	/* chip enable mask */ 
	for (i = 0; (i < MAX_NUM_OF_CHIPS) && (i < MAX_NAND_CE_IN_GPMI); i++) 
		cs_rdy_mask |= (1 << i ); 

	/* place APBH in run mode (not reset and no clock gating) */ 
	BF_CS2(APBH_CTRL0, SFTRST, 0, CLKGATE, 0);

#if 0
	if ( MAX_NAND_CE_IN_GPMI == 2 ) { 
		/* setup GPMI1[23] that control switch between chip0,1 <-> chip2,3 */ 
		HW_PINCTRL_MUXSEL3_CLR(0x0000c000); 
		HW_PINCTRL_MUXSEL3_SET(0x0000c000); // select gpio1[23] as output mode 
		HW_PINCTRL_DOE1_CLR(0x00800000); 
		HW_PINCTRL_DOE1_SET(0x00800000);    // output enable 
		HW_PINCTRL_DOUT1_CLR(0x00800000);   // set low -> select chip 0,1 
	}
#endif

	// enable the GPMI block
	gpmi_enable(0, cs_rdy_mask );

	// set drive strength of ALE(22), CLE(23), WRn(21), RDn(17) to 8mA 
	HW_PINCTRL_DRIVE0_SET(0x00E20000); 

	// Setup GPMI timing and other miscellaneous parameters.
	// set GPMI into NAND mode, and reset.. 
	gpmi_nand8_pio_config(timing.tAS, timing.tDS, timing.tDH, timing.timeout); 
#if VERBOSE
	printk("<UFD> Current Timing : TAS(%d), TDS(%d), TDH(%d) Timeout(%d)\n", 
	       timing.tAS, timing.tDS, timing.tDH, timing.timeout); 
#endif
	// Setup GPMI DSAMPLE timing 
	BW_GPMI_CTRL1_DSAMPLE_TIME(2); 

#if (ECC_METHOD == HW_ECC)
	// enable HWECC block. 
	BF_CS2(HWECC_CTRL, SFTRST, 0, CLKGATE, 0);
#endif

#if USE_LLD_ISR
	// clear any pending interrupts... 
	HW_APBH_CTRL1_CLR(APBH_IRQ(APBH_CHANNEL_MASK(4)));
	HW_APBH_CTRL1_CLR(APBH_IRQ(APBH_CHANNEL_MASK(5)));
  	HW_APBH_CTRL1_CLR(APBH_IRQ(APBH_CHANNEL_MASK(0)));

	err = request_irq(INT_GPMI_DMA, lld_isr, SA_INTERRUPT, 
			  "gpmi", NULL);

	err = request_irq(INT_ECC_DMA, lld_isr, SA_INTERRUPT, 
			  "ecc", NULL);

	BW_APBH_CTRL1_CH0_CMDCMPLT_IRQ_EN(1);
	BW_APBH_CTRL1_CH4_CMDCMPLT_IRQ_EN(1);
	BW_APBH_CTRL1_CH5_CMDCMPLT_IRQ_EN(1);
#endif 
	dma_buf_paddr = stmp_virt_to_phys(dma_buf); 

#if USE_OCRAM_DESCRIPTOR 
	g_desc_buf = (unsigned char *)(OCRAM_RFS_START_VIRT);
	printk("<UFD> Using OCRAM descriptors : 0x%x\n", (unsigned)g_desc_buf); 
#else 
	g_desc_buf = (unsigned char *) kmalloc(DESC_BUF_SIZE, GFP_KERNEL); 
	if ( g_desc_buf == NULL ) { 
		panic("%s:Out of memory\n", __FUNCTION__);
	}
#endif 

	memset(g_desc_buf, 0, DESC_BUF_SIZE); 
	ptr = g_desc_buf; 

    ptr = g_desc_buf + DESC_BUF_SIZE - DEFAULT_DESC_SIZE;
    ptr = init_apbh_descriptors(ptr, DEFAULT_DESC_SIZE);
	ASSERT( ptr < g_desc_buf + DESC_BUF_SIZE ); 
	lld_stat.apbh_desc_size = (ptr - (g_desc_buf + DESC_BUF_SIZE - DEFAULT_DESC_SIZE));

	/* remaining buf is for lld descriptors (5*128) * MAX_CE */ 
	lld_stat.lld_desc_size = ((unsigned)g_desc_buf + DESC_BUF_SIZE) - (unsigned)ptr; 

	/* register each physical flash device (chip) */
	for (i = 0; i < MAX_NUM_OF_CHIPS; i++) {
		unsigned char maker, dev_code; 
		
		/* clear the flash operation logs */
		prev_op[i].Command = OP_NONE;
		ecc_active[i] = 0;

		/* initialize NAND info structures */
		MEMSET((void *)&nand_info[i], 0, sizeof(nand_info_t)); // BUG fix - hcyun 
		
		/* Test if it is exist */ 
		if ( FLM_MLC8G_2_Read_ID(i, &maker, &dev_code) != FM_SUCCESS ) {
			break;
		}
		// skkim test
		//flash_spec->NumDiesPerCE = 1;
		//flash_spec->NumPlanes = 2;
		//flash_spec->NumBlocks -= 400; // last 100MB reserved for F/W

		/* Read ID should assign flash_spec */ 
		assert(flash_spec); 

		/* register each physical flash device (chip) */
		err = PFD_RegisterFlashDevice(PFD_LARGE_NAND,
					      i,
					      flash_spec,
					      &flash_ops, &prev_op[i]);
		if (err)
			return (FM_INIT_FAIL);
	}

	g_use_odd_even_copyback = FALSE; 

	init_lld_proc(); 

	static_data_buf = (unsigned char *)kmalloc(flash_spec->DataSize * flash_spec->NumPlanes * MAX_NUM_OF_CHIPS, GFP_KERNEL);
	if (static_data_buf == NULL) {
        panic("%s: Out of memory for data buffers.\n", __FUNCTION__);
    }
    
    static_spare_buf = (unsigned char *)kmalloc(SPARE_SIZE * flash_spec->NumPlanes * MAX_NUM_OF_CHIPS, GFP_KERNEL);
    if (static_spare_buf == NULL) {
        panic("%s: Out of memory for spare buffers.\n", __FUNCTION__);
    }

#if VERBOSE
    printk("%s: sizeof(SECMLC_dma_serial_program_t) = %d\n", __FUNCTION__, sizeof(SECMLC_dma_serial_program_t));
    printk("%s: sizeof(SECMLC_dma_block_erase_t) = %d\n", __FUNCTION__, sizeof(SECMLC_dma_block_erase_t));
    printk("%s: sizeof(SECMLC_dma_block_erase_group_t) = %d\n", __FUNCTION__, sizeof(SECMLC_dma_block_erase_group_t));
    printk("%s: sizeof(SECMLC_dma_check_status_t) = %d\n", __FUNCTION__, sizeof(SECMLC_dma_check_status_t));
    printk("%s: sizeof(SECMLC_dma_serial_read_t) = %d\n", __FUNCTION__, sizeof(SECMLC_dma_serial_read_t));
#endif

	return (FM_SUCCESS);
}				/* end of FLM_MLC8G_2_Init */


/* FLM_MLC8G_2_Open : open(initialize) Flash Memory device for future use
    NAND Flash Configuration register initialization
    issue RESET command to Flash Memory */
INT32 FLM_MLC8G_2_Open(UINT16 chip)
{
	int ret; 

	SECMLC_dma_reset_device_t reset_chain;

	PDEBUG("%s entered\n", __FUNCTION__); 

	flash_select_channel(chip);

#if 1 // hcyun ?? 
	// prepare dma chain 
	SECMLC_build_dma_reset_device
		(&reset_chain, 
		 GET_CS(chip), 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA, 
		 (apbh_dma_t*) APBH_SUCCESS_DMA); 

	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)&reset_chain);

	// start DMA 
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 2);
	
	// poll wait.. 
	ret = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 10000); /* wait 10ms */ 

	if ( ret ) 
		return (FM_ERROR); 
#endif 

#if 0
    printk("\nLLD TEST for CHIP-%d\n\n", chip);
    lld_test2(chip, 2048, 128, 1);
#endif

	return FM_SUCCESS;	
}				/* end of FLM_MLC8G_2_Open */

/* FLM_MLC8G_2_Close : close Flash Memory device
               just symbolic function, practically nothing to do */
INT32 FLM_MLC8G_2_Close(UINT16 chip)
{
	return (FLM_MLC8G_2_Sync(chip));
}				/* end of FLM_MLC8G_2_Close */

/* FLM_MLC8G_2_Read_ID : read the Flash Memory ID information
    parameters : drive number, block number */
INT32 FLM_MLC8G_2_Read_ID(UINT16 chip, UINT8 * maker, UINT8 * dev_code)
{
	INT32 err;
	INT32 ret; 
	UINT8 id_data1, id_data2, id_data3;

	nand_info_t *n;
	FLASH_SPEC  *f;

	SECMLC_dma_read_id_t read_id; 

	PDEBUG("%s entered for chip (%d)\n", __FUNCTION__, chip); 

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, TRUE);
	if (err)
		return (err);

	SECMLC_build_dma_read_id
		(&read_id,
		 GET_CS(chip), 
		 (void *)dma_buf_paddr, /* buffer_paddr */ 
		 (apbh_dma_t*) APBH_SUCCESS_DMA);

	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)&read_id);

	PDEBUG("setup dma channel for chip (%d)\n", chip); 

	ASSERT(chip < MAX_NUM_OF_CHIPS); 

	// start DMA 
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 2);

	rfs_invalidate_dcache_range((unsigned int)dma_buf, (unsigned int)dma_buf + 16); 

	// poll wait.. 
	ret = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 100000); /* wait 10ms */ 

	if ( !ret ) {
		/* successfully read id */ 
		int i;
#if VERBOSE
		printk("<UFD> Device ID for chip %d is ", chip); 
#endif
		for ( i = 0; i < 5; i++ ) { /* id size = 5 */ 
		    nand_info[chip].id[i] = dma_buf[i];
#if VERBOSE
			printk ("%x ", dma_buf[i]); 
#endif
		}
#if VERBOSE
		printk ("\n"); 
#endif
	} else {
		printk("Failed....\n"); 
		return FM_ERROR; 
	}

	n = &nand_info[chip];	

	*maker    = n->id[0]; 
	*dev_code = n->id[1]; 

	if ( *maker != 0xEC ) /* samsung NAND */ 
		return FM_ERROR;

	id_data1  = n->id[2];
	id_data2  = n->id[3];
	id_data3  = n->id[4];

	/* extract information from the 3rd ID data */
	n->internal_chip_number = 1 << (id_data1 & 0x03);
	n->cell_type            = 2 << ((id_data1 & 0x0c) >> 2);
	n->num_simul_prog_pages = 1 << ((id_data1 & 0x30) >> 4);
	n->interleave_support   = (id_data1 & 0x40) >> 6;
	n->cache_prog_support   = (id_data1 & 0x80) >> 7;
	
	/* extract information from the 4th ID data */
	n->page_size            = 1024 << (id_data2 & 0x03);
	n->block_size           = (64*1024) << ((id_data2 & 0x30) >> 4);
	n->spare_size_in_512    = 8 << ((id_data2 & 0x04) >> 2);
	n->organization         = 8 << ((id_data2 & 0x40) >> 6);
	n->serial_access_min    = 50 >> ((id_data2 & 0x80) >> 7);
	
	/* extract information from the 5th ID data */
	n->plane_number         = 1 << ((id_data3 & 0x0c) >> 2);
	n->plane_size           = (8*1024*1024) << ((id_data3 & 0x70) >> 4);

#if VERBOSE
	printk("internal #chip = %d\n", 1<<(id_data1 & 0x03));
	printk("cell type = %d level cell\n", 2<<((id_data1 & 0xc)>>2));
	printk("number of simultaneously programmed pages = %d\n", 1 << ((id_data1 & 0x30)>>4));
	printk("support interleave = %d\n", ((id_data1 & 0x40)>>6));
	printk("support cache program = %d\n", ((id_data1 & 0x80)>>7));


	printk("page size = %dKB\n", 1 << (id_data2 & 0x03));
	printk("block size = %dKB\n", 64 << ((id_data2 & 0x30)>>4)); 
	printk("organization = x%d\n", 8 << ((id_data2 & 0x40)>>6));

	printk("plane number = %d\n", 1<<((id_data3 & 0x0c)>>2));
	printk("plane size = %dMB\n", 8<<((id_data3 & 0x70)>>4));
#endif 

	f = flash_spec;

	f->NumDiesPerCE    = n->internal_chip_number;
	f->NumPlanes       = n->plane_number;
	f->DataSize        = n->page_size;
	f->SectorsPerPage  = n->page_size >> 9;
	f->SpareSize       = n->spare_size_in_512 * f->SectorsPerPage;
	f->PageSize        = f->DataSize + f->SpareSize;
	f->BlockSize       = n->block_size;
	f->PagesPerBlock   = f->BlockSize / f->DataSize;
	f->NumBlocks       = n->plane_size / f->BlockSize * n->plane_number;
	f->MaxNumBadBlocks = f->NumBlocks * 245 / 10000;    /* 2.45% */

    real_num_blocks = f->NumBlocks;     // only for SigmaTel 37xx evaluation board
	g_copy_back_method = SW_COPYBACK; 
	
	if ( n->id[2] == 0x55 && n->id[3] == 0x25 && n->id[4] == 0x58 ) // K9HAG08U1M
		f->MaxNumBadBlocks = 160;
	if ( n->id[2] == 0x55 && n->id[3] == 0x25 && n->id[4] == 0x68 ) // K9HBG08U1M
		f->MaxNumBadBlocks = 200;
	if ( n->id[2] == 0x14 && n->id[3] == 0x25 && n->id[4] == 0x64 ) // K9LAG08U0M
		f->MaxNumBadBlocks = 160;
	if ( n->id[2] == 0x14 && n->id[3] == 0xb6 && n->id[4] == 0x74 ) // K9GAG08U0M
		g_copy_back_method = SW_COPYBACK; /* !!FIXME!! */

	// TODO: K9LBG08U0M. 
	
#if VERBOSE
	printk("flash_spec[%d].NumDiesPerCE    = %d\n", chip, f->NumDiesPerCE);
	printk("flash_spec[%d].NumPlanes       = %d\n", chip, f->NumPlanes); 
	printk("flash_spec[%d].PageSize        = %d\n", chip, f->PageSize);
	printk("flash_spec[%d].DataSize        = %d\n", chip, f->DataSize);
	printk("flash_spec[%d].SpareSize       = %d\n", chip, f->SpareSize);
	printk("flash_spec[%d].SectorsPerPage  = %d\n", chip, f->SectorsPerPage);
	printk("flash_spec[%d].PagesPerBlock   = %d\n", chip, f->PagesPerBlock);
	printk("flash_spec[%d].BlockSize       = %d\n", chip, f->BlockSize);
	printk("flash_spec[%d].NumBlocks       = %d\n", chip, f->NumBlocks);
	printk("flash_spec[%d].MaxNumBadBlocks = %d\n", chip, f->MaxNumBadBlocks);

	printk("Feature Setting\n"); 
	printk("CopyBack : %s\n", (g_copy_back_method == HW_COPYBACK) ? "Hardware" : "Software"); 
	printk("--------------------\n"); 
	
#endif

	switch (f->DataSize) {
		case 1024: ROW_SHIFT = 11; BLK_SHIFT = (11 + 7); break;
		case 2048: ROW_SHIFT = 12; BLK_SHIFT = (12 + 7); break;
		case 4096: ROW_SHIFT = 13; BLK_SHIFT = (13 + 7); break;
		case 8192: ROW_SHIFT = 14; BLK_SHIFT = (14 + 7); break;
		default:
			printk("Not supported NAND flash page size.\n"); 
			return FM_ERROR; 
	}

	SECMLC_init_addr(ROW_SHIFT); 

	return (FM_SUCCESS);
}				/* end of FLM_MLC8G_2_Read_ID */

/* FLM_MLC8G_2_Read_Page : read a page from data
and spare area (2112 bytes)
    parameters : data buffer, block number, page number */
INT32 FLM_MLC8G_2_Read_Page(UINT16 chip,
		    UINT32 block,
		    UINT16 page,
		    UINT16 sector_offset,
		    UINT16 num_sectors, UINT8 * dbuf, UINT8 * sbuf)
{
	int err; 
	
	lld_stat.read ++; 

#if STRICT_CHECK
	/* check if the block and page numbers are valid */
	if ((block >= flash_spec->NumBlocks) ||
	    (page >= flash_spec->PagesPerBlock)) {
		return (FM_ILLEGAL_ACCESS);
	}
	if ((num_sectors == 0) ||
	    (num_sectors + sector_offset > flash_spec->SectorsPerPage)) {
		return (FM_ILLEGAL_ACCESS);
	}
#endif

	if (dbuf != NULL && sbuf != NULL) {
		/* read main & spare area */
		err = flash_read_page
			(chip, block, page, sector_offset, num_sectors, dbuf,
			 sbuf, FLASH_AREA_A);
	} else if (dbuf != NULL && sbuf == NULL) {
		/* read main area only */
		err = flash_read_page
			(chip, block, page, sector_offset, num_sectors, dbuf,
			 NULL, FLASH_AREA_A);
	} else if (dbuf == NULL && sbuf != NULL) {
		/* read spare area only */
		return (flash_read_page
			(chip, block, page, sector_offset, num_sectors, NULL,
			 sbuf, FLASH_AREA_C));
	} else {
		return (FM_ERROR);
	}

	return (err);
}				/* end of FLM_MLC8G_2_Read_Page */

/* FLM_MLC8G_2_Read_Page_Group : read pages in a group (whole page - 2112 bytes)
    parameters : buffers, block numbers, page number
				 flag to indicate including planes
	For K9HBG08U1M, The two planes are always included in all group operations,
	so the "flag" has no effect. */
INT32  FLM_MLC8G_2_Read_Page_Group(UINT16 chip, 
		    UINT32 *block, UINT16 page,
			UINT8 *dbuf_group[], UINT8 *sbuf_group[], INT32 *flag)
{
	INT32 i, err;


	lld_stat.read_group ++; 

	PDEBUG("%s: \nchip(%d), blocks(%d,%d,%d,%d), page(%d)\n", __FUNCTION__, 
	       chip, block[0], block[1], block[2], block[3], page); 

	err = FM_SUCCESS;
	for (i = 0; i < flash_spec->NumPlanes; i++) {
		flag[i] = flash_read_page(chip, block[i], page, 0, flash_spec->SectorsPerPage, dbuf_group[i],
					  sbuf_group[i], FLASH_AREA_A);
		if (flag[i]) {
			err = flag[i];
			break;
		}
	}

	return (err);
}				/* end of FLM_MLC8G_2_Read_Page_Group */



/* FLM_MLC8G_2_Write_Page : write a page from data and spare area (2112 bytes)
    parameters : data buffer, block number, page number */
INT32 FLM_MLC8G_2_Write_Page(UINT16 chip,
		     UINT32 block,
		     UINT16 page,
		     UINT16 sector_offset,
		     UINT16 num_sectors,
		     UINT8 * dbuf, UINT8 * sbuf, BOOL is_last)
{
	INT32 err;

	lld_stat.write ++; 

	is_last = TRUE;

#if STRICT_CHECK
	/* check if the block and page numbers are valid */
	if ((block >= flash_spec->NumBlocks) ||
	    (page >= flash_spec->PagesPerBlock)) {
		return (FM_ILLEGAL_ACCESS);
	}
	if ((num_sectors == 0) ||
	    (num_sectors + sector_offset > flash_spec->SectorsPerPage)) {
		return (FM_ILLEGAL_ACCESS);
	}
#endif

	if (dbuf != NULL && sbuf != NULL) {
		/* write main & spare area */
		err =
		    flash_write_page(chip, block, page, sector_offset,
				     num_sectors, dbuf, sbuf, FLASH_AREA_A,
				     is_last);
		if (err)
			return (err);

	}

	else if (dbuf != NULL && sbuf == NULL) {
		/* write main area only */
		err =
		    flash_write_page(chip, block, page, sector_offset,
				     num_sectors, dbuf, NULL, FLASH_AREA_A,
				     is_last);
		if (err)
			return (err);

	}

	else if (dbuf == NULL && sbuf != NULL) {
		/* write spare area only */
		err =
		    flash_write_page(chip, block, page, sector_offset,
				     num_sectors, NULL, sbuf, FLASH_AREA_C,
				     is_last);
		if (err)
			return (err);

	}

	else {
		printk("%s: null buffer error\n", __FUNCTION__); 
		printk("chip=%d, block=%d, page=%d, sector_offset=%d, num_sectors=%d\n",
		       chip, block, page, sector_offset, num_sectors); 
		return (FM_ERROR);
	}
	
	if (!err) {
    	prev_op[chip].Command = OP_WRITE;
    	prev_op[chip].Param.Write.Block = block;
    	prev_op[chip].Param.Write.Page = page;
    	prev_op[chip].Param.Write.SectorOffset = sector_offset;
    	prev_op[chip].Param.Write.NumSectors = num_sectors;
    	prev_op[chip].Param.Write.DBuf = dbuf;
    	prev_op[chip].Param.Write.SBuf = sbuf;
    	prev_op[chip].Param.Write.IsLast = is_last;
	}

	return (FM_SUCCESS);
}				/* end of FLM_MLC8G_2_Write_Page */



INT32  two_plane_write_group(UINT16 chip, 
			UINT32 *block, UINT16 page,
			UINT8 *dbuf_group[], UINT8 *sbuf_group[], 
			INT32 *flag, BOOL is_last)
{
	INT32 i,err;
	INT32 addr; 
	UINT8 *sbuf[2];

	UINT32 __block[2];
	UINT32 __page; 

	SECMLC_dma_serial_program_t *program1 = LLD_DESC_BUF(chip, 0, SECMLC_dma_serial_program_t);
	SECMLC_dma_serial_program_t *program2 = LLD_DESC_BUF(chip, 1, SECMLC_dma_serial_program_t);
	SECMLC_dma_check_status_t *status =     LLD_DESC_BUF(chip, 2, SECMLC_dma_check_status_t); 

#if TIMING_LLD
	unsigned start, duration; 
	start = get_usec(); 
#endif /* TIMING_LLD */

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, FALSE);
	if (err)
		return (err);

	PDEBUG("%s: \nchip(%d), block1(%d), block2(%d) page(%d)\n", 
	       __FUNCTION__, chip, block[0], block[1], page); 
	
	for ( i = 0; i < 2; i++ ) {
		__block[i] = get_pbn(block[i]);
		sbuf[i] = static_spare_buf + chip * SPARE_SIZE * flash_spec->NumPlanes + i * SPARE_SIZE;
		memcpy(sbuf[i], sbuf_group[i], flash_spec->SpareSize);
	}

	__page = page; 

	// build DMA descriptor 
#if !USE_16GBIT_NAND
	addr = (__block[0] << BLK_SHIFT) + (__page << ROW_SHIFT); 
#else 
	addr = (__block[0] << (BLK_SHIFT-4)) + (__page << (ROW_SHIFT-4)); 
#endif 

    ddi_nand_hal_ResetECC8();

	SECMLC_build_dma_serial_program_group1
		(program1,
		 GET_CS(chip),
		 addr,// offset 
		 flash_spec->DataSize,
		 (void *)stmp_virt_to_phys(dbuf_group[0]),
		 (void *)stmp_virt_to_phys(sbuf[0]),
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) program2);

	rfs_clean_dcache_range((unsigned int)dbuf_group[0], (unsigned int)dbuf_group[0] + flash_spec->DataSize);
	rfs_clean_dcache_range((unsigned int)sbuf[0], (unsigned int)sbuf[0] + flash_spec->SpareSize); 

#if !USE_16GBIT_NAND
	addr = (__block[1] << BLK_SHIFT) + (__page << ROW_SHIFT); 
#else 
	addr = (__block[1] << (BLK_SHIFT-4)) + (__page << (ROW_SHIFT-4)); 
#endif 
	SECMLC_build_dma_serial_program_group2
		(program2,
		 GET_CS(chip),
		 addr,// offset 
		 flash_spec->DataSize,
		 (void *)stmp_virt_to_phys(dbuf_group[1]),
		 (void *)stmp_virt_to_phys(sbuf[1]),
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) status);

	rfs_clean_dcache_range((unsigned int)dbuf_group[1], (unsigned int)dbuf_group[1] + flash_spec->DataSize);
	rfs_clean_dcache_range((unsigned int)sbuf[1], (unsigned int)sbuf[1] + flash_spec->SpareSize); 
	
	SECMLC_build_dma_check_status
		(status,
		 GET_CS(chip),
		 0x01, // pass/fail - IO 0 
		 0x00, 
		 (apbh_dma_t*) APBH_FAILURE_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA); 
	
	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)program1);

	// start DMA
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 2);	

	prev_op[chip].Command = OP_WRITE_GROUP;
	prev_op[chip].Param.WriteGroup.Page = page;
	prev_op[chip].Param.WriteGroup.IsLast = is_last;
	for (i = 0; i < 2; i++) {
	    prev_op[chip].Param.WriteGroup.Block[i] = block[i];
	    prev_op[chip].Param.WriteGroup.DBuf[i] = dbuf_group[i];
	    prev_op[chip].Param.WriteGroup.SBuf[i] = sbuf_group[i];
	    prev_op[chip].Param.WriteGroup.Flag[i] = flag[i];
	}
	
	for (i = 0; i < 2; i++) {
	    if (flag[i]) flag[i] = FM_SUCCESS;
	}

#if TIMING_LLD
	duration = get_usec_elapsed(start, get_usec()); 
	lld_perf.write_page_group.min = MIN(lld_perf.write_page_group.min, duration); 
	lld_perf.write_page_group.max = MAX(lld_perf.write_page_group.max, duration); 
	lld_perf.write_page_group.cnt ++; 
	lld_perf.write_page_group.tot += duration; 
#endif 
	
    ecc_active[chip] = 1;
	ecc_descriptor[chip] = &program2->wait_dma;
    ecc_last_chip = chip;
    	
	return (FM_SUCCESS);	// program success
}				/* end of two_plane_write_group */


INT32  four_plane_write_group(UINT16 chip, 
			UINT32 *block, UINT16 page,
			UINT8 *dbuf_group[], UINT8 *sbuf_group[], 
			INT32 *flag, BOOL is_last)
{
	INT32 i,err;
	INT32 addr; 
	UINT8 *sbuf[4];
	
	UINT32 __block[4];
	UINT32 __page; 

	SECMLC_dma_serial_program_t *program1 = LLD_DESC_BUF(chip, 0, SECMLC_dma_serial_program_t); 
	SECMLC_dma_serial_program_t *program2 = LLD_DESC_BUF(chip, 1, SECMLC_dma_serial_program_t); 
	SECMLC_dma_serial_program_t *program3 = LLD_DESC_BUF(chip, 2, SECMLC_dma_serial_program_t); 
	SECMLC_dma_check_status_t *dbsy2 =      LLD_DESC_BUF(chip, 3, SECMLC_dma_check_status_t); 
	SECMLC_dma_serial_program_t *program4 = LLD_DESC_BUF(chip, 4, SECMLC_dma_serial_program_t); 
    SECMLC_dma_check_status_t *status1 =    LLD_DESC_BUF(chip, 5, SECMLC_dma_check_status_t); 
    SECMLC_dma_check_status_t *status2 =    LLD_DESC_BUF(chip, 6, SECMLC_dma_check_status_t); 

#if TIMING_LLD
	unsigned start, duration; 
	start = get_usec(); 
#endif /* TIMING_LLD */

	lld_stat.write_group ++; 

#if STRICT_CHECK
	/* check if the block number is valid */
	if ((block[0] >= flash_spec->NumBlocks) || 
	    (block[1] >= flash_spec->NumBlocks) ||
	    (block[2] >= flash_spec->NumBlocks) ||
	    (block[3] >= flash_spec->NumBlocks) ||
	    ((block[0] & 0x03) != 0) ||
	    ((block[1] & 0x03) != 1) ||
	    ((block[2] & 0x03) != 2) ||
	    ((block[3] & 0x03) != 3) ||
	    ((block[0] >> 2) != (block[1] >> 2)) ||
	    ((block[0] >> 2) != (block[2] >> 2)) ||
	    ((block[0] >> 2) != (block[3] >> 2))) {
		return (FM_ILLEGAL_ACCESS);
	}
#endif

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, FALSE);
	if (err)
		return (err);

	PDEBUG("%s: \nchip(%d), block1(%d), block2(%d), block3(%d), block4(%d), page(%d)\n", 
	       __FUNCTION__, chip, block[0], block[1], block[2], block[3], page); 

	for ( i = 0; i < 4; i++ ) {
		__block[i] = get_pbn(block[i]);
		sbuf[i] = static_spare_buf + chip * SPARE_SIZE * flash_spec->NumPlanes + i * SPARE_SIZE;
		memcpy(sbuf[i], sbuf_group[i], flash_spec->SpareSize);
	}

	__page = page; 

	// build DMA descriptor 
#if !USE_16GBIT_NAND
	addr = (__block[0] << BLK_SHIFT) + (__page << ROW_SHIFT); 
#else 
	addr = (__block[0] << (BLK_SHIFT-4)) + (__page << (ROW_SHIFT-4)); 
#endif 

    ddi_nand_hal_ResetECC8();

	SECMLC_build_dma_serial_program_group1
		(program1,
		 GET_CS(chip),
		 addr,// offset 
		 flash_spec->DataSize,
		 (void *)stmp_virt_to_phys(dbuf_group[0]),
		 (void *)stmp_virt_to_phys(sbuf[0]),
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) program2);

	rfs_clean_dcache_range((unsigned int)dbuf_group[0], (unsigned int)dbuf_group[0] + flash_spec->DataSize);
	rfs_clean_dcache_range((unsigned int)sbuf[0], (unsigned int)sbuf[0] + flash_spec->SpareSize); 

#if !USE_16GBIT_NAND
	addr = (__block[1] << BLK_SHIFT) + (__page << ROW_SHIFT); 
#else 
	addr = (__block[1] << (BLK_SHIFT-4)) + (__page << (ROW_SHIFT-4)); 
#endif 
	SECMLC_build_dma_serial_program_group2
		(program2,
		 GET_CS(chip),
		 addr,// offset 
		 flash_spec->DataSize,
		 (void *)stmp_virt_to_phys(dbuf_group[1]),
		 (void *)stmp_virt_to_phys(sbuf[1]),
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) program3);

	rfs_clean_dcache_range((unsigned int)dbuf_group[1], (unsigned int)dbuf_group[1] + flash_spec->DataSize);
	rfs_clean_dcache_range((unsigned int)sbuf[1], (unsigned int)sbuf[1] + flash_spec->SpareSize); 
	
#if !USE_16GBIT_NAND
	addr = (__block[2] << BLK_SHIFT) + (__page << ROW_SHIFT); 
#else 
	addr = (__block[2] << (BLK_SHIFT-4)) + (__page << (ROW_SHIFT-4)); 
#endif
	SECMLC_build_dma_serial_program_group3
		(program3,
		 GET_CS(chip),
		 addr,// offset 
		 flash_spec->DataSize,
		 (void *)stmp_virt_to_phys(dbuf_group[2]),
		 (void *)stmp_virt_to_phys(sbuf[2]),
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) dbsy2);

	rfs_clean_dcache_range((unsigned int)dbuf_group[2], (unsigned int)dbuf_group[2] + flash_spec->DataSize);
	rfs_clean_dcache_range((unsigned int)sbuf[2], (unsigned int)sbuf[2] + flash_spec->SpareSize); 
	
	SECMLC_build_dma_check_dbsy2
		(dbsy2,
		 GET_CS(chip),
		 0x40, // ready/busy - IO 6 
		 0x40, 
		 (apbh_dma_t*) dbsy2,
		 (apbh_dma_t*) program4);
	
#if !USE_16GBIT_NAND
	addr = (__block[3] << BLK_SHIFT) + (__page << ROW_SHIFT); 
#else 
	addr = (__block[3] << (BLK_SHIFT-4)) + (__page << (ROW_SHIFT-4)); 
#endif 
	SECMLC_build_dma_serial_program_group4
		(program4,
		 GET_CS(chip),
		 addr,// offset 
		 flash_spec->DataSize,
		 (void *)stmp_virt_to_phys(dbuf_group[3]),
		 (void *)stmp_virt_to_phys(sbuf[3]),
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) status1);

	rfs_clean_dcache_range((unsigned int)dbuf_group[3], (unsigned int)dbuf_group[3] + flash_spec->DataSize);
	rfs_clean_dcache_range((unsigned int)sbuf[3], (unsigned int)sbuf[3] + flash_spec->SpareSize); 

	SECMLC_build_dma_check_status1
		(status1,
		 GET_CS(chip),
		 0x01, // pass/fail - IO 0 
		 0x00, 
		 (apbh_dma_t*) APBH_FAILURE_DMA,
		 (apbh_dma_t*) status2); 
	
	SECMLC_build_dma_check_status2
		(status2,
		 GET_CS(chip),
		 0x01, // pass/fail - IO 0 
		 0x00, 
		 (apbh_dma_t*) APBH_FAILURE_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA);
	
	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)program1);

    // start DMA
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 3);	
	
	prev_op[chip].Command = OP_WRITE_GROUP;
	prev_op[chip].Param.WriteGroup.Page = page;
	prev_op[chip].Param.WriteGroup.IsLast = is_last;
	for (i = 0; i < 4; i++) {
	    prev_op[chip].Param.WriteGroup.Block[i] = block[i];
	    prev_op[chip].Param.WriteGroup.DBuf[i] = dbuf_group[i];
	    prev_op[chip].Param.WriteGroup.SBuf[i] = sbuf_group[i];
	    prev_op[chip].Param.WriteGroup.Flag[i] = flag[i];
	}
	
	for (i = 0; i < 4; i++) {
	    if (flag[i]) flag[i] = FM_SUCCESS;
	}

#if TIMING_LLD
	duration = get_usec_elapsed(start, get_usec()); 
	lld_perf.write_page_group.min = MIN(lld_perf.write_page_group.min, duration); 
	lld_perf.write_page_group.max = MAX(lld_perf.write_page_group.max, duration); 
	lld_perf.write_page_group.cnt ++; 
	lld_perf.write_page_group.tot += duration; 
#endif 
	
    ecc_active[chip] = 1;
	ecc_descriptor[chip] = &program4->wait_dma;
    ecc_last_chip = chip;
    	
	return (FM_SUCCESS);	// program success
}				/* end of four_plane_write_group */


/* FLM_MLC8G_2_Write_Page_Group : write pages in a group (whole page - 2112 bytes)
    parameters : buffers, block numbers, page number
				 flag to indicate including planes
	For K9HBG08U1M, The two planes are always included in all group operations,
	so the "flag" has no effect. The "is_last" has also no meaning since there
	isn't cache program function in K9HBG08U1M. */
INT32  FLM_MLC8G_2_Write_Page_Group(UINT16 chip, 
			UINT32 *block, UINT16 page,
			UINT8 *dbuf_group[], UINT8 *sbuf_group[], 
			INT32 *flag, BOOL is_last)
{
	if ( flash_spec->NumPlanes == 2) {
		return two_plane_write_group(chip, block, page, dbuf_group, sbuf_group, flag, is_last); 
	} else if ( flash_spec->NumPlanes == 4) {
		return four_plane_write_group(chip, block, page, dbuf_group, sbuf_group, flag, is_last); 		
	} else {
		printk("%s: ERROR: unsupported plane size = %d\n", __FUNCTION__, flash_spec->NumPlanes);
		return FM_ERROR; 
	}
}


/* FLM_MLC8G_2_Copy_Back : copy the source page into the destination page
    in the same plane using the internal page buffer
    parameters : src block, src page, dest block, dest page */
INT32 FLM_MLC8G_2_Copy_Back(UINT16 chip,
		    UINT32 src_block,
		    UINT16 src_page, UINT32 dest_block, UINT16 dest_page)
{
	INT32 err;
	UINT32 ecc_result;
    UINT8  *dbuf = static_data_buf  + chip * flash_spec->DataSize * flash_spec->NumPlanes;
    UINT8  *sbuf = static_spare_buf + chip * SPARE_SIZE * flash_spec->NumPlanes;
    
    lld_stat.copyback ++;
    
    err = flash_copyback_read_page(chip, src_block, src_page, dbuf, sbuf, &ecc_result);
    if (err) return(err);
    
    if (ecc_result & BM_ECC8_STATUS0_UNCORRECTABLE) {
        // uncorrectable bit-flip error occurred
        lld_stat.copyback_fail ++;
        return(FM_ECC_ERROR);
    }
    else if (ecc_result & BM_ECC8_STATUS0_ALLONES) {
        // source page is all 0xFF; nothing to do here
        return(FM_SUCCESS);
    }
    else if (ecc_result & BM_ECC8_STATUS0_CORRECTED) {
        // correctable bit-flip error; perform a normal page write
        err = flash_write_page(chip, dest_block, dest_page, 
                               0, flash_spec->SectorsPerPage,
                               dbuf, sbuf, FLASH_AREA_A, 1);
    }
    else {
        // no bit-flip error; now copyback program can be performed
        err = flash_copyback_write_page(chip, dest_block, dest_page);
        lld_stat.real_copyback ++;
    }
   
    if (!err) {
        prev_op[chip].Command = OP_COPYBACK;
    	prev_op[chip].Param.CopyBack.SrcBlock = src_block;
    	prev_op[chip].Param.CopyBack.SrcPage = src_page;
    	prev_op[chip].Param.CopyBack.DestBlock = dest_block;
    	prev_op[chip].Param.CopyBack.DestPage = dest_page;
    }

	return (err);
}				/* end of FLM_MLC8G_2_Copy_Back */


INT32 two_plane_copyback_group(UINT16 chip,
		    UINT32 *src_block, UINT16 *src_page, 
			UINT32 *dest_block, UINT16 dest_page, INT32 *flag)
{
    INT32  err, i, command_issued = TRUE;
    UINT32 ecc_result[2];
    UINT8  *dbuf[2], *sbuf[2];

    for (i = 0; i < 2; i++) {
        dbuf[i] = static_data_buf  + chip * flash_spec->DataSize * flash_spec->NumPlanes + i * flash_spec->DataSize;
        sbuf[i] = static_spare_buf + chip * SPARE_SIZE * flash_spec->NumPlanes + i * SPARE_SIZE;
    }
    
#if 0    
    err = flash_copyback_read_page_two_plane(chip, src_block, src_page, dbuf, sbuf);
    if (err) return (err);
#else
    for (i = 0; i < 2; i++) {
        err = flash_copyback_read_page(chip, src_block[i], src_page[i], dbuf[i], sbuf[i], &ecc_result[i]);
        if (err) return(err);
    }
#endif
    
    if ((ecc_result[0] & BM_ECC8_STATUS0_UNCORRECTABLE) || (ecc_result[1] & BM_ECC8_STATUS0_UNCORRECTABLE)) {
        // uncorrectable bit-flip error occurred
        err = FM_ECC_ERROR;
        command_issued = FALSE;
    }
    else if ((ecc_result[0] & BM_ECC8_STATUS0_ALLONES) && (ecc_result[1] & BM_ECC8_STATUS0_ALLONES)) {
        // source page is all 0xFF; nothing to do here
        err = FM_SUCCESS;
        command_issued = FALSE;
        }
    else if ((ecc_result[0] & BM_ECC8_STATUS0_CORRECTED) || (ecc_result[1] & BM_ECC8_STATUS0_CORRECTED)) {
        // correctable bit-flip error; perform normal page writes
        err = two_plane_write_group(chip, dest_block, dest_page,
			                        dbuf, sbuf, flag, 1);
    }
    else {
        // no bit-flip error; now copyback program group can be performed
        err = flash_copyback_write_page_two_plane(chip, dest_block, dest_page);
        lld_stat.real_copyback_group ++;
    }
    
    if (!err && command_issued) {
    	for (i = 0; i < 2; i++) {
    	    prev_op[chip].Command = OP_COPYBACK_GROUP;
        	prev_op[chip].Param.CopyBackGroup.SrcBlock[i] = src_block[i];
        	prev_op[chip].Param.CopyBackGroup.SrcPage[i] = src_page[i];
        	prev_op[chip].Param.CopyBackGroup.DestBlock[i] = dest_block[i];
        	prev_op[chip].Param.CopyBackGroup.Flag[i] = flag[i];
    	}
        prev_op[chip].Param.CopyBackGroup.DestPage = dest_page;
    }

    for (i = 0; i < 2; i++) {
        if (flag[i]) flag[i] = err;
    }

	return (err);
}   /* end of two_plane_copyback_group */


INT32 four_plane_copyback_group(UINT16 chip,
		    UINT32 *src_block, UINT16 *src_page, 
			UINT32 *dest_block, UINT16 dest_page, INT32 *flag)
{
    INT32 err;
    
    err = two_plane_copyback_group(chip, src_block, src_page, dest_block, dest_page, flag);
    if (err) return (err);
    
    err = two_plane_copyback_group(chip, &src_block[2], &src_page[2], &dest_block[2], dest_page, &flag[2]);
    return (err);
}   /* end of four_plane_copyback_group */

/* FLM_MLC8G_2_Copy_Back_Group : copy the source page into the destination page
    in the same plane using the internal page buffer (group operation)
    parameters : src blocks, src pages, dest blocks, dest page
				 flag to indicate including planes
	For K9WAG08U1M, The two planes are always included in all group operations,
	so the "flag" has no effect. */
INT32 FLM_MLC8G_2_Copy_Back_Group(UINT16 chip,
		    UINT32 *src_block, UINT16 *src_page, 
			UINT32 *dest_block, UINT16 dest_page, INT32 *flag)
{
    INT32 err, i;
    
    lld_stat.copyback_group ++;
    
	if (flash_spec->NumPlanes == 2) {
		err = two_plane_copyback_group(chip, src_block, src_page, dest_block, dest_page, flag); 
	} else if (flash_spec->NumPlanes == 4) {
		err = four_plane_copyback_group(chip, src_block, src_page, dest_block, dest_page, flag);		
	} else {
		printk("%s: ERROR: unsupported plane size = %d\n", __FUNCTION__, flash_spec->NumPlanes);
		err = FM_ERROR; 
	}
	
    if (err) {
        lld_stat.copyback_group_fail ++;
    }
    
    return (err);
}

/* FLM_MLC8G_2_Erase : erase an entire block
    parameters : block number to be erased */
INT32 FLM_MLC8G_2_Erase(UINT16 chip, UINT32 block)
{
	INT32 err;
	INT32 addr;
	UINT32 __block;
	
	SECMLC_dma_block_erase_t *erase =   LLD_DESC_BUF(chip, 0, SECMLC_dma_block_erase_t); 
	SECMLC_dma_check_status_t *status = LLD_DESC_BUF(chip, 1, SECMLC_dma_check_status_t);  

	lld_stat.erase ++; 

	__block = get_pbn(block); 

	PDEBUG("%s: chip %d, block %d\n", __FUNCTION__, chip, block);

#if STRICT_CHECK
	/* check if the block number is valid */
	if (block >= flash_spec->NumBlocks)
		return (FM_ILLEGAL_ACCESS);
#endif

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, TRUE);
	if (err)
		return (err);

#if !USE_16GBIT_NAND
	addr = __block << BLK_SHIFT;
#else 
	addr = __block << (BLK_SHIFT-4);
#endif 
	// build DMA descriptors
	SECMLC_build_dma_block_erase
		(erase,
		 GET_CS(chip), 
		 addr,// offset 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) status);  /* link to status chain */ 

	SECMLC_build_dma_check_status
		(status, 
		 GET_CS(chip), 
		 0x01,/* I/O 0 */ 
		 0x00,/* 0 success, 1 failure */ 
		 (apbh_dma_t*) APBH_FAILURE_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA); 

#if DEBUG_CHAIN
	printk("tx_cle1 = %x\n", erase->tx_cle1); 
	printk("tx_cle2 = %x\n", erase->tx_cle2); 
	printk("tx_cle1_addr_buf : \n"); 
	for ( i = 0 ; i < 1 + ROW_SIZE; i++ ) 
		printk(" %x ", erase->tx_cle1_row_buf[i]); 
	printk("\n"); 
	printk("tx_cle2 = %x\n", erase->tx_cle2); 
#endif 

	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)erase);

	// start DMA 
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 3);

	prev_op[chip].Command = OP_ERASE;
	prev_op[chip].Param.Erase.Block = block;

	return (FM_SUCCESS);	// erase success
}				/* end of FLM_MLC8G_2_Erase */

INT32 four_plane_erase_group(UINT16 chip, UINT32 *block, INT32 *flag)
{
	INT32 err, i;
	UINT32 addr[MAX_BLOCKS_IN_GROUP];
	UINT32 __block[MAX_BLOCKS_IN_GROUP]; /* real block number */ 
	
	SECMLC_dma_block_erase_group_t *erase_group1 = LLD_DESC_BUF(chip, 0, SECMLC_dma_block_erase_group_t);  
	SECMLC_dma_block_erase_group_t *erase_group2 = LLD_DESC_BUF(chip, 1, SECMLC_dma_block_erase_group_t);  
	SECMLC_dma_check_status_t *status1 =           LLD_DESC_BUF(chip, 2, SECMLC_dma_check_status_t);
	SECMLC_dma_check_status_t *status2 =           LLD_DESC_BUF(chip, 3, SECMLC_dma_check_status_t);

#if TIMING_LLD
	unsigned start, duration; 
	start = get_usec(); 
#endif /* TIMING_LLD */ 

	lld_stat.erase_group++; 

	for ( i = 0; i < flash_spec->NumPlanes; i++ ) {
		__block[i] = get_pbn(block[i]); 
	}

	PDEBUG("%s: chip %d, block1(%d), block2(%d), block3(%d), block4(%d)\n", 
	       __FUNCTION__, chip, block[0], block[1], block[2], block[3]);

#if STRICT_CHECK
	/* check if the block number is valid */
	if ((block[0] >= flash_spec->NumBlocks) || 
	    (block[1] >= flash_spec->NumBlocks) ||
	    (block[2] >= flash_spec->NumBlocks) ||
	    (block[3] >= flash_spec->NumBlocks) ||
	    ((block[0] & 0x03) != 0) ||
	    ((block[1] & 0x03) != 1) ||
	    ((block[2] & 0x03) != 2) ||
	    ((block[3] & 0x03) != 3) ||
	    ((block[0] >> 2) != (block[1] >> 2)) ||
	    ((block[0] >> 2) != (block[2] >> 2)) ||
	    ((block[0] >> 2) != (block[3] >> 2))) {
		return (FM_ILLEGAL_ACCESS);
	}
#endif

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, TRUE);
	if (err) 
		return (err);
	
#if !USE_16GBIT_NAND 
	addr[0] = __block[0] << BLK_SHIFT;
	addr[1] = __block[1] << BLK_SHIFT;
	addr[2] = __block[2] << BLK_SHIFT;
	addr[3] = __block[3] << BLK_SHIFT;
#else  	
	addr[0] = __block[0] << (BLK_SHIFT-4);
	addr[1] = __block[1] << (BLK_SHIFT-4);
	addr[2] = __block[2] << (BLK_SHIFT-4);
	addr[3] = __block[3] << (BLK_SHIFT-4);
#endif

	// build DMA descriptors
	SECMLC_build_dma_block_erase_group1
		(erase_group1,
		 GET_CS(chip),
		 &addr[0],// offset 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) erase_group2);

	SECMLC_build_dma_block_erase_group2
		(erase_group2,
		 GET_CS(chip),
		 &addr[2],// offset 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) status1);  /* link to status chain */
	
	SECMLC_build_dma_check_status1
		(status1, 
		 GET_CS(chip),
		 0x01,/* I/O 0 */ 
		 0x00,/* 0 success, 1 failure */ 
		 (apbh_dma_t*) APBH_FAILURE_DMA,
		 (apbh_dma_t*) status2); 

	SECMLC_build_dma_check_status2
		(status2, 
		 GET_CS(chip),
		 0x01,/* I/O 0 */ 
		 0x00,/* 0 success, 1 failure */ 
		 (apbh_dma_t*) APBH_FAILURE_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA);
	
	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)erase_group1);

	// start DMA 
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 4);
	
	prev_op[chip].Command = OP_ERASE_GROUP;
	for (i = 0; i < flash_spec->NumPlanes; i++) {
	    prev_op[chip].Param.EraseGroup.Block[i] = block[i];
	    prev_op[chip].Param.EraseGroup.Flag[i] = flag[i];
	}

	for (i = 0; i < flash_spec->NumPlanes; i++) {
	    if (flag[i]) flag[i] = FM_SUCCESS;
	}

#if TIMING_LLD
	duration = get_usec_elapsed(start, get_usec()); 
	lld_perf.erase_group.min = MIN(lld_perf.erase_group.min, duration); 
	lld_perf.erase_group.max = MAX(lld_perf.erase_group.max, duration); 
	lld_perf.erase_group.cnt ++; 
	lld_perf.erase_group.tot += duration; 
#endif 

	return (FM_SUCCESS);	// erase success
}				/* end of FLM_MLC8G_2_Erase_Group */

INT32 two_plane_erase_group(UINT16 chip, UINT32 *block, INT32 *flag)
{
	INT32 err, i;
	UINT32 addr[MAX_BLOCKS_IN_GROUP];
	UINT32 __block[MAX_BLOCKS_IN_GROUP]; /* real block number */ 
	
	SECMLC_dma_block_erase_group_t *erase_group = LLD_DESC_BUF(chip, 0, SECMLC_dma_block_erase_group_t);  
	SECMLC_dma_check_status_t *status =           LLD_DESC_BUF(chip, 1, SECMLC_dma_check_status_t);

#if TIMING_LLD
	unsigned start, duration; 
	start = get_usec(); 
#endif /* TIMING_LLD */ 

	lld_stat.erase_group++; 

	for ( i = 0; i < flash_spec->NumPlanes; i++ ) {
		__block[i] = get_pbn(block[i]); 
	}

	PDEBUG("%s: chip %d, block1(%d), block2(%d), block3(%d), block4(%d)\n", 
	       __FUNCTION__, chip, block[0], block[1], block[2], block[3]);

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, TRUE);
	if (err) 
		return (err);
	
#if !USE_16GBIT_NAND 
	addr[0] = __block[0] << BLK_SHIFT;
	addr[1] = __block[1] << BLK_SHIFT;
#else  	
	addr[0] = __block[0] << (BLK_SHIFT-4);
	addr[1] = __block[1] << (BLK_SHIFT-4);
#endif

	// build DMA descriptors
	SECMLC_build_dma_block_erase_group
		(erase_group,
		 GET_CS(chip),
		 &addr[0],// offset 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) status);

	SECMLC_build_dma_check_status
		(status, 
		 GET_CS(chip),
		 0x01,/* I/O 0 */ 
		 0x00,/* 0 success, 1 failure */ 
		 (apbh_dma_t*) APBH_FAILURE_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA); 

	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)erase_group);

	// start DMA 
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 3);
	
	prev_op[chip].Command = OP_ERASE_GROUP;
	for (i = 0; i < flash_spec->NumPlanes; i++) {
	    prev_op[chip].Param.EraseGroup.Block[i] = block[i];
	    prev_op[chip].Param.EraseGroup.Flag[i] = flag[i];
	}

	for (i = 0; i < flash_spec->NumPlanes; i++) {
	    if (flag[i]) flag[i] = FM_SUCCESS;
	}

#if TIMING_LLD
	duration = get_usec_elapsed(start, get_usec()); 
	lld_perf.erase_group.min = MIN(lld_perf.erase_group.min, duration); 
	lld_perf.erase_group.max = MAX(lld_perf.erase_group.max, duration); 
	lld_perf.erase_group.cnt ++; 
	lld_perf.erase_group.tot += duration; 
#endif 

	return (FM_SUCCESS);	// erase success
}				/* end of FLM_MLC8G_2_Erase_Group */

/* FLM_MLC8G_2_Erase_Group : erase two blocks in each plane
    parameters : chip number, block numbers to be erased, 
    flag to indicate including planes

    For K9HBG08U1M, The two planes are always included in all group operations,
    so the "flag" has no effect. 
*/
INT32 FLM_MLC8G_2_Erase_Group(UINT16 chip, UINT32 *block, INT32 *flag)
{

	if ( flash_spec->NumPlanes == 2) {
		return two_plane_erase_group(chip, block, flag);
	} else if ( flash_spec->NumPlanes == 4) {
		return four_plane_erase_group(chip, block, flag);
	} else {
		printk("%s: ERROR: unsupported plane size = %d\n", __FUNCTION__, flash_spec->NumPlanes);
		return FM_ERROR; 
	}
}



/* FLM_MLC8G_2_Sync : wait until the previous operation complete
    its execution */
INT32 FLM_MLC8G_2_Sync(UINT16 chip)
{
	flash_select_channel(chip);

	return (flash_sync(chip, TRUE));
}				/* end of FLM_MLC8G_2_Sync */

/* FLM_MLC8G_2_IsBadBlock : check if the block is bad using spare information
    if the 6th bytes of the first two page are not 0xff, the block is bad
    parameters : drive number, block number */
BOOL FLM_MLC8G_2_IsBadBlock(UINT16 chip, UINT32 block)
{
#if 1
	INT32 err;
	UINT8 buf[16*8];

	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, TRUE);
	if (err)
		return (err);

	// the chip bank selection is not needed. 
	// it is done in flash_read_page()

	flash_read_page(chip, block, 127, 0, flash_spec->SectorsPerPage, NULL, buf, FLASH_AREA_C);

	if (buf[0] != 0xFF) {
		return (TRUE);
	}

	return (FALSE);
	
#else
	INT32 err;
	UINT8  *dbuf = static_data_buf  + chip * flash_spec->DataSize * flash_spec->NumPlanes;
    UINT8  *sbuf = static_spare_buf + chip * SPARE_SIZE * flash_spec->NumPlanes;

	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, TRUE);
	if (err)
		return (err);

	// the chip bank selection is not needed. 
	// it is done in flash_read_page()

	flash_read_page(chip, block, 127, 0, flash_spec->SectorsPerPage, dbuf, sbuf, FLASH_AREA_A);

	if (dbuf[4005] != 0xFF) {
		return (TRUE);
	}

	return (FALSE);
#endif
}				/* end of FLM_MLC8G_2_IsBadBlock */


BOOL FLM_MLC8G_2_Is_Multi_OK(UINT16 chip, UINT32 *block, INT32 *flag)
{
    UINT32 i, boundary, actual_planes, group = 0xFFFFFFFF;
    UINT32 __block[MAX_BLOCKS_IN_GROUP]; /* real block number */

    if (flash_spec->NumDiesPerCE == 1) {
        boundary = real_num_blocks;
        actual_planes = flash_spec->NumPlanes;
    }
    else {
        boundary = (real_num_blocks >> 1);
        actual_planes = (flash_spec->NumPlanes >> 1);
    }
    
    for (i = 0; i < flash_spec->NumPlanes; i++) {
        if (!flag[i]) return(FALSE);
        __block[i] = get_pbn(block[i]); 

        if (i < 2) {
            if (__block[i] >= boundary) return(FALSE);
            if ((__block[i] & 0x1) != i) return(FALSE);
            if (group == 0xFFFFFFFF) group = (__block[i] >> BITS[actual_planes]);
            else if (group != (__block[i] >> BITS[actual_planes])) return(FALSE);
        }
        else if (i < 4) {
            if (__block[i] < boundary) return(FALSE);
            if ((__block[i] & 0x1) != (i & 0x1)) return(FALSE);
            if (group != ((__block[i] - boundary) >> BITS[actual_planes])) return(FALSE);
        }
        else return(FALSE);
    }
    
    return(TRUE);
}


/*----------------------------------------------------------------------*/
/*  Local Function Definitions                                          */
/*----------------------------------------------------------------------*/

/* read a page (data or spare) from given block/page
    parameters : block address, page address, buffer, area */
static INT32 flash_read_page(UINT16 chip,
			     UINT32 block,
			     UINT16 page,
			     UINT16 sector_offset,
			     UINT16 num_sectors,
			     UINT8 * data_buf, UINT8 * spare_buf, UINT8 area)
{
	INT32 err = FM_SUCCESS;
	INT32 addr, erased = 0;
	UINT32 ecc_result;
	UINT32 __block;
	UINT32 __page;
	UINT8 *sbuf = static_spare_buf + chip * SPARE_SIZE * flash_spec->NumPlanes;

	SECMLC_dma_serial_read_t *serial_read = LLD_DESC_BUF(chip, 0, SECMLC_dma_serial_read_t);  

#if (TIMING_LLD == 1 ) 
	unsigned start, duration; 
#endif 

#if 0 // hcyun 
	if ( (data_buf >= VMALLOC_START && data_buf < STMP36XX_SRAM_BASE) || 
	     (spare_buf >= VMALLOC_START && spare_buf < STMP36XX_SRAM_BASE) ) {
	    printk("%s: dbuf(0x%x), sbuf(0x%x)\n", 
		   __FUNCTION__, data_buf, spare_buf); 
	    while(1); 
	}
#endif 

	__block = get_pbn(block); 
	__page = page; 
	
	if (data_buf == NULL) {
	    data_buf = static_data_buf + chip * flash_spec->DataSize * flash_spec->NumPlanes;
	}

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, TRUE);
	if (err)
		return (err);
  
#if (TIMING_LLD == 1 ) 
	start = get_usec();
#endif 		

	PDEBUG("%s: \nchip(%d), block(%d), page(%d), sector offset(%d), sector number (%d), \narea (%d)\n", 
	       __FUNCTION__, chip, block, page, sector_offset, num_sectors, (int)area); 

	prev_op[chip].Command = OP_READ; 

	/* read page and optionally read spare with random data output */ 
#if !USE_16GBIT_NAND
	addr = (__block << BLK_SHIFT) + (__page << ROW_SHIFT) + (sector_offset << 9); 
#else 
	addr = (__block << (BLK_SHIFT-4)) + (__page << (ROW_SHIFT-4)) + (sector_offset << (9-4)); 
#endif 

	// PDEBUG("a) main row addr = 0x%08x\n", addr); 
	ddi_nand_hal_ResetECC8();

	SECMLC_build_dma_serial_read
		(serial_read,
		 GET_CS(chip),
		 addr,// offset 
		 (512 * num_sectors),
		 (void *)stmp_virt_to_phys(data_buf), 
		 (void *)stmp_virt_to_phys(sbuf), 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA); 

	rfs_invalidate_dcache_range((unsigned)data_buf, (unsigned)data_buf + (512 * num_sectors)); 
	rfs_invalidate_dcache_range((unsigned)sbuf, (unsigned)sbuf + (16 * num_sectors)); 
			
	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)serial_read);

	// start DMA 
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 1);

	/* 
	   Wait previous DMA. tR(25us) + (tAS + (flash_spec->DataSize + 64) * ( tDS + tDH )) 

	   Maximum Delay 
	   = flash busy time + controller transfer time 
	   = (tR)            + (tAS + (flash_spec->DataSize + 64) * ( tDS + tDH ))  
	   = 125.6us 
	   
	   (*) K9F108U0M  tAS = 0, tDS = 33ns, tDH = 16ns, tR = 20us
	   
	*/ 
	
	err = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 4000 * T_OVERHEAD); 

	if (err && err != APBH_MISSING_IRQ ) { 
		PDEBUG("%s: Read failed. chip(%d), block(%d), page(%d), offset(%d), nsect(%d)\n", 
		       __FUNCTION__, 
		       chip, block, page, sector_offset, num_sectors); 
		lld_stat.read_fail ++; 

		return (FM_READ_ERROR); 
	}
	
#if (TIMING_LLD == 1 ) 
	if ( area == FLASH_AREA_A && num_sectors == 4 ) { 
		duration = get_usec_elapsed(start, get_usec()); 
		lld_perf.read_page.min = MIN(lld_perf.read_page.min, duration); 
		lld_perf.read_page.max = MAX(lld_perf.read_page.max, duration); 
		lld_perf.read_page.cnt ++; 
		lld_perf.read_page.tot += duration; 
	}
#endif 		

    // check ECC8_STATUS0 register
    ecc_result = HW_ECC8_STATUS0_RD();
    if (ecc_result & BM_ECC8_STATUS0_CORRECTED) {
        err = FM_SUCCESS;
    }
    else if (ecc_result & BM_ECC8_STATUS0_UNCORRECTABLE) {
        err = FM_ECC_ERROR;
    }
    else if (ecc_result & BM_ECC8_STATUS0_ALLONES) {
        erased = 1;
        err = FM_SUCCESS;
    }
    
	if (spare_buf != NULL) {
	    if (erased) {
	        memset(spare_buf, 0xFF, flash_spec->SpareSize);
	    }
	    else {
	        memcpy(spare_buf, sbuf, flash_spec->SpareSize);
	    }
	}
	
	return (err);
}				/* end of flash_read_page */

/* program a page (data or spare) to given block/page
    parameters : block address, page address, buffer, area, multi-flag */
static INT32 flash_write_page(UINT16 chip,
			      UINT32 block,
			      UINT16 page,
			      UINT16 sector_offset,
			      UINT16 num_sectors,
			      UINT8 * data_buf,
			      UINT8 * spare_buf, UINT8 area, BOOL is_last)
{
	INT32 err;
	UINT32 addr; 
	UINT8 *sbuf = static_spare_buf + chip * SPARE_SIZE * flash_spec->NumPlanes;

	UINT32 __block;
	UINT32 __page; 

	SECMLC_dma_serial_program_t *program = LLD_DESC_BUF(chip, 0, SECMLC_dma_serial_program_t);
	SECMLC_dma_check_status_t *status =    LLD_DESC_BUF(chip, 1,SECMLC_dma_check_status_t); 
	
	__block = get_pbn(block); 
	__page = page; 

#if 0 // hcyun 
	if ( (data_buf >= VMALLOC_START && data_buf < STMP36XX_SRAM_BASE) || 
	     (spare_buf >= VMALLOC_START && spare_buf < STMP36XX_SRAM_BASE) ) {
	    printk("%s: dbuf(0x%x), sbuf(0x%x)\n", 
		   __FUNCTION__, data_buf, spare_buf); 
	    while(1); 
	}
#endif 

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, FALSE);
	if (err)
		return (err);

	PDEBUG("%s: \nchip(%d), block(%d), page(%d), sector offset(%d), sector number (%d), area (%d),\n pid(%d)\n", 
	       __FUNCTION__, chip, block, page, sector_offset, num_sectors, area, 
	       current->pid); 

    if (data_buf == NULL) {
	    data_buf = static_data_buf + chip * flash_spec->DataSize * flash_spec->NumPlanes;
	    memset(data_buf, 0xFF, flash_spec->DataSize);
	}

    if (spare_buf != NULL) {
        memcpy(sbuf, spare_buf, flash_spec->SpareSize);
    }
    else {
        memset(sbuf, 0xFF, flash_spec->SpareSize);
    }

#if !USE_16GBIT_NAND
	addr = (__block << BLK_SHIFT) + (__page << ROW_SHIFT) + (sector_offset << 9); 
#else
	addr = (__block << (BLK_SHIFT-4)) + (__page << (ROW_SHIFT-4)) + (sector_offset << (9-4)); 
#endif 

    ddi_nand_hal_ResetECC8();

	// build DMA descriptor 
	SECMLC_build_dma_serial_program
		(program,
		 GET_CS(chip), 
		 addr,// offset 
		 (512 * num_sectors), 
		 (void *)stmp_virt_to_phys(data_buf), 
		 (void *)stmp_virt_to_phys(sbuf), 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) status);

	rfs_clean_dcache_range((unsigned int)data_buf, (unsigned int)data_buf + (512 * num_sectors));
	rfs_clean_dcache_range((unsigned int)sbuf, (unsigned int)sbuf + (16 * num_sectors));

	/* append check status command at the end of the program dma chain */ 
	SECMLC_build_dma_check_status
		(status,
		 GET_CS(chip), 
		 0x01, // pass/fail - IO 0 
		 0x00, 
		 (apbh_dma_t*) APBH_FAILURE_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA); 

	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)program);

	// start DMA (program, check status)
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 2);

	ecc_active[chip] = 1;
	ecc_descriptor[chip] = &program->wait_dma;
	ecc_last_chip = chip;
	
	return (FM_SUCCESS);	// program success
}				/* end of flash_write_page */

/* read a page from given block/page for copyback
    parameters : block address, page address, buffer */
static INT32 flash_copyback_read_page(UINT16 chip,
			     UINT32 block,
			     UINT16 page,
			     UINT8 * data_buf,
			     UINT8 * spare_buf,
			     UINT32 * ecc_result)
{
	INT32 err;
	INT32 addr, i;

	UINT32 __block;
	UINT32 __page; 

	SECMLC_dma_copyback_read_t *copyback_read = LLD_DESC_BUF(chip, 0, SECMLC_dma_copyback_read_t);  

#if (TIMING_LLD == 1 ) 
	unsigned start, duration; 
#endif

#if 0 // hcyun 
	if ( page_buf >= VMALLOC_START && page_buf < STMP36XX_SRAM_BASE ) {
	    printk("%s: page_buf(0x%x)\n", __FUNCTION__, page_buf); 
	    while(1); 
	}
#endif 

	__block = get_pbn(block); 
	__page = page; 

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, TRUE);
	if (err) return (err);
  
#if (TIMING_LLD == 1 ) 
	start = get_usec();
#endif 		

	PDEBUG("%s: \nchip(%d), block(%d), page(%d)\n", __FUNCTION__, chip, block, page); 

	prev_op[chip].Command = OP_READ; 

	/* read page and optionally read spare with random data output */ 
#if !USE_16GBIT_NAND
	addr = (__block << BLK_SHIFT) + (__page << ROW_SHIFT); 
#else 
	addr = (__block << (BLK_SHIFT-4)) + (__page << (ROW_SHIFT-4)); 
#endif 

    ddi_nand_hal_ResetECC8();

	SECMLC_build_dma_copyback_read
		(copyback_read,
		 GET_CS(chip),
		 addr,// offset 
		 flash_spec->DataSize,
		 (void *)stmp_virt_to_phys(data_buf), 
		 (void *)stmp_virt_to_phys(spare_buf), 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA); 

	rfs_invalidate_dcache_range((unsigned)data_buf, (unsigned)data_buf + flash_spec->DataSize); 
	rfs_invalidate_dcache_range((unsigned)spare_buf, (unsigned)spare_buf + flash_spec->SpareSize);
		
	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)copyback_read);

	// start DMA 
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 1);

	/* 
	   Wait previous DMA. tR(25us) + (tAS + (flash_spec->DataSize + 64) * ( tDS + tDH )) 

	   Maximum Delay 
	   = flash busy time + controller transfer time 
	   = (tR)            + (tAS + (flash_spec->DataSize + 64) * ( tDS + tDH ))  
	   = 125.6us 
	   
	   (*) K9F108U0M  tAS = 0, tDS = 33ns, tDH = 16ns, tR = 20us
	   
	*/ 
	
	err = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 4000 * T_OVERHEAD); 

	if (err && err != APBH_MISSING_IRQ ) { 
		PDEBUG("%s: Read failed. chip(%d), block(%d), page(%d)\n", 
		       __FUNCTION__, chip, block, page); 
		lld_stat.read_fail ++; 

		return (FM_READ_ERROR); 
	}

#if (TIMING_LLD == 1 ) 
	duration = get_usec_elapsed(start, get_usec()); 
	lld_perf.read_page.min = MIN(lld_perf.read_page.min, duration); 
	lld_perf.read_page.max = MAX(lld_perf.read_page.max, duration); 
	lld_perf.read_page.cnt ++; 
	lld_perf.read_page.tot += duration; 
#endif 		

    // check ECC8_STATUS0 register
    *ecc_result = HW_ECC8_STATUS0_RD();
	return (FM_SUCCESS);
}				/* end of flash_copyback_read_page */


#if 0
/* read a group of pages from given blocks/page for copyback
    parameters : block addresses, page address, buffers */
static INT32 flash_copyback_read_page_two_plane(UINT16 chip, 
                    UINT32 * block, 
                    UINT16 * page, 
                    UINT8 * data_buf[],
                    UINT8 * spare_buf[])
{
    INT32 err, i;
	UINT32 addr[2];

	UINT32 __block[2];
	UINT32 __page[2]; 

	SECMLC_dma_copyback_read_group1_t *copyback_read_group1 =  LLD_DESC_BUF(chip, 0, SECMLC_dma_copyback_read_group1_t);
	SECMLC_dma_copyback_read_group2_t *copyback_read_group21 = LLD_DESC_BUF(chip, 1, SECMLC_dma_copyback_read_group2_t);
	SECMLC_dma_copyback_read_group2_t *copyback_read_group22 = LLD_DESC_BUF(chip, 2, SECMLC_dma_copyback_read_group2_t);  

#if (TIMING_LLD == 1 ) 
	unsigned start, duration; 
#endif

#if 0 // hcyun 
    for (i = 0; i < 2; i++) {
    	if ( page_buf[i] >= VMALLOC_START && page_buf[i] < STMP36XX_SRAM_BASE ) {
    	    printk("%s: page_buf(0x%x)\n", __FUNCTION__, page_buf[i]); 
    	    while(1); 
    	}
    }
#endif 

    for (i = 0; i < 2; i++) {
    	__block[i] = get_pbn(block[i]); 
    	__page[i] = page[i]; 
    }

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, TRUE);
	if (err) return (err);
  
#if (TIMING_LLD == 1 ) 
	start = get_usec();
#endif 		

	PDEBUG("%s: \nchip(%d), blocks(%d, %d), page(%d, %d)\n", __FUNCTION__, chip, block[0], block[1], page[0], page[1]); 

	prev_op[chip].Command = OP_READ_GROUP; 

	/* read page and optionally read spare with random data output */ 
	for (i = 0; i < 2; i++) {
#if !USE_16GBIT_NAND
	    addr[i] = (__block[i] << BLK_SHIFT) + (__page[i] << ROW_SHIFT); 
#else 
	    addr[i] = (__block[i] << (BLK_SHIFT-4)) + (__page[i] << (ROW_SHIFT-4)); 
#endif 
    }

    SECMLC_build_dma_copyback_read_group1
		(copyback_read_group1,
		 GET_CS(chip),
		 addr,// offset 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) copyback_read_group21); 

	SECMLC_build_dma_copyback_read_group2
		(copyback_read_group21,
		 GET_CS(chip),
		 addr[0],// offset 
		 flash_spec->PageSize,
		 (void *)stmp_virt_to_phys(page_buf[0]), 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) copyback_read_group22); 
		 
	SECMLC_build_dma_copyback_read_group2
		(copyback_read_group22,
		 GET_CS(chip),
		 addr[1],// offset 
		 flash_spec->PageSize,
		 (void *)stmp_virt_to_phys(page_buf[1]), 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA); 

	rfs_invalidate_dcache_range((unsigned)page_buf[0], (unsigned)page_buf[0] + flash_spec->PageSize); 
	rfs_invalidate_dcache_range((unsigned)page_buf[1], (unsigned)page_buf[1] + flash_spec->PageSize); 
		
	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)copyback_read_group1);

	// start DMA 
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 4);

	/* 
	   Wait previous DMA. tR(25us) + (tAS + (flash_spec->DataSize + 64) * ( tDS + tDH )) 

	   Maximum Delay 
	   = flash busy time + controller transfer time 
	   = (tR)            + (tAS + (flash_spec->DataSize + 64) * ( tDS + tDH ))  
	   = 125.6us 
	   
	   (*) K9F108U0M  tAS = 0, tDS = 33ns, tDH = 16ns, tR = 20us
	   
	*/ 
	
	err = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 400 * T_OVERHEAD); 

	if (err && err != APBH_MISSING_IRQ ) { 
		PDEBUG("%s: Read failed. chip(%d), block(%d, %d), page(%d, %d)\n", 
		       __FUNCTION__, chip, block[0], block[1], page[0], page[1]); 
		lld_stat.read_fail ++; 

		return (FM_READ_ERROR); 
	}

#if (TIMING_LLD == 1 ) 
	duration = get_usec_elapsed(start, get_usec()); 
	lld_perf.read_page.min = MIN(lld_perf.read_page.min, duration); 
	lld_perf.read_page.max = MAX(lld_perf.read_page.max, duration); 
	lld_perf.read_page.cnt ++; 
	lld_perf.read_page.tot += duration; 
#endif 		
   
	return (err);
}   /* end of flash_copyback_read_page_two_plane */
#endif


/* program a page to given block/page for copyback
    parameters : block address, page address, buffer */
static INT32 flash_copyback_write_page(UINT16 chip,
			      UINT32 block,
			      UINT16 page)
{
	INT32 err;
	UINT32 addr; 

	UINT32 __block;
	UINT32 __page; 

	SECMLC_dma_copyback_program_t *copyback_program = LLD_DESC_BUF(chip, 0, SECMLC_dma_copyback_program_t);
	SECMLC_dma_check_status_t *status =               LLD_DESC_BUF(chip, 1, SECMLC_dma_check_status_t); 
	
	__block = get_pbn(block); 
	__page = page; 

	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, FALSE);
	if (err) return (err);

	PDEBUG("%s: \nchip(%d), block(%d), page(%d), pid(%d)\n", 
	       __FUNCTION__, chip, block, page, current->pid); 

#if !USE_16GBIT_NAND
	addr = (__block << BLK_SHIFT) + (__page << ROW_SHIFT); 
#else
	addr = (__block << (BLK_SHIFT-4)) + (__page << (ROW_SHIFT-4)); 
#endif 

	// build DMA descriptor 
	SECMLC_build_dma_copyback_program
		(copyback_program,
		 GET_CS(chip), 
		 addr,// offset 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) status);

	/* append check status command at the end of the program dma chain */ 
	SECMLC_build_dma_check_status
		(status,
		 GET_CS(chip), 
		 0x01, // pass/fail - IO 0 
		 0x00, 
		 (apbh_dma_t*) APBH_FAILURE_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA); 


	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)copyback_program);

	// start DMA
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 2);

	return (FM_SUCCESS);	// program success
}				/* end of flash_copyback_write_page */

/* program a group of pages to given blocks/page for copyback
    parameters : block addresses, page address, buffers */
static INT32 flash_copyback_write_page_two_plane(UINT16 chip,
			      UINT32 * block,
			      UINT16 page)
{
    INT32 err, i;
	UINT32 addr[2]; 

	UINT32 __block[2];
	UINT32 __page[2]; 

	SECMLC_dma_copyback_program_t *copyback_program1 = LLD_DESC_BUF(chip, 0, SECMLC_dma_copyback_program_t);
	SECMLC_dma_copyback_program_t *copyback_program2 = LLD_DESC_BUF(chip, 1, SECMLC_dma_copyback_program_t);
	SECMLC_dma_check_status_t *status =                LLD_DESC_BUF(chip, 2, SECMLC_dma_check_status_t); 
	
	for (i = 0; i < 2; i++) {
    	__block[i] = get_pbn(block[i]); 
    	__page[i] = page; 
    }
	
	// select the chip bank
	flash_select_channel(chip);

	/* sync previous operation */
	err = flash_sync(chip, FALSE);
	if (err) return (err);

	PDEBUG("%s: \nchip(%d), block(%d, %d), page(%d), pid(%d)\n", 
	       __FUNCTION__, chip, block[0], block[1], page, current->pid); 

    for (i = 0; i < 2; i++) {
#if !USE_16GBIT_NAND
	    addr[i] = (__block[i] << BLK_SHIFT) + (__page[i] << ROW_SHIFT); 
#else 
	    addr[i] = (__block[i] << (BLK_SHIFT-4)) + (__page[i] << (ROW_SHIFT-4)); 
#endif 
    }

	// build DMA descriptor 
	SECMLC_build_dma_copyback_program_group1
		(copyback_program1,
		 GET_CS(chip), 
		 addr[0],// offset 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) copyback_program2);
	
	SECMLC_build_dma_copyback_program_group2
		(copyback_program2,
		 GET_CS(chip), 
		 addr[1],// offset 
		 (apbh_dma_t*) APBH_TIMEOUT_DMA,
		 (apbh_dma_t*) status);

	/* append check status command at the end of the program dma chain */ 
	SECMLC_build_dma_check_status
		(status,
		 GET_CS(chip), 
		 0x01, // pass/fail - IO 0 
		 0x00, 
		 (apbh_dma_t*) APBH_FAILURE_DMA,
		 (apbh_dma_t*) APBH_SUCCESS_DMA); 

	// setup DMA 
	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)copyback_program1);

	// start DMA
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 2);

	return (FM_SUCCESS);	// program success
}   /* end of flash_copyback_write_page_two_plane */


/* flash_sync() should check not only the given chip number but also 
   the other chip which has same chip select in the oposite bank,
   since the extra-bank selection mechanism in stmp36xx allows the same
   chip selection and state machine in the NAND control logic */
static INT32 __flash_sync(UINT16 chip, BOOL sync_cache_prog)
{
	int ret; 

	if ((prev_op[chip].Command & OP_SYNC_MASK)) {
		prev_op[chip].Command &= ~((UINT16)OP_SYNC_MASK);
		return (prev_op[chip].Result);
	}

	if ( prev_op[chip].Command == OP_NONE)
		return (FM_SUCCESS); 

	if (prev_op[chip].Command == OP_READ ) {
		prev_op[chip].Command = OP_NONE;
		return (FM_SUCCESS); 
	}

	if (prev_op[chip].Command == OP_ERASE) {
		// Wait previous DMA. tBERS(3ms) + tAS + tDS + tDH  
		ret = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 2100 * T_OVERHEAD); 

		if ( ret && ret != APBH_MISSING_IRQ ) { 
			printk("%s:Erase failed. chip(%d), block(%d)\n", 
			       __FUNCTION__, chip, prev_op[chip].Param.Erase.Block ); 
			lld_stat.erase_fail++;
			return (FM_PREV_ERASE_ERROR); 				
		}

	} else if (prev_op[chip].Command == OP_ERASE_GROUP) {
		// Wait previous DMA. tBERS(3ms) + tAS + tDS + tDH  
		ret = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 2100 * T_OVERHEAD); 

		if ( ret && ret != APBH_MISSING_IRQ ) { 
			printk("%s: Erase Group failed. chip(%d), block1(%d), block2(%d)\n", 
			       __FUNCTION__, 
			       chip, prev_op[chip].Param.EraseGroup.Block[0],
			       prev_op[chip].Param.EraseGroup.Block[1]); 
			lld_stat.erase_group_fail ++; 
			return (FM_PREV_ERASE_GROUP_ERROR); 
		}

	} else if (prev_op[chip].Command == OP_WRITE) {
		// Wait previous DMA. maximum Transfer(200us) + tPROG(700us)
		ret = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 4000 * T_OVERHEAD); 
		ecc_active[chip] = 0;
		
		if ( ret && ret != APBH_MISSING_IRQ ) { 
			printk("%s: Write failed. chip(%d), block(%d), page(%d), offset(%d), nsect(%d), last(%d)\n", 
			       __FUNCTION__, 
			       chip, 
			       prev_op[chip].Param.Write.Block,
			       prev_op[chip].Param.Write.Page,
			       prev_op[chip].Param.Write.SectorOffset,
			       prev_op[chip].Param.Write.NumSectors, 
			       prev_op[chip].Param.Write.IsLast); 
			lld_stat.write_fail++;
			return (FM_PREV_WRITE_ERROR); 
		}

	} else if (prev_op[chip].Command == OP_WRITE_GROUP) {

		// Wait previous DMA. maximum Transfer(200us)*2 + tDBSY(1) + tPROG(700us)
		ret = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 1100 * T_OVERHEAD); 
		ecc_active[chip] = 0;

		if ( ret && ret != APBH_MISSING_IRQ ) { 
			printk("%s: Write Group failed. chip(%d), block1(%d), block2(%d), block3(%d), block4(%d), page(%d)\n", 
			       __FUNCTION__, 
			       chip, 
			       prev_op[chip].Param.WriteGroup.Block[0],
			       prev_op[chip].Param.WriteGroup.Block[1],
			       prev_op[chip].Param.WriteGroup.Block[2],
			       prev_op[chip].Param.WriteGroup.Block[3],
			       prev_op[chip].Param.WriteGroup.Page); 
			lld_stat.write_group_fail++; 
			return (FM_PREV_WRITE_GROUP_ERROR); 
		}
	
	} else if (prev_op[chip].Command == OP_COPYBACK) {
		// Wait previous DMA. maximum possible Transfer(200us) + tPROG(700us)
		ret = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 900 * T_OVERHEAD); 
		if ( ret && ret != APBH_MISSING_IRQ ) { 
			printk("%s: Copyback failed. chip(%d), src_block(%d), src_page(%d), dest_block(%d), dest_page(%d)\n", 
			       __FUNCTION__, 
			       chip, 
			       prev_op[chip].Param.CopyBack.SrcBlock,
			       prev_op[chip].Param.CopyBack.SrcPage,
			       prev_op[chip].Param.CopyBack.DestBlock,
			       prev_op[chip].Param.CopyBack.DestPage); 
			lld_stat.copyback_fail++;
			return (FM_PREV_WRITE_ERROR); 
		}

	} else if (prev_op[chip].Command == OP_COPYBACK_GROUP) {

		// Wait previous DMA. maximum possible Transfer(200us)*2 + tDBSY(1) + tPROG(700us)
		ret = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 1100 * T_OVERHEAD); 

		if ( ret && ret != APBH_MISSING_IRQ ) { 
			printk("%s: Copyback Group failed. chip(%d), src_block1(%d), src_block2(%d), src_page1(%d), src_page2(%d)"
			                                            "dest_block1(%d), dest_block2(%d), dest_page(%d)\n", 
			       __FUNCTION__, 
			       chip, 
			       prev_op[chip].Param.CopyBackGroup.SrcBlock[0],
			       prev_op[chip].Param.CopyBackGroup.SrcBlock[1],
			       prev_op[chip].Param.CopyBackGroup.SrcPage[0],
			       prev_op[chip].Param.CopyBackGroup.SrcPage[1],
			       prev_op[chip].Param.CopyBackGroup.DestBlock[0],
			       prev_op[chip].Param.CopyBackGroup.DestBlock[1],
			       prev_op[chip].Param.CopyBackGroup.DestPage); 
			lld_stat.copyback_group_fail++; 
			return (FM_PREV_WRITE_GROUP_ERROR); 
		}
	}
	
	prev_op[chip].Command = OP_NONE;
	return (FM_SUCCESS);
}


static INT32 flash_sync(UINT16 chip, BOOL sync_cache_prog)
{
#if 0
	int i, ret;
	/* sync all due to 3700 ECC dma chain crash issue */
	for (i = 0; i < MAX_NUM_OF_CHIPS; i++) {
		if ((ret = __flash_sync(i, sync_cache_prog)) != FM_SUCCESS)
			return ret;
	}
	return FM_SUCCESS;
#else
    /* 4 DMA channels can be used in parallel on 37xx, but be careful
       not to corrupt on-going ECC operation on other channels */
    int ret;
    unsigned curar;
    
    if ((ret = __flash_sync(chip, sync_cache_prog)) != FM_SUCCESS)
        return ret;
    
    if (ecc_active[ecc_last_chip]) {
        while (1) {
            curar = stmp_phys_to_virt(HW_APBH_CHn_CURCMDAR_RD(GPMI_APBH_CHANNEL_IN_FLM(ecc_last_chip)));
            /* check if the on-going DMA chain has executed ECC part already */
            if (curar >= (unsigned)ecc_descriptor[ecc_last_chip]) break;
            udelay(5);
        }
    }
    
    return FM_SUCCESS;
#endif
}


#if 0 
static UINT8 flash_read_status(int chip)
{
	SECMLC_dma_read_status_t *status = &read_status_chain[chip % MAX_NUM_OF_DMAS];
	int ret; 

	// no need to select chip bank
	// the corresponding chip bank must be selected already
	// whenever the flash_read_status() is called.

	SECMLC_build_dma_read_status
		( status, 
		  chip, 
		  (void *)dma_buf_paddr, 
		  (apbh_dma_t*) APBH_SUCCESS_DMA); 



	apbh_setup_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), (apbh_dma_t *)status);
	apbh_start_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 2);		

	rfs_invalidate_dcache_range((unsigned int)dma_buf, (unsigned int)dma_buf+4);
	ret = apbh_wait_channel(GPMI_APBH_CHANNEL_IN_FLM(chip), 10000); /* wait 10ms */ 
	
	if ( dma_buf[0] & 0x01 ) 
		printk("I/O 0 Fail\n"); 
	
	if ( dma_buf[0] & 0x20 ) 
		printk("I/O 5 Ready\n"); 
	
	if ( dma_buf[0] & 0x40 ) 
		printk("I/O 6 Ready\n"); 
	
	return dma_buf[0]; 
}
#endif 


static int 
lld_timing_read_proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0; 

	timing.tAS = HW_GPMI_TIMING0.B.ADDRESS_SETUP; 
	timing.tDS = HW_GPMI_TIMING0.B.DATA_SETUP; 
	timing.tDH = HW_GPMI_TIMING0.B.DATA_HOLD; 

	len += sprintf(buf + len, "tAS = %d\n", timing.tAS); 
	len += sprintf(buf + len, "tDS = %d\n", timing.tDS); 
	len += sprintf(buf + len, "tDH = %d\n", timing.tDH); 
	len += sprintf(buf + len, "timeout = %d\n", timing.timeout); 
	
	len += sprintf(buf + len, "tR = %d\n", timing.tR); 
	len += sprintf(buf + len, "tPROG = %d\n", timing.tPROG); 
	len += sprintf(buf + len, "tCBSY = %d\n", timing.tCBSY); 
	len += sprintf(buf + len, "tBERS = %d\n\n", timing.tBERS); 
	len += sprintf(buf + len, "Usage: $ echo \"<tAS> <tDS> <tDH> <timeout> \" > /proc/lld/timing\n\n"); 

	
	*eof = 1; 
	return len; 
}

static ssize_t 
lld_timing_write_proc(struct file * file, const char * buf, 
	       unsigned long count, void *data)
{
	char cmd[80]; 
	int val;

	sscanf(buf, "%s %d", cmd, &val); 

	if ( !strcmp(cmd, "tAS") ) 
		timing.tAS = val; 
	else if ( !strcmp(cmd, "tDS") ) 
		timing.tDS = val;
	else if ( !strcmp(cmd, "tDH") ) 
		timing.tDH = val; 
	else if ( !strcmp(cmd, "tPROG" ) )
		timing.tPROG = val; 
	else if ( !strcmp(cmd, "tBERS" ) ) 
		timing.tBERS = val; 
	else if ( !strcmp(cmd, "tR" ) ) 
		timing.tR = val; 
	else {
		printk("Invalid command\n"); 
		return count; 
	}

	BF_CS3(GPMI_TIMING0, 
	       ADDRESS_SETUP, timing.tAS, 
	       DATA_SETUP, timing.tDS, 
	       DATA_HOLD, timing.tDH); 
	BW_GPMI_TIMING1_DEVICE_BUSY_TIMEOUT(timing.timeout); 

	printk("Current Timing : TAS(%d), TDS(%d), TDH(%d) Timeout(%d)\n", 
	       timing.tAS, timing.tDS, timing.tDH, timing.timeout); 

	printk("Current Timing : tR(%d), tPROG(%d), tBERS(%d)\n", 
	       timing.tR, timing.tPROG, timing.tBERS); 

	return count; 
}

static int 
lld_test_read_proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0; 

	len += sprintf(buf + len, "\n[LLD] performance results.\n"); 

	// TODO 

	len += sprintf(buf + len, "Usage: $ echo \"[func|perf|hwecc|format]\" > /proc/lld/test\n"); 

	*eof = 1; 
	return len; 
}

static ssize_t 
lld_test_write_proc(struct file * file, const char * buf, 
		    unsigned long count, void *data)
{	
#if 0
	if ( !strncmp(buf, "read", 4) ) 
		sync_read_test();
	else if ( !strncmp(buf, "write", 5) ) {
		int chip, start, end;

		sscanf(buf + 5, "%d %d %d", &chip, &start, &end);
		two_plane_write_test(chip, start, end); /* non-interleave, interleave */ 
	}
	else if ( !strncmp(buf, "badblks", 6) ) 
		check_badblks(); 
#endif

	return count; 
}


static int 
lld_dump_read_proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0; 

	len += sprintf(buf + len, "\n[LLD] dump page content .\n"); 
	len += sprintf(buf + len, "Usage: $ echo \"<chip> <block> <page> <offset> <nsect> <layout>\" > /proc/lld/dump\n"); 

	*eof = 1; 
	return len; 
}

static ssize_t 
lld_dump_write_proc(struct file * file, const char * buf, 
		    unsigned long count, void *data)
{
	int chip, block, page, offset, nsect; 
	unsigned char dbuf[4096], sbuf[128]; 
	int layout;
	int ret; 
	int i;

	sscanf(buf, "%d %d %d %d %d %d", &chip, &block, &page, &offset, &nsect, &layout); 
	
	memset(dbuf, 0xff, flash_spec->DataSize); 
	memset(sbuf, 0xff, flash_spec->SpareSize); 

	ret = FLM_MLC8G_2_Read_Page(chip, block, page, offset, nsect, dbuf, sbuf); 

	printk("Contents of chip(%d), block(%d), page(%d), offset(%d), nsect(%d)\n", 
	       chip, block, page, offset, nsect); 
	printk("\nData Area : \n");  
	for ( i = 0; i < nsect * 512; i++ ) {
		if ( i % 16 == 0 ) printk("\n%3x: ", i);
		printk("%3x", dbuf[i]);
	}
	printk("\n");
	printk("\nSpare Area\n");
	for ( i = 0; i < nsect * 16; i++ ) {
		if ( i % 16 == 0 ) printk("\n%3x: ", i);
		printk("%3x", sbuf[i]);
	}
	printk("\n");

	flush_dcache_range((unsigned)sbuf, (unsigned)sbuf + flash_spec->SpareSize);
	invalidate_dcache_range((unsigned)sbuf, (unsigned)sbuf + flash_spec->SpareSize);

	printk("\nCorrect ECC data must be as below\n"); 
	printk("\nSpare Area\n");
	for ( i = 0; i < nsect * 16; i++ ) {
		if ( i % 16 == 0 ) printk("\n%3x: ", i);
		printk("%3x", sbuf[i]);
	}
	printk("\n");

	return count; 
}

static int 
lld_debug_read_proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0; 


	len += sprintf(buf + len, "\n[LLD] current debug level = %d\n", lld_debug); 
	len += sprintf(buf + len, "Usage: $ echo \"<[debug <debug level> | reset ]\" > /proc/lld/debug\n"); 
	len += sprintf(buf + len, "0 - None, 1 - Critical, 2 - Verbose\n"); 

	len += sprintf(buf + len, "\nMISC informatino\n=======================\n"); 
	len += sprintf(buf + len, "LLD Descriptor size : %d\n", lld_stat.lld_desc_size); 
	len += sprintf(buf + len, "APBH Descriptor size : %d\n", lld_stat.apbh_desc_size); 
	len += sprintf(buf + len, "ECC Descriptor size : %d\n", lld_stat.ecc_desc_size); 
	len += sprintf(buf + len, "ECC Buffer size : %d\n", lld_stat.ecc_buf_size); 

#if TIMING_LLD 
	len += sprintf(buf + len, "\n\nLLD Timing \n=======================\n"); 
	len += sprintf(buf + len, "read_page : min(%u) max(%u) cnt(%u) avg(%u)\n", 
		       lld_perf.read_page.min, lld_perf.read_page.max, lld_perf.read_page.cnt, 
		       (lld_perf.read_page.cnt) ? lld_perf.read_page.tot / lld_perf.read_page.cnt : 0); 
	len += sprintf(buf + len, "write_page_group : min(%u) max(%u) cnt(%u) avg(%u)\n", 
		       lld_perf.write_page_group.min, lld_perf.write_page_group.max, 
		       lld_perf.write_page_group.cnt, 
		       (lld_perf.write_page_group.cnt) ? 
		       lld_perf.write_page_group.tot / lld_perf.write_page_group.cnt : 0); 

	len += sprintf(buf + len, "erase_group : min(%u) max(%u) cnt(%u) avg(%u)\n\n", 
		       lld_perf.erase_group.min, lld_perf.erase_group.max, 
		       lld_perf.erase_group.cnt, 
		       (lld_perf.erase_group.cnt) ? 
		       lld_perf.erase_group.tot / lld_perf.erase_group.cnt : 0); 
#endif 

	len += sprintf(buf + len, "\nLLD Statistics\n=======================\n"); 

	len += sprintf(buf + len, ">> General \n"); 
#if USE_LLD_ISR
	len += sprintf(buf + len, "GPMI correct isr count = %d\n", gpmi_isr_ok); 
	len += sprintf(buf + len, "GPMI error isr count = %d\n", gpmi_isr_err); 
#else 
	len += sprintf(buf + len, "nand_fail : %d\n", lld_stat.nand_fail); 
	len += sprintf(buf + len, "timeout : %d\n", lld_stat.timeout); 
	len += sprintf(buf + len, "missing irq : %d\n", lld_stat.missing_irq); 
#endif /* USE_LLD_ISR */ 
	
	len += sprintf(buf + len, "\n>> Single operation failures \n"); 
	len += sprintf(buf + len, "read : %d/%d\n", lld_stat.read_fail, lld_stat.read + lld_stat.read_group); 
	len += sprintf(buf + len, "write : %d/%d\n", lld_stat.write_fail, lld_stat.write); 
	len += sprintf(buf + len, "erase : %d/%d\n", lld_stat.erase_fail, lld_stat.erase); 
	len += sprintf(buf + len, "copyback : %d/%d\n", lld_stat.copyback_fail, lld_stat.copyback); 

	len += sprintf(buf + len, "\n>> Group operation failures \n"); 
	len += sprintf(buf + len, "write_grp : %d/%d\n", 
		       lld_stat.write_group_fail, lld_stat.write_group); 
	len += sprintf(buf + len, "erase_grp : %d/%d\n", 
		       lld_stat.erase_group_fail, lld_stat.erase_group); 
	len += sprintf(buf + len, "copback_grp : %d/%d\n", 
			   lld_stat.copyback_group_fail, lld_stat.copyback_group); 

	len += sprintf(buf + len, "\n>> ECC Errors\n"); 
	len += sprintf(buf + len, "correctable ecc err : %d\n", lld_stat.corr_ecc_err); 
	len += sprintf(buf + len, "uncorrectable ecc err : %d\n", lld_stat.uncorr_ecc_err); 

	*eof = 1; 
	return len; 
}

static ssize_t 
lld_debug_write_proc(struct file * file, const char * buf, 
		     unsigned long count, void *data)
{
	if ( !strncmp(buf, "reset", 5) ) {
		printk("Reset performance \n"); 

		memset(&lld_stat, 0, sizeof(lld_stat)); 

#if TIMING_LLD
		memset(&lld_perf, 0, sizeof(lld_perf_t)); 
		lld_perf.erase_group.min = lld_perf.write_page_group.min = lld_perf.read_page.min = 0xffffffff; 
#endif 

		return count;
	} else if ( !strncmp(buf, "debug", 5) ) {
		sscanf(buf + 6, "%d", &lld_debug);
	}

	return count; 
}

static int 
lld_nandsb_read_proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0; 

	len += sprintf(buf + len, "Usage: $ echo \"<file size in bytes>\" > /proc/lld/nandsb\n"); 

	*eof = 1; 
	return len; 
}

static ssize_t 
lld_nandsb_write_proc(struct file * file, const char * buf, 
		     unsigned long count, void *data)
{
	int size, nsect; 

	extern int nand_writesb(unsigned char *, int ); 

	sscanf(buf, "%d",  &size); 

	if ( size % 512 ) {
		printk("ERROR: %d must be multiple of 512\n", size); 
	}
	
	nsect = size / 512; 
	printk("%s: program %d sectors of nandsb boot image\n", __FUNCTION__, nsect); 

	//nand_writesb((u8 *)STMP36XX_SRAM_AUDIO_USER, nsect);
	
	return count; 
}

static int 
lld_type_read_proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0; 
	len += sprintf(buf + len, "MLC %dk", flash_spec->DataSize/1024); 

	*eof = 1; 
	return len; 
}

static int 
lld_stress_test_read_proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0; 

	len += sprintf(buf + len, "\n[LLD] stress test...\n"); 
	len += sprintf(buf + len, "Usage: $ echo \"<chip> <start_block> <num_blocks> <repeat_count>\" > /proc/lld/stress_test\n"); 

	*eof = 1; 
	return len; 
}

static ssize_t 
lld_stress_test_write_proc(struct file * file, const char * buf, 
		    unsigned long count, void *data)
{
    unsigned int chip, start_block, num_blocks, round;

    sscanf(buf, "%d %d %d %d", &chip, &start_block, &num_blocks, &round);
    lld_test((UINT16)chip, start_block, num_blocks, (UINT16)round);

	return count; 
}


static int chk_ufdraw_result = 0;

static int 
lld_chk_ufdraw_read_proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0; 

	len += sprintf(buf + len, "%d", chk_ufdraw_result);

	*eof = 1; 
	return len; 
}

static INT32 
is_all_FF(UINT8 *page, INT32 size)
{
    INT32 i;

    for (i = 0; i < size; i++, page++) {
        if (*page != (UINT8) 0xFF) return(FALSE);
    }
    return(TRUE);
}

static ssize_t 
lld_chk_ufdraw_write_proc(struct file * file, const char * buf, 
		    unsigned long count, void *data)
{
    INT32  err;
    UINT32 ufdraw_idx, img_size;
    UINT32 dev_id, b, p, num_pages;
    UINT32 all_FF_page;
    FLASH_SPEC spec;
    
    static UINT32 __dbuf[4096/4], __sbuf[128/4];
    UINT8 *dbuf = (UINT8 *)__dbuf;
    UINT8 *sbuf = (UINT8 *)__sbuf;
    
    sscanf(buf, "%d %d", &ufdraw_idx, &img_size);
    
    dev_id = SET_DEV_ID(LFD_BLK_DEVICE_RAW, SET_RAWDEV_SERIAL(RAW_BLKDEV_TABLE, ufdraw_idx));
    err = FD_Open(dev_id);
    if (err) {
        //printk("%s: FD_Open() failed for 'ufdraw%c'\n", __FUNCTION__, 'a'+ufdraw_idx);
        goto error2;
    }
        
    err = FD_GetDeviceInfo(dev_id, &spec);
    if (err) {
        //printk("%s: FD_GetDeviceInfo() failed for 'ufdraw%c'\n", __FUNCTION__, 'a'+ufdraw_idx);
        goto error1;
    }
    
    num_pages = (img_size + spec.DataSize - 1) / spec.DataSize;
    
    //printk("<Check Result for 'ufdraw%c'>\n", 'a'+ufdraw_idx);
    //printk("-----------------------------------------------\n");
    
    for (b = 0; b < spec.NumBlocks; b++) {
        //printk("%04d: ", b);
        //all_FF_page = 0;
        for (p = 0; p < spec.PagesPerBlock; p++) {
            err = FD_ReadPage(dev_id, b, p, 0, spec.SectorsPerPage, dbuf, sbuf);
            if (err) {
                //printk("\n%s: FD_ReadPage() error (block=%d, page=%d, err=%d)\n", 
                //       __FUNCTION__, b, p, err);
                goto error1;
            }
            //if (is_all_FF(dbuf, spec.DataSize)) all_FF_page++;
            if (is_all_FF(dbuf, spec.DataSize)) {
                //printk("\n%s: all FF page found for 'ufdraw%c' (block=%d, page=%d) !!\n", 
                //       __FUNCTION__, 'a'+ufdraw_idx, b, p);
                goto error1;
            }
            num_pages--;
            if (num_pages == 0) goto end;
        }
        //if (all_FF_page == 128) printk("     -");
        //else printk("   %3d", all_FF_page);
        //printk("\n");
    }
    //printk("\n");

end:
    FD_Close(dev_id);
    chk_ufdraw_result = 1;
	return count; 

error1:
    FD_Close(dev_id);

error2:
    chk_ufdraw_result = 0;
	return count; 
}


BOOL usb_in_msc = 0;
BOOL usb_in_mtp = 0;

EXPORT_SYMBOL(usb_in_msc);
EXPORT_SYMBOL(usb_in_mtp);

static ssize_t 
lld_usb_mode_write_proc(struct file * file, const char * buf, 
		          unsigned long count, void *data)
{
    char usb_mode[32];

    sscanf(buf, "%s", usb_mode);
    
    if (strcmp(usb_mode, "msc") == 0) usb_in_msc = 1;
    else usb_in_msc = 0;
    
    if (strcmp(usb_mode, "mtp") == 0) usb_in_mtp = 1;
    else usb_in_mtp = 0;

	return count; 
}

static int init_lld_proc(void)
{
	struct proc_dir_entry *proc_lld_dir = NULL;
	struct proc_dir_entry *part_root; 

	// Register proc entry 
	proc_lld_dir = proc_mkdir("lld", 0);

	part_root = create_proc_entry("timing", S_IWUSR | S_IRUGO, proc_lld_dir); 
	part_root->read_proc = lld_timing_read_proc; 
	part_root->write_proc = lld_timing_write_proc; 

	part_root = create_proc_entry("test", S_IWUSR | S_IRUGO, proc_lld_dir); 
	part_root->read_proc = lld_test_read_proc; 
	part_root->write_proc = lld_test_write_proc; 

	part_root = create_proc_entry("dump", S_IWUSR | S_IRUGO, proc_lld_dir); 
	part_root->read_proc = lld_dump_read_proc; 
	part_root->write_proc = lld_dump_write_proc; 

	part_root = create_proc_entry("debug", S_IWUSR | S_IRUGO, proc_lld_dir); 
	part_root->read_proc = lld_debug_read_proc; 
	part_root->write_proc = lld_debug_write_proc; 

	part_root = create_proc_entry("nandsb", S_IWUSR | S_IRUGO, proc_lld_dir); 
	part_root->read_proc = lld_nandsb_read_proc; 
	part_root->write_proc = lld_nandsb_write_proc; 

	part_root = create_proc_read_entry("type", 0, proc_lld_dir, lld_type_read_proc, NULL ); 
	
	part_root = create_proc_entry("stress_test", S_IWUSR | S_IRUGO, proc_lld_dir); 
	part_root->read_proc = lld_stress_test_read_proc; 
	part_root->write_proc = lld_stress_test_write_proc; 

	part_root = create_proc_entry("chk_ufdraw", S_IWUSR | S_IRUGO, proc_lld_dir); 
	part_root->read_proc = lld_chk_ufdraw_read_proc; 
	part_root->write_proc = lld_chk_ufdraw_write_proc;

	part_root = create_proc_entry("usb_mode", S_IWUSR | S_IRUGO, proc_lld_dir); 
	part_root->write_proc = lld_usb_mode_write_proc;

	return 0; 
}

static void cleanup_lld(void)
{
	remove_proc_entry("lld", 0); 
}


#define USE_EXPLICIT_SYNC 0

static void lld_test(UINT16 chip, UINT32 start_block, UINT32 num_blocks, UINT16 round)
{
    int bad, ret, i, j, k, l, n = 1, wbuf_index = 0;
    int erase_errcnt = 0, write_errcnt = 0, read_errcnt = 0, verify_errcnt = 0;
    static unsigned char wbuf[2][4096 + 128];
    static unsigned char rbuf[4096 + 128];
    unsigned char *dbuf_group[4], *sbuf_group[4];
    UINT32 block[4];
    INT32 flag[4];

    printk("[LLD] Stress Test starts...\n");
    
    for (i = 0; i < flash_spec->DataSize; i++) {
        wbuf[0][i] = (unsigned char)i;
        wbuf[1][i] = (unsigned char)i;
    }
    for (; i < flash_spec->PageSize; i++) {
        wbuf[0][i] = 0xff;
        wbuf[1][i] = 0xff;
    }

again:    
    printk("\n==============================================\n");
    printk("    Stress Test Round %d\n", n);
    printk("==============================================\n\n");
    
    printk("Phase 1: filling data ...\n\n");

    for (i = start_block; i < start_block + num_blocks; i += flash_spec->NumPlanes) {
        printk("Block %d", i);
        for (l = 1; l < flash_spec->NumPlanes; l++) printk(", %d", i+l);
        printk(" ... ");
        
        // bad block check
#if 1
        bad = 0;
        for (l = 0; l < flash_spec->NumPlanes; l++) {
            if (FLM_MLC8G_2_IsBadBlock(chip, i+l)) {
                printk("!!! Bad block !!!\n");
                bad = 1;
            }
        }
        if (bad) continue;
#endif

        printk("erase ... ");
        
        if (flash_spec->NumPlanes == 1) {
            
            // erase block
            if (FLM_MLC8G_2_Erase(chip, i)) {
                printk("FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }

#if USE_EXPLICIT_SYNC
            if (FLM_MLC8G_2_Sync(chip)) {
                printk("sync FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }
#endif
        }
        else {

            // erase block group
            for (l = 0; l < flash_spec->NumPlanes; l++) {
                block[l] = i + l;
                flag[l] = 1;
            }
            if (FLM_MLC8G_2_Erase_Group(chip, block, flag)) {
                printk("FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }

#if USE_EXPLICIT_SYNC
            if (FLM_MLC8G_2_Sync(chip)) {
                printk("sync FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }
#endif
        }
        
        printk("write ... ");
        
        if (flash_spec->NumPlanes == 1) {
            
            // write pages in the block
            for (j = 0; j < flash_spec->PagesPerBlock; j++) {
                
                wbuf_index = 1 - wbuf_index;
                
                for (k = 0; k < flash_spec->SectorsPerPage; k++) {
                    *((UINT32 *)(wbuf[wbuf_index] + 512*k)) = i;
                    *((UINT32 *)(wbuf[wbuf_index] + 512*k + 4)) = (UINT32)j;
                }
            
                ret = FLM_MLC8G_2_Write_Page(chip, i, j, 
                                             0, flash_spec->SectorsPerPage, 
                                             wbuf[wbuf_index], 
                                             wbuf[wbuf_index] + flash_spec->DataSize, 1);
                if (ret) {
                    printk("FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    write_errcnt++;
                    continue;
                }

#if USE_EXPLICIT_SYNC
                ret = FLM_MLC8G_2_Sync(chip);
                if (ret) {
                    printk("sync FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    write_errcnt++;
                    continue;
                }
#endif
            }
        }
        else {
            
            // write pages in the block group
            for (j = 0; j < flash_spec->PagesPerBlock; j++) {
                
                wbuf_index = 1 - wbuf_index;
                
                for (k = 0; k < flash_spec->SectorsPerPage; k++) {
                    *((UINT32 *)(wbuf[wbuf_index] + 512*k)) = i;
                    *((UINT32 *)(wbuf[wbuf_index] + 512*k + 4)) = (UINT32)j;
                }

                for (l = 0; l < flash_spec->NumPlanes; l++) {
                    block[l] = i + l;
                    dbuf_group[l] = wbuf[wbuf_index]; 
                    sbuf_group[l] = wbuf[wbuf_index] + flash_spec->DataSize;
                    flag[l] = 1;
                }
                
                ret = FLM_MLC8G_2_Write_Page_Group(chip, block, j, 
                                                   dbuf_group, sbuf_group, 
                                                   flag, 1);
                if (ret) {
                    printk("FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    write_errcnt++;
                    continue;
                }

#if USE_EXPLICIT_SYNC
                ret = FLM_MLC8G_2_Sync(chip);
                if (ret) {
                    printk("sync FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    write_errcnt++;
                    continue;
                }
#endif
            }
        }
        
        printk("Done.\n");
    }
    
    printk("\nPhase 2: verifying data ...\n\n");

    for (i = start_block; i < start_block + num_blocks; i += flash_spec->NumPlanes) {
        printk("Block %d", i);
        for (l = 1; l < flash_spec->NumPlanes; l++) printk(", %d", i+l);
        printk(" ... ");

        // bad block check
#if 1
        bad = 0;
        for (l = 0; l < flash_spec->NumPlanes; l++) {
            if (FLM_MLC8G_2_IsBadBlock(chip, i+l)) {
                printk("!!! Bad block !!!\n");
                bad = 1;
            }
        }
        if (bad) continue;
#endif

        printk("read & verify ... ");

        // read pages in the block
        for (l = 0; l < flash_spec->NumPlanes; l++) {
            for (j = 0; j < flash_spec->PagesPerBlock; j++) {              
                ret = FLM_MLC8G_2_Read_Page(chip, i + l, j, 
                                            0, flash_spec->SectorsPerPage, 
                                            rbuf, rbuf + flash_spec->DataSize);
                if (ret) {
                    printk("read FAILED for block %d, page %d (error=%d).\n", i + l, j, ret);
                    prev_op[chip].Command = OP_NONE;
                    read_errcnt++;
                    continue;
                }
                
                for (k = 0; k < flash_spec->SectorsPerPage; k++) {
                    *((UINT32 *)(wbuf[wbuf_index] + 512*k)) = i;
                    *((UINT32 *)(wbuf[wbuf_index] + 512*k + 4)) = (UINT32)j;
                }

                if (memcmp(rbuf, wbuf[wbuf_index], flash_spec->DataSize)) {
                    printk("verify FAILED for block %d, page %d.\n", i + l, j);
                    verify_errcnt++;
#if 0
                    printk("Data dump for block %d, page %d:\n", i, j);
                    for (k = 0; k < flash_spec->PageSize; k++) {
                        if (k == flash_spec->DataSize) printk("\n");
                        if (k % 16 == 0) printk("%04x: ", k);
                        if (k % 8 == 0) printk(" ");
                        printk("%02x ", rbuf[k]);
                        if (k % 16 == 15) printk("\n");
                    }
#endif
                    continue;
                }
            }
        }
        
        printk("Done.\n");
    }
    
    printk("\n");
    printk("# of erase  errors = %d\n", erase_errcnt);
    printk("# of write  errors = %d\n", write_errcnt);
    printk("# of read   errors = %d\n", read_errcnt);
    printk("# of verify errors = %d\n", verify_errcnt);
    
    n++;
    if (n <= round) goto again;

    printk("\nLLD Test finished.\n");
}


// lld_test2: test for copyback

static void lld_test2(UINT16 chip, UINT32 start_block, UINT32 num_blocks, UINT16 round)
{
    int bad, ret, i, j, k, l, n = 1, wbuf_index = 0;
    int erase_errcnt = 0, write_errcnt = 0, copyback_errcnt = 0, read_errcnt = 0, verify_errcnt = 0;
    static unsigned char wbuf[2][4096 + 128];
    static unsigned char rbuf[2][4096 + 128];
    unsigned char *dbuf_group[4], *sbuf_group[4];
    UINT32 block[4];
    INT32 flag[4];

    printk("[LLD] Stress Test starts...\n");
    num_blocks >>= 1;
    
    for (i = 0; i < flash_spec->DataSize; i++) {
        wbuf[0][i] = (unsigned char)i;
        wbuf[1][i] = (unsigned char)i;
    }
    for (; i < flash_spec->PageSize; i++) {
        wbuf[0][i] = 0xff;
        wbuf[1][i] = 0xff;
    }

again:    
    printk("\n==============================================\n");
    printk("    Stress Test Round %d\n", n);
    printk("==============================================\n\n");
    
    printk("Phase 1: filling data ...\n\n");

    for (i = start_block; i < start_block + num_blocks; i += flash_spec->NumPlanes) {
        printk("Block %d", i);
        for (l = 1; l < flash_spec->NumPlanes; l++) printk(", %d", i+l);
        printk(" ... ");
        
        // bad block check
#if 1
        bad = 0;
        for (l = 0; l < flash_spec->NumPlanes; l++) {
            if (FLM_MLC8G_2_IsBadBlock(chip, i+l)) {
                printk("!!! Bad block !!!\n");
                bad = 1;
            }
        }
        if (bad) continue;
#endif

        printk("erase ... ");
        
        if (flash_spec->NumPlanes == 1) {
            
            // erase block
            if (FLM_MLC8G_2_Erase(chip, i)) {
                printk("FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }

#if USE_EXPLICIT_SYNC
            if (FLM_MLC8G_2_Sync(chip)) {
                printk("sync FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }
#endif
        }
        else {

            // erase block group
            for (l = 0; l < flash_spec->NumPlanes; l++) {
                block[l] = i + l;
                flag[l] = 1;
            }
            if (FLM_MLC8G_2_Erase_Group(chip, block, flag)) {
                printk("FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }

#if USE_EXPLICIT_SYNC
            if (FLM_MLC8G_2_Sync(chip)) {
                printk("sync FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }
#endif
        }
        
        printk("write ... ");
        
        if (flash_spec->NumPlanes == 1) {
            
            // write pages in the block
            for (j = 0; j < flash_spec->PagesPerBlock; j++) {
                
                wbuf_index = 1 - wbuf_index;
                
                for (k = 0; k < flash_spec->SectorsPerPage; k++) {
                    *((UINT32 *)(wbuf[wbuf_index] + 512*k)) = i;
                    *((UINT32 *)(wbuf[wbuf_index] + 512*k + 4)) = (UINT32)j;
                }
            
                ret = FLM_MLC8G_2_Write_Page(chip, i, j, 
                                             0, flash_spec->SectorsPerPage, 
                                             wbuf[wbuf_index], 
                                             wbuf[wbuf_index] + flash_spec->DataSize, 1);
                if (ret) {
                    printk("FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    write_errcnt++;
                    continue;
                }

#if USE_EXPLICIT_SYNC
                ret = FLM_MLC8G_2_Sync(chip);
                if (ret) {
                    printk("sync FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    write_errcnt++;
                    continue;
                }
#endif
            }
        }
        else {
            
            // write pages in the block group
            for (j = 0; j < flash_spec->PagesPerBlock; j++) {
                
                wbuf_index = 1 - wbuf_index;
                
                for (k = 0; k < flash_spec->SectorsPerPage; k++) {
                    *((UINT32 *)(wbuf[wbuf_index] + 512*k)) = i;
                    *((UINT32 *)(wbuf[wbuf_index] + 512*k + 4)) = (UINT32)j;
                }

                for (l = 0; l < flash_spec->NumPlanes; l++) {
                    block[l] = i + l;
                    dbuf_group[l] = wbuf[wbuf_index]; 
                    sbuf_group[l] = wbuf[wbuf_index] + flash_spec->DataSize;
                    flag[l] = 1;
                }
                
                ret = FLM_MLC8G_2_Write_Page_Group(chip, block, j, 
                                                   dbuf_group, sbuf_group, 
                                                   flag, 1);
                if (ret) {
                    printk("FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    write_errcnt++;
                    continue;
                }

#if USE_EXPLICIT_SYNC
                ret = FLM_MLC8G_2_Sync(chip);
                if (ret) {
                    printk("sync FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    write_errcnt++;
                    continue;
                }
#endif
            }
        }
        
        printk("Done.\n");
    }

    printk("\nPhase 2: copyback data ...\n\n");

    for (i = start_block + num_blocks; i < start_block + num_blocks*2; i += flash_spec->NumPlanes) {
        printk("Block %d", i);
        for (l = 1; l < flash_spec->NumPlanes; l++) printk(", %d", i+l);
        printk(" ... ");
        
        // bad block check
#if 1
        bad = 0;
        for (l = 0; l < flash_spec->NumPlanes; l++) {
            if (FLM_MLC8G_2_IsBadBlock(chip, i+l)) {
                printk("!!! Bad block !!!\n");
                bad = 1;
            }
        }
        if (bad) continue;
#endif

        printk("erase ... ");
        
        if (flash_spec->NumPlanes == 1) {
            
            // erase block
            if (FLM_MLC8G_2_Erase(chip, i)) {
                printk("FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }

#if USE_EXPLICIT_SYNC
            if (FLM_MLC8G_2_Sync(chip)) {
                printk("sync FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }
#endif
        }
        else {

            // erase block group
            for (l = 0; l < flash_spec->NumPlanes; l++) {
                block[l] = i + l;
                flag[l] = 1;
            }
            if (FLM_MLC8G_2_Erase_Group(chip, block, flag)) {
                printk("FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }

#if USE_EXPLICIT_SYNC
            if (FLM_MLC8G_2_Sync(chip)) {
                printk("sync FAILED!\n");
                prev_op[chip].Command = OP_NONE;
                erase_errcnt++;
                continue;
            }
#endif
        }
        
        printk("copyback ... ");
        
        if (flash_spec->NumPlanes == 1) {
            
            // copyback pages in the block
            for (j = 0; j < flash_spec->PagesPerBlock; j++) {
                       
                ret = FLM_MLC8G_2_Copy_Back(chip, i - num_blocks, j, i, j);
                if (ret) {
                    printk("FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    copyback_errcnt++;
                    continue;
                }

#if USE_EXPLICIT_SYNC
                ret = FLM_MLC8G_2_Sync(chip);
                if (ret) {
                    printk("sync FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    copyback_errcnt++;
                    continue;
                }
#endif
            }
        }
        else {
            
            // copyback pages in the block group
            for (j = 0; j < flash_spec->PagesPerBlock; j++) {
                UINT32 src_block[4], dest_block[4];
                UINT16 src_page[4];

                for (l = 0; l < flash_spec->NumPlanes; l++) {
                    src_block[l] = i - num_blocks + l;
                    src_page[l] = j;
                    dest_block[l] = i + l;
                    flag[l] = 1;
                }

#if 1
                ret = FLM_MLC8G_2_Copy_Back_Group(chip, src_block, src_page, 
                                                  dest_block, j, flag);
                if (ret) {
                    printk("FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    copyback_errcnt++;
                    continue;
                }
#else
                for (l = 0; l < flash_spec->NumPlanes; l++) {
                    ret = FLM_MLC8G_2_Copy_Back(chip, src_block[l], j, dest_block[l], j);
                    if (ret) {
                        printk("FAILED for page %d (error=%d).\n", j, ret);
                        prev_op[chip].Command = OP_NONE;
                        copyback_errcnt++;
                        continue;
                    }
                }
#endif

#if USE_EXPLICIT_SYNC
                ret = FLM_MLC8G_2_Sync(chip);
                if (ret) {
                    printk("sync FAILED for page %d (error=%d).\n", j, ret);
                    prev_op[chip].Command = OP_NONE;
                    copyback_errcnt++;
                    continue;
                }
#endif
            }
        }
        
        printk("Done.\n");
    }
    
    printk("\nPhase 3: verifying data ...\n\n");

    for (i = start_block; i < start_block + num_blocks; i += flash_spec->NumPlanes) {
        printk("Block %d", i);
        for (l = 1; l < flash_spec->NumPlanes; l++) printk(", %d", i+l);
        printk(" ... ");

        // bad block check
#if 1
        bad = 0;
        for (l = 0; l < flash_spec->NumPlanes; l++) {
            if (FLM_MLC8G_2_IsBadBlock(chip, i+l)) {
                printk("!!! Bad block !!!\n");
                bad = 1;
            }
        }
        if (bad) continue;
#endif

        printk("read & verify ... ");

        // read pages in the block
        for (l = 0; l < flash_spec->NumPlanes; l++) {
            for (j = 0; j < flash_spec->PagesPerBlock; j++) {              
                ret = FLM_MLC8G_2_Read_Page(chip, i + l, j, 
                                            0, flash_spec->SectorsPerPage, 
                                            rbuf[0], rbuf[0] + flash_spec->DataSize);
                if (ret) {
                    printk("read FAILED for block %d, page %d (error=%d).\n", i + l, j, ret);
                    prev_op[chip].Command = OP_NONE;
                    read_errcnt++;
                    continue;
                }
                
                ret = FLM_MLC8G_2_Read_Page(chip, i + num_blocks + l, j, 
                                            0, flash_spec->SectorsPerPage, 
                                            rbuf[1], rbuf[1] + flash_spec->DataSize);
                if (ret) {
                    printk("read FAILED for block %d, page %d (error=%d).\n", i + num_blocks + l, j, ret);
                    prev_op[chip].Command = OP_NONE;
                    read_errcnt++;
                    continue;
                }

                if (memcmp(rbuf[0], rbuf[1], flash_spec->DataSize)) {
                    printk("verify FAILED for block %d, page %d.\n", i + num_blocks + l, j);
                    verify_errcnt++;
#if 0
                    printk("Data dump for block %d, page %d:\n", i + num_blocks + l, j);
                    for (k = 0; k < flash_spec->PageSize; k++) {
                        if (k == flash_spec->DataSize) printk("\n");
                        if (k % 16 == 0) printk("%04x: ", k);
                        if (k % 8 == 0) printk(" ");
                        printk("%02x ", rbuf[1][k]);
                        if (k % 16 == 15) printk("\n");
                    }
#endif
                    continue;
                }
            }
        }
        
        printk("Done.\n");
    }
    
    printk("\n");
    printk("# of erase    errors = %d\n", erase_errcnt);
    printk("# of write    errors = %d\n", write_errcnt);
    printk("# of copyback errors = %d\n", copyback_errcnt);
    printk("# of read     errors = %d\n", read_errcnt);
    printk("# of verify   errors = %d\n", verify_errcnt);
    
    n++;
    if (n <= round) goto again;

    printk("\nLLD Test finished.\n");
}

#endif				/* USE_LARGE_MULTI_MLC_NAND */

/* end of fm_driver_ld.c */

/* 
   $Log: fm_driver_stmp36xx.c,v $
   Revision 1.10  2007/09/12 08:32:00  hcyun
   add copyback

   - hcyun

   Revision 1.9  2007/09/12 03:41:20  hcyun

   zeen-rfs-070911 ???? ??? ????.

   - two-plane copyback read ??? unrecoverable ECC ??? ?? ???
     ??? ?? ?? ?? ???, ?? 2?? single-plane copyback read
     ???? ??? ???????.

   - RFS FAT ??? ?? ??? ???? ??? ??? ??? ???
     ???????. (??? FAT sector read? ??? ?? ?? ???
     ????.)

   - hcyun

   Revision 1.8  2007/09/10 12:06:00  hcyun
   copyback operation added (but not used)

   - hcyun

   Revision 1.7  2007/08/16 07:49:10  hcyun
   fix mistake..

   - hcyun

   Revision 1.4  2007/08/16 05:59:35  hcyun
   [BUGFIX] two_plane_write_group bug fix. now support 4/8/16Gbit mono die chip
   - hcyun

   Revision 1.3  2007/08/11 04:00:47  hcyun
   4plane write bug fix

   - hcyun

   Revision 1.2  2007/07/27 06:40:28  hcyun
   bug fix.

   Revision 1.1  2007/07/26 09:40:23  hcyun
   DDP v0.1

   Revision 1.3  2006/07/27 07:01:01  hcyun
   [BUG FIX] bad block reservation is now 200 to meet the specification

   Revision 1.2  2006/07/27 01:05:16  hcyun
   [BUG FIX] to support 8GB

   Revision 1.1  2006/07/18 08:58:16  hcyun
   8GB support (k9hbg08u1m,...)
   simultaneous support for all possible nand chip including SLC(1/2/4GB), MLC(4Gb 1/2/4GB), MLC(8Gb 1GB), MLC(8Gb 2/4/8GB)

   Revision 1.5  2006/07/06 05:15:13  hcyun
   4Gb MLC work-around is applied.. (reduce storage size a little bit)

   Revision 1.4  2006/06/10 04:37:20  hcyun
   back to original.. (do not workaround 4Gb MLC bug)

   Revision 1.2  2006/05/05 03:39:21  hcyun
   range check.

   - hcyun

   Revision 1.1  2006/05/02 11:05:00  hcyun
   from main z-proj source

   Revision 1.6  2006/04/21 07:33:33  hcyun
   In the MLC type devices, bad block location is changed to the last page (127).

   - hcyun

   Revision 1.5  2006/04/21 01:48:08  hcyun
   fix potential bug on following case

   user read(sector1, 0x50300000) <-- as this do not call flash_ecc_sync() on the FLM_MLC8G_2_Sync(), it's possible to read again while the ecc engine is working
   user read(sector2, 0x50300000)

   - hcyun

   Revision 1.4  2006/04/05 04:55:37  hcyun
   collect merge info

   Revision 1.3  2006/03/31 01:55:37  hcyun
   remove error message on nandsb writing.

   Revision 1.2  2006/03/30 12:58:48  hcyun
   remove unecessary message

   Revision 1.1  2006/03/27 13:17:08  hcyun
   K9HBG08U1M (MLC) support.

   Revision 1.48  2005/12/15 04:42:56  hcyun
   do not initialize spare buffer. and layout doesn't work anymore.

   - hcyun

   Revision 1.47  2005/12/12 09:25:40  hcyun
   corrupted bad block check.

   - hcyun

   Revision 1.46  2005/12/12 00:02:54  hcyun
   initializing must be 0xff (not 0)
   - hcyun

   Revision 1.45  2005/12/09 00:02:11  hcyun
   seperate set tAS, tDS, tDH

   - hcyun

   Revision 1.44  2005/12/01 05:54:35  hcyun
   Detect if 0 and 2 chip are identical.

   - hcyun

   Revision 1.43  2005/11/29 06:29:30  hcyun
   spare must be null on read test

   - hcyun

   Revision 1.42  2005/11/25 04:14:23  hcyun
   support two-plane write performance test.. (both "read" and "write" test support)

   - hcyun

   Revision 1.41  2005/11/15 06:38:54  hcyun
   handle when PHORE is not 1 (meaning it's not at the end of DMA chain)


   - hcyun

   Revision 1.40  2005/11/15 01:02:30  hcyun
   remove uncorrect return

   - hcyun

   Revision 1.39  2005/11/14 15:21:45  hcyun
   add 5us delay to stabilize bank selection. (originally 70ns max)

   - hcyun

   Revision 1.38  2005/11/14 14:02:12  hcyun
   use 1 field.

   - hcyun

   Revision 1.37  2005/11/14 12:11:42  hcyun
   4GB bug fix on flash_select_channel..

   - hcyun

   Revision 1.36  2005/11/14 02:24:40  hcyun
   4GB support
   level of interleaving is not 2

   - hcyun

   Revision 1.35  2005/11/09 11:17:22  hcyun
   minor cleanup and fail-safe gpmi timing read.
   - hcyun

   Revision 1.34  2005/11/09 09:49:29  hcyun
   support UFD level ECC pipelining..

   - hcyun

   Revision 1.33  2005/11/08 04:18:02  hcyun
   remove warning..
   chain itself to avoid missing irq problem.

   - hcyun

   Revision 1.32  2005/11/05 23:30:58  hcyun
   add prev_ecc structure to check previous ecc location.

   pipeline option is now located in the fd_physical.h

   - hcyun

   Revision 1.31  2005/11/03 13:08:57  hcyun
   printk bug fix.. but I'm not sure hwecc is safe because I found one uncorrectable ECC error.

   - hcyun

   Revision 1.30  2005/11/03 02:30:40  hcyun
   support SRAM descriptor (require SRAM map change)
   support SRAM ECC Buffer (require SRAM map change)
   support ECC Pipelining (LLD level only), about 20% read performance improvement when accessing over 4KB base
   minimize wait timing (T_OVERHEAD = 10)

   - hcyun

   Revision 1.27  2005/10/30 06:23:35  hcyun
   TIMING_SEM -> TIMING_RFS

   - hcyun

   Revision 1.26  2005/10/23 01:45:08  hcyun
   rfs_dcache_.... (require new hardware.h)
   added more statistic

   - hcyun

   Revision 1.25  2005/10/13 00:23:00  zzinho
   added by heechul in USA

   Revision 1.24  2005/10/08 04:53:29  hcyun
   ..
   - hcyun

   Revision 1.23  2005/10/07 01:50:05  hcyun
   remove IRQ error messages.

   - hcyun

   Revision 1.22  2005/09/30 11:34:00  hcyun
   support SRAM buffer in the driver..

   - hcyun

   Revision 1.21  2005/09/28 09:25:45  hcyun
   compile error fix.. when we don't use TIMING_SEM or TIMING_LLD
   - hcyun

   Revision 1.20  2005/09/28 07:35:41  hcyun
   support interrupt

   - hcyun

   Revision 1.19  2005/09/16 00:11:48  hcyun
   nandboot support
   default partition is chagned. now kernel and cramfs is located at ufdrawa and ufdrawb respectively.

   - hcyun

   Revision 1.18  2005/09/15 00:26:14  hcyun
   reset handling
   missing irq handling improved..

   - hcyun

   Revision 1.17  2005/09/01 01:45:04  hcyun
   multi-plane bug fixed. now we use full multi-plane operations..
   see README.rfs-2GB for more instructions

   Revision 1.16  2005/08/30 05:27:44  hcyun
   wait timeout timeing is adjusted close to real expected timing. you can add or remove the timing by modifying T_OVERHEAD multiplier

   - hcyun

   Revision 1.15  2005/08/30 00:05:21  hcyun
   avoid missing irq problem..

   - hcyun

   Revision 1.14  2005/08/29 05:07:21  hcyun
   Stable new rfs (?)

   - single plane
   - hwecc
   - sw-copyback

   - err_recover() bug fix (handling previous write operation)
   - FLM_MLC8G_2_Read_Page_Group() bug fix

   Revision 1.13  2005/08/25 07:04:09  biglow
   - change low level drivers

   Revision 1.12  2005/08/25 00:12:06  hcyun
   APBH_TIMEOUT detect...

   - hcyun

   Revision 1.11  2005/08/23 01:52:38  hcyun
   TAS : 0 --> 1
   print total number of badblocks during initial format and feraseall
   removed APBH_timout error avoidance (need verification).

   - hcyun

   Revision 1.10  2005/08/20 00:58:10  biglow
   - update rfs which is worked fib fixed chip only.

   Revision 1.9  2005/07/21 05:44:15  hcyun
   merged yhbae's addition - mainly test code, plus bank selection code (currently it has a bug)

   2-plane operation is not enabled in the flash_spec structure.

   - hcyun

   Revision 1.5  2005/07/05 10:36:41  hcyun
   ufd 2.0: 1 package 2GB support..

   - hcyun

   Revision 1.3  2005/06/29 09:51:13  hcyun
   flash_sync() error handling position changed..
   apbh_poll_channel() return value is changed.


   - hcyun

   Revision 1.2  2005/06/28 07:29:58  hcyun
   conservative timing.
   add comments.

   - hcyun

   Revision 1.1  2005/06/27 07:47:17  hcyun
   first working (kernel panic due to sdram corruption??)

   - hcyun


   Old Log:

   Revision 1.24  2005/06/22 02:19:01  hcyun
   address translation bug fixed commented from zeen.

   - hcyun

   Revision 1.23  2005/06/20 15:44:48  hcyun
   ROW_SHIFT : 11 -> 12

   - hcyun

   Revision 1.22  2005/06/12 14:07:37  hcyun
   do not use cache program.. currently it has a bug.. T_T

   - hcyun

   Revision 1.21  2005/06/09 14:21:06  hcyun
   TA3 board : gpio switch

   - hcyun

   Revision 1.20  2005/06/08 13:50:25  hcyun
   minior cleanup.. GPMI enable debug message removed..

   - hcyun

   Revision 1.19  2005/06/08 11:31:17  hcyun
   Critical bug fix of serial ri program: add IRQONCOMPLETE on the dma chain. Now, it works exactly at expected speed..

   - hcyun

   Revision 1.18  2005/06/06 15:38:54  hcyun
   now copyback seems to works.
   read time is incresed.. and it seems to require it for peroper operation.

   - hcyun

   Revision 1.17  2005/06/02 19:16:00  hcyun
   cache program fixup. not it doesn't issue extra check status command..

   - hcyun

   Revision 1.16  2005/05/30 13:48:13  hcyun
   changed to SW_ECC. HW_ECC is not working correctly right now.

   - hcyun

   Revision 1.15  2005/05/30 10:22:30  hcyun
   Added full test case with error handling..
   FLM_MLC8G_2_TEST, FLM_MLC8G_2_Format

   - hcyun

   Revision 1.14  2005/05/27 21:11:00  hcyun
   flush/invalidate_dcache_range input parameter fixed.

   ECC512 code require kernel mode alignment trap handler

   - hcyun

   Revision 1.13  2005/05/26 20:57:17  hcyun
   ..

   - hcyun

   Revision 1.12  2005/05/24 17:34:05  hcyun
   add debug message on fault..

   - hcyun

   Revision 1.11  2005/05/18 05:44:57  hcyun
   write_verity bug fix..

   - hcyun

   Revision 1.9  2005/05/17 08:03:42  hcyun
   Adjusted timing... almost 10x than I expected. why???

   - hcyun

   Revision 1.8  2005/05/16 03:37:33  hcyun
   n_chips : 2 -> 1

   gbbm init doesn't works..

   - hcyun

   Revision 1.7  2005/05/16 01:47:39  hcyun
   first compiled version.. but not working yet..

   - hcyun

   Revision 1.6  2005/05/15 23:52:15  hcyun
   removing warnings, spare read address translation bug fixed.

   - hcyun

   Revision 1.5  2005/05/15 23:01:38  hcyun
   lots of cleanup... not yet compiled..

   - hcyun

   Revision 1.4  2005/05/15 05:11:04  hcyun
   copy-back, cache-program, random-input, random-output added. and removed unnecessary functions & types.
   - hcyun

   Revision 1.3  2005/05/14 07:07:56  hcyun
   copy-back, cache-program, random-input, random-output added.
   fm_driver for stmp36xx added
   
   Revision 1.2  2005/05/13 14:24:37  hcyun
   onenand, small lld added for compilation..
   
   Revision 1.1  2005/05/13 03:40:18  hcyun
   stmp36xx lld driver..
   
*/ 
