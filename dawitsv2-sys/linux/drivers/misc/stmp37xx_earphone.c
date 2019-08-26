/* $Id: stmp37xx_earphone.c, v0.7 2008/08/27 BK.Koo Exp $ */
/***********************************************************************
* \file stmp37xx_earphone.c
* \brief stmp37xx earphone relation.
* \author Koo ByoungKi <byeonggi.gu@partner.samsung.com>
* \version $Revision: v0.7 $
* \date $Date: 2008/08/27 $
*
* This file is a earphone functions using Timer in stmp37xx.
************************************************************************/

/********************************************************
*                                                       *
* Include Header File of stmp37xx_earphone.c             *
*                                                       *
*********************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/smp_lock.h>
#include <linux/spinlock.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <asm/arch/stmp37xx.h>
#include <asm/arch/stmp37xx_pm.h>
#include <asm/arch/lradc.h>

#define DRIVER_VERSION "$Revision: 0.9 $"
#define EPEVENT_MINOR 71

#define DEV_NAME "Earphone"
#define EARPHONE_ADC_CH		LRADC_CH_4
#define EARPHONE_LOOPS_PER_SAMPLE	(0)
#define EARPHONE_SAMPLES_PER_SEC	(1) /* 10 interrupts per 1 second */

#define EARPHONE_FREQ \
	(2000 / ((EARPHONE_LOOPS_PER_SAMPLE+1) * EARPHONE_SAMPLES_PER_SEC))

#if EARPHONE_LOOPS_PER_SAMPLE > 0
# define EARPHONE_ADC_ACC (1)
#else
# define EARPHONE_ADC_ACC (0)
#endif

static int wakeup_status = 0;

DECLARE_WAIT_QUEUE_HEAD(eq_poll_wait_queue);

/**
 * Structure of earphone status event
 */
struct ep_status_event
{
	volatile unsigned short type; /**< Event type */
	volatile unsigned short status; /**< Event value */
};

static volatile struct ep_status_event ep_status = {0, 0};
static int mod_usecount = 0;
static volatile int change = 0;

/* earphone detection timer structure */
static struct timer_list detect_ep_timer;

enum {
	EP_CONNECTED = 1,
	EP_DISCONNECTED = 0
};

/**
 * \brief read earphone disconnection event
 * \param filp file descriptor (IN)
 * \param buf user buffer space (OUT)
 * \param len length to read (IN)
 * \param ptr offset to read (IN)
 * \return size after reading data
 *
 * earphone disconnection event is sent to user space.
 */
static ssize_t stmp37xx_epevent_read(struct file *filp, 
	char *buf, size_t len, loff_t *ptr);
/**
 * \brief add wait queue for earphone disconnection event
 * \param filp file descriptor (IN)
 * \param wait poll table (IN)
 * \return type of event
 *
 * Use Poll method for passing earphone disconnection event.
 */
static unsigned int stmp37xx_epevent_poll(struct file *filp, poll_table *wait);
/**
 * \brief open earphone disconnection event driver
 * \param inode inode information (IN)
 * \param filp file descriptor (IN)
 * \return 0 : success \notherwise : fail
 *
 * Open function for using earphone disconnection event.
 */
static int stmp37xx_epevent_open(struct inode *inode, struct file * filp);
/**
 * \brief close earphone disconnection event driver
 * \param inode inode information (IN)
 * \param filp file descriptor (IN)
 * \return 0 : success \notherwise : fail
 *
 * Close function for using earphone disconnection event.
 */
static int stmp37xx_epevent_release(struct inode *inode, struct file *filp);
/**
 * \brief init module
 * \return 0 : success \notherwise : fail
 *
 * Register event handler for detecting earphone disconnection event.
 */
static int __init stmp37xx_epevent_init(void);
/**
 * \brief clean module
 *
 * Unregiser ep event handler for safe.
 */
static void __exit stmp37xx_epevent_exit(void);

static void pm_earphone_callback(ss_pm_request_t event);

int adc_delay_slot = -1;


void init_epdetect_pin(void)
{
	/* set ssp detect pin to gpio input */
	/*HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE ); 
	HW_PINCTRL_MUXSEL1_CLR(0x000C0000); 
	HW_PINCTRL_MUXSEL1_SET(0x000C0000); //BANK0 25 -> GPIO mode to detect ep
	HW_PINCTRL_DRIVE0_SET(0x00000000); 
	HW_PINCTRL_DOE0_CLR(0x02000000);*/
}

