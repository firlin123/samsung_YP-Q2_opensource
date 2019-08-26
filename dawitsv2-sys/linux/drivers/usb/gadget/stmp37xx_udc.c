/*
 * linux/drivers/usb/gadget/stmp37xx_udc.c
 * ARC/stmp37xx on-chip high speed USB device controller
 *
 * Copyright (C) 2005 John Ripley, SigmaTel Inc
 * Copyright (C) 2004 Mikko Lahteenmaki, Nordic ID
 * Copyright (C) 2004 Bo Henriksen, Nordic ID
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "stmp37xx_udc.h"

#include <asm/cacheflush.h>

/* By leeth, usb transferring/idle/suspend event process at 20080920 */
#if defined(CONFIG_STMP37XX_USBEVENT)
#include <asm/arch/stmp37xx_usbevent.h>
#endif
#if 0
/* By leeth, control hclk auto slowdown at 20081231 */
#include <asm/arch/stmp37xx_pm.h>
#endif

#define	DRIVER_DESC		"stmp37xx USB Device Controller"
#define	DRIVER_VERSION		__DATE__


struct stmp37xx_udc *the_controller;

static const char driver_name[] = "stmp37xx_udc";
static const char driver_desc[] = DRIVER_DESC;
static const char ep0name[] = "ep0-control";
/* By leeth, add test mode for USB I/F test at 20080806 */
static struct timer_list timer_test_mode;
/* By leeth, usb transferring/idle/suspend event process at 20080920 */
#if defined(CONFIG_STMP37XX_USBEVENT)
static struct timer_list timer_usb_event;
#endif
/* By leeth, for reset device at usb connected, it should be occured 
 * disconnect event to user plain at 20080911 */
static int ep0_configured = 0;

static struct usb_ep_ops stmp37xx_ep_ops = {
	.enable = stmp37xx_ep_enable,
	.disable = stmp37xx_ep_disable,

	.alloc_request = stmp37xx_alloc_request,
	.free_request = stmp37xx_free_request,

	.queue = stmp37xx_queue,
	.dequeue = stmp37xx_dequeue,

	.set_halt = stmp37xx_set_halt,
	.fifo_status = stmp37xx_fifo_status,
	.fifo_flush = stmp37xx_fifo_flush,
};


/* By leeth, usb transferring/idle/suspend event process at 20080920 */
#if defined(CONFIG_STMP37XX_USBEVENT)
static void send_idle_event(unsigned long arg)
{
	struct usbevent_s event;

	event.type = USB_TRANSFER_EVENT;
	event.status = USB_EVENT_IDLE;
	send_usbevent_msg(&event);
}

static void start_idle_timer(void)
{
	setup_timer(&timer_usb_event, send_idle_event, 0);
	mod_timer(&timer_usb_event, jiffies + 200); /* 2 secs */
}

static void stop_idle_timer(void)
{
	del_timer(&timer_usb_event);
}
#endif

/* By leeth, for reset device at usb connected, it should be occured 
 * disconnect event to user plain at 20080911 */
static void set_configured(void)
{
	ep0_configured = 1;
}

/* By leeth, for reset device at usb connected, it should be occured 
 * disconnect event to user plain at 20080911 */
static void reset_configured(void)
{
	ep0_configured = 0;
}

/* By leeth, for reset device at usb connected, it should be occured 
 * disconnect event to user plain at 20080911 */
static int is_configured(void)
{
	return ep0_configured;
}

/* By leeth, add test mode for USB I/F test at 20080806 */
/* Enter usb test mode */
static void set_test_mode(unsigned long test_mode)
{
	test_mode = test_mode & 0xff;

	printk("USB Test Mode %d\n", test_mode);
	
	/* USB Endpoint0 TX Toggle Reset */
	BW_USBCTRL_ENDPTCTRLn_TXR(0, 1);

	/* Disable Asynchronous Schedule */
	BW_USBCTRL_USBCMD_PSE(0);
	/* Disable Periodic Schedule */
	BW_USBCTRL_USBCMD_ASE(0);

	/* Enter Port Suspend State */
	BW_USBCTRL_PORTSC1_SUSP(1);

	/* Stop USB */
	BW_USBCTRL_USBCMD_RS(0);

	/* Set Test Mode */
	/* 
	 * 0000b (0) TEST_DISABLE
	 * 0001b (1) TEST_J_STATE
	 * 0010b (2) TEST_K_STATE
	 * 0011b (3) TEST_J_SE0_NAK
	 * 0100b (4) TEST_PACKET
	 * 0101b (5) TEST_FORCE_ENABLE_HS
	 * 0110b (6) TEST_FORCE_ENABLE_FS
	 * 0111b (7) TEST_FORCE_ENABLE_LS
	 */
	BW_USBCTRL_PORTSC1_PTC(test_mode);

	/* Start USB */
	BW_USBCTRL_USBCMD_RS(1);

	/* Set Test Mode */
	BW_USBCTRL_PORTSC1_PTC(test_mode);

} /* EndBody */

/* Prime an endpoint */
static void ep0_prime(struct stmp37xx_udc *dev, int length)
{
	struct stmp37xx_ep *ep = &dev->ep[0];
    	u32 prime_bitmask = ep->ep_bitmask;
    	volatile u32 *dqh = ep->dqh;
	/* Cache flushing */
	unsigned long virt_start = 0;
	unsigned long virt_end = 0;
		
    	
	STMPUDC_DEBUG("\n");

	if (dev->ep0_state != SEND_REPLY &&
	    dev->ep0_state != SEND_ACK &&
	    dev->ep0_state != SEND_ZLT) {
 		STMPUDC_DEBUG("EP0OUT prime\n");
 		
		/* EP0OUT prime mask */
		prime_bitmask = (1<<0);
		
		/* DQH is different too */
		dqh = ep->out_dqh;
	}

	if (length >= 0) {
		/* Setup DTD. This never crosses a page for EP0. */
		ep->dtd[0] = 1;
		ep->dtd[1] = (length << 16) | (1<<15) | 0x80;
		ep->dtd[2] = dev->ep0_bounce_phys;
		/* Won't cross a page because we allocate in 64 byte chunks. */
		
		/* Setup DQH data pointer */
		dqh[2] = (u32) ep->dtd_phys;
		
	    	/* Note length for completion */
		ep->dtd_bytes = length;
    	}
    	else
    	{
	    	/* No data hence no DTD (for setup packets) */
		dqh[2] = 1;
		ep->dtd_bytes = 0;
	}

	virt_start = (unsigned long)ep;
	virt_end = virt_start + sizeof(struct stmp37xx_ep);
	
	/* Clean the buffer before DMA write to host */
	arm926_dma_clean_range(virt_start, virt_end);
	
	virt_start = (unsigned long)dqh;
	virt_end = virt_start + QH_SIZE;
	
	/* Clean the buffer before DMA write to host */
	arm926_dma_clean_range(virt_start, virt_end);

	virt_start = (unsigned long)ep->dtd;
	virt_end = virt_start + TD_SIZE;

	/* Clean the buffer before DMA write to host */
	arm926_dma_clean_range(virt_start, virt_end);

	/* Prime it */
	HW_USBCTRL_ENDPTPRIME_WR(prime_bitmask);
	while(HW_USBCTRL_ENDPTPRIME_RD() & prime_bitmask)
		;

	ep->primed = 1;
}

static unsigned fill_dtd_pages(unsigned *dtd, char *virt,
			       unsigned bytes, unsigned max_packet)
{
	unsigned entry;
	unsigned xfer_bytes;

	// First entry is in word 2. This can have a sub-4KB offset,
	// and transfers up to the end of the 4KB page.
	dtd[2] = stmp_virt_to_phys(virt);
	STMPUDC_DEBUG("### %p %p\n", dtd[2], virt);
	xfer_bytes = 4096 - (((unsigned) virt) & 4095);
	if(xfer_bytes > bytes)
		xfer_bytes = bytes;
	virt += xfer_bytes;
	bytes -= xfer_bytes;

	// Fill in up to 3 more entries
	for(entry = 1; entry < 4 && bytes > 0; ++entry)
	{
		// Transfer up to 4KB
		unsigned this_bytes = bytes;
		if(this_bytes > 4096)
			this_bytes = 4096;

		// Special handling for the last entry in the DTD.
		if(entry == 3)
		{
			// Calculate size of the last packet resulting
			// from this DTD.
			unsigned last_packet =
				(xfer_bytes + this_bytes) & (max_packet - 1);
			if(last_packet != 0 && this_bytes != bytes)
			{
				// Will be a short packet, but the
				// entire transfer will not be
				// complete. This will cause an early
				// termination. To correct, shave of
				// some bytes so it's a multiple of
				// the packet size.
				this_bytes -= last_packet;
			}
		}
		dtd[2 + entry] = stmp_virt_to_phys(virt);
		STMPUDC_DEBUG("### %p %p\n", dtd[2+entry], virt);
		virt += this_bytes;
		bytes -= this_bytes;
		xfer_bytes += this_bytes;
	}

	// Return the number of bytes filled
	return xfer_bytes;
}

static void epn_prime(struct stmp37xx_ep *ep, int length)
{
    	u32 prime_bitmask = ep->ep_bitmask;
    	volatile u32 *dqh = ep->dqh;
	/* Cache flushing */
	unsigned long virt_start = 0;
	unsigned long virt_end = 0;
    	
	STMPUDC_DEBUG("ep_index: %d length: %d\n", ep_index(ep), length);
	STMPUDC_DEBUG("dtd: %p dqh: %p\n", ep->dtd, dqh);

	BUG_ON(length < 0);

	/* Note length for completion */
	ep->dtd_bytes = length;
	
	/* Setup the DTD header */
	ep->dtd[0] = 1;
	ep->dtd[1] = (length << 16) | (1<<15) | 0x80;

	/* Setup DQH data pointer */
	dqh[2] = (u32) ep->dtd_phys;

	virt_start = (unsigned long)ep;
	virt_end = virt_start + sizeof(struct stmp37xx_ep);
	
	/* Clean the buffer before DMA write to host */
	arm926_dma_clean_range(virt_start, virt_end);
	
	virt_start = (unsigned long)dqh;
	virt_end = virt_start + QH_SIZE;
	
	/* Clean the buffer before DMA write to host */
	arm926_dma_clean_range(virt_start, virt_end);

	virt_start = (unsigned long)ep->dtd;
	virt_end = virt_start + TD_SIZE;

	/* Clean the buffer before DMA write to host */
	arm926_dma_clean_range(virt_start, virt_end);

	/* Prime it */
	HW_USBCTRL_ENDPTPRIME_WR(prime_bitmask);
	while(HW_USBCTRL_ENDPTPRIME_RD() & prime_bitmask)
		;

	ep->primed = 1;
}


