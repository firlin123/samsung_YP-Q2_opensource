/*
 * Copyright (C) 2007 SigmaTel, Inc., Ioannis Kappas <ikappas@sigmatel.com>
 *
 * Portions copyright (C) 2003 Russell King, PXA MMCI Driver
 * Portions copyright (C) 2004-2005 Pierre Ossman, W83L51xD SD/MMC driver
 *
 * This  program is  free  software; you  can  redistribute it  and/or
 * modify  it under the  terms of  the GNU  General Public  License as
 * published by the Free Software  Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 *
 * You should have  received a copy of the  GNU General Public License
 * along  with  this program;  if  not,  write  to the  Free  Software
 * Foundation,  Inc.,  51 Franklin  Street,  Fifth  Floor, Boston,  MA
 * 02110-1301, USA.
 */
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <asm/hardware.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/host.h>
//#include <linux/mmc/protocol.h>
#include <linux/highmem.h>

#include <asm/arch/stmp37xx.h>
#include <asm/arch/dma.h>

/* TODO:
 *
 * a. This is a moviNAND  driver. Hot-plug in/out support of MMC cards
 * hasn't been a requirement and is not supported in this release.
 *
 * b. Figure out whether scatterlists can be direclty DMA'd without the
 * use of memcpys and thus improve performance.
 *
 */

//#define DEBUG printk
//#define TERSE printk

#ifndef DEBUG
# define DEBUG(fmt,args...)
#endif
#ifndef TERSE
# define TERSE(fmt,args...)
#endif


#define DRIVER_NAME	"stmp37xx-mmc"

#define PLL_SPEED 480000000

#define CLOCKRATE_MIN 400000
#define CLOCKRATE_MAX 48000000


/* Definitions reused from SDK5 */
/* From hw_ssp.h */
#define HW_SSP_IO_PINCTRL_MUXSELECT                     0x00FFF000
#define HW_SSP_PINCTRL_DRIVE6_MASK                      0xFF000000
#define HW_SSP_PINCTRL_DRIVE6_3V_8MA                    0x55000000
#define HW_SSP_PINCTRL_DRIVE7_MASK                      0x000FFFFF
#define HW_SSP_PINCTRL_DRIVE7_3V_8MA                    0x00045555
/* Mask  for  the bit  in  37xx  that selects  the  PWM3  pin used  to
 * control */
/* MMC/SD socket power. */
#define HW_SSP_SOCKET_POWER_BIT_MASK                    (1 << 3)
/* Mask for the pin that the card socket's hardware write protect signal is */
/* connected, PWM4. */
#define HW_SSP_WRITE_PROTECT_BIT_MASK					(1 << 4)
/* Mask for the bit in 37xx that controls the SSP1_DETECT pin in several registers */
/* for GPIO  Bank 1, including  HW_PINCTRL_DOE1, HW_PINCTRL_IRQLEVEL1,
 * etc. */
#define HW_SSP_CARD_DETECT_BIT_MASK                     (1 << 28)
/* Mask for bits in 37xx  MUXSEL3 register to configure BANK1_PIN28 as
 * SSP1_DETECT. */
#define HW_SSP_CARD_DETECT_MUXSEL_INIT()                HW_PINCTRL_MUXSEL3_CLR(BM_PINCTRL_MUXSEL3_BANK1_PIN28)
/* From hw_ssp_internal.h */
/* Bit mast for the Bus Select GPIO bit 22. */
#define HW_SSP_BUS_SELECT_MASK 0x00400000
/* SSP Control 1 register Error fields mask */
#define SSP_CTRL1_ERROR_MASK    0x2A828000
#define SSP_STATUS_ERROR_MASK   0x0001F210

/* The constant divider for SSP TIMING register.
 * Set to the minimum value allowed.
 */
#define SSP_TIMING_DIV 2

/* DMA data */
static stmp37xx_dma_user_t dma_user =
{
    .name = "stmp37xx MMC/SD"
};

#define SSP_CHAIN_LEN 2

/* Max value supported for XFER_COUNT */
#define SSP_BUFFER_SIZE (65536 - 512)

static circular_dma_chain_t ssp_dma_chain =
{
	.id = DMA_SSP1,
/* bus channel should be deleted */
    .bus = STMP37XX_BUS_APBH,
    .channel = 1
};

static stmp37xx_dma_descriptor_t ssp_dma_descrs[SSP_CHAIN_LEN];

static dma_addr_t ssp_dma_buffers[SSP_CHAIN_LEN];

struct stmp37xx_mmc_host {
	struct mmc_host		*mmc;
	unsigned int		clkrt;
	
	struct mmc_request	*mrq;
	struct mmc_command	*cmd;
	struct mmc_data		*data;
	
	int bus_width_4; /* Whether the card is capable of 4-bit data */
};

/* Wake on dma command completion
 * TODO: How do I pack them into the above structure and
 * reference them from the interrupt handler?
 */
static wait_queue_head_t wait_q;
static int is_cmd_running;

static void mmc_regs(void)
{
	/* Note that cmd/data registers are not printed out because
	* it direclty changes the state of the peripheral.
	*/
	TERSE("CTRL0: 0x%8x, CMD0 : 0x%8x, CMD1: 0x%8x\n"
		"CMPRF: 0x%8x, CMMSK: 0x%8x, TMNG: 0x%8x\n"
		"CTRL1: 0x%8x,\n"
		"STAT : 0x%8x, DBG  : 0x%8x, VRSN: 0x%8x\n",
		HW_SSP_CTRL0_RD(), HW_SSP_CMD0_RD(), HW_SSP_CMD1_RD(),
		HW_SSP_COMPREF_RD(), HW_SSP_COMPMASK_RD(), HW_SSP_TIMING_RD(),
		HW_SSP_CTRL1_RD(),
		HW_SSP_STATUS_RD(), HW_SSP_DEBUG_RD(), HW_SSP_VERSION_RD());
}

