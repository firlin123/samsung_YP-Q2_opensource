#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>         
#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>     
#include <linux/fcntl.h>   
#include <linux/sched.h>
#include <linux/fs.h>                          
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/arch/irqs.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/arch/stmp37xx_pm.h>
#include <asm/arch/digctl.h>
#include <asm/arch/hardware.h>
#include <asm/arch/usb_common.h>
#include <asm/arch/37xx/regs.h>
#include <asm/arch/37xx/regsdigctl.h>
#include <asm/arch/37xx/regslradc.h>
#include <asm/arch/37xx/regspinctrl.h>
#include "stmp37xx_reset.h"

#define DRIVER_NAME			"stmp37xx_reset"
//#define MAJOR_NUMBER		251 //FIXME: major number	
#define DEL_TIMER		0
#define WAKEUP_TIMER		1

static unsigned int irq_num = IRQ_START_OF_EXT_GPIO + (0 * 32 + 26); //bank0, pin26
static unsigned int reset_flags = 0;
static spinlock_t stmp37xx_reset_lock = SPIN_LOCK_UNLOCKED;

static void stmp37xx_reset_gpio_intr_init(void);
static int stmp37xx_reset_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);


static struct file_operations stmp37xx_reset_fops = {
        .ioctl = stmp37xx_reset_ioctl
};	

static int stmp37xx_reset_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;

	switch(cmd) {
		case DEL_TIMER:
			printk("[Touch] del_timer\n");
			ret = 1; //pstmp37xx_reset_del_timer();
			break;
		case WAKEUP_TIMER:
			printk("[Touch] wakeup_timer\n");
			ret = 1; //pstmp37xx_reset_wakeup_timer();
			break;
	}
	
	return ret;
}

static irqreturn_t stmp37xx_reset_handler (int irq_num, void* dev_idp)
{
        struct platform_device *pdev = dev_idp;
        static int i = 0;
	static int error;

	spin_lock_irqsave(&stmp37xx_reset_lock, reset_flags);
	
	printk("stmp37xx_reset_handler\n\n");

	stmp37xx_reset_gpio_intr_init();

	set_pm_mode(SS_POWER_OFF);

#if 0 //sensitivity test	
	write_buf[0] = MELFAS_ADDR_COMMAND; 
	write_buf[1] = MELFAS_COMMAND_SENSITIVITY; 
	write_buf[2] = MELFAS_SENSITIVITY_HIGH; 
	length = 3;
        if ( !(stmp37xx_reset_reg_write(write_buf, length)) )
		printk("i2c write fail\n");
        if ( !(stmp37xx_reset_reg_read(MELFAS_ADDR_INTENSITY_TSP, read_buf, 7)) )
		printk("i2c read fail\n");
#endif

#if 0 //sleep mode test result OK
	write_buf[0] = MELFAS_ADDR_COMMAND; 
	write_buf[1] = MELFAS_COMMAND_ENTER_SLEEPMODE; //OK 
        if ( !(stmp37xx_reset_reg_write(write_buf, 2)) )
		printk("i2c write fail\n");
        if ( !(stmp37xx_reset_reg_read(MELFAS_ADDR_COMMAND, read_buf, 2)) )
		printk("i2c read fail\n");
#endif

#if 0 //fw, hw veriosn read. OK
	length=4;
	test_reg_addr=MELFAS_ADDR_HW_VERSION;
        if ( !(stmp37xx_reset_reg_read(test_reg_addr, read_buf, length)) )
		printk("i2c read fail\n");
	for(i=0; i<length; i++) 
	{
		if (i==0)
			printk("\nMelfas stmp37xx_reset hw version = %c", read_buf[i] );
		else 
			printk("%c", read_buf[i] );

		if (i==length-1)
			printk("\n");
	}

	length=4;
	test_reg_addr=MELFAS_ADDR_FW_VERSION;
        if ( !(stmp37xx_reset_reg_read(test_reg_addr, read_buf, length)) )
		printk("i2c read fail\n");
	for(i=0; i<length; i++) 
	{
		if (i==0)
			printk("Melfas stmp37xx_reset fw version = %c", read_buf[i] );
		else 
			printk("%c", read_buf[i] );

		if (i==length-1)
			printk("\n");
	}
#endif

	spin_unlock_irqrestore(&stmp37xx_reset_lock, reset_flags);
		
	return IRQ_HANDLED;
}

static int stmp37xx_stmp37xx_reset_open(struct input_dev *dev)
{
	printk("[stmp37xx_reset] stmp37xx_reset driver opened\n");
	return 0;
}

static void stmp37xx_stmp37xx_reset_close(struct input_dev *dev)
{
	printk("[stmp37xx_reset] stmp37xx_reset driver closed\n");
}

static void stmp37xx_reset_gpio_intr_init(void)
{
#if 1 //gpio func init
        //HW_PINCTRL_MUXSEL0_SET(0x3 << 18);      //bank0:9, T-data
        //HW_PINCTRL_MUXSEL1_SET(0x3 << 4);       //bank0:18, T-clock

        /* 1. Configure the CLK GPIO as an INPUT */
        /* Set bank0:18 to input Enable for using "CLK" signal */
        //HW_PINCTRL_DOE0_CLR(0x1 << 18);

        /* 2. Configure the INTR GPIO as an INPUT */
        /* Set bank0:8 to input Enable for using "INTR" signal */
        HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE); 
        HW_PINCTRL_MUXSEL1_SET(0x3 << 20);      //bank0:8, INTR
        HW_PINCTRL_DOE0_CLR(0x1 << 26); 
        HW_PINCTRL_PIN2IRQ0_SET(0x1 << 26); //select interrupt source pin 
        HW_PINCTRL_IRQEN0_SET(0x1 << 26); //IRQ enable
        HW_PINCTRL_IRQLEVEL0_CLR(0x1 << 26); //1:level detection, 0:edge detection
        HW_PINCTRL_IRQPOL0_SET(0x1 << 26); //1:high or rising edge, 0:low or falling edge 
        HW_PINCTRL_IRQSTAT0_CLR(0x1 << 26); 

        /* 3. Configure the DATA GPIO as an INPUT*/
        /* Set bank0:9 to Output High Enable for using "DATA" signal */
	//HW_PINCTRL_DOE0_CLR(0x1 << 9);