static int is_usb_connected(struct stmp37xx_udc *dev)
{
	unsigned long flags;
	unsigned old_pwd;
	int isoff, connected;

	STMPUDC_DEBUG("\n");
	
	spin_lock_irqsave(&dev->lock, flags);
	
	// Power up the PHY if it's powered down
	old_pwd = HW_USBPHY_PWD_RD();
	HW_USBPHY_PWD_WR(0);

	isoff = HW_USBPHY_CTRL_RD() & (BM_USBPHY_CTRL_SFTRST | BM_USBPHY_CTRL_CLKGATE);

	if(isoff) {
		// Minimal PHY startup
		HW_USBPHY_CTRL_CLR(BM_USBPHY_CTRL_SFTRST | BM_USBPHY_CTRL_CLKGATE); 
		HW_USBPHY_PWD.U = 0;
	}

	// Enable resistors
	HW_USBPHY_DEBUG_SET(BM_USBPHY_DEBUG_ENHSTPULLDOWN);
	HW_USBPHY_DEBUG_CLR(BM_USBPHY_DEBUG_HSTPULLDOWN);
	HW_USBPHY_CTRL.B.ENDEVPLUGINDETECT = 1;

	// Wait 250us
	udelay(250);

	connected = (HW_USBPHY_STATUS.B.DEVPLUGIN_STATUS ? 1:0);

	// Turn off plug in detection again (saves power?)
	HW_USBPHY_CTRL.B.ENDEVPLUGINDETECT = 0;

	// Put in previous power state
	HW_USBPHY_PWD_WR(old_pwd);
	if (isoff) {
		// Shutdown PHY again
		HW_USBPHY_PWD.U = 0xffffffff;
		HW_USBPHY_CTRL_SET(BM_USBPHY_CTRL_CLKGATE);
	}

	spin_unlock_irqrestore(&dev->lock, flags);

	return connected;
}

/*
 * 	udc_reinit - initialize software state
 */
static void udc_reinit(struct stmp37xx_udc *dev)
{
	u32 i;

	STMPUDC_DEBUG("\n");

	/* device/ep0 records init */
	INIT_LIST_HEAD(&dev->gadget.ep_list);
	INIT_LIST_HEAD(&dev->gadget.ep0->ep_list);
	dev->ep0_state = WAIT_FOR_SETUP;

	/* basic endpoint records init */
	for (i = 0; i < UDC_MAX_ENDPOINTS; i++) {
		struct stmp37xx_ep *ep  =&dev->ep[i];
		
		if (i != 0)
			list_add_tail(&ep->ep.ep_list, &dev->gadget.ep_list);

		ep->desc = 0;
		ep->stopped = 0;
		ep->primed = 0;
		INIT_LIST_HEAD(&ep->request_head);
	}

	/* the rest was statically initialized, and is read-only */
}

static void udc_dma_free (struct stmp37xx_udc *dev)
{
	int i;
	
	if (!dev->device_pool) {
		return;
	}

	/* Free a usb_request for EP0 (to send reply data) */
	kfree(dev->ep0_req);
	dev->ep0_req = NULL;
	
	/* Free a pool for QH's and DTD's in DMA memory */
	dma_pool_destroy(dev->device_pool);
	dev->device_pool = NULL;

	/* Reset MAXP values for each */
	for (i = 0; i < UDC_MAX_ENDPOINTS; ++i) {
		struct stmp37xx_ep *ep = &dev->ep[i];

		if (i == 0) {
			ep->out_dqh = NULL;
			ep->out_dqh_phys = NULL;
			ep->dqh = NULL;
			ep->dqh_phys = NULL;
		} else if (i == 1 || i == 3) {
			ep->dqh = NULL;
			ep->dqh_phys = NULL;
		} else if (i == 2) {
			ep->dqh = NULL;
			ep->dqh_phys = NULL;
		}
	}
	/* Reset a DTD and packet buffer for EP0IN/EP0OUT traffic */
	dev->ep[0].dtd = NULL;

	/* Reset Data space for EP0 (small) */
	dev->ep0_bounce = NULL;
	/* Reset  DTDs for the other endpoints */
	for(i = 1; i < UDC_MAX_ENDPOINTS; ++i) {
		struct stmp37xx_ep *ep = &dev->ep[i];
		// 64 byte DMA buffer for the DTD
		ep->dtd = NULL;
	}

	udc_reinit(dev);
}

/*
 * 	udc_disable - disable USB device controller
 */
static void udc_disable(struct stmp37xx_udc *dev)
{
	STMPUDC_DEBUG("\n");

	udc_set_address(dev, 0);

	/* Disable interrupts */

	/* if hardware supports it, disconnect from usb */
	/* make_usb_disappear(); */

	/* Reset to default values (bit 1 is reset) */
	BW_USBCTRL_USBCMD_RST(1);

	/* Wait for reset to complete (bit resets itself to 0) */
	while(HW_USBCTRL_USBCMD_RD() & BM_USBCTRL_USBCMD_RST)
		;

	/* Disable the USB */
	// Power off the PHY
	HW_USBPHY_PWD_WR(0xffffffff);
	HW_USBPHY_CTRL_SET(BM_USBPHY_CTRL_CLKGATE | BM_USBPHY_CTRL_SFTRST);

	// Turn off the USB clocks
	HW_CLKCTRL_PLLCTRL0_CLR(BM_CLKCTRL_PLLCTRL0_EN_USB_CLKS);
	HW_DIGCTL_CTRL_SET(BM_DIGCTL_CTRL_USB_CLKGATE);

	//udc_dma_free(dev);

	dev->ep0_state = WAIT_FOR_SETUP;
	dev->gadget.speed = USB_SPEED_UNKNOWN;
	dev->usb_address = 0;

	/* By leeth, for reset device at usb connected, it should be occured 
	 * disconnect event to user plain at 20080911 */
	reset_configured();

#if 0
	/* By leeth, control hclk auto slowdown at 20081231 */
	Enable_HclkSlow(1);
#endif
}

static void udc_dma_alloc (struct stmp37xx_udc *dev)
{
	int i;

	if (dev->device_pool) {
		udc_reinit(dev);
		return;
	}

	/* Allocate a pool for QH's and DTD's in DMA memory */
	dev->device_pool =
		dma_pool_create("udc_pool",
				dev->dev,
				64,
				64 /* byte alignment - 64 bytes is ok for QH and DTD */,
				0 /* no page-crossing issues */);
	/**** NOTE! QH's must be allocated first, and contiguously, as they have to start
		  on a 2k boundary */

	/* Set MAXP values for each */
	for (i = 0; i < UDC_MAX_ENDPOINTS; ++i) {
		struct stmp37xx_ep *ep = &dev->ep[i];
		volatile u32 *virt_out, *virt_in;
		dma_addr_t phys_out, phys_in;

		/* Endpoint list:
		 * EP0OUT
		 * EP0IN
		 * EP1IN
		 * EP2OUT
		 * EP3IN
		 */
		
		/* Each endpoint has an OUT DQH and IN DQH regardless
		 * of whether it's actually synthesized and
		 * used. We'll just not use the ones that don't exist,
		 * but we still have to allocate their space. */
		virt_out = (volatile u32 *) dma_pool_alloc(dev->device_pool,
							   GFP_ATOMIC,
							   &phys_out);

		virt_in = (volatile u32 *) dma_pool_alloc(dev->device_pool,
							  GFP_ATOMIC,
							  &phys_in);

		memset((void *) virt_out, 0, QH_SIZE);
		memset((void *) virt_in, 0, QH_SIZE);

		/* Terminate them */
		virt_out[2] = 1;
		virt_in[2] = 1;

		if (i == 0) {
			ep->out_dqh = virt_out;
			ep->out_dqh_phys = phys_out;
			ep->dqh = virt_in;
			ep->dqh_phys = phys_in;
			STMPUDC_DEBUG("EP%dOUT PTR=%p QH=%p (phys %08x)\n",
				  i, ep,
				  ep->out_dqh, ep->out_dqh_phys);
			STMPUDC_DEBUG("EP%dIN  PTR=%p QH=%p (phys %08x)\n",
				  i, ep,
				  ep->dqh, ep->dqh_phys);
		
		} else if (i == 1 || i == 3) {
			ep->dqh = virt_in;
			ep->dqh_phys = phys_in;
			STMPUDC_DEBUG("EP%dIN  PTR=%p QH=%p (phys %08x)\n",
				  i, ep, ep->dqh, ep->dqh_phys);
		} else if (i == 2) {
			ep->dqh = virt_out;
			ep->dqh_phys = phys_out;
			STMPUDC_DEBUG("EP%dOUT PTR=%p QH=%p (phys %08x)\n",
				  i, ep, ep->dqh, ep->dqh_phys);
		}
		
		flush(ep);
	}
	/* Allocate a DTD and packet buffer for EP0IN/EP0OUT traffic */
	dev->ep[0].dtd = dma_pool_alloc(dev->device_pool, GFP_ATOMIC,
					&dev->ep[0].dtd_phys);
	memset((void *) dev->ep[0].dtd, 0, TD_SIZE);

	/* Data space for EP0 (small) */
	dev->ep0_bounce = dma_pool_alloc(dev->device_pool, GFP_ATOMIC,
					 &dev->ep0_bounce_phys);
	/* Allocate DTDs for the other endpoints */
	for(i = 1; i < UDC_MAX_ENDPOINTS; ++i) {
		struct stmp37xx_ep *ep = &dev->ep[i];
		// 64 byte DMA buffer for the DTD
		ep->dtd = (volatile u32 *) dma_pool_alloc(dev->device_pool,
							  GFP_ATOMIC,
							  &ep->dtd_phys);
	}

	/* Allocate a usb_request for EP0 (to send reply data) */
	dev->ep0_req =
		(struct usb_request *) kmalloc(sizeof(struct usb_request), GFP_KERNEL);
}

#define BYTES2MAXP(x)	(x / 8)
#define MAXP2BYTES(x)	(x * 8)

/* until it's enabled, this UDC should be completely invisible
 * to any USB host.
 */
