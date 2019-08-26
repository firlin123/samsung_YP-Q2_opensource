cmd_libbb/makedev.o := arm-none-linux-gnueabi-gcc -Wp,-MD,libbb/.makedev.o.d   -std=gnu99 -Iinclude -Ilibbb  -I/home/dh7.song/src/stmp37xx/change_compiler_4.1.1/dawitsv2-sys/user/busybox-1.9.1/libbb -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG  -D"BB_VER=KBUILD_STR(1.9.1)" -DBB_BT=AUTOCONF_TIMESTAMP -D_FORTIFY_SOURCE=2  -Wall -Wshadow -Wwrite-strings -Wundef -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Os -fno-builtin-strlen -finline-limit=0 -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-guess-branch-probability -funsigned-char -static-libgcc -falign-functions=1 -falign-jumps=1 -falign-labels=1 -falign-loops=1 -Wdeclaration-after-statement -Wno-pointer-sign    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(makedev)"  -D"KBUILD_MODNAME=KBUILD_STR(makedev)" -c -o libbb/makedev.o libbb/makedev.c

deps_libbb/makedev.o := \
  libbb/makedev.c \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/features.h \
    $(wildcard include/config/c99.h) \
    $(wildcard include/config/ix.h) \
    $(wildcard include/config/ix2.h) \
    $(wildcard include/config/ix199309.h) \
    $(wildcard include/config/ix199506.h) \
    $(wildcard include/config/en.h) \
    $(wildcard include/config/en/extended.h) \
    $(wildcard include/config/x98.h) \
    $(wildcard include/config/en2k.h) \
    $(wildcard include/config/gefile.h) \
    $(wildcard include/config/gefile64.h) \
    $(wildcard include/config/e/offset64.h) \
    $(wildcard include/config/.h) \
    $(wildcard include/config/d.h) \
    $(wildcard include/config/c.h) \
    $(wildcard include/config/ntrant.h) \
    $(wildcard include/config/tify/level.h) \
    $(wildcard include/config/i.h) \
    $(wildcard include/config/ern/inlines.h) \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/predefs.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/cdefs.h \
    $(wildcard include/config/espaces.h) \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/gnu/stubs.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/sysmacros.h \

libbb/makedev.o: $(deps_libbb/makedev.o)

$(deps_libbb/makedev.o):
