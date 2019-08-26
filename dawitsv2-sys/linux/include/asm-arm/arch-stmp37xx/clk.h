#ifndef _ASM_ARCH_CLK_H_
#define _ASM_ARCH_CLK_H_

#include <linux/clk.h>

int clk_change_parent(struct clk *clk, struct clk *parent);
int clk_autoslow_enable(struct clk *clk);
void clk_autoslow_disable(struct clk *clk);
unsigned long clk_get_autoslow_rate(struct clk *clk);
int clk_set_autoslow_rate(struct clk *clk, unsigned long rate);
int clk_register(struct clk *clk);

#endif /* _ASM_ARCH_CLK_H_ */

