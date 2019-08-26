cmd_coreutils/wc.o := arm-none-linux-gnueabi-gcc -Wp,-MD,coreutils/.wc.o.d   -std=gnu99 -Iinclude -Ilibbb  -I/home/dh7.song/src/stmp37xx/change_compiler_4.1.1/dawitsv2-sys/user/busybox-1.9.1/libbb -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG  -D"BB_VER=KBUILD_STR(1.9.1)" -DBB_BT=AUTOCONF_TIMESTAMP -D_FORTIFY_SOURCE=2  -Wall -Wshadow -Wwrite-strings -Wundef -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Os -fno-builtin-strlen -finline-limit=0 -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-guess-branch-probability -funsigned-char -static-libgcc -falign-functions=1 -falign-jumps=1 -falign-labels=1 -falign-loops=1 -Wdeclaration-after-statement -Wno-pointer-sign    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(wc)"  -D"KBUILD_MODNAME=KBUILD_STR(wc)" -c -o coreutils/wc.o coreutils/wc.c

deps_coreutils/wc.o := \
  coreutils/wc.c \
    $(wildcard include/config/locale/support.h) \
    $(wildcard include/config/feature/wc/large.h) \
  include/libbb.h \
    $(wildcard include/config/selinux.h) \
    $(wildcard include/config/feature/shadowpasswds.h) \
    $(wildcard include/config/lfs.h) \
    $(wildcard include/config/feature/buffers/go/on/stack.h) \
    $(wildcard include/config/buffer.h) \
    $(wildcard include/config/ubuffer.h) \
    $(wildcard include/config/feature/buffers/go/in/bss.h) \
    $(wildcard include/config/feature/ipv6.h) \
    $(wildcard include/config/ture/ipv6.h) \
    $(wildcard include/config/feature/prefer/applets.h) \
    $(wildcard include/config/busybox/exec/path.h) \
    $(wildcard include/config/getopt/long.h) \
    $(wildcard include/config/feature/pidfile.h) \
    $(wildcard include/config/feature/syslog.h) \
    $(wildcard include/config/feature/individual.h) \
    $(wildcard include/config/route.h) \
    $(wildcard include/config/gunzip.h) \
    $(wildcard include/config/bunzip2.h) \
    $(wildcard include/config/ktop.h) \
    $(wildcard include/config/ioctl/hex2str/error.h) \
    $(wildcard include/config/feature/editing.h) \
    $(wildcard include/config/feature/editing/history.h) \
    $(wildcard include/config/ture/editing/savehistory.h) \
    $(wildcard include/config/feature/editing/savehistory.h) \
    $(wildcard include/config/feature/tab/completion.h) \
    $(wildcard include/config/feature/username/completion.h) \
    $(wildcard include/config/feature/editing/vi.h) \
    $(wildcard include/config/inux.h) \
    $(wildcard include/config/feature/topmem.h) \
    $(wildcard include/config/pgrep.h) \
    $(wildcard include/config/pkill.h) \
    $(wildcard include/config/feature/devfs.h) \
  include/platform.h \
    $(wildcard include/config/werror.h) \
    $(wildcard include/config///.h) \
    $(wildcard include/config//nommu.h) \
    $(wildcard include/config//mmu.h) \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/byteswap.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/byteswap.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/endian.h \
    $(wildcard include/config/.h) \
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
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/endian.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/arpa/inet.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/netinet/in.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/stdint.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/wchar.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/wordsize.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/socket.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/uio.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/types.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/types.h \
  /opt/codesourcery/lib/gcc/arm-none-linux-gnueabi/4.1.1/include/stddef.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/typesizes.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/time.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/select.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/select.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/sigset.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/time.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/sysmacros.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/pthreadtypes.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/uio.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/socket.h \
  /opt/codesourcery/lib/gcc/arm-none-linux-gnueabi/4.1.1/include/limits.h \
  /opt/codesourcery/lib/gcc/arm-none-linux-gnueabi/4.1.1/include/syslimits.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/limits.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/posix1_lim.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/local_lim.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/linux/limits.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/posix2_lim.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/xopen_lim.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/stdio_lim.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/sockaddr.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/asm/socket.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/asm/sockios.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/in.h \
  /opt/codesourcery/lib/gcc/arm-none-linux-gnueabi/4.1.1/include/stdbool.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/mount.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/ioctl.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/ioctls.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/asm/ioctls.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/asm/ioctl.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/asm-generic/ioctl.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/ioctl-types.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/ttydefaults.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/ctype.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/xlocale.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/dirent.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/dirent.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/errno.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/errno.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/linux/errno.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/asm/errno.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/asm-generic/errno.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/asm-generic/errno-base.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/fcntl.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/fcntl.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/stat.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/stat.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/inttypes.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/mntent.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/stdio.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/paths.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/netdb.h \
    $(wildcard include/config/3/ascii/rules.h) \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/rpc/netdb.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/siginfo.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/netdb.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/setjmp.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/setjmp.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/signal.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/signum.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/sigaction.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/sigcontext.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/asm/sigcontext.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/sigstack.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/ucontext.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/procfs.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/time.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/user.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/sigthread.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/libio.h \
    $(wildcard include/config/a.h) \
    $(wildcard include/config/ar/t.h) \
    $(wildcard include/config//io/file.h) \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/_G_config.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/wchar.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/gconv.h \
  /opt/codesourcery/lib/gcc/arm-none-linux-gnueabi/4.1.1/include/stdarg.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/sys_errlist.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/stdio2.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/stdlib.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/waitflags.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/waitstatus.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/alloca.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/string.h \
    $(wildcard include/config/ing/inlines.h) \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/string3.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/poll.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/poll.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/mman.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/mman.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/statfs.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/statfs.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/wait.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/resource.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/resource.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/termios.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/termios.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/unistd.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/posix_opt.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/environments.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/bits/confname.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/getopt.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/utime.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/sys/param.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/linux/param.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/asm/param.h \
  include/pwd_.h \
    $(wildcard include/config/use/bb/pwd/grp.h) \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/pwd.h \
  include/grp_.h \
  /opt/codesourcery/arm-none-linux-gnueabi/libc/usr/include/grp.h \
  include/xatonum.h \

coreutils/wc.o: $(deps_coreutils/wc.o)

$(deps_coreutils/wc.o):
