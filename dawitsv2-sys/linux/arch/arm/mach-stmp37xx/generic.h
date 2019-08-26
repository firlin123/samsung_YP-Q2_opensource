#ifndef _ARCH_ARM_MACH_STMP37XX_H_
#define _ARCH_ARM_MACH_STMP37XX_H_

/* generic.c */
extern void stmp37xx_map_io(void);

/* time.c */
extern struct sys_timer stmp37xx_timer;

/* irq.c */
extern void stmp37xx_init_irq(void);

#endif /* _ARCH_ARM_MACH_STMP37XX_H_ */
