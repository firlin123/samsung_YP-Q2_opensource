#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include "generic.h"
#include "clock.h"

//#define CONFIG_STMP37XX_CLOCK_DEBUG

//#define __CLK_PRINT_TREE__
#define __CLK_LOCAL_WARN__
//#define __CLK_LOCAL_DBG__

static LIST_HEAD(stmp37xx_clocks);
static DEFINE_MUTEX(clocks_mutex);

#ifdef CONFIG_STMP37XX_CLOCK_DEBUG
# define pr_dbg(x...) printk(x)
#else
# define pr_dbg(x...) do {} while (0)
#endif

#ifdef __CLK_LOCAL_WARN__
#define UARTDBG_BASE IO_ADDRESS(0x80070000) // FIXME
#define UARTDBG_LFCR
#include <asm/arch/uncompress.h>
static inline void __clk_local_printf(const char *c)
{
	while (*c != 0) {
		putc(*c);
		c++;
	}
	flush();
}
# define clk_local_printf(arg...) \
{ \
	char __buf__[128]; \
	snprintf(&__buf__[0], sizeof(__buf__),arg); \
	__clk_local_printf(&__buf__[0]); \
}
# ifdef __CLK_LOCAL_DBG__
#   undef pr_dbg
#   define pr_dbg(x...) clk_local_printf(x)
# endif /* __CLK_LOCAL_DBG__ */
#else
# define clk_local_printf(x)	do {} while (0)
#endif /* __CLK_LOCAL_WARN__ */

struct clk *clk_get(struct device *dev, const char *id)
{
	struct clk *p;
	struct clk *clk = ERR_PTR(-ENOENT);
	int idno;

	if (dev == NULL || dev->bus != &platform_bus_type)
		idno = -1;
	else
		idno = to_platform_device(dev)->id;

	pr_dbg("%s(): search idno:%d, name:%s\n", __func__, idno, id);

	mutex_lock(&clocks_mutex);

	list_for_each_entry(p, &stmp37xx_clocks, list) {
		if (p->id == idno && strcmp(id, p->name) == 0
				&& try_module_get(p->owner)) {
			clk = p;
			goto out_clk_get;
		}
	}

	if (!IS_ERR(clk))
		goto out_clk_get;

	/* check for the case where a device was supplied, but the
	 * clock that was being searched for is not device specific */

	list_for_each_entry(p, &stmp37xx_clocks, list) {
		if (p->id == -1 && strcmp(id, p->name) == 0
				&& try_module_get(p->owner)) {
			clk = p;
			break;
		}
	}

 out_clk_get:
	mutex_unlock(&clocks_mutex);
	return clk;
}

void clk_put(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL)
		return;
	pr_dbg("%s(): try to put '%s'\n", __func__, clk->name);
	module_put(clk->owner);
}

static int __clk_enable(struct clk *clk)
{
	int ret = 0;

	if (IS_ERR(clk) || clk == NULL)
		return -EINVAL;

	__clk_enable(clk->parent);

	if (!atomic_add_unless(&clk->use_count, 1, 0)) {
		atomic_inc(&clk->use_count);
		pr_dbg("%s(): try to enable '%s'\n", __func__, clk->name);
		ret = (clk->enable)(clk);
	}

	return ret;
}

int clk_enable(struct clk *clk)
{
	int ret;
	mutex_lock(&clocks_mutex);
	ret = __clk_enable(clk);
	mutex_unlock(&clocks_mutex);
	return ret;
}

static void __clk_disable(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL)
		return;

	if (atomic_add_unless(&clk->use_count, -1, 1)) {
		atomic_cmpxchg(&clk->use_count, -1, 0);
	} else {
		atomic_set(&clk->use_count, 0);
		pr_dbg("%s(): try to disable '%s'\n", __func__, clk->name);
		(clk->disable)(clk);
	}

	__clk_disable(clk->parent);
}

void clk_disable(struct clk *clk)
{
	mutex_lock(&clocks_mutex);
	__clk_disable(clk);
	mutex_unlock(&clocks_mutex);
}

int clk_autoslow_enable(struct clk *clk)
{
	int ret = 0;

	if (IS_ERR(clk) || clk == NULL)
		return -EINVAL;

	pr_dbg("%s(): try to autoslow enable '%s'\n", __func__, clk->name);

	clk_autoslow_enable(clk->parent);

	mutex_lock(&clocks_mutex);

#if 0
	/* FIXME, ToDo, */
	if ((clk->autoslow_use_count++) == 0)
		ret = (clk->autoslow_enable)(clk);
#endif

	mutex_unlock(&clocks_mutex);
	return ret;
}

void clk_autoslow_disable(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL)
		return;

	pr_dbg("%s(): try to autoslow disable '%s'\n", __func__, clk->name);

	mutex_lock(&clocks_mutex);

#if 0
	/* FIXME, ToDo, */
	if ((--clk->autoslow_use_count) == 0)
		(clk->autoslow_disable)(clk);
#endif

	mutex_unlock(&clocks_mutex);
	clk_autoslow_disable(clk->parent);
}

static unsigned long __clk_get_rate(struct clk *clk)
{
	unsigned long ret = 0;

	pr_dbg("%s(): try to get rate '%s'\n", __func__, clk?clk->name:"NULL");

	if (IS_ERR(clk) || clk == NULL) {
		ret = (unsigned long)-EINVAL;
		goto out_get_rate;
	}

	if (clk->rate != 0) {
		ret = clk->rate;
		goto out_get_rate;
	}

	if (clk->get_rate != NULL) {
		ret = (clk->get_rate)(clk);
		goto out_get_rate;
	}

	if (clk->parent != NULL) {
		ret = __clk_get_rate(clk->parent);
		goto out_get_rate;
	}

 out_get_rate:

	pr_dbg("%s(): end of all\n", __func__);
	return ret;
}

