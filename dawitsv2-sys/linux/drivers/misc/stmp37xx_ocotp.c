/*
 * linux/drivers/misc/stmp37xx_ocotp.c
 * proc interfaces for OCOTP
 *
 * Copyright (C) 2008 MIZI Research, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>

#include <asm/hardware.h>
#include <asm/arch/digctl.h>

static DECLARE_WAIT_QUEUE_HEAD(wait);

/*
 * Samsung UI Interface
 */
#define PROC_DIR	"digctl"
#define PROC_ENTRY1	"uniqueid"
#define PROC_ENTRY2	"swversion"

#define S_VER "VER"
#define S_MODEL "MODEL"

struct semaphore swversion_sem;

struct proc_dir_entry *ocotp_proc_dir;
struct proc_dir_entry *ocotp_proc_entry1;
struct proc_dir_entry *ocotp_proc_entry2;

#define UNIQUEID_LEN	4
static int samsung_ui_interface_uniqueid_i[UNIQUEID_LEN] =
	{
		0x76543210,
		0xfedcba98,
		0x89abcdef,
		0x01234567,
	};

static char samsung_ui_interface_uniqueid_s[UNIQUEID_LEN * 8 + 1];
static serialnumber_t samsung_unique_id;
static version_inf_t samsung_sw_version = {"", "", ""};

static inline int convert_hex_to_ascii(char *c, int h, int u)
{
	if (u)
		return sprintf(c, "%X", h & 0xf);
	else
		return sprintf(c, "%x", h & 0xf);
}

static inline int convert_hex_to_string(char *s, int h, int l, int u)
{
	char *p = &s[0];
	int shift = l * 2;
	int i, size;

	for (i = 0; i < l * 2; i++) {
		size = convert_hex_to_ascii(p, h >> (4 * --shift), u);
		p += size;
	}

	return (int)(p - s);
}

static char *convert_uniqueid_to_string(char *str, int *uniq, int size)
{
	char *s = &str[0];
	int *u = uniq;
	int l = size;
	int i, t;

	for (i = 0; i < l; i++) {
		t = convert_hex_to_string(s, u[l-i-1], l, 1);
		s += t;
	}
	*s = '\0';

	return str;
}

#define __in_ocotp_read()							\
	do {									\
		/* 0. program HCLK < 200 MHz */					\
		/* 1. check if error, busy bits are clear */			\
		HW_OCOTP_CTRL_CLR(BM_OCOTP_CTRL_ERROR | BM_OCOTP_CTRL_BUSY);	\
		/* 2. set HW_OCOTP_CTRL_RD_BANK_OPEN */				\
		HW_OCOTP_CTRL_SET(BM_OCOTP_CTRL_RD_BANK_OPEN);			\
		/* 3. poll HW_OCOTP_CTRL_BUSY clear */				\
		wait_event_interruptible(wait,					\
			 !(HW_OCOTP_CTRL_RD() & BM_OCOTP_CTRL_BUSY));		\
	} while (0)

#define __out_ocotp_read()							\
	/* 5. clear HW_OCOTP_CTRL_RD_BANK_OPEN */				\
	HW_OCOTP_CTRL_CLR(BM_OCOTP_CTRL_RD_BANK_OPEN)

#define BADA_BADA 0xbadabada
#define __check_error(_v) 							\
	(((_v & 0xffffffff) == BADA_BADA) ||					\
	 (HW_OCOTP_CTRL_RD() & BM_OCOTP_CTRL_ERROR))

int ocotp_read_cust(unsigned int *value)
{
	int f = 0x00, t = 0x04, s = t - f;
	int i;

	__in_ocotp_read();

	for (i = 0; i < s; i++) {
		value[i] = HW_OCOTP_CUSTn_RD(i % HW_OCOTP_CUSTn_COUNT);
		if (__check_error(value[i]))
			break;
	}

	__out_ocotp_read();
	
	if (i != s)
		return -EIO;

	return 0;
}

int ocotp_read_crypto(unsigned int *value)
{
	int f = 0x4, t = 0x8, s = t - f;
	int i;

	__in_ocotp_read();

	for (i = 0; i < s; i++) {
		value[i] = HW_OCOTP_CRYPTOn_RD(i % HW_OCOTP_CRYPTOn_COUNT);
		if (__check_error(value[i]))
			break;
	}

	__out_ocotp_read();

	if (i != s)
		return -EIO;

	return 0;
}

