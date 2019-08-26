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
#include "stmp37xx_touchpad_mel.h"
#include "stmp37xx_sw_touchpad_i2c.h"

#define DRIVER_NAME			"stmp37xx_touchpad_melfas"
#define BUTTON_MAX_NUM			9
#define TOUCHPAD_MELFAS_CHIPID		0x20 //FIXME: dhsong
#define POWER_BUTTON_CHECK_TIME		30 //200 //100=100ms
#define TOUCHPAD_BUTTON_CHECK_TIME	50 //10=10msec
#define DEL_TIMER		0	
#define WAKEUP_TIMER		1	
#define MAJOR_NUMBER		251	

//#define DEBUG
//#define TEST_HANDLER
//#define TIMER_FUNC

#ifndef TIMER_FUNC
 #define ENABLE_IRQ
#endif

static struct timer_list touchpad_timer;
static struct timer_list power_button_timer;
static unsigned char input_key = 0;
static unsigned char input_prev_key = 0;
static unsigned int pswitch_val=0;
static unsigned int pswitch_prev_val = 0;
static unsigned char scancode;
static unsigned char finger = 0;
static unsigned char prev_finger = 0;
static unsigned char write_buf[8] = {0};
static unsigned char read_buf[8] = {0};
//static unsigned int irq_num = IRQ_START_OF_EXT_GPIO + (1 * 32 + 21); //bank1,pin21
static unsigned int irq_num = IRQ_START_OF_EXT_GPIO + (0 * 32 + 8); //bank0, pin8
static unsigned int idle_status = 0;
static unsigned int wakeup_status = 0;
static unsigned int key_buffer[5];
static unsigned int key_count = 0;

static void pm_touchpad_callback(ss_pm_request_t event);

static void touchpad_i2c_init(void);
static void touchpad_gpio_intr_init(void);
static char touchpad_reg_write(unsigned char *reg_data, unsigned char length);
static char touchpad_reg_read(unsigned char reg_addr, unsigned char *reg_data, unsigned char length);
static int button_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

static unsigned char button_scancodes[BUTTON_MAX_NUM] = { 
        KEY_BACKSPACE,  KEY_UP,         KEY_ESC,  KEY_LEFT,
        KEY_ENTER,      KEY_RIGHT,      KEY_DOWN,
	KEY_F11,	KEY_F12
};

static unsigned char button_values[BUTTON_MAX_NUM] = { 
        0x67, 0x65, 0x66, 0x64, 0x61, 0x63, 0x62, 0x150, 0x200 
};


static inline char *_get_button_name(int index)
{
        char *button_names[BUTTON_MAX_NUM] = {
                "BACK", "VOL+", "MENU", "REW", "ENTER", "FF", "VOL-", "POWER", "USER"
        };
        return button_names[index];
};

static struct file_operations button_fops = {
        .ioctl = button_ioctl
};	

static int button_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;

	switch(cmd) {
		case DEL_TIMER:
			printk("[Touch] del_timer\n");
			ret = 1; //pbutton_del_timer();
			break;
		case WAKEUP_TIMER:
			printk("[Touch] wakeup_timer\n");
			ret = 1; //pbutton_wakeup_timer();
			break;
	}
	
	return ret;
}

static char touchpad_reg_write(unsigned char *reg_data, unsigned char length)
{
	char ret = 1;

	//swap_data (reg_data, length);

	do {
                ret = sw_touchpad_i2c_write(length, reg_data);
        } while (ret < 0);

        //swap_data (reg_data, length);  /* swap lo/hi byte */

        return ret;
}

static char touchpad_reg_read(unsigned char reg_addr, unsigned char *reg_data, unsigned char length)
{
        char ret = 1;
	int count = 1;
	int error;
	int i;
	static unsigned int start_write_time=0;
	static unsigned int end_write_time=0;
	
	write_buf[0] = reg_addr;

	//start_write_time = HW_RTC_MILLISECONDS_RD();
	error = touchpad_reg_write(write_buf, 1); //write to read reg 
	//printk("write middle time = %d\n", HW_RTC_MILLISECONDS_RD()-start_write_time);
	//printk("error(write) = %d\n\n", error);

	if(!error) {
		printk("i2c reg write error = %d\n\n", error);
		return error;
	}
	
        do {
                ret = sw_touchpad_i2c_read(length, reg_data);
		if(count == 5)
			break;
		count ++;
        } while (ret < 0);

        //swap_data (reg_data, length);  /* swap lo/hi byte */
#ifdef DEBUG
	for(i=0; i<length; i++) 
		printk("reg=%x, read_buf[%d] = 0x%08x\n", reg_addr, i, read_buf[i] );
#endif	

        return ret;
}

