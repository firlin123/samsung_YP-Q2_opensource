/*
 * linux/drivers/misc/stmp37xx_usbevent.c
 * driver for event of connection of usb device
 *
 * Copyright (C) 2008 MIZI Research, Inc.
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
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/arch/irqs.h>

#include <asm/atomic.h>
#include <asm/bitops.h>
#include <asm/arch/stmp37xx_usbevent.h>
#include <asm/arch/stmp37xx_pm.h>

//#define ENABLE_USBEVENT_DEBUG
#define USE_PM_EVENT			// use pm_event_register

#ifdef ENABLE_USBEVENT_DEBUG
#define USBEVENT_DEBUG(X...)       do { printk("[%s:%d](): ", __FUNCTION__, __LINE__); printk(X); } while(0)
#else
#define USBEVENT_DEBUG(X...)       do { } while(0)
#endif

#define STMP37XX_USBEVENT_MINOR	67

static volatile struct usbevent_s usb_status =
	{
		.type	= USB_CONNECT_EVENT,
		.status = USB_EVENT_DISCONNECTED,
	};
static spinlock_t status_lock = SPIN_LOCK_UNLOCKED;
/* By leeth, usb transferring/idle/suspend event process at 20080920 */
static spinlock_t event_lock = SPIN_LOCK_UNLOCKED;

#define CHANGED		1
#define NOT_CHANGED	0

#define CHECK_PERIOD	(HZ/10)
#define CHECK_RETRY		20
static struct timer_list timer;

static atomic_t changed  = ATOMIC_INIT(NOT_CHANGED);

static DECLARE_WAIT_QUEUE_HEAD(wait_event);

struct proc_dir_entry *usbevent_proc_entry = NULL;


/* By leeth, usb transferring/idle/suspend event process at 20080920 */
void send_usbevent_msg(struct usbevent_s *event)
{
	unsigned long flags;
	
	if(usb_status.type == USB_CONNECT_EVENT && 
		usb_status.status == USB_EVENT_DISCONNECTED)
		return;
	
	spin_lock_irqsave(&event_lock, flags);

	if((usb_status.type != event->type) || 
		(usb_status.status != event->status))
	{
		usb_status.type = event->type;
		usb_status.status = event->status;
		atomic_set(&changed, CHANGED);
		wake_up_interruptible(&wait_event);
	}

	spin_unlock_irqrestore(&event_lock, flags);
}
EXPORT_SYMBOL(send_usbevent_msg);

static int usbevent_proc_read(char *page, char **start, off_t off, int count,
			     int *eof, void *data)
{
	unsigned long flags;
	int len = 0;

	spin_lock_irqsave(&status_lock, flags);

	len = sprintf(page, "USBevent ocurred(%s)\n", usb_status.status ? "connected" : "disconnected");

	spin_unlock_irqrestore(&status_lock, flags);

	*eof = true;

	return len;
}

static unsigned int stmp37xx_usbevent_poll(struct file *filp, poll_table *wait)
{
	int mask = 0;

	poll_wait(filp, &wait_event, wait);

	if(atomic_read(&changed) == CHANGED)
		mask = POLLIN | POLLRDNORM;

	return mask;
}

static ssize_t stmp37xx_usbevent_read(struct file *filp, char *buf,
				      size_t len, loff_t *ptr)
{
	unsigned long flags;
	int ret = 0;

	if ((filp->f_flags & O_NONBLOCK))
		return -EAGAIN;

	wait_event_interruptible(wait_event,
				 atomic_read(&changed) == CHANGED);

	spin_lock_irqsave(&status_lock, flags);

	if (len < sizeof(struct usbevent_s)) {
		ret = -EINVAL;
		goto out_read;
	}

	if (copy_to_user(buf, (void *)&usb_status, sizeof(usb_status))) {
		ret = -EFAULT;
		goto out_read;
	}
	else
		ret = sizeof(usb_status);

	atomic_set(&changed, NOT_CHANGED);

 out_read:
	spin_unlock_irqrestore(&status_lock, flags);
	return ret;
}

