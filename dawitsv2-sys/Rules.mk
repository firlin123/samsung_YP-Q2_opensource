TOP		= $(shell pwd)
TOPDIR		= $(shell pwd)
OWNER		= $(shell whoami)
BUILDDIR        = $(TOP)/build
USERDIR         = $(TOP)/user
SYSTEM_BUILD_DIR	= $(BUILDDIR)/system_build 

#CROSS_COMPILE   = /opt/empeg/bin/armv5-empeg-linux-gnueabi-
CROSS_COMPILE   = arm-none-linux-gnueabi-
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CXX		= $(CROSS_COMPILE)g++
AR              = $(CROSS_COMPILE)ar
RANLIB          = $(CROSS_COMPILE)ranlib
STRIP		= $(CROSS_COMPILE)strip
CFLAGS		= -O2 -Wall $(DEBUG) 
