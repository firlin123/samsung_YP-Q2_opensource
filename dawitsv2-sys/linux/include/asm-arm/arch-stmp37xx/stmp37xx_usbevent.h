#ifndef __STMP37XX_USBEVENT_H
#define __STMP37XX_USBEVENT_H

/* By leeth, usb transferring/idle/suspend event process at 20080920 */
/* Event type */
#define USB_CONNECT_EVENT 0x01
#define USB_TRANSFER_EVENT 0x02

/* USB_CONNECT_EVENT status value */
#define USB_EVENT_DISCONNECTED	0x00
#define USB_EVENT_CONNECTED	0x01
#define USB_EVENT_SAFEREMOVE 0x02

/* USB_TRANSFER_EVENT status value */
#define USB_EVENT_IDLE 0x00
#define USB_EVENT_TRANSFER 0x01

/*
 * 'type' member should exist to support Samsung App Interface.
 * Do not change types of members'
 */
struct usbevent_s
{
	unsigned short type;	/* fixed with USB_EVENT */
	unsigned short status;			/* connected, disconnected */
};

/* By leeth, usb transferring/idle/suspend event process at 20080920 */
extern void send_usbevent_msg(struct usbevent_s *event);

#endif /* __STMP37XX_USBEVENT_H */
