#############################################################
#
# mplayer
#
#############################################################
MP_DIR=$(TOPDIR)/middle/mplayer

mplayer:
	(cd $(MP_DIR); ./conf-37xx.sh)
	$(MAKE) -C $(MP_DIR) -f $(MP_DIR)/Makefile

mplayer-clean:
	$(MAKE) -C $(MP_DIR) -f $(MP_DIR)/Makefile clean 

mplayer-sdk: 
	echo "TODO: copy header and libraries to sdk folder if needed"

#############################################################
#
# Toplevel Makefile options
#
#############################################################
TARGETS+=mplayer 