#ifdef USE_PM_EVENT
static int pm_event_cb (ss_pm_request_t event)
{
	unsigned long flags;
	
	spin_lock_irqsave(&status_lock, flags);
	
	switch (event) {
	case SS_PM_USB_INSERTED:
		if(usb_status.type != USB_CONNECT_EVENT || 
			usb_status.status != USB_EVENT_CONNECTED)
		{
			USBEVENT_DEBUG("[USB EVENT] USB PHY is Connected\n");
			/* By leeth, usb transferring/idle/suspend event process at 20080920 */
			usb_status.type = USB_CONNECT_EVENT;
			usb_status.status = USB_EVENT_CONNECTED;
			atomic_set(&changed, CHANGED);
			wake_up_interruptible(&wait_event);
		}
		break;

	case SS_PM_USB_REMOVED:
		if(usb_status.type != USB_CONNECT_EVENT || 
			usb_status.status != USB_EVENT_DISCONNECTED)
		{
			USBEVENT_DEBUG("[USB EVENT] USB PHY is disconnected\n");
			/* By leeth, usb transferring/idle/suspend event process at 20080920 */
			usb_status.type = USB_CONNECT_EVENT;
			usb_status.status = USB_EVENT_DISCONNECTED;
			atomic_set(&changed, CHANGED);
			wake_up_interruptible(&wait_event);
		}
		break;
	}

	spin_unlock_irqrestore(&status_lock, flags);
	return 0;
}
#else
bool PHY_IsConnected (void)
{
    bool bRtn = false;
    bool bClockWasGated= false;

    // Check if the PHY's clock is gated. When we're done, we'll want to put it
    // back the way we found it.
    bClockWasGated = HW_USBPHY_CTRL.B.CLKGATE;
 
    // Clear the soft reset
    HW_USBPHY_CTRL_CLR(BM_USBPHY_CTRL_SFTRST);

    // Ungate the PHY's clock.
    HW_USBPHY_CTRL_CLR(BM_USBPHY_CTRL_CLKGATE);

    // Override ARC control of the 15k pulldowns.
    HW_USBPHY_DEBUG_SET(BM_USBPHY_DEBUG_ENHSTPULLDOWN);

    // Disable the 15k pulldowns.
    // Necessary in device mode; the host/hub will have the 15k pulldowns.
    HW_USBPHY_DEBUG_CLR(BM_USBPHY_DEBUG_HSTPULLDOWN);

    // Enable the 200k pullups for host detection.
    HW_USBPHY_CTRL_SET(BM_USBPHY_CTRL_ENDEVPLUGINDETECT);
    
    //#warning hw_usbphy_IsConnected - wait for pullup settle delay is NOT simulated.  Verify usec counter!
    udelay(250);
    // Check for connectivity.
    bRtn = (HW_USBPHY_STATUS.B.DEVPLUGIN_STATUS ? true : false );

    // Disable the 200k pullups.
    HW_USBPHY_CTRL_CLR(BM_USBPHY_CTRL_ENDEVPLUGINDETECT);

    // Re-gate the clock, if necessary.
    if(bClockWasGated)
    {
        HW_USBPHY_CTRL_SET(BM_USBPHY_CTRL_CLKGATE);
    }
    // Return connection status.
    return bRtn;
}

EXPORT_SYMBOL(PHY_IsConnected);

static void check_usbphy (unsigned long data)
{
	unsigned long flags;

	//printk("check_usbphy()\n");

	/* timer.data is retry count */
	if (--timer.data) {
		if (PHY_IsConnected()) {
			USBEVENT_DEBUG("PHY Connected\n");

			spin_lock_irqsave(&status_lock, flags);
			usb_status.status = USB_EVENT_CONNECTED;
			atomic_set(&changed, CHANGED);

			wake_up_interruptible(&wait_event);
			spin_unlock_irqrestore(&status_lock, flags);
		}
		else {
			/* phy not connected, retry again */
			timer.expires += CHECK_PERIOD;
			add_timer(&timer);
		}
	}
}