int ocotp_read_rom(unsigned int *value)
{
	int f = 0x18, t = 0x1b, s = t - f;
	int i;

	__in_ocotp_read();

	for (i = 0; i < s; i++) {
		value[i] = HW_OCOTP_ROMn_RD(i % HW_OCOTP_ROMn_COUNT);
		if (__check_error(value[i]))
			break;
	}

	__out_ocotp_read();

	if (i != s)
		return -EIO;

	return 0;
}

int ocotp_efuse_open_banks(void)
{
	uint32_t  tmp;
	bool done = 0;

	tmp = HW_DIGCTL_MICROSECONDS_RD();

	while (!done)
	{
		if (HW_DIGCTL_MICROSECONDS_RD() > (tmp + 100))
		//if (hw_digctl_CheckTimeOut(tmp, 100) == 1)
			return -1;
		// Check HW_OCOTP_CTRL_BUSY
		if (!(HW_OCOTP_CTRL_RD() & BM_OCOTP_CTRL_BUSY))
			done = 1;
	}
	// Set HW_OCOTP_CTRL_RD_BANK_OPEN. This will kick the controller to put
	// the fuses into read mode. The controller will set HW_OCOTP_CTRL_BUSY
	// until the OTP contents are readable. Note that if there was a pending write
	// (holding HW_OCOTP_CTRL_BUSY) and
	// HW_OCOTP_CTRL_RD_BANK_OPEN was set, the controller would complete
	// the write and immediately move into read operation (keeping
	// HW_OCOTP_CTRL_BUSY set while the banks are being opened).
	HW_OCOTP_CTRL_SET(BM_OCOTP_CTRL_RD_BANK_OPEN);

	// Poll for HW_OCOTP_CTRL_BUSY clear. When HW_OCOTP_CTRL_BUSY is
	// clear and HW_OCOTP_CTRL_RD_BANK_OPEN is set, read the data from the
	// appropriate memory mapped address. Note that this is not necessary for registers
	// which are shadowed. Reading before HW_OCOTP_CTRL[BUSY] is
	// cleared by the controller, will return 0xBADA_BADA and will result in the setting
	// of HW_OCOTP_CTRL[ERROR]. Because opening banks takes approximately
	// 33 HCLK cycles, immediate polling for BUSY is not recommended.

	// Allow a smidge of time for banks to open BEFORE checking the BUSY bit.
	done = 0;

	tmp = HW_DIGCTL_MICROSECONDS_RD();
	while (!done)
	{
		if (HW_DIGCTL_MICROSECONDS_RD() > (tmp + 1))
		//if (hw_digctl_CheckTimeOut(tmp, 1) == 1)
			done = 1;
	}

	// poll for banks to become ready for reading..
	done = 0;

	tmp = HW_DIGCTL_MICROSECONDS_RD();
	while (!done)
	{
		if (HW_DIGCTL_MICROSECONDS_RD() > (tmp + 100))
		//if (hw_digctl_CheckTimeOut(tmp, 100) == 1)
		{
			HW_OCOTP_CTRL_CLR(BM_OCOTP_CTRL_RD_BANK_OPEN);
			return -1;
		}
		if (!(HW_OCOTP_CTRL_RD() & BM_OCOTP_CTRL_BUSY))
			done = 1;
	}

	return 0;
}

void ocotp_efuse_close_banks(void)
{
	HW_OCOTP_CTRL_CLR(BM_OCOTP_CTRL_RD_BANK_OPEN); 
}

