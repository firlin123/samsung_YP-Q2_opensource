#############################################################
#
# SDL_image
#
#############################################################
SDLIMAGE_DIR=$(TOPDIR)/middle/SDL_image

SDL_image:
	(cd $(SDLIMAGE_DIR); ./configure --disable-tif --disable-lbm --disable-pnm --disable-tga --disable-tif-shared --disable-pcx --disable-xpm --disable-xv --disable-xcf --disable-png --host=arm-none-linux-gnueabi --prefix=$(TOPDIR)/lib/libSDL --with-sdl-prefix=$(TOPDIR)/lib/libSDL CPPFLAGS=-I$(TOPDIR)/lib/libSDL/include)
	$(MAKE) -C $(SDLIMAGE_DIR) -f $(SDLIMAGE_DIR)/Makefile.platform
	$(MAKE) -C $(SDLIMAGE_DIR) -f $(SDLIMAGE_DIR)/Makefile.platform install 

SDL_image-clean:
	$(MAKE) -C $(SDLIMAGE_DIR) -f $(SDLIMAGE_DIR)/Makefile.platform clean 

SDL_image-sdk: 
	echo "TODO: copy header and libraries to sdk folder if needed"

#############################################################
#
# Toplevel Makefile options
#
#############################################################
TARGETS+=SDL jpeg SDL_image
