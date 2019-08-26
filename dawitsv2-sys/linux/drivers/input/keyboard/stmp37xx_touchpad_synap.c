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
#include "stmp37xx_touchpad_synap.h"
#include "stmp37xx_sw_touchpad_i2c.h"
#include "SynaOT.h"
#include "UserOT.h"

#define DRIVER_NAME			"stmp37xx_touchpad_synaptics_OT"
#define BUTTON_MAX_NUM			9
//#define TOUCHPAD_SYNAPTICS_CHIPID		0x2C //FIXME: dhsong
#define POWER_BUTTON_CHECK_TIME		30 //200 //100=100ms
#define TOUCHPAD_BUTTON_CHECK_TIME	50 //10=10msec
#define DEL_TIMER		0	
#define WAKEUP_TIMER		1	
#define MAJOR_NUMBER		251	
#define LOOP_MAX		1 //5	

//#define DEBUG
//#define TEST_HANDLER
//#define TIMER_FUNC
//#define USE_CHRDEV

#ifndef TIMER_FUNC 
 #define ENABLE_IRQ
#endif

#define DEBUG_TOUCHPAD
#ifdef DEBUG_TOUCHPAD
#define PDEBUG_TOUCHPAD(fmt, args...) printk(fmt , ## args)
#else
#define PDEBUG_TOUCHPAD(fmt, args...) do {} while(0)
#endif

static struct timer_list touchpad_timer;
static struct timer_list power_button_timer;
static unsigned char input_key = 0;
static unsigned char input_first_key = 0;
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
static int first_time;
static int second_time;
static int wakeup_event = 0;
static int loop_touchpad = 0; 

static void pm_touchpad_callback(ss_pm_request_t event);

static void touchpad_gpio_intr_init(void);
static int button_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
void touchpad_write(OT_U8 addr_high, OT_U8 addr_low, OT_U8 data_high, OT_U8 data_low);
void touchpad_read(OT_U8 addr_high, OT_U8 addr_low);
static void set_touchpad_sleep(void);
static void set_touchpad_active(void);
static void touchpad_adjust_sensitivity(void);
static void touchpad_reset(void);

static unsigned char button_scancodes[BUTTON_MAX_NUM] = { 
        KEY_BACKSPACE,  KEY_UP,         KEY_ESC,  KEY_LEFT,
        KEY_ENTER,      KEY_RIGHT,      KEY_DOWN,
	KEY_F11,	KEY_F12
};

static unsigned char button_values[BUTTON_MAX_NUM] = { 
        0x2, 0x4, 0x8, 0x1, 0x10, 0x20, 0x40, 0x150, 0x200 
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
		//case DEL_TIMER: //0
		//	printk("[Touch] del_timer\n");
		//	ret = 1; //pbutton_del_timer();
		//	first_time = HW_DIGCTL_MICROSECONDS_RD();
		//	break;
		case DEL_TIMER: //0
			printk("[Touch] call sleep func using ioctl \n");
			//ret = 1; //pbutton_del_timer();
			//first_time = HW_DIGCTL_MICROSECONDS_RD();
			set_touchpad_sleep();
			break;
		case WAKEUP_TIMER: //1
			second_time = HW_DIGCTL_MICROSECONDS_RD();
			printk("check_usec = %d\n", second_time - first_time);
			printk("[Touch] wakeup_timer\n");
			ret = 1; //pbutton_wakeup_timer();
			break;
	}
	
	return ret;
}

void touchpad_read(OT_U8 addr_high, OT_U8 addr_low)
{
	static int error;

	error = OT_touchpad_read(&read_buf, addr_high, addr_low);	
	
	if(error == OT_SUCCESS)
		printk("reg 0x%02x%02x data = 0x%02x%02x\n", addr_high, addr_low, read_buf[0], read_buf[1]);

	else if(error == OT_FAILURE) {
                printk("<touchpad>OT_touchpad_read Fail, %s, %d\n", __FILE__, __LINE__);
                printk("<touchpad>re-init touchpad, %s, %d\n", __FILE__, __LINE__);
		touchpad_reset();
	}
	
}

void touchpad_write(OT_U8 addr_high, OT_U8 addr_low, OT_U8 data_high, OT_U8 data_low)
{
	static int error;

	//touchpad_read(addr_high, addr_low);	

	write_buf[0] = addr_high;   
	write_buf[1] = addr_low;   
	write_buf[2] = data_high;   
	write_buf[3] = data_low;   
	OT_touchpad_write(write_buf, 4);

	//touchpad_read(addr_high, addr_low);	
}

