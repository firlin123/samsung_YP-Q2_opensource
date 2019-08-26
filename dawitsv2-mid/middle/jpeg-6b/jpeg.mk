#############################################################
#
# JPEG
#
#############################################################
JPEG_DIR=$(TOPDIR)/middle/jpeg-6b

jpeg:
	(cd $(JPEG_DIR); ./configure --host=arm-none-linux-gnueabi --prefix=$(TOPDIR)/lib/libSDL --enable-static --enable-shared CC=arm-none-linux-gnueabi-gcc)
	$(MAKE) -C $(JPEG_DIR) -f $(JPEG_DIR)/Makefile.platform
	$(MAKE) -C $(JPEG_DIR) -f $(JPEG_DIR)/Makefile.platform install 

jpeg-clean:
	$(MAKE) -C $(JPEG_DIR) -f $(JPEG_DIR)/Makefile.platform clean 

jpeg-sdk: 
	echo "TODO: copy header and libraries to sdk folder if needed"

#############################################################
#
# Toplevel Makefile options
#
#############################################################
TARGETS+=jpeg
