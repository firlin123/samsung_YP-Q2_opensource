/* $Id: stmp36xx_commevent.c,v 1.6 2007/03/14 09:38:36 biglow Exp $ */

/**
 * \file stmp36xx_commevent.c
 * \brief event for COMM (serial, I2S,... )
 * \author Lee Tae Hun <th76.lee@samsung.com>
 * \version $Revision: 1.6 $
 * \date $Date: 2007/03/14 09:38:36 $
 *
 * For user event server, user get system event of COMM detecting.
 *
 * $Log: stmp36xx_commevent.c,v $
 * Revision 1.6  2007/03/14 09:38:36  biglow
 * PBA detecting method is changed
 *
 * - Taehun Lee
 *
 * Revision 1.5  2007/03/13 10:14:23  biglow
 * PBA detect polarity is changed
 *
 * - Taehun Lee
 *
 * Revision 1.4  2007/03/02 05:49:08  biglow
 * fix unlimited PBA detecting bug
 *
 * - Taehun Lee
 *
 * Revision 1.3  2007/03/02 02:04:39  biglow
 * fix uart power enable/disable method
 *
 * - Taehun Lee
 *
 * Revision 1.2  2007/03/02 02:02:45  biglow
 * fix uart power enable/disable method
 *
 * - Taehun Lee
 *
 * Revision 1.1  2007/01/19 06:07:50  biglow
 * add comm event
 *
 */

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
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <asm/arch/stmp37xx.h>
#include <asm/irq.h>
#include <asm/arch/digctl.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include "stmp37xx_commevent.h"

#define DRIVER_VERSION "$Revision: 1.6 $"
#define COMM_MINOR 70

#define CONFIG_STMP36XX_COMMEVENT 1
#define CONFIG_STMP36XX_UARTPOWER 1
#define CONFIG_STMP36XX_PBA 	1
#define SA_INTERRUPT			0x20000000 /* dummy -- ignored */

#define PBA_PIN_NO_NEW			13 
#define MUX_PBA_PIN_NO_NEW	26 

#define PBA_PIN_NO_OLD			20
#define MUX_PBA_PIN_NO_OLD	8

// get_hw_option_type

#define DRIVER_NAME	"stmp37xx_commevt"

static unsigned int irq_num = 0; //bank0, pin8


DECLARE_WAIT_QUEUE_HEAD(comm_queue);

/**
 * For controling UART in A16 board, set SSP_DETECT with GPIO output.
 */
#define UART_CONTROL_INIT() \
	{ \
		HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE); \
		HW_PINCTRL_MUXSEL1_CLR(0x000c0000); \
		HW_PINCTRL_MUXSEL1_SET(0x000c0000); \
		HW_PINCTRL_DRIVE0_CLR(0x1 << 25); \
		HW_PINCTRL_DOUT0_SET(0x1 << 25); \
		HW_PINCTRL_DOE0_SET(0x1 << 25); \
	}
/**
 * Enable UART
 */
#define UART_POWER_ON() HW_PINCTRL_DOUT0_CLR(0x1 << 25);
/**
 * Disable UART
 */
#define UART_POWER_OFF() HW_PINCTRL_DOUT0_SET(0x1 << 25);
 
/**
 * Structure of comm event
 */
struct comm_event
{
	volatile unsigned short type; /**< Event type */
	volatile unsigned short status; /**< Event value */
};

static volatile struct comm_event comm_status = {MAX_COMMEVENT_NUM, 0};
static int mod_usecount = 0;
static volatile int change = 0;
static int uart_enable = 0;
struct semaphore comm_sem; 
static int  nPbaPinNo = 0;
static int	nMuxPbaPinNo = 0;
/* By leeth, delayed event for stable PBA Detect at 20081128 */
static struct timer_list timer_comm;

/* By leeth, delayed event for stable PBA Detect at 20081128 */
/**
 * \brief check gpio value
 * \param arg comm type (IN)
 * \return value of GPIO
 *
 * This function check value of GPIO interrupt.
 */
static inline unsigned comm_pinvalue_read(comm_event_t arg);

/**
 * \brief check polarity
 * \param arg comm type (IN)
 * \return polarity of GPIO irq
 *
 * This function check polarity of GPIO interrupt.
 */
static inline unsigned comm_irqpol_read(comm_event_t arg);
/**
 * \brief check irq status
 * \return irq status
 *
 * This function check status of GPIO interrupt per event.
 */
static inline comm_event_t comm_irqstat_read(void);
 
/* By leeth, delayed event for stable PBA Detect at 20081128 */
/**
 * \brief recheck cable after detecting
 * \param data event type (IN)
 * 
 * Handle event.
 */
static void comm_timer (unsigned long data);

