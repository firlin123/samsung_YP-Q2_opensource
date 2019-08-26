/*
 * include/mizi/key.h
 *
 * Kernel vs Application API
 * 
 * Copyright (C) 2003,2004 MIZI Research, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 */

#ifndef _INCLUDE_MIZI_KEY_H_
#define _INCLUDE_MIZI_KEY_H_
#ifndef __ASSEMBLY__

#include <linux/input.h>

#ifndef KEY_RELEASED
#define KEY_RELEASED	0
#endif

#ifndef KEY_PRESSED
#define KEY_PRESSED	1
#endif

#ifndef KEY_REPEATED
#define KEY_REPEATED	2
#endif

// from i839 include/mizi/key.h

/* 2006.11.14 : sahara
 * 기존 미지키 매핑과 일반 키보드 값이 매핑이 중복되어 
 * 128 이후로 넘김.
 */

/*
 * Definition of Generic Key Scancode
 */
#define MZ_SCANCODE_LEFT                150
#define MZ_SCANCODE_RIGHT               151
#define MZ_SCANCODE_UP                  152
#define MZ_SCANCODE_DOWN                153
#define MZ_SCANCODE_ENTER               154     
#define MZ_SCANCODE_PAGE_UP             155     /* Page Up */
#define MZ_SCANCODE_PAGE_DOWN           156     /* Page Down */
#define MZ_SCANCODE_BKSP                157     /* Back Space */

/* 
 *  * Key PAD
 *   */
#define MZ_SCANCODE_PAD_0               158
#define MZ_SCANCODE_PAD_1               159
#define MZ_SCANCODE_PAD_2               160
#define MZ_SCANCODE_PAD_3               161
#define MZ_SCANCODE_PAD_4               162
#define MZ_SCANCODE_PAD_5               163
#define MZ_SCANCODE_PAD_6               164
#define MZ_SCANCODE_PAD_7               165
#define MZ_SCANCODE_PAD_8               166
#define MZ_SCANCODE_PAD_9               167
#define MZ_SCANCODE_PAD_MINUS           168
#define MZ_SCANCODE_PAD_PLUS            169
#define MZ_SCANCODE_PAD_ENTER           170
#define MZ_SCANCODE_PAD_PERIOD          171
#define MZ_SCANCODE_PAD_SLASH           172
#define MZ_SCANCODE_PAD_ASTERISK        173

/* 
 *  * Function Key
 *   */
#define MZ_SCANCODE_F4                  174
#define MZ_SCANCODE_F5                  175
#define MZ_SCANCODE_F6                  176
#define MZ_SCANCODE_F7                  177
#define MZ_SCANCODE_F8                  178
#define MZ_SCANCODE_F9                  179
#define MZ_SCANCODE_F10                 180
#define MZ_SCANCODE_F11                 181
#define MZ_SCANCODE_F12                 182

/*
 *  * Undefined Region
 *   */
#define MZ_SCANCODE_U1                  183     /* Unknown */
#define MZ_SCANCODE_U2                  184     /* Unknown */
#define MZ_SCANCODE_U3                  185     /* Unknown */
#define MZ_SCANCODE_U4                  186     /* Unknown */
#define MZ_SCANCODE_U5                  187     /* Unknown */
#define MZ_SCANCODE_U6                  188     /* Unknown */
#define MZ_SCANCODE_U7                  189     /* Unknown */
#define MZ_SCANCODE_U8                  190     /* Unknown */
#define MZ_SCANCODE_U9                  191     /* Unknown */

/*
 *  * Common key definition for PDA
 *   */
#define MZ_SCANCODE_POWER               192
#define MZ_SCANCODE_RECORD              193
#define MZ_SCANCODE_ACTION              MZ_SCANCODE_ENTER
#define MZ_SCANCODE_CAMERA		183
#define MZ_SCANCODE_SLIDE_UP            MZ_SCANCODE_PAGE_UP
#define MZ_SCANCODE_SLIDE_DOWN          MZ_SCANCODE_PAGE_DOWN
#define MZ_SCANCODE_SLIDE_CENTER        MZ_SCANCODE_PAD_ENTER
#define MZ_SCANCODE_HOMEPAGE		196	
#define MZ_SCANCODE_PROG1		MZ_SCANCODE_F8	