static void udc_enable(struct stmp37xx_udc *dev)
{
	STMPUDC_DEBUG("\n");

#if 0
	/* By leeth, control hclk auto slowdown at 20081231 */
	Enable_HclkSlow(0);
#endif

	dev->gadget.speed = USB_SPEED_UNKNOWN;

	/* By leeth, for reset device at usb connected, it should be occured 
	 * disconnect event to user plain at 20080911 */
	reset_configured();

	/*
	 * C.f Chapter 18.1.3.1 Initializing the USB
	 */

	/* Disable the USB */

	/* Enable SUSPEND */

	/*** Power on PHY */

	/* Set these bits so that we can force the OTG bits high so the ARC core operates properly */
	HW_POWER_CTRL_CLR(BM_POWER_CTRL_CLKGATE);
	HW_POWER_DEBUG_SET(BM_POWER_DEBUG_VBUSVALIDPIOLOCK | BM_POWER_DEBUG_AVALIDPIOLOCK | BM_POWER_DEBUG_BVALIDPIOLOCK);
	HW_POWER_STS_SET(BM_POWER_STS_BVALID | BM_POWER_STS_AVALID | BM_POWER_STS_VBUSVALID);

	/* Remove CLKGATE and SFTRST */
	HW_USBPHY_CTRL_CLR(BM_USBPHY_CTRL_CLKGATE | BM_USBPHY_CTRL_SFTRST);

#ifdef CONFIG_STMP36XX_GEN_3600
	/* Ungate USB clock */
	HW_DIGCTL_CTRL_CLR(BM_DIGCTL_CTRL_USB_CLKGATE);
	HW_CLKCTRL_PLLCTRL0_SET(BM_CLKCTRL_PLLCTRL0_EN_USB_CLKS);
	HW_CLKCTRL_UTMICLKCTRL_CLR(0xc0000000U);
#else
	// Turn on the USB clocks
	HW_CLKCTRL_PLLCTRL0_SET(BM_CLKCTRL_PLLCTRL0_EN_USB_CLKS);
	HW_DIGCTL_CTRL_CLR(BM_DIGCTL_CTRL_USB_CLKGATE);

	// Power up the PHY
	HW_USBPHY_PWD_WR(0);
#endif

#ifdef CONFIG_STMP36XX_GEN_3600
	/* Stuff suggested by Fareed */
	HW_CLKCTRL_PLLCTRL0.B.PLLCPNSEL = BV_CLKCTRL_PLLCTRL0_PLLCPNSEL__TIMES_04;
    
	HW_CLKCTRL_PLLCTRL0_SET(BM_CLKCTRL_PLLCTRL0_PLLCPDBLIP);
	BW_DIGCTL_RAMCTRL_TEST_MARGIN(1);	/* Change SRAM margin (?!?!) */

	/* Get debug info out of the PHY */
	//HW_USBPHY_DEBUG.B.CLKGATE = 0;

	/* In order to enhance eye diagram, we must set TXCAL45DP and TXCAL45DN in HW_USBPHY_TX */
	BW_USBPHY_TX_D_CAL(0x07);

	BW_USBPHY_TX_TXCAL45DP(0x0);
	BW_USBPHY_TX_TXCAL45DN(0x0);

	HW_CLKCTRL_PLLCTRL0.B.PLLCPNSEL = BV_CLKCTRL_PLLCTRL0_PLLCPNSEL__TIMES_05;
	HW_CLKCTRL_PLLCTRL0.B.PLLV2ISEL = BV_CLKCTRL_PLLCTRL0_PLLV2ISEL__LOWEST;
#endif
	/* Set precharge bit to cure overshoot problems at the start of packets */
	HW_USBPHY_CTRL_SET(BM_USBPHY_CTRL_ENHSPRECHARGEXMIT);


	#if 0
	HW_CLKCTRL_PLLCTRL0.B.CP_SEL = 2;
	HW_AUDIOOUT_REFCTRL.B.VDDXTAL_TO_VDDD = 1;

	BF_USBPHY_TX_D_CAL(0x07);
	BW_USBPHY_TX_TXCAL45DP(0x00);
	BW_USBPHY_TX_TXCAL45DN(0x00);
	#endif

	#if 0
	HW_USBPHY_DEBUG.B.CLKGATE = 0;
    // This is equivalent to the code in PHY_IsConnected(), except there it is
    //  done with SET/CLR macros
    HW_USBPHY_DEBUG.B.ENHSTPULLDOWN = 0x3;
    HW_USBPHY_DEBUG.B.HSTPULLDOWN = 0x0;
	#endif

	/*** Setup controller */
	
	/* Stop the controller (run-stop RS=0) */
	HW_USBCTRL_USBCMD_WR(HW_USBCTRL_USBCMD_RD() & ~1);

	/* Reset to default values (bit 1 is reset) */
	HW_USBCTRL_USBCMD_WR(HW_USBCTRL_USBCMD_RD() | 2);

	/* Wait for reset to complete (bit resets itself to 0) */
	while(HW_USBCTRL_USBCMD_RD() & 2)
		;

	/* Clear ITC bits in USBCMD */
	HW_USBCTRL_USBCMD_WR(HW_USBCTRL_USBCMD_RD() & 0xff00ffff);

	/* Set controller mode in USBMODE register to device mode,
	   little endian */
	HW_USBCTRL_USBMODE_WR(0x2);

	/* Set address 0 */
	HW_USBCTRL_PERIODICLISTBASE_WR(0);

	/* Make sure the 16 MSBs are zero (weird) */
	HW_USBCTRL_ENDPTSETUPSTAT_WR(0);
	
	/* Zero the semaphore register */
	HW_USBCTRL_ENDPTSETUPSTAT_WR(0xffff);

	/* Flush all endpoints */
	HW_USBCTRL_ENDPTFLUSH_WR(0xffffffffU);

	udc_dma_alloc(dev);

	/* Point queue head at the DQH's */
	HW_USBCTRL_ASYNCLISTADDR_WR((unsigned) dev->ep[0].out_dqh_phys);

	/*** Configure USB device port (port 1) */

	/* 8 bit 60MHz UTMI PHY */

	/* Allow FS or HS */
	HW_USBCTRL_PORTSC1_WR(0x00000005);

	/* Initialise endpont 0 properties (tx/rx enable) */
	HW_USBCTRL_ENDPTCTRLn_WR(0, 0x00800080);

	/* Set burst size to 8, so it works with the EMI
	   This register is UNDOCUMENTED. */
	HW_USBCTRL_BURSTSIZE_WR((8 << 8) | (8 << 0));

	/* Turn on TX/RX on endpoint 0 */
	HW_USBCTRL_ENDPTCTRLn_WR(0, 0x00800080);

	/* Enable interrupt reception */
	HW_USBCTRL_USBINTR_WR(    (1 << 8) |	/* Sleep enable */
				  (1 << 6) |	/* USB Reset */
				  (1 << 4) |	/* System error */
				  (1 << 2) |	/* Port change */
				  (1 << 1) |	/* USB error */
				  (1 << 0));	/* USB interrupt */


	/* Set usb to RUN mode */
	HW_USBCTRL_USBCMD_WR(HW_USBCTRL_USBCMD_RD() | 1);

	/* Prime EP0 for setup packets, no DTD required as SETUPs are
	 * stored in the DQH */

	/* Setup no zero length terminate, packet size, interrupt on setup */
	dev->ep[0].out_dqh[0] = (1 << 29) | (64 << 16) | (1 << 15);
	dev->ep[0].out_dqh[2] = 1;
	dev->ep0_state = WAIT_FOR_SETUP;
	ep0_prime(dev, -1);
}

/*
  Register entry point for the peripheral controller driver.
*/
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct stmp37xx_udc *dev = the_controller;
	int retval;

	STMPUDC_DEBUG("%s\n", driver->driver.name);

	if (!driver
	    || !driver->bind
	    || !driver->unbind || !driver->disconnect || !driver->setup)
		return -EINVAL;
	if (!dev)
		return -ENODEV;
	if (dev->driver)
		return -EBUSY;


	/* first hook up the driver ... */
	dev->driver = driver;
	dev->gadget.dev.driver = &driver->driver;

	retval = device_add(&dev->gadget.dev);
	if (retval) {
		printk("%s: device_add error %d\n",
		       dev->gadget.name, retval);
		return retval;
	}
	
	retval = driver->bind(&dev->gadget);
	if (retval) {
		printk("%s: bind to driver %s --> error %d\n", dev->gadget.name,
		       driver->driver.name, retval);
		device_del(&dev->gadget.dev);

		dev->driver = 0;
		dev->gadget.dev.driver = 0;
		return retval;
	}

	/* ... then enable host detection and ep0; and we're ready
	 * for set_configuration as well as eventual disconnect.
	 * NOTE:  this shouldn't power up until later.
	 */
	printk("%s: registered gadget driver '%s'\n", dev->gadget.name,
	       driver->driver.name);

	//usbact not needed - to cehck if usbact is needed
	udc_enable(dev);

	return 0;
}

EXPORT_SYMBOL(usb_gadget_register_driver);

/*
  Unregister entry point for the peripheral controller driver.
*/
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct stmp37xx_udc *dev = the_controller;
	unsigned long flags;

	if (!dev)
		return -ENODEV;
	if (!driver || driver != dev->driver)
		return -EINVAL;

	spin_lock_irqsave(&dev->lock, flags);
	dev->driver = 0;
	stop_activity(dev, driver);
	spin_unlock_irqrestore(&dev->lock, flags);

	driver->unbind(&dev->gadget);
	device_del(&dev->gadget.dev);

	udc_disable(dev);

	STMPUDC_DEBUG("unregistered gadget driver '%s'\n", driver->driver.name);
	return 0;
}

EXPORT_SYMBOL(usb_gadget_unregister_driver);

/*
 *	done - retire a request; caller blocked irqs
 *  INDEX register is preserved to keep same
 */
static void done(struct stmp37xx_ep *ep, struct stmp37xx_request *req, int status)
{
	unsigned int stopped = ep->stopped;

	STMPUDC_DEBUG("\n");
	
	list_del_init(&req->list);

	if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	if (status && status != -ESHUTDOWN)
		STMPUDC_DEBUG("complete %s req %p stat %d len %u/%u\n",
		      ep->ep.name, &req->req, status,
		      req->req.actual, req->req.length);

	/* don't modify queue heads during completion callback */
	ep->stopped = 1;

	spin_unlock(&ep->dev->lock);
	req->req.complete(&ep->ep, &req->req);
	spin_lock(&ep->dev->lock);

	ep->stopped = stopped;
}

/*
 * 	nuke - dequeue ALL requests
 */
void nuke(struct stmp37xx_ep *ep, int status)
{
	struct stmp37xx_request *req;
	struct stmp37xx_request *reqb = NULL;

	STMPUDC_DEBUG("\n");

	/* Flush FIFO */
	flush(ep);

	/* called with irqs blocked */
	while (!list_empty(&ep->request_head)) {
		req = list_entry(ep->request_head.next,
				 struct stmp37xx_request, list);
/* hack */
		if (req == reqb) {
			break;
		}
		reqb = req;
/* hack */
		done(ep, req, status);
	}
}

/** Flush EP
 */
