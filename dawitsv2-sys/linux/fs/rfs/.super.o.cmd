cmd_fs/rfs/super.o := /opt/empeg/bin/armv5-empeg-linux-gnueabi-gcc -Wp,-MD,fs/rfs/.super.o.d  -nostdinc -isystem /opt/empeg/lib/gcc/armv5-empeg-linux-gnueabi/4.0.2/include -D__KERNEL__ -Iinclude  -include include/linux/autoconf.h -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Os -marm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -mabi=apcs-gnu -mno-thumb-interwork -D__LINUX_ARM_ARCH__=5 -march=armv5te -mtune=arm9tdmi  -msoft-float -Uarm -fno-omit-frame-pointer -fno-optimize-sibling-calls   -Wdeclaration-after-statement -Wno-pointer-sign   -Iinclude/linux/rfs -Iinclude/linux/ufd  -DMODULE -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(super)"  -D"KBUILD_MODNAME=KBUILD_STR(rfs)" -c -o fs/rfs/super.o fs/rfs/super.c

deps_fs/rfs/super.o := \
  fs/rfs/super.c \
    $(wildcard include/config/page/cache.h) \
    $(wildcard include/config/auto/format.h) \
    $(wildcard include/config/debug.h) \
    $(wildcard include/config/rfs/nls.h) \
    $(wildcard include/config/nls/default.h) \
  include/linux/sched.h \
    $(wildcard include/config/sched/debug.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/no/hz.h) \
    $(wildcard include/config/detect/softlockup.h) \
    $(wildcard include/config/split/ptlock/cpus.h) \
    $(wildcard include/config/keys.h) \
    $(wildcard include/config/bsd/process/acct.h) \
    $(wildcard include/config/taskstats.h) \
    $(wildcard include/config/audit.h) \
    $(wildcard include/config/inotify/user.h) \
    $(wildcard include/config/posix/mqueue.h) \
    $(wildcard include/config/fair/user/sched.h) \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/schedstats.h) \
    $(wildcard include/config/task/delay/acct.h) \
    $(wildcard include/config/fair/group/sched.h) \
    $(wildcard include/config/preempt/notifiers.h) \
    $(wildcard include/config/blk/dev/io/trace.h) \
    $(wildcard include/config/cc/stackprotector.h) \
    $(wildcard include/config/sysvipc.h) \
    $(wildcard include/config/security.h) \
    $(wildcard include/config/rt/mutexes.h) \
    $(wildcard include/config/debug/mutexes.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/task/xacct.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/cpusets.h) \
    $(wildcard include/config/cgroups.h) \
    $(wildcard include/config/futex.h) \
    $(wildcard include/config/compat.h) \
    $(wildcard include/config/fault/injection.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/preempt.h) \
  include/asm/param.h \
    $(wildcard include/config/hz.h) \
  include/linux/capability.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbd.h) \
    $(wildcard include/config/lsf.h) \
    $(wildcard include/config/resources/64bit.h) \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  include/linux/compiler.h \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc4.h \
    $(wildcard include/config/forced/inlining.h) \
  include/linux/compiler-gcc.h \
  include/asm/posix_types.h \
  include/asm/types.h \
  include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/spinlock/sleep.h) \
    $(wildcard include/config/printk.h) \
  /opt/empeg/lib/gcc/armv5-empeg-linux-gnueabi/4.0.2/include/stdarg.h \
  include/linux/linkage.h \
  include/asm/linkage.h \
  include/linux/bitops.h \
  include/asm/bitops.h \
  include/asm/system.h \
    $(wildcard include/config/cpu/cp15.h) \
    $(wildcard include/config/cpu/xsc3.h) \
    $(wildcard include/config/cpu/xscale.h) \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/sa110.h) \
  include/asm/memory.h \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/discontigmem.h) \
  include/asm/arch/memory.h \
  include/asm/sizes.h \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem.h) \
    $(wildcard include/config/out/of/line/pfn/to/page.h) \
  include/linux/stringify.h \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags/support.h) \
    $(wildcard include/config/x86.h) \
  include/asm/irqflags.h \
  include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  include/asm/hwcap.h \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/lock.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/linux/byteorder/swab.h \
  include/linux/byteorder/generic.h \
  include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/debug/bugverbose.h) \
  include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug.h) \
  include/asm/div64.h \
  include/linux/timex.h \
  include/linux/time.h \
  include/linux/cache.h \
  include/asm/cache.h \
  include/linux/seqlock.h \
  include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
  include/linux/preempt.h \
    $(wildcard include/config/debug/preempt.h) \
  include/linux/thread_info.h \
  include/asm/thread_info.h \
    $(wildcard include/config/debug/stack/usage.h) \
  include/asm/fpstate.h \
    $(wildcard include/config/iwmmxt.h) \
  include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
  include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  include/linux/poison.h \
  include/linux/prefetch.h \
  include/asm/processor.h \
  include/linux/bottom_half.h \
  include/linux/spinlock_types.h \
  include/linux/spinlock_types_up.h \
  include/linux/lockdep.h \
    $(wildcard include/config/lock/stat.h) \
    $(wildcard include/config/generic/hardirqs.h) \
    $(wildcard include/config/prove/locking.h) \
  include/linux/spinlock_up.h \
  include/linux/spinlock_api_up.h \
  include/asm/atomic.h \
  include/asm-generic/atomic.h \
  include/asm/timex.h \
  include/asm/arch/timex.h \
  include/linux/jiffies.h \
  include/linux/calc64.h \
  include/linux/rbtree.h \
  include/linux/cpumask.h \
  include/linux/bitmap.h \
  include/linux/string.h \
  include/asm/string.h \
  include/linux/errno.h \
  include/asm/errno.h \
  include/asm-generic/errno.h \
  include/asm-generic/errno-base.h \
  include/linux/nodemask.h \
    $(wildcard include/config/highmem.h) \
  include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  include/linux/mm_types.h \
  include/linux/auxvec.h \
  include/asm/auxvec.h \
  include/linux/prio_tree.h \
  include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  include/linux/rwsem-spinlock.h \
  include/linux/completion.h \
  include/linux/wait.h \
  include/asm/current.h \
  include/asm/page.h \
    $(wildcard include/config/cpu/copy/v3.h) \
    $(wildcard include/config/cpu/copy/v4wt.h) \
    $(wildcard include/config/cpu/copy/v4wb.h) \
    $(wildcard include/config/cpu/copy/v6.h) \
    $(wildcard include/config/aeabi.h) \
  include/asm/glue.h \
    $(wildcard include/config/cpu/arm610.h) \
    $(wildcard include/config/cpu/arm710.h) \
    $(wildcard include/config/cpu/abrt/lv4t.h) \
    $(wildcard include/config/cpu/abrt/ev4.h) \
    $(wildcard include/config/cpu/abrt/ev4t.h) \
    $(wildcard include/config/cpu/abrt/ev5tj.h) \
    $(wildcard include/config/cpu/abrt/ev5t.h) \
    $(wildcard include/config/cpu/abrt/ev6.h) \
    $(wildcard include/config/cpu/abrt/ev7.h) \
  include/asm-generic/page.h \
  include/asm/mmu.h \
    $(wildcard include/config/cpu/has/asid.h) \
  include/asm/semaphore.h \
  include/asm/locks.h \
  include/asm/cputime.h \
  include/asm-generic/cputime.h \
  include/linux/smp.h \
  include/linux/sem.h \
  include/linux/ipc.h \
  include/asm/ipcbuf.h \
  include/linux/kref.h \
  include/asm/sembuf.h \
  include/linux/signal.h \
  include/asm/signal.h \
  include/asm-generic/signal.h \
  include/asm/sigcontext.h \
  include/asm/siginfo.h \
  include/asm-generic/siginfo.h \
  include/linux/securebits.h \
  include/linux/fs_struct.h \
  include/linux/pid.h \
  include/linux/rcupdate.h \
  include/linux/percpu.h \
    $(wildcard include/config/modules.h) \
  include/linux/slab.h \
    $(wildcard include/config/slab/debug.h) \
    $(wildcard include/config/slub.h) \
    $(wildcard include/config/slob.h) \
    $(wildcard include/config/debug/slab.h) \
    $(wildcard include/config/slabinfo.h) \
  include/linux/gfp.h \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/zone/dma32.h) \
  include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/arch/populates/node/map.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/have/memory/present.h) \
    $(wildcard include/config/need/node/memmap/size.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/have/arch/early/pfn/to/nid.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/nodes/span/other/nodes.h) \
    $(wildcard include/config/holes/in/zone.h) \
  include/linux/init.h \
    $(wildcard include/config/hotplug.h) \
    $(wildcard include/config/acpi/hotplug/memory.h) \
  include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  include/linux/memory_hotplug.h \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
  include/linux/notifier.h \
  include/linux/mutex.h \
  include/linux/srcu.h \
  include/linux/topology.h \
    $(wildcard include/config/sched/smt.h) \
    $(wildcard include/config/sched/mc.h) \
  include/asm/topology.h \
  include/asm-generic/topology.h \
  include/linux/slab_def.h \
  include/linux/kmalloc_sizes.h \
  include/asm/percpu.h \
  include/asm-generic/percpu.h \
  include/linux/proportions.h \
  include/linux/percpu_counter.h \
  include/linux/seccomp.h \
    $(wildcard include/config/seccomp.h) \
  include/linux/futex.h \
  include/linux/rtmutex.h \
    $(wildcard include/config/debug/rt/mutexes.h) \
  include/linux/plist.h \
    $(wildcard include/config/debug/pi/list.h) \
  include/linux/param.h \
  include/linux/resource.h \
  include/asm/resource.h \
  include/asm-generic/resource.h \
  include/linux/timer.h \
    $(wildcard include/config/timer/stats.h) \
  include/linux/ktime.h \
    $(wildcard include/config/ktime/scalar.h) \
  include/linux/hrtimer.h \
    $(wildcard include/config/high/res/timers.h) \
  include/linux/task_io_accounting.h \
    $(wildcard include/config/task/io/accounting.h) \
  include/linux/kobject.h \
  include/linux/sysfs.h \
  include/linux/aio.h \
  include/linux/workqueue.h \
  include/linux/aio_abi.h \
  include/linux/uio.h \
  include/linux/unistd.h \
  include/asm/unistd.h \
    $(wildcard include/config/oabi/compat.h) \
  include/linux/module.h \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
    $(wildcard include/config/module/unload.h) \
    $(wildcard include/config/kallsyms.h) \
    $(wildcard include/config/markers.h) \
  include/linux/stat.h \
  include/asm/stat.h \
  include/linux/kmod.h \
    $(wildcard include/config/kmod.h) \
  include/linux/elf.h \
  include/linux/elf-em.h \
  include/asm/elf.h \
  include/asm/user.h \
  include/linux/moduleparam.h \
    $(wildcard include/config/alpha.h) \
    $(wildcard include/config/ia64.h) \
    $(wildcard include/config/ppc64.h) \
  include/linux/marker.h \
  include/asm/local.h \
  include/asm-generic/local.h \
  include/linux/hardirq.h \
    $(wildcard include/config/preempt/bkl.h) \
    $(wildcard include/config/virt/cpu/accounting.h) \
  include/linux/smp_lock.h \
    $(wildcard include/config/lock/kernel.h) \
  include/asm/hardirq.h \
  include/asm/irq.h \
  include/asm/arch/irqs.h \
  include/asm/arch/hardware.h \
    $(wildcard include/config/mach/arma37.h) \
  include/asm/arch/platform.h \
  include/asm/arch/stmp37xx.h \
  include/asm/arch/37xx/regsapbh.h \
  include/asm/arch/37xx/regs.h \
  include/asm/arch/37xx/regsapbx.h \
  include/asm/arch/37xx/regsaudioin.h \
  include/asm/arch/37xx/regsaudioout.h \
  include/asm/arch/37xx/regsclkctrl.h \
  include/asm/arch/37xx/regsdcp.h \
  include/asm/arch/37xx/regsdigctl.h \
  include/asm/arch/37xx/regsdram.h \
  include/asm/arch/37xx/regsdri.h \
  include/asm/arch/37xx/regsecc8.h \
  include/asm/arch/37xx/regsemi.h \
  include/asm/arch/37xx/regsgpiomon.h \
  include/asm/arch/37xx/regsgpmi.h \
  include/asm/arch/37xx/regsi2c.h \
  include/asm/arch/37xx/regsicoll.h \
  include/asm/arch/37xx/regsir.h \
  include/asm/arch/37xx/regslcdif.h \
  include/asm/arch/37xx/regslradc.h \
  include/asm/arch/37xx/regsocotp.h \
  include/asm/arch/37xx/regspinctrl.h \
  include/asm/arch/37xx/regspower.h \
  include/asm/arch/37xx/regspwm.h \
  include/asm/arch/37xx/regsrtc.h \
  include/asm/arch/37xx/regssaif.h \
  include/asm/arch/37xx/regsspdif.h \
  include/asm/arch/37xx/regsssp.h \
  include/asm/arch/37xx/regstimrot.h \
  include/asm/arch/37xx/regsuartapp.h \
  include/asm/arch/37xx/regsuartdbg.h \
  include/asm/arch/37xx/regsusbctrl.h \
  include/asm/arch/37xx/regsusbphy.h \
  include/asm/arch/arma37.h \
    $(wildcard include/config/arma37/rev/01/val.h) \
    $(wildcard include/config/arma37/rev/01.h) \
    $(wildcard include/config/arma37/rev.h) \
    $(wildcard include/config/arma/rev/01/val.h) \
    $(wildcard include/config/arma37/rev/desc.h) \
  include/asm/arch/gpio.h \
  include/asm/arch/37xx/regs.h \
  include/asm/arch/bitfield.h \
  include/asm/arch/arma37_01.h \
  include/asm/arch/gpio.h \
  include/linux/irq_cpustat.h \
  include/asm/module.h \
  include/linux/fs.h \
    $(wildcard include/config/dnotify.h) \
    $(wildcard include/config/quota.h) \
    $(wildcard include/config/inotify.h) \
    $(wildcard include/config/epoll.h) \
    $(wildcard include/config/auditsyscall.h) \
    $(wildcard include/config/block.h) \
    $(wildcard include/config/fs/xip.h) \
    $(wildcard include/config/migration.h) \
  include/linux/limits.h \
  include/linux/ioctl.h \
  include/asm/ioctl.h \
  include/asm-generic/ioctl.h \
  include/linux/kdev_t.h \
  include/linux/dcache.h \
    $(wildcard include/config/profiling.h) \
  include/linux/namei.h \
  include/linux/radix-tree.h \
  include/linux/quota.h \
  include/linux/dqblk_xfs.h \
  include/linux/dqblk_v1.h \
  include/linux/dqblk_v2.h \
  include/linux/nfs_fs_i.h \
  include/linux/nfs.h \
  include/linux/sunrpc/msg_prot.h \
  include/linux/fcntl.h \
  include/asm/fcntl.h \
  include/asm-generic/fcntl.h \
    $(wildcard include/config/64bit.h) \
  include/linux/err.h \
  include/linux/rfs_fs_i.h \
  include/linux/statfs.h \
  include/asm/statfs.h \
  include/linux/blkdev.h \
    $(wildcard include/config/blk/dev/bsg.h) \
    $(wildcard include/config/bounce.h) \
  include/linux/major.h \
  include/linux/genhd.h \
    $(wildcard include/config/fail/make/request.h) \
    $(wildcard include/config/solaris/x86/partition.h) \
    $(wildcard include/config/bsd/disklabel.h) \
    $(wildcard include/config/unixware/disklabel.h) \
    $(wildcard include/config/minix/subpartition.h) \
  include/linux/device.h \
    $(wildcard include/config/debug/devres.h) \
  include/linux/ioport.h \
  include/linux/klist.h \
  include/linux/pm.h \
    $(wildcard include/config/pm/sleep.h) \
  include/asm/device.h \
    $(wildcard include/config/dmabounce.h) \
  include/linux/pagemap.h \
  include/linux/mm.h \
    $(wildcard include/config/sysctl.h) \
    $(wildcard include/config/stack/growsup.h) \
    $(wildcard include/config/debug/vm.h) \
    $(wildcard include/config/shmem.h) \
    $(wildcard include/config/proc/fs.h) \
    $(wildcard include/config/debug/pagealloc.h) \
  include/linux/debug_locks.h \
    $(wildcard include/config/debug/locking/api/selftests.h) \
  include/linux/security.h \
    $(wildcard include/config/security/network.h) \
    $(wildcard include/config/security/network/xfrm.h) \
  include/linux/binfmts.h \
  include/linux/shm.h \
  include/asm/shmparam.h \
  include/asm/shmbuf.h \
  include/linux/msg.h \
  include/asm/msgbuf.h \
  include/linux/key.h \
  include/linux/xfrm.h \
  include/net/flow.h \
  include/linux/in6.h \
  include/asm/pgtable.h \
  include/asm-generic/4level-fixup.h \
  include/asm/proc-fns.h \
    $(wildcard include/config/cpu/32.h) \
    $(wildcard include/config/cpu/arm7tdmi.h) \
    $(wildcard include/config/cpu/arm720t.h) \
    $(wildcard include/config/cpu/arm740t.h) \
    $(wildcard include/config/cpu/arm9tdmi.h) \
    $(wildcard include/config/cpu/arm920t.h) \
    $(wildcard include/config/cpu/arm922t.h) \
    $(wildcard include/config/cpu/arm925t.h) \
    $(wildcard include/config/cpu/arm926t.h) \
    $(wildcard include/config/cpu/arm940t.h) \
    $(wildcard include/config/cpu/arm946e.h) \
    $(wildcard include/config/cpu/arm1020.h) \
    $(wildcard include/config/cpu/arm1020e.h) \
    $(wildcard include/config/cpu/arm1022.h) \
    $(wildcard include/config/cpu/arm1026.h) \
    $(wildcard include/config/cpu/v6.h) \
    $(wildcard include/config/cpu/v7.h) \
  include/asm/cpu-single.h \
  include/asm/arch/vmalloc.h \
  include/asm/pgtable-hwdef.h \
  include/asm-generic/pgtable.h \
  include/linux/page-flags.h \
    $(wildcard include/config/s390.h) \
    $(wildcard include/config/swap.h) \
  include/linux/vmstat.h \
    $(wildcard include/config/vm/event/counters.h) \
  include/linux/highmem.h \
  include/linux/uaccess.h \
  include/asm/uaccess.h \
  include/asm/cacheflush.h \
    $(wildcard include/config/cpu/cache/v3.h) \
    $(wildcard include/config/cpu/cache/v4.h) \
    $(wildcard include/config/cpu/cache/v4wb.h) \
    $(wildcard include/config/outer/cache.h) \
    $(wildcard include/config/cpu/cache/vipt.h) \
    $(wildcard include/config/cpu/cache/vivt.h) \
  include/asm/kmap_types.h \
  include/linux/backing-dev.h \
  include/linux/mempool.h \
  include/linux/bio.h \
  include/linux/ioprio.h \
  include/asm/io.h \
  include/asm/arch/io.h \
  include/linux/bsg.h \
  include/asm/scatterlist.h \
    $(wildcard include/config/debug/sg.h) \
  include/linux/elevator.h \
  include/linux/quotaops.h \
  include/linux/ctype.h \
  include/linux/rfs/fm_global.h \
  include/linux/delay.h \
  include/asm/delay.h \
  include/linux/rfs/fm_blkdev.h \
  include/linux/rfs/fm_global.h \
  include/linux/rfs/rfs_api.h \
  include/linux/rfs/rfs.h \
    $(wildcard include/config/rfs/fat32.h) \
    $(wildcard include/config/free/cluster/management.h) \
  include/linux/rfs/rfs_cache.h \
  include/linux/rfs/rfs_log.h \
  include/linux/rfs/rfs_api.h \

fs/rfs/super.o: $(deps_fs/rfs/super.o)

$(deps_fs/rfs/super.o):
