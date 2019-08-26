/* $Id: usb_common.h,v 1.2 2006/05/03 03:23:14 biglow Exp $ */

/**
 * \file usb_common.h
 * \brief common function of usb
 * \author Lee Tae Hun <th76.lee@samsung.com>
 * \version $Revision: 1.2 $
 * \date $Date: 2006/05/03 03:23:14 $
 *
 * This is common definitions related to USB based on STMP36xx.
 * $Log: usb_common.h,v $
 * Revision 1.2  2006/05/03 03:23:14  biglow
 * - udpate for modulization ARC USB core lib
 *
 * Revision 1.1  2005/12/31 08:13:31  biglow
 * - declare usb common functions
 *
 */

 #ifndef __USB_COMMON_H
 #define __USB_COMMON_H

#ifdef CONFIG_USB_GADGET_STMP36XX
extern unsigned long get_udc_event(void);
extern int is_usb_connected(void);
extern int check_usb_connection(void);
extern int is_usb_enabled(void);
extern int register_usbevent(pm_call_back func, int type);
extern int unregister_usbevent(pm_call_back func, int type);

extern void start_usb_clock(void);
extern void stop_usb_clock(void);
extern void set_usb_clock(int mode);
#else /* CONFIG_STMP36XX_GADGET_STMP36XX */
#define get_udc_event()					0
#define is_usb_connected()				0
#define check_usb_connection()			0
#define is_usb_enabled()				0
#define register_usbevent(func, type)	0
#define unregister_usbevent(func, type)	0

#define start_usb_clock() do { } while(0)
#define stop_usb_clock() do { } while(0)
#define set_usb_clock() do { } while(0)
#endif /* CONFIG_STMP36XX_GADGET_STMP36XX */

 #endif /* __USB_COMMON_H */