unsigned long clk_get_rate(struct clk *clk)
{
	unsigned long ret;

	if (IS_ERR(clk) || clk == NULL)
		return -EINVAL;

	pr_dbg("%s(): '%s'\n", __func__, clk->name);

	mutex_lock(&clocks_mutex);
	ret = __clk_get_rate(clk);
	mutex_unlock(&clocks_mutex);
	return ret;
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	long ret = -EINVAL;
	unsigned long flags;

	if (IS_ERR(clk) || clk == NULL)
		return -EINVAL;

	pr_dbg("%s(): try to round rate '%s'\n", __func__, clk->name);

	mutex_lock(&clocks_mutex);
	local_irq_save(flags);
	ret = (clk->round_rate)(clk, rate);
	local_irq_restore(flags);
	mutex_unlock(&clocks_mutex);

	return ret;
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret;
	unsigned long flags;

	if (IS_ERR(clk) || clk == NULL)
		return -EINVAL;

	pr_dbg("%s(): try to set rate '%s'\n", __func__, clk->name);

	mutex_lock(&clocks_mutex);
	local_irq_save(flags);
	ret = (clk->set_rate)(clk, rate);
	local_irq_restore(flags);
	mutex_unlock(&clocks_mutex);

	return ret;
}

unsigned long clk_get_autoslow_rate(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL)
		return (unsigned long)-EINVAL;

	pr_dbg("%s(): try to get autoslow rate '%s'\n", __func__, clk->name);

	if (clk->autoslow_rate != 0)
		return clk->autoslow_rate;

	if (clk->get_autoslow_rate != NULL)
		return (clk->get_autoslow_rate)(clk);

	if (clk->parent != NULL)
		return clk_get_autoslow_rate(clk->parent);

	return clk->autoslow_rate;
}

int clk_set_autoslow_rate(struct clk *clk, unsigned long rate)
{
	int ret;

	if (IS_ERR(clk) || clk == NULL)
		return -EINVAL;

	pr_dbg("%s(): try to set autoslow rate '%s'\n", __func__, clk->name);

	mutex_lock(&clocks_mutex);
	ret = (clk->set_autoslow_rate)(clk, rate);
	mutex_unlock(&clocks_mutex);

	return ret;
}

struct clk *clk_get_parent(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL)
		return ERR_PTR(-ENOENT);
	pr_dbg("%s(): try to get parent '%s'\n", __func__, clk->name);
	return clk->parent;
}

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int ret = 0;

	if (IS_ERR(clk) || clk == NULL)
		return -EINVAL;

	pr_dbg("%s(): try to set parent '%s' '%s'\n", __func__,
		clk->name, parent ? parent->name:"NULL");

	if (!clk->set_parent)
		return -EINVAL;

	if (clk->parent == parent) {
		return 0;
	}

	mutex_lock(&clocks_mutex);
	ret = (clk->set_parent)(clk, parent);
	/* FIXME */
	if (ret == 0) {
		clk->parent = parent;
	}
	mutex_unlock(&clocks_mutex);

	return ret;
}

static void __count_enabled_child(struct clk *this, int *num)
{
	struct clk *p;

	list_for_each_entry(p, &stmp37xx_clocks, list) {
		if (p->parent == this) {
			if (p->was_enabled && p->was_enabled(p))
				*num = *num + 1;
			__count_enabled_child(p, num);
		}
	}
}

int clk_change_parent(struct clk *clk, struct clk *parent)
{
	int ret = 0;
	struct clk *old_parent;
	int i, num_child = 0;

	if (IS_ERR(clk) || clk == NULL)
		return -EINVAL;

	old_parent = clk->parent;

	pr_dbg("%s(): '%s' try to change parent '%s' from '%s'\n", __func__,
		clk->name,
		parent ? parent->name:"NULL",
		old_parent ? old_parent->name:"NULL");

	if (!clk->set_parent)
		return -EINVAL;

	if (clk->parent == parent) {
		return 0;
	}

	mutex_lock(&clocks_mutex);

	/* get number of enabled child clocks of this clock */
	num_child = 0;
	__count_enabled_child(clk, &num_child);

	if (clk->was_enabled && clk->was_enabled(clk))
		num_child ++;

	pr_dbg("%s(): count = %d\n", __func__, num_child);

	/* enable new parent num_child times */
	if (parent) {
		for (i = 0; i < num_child; i++) {
			__clk_enable(parent);
		}
	}

	/* change clock */
	ret = (clk->set_parent)(clk, parent);
	if (ret == 0) {
		clk->parent = parent;
	} else {
		/* roll back */
		if (parent) {
			for (i = 0; i < num_child; i++) {
				__clk_disable(parent);
			}
		}
	}
	
	/* disable old parent num_child times */
	if ((ret == 0) && old_parent) {
		for (i = 0; i < num_child; i++) {
			__clk_disable(old_parent);
		}
	}

	mutex_unlock(&clocks_mutex);

	return ret;
}

static int clk_dummy_enable(struct clk *clk)
{
	pr_dbg("%s(): '%s'\n", __func__, clk->name);
	return 0;
}

static void clk_dummy_disable(struct clk *clk)
{
	pr_dbg("%s(): '%s'\n", __func__, clk->name);
}

static unsigned long clk_dummy_get_rate(struct clk *clk)
{
	pr_dbg("%s(): '%s'\n", __func__, clk->name);
	return 0;
}

static int clk_dummy_was_enabled(struct clk *clk)
{
	pr_dbg("%s(): '%s'\n", __func__, clk->name);
	return 1;
}

