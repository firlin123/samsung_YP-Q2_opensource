##############################################################
# MENDEL Prject Makefile
#
##############################################################

ifeq ($(TOP),)
TOP			= $(shell pwd)
endif

VARIANT			?= 37xx

PREBUILDDIR		= $(TOP)/prebuild
BUILDDIR		= $(TOP)/build
USERDIR			= $(TOP)/user
# busybox build
BUSYBOX_DIR		= $(USERDIR)/busybox-1.9.1
BUSYBOX_INSTALL_DIR	= $(USERDIR)/busybox-install
BUSYBOX_CPIO		= $(BUILDDIR)/busybox.cpio

# RootFS
ROOTFS_SQFS		= $(BUILDDIR)/rootfs.sqfs
ROOTFS_DIR		= $(TOP)/output/target/romfs
ROOTFS_MODULES_DIR	= $(ROOTFS_DIR)/lib/modules

# Dirs
OUTPUT_DIR		= $(TOP)/output/$(VARIANT)

TOOLS_HOST_DIR		= $(TOP)/tools/host

CROSS_COMPILE ?= arm-none-linux-gnueabi-
export CROSS_COMPILE

# Look for cross-compiled root next to compiler dir
# !!hack!! to get toolchain GLIBC location
ifeq ($(CROSS_COMPILE),arm-none-linux-gnueabi-)
	ARM_ROOT_LIB_DIR  = $(realpath $(dir $(shell ${CROSS_COMPILE}gcc -print-file-name=ld-2.3.6.so)))
	GCC_ROOT_LIB_DIR  = $(realpath $(dir $(shell ${CROSS_COMPILE}gcc -print-file-name=libstdc++.so)))
	#ARM_ROOT_LIB_DIR  = $(realpath $(dir $(shell ${CROSS_COMPILE}gcc -print-file-name=ld-2.3.6.so))../../lib/armv4t)
else
	ARM_ROOT_LIB_DIR  = $(realpath $(dir $(shell ${CROSS_COMPILE}gcc -print-file-name=ld-2.3.6.so))../../armv5-root/lib)
	GCC_ROOT_LIB_DIR  = $(realpath $(dir $(shell ${CROSS_COMPILE}gcc -print-file-name=libstdc++.so))../../armv5-root/lib)
endif


MAKEDEVS_SCRIPT		= $(BUILDDIR)/makedevs
FAKEROOT		= fakeroot 
DEPMOD			= /sbin/depmod

CPIO_IN_FLAGS		= --quiet -i
CPIO_OUT_FLAGS		= --quiet -H newc -o

# Override the banana nonsense
LINUX_FLAGS             = ARCH=arm CROSS_COMPILE=${CROSS_COMPILE}
BUSYBOX_FLAG    = ARCH=arm CROSS_COMPILE=${CROSS_COMPILE}

ZIMAGE			= $(OUTPUT_DIR)/arch/arm/boot/zImage

banner = \
	@echo "===============================================";\
	 echo "$(1)" " @"`date +%T`;\
	 echo "==============================================="\

