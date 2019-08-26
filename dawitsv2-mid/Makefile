#
#  makefile to build entire platform 
#


#--------------------------------------------------------------
#
#

MODEL	= Q2
RELVER	= 0.01
COUNTRY	= WA
POST	= TEST
USB	= MSC
YPLANG = EN

TOPDIR	= $(shell pwd)
OWNER	= $(shell whoami)

ROOTFS	= $(TOPDIR)/target/romfs
APPFS	= $(TOPDIR)/target/appfs

DATE	:=$(shell date -u +%Y%m%d)

CROSS_COMPILE   = arm-none-linux-gnueabi-
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CXX		= $(CROSS_COMPILE)g++
AR              = $(CROSS_COMPILE)ar
RANLIB          = $(CROSS_COMPILE)ranlib
STRIP		= $(CROSS_COMPILE)strip

DEBUG		= # -g 
CFLAGS		= -O2 -Wall $(DEBUG)

export MODEL RELVER COUNTRY POST USB YPLANG TOPDIR APPFS ROOTFS DATE
export CC AS LD CXX AR RANLIB STRIP 

all: world 

#############################################################
# application make files 
#############################################################

TARGETS:=

include middle/*/*.mk # include makefiles for library packages

TARGETS_CLEAN:=$(patsubst %,%-clean,$(TARGETS))
TARGETS_SDK:=$(patsubst %,%-sdk,$(TARGETS))


installtarget:


firmware:


Rules.mk: Rules.mk.in
	echo "TOPDIR=$(TOPDIR)" > Rules.mk 
	cat Rules.mk.in >> Rules.mk

maketarget:
	(cd target; tar zcvf $(TOPDIR)/proj/$(MODEL)/target-base.tar.gz appfs)

world: Rules.mk installtarget $(TARGETS) firmware

#############################################################
#
# Cleanup 
#
#############################################################
clean: $(TARGETS_CLEAN) dummy	
	rm -rf target

help:
	@echo 'Cleaning:'
	@echo '  clean                  - delete temporary files created by build'
	@echo
	@echo 'Build:'
	@echo '  all                    - make world'
	@echo
	@echo 'Miscellaneous:'
	@echo '  sdk                    - create header and library files ' 
	@echo
	@echo 'See docs/README '
	@echo 

.PHONY: dummy


