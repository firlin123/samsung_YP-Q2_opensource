/* drivers/rtc/rtc-stmp3xxx.c
 *
 * Copyright (c) 2007 Sigmatel, Inc.
 *         Peter Hartley, <peter.hartley@sigmatel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Sigmatel STMP37xx SoC RTC driver
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
/* By leeth, udpate system date with RTC for preventing the time gap at 20081213 */
#include <linux/time.h>

#include <asm/hardware.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/rtc.h>
#include <asm/arch-stmp37xx/stmp3xxx_rtc_ioctl.h>

/* By leeth, udpate system date with RTC for preventing the time gap at 20081213 */
static struct timer_list update_sys_date_timer;

/* By leeth, udpate system date with RTC for preventing the time gap at 20081213 */
static void update_sys_date(unsigned long data)
{
	uint32_t t;
	unsigned int wait;
	int time_delta;
	struct timespec adjust_tv;

	do {
		wait = HW_RTC_STAT.B.STALE_REGS & 0x80;
		if (wait)
			cpu_relax();
	} while (wait);
	
 	/* set kernel time of day timer */
 	t = HW_RTC_SECONDS_RD();

	/* 2009.01.05: Avoiding RTC error, does this happen? { */
	time_delta = t - xtime.tv_sec;
	if(time_delta >= 2) {
		printk("<RTC> compensate %d sec\n", time_delta - 1);
		t -= (time_delta - 1);
	}
	/* 2009.01.05: Avoiding RTC error, does this happen? } */
	
	adjust_tv.tv_sec = t;
	adjust_tv.tv_nsec = 0;
	do_settimeofday(&adjust_tv);

	/* register timer with 60s expire time */
	mod_timer(&update_sys_date_timer, jiffies + 6000);
}

/* Time read/write */
static int stmp3xxx_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int wait;
	uint32_t t;

	do {
		wait = HW_RTC_STAT.B.STALE_REGS & 0x80;
		if (wait)
			cpu_relax();
	} while (wait);

	t = HW_RTC_SECONDS_RD();
	rtc_time_to_tm(t, rtc_tm);

	return 0;
}

static int stmp3xxx_rtc_settime(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned long t;
	int rc = rtc_tm_to_time(rtc_tm, &t);
	if (rc == 0) {
		unsigned int wait;

		HW_RTC_SECONDS_WR(t);

		/* The datasheet doesn't say which way round the
		 * NEW_REGS/STALE_REGS bitfields go. In fact it's 0x1=P0,
		 * 0x2=P1, .., 0x20=P5, 0x40=ALARM, 0x80=SECONDS,
		 */
		do {
			wait = HW_RTC_STAT.B.NEW_REGS & 0x80;
			if (wait)
				cpu_relax();
		} while (wait);
	}
	return rc;
}

static int stmp3xxx_rtc_open(struct device *dev)
{

	struct rtc_time tm;
	
	HW_RTC_PERSISTENT0_SET(BM_RTC_PERSISTENT0_XTAL32KHZ_PWRUP);
	HW_RTC_PERSISTENT0_SET(BM_RTC_PERSISTENT0_CLOCKSOURCE);
//	printk("HW_RTC_PERSISTENT0=0x%08x\n\n", HW_RTC_PERSISTENT0);

//	Debug Rtn ====> rtc soft reset
//	HW_RTC_CTRL_CLR(BM_RTC_CTRL_SFTRST); // remove the soft reset condition
//	HW_RTC_CTRL_CLR(BM_RTC_CTRL_CLKGATE); // enable clocks within the RTC
//	HW_RTC_CTRL_CLR(BM_RTC_CTRL_ALARM_IRQ); // reset the alarm interrupt by clearing its status bit

	stmp3xxx_rtc_gettime(dev, &tm);
	/* set default rtc value to 1980/01/01/00:00:00 */
	//printk("[DRIVER] tm_year [%d]\n", tm.tm_year);
	if(tm.tm_year < 80)
	{
		tm.tm_year = 80;
		tm.tm_mon = 0;
		tm.tm_mday = 1;
		tm.tm_hour = 0;
		tm.tm_min = 0;
		tm.tm_sec = 0;
		stmp3xxx_rtc_settime(dev, &tm);
	}

//	stmp3xxx_rtc_gettime(dev, &tm);
	
//	printk("[DRIVER] RTC_TIME\t: %02d:%02d:%02d\n"
//			"[DRIVER] RTC_DATE\t: %04d-%02d-%02d\n"
//			"[DRIVER] RTC_EPOCH\t: %04d\n",
//			tm.tm_hour, tm.tm_min, tm.tm_sec,
//			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 1970);
	
	return 0;
}