static void flush(struct stmp37xx_ep *ep)
{
	STMPUDC_DEBUG("\n");

	if(ep->primed)
	{
		// Wait for any primes to complete
		while(HW_USBCTRL_ENDPTPRIME_RD() & ep->ep_bitmask)
			;
	        
		// Flush the EP & wait for flush to complete
		HW_USBCTRL_ENDPTFLUSH_WR(ep->ep_bitmask);
		while(HW_USBCTRL_ENDPTFLUSH_RD() & ep->ep_bitmask)
			;

		// Force EP to complete
		HW_USBCTRL_ENDPTCOMPLETE_WR(ep->ep_bitmask);

		// Reprogram DQH with max packet size (it may have changed)
		// This is the ideal time to do it, as the whole EP is idle.
		memset((void*)ep->dqh, 0, 64);
		// Setup no zero length terminate, packet size, interrupt on setup.
		ep->dqh[0] = (1 << 29) | (ep->ep.maxpacket << 16) | (1 << 15);
		// Set 'next' pointer to termination.
		ep->dqh[2] = 1;

		/* Now it's reset */
		ep->primed = 0;
		ep->dtd_bytes = 0;
	}
}

static void stop_activity(struct stmp37xx_udc *dev,
			  struct usb_gadget_driver *driver)
{
	int i;

	STMPUDC_DEBUG("\n");
	/* don't disconnect drivers more than once */
	if (dev->gadget.speed == USB_SPEED_UNKNOWN)
		driver = 0;
	dev->gadget.speed = USB_SPEED_UNKNOWN;

	/* prevent new request submissions, kill any outstanding requests  */
	for (i = 0; i < UDC_MAX_ENDPOINTS; i++) {
		struct stmp37xx_ep *ep = &dev->ep[i];
		ep->stopped = 1;
		nuke(ep, -ESHUTDOWN);
	}

	/* report disconnect; the driver is already quiesced */
	if (driver) {
		spin_unlock(&dev->lock);
		driver->disconnect(&dev->gadget);
		spin_lock(&dev->lock);
	}

	/* re-init driver-visible data structures */
	udc_reinit(dev);
}

/* By leeth, for USB command verifier test at 20081110 */
/** After set configuration, you must clear all ep state.
 */
static inline void clear_ep_state(struct stmp37xx_udc *dev)
{
	unsigned i;

	/* hardware SET_{CONFIGURATION,INTERFACE} automagic resets endpoint
	 * fifos, and pending transactions mustn't be continued in any case.
	 */
	for(i = 1; i < UDC_MAX_ENDPOINTS; i++)
	{
		nuke(&dev->ep[i], -ECONNABORTED);
	}
}

/** Handle USB RESET interrupt
 */
static void stmp37xx_reset_intr(struct stmp37xx_udc *dev)
{
	/* Does not work always... */

	STMPUDC_DEBUG("\n");

	// Set address 0
	HW_USBCTRL_PERIODICLISTBASE_WR(0);

	// Clear all setup token semaphores
	HW_USBCTRL_ENDPTSETUPSTAT_WR(HW_USBCTRL_ENDPTSETUPSTAT_RD());

	// Clear all endpoint complete bits
	HW_USBCTRL_ENDPTCOMPLETE_WR(HW_USBCTRL_ENDPTCOMPLETE_RD());

	STMPUDC_DEBUG("USBCMD register is: 0x%08x\n", HW_USBCTRL_USBCMD_RD());
	// Cancel all primed status by waiting for prime to read zero then
	// writing 0xfffffffff to flush
	while(HW_USBCTRL_ENDPTPRIME_RD())
	{
		STMPUDC_DEBUG("ENDPTPRIME is 0x%08x\n", HW_USBCTRL_ENDPTPRIME_RD());
	}
	HW_USBCTRL_ENDPTFLUSH_WR(0xffffffff);

	// Note that we're resetting
	dev->resetting = 1;

	// DON'T RESET EPs! This is done by the port change, which will happen
	// after the reset

	//m_want_link_event = true;

#if 0
	/* Re-enable UDC */
	udc_enable(dev);
#endif

	/* By leeth, for reset device at usb connected, it should be occured 
	 * disconnect event to user plain at 20080911 */
#if 1
	if(is_configured())
	{
		reset_configured();
		stop_activity(dev, dev->driver);
	/* By leeth, usb transferring/idle/suspend event process at 20080920 */
#if defined(CONFIG_STMP37XX_USBEVENT)
		{
			struct usbevent_s event;

			stop_idle_timer();
			
			event.type = USB_TRANSFER_EVENT;
			event.status = USB_EVENT_IDLE;
			send_usbevent_msg(&event);
		}
#endif
	}
#else
	dev->gadget.speed = USB_SPEED_FULL;
#endif

	STMPUDC_DEBUG("Reset done\n");
}

static void stmp37xx_port_status_intr(struct stmp37xx_udc *dev)
{
	unsigned portsc = HW_USBCTRL_PORTSC1_RD();
	unsigned pspd = (portsc >> 26) & 3;
	int new_speed;

	// If we just saw a reset, this port status change would have been
	// triggered by the end of the reset
	if (dev->resetting) {
		dev->resetting = 0;
	}

	// Only take the new speed if the reset has finished on the bus
	if (!(portsc & (1<<8))) {
		int i;
		STMPUDC_DEBUG("Port change: %ssuspended, %s\n",
		      (portsc & (1 << 7)) ? "" : "not ",
		      (portsc & 1) ? "attached" : "detached");

		// Changed speed?
		new_speed = (pspd == 2) ? USB_SPEED_HIGH:USB_SPEED_FULL;

		if (new_speed != dev->gadget.speed) {
			// Changed speed from last time, reset EP lengths
			dev->gadget.speed = new_speed;

			if(new_speed == USB_SPEED_HIGH)
			    STMPUDC_DEBUG("High speed\n");
			else
			    STMPUDC_DEBUG("Full speed\n");
		}

		// Reset all the endpoints (speed detect happens after reset)
		for(i = 0; i < UDC_MAX_ENDPOINTS; ++i)
			flush(&dev->ep[i]);
	}
}

/*
 *	stmp37xx usb client interrupt handler.
 */
static irqreturn_t stmp37xx_udc_irq(int irq, void *_dev)
{
	struct stmp37xx_udc *dev = _dev;

	spin_lock(&dev->lock);

	for (;;) {
		unsigned raw_device_status = HW_USBCTRL_USBSTS_RD();
		// Apparently there's a bug in the ARC core where status isn't masked.
		unsigned device_status = raw_device_status & HW_USBCTRL_USBINTR_RD();
		unsigned setup_status = HW_USBCTRL_ENDPTSETUPSTAT_RD();
		unsigned endpoint_completions = HW_USBCTRL_ENDPTCOMPLETE_RD();
		STMPUDC_DEBUG("\nIRQ %x %x %x\n", device_status, setup_status, endpoint_completions);
		//0x%08x 0x%08x 0x%08x\n",
		//device_status, setup_status, endpoint_completions);

		// End loop if no work is left pending.
		if (!device_status & !setup_status && !endpoint_completions) {
			//STMPUDC_DEBUG("Done ISR\n");
			break;
		}

		if (device_status) {
			// Clear interrupts.
			HW_USBCTRL_USBSTS_WR(device_status);

			// USB reset
			if (device_status & (1 << 6)) {
				STMPUDC_DEBUG("USB reset\n");
				stmp37xx_reset_intr(dev);
			}

			// Port status change
			if (device_status & (1 << 2)) {
				stmp37xx_port_status_intr(dev);
			}

			// Suspend
			if (device_status & (1 << 8)) {
				STMPUDC_DEBUG("Suspend\n");

				// Exit loop
				//m_want_link_event = true;
				break;
			}

			if (device_status & (1 << 1)) {
				STMPUDC_DEBUG("Device error\n");        
			}
		}

		if (setup_status) {
			stmp37xx_ep0_setup(dev);
		}

		if (endpoint_completions) {
			unsigned ep;

			//printk("EP completions %08x\n", endpoint_completions);
			for (ep = 0; ep < UDC_MAX_ENDPOINTS; ++ep) {
				if (endpoint_completions & (1 << ep)) {
					//printk("EP%uOUT complete\n", ep);
					HW_USBCTRL_ENDPTCOMPLETE_WR(1 << ep);

					/* Process EP0 out */
					if (ep == 0)
						stmp37xx_ep0_out(dev);
					else
						stmp37xx_epn_out(dev, ep);

				}

				if (endpoint_completions & (1 << (16 + ep))) {
					//printk("EP%uIN complete\n", ep);
					HW_USBCTRL_ENDPTCOMPLETE_WR(1 << (16 + ep));

					if (ep == 0)
						stmp37xx_ep0_in(dev);
					else
						stmp37xx_epn_in(dev, ep);
				}
			}
		}
	}

	spin_unlock(&dev->lock);

	STMPUDC_DEBUG("IRQ exit\n");
	return IRQ_HANDLED;
}

static int stmp37xx_ep_enable(struct usb_ep *_ep,
			     const struct usb_endpoint_descriptor *desc)
{
	struct stmp37xx_ep *ep;
	struct stmp37xx_udc *dev;
	unsigned long flags;

	STMPUDC_DEBUG("\n");

	ep = container_of(_ep, struct stmp37xx_ep, ep);
	if (!_ep || !desc || ep->desc || _ep->name == ep0name
	    || desc->bDescriptorType != USB_DT_ENDPOINT
	    || ep->ep_address != desc->bEndpointAddress
	    || ep->fifo_size < le16_to_cpu(desc->wMaxPacketSize)) {
		STMPUDC_DEBUG("bad ep or descriptor (%s)\n", _ep->name);
		return -EINVAL;
	}

	/* xfer types must match, except that interrupt ~= bulk */
	if (ep->attributes != desc->bmAttributes
	    && ep->attributes != USB_ENDPOINT_XFER_BULK
	    && desc->bmAttributes != USB_ENDPOINT_XFER_INT) {
		STMPUDC_DEBUG("%s type mismatch\n",  _ep->name);
		return -EINVAL;
	}

	/* hardware _could_ do smaller, but driver doesn't */
	if ((desc->bmAttributes == USB_ENDPOINT_XFER_BULK
	     && le16_to_cpu(desc->wMaxPacketSize) > EPN_MAXPACKETSIZE)
	    || !desc->wMaxPacketSize) {
		STMPUDC_DEBUG("bad %s maxpacket\n", _ep->name);
		return -ERANGE;
	}

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN) {
		STMPUDC_DEBUG("bogus device state\n");
		return -ESHUTDOWN;
	}

	spin_lock_irqsave(&ep->dev->lock, flags);

	ep->stopped = 0;
	ep->primed = 0;
	ep->desc = desc;
	ep->ep.maxpacket = le16_to_cpu(desc->wMaxPacketSize);
	
	/* Set new maximum packet size for this endpoint. */
	ep->dqh[0] = (1 << 29) | (ep->ep.maxpacket << 16);

	/* Reset halt state (does flush) */
	stmp37xx_set_halt(_ep, 0);

	STMPUDC_DEBUG("Enable endpoint %d\n", ep_index(ep));
	if (ep_index(ep) == 0 && !ep_is_in(ep))
		HW_USBCTRL_ENDPTCTRLn_WR(0,
			(HW_USBCTRL_ENDPTCTRLn_RD(0) & 0xffff0000) | 0x00000080);
	else if(ep_index(ep) == 0 && ep_is_in(ep))
		HW_USBCTRL_ENDPTCTRLn_WR(0,
			(HW_USBCTRL_ENDPTCTRLn_RD(0) & 0x0000ffff) | 0x00800000);
	else if(ep_index(ep) == 1 && ep_is_in(ep))
		HW_USBCTRL_ENDPTCTRLn_WR(1, 0x00c80000);
	else if(ep_index(ep) == 2 && !ep_is_in(ep))
		HW_USBCTRL_ENDPTCTRLn_WR(2, 0x000000c8);
	else if(ep_index(ep) == 3 && ep_is_in(ep))
		HW_USBCTRL_ENDPTCTRLn_WR(3, 0x00c80000);
	else
		BUG();
	spin_unlock_irqrestore(&ep->dev->lock, flags);

	STMPUDC_DEBUG("enabled %s\n",  _ep->name);
	return 0;
}