void ocotp_getchipserialnumber(serialnumber_t *serno)
{
	const char* lookup = "0123456789ABCDEF";
	int32_t i, nullserial ;

	union {
		uint32_t word[4];
		char     byte[16];
	} uniqueid ;

	// must power-up fuse array to read OPS bits
	if (ocotp_efuse_open_banks() != 0)
		return;

	nullserial = !(HW_OCOTP_OPSn_RD(3) | HW_OCOTP_OPSn_RD(2));

	// If Serial Number is NULL, fake a serial number.
	if (nullserial)
	{
		ocotp_efuse_close_banks();  // power down fuse array

		//
		// FORCE A NON-ZERO SERIAL NUMBER
		//

		serno->rawsizeinbytes = SERIAL_NUMBER_MAX_RAW_BYTES;
		serno->asciisizeinchars = SERIAL_NUMBER_MAX_ASCII_CHARS;

		memcpy(serno->raw, "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15", 
			SERIAL_NUMBER_MAX_RAW_BYTES);
		memcpy(serno->ascii, "00010203040506070809101112131415", 
			SERIAL_NUMBER_MAX_ASCII_CHARS + 1);

		return;
	}

	// See "STMP37xx - TA1  One Time Programmable (OTP) Control Bits" Document
	serno->rawsizeinbytes = SERIAL_NUMBER_MAX_RAW_BYTES;
	serno->asciisizeinchars = SERIAL_NUMBER_MAX_ASCII_CHARS;

	uniqueid.word[3] = HW_OCOTP_OPSn_RD(0);
	uniqueid.word[2] = HW_OCOTP_OPSn_RD(1);
	uniqueid.word[1] = HW_OCOTP_OPSn_RD(2);
	uniqueid.word[0] = HW_OCOTP_OPSn_RD(3);

	ocotp_efuse_close_banks(); // power-down fuse array

	for (i=serno->rawsizeinbytes-1; i >=0; i--)
	{
		serno->raw[i] = uniqueid.byte[i];

		// convert to ascii (little endian)
		serno->ascii[serno->asciisizeinchars-((2*i)+2)] = lookup[uniqueid.byte[i] >> 4];
		serno->ascii[serno->asciisizeinchars-((2*i)+1)] = lookup[uniqueid.byte[i] & 0xF];
		//printf("serno->ascii[%d]=%c\n", 32-((2*i)+2), serno->ascii[32-((2*i)+2)]);
		//printf("serno->ascii[%d]=%c\n", 32-((2*i)+1), serno->ascii[32-((2*i)+1)]);
	}

	// Finally, NULL terminate the string
	serno->ascii[32] = '\0';
}

void ocotp_getinfofromproc(const char *source, unsigned long len)
{
	const char ver[] = S_VER;
	const char model[] = S_MODEL;
	char dummy[5] = "";
	int i, vercount, modcount;

	vercount = modcount = 0;
	for(i = 0; i < len; i++)
	{
		if(source[i] == ver[vercount])
		{
			vercount++;
			if(vercount == strlen(ver))
			{
				sscanf(source, "%s %s %s", dummy, 
					samsung_sw_version.version, samsung_sw_version.nation);
				break;
			}
			
		}
		else
		{
			vercount = 0;
		}
		if(source[i] == model[modcount])
		{
			modcount++;
			if(modcount == strlen(model))
			{
				sscanf(source, "%s %s", dummy, 
					samsung_sw_version.model);
				break;
			}
		}
		else
		{
			modcount = 0;
		}
	}
}

int get_sw_version(version_inf_t *ver)
{
	strcpy(ver->version, samsung_sw_version.version);
	strcpy(ver->nation, samsung_sw_version.nation);
	strcpy(ver->model, samsung_sw_version.model);
	
	return 0;
}
EXPORT_SYMBOL(get_sw_version);

int get_hw_option_type(void)
{
        unsigned int rotarya = 0x0;
        unsigned int rotaryb = 0x0;
 
        stmp37xx_gpio_set_af( pin_GPIO(2, 8), 3); //gpio func
        stmp37xx_gpio_set_dir( pin_GPIO(2, 8), GPIO_DIR_IN); //data input
        rotaryb = ( stmp37xx_gpio_get_level( pin_GPIO(2, 8) ) >> 8 );   
 
        stmp37xx_gpio_set_af( pin_GPIO(2, 7), 3); //gpio func
        stmp37xx_gpio_set_dir( pin_GPIO(2, 7), GPIO_DIR_IN); //data input
        rotarya = ( stmp37xx_gpio_get_level( pin_GPIO(2, 7) ) >> 7 );   
 
        if(rotaryb == 0x0 && rotarya == 0x0)
                return 1;
 
        else if(rotaryb == 0x0 && rotarya == 0x1)
                return 2;
 
        else if(rotaryb == 0x1 && rotarya == 0x0)
                return 3;
 
        else if(rotaryb == 0x1 && rotarya == 0x1)
                return 4;

	return 5;
}
EXPORT_SYMBOL(get_hw_option_type);

