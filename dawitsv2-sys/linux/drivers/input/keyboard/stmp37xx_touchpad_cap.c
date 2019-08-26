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
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/arch/irqs.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/arch/digctl.h>
#include <asm/arch/stmp37xx_pm.h>
#include <asm/arch/hardware.h>
#include <asm/arch/usb_common.h>
#include <asm/arch/37xx/regs.h>
#include <asm/arch/37xx/regsdigctl.h>
#include <asm/arch/37xx/regslradc.h>
#include <asm/arch/37xx/regspinctrl.h>
#include "mepLib.h"
#include "mep_config.h"

#define DRIVER_NAME		"stmp37xx_touchpad"
#define BUTTON_MAX_NUM		9
#define TOUCHPAD_MAJOR		253
#define POWER_BUTTON_CHECK_TIME         30 //200 //100=100ms
#define TOUCHPAD_CHECK_TIME_STEP	30 //30=30msec	

#define MEP_GET_DATA1(packetPtr)                ((packetPtr)->rawPkt[1])
#define MEP_GET_PKTYPE(packetPtr)               ((MEP_GET_DATA1(packetPtr)>>4) & 0x0F)
#define MEP_NATIVE_PACKET               0x1
#define MEP_GENERAL_PACKET              0x3
#define MEP_PKTYPE_ERROR                0x0
#define MEP_PKTYPE_HELLO                0x1
#define MEP_PKTYPE_ACK                  0x2

static struct timer_list touchpad_timer;
static struct timer_list power_button_timer;
static unsigned int input_key = 0;
static unsigned int input_prev_key = 0;
static unsigned int input_first_key = 0;
static unsigned int pswitch_val=0;
static unsigned int pswitch_prev_val = 0;
static unsigned char scancode;
static unsigned int idle_status = 0;
static unsigned int wakeup_status = 0;
static unsigned int key_buffer[5];
static unsigned int key_count = 0;
static int init = 0;
mep_packet_t txBuf, rxBuf;

static void pm_touchpad_callback(ss_pm_request_t event);

static unsigned char button_scancodes[BUTTON_MAX_NUM] = { 
        KEY_BACKSPACE,  KEY_UP,         KEY_ESC,  KEY_LEFT, 
        KEY_ENTER,      KEY_RIGHT,      KEY_DOWN,
	KEY_F11,	KEY_F12 //dhsong
};

static unsigned int button_values[BUTTON_MAX_NUM] = { 
        1, 2, 4, 8, 16, 32, 64, 300, 350 
};


static inline char *_get_button_name(int index)
{
        char *button_names[BUTTON_MAX_NUM] = {
                "BACK", "VOL+", "MENU", "REW", "ENTER", "FF", "VOL-", "POWER", "USER"
        };
        return button_names[index];
};

static void tx_buf_init(void)
{
	static int i;

        for( i=0; i<8; i++ )
	        txBuf.rawPkt[i] = 0;
	
}

static void rx_buf_init(void)
{
	static int i;

        for( i=0; i<8; i++ )
	        rxBuf.rawPkt[i] = 0;
	
}

static unsigned int analysis_response(mep_packet_t* packet)
{
	switch ( MEP_GET_CTRL(packet) )
	{
		case MEP_GENERAL_PACKET:
			if (MEP_GET_PKTYPE(packet) == MEP_PKTYPE_HELLO) {
				printk("\t***HELLO***\n");
				return MEP_PKTYPE_HELLO;
			}
			else if (MEP_GET_PKTYPE(packet) == MEP_PKTYPE_ERROR) {
				printk("\t!!!ERROR!!!\n");
				return MEP_PKTYPE_ERROR;
			}
			else if (MEP_GET_PKTYPE(packet) == MEP_PKTYPE_ACK) {
				printk("\t----ACK----\n");
				return MEP_PKTYPE_ACK;
			}
			else
				printk("\t++++ Mystery response %x received.\n", MEP_GET_PKTYPE(packet));
			break;

		case MEP_NATIVE_PACKET:	
			//printk("\tNative Packet...\n");
			break;

		default:
			//printk("\t????Unrecognized response type.\n");
			break;
	}
}

static void set_touchpad_sleep(void)
{
	tx_buf_init();

	txBuf.rawPkt[0] = 0x0A;     /* Enter Sleep Mode */
	if ( mep_tx(&txBuf) != MEP_NOERR) {
		printk("[Touchpad] sleep\n");
	}
}

static void set_touchpad_active(void)
{
	tx_buf_init();

	txBuf.rawPkt[0] = 0x09;     /* Enter Sleep Mode */
	if ( mep_tx(&txBuf) != MEP_NOERR) {
		printk("[Touchpad] active\n");
	}
}

