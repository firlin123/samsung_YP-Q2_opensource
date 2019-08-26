########################################################
# Makefile to build applications
# - Customize Apps directory to build
#
ifeq ($(TOP),)
	export TOP= $(realpath ../)
endif

.PHONY:	all
all:
	@for a in $(APPS_DIR); do $(MAKE) -C $$a ; done

.PHONY:	install
install:
	@for a in $(APPS_DIR); do $(MAKE) -C $$a install; done

.PHONY:	dep depend
dep depend:
	@for a in $(APPS_DIR); do $(MAKE) -C $$a dep; done

.PHONY:	configure
configure:
	@for a in $(APPS_DIR); do [ -e $$a/conf-37xx.sh ] && cd $$a && sh conf-37xx.sh && cd -; done
	
.PHONY:	clean
clean:
	@for a in $(APPS_DIR); do $(MAKE) -C $$a clean; done
