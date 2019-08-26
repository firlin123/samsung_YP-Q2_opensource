include ../config.mak

ifeq ($(STMP_BUILD),no)
OBJS = adler32.o \
       aes.o \
       base64.o \
       crc.o \
       intfloat_readwrite.o \
       fifo.o \
       lls.o \
       log.o \
       lzo.o \
       md5.o \
       mem.o \
       random.o \
       rational.o \
       tree.o \

endif

OBJS += string.o \
       mathematics.o \
	   
HEADERS = adler32.h \
          avstring.h \
          avutil.h \
          base64.h \
          common.h \
          fifo.h \
          integer.h \
          intfloat_readwrite.h \
          log.h \
          lzo.h \
          mathematics.h \
          md5.h \
          mem.h \
          random.h \
          rational.h \

NAME=avutil
LIBVERSION=$(LAVUVERSION)
LIBMAJOR=$(LAVUMAJOR)

include ../common.mak