static void touchpad_adjust_sensitivity()
{
	touchpad_write(0x00, 0x10, 0xc0, 0xc0); 	
	touchpad_write(0x00, 0x11, 0xc0, 0xc0); 	
	touchpad_write(0x00, 0x12, 0xc0, 0xc0); 	
	touchpad_write(0x00, 0x13, 0xc0, 0xc0); 	
}

static void set_touchpad_sleep(void)
{
	touchpad_write(0x00, 0x04, 0x00, 0x00); //reg 0x0004, write:0x0000 //button disable, 1206
	touchpad_write(0x00, 0x01, 0x00, 0x80); //reg 0x0001, write:0x0080
 
        printk("[Touchpad] sleep\n");
}
 
static void set_touchpad_active(void)
{
	int error;

#if 1 //re-init touchpad
	touchpad_reset();
#else
	gpio_init();

	touchpad_gpio_intr_init();
	
	touchpad_write(0x00, 0x01, 0x00, 0x20); //reg 0x0001, write:0x0020
	touchpad_write(0x00, 0x04, 0x00, 0x7f); //reg 0x0004, write:0x0000 //button enable, 1206
#endif
	//printk("[Touchpad] active\n");
}

static void power_button_handler(unsigned long arg)
{
        struct platform_device *pdev = arg;
	struct input_dev *input_dev = platform_get_drvdata(pdev);
	int i = -1;
	unsigned int pw_scancode;
	static unsigned int key_status;

	//del_timer(&power_button_timer);
	//printk("touchpad intr pin status = %d\n\n", stmp37xx_gpio_get_level( pin_GPIO(0, 8) ) >> 8 );


	if(wakeup_event == 1 && HW_POWER_STS.B.VDD5V_GT_VDDIO==0) {
		pswitch_val = 1;
		key_count = 3;
	}
	else
	{
	        pswitch_val = HW_POWER_STS.B.PSWITCH;
	        //printk("pswitch_val = 0x%08x\n", pswitch_val );

		if (pswitch_val && key_count<3) {
			key_buffer[key_count] = pswitch_val;
			//printk("key_buffer[%d] = %d\n", key_count, key_buffer[key_count]);
			key_count++;
		}
		else key_count = 0;
	}
  #if 1	
	if(key_count==3){
		if( key_buffer[0]==3 || key_buffer[1]==3 || key_buffer[2]==3 ){
			pswitch_val = USER_KEY;
	                i = 8;
	                //pw_scancode = button_scancodes[i];
	                pw_scancode = KEY_F12;
			if ( pswitch_prev_val != pswitch_val) { 
		                input_report_key(input_dev, pw_scancode, 1); 
		                //printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
		                PDEBUG_TOUCHPAD("<touchpad> USER button pressed %d\n", pw_scancode);
			}
		        //printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
		}
		else { 
			pswitch_val = POWER_KEY;
	                i = 7;
	                //pw_scancode = button_scancodes[i];
	                pw_scancode = KEY_F11;
			if ( pswitch_prev_val != pswitch_val) { 
		                input_report_key(input_dev, pw_scancode, 1); 
		                //printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
		                PDEBUG_TOUCHPAD("<touchpad> POWER button pressed %d\n", pw_scancode);
			}
	                //printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
		}
		pswitch_prev_val = pswitch_val;
	}

	if( (pswitch_prev_val) && (!pswitch_val) ) { 
	        if(pswitch_prev_val == 1) {
	                i=7;
	        	pw_scancode = KEY_F11;
	        	input_report_key(input_dev, pw_scancode, 0); 
		        PDEBUG_TOUCHPAD("<touchpad> POWER button release %d\n", pw_scancode);
		}
	        else if(pswitch_prev_val == 3) {
	                i=8;
	        	pw_scancode = KEY_F12;
	        	input_report_key(input_dev, pw_scancode, 0); 
		        PDEBUG_TOUCHPAD("<touchpad> USER button release %d\n", pw_scancode);
		}
	        //pw_scancode = button_scancodes[i];
	        //input_report_key(input_dev, pw_scancode, 0); 
	        //printk("[%s] release %d\n\n", _get_button_name(i), pw_scancode);
	        pswitch_prev_val = 0;
	}
  #endif
	
	del_timer(&power_button_timer);
	power_button_timer.expires = get_jiffies_64() + (POWER_BUTTON_CHECK_TIME*HZ/1000); 
	wakeup_event = 0;
	add_timer(&power_button_timer);

#if 0
        if(idle_status == 1) {
                del_timer(&power_button_timer);
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
                idle_status = 0;
        }
#endif
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

	printk("test_handler\n");

	printk("touchpad intr pin status = %d\n", stmp37xx_gpio_get_level( pin_GPIO(0, 8) ) >> 8 );
	printk("touchpad intr pin status = %d\n", (HW_PINCTRL_DIN0_RD()>>8) & 0x00000001 );
	printk("touchpad intr pin status = %d\n", HW_PINCTRL_DIN0_RD() & (0x1 << 8) );

	touchpad_gpio_intr_init();

	stmp37xx_gpio_set_af( pin_GPIO(0, 8), 3); //gpio func
	stmp37xx_gpio_set_dir( pin_GPIO(0, 8), GPIO_DIR_IN); //data output

	error = OT_touchpad_read(&read_buf, 0x01, 0x09); //read reg 0x0109 

	printk("touchpad intr pin status = %d\n", stmp37xx_gpio_get_level( pin_GPIO(0, 8) ) >> 8 );
	printk("touchpad intr pin status = %d\n", (HW_PINCTRL_DIN0_RD() >> 8)&0x00000001 );
	printk("touchpad intr pin status = %d\n", HW_PINCTRL_DIN0_RD() & (0x1 << 8) );

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
#if 0 //delay test
	//start_time = HW_DIGCTL_MICROSECONDS_RD();
	start_time = HW_RTC_MILLISECONDS_RD();
	//STMP37XX_UDELAY(1000);
	udelay(2000);
	//end_time = HW_DIGCTL_MICROSECONDS_RD();
	end_time = HW_RTC_MILLISECONDS_RD();
	printk("udelay = %d\n", (end_time - start_time)*1000 );

	start_time = 0;
	end_time = 0;
	start_time = HW_RTC_MILLISECONDS_RD();
	//STMP37XX_MDELAY(30);
	mdelay(100);
	end_time = HW_RTC_MILLISECONDS_RD();
	printk("mdelay = %d\n", end_time - start_time);
	
	//udelay(10);
#endif
        //printk("[touch] irq handler\n");
	//printk("%d, HW_POWER_MINPWR = 0x%08x\n", __LINE__, HW_POWER_MINPWR_RD() );

	gpio_init(); //add 1208

	touchpad_gpio_intr_init();

	error = OT_touchpad_read(&read_buf, 0x01, 0x09); //read reg 0x0109	
	if(error == OT_FAILURE) {
                printk("<touchpad>OT_touchpad_read Fail, %s, %d\n", __FILE__, __LINE__);
                printk("<touchpad>touchpad is reset, %s, %d\n", __FILE__, __LINE__);
		touchpad_reset();
		return IRQ_NONE;
	}
	
	input_key = read_buf[1]; //key value	
	printk("<touchpad> input_key = 0x%08x\n", input_key);  //enable 1206



if(HW_POWER_STS.B.PSWITCH == 0) { //if power_key or user_key is not pressed
	if( (!input_first_key) )
                input_first_key = input_key;

        if ( (input_key) && (input_first_key) ) {
                for (i = 0; i < BUTTON_MAX_NUM; i++) {
                        if (input_first_key == button_values[i])
                                break;
                }
                scancode = button_scancodes[i];
                //printk("input_dev->name = %s\n\n",input_dev->name,  __FILE__, __LINE__);
                //printk("init = %d\n", init);
                //printk("input_key = %d\n", input_key);
		if ( input_prev_key != input_first_key) {
	                input_report_key(input_dev, scancode, 1);
	                //printk("[%s] press %d\n", _get_button_name(i), scancode);
		}
		input_prev_key = input_first_key;
        }
//} 
        if ( (input_first_key) && (!input_key) ) { 
                for (i = 0; i < BUTTON_MAX_NUM; i++) {
                        if (input_first_key == button_values[i])
                                break;
                }
                scancode = button_scancodes[i];
                //printk("input_dev->name = %s\n\n",input_dev->name,  __FILE__, __LINE__);
                //printk("init = %d\n", init);
                //printk("input_key = %d\n", input_key);
                input_report_key(input_dev, scancode, 0);
                //printk("[%s] release %d\n", _get_button_name(i), scancode);
		input_first_key = 0;
                input_prev_key = 0;
        }
} 
	//end_time = HW_DIGCTL_MICROSECONDS_RD();
	//printk("isr_time = %d\n", end_time - start_time);
	//printk("%d, touchpad intr pin status = %d\n", __LINE__, stmp37xx_gpio_get_level( pin_GPIO(0, 8) ) >> 8 ); //1206

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
	touchpad_gpio_intr_init();
#if 0 //for malfas
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
#endif //for melfas
	prev_finger = finger;
        touchpad_timer.expires = get_jiffies_64() + (TOUCHPAD_BUTTON_CHECK_TIME*HZ/1000); 
        add_timer(&touchpad_timer);
}
#endif //ENABLE_IRQ

