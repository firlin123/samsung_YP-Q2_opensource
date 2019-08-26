/* 
 * pinctrl.c PINCTRL set/view for STMP3600 
 *
 * Copyright (C) 2005 Samsung Electronics Inc. 
 *                    by Heechul Yun <heechul.yun@samsung.com> 
 *
 */
#include <linux/config.h>

#include <linux/kernel.h> 
#include <linux/module.h>

#include <linux/init.h> 

#include <linux/fs.h>
#include <linux/proc_fs.h>

//#include <asm/arch/clocks.h>
#include <asm/arch/clk.h> //dhsong

#include <asm/arch/pinctrl.h>
 
//#include <asm/arch/regs/regspinctrl.h>
#include <asm/arch/37xx/regspinctrl.h> //dhsong



struct semaphore pin_sem; 

char *pin_desc[4][32][4] = { /* bank, pin, func */ 
	{ /* bank 0 */ 
		/* MUX 0 */ 
		{ "GPMI_D0", "N/A", "N/A", "GPIO" }, // 0 
		{ "GPMI_D1", "N/A", "SSP2_D1", "GPIO" }, 
		{ "GPMI_D2", "N/A", "SSP2_D2", "GPIO" }, 
		{ "GPMI_D3", "N/A", "SSP2_D3", "GPIO" }, 
		{ "GPMI_D4", "N/A", "SSP2_D4", "GPIO" }, 
		{ "GPMI_D5", "N/A", "SSP2_D5", "GPIO" }, 
		{ "GPMI_D6", "N/A", "SSP2_D6", "GPIO" }, 
		{ "GPMI_D7", "N/A", "SSP2_D7", "GPIO" }, 
		{ "GPMI_D8", "EMI_A15", "N/A", "GPIO" }, // 8 
		{ "GPMI_D9", "EMI_A16", "N/A", "GPIO" }, 
		{ "GPMI_D10", "EMI_A17", "N/A", "GPIO" }, 
		{ "GPMI_D11", "EMI_A18", "N/A", "GPIO" }, 
		{ "GPMI_D12", "EMI_A19", "GPMI_CE0n", "GPIO" }, 
		{ "GPMI_D13", "EMI_A20", "GPMI_CE1n", "GPIO" }, 
		{ "GPMI_D14", "EMI_A21", "GPMI_CE2n", "GPIO" }, 
		{ "GPMI_D15", "EMI_A22", "GPMI_CE3n", "GPIO" }, 
		/* Mux 1 */ 
		{ "GPMI_A0", "EMI_A23", "N/A", "GPIO" }, //16
		{ "GPMI_A1", "EMI_A24", "N/A", "GPIO" },
		{ "GPMI_A2", "EMI_A25", "IR_DOUT", "GPIO" }, 
		{ "GPMI_RB0", "N/A", "SSP2_DET", "GPIO" }, 	
		{ "GPMI_RB2", "UART2_RX", "SSP2_CMD", "GPIO" }, 	
		{ "GPMI_RB3", "EMI_OEN", "IR_DIN", "GPIO" },
		{ "GPMI_RSTN", "EMI_RSTN", "JTAG_TRST_N", "GPIO" },
		{ "GPMI_IRQ", "UART2_TX", "SSP2_SCK", "GPIO" },   
		{ "GPMI_WRn", "N/A", "N/A", "GPIO" },//24
		{ "GPMI_RDn", "N/A", "N/A", "GPIO" },
		{ "UART2_CTS", "N/A", "SSP1_D4", "GPIO" },
		{ "UART2_RTS", "IR_CLK", "SSP1_D5", "GPIO" },
		{ "UART2_RX", "IR_DIN", "SSP1_D6", "GPIO" },
		{ "UART2_TX", "IR_DOUT", "SSP1_D7", "GPIO" },
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" }
	
	}, 
	{ /* bank 1 */ 
		/* MUX 2 */ 
		{ "LCD_D0", "ETM_DA0", "N/A", "GPIO" }, // 0 
		{ "LCD_D1", "ETM_DA1", "N/A", "GPIO" }, 
		{ "LCD_D2", "ETM_DA2", "N/A", "GPIO" }, 
		{ "LCD_D3", "ETM_DA3", "N/A", "GPIO" }, 
		{ "LCD_D4", "ETM_DA4", "N/A", "GPIO" }, 
		{ "LCD_D5", "ETM_DA5", "N/A", "GPIO" }, 
		{ "LCD_D6", "ETM_DA6", "N/A", "GPIO" }, 
		{ "LCD_D7", "ETM_DA7", "N/A", "GPIO" }, 
		{ "LCD_D8", "ETM_DA0", "SAIF2_D0", "GPIO" }, // 8 
		{ "LCD_D9", "ETM_DA1", "SAIF1_D0", "GPIO" }, 
		{ "LCD_D10", "ETM_DA2", "SAIF_MCLK_BITCLK", "GPIO" }, 
		{ "LCD_D11", "ETM_DA3", "SAIF_IRCLK", "GPIO" }, 
		{ "LCD_D12", "ETM_DA4", "SAIF2_D1", "GPIO" }, 
		{ "LCD_D13", "ETM_DA5", "SAIF2_D2", "GPIO" }, 
		{ "LCD_D14", "ETM_DA6", "SAIF1_D2", "GPIO" }, 
		{ "LCD_D15", "ETM_DA7", "LCD_VSYNC", "GPIO" }, 
		/* Mux 3 */ 
		{ "LCD_RESET", "N/A", "N/A", "GPIO" },  // 16 
		{ "LCD_REG", "N/A", "N/A", "GPIO" }, 	
		{ "LCD_WR_RWN", "N/A", "N/A", "GPIO" }, 	
		{ "LCD_RD_E", "N/A", "N/A", "GPIO" },
		{ "LCD_CS", "N/A", "N/A", "GPIO" },
		{ "LCD_BUSY", "LCD_VSYNC", "SAIF1_D1", "GPIO" },
		{ "SSP1_CMD", "N/A", "N/A", "GPIO" },
		{ "SSP1_SCK", "N/A", "N/A", "GPIO" },
		{ "SSP1_D0", "N/A", "N/A", "GPIO" },//24
		{ "SSP1_D1", "GPMI_CE2n", "JTAG_TCK", "GPIO" },
		{ "SSP1_D2", "UART1_CTS", "JTAG_RTCK", "GPIO" },
		{ "SSP1_D3", "UART1_RTS", "JTAG_TMS", "GPIO" },
		{ "SSP1_DET", "ETM_PSA2", "USB_OTG_ID", "GPIO" },
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" }
		
	},
	{ /* bank 2 */ 
		/* MUX 4 */ 
		{ "PWM0", "ETM_TSYNCA", "UART1_RX", "GPIO" }, //0
		{ "PWM1", "ETM_PSA1", "UART1_TX", "GPIO" }, 
		{ "PWM2", "SPDIF", "SAIF_ALT_BITCLK", "GPIO" }, 
		{ "PWM3", "ETM_PSA0", "UART2_CTS", "GPIO" }, 
		{ "PWM4", "ETM_TCLK", "UART2_RTS", "GPIO" }, 
		{ "I2C_CLK", "GPMI_RB3", "N/A", "GPIO" }, 	
		{ "I2C_SD", "GPMI_CE3n", "N/A", "GPIO" }, 	
		{ "TIMROT1", "UART1_TX", "JTAG_TDO", "GPIO" }, 	
		{ "TIMROT2", "UART1_RX", "JTAG_TDI", "GPIO" }, 	//8
		{ "EMI_CKE", "N/A", "N/A", "GPIO" }, 
		{ "EMI_RASn", "N/A", "N/A", "GPIO" }, 
		{ "EMI_CASn", "N/A", "N/A", "GPIO" }, 
		{ "EMI_CE0n", "GPMI_CE2n", "N/A", "GPIO" },  
		{ "EMI_CE1n", "GPMI_CE3n", "IR_CLK", "GPIO" }, 
		{ "EMI_CE2n", "GPMI_CE1n", "SSP2_D0", "GPIO" }, 
		{ "EMI_CE3n", "GPMI_CE0n", "N/A", "GPIO" }, 
		/* MUX 5 */ 
		{ "EMI_A0", "N/A", "N/A", "GPIO" },//16 
		{ "EMI_A1", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A2", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A3", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A4", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A5", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A6", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A7", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A8", "N/A", "N/A", "GPIO" }, // 24 
		{ "EMI_A9", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A10", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A11", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A12", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A13", "N/A", "N/A", "GPIO" }, 
		{ "EMI_A14", "N/A", "N/A", "GPIO" }, 
		{ "EMI_WEN", "N/A", "N/A", "GPIO" }

	}, 
	{ /* bank 3 */ 
		/* MUX 6 */ 
		{ "EMI_D0", "N/A", "N/A", "DISABLE" },  //0 
		{ "EMI_D1", "N/A", "N/A", "DISABLE" }, 	
		{ "EMI_D2", "N/A", "N/A", "DISABLE" }, 	
		{ "EMI_D3", "N/A", "N/A", "DISABLE" },
		{ "EMI_D4", "N/A", "N/A", "DISABLE" },
		{ "EMI_D5", "N/A", "N/A", "DISABLE" },
		{ "EMI_D6", "N/A", "N/A", "DISABLE" },
		{ "EMI_D7", "N/A", "N/A", "DISABLE" },
		{ "EMI_D8", "N/A", "N/A", "DISABLE" },// 8 
		{ "EMI_D9", "N/A", "N/A", "DISABLE" },
		{ "EMI_D10", "N/A", "N/A", "DISABLE" },
		{ "EMI_D11", "N/A", "N/A", "DISABLE" },
		{ "EMI_D12", "N/A", "N/A", "DISABLE" },
		{ "EMI_D13", "N/A", "N/A", "DISABLE" },
		{ "EMI_D14", "N/A", "N/A", "DISABLE" },
		{ "EMI_D15", "N/A", "N/A", "DISABLE" },
		/* MUX 7 */ 
		{ "EMI_DQS0", "N/A", "N/A", "DISABLE" },  // 16 
		{ "EMI_DQS1", "N/A", "N/A", "DISABLE" }, 	
		{ "EMI_DQM0", "N/A", "N/A", "DISABLE" }, 	
		{ "EMI_DQM1", "N/A", "N/A", "DISABLE" },
		{ "EMI_CLK", "N/A", "N/A", "DISABLE" },
		{ "EMI_CLKn", "N/A", "N/A", "DISABLE" },
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" },// 24 
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" },
		{ "N/A", "N/A", "N/A", "N/A" }

	}
};

#define GPIO_OUT 1 
#define GPIO_IN  0

int get_pin_gpio_mode(int bank, int pin) 
{
	volatile unsigned *addr; 
	unsigned val; 
	int mode; 

	if ( get_gpio_pin_func(bank, pin) != GPIO_MODE ) 
		return -1; 

	addr = (unsigned *)(HW_PINCTRL_DOE0_ADDR + (bank << 8));
	val  = *addr; 

	mode = (val >> pin) & 0x1; 

	return mode; 
}

int set_pin_gpio_mode(int bank, int pin, int mode)
{
	volatile unsigned *addr; 

	//make sure target pin is GPIO first
	set_gpio_pin_func(bank, pin, GPIO_MODE);
	
	if ( mode == GPIO_OUT ) { /* Out mode */ 
		if ( get_gpio_pin_func(bank, pin) == GPIO_MODE ) {
			addr = (unsigned *)(HW_PINCTRL_DOE0_ADDR + (bank << 8));
			*(addr + 1) = (1 << pin); 
			printk(KERN_DEBUG "[PINCTRL] bank(%d), pin(%d) changed to output mode\n", bank, pin); 
		} else {
			printk("ERROR: bank(%d), pin(%d) is not configured to gpio\n", bank, pin); 
		} 
	} 
	else if( mode == GPIO_IN ) /* IN mode */ 
	{ 
		if ( get_gpio_pin_func(bank, pin) == GPIO_MODE ) {
			addr = (unsigned *)(HW_PINCTRL_DOE0_ADDR + (bank << 8));
			*(addr + 2) = (1 << pin); 
			printk(KERN_DEBUG "[PINCTRL] bank(%d), pin(%d) changed to input mode\n", bank, pin); 
		} else {
			printk("ERROR: bank(%d), pin(%d) is not configured to gpio\n", bank, pin); 
		} 
	}

	return 0; 
}

int set_pin_gpio_val(int bank, int pin, int val)
{
	volatile unsigned *addr; 

	addr = (unsigned *)(HW_PINCTRL_DOUT0_ADDR + (bank << 8));

	if ( val == 0 ) { 
		*(addr + 2) = 1 << pin; 
	} else {
		*(addr + 1) = 1 << pin;
	}

	printk(KERN_DEBUG "[PINCTRL] bank(%d), pin(%d)'s GPIO DOUT is setted to %d\n", 
	       bank, pin, val); 

	return 0; 
}

int get_pin_gpio_val(int bank, int pin)
{
	volatile unsigned *addr;
	int val;
	
	addr = (unsigned *)(HW_PINCTRL_DIN0_ADDR + (bank << 8));
	val = (*addr) & (1 << pin );

	if(val !=0) return 1;
	else return 0;
}

int set_gpio_pin_func(int bank, int pin, int func)
{
	volatile unsigned *addr; 

	if ( bank < 0 || bank >= 4 ||  
	     pin < 0 || pin >= 32 || 
	     func < 0 || func >= 4 ) 
	{
		printk("[PINCTRL] Error. Out of range..\n"); 
		return -1; 
	}
	
	addr = (unsigned *)(HW_PINCTRL_MUXSEL0_ADDR + (bank * 0x100) + ((pin >> 4) * 0x10)); 
	*(addr + 2) = 0x03 << ((pin % 16 ) << 1);  // clear 
	*(addr + 1) = func << ((pin % 16 ) << 1);  // set 
	return 0; 
}

int get_gpio_pin_func(int bank, int pin)
{
#if 0 //
	volatile unsigned *addr; 
	int val; 
	addr = (unsigned *)(HW_PINCTRL_MUXSEL0_ADDR + (bank * 0x100) + ((pin >> 4) * 0x10)); 
 	val = (*addr >> ((pin % 16) << 1)) & GPIO_MODE; 
#else //add dhsong
	int val;
	val = stmp37xx_gpio_get_af( pin_GPIO(bank,  pin));
	//val = val & GPIO_MODE;
#endif
	return val;
}

/*
 * @brief Get drive strength of each pin 
 * @return   0 - 4mA, 1 - 8mA 
 */ 
int get_pin_strength(int bank, int pin)
{
	volatile unsigned *addr; 
	addr = (unsigned *)(HW_PINCTRL_DRIVE0_ADDR + (bank * 0x100)); 
 	if (*addr & (1 << pin) ) return 1; 
	else return 0; 
}

static int 
pinctrl_read_procmem(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int bank, pin, len = 0; 
	int limit = count - 80;

	bank = (int)data; 

	len += sprintf(buf + len, "\n[PINCTRL] Current PINCTRL settings\n"); 
	if (down_interruptible (&pin_sem))
		return -ERESTARTSYS;	
	
	len += sprintf(buf + len, "\nBank : %d\n\n", bank); 
	len += sprintf(buf + len, "------------------------------------------\n");
	len += sprintf(buf + len, "pin    function(mode)     value  strength\n");
	len += sprintf(buf + len, "------------------------------------------\n");
	for ( pin = 0; pin < 32 && len <= limit ; pin++ ) {
		int func = get_gpio_pin_func(bank, pin); 
		int mode = get_pin_gpio_mode(bank, pin); 
		int val  = get_pin_gpio_val(bank, pin); 
		int strength = get_pin_strength(bank, pin); 
		len += sprintf(buf + len, "%3d %11s(%3s)   %3d    %s\n", 
			       pin, pin_desc[bank][pin][func], 
			       (mode > 0)? "out" : ((mode == 0) ? "in" : "N/A"), 
			       val, 
			       (strength) ? "8mA" : "4mA" ); 
	}
	
	up (&pin_sem);

	*eof = 1; 
	return len; 
}

static ssize_t 
pinctrl_write_procmem(struct file * file, const char * buf, 
		unsigned long count, void *data)
{
	int bank, pin, func; 
	char gpio_mode[10];
	int gpio_val; 

        if (down_interruptible (&pin_sem))
                return -ERESTARTSYS;	

	bank = (int)data; 
	
	sscanf(buf, "%d %d %s %d", &pin, &func, gpio_mode, &gpio_val); 

	set_gpio_pin_func(bank, pin, func); 

	if ( func == GPIO_MODE) {
		/* gpio mode */ 
		if (!strncmp(gpio_mode, "out", 3)) { 
			/* output enable */ 
			set_pin_gpio_mode(bank, pin, GPIO_OUT); 
			/* set value */ 
			set_pin_gpio_val(bank, pin, gpio_val); 
		} else if (!strncmp(gpio_mode, "in", 2)) { 
			/* input enable */ 
			set_pin_gpio_mode(bank, pin, GPIO_IN); 
		}
	}
	printk(KERN_DEBUG "[PINCTRL] pin mapping of (%d, %d) is changed to %d\n", bank, pin, func); 
        up (&pin_sem);

	return count; 
}



int __init init_pinctrl(void)
{
	struct proc_dir_entry *proc_pinctrl_dir = NULL;
	struct proc_dir_entry *part_root; 
	int bank; 

	// semaphore init
	sema_init (&pin_sem, 1);

	// Register proc entry 
	proc_pinctrl_dir = proc_mkdir("pinctrl", 0);
	for ( bank = 0; bank < 4; bank++ ) { 
		char name[20]; 
		sprintf(name, "bank%d", bank); 
		part_root = create_proc_entry(name, S_IWUSR | S_IRUGO, proc_pinctrl_dir); 
		part_root->read_proc = pinctrl_read_procmem; 
		part_root->write_proc = pinctrl_write_procmem; 
		part_root->data = (void *)bank; 
	}

	return 0; 
}

void __exit cleanup_pinctrl(void)
{
	remove_proc_entry("pinctrl", 0); 
}

module_init(init_pinctrl);
module_exit(cleanup_pinctrl); 


