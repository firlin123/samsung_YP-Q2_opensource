#
# Makefile for misc devices that really don't fit anywhere else.
#
obj- := misc.o	# Dummy rule to force built-in.o to be made

obj-$(CONFIG_IBM_ASM)		+= ibmasm/
obj-$(CONFIG_HDPU_FEATURES)	+= hdpuftrs/
obj-$(CONFIG_MSI_LAPTOP)     += msi-laptop.o
obj-$(CONFIG_ASUS_LAPTOP)     += asus-laptop.o
obj-$(CONFIG_ATMEL_SSC)		+= atmel-ssc.o
obj-$(CONFIG_LKDTM)		+= lkdtm.o
obj-$(CONFIG_TIFM_CORE)       	+= tifm_core.o
obj-$(CONFIG_TIFM_7XX1)       	+= tifm_7xx1.o
obj-$(CONFIG_PHANTOM)		+= phantom.o
obj-$(CONFIG_SGI_IOC4)		+= ioc4.o
obj-$(CONFIG_SONY_LAPTOP)	+= sony-laptop.o
obj-$(CONFIG_THINKPAD_ACPI)	+= thinkpad_acpi.o
obj-$(CONFIG_FUJITSU_LAPTOP)	+= fujitsu-laptop.o
obj-$(CONFIG_EEPROM_93CX6)	+= eeprom_93cx6.o
obj-$(CONFIG_STMP37XX_USBEVENT)	+= stmp37xx_usbevent.o
obj-$(CONFIG_STMP37XX_OCOTP)	+= stmp37xx_ocotp.o
obj-$(CONFIG_STMP37XX_NANDSB)	+= stmp37xx_nandsb.o
obj-$(CONFIG_STMP37XX_PWMLED)   += stmp37xx_pwmled.o
obj-$(CONFIG_STMP37XX_FM_SI4702) += stmp37xx_fmtuner_si4702.o stmp37xx_i2c.o
obj-$(CONFIG_STMP37XX_FM_SI4703) += stmp37xx_fmtuner_si4703.o stmp37xx_sw_i2c.o
obj-$(CONFIG_STMP37XX_FM_SI4721) += stmp37xx_fmtuner_si4721.o stmp37xx_sw_i2c.o
obj-$(CONFIG_STMP37XX_EAREVENT) += stmp37xx_earphone.o
obj-$(CONFIG_T32_VIRTUAL) += stmp37xx_t32.o
obj-$(CONFIG_STMP37XX_COMMEVENT) += stmp37xx_commevent.o
