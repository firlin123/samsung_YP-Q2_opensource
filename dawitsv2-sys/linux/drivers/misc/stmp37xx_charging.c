//#include <linux/config.h>
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
//#include <linux/kmod.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h> 
#include <asm/dma.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include "stmp37xx_charging.h"

#define MAJOR_NUMBER 253

#define DEBUG
#ifdef DEBUG
# define dbg_printk(fmt, arg...)  \
	do { \
	printk("%s(%d) " fmt, __FUNCTION__, __LINE__, ## arg); \
	} while (0)
#else
# define dbg_printk(fmt, arg...) do{}while(0)
#endif // DEBUG

static int charging_open(struct inode *inode,struct file *filp);
static int charging_release(struct inode *inode,struct file *filp);
static int charging_ioctl(struct inode *inode, struct file *file,unsigned int cmd);
static int charging_clear(unsigned int nSel);
static unsigned int i;
static unsigned int iomode;
static unsigned int	nSelmA = 0;
static unsigned int  nCharging_Clear = 1;


static struct file_operations charging_fops = {
	.open    =   charging_open,
	.release =   charging_release,
	.ioctl   =   charging_ioctl
};


static int charging_open(struct inode *inode, struct file *filp)
{
	printk("charging Driver open\n");

	return 0;
}

static int charging_release(struct inode *inode, struct file *filp)
{
	printk("charging Driver close\n");
	return 0;
}

static int charging_ioctl(struct inode *inode, struct file *file, unsigned int cmd)
{
	nSelmA = cmd;
	

	HW_POWER_CTRL_CLR(BM_POWER_CTRL_CLKGATE); //30 bit
	HW_POWER_CHARGE_CLR(0x1 << 16); //PWD_BATTCHRG Power UP(set 0)
	HW_POWER_CHARGE_SET(0x1 << 8 ); //STOP_ILIMIT set 10mA

	switch(cmd)
	{
	case CMD_CHARGING_10:
		HW_POWER_CHARGE_SET(0x1 << 0 ); //BATTCHRG_I set 10mA
		break;
	case CMD_CHARGING_20:
		HW_POWER_CHARGE_SET(0x1 << 1 ); //BATTCHRG_I set 20mA
		break;
	case CMD_CHARGING_50:
		HW_POWER_CHARGE_SET(0x1 << 2 ); //BATTCHRG_I set 50mA
		break;
	case CMD_CHARGING_100:
		HW_POWER_CHARGE_SET(0x1 << 3 ); //BATTCHRG_I set 100mA
		break;
	case CMD_CHARGING_150:
		HW_POWER_CHARGE_SET(0x1 << 3 ); //BATTCHRG_I set 150mA
		HW_POWER_CHARGE_SET(0x1 << 2 ); //BATTCHRG_I set 150mA
		break;
	case CMD_CHARGING_200:
		HW_POWER_CHARGE_SET(0x1 << 4 ); //BATTCHRG_I set 200mA
		break;
	case CMD_CHARGING_250:
		HW_POWER_CHARGE_SET(0x1 << 2 ); //BATTCHRG_I set 250mA
		HW_POWER_CHARGE_SET(0x1 << 4 ); //BATTCHRG_I set 250mA
		break;
	case CMD_CHARGING_300:
		HW_POWER_CHARGE_SET(0x1 << 3 ); //BATTCHRG_I set 300mA
		HW_POWER_CHARGE_SET(0x1 << 4 ); //BATTCHRG_I set 300mA
		break;
	case CMD_CHARGING_350:
		HW_POWER_CHARGE_SET(0x1 << 2 ); //BATTCHRG_I set 350mA
		HW_POWER_CHARGE_SET(0x1 << 3 ); //BATTCHRG_I set 350mA
		HW_POWER_CHARGE_SET(0x1 << 4 ); //BATTCHRG_I set 350mA
		break;
	case CMD_CHARGING_400:
		HW_POWER_CHARGE_SET(0x1 << 5 ); //BATTCHRG_I set 400mA
		break;
	case CMD_CHARGING_450:
		HW_POWER_CHARGE_SET(0x1 << 5 ); //BATTCHRG_I set 450mA
		HW_POWER_CHARGE_SET(0x1 << 2 ); //BATTCHRG_I set 450mA
		break;
	case CMD_CHARGING_500:
		HW_POWER_CHARGE_SET(0x1 << 5 ); //BATTCHRG_I set 500mA
		HW_POWER_CHARGE_SET(0x1 << 3 ); //BATTCHRG_I set 500mA
		break;
	case CMD_CHARGING_CLEAR:
		if(!nCharging_Clear)charging_clear(nSelmA);
		break;
	}

	nCharging_Clear = 0;
	
	return 0;
}

static int charging_clear(unsigned int nSel)
{
	nCharging_Clear = 1;
	
	HW_POWER_CHARGE_SET(0x1 << 16); //PWD_BATTCHRG Power Down(set 1)
	HW_POWER_CHARGE_CLR(0x1 << 8 ); //STOP_ILIMIT set 10mA

	HW_POWER_CHARGE_CLR(0x3F); //all clear, add dhsong
#if 0 //disable dhsong
	switch(nSel)
	{
	case CMD_CHARGING_10:
		HW_POWER_CHARGE_CLR(0x1 << 0 ); //BATTCHRG_I set 10mA
		break;
	case CMD_CHARGING_20:
		HW_POWER_CHARGE_CLR(0x1 << 1 ); //BATTCHRG_I set 20mA
		break;
	case CMD_CHARGING_50:
		HW_POWER_CHARGE_CLR(0x1 << 2 ); //BATTCHRG_I set 50mA
		break;
	case CMD_CHARGING_100:
		HW_POWER_CHARGE_CLR(0x1 << 3 ); //BATTCHRG_I set 100mA
		break;
	case CMD_CHARGING_150:
		HW_POWER_CHARGE_CLR(0x1 << 3 ); //BATTCHRG_I set 150mA
		HW_POWER_CHARGE_CLR(0x1 << 2 ); //BATTCHRG_I set 150mA
		break;
	case CMD_CHARGING_200:
		HW_POWER_CHARGE_CLR(0x1 << 4 ); //BATTCHRG_I set 200mA
		break;
	case CMD_CHARGING_250:
		HW_POWER_CHARGE_CLR(0x1 << 2 ); //BATTCHRG_I set 250mA
		HW_POWER_CHARGE_CLR(0x1 << 4 ); //BATTCHRG_I set 250mA
		break;
	case CMD_CHARGING_300:
		HW_POWER_CHARGE_CLR(0x1 << 3 ); //BATTCHRG_I set 300mA
		HW_POWER_CHARGE_CLR(0x1 << 4 ); //BATTCHRG_I set 300mA
		break;
	case CMD_CHARGING_350:
		HW_POWER_CHARGE_CLR(0x1 << 2 ); //BATTCHRG_I set 350mA
		HW_POWER_CHARGE_CLR(0x1 << 3 ); //BATTCHRG_I set 350mA
		HW_POWER_CHARGE_CLR(0x1 << 4 ); //BATTCHRG_I set 350mA
		break;
	case CMD_CHARGING_400:
		HW_POWER_CHARGE_CLR(0x1 << 5 ); //BATTCHRG_I set 400mA
		break;
	case CMD_CHARGING_450:
		HW_POWER_CHARGE_CLR(0x1 << 5 ); //BATTCHRG_I set 450mA
		HW_POWER_CHARGE_CLR(0x1 << 2 ); //BATTCHRG_I set 450mA
		break;
	case CMD_CHARGING_500:
		HW_POWER_CHARGE_CLR(0x1 << 5 ); //BATTCHRG_I set 500mA
		HW_POWER_CHARGE_CLR(0x1 << 3 ); //BATTCHRG_I set 500mA
		break;
	}
#endif
}

int __init charging_init(void)
{
	int ret;

	register_chrdev( MAJOR_NUMBER, "charge_dev", &charging_fops);
	printk("chager driver loaded sucessfully\n");
	printk("POWER_CTRL 0x%8x\n",HW_POWER_CTRL_RD());
	printk("POWER_CHARGE 0x%8x\n",HW_POWER_CHARGE_RD());
	return 0;
}

void __exit charging_exit(void)
{
	if(!nCharging_Clear)charging_clear(nSelmA);
	unregister_chrdev( MAJOR_NUMBER, "charge_dev");
	printk("charger driver unloaded sucessfully\n");
	printk("POWER_CTRL 0x%8x\n",HW_POWER_CTRL_RD());
	printk("POWER_CHARGE 0x%8x\n",HW_POWER_CHARGE_RD());
}

module_init(charging_init);
module_exit(charging_exit);