static void tx_and_rx_check(void)
{
	int count = 1;
	mep_err_t err;

	rx_buf_init();

	if(mep_tx(&txBuf) != MEP_NOERR)
		printk("packet transmit fail!\n");


	while( ((err = mep_rx(&rxBuf)) != MEP_NOERR) )
	{
		printk("\tnot yet received!\n");
		analysis_response(&rxBuf);
		if(count == 5)
			break;
		count ++;
	}
	if ( (err = mep_rx(&rxBuf)) != MEP_NOERR )
		printk("packet receive fail \n");

	/* Check response packet */
	analysis_response(&rxBuf);
}

static void send_reset_receive_hello(void)
{
	printk("\t{send_reset_receive_hello}\n");

	/* "Reset" command ---> "Hello" packet should be returned */
	MEP_SET_ADDR(&txBuf, 0x00);                 /* 0x08 = 0000 1000 */
	MEP_SET_CTRL(&txBuf, 0x1);                  /* non-global(0), short command(1) */
	MEP_SET_SHORTCMD(&txBuf, 0x0);              /* reset cmd(000) */

	/* Transmit command and Receive reponse and check it */
	tx_and_rx_check();
}

static void send_report_receive_hello(void)
{

	printk("\t{send_report_receive_hello}\n");

	/* Set MEP parameter 0x20 : Report Mode */
	//txBuf.rawPkt[0] = 0x13;   /* global command, 3 bytes after this one */
	txBuf.rawPkt[0] = 0x03;             /* non-global command, 3 bytes after this one */
	txBuf.rawPkt[1] = 0x60;             /* parameter number $20 + 0x40 */

	//txBuf.rawPkt[2] = 0x00;   /* Position Sensor Sensitivity(-8 ~ +7) = 0 */
	txBuf.rawPkt[2] = 0x07;           /* Position Sensor Sensitivity(-8 ~ +7) = +7 */
	//txBuf.rawPkt[2] = 0x08;             /* Position Sensor Sensitivity(-8 ~ +7) = -8 */

	txBuf.rawPkt[3] = 0x91;             /* 80 reports/sec, absolute packets */
	//txBuf.rawPkt[3] = 0x81;             /* 80 reports/sec, absolute packets */
	//txBuf.rawPkt[3] = 0x89;           /* 80 reports/sec, enable scroll, absolute packets */
	txBuf.rawPkt[4] = 0x00;
	txBuf.rawPkt[5] = 0x00;
	txBuf.rawPkt[6] = 0x00;
	txBuf.rawPkt[7] = 0x00;

	/* Transmit command and Receive reponse and check it */
	tx_and_rx_check();
}

static void ResponseMepPkt( void )
{
        static int i;
        int temp;
#if 0 
        for( i=0; i<8; i++)
	        rxBuf.rawPkt[i] = 0;
#endif
        temp = mep_rx(&rxBuf);
#if 0 
        for( i=0; i<8; i++) //add dhsong
	{
		printk("rxBuf.rawPkt[%d] = 0x%08x\n\n", i, rxBuf.rawPkt[i] );
		if(i==7) printk("\n\n");
	}
#endif
}

