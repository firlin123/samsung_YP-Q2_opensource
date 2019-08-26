/* $Id: stmp36xx_battery_event.h,v 1.1 2005/10/19 08:48:57 zzinho Exp $ */

/**
 * \file stmp36xx_event.h
 * \brief event for system information
 * \author 
 * \version $Revision: 1.1 $
 * \date $Date: 2005/10/19 08:48:57 $
 *
 * For user event server, user get system event 
 * such as Battery level, 5V source information.
 *
 */

#ifndef __STMP36XX_BATTER_EVENT_H
#define __STMP36XX_BATTER_EVENT_H


/* battery level event status */
#define EVENT_BATTERY_LEVEL_0 0x00
#define EVENT_BATTERY_LEVEL_1 0x01
#define EVENT_BATTERY_LEVEL_2 0x02
#define EVENT_BATTERY_LEVEL_3 0x03
#define EVENT_BATTERY_LEVEL_4 0x04
#define EVENT_BATTERY_LEVEL_5 0x05

/* battery 5v src event status */
#define EVENT_BATTERY_5V_SRC_NONE 0x06
#define EVENT_BATTERY_5V_SRC_AC 0x07
#define EVENT_BATTERY_5V_SRC_USB 0x08

/* battery 5v charging status event status */
#define EVENT_BATTERY_5V_CHARGING_ON 0x09
#define EVENT_BATTERY_5V_CHARGING_OFF 0x10
#define EVENT_BATTERY_5V_CHARGING_COMPLETE 0x11

#endif /* __STMP36XX_USB_EVENT_H */