static void power_button_handler(unsigned long arg)
{
        struct platform_device *pdev = arg;
	struct input_dev *input_dev = platform_get_drvdata(pdev);
	int i = 0;
	int pw_scancode;
	static unsigned int key_status;

	del_timer(&power_button_timer);

        pswitch_val = HW_POWER_STS.B.PSWITCH;
        //printk("pswitch_val = 0x%08x\n\n", pswitch_val );

#if 0
        if(pswitch_val == 1) {
                i = 7;
                pw_scancode = button_scancodes[i];
                input_report_key(input_dev, pw_scancode, 1); 
                printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
        }   
        else if(pswitch_val == 3) {
                i = 8;
                pw_scancode = button_scancodes[i];
                input_report_key(input_dev, pw_scancode, 1); 
                printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
        }   


        else if( (pswitch_prev_val) && (!pswitch_val) ) { 
                if(pswitch_prev_val == 1)
                        i=7;
                else if(pswitch_prev_val == 3)
                        i=8;
                pw_scancode = button_scancodes[i];
                input_report_key(input_dev, pw_scancode, 0); 
                printk("[%s] release %d\n\n", _get_button_name(i), pw_scancode);
                pswitch_prev_val = 0;
        }   
        pswitch_prev_val = pswitch_val;
        power_button_timer.expires = get_jiffies_64() + (POWER_BUTTON_CHECK_TIME*HZ/1000); 
        add_timer(&power_button_timer);

#else
	if (pswitch_val && key_count<3) {
		key_buffer[key_count] = pswitch_val;
		//printk("key_buffer[%d] = %d\n", key_count, key_buffer[key_count]);
		key_count++;
	}
	else key_count = 0;
  #if 1	
	if(key_count==3){
		if( key_buffer[0]==3 || key_buffer[1]==3 || key_buffer[2]==3 ){
			pswitch_val = USER_KEY;
	                i = 8;
	                pw_scancode = button_scancodes[i];
			if ( pswitch_prev_val != pswitch_val) { 
		                input_report_key(input_dev, pw_scancode, 1); 
		                printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
			}
		        //printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
		}
		else { 
			pswitch_val = POWER_KEY;
	                i = 7;
	                pw_scancode = button_scancodes[i];
			if ( pswitch_prev_val != pswitch_val) { 
		                input_report_key(input_dev, pw_scancode, 1); 
		                printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
			}
	                //printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
		}
		pswitch_prev_val = pswitch_val;
	}

	if( (pswitch_prev_val) && (!pswitch_val) ) { 
	        if(pswitch_prev_val == 1)
	                i=7;
	        else if(pswitch_prev_val == 3)
	                i=8;
	        pw_scancode = button_scancodes[i];
	        input_report_key(input_dev, pw_scancode, 0); 
	        printk("[%s] release %d\n\n", _get_button_name(i), pw_scancode);
	        pswitch_prev_val = 0;
	}
  #endif
	power_button_timer.expires = get_jiffies_64() + (POWER_BUTTON_CHECK_TIME*HZ/1000); 
	add_timer(&power_button_timer);
#endif

        if(idle_status == 1) {
                del_timer(&power_button_timer);
#if 0
                key_status = 0; //1=press, 0=release
                pw_scancode = button_scancodes[7]; //if idle mode, release power button

                if(pswitch_prev_val == 1) {
                        input_report_key(input_dev, pw_scancode, key_status); //1=press, 0=release
                        if(key_status == 1)
                                printk("[%s] press(idle) %d\n\n", _get_button_name(7), pw_scancode);
                        else
                                printk("[%s] release(idle) %d\n\n", _get_button_name(7), pw_scancode);
                }
		pswitch_prev_val = 0;
#endif
                idle_status = 0;
        }
}

