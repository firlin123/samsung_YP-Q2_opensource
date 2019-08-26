#include <linux/delay.h>
#include <linux/module.h>

void pwm_change (int channel, uint32_t freq, int brightness)
{
    uint32_t clkPWM = 24*1000*1000, period = 0;
    uint16_t cdiv = 0, active, inactive;

    while (true)
    {
        period = (clkPWM / freq) - 1;
        if (period > 0xFFFF)
        {
            if (++cdiv > 7)
                break;
            clkPWM >>= 1;
        }
        else
            break;
    }
    active = 0;
    inactive = period*brightness/100;
    //printk("%ld %d %d %d\n", freq, period, active, inactive);

    BF_CS2n(PWM_ACTIVEn, channel, 
            INACTIVE, inactive, 
            ACTIVE, active);

    BF_CS5n(PWM_PERIODn, channel, 
            MATT, 0, 
            CDIV, cdiv, 
            INACTIVE_STATE, 2,  //PWM_STATE_LOW, 
            ACTIVE_STATE, 3, //PWM_STATE_HIGH, 
            PERIOD, period);
}

void pwm_on (int channel)
{
	// PWM reset
	BF_CLR(PWM_CTRL, SFTRST);
	// Make sure that we are completely out of reset before continuing.
	while (HW_PWM_CTRL.B.SFTRST);
	// Select MUX to PWM2
	HW_PINCTRL_MUXSEL4_CLR(3 << (channel * 2));

	BF_CLR(PWM_CTRL, CLKGATE);
	BF_CLR(CLKCTRL_XTAL, PWM_CLK24M_GATE);

	HW_PWM_CTRL_SET(1<<channel);
}

void pwm_off (int channel)
{
	HW_PWM_CTRL_CLR(1<<channel);
	mdelay(1);

	//HW_PINCTRL_MUXSEL4_SET(3 << (channel * 2)); /* set as gpio */

	BF_SET(CLKCTRL_XTAL, PWM_CLK24M_GATE);
	BF_SET(PWM_CTRL, CLKGATE);
}


