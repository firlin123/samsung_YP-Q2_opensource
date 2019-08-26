cmd_libbb/perror_nomsg.o := arm-none-linux-gnueabi-gcc -Wp,-MD,libbb/.perror_nomsg.o.d   -std=gnu99 -Iinclude -Ilibbb  -I/home/dh7.song/src/stmp37xx/change_compiler_4.1.1/dawitsv2-sys/user/busybox-1.9.1/libbb -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG  -D"BB_VER=KBUILD_STR(1.9.1)" -DBB_BT=AUTOCONF_TIMESTAMP -D_FORTIFY_SOURCE=2  -Wall -Wshadow -Wwrite-strings -Wundef -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Os -fno-builtin-strlen -finline-limit=0 -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-guess-branch-probability -funsigned-char -static-libgcc -falign-functions=1 -falign-jumps=1 -falign-labels=1 -falign-loops=1 -Wdeclaration-after-statement -Wno-pointer-sign    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(perror_nomsg)"  -D"KBUILD_MODNAME=KBUILD_STR(perror_nomsg)" -c -o libbb/perror_nomsg.o libbb/perror_nomsg.c

deps_libbb/perror_nomsg.o := \
  libbb/perror_nomsg.c \

libbb/perror_nomsg.o: $(deps_libbb/perror_nomsg.o)

$(deps_libbb/perror_nomsg.o):