void detect_ep_status(unsigned long arg)
{
	int current_status;
	volatile int earDet;
	unsigned int  this_value;
	unsigned long  this_jiffy;
	int ret, i = 0;

	ret = result_lradc(4, &this_value, &this_jiffy);
	if (ret < 0) {
		printk("%s(): result_lradc\n", __func__);
		return;
	}

	printk("%u %u : %u\n", 0, ret, this_value);

//	earphone_adc_init(slot, channel, 0);
//	start_lradc_delay(adc_delay_slot);

	earDet = ret;

	if(earDet==0)
	{
		current_status = EP_CONNECTED;
	}
	else
	{
		current_status = EP_DISCONNECTED;
	}
	
	if((current_status == EP_DISCONNECTED) && (ep_status.status == EP_CONNECTED))
	{
		ep_status.type = 1;
		ep_status.status = EP_DISCONNECTED;
		wake_up_interruptible(&eq_poll_wait_queue);
		change = 1;
	}

	if((current_status == EP_CONNECTED) && (ep_status.status == EP_DISCONNECTED))
	{
		ep_status.type = 1;
		ep_status.status = EP_CONNECTED;
		wake_up_interruptible(&eq_poll_wait_queue);
		change = 1;
	}

	ep_status.status = current_status;

	detect_ep_timer.expires=jiffies+100; // 1sec
	add_timer(&detect_ep_timer); 
}

static ssize_t 
stmp37xx_epevent_read(struct file *filp, char *buf, size_t len, loff_t *ptr)
{
	int length, ret;
	
	if(!(filp->f_flags & O_NONBLOCK))
	{
		/* If device is opened blocking IO, it must wait event */
		wait_event_interruptible(eq_poll_wait_queue, change);
	}
	if(len < sizeof(ep_status))
		length = len;
	else
		length = sizeof(ep_status);

	if (copy_to_user (buf, (void *)&ep_status, length))
		ret = -EFAULT;
	else
		ret = length;

	change = 0;
	
	return ret;	
}

static unsigned int stmp37xx_epevent_poll(struct file *filp, poll_table *wait)
{
	int mask = 0;

	/* add wait queue for waiting event */
	poll_wait(filp, &eq_poll_wait_queue, wait);
	if(change)
		mask = POLLIN;

	return mask;
}

static int 
stmp37xx_epevent_open(struct inode *inode, struct file * filp)
{
	if (mod_usecount != 0)
	{
		printk(KERN_ERR "Already opened!!\n");
		return -EAGAIN;
	}

	printk("%s()\n", __func__);
	enable_lradc(EARPHONE_ADC_CH);
	start_lradc_delay(adc_delay_slot);

	mod_usecount = 1;
	
	return 0;
}

static int 
stmp37xx_epevent_release(struct inode *inode, struct file *filp)
{
	//close
	printk("%s()\n", __func__);
	disable_lradc(EARPHONE_ADC_CH);

	mod_usecount = 0;

	return 0;
}

static int earphone_adc_init(int slot, int channel, void *data)
{
	config_lradc(channel, 1, EARPHONE_ADC_ACC, EARPHONE_LOOPS_PER_SAMPLE);
	config_lradc_delay(adc_delay_slot, EARPHONE_LOOPS_PER_SAMPLE, EARPHONE_FREQ);

	return 0;
}

static void earphone_adc_handler(int slot, int channel, void *data)
{
	int current_status;
	volatile int earDet;
	unsigned int  this_value;
	unsigned long  this_jiffy;
	int ret, i = 0;

#if 1
	ret = result_lradc(channel, &this_value, &this_jiffy);
	if (ret < 0) {
		printk("%s(): result_lradc\n", __func__);
		return;
	}
	//printk("%d: this_value = %d\n", __LINE__, this_value);

//======================================================================
	//if(this_value < 20) earDet = 0;
	if(this_value < 100) earDet = 0; //for 2nd board
	else earDet = 1;

	if(earDet==0)
	{
		current_status = EP_CONNECTED;
	}
	else
	{
		current_status = EP_DISCONNECTED;
	}

	if(wakeup_status == 1) { //only use when opm_event is wake up event to send current earphone status to application
		if(this_value != 0) { //to ignore first-time check val
			if((current_status == EP_DISCONNECTED))
			{
				ep_status.type = 1;
				ep_status.status = EP_DISCONNECTED;
				wake_up_interruptible(&eq_poll_wait_queue);
				change = 1;
			}
		
			if((current_status == EP_CONNECTED))
			{
				ep_status.type = 1;
				ep_status.status = EP_CONNECTED;
				wake_up_interruptible(&eq_poll_wait_queue);
				change = 1;
			}
			wakeup_status = 0;
		}
	}
	else if(wakeup_status == 0) {
		if((current_status == EP_DISCONNECTED) && (ep_status.status == EP_CONNECTED))
		{
			ep_status.type = 1;
			ep_status.status = EP_DISCONNECTED;
			wake_up_interruptible(&eq_poll_wait_queue);
			change = 1;
		}
	
		if((current_status == EP_CONNECTED) && (ep_status.status == EP_DISCONNECTED))
		{
			ep_status.type = 1;
			ep_status.status = EP_CONNECTED;
			wake_up_interruptible(&eq_poll_wait_queue);
			change = 1;
		}
	}

	ep_status.status = current_status;

	//===============================================================
//	printk("%u %u : %u\n", 9, ret, this_value);

	earphone_adc_init(slot, channel, 0);
	start_lradc_delay(adc_delay_slot);
#endif	

}

