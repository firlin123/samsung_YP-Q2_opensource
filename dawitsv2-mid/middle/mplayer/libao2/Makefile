include ../../../Rules.mk

include ../config.mak

LIBNAME_MPLAYER = libao2.a

ifeq ($(STMP_BUILD),no)
SRCS_MPLAYER = \
               ao_mpegpes.c \
               ao_pcm.c \

endif

SRCS_MPLAYER += audio_out.c \
               ao_null.c \
				$(AO_SRCS)

#SRCS_MPLAYER += ao_stmp37xx.c

include ../mpcommon.mak