/** Disable EP
 */
static int stmp37xx_ep_disable(struct usb_ep *_ep)
{
	struct stmp37xx_ep *ep;
	unsigned long flags;

	STMPUDC_DEBUG("\n");

	ep = container_of(_ep, struct stmp37xx_ep, ep);
	if (!_ep || !ep->desc) {
		STMPUDC_DEBUG("%s not enabled\n", 
		      _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	spin_lock_irqsave(&ep->dev->lock, flags);
	
	/* Nuke all pending requests (does flush) */
	nuke(ep, -ESHUTDOWN);

	ep->desc = 0;
	ep->stopped = 1;

	spin_unlock_irqrestore(&ep->dev->lock, flags);

	STMPUDC_DEBUG("disabled %s\n", _ep->name);
	return 0;
}

static struct usb_request *stmp37xx_alloc_request(struct usb_ep *ep,
							unsigned gfp_flags)
{
	struct stmp37xx_request *req;

	STMPUDC_DEBUG("\n");

	req = kmalloc(sizeof *req, gfp_flags);
	if (!req)
		return 0;

	memset(req, 0, sizeof *req);
	INIT_LIST_HEAD(&req->list);
	return &req->req;
}

static void stmp37xx_free_request(struct usb_ep *ep, struct usb_request *_req)
{
	struct stmp37xx_request *req;

	STMPUDC_DEBUG("\n");

	req = container_of(_req, struct stmp37xx_request, req);
	kfree(req);
}

static void stmp37xx_complete (struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length)
		STMPUDC_DEBUG ("complete --> %d, %d/%d\n",
				req->status, req->actual, req->length);
}

/** Queue one request
 *  Kickstart transfer if needed
 */
static int stmp37xx_queue(struct usb_ep *_ep, struct usb_request *_req,
				unsigned gfp_flags)
{
	struct stmp37xx_request *req;
	struct stmp37xx_ep *ep;
	struct stmp37xx_udc *dev;
	unsigned long flags;
	unsigned long virt_start;
	unsigned long virt_end;

	req = container_of(_req, struct stmp37xx_request, req);
	if (unlikely
	    (!_req || !_req->complete || (!_req->buf && _req->length > 0))) {
		STMPUDC_DEBUG("bad params\n");
		STMPUDC_DEBUG("_req %p\n", _req);
		STMPUDC_DEBUG("_req->complete %p\n", _req->complete);
		STMPUDC_DEBUG("_reg->buf %p\n", _req->buf);
		return -EINVAL;
	}

	ep = container_of(_ep, struct stmp37xx_ep, ep);
	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		STMPUDC_DEBUG("bad ep\n");
		return -EINVAL;
	}

	dev = ep->dev;
	if (unlikely(!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		STMPUDC_DEBUG("bogus device state %p\n",  dev->driver);
		return -ESHUTDOWN;
	}

	if(ep->stopped) {
		//printk("trying to queue to stopped ep%d\n", ep_index(ep));
	}
	
	STMPUDC_DEBUG("%s queue %d\n", _ep->name, _req->length);
	
	spin_lock_irqsave(&dev->lock, flags);

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	/* Cache flushing */
	virt_start = (unsigned long) _req->buf;
	virt_end = virt_start + _req->length;

	if(ep_is_in(ep)) {
		/* Clean the buffer before DMA write to host */
		arm926_dma_clean_range(virt_start, virt_end);
	} else {
		/* Invalidate the buffer before DMA read from host */
		arm926_dma_inv_range(virt_start, virt_end);
	}
	
	/* Add this to the queue */
	list_add_tail(&req->list, &ep->request_head);

	/* Potentially kick start the endpoint if it stopped. */
	if (unlikely(ep_index(ep) == 0)) {
		/* EP0 */
		stmp37xx_ep0_kick(dev, ep);
	} else {
		/* EP1, EP2, EP3 (in and out) */
		stmp37xx_epn_kick(dev, ep);
	}

	spin_unlock_irqrestore(&dev->lock, flags);

	return 0;
}

/* dequeue JUST ONE request */
static int stmp37xx_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct stmp37xx_ep *ep;
	struct stmp37xx_request *req;
	unsigned long flags;

	STMPUDC_DEBUG("\n");

	ep = container_of(_ep, struct stmp37xx_ep, ep);
	if (!_ep || ep->ep.name == ep0name)
		return -EINVAL;


	spin_lock_irqsave(&ep->dev->lock, flags);

	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->request_head, list) {
		if (&req->req == _req)
			break;
	}
	if (&req->req != _req) {
		spin_unlock_irqrestore(&ep->dev->lock, flags);
		return -EINVAL;
	}

	done(ep, req, -ECONNRESET);

	flush(ep);

	spin_unlock_irqrestore(&ep->dev->lock, flags);
	return 0;
}

/** Halt specific EP
 *  Return 0 if success
 *  NOTE: Sets INDEX register to EP !
 */
static int stmp37xx_set_halt(struct usb_ep *_ep, int value)
{
	struct stmp37xx_ep *ep;
	unsigned long flags;

	ep = container_of(_ep, struct stmp37xx_ep, ep);
	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		STMPUDC_DEBUG("bad ep\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&ep->dev->lock, flags);

	/* EP0? */
	if (ep_index(ep) == 0) {
	    	/* Stall? */
	    	if (value) {
			/* Set stall on both in & out */
			HW_USBCTRL_ENDPTCTRLn_SET(0, 0x00010001);
		} else {
			/* Clear stall on both in & out */
			HW_USBCTRL_ENDPTCTRLn_CLR(0, 0x00010001);
		}
	} else if (ep_is_in(ep)) {
		/* IN endpoint */
		if (value && ep->primed) {
			/* There are outstanding transfers on this
			 * IN endpoint. You can't stall it yet. */
			int empty = list_empty(&ep->request_head);
			(void) empty;
			spin_unlock_irqrestore(&ep->dev->lock, flags);
			STMPUDC_DEBUG("Can't halt IN endpoint yet: %d\n", empty);
			return -EAGAIN;
		}
		/* Kill the endpoint */
		if(value) {
			/* Stall */
			if(ep_index(ep) == 1)
				HW_USBCTRL_ENDPTCTRLn_SET(1, 0x00010000);
			else if(ep_index(ep) == 3)
				HW_USBCTRL_ENDPTCTRLn_SET(3, 0x00010000);
			flush(ep);
		} else {
			/* Unstall. Also reset the data toggle. */
			flush(ep);
			if(ep_index(ep) == 1) {
				HW_USBCTRL_ENDPTCTRLn_SET(1, 0x00400000);
				HW_USBCTRL_ENDPTCTRLn_CLR(1, 0x00010000);
			} else if(ep_index(ep) == 3) {
				HW_USBCTRL_ENDPTCTRLn_SET(3, 0x00400000);
				HW_USBCTRL_ENDPTCTRLn_CLR(3, 0x00010000);
			}
		}
	} else {
		/* OUT endpoint. That means EP2OUT. */
		if(value) {
			/* Stall */
			HW_USBCTRL_ENDPTCTRLn_SET(2, 0x00000001);
			flush(ep);
		} else {
			/* Unstall. Also reset the data toggle. */
			flush(ep);
			HW_USBCTRL_ENDPTCTRLn_SET(2, 0x00000040);
			HW_USBCTRL_ENDPTCTRLn_CLR(2, 0x00000001);
		}
	}
	

	if (value)
		ep->stopped = 1;
	else
		ep->stopped = 0;

	if (!value && ep_index(ep) != 0) {
		/* Kick off EP1..3 if they have stuff queued */
		stmp37xx_epn_kick(ep->dev, ep);
	}
	
	STMPUDC_DEBUG("%s %s halted\n", _ep->name, value == 0 ? "NOT" : "IS");

	spin_unlock_irqrestore(&ep->dev->lock, flags);

	return 0;
}

/** Return bytes in EP FIFO
 *  NOTE: Sets INDEX register to EP
 */
static int stmp37xx_fifo_status(struct usb_ep *_ep)
{
	int count = 0;
	struct stmp37xx_ep *ep;

	ep = container_of(_ep, struct stmp37xx_ep, ep);
	if (!_ep) {
		STMPUDC_DEBUG("bad ep\n");
		return -ENODEV;
	}

	STMPUDC_DEBUG("ep_index:  %d\n", ep_index(ep));

	/* LPD can't report unclaimed bytes from IN fifos */
	if (ep_is_in(ep))
		return -EOPNOTSUPP;

#if 0
	csr = usb_read(ep->csr1);
	if (ep->dev->gadget.speed != USB_SPEED_UNKNOWN ||
	    csr & USB_OUT_CSR1_OUT_PKT_RDY) {
		count = usb_read(USB_OUT_FIFO_WC1);
	}
#endif
	return count;
}

/** Flush EP FIFO
 *  NOTE: Sets INDEX register to EP
 */
static void stmp37xx_fifo_flush(struct usb_ep *_ep)
{
	struct stmp37xx_ep *ep;

	ep = container_of(_ep, struct stmp37xx_ep, ep);
	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		STMPUDC_DEBUG("bad ep\n");
		return;
	}

	flush(ep);
}

/**
 * udc_set_address - set the USB address for this device
 * @address:
 *
 * Called from control endpoint function after it decodes a set address setup packet.
 */
static void udc_set_address(struct stmp37xx_udc *dev, unsigned char address)
{
	STMPUDC_DEBUG_EP0("\n");
	
	/* Note address and set it after we've received the end-of-transaction EP0 ack */
	dev->usb_address = address;
	dev->usb_address_defer = 1;
}

