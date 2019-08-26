#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y += generic.o time.o irq.o gpio.o dma.o lradc.o
obj-y += pm.o clocks/ power/

obj-$(CONFIG_CPU_STMP3700)		+= stmp3700.o

#obj-$(CONFIG_MACH_ARMA37)		+= arma37.o lradc-vddio.o stmp37xx_battery.o #stmp37xx_reset.o
obj-$(CONFIG_MACH_ARMA37)		+= arma37.o stmp37xx_battery.o #stmp37xx_reset.o

led-y := leds.o
led-$(CONFIG_MACH_ARMA37)		+= leds-arma37.o
obj-$(CONFIG_LEDS)			+= $(led-y)

obj-$(CONFIG_STMP3XXX_CSC)              += stmp3xxx_csc.o
obj-$(CONFIG_STMP3XXX_OCRAM)              += ocram.o
obj-$(CONFIG_STMP3XXX_DIGCTL)              += stmp3xxx_digctl.o



#obj-y					+= stmp36xx_dma.o
#obj-$(CONFIG_STMP3XXX_DAC)		+= stmp3xxx_dac.o
#obj-$(CONFIG_STMP3XXX_LRADC)		+= stmp3xxx_lradc.o
#obj-$(CONFIG_STMP3XXX_BUTTONS)		+= stmp3xxx_button.o
#obj-$(CONFIG_STMP3XXX_TOUCHSCREEN)	+= stmp3xxx_touchscreen.o
#obj-$(CONFIG_STMP3XXX_GPIO)		+= stmp3xxx_gpio.o
#obj-$(CONFIG_STMP3XXX_BATTERY)		+= stmp3xxx_battery.o
#obj-$(CONFIG_STMP3XXX_ADC)              += stmp3xxx_adc.o