#ifdef ENABLE_IRQ
static irqreturn_t test_handler (int irq_num, void* dev_idp)
{
        struct platform_device *pdev = dev_idp;
	struct input_dev *input_dev = platform_get_drvdata(pdev);
        static unsigned char test_reg_addr;
        static int i = 0;
	static int error;
	static int length;

	printk("test_handler\n\n");

	touchpad_i2c_init();
	touchpad_gpio_intr_init();

#if 0 //sensitivity test	
	write_buf[0] = MELFAS_ADDR_COMMAND; 
	write_buf[1] = MELFAS_COMMAND_SENSITIVITY; 
	write_buf[2] = MELFAS_SENSITIVITY_HIGH; 
	length = 3;
        if ( !(touchpad_reg_write(write_buf, length)) )
		printk("i2c write fail\n");
        if ( !(touchpad_reg_read(MELFAS_ADDR_INTENSITY_TSP, read_buf, 7)) )
		printk("i2c read fail\n");
#endif

#if 0 //sleep mode test result OK
	write_buf[0] = MELFAS_ADDR_COMMAND; 
	write_buf[1] = MELFAS_COMMAND_ENTER_SLEEPMODE; //OK 
        if ( !(touchpad_reg_write(write_buf, 2)) )
		printk("i2c write fail\n");
        if ( !(touchpad_reg_read(MELFAS_ADDR_COMMAND, read_buf, 2)) )
		printk("i2c read fail\n");
#endif

#if 0 //fw, hw veriosn read. OK
	length=4;
	test_reg_addr=MELFAS_ADDR_HW_VERSION;
        if ( !(touchpad_reg_read(test_reg_addr, read_buf, length)) )
		printk("i2c read fail\n");
	for(i=0; i<length; i++) 
	{
		if (i==0)
			printk("\nMelfas touchpad hw version = %c", read_buf[i] );
		else 
			printk("%c", read_buf[i] );

		if (i==length-1)
			printk("\n");
	}

	length=4;
	test_reg_addr=MELFAS_ADDR_FW_VERSION;
        if ( !(touchpad_reg_read(test_reg_addr, read_buf, length)) )
		printk("i2c read fail\n");
	for(i=0; i<length; i++) 
	{
		if (i==0)
			printk("Melfas touchpad fw version = %c", read_buf[i] );
		else 
			printk("%c", read_buf[i] );

		if (i==length-1)
			printk("\n");
	}
#endif

	//feedback flag clear, for next read
	write_buf[0]=MELFAS_ADDR_FEEDBACK_FLAG;
	write_buf[1]=MELFAS_FEEDBACK_FLAG_CLR;
        if ( !(touchpad_reg_write(write_buf, 2)) ) {
		printk("i2c_write_test faili\n");
	}
	
	return IRQ_HANDLED;
}