static int stmp37xx_touchpad_open(struct input_dev *dev)
{
	//printk("[touchpad] Synaptics OT touchpad driver opened\n");
	return 0;
}

static void stmp37xx_touchpad_close(struct input_dev *dev)
{
	printk("[touchpad] Synaptics OT touchpad driver closed\n");
}

static void touchpad_gpio_intr_init(void)
{
#if 1 //gpio func init
        //HW_PINCTRL_MUXSEL0_SET(0x3 << 18);      //bank0:9, T-data
        //HW_PINCTRL_MUXSEL1_SET(0x3 << 4);       //bank0:18, T-clock

        /* 1. Configure the CLK GPIO as an INPUT */
        /* Set bank0:18 to input Enable for using "CLK" signal */
        //HW_PINCTRL_DOE0_CLR(0x1 << 18);

        /* 2. Configure the INTR GPIO as an INPUT */
        /* Set bank0:8 to input Enable for using "INTR" signal */
        //HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE); 
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
                        //printk("<touchpad> IDLE \n");
			set_touchpad_sleep();
                	del_timer(&power_button_timer);
        		disable_irq(irq_num);
#ifdef TIMER_FUNC
                        del_timer(&touchpad_timer);
#endif
                        break;
                case SS_PM_SET_WAKEUP:
                        //printk("<touchpad> WAKEUP \n");
        		enable_irq(irq_num);
			set_touchpad_active();
                        del_timer(&power_button_timer);
                        power_button_timer.expires = get_jiffies_64() + (POWER_BUTTON_CHECK_TIME*HZ/1000);
			wakeup_event = 1;
                        add_timer(&power_button_timer);
#ifdef TIMER_FUNC
                        del_timer(&touchpad_timer);
                        touchpad_timer.expires = get_jiffies_64() + (TOUCHPAD_BUTTON_CHECK_TIME*HZ/1000);
                        add_timer(&touchpad_timer);
#endif
                        break;
		/* to resolve the problem that touchpad auto running when 5v is connected */
                case SS_PM_5V_REMOVED:
                        printk("<Touchpad> 5v is removed\n");
                        set_touchpad_sleep();
                        set_touchpad_active();
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

static void touchpad_reset(void)
{
	int error;
	int loop_count = 0;

	while(loop_count < LOOP_MAX) {	
		//printk("<touchpad>loop_count = %d\n", loop_count);

		touchpad_gpio_intr_init();
	
		error = OT_touchpad_init();
	
		if(error != OT_FAILURE) {
			printk("<touchpad>touchpad reset success\n");
			break;
		}
		else if(error == OT_FAILURE) {
	                printk("<touchpad>touchpad init failed, loop_count=%d\n", loop_count);
       		}
		loop_count++;
	}
	//return error;
}

static int read_all_touchpad_reg()
{
	int i; 
	
	for(i=0; i<20; i++) {
		touchpad_read(0x00, i); //confirm sensitivity
	}


}

static int touchpad_proc_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *buf;

	buf = page;

	printk("\n<< <touchpad> print register val >>\n");
	read_all_touchpad_reg();

	*eof = 1;

        return buf - page;
}