int get_chip_serial_number(serialnumber_t *psn)
{
	/*
	if(convert_uniqueid_to_string(psn, samsung_ui_interface_uniqueid_i, 4))
	{
		return 0;
	}
	else
	{
		return -1;
	}
	*/

	memcpy(psn, &samsung_unique_id, sizeof(serialnumber_t));

	return 0;
}
EXPORT_SYMBOL(get_chip_serial_number);

static int ocotp_proc_read(char *page, char **start, off_t off, int count,
			     int *eof, void *data)
{
	int len = 0;

	/*
	int value[4];
	int ret;

	ret = ocotp_read_cust(&value[0]);
	//ret = ocotp_read_crypto(&value[0]);
	//ret = ocotp_read_custcap(value);
	//ret = ocotp_read_rom(&value[0]);
	if (ret < 0)
		return ret;

	len = sprintf(page, "%s\n", convert_uniqueid_to_string
		      (samsung_ui_interface_uniqueid_s,
		       samsung_ui_interface_uniqueid_i,
		       4));
	*/
	len = sprintf(page, "%s\n", samsung_unique_id.ascii);

	*eof = true;

	return len;
}

static int swversion_proc_read(char *page, char **start, off_t off, int count,
			     int *eof, void *data)
{
	int len = 0;

	*start = page; 
	
	if (down_interruptible(&swversion_sem))
		return -ERESTARTSYS;

	if (off == 0)
	{
		len += sprintf(page + len, "%s %s %s\n", samsung_sw_version.model, 
			samsung_sw_version.version, samsung_sw_version.nation);
		
		*start = page + off;
		off = 0;
	}

	up(&swversion_sem);

	*eof = true;

	return len;
}

static int swversion_proc_write(struct file *fid, const char *page, int count,
			     int *eof, void *data)
{
	int i = 0, j = 0;

	if (down_interruptible(&swversion_sem))
		return -ERESTARTSYS;

	for (i = 0; i < count; i++)
	{
		if(page[i] == '\n')
		{
			ocotp_getinfofromproc((page + j), (i - j));
			j = i + 1;
		}
	}

	up(&swversion_sem);

	return count;
}


static int __init stmp37xx_ocotp_init(void)
{
	int ret = 0;

	sema_init(&swversion_sem, 1);

	ocotp_proc_dir = proc_mkdir(PROC_DIR, NULL);
	if (!ocotp_proc_dir) {
		printk("%s(): failed to proc_mkdir()\n", __func__);
		ret = -ENOMEM;
		goto err1_init;
	}

	ocotp_proc_entry1 = create_proc_read_entry(PROC_ENTRY1, 0, ocotp_proc_dir,
					     ocotp_proc_read, NULL);
	if (!ocotp_proc_entry1) {
		printk("%s(): failed to create_proc_read_entry1()\n", __func__);
		ret = -ENOMEM;
		goto err2_init;
	}

	ocotp_proc_entry2 = create_proc_entry(PROC_ENTRY2, S_IWUSR | S_IRUGO, ocotp_proc_dir);
	if (!ocotp_proc_entry2) {
		printk("%s(): failed to create_proc_read_entry2()\n", __func__);
		ret = -ENOMEM;
		goto err3_init;
	}
	ocotp_proc_entry2->read_proc = swversion_proc_read;
	ocotp_proc_entry2->write_proc = swversion_proc_write;
	ocotp_proc_entry2->data = NULL; 	

	ocotp_getchipserialnumber(&samsung_unique_id);


	return 0;

 err3_init:
	remove_proc_entry(PROC_ENTRY1, ocotp_proc_dir);
 err2_init:
	remove_proc_entry(PROC_DIR, NULL);
 err1_init:
	return ret;
}

static void __exit stmp37xx_ocotp_exit(void)
{
	remove_proc_entry(PROC_ENTRY2, ocotp_proc_dir);
	remove_proc_entry(PROC_ENTRY1, ocotp_proc_dir);
	remove_proc_entry(PROC_DIR, NULL);
}

module_init(stmp37xx_ocotp_init);
module_exit(stmp37xx_ocotp_exit);

MODULE_DESCRIPTION("proc interfaces for ocotp");
MODULE_AUTHOR("Ryu Ho-Eun <rhe201@mizi.com>");
MODULE_LICENSE("GPL");