static irqreturn_t touchpad_handler (int irq_num, void* dev_idp)
{
        struct platform_device *pdev = dev_idp;
	struct input_dev *input_dev = platform_get_drvdata(pdev);
        static int new_pressed = 0;
        static int old_pressed = 0;
        static unsigned char scancode;
        static int i = 0;
	static int error;
	static unsigned int start_time=0;
	static unsigned int end_time=0;

	start_time = HW_DIGCTL_MICROSECONDS_RD();
	//udelay(1000);
	//printk("udelay = %d\n", HW_DIGCTL_MICROSECONDS_RD() - start_time);


        //printk("[touch] irq handler\n");
	touchpad_i2c_init();
	touchpad_gpio_intr_init();

#ifdef FAST_ISR_VER //20081006, read 1 byte for improving isr processing
	if ( !(touchpad_reg_read(MELFAS_ADDR_KEY_STATUS, read_buf, 1)) )
		printk("i2c_read_test faili\n");

	finger = (read_buf[0] >> 7); //status(press/release)	
	input_key = read_buf[0] & 0x7f; //key value	
	//printk("finger = 0x%08x\n", finger);
	//printk("input_key = 0x%08x\n", input_key);

        if ( finger==0x1 ) {
                for (i = 0; i < BUTTON_MAX_NUM; i++) {
                        if (input_key == button_values[i])
                                break;
                }
                scancode = button_scancodes[i];
                //printk("input_dev->name = %s\n\n",input_dev->name,  __FILE__, __LINE__);
                //printk("init = %d\n", init);
                //printk("input_key = %d\n", input_key);
        //        printk("[%s] press %d\n", _get_button_name(i), scancode);
                input_report_key(input_dev, scancode, 1);
        }
 
        if ( finger==0x0 ) { 
                for (i = 0; i < BUTTON_MAX_NUM; i++) {
                        if (input_key == button_values[i])
                                break;
                }
                scancode = button_scancodes[i];
                //printk("input_dev->name = %s\n\n",input_dev->name,  __FILE__, __LINE__);
                //printk("init = %d\n", init);
                //printk("input_key = %d\n", input_key);
        //        printk("[%s] release %d\n", _get_button_name(i), scancode);
                input_report_key(input_dev, scancode, 0);
        }
	end_time = HW_DIGCTL_MICROSECONDS_RD();
	printk("isr time = %d\n", end_time - start_time);
#else
	if ( !(touchpad_reg_read(MELFAS_ADDR_KEY_STATUS, read_buf, 2)) )
		printk("i2c_read_test faili\n");
	finger = read_buf[0]; //status(press/release)	
	input_key = read_buf[1]; //key value	
	//printk("finger = 0x%08x\n", finger);
	//printk("input_key = 0x%08x\n", input_key);

	//feedback flag clear
	write_buf[0] = MELFAS_ADDR_FEEDBACK_FLAG;
	write_buf[1] = MELFAS_FEEDBACK_FLAG_CLR;
        if ( !(touchpad_reg_write(write_buf, 2)) )
		printk("i2c_write_test faili\n");

        if ( finger==MELFAS_CONTACTED_STATUS ) {
                for (i = 0; i < BUTTON_MAX_NUM; i++) {
                        if (input_key == button_values[i])
                                break;
                }
                scancode = button_scancodes[i];
                //printk("input_dev->name = %s\n\n",input_dev->name,  __FILE__, __LINE__);
                //printk("init = %d\n", init);
                //printk("input_key = %d\n", input_key);
                printk("[%s] press %d\n", _get_button_name(i), scancode);
                input_report_key(input_dev, scancode, 1);
        }
 
        if ( finger==MELFAS_UNCONTACTED_STATUS ) { 
                for (i = 0; i < BUTTON_MAX_NUM; i++) {
                        if (input_key == button_values[i])
                                break;
                }
                scancode = button_scancodes[i];
                //printk("input_dev->name = %s\n\n",input_dev->name,  __FILE__, __LINE__);
                //printk("init = %d\n", init);
                //printk("input_key = %d\n", input_key);
                printk("[%s] release %d\n", _get_button_name(i), scancode);
                input_report_key(input_dev, scancode, 0);
        }
#endif

	return IRQ_HANDLED;
}
	