#else
	/*bank 0:18, T-clock*/
	stmp37xx_gpio_set_af( pin_GPIO(0, 18), 3); //gpio func
	stmp37xx_gpio_set_dir( pin_GPIO(0, 18), GPIO_DIR_OUT); //data output
	stmp37xx_gpio_set_level( pin_GPIO(0, 18), 1); //level high	
	
	/*bank 0:8, T-INTR*/
	stmp37xx_gpio_set_af( pin_GPIO(0, 8), 3); //gpio func
	stmp37xx_gpio_set_dir( pin_GPIO(0, 8), GPIO_DIR_IN); //data output

	/*bank 0:9, data*/
	stmp37xx_gpio_set_af( pin_GPIO(0, 9), 3); //gpio func
	stmp37xx_gpio_set_dir( pin_GPIO(0, 9), GPIO_DIR_IN); //data input
#endif	
}

static void pm_stmp37xx_reset_callback(ss_pm_request_t event)
{
        switch(event) {
                case SS_PM_IDLE:
                        //printk("[Touchpad] SS_PM_IDLE \n\n");
                        break;
                case SS_PM_SET_WAKEUP:
                        //printk("[Touchpad] SS_PM_SET_WAKEUP \n\n");
                        break;
                case SS_PM_LCD_ON:
                        //printk("[Touchpad] SS_PM_LCD_ON \n\n");
                        break;
                case SS_PM_LCD_OFF:
                        //printk("[Touchpad] SS_PM_LCD_OFF \n\n");
                        break;
        }
}

#ifdef CONFIG_PM
#else
#define stmp37xx_stmp37xx_reset_suspend	NULL
#define stmp37xx_stmp37xx_reset_resume	NULL
#endif

static int __devinit stmp37xx_stmp37xx_reset_probe(struct platform_device *pdev)
{
	struct input_dev *input_dev;
	int i, row, col, error;
	int code;
	int read_length;
	int otinit_status;


	platform_set_drvdata(pdev, NULL);


        // irqflas can be
        //         IRQ_TYPE_EDGE_RISING, IRQ_TYPE_EDGE_FALLING
        //         IRQ_TYPE_LEVEL_HIGH, IRQ_TYPE_LEVEL_LOW
        error = request_irq(irq_num, stmp37xx_reset_handler, IRQ_TYPE_EDGE_RISING, "Touchpad_GPIO_irq", pdev);
        //error = request_irq(IRQ_GPIO_BANK0, stmp37xx_reset_handler, IRQ_TYPE_EDGE_FALLING, "GPIO irq test", pdev);
        //error = request_irq(IRQ_VDD5V, stmp37xx_reset_handler, IRQ_TYPE_NONE, "GPIO irq test", pdev);

        if (error != 0) {
		printk("[Touch] Cannot register interrupt stmp37xx_reset GPIO irq (err=%d)\n", error);
                return error;
        }
	else printk("[Touch] stmp37xx_reset irq register successfully\n\n");

        enable_irq(irq_num);

	stmp37xx_reset_gpio_intr_init();

	//ss_pm_register(SS_PM_TOUCHPAD_DEV, pm_stmp37xx_reset_callback);

	return 0;

 err_free_irq:
	platform_set_drvdata(pdev, NULL);
 err_free_dev:
	platform_set_drvdata(pdev, NULL);
 err_alloc:
	return error; 
 err_clk:
	return error;
}

static int __devexit stmp37xx_stmp37xx_reset_remove(struct platform_device *pdev)
{
	struct input_dev *input_dev = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver stmp37xx_stmp37xx_reset_driver = {
	.probe		= stmp37xx_stmp37xx_reset_probe,
	.remove		= __devexit_p(stmp37xx_stmp37xx_reset_remove),
	.suspend	= stmp37xx_stmp37xx_reset_suspend,
	.resume		= stmp37xx_stmp37xx_reset_resume,
	.driver		= {
		.name	= DRIVER_NAME,
	},
};

static int __init stmp37xx_stmp37xx_reset_init(void)
{
	//register_chrdev( MAJOR_NUMBER, "stmp37xx_reset_drv", &stmp37xx_reset_fops);
	return platform_driver_register(&stmp37xx_stmp37xx_reset_driver);
}

static void __exit stmp37xx_stmp37xx_reset_exit(void)
{
	//unregister_chrdev( MAJOR_NUMBER, "stmp37xx_reset_drv");
	platform_driver_unregister(&stmp37xx_stmp37xx_reset_driver);
}

module_init(stmp37xx_stmp37xx_reset_init);
module_exit(stmp37xx_stmp37xx_reset_exit);

MODULE_DESCRIPTION("STMP37XX MELFAS Touchpad Driver");
MODULE_LICENSE("GPL");