static void stmp3xxx_rtc_release(struct device *dev)
{
}
static void stmp3xxx_rtc_update_xtime(void)
{
	uint32_t t;
	unsigned int wait;
	struct timespec adjust_tv;

	do {
		wait = HW_RTC_STAT.B.STALE_REGS & 0x80;
		if (wait)
			cpu_relax();
	} while (wait);
	
 	/* set kernel time of day timer */
 	t = HW_RTC_SECONDS_RD();

	adjust_tv.tv_sec = t;
	adjust_tv.tv_nsec = 0;
	do_settimeofday(&adjust_tv);
}
static int stmp3xxx_rtc_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	unsigned int value;

	switch (cmd) {
	
	case STMP3XXX_RTC_GET_LOCK:
		value = HW_RTC_PERSISTENT0.B.LCK_SECS;
		return put_user(value, (unsigned int __user*)arg);

	case STMP3XXX_RTC_LOCK:
		HW_RTC_PERSISTENT0_SET(BM_RTC_PERSISTENT0_LCK_SECS);
		return 0;
	case 34://STMP3XXX_RTC_UPDATE_XTIME:
		stmp3xxx_rtc_update_xtime();
		return 0;		
	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}

static struct rtc_class_ops stmp3xxx_rtc_ops = {
	.open		= stmp3xxx_rtc_open,
	.release	= stmp3xxx_rtc_release,
	.ioctl		= stmp3xxx_rtc_ioctl,
	.read_time	= stmp3xxx_rtc_gettime,
	.set_time	= stmp3xxx_rtc_settime,
};

static int stmp3xxx_rtc_remove(struct platform_device *dev)
{
	struct rtc_device *rtc = platform_get_drvdata(dev);

	if (rtc)
		rtc_device_unregister(rtc);

	return 0;
}