#else //TIMER_FUNC
static void button_handler(unsigned long arg)
{
        struct platform_device *pdev = arg;
	struct input_dev *input_dev = platform_get_drvdata(pdev);
        static int new_pressed = 0;
        static int old_pressed = 0;
        static unsigned char scancode;
        static int i = 0;
	static int error;
	static unsigned int pswitch_val=0;
	//static unsigned char cmd_size, reply_size;
	//static unsigned char *cmd, *reply;

	del_timer(&touchpad_timer);
	touchpad_i2c_init();
	touchpad_gpio_intr_init();

#if 0
        pswitch_val = HW_POWER_STS.B.PSWITCH;
        //printk("pswitch_val = 0x%08x\n\n", pswitch_val );
        if(pswitch_val == 1) {
                i = 7;
                pswitch_prev_val = pswitch_val;
                scancode = button_scancodes[i];
                printk("[%s] press %d\n", _get_button_name(i), scancode);
                input_report_key(input_dev, scancode, 1); 
        }   
        else if(pswitch_val == 3) {
                i = 8;
                pswitch_prev_val = pswitch_val;
                scancode = button_scancodes[i];
                printk("[%s] press %d\n", _get_button_name(i), scancode);
                input_report_key(input_dev, scancode, 1); 
        }   
        else if( (pswitch_prev_val) && (!pswitch_val) ) { 
                if(pswitch_val == 1)
                        i=7;
                else if(pswitch_val == 3)
                        i=8;
                scancode = button_scancodes[i];
                input_report_key(input_dev, scancode, 0); 
                printk("[%s] release %d\n\n", _get_button_name(i), scancode);
                pswitch_prev_val = 0;
        }   
#endif
	//feedback flag set, for read
	write_buf[0] = MELFAS_ADDR_FEEDBACK_FLAG;
	write_buf[1] = MELFAS_FEEDBACK_FLAG_SET;
        if ( !(touchpad_reg_write(write_buf, 2)) )
		printk("i2c_write_test faili\n");

#if 0 
	//i2c read/write test
	write_buf[0] = MELFAS_ADDR_LED_ONOFF;
	write_buf[1] = 0x5;
        if ( !(touchpad_reg_write(write_buf, 2)) )
		printk("i2c_write_test faili\n");
        if ( !(touchpad_reg_read(MELFAS_ADDR_LED_ONOFF, read_buf, 2)) )
        //if ( !(touchpad_reg_read(MELFAS_ADDR_HW_VERSION, read_buf, 2)) )
        //if ( !(touchpad_reg_read(MELFAS_ADDR_FW_VERSION, read_buf, 2)) )
		printk("i2c_read_test faili\n");
#endif
        if ( !(touchpad_reg_read(MELFAS_ADDR_KEY_STATUS, read_buf, 2)) )
		printk("i2c_read_test faili\n");
	finger = read_buf[0];	
	input_key = read_buf[1];	
	//printk("finger = 0x%08x\n", finger);
	//printk("input_key = 0x%08x\n", input_key);

	//feedback flag clear
	write_buf[0]=MELFAS_ADDR_FEEDBACK_FLAG;
	write_buf[1]=MELFAS_FEEDBACK_FLAG_CLR;
        if ( !(touchpad_reg_write(write_buf, 2)) )
		printk("i2c_write_test faili\n");

        if ( finger==MELFAS_CONTACTED_STATUS ) {
                for (i = 0; i < BUTTON_MAX_NUM; i++) {
                        if (input_key == button_values[i])
                                break;
                }
                scancode = button_scancodes[i];
                //printk("input_dev->name = %s\n\n",input_dev->name,  __FILE__, __LINE__);
                //printk("init = %d\n", init);
                //printk("input_key = %d\n", input_key);
                printk("[%s] press %d\n", _get_button_name(i), scancode);
                input_report_key(input_dev, scancode, 1);
        }
 
        if ( (finger==MELFAS_UNCONTACTED_STATUS) && (prev_finger==MELFAS_CONTACTED_STATUS) ) { 
                for (i = 0; i < BUTTON_MAX_NUM; i++) {
                        if (input_key == button_values[i])
                                break;
                }
                scancode = button_scancodes[i];
                //printk("input_dev->name = %s\n\n",input_dev->name,  __FILE__, __LINE__);
                //printk("init = %d\n", init);
                //printk("input_key = %d\n", input_key);
                printk("[%s] release %d\n", _get_button_name(i), scancode);
                input_report_key(input_dev, scancode, 0);
        }
	prev_finger = finger;
        touchpad_timer.expires = get_jiffies_64() + (TOUCHPAD_BUTTON_CHECK_TIME*HZ/1000); 
        add_timer(&touchpad_timer);
}
#endif //ENABLE_IRQ

static int stmp37xx_touchpad_open(struct input_dev *dev)
{
	printk("[touchpad] Melfas touchpad driver opened\n");
	return 0;
}

static void stmp37xx_touchpad_close(struct input_dev *dev)
{
	printk("[touchpad] Melfas touchpad driver closed\n");
}

static void touchpad_i2c_init(void)
{
	sw_touchpad_i2c_init(TOUCHPAD_MELFAS_CHIPID);
}

static void touchpad_gpio_intr_init(void)
{
#if 1 //gpio func init
        HW_PINCTRL_MUXSEL0_SET(0x3 << 16);      //bank0:8, INTR
        //HW_PINCTRL_MUXSEL0_SET(0x3 << 18);      //bank0:9, T-data
        //HW_PINCTRL_MUXSEL1_SET(0x3 << 4);       //bank0:18, T-clock

        /* 1. Configure the CLK GPIO as an INPUT */
        /* Set bank0:18 to input Enable for using "CLK" signal */
        //HW_PINCTRL_DOE0_CLR(0x1 << 18);

        /* 2. Configure the INTR GPIO as an INPUT */
        /* Set bank0:8 to input Enable for using "INTR" signal */
        HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE); 
        HW_PINCTRL_MUXSEL0_SET(0x3 << 16);      //bank0:8, INTR
        HW_PINCTRL_DOE0_CLR(0x1 << 8); 
        HW_PINCTRL_PIN2IRQ0_SET(0x1 << 8); //select interrupt source pin 
        HW_PINCTRL_IRQEN0_SET(0x1 << 8); //IRQ enable
        HW_PINCTRL_IRQLEVEL0_CLR(0x1 << 8); //1:level detection, 0:edge detection
        HW_PINCTRL_IRQPOL0_CLR(0x1 << 8); //1:high or rising edge, 0:low or falling edge 
        HW_PINCTRL_IRQSTAT0_CLR(0x1 << 8); 

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