/* Called on DMA complete */
//static irqreturn_t ssp_dma_int(int irq, void *dev_id)
static void ssp_dma_int (int id, unsigned int dma_status,  void* dev_id)
{
	if (is_cmd_running)
	{
		is_cmd_running = 0;
		wake_up_interruptible(&wait_q);
	}
}

#define CHK_CLR_HW_SSP_INT( X ) if ( HW_SSP_CTRL1_RD() & BM_SSP_CTRL1_##X ) \
                                { HW_SSP_CTRL1_CLR( BM_SSP_CTRL1_##X ); TERSE("cleared " #X "\n"); }
/* Called on SSP error conditions */
static irqreturn_t ssp_error_int (int irq, void *dev_id)
{
	TERSE("-->ssp error happened\n");
	
	CHK_CLR_HW_SSP_INT( RECV_TIMEOUT_IRQ );
	CHK_CLR_HW_SSP_INT( DATA_CRC_IRQ );
	CHK_CLR_HW_SSP_INT( DATA_TIMEOUT_IRQ );
	CHK_CLR_HW_SSP_INT( RESP_TIMEOUT_IRQ );
	CHK_CLR_HW_SSP_INT( RESP_ERR_IRQ );
	
	if (is_cmd_running)
	{
		is_cmd_running = 0;
		wake_up_interruptible(&wait_q);
    }
	return IRQ_HANDLED;
}

static void print_mmc_command (const struct mmc_command* cmd)
{
    TERSE("MMC COMMAND:\n");
    TERSE("opcode: %u, arg: %u, resp %u %u %u %u, flags 0x%x\n"
	   "retries: %u, error: %u\n",
	   cmd->opcode, cmd->arg,
	   cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3],
	   cmd->flags, cmd->retries, cmd->error);
}

/* Reset ssp peripheral to default values */
static void stmp37xx_mmc_reset (void)
{
    hw_ssp_ctrl0_t sControl0;
    hw_ssp_ctrl1_t sControl1;

    /* Soft reset cycle the block */
    HW_SSP_CTRL0_SET(BM_SSP_CTRL0_SFTRST);

    while (!HW_SSP_CTRL0.B.CLKGATE);

    HW_SSP_CTRL0_CLR(BM_SSP_CTRL0_SFTRST | BM_SSP_CTRL0_CLKGATE);

    /* Configure SSP Control Register 0 */
    sControl0.U = 0;

    sControl0.B.LOCK_CS = 0;
    sControl0.B.IGNORE_CRC = 1;
    sControl0.B.BUS_WIDTH = BV_SSP_CTRL0_BUS_WIDTH__ONE_BIT;
    sControl0.B.WAIT_FOR_IRQ = 0;
    sControl0.B.LONG_RESP = 0;
    sControl0.B.CHECK_RESP = 0;
    sControl0.B.GET_RESP = 0;

    /* Configure SSP Control Register 1 */
    sControl1.U = 0;

    sControl1.B.DMA_ENABLE = 1;

    sControl1.B.CEATA_CCS_ERR_EN = 0;

    sControl1.B.SLAVE_OUT_DISABLE = 0;
    sControl1.B.PHASE = 0;
    sControl1.B.POLARITY = 1;
    sControl1.B.WORD_LENGTH = BV_SSP_CTRL1_WORD_LENGTH__EIGHT_BITS;
    sControl1.B.SLAVE_MODE = 0;
    sControl1.B.SSP_MODE = BV_SSP_CTRL1_SSP_MODE__SD_MMC;

    sControl1.B.RECV_TIMEOUT_IRQ_EN = 1;
    sControl1.B.DATA_CRC_IRQ_EN = 1;
    sControl1.B.DATA_TIMEOUT_IRQ_EN = 1;
    sControl1.B.RESP_TIMEOUT_IRQ_EN = 1;
    sControl1.B.RESP_ERR_IRQ_EN = 1;

	BW_SSP_TIMING_TIMEOUT(0xFFFF);
	BW_SSP_TIMING_CLOCK_DIVIDE( SSP_TIMING_DIV );

    /* Write the SSP Control Register 0 and 1 values out to the interface */
    HW_SSP_CTRL0_WR(sControl0.U);
    HW_SSP_CTRL1_WR(sControl1.U);

    mmc_regs();
}

/* Send the BC command to the device */
static void stmp37xx_mmc_bc( struct stmp37xx_mmc_host *host, struct mmc_command *cmd )
{
    hw_ssp_ctrl0_t ctrl0;
    hw_ssp_cmd0_t cmd0;
    hw_ssp_cmd1_t cmd1;

    stmp37xx_dma_descriptor_t* descr;

    descr = circ_get_free_head( &ssp_dma_chain );
    descr->command->cmd = 0;
    descr->command->cmd =
	BF_APBH_CHn_CMD_XFER_COUNT(0) |
	BF_APBH_CHn_CMD_CMDWORDS(3) |
	BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
	BF_APBH_CHn_CMD_SEMAPHORE(1) |
	BF_APBH_CHn_CMD_IRQONCMPLT(1) |
	BF_APBH_CHn_CMD_CHAIN(1) |
	BF_APBH_CHn_CMD_COMMAND(BV_APBH_CHn_CMD_COMMAND__NO_DMA_XFER);

    ctrl0.U = BF_SSP_CTRL0_ENABLE(1) |
	BF_SSP_CTRL0_IGNORE_CRC(1) |
	BF_SSP_CTRL0_GET_RESP(0);
    cmd0.U = 0;
    cmd0.B.CMD = cmd->opcode;
    cmd0.B.APPEND_8CYC = 1;
    cmd1.U = 0;
    cmd1.B.CMD_ARG = cmd->arg;
    descr->command->pio_words[0] = ctrl0.U;
    descr->command->pio_words[1] = cmd0.U;
    descr->command->pio_words[2] = cmd1.U;

    BUG_ON(is_cmd_running);
    is_cmd_running = 1;

    circ_advance_active( &ssp_dma_chain, 1 );

	wait_event_interruptible(wait_q, is_cmd_running == 0);

#if 0
    if ( HW_SSP_STATUS.B.TIMEOUT ) cmd->error |= MMC_ERR_TIMEOUT;
    if ( HW_SSP_STATUS.B.RESP_TIMEOUT ) cmd->error |= MMC_ERR_TIMEOUT;
    if ( HW_SSP_STATUS.B.RESP_CRC_ERR ) cmd->error |= MMC_ERR_BADCRC;
    if ( HW_SSP_STATUS.B.RESP_ERR ) cmd->error |= MMC_ERR_FAILED;
#else
	if ( HW_SSP_STATUS.B.TIMEOUT ) cmd->error = -ETIMEDOUT;
	if ( HW_SSP_STATUS.B.RESP_TIMEOUT ) cmd->error = -ETIMEDOUT;
	if ( HW_SSP_STATUS.B.RESP_CRC_ERR ) cmd->error = -EILSEQ;
	if ( HW_SSP_STATUS.B.RESP_ERR ) cmd->error = -EIO;
#endif
    circ_advance_cooked( &ssp_dma_chain );

	if (cmd->error) {
		DEBUG("command error, cmd 0x%x\n", cmd->error);
		stmp37xx_dma_reset_channel(ssp_dma_chain.id);
		circ_clear_chain( &ssp_dma_chain );
		stmp37xx_dma_go(ssp_dma_chain.id, &ssp_dma_descrs[0], 0);
	}
	else {
		circ_advance_free( &ssp_dma_chain, 1 );
	}
}

/* Send the ac command to the device */
static void stmp37xx_mmc_ac( struct stmp37xx_mmc_host *host, struct mmc_command *cmd )
{
    hw_ssp_ctrl0_t ctrl0;
    hw_ssp_cmd0_t cmd0;
    hw_ssp_cmd1_t cmd1;

    stmp37xx_dma_descriptor_t* descr;

    const int ignore_crc = mmc_resp_type(cmd) & MMC_RSP_CRC ? 0 : 1;
    const int resp = mmc_resp_type(cmd) & MMC_RSP_PRESENT ? 1 : 0;
    const int long_resp = mmc_resp_type(cmd) & MMC_RSP_136 ? 1 : 0;
    TERSE("response is %d, ignore crc is %d\n", resp, ignore_crc);

    descr = circ_get_free_head( &ssp_dma_chain );
    descr->command->cmd = 0;
    descr->command->cmd =
	BF_APBH_CHn_CMD_XFER_COUNT(0) |
	BF_APBH_CHn_CMD_CMDWORDS(3) |
	BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
	BF_APBH_CHn_CMD_SEMAPHORE(1) |
	BF_APBH_CHn_CMD_IRQONCMPLT(1) |
	BF_APBH_CHn_CMD_CHAIN(1) |
	BF_APBH_CHn_CMD_COMMAND(BV_APBH_CHn_CMD_COMMAND__NO_DMA_XFER);

    ctrl0.U = BF_SSP_CTRL0_ENABLE(1) |
 	BF_SSP_CTRL0_IGNORE_CRC(ignore_crc) |
	BF_SSP_CTRL0_LONG_RESP(long_resp) |
 	BF_SSP_CTRL0_GET_RESP(resp);
    cmd0.U = 0;
    cmd0.B.CMD = cmd->opcode;
    cmd0.B.BLOCK_COUNT = 0;
    cmd1.U = 0;
    cmd1.B.CMD_ARG = cmd->arg;
    descr->command->pio_words[0] = ctrl0.U;
    descr->command->pio_words[1] = cmd0.U;
    descr->command->pio_words[2] = cmd1.U;

    BUG_ON(is_cmd_running);
    is_cmd_running = 1;
    circ_advance_active( &ssp_dma_chain, 1 );

    wait_event_interruptible(wait_q,
			     is_cmd_running == 0);

    TERSE("Response type 0x%x\n", mmc_resp_type( cmd ) );
    switch (mmc_resp_type( cmd ))
    {
    case MMC_RSP_NONE:
	while ( BF_RD(SSP_CTRL0, RUN ) );
	break;
    case MMC_RSP_R1:
    case MMC_RSP_R1B:
    case MMC_RSP_R3:
	while ( BF_RD(SSP_CTRL0, RUN ) );
	cmd->resp[0] = HW_SSP_SDRESP0_RD();
	DEBUG("Response is 0x%x\n", cmd->resp[0]);
	break;
    case MMC_RSP_R2:
	while ( BF_RD(SSP_CTRL0, RUN ) );
	cmd->resp[3] = HW_SSP_SDRESP0_RD();
	cmd->resp[2] = HW_SSP_SDRESP1_RD();
	cmd->resp[1] = HW_SSP_SDRESP2_RD();
	cmd->resp[0] = HW_SSP_SDRESP3_RD();
	DEBUG("Response is 0x%x, 0x%x, 0x%x, 0x%x\n",
	       cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
	break;
    default:
	DEBUG("Unsupported response type 0x%x\n",  mmc_resp_type( cmd ));
	BUG();
	break;
    }

#if 0
    if ( HW_SSP_STATUS.B.TIMEOUT ) cmd->error |= MMC_ERR_TIMEOUT;
    if ( HW_SSP_STATUS.B.RESP_TIMEOUT ) cmd->error |= MMC_ERR_TIMEOUT;
    if ( HW_SSP_STATUS.B.RESP_CRC_ERR ) cmd->error |= MMC_ERR_BADCRC;
    if ( HW_SSP_STATUS.B.RESP_ERR ) cmd->error |= MMC_ERR_FAILED;
#else
	if ( HW_SSP_STATUS.B.TIMEOUT ) cmd->error = -ETIMEDOUT;
	if ( HW_SSP_STATUS.B.RESP_TIMEOUT ) cmd->error = -ETIMEDOUT;
	if ( HW_SSP_STATUS.B.RESP_CRC_ERR ) cmd->error = -EILSEQ;
	if ( HW_SSP_STATUS.B.RESP_ERR ) cmd->error = -EIO;
#endif

    {
	unsigned adv = circ_advance_cooked( &ssp_dma_chain );
	DEBUG("advanced %u dma commands\n", adv);
	if ( adv == 0)
	{
	    TERSE("ERROR: dma command not finished\n");
	}

    }

    if (cmd->error) {
	TERSE("command error, cmd 0x%x\n", cmd->error);
	stmp37xx_dma_reset_channel(ssp_dma_chain.id);
	circ_clear_chain( &ssp_dma_chain );
	stmp37xx_dma_go(ssp_dma_chain.id, &ssp_dma_descrs[0], 0);
    } else {
	circ_advance_free( &ssp_dma_chain, 1 );
    }

}

/* Copy data from sg list to dma buffer */
static unsigned stmp37xx_sg_to_dma(struct mmc_data *data, unsigned size, stmp37xx_dma_descriptor_t* descr)
{
    unsigned int len, i, bytes_copied = 0;
    struct scatterlist *sg;
    char *dmabuf = descr->virtual_buf_ptr;
    char *sgbuf;

    sg = data->sg;
    len = data->sg_len;

    /*
     * Just loop through all entries. Size might not
     * be the entire list though so make sure that
     * we do not transfer too much.
     */
    for (i = 0; i < len; i++) {
	sgbuf = kmap_atomic(sg_page(&sg[i]), KM_BIO_SRC_IRQ) + sg[i].offset;
	if (size < sg[i].length)
	    memcpy(dmabuf, sgbuf, size);
	else
	    memcpy(dmabuf, sgbuf, sg[i].length);
	kunmap_atomic(sgbuf, KM_BIO_SRC_IRQ);
	dmabuf += sg[i].length;

	if (size < sg[i].length)
	{
	    bytes_copied += size;
	    size = 0;
	}
	else
	{
	    size -= sg[i].length;

	    bytes_copied += sg[i].length;
	}

	if (size == 0)
	    break;
    }

    return bytes_copied;

 }

/* Receive data from DMA channel, copying them over to the sg list */
static void stmp37xx_dma_to_sg(struct mmc_data *data, const unsigned dma_size)
{
    unsigned int len, i;
    struct scatterlist *sg;
    char *dmabuf = NULL;
    char *sgbuf;
    stmp37xx_dma_descriptor_t* descr;
    unsigned size = dma_size;

    sg = data->sg;
    len = data->sg_len;
    DEBUG("Cooked count is %u\n", ssp_dma_chain.cooked_count);
    BUG_ON( ssp_dma_chain.cooked_count != 1 );
    descr = circ_get_cooked_head( &ssp_dma_chain );
    dmabuf = descr->virtual_buf_ptr;

    for (i = 0; i < len; i++)
    {
	sgbuf = kmap_atomic(sg_page(&sg[i]), KM_BIO_SRC_IRQ) + sg[i].offset;
    if (size < sg[i].length)
	    memcpy(sgbuf, dmabuf, size);
	else
	    memcpy(sgbuf, dmabuf, sg[i].length);
	kunmap_atomic(sgbuf, KM_BIO_SRC_IRQ);
	dmabuf += sg[i].length;

	if (size < sg[i].length)
	    size = 0;
	else
	    size -= sg[i].length;

 	if (size == 0)
 	    break;

    }

    circ_advance_free( &ssp_dma_chain, 1);

    BUG_ON( ssp_dma_chain.cooked_count > 0 );
    DEBUG("remaineing %u bytes\n", size);
    BUG_ON( size > 0 );

    data->bytes_xfered = dma_size - size;
}

/* Convert ns to tick count according to the current sclk speed */
static unsigned short stmp37xx_ns_to_ssp_ticks( unsigned clock_rate, unsigned ns )
{
    const unsigned ssp_timeout_mul = 4096;
    /* Calculate ticks in ms since ns are large numbers and might overflow */
    const unsigned clock_per_ms = clock_rate / 1000;
    const unsigned ms = ns / 1000;
    const unsigned ticks = ms * clock_per_ms;
    const unsigned ssp_ticks = ticks / ssp_timeout_mul;

    BUG_ON(ssp_ticks == 0);
    return ssp_ticks;
}

/* Send adtc command to the card */
static void stmp37xx_mmc_adtc( struct stmp37xx_mmc_host *host, struct mmc_command *cmd )
{
    hw_ssp_ctrl0_t ctrl0;
    hw_ssp_cmd0_t cmd0;
    hw_ssp_cmd1_t cmd1;
    const unsigned data_size = cmd->data->blksz * cmd->data->blocks;

    stmp37xx_dma_descriptor_t* descr;

    const int ignore_crc = mmc_resp_type(cmd) & MMC_RSP_CRC ? 0 : 1;
    const int resp = mmc_resp_type(cmd) & MMC_RSP_PRESENT ? 1 : 0;
    const int long_resp = mmc_resp_type(cmd) & MMC_RSP_136 ? 1 : 0;
    int is_reading = 0;
    DEBUG("response is %d, ignore crc is %d\n", resp, ignore_crc);
    DEBUG("data list: %u, blocksz: %u, blocks %u, timeout %uns %uclks, flags 0x%x\n",
	   cmd->data->sg_len, cmd->data->blksz,cmd->data->blocks,
 	   cmd->data->timeout_ns, cmd->data->timeout_clks, cmd->data->flags);

    descr = circ_get_free_head( &ssp_dma_chain );

    if ( cmd->data->flags & MMC_DATA_WRITE )
    {
	int copy_size = stmp37xx_sg_to_dma( cmd->data, data_size, descr);
	DEBUG("Data Write, required %u, copied %u\n", data_size, copy_size);
	BUG_ON( copy_size < data_size );
	is_reading = 0;
    }
    else if ( cmd->data->flags & MMC_DATA_READ )
    {
	DEBUG("Data Read\n");
	is_reading = 1;
    }
    else
    {
	DEBUG("Unsuspported data mode, 0x%x\n", cmd->data->flags);
	BUG();
    }
#if 0
    if ( cmd->data->flags & MMC_DATA_MULTI )
    {
	DEBUG("multiple data transfer. Stop request is 0x%p\n", cmd->data->stop);
    }
#endif
    BUG_ON( cmd->data->flags & MMC_DATA_STREAM);

    BUG_ON( (data_size % 8) > 0 );
    DEBUG("Data size is %u\n", data_size);
    descr->command->cmd = 0;
    descr->command->cmd =
	BF_APBH_CHn_CMD_XFER_COUNT(data_size) |
	BF_APBH_CHn_CMD_CMDWORDS(3) |
	BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
	BF_APBH_CHn_CMD_SEMAPHORE(1) |
	BF_APBH_CHn_CMD_IRQONCMPLT(1) |
	BF_APBH_CHn_CMD_CHAIN(1) |
	BF_APBH_CHn_CMD_COMMAND(is_reading ? BV_APBH_CHn_CMD_COMMAND__DMA_WRITE :
				BV_APBH_CHn_CMD_COMMAND__DMA_READ);

    ctrl0.U = BF_SSP_CTRL0_LOCK_CS(0) |
	BF_SSP_CTRL0_IGNORE_CRC(ignore_crc) |
	BF_SSP_CTRL0_READ(is_reading ? 1 : 0) |
	BF_SSP_CTRL0_DATA_XFER(1) |
	BF_SSP_CTRL0_BUS_WIDTH(host->bus_width_4 ? BV_SSP_CTRL0_BUS_WIDTH__FOUR_BIT : BV_SSP_CTRL0_BUS_WIDTH__ONE_BIT) |
	BF_SSP_CTRL0_WAIT_FOR_IRQ(1) |
	BF_SSP_CTRL0_WAIT_FOR_CMD(0) |
	BF_SSP_CTRL0_LONG_RESP(long_resp) |
	BF_SSP_CTRL0_GET_RESP(resp) |
	BF_SSP_CTRL0_ENABLE(1) |
	BF_SSP_CTRL0_XFER_COUNT(data_size);

    {
        unsigned log2_block_size;
        DEBUG("Block size is %u\n", cmd->data->blksz);
        /*
        ** We need to set the hardware register to the logarithm to base 2 of
        ** the block size. We compute this value by shifting the block size
        ** down one bit at a time until it becomes zero.
        */
        for (log2_block_size = 0; (cmd->data->blksz >> (log2_block_size + 1)) != 0; log2_block_size++)
        {}

        cmd0.U = BF_SSP_CMD0_BLOCK_SIZE( log2_block_size );
	cmd0.B.CMD = cmd->opcode;
	cmd0.B.BLOCK_COUNT = cmd->data->blocks - 1;
	if (cmd->opcode == 12 ) cmd0.B.APPEND_8CYC = 1;
    }
    cmd1.U = 0;
    cmd1.B.CMD_ARG = cmd->arg;
    descr->command->pio_words[0] = ctrl0.U;
    descr->command->pio_words[1] = cmd0.U;
    descr->command->pio_words[2] = cmd1.U;

    /* Set the timeout count */
    BW_SSP_TIMING_TIMEOUT( stmp37xx_ns_to_ssp_ticks(host->clkrt, cmd->data->timeout_ns) );

    BUG_ON(is_cmd_running);
    is_cmd_running = 1;
    circ_advance_active( &ssp_dma_chain, 1 );

    mmc_regs();

    wait_event_interruptible(wait_q,
			     is_cmd_running == 0);

    DEBUG("Response type 0x%x\n", mmc_resp_type( cmd ) );
    switch (mmc_resp_type( cmd ))
    {
    case MMC_RSP_NONE:
	break;
    case MMC_RSP_R1:
    case MMC_RSP_R3:
	cmd->resp[0] = HW_SSP_SDRESP0_RD();
	DEBUG("Response is 0x%x\n", cmd->resp[0]);
	break;
    case MMC_RSP_R2:
	cmd->resp[3] = HW_SSP_SDRESP0_RD();
	cmd->resp[2] = HW_SSP_SDRESP1_RD();
	cmd->resp[1] = HW_SSP_SDRESP2_RD();
	cmd->resp[0] = HW_SSP_SDRESP3_RD();
	DEBUG("Response is 0x%x, 0x%x, 0x%x, 0x%x\n",
	       cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
	break;
    default:
	DEBUG("Unsupported response type 0x%x\n",  mmc_resp_type( cmd ));
	BUG();
	break;
    }

#if 0
    if ( HW_SSP_STATUS.B.TIMEOUT ) cmd->error |= MMC_ERR_TIMEOUT;
    if ( HW_SSP_STATUS.B.RESP_TIMEOUT ) cmd->error |= MMC_ERR_TIMEOUT;
    if ( HW_SSP_STATUS.B.RESP_CRC_ERR ) cmd->error |= MMC_ERR_BADCRC;
    if ( HW_SSP_STATUS.B.RESP_ERR ) cmd->error |= MMC_ERR_FAILED;
#else
	if ( HW_SSP_STATUS.B.TIMEOUT ) cmd->error = -ETIMEDOUT;
	if ( HW_SSP_STATUS.B.RESP_TIMEOUT ) cmd->error = -ETIMEDOUT;
	if ( HW_SSP_STATUS.B.RESP_CRC_ERR ) cmd->error = -EILSEQ;
	if ( HW_SSP_STATUS.B.RESP_ERR ) cmd->error = -EIO;
#endif

    {
	unsigned adv = circ_advance_cooked( &ssp_dma_chain );
	DEBUG("advanced %u dma commands\n", adv);
	if ( adv == 0 ) { TERSE("Error: DMA command not finished\n"); }
    }

    if (cmd->error) {
	DEBUG("command error, cmd 0x%x\n", cmd->error);
	stmp37xx_dma_reset_channel(ssp_dma_chain.id);
	circ_clear_chain( &ssp_dma_chain );
	stmp37xx_dma_go(ssp_dma_chain.id, &ssp_dma_descrs[0], 0);
    } else {
	if ( is_reading )
	{
	    stmp37xx_dma_to_sg( cmd->data, data_size );
	}
	else
	{
	    cmd->data->bytes_xfered = data_size;
	    DEBUG("transferred %u bytes\n", cmd->data->bytes_xfered);
	    circ_advance_free( &ssp_dma_chain, 1 );
	}

    }
}

/* Begin sedning a command to the card */
static void stmp37xx_mmc_start_cmd(struct stmp37xx_mmc_host *host, struct mmc_command *cmd)
{
	host->cmd = cmd;

	print_mmc_command( cmd );

	DEBUG("Command type is 0x%x, flags 0x%x, AND 0x%x\n", mmc_cmd_type( cmd ), cmd->flags,  cmd->flags & MMC_CMD_MASK);
	switch ( mmc_cmd_type( cmd ) )
	{
	case MMC_CMD_BC:
	    stmp37xx_mmc_bc( host, cmd );
	    break;
	case MMC_CMD_BCR:
	    stmp37xx_mmc_ac( host, cmd );
	    break;
	case MMC_CMD_AC:
	    stmp37xx_mmc_ac( host, cmd );
	    break;
	case MMC_CMD_ADTC:
	    stmp37xx_mmc_adtc( host, cmd );
	    break;
	default:
	    BUG();
	    break;
	}
}

/* static unsigned req_count = 0; */
/* Handle MMC request */
static void stmp37xx_mmc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct stmp37xx_mmc_host *host = mmc_priv(mmc);

	host->mrq = mrq;
	// Todo: hack
	if (!HW_APBH_CHn_NXTCMDAR_RD(1)) {
		stmp37xx_dma_reset_channel(ssp_dma_chain.id);
		circ_clear_chain( &ssp_dma_chain );
		stmp37xx_dma_go(ssp_dma_chain.id, &ssp_dma_descrs[0], 0);
	}
/*     TERSE("**** MMC request %u\n", req_count++); */
	TERSE("**** MMC request\n");
	if ( mrq->data && mrq->data->stop ) {
		DEBUG("stop opcode is %u\n", mrq->data->stop->opcode);
	}
	stmp37xx_mmc_start_cmd( host, mrq->cmd/*, cmdat*/ );

	if ( mrq->data && mrq->data->stop ) {
		DEBUG("stopping\n");
		DEBUG("stop opcode is %u\n", mrq->data->stop->opcode);
		stmp37xx_mmc_start_cmd( host, mrq->data->stop/*, cmdat*/ );
	}

	mmc_request_done(mmc, mrq);

}

/* Return read only state of card */
static int stmp37xx_mmc_get_ro(struct mmc_host *mmc)
{
    return HW_PINCTRL_DIN2_RD() & (HW_SSP_WRITE_PROTECT_BIT_MASK); /* PWM4 pin */
}

/* About setting the clock speed.
 * There are two clock sources for SCLK; XTAL and PLL.
 * We plan of using the PLL@480Mhz.
 * The PLL can be further subdivided by ref_io, SSP Clock and SSP timming in this order.
 * The ref_io divider will leave the clock @480Mhz.
 * The SSP Clock will be configurable to get to the required clock speed.
 * The SSP Timing will subdivide by 2 (the minimum it can support).
 * Thus to calculate the speed, the following formula is used: 480 / (SSP clock * 2);
 */
unsigned stmp37xx_hz_to_ssp_div( unsigned hz )
{
    /* round up divider. Minimum 1, max 511 */
    unsigned divider = (PLL_SPEED + hz - 1) / hz;
    if ( divider == 0 ) divider = 1;
    else if (divider > 511 ) divider = 511;
    DEBUG("divider is %u for %uhz\n", divider, hz);
    return divider;
}

void stmp37xx_set_sclk_speed( unsigned hz )
{
    TERSE("Asked to set sclk speed to %uhz\n", hz);
#if 0
	// todo : not wokring
    {
	BUG_ON( HW_CLKCTRL_PLLCTRL0.B.POWER == 0 ); /* PLL is one */
	BUG_ON( HW_CLKCTRL_FRAC.B.CLKGATEIO == 0 &&
		HW_CLKCTRL_FRAC.B.IOFRAC != 18 ); /* Full 480 out of ref_io */

	/* Switch SSP to crystal reference and turn it on.
	 * It is adviced to set the divider in XTAL rather than in ref_io
	 * and then switch to ref_io
	 */
	HW_CLKCTRL_CLKSEQ_SET( BM_CLKCTRL_CLKSEQ_BYPASS_SSP );
	HW_CLKCTRL_SSP_CLR( BM_CLKCTRL_SSP_CLKGATE );
	while ( HW_CLKCTRL_SSP.B.BUSY );
	HW_CLKCTRL_SSP.B.DIV = stmp37xx_hz_to_ssp_div( hz * SSP_TIMING_DIV );
 	while ( HW_CLKCTRL_SSP.B.BUSY );

 	HW_CLKCTRL_CLKSEQ_CLR( BM_CLKCTRL_CLKSEQ_BYPASS_SSP ); /* can switch on the fly */

    }
#else
	{
		extern int ddi_clocks_SetSspClk(uint32_t *kHz, bool );
		uint32_t kHz = hz/1000*SSP_TIMING_DIV;
		ddi_clocks_SetSspClk(&kHz, true);
	}
#endif
}

/* Configure card */
static void stmp37xx_mmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct stmp37xx_mmc_host *host = mmc_priv(mmc);

	TERSE("mmc set ios: Clock %u, vdd %u, bus_mode %u, chip_select %u, power mode %u, bus_width %u\n",
		  ios->clock, ios->vdd, ios->bus_mode, ios->chip_select, ios->power_mode, ios->bus_width );

	if ( ios->bus_mode == MMC_BUSMODE_PUSHPULL ) {
		HW_PINCTRL_PULL1_CLR(BM_PINCTRL_PULL1_BANK1_PIN22);
	}
	else {
		HW_PINCTRL_PULL1_SET(BM_PINCTRL_PULL1_BANK1_PIN22);
	}

	if ( ios->bus_width == MMC_BUS_WIDTH_4 ) {
		host->bus_width_4 = 1;
	}
	else {
		host->bus_width_4 = 0;
	}

	if ( ios->clock > 0 ) {
		stmp37xx_set_sclk_speed( ios->clock );
		host->clkrt = ios->clock;
	}
}

 static const struct mmc_host_ops stmp37xx_mmc_ops = {
 	.request	= stmp37xx_mmc_request,
	.get_ro		= stmp37xx_mmc_get_ro,
	.set_ios	= stmp37xx_mmc_set_ios,
};

/* Probe peripheral for connected cards */
static int stmp37xx_mmc_probe(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct stmp37xx_mmc_host *host = NULL;
	int ret;

	TERSE("Probing for mmc/sd device %s\n", pdev->name);

 	mmc = mmc_alloc_host(sizeof(struct stmp37xx_mmc_host), &pdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto out;
	}

	mmc->ops = &stmp37xx_mmc_ops;
	mmc->f_min = CLOCKRATE_MIN;
	mmc->f_max = CLOCKRATE_MAX;
//	mmc->mode = MMC_MODE_MMC;
	mmc->caps = MMC_CAP_4_BIT_DATA | MMC_CAP_MULTIWRITE;

	/* Maximum block count requests. */
 	mmc->max_hw_segs = SSP_BUFFER_SIZE / 512;
 	mmc->max_phys_segs = SSP_BUFFER_SIZE / 512;
 	//mmc->max_sectors = SSP_BUFFER_SIZE / 512;
	mmc->max_seg_size = SSP_BUFFER_SIZE;


	host = mmc_priv(mmc);
	host->mmc = mmc;
	mmc->ocr_avail = MMC_VDD_32_33|MMC_VDD_33_34;

	init_waitqueue_head(&wait_q);

	platform_set_drvdata(pdev, mmc);

	mmc_add_host(mmc);
	return 0;

 out:
	DEBUG("error in probe\n");
	if (mmc)
		mmc_free_host(mmc);
	return ret;
}

static struct platform_device stmp37xx_mmc_device = {
    .name = "stmp37xx-mmc",
    .id   = -1,
    .dev  = {
	.coherent_dma_mask = ISA_DMA_THRESHOLD,
    }
};


static int _stmp37xx_mmc_init(void)
{
    stmp37xx_set_sclk_speed( CLOCKRATE_MIN );

   	/* Configure data+clk+cmd pins for SSP functionality. */
   	HW_PINCTRL_MUXSEL3_CLR(HW_SSP_IO_PINCTRL_MUXSELECT);

	/* Configure SSP pins data+clk+cmd for 8mA drive strength, 3 volts. */
	HW_PINCTRL_DRIVE6_CLR(HW_SSP_PINCTRL_DRIVE6_MASK);
	HW_PINCTRL_DRIVE7_CLR(HW_SSP_PINCTRL_DRIVE7_MASK);
	HW_PINCTRL_DRIVE6_SET(HW_SSP_PINCTRL_DRIVE6_3V_8MA);
	HW_PINCTRL_DRIVE7_SET(HW_SSP_PINCTRL_DRIVE7_3V_8MA);


	/* Set PWM3 and PWM4 to GPIOs. */
	HW_PINCTRL_MUXSEL4_SET(BM_PINCTRL_MUXSEL4_BANK2_PIN03 | BM_PINCTRL_MUXSEL4_BANK2_PIN04);
	HW_PINCTRL_DOE2_SET(HW_SSP_SOCKET_POWER_BIT_MASK);  /* PWM3=socket power is an output */
	HW_PINCTRL_DOE2_CLR(HW_SSP_WRITE_PROTECT_BIT_MASK); /* PWM4=WP# is an input */

    /* Drive power to the MMC socket */
    HW_PINCTRL_DOUT2_SET(HW_SSP_SOCKET_POWER_BIT_MASK);
    mdelay(100);
    HW_PINCTRL_DOUT2_CLR(HW_SSP_SOCKET_POWER_BIT_MASK);
    mdelay(100);

    { /* card detect  */
	HW_PINCTRL_DOE1_CLR( HW_SSP_CARD_DETECT_BIT_MASK );
	HW_SSP_CARD_DETECT_MUXSEL_INIT();
    }

    // Reset MMC block
    stmp37xx_mmc_reset();
    // Also reset DMA
	stmp37xx_dma_reset_channel(ssp_dma_chain.id);

    /* Set pullups  on the MMC cmd  and data lines. The  pullup on the
     *  cmd line will be set off as dicated by the ios command
     */
    {
	HW_PINCTRL_PULL1_SET(BM_PINCTRL_PULL1_BANK1_PIN22);
	HW_PINCTRL_PULL1_SET(BM_PINCTRL_PULL1_BANK1_PIN24);
	HW_PINCTRL_PULL1_SET(BM_PINCTRL_PULL1_BANK1_PIN25);
	HW_PINCTRL_PULL1_SET(BM_PINCTRL_PULL1_BANK1_PIN26);
	HW_PINCTRL_PULL1_SET(BM_PINCTRL_PULL1_BANK1_PIN27);
	udelay(150);
    }

    /* The  initialisation sequence dictates  sending logical  ones to
     * the card. Of course it requires the clock to be active for this
     * purpose, so  we turn it to  GPIO and create a  waveform on sclk
     * (note that the  clock is otherwise set on  by the dma commands,
     * off otherwise ).
     */
#if 1
    {
	int i = 0;
	HW_PINCTRL_MUXSEL3_SET(0x3 << 14);
	HW_PINCTRL_DOE1_SET(1 << 23);
	/* This will create a clock pulse for about 1ms @ 500Khz */
	for (i = 0; i < 500; i++)
	{
	    HW_PINCTRL_DOUT1_CLR(1 << 23);
	    udelay(1);
	    HW_PINCTRL_DOUT1_SET(1 << 23);
	    udelay(1);
	}
	HW_PINCTRL_MUXSEL3_CLR( (0x3 << 14) );
    }
#endif
    return 1;
}

/* Allocate and initialise the DMA chains */
static int dma_init(void)
{
	void* virtual_addr;
	int i;

	stmp37xx_dma_user_init(&stmp37xx_mmc_device.dev, &dma_user);

	if (!make_circular_dma_chain( &dma_user, &ssp_dma_chain, ssp_dma_descrs, SSP_CHAIN_LEN) )
		return -1;
	stmp37xx_dma_go(ssp_dma_chain.id, &ssp_dma_descrs[0], 0);

	for (i = 0; i < SSP_CHAIN_LEN; i++) {
		stmp37xx_dma_descriptor_t* descr = &ssp_dma_descrs[i];
		virtual_addr = dma_alloc_coherent( &stmp37xx_mmc_device.dev,
										   SSP_BUFFER_SIZE,
										   &ssp_dma_buffers[i],
										   GFP_DMA);
		memset( virtual_addr, 0x4B, SSP_BUFFER_SIZE );
		descr->command->buf_ptr = ssp_dma_buffers[i];
		descr->virtual_buf_ptr = virtual_addr;
	};


	/* Just to make certain that the buffers are clean what's so over */
	arm926_flush_kern_cache_all();

	return 0;
}


static struct platform_driver stmp37xx_mmc_driver = {
 	.probe		= stmp37xx_mmc_probe,
/*  	.remove		= stmp37xx_mmc_remove,  */
	.suspend	= NULL,
	.resume		= NULL,
/* 	.suspend	= stmp37xx_mmc_suspend, */
/* 	.resume		= stmp37xx_mmc_resume, */
	.driver		= {
		.name	= DRIVER_NAME,
		.owner =  THIS_MODULE,
	},
};

static int __init stmp37xx_mmc_init(void)
{
	int result = 0;
	TERSE("Initialising mmc/sd\n");

	_stmp37xx_mmc_init();

	result = platform_driver_register(&stmp37xx_mmc_driver);
	if (result < 0)
		return result;

	TERSE("add platform\n");
	result = platform_device_register(&stmp37xx_mmc_device);
	if (result) {
		platform_driver_unregister(&stmp37xx_mmc_driver);
		return result;
	}

	result = dma_init();

	if ( result ) {
		DEBUG("Error in dma init\n");
		return result;
	}

	{
		int retval = -EINVAL;
		//retval = request_irq(IRQ_SSP1_DMA, ssp_dma_int, 0, "stmp37xx ssp DMA", NULL);
		retval = stmp37xx_request_dma(ssp_dma_chain.id, "stmp37xx ssp DMA", ssp_dma_int, NULL);
		retval = request_irq(IRQ_SSP_ERROR, ssp_error_int, 0, "stmp37xx ssp error", NULL);
		if (retval)
			return retval;
	}

	return result;
}

static void __exit stmp37xx_mmc_exit(void)
{
    int i;

	stmp37xx_free_dma(ssp_dma_chain.id);
    //free_irq(IRQ_SSP1_DMA, NULL);
    free_irq(IRQ_SSP_ERROR, NULL);

    for (i = 0; i < SSP_CHAIN_LEN; i++)
    {
	dma_free_coherent( &stmp37xx_mmc_device.dev,
			   SSP_BUFFER_SIZE,
			   ssp_dma_descrs[i].virtual_buf_ptr,
			   ssp_dma_buffers[i] );

    }

    stmp37xx_dma_user_destroy( &dma_user );

    platform_driver_unregister(&stmp37xx_mmc_driver);
}

module_init(stmp37xx_mmc_init);
module_exit(stmp37xx_mmc_exit);

MODULE_DESCRIPTION("STMP37xx MMC peripheral");
MODULE_LICENSE("GPL");