static struct clk clk_ref_xtal = {
	.name		= "ref_xtal",
	.id		= -1,
	.rate		= 24000000,
	.parent		= NULL,
};

static struct clk clk_xtal32k = {
	.name		= "xtal-rtc",
	.id		= -1,
	.rate		= 32768,
	.parent		= NULL,
};

static struct clk clk_pll = {
	.name		= "pll",
	.id		= -1,
	.rate		= 480000000,
	.parent		= &clk_ref_xtal,
/* ToDo
	.enable		= stmp37xx_pll_on,
	.disable	= stmp37xx_pll_off,
*/
};

static __init struct clk *__detect_parent(struct clk *clk)
{
	unsigned int tmp = 0;
	struct clk *parent = clk->_parent_first;

	if (clk->_prnt_mask) {
		tmp = *(volatile unsigned int*)(clk->_prnt_addr);
		if ((tmp & clk->_prnt_mask) != clk->_prnt_mask)
			parent = clk->_parent_second;
	}
	return parent;
}

static int stmp37xx_clk_general_enable(struct clk *clk)
{
	unsigned int tmp = *(volatile unsigned int*)(clk->_ctrl_addr);

	pr_dbg("%s(): '%s', [0x%08lx], 0x%08x, %d\n", __func__, clk->name,
			clk->_ctrl_addr, clk->_ctrl_mask, clk->_ctrl_inverse);

	if (clk->_ctrl_inverse) {
		tmp &= ~(clk->_ctrl_mask);
	} else {
		tmp |= (clk->_ctrl_mask);
	}
	*(volatile unsigned int*)(clk->_ctrl_addr) = tmp;
	return 0;
}

static void stmp37xx_clk_general_disable(struct clk *clk)
{
	unsigned int tmp = *(volatile unsigned int*)(clk->_ctrl_addr);

	pr_dbg("%s(): '%s', [0x%08lx], 0x%08x, %d\n", __func__, clk->name,
			clk->_ctrl_addr, clk->_ctrl_mask, clk->_ctrl_inverse);

	if (clk->_ctrl_inverse) {
		tmp &= ~(clk->_ctrl_mask);
	} else {
		tmp |= (clk->_ctrl_mask);
	}
	*(volatile unsigned int*)(clk->_ctrl_addr) = tmp;
}

static int stmp37xx_clk_general_was_enabled(struct clk *clk)
{
	unsigned int tmp = *(volatile unsigned int*)(clk->_ctrl_addr);
	int ret = 0;

	pr_dbg("%s(): '%s', [0x%08lx], 0x%08x, %d\n", __func__, clk->name,
			clk->_ctrl_addr, clk->_ctrl_mask, clk->_ctrl_inverse);

	if (clk->_ctrl_inverse) {
		ret = ~tmp & clk->_ctrl_mask;
	} else {
		ret = tmp & clk->_ctrl_mask;
	}
	
	return ret;
}

static int stmp37xx_clk_general_set_parent(struct clk *clk, struct clk *parent)
{
	unsigned int tmp = *(volatile unsigned int*)(clk->_prnt_addr);
	unsigned int tmp_1, tmp_0;

	pr_dbg("%s(): '%s'\n", __func__, clk->name);
	pr_dbg("%s(): new '%s'\n", __func__, parent ? parent->name:"NULL");

	if ((parent != clk->_parent_first) && (parent != clk->_parent_second)) {
		return -EINVAL;
	}

	pr_dbg("%s(): '%s', [0x%08lx], 0x%08x\n", __func__, clk->name,
			clk->_prnt_addr, clk->_prnt_mask);

	tmp_1 = tmp | (clk->_prnt_mask);
	tmp_0 = tmp & ~(clk->_prnt_mask);

	if (parent == clk->_parent_first) {
		*(volatile unsigned int*)(clk->_prnt_addr) = tmp_1;
	} else {
		*(volatile unsigned int*)(clk->_prnt_addr) = tmp_0;
	}

	return 0;
}

static unsigned long stmp37xx_clk_general_get_rate(struct clk *clk)
{
	unsigned int tmp = *(volatile unsigned int*)clk->_param_addr;
	unsigned long mod, base;
	unsigned long long ll;
	int shift = clk->_param_shift;
	unsigned int frac = 0;

	pr_dbg("%s(): '%s', shift %d\n", __func__, clk->name, shift);

	if (clk->_param_fracpos >= 0)
		frac = 1 << clk->_param_fracpos;

	if (tmp & frac) {
		/* ToDo */
		clk_local_printf("%s: FRAC, not supported\n", clk->name);
		BUG();
	}

	base = __clk_get_rate(clk->parent);
	mod = (tmp & clk->_param_mask) >> shift;

	ll = (unsigned long long)base * clk->_param_mul;

	do_div(ll, mod);

	pr_dbg("%s(): end of all\n", __func__);
	return (unsigned long)ll;
}

static long stmp37xx_clk_general_round_rate(struct clk *clk, unsigned long rate)
{
	unsigned int tmp = *(volatile unsigned int*)clk->_param_addr;
	unsigned long mod, base;
	unsigned long long ll;
	int shift = clk->_param_shift;
	unsigned int frac = 0, busy = 0;

	pr_dbg("%s(): '%s', shift:%d\n", __func__, clk->name, shift);

	if (clk->_param_fracpos >= 0)
		frac = 1 << clk->_param_fracpos;

	if (clk->_param_busypos >= 0)
		busy = 1 << clk->_param_busypos;

	base = __clk_get_rate(clk->parent);
	ll = (unsigned long long)base * clk->_param_mul;
	do_div(ll, rate);
	mod = ll;	
	mod = max_t(unsigned int, clk->_param_divmin, mod);
	mod = min_t(unsigned int, clk->_param_divmax, mod);

	tmp = *(volatile unsigned int*)clk->_param_addr;
	/* integer divider only */
	tmp &= ~frac;
	tmp &= ~(clk->_param_mask);
	tmp |= mod << shift;
	*(volatile unsigned int*)clk->_param_addr = tmp;

	while (*(volatile unsigned int*)clk->_param_addr & busy) {
		barrier();
	}	

	pr_dbg("%s(): end of all\n", __func__);

	return (long)stmp37xx_clk_general_get_rate(clk);
}