static void pm_touchpad_callback(ss_pm_request_t event)
{
        switch(event) {
                case SS_PM_IDLE:
                        //printk("[Touchpad] SS_PM_IDLE \n\n");
                        idle_status = 1;
#ifdef TIMER_FUNC
                        del_timer(&touchpad_timer);
#endif
                        break;
                case SS_PM_SET_WAKEUP:
                        //printk("[Touchpad] SS_PM_SET_WAKEUP \n\n");
                        idle_status = 0;
                        wakeup_status = 1;
                        //del_timer(&power_button_timer);
                        power_button_timer.expires = get_jiffies_64() + (POWER_BUTTON_CHECK_TIME*HZ/1000);
                        add_timer(&power_button_timer);
#ifdef TIMER_FUNC
                        del_timer(&touchpad_timer);
                        touchpad_timer.expires = get_jiffies_64() + (TOUCHPAD_BUTTON_CHECK_TIME*HZ/1000);
                        add_timer(&touchpad_timer);
#endif
                        break;
                case SS_PM_LCD_ON:
                        //printk("[Touchpad] SS_PM_LCD_ON \n\n");
                        //set_touchpad_active();
                        //del_timer(&touchpad_timer);
                        //touchpad_timer.expires = get_jiffies_64() + (POWER_BUTTON_CHECK_TIME*HZ/1000);
                        //add_timer(&touchpad_timer);
                        break;
                case SS_PM_LCD_OFF:
                        //printk("[Touchpad] SS_PM_LCD_OFF \n\n");
                        //set_touchpad_sleep();
                        //del_timer(&touchpad_timer);
                        break;
        }
}

#ifdef CONFIG_PM
#else
#define stmp37xx_touchpad_suspend	NULL
#define stmp37xx_touchpad_resume	NULL
#endif

