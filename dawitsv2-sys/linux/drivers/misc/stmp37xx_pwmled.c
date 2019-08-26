/** 
 * @file        stmp37xx_pwmled.c
 * @brief       Implementation for pwm led control driver
 */
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/fs.h>

#include <asm/uaccess.h>

#include "stmp37xx_pwmled.h"
#include <asm-arm/arch-stmp37xx/stmp37xx_pwmled_ioctl.h>

#define PWMLED_MAJOR 241
#define PWMLED_PERIOD_MAX 0x2EE0 /* 12000 cycles */
#define PWMLED_INACTIVE_MIN 675//150 //100nF //20 10nF   /* 20 cycles */
#define PWMLED_INACTIVE_MAX 775//185 //100nF //27 10nF

#define PWMLED_BRIGHTNESS_MAX 6
#define PWMLED_BRIGHTNESS_MIN 0

#define STMP37XX_PWMLED_CHANNEL 2

static UC led_brightness = 0;
static UC led_onoff;

DECLARE_MUTEX(led_ioctl_mutex);

/* ioctl define */
enum {
	LED_ON = 0,
	LED_OFF,
	LED_SET_LEVEL,
	LED_GET_MAX_LEVEL,
	LED_PIN_HIGH,
	LED_PIN_LOW
};

static const unsigned short cusinative[PWMLED_BRIGHTNESS_MAX + 1] = {
	0, 680, 690, 700, 710, 720, 750 + 150
};

#if 0  /* previous ioctl */
#define PWMLED_MAGIC         0xAA
#define PWMLED_ON            _IO(PWMLED_MAGIC, 0)
#define PWMLED_OFF           _IO(PWMLED_MAGIC, 1)
#define PWMLED_BL_LEVEL      _IOW(PWMLED_MAGIC, 2, UC *)
#endif

/** 
 * @brief       control the level of led brightness
 */
void stmp37xx_pwmled_control(UC level)
{
	int active = 1;
	int inactive = 0;

	led_brightness = level;

	if (level > 0) {
		//inactive = PWMLED_INACTIVE_MIN + (level * 15) + active;
		inactive = cusinative[level] + active;
	} else {
		inactive = active;
	}
	
	BF_CS2n(PWM_ACTIVEn, STMP37XX_PWMLED_CHANNEL, 
		INACTIVE, inactive, 
		ACTIVE, active);
	
	BF_CS5n(PWM_PERIODn, STMP37XX_PWMLED_CHANNEL, 
		MATT, 0,              /* Not use MATT */
		CDIV, 0,
		INACTIVE_STATE, 0x02, /* Low */
		ACTIVE_STATE, 0x03,   /* High */
		PERIOD, PWMLED_PERIOD_MAX);
}
/** 
 * @brief       turn on led
 */
void stmp37xx_pwmled_on(void)
{
	led_onoff = 1;

	/* PWM reset */
	BF_CLR(PWM_CTRL, SFTRST);
	while (HW_PWM_CTRL.B.SFTRST);

	/* Select MUX to PWM2 */
	stmp37xx_gpio_set_dir(BANK2_PIN2, GPIO_DIR_OUT);
	stmp37xx_gpio_set_af(BANK2_PIN2, 0);

	BF_CLR(PWM_CTRL, CLKGATE);
	BF_CLR(CLKCTRL_XTAL, PWM_CLK24M_GATE);

	HW_PWM_CTRL_SET(1 << STMP37XX_PWMLED_CHANNEL);

	stmp37xx_pwmled_control(led_brightness);//PWMLED_BRIGHTNESS_MAX);
}
/** 
 * @brief       turn off led
 */