static int stmp3xxx_rtc_probe(struct platform_device *pdev)
{
	uint32_t hwversion;
	struct rtc_device *rtc;

		//HW_RTC_PERSISTENT0_SET(BM_RTC_PERSISTENT0_XTAL32_FREQ);
		/* 32.768kHz 사용하므로 0으로 세팅해야 함 : jinho.lim */
		HW_RTC_PERSISTENT0_CLR(0x1 << 6);//x32_freq
        HW_RTC_PERSISTENT0_CLR(0x1 << 4);//24M power down
        HW_RTC_PERSISTENT0_SET(BM_RTC_PERSISTENT0_XTAL32KHZ_PWRUP);
        HW_RTC_PERSISTENT0_SET(BM_RTC_PERSISTENT0_CLOCKSOURCE);
        printk("HW_RTC_PERSISTENT0=0x%08x\n\n", HW_RTC_PERSISTENT0);

	hwversion = HW_RTC_VERSION_RD();
	hw_rtc_stat_t rtc_stat = HW_RTC_STAT;
	hw_rtc_persistent0_t rtc_per = HW_RTC_PERSISTENT0;

	printk("STMP3xxx RTC driver v1.0 hardware v%u.%u.%u\n",
		   (hwversion >> 24),
		   (hwversion >> 16) & 0xFFu,
		   hwversion & 0xFFFFu);

	printk("RTC status:");
	if (rtc_stat.B.XTAL32768_PRESENT)
		printk(" XTAL32768");
	if (rtc_stat.B.XTAL32000_PRESENT)
		printk(" XTAL32000");
	if (rtc_stat.B.WATCHDOG_PRESENT)
		printk(" WATCHDOG");
	if (rtc_stat.B.ALARM_PRESENT)
		printk(" ALARM");
	if (rtc_stat.B.RTC_PRESENT)
		printk(" RTC");
	printk("\n");

	printk("RTC state:");
	if (rtc_per.B.CLOCKSOURCE)
		printk(" CLOCKSOURCE32K");
	if (rtc_per.B.ALARM_WAKE_EN)
		printk(" ALARM_WAKE_EN");
	if (rtc_per.B.ALARM_EN)
		printk(" ALARM_EN");
	if (rtc_per.B.LCK_SECS)
		printk(" LCK_SECS");
	if (rtc_per.B.XTAL24MHZ_PWRUP)
		printk(" XTAL24MHZ_PWRUP");
	if (rtc_per.B.XTAL32KHZ_PWRUP)
		printk(" XTAL32KHZ_PWRUP");
	if (rtc_per.B.XTAL32_FREQ)
		printk(" XTAL32_FREQ");
	if (rtc_per.B.ALARM_WAKE)
		printk(" ALARM_WAKE");
	printk(" MSEC_RES=%u\n", rtc_per.B.MSEC_RES);

	if (!rtc_stat.B.RTC_PRESENT)
		return -ENODEV;

	rtc = rtc_device_register(pdev->name, &pdev->dev, &stmp3xxx_rtc_ops,
							  THIS_MODULE);

	if (IS_ERR(rtc))
		return PTR_ERR(rtc);

	platform_set_drvdata(pdev, rtc);

	return 0;
}

static struct platform_driver stmp3xxx_rtcdrv = {
	.probe      = stmp3xxx_rtc_probe,
	.remove     = stmp3xxx_rtc_remove,
	.driver     = {
		.name   = "stmp3xxx-rtc",
		.owner  = THIS_MODULE,
	},
};

static struct platform_device stmp3xxx_rtcdev = {
	.name   = "stmp3xxx-rtc",
	.id     = 252,
};


static int rtc_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *p = page;
	int len;
	struct rtc_time tm;

	stmp3xxx_rtc_gettime(0, &tm);
	p += sprintf(p, "rtc_time\t: %02d:%02d:%02d\n"
			"rtc_date\t: %04d-%02d-%02d\n"
			"rtc_epoch\t: %04d\n",
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 1970);
	
	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

static int __init stmp3xxx_rtc_init(void)
{
	struct proc_dir_entry *entry;

	int rc = platform_driver_register(&stmp3xxx_rtcdrv);
	if (!rc) {
		rc = platform_device_register(&stmp3xxx_rtcdev);
		if (rc)
			platform_driver_unregister(&stmp3xxx_rtcdrv);
	}

	/* register proc entry */
	entry = create_proc_entry ("rtc", S_IWUSR | S_IRUGO, NULL);
	if (entry) {
		entry->read_proc = rtc_read_proc;
		entry->data = NULL; 
	}

	/* By leeth, udpate system date with RTC for preventing the time gap at 20081213 */
	setup_timer(&update_sys_date_timer, update_sys_date, 0);
	/* register timer with 60s expire time */
	mod_timer(&update_sys_date_timer, jiffies + 6000);

	return rc;
}

static void __exit stmp3xxx_rtc_exit(void)
{

	remove_proc_entry("driver/rtc", NULL);

	platform_driver_unregister(&stmp3xxx_rtcdrv);
}

module_init(stmp3xxx_rtc_init);
module_exit(stmp3xxx_rtc_exit);

MODULE_DESCRIPTION("Sigmatel STMP3xxx RTC Driver");
MODULE_AUTHOR("Peter Hartley <peter.hartley@sigmatel.com>");
MODULE_LICENSE("GPL");