/*
 * DATA_STATE_RECV (OUT_PKT_RDY)
 *      - if error
 *              set EP0_CLR_OUT | EP0_DATA_END | EP0_SEND_STALL bits
 *      - else
 *              set EP0_CLR_OUT bit
 				if last set EP0_DATA_END bit
 */
static void stmp37xx_ep0_out(struct stmp37xx_udc *dev)
{
	STMPUDC_DEBUG_EP0("\n");

	/* This will be called when we get the 0-byte OUTs to complete a setup transaction */
	if (dev->ep0_state == WAIT_FOR_ACK) {
	    	/* Check length - should be 0 bytes */
	    	
	    	STMPUDC_DEBUG_EP0("ACK received\n");
	    	
	    	/* Prime EP0 for setup packet */
	    	dev->ep0_state = WAIT_FOR_SETUP;
	    	ep0_prime(dev, -1);
	    	return;
        } else if (dev->ep0_state == WAIT_FOR_DATA) {
	    	struct stmp37xx_ep *ep = &dev->ep[0];
		struct stmp37xx_request *req;
		/* Cache flushing */
		unsigned long virt_start = (unsigned long)ep->dtd;
		unsigned long virt_end = virt_start + TD_SIZE;

		/* Invalidate the buffer before DMA read from host */
		arm926_dma_inv_range(virt_start, virt_end);
	    	
	    	/* Complete the req */
		if (!list_empty(&ep->request_head)) {
		    	int bytes_done = ep->dtd_bytes - (ep->dtd[1] >> 16);
		    	
		    	STMPUDC_DEBUG_EP0("EP0OUT received %d bytes, status %02x\n", bytes_done, ep->dtd[1] & 0xff);
		    	
			/* Retreive request */
			req = list_entry(ep->request_head.next,
					 struct stmp37xx_request, list);

			/* Copy the data out */
			if (bytes_done > 0) {
				memcpy(req->req.buf + req->req.actual,
				       (void *) dev->ep0_bounce, bytes_done);
				req->req.actual += bytes_done;
				STMPUDC_DEBUG_EP0("%d/%d bytes done\n", req->req.actual, req->req.length);
			}

			/* Have we done it all? */
			if (req->req.length == req->req.actual) {
				/* Next will be a reply */
				dev->ep0_state = SEND_REPLY;
	    	
				/* All done! */
				done(ep, req, 0);
		    	} else {
			    	/* Re-prime for more */
			    	dev->ep0_state = WAIT_FOR_DATA;
			    	stmp37xx_ep0_kick(dev, ep);
			}
			
			return;
		} else {
			STMPUDC_DEBUG_EP0("Hmmm, no req for EP0OUT data!\n");
		}	    	
	}

	STMPUDC_DEBUG_EP0("Got EP0OUT when we weren't expecting it...\n");
}

/*
 * DATA_STATE_XMIT
 */
static void stmp37xx_ep0_in(struct stmp37xx_udc *dev)
{
        struct stmp37xx_request *req;
        struct stmp37xx_ep *ep = &dev->ep[0];
        
	STMPUDC_DEBUG_EP0("\n" );
	
    	/* We have just sent a packet on EP0IN */
	if (dev->ep0_state == SEND_ACK) {
	    	/* We just sent a zero-byte reply */
	    	
	        /* We may have a defered set address - SET_ADDRESS sends a zero byte reply */
	        if (dev->usb_address_defer) {
		    /* Set address and clear flag */
		    HW_USBCTRL_PERIODICLISTBASE_WR(dev->usb_address << 25);		    
		    dev->usb_address_defer = 0;
		    STMPUDC_DEBUG_EP0("Done set address\n");
		}
		
		/* This was the end of the transaction, so we're waiting for SETUP now */
	    	dev->ep0_state = WAIT_FOR_SETUP;
	    	ep0_prime(dev, -1);
	    	
	    	return;
    	}

	/* How is this request going? */
	if (!list_empty(&ep->request_head)) {
	    	/* Retreive request */
		int kick = 0;
		req = list_entry(ep->request_head.next,
				 struct stmp37xx_request, list);

		/* Have we done it all? */
		if (req->req.length != req->req.actual) {
		    	/* Send next packet */
			kick = 1;
		} else if ((req->req.length % EP0_PACKETSIZE) == 0 &&
				   req->req.length != 0 &&		// 2008.06.11(mooji) handle in case of zero length data IN 
				   dev->ep0_state != SEND_ZLT) {
			/* Transfer was an exact multiple of the packet size.
			   Need a zero length terminator.
			   Go to SEND_ZLT state so we don't end up in a loop */
			dev->ep0_state = SEND_ZLT;
			kick = 1;
		}
		
		if (kick) {
		    	stmp37xx_ep0_kick(dev, ep);
		    	return;
		}
		    
		/* All done */
		done(ep, req, 0);
	}
    	
    	/* Now waiting for the EP0OUT ACK */
	dev->ep0_state = WAIT_FOR_ACK;
    	ep0_prime(dev, EP0_PACKETSIZE);
}

static int stmp37xx_ep0_kick(struct stmp37xx_udc *dev, struct stmp37xx_ep *ep)
{
	struct stmp37xx_request *req;
	//int ret, need_zlp = 0;
	int length;
	char *buf;
	int max = 64; //le16_to_cpu(ep->desc->wMaxPacketSize);	

	STMPUDC_DEBUG_EP0("\n");

	if (list_empty(&ep->request_head)) {
	        /* There's nothing to be sent */
	        return 0;
    	} else {
		req = list_entry(ep->request_head.next,
				 struct stmp37xx_request, list);
	}
	
	if (!req) {
		STMPUDC_DEBUG_EP0("NULL REQ\n");
		return 0;
	}

	/* Sort out length that this DTD is going to deal with */
	length = req->req.length - req->req.actual;
	length = min(length, max);

	/* EP0IN? */
	if (dev->ep0_state == SEND_REPLY) {
		/* Copy packet over: the DTD/data space is pre-allocated */
		buf = req->req.buf + req->req.actual;
		memcpy((void *) dev->ep0_bounce, buf, length);

		req->req.actual += length;
    	}
    	
	/* Program up dqh and dtd then prime the EP */
	ep0_prime(dev, length);

	if(dev->ep0_state == SEND_ZLT) {
		/* Special case for zero-length transfers. */
		return 1;
	}
	
	/* Is this the last packet? */
	if (req->req.actual == req->req.length) {
		/* We're just waiting for a 0-byte EP0OUT ACK now */
		dev->ep0_state = WAIT_FOR_ACK;
	} else {
		/* More to send */
		dev->ep0_state = SEND_REPLY;
	}
	return 1;
}

static int stmp37xx_handle_get_status(struct stmp37xx_udc *dev,
					    struct usb_ctrlrequest *ctrl)
{
	struct stmp37xx_ep *ep0 = &dev->ep[0];
	/* By leeth, for USB command verifier test at 20081110 */
	struct stmp37xx_ep *qep;
	struct usb_request *req;
	int reqtype = (ctrl->bRequestType & USB_RECIP_MASK);

	// EWWWWW. Bad Hugo! Bad!
	static u16 val = 0;
	
	/* By leeth, for USB command verifier test at 20081110 */
	if (reqtype == USB_RECIP_INTERFACE) {
		/* This is not supported.
		 * And according to the USB spec, this one does nothing..
		 * Just return 0
		 */
		STMPUDC_DEBUG_SETUP("GET_STATUS: USB_RECIP_INTERFACE\n");
	} else if (reqtype == USB_RECIP_DEVICE) {
		STMPUDC_DEBUG_SETUP("GET_STATUS: USB_RECIP_DEVICE\n");
	} else if (reqtype == USB_RECIP_ENDPOINT) {
		int ep_num = (ctrl->wIndex & ~USB_DIR_IN);

		STMPUDC_DEBUG_SETUP
		    ("GET_STATUS: USB_RECIP_ENDPOINT (%d), ctrl->wLength = %d\n",
		     ep_num, ctrl->wLength);

		qep = &dev->ep[ep_num];
		
		if (ep_num > UDC_MAX_ENDPOINTS)
		{
			stmp37xx_set_halt(&ep0->ep, 1);
			return -EOPNOTSUPP;
		}

		val = ((HW_USBCTRL_ENDPTCTRLn_RD(ep_num) & 
			(BM_USBCTRL_ENDPTCTRLn_TXS | BM_USBCTRL_ENDPTCTRLn_RXS)) ? 1 : 0);
		STMPUDC_DEBUG_SETUP("GET_STATUS, ep: %d (%x), val = %d\n", ep_num,
			    ctrl->wIndex, val);
	} else {
		STMPUDC_DEBUG_SETUP("Unknown REQ TYPE: %d\n", reqtype);
		return -EOPNOTSUPP;
	}

	/* Form 16-bit reply */
	req = dev->ep0_req;
	req->buf = (u8*)&val;
	req->length = sizeof(val);
	req->complete = stmp37xx_complete;
	return stmp37xx_queue (&ep0->ep, req, GFP_ATOMIC);
}

/*
 * WAIT_FOR_SETUP (OUT_PKT_RDY)
 *      - read data packet from EP0 FIFO
 *      - decode command
 */
