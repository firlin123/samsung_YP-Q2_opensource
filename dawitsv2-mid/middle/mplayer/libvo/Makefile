include ../../../Rules.mk

include ../config.mak

LIBNAME_COMMON  = libosd.a
LIBNAME_MPLAYER = libvo.a

ifeq ($(STMP_BUILD),no)
SRCS_MPLAYER = aspect.c \
               geometry.c \
               spuenc.c \
               vo_mpegpes.c \
               vo_null.c \
               vo_yuv4mpeg.c \

endif

SRCS_MPLAYER += \
               video_out.c \
               $(VO_SRCS) \

SRCS_MPLAYER-$(VIDIX)          +=  vosub_vidix.c

ifeq ($(STMP_BUILD),no)
SRCS_COMMON = aclib.c \
              osd.c \
              sub.c \

endif
			  
SRCS_COMMON-$(BITMAP_FONT) +=  font_load.c
SRCS_COMMON-$(FREETYPE)    +=  font_load_ft.c

include ../mpcommon.mak

%.o: %.m
	$(CC) $(CFLAGS) -c -o $@ $<

vo_png.o: CFLAGS := $(filter-out -I../libavcodec,$(CFLAGS))
