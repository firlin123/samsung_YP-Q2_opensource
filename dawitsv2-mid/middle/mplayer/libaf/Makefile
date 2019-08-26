include ../../../Rules.mk

include ../config.mak

LIBNAME_COMMON = libaf.a

ifeq ($(STMP_BUILD),no)
SRCS_COMMON = af.c \
              af_center.c \
              af_channels.c \
              af_comp.c \
              af_delay.c \
              af_dummy.c \
              af_equalizer.c \
              af_extrastereo.c \
              af_format.c \
              af_gate.c \
              af_hrtf.c \
              af_karaoke.c \
              af_pan.c \
              af_resample.c \
              af_sinesuppress.c \
              af_sub.c \
              af_surround.c \
              af_sweep.c \
              af_volnorm.c \
              af_volume.c \
              filter.c \
              window.c \
              $(AF_SRCS) \

endif

SRCS_COMMON += af.c \
              af_dummy.c \
	      af_channels.c \
              af_tools.c \
              format.c \
              $(AF_SRCS) \
			  
SRCS_COMMON-$(HAVE_SYS_MMAN_H)      += af_export.c

ifeq ($(STMP_BUILD),no)
SRCS_COMMON-$(LIBAVCODEC)           += af_lavcresample.c
endif

include ../mpcommon.mak