/**
 * \brief detecting cable
 * \param irq interrupt number (IN)
 * \param dev device ID (IN)
 * \param r  unused (IN)
 *
 * Handle GPIO interrupt for detecting comm connection.
 */
//static void comm_detecting_irq(int irq, void *_dev, 
//	struct pt_regs *r);
static irqreturn_t comm_detecting_irq (int irq_num, void* dev_idp);
 
/**
 * \brief read comm event
 * \param filp file descriptor (IN)
 * \param buf user buffer space (OUT)
 * \param len length to read (IN)
 * \param ptr offset to read (IN)
 * \return size after reading data
 *
 * Comm event is sent to user space.
 * If status field is 1 in comm_event structures, 
 * comm cable is inserted. Otherwise, comm cable is reomved.
 */
static ssize_t comm_read(struct file *filp, char *buf, 
	size_t len, loff_t *ptr);
/**
 * \brief add wait queue for comm event
 * \param filp file descriptor (IN)
 * \param wait poll table (IN)
 * \return type of event
 *
 * Use Poll method for passing comm event.
 */
static unsigned int comm_poll(struct file *filp, poll_table *wait);
/**
 * \brief open comm event driver
 * \param inode inode information (IN)
 * \param filp file descriptor (IN)
 * \return 0 : success \notherwise : fail
 *
 * Open function for using comm event.
 */
static int comm_open(struct inode *inode, struct file * filp);
/**
 * \brief close comm event driver
 n* \param inode inode information (IN)
 * \param filp file descriptor (IN)
 * \return 0 : success \notherwise : fail
 *
 * Close function for using comm event.
 */
static int comm_release(struct inode *inode, struct file *filp);
/**
 * \brief init module
 * \return 0 : success \notherwise : fail
 *
 * Initialize GPIO interrupt and timer for detecting comm event.
 */
static int __init comm_init(void);
/**
 * \brief clean module
 *
 * Uninitialize GPIO interrupt for safe.
 */
static void __exit comm_cleanup(void);

static void PBA_DETECT_IRQ_INIT(void)
{
        HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE);

	if(nPbaPinNo == 13)
		HW_PINCTRL_MUXSEL0_SET(0x3 << nMuxPbaPinNo);      //BANK0_PIN13, INTR
	else
		HW_PINCTRL_MUXSEL1_SET(0x3 << nMuxPbaPinNo);      //BANK0_PIN13, INTR		

	HW_PINCTRL_DOE0_CLR(0x1 << nPbaPinNo);
       HW_PINCTRL_PIN2IRQ0_SET(0x1 << nPbaPinNo); //select interrupt source pin 
       HW_PINCTRL_IRQEN0_SET(0x1 << nPbaPinNo); //IRQ enable
	/* By leeth, delayed event for stable PBA Detect at 20081128 */
       HW_PINCTRL_IRQLEVEL0_SET(0x1 << nPbaPinNo); //1:level detection, 0:edge detection
       HW_PINCTRL_IRQPOL0_SET(0x1 << nPbaPinNo); //1:high or rising edge, 0:low or falling edge 
       HW_PINCTRL_IRQSTAT0_CLR(0x1 << nPbaPinNo);
}

/* By leeth, delayed event for stable PBA Detect at 20081128 */
static inline unsigned comm_pinvalue_read(comm_event_t arg)
{
	reg32_t ret = 0;

	switch(arg)
	{
	case PBA_EVENT:
		ret = HW_PINCTRL_DIN0_RD() & (0x1 << nPbaPinNo);
		break;
	default:
		break;
	}

	return ret;
}

static inline unsigned comm_irqpol_read(comm_event_t arg)
{
	reg32_t ret = 0;

	switch(arg)
	{
	case PBA_EVENT:
		ret = HW_PINCTRL_IRQPOL0_RD() & (0x1 << nPbaPinNo);
		break;
	default:
		break;
	}

	return ret;
}

static inline comm_event_t comm_irqstat_read(void)
{
	reg32_t ret = 0;

	if(HW_PINCTRL_IRQSTAT0_RD() & (0x1 << nPbaPinNo))
		ret = PBA_EVENT;
	return ret;
}

/* By leeth, delayed event for stable PBA Detect at 20081128 */
static void comm_timer (unsigned long data)
{
	unsigned short status;

	status = (unsigned short)((comm_pinvalue_read((comm_event_t)data)) ? 1 : 0);

	if((comm_status.type != (unsigned short)data ) || 
		comm_status.status != status)
	{
		if(comm_status.type == MAX_COMMEVENT_NUM && 
			comm_status.status == status)
		{
			comm_status.type = data;
			comm_status.status = status;
		}
		else
		{
			comm_status.type = data;
			comm_status.status = status;
			change = 1;
			wake_up_interruptible(&comm_queue);
			if((comm_status.status == 0) && (!uart_enable))
			{
				UART_POWER_OFF();
			}
			if((comm_status.status == 1) && (!uart_enable))
			{
				UART_POWER_ON();
			}
		}
	}
}