static int stmp37xx_clk_general_set_rate(struct clk *clk, unsigned long rate)
{
	/* ToDo
	 *   check
	 */
	stmp37xx_clk_general_round_rate(clk, rate);
	return 0;
}

static unsigned long stmp37xx_clk_p_get_rate(struct clk *clk)
{
	unsigned int tmp;
	unsigned long base, div;
	int is_frac;
	unsigned long f;

	base = __clk_get_rate(clk->parent);

	tmp = HW_CLKCTRL_CPU_RD();

	if (clk->parent == &clk_ref_xtal) {
		div = (tmp >> 16) & 0x3ff;
		is_frac = tmp & BM_CLKCTRL_CPU_DIV_XTAL_FRAC_EN;
	} else {
		div = tmp & 0x3ff;
		is_frac = tmp & BM_CLKCTRL_CPU_DIV_CPU_FRAC_EN;
	}

	if (is_frac) {
		/* ToDo */
		clk_local_printf("%s: FRAC, not supported\n", clk->name);
		BUG();
	} else {
		f = base / div;
	}

	return f;
}

static long stmp37xx_clk_p_round_rate(struct clk *clk, unsigned long rate)
{
	unsigned int tmp;
	unsigned long base = __clk_get_rate(clk->parent);
	unsigned long mod;

	pr_dbg("%s(): '%s' to %lu, base '%s'@%lu\n", __func__,
			clk->name, rate,
			clk->parent ? clk->parent->name:"NULL", base);

	tmp = HW_CLKCTRL_CPU_RD();

	/* integer divier only */
	tmp &= ~(BM_CLKCTRL_CPU_DIV_XTAL_FRAC_EN | BM_CLKCTRL_CPU_DIV_CPU_FRAC_EN);

	mod = base / rate;
	mod = min_t(unsigned long, 0x3ff, mod);
	mod = max_t(unsigned long, 1, mod);

	if (clk->parent == &clk_ref_xtal) {
		/* ref_xtal */
		tmp &= ~(0x3ff << 16);
		tmp |= (mod << 16);
	} else {
		/* ref_cpu */
		tmp &= ~(0x3ff);
		tmp |= mod;
	}

	HW_CLKCTRL_CPU_WR(tmp);

	pr_dbg("%s(): wait after\n", __func__);
	while (HW_CLKCTRL_CPU_RD() &
		(BM_CLKCTRL_CPU_BUSY_REF_XTAL | BM_CLKCTRL_CPU_BUSY_REF_CPU)) {
		barrier();
	}
		
	pr_dbg("%s(): end of all\n", __func__);

	return (long)stmp37xx_clk_p_get_rate(clk);
}

static int stmp37xx_clk_p_set_rate(struct clk *clk, unsigned long rate)
{
	/* ToDo
	 *  check
	 */
	stmp37xx_clk_p_round_rate(clk, rate);	
	return 0;
}

static unsigned long stmp37xx_clk_emi_get_rate(struct clk *clk)
{
	unsigned int tmp;
	unsigned long base, div;
	unsigned long f;

	base = __clk_get_rate(clk->parent);

	tmp = HW_CLKCTRL_EMI_RD();

	if (clk->parent == &clk_ref_xtal) {
		div = (tmp >> 8) & 0xf;
	} else {
		div = tmp & 0x3f;
	}

	f = base / div;

	return f;
}

static long stmp37xx_clk_emi_round_rate(struct clk *clk, unsigned long rate)
{
	unsigned int tmp;
	unsigned long base = __clk_get_rate(clk->parent);
	unsigned long mod;

	pr_dbg("%s(): '%s' to %lu, base '%s'@%lu\n", __func__,
			clk->name, rate,
			clk->parent ? clk->parent->name:"NULL", base);

	tmp = HW_CLKCTRL_EMI_RD();

	mod = base / rate;
	mod = max_t(unsigned long, 1, mod);

	if (clk->parent == &clk_ref_xtal) {
		/* ref_xtal */
		mod = min_t(unsigned long, 0xf, mod);
		tmp &= ~(0xf << 8);
		tmp |= (mod << 8);
	} else {
		/* ref_cpu */
		mod = min_t(unsigned long, 0x3f, mod);
		tmp &= ~(0x3f);
		tmp |= mod;
	}

	HW_CLKCTRL_EMI_WR(tmp);

	pr_dbg("%s(): wait after\n", __func__);
	while (HW_CLKCTRL_EMI_RD() &
		(BM_CLKCTRL_EMI_BUSY_REF_XTAL | BM_CLKCTRL_EMI_BUSY_REF_EMI)) {
		barrier();
	}
		
	pr_dbg("%s(): end of all\n", __func__);

	return (long)stmp37xx_clk_emi_get_rate(clk);
}

static int stmp37xx_clk_emi_set_rate(struct clk *clk, unsigned long rate)
{
	/* ToDo
	 *  check
	 */
	stmp37xx_clk_emi_round_rate(clk, rate);	
	return 0;
}

