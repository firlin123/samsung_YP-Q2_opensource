#############################################################
#
# FREETYPE
#
#############################################################
FREETYPE_DIR=$(TOPDIR)/middle/Freetype/ft2-2.2.1/freetype2

freetype:
	(cd $(FREETYPE_DIR); ./configure --host=arm-none-linux-gnueabi --prefix=$(TOPDIR)/lib/libfreetype arm-unkown-linux-gnu CC=arm-none-linux-gnueabi-gcc)
	$(MAKE) -C $(FREETYPE_DIR) -f $(FREETYPE_DIR)/Makefile
	$(MAKE) -C $(FREETYPE_DIR) -f $(FREETYPE_DIR)/Makefile install 

freetype-clean:
	$(MAKE) -C $(FREETYPE_DIR) -f $(FREETYPE_DIR)/Makefile clean 

freetype-sdk: 
	echo "TODO: copy header and libraries to sdk folder if needed"

#############################################################
#
# Toplevel Makefile options
#
#############################################################
TARGETS+=freetype 

