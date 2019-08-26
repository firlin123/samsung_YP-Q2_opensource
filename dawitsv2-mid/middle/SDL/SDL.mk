#############################################################
#
# SDL
#
#############################################################
SDL_DIR=$(TOPDIR)/middle/SDL

SDL:
	(cd $(SDL_DIR); ./configure --host=arm-none-linux-gnueabi --enable-video-fbcon --disable-video-dummy --disable-video-dga --disable-arts --disable-esd --disable-alsa --disable-cdrom --disable-video-x11 --disable-nasm --disable-joystick --prefix=$(TOPDIR)/lib/libSDL arm-unkown-linux-gnu)
	$(MAKE) -C $(SDL_DIR) -f $(SDL_DIR)/Makefile.platform
	$(MAKE) -C $(SDL_DIR) -f $(SDL_DIR)/Makefile.platform install 

SDL-clean:
	$(MAKE) -C $(SDL_DIR) -f $(SDL_DIR)/Makefile.platform clean 

SDL-sdk: 
	echo "TODO: copy header and libraries to sdk folder if needed"

#############################################################
#
# Toplevel Makefile options
#
#############################################################
TARGETS+=SDL 