static int __devinit stmp37xx_touchpad_probe(struct platform_device *pdev)
{
	struct input_dev *input_dev;
	int i, row, col, error;
	int code;
	int read_length;
	/* Create and register the input driver. */
	input_dev = input_allocate_device();
	if (!input_dev) {
		printk(KERN_ERR "Cannot request stmp37xx_touchpad device\n");
		error = -ENOMEM;
		goto err_alloc;
	}

	input_dev->name = DRIVER_NAME;
	input_dev->id.bustype = BUS_HOST;
	input_dev->open = stmp37xx_touchpad_open;
	input_dev->close = stmp37xx_touchpad_close;
	input_dev->dev.parent = &pdev->dev;
	input_dev->evbit[0] = BIT_MASK(EV_KEY);

        for (i = 0; i < ARRAY_SIZE(button_scancodes); i++) {
                int code = button_scancodes[i];
                set_bit(code, input_dev->keybit);
        }
        clear_bit(0, input_dev->keybit);

	platform_set_drvdata(pdev, input_dev);

	/* Register the input device */
	error = input_register_device(input_dev);
	if (error) {
		printk("input register ERROR, %s, %d\n\n", __FILE__, __LINE__);
		goto err_free_dev;
	}
        printk("%s, %d\n\n", __FILE__, __LINE__);

        //mep_init();
	//tkey_Reset();
        //i2c_read_test(MELFAS_ADDR_FW_VERSION);

#ifdef ENABLE_IRQ
        // irqflas can be
        //         IRQ_TYPE_EDGE_RISING, IRQ_TYPE_EDGE_FALLING
        //         IRQ_TYPE_LEVEL_HIGH, IRQ_TYPE_LEVEL_LOW
 #ifdef TEST_HANDLER
        error = request_irq(irq_num, test_handler, IRQ_TYPE_EDGE_FALLING, "Touchpad_GPIO_irq", pdev);
        //error = request_irq(IRQ_GPIO_BANK0, test_handler, IRQ_TYPE_EDGE_FALLING, "GPIO irq test", pdev);
        //error = request_irq(IRQ_VDD5V, test_handler, IRQ_TYPE_NONE, "GPIO irq test", pdev);
 #else     
	error = request_irq(irq_num, touchpad_handler, IRQ_TYPE_EDGE_FALLING, "Touchpad_GPIO_irq", pdev);
 #endif
        if (error != 0) {
		printk("[Touch] Cannot register interrupt touchpad GPIO irq (err=%d)\n", error);
                return error;
        }
	else printk("[Touch] Touchpad GPIO irq register successfully\n\n");

        enable_irq(irq_num);
#endif //ENABLE_IRQ

        init_timer(&power_button_timer);
        power_button_timer.expires = get_jiffies_64() + (POWER_BUTTON_CHECK_TIME*HZ/1000); 
        power_button_timer.function = power_button_handler;
        power_button_timer.data = pdev; //input_dev;
        add_timer(&power_button_timer);

#ifdef TIMER_FUNC
        init_timer(&touchpad_timer);
        touchpad_timer.expires = get_jiffies_64() + (TOUCHPAD_BUTTON_CHECK_TIME*HZ/1000); 
        touchpad_timer.function = button_handler;
        touchpad_timer.data = pdev; //input_dev;
        add_timer(&touchpad_timer);
#endif
		

	touchpad_i2c_init();
	touchpad_gpio_intr_init();

#if 1 //print melfas touchpad hw, fw version 
	read_length=4;
        if ( !(touchpad_reg_read(MELFAS_ADDR_HW_VERSION, read_buf, read_length)) )
		printk("touchpad reg read error, %d\n", __LINE__);
	for(i=0; i<read_length; i++) 
	{
		if (i==0)
			printk("\nMelfas touchpad hw version = %c", read_buf[i] );
		else 
			printk("%c", read_buf[i] );

		if (i==read_length-1)
			printk("\n");
	}

	read_length=4;
        if ( !(touchpad_reg_read(MELFAS_ADDR_FW_VERSION, read_buf, read_length)) )
		printk("touchpad reg read error\n");
	for(i=0; i<read_length; i++) 
	{
		if (i==0)
			printk("Melfas touchpad fw version = %c", read_buf[i] );
		else 
			printk("%c", read_buf[i] );

		if (i==read_length-1)
			printk("\n");
	}

	//feedback flag clear
	write_buf[0]=MELFAS_ADDR_FEEDBACK_FLAG;
	write_buf[1]=MELFAS_FEEDBACK_FLAG_CLR;
        if ( !(touchpad_reg_write(write_buf, 2)) )
		printk("touchpad reg write error, %d\n", __LINE__);
#endif
	ss_pm_register(SS_PM_TOUCHPAD_DEV, pm_touchpad_callback);

	return 0;

 err_free_irq:
	platform_set_drvdata(pdev, NULL);
 err_free_dev:
	input_free_device(input_dev);
 err_alloc:
	return error; 
 err_clk:
	return error;
}

static int __devexit stmp37xx_touchpad_remove(struct platform_device *pdev)
{
	struct input_dev *input_dev = platform_get_drvdata(pdev);
	
	del_timer(&touchpad_timer);
	input_unregister_device(input_dev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver stmp37xx_touchpad_driver = {
	.probe		= stmp37xx_touchpad_probe,
	.remove		= __devexit_p(stmp37xx_touchpad_remove),
	.suspend	= stmp37xx_touchpad_suspend,
	.resume		= stmp37xx_touchpad_resume,
	.driver		= {
		.name	= DRIVER_NAME,
	},
};

static int __init stmp37xx_touchpad_init(void)
{
	register_chrdev( MAJOR_NUMBER, "button_drv", &button_fops);
	return platform_driver_register(&stmp37xx_touchpad_driver);
}

static void __exit stmp37xx_touchpad_exit(void)
{
	unregister_chrdev( MAJOR_NUMBER, "button_drv");
	platform_driver_unregister(&stmp37xx_touchpad_driver);
}

module_init(stmp37xx_touchpad_init);
module_exit(stmp37xx_touchpad_exit);

MODULE_DESCRIPTION("STMP37XX MELFAS Touchpad Driver");
MODULE_LICENSE("GPL");