static void stmp37xx_ep0_setup(struct stmp37xx_udc *dev)
{
	struct stmp37xx_ep *ep = &dev->ep[0];
	struct usb_ctrlrequest ctrl;
	int bytes;
	struct usb_request *req;
#if 0
	int expecting_ep0out = 0;
#endif
	/* Cache flushing */
	unsigned long virt_start = (unsigned long)ep->out_dqh;
	unsigned long virt_end = virt_start + QH_SIZE;

	/* Invalidate the buffer before DMA read from host */
	arm926_dma_inv_range(virt_start, virt_end);

	STMPUDC_DEBUG_SETUP("\n");
	
	/* Grab the setup bytes and clear the flag */
	memcpy((unsigned char *)&ctrl, (unsigned char*)(ep->out_dqh + 10), 8);	// words 10 and 11
	bytes = 8;
	HW_USBCTRL_ENDPTSETUPSTAT_WR(1);

	/* Nuke all previous transfers */
	//nuke(ep, -EPROTO);
	
	STMPUDC_DEBUG_SETUP("Read CTRL REQ %d bytes\n", bytes);
	STMPUDC_DEBUG_SETUP("CTRL.bRequestType = 0x%x (is_in %d)\n", ctrl.bRequestType,
		    ctrl.bRequestType == USB_DIR_IN);
	STMPUDC_DEBUG_SETUP("CTRL.bRequest = 0x%x\n", ctrl.bRequest);
	STMPUDC_DEBUG_SETUP("CTRL.wLength = 0x%x\n", ctrl.wLength);
	STMPUDC_DEBUG_SETUP("CTRL.wValue = 0x%x (0x%x)\n", ctrl.wValue, ctrl.wValue >> 8);
	STMPUDC_DEBUG_SETUP("CTRL.wIndex = 0x%x\n", ctrl.wIndex);

	dev->req_pending = 1;

	/* Note if this SETUP packet expected a reply or more data */
	if ((ctrl.bRequestType & USB_DIR_IN)==0 && ctrl.wLength > 0) {
	    	/* We expect more data */
	    	dev->ep0_state = WAIT_FOR_DATA;
		STMPUDC_DEBUG_SETUP("EP0 State WAIT_FOR_DATA\n");
	} else {
	    	/* We're generally replying to setup packets */
	    	dev->ep0_state = SEND_REPLY;
		STMPUDC_DEBUG_SETUP("EP0 State SEND_REPLY\n");
	}

	/* Handle some SETUP packets ourselves */
	switch (ctrl.bRequest) {
	case USB_REQ_SET_ADDRESS:
		/* By leeth, for reset device at usb connected, it should be occured 
		 * disconnect event to user plain at 20080911 */
		reset_configured();
		if (ctrl.bRequestType != (USB_TYPE_STANDARD | USB_RECIP_DEVICE))
			break;

		STMPUDC_DEBUG_SETUP("USB_REQ_SET_ADDRESS (%d)\n", ctrl.wValue);
		udc_set_address(dev, ctrl.wValue);
		
		/* Send a zero byte ACK reply, no need for a req */
		dev->ep0_state = SEND_ACK;		
		ep0_prime(dev, 0);
		
		return;

	case USB_REQ_GET_STATUS:
	    	/* Send encapsulated command comes here (wrong bRequestType for a GET_STATUS) */
		STMPUDC_DEBUG_SETUP("USB_REQ_GET_STATUS\n");
		if (stmp37xx_handle_get_status(dev, &ctrl) != 0) {
			STMPUDC_DEBUG("handle_get_status failed\n");
		}
		return;

	case USB_REQ_CLEAR_FEATURE:
	case USB_REQ_SET_FEATURE:
		STMPUDC_DEBUG_SETUP("USB_REQ_CLEAR/SET_FEATURE\n");
		/* By leeth, add test mode for USB I/F test at 20080806 */
		if (ctrl.bRequestType == USB_RECIP_ENDPOINT) {
			struct stmp37xx_ep *qep;
			int ep_num = (ctrl.wIndex & 0x0f);

			/* Support only HALT feature */
			if (ctrl.wValue != 0 || ctrl.wLength != 0
			    || ep_num > UDC_MAX_ENDPOINTS || ep_num < 0)
				break;

			qep = &dev->ep[ep_num];
			if (ctrl.bRequest == USB_REQ_SET_FEATURE) {
				STMPUDC_DEBUG_SETUP("SET_FEATURE (%d)\n",
					    ep_num);
				stmp37xx_set_halt(&qep->ep, 1);
			} else {
				STMPUDC_DEBUG_SETUP("CLR_FEATURE (%d)\n",
					    ep_num);
				stmp37xx_set_halt(&qep->ep, 0);
			}

			/* Reply with a ZLP on next IN token */
			req = dev->ep0_req;
			req->buf = (u8*) 0;
			req->length = 0;
			req->complete = stmp37xx_complete;
			stmp37xx_queue(&ep->ep, req, GFP_ATOMIC);
			return;
		} else if (ctrl.bRequestType == USB_RECIP_DEVICE) {
			struct stmp37xx_ep *qep;
			int ep_num = (ctrl.wIndex & 0x0f);
			unsigned long test_mode = (ctrl.wIndex & 0xff00) >> 8;

			qep = &dev->ep[ep_num];
			if (ctrl.bRequest  == USB_REQ_SET_FEATURE) {
				switch (ctrl.wValue) {
				case 1: /* Wake Up */
					set_test_mode(0);
					break;
				case 2: /* Test Mode */
					del_timer(&timer_test_mode);
					setup_timer(&timer_test_mode, set_test_mode, test_mode);
					mod_timer(&timer_test_mode, jiffies + 1);
					break;
				default:
					stmp37xx_set_halt(&ep->ep, 1);
					break;
				}
			} else if (ctrl.bRequest == USB_REQ_CLEAR_FEATURE) {
				set_test_mode(0);
			}
			
			/* Reply with a ZLP on next IN token */
			req = dev->ep0_req;
			req->buf = (u8*) 0;
			req->length = 0;
			req->complete = stmp37xx_complete;
			stmp37xx_queue(&ep->ep, req, GFP_ATOMIC);
		}
		break;

	case USB_REQ_SET_CONFIGURATION:
		/* By leeth, for USB command verifier test at 20081110 */
		clear_ep_state(dev);
		/* By leeth, for reset device at usb connected, it should be occured 
		 * disconnect event to user plain at 20080911 */
		set_configured();

		break;
	/* By leeth, for USB command verifier test at 20081110 */
	case USB_REQ_SET_INTERFACE:
		STMPUDC_DEBUG_SETUP(2, "USB_REQ_SET_INTERFACE\n");
		if(ctrl.bRequestType == USB_RECIP_INTERFACE)
		{
			/* udc hardware is broken by design:
			 *  - altsetting may only be zero;
			 *  - hw resets all interfaces' eps;
			 *  - ep reset doesn't include halt(?).
			 */
			STMPUDC_DEBUG_SETUP(2, "broken set_interface (%d/%d)\n",
				ctrl.wIndex, ctrl.wValue);
			if(ctrl.wLength == 0)
			{
				/* Reply with a ZLP on next IN token */
				req = dev->ep0_req;
				req->buf = (u8*) 0;
				req->length = 0;
				req->complete = stmp37xx_complete;
				stmp37xx_queue(&ep->ep, req, GFP_ATOMIC);
				return;
			}
		}
		break;

	default:
		break;
	}

	if (likely(dev->driver)) {
	    	int i;
	    	
		/* device-2-host (IN) or no data setup command, process immediately */
		spin_unlock(&dev->lock);
		i = dev->driver->setup(&dev->gadget, &ctrl);
		spin_lock(&dev->lock);

		if (i < 0) {
			/* setup processing failed, force stall */
			STMPUDC_DEBUG_SETUP
			    ("  --> ERROR: gadget setup FAILED (stalling), setup returned %d\n",
			     i);

			/* ep->stopped = 1; */
			dev->ep0_state = WAIT_FOR_SETUP;
		}
	}	
}

static int stmp37xx_epn_kick(struct stmp37xx_udc *dev,
				   struct stmp37xx_ep *ep)
{
	struct stmp37xx_request *req;
	int length, max, filled;
	char *buf;
		
	if(ep->primed || ep->stopped)
		return 1;

	if (list_empty(&ep->request_head)) {
		/* There's nothing left in the queue */
		return 0;
	} else {
		req = list_entry(ep->request_head.next,
				 struct stmp37xx_request, list);
		if (!req) {
			STMPUDC_DEBUG_EP0("NULL REQ\n");
			return 0;
		}
	}
	
	max = le16_to_cpu(ep->desc->wMaxPacketSize);
	
	/* Grab the buffer start and remaining length */
	buf = req->req.buf + req->req.actual;
	length = req->req.length - req->req.actual;

	STMPUDC_DEBUG("EP%d Kick buf %p, length %d actual %x\n", 
		ep->ep_address, req->req.buf, 
		req->req.length, req->req.actual);
	/* Fill up to 4 entries of the DTD */
	filled = fill_dtd_pages((unsigned *) ep->dtd, buf, length, max);

	STMPUDC_DEBUG("Filled %u of %u bytes\n", filled, length);
	
	if(ep_is_in(ep))
		STMPUDC_DEBUG("EP%dIN kick\n", ep_index(ep));
	else
		STMPUDC_DEBUG("EP%dOUT kick\n", ep_index(ep));
	
	/* Prime it up and let it go */
	epn_prime(ep, filled);
	return 1;
}

static void stmp37xx_epn_out(struct stmp37xx_udc *dev, int epn)
{
	/* Bulk endpoint receive completed. */
	
	struct stmp37xx_ep *ep = &dev->ep[epn];
	struct stmp37xx_request *req;

	/* Cache flushing */
	unsigned long virt_start = (unsigned long)ep->dtd;
	unsigned long virt_end = virt_start + TD_SIZE;

	/* Invalidate the buffer before DMA read from host */
	arm926_dma_inv_range(virt_start, virt_end);

	STMPUDC_DEBUG_EP0("\n");

	/* Complete the req */
	if (!list_empty(&ep->request_head)) {
		int max = le16_to_cpu(ep->desc->wMaxPacketSize);	
		int bytes_done = ep->dtd_bytes - (ep->dtd[1] >> 16);
		int short_packet =
			(bytes_done == 0) || ((bytes_done & (max - 1)) != 0);

		/* No longer primed (no multiple queuing at the moment) */
		ep->primed = 0;
		
		/* Retreive request */
		req = list_entry(ep->request_head.next,
				 struct stmp37xx_request, list);
		
		STMPUDC_DEBUG("EP%dOUT received %d bytes, status %02x\n",
		      epn, bytes_done, ep->dtd[1] & 0xff);
		
		/* Note transfer count */
		if (bytes_done > 0) {
			req->req.actual += bytes_done;
			STMPUDC_DEBUG("%d/%d bytes done\n",
			      req->req.actual, req->req.length);
		}

		/* Have we done it all? */
		if (req->req.length == req->req.actual || short_packet
			/* hack !! check  */
			|| bytes_done <= max)
			/* hack !!! check */
		{
			/* All done! */
			STMPUDC_DEBUG("All done (short=%d, ok=%d)\n",
			      short_packet,
			      req->req.short_not_ok);
			done(ep, req, 0);
		}
		
		/* Re-prime for more if this entry wasn't complete, or if
		 * there are further items on the queue.
		 * */
		if (!list_empty(&ep->request_head)) {
			if(short_packet && req->req.short_not_ok) {
				STMPUDC_DEBUG("Short packet not ok, not repriming\n");
			}
			else {
				STMPUDC_DEBUG("Reprime for more\n");
				stmp37xx_epn_kick(dev, ep);
			}
		}
	} else {
		STMPUDC_DEBUG("Hmmm, no req for EP%dOUT data!\n", epn);
		//BUG();
	}

	/* By leeth, usb transferring/idle/suspend event process at 20080920 */
#if defined(CONFIG_STMP37XX_USBEVENT)
	if(req->req.actual >= EPN_MAXPACKETSIZE)
	{
		struct usbevent_s event;

		stop_idle_timer();

		event.type = USB_TRANSFER_EVENT;
		event.status = USB_EVENT_TRANSFER;
		send_usbevent_msg(&event);

		start_idle_timer();
	}
#endif
}