static irqreturn_t comm_detecting_irq (int irq_num, void* dev_idp)
{
	comm_event_t event = comm_irqstat_read();

	/* By leeth, delayed event for stable PBA Detect at 20081128 */
	del_timer(&timer_comm);

	/* after checking IRQ polarity, toggle polarity of GPIO irq */
	/* when bordeaux is inserted, GPIO is gone to low */
	if(!comm_irqpol_read(event))
	{
		HW_PINCTRL_IRQPOL0_SET(0x1 << nPbaPinNo);
	}
	else
	{
		HW_PINCTRL_IRQPOL0_CLR(0x1 << nPbaPinNo); 
	}

	/* By leeth, delayed event for stable PBA Detect at 20081128 */
	if(event != MAX_COMMEVENT_NUM)
	{
		setup_timer(&timer_comm, comm_timer, event);
		mod_timer(&timer_comm, jiffies + (500*HZ/1000));
	}

	/* clear irq status for next interrupt */
	HW_PINCTRL_IRQSTAT0_CLR(0x1 << nPbaPinNo); 

	return IRQ_HANDLED;
}
 

static ssize_t 
comm_read(struct file *filp, char *buf, size_t len, loff_t *ptr)
{
	int length, ret;
	
	if(!(filp->f_flags & O_NONBLOCK))
	{
		/* If device is opened blocking IO, it must wait event */
		wait_event_interruptible(comm_queue, change);
	}
	if(len < sizeof(comm_status))
		length = len;
	else
		length = sizeof(comm_status);

	if (copy_to_user (buf, (void *)&comm_status, length))
		ret = -EFAULT;
	else
		ret = length;

	change = 0;

	return ret;	
}

static unsigned int 
comm_poll(struct file *filp, poll_table *wait)
{
	int mask = 0;

	/* add wait queue for waiting event */
	poll_wait(filp, &comm_queue, wait);
	if(change)
		mask = POLLIN;

	return mask;
}

static int 
comm_open(struct inode *inode, struct file * filp)
{
	if (mod_usecount != 0)
	{
		printk(KERN_ERR "Already opened!!\n");
		return -EAGAIN;
	}

	mod_usecount = 1;

	/* clear irq status for first interrupt */
	HW_PINCTRL_IRQSTAT0_CLR(0x1 << nPbaPinNo); 
	HW_PINCTRL_IRQEN0_SET(0x1 << nPbaPinNo);
 
	return 0;
}

static int 
comm_release(struct inode *inode, struct file *filp)
{
 

       HW_PINCTRL_IRQEN0_CLR(0x1 << nPbaPinNo);
	/* clear irq status for first interrupt */
	HW_PINCTRL_IRQSTAT0_CLR(0x1 << nPbaPinNo); 
	
	mod_usecount = 0;

	return 0;
}

static struct file_operations comm_fops = {
	read:		comm_read, 
	poll:		comm_poll, 
	open:		comm_open,
	release:	comm_release,
};

static struct miscdevice comm_misc = {
	minor : COMM_MINOR,
	name  : "comm_event",
	fops  : &comm_fops,
};

static int comm_read_proc(char *buf, char **start, off_t offset,
	int count, int *eof, void *data)
{
	int len = 0; 
	
	*start = buf; 

	if (down_interruptible (&comm_sem))
		return -ERESTARTSYS;

	if(offset == 0)
	{ 
		len += sprintf(buf + len, "\n<< [COMM Status] >>\n");
		len += sprintf(buf + len, "Mode : ");
		if(comm_status.type == PBA_EVENT)
			len += sprintf(buf + len, "PBA\n");
		else if(comm_status.type == BORDEAUX_EVENT)
			len += sprintf(buf + len, "BORDEAUX\n");
		else
			len += sprintf(buf + len, "UNKNOWN\n");
		len += sprintf(buf + len, "Status : ");
		if(comm_status.status == 1)
			len += sprintf(buf + len, "connected\n");
		else if(comm_status.status == 0)
			len += sprintf(buf + len, "disconnected\n");
		else
			len += sprintf(buf + len, "unknown\n");
		if(uart_enable)
			len += sprintf(buf + len, "UART is always turned on\n");
		len += sprintf(buf + len, "MUXSEL0 0x%08x\n", HW_PINCTRL_MUXSEL0_RD());
		len += sprintf(buf + len, "DOE0 0x%08x\n", HW_PINCTRL_DOE0_RD());
		len += sprintf(buf + len, "DIN0 0x%08x\n", HW_PINCTRL_DIN0_RD());
		
	}

	up (&comm_sem);

	*eof = 1; 

	return len; 
}