# Build the planet
.PHONY: all
all: $(ZIMAGE) $(ROOTFS_SQFS)
	@ls -al $(BUILDDIR)/*.cpio $(BUILDDIR)/rootfs.* $(BUILDDIR)/zImage
	@rm -rf $(BUILDDIR)/system_build
	@mkdir $(BUILDDIR)/system_build
	@cp -rf $(BUILDDIR)/rootfs.sqfs \
		$(TOP)/output/37xx/arch/arm/boot/zImage \
		$(TOP)/output/37xx/vmlinux \
		$(BUILDDIR)/system_build/.

.PHONY: zImage $(ZIMAGE)
zImage: $(ZIMAGE)

$(ZIMAGE): $(OUTPUT_DIR)/.config
	$(call banner, "Building Linux zImage...")
	#$(MAKE) $(LINUX_FLAGS) -j -C $(TOP)/linux O=$(OUTPUT_DIR) zImage
	$(MAKE) $(LINUX_FLAGS) -j 10 -C $(TOP)/linux O=$(OUTPUT_DIR) zImage
	@ln -sf $@ $(BUILDDIR)/.
	@ls -lL $(BUILDDIR)/zImage

.PHONY: busybox  
busybox:
	@(if [ -e $(BUSYBOX_INSTALL_DIR) ]; then \
	 echo "Removing old busybox install tree" ; \
	 rm -rf $(BUSYBOX_INSTALL_DIR); fi)
#	$(MAKE) -C $(BUSYBOX_DIR) oldconfig
	$(call banner, "Cleaning Busybox")
	$(MAKE) -C $(BUSYBOX_DIR) $(BUSYBOX_FLAG) clean
	$(call banner, "Rebuilding Busybox")
	$(MAKE) -C $(BUSYBOX_DIR) $(BUSYBOX_FLAG)	
	@echo Installing Busybox to ./$(BUSYBOX_INSTALL_DIR)/
	$(MAKE) -C $(BUSYBOX_DIR) install PREFIX=$(BUSYBOX_INSTALL_DIR) $(BUSYBOX_FLAG)
	@echo Making Busybox CPIO archive
	((cd $(BUSYBOX_INSTALL_DIR); find | cpio $(CPIO_OUT_FLAGS)) >$(BUSYBOX_CPIO))
	@echo Removing temporary trees
	rm -rf $(BUSYBOX_INSTALL_DIR)
	
.PHONY: busybox-clean 
busybox-clean:
	$(MAKE) -C $(BUSYBOX_DIR) $(BUSYBOX_FLAG) clean
	@rm -rf $(BUSYBOX_CPIO)

busybox-config:
	@echo Configuring Busybox
	$(MAKE) -C $(BUSYBOX_DIR) menuconfig PREFIX=$(BUSYBOX_INSTALL_DIR) $(BUSYBOX_FLAG)


.PHONY:	rootfs-under-fakeroot
rootfs-under-fakeroot:
	@(if [ -e $(ROOTFS_DIR) ]; then \
		echo "Removing old rootfs temporary tree" ; \
		rm -rf $(ROOTFS_DIR); fi)

	@echo Creating rootfs tree structure
	@mkdir -p $(ROOTFS_DIR)
	@mkdir -p $(ROOTFS_MODULES_DIR)
	@mkdir -p $(ROOTFS_DIR)/dev
	@mkdir -p $(ROOTFS_DIR)/appfs
	@mkdir -p $(ROOTFS_DIR)/share
	@mkdir -p $(ROOTFS_DIR)/etc/init.d
	@mkdir -p $(ROOTFS_DIR)/proc
	@mkdir -p $(ROOTFS_DIR)/sys
	@mkdir -p $(ROOTFS_DIR)/mnt/
	@mkdir -p $(ROOTFS_DIR)/mnt/tmp
	@mkdir -p $(ROOTFS_DIR)/mnt/usb
	@mkdir -p $(ROOTFS_DIR)/mnt/app
	@(cd $(ROOTFS_DIR)/dev; sh $(MAKEDEVS_SCRIPT))

	@echo Unpacking Busybox into rootfs tree
	@(cd $(ROOTFS_DIR) && cpio $(CPIO_IN_FLAGS) < $(BUSYBOX_CPIO))
	@ln -s sbin/init $(ROOTFS_DIR)/init
	
# copy libraries (root)
	@echo Copying Root libraries
	@mkdir -p $(ROOTFS_DIR)/lib
	@cp -a $(ARM_ROOT_LIB_DIR)/*so* $(ROOTFS_DIR)/lib
	@cp -a $(GCC_ROOT_LIB_DIR)/*so* $(ROOTFS_DIR)/lib #dhsong
	
	@(if grep CONFIG_MODULES=y $(OUTPUT_DIR)/.config; then \
		echo Installing modules into rootfs tree; \
		$(MAKE) $(LINUX_FLAGS) -C $(OUTPUT_DIR) modules_install INSTALL_MOD_PATH=$(ROOTFS_DIR) O=$(OUTPUT_DIR); \
		echo Running depmod; \
		$(DEPMOD) -a -b $(ROOTFS_DIR) -r $(shell cat $(OUTPUT_DIR)/include/linux/utsrelease.h | grep UTS_RELEASE | sed 's/.*"\(.*\)".*/\1/g'); \
		fi)

	@echo "Removing Unnessarary Root libraries & Stripping..."
	-@rm $(ROOTFS_DIR)/lib/libnss* $(ROOTFS_DIR)/lib/libnsl* \
		$(ROOTFS_DIR)/lib/libBrokenLocale* $(ROOTFS_DIR)/lib/libanl* $(ROOTFS_DIR)/lib/libcidn* \
		$(ROOTFS_DIR)/lib/libmemusage* \
		$(ROOTFS_DIR)/lib/libpcprofile* $(ROOTFS_DIR)/lib/libresolv*
	@${CROSS_COMPILE}strip -s $(ROOTFS_DIR)/lib/*so*
	@echo Done.

	@echo "Copying prebuild files"
	@cp -r $(PREBUILDDIR)/* $(ROOTFS_DIR)

#   For gadget storage test only - 1MB FAT image
#	@echo Copying backing storage for USB file storage
#	@cp backing $(ROOTFS_DIR)/backing
	@echo Making squashfs
	@rm -f $(ROOTFS_SQFS)
#	@((cd $(ROOTFS_DIR); $(TOOLS_HOST_DIR)/mksquashfs bin/ rootfs.squashfs))
	@(cd $(ROOTFS_DIR); $(TOOLS_HOST_DIR)/mksquashfs ./ $(ROOTFS_SQFS) -all-root )
#	@cd $(ROOTFS_DIR)
#	@$(TOOLS_HOST_DIR)/mksquashfs ./ $(ROOTFS_SQFS) -all-root 

#	@echo "Removing rootfs temporary tree"
#	@rm -rf $(ROOTFS_DIR)
	

.PHONY: modules
modules:
	$(call banner, "Making kernel modules")
	@(if [ -e $(TOP)/linux/fs/rfs/lib/ftl_4k.lib ]; then \
		cp -f $(TOP)/linux/fs/rfs/lib/ftl_4k.lib \
		$(OUTPUT_DIR)/fs/rfs/ftl_4k.o; fi)
	@(if [ -e $(TOP)/linux/fs/rfs/lib/rfs_cache.lib ]; then \
		cp -f $(TOP)/linux/fs/rfs/lib/rfs_cache.lib \
		$(OUTPUT_DIR)/fs/rfs/rfs_cache.o; fi)
	@(if [ -e $(TOP)/linux/fs/rfs/lib/rfs_code_convert.lib ]; then \
		cp -f $(TOP)/linux/fs/rfs/lib/rfs_code_convert.lib \
		$(OUTPUT_DIR)/fs/rfs/rfs_code_convert.o; fi)
	@(if [ -e $(TOP)/linux/fs/rfs/lib/rfs_linuxk.lib ]; then \
		cp -f $(TOP)/linux/fs/rfs/lib/rfs_linuxk.lib \
		$(OUTPUT_DIR)/fs/rfs/rfs_linuxk.o; fi)
	@(if [ -e $(TOP)/linux/fs/rfs/lib/rfs_log.lib ]; then \
		cp -f $(TOP)/linux/fs/rfs/lib/rfs_log.lib \
		$(OUTPUT_DIR)/fs/rfs/rfs_log.o; fi)
	@(if ! [ -e $(OUTPUT_DIR)/.config ]; then \
		echo "Using default .config" ; \
		mkdir -p $(OUTPUT_DIR) ; \
		cp $(TOP)/linux/arch/arm/configs/stmp$(VARIANT)_defconfig $(OUTPUT_DIR)/.config ; \
		$(MAKE) $(LINUX_FLAGS) -C $(TOP)/linux oldconfig O=$(OUTPUT_DIR); fi)
	@(if grep CONFIG_MODULES=y $(OUTPUT_DIR)/.config; then \
		$(MAKE) $(LINUX_FLAGS) -C $(TOP)/linux O=$(OUTPUT_DIR) modules ; fi)

# Recursive make (/spit) run under a fakeroot so we can make device nodes etc
.PHONY:		rootfs
#$(ROOTFS_SQFS) rootfs: modules apps $(MAKEDEVS_SCRIPT) #dhsong
$(ROOTFS_SQFS) rootfs: modules $(MAKEDEVS_SCRIPT)
	$(call banner, "Making $(notdir $(ROOTFS_SQFS))")
	@$(FAKEROOT) $(MAKE) rootfs-under-fakeroot
	@ls -al $(ROOTFS_SQFS)
	@chmod +r $(ROOTFS_SQFS)
	
$(OUTPUT_DIR)/.config:	$(TOP)/linux/arch/arm/configs/stmp$(VARIANT)_defconfig
	@(if [ -f $@ ] ; then \
		echo "***********" ; \
		echo Warning: $< has changed. You should probably delete $@ or merge changes or this build may fail and/or the application may function incorrectly. ; \
		echo "***********" ; \
	else \
		mkdir -p $(OUTPUT_DIR) ; \
		echo Copying default $< to $@ ; \
		cp $< $@ ; \
	fi)

.PHONY:			kernel-config
kernel-config:
	$(MAKE) $(LINUX_FLAGS) -C $(TOP)/linux menuconfig O=$(OUTPUT_DIR)
	@cp $(OUTPUT_DIR)/.config $(TOP)/linux/arch/arm/configs/stmp$(VARIANT)_defconfig
	
	
.PHONY: help
help:
	@echo ""
	@echo "-----------------------------------------------------------------------------"
	@echo " NOTE: each apps & libs may need to be configured before building the project"
	@echo "       refer to conf-37xx.sh in each directory to configure"
	@echo "-----------------------------------------------------------------------------"
	@echo ""
	@echo " make [all]		: build all (bootldr, zImage, rootfs, modules, sb)"
	@echo " 			  busybox should be built first"
	@echo " make clean 	  	: clean bootldr, kernel, rootfs"
	@echo ""
	@echo " make zImage  		: build krenel zImage"
	@echo " make kernel-config  	: configure kernel"
	@echo ""
	@echo " make rootfs  		: build rootfs, will build modules & apps as well"
	@echo " make modules 		: build kernel modules"
	@echo ""
	@echo " make busybox         	: build busybox"
	@echo " make busybox-clean   	: clean only busybox"
	@echo " make busybox-config	: configure busybox"
	@echo ""

.PHONY: clean
clean:
	@rm -f zImage *.sb
	@rm -f $(ROOTFS_SQFS)
	@rm -rf $(ROOTFS_DIR)
	@rm -rf ./output #$(OUTPUT_DIR)
	
	#$(call banner, "Cleaning zImage")
	#$(MAKE) $(LINUX_FLAGS) -C ./linux O=$(OUTPUT_DIR) clean
	
	
.PHONY: rebuild
#rebuild: clean libs busybox sb
rebuild: clean busybox