void stmp37xx_pwmled_off(void)
{
	led_onoff = 0;

	stmp37xx_pwmled_control(PWMLED_BRIGHTNESS_MIN);

	HW_PWM_CTRL_CLR(1 << STMP37XX_PWMLED_CHANNEL);
	mdelay(1);

	 /* Select MUX to gpio */
	stmp37xx_gpio_set_af(BANK2_PIN2, 3);
	stmp37xx_gpio_set_dir(BANK2_PIN2, GPIO_DIR_IN);

	BF_SET(CLKCTRL_XTAL, PWM_CLK24M_GATE);
	BF_SET(PWM_CTRL, CLKGATE);
}
/**
 * @brief         open led device
 */
static int stmp37xx_pwmled_open(struct inode *inode, struct file *file) 
{
	return 0;
}
/**
 * @brief         release led device
 */
static int stmp37xx_pwmled_release(struct inode *inode, struct file *file)
{
	return 0;
}
/**
 * @brief         loctl for led device
 */
static int stmp37xx_pwmled_ioctl(struct inode *inode, struct file *filp, UI cmd, UL arg)
{
	int ret = RETURN_OK;
	int val;
	
	if (down_interruptible(&led_ioctl_mutex)) {
		return -ERESTART;
	}
	switch(cmd) {
	case LED_ON:
		stmp37xx_pwmled_on();
		break;
	case LED_OFF:
		stmp37xx_pwmled_off();
		break;
	case LED_SET_LEVEL:
		if (get_user(val, (UC *) arg)) {
			return -EINVAL;
		}
		if(val > PWMLED_BRIGHTNESS_MAX || val < PWMLED_BRIGHTNESS_MIN) {
			return -1;
		}
		stmp37xx_pwmled_control(val);
		break;
	case LED_GET_MAX_LEVEL:
		ret = PWMLED_BRIGHTNESS_MAX - PWMLED_BRIGHTNESS_MIN;
		break;
	case LED_PIN_HIGH:
		HW_PINCTRL_MUXSEL4_SET(0x30); /* Set gpio */
		HW_PINCTRL_DOE2_SET(1 << STMP37XX_PWMLED_CHANNEL); /* Set out */
		HW_PINCTRL_DOUT2_SET(0x1 << STMP37XX_PWMLED_CHANNEL); /* Set high */
		break;
	case LED_PIN_LOW:
		HW_PINCTRL_MUXSEL4_SET(0x30); /* Set gpio */
		HW_PINCTRL_DOE2_SET(1 << STMP37XX_PWMLED_CHANNEL); /* Set out */
		HW_PINCTRL_DOUT2_CLR(0x1 << STMP37XX_PWMLED_CHANNEL); /* Set low */
		break;
	default:
		break;
	}
	up(&led_ioctl_mutex);

	return ret;
}

static struct file_operations pwmled_fops = {
	.owner		= THIS_MODULE,
	.open		= stmp37xx_pwmled_open,
	.release	= stmp37xx_pwmled_release,
	.ioctl		= stmp37xx_pwmled_ioctl,
};

#ifdef CONFIG_PROC_FS
static SI pwmled_read_proc(char *page, char **start, off_t off, SI count, SI *eof, void *data) 
{
	SI length = 0;
	
	if (off != 0) {
		return 0;
	}

	*start = page;

	length += sprintf(page, "STMP37XX PWM LED driver\n");
	length += sprintf(page + length, "  led   : %d\n", led_onoff);
	length += sprintf(page + length, "  level : %d\n", led_brightness);

	*start = page + off;
	off = 0;
	*eof = 1;

	return length;
}
#ifndef isdigit
#define isdigit(c) (c >= '0' && c <= '9')
#endif