static void set_touchpad_init(void)
{

        /* Buffer Init */
        tx_buf_init();
        rx_buf_init();

        /* Send RESET pkt and check if receiving HELLO pkt */
        send_reset_receive_hello();

        /* Set MEP parameter $20 : Report Mode */
        send_report_receive_hello();
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
        if (pswitch_val && key_count<3) {
                key_buffer[key_count] = pswitch_val;
                //printk("key_buffer[%d] = %d\n", key_count, key_buffer[key_count]);
                key_count++;
        }
        else key_count = 0;
  #if 1 
        if(key_count==3){
                if( key_buffer[0]==3 || key_buffer[1]==3 || key_buffer[2]==3 ){
                        pswitch_val = 3; //USER_KEY;
                        i = 8;
                        pw_scancode = button_scancodes[i];
                        if ( pswitch_prev_val != pswitch_val) {
                                input_report_key(input_dev, pw_scancode, 1);
                                printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
                        }
                        //printk("[%s] press %d\n", _get_button_name(i), pw_scancode);
                }
                else {
                        pswitch_val = 1; //POWER_KEY;
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

static void button_handler(unsigned long arg)
{
        struct platform_device *pdev = arg;
	struct input_dev *input_dev = platform_get_drvdata(pdev);
        static int new_pressed = 0;
        static int old_pressed = 0;
	static int i = 0;

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
        mep_pl_init();
        ResponseMepPkt();
	init = rxBuf.rawPkt[0];

	input_key = rxBuf.rawPkt[1];

        if(!input_first_key)
                input_first_key = input_key;

        if ( (init) && (input_first_key) && (input_key) ) { 
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
		        printk("[%s] press %d\n", _get_button_name(i), scancode);
		}
		input_prev_key = input_first_key;
	}

        if ( (init) && (input_first_key) && (!input_key) ) { 
		for (i = 0; i < BUTTON_MAX_NUM; i++) {
	                if (input_first_key == button_values[i])
	                        break;
                }
	        scancode = button_scancodes[i];
	        //printk("input_dev->name = %s\n\n",input_dev->name,  __FILE__, __LINE__);
	        //printk("init = %d\n", init);
	        //printk("input_key = %d\n", input_key);
	        printk("[%s] release %d\n", _get_button_name(i), scancode);
	 	input_report_key(input_dev, scancode, 0); 
		input_first_key = 0;
		input_prev_key = 0;
	}

        touchpad_timer.expires = get_jiffies_64() + ( TOUCHPAD_CHECK_TIME_STEP*HZ/1000); 
        add_timer(&touchpad_timer);
}

static void pm_touchpad_callback(ss_pm_request_t event)
{
	switch(event) {
		case SS_PM_IDLE:
			//printk("[Touchpad] SS_PM_IDLE \n\n");
			idle_status = 1;
			set_touchpad_sleep();
	        	del_timer(&touchpad_timer);
			break;
		case SS_PM_SET_WAKEUP:
			//printk("[Touchpad] SS_PM_SET_WAKEUP \n\n");
			set_touchpad_active();
			idle_status = 0;
                        wakeup_status = 1;
                        power_button_timer.expires = get_jiffies_64() + (POWER_BUTTON_CHECK_TIME*HZ/1000);
                        add_timer(&power_button_timer);

		        touchpad_timer.expires = get_jiffies_64() + (TOUCHPAD_CHECK_TIME_STEP*HZ/1000); 
		        add_timer(&touchpad_timer);
			break;
		case SS_PM_LCD_ON:
			//printk("[Touchpad] SS_PM_LCD_ON \n\n");
			break;
		case SS_PM_LCD_OFF:
			//printk("[Touchpad] SS_PM_LCD_OFF \n\n");
			break;
	}
}

static int stmp37xx_touchpad_open(struct input_dev *dev)
{
	printk("[Touchpad] Synaptics touchpad driver opened\n");
	return 0;
}

static void stmp37xx_touchpad_close(struct input_dev *dev)
{
	printk("[Touchpad] Synaptics touchpad driver closed\n");
}

#ifdef CONFIG_PM
#else
#define stmp37xx_touchpad_suspend	NULL
#define stmp37xx_touchpad_resume	NULL
#endif

static int __devinit stmp37xx_touchpad_probe(struct platform_device *pdev)
{
	struct input_dev *input_dev;
	int i, error;
	int code;

	printk("[Touchpad] probe func, %d\n", __LINE__);
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
	error = input_register_device(input_dev); //open func called
	if (error) {
		printk("input register ERROR, %s, %d\n\n", __FILE__, __LINE__);
		goto err_free_dev;
	}
	printk("[Touchpad] probe func, %d\n", __LINE__);
#if 0
	error = register_chrdev(TOUCHPAD_MAJOR, DRIVER_NAME, &stmp37xx_touchpad_fops);
        if (retval < 0) {
		printk("stmp37xx touchpad device register fail\n");
                return retval;
        }
#endif

        init_timer(&power_button_timer);
        power_button_timer.expires = get_jiffies_64() + (POWER_BUTTON_CHECK_TIME*HZ/1000);
        power_button_timer.function = power_button_handler;
        power_button_timer.data = pdev; //input_dev;
        add_timer(&power_button_timer);	

        mep_init();
	//rx_buf_init();
	set_touchpad_init();

        init_timer(&touchpad_timer);
        touchpad_timer.expires = get_jiffies_64() + (TOUCHPAD_CHECK_TIME_STEP*HZ/1000); 
        touchpad_timer.function = button_handler;
        touchpad_timer.data = pdev; 
        add_timer(&touchpad_timer);

	ss_pm_register(SS_PM_TOUCHPAD_DEV, pm_touchpad_callback);
	return 0;

 err_free_dev:
	input_free_device(input_dev);
 err_alloc:
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
	return platform_driver_register(&stmp37xx_touchpad_driver);
}

static void __exit stmp37xx_touchpad_exit(void)
{
	platform_driver_unregister(&stmp37xx_touchpad_driver);
}

module_init(stmp37xx_touchpad_init);
module_exit(stmp37xx_touchpad_exit);

MODULE_DESCRIPTION("STMP37XX Synaptics Keypad Driver");
MODULE_LICENSE("GPL");