static irqreturn_t vdd5v_interrupt (int irq_num, void* dev_idp)
{
	unsigned long flags;

	spin_lock_irqsave(&status_lock, flags);

	/* 5v conneccted */
	if (BF_RD(POWER_STS,VDD5V_GT_VDDIO)) {
		/* enble vdd5v disconnection irq */
		BF_CLR(POWER_CTRL, POLARITY_VDD5V_GT_VDDIO);

		USBEVENT_DEBUG("vdd5v connected\n");

		if (PHY_IsConnected()) {
			USBEVENT_DEBUG("PHY_IsConnected\n");
			usb_status.status = USB_EVENT_CONNECTED;
			atomic_set(&changed, CHANGED);
			wake_up_interruptible(&wait_event);
		}
		else {
			/* phy not connect, check later if it is USB or Charger */
			del_timer(&timer);
			timer.data = CHECK_RETRY;
			timer.expires = jiffies+CHECK_PERIOD;
			add_timer(&timer);
		}
	}
	/* 5v disconnected */
	else {
		/* enble vdd5v connection irq */
		BF_SET(POWER_CTRL, POLARITY_VDD5V_GT_VDDIO);

		USBEVENT_DEBUG("vdd5v disconnected\n");

		usb_status.status = USB_EVENT_DISCONNECTED;
		atomic_set(&changed, CHANGED);
		wake_up_interruptible(&wait_event);
	}

	spin_unlock_irqrestore(&status_lock, flags);

	BF_CLR(POWER_CTRL, VDD5V_GT_VDDIO_IRQ);
	return IRQ_HANDLED;
}
#endif

static int stmp37xx_usbevent_open(struct inode *inode, struct file *filp)
{

	return 0;
}

static int stmp37xx_usbevent_close(struct inode *inode, struct file *filp)
{

	return 0;
}

static struct file_operations stmp37xx_usbevent_fops = {
	.read		= stmp37xx_usbevent_read, 
	.poll		= stmp37xx_usbevent_poll, 
	.open		= stmp37xx_usbevent_open,
	.release	= stmp37xx_usbevent_close,
};


static struct miscdevice stmp37xx_usbevent_device = {
	.minor	= STMP37XX_USBEVENT_MINOR,
	.name	= "stmp37xx_usbevent",
	.fops	= &stmp37xx_usbevent_fops,
};

static int __init stmp37xx_usbevent_init(void)
{
	int ret = 0;

	if ((ret = misc_register(&stmp37xx_usbevent_device)) < 0) {
		printk("failed to register device /dev/%s\n",
			   stmp37xx_usbevent_device.name);
		goto out_init;
	}

	usbevent_proc_entry = create_proc_read_entry("usbevent", 0, NULL,
					     usbevent_proc_read, NULL);
	if (!usbevent_proc_entry) {
		printk("%s(): failed to create_proc_read_entry()\n", __func__);
		ret = -ENOMEM;
		goto out_init;
	}

#ifdef USE_PM_EVENT
	/* register pm event */
	ss_pm_register(SS_PM_USB_DEV, pm_event_cb);

	if(is_USB_connected())
	{
		unsigned long flags;

		local_irq_save(flags);
		pm_event_cb(SS_PM_USB_INSERTED);
		local_irq_restore(flags);
	}

#else
	setup_timer(&timer, check_usbphy, 0);

	/* vdd5v irq register */
	ret = request_irq(IRQ_VDD5V, vdd5v_interrupt , 0, "event", NULL);

	/* enble vdd5v interrupt */
	BF_SET(POWER_CTRL, ENIRQ_VDD5V_GT_VDDIO);
	/* enble vdd5v connection irq */
	BF_SET(POWER_CTRL, POLARITY_VDD5V_GT_VDDIO);
#endif

 out_init:
	return ret;
}

static void __exit stmp37xx_usbevent_exit(void)
{
	remove_proc_entry("usbevent", NULL);
	misc_deregister(&stmp37xx_usbevent_device);
}

module_init(stmp37xx_usbevent_init);
module_exit(stmp37xx_usbevent_exit);

MODULE_DESCRIPTION("driver for event of connection of usb device");
MODULE_AUTHOR("Ryu Ho-Eun <rhe201@mizi.com>");
MODULE_LICENSE("GPL");
