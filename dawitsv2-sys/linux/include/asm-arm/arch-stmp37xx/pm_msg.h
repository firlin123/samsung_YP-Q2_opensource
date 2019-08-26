/** @file pm_msg.h
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
 
#ifndef _PM_MSG_H_
#define _PM_MSG_H_


enum SS_PM_MSG
{

	SS_POWER_OFF	= 0,
	SS_IDLE		= 1,
	SS_FMT 		= 2,
	SS_AUDIO 	= 3,
	SS_AUDIO_DNSE	= 4,
	SS_WMA		= 6,
	SS_WMA_DNSE	= 7,
	SS_OGG		= 9,
	SS_OGG_DNSE	= 10,
	SS_FM           = 12, 
	SS_FM_DNSE      = 13, 
	SS_RECORDING    = 14, 
	SS_USB 		= 16,
	SS_MAX_CPU	= 19
};

#endif
