#############################################################
#
# SDL_gfx
#
#############################################################
SDLGFX_DIR=$(TOPDIR)/middle/SDL_gfx

SDL_gfx:
	(cd $(SDLGFX_DIR); ./configure --host=arm-none-linux-gnueabi --prefix=$(TOPDIR)/lib/libSDL --disable-mmx --with-sdl-prefix=$(TOPDIR)/lib/libSDL arm-unkown-linux-gnu; chmod +w Makefile.in)
	$(MAKE) -C $(SDLGFX_DIR) -f $(SDLGFX_DIR)/Makefile.platform
	$(MAKE) -C $(SDLGFX_DIR) -f $(SDLGFX_DIR)/Makefile.platform install 

SDL_gfx-clean:
	$(MAKE) -C $(SDLGFX_DIR) -f $(SDLGFX_DIR)/Makefile.platform clean 

SDL_gfx-sdk: 
	echo "TODO: copy header and libraries to sdk folder if needed"

#############################################################
#
# Toplevel Makefile options
#
#############################################################
TARGETS+=SDL SDL_gfx
