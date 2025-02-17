/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: cfft.h,v 1.1 2008/08/15 01:12:36 zzinho Exp $
**/

#ifndef __CFFT_H__
#define __CFFT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t n;
    uint16_t ifac[15];
    complex_t *work;
    complex_t *tab;
} cfft_info;


void cfftf(cfft_info *cfft, complex_t *c);
void cfftb(cfft_info *cfft, complex_t *c);
cfft_info *cffti(uint16_t n);
void cfftu(cfft_info *cfft);


#ifdef __cplusplus
}
#endif
#endif
