#ifndef _ASM_ARCH_LRADC_H_
#define _ASM_ARCH_LRADC_H_

#define LRADC_CH_0		(0)
#define LRADC_CH_1		(1)
#define LRADC_CH_2		(2)
#define LRADC_CH_3		(3)
#define LRADC_CH_4		(4)
#define LRADC_CH_5		(5)
#define LRADC_CH_VDDIO		(6)
#define LRADC_CH_BATTERY	(7)
#define LRADC_CH_PMOS_THIN	(8)
#define LRADC_CH_NMOS_THIN	(9)
#define LRADC_CH_PMOS_THICK	(10)
#define LRADC_CH_NMOS_THICH	(11)
#define LRADC_CH_USB_DP		(12)
#define LRADC_CH_USB_DN		(13)
#define LRADC_CH_VBG		(14)
#define LRADC_CH_5V		(15)

struct lradc_ops {
	int (*init)(int slot, int channel, void *data);
	void (*deinit)(int slot, int channel, void *data);
	void (*start)(int slot, int channel, void *data);
	void (*handler)(int slot, int channel, void *data);
	void *private_data;
	unsigned int num_of_samples;
};

#ifndef __ASSEMBLY__
void enable_lradc(int channel);
void disable_lradc(int channel);
int __must_check request_lradc(int channel, const char *, struct lradc_ops *);
void free_lradc(int channel, struct lradc_ops *);
int config_lradc(int channel, int divide_by_two, int acc, int num_sample);
void reset_lradc(int channel);
void start_lradc(int channel);
int __must_check result_lradc(int channel, unsigned int *result,
		unsigned long *jiffy);
int __must_check raw_result_lradc(int channel, unsigned int *result,
		unsigned long *jiffy);

int __must_check request_lradc_delay(void);
void assign_lradc_delay(int type, int delay_slot, int id);
void free_lradc_delay(int delay_slot);
void config_lradc_delay(int delay_slot, int loop_count, int delay);
void start_lradc_delay(int delay_slot);
#endif /* __ASSEMBLY__ */

#endif /* _ASM_ARCH_LRADC_H_ */
