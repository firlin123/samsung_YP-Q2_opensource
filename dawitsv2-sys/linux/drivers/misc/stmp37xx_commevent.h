/* $Id: stmp36xx_commevent.h,v 1.1 2007/01/19 06:07:10 biglow Exp $ */

/**
 * \file stmp36xx_commevent.h
 * \brief event for COMM (serial, I2S,... )
 * \author Lee Tae Hun <th76.lee@samsung.com>
 * \version $Revision: 1.1 $
 * \date $Date: 2007/01/19 06:07:10 $
 *
 * For user event server, user get system event of COMM detecting.
 *
 * $Log: stmp36xx_commevent.h,v $
 * Revision 1.1  2007/01/19 06:07:10  biglow
 * add comm event
 *
 */

#ifndef __STMP37XX_COMM_EVENT_H
#define __STMP37XX_COMM_EVENT_H

typedef enum
{
	PBA_EVENT = 0,
	BORDEAUX_EVENT,
	MAX_COMMEVENT_NUM
}comm_event_t;

#endif /* __STMP37XX_COMM_EVENT_H */

