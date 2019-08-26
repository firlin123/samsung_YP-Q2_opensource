/*
 * linux/drivers/usb/gadget/transdimension_udc.h
 * ARC/Transdimension on-chip high speed USB device controller
 *
 * Copyright (C) 2005 SigmaTel Inc
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

#ifndef __STMP37XX_UDC_H_
#define __STMP37XX_UDC_H_

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>

#include <asm/byteorder.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/hardware.h>
#include <asm/cacheflush.h>


#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/dmapool.h>

#include <asm/arch/stmp37xx.h>


// Max packet size
#define EP0_PACKETSIZE  	0x40
#define EP0_MAXPACKETSIZE  	0x40
/* By leeth, usb transferring/idle/suspend event process at 20080920 */
#define EPN_MAXPACKETSIZE  0x200

/* Depends on the configuration of the core IP: ARC/Transdimension have separate EP's for EP0
   in & out, so the 5 that is reported in DCCPARAMS is really 4 for this */
#define UDC_MAX_ENDPOINTS       4

/* States for EP0 */
#define WAIT_FOR_SETUP          0 /* Waiting for EP0OUT SETUP traffic */
#define WAIT_FOR_DATA		1 /* Waiting for EP0OUT following data */
#define SEND_REPLY              2 /* Waiting for EP0IN reply to be sent */
#define WAIT_FOR_ACK		3 /* Waiting for EP0OUT 0-byte ACK packet */
#define SEND_ACK		4 /* Waiting for EP0IN 0-byte ack to be sent */
#define SEND_ZLT		5 /* Waiting for EP0IN 0-byte ZLT to be sent */

/* Size of queue head & transfer descriptor */
#define QH_SIZE			48
#define TD_SIZE			32

/* ********************************************************************************************* */
/* IO
 */

typedef enum ep_type {
	ep_control, ep_bulk_in, ep_bulk_out, ep_interrupt
} ep_type_t;

typedef enum ep_state {
	st_idle, st_xfer, st_complete, st_stalled, st_wait_unstall
} ep_state_t;

/* An individual USB request from a gadget driver. This may take many
 * DTDs to complete, so we also keep track of the next_offset into the
 * buffer to transfer. */
struct stmp37xx_request {
	struct usb_request req;		/* Actual contained usb_request */

	struct list_head list;		/* List entry */
};

/* Representation of an endpoint. EP0 has both OUT and IN, so there's
 * an extra dqh entry for it (out_dqh). */
struct stmp37xx_ep {
	struct usb_ep ep;
	struct stmp37xx_udc *dev;
	const struct usb_endpoint_descriptor *desc;
	struct list_head request_head;	/* List of requests */
	u32 fifo_size; /* max fifo size of each endpoint */

	u8 primed;
	u8 stopped;
	u8 ep_address;
	u8 attributes;

	u32 ep_bitmask;			/* Bitmask of EP for operations */

	ep_type_t ep_type;
	ep_state_t ep_state;		/* State EP hardware is in */

	volatile u32 *dqh;		/* Queue head */
	dma_addr_t dqh_phys;
	
	volatile u32 *out_dqh;		/* EP0 ONLY - OUT Queue head */
	dma_addr_t out_dqh_phys;
	
	volatile u32 *dtd;		/* Use a single DTD (small bounce) */
	dma_addr_t dtd_phys;
	u32 dtd_bytes;			/* Number of bytes in DTD */
};

struct stmp37xx_udc {
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;
	struct device *dev;
	struct usb_request *ep0_req;
	spinlock_t lock;

	/* A DMA buffer pool for QH's and DTD's */
	struct dma_pool *device_pool;

        /* Buffer for ep0 transfers */
	volatile u8 *ep0_bounce;        /* Bounce buffer for this endpoint */
	dma_addr_t ep0_bounce_phys;     /* DMA address of bounce buffer */
	u32 ep0_bounce_size;            /* Size of bounce buffer */
	
	int ep0_state;
	struct stmp37xx_ep ep[UDC_MAX_ENDPOINTS];
	
	unsigned char usb_address;
	
	/* Are we in reset (waiting for port change to indicate end of reset) */
	int resetting;

	unsigned req_pending:1, req_std:1, req_config:1, usb_address_defer:1;
};

extern struct stmp37xx_udc *the_controller;

#define ep_is_in(EP) 		(((EP)->ep_address & USB_DIR_IN) == USB_DIR_IN)
#define ep_index(EP) 		((EP)->ep_address & 0xf)
#define ep_maxpacket(EP) 	((EP)->ep.maxpacket)


#undef CONFIG_USB_DEBUG
//#define  CONFIG_USB_DEBUG

#ifdef CONFIG_USB_DEBUG
#define STMPUDC_DEBUG(X...)       do { printk("[%s:%d](): ", __FUNCTION__, __LINE__); printk(X); } while(0)
#define STMPUDC_DEBUG_EP0(X...)   do { printk("[%s:%d](): ", __FUNCTION__, __LINE__); printk(X); } while(0)
#define STMPUDC_DEBUG_SETUP(X...) do { printk("[%s:%d](): ", __FUNCTION__, __LINE__); printk(X); } while(0)
#else
#define STMPUDC_DEBUG(X...)       do { } while(0)
#define STMPUDC_DEBUG_EP0(X...)   do { } while(0)
#define STMPUDC_DEBUG_SETUP(X...) do { } while(0)
#endif

/*
  Local declarations.
*/
static int stmp37xx_ep_enable(struct usb_ep *ep,
			     const struct usb_endpoint_descriptor *);
static int stmp37xx_ep_disable(struct usb_ep *ep);
static struct usb_request *stmp37xx_alloc_request(struct usb_ep *ep, unsigned);
static void stmp37xx_free_request(struct usb_ep *ep, struct usb_request *);
static int stmp37xx_queue(struct usb_ep *ep, struct usb_request *, unsigned);
static int stmp37xx_dequeue(struct usb_ep *ep, struct usb_request *);
static int stmp37xx_set_halt(struct usb_ep *ep, int);
static int stmp37xx_fifo_status(struct usb_ep *ep);
static void stmp37xx_fifo_flush(struct usb_ep *ep);
static int stmp37xx_ep0_kick(struct stmp37xx_udc *dev, struct stmp37xx_ep *ep);
static void stmp37xx_ep0_setup(struct stmp37xx_udc *dev);
static void stmp37xx_ep0_in(struct stmp37xx_udc *dev);
static void stmp37xx_ep0_out(struct stmp37xx_udc *dev);
static int stmp37xx_epn_kick(struct stmp37xx_udc *dev, struct stmp37xx_ep *ep);
static void stmp37xx_epn_out(struct stmp37xx_udc *dev, int epn);
static void stmp37xx_epn_in(struct stmp37xx_udc *dev, int epn);
static void stmp37xx_complete (struct usb_ep *ep, struct usb_request *req);

static void done(struct stmp37xx_ep *ep, struct stmp37xx_request *req,
		 int status);
static void stop_activity(struct stmp37xx_udc *dev,
			  struct usb_gadget_driver *driver);
static void flush(struct stmp37xx_ep *ep);
static void udc_enable(struct stmp37xx_udc *dev);
static void udc_set_address(struct stmp37xx_udc *dev, unsigned char address);
static void ep0_prime(struct stmp37xx_udc *dev, int length);
static void epn_prime(struct stmp37xx_ep *ep, int length);

#endif
