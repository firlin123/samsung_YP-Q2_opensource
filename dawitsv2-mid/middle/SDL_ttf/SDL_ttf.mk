#############################################################
#
# SDL_ttf
#
#############################################################
SDLTTF_DIR=$(TOPDIR)/middle/SDL_ttf

SDL_ttf:
	(cd $(SDLTTF_DIR); touch missing; ./configure --host=arm-none-linux-gnueabi --prefix=$(TOPDIR)/lib/libSDL --with-sdl-prefix=$(TOPDIR)/lib/libSDL --with-freetype-prefix=$(TOPDIR)/lib/libfreetype --without-x)
	$(MAKE) -C $(SDLTTF_DIR) -f $(SDLTTF_DIR)/Makefile.platform
	$(MAKE) -C $(SDLTTF_DIR) -f $(SDLTTF_DIR)/Makefile.platform install 

SDL_ttf-clean:
	$(MAKE) -C $(SDLTTF_DIR) -f $(SDLTTF_DIR)/Makefile.platform clean 

SDL_ttf-sdk: 
	echo "TODO: copy header and libraries to sdk folder if needed"

#############################################################
#
# Toplevel Makefile options
#
#############################################################
TARGETS+=SDL SDL_ttf