static struct clk stmp37xx_dev_clks[] = {
	/* first step */
	[_IDX_REF_CPU] = {
		.name		= "ref_cpu",
		.id		= -1,
		.parent		= &clk_pll,

/* for xxx_rate() */
		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_FRAC_ADDR,
		._param_mask	= BM_CLKCTRL_FRAC_CPUFRAC,
		._param_shift	= BP_CLKCTRL_FRAC_CPUFRAC,
		._param_mul	= 18,
		._param_divmin	= 18,
		._param_divmax	= 35,
		._param_fracpos	= -1,
		._param_busypos = -1,

/* for enable()/disable()
 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
*/
//		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_FRAC_ADDR,
		._ctrl_mask	= BM_CLKCTRL_FRAC_CLKGATECPU,
		._ctrl_inverse	= 1,
	},
	[_IDX_REF_EMI] = {
		.name		= "ref_emi",
		.id		= -1,
		.parent		= &clk_pll,

/* for xxx_rate() */
		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_FRAC_ADDR,
		._param_mask	= BM_CLKCTRL_FRAC_EMIFRAC,
		._param_shift	= BP_CLKCTRL_FRAC_EMIFRAC,
		._param_mul	= 18,
		._param_divmin	= 18,
		._param_divmax	= 35,
		._param_fracpos	= -1,
		._param_busypos = -1,

/* for enable()/disable() */
 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
//		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_FRAC_ADDR,
		._ctrl_mask	= BM_CLKCTRL_FRAC_CLKGATEEMI,
		._ctrl_inverse	= 1,
	},
	[_IDX_REF_IO] = {
		.name		= "ref_io",
		.id		= -1,
		.parent		= &clk_pll,

/* for xxx_rate() */
		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_FRAC_ADDR,
		._param_mask	= BM_CLKCTRL_FRAC_IOFRAC,
		._param_shift	= BP_CLKCTRL_FRAC_IOFRAC,
		._param_mul	= 18,
		._param_divmin	= 18,
		._param_divmax	= 35,
		._param_fracpos	= -1,
		._param_busypos = -1,

/* for enable()/disable() */
 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
//		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_FRAC_ADDR,
		._ctrl_mask	= BM_CLKCTRL_FRAC_CLKGATEIO,
		._ctrl_inverse	= 1,
	},
	[_IDX_REF_PIX] = {
		.name		= "ref_pix",
		.id		= -1,
		.parent		= &clk_pll,

/* for xxx_rate() */
		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_FRAC_ADDR,
		._param_mask	= BM_CLKCTRL_FRAC_PIXFRAC,
		._param_shift	= BP_CLKCTRL_FRAC_PIXFRAC,
		._param_mul	= 18,
		._param_divmin	= 18,
		._param_divmax	= 35,
		._param_fracpos	= -1,
		._param_busypos = -1,

/* for enable()/disable() */
 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
//		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_FRAC_ADDR,
		._ctrl_mask	= BM_CLKCTRL_FRAC_CLKGATEPIX,
		._ctrl_inverse	= 1,
	},
	[_IDX_REF_PLL] = {
		.name		= "ref_pll",
		.id		= -1,
		.parent		= &clk_pll,
	},