static ssize_t comm_write_proc(struct file * file, const char * buf, 
	unsigned long count, void *data)
{
	char cmd0[64], cmd1[64]; 
	
	sscanf(buf, "%s %s", cmd0, cmd1);
	
	if (down_interruptible (&comm_sem))
		return -ERESTARTSYS;

	if(!strcmp(cmd0, "uart"))
	{
		if(!strcmp(cmd1, "1"))
		{
			UART_POWER_ON();
			uart_enable = 1;
		}
		else if(!strcmp(cmd1, "0"))
		{
			UART_POWER_OFF();
			uart_enable = 0;
		}
	}
	
	up (&comm_sem);

	return count; 
}

static int __devinit stmp37xx_commevt_probe(struct platform_device *pdev)
{
        struct input_dev *input_dev;
        int i, row, col, error;
        int code;
        int read_length;
        int otinit_status;
        
        platform_set_drvdata(pdev, NULL);
 

	/* init port map for using GPIO int */
	PBA_DETECT_IRQ_INIT();

        // irqflas can be
        //         IRQ_TYPE_EDGE_RISING, IRQ_TYPE_EDGE_FALLING
        //         IRQ_TYPE_LEVEL_HIGH, IRQ_TYPE_LEVEL_LOW
	error = request_irq(irq_num, comm_detecting_irq, IRQ_TYPE_LEVEL_HIGH, "pbadetect", pdev);

	if(error<0)
	{
		printk(KERN_ERR "stmp36xx: cannot register pba %d detecting isr\n", error);
		return error;
	}
	else 
		printk("stmp37xx: can register pba %d detecting isr\n", error);

 
        enable_irq(irq_num);
 
        //ss_pm_register(SS_PM_TOUCHPAD_DEV, pm_touchpad_callback);
 
        return 0;
 
}


static struct platform_driver stmp37xx_commevt_driver = { 
        .probe          = stmp37xx_commevt_probe,
//        .remove         = __devexit_p(stmp37xx_commevt_remove),
//        .suspend        = stmp37xx_commevt_suspend,
//        .resume         = stmp37xx_commevt_resume,
        .driver         = { 
                .name   = DRIVER_NAME,
        },  
};

static int __init 
comm_init(void)
{
	int retval;
	int nBDVer;
 
	struct proc_dir_entry *proc_ent;

	nBDVer = get_hw_option_type();

	printk("<<<<<<<<get_hw_option_type = %d\n", nBDVer);

	if(nBDVer == 1){
			nPbaPinNo = PBA_PIN_NO_NEW;
			nMuxPbaPinNo = MUX_PBA_PIN_NO_NEW;
	}else if(nBDVer == 4){
			nPbaPinNo = PBA_PIN_NO_OLD;
			nMuxPbaPinNo = MUX_PBA_PIN_NO_OLD;
	}else{
			nPbaPinNo = 0;
			nMuxPbaPinNo = 0;			
	}

	printk("<<<<<<<<nPbaPinNo = %d\n", nPbaPinNo);
	printk("<<<<<<<<nMuxPbaPinNo = %d\n", nMuxPbaPinNo);
	
       irq_num = IRQ_START_OF_EXT_GPIO + (0 * 32 + nPbaPinNo); //bank0, pinNO

	/* register device driver */
	if (misc_register(&comm_misc) != 0)
	{
		printk(KERN_ERR "Cannot register device /dev/%s\n",
			   comm_misc.name);
		return -EFAULT;
	}

	/* For using UART, GPIO setting is neccesary. */
	UART_CONTROL_INIT();

#if defined(CONFIG_STMP36XX_UARTPOWER)
	UART_POWER_ON();
	uart_enable = 1;
#endif

	/* register proc entry */
	sema_init (&comm_sem, 1);

	proc_ent = create_proc_entry("commctrl", S_IWUSR | S_IRUGO, NULL);
	
	proc_ent->read_proc = comm_read_proc;
	proc_ent->write_proc = comm_write_proc;
	proc_ent->data = NULL; 

	printk(KERN_INFO "comm event driver verision : %s\n", DRIVER_VERSION);
	
	platform_driver_register(&stmp37xx_commevt_driver);

	return 0;
}

static void __exit 
comm_cleanup(void)
{
	/* disable GPIO interrupt */
 	free_irq(irq_num, NULL);


	/* unregister device driver */
	misc_deregister(&comm_misc);
}

module_init(comm_init);
module_exit(comm_cleanup);


