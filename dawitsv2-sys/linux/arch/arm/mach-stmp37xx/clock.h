#ifndef _ARCH_ARM_MACH_CLOCK_H_
#define _ARCH_ARM_MACH_CLOCK_H_

struct clk {
	struct list_head	list;
	struct module		*owner;
	struct clk		*parent;
	const char		*name;
	int			id;
	unsigned long		rate;
	unsigned int		delay_us;
	atomic_t		use_count;
	int			(*enable)(struct clk *);
	void			(*disable)(struct clk *);
	unsigned long		(*get_rate)(struct clk *);
	int			(*set_rate)(struct clk *, unsigned long);
	long			(*round_rate)(struct clk *, unsigned long);
	int			(*set_parent)(struct clk *, struct clk *parent);
	int			(*was_enabled)(struct clk *);

	unsigned long		autoslow_rate;
	int			(*autoslow_enable)(struct clk *);
	void			(*autoslow_disable)(struct clk *);
	unsigned long		(*get_autoslow_rate)(struct clk *);
	unsigned long		(*set_autoslow_rate)(struct clk *, unsigned long);

	/* for enable()/disable() */
	const unsigned long	_ctrl_addr;
	const unsigned int	_ctrl_mask, _ctrl_inverse;

	/* for __detect_parent(), set_parent() */
	const unsigned long	_prnt_addr;
	const unsigned int	_prnt_mask;
	
	/* for general_pll_xxx_rate  */
	const unsigned long	_param_addr, _param_mul;
	const unsigned int	_param_mask, _param_divmin, _param_divmax;
	const int		_param_shift, _param_fracpos, _param_busypos;
	struct clk		*_parent_first, *_parent_second;
};

extern int clk_autoslow_enable(struct clk *);
extern void clk_autoslow_disable(struct clk *);

extern unsigned long clk_get_autoslow_rate(struct clk *);
extern int clk_set_autoslow_rate(struct clk *, unsigned long);

extern int stmp37xx_register_clock(struct clk *clk);
extern void stmp37xx_init_clock(void);

#define _IDX_REF_CPU 0
#define _IDX_REF_EMI 1
#define _IDX_REF_IO  2
#define _IDX_REF_PIX 3
#define _IDX_REF_PLL 4
#define _IDX_CLK_P   5
#define _IDX_CLK_IROV 6

#endif /* _ARCH_ARM_MACH_CLOCK_H_ */