	/* second step */
	[_IDX_CLK_P] = {
		.name		= "clk_p",
		.id		= -1,

		.set_rate	= stmp37xx_clk_p_set_rate,
		.round_rate	= stmp37xx_clk_p_round_rate,
		.get_rate	= stmp37xx_clk_p_get_rate,

		.was_enabled	= clk_dummy_was_enabled,

/* for __detect_parent(), genral_set_parent() */
		.set_parent	= stmp37xx_clk_general_set_parent,
		._prnt_addr	= HW_CLKCTRL_CLKSEQ_ADDR,
		._prnt_mask	= BM_CLKCTRL_CLKSEQ_BYPASS_CPU,
		._parent_first	= &clk_ref_xtal,
		._parent_second	= &stmp37xx_dev_clks[_IDX_REF_CPU],
	},
	[_IDX_CLK_IROV] = {
		.name		= "clk_irov",
		.id		= -1,
/* ToDo
 * 		autodiv
*/
		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_IR_ADDR,
		._param_mask	= BM_CLKCTRL_IR_IROV_DIV,
		._param_shift	= BP_CLKCTRL_IR_IROV_DIV,
		._param_mul	= 1,
		._param_divmin	= 1,
		._param_divmax	= 0x1ff,
		._param_fracpos	= -1,
		._param_busypos = BP_CLKCTRL_IR_IROV_BUSY,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_IR_ADDR,
		._ctrl_mask	= BM_CLKCTRL_IR_CLKGATE,
		._ctrl_inverse	= 1,

/* for __detect_parent(), genral_set_parent() */
		.set_parent	= stmp37xx_clk_general_set_parent,
		._prnt_addr	= HW_CLKCTRL_CLKSEQ_ADDR,
		._prnt_mask	= BM_CLKCTRL_CLKSEQ_BYPASS_IR,
		._parent_first	= &clk_ref_xtal,
		._parent_second	= &stmp37xx_dev_clks[_IDX_REF_IO],
	},
	{
		.name		= "clk_32k",
		.id		= -1,
		.rate		= 32000,
		.parent		= &clk_ref_xtal,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_XTAL_ADDR,
		._ctrl_mask	= BM_CLKCTRL_XTAL_TIMROT_CLK32K_GATE,
		._ctrl_inverse	= 1,
	},
	{
		.name		= "clk_pcmspdif",
		.id		= -1,
		.parent		= &stmp37xx_dev_clks[_IDX_REF_PLL],

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_SPDIF_ADDR,
		._ctrl_mask	= BM_CLKCTRL_SPDIF_CLKGATE,
		._ctrl_inverse	= 1,
	},
	{
		.name		= "clk_uart",
		.id		= -1,
		.parent		= &clk_ref_xtal,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_XTAL_ADDR,
		._ctrl_mask	= BM_CLKCTRL_XTAL_UART_CLK_GATE,
		._ctrl_inverse	= 1,
	},
	{
		.name		= "clk_filt24m",
		.id		= -1,
		.parent		= &clk_ref_xtal,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_XTAL_ADDR,
		._ctrl_mask	= BM_CLKCTRL_XTAL_FILT_CLK24M_GATE,
		._ctrl_inverse	= 1,
	},
	{
		.name		= "clk_pwm24m",
		.id		= -1,
		.parent		= &clk_ref_xtal,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_XTAL_ADDR,
		._ctrl_mask	= BM_CLKCTRL_XTAL_PWM_CLK24M_GATE,
		._ctrl_inverse	= 1,
	},
	{
		.name		= "clk_dri24m",
		.id		= -1,
		.parent		= &clk_ref_xtal,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_XTAL_ADDR,
		._ctrl_mask	= BM_CLKCTRL_XTAL_DRI_CLK24M_GATE,
		._ctrl_inverse	= 1,
	},
	{
		.name		= "clk_1m",
		.id		= -1,
		.rate		= 1000000,
		.parent		= &clk_ref_xtal,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_XTAL_ADDR,
		._ctrl_mask	= BM_CLKCTRL_XTAL_DIGCTRL_CLK1M_GATE,
		._ctrl_inverse	= 1,
	},
	{
		.name		= "clk_x",
		.id		= -1,
		.parent		= &clk_ref_xtal,

		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_XBUS_ADDR,
		._param_mask	= BM_CLKCTRL_XBUS_DIV,
		._param_shift	= BP_CLKCTRL_XBUS_DIV,
		._param_mul	= 1,
		._param_divmin	= 1,
		._param_divmax	= 0x3ff,
		._param_fracpos	= BP_CLKCTRL_XBUS_DIV_FRAC_EN,
		._param_busypos = BP_CLKCTRL_XBUS_BUSY,

		.was_enabled	= clk_dummy_was_enabled,
	},
	{
		.name		= "clk_ana24m",
		.id		= -1,
		.parent		= &clk_ref_xtal,
/* ToDo
		.enable		= stmp37xx_clk_ana24m_enable,
		.disable	= stmp37xx_clk_ana24m_disable,
*/
	},
	{
		.name		= "clk_rtc",
		.id		= -1,
/* ToDo
		.get_rate	= stmp37xx_clk_rtc_get_rate,
		.set_parent	= stmp37xx_clk_rtc_set_parent,
*/

/* for __detect_parent(), genral_set_parent() */
		.set_parent	= stmp37xx_clk_general_set_parent,
		._prnt_addr	= HW_RTC_PERSISTENT0_ADDR,
		._prnt_mask	= BM_RTC_PERSISTENT0_CLOCKSOURCE,
		._parent_first	= &clk_xtal32k,
		._parent_second	= &clk_ref_xtal,
	},

	/* third step */
	{
		.name		= "clk_h",
		.id		= -1,
		.parent		= &stmp37xx_dev_clks[_IDX_CLK_P],

		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_HBUS_ADDR,
		._param_mask	= BM_CLKCTRL_HBUS_DIV,
		._param_shift	= BP_CLKCTRL_HBUS_DIV,
		._param_mul	= 1,
		._param_divmin	= 1,
		._param_divmax	= 0x1f,
		._param_fracpos	= BP_CLKCTRL_HBUS_DIV_FRAC_EN,
		._param_busypos = BP_CLKCTRL_HBUS_BUSY,

		.was_enabled	= clk_dummy_was_enabled,
	},
	{
		.name		= "clk_emi",
		.id		= -1,

		.set_rate	= stmp37xx_clk_emi_set_rate,
		.round_rate	= stmp37xx_clk_emi_round_rate,
		.get_rate	= stmp37xx_clk_emi_get_rate,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_EMI_ADDR,
		._ctrl_mask	= BM_CLKCTRL_EMI_CLKGATE,
		._ctrl_inverse	= 1,

/* for __detect_parent(), genral_set_parent() */
		.set_parent	= stmp37xx_clk_general_set_parent,
		._prnt_addr	= HW_CLKCTRL_CLKSEQ_ADDR,
		._prnt_mask	= BM_CLKCTRL_CLKSEQ_BYPASS_EMI,
		._parent_first	= &clk_ref_xtal,
		._parent_second	= &stmp37xx_dev_clks[_IDX_REF_EMI],
	},
	{
		.name		= "clk_ssp",
		.id		= -1,

		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_SSP_ADDR,
		._param_mask	= BM_CLKCTRL_SSP_DIV,
		._param_shift	= BP_CLKCTRL_SSP_DIV,
		._param_mul	= 1,
		._param_divmin	= 1,
		._param_divmax	= 0x1ff,
		._param_fracpos	= BP_CLKCTRL_SSP_DIV_FRAC_EN,
		._param_busypos = BP_CLKCTRL_SSP_BUSY,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_SSP_ADDR,
		._ctrl_mask	= BM_CLKCTRL_SSP_CLKGATE,
		._ctrl_inverse	= 1,

		.set_parent	= stmp37xx_clk_general_set_parent,
		._prnt_addr	= HW_CLKCTRL_CLKSEQ_ADDR,
		._prnt_mask	= BM_CLKCTRL_CLKSEQ_BYPASS_SSP,
		._parent_first	= &clk_ref_xtal,
		._parent_second	= &stmp37xx_dev_clks[_IDX_REF_IO],
	},
	{
		.name		= "clk_gpmi",
		.id		= -1,

		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_GPMI_ADDR,
		._param_mask	= BM_CLKCTRL_GPMI_DIV,
		._param_shift	= BP_CLKCTRL_GPMI_DIV,
		._param_mul	= 1,
		._param_divmin	= 1,
		._param_divmax	= 0x3ff,
		._param_fracpos	= BP_CLKCTRL_GPMI_DIV_FRAC_EN,
		._param_busypos = BP_CLKCTRL_GPMI_BUSY,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_GPMI_ADDR,
		._ctrl_mask	= BM_CLKCTRL_GPMI_CLKGATE,
		._ctrl_inverse	= 1,

		.set_parent	= stmp37xx_clk_general_set_parent,
		._prnt_addr	= HW_CLKCTRL_CLKSEQ_ADDR,
		._prnt_mask	= BM_CLKCTRL_CLKSEQ_BYPASS_GPMI,
		._parent_first	= &clk_ref_xtal,
		._parent_second	= &stmp37xx_dev_clks[_IDX_REF_IO],
	},
	{
		.name		= "clk_adc",
		.id		= -1,
		.rate		= 2000,
		.parent		= &clk_ref_xtal,
/* ToDo
		.enable		= stmp37xx_clk_adc_enable,
		.disable	= stmp37xx_clk_adc_disable,
*/
	},