/*
 *  * Common key definition for Phone
 *   */
#define MZ_SCANCODE_ASTERISK            MZ_SCANCODE_PAD_ASTERISK
#define MZ_SCANCODE_SHARP               MZ_SCANCODE_PAD_MINUS
#define MZ_SCANCODE_SEND                194
#define MZ_SCANCODE_END                 195
#define MZ_SCANCODE_MENU                196
#define MZ_SCANCODE_CLR                 197

//---------------------------------------------------------
// from i839 include/mizi/i8xx.h
/* button definition */

#define HN_POWER_BUTTON         MZ_SCANCODE_POWER
#define HN_CAMERA_BUTTON        MZ_SCANCODE_CAMERA
#define HN_CAMERA_AF_BUTTON     MZ_SCANCODE_AUTOFOCUS
#define HN_VOICE_BUTTON         MZ_SCANCODE_RECORD

#define HN_SIDE_UP_BUTTON       MZ_SCANCODE_SLIDE_UP
#define HN_SIDE_DOWN_BUTTON     MZ_SCANCODE_SLIDE_DOWN

#define HN_HOME_BUTTON          MZ_SCANCODE_HOMEPAGE
#define HN_BACK_BUTTON          MZ_SCANCODE_BACK
#define HN_SEND_BUTTON          MZ_SCANCODE_SEND
#define HN_END_BUTTON           MZ_SCANCODE_END
//#define HN_END_LONG_BUTTON      MZ_SCANCODE_F11

#define HN_UP_BUTTON            MZ_SCANCODE_UP
#define HN_DOWN_BUTTON          MZ_SCANCODE_DOWN
#define HN_LEFT_BUTTON          MZ_SCANCODE_LEFT
#define HN_RIGHT_BUTTON         MZ_SCANCODE_RIGHT
#define HN_OK_BUTTON            MZ_SCANCODE_ACTION
#define HN_EAR_SEND_BUTTON      MZ_SCANCODE_U2
#define HN_0_BUTTON             MZ_SCANCODE_PAD_0
#define HN_1_BUTTON             MZ_SCANCODE_PAD_1
#define HN_2_BUTTON             MZ_SCANCODE_PAD_2
#define HN_3_BUTTON             MZ_SCANCODE_PAD_3
#define HN_4_BUTTON             MZ_SCANCODE_PAD_4
#define HN_5_BUTTON             MZ_SCANCODE_PAD_5
#define HN_6_BUTTON             MZ_SCANCODE_PAD_6
#define HN_7_BUTTON             MZ_SCANCODE_PAD_7
#define HN_8_BUTTON             MZ_SCANCODE_PAD_8
#define HN_9_BUTTON             MZ_SCANCODE_PAD_9
#define HN_ASTERISK_BUTTON      MZ_SCANCODE_ASTERISK
#define HN_SHARP_BUTTON         MZ_SCANCODE_SHARP

#define HN_MENU_BUTTON          MZ_SCANCODE_MENU
#define HN_CANCEL_BUTTON        MZ_SCANCODE_CANCEL
#define HN_ADDRESS_BUTTON       MZ_SCANCODE_ADDRESSBOOK
#define HN_MSG_BUTTON           MZ_SCANCODE_EMAIL
#define HN_UMAX_BUTTON          MZ_SCANCODE_PROG1

#define HN_PLAY_BUTTON          MZ_SCANCODE_PLAY
#define HN_PREVIOUS_BUTTON      MZ_SCANCODE_PREVIOUS
#define HN_NEXT_BUTTON          MZ_SCANCODE_NEXT
#define HN_CG_BUTTON            MZ_SCANCODE_CG

//---------------------------------------------------------


#endif	/* __ASSEMBLY__ */
#endif /* _INCLUDE_MIZI_KEY_H_ */