static void stmp37xx_epn_in(struct stmp37xx_udc *dev, int epn)
{
	/* Bulk endpoint transmit completed. */
	
	struct stmp37xx_ep *ep = &dev->ep[epn];
	struct stmp37xx_request *req;
	
	/* Cache flushing */
	unsigned long virt_start = (unsigned long)ep->dtd;
	unsigned long virt_end = virt_start + TD_SIZE;

	/* Invalidate the buffer before DMA read from host */
	arm926_dma_inv_range(virt_start, virt_end);
	
	STMPUDC_DEBUG_EP0("\n");

	/* Complete the req */
	if (!list_empty(&ep->request_head)) {
		int max = le16_to_cpu(ep->desc->wMaxPacketSize);	
		int bytes_done = ep->dtd_bytes - (ep->dtd[1] >> 16);
		int short_packet =
			(bytes_done == 0) ||
			((bytes_done & (max - 1)) != 0);
		int is_last;

		/* No longer primed (no multiple queuing at the moment) */
		ep->primed = 0;
		
		/* Retreive request */
		req = list_entry(ep->request_head.next,
				 struct stmp37xx_request, list);
		
		STMPUDC_DEBUG("EP%dIN trasmitted %d bytes, status %02x\n",
		      epn, bytes_done, ep->dtd[1] & 0xff);
		
		/* Note transfer count */
		if (bytes_done > 0) {
			req->req.actual += bytes_done;
			STMPUDC_DEBUG("%d/%d bytes done\n",
			      req->req.actual, req->req.length);
		}

		/* Have we done it all? */
		if (short_packet) {
			/* If we just output a short packet, this transfer
			 * is over. */
			is_last = 1;
		} else if (req->req.length != req->req.actual) {
			/* Still some data to transmit due to splitting */
			is_last = 0;
		} else if (req->req.zero) {
			/* Transferred all data but we need to
			 * terminate with a zero length packet */
			is_last = 0;
		} else {
			/* Transferred all data and no need for a ZLT */
			is_last = 1;
		}

		if (is_last) {
			STMPUDC_DEBUG("All done (short=%d)\n", short_packet);
			done(ep, req, 0);
		}
		
		/* Re-prime for more if this entry wasn't complete, or if
		 * there are further items on the queue */
		if (!list_empty(&ep->request_head)) {
			STMPUDC_DEBUG("Reprime for more\n");
			stmp37xx_epn_kick(dev, ep);
		}
	} else {
		STMPUDC_DEBUG("Hmmm, no req for EP%dIN data!\n", epn);
		BUG();
	}

	/* By leeth, usb transferring/idle/suspend event process at 20080920 */
#if defined(CONFIG_STMP37XX_USBEVENT)
	if(req->req.actual >= EPN_MAXPACKETSIZE)
	{
		struct usbevent_s event;

		stop_idle_timer();

		event.type = USB_TRANSFER_EVENT;
		event.status = USB_EVENT_TRANSFER;
		send_usbevent_msg(&event);

		start_idle_timer();
	}
#endif
}

/* ---------------------------------------------------------------------------
 * 	device-scoped parts of the api to the usb controller hardware
 * ---------------------------------------------------------------------------
 */

static int stmp37xx_udc_get_frame(struct usb_gadget *_gadget)
{
	STMPUDC_DEBUG("\n");
	return HW_USBCTRL_FRINDEX_RD();
}

static int stmp37xx_udc_wakeup(struct usb_gadget *_gadget)
{
	/* host may not have enabled remote wakeup */
	/*if ((UDCCS0 & UDCCS0_DRWF) == 0)
	   return -EHOSTUNREACH;
	   udc_set_mask_UDCCR(UDCCR_RSM); */
	return -ENOTSUPP;
}

static const struct usb_gadget_ops stmp37xx_udc_ops = {
	.get_frame = stmp37xx_udc_get_frame,
	.wakeup = stmp37xx_udc_wakeup,
	/* current versions must always be self-powered */
};

static void nop_release(struct device *dev)
{
	STMPUDC_DEBUG("\n");
}


static struct stmp37xx_udc memory = {
	.usb_address = 0,
	.device_pool = NULL,

	.gadget = {
		   .ops = &stmp37xx_udc_ops,
		   .ep0 = &memory.ep[0].ep,
		   .name = driver_name,
		   .dev = {
			   .bus_id = "gadget",
			   .release = nop_release,
			   },
		   },

	/* control endpoint (ep0in) */
	.ep[0] = {
		  .ep = {
			 .name = ep0name,
			 .ops = &stmp37xx_ep_ops,
			 .maxpacket = EP0_PACKETSIZE,
			 },
		  .dev = &memory,
		  .fifo_size = EPN_MAXPACKETSIZE, 

		  .ep_address = USB_DIR_IN,
		  .attributes = 0,
		  .ep_bitmask = (1<<16),

		  .ep_type = ep_control,
		  },
	/* first group of endpoints */
	.ep[1] = {
		  .ep = {
			 .name = "ep1in-bulk",
			 .ops = &stmp37xx_ep_ops,
			 .maxpacket = 512,
			 },
		  .dev = &memory,
		  .fifo_size = EPN_MAXPACKETSIZE, 

		  .ep_address = USB_DIR_IN | 1,
		  .attributes = USB_ENDPOINT_XFER_BULK,
		  .ep_bitmask = (1<<17),

		  .ep_type = ep_bulk_in,
		  },

	.ep[2] = {
		  .ep = {
			 .name = "ep2out-bulk",
			 .ops = &stmp37xx_ep_ops,
			 .maxpacket = 512,
			 },
		  .dev = &memory,
		  .fifo_size = EPN_MAXPACKETSIZE, 
		  
		  .ep_address = 2,
		  .attributes = USB_ENDPOINT_XFER_BULK,
		  .ep_bitmask = (1<<2),

		  .ep_type = ep_bulk_out,
		  },
	.ep[3] = {
		  .ep = {
			 .name = "ep3in-int",
			 .ops = &stmp37xx_ep_ops,
			 .maxpacket = 512,
			 },
		  .dev = &memory,
		  .fifo_size = EPN_MAXPACKETSIZE, 

		  .ep_address = USB_DIR_IN | 3,
		  .attributes = USB_ENDPOINT_XFER_INT,
		  .ep_bitmask = (1<<19),

		  .ep_type = ep_interrupt,
		  },
};

static const char proc_node_name[] = "driver/udc";

static int
udc_proc_read(char *page, char **start, off_t off, int count,
	      int *eof, void *_dev)
{
	char *buf = page;
	struct stmp37xx_udc *dev = _dev;
	char *next = buf;
	unsigned size = count;
	unsigned long flags;
	int t;

	if (off != 0)
		return 0;

	local_irq_save(flags);

	/* basic device status */
	t = scnprintf(next, size,
		      DRIVER_DESC "\n"
		      "%s version: %s\n"
		      "Gadget driver: %s\n"
		      "Host: %s\n\n",
		      driver_name, DRIVER_VERSION,
		      dev->driver ? dev->driver->driver.name : "(none)",
		      is_usb_connected(dev)? "full speed" : "disconnected");
	size -= t;
	next += t;

	local_irq_restore(flags);
	*eof = 1;
	return count - size;
}

static int 
udc_proc_write(struct file * file, const char * buf, 
	unsigned long count, void *data)
{
	char cmd0[64], cmd1[64]; 
	struct stmp37xx_udc *dev = &memory;

	sscanf(buf, "%s %s", cmd0, cmd1);

	if(!strcmp(cmd0, "usbact")) {
		switch(cmd1[0]) {
		case '0':
			udc_disable(dev);
			udc_reinit(dev);
			printk("USBACT 0\n");
			break;
		case '1':
			udc_enable(dev);
			printk("USBACT 1\n");
		}
	}
	return count;
}

#define create_proc_files() 	create_proc_read_entry(proc_node_name, 0, NULL, udc_proc_read, dev)
#define remove_proc_files() 	remove_proc_entry(proc_node_name, NULL)

/*
 * 	probe - binds to the platform device
 */
static int stmp37xx_udc_probe(struct platform_device *pdev)
{
	struct stmp37xx_udc *dev = &memory;
	int retval;

	STMPUDC_DEBUG("\n");

	spin_lock_init(&dev->lock);
	dev->dev = &pdev->dev;

	device_initialize(&dev->gadget.dev);
	dev->gadget.dev.parent = dev->dev;
	dev->gadget.is_dualspeed = 1;
	
	the_controller = dev;
	platform_set_drvdata(pdev, dev);

	udc_disable(dev);
	udc_reinit(dev);

	/* irq setup after old hardware state is cleaned up */
	retval = request_irq(IRQ_USB_CTRL, stmp37xx_udc_irq, IRQF_DISABLED, driver_name, dev);
	if (retval != 0) {
		STMPUDC_DEBUG(KERN_ERR "%s: can't get irq %i, err %d\n", driver_name,
		      IRQ_USB_CTRL, retval);
		return -EBUSY;
	}

#if 0
	create_proc_files();

#else
	struct proc_dir_entry *proc_ent; 
	proc_ent = create_proc_entry("gadget_udc", S_IWUSR | S_IRUGO, NULL);
	if(!proc_ent)
	{
		STMPUDC_DEBUG(KERN_ERR, "%s: cannot create proc\n", driver_name);
		return 0;
	}
	
	proc_ent->read_proc = udc_proc_read;
	proc_ent->write_proc = udc_proc_write;
	proc_ent->data = NULL; 
#endif
	return retval;
}

static int stmp37xx_udc_remove(struct platform_device *pdev)
{
	struct stmp37xx_udc *dev = platform_get_drvdata(pdev);

	STMPUDC_DEBUG("\n");

	udc_disable(dev);
#if 0
	remove_proc_files();
#else
	remove_proc_entry("gadget_udc", NULL);
#endif
	usb_gadget_unregister_driver(dev->driver);

	free_irq(IRQ_USB_CTRL, dev);

	platform_set_drvdata(pdev, NULL);

	the_controller = 0;

	return 0;
}

static struct platform_driver stmp37xx_udc_driver = {
	.driver		= {
		.name	= (char*)driver_name,
		.bus	= &platform_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe		= stmp37xx_udc_probe,
	.remove		= stmp37xx_udc_remove,
	.suspend	= NULL,
	.resume		= NULL,
};


static int __init udc_init(void)
{
	STMPUDC_DEBUG("%s version %s\n", driver_name, DRIVER_VERSION);

	/* By leeth, add test mode for USB I/F test at 20080806 */
	setup_timer(&timer_test_mode, set_test_mode, 0);

	return platform_driver_register(&stmp37xx_udc_driver);
}

static void __exit udc_exit(void)
{
	platform_driver_unregister(&stmp37xx_udc_driver);
}

module_init(udc_init);
module_exit(udc_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Mikko Lahteenmaki, Bo Henriksen, John Ripley");
MODULE_LICENSE("GPL");