	/* forth step */
	{
/* ToDo	
 *  autodiv
 */
		.name		= "clk_ir",
		.id		= -1,
		.parent		= &stmp37xx_dev_clks[_IDX_CLK_IROV],

		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_IR_ADDR,
		._param_mask	= BM_CLKCTRL_IR_IR_DIV,
		._param_shift	= BP_CLKCTRL_IR_IR_DIV,
		._param_mul	= 1,
		._param_divmin	= 1,
		._param_divmax	= 0x3ff,
		._param_fracpos	= -1,
		._param_busypos = BP_CLKCTRL_IR_IR_BUSY,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_IR_ADDR,
		._ctrl_mask	= BM_CLKCTRL_IR_CLKGATE,
		._ctrl_inverse	= 1,
	},
	{	
		.name		= "clk_pix",
		.id		= -1,

		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_PIX_ADDR,
		._param_mask	= BM_CLKCTRL_PIX_DIV,
		._param_shift	= BP_CLKCTRL_PIX_DIV,
		._param_mul	= 1,
		._param_divmin	= 1,
		._param_divmax	= 0x7fff,
		._param_fracpos	= BP_CLKCTRL_PIX_DIV_FRAC_EN,
		._param_busypos = BP_CLKCTRL_PIX_BUSY,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_PIX_ADDR,
		._ctrl_mask	= BM_CLKCTRL_PIX_CLKGATE,
		._ctrl_inverse	= 1,

		.set_parent	= stmp37xx_clk_general_set_parent,
		._prnt_addr	= HW_CLKCTRL_CLKSEQ_ADDR,
		._prnt_mask	= BM_CLKCTRL_CLKSEQ_BYPASS_PIX,
		._parent_first	= &clk_ref_xtal,
		._parent_second	= &stmp37xx_dev_clks[_IDX_REF_PIX],
	},
	{
		.name		= "clk_saif",
		.id		= -1,

		.set_rate	= stmp37xx_clk_general_set_rate,
		.round_rate	= stmp37xx_clk_general_round_rate,
		.get_rate	= stmp37xx_clk_general_get_rate,
		._param_addr	= HW_CLKCTRL_SAIF_ADDR,
		._param_mask	= BM_CLKCTRL_SAIF_DIV,
		._param_shift	= BP_CLKCTRL_SAIF_DIV,
		._param_mul	= 1,
		._param_divmin	= 1,
		._param_divmax	= 0xffff,
		._param_fracpos	= BP_CLKCTRL_SAIF_DIV_FRAC_EN,
		._param_busypos = BP_CLKCTRL_SAIF_BUSY,

 		.enable		= stmp37xx_clk_general_enable,
 		.disable	= stmp37xx_clk_general_disable,
		.was_enabled	= stmp37xx_clk_general_was_enabled,
		._ctrl_addr	= HW_CLKCTRL_SAIF_ADDR,
		._ctrl_mask	= BM_CLKCTRL_SAIF_CLKGATE,
		._ctrl_inverse	= 1,

		.set_parent	= stmp37xx_clk_general_set_parent,
		._prnt_addr	= HW_CLKCTRL_CLKSEQ_ADDR,
		._prnt_mask	= BM_CLKCTRL_CLKSEQ_BYPASS_SAIF,
		._parent_first	= &clk_ref_xtal,
		._parent_second	= &stmp37xx_dev_clks[_IDX_REF_PLL],
	},


	/* ToDo: all */
};

static int __stmp37xx_exist_clock(struct clk *clk)
{
	struct clk *p;
	list_for_each_entry(p, &stmp37xx_clocks, list) {
		if (!strcmp(clk->name, p->name))
			return -EEXIST;
	}
	return 0;
}

static inline int __stmp37xx_register_clock(struct clk *clk)
{
	INIT_LIST_HEAD(&clk->list);

	if (!clk->enable)
		clk->enable = clk_dummy_enable;
	if (!clk->disable)
		clk->disable = clk_dummy_disable;
	if (!clk->autoslow_enable)
		clk->autoslow_enable = clk_dummy_enable;
	if (!clk->autoslow_disable)
		clk->autoslow_disable = clk_dummy_disable;

	if (!clk->rate && !clk->parent && !clk->get_rate) {
		printk("%s(): warn: '%s': cannot get rate.\n", __func__,
				clk->name);
		clk->get_rate = clk_dummy_get_rate;
	}

	list_add(&clk->list, &stmp37xx_clocks);
	return 0;
}

int clk_register(struct clk *clk)
{
	int ret;
	mutex_lock(&clocks_mutex);
	if (!(ret = __stmp37xx_exist_clock(clk))) {
		ret = __stmp37xx_register_clock(clk);
	}
	mutex_unlock(&clocks_mutex);
	return ret;
}

