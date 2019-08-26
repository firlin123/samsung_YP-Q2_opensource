/* Imported from the dvbstream project
 *
 * Modified for use with MPlayer, for details see the changelog at
 * http://svn.mplayerhq.hu/mplayer/trunk/
 * $Id: rtp.h,v 1.1 2008/08/15 01:17:04 zzinho Exp $
 */

#ifndef RTP_H
#define RTP_H

int read_rtp_from_server(int fd, char *buffer, int length);

#endif
