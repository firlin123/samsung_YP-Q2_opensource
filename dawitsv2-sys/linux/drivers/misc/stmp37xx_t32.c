/** 
 * @file        stmp37xx_t32.c
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>

#ifdef CONFIG_PROC_FS
/**
 * @brief         read proc
 */
static int t32_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data) 
{
	int length = 0;
	
	if (off != 0) {
		return 0;
	}

	*start = page;

	length += sprintf(page, "Virtual T32 driver\n");
	length += sprintf(page + length, "  HW_DIGCTL_CTRL: 0x%08x\n", HW_DIGCTL_CTRL.U);
	if(HW_DIGCTL_CTRL.U & 0x40) {
		length += sprintf(page + length, "  USE_SERIAL_JTAG enabled\n");
	} else {
		length += sprintf(page + length, "  USE_SERIAL_JTAG disabled\n");
	}
	
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
	  i = i * 10 + *(s++) - '0';
	return i;
}
/**
 * @brief         write proc
 */
static ssize_t t32_write_proc(struct file * file, const char * buf, unsigned long count, void *data) 
{
	char c0[64];
	char c1[64];
	unsigned int value;
	
	sscanf(buf, "%s %s", c0, c1);

	if (!strcmp(c0, "enable")) {
		printk("Set USE_SERIAL_JTAG field\n");
		HW_DIGCTL_CTRL.U |= 0x44;
		printk("  HW_DIGCTL_CTRL: 0x%08x\n", HW_DIGCTL_CTRL.U);
	} else if (!strcmp(c0, "disable")) {
		printk("Clear USE_SERIAL_JTAG field\n");
		HW_DIGCTL_CTRL.U &= ~0x40;
		printk("  HW_DIGCTL_CTRL: 0x%08x\n", HW_DIGCTL_CTRL.U);
	} else if (!strcmp(c0, "set")) {
		value = atoi(c1);
		HW_DIGCTL_CTRL.U = value;
		printk("Set HW_DIGCTL_CTRL: 0x%08x\n", HW_DIGCTL_CTRL.U);
	}
	return count;
}
#endif
/**
 * @brief         module init function
 */
static int __init t32_init(void) 
{

#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *proc_entry;

	proc_entry = create_proc_entry("t32", S_IWUSR | S_IRUGO, NULL);
	proc_entry->read_proc = t32_read_proc;
	proc_entry->write_proc = t32_write_proc;
	proc_entry->data = NULL;
#endif	
	return 0;
}
/**
 * @brief         module exit function
 */
static void __exit t32_exit(void) 
{
#ifdef CONFIG_PROC_FS
	remove_proc_entry("t32", NULL);
#endif
}

module_init(t32_init);
module_exit(t32_exit);