/* at boot time */
static void __init __clk_check_init_state(void)
{
	struct clk *p;

	list_for_each_entry(p, &stmp37xx_clocks, list) {
		if (p->was_enabled && p->was_enabled(p)) {
			__clk_enable(p);
		}
	}
}

#ifdef __CLK_PRINT_TREE__
static int __tree_depth = 0;
static void __clk_print_sub_tree(struct clk *parent)
{
	struct clk *p;
	int i;
	list_for_each_entry(p, &stmp37xx_clocks, list) {
		if (p->parent == parent) {
			for(i=1;i<__tree_depth;i++) printk("%8s"," ");
			printk("     +-> %-10s (%d) %lu Hz\n", p->name,
					atomic_read(&p->use_count),
					__clk_get_rate(p));
			__tree_depth++;
			__clk_print_sub_tree(p);
			__tree_depth--;
		}
	}
}

void clk_print_tree(void)
{
	struct clk *p;
	mutex_lock(&clocks_mutex);
	list_for_each_entry(p, &stmp37xx_clocks, list) {
		if (p->parent == NULL) {
			printk("%-10s (%d) %lu Hz\n", p->name,
					atomic_read(&p->use_count),
					__clk_get_rate(p));
			__tree_depth++;
			__clk_print_sub_tree(p);
			__tree_depth--;
		}
	}
	mutex_unlock(&clocks_mutex);
}
EXPORT_SYMBOL(clk_print_tree);
#endif /* __CLK_PRINT_TREE__ */

void __init stmp37xx_init_clock(void)
{
	int i;
	struct clk *clk;

	/* Turn off auto-slow and other tricks */
	HW_CLKCTRL_HBUS_CLR(0x07f00000U);

	mutex_lock(&clocks_mutex);

	for (i = 0; i < ARRAY_SIZE(stmp37xx_dev_clks); i++) {
		clk = &stmp37xx_dev_clks[i];
		atomic_set(&clk->use_count, 0);
		if (clk->_prnt_mask) {
			clk->parent = __detect_parent(clk);
			pr_dbg("%s(): parent of '%s' is '%s'\n", __func__,
					clk->name, clk->parent->name);
		}
	}

	atomic_set(&clk_ref_xtal.use_count, 0);
	atomic_set(&clk_pll.use_count, 0);
	atomic_set(&clk_xtal32k.use_count, 0);

	__stmp37xx_register_clock(&clk_ref_xtal);
	__stmp37xx_register_clock(&clk_pll);
	__stmp37xx_register_clock(&clk_xtal32k);

	for (i = 0; i < ARRAY_SIZE(stmp37xx_dev_clks); i++) {
		clk = &stmp37xx_dev_clks[i];
		__stmp37xx_register_clock(clk);
	}

	__clk_check_init_state();

	mutex_unlock(&clocks_mutex);

#if 0
/* clock test */
  {
	struct clk *clk2, *clk3;

	/* change clock rate */

	clk = clk_get(NULL, "ref_cpu");
	if (IS_ERR(clk)) {
		printk("!ref_cpu\n");
	}
	clk_round_rate(clk, 360000000);

	clk2 = clk_get(NULL, "clk_p");
	if (IS_ERR(clk2)) {
		printk("!clk_p\n");
	}
	clk_set_parent(clk2, clk);
	printk("clk_p: %ld\n", clk_round_rate(clk2, 360000000));
	clk_put(clk2);
	clk_put(clk);

	clk = clk_get(NULL, "clk_h");
	if (IS_ERR(clk)) {
		printk("!clk_h\n");
	}
	printk("clk_h: %ld\n", clk_round_rate(clk, 110000000));
	clk_put(clk);

	/* change parent */

#ifdef __CLK_PRINT_TREE__
	clk_print_tree();
#endif
	clk = clk_get(NULL, "ref_xtal");
	if (IS_ERR(clk)) {
		printk("!ref_xtal\n");
	}
	clk2 = clk_get(NULL, "clk_p");
	clk3 = clk_get_parent(clk2);
# if 0
	clk_enable(clk);
	clk_set_parent(clk2, clk);
	clk_disable(clk3);
# else
	clk_change_parent(clk2, clk);
# endif
	printk("clk_p: %lu\n", clk_get_rate(clk2));
	clk_put(clk);
	clk_put(clk2);
	clk_put(clk3);
  }
#endif

#ifdef __CLK_PRINT_TREE__
	clk_print_tree();
#endif
	
	clk = clk_get(NULL, "clk_p");
	printk("CLK_P: %lu\n", clk_get_rate(clk));
	clk_put(clk);

	clk = clk_get(NULL, "clk_h");
	printk("CLK_H: %lu\n", clk_get_rate(clk));
	clk_put(clk);
}

/* implementation of linux/clk.h */
EXPORT_SYMBOL(clk_get);
EXPORT_SYMBOL(clk_put);
EXPORT_SYMBOL(clk_enable);
EXPORT_SYMBOL(clk_disable);
EXPORT_SYMBOL(clk_get_rate);
EXPORT_SYMBOL(clk_round_rate);
EXPORT_SYMBOL(clk_set_rate);
EXPORT_SYMBOL(clk_get_parent);
EXPORT_SYMBOL(clk_set_parent);

/* stmp37xx private */
EXPORT_SYMBOL(clk_change_parent);
EXPORT_SYMBOL(clk_autoslow_enable);
EXPORT_SYMBOL(clk_autoslow_disable);
EXPORT_SYMBOL(clk_get_autoslow_rate);
EXPORT_SYMBOL(clk_set_autoslow_rate);
EXPORT_SYMBOL(clk_register);