__inline static int atoi(char *s)
{
	int i = 0;
	while (isdigit(*s))
	  i = i*10 + *(s++) - '0';
	return i;
}
static ssize_t pwmled_write_proc(struct file * file, const char * buf, UL count, void *data) 
{
	char c0[16];
	char c1[16]; 
	char c2[16];
	char c3[16];

	sscanf(buf, "%s %s %s %s", c0, c1, c2, c3);

	if (!strcmp(c0, "setbr")) {
		BF_CS2n(PWM_ACTIVEn, 2, INACTIVE, atoi(c2), ACTIVE, atoi(c1));
		BF_CS5n(PWM_PERIODn, 2, MATT, 0, CDIV, 0, INACTIVE_STATE, 0x02, ACTIVE_STATE, 0x03, PERIOD, atoi(c3));
	}
	if (!strcmp(c0, "dimming")) {
		int i;
		
		i = PWMLED_INACTIVE_MAX;
		while(i-- > PWMLED_INACTIVE_MIN) {
			mdelay(50);
			BF_CS2n(PWM_ACTIVEn, 2, INACTIVE, i, ACTIVE, 0);
			BF_CS5n(PWM_PERIODn, 2, MATT, 0, CDIV, 0, INACTIVE_STATE, 0x02, ACTIVE_STATE, 0x03, PERIOD, 12000);
		}
		i = PWMLED_INACTIVE_MIN;
		while(i++ < PWMLED_INACTIVE_MAX) {
			mdelay(50);
			BF_CS2n(PWM_ACTIVEn, 2, INACTIVE, i, ACTIVE, 0);
			BF_CS5n(PWM_PERIODn, 2, MATT, 0, CDIV, 0, INACTIVE_STATE, 0x02, ACTIVE_STATE, 0x03, PERIOD, 12000);
		}
	}
	if(!strcmp(c0, "level")) {
		if(atoi(c1) > PWMLED_BRIGHTNESS_MAX || atoi(c1) < PWMLED_BRIGHTNESS_MIN) {
			printk("%s: out of lange\n", __func__);
		} else {
			stmp37xx_pwmled_control(atoi(c1));
		}
	}
	if (!strcmp(c0, "led")) {
		if(!strcmp(c1, "on")) {
			stmp37xx_pwmled_on();
		} else {
			stmp37xx_pwmled_off();
		}
	}
	if (!strcmp(c0, "high")) {
		HW_PINCTRL_MUXSEL4_SET(0x30); /* Set gpio */
		HW_PINCTRL_DOE2_SET(1 << STMP37XX_PWMLED_CHANNEL); /* Set out */
		HW_PINCTRL_DOUT2_SET(0x1 << STMP37XX_PWMLED_CHANNEL); /* Set high */		
	}
	if (!strcmp(c0, "low")) {
		HW_PINCTRL_MUXSEL4_SET(0x30); /* Set gpio */
		HW_PINCTRL_DOE2_SET(1 << STMP37XX_PWMLED_CHANNEL); /* Set out */
		HW_PINCTRL_DOUT2_CLR(0x1 << STMP37XX_PWMLED_CHANNEL); /* Set low */
	}
	
	return count;
}
#endif
/**
 * @brief         module init
 */
static int __init stmp37xx_pwmled_init(void)
{
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *proc_entry;

	proc_entry = create_proc_entry("led", S_IWUSR | S_IRUGO, NULL);
	proc_entry->read_proc = pwmled_read_proc;
	proc_entry->write_proc = pwmled_write_proc;
	proc_entry->data = NULL;
#endif	
	printk("stmp37xx_pwmled : %s\n", __func__);

	return register_chrdev(PWMLED_MAJOR, "PWMLED", &pwmled_fops);
}
/**
 * @brief         module exit
 */
static void __exit stmp37xx_pwmled_exit(void)
{
#ifdef CONFIG_PROC_FS
	remove_proc_entry("led", NULL);
#endif
	printk("stmp37xx_pwmled : %s\n", __func__);

	unregister_chrdev(PWMLED_MAJOR, "PWMLED");
}

module_init(stmp37xx_pwmled_init);
module_exit(stmp37xx_pwmled_exit);

MODULE_DESCRIPTION("STMP3760 PWM LED controller");
MODULE_AUTHOR("Samsung MP Group");
MODULE_LICENSE("GPL");
