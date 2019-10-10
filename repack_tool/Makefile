Q2C=Q2Crypt
all: unpack
unpack: crypttool
	./bin/unpack.sh
repack: crypttool
	./bin/repack.sh
crypttool:
ifeq ("$(wildcard bin/$(Q2C))","")
	@echo "CC $(Q2C).."
	@$(CC) -O3 -Wall src/Q2Crypt.c src/md5.c -o bin/$(Q2C)
endif
fclean: clean
	rm -f Q2Update*.dat
clean: cleancrypttool
	rm -rf q2update_unpacked
cleancrypttool:
	rm -f bin/$(Q2C)
