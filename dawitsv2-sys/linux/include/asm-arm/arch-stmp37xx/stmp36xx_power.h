/** @file stmp36xx_power.h
 * @brief <b>%Project Template: STMP36XX Power Control</b>
 * Copyright (C) 2005 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This document is the property of Samsung Electronics Co., Ltd.
 * It is considered confidential and proprietary.
 *
 * This document may not be reproduced or transmitted in any form, 
 * in whole or in part, without the express written permission of
 * Samsung Electronics Co., Ltd. 
 *
 * @b Description: General Power control for Power management
 */
 
#ifndef _SMTP36XX_POWER_H_
#define _SMTP36XX_POWER_H_

#include "stmp37xx_pm.h"


extern unsigned int is_usb_cable(void);
extern int get_vddd_value_from_clk(int cpu, int hbus, bool usb);

extern power_err_t apply_pw_policy(int mode);
extern power_err_t set_core_level(int core_step);

// added by hcyun to handle volume max + ac conencted hang problem. 
extern power_err_t set_io_level(int io_step);
extern int get_vdd_value(int part);
extern void set_charging_power(unsigned int val);
extern int is_ac_connected(void); // hcyun 

extern void enableDCDCconverter(void); //jinho.lim
extern void disableDCDCconverter(void); //jinho.lim
#endif // _SMTP36XX_POWER_H_



