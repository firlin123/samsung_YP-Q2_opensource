/*
 *  linux/include/asm-arm/arch-stmp36xx/irqs.h
 *
 *  Copyright (C) 2005 Sigmatel Inc
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

#include <asm/arch/hardware.h>

#define IRQ_DEBUG_UART                 0
#define IRQ_COMMS_RX                   1
#define IRQ_COMMS_TX                   1
#define IRQ_SSP2_ERROR                 2
#define IRQ_VDD5V                      3
#define IRQ_HEADPHONE_SHORT            4
#define IRQ_DAC_DMA                    5
#define IRQ_DAC_ERROR                  6
#define IRQ_ADC_DMA                    7
#define IRQ_ADC_ERROR                  8
#define IRQ_SPDIF_DMA                  9
#define IRQ_SAIF2_DMA                  9
#define IRQ_SPDIF_ERROR                10
#define IRQ_SAIF1_IRQ                  10
#define IRQ_SAIF2_IRQ                  10
#define IRQ_USB_CTRL                   11
#define IRQ_USB_WAKEUP                 12
#define IRQ_GPMI_DMA                   13
#define IRQ_SSP1_DMA                   14
#define IRQ_SSP_ERROR                  15
#define IRQ_GPIO_BANK0                 16
#define IRQ_GPIO_BANK1                 17
#define IRQ_GPIO_BANK2                 18
#define IRQ_SAIF1_DMA                  19
#define IRQ_SSP2_DMA                   20
#define IRQ_ECC8_IRQ                   21
#define IRQ_RTC_ALARM                  22
#define IRQ_UARTAPP_TX_DMA             23
#define IRQ_UARTAPP_INTERNAL           24
#define IRQ_UARTAPP_RX_DMA             25
#define IRQ_I2C_DMA                    26
#define IRQ_I2C_ERROR                  27
#define IRQ_TIMER0                     28
#define IRQ_TIMER1                     29
#define IRQ_TIMER2                     30
#define IRQ_TIMER3                     31
#define IRQ_BATT_BRNOUT                32
#define IRQ_VDDD_BRNOUT                33
#define IRQ_VDDIO_BRNOUT               34
#define IRQ_VDD18_BRNOUT               35
#define IRQ_TOUCH_DETECT               36
#define IRQ_LRADC_CH0                  37
#define IRQ_LRADC_CH1                  38
#define IRQ_LRADC_CH2                  39
#define IRQ_LRADC_CH3                  40
#define IRQ_LRADC_CH4                  41
#define IRQ_LRADC_CH5                  42
#define IRQ_LRADC_CH6                  43
#define IRQ_LRADC_CH7                  44
#define IRQ_LCDIF_DMA                  45
#define IRQ_LCDIF_ERROR                46
#define IRQ_DIGCTL_DEBUG_TRAP          47
#define IRQ_RTC_1MSEC                  48
#define IRQ_DRI_DMA                    49
#define IRQ_DRI_ATTENTION              50
#define IRQ_GPMI_ATTENTION             51
#define IRQ_IR                         52
#define IRQ_DCP_VMI                    53
#define IRQ_DCP                        54
#define IRQ_RESERVED_55                55
#define IRQ_RESERVED_56                56
#define IRQ_RESERVED_57                57
#define IRQ_RESERVED_58                58
#define IRQ_RESERVED_59                59
#define SW_IRQ_60                      60
#define SW_IRQ_61                      61
#define SW_IRQ_62                      62
#define SW_IRQ_63                      63
#define NR_CORE_IRQS (64)

#define IRQ_START_OF_EXT_GPIO (NR_CORE_IRQS)
#define NR_EXT_GPIO_IRQS (32*3) /* bank 0/1/2 */

/* export to upper layer */
#define NR_IRQS (NR_CORE_IRQS + NR_EXT_GPIO_IRQS)

// TIMER and BRNOUT are FIQ capable
#define FIQ_START			IRQ_TIMER0

/* Hard disk IRQ is a GPMI attention IRQ */
#define IRQ_HARDDISK		IRQ_GPMI_ATTENTION

#include "gpio.h"
#define PIN2IRQ(_pin) \
	(ext_GPIO_BANK(_pin) * 32 + ext_GPIO_PIN(_pin) + IRQ_START_OF_EXT_GPIO)
#define __IRQ2BANK(_irq) ((_irq - IRQ_START_OF_EXT_GPIO) / 32)
#define __IRQ2PINO(_irq) ((_irq - IRQ_START_OF_EXT_GPIO) % 32)
#define IRQ2PIN(_irq) pin_GPIO( __IRQ2BANK(_irq), __IRQ2PINO(_irq))

/* ACK interrupt on the way out */
#define irq_finish(irq) do { HW_ICOLL_LEVELACK_WR(1); (void) HW_ICOLL_STAT_RD(); } while (0)