static void pm_earphone_callback(ss_pm_request_t event)
{
        switch(event) {
               case SS_PM_IDLE:
                        //printk("<earphone> IDLE \n");
	disable_lradc(EARPHONE_ADC_CH);
                        break;
                case SS_PM_SET_WAKEUP:
                        //printk("<earphone> WAKEUP \n");
			wakeup_status = 1;
	enable_lradc(EARPHONE_ADC_CH);
	start_lradc_delay(adc_delay_slot);
                        break;
        }
}

static struct file_operations epevent_fops = {
	read:		stmp37xx_epevent_read, 
	poll:		stmp37xx_epevent_poll, 
	open:		stmp37xx_epevent_open,
	release:	stmp37xx_epevent_release,
};

static struct miscdevice epevent_misc = {
	minor : EPEVENT_MINOR,
	name  : "misc/ep_event",
	fops  : &epevent_fops,
};

static struct lradc_ops earphone_ops = {
	.init           = earphone_adc_init,
	.handler        = earphone_adc_handler,
	.num_of_samples = EARPHONE_LOOPS_PER_SAMPLE,
};

static int __init 
stmp37xx_epevent_init(void)
{
	int ret;

	/* register device driver */
	if (misc_register(&epevent_misc) != 0)
	{
		printk(KERN_ERR "Cannot register device /dev/misc/%s\n",
			   epevent_misc.name);
		return -EFAULT;
	}

	///////////////////////////////////////////////////////////////////////////

	//printk("%s::%s()\n", __FILE__, __func__);

	ret = request_lradc(EARPHONE_ADC_CH, DEV_NAME, &earphone_ops);
	if (ret < 0) {
		printk("%s(): request_lradc() fail(%d)\n", __func__, ret);
		return -EINVAL;
	}	

	ret = request_lradc_delay();
	if (ret < 0) {
		printk("%s(): request_lradc_delay() fail(%d)\n", __func__, ret);
		free_lradc(EARPHONE_ADC_CH, &earphone_ops);
		return -EINVAL;
	}

	adc_delay_slot = ret;
	assign_lradc_delay(0, ret, EARPHONE_ADC_CH);
	

	//printk("%s()\n", __func__);
	enable_lradc(EARPHONE_ADC_CH);
	start_lradc_delay(adc_delay_slot);

	///////////////////////////////////////////////////////////////////////////////

	ep_status.status = EP_CONNECTED;
	
	/* Initialize battery timer structure */
//	init_timer(&detect_ep_timer);
//	detect_ep_timer.expires=jiffies+100; // 3sec
//	detect_ep_timer.function=detect_ep_status;
//	detect_ep_timer.data=1;
	
	/* register battery timer */
//	add_timer(&detect_ep_timer);

	ss_pm_register(SS_PM_EAR_DEV, pm_earphone_callback);

	//printk("<earphone> STMP37xx earphone event driver version : %s\n", DRIVER_VERSION);
	printk("<earphone> STMP37xx earphone event driver init success\n");
	
	return 0;
}

static void __exit 
stmp37xx_epevent_exit(void)
{
	printk("%s()\n", __func__);
	if (adc_delay_slot >= 0) {
		free_lradc_delay(adc_delay_slot);
		free_lradc(EARPHONE_ADC_CH, &earphone_ops);
		adc_delay_slot = -1;
	}

	/* unregister device driver */
	misc_deregister(&epevent_misc);
}

module_init(stmp37xx_epevent_init);
module_exit(stmp37xx_epevent_exit);