static ssize_t touchpad_proc_write (struct file * file, const char * buf, unsigned long count, void *data)
{
	char cmd[64];
        unsigned int value1, value2;
        unsigned int reg_high, reg_low, val_high, val_low;
        unsigned long flags;

        sscanf(buf, "%s %u %u", cmd, &value1, &value2);

        if (!strcmp(cmd, "sens")) {
                	//set_pm_mode_stable(value1)
                        return count;
        }

        else if (!strcmp(cmd, "reset")) {
                touchpad_reset();
        }
	
	else if(!strcmp(cmd, "reg")){
		reg_high = (value1 & 0xFF00) >> 8;
		reg_low  = (value1 & 0x00FF); 
		val_high = (value2 & 0xFF00) >> 8;
		val_low  = (value2 & 0x00FF); 
		printk("reg = 0x%02x%02x\n", reg_high, reg_low);
		printk("val = 0x%02x%02x\n", val_high, val_low);
	
		printk("\n>>>>>print register value\n");
		touchpad_write(reg_high, reg_low, val_high, val_low); 
		read_all_touchpad_reg();
	}

        else if (!strcmp(cmd, "sleep")) {
                set_touchpad_sleep();
        }

	return count;

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
	int otinit_status;
	int ocotp_test = 0;
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

	//ocotp_test = lf_get_hw_option_type();
	//printk("ocotp_test = %d\n\n", ocotp_test);

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
	//else printk("<touchpad> Synaptics OT Touchpad GPIO irq register successfully\n");

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
		

#ifdef TEST_HANDLER //force intr pin set input(high)
	stmp37xx_gpio_set_af( pin_GPIO(0, 8), 3); //gpio func
	stmp37xx_gpio_set_dir( pin_GPIO(0, 8), GPIO_DIR_IN); //data output

	touchpad_gpio_intr_init();
	//gpio_init();
        //OT_Init_BitBang_I2C();                  // Do not remove this. 
	//OT_I2C_START();
	error = OT_touchpad_init();
#else

  #if 0
	loop_touchpad = 0;
	while(loop_touchpad < LOOP_MAX) {	
		//printk("loop_touchpad = %d\n", loop_touchpad);

		touchpad_gpio_intr_init();
	
		error = OT_touchpad_init();
	
		if(error != OT_FAILURE) {
			printk("<touchpad>touchpad init success\n");
			//touchpad_write(0x00, 0x01, 0x00, 0x28);
			//touchpad_adjust_sensitivity(); //move to touchpad init, 20081210
			printk("<touchpad> confirm touchpad sensitivity\n");
			touchpad_read(0x00, 0x10); //confirm sensitivity
			touchpad_read(0x00, 0x11); //confirm sensitivity
			touchpad_read(0x00, 0x12); //confirm sensitivity
			touchpad_read(0x00, 0x13); //confirm sensitivity
			break;
		}
		else if(error == OT_FAILURE) {
	                printk("<touchpad>touchpad init failed\n");
       		}
		loop_touchpad++;
	}
  #else
	touchpad_reset(); //20090121, dhsong

#if 0
	printk("<touchpad> confirm touchpad sensitivity\n");
	touchpad_read(0x00, 0x10); //confirm sensitivity
	touchpad_read(0x00, 0x11); //confirm sensitivity
	touchpad_read(0x00, 0x12); //confirm sensitivity
	touchpad_read(0x00, 0x13); //confirm sensitivity
	printk("<touchpad> confirm touchpad sensitivity\n\n");
#endif

  #endif


#endif
	printk("<touchpad> touchpad intr pin status = %d\n", stmp37xx_gpio_get_level( pin_GPIO(0, 8) ) >> 8 );
	//printk("touchpad intr pin status = %d\n\n", (HW_PINCTRL_DIN0_RD() >> 8)&0x00000001 );

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
	int ret;

        struct proc_dir_entry *proc_touchpad_entry;

#ifdef USE_CHRDEV
	ret = register_chrdev( MAJOR_NUMBER, "button_drv", &button_fops);
	if(ret<0)
		printk("[Touchpad drv] unanble to register driver\n");
	else
		printk("[Touchpad drv] register driver successfully\n");
#endif
        proc_touchpad_entry = create_proc_entry("touchpad", S_IWUSR | S_IRUGO, NULL);
        proc_touchpad_entry->read_proc = touchpad_proc_read;
        proc_touchpad_entry->write_proc = touchpad_proc_write;
        proc_touchpad_entry->data = NULL;

	return platform_driver_register(&stmp37xx_touchpad_driver);
}

static void __exit stmp37xx_touchpad_exit(void)
{
#ifdef USE_CHRDEV
	unregister_chrdev( MAJOR_NUMBER, "button_drv");
#endif
	remove_proc_entry("touchpad", NULL);

	platform_driver_unregister(&stmp37xx_touchpad_driver);
}

module_init(stmp37xx_touchpad_init);
module_exit(stmp37xx_touchpad_exit);

MODULE_DESCRIPTION("STMP37XX MELFAS Touchpad Driver");
MODULE_LICENSE("GPL");
