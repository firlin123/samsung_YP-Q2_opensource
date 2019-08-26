/* $Id: stmp36xx_audioout_dac.c,v 1.198 2008/06/12 00:07:24 biglow Exp $ */

/**
 * \file stmp36xx_audioout_dac.c
 * \brief audioout of stmp36xx
 * \author LIM JIN HO <jinho.lim@samsung.com>
 * \version $Revision: 1.198 $
 * \date $Date: 2008/06/12 00:07:24 $
 *
 * This file implements audioout(DAC) driver for SMTP36XX(sigmatel).
 * $Log: stmp36xx_audioout_dac.c,v $
 * Revision 1.198  2008/06/12 00:07:24  biglow
 * update from SEHZ by zzinho
 *
 * --Taehun Lee
 *
 * Revision 1.197  2008/06/11 09:39:44  biglow
 * update from SEHZ by zzinho
 *
 * --Taehun Lee
 *
 * Revision 1.196  2008/05/14 03:57:15  zzinho
 * FR volume control bug fix
 *
 * by jinho.lim
 *
 * Revision 1.195  2008/05/10 03:32:59  zzinho
 * volum table
 *
 * by jinho.lim
 *
 * Revision 1.194  2008/04/22 06:49:36  zzinho
 * FR output modified
 *
 * by jinho.lim
 *
 * Revision 1.193  2008/04/21 03:11:54  zzinho
 * FR output modified
 *
 * by jinho.lim
 *
 * Revision 1.192  2008/04/18 07:02:47  zzinho
 * remove cpuclk intr wait function
 *
 * by jinho.lim
 *
 * Revision 1.191  2008/04/16 02:08:21  zzinho
 * adjust volume 1 level
 *
 * by jinho.lim
 *
 * Revision 1.190  2008/01/03 07:38:23  zzinho
 * clear consumed data after dac interrupt
 *
 * by jinho.lim
 *
 * Revision 1.189  2007/12/19 06:51:05  zzinho
 * remove dbg printk
 *
 * by jinho.lim
 *
 * Revision 1.188  2007/12/15 06:21:07  zzinho
 * speed src int.frac value tune
 *
 * by jinho.lim
 *
 * Revision 1.187  2007/12/14 11:14:32  zzinho
 * add playspeed function
 *
 * by jinho.lim
 *
 * Revision 1.186  2007/12/14 00:53:45  zzinho
 * YP-U4 HW mute control
 *
 * by jinho.lim
 *
 * Revision 1.185  2007/12/13 13:01:26  zzinho
 * YP-U4 HW mute control
 *
 * by jinho.lim
 *
 * Revision 1.184  2007/12/13 12:25:13  zzinho
 * YP-U4 HW mute control
 *
 * by jinho.lim
 *
 * Revision 1.183  2007/12/13 12:23:09  zzinho
 * YP-U4 HW mute control
 *
 * by jinho.lim
 *
 * Revision 1.182  2007/11/28 01:52:04  zzinho
 * add set default volume ioctl for beep
 * by jinho.lim
 *
 * Revision 1.181  2007/11/28 00:28:46  zzinho
 * add set dac volume ioctl
 * by jinho.lim
 *
 * Revision 1.180  2007/10/23 12:44:49  gold1004
 * remove pop noise.
 *
 * -gold1004
 *
 * Revision 1.179  2007/10/19 00:12:32  zzinho
 * code refine
 * by jinho.lim
 *
 * Revision 1.178  2007/10/17 10:23:53  zzinho
 * off capless mode
 * by jinho.lim
 *
 * Revision 1.177  2007/10/09 00:18:01  zzinho
 * mute on when init dac
 * by jinho.lim
 *
 * Revision 1.176  2007/10/02 06:32:11  zzinho
 * led control
 * by jinho.lim
 *
 * Revision 1.175  2007/09/27 01:12:38  zzinho
 * vag value 0xd->0xf to increase output level
 * by jinho.lim
 *
 * Revision 1.174  2007/09/20 07:09:56  zzinho
 *
 * by jinho.lim
 *
 * Revision 1.173  2007/09/19 08:55:40  zzinho
 * disable clkctrl intr wait when max volume
 * by jinho.lim
 *
 * Revision 1.172  2007/09/19 05:56:32  biglow
 * - board type detecting method changed
 *
 * -- Taehun Lee
 *
 * Revision 1.171  2007/09/05 02:07:43  zzinho
 * max user volume = 30
 * by jinho.lim
 *
 * Revision 1.170  2007/08/23 09:04:32  zzinho
 * add 4 case volume tabe
 * by jinho.lim
 *
 * Revision 1.169  2007/08/01 09:13:04  zzinho
 * add fm volume table for s3 project
 * by jinho.lim
 *
 * Revision 1.168  2007/08/01 07:34:24  zzinho
 * change volume table for S3 project
 * by jinho.lim
 *
 * Revision 1.167  2007/08/01 06:37:02  zzinho
 * change volume table for S3 project
 * by jinho.lim
 *
 * Revision 1.166  2007/07/31 09:00:59  zzinho
 * change volume table for S3 project
 * by jinho.lim
 *
 * Revision 1.165  2007/07/20 04:13:42  zzinho
 * remove debug
 * by jinho
 *
 * Revision 1.164  2007/07/20 03:09:46  zzinho
 * power tunning for S3 BD
 * by jinho
 *
 * Revision 1.163  2007/07/11 00:02:18  zzinho
 * capless mode added
 *
 * Revision 1.162  2007/06/27 06:17:17  zzinho
 * code refine for S3
 * by jinho.lim
 *
 * Revision 1.161  2007/06/21 05:29:07  zzinho
 * update to latest ZB source
 * by jinho.lim
 *
 * Revision 1.160  2007/05/22 07:56:13  zzinho
 * minor code modification
 * by jinho.lim
 *
 * Revision 1.159  2007/05/22 04:05:05  zzinho
 * EQ volume table modify as B&O's request
 * by jinho.lim
 *
 * Revision 1.158  2007/05/22 00:41:21  zzinho
 * EQ volume table modify as B&O's request
 * by jinho.lim
 *
 * Revision 1.157  2007/05/08 01:58:41  zzinho
 * audio voluem step is changed by B&O's recommendation and Max output level is increased to 20mV over
 * by jinho.lim
 *
 * Revision 1.156  2007/03/22 05:54:38  biglow
 * Update from China by zzinho
 *
 * - Taehun Lee
 *
 * Revision 1.155  2007/03/21 07:27:03  biglow
 * Update from China by zzinho
 *
 * - Taehun Lee
 *
 * Revision 1.154  2007/02/01 02:57:03  zzinho
 * code refine to reflect result of prevent test
 *
 * by jinho
 *
 * Revision 1.153  2007/01/10 01:09:54  zzinho
 * eq headroom -12dB modify
 *
 * Revision 1.152  2006/12/07 08:16:53  zzinho
 * volume table modified
 * with eq -12dB => -9dB
 *
 * Revision 1.151  2006/09/21 23:52:43  biglow
 * ADC off
 *
 * Revision 1.150  2006/08/12 08:48:39  hcyun
 * fix FM sound hang problem.
 *
 * Revision 1.149  2006/08/08 08:39:20  release
 * At max volume, reducing charging current is deleted
 *
 * Revision 1.147  2006/07/28 06:45:23  hcyun
 * decreasing AC charging current from 500 to 300mA when audio volume is max. this is intended to support audio docking station.
 *
 * Revision 1.146  2006/05/19 03:43:42  zzinho
 * update with static sound driver
 *
 * Revision 1.157  2006/04/17 05:00:03  yoonhark
 * modification for removing pop noise in start part of recording file
 *
 * Revision 1.156  2006/04/10 02:15:04  zzinho
 * remove fm event check function
 *
 * Revision 1.155  2006/04/08 01:59:27  zzinho
 * add fm open release event callback
 * add when audio sync, immediately stop audioin dma
 *
 * Revision 1.154  2006/04/05 06:36:12  zzinho
 * recording status check routine added
 *
 * Revision 1.153  2006/04/05 06:33:17  zzinho
 * recording status check routine added
 *
 * Revision 1.152  2006/04/05 06:30:21  zzinho
 * recording status check routine added
 *
 * Revision 1.151  2006/03/27 10:22:48  zzinho
 * ADC buffer 32K -> 128K
 *
 * Revision 1.150  2006/03/10 08:00:25  zzinho
 * remove cradle mute on when audio stop
 *
 * Revision 1.149  2006/03/07 06:30:03  zzinho
 * dos2unix
 *
 * Revision 1.148  2006/03/07 06:28:30  zzinho
 * add cradle mute condition as HW rev type
 *
 * Revision 1.147  2006/03/07 06:26:28  zzinho
 * add cradle mute condition as HW rev type
 *
 * Revision 1.146  2006/03/06 11:58:24  zzinho
 * add chk_sw_version to check FR volume table
 *
 * Revision 1.145  2006/03/06 08:13:07  zzinho
 * audio driver ioctl change
 * SNDCTL_DSP_EU_OUTPUT -> SNDCTL_DSP_FRANCE_OUTPUT
 *
 * Revision 1.144  2006/03/02 04:34:00  zzinho
 * add cradle mute control
 *
 * Revision 1.143  2006/03/02 01:29:43  zzinho
 * TA5 -> TB1 modify
 *
 * Revision 1.142  2006/02/28 04:02:25  zzinho
 * change ADC buf size
 *
 * Revision 1.141  2006/02/22 00:42:11  zzinho
 * removed warning message
 *
 * Revision 1.140  2006/02/08 01:22:53  zzinho
 * back to common volume table
 *
 * Revision 1.139  2006/02/08 01:12:39  zzinho
 * eu volume table
 *
 * Revision 1.138  2005/12/22 09:39:49  biglow
 * - update of jjinho
 *
 * Revision 1.137  2005/12/22 06:43:27  hcyun
 * Jinho's volume table setting ioctl code addition.
 *
 * Revision 1.136  2005/12/14 06:21:54  zzinho
 * max volume core change to 2.108V
 *
 * Revision 1.135  2005/12/14 05:30:49  zzinho
 * max volume core change to 1.920V
 *
 * Revision 1.134  2005/12/14 03:35:22  zzinho
 * volume table is modified
 *
 * Revision 1.133  2005/12/14 01:37:32  zzinho
 * volume table is modified
 *
 * Revision 1.132  2005/12/13 10:38:14  zzinho
 * remove hp_i1_adv register to increase SNR
 *
 * Revision 1.131  2005/12/12 05:59:34  zzinho
 * remove unnecessary function and define
 *
 * Revision 1.130  2005/12/12 04:06:08  zzinho
 * remove debug
 *
 * Revision 1.129  2005/12/08 07:03:27  zzinho
 * remove debug
 *
 * Revision 1.128  2005/12/08 06:59:13  zzinho
 * volume table modification
 * EU volume table addition
 *
 * Revision 1.127  2005/12/07 11:48:37  zzinho
 * volume table modification
 * EU volume table addition
 *
 * Revision 1.126  2005/12/05 12:16:26  zzinho
 * 1. volume table init version addition for EU
 * 2. Postprocessing mute modification
 * - 120msec -> 30msec first skip time
 *
 * Revision 1.125  2005/12/05 11:53:03  zzinho
 * 1. volume table init version addition for EU
 * 2. Postprocessing mute modification
 * - 120msec -> 30msec first skip time
 *
 * Revision 1.124  2005/11/26 08:48:39  zzinho
 * remove pop noise when booting
 *
 * Revision 1.123  2005/11/24 10:30:56  zzinho
 * volume set case modified
 *
 * Revision 1.122  2005/11/24 08:42:37  zzinho
 * if prev volume == UI set volume
 * return
 *
 * Revision 1.121  2005/11/20 10:37:56  zzinho
 * remove debug print
 *
 * Revision 1.120  2005/11/20 10:06:37  zzinho
 * audio reset sync addition
 *
 * Revision 1.119  2005/11/20 06:06:08  zzinho
 * MEMCPY is modified
 * decrement_sema 1 -> 2 => it cause gabage data addition when low clock
 *
 * Revision 1.118  2005/11/18 05:27:13  zzinho
 * *** empty log message ***
 *
 * Revision 1.117  2005/11/17 12:47:33  zzinho
 * dac dma desc memory changed to sram from sdram
 *
 * Revision 1.116  2005/11/17 08:34:36  zzinho
 * VAG reference is added to remove hissing noise when VDD is changed
 *
 * Revision 1.115  2005/11/15 10:18:52  zzinho
 * pop up improvement when enable beep
 *
 * Revision 1.114  2005/11/15 07:40:45  zzinho
 * pop up improvement
 *
 * Revision 1.113  2005/11/14 10:14:52  zzinho
 * postprocessing vol fade in/out is modified
 *
 * Revision 1.112  2005/11/14 06:31:39  zzinho
 * increment sem 2->1 change
 *
 * Revision 1.111  2005/11/14 06:22:46  zzinho
 * volume fade in -> out crash when repeat user press pause/restart button fastly
 *
 * Revision 1.110  2005/11/14 06:22:14  zzinho
 * volume fade in -> out crash when repeat user press pause/restart button fastly
 *
 * Revision 1.109  2005/11/13 10:25:39  zzinho
 * delay added when open and write
 *
 * Revision 1.108  2005/11/13 07:29:09  zzinho
 * volume fade in/out with 10msec timer
 *
 * Revision 1.107  2005/11/12 08:31:35  zzinho
 * volume fade in addition
 *
 * Revision 1.106  2005/11/12 04:31:55  zzinho
 * *** empty log message ***
 *
 * Revision 1.105  2005/11/11 06:33:55  zzinho
 * pm idle/wakeup reset/restart addition
 *
 * Revision 1.104  2005/11/10 06:42:07  zzinho
 * *** empty log message ***
 *
 * Revision 1.103  2005/11/10 06:19:52  zzinho
 * *** empty log message ***
 *
 * Revision 1.102  2005/11/09 05:33:30  zzinho
 * audio exit addition
 *
 * Revision 1.101  2005/11/08 12:16:28  zzinho
 * when audio init, irq start once addition to remove audio start popup noise
 *
 * Revision 1.100  2005/11/05 04:47:37  zzinho
 * fade in/out addition when postprocessing is changed
 *
 * Revision 1.99  2005/11/03 13:11:04  zzinho
 * add controlstopvol in checkpostprocess
 *
 * Revision 1.98  2005/11/03 05:08:16  zzinho
 * audio in modified
 *
 * Revision 1.97  2005/11/02 07:41:43  zzinho
 * *** empty log message ***
 *
 * Revision 1.96  2005/11/02 06:08:32  zzinho
 * volume table changed
 *
 * Revision 1.95  2005/11/01 09:12:39  zzinho
 * refer define hardware.h
 *
 * Revision 1.94  2005/10/31 12:33:30  zzinho
 * enable zcd
 *
 * Revision 1.93  2005/10/31 12:31:57  zzinho
 * disable zcd
 *
 * Revision 1.92  2005/10/31 12:29:04  zzinho
 * disable zcd
 *
 * Revision 1.91  2005/10/31 09:41:42  zzinho
 * function added
 *
 * Revision 1.90  2005/10/28 11:42:26  zzinho
 * no sound time changed when sound mode is changed
 * -> add delay
 *
 * Revision 1.89  2005/10/28 09:53:38  zzinho
 * no sound time changed when sound mode is changed
 *
 * Revision 1.88  2005/10/22 01:40:38  zzinho
 * checkpostprocessing size
 *
 * Revision 1.87  2005/10/22 00:36:07  zzinho
 * select_sound_status addition
 *
 * Revision 1.86  2005/10/21 01:35:29  zzinho
 * max volume fault bug modify
 *
 * Revision 1.85  2005/10/19 02:28:54  zzinho
 * setPlayingStatus(), getPlayingStatus(), setCoreLevle() addition
 *
 * Revision 1.84  2005/10/18 12:05:45  zzinho
 * controlStopVol() is added by jinho.lim
 *
 * Revision 1.83  2005/10/18 08:43:16  zzinho
 * if audio is stopped && volume 100, we cannot change VDD for max sound output
 * added by jinho.lim
 *
 * Revision 1.82  2005/10/14 11:22:50  zzinho
 * fix volume table
 *
 * Revision 1.81  2005/10/14 08:10:20  zzinho
 * fix volume table and add checkdacptr function
 *
 * Revision 1.80  2005/10/12 07:48:13  zzinho
 * volume table changed
 *
 * Revision 1.79  2005/10/10 07:34:12  zzinho
 * remove unnecessary code
 *
 * Revision 1.78  2005/10/08 03:53:46  zzinho
 * type change
 *
 * Revision 1.77  2005/10/07 06:31:07  zzinho
 * add volume table for 33 step
 *
 * Revision 1.76  2005/10/07 06:04:42  zzinho
 * add volume table for 33 step
 *
 * Revision 1.75  2005/10/07 02:39:34  zzinho
 * add delay to remove popup noise at HW mute off
 *
 * Revision 1.74  2005/10/04 05:53:13  zzinho
 * buffer point type change
 * short -> char
 *
 * Revision 1.73  2005/10/01 01:34:47  zzinho
 * add if initDev == 0
 * skip checkPostProcessing
 *
 * Revision 1.72  2005/10/01 01:07:19  zzinho
 * *** empty log message ***
 *
 * Revision 1.71  2005/10/01 00:30:18  zzinho
 * fillDecoderBuffer write pointer change
 *
 * Revision 1.70  2005/09/30 08:00:10  zzinho
 * add debug for copy_from_user fail
 *
 * Revision 1.69  2005/09/30 06:04:55  zzinho
 * add debug printk by copy_from_user fail
 *
 * Revision 1.68  2005/09/29 06:36:04  zzinho
 * *** empty log message ***
 *
 * Revision 1.67  2005/09/28 09:27:25  zzinho
 * *** empty log message ***
 *
 * Revision 1.66  2005/09/28 04:46:34  zzinho
 * add volume table
 *
 * Revision 1.65  2005/09/27 08:07:30  zzinho
 * add volume table
 *
 * Revision 1.64  2005/09/23 10:25:27  zzinho
 * change mmap for ogg sdram
 *
 * Revision 1.63  2005/09/23 07:54:00  zzinho
 * remove unnecessary function
 * remove unnecessary sdram alloc
 * modify audio_write
 *
 * Revision 1.62  2005/09/22 02:11:00  zzinho
 * change buffer mechanism
 * 1. use one buffer and write/read buffer pointer
 * 2. remove memcpy dma
 *
 * Revision 1.61  2005/09/21 00:07:14  zzinho
 * no decoder buf mode add
 *
 * Revision 1.60  2005/09/20 05:23:20  zzinho
 * modify error in volume table
 *
 * Revision 1.59  2005/09/20 05:11:23  zzinho
 * modify volume table
 *
 * Revision 1.58  2005/09/15 00:35:58  zzinho
 * add volume table & ioctl for eq & 3D post processing status of user
 *
 * Revision 1.57  2005/09/14 02:48:44  zzinho
 * hw mute on function added
 *
 * Revision 1.56  2005/09/12 06:41:46  zzinho
 * *** empty log message ***
 *
 * Revision 1.55  2005/09/12 05:36:51  zzinho
 * remove printf
 *
 * Revision 1.54  2005/09/12 04:27:17  zzinho
 * hw mute position changed
 *
 * Revision 1.53  2005/09/12 00:10:03  zzinho
 * add new ioctl for mic gain
 *
 * Revision 1.52  2005/09/08 06:03:48  zzinho
 * *** empty log message ***
 *
 * Revision 1.51  2005/09/07 11:25:23  zzinho
 * add audioin
 *
 * Revision 1.50  2005/09/06 10:49:46  zzinho
 * mv hp sel to write
 *
 * Revision 1.49  2005/09/02 02:24:28  zzinho
 * printf remove
 *
 * Revision 1.48  2005/09/01 08:20:31  zzinho
 * fix volume control from HP amp to DAC
 * HP amp default 0x03
 * DAC volume range -100dB ~ 0dB
 *
 * Revision 1.47  2005/08/31 10:20:52  zzinho
 * 1. When audio clear, codec_phys.. init flag remove
 *
 * Revision 1.46  2005/08/23 09:16:07  zzinho
 * remove warning msg
 *
 * Revision 1.45  2005/08/22 01:44:57  zzinho
 * mmap, release change
 *
 * Revision 1.43  2005/08/11 08:20:14  zzinho
 * *** empty log message ***
 *
 * Revision 1.42  2005/08/11 00:44:52  zzinho
 * remove start release sem
 * add delay 200msec in case release
 *
 * Revision 1.41  2005/08/09 04:02:09  zzinho
 * jinho version up
 *
 * Revision 1.40  2005/08/03 05:24:00  zzinho
 * when stop the play, set the zero the current addr
 *
 * Revision 1.39  2005/08/03 00:40:48  zzinho
 * remove pop noise 80%
 *
 * Revision 1.38  2005/08/02 09:36:57  zzinho
 * *** empty log message ***
 *
 * Revision 1.37  2005/08/02 02:29:47  zzinho
 * modify max dac volume 0xf1->0xf2
 *
 * Revision 1.36  2005/08/02 01:58:36  zzinho
 * modify dac volume max value to avoid clipping
 *
 * Revision 1.35  2005/08/01 06:57:19  zzinho
 * back volume 0~100
 *
 * Revision 1.34  2005/07/29 08:24:08  zzinho
 * modify audioout dma structure
 *
 * Revision 1.33  2005/07/28 06:16:34  zzinho
 * version up
 *
 * Revision 1.32  2005/07/28 02:31:45  zzinho
 * *** empty log message ***
 *
 * Revision 1.31  2005/07/28 02:10:12  zzinho
 * default vol dac 95 hp 20
 *
 * Revision 1.30  2005/07/28 02:05:28  zzinho
 * up version
 *
 * Revision 1.29  2005/07/28 01:50:58  zzinho
 * up version
 *
 * Revision 1.28  2005/07/27 12:06:39  zzinho
 * modify hp volume setting
 *
 * Revision 1.27  2005/07/27 06:45:54  zzinho
 * add sz == 0 panic
 *
 * Revision 1.26  2005/07/27 04:36:51  zzinho
 * modify initialze dac process
 *
 * Revision 1.25  2005/07/26 02:43:59  zzinho
 * modify apbx ctrl0 clr
 *
 * Revision 1.24  2005/07/26 00:01:55  zzinho
 * *** empty log message ***
 *
 * Revision 1.23  2005/07/23 05:35:03  zzinho
 * modify name
 * add apbx audioout dma irq timeout
 *
 * Revision 1.22  2005/07/15 12:42:58  zzinho
 * back 0708
 * because of dac dma kill
 * jinho.lim
 *
 * Revision 1.21  2005/07/15 11:58:51  zzinho
 * back 0708
 * because of dac dma kill
 * jinho.lim
 *
 * Revision 1.18  2005/07/06 10:46:36  zzinho
 * defult volume 80
 *
 * Revision 1.17  2005/07/04 09:11:48  zzinho
 * *** empty log message ***
 *
 * Revision 1.16  2005/07/04 05:22:12  zzinho
 * in case audio_write
 * if the driver buffer full, no wait and return.
 *
 * Revision 1.15  2005/06/30 07:47:50  zzinho
 * *** empty log message ***
 *
 * Revision 1.14  2005/06/30 02:37:37  zzinho
 * fill->copy->start : bug
 * fill->copy->fill->start : modify
 *
 * Revision 1.13  2005/06/22 09:46:23  zzinho
 * set seg ioctl support
 *
 * Revision 1.12  2005/06/11 17:37:31  zzinho
 * *** empty log message ***
 *
 * Revision 1.11  2005/06/08 17:38:21  zzinho
 * update to TA2 board test
 *
 * Revision 1.10  2005/06/03 19:25:09  zzinho
 * HP AMP Class AB register set
 *
 * Revision 1.9  2005/06/02 19:10:48  zzinho
 * buffer management modify to use 4 dma chain
 *
 * Revision 1.5  2005/05/31 17:10:21  zzinho
 * modify doxygen format
 * and
 * add mmap ioctl
 *
 * Revision 0.1  2005/04/29 zzinho
 * - add first revision
 *
 */
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/dma-mapping.h>
#include <asm/arch/digctl.h>

#include <asm/memory.h>
#include <asm/io.h>

#include <asm/hardware.h>
#include <asm/arch/hardware.h>
#include <asm/arch/ocram.h>
#include <asm/arch/stmp36xx_power.h>
#include <asm-arm/arch-stmp37xx/37xx/regs.h>
#include <asm-arm/arch-stmp37xx/37xx/regsaudioin.h>
#include <asm-arm/arch-stmp37xx/37xx/regsaudioout.h>
#include <asm-arm/arch-stmp37xx/37xx/regsapbx.h>
#include <asm-arm/arch-stmp37xx/37xx/regsapbh.h>
#include <asm-arm/arch-stmp37xx/37xx/regsmemcpy.h>
#include <asm-arm/arch-stmp37xx/37xx/regspinctrl.h>
#include <asm-arm/arch-stmp37xx/37xx/regspower.h>
#include <asm-arm/arch-stmp37xx/37xx/regsclkctrl.h>
#include <asm-arm/arch-stmp37xx/pinctrl.h>
#include <asm-arm/arch-stmp37xx/gpio.h>

#include "stmp37xx_audio.h"
#include "stmp37xx_audioout_dac.h"

#include <asm/arch-stmp37xx/stmp37xx_pm.h>

#define MAX_USER_VOLUME 30
#define MAX_VOLUME_CORE 0x1C // 1.920V
#define NOR_VOLUME_CORE 0x17 // 1.760V
#define CAPLESS_MODE 0
#define MAX_VGA_VAL 0xF // 1.000V for 2.100VDDA 
#define DEF_VGA_VAL 0x9 // 0.850V for 1.750VDDA

/*---------- Structure Definitions ----------*/
struct stmp_dac_value_t
{
	unsigned char volume;
	unsigned char play_speed;
	unsigned short volume_reg;
	unsigned short spkvol_reg;
	unsigned short hpvol_reg;
	unsigned short current_post;
	unsigned short board_revision;
	int gain_attenuation;
	int selectSound;
	int maxVDD_flag;
	int isPlaying;
	int isRecording; 	
	int lastDacPtr;
	int buf_size;
	int volume_fade;
	int sw_ver;
	int dsp_fade;
	unsigned long samplingRate;
};

struct stmp_dac_value_t stmp_dac_value = {
	volume: 0, //max value to avoid cliping
	play_speed: PLAY_SPEED_X1_0,
	volume_reg: 0xd0, //max value to avoid cliping with max hp volumne
	spkvol_reg: 0x00, 
	hpvol_reg: 0x03,
	current_post: NO_PROCESSING,
	board_revision: 0,
	gain_attenuation: 0,
	selectSound: NO_SELECT,
	maxVDD_flag: 0,
	isPlaying: NO_PLAYING,	
	isRecording: NO_RECORDING,
	lastDacPtr: 0,
	buf_size: STMP37XX_SRAM_AUDIO_KERNEL_SIZE,
	volume_fade: NO_FADE,
	sw_ver: VER_COMMON,
	dsp_fade: 1,
	samplingRate: 44100
};

struct stmp_dma_cmd_t
{
	reg32_t *desc_cmd_ptr;
	reg32_t phys_desc_cmd_ptr;
};

static struct stmp_dma_cmd_t dac_dma_cmd = {
	desc_cmd_ptr: (unsigned int*)NULL,
	phys_desc_cmd_ptr: 0,
};


/* ------------ Definitions ------------ */
/* mutex to access only one */
static spinlock_t dac_lock;

enum {
	MUTE_OFF = 0,
	MUTE_ON
};

enum {
	DISABLE_CLK_INTRWAIT = 0,
	ENABLE_CLK_INTRWAIT
};

static struct timer_list post_volume_timer;
static struct timer_list volume_fade_timer;
static struct timer_list user_volume_set_timer;

static int DNSe_fade=FADE_OUT;
int isHWMuteon=0;
int vol_fadein_ing = 0;
int vol_fadeout_ing = 0;
/* ------------ HW Dependent Functions ------------ */
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
static void addDecoderValidBytes(decoder_buf_t *decoderData, signed int validBytes);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int allocDMADac_desc(unsigned int bufferSize);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int checkDacPtr(audio_stream_t *audio_s);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void checkPostprocessing(unsigned long arg);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void checkVolumeFade(unsigned long arg);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void controlStopVol(int effect);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int doFillDac(void *audio_s, unsigned int ALL);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void exitAudioout(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int fillDecoderBuffer(audio_stream_t *audio_s, const void *userBuffer);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void freeDMA_Desc(unsigned int bufferSize);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getBufSize(void); 
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
unsigned int getDacPointer(unsigned int phydac_addr);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
unsigned long getDacSamplingRate(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getDacVolume(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getDacVolume_reg(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getDacVolume_reg_post(int val);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
static signed int getDecoderValidBytes(decoder_buf_t *decoderData);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getHPVolume(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getSpeakerVolume(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
 int getMaxSoundflag(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getPlayingStatus(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getSelectSoundStatus(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getSWVersion(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getVolumeFade(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int initializeAudioout(audio_stream_t *s);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
static void initializeDac(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void initDacSync(audio_stream_t *s);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void initDacWrite(audio_stream_t *s, const char *buffer);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int onHWmute(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int offHWmute(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setBufSize(int bufferSize);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
static void subDecoderValidBytes(decoder_buf_t *decoderData, signed int validBytes);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setupDMA(unsigned int p_dacPointer, unsigned long bufferSize);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setDMADesc(unsigned int p_dacPointer, unsigned long bufferSize);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void startDac(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int stopDacProcessing(audio_stream_t *audio_s);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void startHWAudioout(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void stopHWAudioout(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setHPVolume(int val);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setUserVolume(int user_volume);
void setDacVolume(int val);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setSpeakerVolume(int val);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setSWVersion(unsigned short version);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setDacSamplingRate(unsigned long samplingRate);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setPlayingStatus(int isPlaying);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setPostProcessingFlag(audio_buf_t *dac_b, unsigned short setValue, int gain_attenuation);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setVDDflag(int MAX_VDD);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setVolumeFade(int fade);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setDefaultVGA(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
 void setMaxVGA(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
#ifdef USE_MAX_CORE_LEVEL 
void setCoreLevel(int volume);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
 #endif
int updateDacBytes(void *audio_s);
int userVol2DacVol(int user_volume); 
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */

static void addDecoderValidBytes(decoder_buf_t *decoderData, signed int validBytes)
{
	spin_lock_irq(&dac_lock);
	decoderData->decoderValidBytes+=validBytes;
//    printk("add [%d] \n", decoderData->decoderValidBytes);
	spin_unlock_irq(&dac_lock);
}

int allocDMADac_desc(unsigned int bufferSize)
{
	int dmadescsize = 0, n=0;
	unsigned int *desc_cmd_ptr = NULL;
	unsigned int phys_desc_cmd_ptr = 0;

	n = bufferSize / MIN_STREAM_SIZE;
	dmadescsize = DAC_DESC_SIZE*n;

	/* desc 0 consistent alloc */
	do {
/* by jinho.lim : 2008. 8. 11 */		
#if 0//def CONFIG_STMP36XX_SRAM
		phys_desc_cmd_ptr = STMP37XX_SRAM_AUDIO_KERNEL_DESC; //real addr
		desc_cmd_ptr = ioremap(STMP37XX_SRAM_AUDIO_KERNEL_DESC, dmadescsize); //virture addr
#else
		desc_cmd_ptr = dev_dma_alloc(dmadescsize, &phys_desc_cmd_ptr);

		if (!desc_cmd_ptr)
			dmadescsize -= DAC_DESC_SIZE*n;
#endif
	} while (!desc_cmd_ptr && dmadescsize);

	if(desc_cmd_ptr == (unsigned int *)NULL)
	{
		printk("DAC_DMA: ERROR stmp36xx dac dma descriptor00 is not allocated\n");
		return -ENOMEM;
	}
	
	memzero(desc_cmd_ptr, dmadescsize);

	dac_dma_cmd.desc_cmd_ptr = desc_cmd_ptr;
	dac_dma_cmd.phys_desc_cmd_ptr = phys_desc_cmd_ptr;

	return 0;
}

int checkDacPtr(audio_stream_t *audio_s)
{
	audio_buf_t *dac_b = audio_s->audio_buffers;

	// to set dac dma position at start position
	int dac_pointer=0;
	dac_pointer = getDacPointer(dac_b->phyStartaddr);

	//printk("end[%d] last[%d]\n", dac_pointer, stmp_dac_value.lastDacPtr);
	if(dac_pointer < stmp_dac_value.lastDacPtr)
	{
		BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ(1);
		BF_WRn(APBX_CHn_SEMA, 1, INCREMENT_SEMA, 1); 
		return -1;
	}
	return 0;
}

#ifdef HP_VOLUME_CONTROL
void checkVolumeFade(unsigned long arg)
{
	static int i=0, j=0;
	int val = 0, val_reg = 0;
	int fade = 0;
	int isPlaying = 0;

	isPlaying = getPlayingStatus();
	fade = getVolumeFade();
	
	if(fade == FADE_OUT)
	{
		val = getHPVolume();
		i++;
		/* 3dB Â÷ÀÌ */
		val_reg = val + i*6;
		if(val_reg >= HP_VOLUME_MIN)
			val_reg = HP_VOLUME_MIN;
		//printk("[D%d]\n", val_reg);
		if(isPlaying == PLAYING)
		{
			BF_CS2(AUDIOOUT_HPVOL, 
				VOL_LEFT,  val_reg, 
				VOL_RIGHT, val_reg);
			volume_fade_timer.expires=jiffies+1; // 10msec
			/* register timer */
			add_timer(&volume_fade_timer);
		}
		else
		{
			i = 0;
			setVolumeFade(NO_FADE); 
			vol_fadeout_ing = 0;						
		}
	}
	else if(fade == FADE_IN)
	{
		j++;
		val_reg = HP_VOLUME_MIN - j*6;
		//printk("[U%d]\n", val_reg);
		val = getHPVolume();
		
		if(val_reg > val)
		{
			BF_CS2(AUDIOOUT_HPVOL, 
				VOL_LEFT,  val_reg, 
				VOL_RIGHT, val_reg);
			volume_fade_timer.expires=jiffies+1; // 10msec
			/* register timer */
			add_timer(&volume_fade_timer);
		}
		else
		{
			setHPVolume(stmp_dac_value.hpvol_reg);
			
			#ifdef USE_MAX_CORE_LEVEL
			if(stmp_dac_value.sw_ver == VER_COMMON)
			{
				if(stmp_dac_value.volume == MAX_USER_VOLUME)
					setCoreLevel(stmp_dac_value.volume);
			}
			#endif
			j = 0;
			setVolumeFade(NO_FADE); 
			vol_fadein_ing = 0; 		
		}
	}
}

void controlStopVol(int fade)
{
	static int i=0, j=0;
	int val = 0, val_reg = 0;

	if(fade == NO_FADE)
	{
		//printk("[DRIVER] NO val = 0\n");
		BF_CS2(AUDIOOUT_HPVOL, 
			VOL_LEFT,  HP_VOLUME_MIN, 
			VOL_RIGHT, HP_VOLUME_MIN);
		DNSe_fade = FADE_IN;
		i = 0;
	}
	else if(fade == FADE_OUT)
	{
		val = getHPVolume();
		i++;

		val_reg = val + i*6;
		
		if(val_reg >= HP_VOLUME_MIN)
			val_reg = HP_VOLUME_MIN;
			
		//printk("[DRIVER] FOUT val_reg[%d] val[%d]\n", val_reg, val);

		BF_CS2(AUDIOOUT_HPVOL, 
			VOL_LEFT,  val_reg, 
			VOL_RIGHT, val_reg);
	}
	else if(fade == FADE_IN)
	{
		j++;
		val_reg = HP_VOLUME_MIN - j*6;		
		val = getHPVolume();		
		if(val_reg > val)
		{
			//printk("[DRIVER] FIN val_reg[%d] val[%d]\n", val_reg, val);

			BF_CS2(AUDIOOUT_HPVOL, 
				VOL_LEFT,  val_reg, 
				VOL_RIGHT, val_reg);
		}
		else
		{
			setHPVolume(stmp_dac_value.hpvol_reg);
			//printk("[DRIVER] stmp_dac_value.hpvol_reg = %d\n", stmp_dac_value.hpvol_reg);
			#ifdef USE_MAX_CORE_LEVEL
			if(stmp_dac_value.sw_ver == VER_COMMON)
			{
				if(stmp_dac_value.volume == MAX_USER_VOLUME)
					setCoreLevel(stmp_dac_value.volume);
			}
			#endif
			DNSe_fade = FADE_OUT;
			j = 0;
		}
	}
}

#else

void checkVolumeFade(unsigned long arg)
{
	static int i=0, j=0;
	int val = 0, val_reg = 0;
	int volume = 0;
	int fade = 0;
	int isPlaying = 0;

	isPlaying = getPlayingStatus();
	fade = getVolumeFade();
	
	//printk("===> checkVolumeFade : fade=%d\n", fade);
	
	if(fade == FADE_OUT)
	{
		val = (getDacVolume_reg()-0x37) >> 1;
		i++;
		volume = val - i*3;
		if(volume <= 1)
			volume = 1;
		if(isPlaying == PLAYING)
		{
			val_reg = 0x37 + (volume << 1); 

			//printk("[D:%d, v%d]\n", val_reg, volume);
			BF_CS2(AUDIOOUT_DACVOLUME, 
				VOLUME_LEFT,  val_reg, 
				VOLUME_RIGHT, val_reg);
			volume_fade_timer.expires=jiffies+1; // 10msec
			/* register timer */
			add_timer(&volume_fade_timer);
		}
		else
		{
			i = 0;
			setVolumeFade(NO_FADE); 
			vol_fadeout_ing = 0;		    		    
		}
	}
	else if(fade == FADE_IN)
	{
		j++;
		volume = 50 + j*5;//MIN_DACVOL_REG + i*1;
		val = (getDacVolume_reg()-0x37) >> 1;
		
		if(volume < val)
		{
			val_reg = 0x37 + (volume << 1); 

			//printk("[U:%d, %d]\n", val_reg, volume);
			BF_CS2(AUDIOOUT_DACVOLUME, 
				VOLUME_LEFT,  val_reg, 
				VOLUME_RIGHT, val_reg);
			volume_fade_timer.expires=jiffies+1; // 10msec
			/* register timer */
			add_timer(&volume_fade_timer);
		}
		else
		{
			setDacVolume(stmp_dac_value.volume);	
			
			#ifdef USE_MAX_CORE_LEVEL
			if(stmp_dac_value.sw_ver == VER_COMMON)
			{
				if(stmp_dac_value.volume == MAX_USER_VOLUME)
					setCoreLevel(stmp_dac_value.volume);
			}
			#endif
			j = 0;
			setVolumeFade(NO_FADE); 
			vol_fadein_ing = 0;		    
		}
	}
}

void controlStopVol(int fade)
{
	static int i=0, j=0;
	int val = 0, val_reg = 0;
	int volume = 0;

	if(fade == NO_FADE)
	{
		//printk("[DRIVER] NO val = 0\n");
		BF_CS2(AUDIOOUT_DACVOLUME, 
			VOLUME_LEFT,  DAC_VOLUME_MIN, 
			VOLUME_RIGHT, DAC_VOLUME_MIN);
		DNSe_fade = FADE_IN;
		i = 0;
	}
	else if(fade == FADE_OUT)
	{
		val = (getDacVolume_reg()-0x37) >> 1;
		i++;

		volume = val - i*3;
		
		if(volume <= 1)
			volume = 1;
			
		//printk("[DRIVER] FOUT vol[%d] val[%d]\n", volume, val);
		val_reg = 0x37 + (volume << 1); 

		BF_CS2(AUDIOOUT_DACVOLUME, 
			VOLUME_LEFT,  val_reg, 
			VOLUME_RIGHT, val_reg);
	}
	else if(fade == FADE_IN)
	{
		j++;
		volume = j*3;
		val = (getDacVolume_reg_post(getDacVolume())-0x37) >> 1;
		if(volume < val)
		{
			//printk("[DRIVER] FIN vol[%d] val[%d]\n", volume, val);
			val_reg = 0x37 + (volume << 1); 

			BF_CS2(AUDIOOUT_DACVOLUME, 
				VOLUME_LEFT,  val_reg, 
				VOLUME_RIGHT, val_reg);
		}
		else
		{
			setDacVolume(stmp_dac_value.volume);	
			//printk("[DRIVER] stmp val = %d\n", (getDacVolume_reg()-0x37)>>1);
			#ifdef USE_MAX_CORE_LEVEL
			if(stmp_dac_value.sw_ver == VER_COMMON)
			{
				if(stmp_dac_value.volume == MAX_USER_VOLUME)
					setCoreLevel(stmp_dac_value.volume);
			}
			#endif
			DNSe_fade = FADE_OUT;
			volume = 0;
			j = 0;
		}
	}
}

#endif

void checkPostprocessing(unsigned long arg)
{
	static int msec_cnt = 0;

	if(stmp_dac_value.selectSound == SOUND_SELECT)
	{
		if(DNSe_fade == FADE_OUT)
		{
			msec_cnt ++;
			if(msec_cnt >= 3) //  30msec
			{
				controlStopVol(FADE_OUT);
			}

	//		printk("[DRIVER] msec_cnt [%d0msec]\n", msec_cnt);
			if(msec_cnt >= 50) // to 500msec
			{
				controlStopVol(NO_FADE);
				msec_cnt = 0;
			}
			post_volume_timer.expires=jiffies+1; // 10msec
			/* register timer */
			add_timer(&post_volume_timer);
		}
		else if(DNSe_fade == FADE_IN)
		{
			controlStopVol(FADE_IN);

			if(DNSe_fade == FADE_OUT)
			{
				del_timer(&post_volume_timer);
				stmp_dac_value.selectSound = NO_SELECT;
			}
			else
			{
				post_volume_timer.expires=jiffies+1; // 10msec
				/* register timer */
				add_timer(&post_volume_timer);
			}
		}

	}

}

void exitAudioout(void)
{
	onHWmute();
}

int fillDecoderBuffer(audio_stream_t *audio_s, const void *userBuffer)
{
	//Go read from the hard-drive and fill the intermediate buffer..
	decoder_buf_t *decoderData = audio_s->decoder_buffers;

	char * fill1Ptr=NULL;
	char * fill2Ptr=NULL;
	unsigned int  fill1Bytes=0;
	unsigned int  fill2Bytes=0;
	unsigned int  bytesToCopy1=0;
	unsigned int  bytesToCopy2=0;
	unsigned int numberBytes = 0;
	int err = 0;
	unsigned int buffer_size = 0;

	buffer_size = getBufSize();

	numberBytes = buffer_size - getDecoderValidBytes(decoderData);

	if (numberBytes == 0) 
		return 0;
	else if(numberBytes > buffer_size)
	{
		numberBytes = buffer_size;
	}

	fill1Ptr = (char * )(decoderData->decoderBuffer + decoderData->decoderWritePointer);

	if(decoderData->AvaliableUserBytes < numberBytes)
	{
		fill1Bytes = decoderData->AvaliableUserBytes;
	}
	else
	{
		fill1Bytes = numberBytes;
	}
	
	fill2Ptr = (char *)(decoderData->decoderBuffer);
	
	if ( (fill1Bytes + decoderData->decoderWritePointer) > buffer_size)
	{
		fill2Bytes = fill1Bytes - (buffer_size - decoderData->decoderWritePointer);
		fill1Bytes = buffer_size - decoderData->decoderWritePointer;
		decoderData->decoderWritePointer = fill2Bytes;
	}
	else
	{
		decoderData->decoderWritePointer += fill1Bytes;
	}

	if (decoderData->decoderWritePointer >= buffer_size)
	{
		decoderData->decoderWritePointer =  0;//decoderData->decoderWritePointer - buffer_size;
	}

	bytesToCopy1 = fill1Bytes;

	err = copy_from_user(fill1Ptr, userBuffer, bytesToCopy1);
	if (err)
	{
		printk("[DRIVER] audio copy_from_user 1 fail length[%u] error[%d]\n", bytesToCopy1, err);
		printk("[DRIVER] decoderWritePointer[%u] AvaliableUserBytes[%u] fill2Bytes[%u]\n", 
			decoderData->decoderWritePointer, decoderData->AvaliableUserBytes, fill2Bytes);
		return -EFAULT;
	}

	decoderData->AvaliableUserBytes -= bytesToCopy1;
	
	addDecoderValidBytes(decoderData,bytesToCopy1); 

	if( (fill2Bytes != 0) && (decoderData->AvaliableUserBytes > 0) )
	{
		bytesToCopy2 = fill2Bytes;

		err = copy_from_user(fill2Ptr, userBuffer+fill1Bytes, bytesToCopy2);
		if (err)
		{
			printk("[DRIVER] audio copy_from_user 2 fail! length[%u] error[%d]\n", bytesToCopy2, err);
			return -EFAULT;
		}
		decoderData->AvaliableUserBytes -= bytesToCopy2;
	}

	addDecoderValidBytes(decoderData,bytesToCopy2); 

	return (bytesToCopy1 + bytesToCopy2);

}

void freeDMA_Desc(unsigned int bufferSize)
{
	int dmadescsize = 0, n=0;

	n = bufferSize / 2048;
	dmadescsize = DAC_DESC_SIZE*n;

	/* free DMADac_desc */
	if (dac_dma_cmd.desc_cmd_ptr) 
	{
		dev_dma_free(dmadescsize,
				  dac_dma_cmd.desc_cmd_ptr, dac_dma_cmd.phys_desc_cmd_ptr);
		dac_dma_cmd.desc_cmd_ptr = (reg32_t *)NULL;
		dac_dma_cmd.phys_desc_cmd_ptr = (reg32_t)NULL;
	}
	
}

int getBufSize(void)
{
	return stmp_dac_value.buf_size;
}

unsigned int getDacPointer(unsigned int phydac_addr)
{
	hw_apbx_chn_debug2_t ptr;
	unsigned int dmaBytes;
	
	ptr=HW_APBX_CHn_DEBUG2(1);

	dmaBytes=(HW_APBX_CHn_BAR_RD(1) - (unsigned long)phydac_addr);

	dmaBytes &= ~1; //If were really 1 byte extra into it, forget it... we don't play mono samples.

	dmaBytes -= 32;
	//printk("[DRIVER] ISR%d\n", dmaBytes);
	return dmaBytes;
}

unsigned long getDacSamplingRate(void)
{
	return stmp_dac_value.samplingRate;
}

int getDacVolume(void)
{
    return stmp_dac_value.volume;
}

int getDacVolume_reg(void)
{
    return stmp_dac_value.volume_reg;
}

int getDacVolume_reg_post(int val)
{
	int real_value = 0;

	real_value = userVol2DacVol(val);
	
	return (0x37 + real_value); 
}

static signed int getDecoderValidBytes(decoder_buf_t *decoderData)
{
	signed int ret;
	spin_lock_irq(&dac_lock);
	ret=decoderData->decoderValidBytes;
	spin_unlock_irq(&dac_lock);
	return ret;
}

int getHPVolume(void)
{
	return stmp_dac_value.hpvol_reg;
}

int getSpeakerVolume(void)
{
	return stmp_dac_value.spkvol_reg;
}

int getSWVersion(void)
{
	return stmp_dac_value.sw_ver;
}

int getMaxSoundflag(void)
{
	return stmp_dac_value.maxVDD_flag;
}

int getPlayingStatus(void)
{
	return stmp_dac_value.isPlaying;
}

int getRecordingStatus(void)
{
	return stmp_dac_value.isRecording;
}

int getSelectSoundStatus(void)
{
	return stmp_dac_value.selectSound;
}

int getVolumeFade(void)
{
	return stmp_dac_value.volume_fade;
}

static void dac_init(void)
{
	DPRINTK("\n");
	// Keep audio input off
	HW_AUDIOOUT_CTRL.B.CLKGATE = false;
	HW_AUDIOIN_CTRL.B.CLKGATE = false;
	HW_AUDIOIN_CTRL.B.SFTRST = false;
	HW_AUDIOOUT_PWRDN.B.RIGHT_ADC = true;
	HW_AUDIOOUT_PWRDN.B.ADC = true;
	HW_AUDIOOUT_PWRDN.B.CAPLESS = true;
	HW_AUDIOOUT_ANACTRL.B.HP_HOLD_GND = true;	// Prevent Digital Noise

	// DAC reset and gating
	HW_AUDIOOUT_CTRL.B.SFTRST = false;

	// Digital filter gating
	HW_CLKCTRL_XTAL.B.FILT_CLK24M_GATE = false;
	msleep(1);

	// Disable the ADC
	HW_AUDIOOUT_PWRDN.B.ADC = true;
	HW_AUDIOIN_ANACLKCTRL.B.CLKGATE = true;

	// Enable the DAC
	HW_AUDIOOUT_PWRDN.B.DAC = false;
	HW_AUDIOOUT_ANACLKCTRL.B.CLKGATE = false;

	HW_AUDIOOUT_CTRL.B.WORD_LENGTH = 1; // 16-bit PCM samples
	HW_AUDIOOUT_CTRL.B.DMAWAIT_COUNT = 16;	// @todo fine-tune this 

	// Transfer XTAL bias to bandgap (power saving)
	//HW_AUDIOOUT_REFCTRL.B.XTAL_BGR_BIAS = 1;
	// Transfer DAC bias from self-bias to bandgap (power saving)
	HW_AUDIOOUT_PWRDN.B.SELFBIAS = 1;

	HW_AUDIOIN_CTRL.B.CLKGATE = true;
	HW_AUDIOOUT_CTRL.B.CLKGATE = false;

	msleep(2);
}


int
initializeAudioout(audio_stream_t *s)
{
	audio_buf_t *dac_b = s->audio_buffers;

	onHWmute();
	/* DMA setup & enable IRQ */
	setupDMA(dac_b->phyStartaddr, s->fragsize*s->nbfrags);
	initializeDac();
	setDefaultVGA();
	return 0;
}

// Bit-bang samples to the dac
void dac_blat(unsigned short value, int num_samples)
{
	unsigned sample;

	//	ASSERT(HW_APBX_CHn_DEBUG1(1).B.STATEMACHINE == 0);

	sample = (unsigned)value;	// copy to other channel
	sample += (sample << 16);

	while (num_samples > 0) {
		if (HW_AUDIOOUT_DACDEBUG.B.FIFO_STATUS == 1) {
			HW_AUDIOOUT_DATA_WR(sample);
			num_samples--;
		}
	}
}

void dac_ramp_partial(short *u16DacValue, short target_value, short step)
{
	if (step < 0) {
		while (*u16DacValue > target_value) {
			dac_blat(*u16DacValue, 1);
			*u16DacValue += step;
		}
	} else {
		while (*u16DacValue < target_value) {
			dac_blat(*u16DacValue, 1);
			*u16DacValue += step;
		}
	}
}


void dac_ramp_up(uint8_t newVagValue, uint8_t oldVagValue)
{
	int16_t s16DacValue = 0x7FFF;	// set to lowest voltage	

	DPRINTK("\n");

	dac_ramp_partial(&s16DacValue, 0x7000, -2);
	dac_ramp_partial(&s16DacValue, 0x6000, -3);
	dac_ramp_partial(&s16DacValue, 0x5000, -3);
	dac_ramp_partial(&s16DacValue, 0x4000, -4);

	// Slowly raise VAG back to set level
	while (newVagValue != oldVagValue) {
		if (newVagValue > oldVagValue) {
			BF_CLR(AUDIOOUT_REFCTRL, VAG_VAL);
			BF_SETV(AUDIOOUT_REFCTRL, VAG_VAL, --newVagValue);
			dac_blat(s16DacValue, 512);
		} else {
			BF_CLR(AUDIOOUT_REFCTRL, VAG_VAL);
			BF_SETV(AUDIOOUT_REFCTRL, VAG_VAL, ++newVagValue);
			dac_blat(s16DacValue, 512);
		}
	}

	dac_ramp_partial(&s16DacValue, 0x3000, -4);
	dac_ramp_partial(&s16DacValue, 0x2000, -3);
	dac_ramp_partial(&s16DacValue, 0x1000, -2);
	dac_ramp_partial(&s16DacValue, 0x0000, -1);
}

/* add dhsong */
static void wakeup_Dac(void)
{
	int16_t u16DacValue;
	uint8_t oldVagValue=DEF_VGA_VAL;
	uint8_t newVagValue=0;

	// ************************************************
	// *** Initialize Digital Filter DAC (AUDIOOUT) ***
	// ************************************************
//	dac_init(); //disable to prevent pop noise, dhsong

	// Clear SFTRST, Turn off Clock Gating.
	BF_CS2(AUDIOOUT_CTRL, SFTRST, 0, CLKGATE, 0);
	BF_CS1(AUDIOOUT_ANACLKCTRL, CLKGATE, 0);

	// Force DAC to issue Quiet mode until we are ready to run.
	BF_SET(AUDIOOUT_CTRL, DAC_ZERO_ENABLE);
	
	BF_CS1(AUDIOOUT_PWRDN, DAC, 0);
	BF_CS1(AUDIOOUT_CTRL, WORD_LENGTH, 1);
	BF_CS1(AUDIOOUT_CTRL, DMAWAIT_COUNT, 16); // @todo fine-tune this      

	// Update DAC volume over zero-crossings
	//BF_SET(AUDIOOUT_DACVOLUME, EN_ZCD);

	//BW_AUDIOOUT_PWRDN_SPEAKER(1);
	// Setup Output for a clean signal
//	BF_CS1(AUDIOOUT_ANACTRL, HP_HOLD_GND, true); // Prevent Digital Noise  //diasble to prevent pop noise, dhsong
	BW_AUDIOOUT_PWRDN_DAC(0);
	BW_AUDIOOUT_PWRDN_ADC(1);
	#if CAPLESS_MODE
	BW_AUDIOOUT_PWRDN_CAPLESS(0);
	printk("power on capless mode\n");
	#else
	BW_AUDIOOUT_PWRDN_CAPLESS(1);
	#endif
	//BW_AUDIOOUT_PWRDN_HEADPHONE(0);

	BF_CS1(AUDIOOUT_ANACTRL, HP_CLASSAB, false);	// Disable ClassAB  
	// Reduce VAG so that we can get closer to ground when starting 
	newVagValue = 0;
	BF_CLR(AUDIOOUT_REFCTRL, VAG_VAL);
	BF_SETV(AUDIOOUT_REFCTRL, VAG_VAL, newVagValue);

	// Set Sample Rate Convert Hold, Sample Rate Conversion Factor Integer and Fractional values.
	// Use Base Rate Multiplier default value.
	setDacSamplingRate(stmp_dac_value.samplingRate);

	// Set defaults (DAS - LR_SWAP should not be here)
	BF_CS2(AUDIOOUT_CTRL, LR_SWAP, 0,       INVERT_1BIT, 0);
	BF_CS2(AUDIOOUT_CTRL, DMAWAIT_COUNT, 0, EDGE_SYNC, 0);
	BF_CS2(AUDIOOUT_CTRL, SS3D_EFFECT, 0,   LOOPBACK, 0);

	BF_CLR(AUDIOOUT_CTRL, DAC_ZERO_ENABLE); // Stop sending zeros

	// Set APBX DMA Engine to Run Mode.
	BF_CS2(APBX_CTRL0, SFTRST, 0, CLKGATE, 0);
	startDac();

	//setDacVolume(0xff);	//pjdagon

	// Full deflection negative
	u16DacValue = 0x7fff;
	
	// Write DAC to ground for several milliseconds
	dac_blat(u16DacValue, 512);
	
	// PowerUp Headphone, Unmute
	BF_CLR(AUDIOOUT_PWRDN, HEADPHONE);	// Pwrup HP amp
	
	dac_blat(u16DacValue, 256);
	
//	BF_CS1(AUDIOOUT_ANACTRL, HP_HOLD_GND, false); // Release hold to GND //diasble to prevent pop noise, dhsong
	dac_blat(u16DacValue, 256);

	//dac_ramp_up(newVagValue, oldVagValue);  // Ramp to VAG //disable to reduce wakeup delay, 20080930 dhsong
	
	// Need to wait here - block for the time being...		 
	//msleep(1700);
	BF_CS1(AUDIOOUT_ANACTRL, HP_CLASSAB, true); // Enable ClassAB	 
	return;
}

static void 
initializeDac(void)
{
#if 0 // for 36xx
	// ************************************************
	// *** Initialize Digital Filter DAC (AUDIOOUT) ***
	// ************************************************
	// Clear SFTRST, Turn off Clock Gating.
	BF_CS2(AUDIOOUT_CTRL, SFTRST, 0, CLKGATE, 0);
	BF_CS1(AUDIOOUT_ANACLKCTRL, CLKGATE, 0);
	
	BF_CS1(AUDIOOUT_PWRDN, DAC, 0);
	
	BF_CS1(AUDIOOUT_CTRL, WORD_LENGTH, 1);
	
	//BW_AUDIOOUT_ANACTRL_HP_HOLD_GND(1);
	BW_AUDIOOUT_ANACTRL_HP_CLASSAB(1);
	// Enable IRQ to ICOLL.
	// BF_CS1(AUDIOOUT_CTRL, FIFO_ERROR_IRQ_EN, 1);
	
	// Update DAC volume over zero-crossings
	BF_SET(AUDIOOUT_DACVOLUME, EN_ZCD);
	/* below register is removed in 37xx */
	//BF_SET(AUDIOOUT_ANACTRL,   EN_ZCD);
	
	/* below register is removed in 37xx */
	//BW_AUDIOOUT_PWRDN_SPEAKER(1);
	BW_AUDIOOUT_PWRDN_DAC(0);
	BW_AUDIOOUT_PWRDN_ADC(1);
#if CAPLESS_MODE
	BW_AUDIOOUT_PWRDN_CAPLESS(0);
	printk("power on capless mode\n");
#else
	BW_AUDIOOUT_PWRDN_CAPLESS(1);
#endif
	BW_AUDIOOUT_PWRDN_HEADPHONE(0);
	
	// Set Sample Rate Convert Hold, Sample Rate Conversion Factor Integer and Fractional values.
	// Use Base Rate Multiplier default value.
	setDacSamplingRate(stmp_dac_value.samplingRate);
	
	// Set defaults (DAS - LR_SWAP should not be here)
	BF_CS2(AUDIOOUT_CTRL, LR_SWAP, 0,		INVERT_1BIT, 0);
	BF_CS2(AUDIOOUT_CTRL, DMAWAIT_COUNT, 0, EDGE_SYNC, 0);
	BF_CS2(AUDIOOUT_CTRL, SS3D_EFFECT, 0,	LOOPBACK, 0);
	
	// Set APBX DMA Engine to Run Mode.
	BF_CS2(APBX_CTRL0, SFTRST, 0, CLKGATE, 0);
	startDac();
	
	setDacVolume(0xff); 	//pjdagon
	
	//BW_AUDIOOUT_ANACTRL_HP_HOLD_GND(0);
	
#else // for 37xx

	int16_t u16DacValue;
	uint8_t oldVagValue=DEF_VGA_VAL;
	uint8_t newVagValue=0;

	// ************************************************
	// *** Initialize Digital Filter DAC (AUDIOOUT) ***
	// ************************************************
	dac_init();

	// Clear SFTRST, Turn off Clock Gating.
	BF_CS2(AUDIOOUT_CTRL, SFTRST, 0, CLKGATE, 0);
	BF_CS1(AUDIOOUT_ANACLKCTRL, CLKGATE, 0);

	// Force DAC to issue Quiet mode until we are ready to run.
	BF_SET(AUDIOOUT_CTRL, DAC_ZERO_ENABLE);
	
	BF_CS1(AUDIOOUT_PWRDN, DAC, 0);
	BF_CS1(AUDIOOUT_CTRL, WORD_LENGTH, 1);
	BF_CS1(AUDIOOUT_CTRL, DMAWAIT_COUNT, 16); // @todo fine-tune this      

	// Update DAC volume over zero-crossings
	//BF_SET(AUDIOOUT_DACVOLUME, EN_ZCD);

	//BW_AUDIOOUT_PWRDN_SPEAKER(1);
	// Setup Output for a clean signal
	BF_CS1(AUDIOOUT_ANACTRL, HP_HOLD_GND, true);	// Prevent Digital Noise
	BW_AUDIOOUT_PWRDN_DAC(0);
	BW_AUDIOOUT_PWRDN_ADC(1);
	#if CAPLESS_MODE
	BW_AUDIOOUT_PWRDN_CAPLESS(0);
	printk("power on capless mode\n");
	#else
	BW_AUDIOOUT_PWRDN_CAPLESS(1);
	#endif
	//BW_AUDIOOUT_PWRDN_HEADPHONE(0);

	BF_CS1(AUDIOOUT_ANACTRL, HP_CLASSAB, false);	// Disable ClassAB  
	// Reduce VAG so that we can get closer to ground when starting 
	newVagValue = 0;
	BF_CLR(AUDIOOUT_REFCTRL, VAG_VAL);
	BF_SETV(AUDIOOUT_REFCTRL, VAG_VAL, newVagValue);

	// Set Sample Rate Convert Hold, Sample Rate Conversion Factor Integer and Fractional values.
	// Use Base Rate Multiplier default value.
	setDacSamplingRate(stmp_dac_value.samplingRate);

	// Set defaults (DAS - LR_SWAP should not be here)
	BF_CS2(AUDIOOUT_CTRL, LR_SWAP, 0,       INVERT_1BIT, 0);
	BF_CS2(AUDIOOUT_CTRL, DMAWAIT_COUNT, 0, EDGE_SYNC, 0);
	BF_CS2(AUDIOOUT_CTRL, SS3D_EFFECT, 0,   LOOPBACK, 0);

	BF_CLR(AUDIOOUT_CTRL, DAC_ZERO_ENABLE); // Stop sending zeros

	// Set APBX DMA Engine to Run Mode.
	BF_CS2(APBX_CTRL0, SFTRST, 0, CLKGATE, 0);
	startDac();

	setDacVolume(0xff);	//pjdagon

	// Full deflection negative
	u16DacValue = 0x7fff;
	
	// Write DAC to ground for several milliseconds
	dac_blat(u16DacValue, 512);
	
	// PowerUp Headphone, Unmute
	BF_CLR(AUDIOOUT_PWRDN, HEADPHONE);	// Pwrup HP amp
	
	dac_blat(u16DacValue, 256);
	
	BF_CS1(AUDIOOUT_ANACTRL, HP_HOLD_GND, false);	// Release hold to GND
	dac_blat(u16DacValue, 256);
	dac_ramp_up(newVagValue, oldVagValue);	// Ramp to VAG
	
	// Need to wait here - block for the time being...		 
	//msleep(1700);
	BF_CS1(AUDIOOUT_ANACTRL, HP_CLASSAB, true); // Enable ClassAB	 
#endif
	return;
	
}   // initialize()

void initDacSync(audio_stream_t *s)
{
	audio_buf_t *dac_b = s->audio_buffers;
	int vol=0;

    DPRINTK("[DRIVER] initDacSync\n");

	vol = getDacVolume();
	if(isHWMuteon == 0 && vol > 0)
	{
		offHWmute();	
	}
	/* if volume MAX_USER_VOLUME, set core level to Max */
	setDacVolume(vol);	
	
	#ifdef USE_MAX_CORE_LEVEL
	if(stmp_dac_value.sw_ver == VER_COMMON)
	{
		if(vol == MAX_USER_VOLUME)
			setCoreLevel(vol);
	}
	#endif
	/* HP input select */
	BW_AUDIOOUT_HPVOL_SELECT(0x0);

	BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ(1);
	BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ_EN(1);
	/* Start DMA channel(s) by incrementing semaphore. */
	BF_WRn(APBX_CHn_SEMA, 1, INCREMENT_SEMA, 2); 
	dac_b->initDevice = 1;
	setPlayingStatus(PLAYING);
	/* for sleep utill audio stop completely */
	dac_b->initDevice = 1;	
}

void initDacWrite(audio_stream_t *s, const char *buffer)
{
	decoder_buf_t *decoder_b = s->decoder_buffers;
	audio_buf_t *dac_b = s->audio_buffers;
	int buffer_size = 0;
	
	buffer_size = getBufSize();

	/* to fill decoder buffer and copy to dac buffer from decoder */
	if(dac_b->IsFillDac == 0)
	{
		/* to prevent sound before audio fade in */
		#ifdef HP_VOLUME_CONTROL
		BF_CS2(AUDIOOUT_HPVOL, 
			VOL_LEFT,  HP_VOLUME_MIN, 
			VOL_RIGHT, HP_VOLUME_MIN);
		#else
	    BF_CS2(AUDIOOUT_DACVOLUME, 
	       VOLUME_LEFT,  DAC_VOLUME_MIN, 
    	   VOLUME_RIGHT, DAC_VOLUME_MIN);
		#endif
		
		/* Note that in most cases OSS will wait until full fragments have been written before starting the playback. */
		if(decoder_b->decoderValidBytes == buffer_size)
			dac_b->IsFillDac = 1;
	}
	/* after copy to dac buffer.. to fill decoder buffer again to prevent buffer underflow */
	if(dac_b->IsFillDac == 1)
	{
		if(decoder_b->decoderValidBytes == buffer_size)
		{
			/* for volume fade in at audio start/restart */
			if(stmp_dac_value.dsp_fade==1)
				setVolumeFade(FADE_IN); 
			else
			{
				setDacVolume(stmp_dac_value.volume);
			#ifdef USE_MAX_CORE_LEVEL
				if(stmp_dac_value.sw_ver == VER_COMMON)
				{
					if(stmp_dac_value.volume == MAX_USER_VOLUME)
						setCoreLevel(stmp_dac_value.volume);
				}
			#endif
			}
			/* HP input select */
			BW_AUDIOOUT_HPVOL_SELECT(0x0);
			/* Start DMA channel(s) by incrementing semaphore. */
			BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ(1);
			BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ_EN(1);
			BF_WRn(APBX_CHn_SEMA, 1, INCREMENT_SEMA, 2);
			//HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ);

			DPRINTK("[DRIVER] start IRQ!!! [%u]\n", getDacPointer(dac_b->phyStartaddr));
			dac_b->initDevice = 1;
			setPlayingStatus(PLAYING);
			if(isHWMuteon == 0 && stmp_dac_value.volume>0)
			{
				//setDefaultVGA();
				offHWmute();	
			}
		}
	}

}

int onHWmute(void)
{
#ifdef CONFIG_BOARD_Q2
    static int bank = 0;
    static int pin = 0;

	if(stmp_dac_value.board_revision == 4)
	{
		bank = 0;
		pin = 21;
	}
	else
	{
		bank = 0;
		pin = 12;
	}

	stmp37xx_gpio_set_af( pin_GPIO(bank,  pin), GPIO_MODE); //3); //muxsel set gpio func
	stmp37xx_gpio_set_dir( pin_GPIO(bank,  pin), GPIO_DIR_OUT); //output
	stmp37xx_gpio_set_level( pin_GPIO(bank,  pin), 0); //low

	/* for Q2 - BANK0 21 - MUX Reg1 10/11 : high(mute off), low(mute on) */
	/*HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE );
	HW_PINCTRL_MUXSEL1_CLR(0x00000C00);
	HW_PINCTRL_MUXSEL1_SET(0x00000C00);
	HW_PINCTRL_DRIVE0_SET(0x00000000);
	HW_PINCTRL_DOE0_CLR(0x00200000);
	HW_PINCTRL_DOE0_SET(0x00200000);
	HW_PINCTRL_DOUT0_CLR(0x00200000);
	HW_PINCTRL_DOUT0_SET(0x00000000);
	*/
#else
	HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE );
	HW_PINCTRL_MUXSEL3_CLR(0x00003000);
	HW_PINCTRL_MUXSEL3_SET(0x00003000);
	HW_PINCTRL_DRIVE1_SET(0x00000000);
	HW_PINCTRL_DOE1_CLR(0x00400000);
	HW_PINCTRL_DOE1_SET(0x00400000);
	HW_PINCTRL_DOUT1_CLR(0x00400000);
	HW_PINCTRL_DOUT1_SET(0x00400000);
#endif

	DPRINTK("onHWmute\n");
	isHWMuteon=0;
	return 0;
}

int offHWmute(void)
{
	/* hw mute off */
#ifdef CONFIG_BOARD_Q2
    /* for Q2 - BANK0 21 - MUX Reg1 10/11 : high(mute off), low(mute on) */
    static int bank = 0; 
    static int pin = 0; 
     
	if(stmp_dac_value.board_revision == 4)
	{
		bank = 0;
		pin = 21;
	}
	else
	{
		bank = 0;
		pin = 12;
	}
	
	stmp37xx_gpio_set_af( pin_GPIO(bank,  pin), GPIO_MODE); //3); //muxsel set gpio func
	stmp37xx_gpio_set_dir( pin_GPIO(bank,  pin), GPIO_DIR_OUT); //output
	stmp37xx_gpio_set_level( pin_GPIO(bank,  pin), 1); //high
	/*
	HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE );
	HW_PINCTRL_MUXSEL1_CLR(0x00000C00);
	HW_PINCTRL_MUXSEL1_SET(0x00000C00);
	HW_PINCTRL_DRIVE0_SET(0x00000000);
	HW_PINCTRL_DOE0_CLR(0x00200000);
	HW_PINCTRL_DOE0_SET(0x00200000);
	HW_PINCTRL_DOUT0_CLR(0x00200000);
	HW_PINCTRL_DOUT0_SET(0x00200000);
	*/
#else	
	HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | BM_PINCTRL_CTRL_CLKGATE );
	HW_PINCTRL_MUXSEL3_CLR(0x00003000);
	HW_PINCTRL_MUXSEL3_SET(0x00003000); //BANK1 22 -> GPIO mode to mute off audio
	HW_PINCTRL_DRIVE1_SET(0x00000000);
	HW_PINCTRL_DOE1_CLR(0x00400000);
	HW_PINCTRL_DOE1_SET(0x00400000);
	HW_PINCTRL_DOUT1_CLR(0x00400000);
	HW_PINCTRL_DOUT1_SET(0x00000000);
#endif	

	DPRINTK("offHWmute\n");
	isHWMuteon=1;
	return 0;
}

static void subDecoderValidBytes(decoder_buf_t *decoderData, signed int validBytes)
{       
	spin_lock_irq(&dac_lock);
	decoderData->decoderValidBytes-=validBytes;
	if (decoderData->decoderValidBytes < 0)
	{
		//printk("Underrun!\n");
		decoderData->decoderValidBytes=0;
	}
	//printk("sub [%d] \n", decoderData->decoderValidBytes);
	spin_unlock_irq(&dac_lock);
}

void
setupDMA(unsigned int p_dacPointer, unsigned long bufferSize)
{
	unsigned int  resetMask=0;
	int wait=0, n=0, i=0, desc_size=0, phy_ptr_n=0;
	unsigned int phys_dacpointer = p_dacPointer;
	unsigned long sz = 0;

	n = bufferSize / MIN_STREAM_SIZE;
	sz = bufferSize / n;
	desc_size = DAC_DESC_SIZE;

	DPRINTK("[DRIVER] setupDMA : phys_dacpointer=%x, bufferSize=%d, n=%d, sz=%d, desc_size=%d\n", phys_dacpointer, bufferSize, n, sz, desc_size);

	if(sz == 0)
		panic("[Serious Error] dma desc size zero!!\n");

	for (i=0; i<n; i++)
	{
		phy_ptr_n++;
		if(phy_ptr_n == n)
			phy_ptr_n = 0;
		
		/* dac dma desc 0 init */
		dac_dma_cmd.desc_cmd_ptr[i*3] = dac_dma_cmd.phys_desc_cmd_ptr + desc_size*phy_ptr_n;
		dac_dma_cmd.desc_cmd_ptr[i*3+1] = 
			(BF_APBX_CHn_CMD_XFER_COUNT	 (sz) |
					 BF_APBX_CHn_CMD_CHAIN(1) |
					 BF_APBX_CHn_CMD_SEMAPHORE(1) |
					 BF_APBX_CHn_CMD_IRQONCMPLT(1) |
					 BV_FLD(APBX_CHn_CMD, COMMAND, DMA_READ));
		dac_dma_cmd.desc_cmd_ptr[i*3+2] = (reg32_t) phys_dacpointer+(sz*i);              // Point to input buffer.
	}

	HW_APBX_CTRL0_CLR(BM_APBX_CTRL0_SFTRST | BM_APBX_CTRL0_CLKGATE);
	// Reset DMA channels 1 AUDIOOUT.
	resetMask = BF_APBX_CTRL0_RESET_CHANNEL(BV_APBX_CTRL0_RESET_CHANNEL__AUDIOOUT);
	HW_APBX_CTRL0_WR(resetMask);
	
	// Poll for both channels to complete reset.
	for (wait = 0; wait < 10; wait++)
	{
		if ( (resetMask & HW_APBX_CTRL0_RD()) == 0 )
		  break;
	}
	
	if (wait == 10)
	{
		printk("\nERROR: Timeout waiting for DMA Channel(s) to Reset.\n");
	}

	// Set up DMA channel configuration.
	BF_WRn(APBX_CHn_NXTCMDAR, 1, CMD_ADDR, (reg32_t)dac_dma_cmd.phys_desc_cmd_ptr);

	// Enable IRQ
	#if 1
	BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ(1);
	BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ_EN(1);
	#else
	BF_CLR(APBX_CTRL1, CH1_CMDCMPLT_IRQ);
	BF_SET(APBX_CTRL1, CH1_CMDCMPLT_IRQ_EN);
	#endif

	/* removed by jinho.lim - for 37xx */
	//BF_WRn(APBX_CHn_SEMA, 1, INCREMENT_SEMA, 1);

} 

void
setDMADesc(unsigned int p_dacPointer, unsigned long bufferSize)
{
	int n=0, i=0, desc_size=0, phy_ptr_n=0;
	unsigned int phys_dacpointer = p_dacPointer;
	unsigned long sz = 0;

	n = bufferSize / MIN_STREAM_SIZE;
	sz = bufferSize / n;
	desc_size = DAC_DESC_SIZE;

	if(sz == 0)
		panic("[Serious Error] dma desc size zero!!\n");

	for (i=0; i<n; i++)
	{
		phy_ptr_n++;
		if(phy_ptr_n == n)
			phy_ptr_n = 0;
		
		/* dac dma desc 0 init */
		dac_dma_cmd.desc_cmd_ptr[i*3] = dac_dma_cmd.phys_desc_cmd_ptr + desc_size*phy_ptr_n;
		dac_dma_cmd.desc_cmd_ptr[i*3+1] = 
			(BF_APBX_CHn_CMD_XFER_COUNT	 (sz) |
					 BF_APBX_CHn_CMD_CHAIN(1) |
					 BF_APBX_CHn_CMD_SEMAPHORE(1) |
					 BF_APBX_CHn_CMD_IRQONCMPLT(1) |
					 BV_FLD(APBX_CHn_CMD, COMMAND, DMA_READ));
		dac_dma_cmd.desc_cmd_ptr[i*3+2] = (reg32_t) phys_dacpointer+(sz*i);              // Point to input buffer.
	}

	// Set up DMA channel configuration.
	BF_WRn(APBX_CHn_NXTCMDAR, 1, CMD_ADDR, dac_dma_cmd.phys_desc_cmd_ptr);

} 


void startDac(void)
{
	BF_CS2(AUDIOOUT_CTRL, SFTRST, 0, CLKGATE, 0);
	BF_CS1(AUDIOOUT_CTRL, RUN, 1);
}

int stopDacProcessing(audio_stream_t *audio_s)
{
	decoder_buf_t *decoder_b = audio_s->decoder_buffers;
	audio_buf_t *dac_b = audio_s->audio_buffers;
	int buf_size=0;
	
	#ifdef HP_VOLUME_CONTROL
	BF_CS2(AUDIOOUT_HPVOL, 
		VOL_LEFT,  HP_VOLUME_MIN, 
		VOL_RIGHT, HP_VOLUME_MIN);
	#else
	BF_CS2(AUDIOOUT_DACVOLUME, 
		VOLUME_LEFT,  DAC_VOLUME_MIN, 
		VOLUME_RIGHT, DAC_VOLUME_MIN);
	#endif

	onHWmute();

//	if( checkDacPtr(audio_s) != 0 )
//		return -1;

	/* disable DAC DMA irq */
	BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ_EN(0);

	/* init flag */
	decoder_b->decoderWritePointer = 0;
	audio_s->audio_buffers->initDevice = 0;
	setPlayingStatus(NO_PLAYING);

	/* if current VDD is MAX, return previous VDD */
	if(stmp_dac_value.volume == MAX_USER_VOLUME)
	{
		setCoreLevel(MAX_USER_VOLUME);
	}

	/* reset Dac DMA Ptr */
	buf_size = getBufSize();
	setDMADesc(dac_b->phyStartaddr, buf_size);
	return 0;
}

void startHWAudioout(void)
{
	//setupDMA(STMP37XX_SRAM_AUDIO_KERNEL_START, stmp_dac_value.buf_size);
	setupDMA(OCRAM_DAC_START, stmp_dac_value.buf_size);
	//initializeDac();
	wakeup_Dac(); //add dhsong
	setDefaultVGA();	
	DPRINTK("[DRIVER] POWER[%x] RUN[%x] -- #AUDIO START# --\n",HW_AUDIOOUT_PWRDN_RD(), HW_AUDIOOUT_CTRL_RD());
}

void stopHWAudioout(void)
{
	/* HW mute on */
	onHWmute();
	/* power down audioout/in */
	BF_CS1(AUDIOOUT_ANACLKCTRL, CLKGATE, 0);
	BW_AUDIOOUT_PWRDN_DAC(1);
	BW_AUDIOOUT_PWRDN_ADC(1);
	BW_AUDIOOUT_PWRDN_CAPLESS(1);
	BW_AUDIOOUT_PWRDN_HEADPHONE(1);

	/* audioout reset */
	BF_CS1(AUDIOOUT_CTRL, RUN, 0);
	BF_CS1(AUDIOOUT_ANACLKCTRL, CLKGATE, 1);
	BF_CS2(AUDIOOUT_CTRL, SFTRST, 1, CLKGATE, 1);
	DPRINTK("[DRIVER] POWER[%x] RUN[%x] -- #AUDIO RESET# --\n",HW_AUDIOOUT_PWRDN_RD(), HW_AUDIOOUT_CTRL_RD());
	/* audioin reset */
	BF_CS1(AUDIOIN_CTRL, RUN, 0);
	BF_CS2(AUDIOIN_CTRL, SFTRST, 1, CLKGATE, 1);  // cleared LOOPBACK to see 1 bit outputs.
	BF_CS1(AUDIOIN_ANACLKCTRL, CLKGATE, 1);    
}


void setBufSize(int bufferSize)
{
	stmp_dac_value.buf_size = bufferSize;
}

void setHPVolume(int val)
{
	DPRINTK("===> setHPVolume val=%x\n", val);
	stmp_dac_value.hpvol_reg = val; 

	BF_CS3(AUDIOOUT_HPVOL, 
	    MUTE,    MUTE_OFF, 
	    VOL_LEFT,  stmp_dac_value.hpvol_reg, 
	    VOL_RIGHT, stmp_dac_value.hpvol_reg);
}

#ifdef USE_MAX_CORE_LEVEL	
const unsigned int HP_CM_VOL_TBL[31] =
{
	1,
	0x7E, 0x76, 0x6C, 0x62, 0x5A, 0x54, 0x4E, 0x4A, 0x46, 0x42, 
	0x3E, 0x3A, 0x36, 0x32, 0x30, 0x2E, 0x2C, 0x2A, 0x28, 0x26, 
	0x24, 0x22, 0x20, 0x1E, 0x1C, 0x18, 0x14, 0x10, 0x0C, 0x0C
};

const unsigned int HP_FR_VOL_TBL[31] =
{
	1,
	0x7E, 0x76, 0x6C, 0x62, 0x5A, 0x54, 0x4E, 0x4A, 0x46, 0x42, 
	0x3E, 0x3A, 0x36, 0x32, 0x30, 0x2E, 0x2C, 0x2A, 0x28, 0x26, 
	0x24, 0x22, 0x20, 0x1E, 0x1C, 0x18, 0x14, 0x10, 0x0C, 0x0C
};

#if 1
const unsigned int DAC_CM_VOL_TBL[31] =
{
	1,
	39, 46, 51, 56, 60, 63, 66, 68, 70, 72, 
	74, 76, 78, 80, 81, 82, 83, 84, 85, 86, 
	87, 88, 89, 90, 91, 93, 95, 97, 99, 99
};

const unsigned int DAC_FR_VOL_TBL[31] =
{
	1,
	39, 44, 48, 51, 54, 56, 58, 60, 62, 64, 
	66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 
	76, 77, 79, 81, 83, 85, 87, 89, 91, 93
};
#else
const unsigned int DAC_CM_VOL_TBL[31] =
{
	1,
	36, 45, 54, 57, 60, 63, 66, 68, 70, 72, 
	74, 76, 78, 80, 81, 82, 83, 84, 85, 86, 
	87, 88, 89, 90, 91, 93, 95, 97, 99, 99
};

const unsigned int DAC_FR_VOL_TBL[31] =
{
	1,
	36, 42, 48, 51, 54, 56, 58, 60, 62, 64, 
	66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 
	76, 77, 79, 81, 83, 85, 87, 89, 91, 93
};
#endif

#else
const unsigned int DAC_CM_VOL_TBL[31] =
{
	1,
	38, 45, 50, 55, 59, 62, 64, 66, 68, 70, 
	72, 74, 76, 78, 79, 80, 81, 82, 83, 84, 
	85, 86, 87, 88, 89, 91, 93, 95, 97, 99
};

const unsigned int DAC_FR_VOL_TBL[31] =
{
	1,
	38, 43, 47, 50, 53, 55, 57, 59, 61, 63, 
	65, 67, 69, 71, 72, 73, 74, 75, 76, 77, 
	78, 79, 80, 81, 82, 84, 86, 88, 90, 92
};
#endif


int userVol2HPVol(int user_volume)
{
	int real_value = 0;

	if(stmp_dac_value.sw_ver == VER_COMMON)
	{
		printk("NO_PROCESSING VER_COMMON %d\n",user_volume);
		real_value = HP_CM_VOL_TBL[user_volume];
	}
	else if(stmp_dac_value.sw_ver == VER_FR)
	{
		printk("NO_PROCESSING VER_FR %d\n",user_volume);
		real_value = HP_FR_VOL_TBL[user_volume];
	}
	
	if(stmp_dac_value.current_post == DNSE_PROCESSING)
	{
		if(stmp_dac_value.gain_attenuation == 6)
		{
			if(stmp_dac_value.sw_ver == VER_COMMON)
			{
				printk("6-DNSE_PROCESSING COMMON %d\n",user_volume);
				if(user_volume == 30) real_value = 0x0C;
				else if(user_volume == 29) real_value = 0x0C; 
				else if(user_volume == 28) real_value = 0x0D; 
				else if(user_volume == 27) real_value = 0x0E; 
				else if(user_volume == 26) real_value = 0x0F; 
				else if(user_volume >= 1) real_value -= 6<<1; 
				else real_value = 0x7E;
			}
			else if(stmp_dac_value.sw_ver == VER_FR)
			{
				printk("6-DNSE_PROCESSING FR %d\n",user_volume);
				if(user_volume == 30) real_value = 0x18; 
				else if(user_volume == 29) real_value = 0x19; 
				else if(user_volume == 28) real_value = 0x1B; 
				else if(user_volume == 27) real_value = 0x1D; 
				else if(user_volume == 26) real_value = 0x1F; 
				else if(user_volume >= 1) real_value -= 6<<1;
				else real_value = 0x7E;
			}
		}
		else if(stmp_dac_value.gain_attenuation == 9)
		{
			if(stmp_dac_value.sw_ver == VER_COMMON)
			{
				printk("9-DNSE_PROCESSING COMMON %d\n",user_volume);
				if(user_volume == 30) real_value = 0x0B; 
				else if(user_volume == 29) real_value = 0x0B; 
				else if(user_volume == 28) real_value = 0x0C;
				else if(user_volume == 27) real_value = 0x0D; 
				else if(user_volume == 26) real_value = 0x0E;
				else if(user_volume == 25) real_value = 0x0F; 
				else if(user_volume == 24) real_value = 0x10;
				else if(user_volume == 23) real_value = 0x11;
				else if(user_volume == 22) real_value = 0x12;
				else if(user_volume == 21) real_value = 0x13;
				else if(user_volume >= 1) real_value -= 9<<1; 
				else real_value = 0x7E;
			}
			else if(stmp_dac_value.sw_ver == VER_FR)
			{
				printk("9-DNSE_PROCESSING FR %d\n",user_volume);
				if(user_volume == 30) real_value = 0x17; 
				else if(user_volume == 29) real_value = 0x18;
				else if(user_volume == 28) real_value = 0x19; 
				else if(user_volume == 27) real_value = 0x1A;
				else if(user_volume == 26) real_value = 0x1B; 
				else if(user_volume == 25) real_value = 0x1C; 
				else if(user_volume == 24) real_value = 0x1D; 
				else if(user_volume >= 1) real_value -= 9<<1;
				else real_value = 0x7E;
			}
		}
		else if(stmp_dac_value.gain_attenuation == 12)
		{
			if(stmp_dac_value.sw_ver == VER_COMMON)
			{
				printk("12-DNSE_PROCESSING COMMON %d\n",user_volume);
				if(user_volume == 30) real_value = 0x05; 
				else if(user_volume == 29) real_value = 0x05; 
				else if(user_volume == 28) real_value = 0x06; 
				else if(user_volume == 27) real_value = 0x07; 
				else if(user_volume == 26) real_value = 0x08; 
				else if(user_volume == 25) real_value = 0x09; 
				else if(user_volume == 24) real_value = 0x0A;
				else if(user_volume == 23) real_value = 0x0B;
				else if(user_volume == 22) real_value = 0x0C;
				else if(user_volume == 21) real_value = 0x0D;
				else if(user_volume >= 1) real_value -= 12<<1;
				else real_value = 0x7E;
			}
			else if(stmp_dac_value.sw_ver == VER_FR)
			{
				printk("12-DNSE_PROCESSING FR %d\n",user_volume);
				if(user_volume == 30) real_value = 0x13;
				else if(user_volume == 29) real_value = 0x14;
				else if(user_volume == 28) real_value = 0x15;
				else if(user_volume == 27) real_value = 0x16;
				else if(user_volume == 26) real_value = 0x17;
				else if(user_volume == 25) real_value = 0x18;
				else if(user_volume == 24) real_value = 0x19; 
				else if(user_volume == 23) real_value = 0x1A;
				else if(user_volume == 22) real_value = 0x1B;
				else if(user_volume >= 1) real_value -= 12<<1;
				else real_value = 0x7E;
			}
		}
	}
	return real_value;

}

void setDacVolume(int user_volume)
{
	int dac_volume = 0;
	int hp_volume = 0;
	/* dac volume */
	/* Volume ranges from full scale 0dB (0xFF) to -100dB (0x37). Each increment of this bit-field causes */
	/* a half dB increase in volume. Note that values 0x00-0x37 all produce the same attenuation level of -100dB. */
	/* So convert 0 -> 100 volume to 0x37 -> 0xFF */

	stmp_dac_value.volume = user_volume;

	/* to remove of pop noise : pjdragon */
	if(user_volume == 0xff )
		setHPVolume(0x7e);
	else 
		setHPVolume(0x0C);

	dac_volume = userVol2DacVol(user_volume);
	stmp_dac_value.volume_reg = 0x37 + dac_volume;
	//printk("===> Dac volume=%x, user_volume=%d\n", stmp_dac_value.volume_reg, user_volume);


	BF_CS4(AUDIOOUT_DACVOLUME, 
	    MUTE_LEFT,    MUTE_OFF, 
	    VOLUME_LEFT,  stmp_dac_value.volume_reg, 
	    MUTE_RIGHT,   MUTE_OFF, 
	    VOLUME_RIGHT, stmp_dac_value.volume_reg);

}

static int gFinish_set_user_volume = 0;

void setUserVol_timer(unsigned long arg)
{
	int user_volume = arg;
	//printk("===> VOLUME = %d, stmp_dac_value.volume_reg=%d, user_volume=%d\n",
	//	HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT, stmp_dac_value.volume_reg, user_volume);
	
	if(HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT == stmp_dac_value.volume_reg)
	{
		//printk("===> del_timer\n");
		del_timer(&user_volume_set_timer);
		gFinish_set_user_volume = 1;
		return; 
	}
	else if(HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT < stmp_dac_value.volume_reg)
	{
		HW_AUDIOOUT_DACVOLUME.B.MUTE_LEFT = (user_volume == 0) ? 1 : 0;
		HW_AUDIOOUT_DACVOLUME.B.MUTE_RIGHT = (user_volume == 0) ? 1 : 0;
		HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT = HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT+1;
		HW_AUDIOOUT_DACVOLUME.B.VOLUME_RIGHT = HW_AUDIOOUT_DACVOLUME.B.VOLUME_RIGHT+1;
	}
	else if(HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT > stmp_dac_value.volume_reg)
	{
		HW_AUDIOOUT_DACVOLUME.B.MUTE_LEFT = (user_volume == 0) ? 1 : 0;
		HW_AUDIOOUT_DACVOLUME.B.MUTE_RIGHT = (user_volume == 0) ? 1 : 0;
		HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT = HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT-1;
		HW_AUDIOOUT_DACVOLUME.B.VOLUME_RIGHT = HW_AUDIOOUT_DACVOLUME.B.VOLUME_RIGHT-1;
	}

	if(stmp_dac_value.volume == 0)
	{
		if(isHWMuteon == 1)
			onHWmute();
	}
	else
	{
		if(isHWMuteon == 0)
			offHWmute();
	}
	
	user_volume_set_timer.expires=jiffies+1; // 10msec
	/* register timer */
	add_timer(&user_volume_set_timer);

}

void setUserVolume(int user_volume)
{
	int dac_volume = 0;
	int hp_volume = 0;
	/* dac volume */
	/* Volume ranges from full scale 0dB (0xFF) to -100dB (0x37). Each increment of this bit-field causes */
	/* a half dB increase in volume. Note that values 0x00-0x37 all produce the same attenuation level of -100dB. */
	/* So convert 0 -> 100 volume to 0x37 -> 0xFF */

#ifdef HP_VOLUME_CONTROL
	/* to remove of pop noise : pjdragon */
	if(user_volume == 0xff) 
		setHPVolume(0x7e);
	else 
	{
		hp_volume = userVol2HPVol(user_volume);
		//printk("hp_volume=%d\n", hp_volume);
		if(hp_volume > 0x7E)
			hp_volume = 0x7E;
		setHPVolume(hp_volume);
	}
#else
	stmp_dac_value.volume = user_volume;
	if(stmp_dac_value.isPlaying == NO_PLAYING)
		return;

	setHPVolume(0x0C);

	dac_volume = userVol2DacVol(user_volume);
	stmp_dac_value.volume_reg = 0x37 + dac_volume;
	//printk("===> User volume=%x, user_volume=%d\n", stmp_dac_value.volume_reg, user_volume);

#if 0
	HW_AUDIOOUT_DACVOLUME.B.MUTE_LEFT = (user_volume == 0) ? 1 : 0;
	HW_AUDIOOUT_DACVOLUME.B.MUTE_RIGHT = (user_volume == 0) ? 1 : 0;
	HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT = stmp_dac_value.volume_reg;
	HW_AUDIOOUT_DACVOLUME.B.VOLUME_RIGHT = stmp_dac_value.volume_reg;
#else
	if(gFinish_set_user_volume == 1)
	{
		//printk("===> now user volume setting.... del timer\n");
		del_timer(&user_volume_set_timer);
		gFinish_set_user_volume = 0;
	}
		
	init_timer(&user_volume_set_timer);
	
	user_volume_set_timer.expires=jiffies+1; // 10msec
	user_volume_set_timer.function=setUserVol_timer;
	user_volume_set_timer.data=user_volume;
	
	/* register timer */
	add_timer(&user_volume_set_timer);
#endif
#endif	  
}


void setDefaultVolume(int val)
{
	stmp_dac_value.current_post = NO_PROCESSING;
	stmp_dac_value.gain_attenuation = 0;

	setUserVolume(val);
}

void setPlaySpeed(int val)
{
	stmp_dac_value.play_speed = val;
	//printk("setPlaySpeed=%d\n", stmp_dac_value.play_speed);
	setDacSamplingRate(stmp_dac_value.samplingRate);
}

void setDSPFade(int val)
{
	//printk("===> SETSETSET=%d\n", val);
	stmp_dac_value.dsp_fade = val;
}

int getDSPFade(void)
{
	//printk("===> GETGETGET=%d\n", stmp_dac_value.dsp_fade);
	return stmp_dac_value.dsp_fade;
}

void setSpeakerVolume(int val)
{
	stmp_dac_value.spkvol_reg = val; 
	// 37xx has no spk volume
	//BF_CS1(AUDIOOUT_SPKRVOL, VOL, stmp_dac_value.spkvol_reg);

}

void setSWVersion(unsigned short version)
{
	/* we must check GPIO to know HW Version whether USA/KOR or EU */
	if(version == VER_COMMON)
	{	
		DPRINTK("[DRIVER] setSWVersion [VER_COMMON]\n");
		stmp_dac_value.sw_ver = VER_COMMON;
	}
	else if(version == VER_FR)
	{	
		DPRINTK("[DRIVER] setSWVersion [VER_FR]\n");
		stmp_dac_value.sw_ver = VER_FR;
	}
}

void setBoardRevision(unsigned short revision)
{
	DPRINTK("[DRIVER] setBoardRevision = %u\n", revision);

	stmp_dac_value.board_revision = revision;
}


void setDacSamplingRate(unsigned long samplingRate)
{
	// Set Sample Rate Convert Hold, Sample Rate Conversion Factor Integer and Fractional values.
	// Use Base Rate Multiplier default value.
	unsigned long base_rate_mult = 0x4;
	unsigned long hold = 0x0;
	unsigned long sample_rate_int = 0xF;
	unsigned long sample_rate_frac = 0x13FF;

	DPRINTK("[DRIVER] setDacSamplingRate rate = %u\n", samplingRate);

     /*
     *  Available sampling rates:
     *  192kHz, 176.4kHz, 128kHz, 96kHz, 88.2KHz, 64kHz, 48kHz, 44.1kHz, 32kHz, 24kHz, 22.05kHz, 16kHz, 12kHz, 11.025kHz, 8kHz,
     *  and half of those 24kHz, 16kHz, (4kHz)
     *  Won't bother supporting those in ().
     */
    if (samplingRate >= 192000) samplingRate = 192000;
    else if (samplingRate >= 176400) samplingRate = 176400;
    else if (samplingRate >= 128000) samplingRate = 128000;
    else if (samplingRate >= 96000) samplingRate = 96000;
    else if (samplingRate >= 88200) samplingRate = 88200;
    else if (samplingRate >= 64000) samplingRate = 64000;
    else if (samplingRate >= 48000) samplingRate = 48000;
    else if (samplingRate >= 44100) samplingRate = 44100;
    else if (samplingRate >= 32000) samplingRate = 32000;
    else if (samplingRate >= 24000) samplingRate = 24000;
    else if (samplingRate >= 22050) samplingRate = 22050;
    else if (samplingRate >= 16000) samplingRate = 16000;
    else if (samplingRate >= 12000) samplingRate = 12000;
    else if (samplingRate >= 11025) samplingRate = 11025;
    else samplingRate = 8000;

	switch (samplingRate)
	{
		case 8000:
		case 11025:
		case 12000:
			base_rate_mult=1; hold=3;
			break;
		case 16000:
		case 22050:
		case 24000:
			base_rate_mult=1; hold=1;
			break;
		case 32000:
		case 44100:
		case 48000:
			base_rate_mult=1; hold=0;
			break;
		case 64000:
		case 88200:
		case 96000:
			base_rate_mult=2; hold=0;
			break;
		case 128000:
		case 176400:
		case 192000:
			base_rate_mult=4; hold=0;
			break;
		default: break; 
	}

	stmp_dac_value.samplingRate = samplingRate;

	switch (samplingRate)
	{
		case 8000:
		case 16000:
		case 32000:
		case 64000:
		case 128000:
		{
			if(stmp_dac_value.play_speed == PLAY_SPEED_X1_1) {
				sample_rate_int=0x15; sample_rate_frac=0x092D;
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X1_2) {
				sample_rate_int=0x13; sample_rate_frac=0x0BE4;
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X1_3) {
				sample_rate_int=0x11; sample_rate_frac=0x1577;
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X0_7) {
				sample_rate_int=0x1F; sample_rate_frac=0x0384;
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X0_8) {
				sample_rate_int=0x1C; sample_rate_frac=0x0A4C;
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X0_9) {
				sample_rate_int=0x19; sample_rate_frac=0x1B1E;
			}
			else {				
				sample_rate_int=0x17; sample_rate_frac=0x0e00;
			}
		}
			break;
		case 11025:
		case 22050:
		case 44100:
		case 88200:
		case 176400:
		{
			if(stmp_dac_value.play_speed == PLAY_SPEED_X1_1) {
				//sample_rate_int=0xF; sample_rate_frac=0x2549;
				sample_rate_int=0xF; sample_rate_frac=0x11F3;
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X1_2) {
				//sample_rate_int=0xD; sample_rate_frac=0x3E24;		
				sample_rate_int=0xE; sample_rate_frac=0x021D;
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X1_3) {
				sample_rate_int=0xC; sample_rate_frac=0x1E54;						
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X0_7) {
				sample_rate_int=0x16; sample_rate_frac=0x18C7;	
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X0_8) {
				sample_rate_int=0x14; sample_rate_frac=0x1686;	
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X0_9) {
				//sample_rate_int=0x13; sample_rate_frac=0x0131;	
				sample_rate_int=0x12; sample_rate_frac=0x1B94;	
			}
			else {
				sample_rate_int=0x11; sample_rate_frac=0x0037;
			}
		}
			break;
		case 12000:
		case 24000:
		case 48000:
		case 96000:
		case 192000:
		{
			if(stmp_dac_value.play_speed == PLAY_SPEED_X1_1) {
				sample_rate_int=0xE; sample_rate_frac=0x03F9;	
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X1_2) {
				sample_rate_int=0xC; sample_rate_frac=0x2005;	
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X1_3) {
				sample_rate_int=0xB; sample_rate_frac=0x198F;	
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X0_7) {
				sample_rate_int=0x14; sample_rate_frac=0x193F;	
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X0_8) {
				sample_rate_int=0x12; sample_rate_frac=0x1E0E;	
			}
			else if(stmp_dac_value.play_speed == PLAY_SPEED_X0_9) {
				sample_rate_int=0x11; sample_rate_frac=0x0277;	
			}
			else {
				sample_rate_int=0xf; sample_rate_frac=0x13ff;
			}
		}
			break;
	
		default: break; 
	}

	//printk("[AUDIO] stmp_dac_value.play_speed=%d, sample_rate_int=%x, sample_rate_frac=%x\n", stmp_dac_value.play_speed, sample_rate_int, sample_rate_frac);

	BF_CS4(AUDIOOUT_DACSRR, 
		SRC_HOLD, hold, 
		SRC_INT,  sample_rate_int, 
		SRC_FRAC, sample_rate_frac,
		BASEMULT, base_rate_mult);
}

void setPlayingStatus(int isPlaying)
{
	if(isPlaying)
		stmp_dac_value.isPlaying = PLAYING;
	else
		stmp_dac_value.isPlaying = NO_PLAYING;
}
	
void setPostProcessingFlag(audio_buf_t *dac_b, unsigned short setValue, int gain_attenuation)
{
	//printk("[DRIVER] setPostProcessingFlag [0x%d], gain_attenuation=%d\n", setValue, gain_attenuation);

	stmp_dac_value.gain_attenuation = gain_attenuation;
	
	if(dac_b->initDevice == 0)
	{
		if(stmp_dac_value.current_post != setValue)
		{
			setDacVolume(stmp_dac_value.volume);
		}
		stmp_dac_value.current_post = setValue;
		stmp_dac_value.selectSound = NO_SELECT;
	}
	else
	{
		stmp_dac_value.current_post = setValue;
		DPRINTK("[DRIVER] Timer init for DNSE Fade [%d]\n", stmp_dac_value.current_post);
		
		if(stmp_dac_value.selectSound == SOUND_SELECT)
		{
			DPRINTK("[DRIVER] now dnse is fading return\n");
			return;
		}

		stmp_dac_value.selectSound = SOUND_SELECT;
		init_timer(&post_volume_timer);
	
		post_volume_timer.expires=jiffies+1; // 10msec
		post_volume_timer.function=checkPostprocessing;
		post_volume_timer.data=0;
		
		/* register timer */
		add_timer(&post_volume_timer);
	}
}

void setRecordingStatus(int isRecording)
{
	if(isRecording)
		stmp_dac_value.isRecording = RECORDING;
	else
		stmp_dac_value.isRecording = NO_RECORDING;
}

void setVDDflag(int flag)
{
	if(flag == MAX_VDD)
		stmp_dac_value.maxVDD_flag = 1;
	else
		stmp_dac_value.maxVDD_flag = 0;
}

void setDefaultVGA(void)
{
	DPRINTK("setDefaultVGA\n");
	BW_AUDIOOUT_REFCTRL_VBG_ADJ(0x3);		
	BW_AUDIOOUT_REFCTRL_RAISE_REF(1);	
	BW_AUDIOOUT_REFCTRL_ADJ_VAG(1);
	BW_AUDIOOUT_REFCTRL_VAG_VAL(DEF_VGA_VAL);
	/* By leeth, at power on, some set can't be contolled VDDD. it is fixed at 20090530 */
#if 0
	HW_POWER_VDDDCTRL_SET(0x1 << 21); //switchs source of vddd from vdda to battery, dhsong 
	HW_POWER_VDDDCTRL_SET(0x1 << 22); //enable the vddd linear regulator, dhsong
#endif
}

void setMaxVGA(void)
{
	DPRINTK("setMaxVGA\n");
	BW_AUDIOOUT_REFCTRL_VBG_ADJ(0x3);		
	BW_AUDIOOUT_REFCTRL_RAISE_REF(1);	
	BW_AUDIOOUT_REFCTRL_ADJ_VAG(1);
	BW_AUDIOOUT_REFCTRL_VAG_VAL(MAX_VGA_VAL);
	/* By leeth, at power on, some set can't be contolled VDDD. it is fixed at 20090530 */
#if 0
	HW_POWER_VDDDCTRL_SET(0x1 << 21); //switchs source of vddd from vdda to battery, dhsong 
	HW_POWER_VDDDCTRL_SET(0x1 << 22); //enable the vddd linear regulator, dhsong
#endif
}

#ifdef USE_MAX_CORE_LEVEL
#if 1 //for 37xx
void setCoreLevel(int volume)
{
	int isPlaying = 0;
	isPlaying = getPlayingStatus();

	if( (volume == MAX_USER_VOLUME) && (isPlaying == PLAYING) )
	{
		DPRINTK("===> Set to Max VDDA 2100V\n");
		setMaxVGA();		
		//hw_power_SetVddaValue(2100);   
		ddi_power_SetVdda(2100, 1925); //vdda, vdda_bo, dhsong
	}
	else
	{
		DPRINTK("===> Set to Nor VDDA 1750V\n");
		setDefaultVGA();
		//hw_power_SetVddaValue(1750);   
		ddi_power_SetVdda(1750, 1575); //vdda, vdda_bo, dhsong
	}
}
#else
void setClkIntrWait(int onoff)
{
	if(onoff == ENABLE_CLK_INTRWAIT)
	{
		/* restore clk setting */
		HW_CLKCTRL_CPUCLKCTRL_WR(	 BF_CLKCTRL_CPUCLKCTRL_INTERRUPT_WAIT(1));
	}
	else
	{
		/* disable hbus auto slow mode and disable cpu interrupt wait */
		HW_CLKCTRL_CPUCLKCTRL_WR(	 BF_CLKCTRL_CPUCLKCTRL_INTERRUPT_WAIT(0));
	}
	while ( BF_RD(CLKCTRL_CPUCLKCTRL, BUSY ) ) ; 
}
void setCoreLevel(int volume)
{
	int isPlaying = 0;
	isPlaying = getPlayingStatus();

/*	remove 0404
	int isRecording = 0;
	isRecording = getRecordingStatus();
	
	if( (volume == MAX_USER_VOLUME) && (isPlaying == PLAYING) && (isRecording == NO_RECORDING) )
*/
	if( (volume == MAX_USER_VOLUME) && (isPlaying == PLAYING) )
	{
		if(HW_AUDIOOUT_REFCTRL_RD() != 0x4010d0)
			setMaxVGA();
		/* disable cpuclk intr wait when max volume */
		//setClkIntrWait(DISABLE_CLK_INTRWAIT);

		DPRINTK("[DRIVER] REFCTRL[%x] -- #Max volume Value# --\n",HW_AUDIOOUT_REFCTRL_RD());
		if(HW_POWER_VDDCTRL.VDDD_TRG != MAX_VOLUME_CORE)
		{
			setVDDflag(NO_MAX_VDD);
			set_core_level(MAX_VOLUME_CORE);	
		}
		setVDDflag(MAX_VDD);
	}
	else
	{
		setDefaultVGA();
		/* restore cpuclk intr wait when normal volume */
		//setClkIntrWait(ENABLE_CLK_INTRWAIT);

		DPRINTK("[DRIVER] REFCTRL[%x] -- #Default Value# --\n",HW_AUDIOOUT_REFCTRL_RD());
		setVDDflag(NO_MAX_VDD);
		if(HW_POWER_VDDCTRL.VDDD_TRG == MAX_VOLUME_CORE)
		{
			set_core_level(NOR_VOLUME_CORE);
		}
	}
}
#endif
#endif

void setVolumeFade(int fade)
{
	stmp_dac_value.volume_fade = fade;

	//printk("===> setVolumeFade : fade=%d\n", fade);

	if(fade == FADE_IN)
	{
		#ifdef HP_VOLUME_CONTROL
		BF_CS2(AUDIOOUT_HPVOL, 
			VOL_LEFT,  HP_VOLUME_MIN, 
			VOL_RIGHT, HP_VOLUME_MIN);
		#else		
		BF_CS2(AUDIOOUT_DACVOLUME, 
		   VOLUME_LEFT,  DAC_VOLUME_MIN, 
		   VOLUME_RIGHT, DAC_VOLUME_MIN);
		#endif
	
		//printk("[DRIVER] Timer init for VOL Fade IN\n");
		if(vol_fadein_ing == 1)
		{
			DPRINTK("[DRIVER] now volume is fading in.. return\n");
			return;
		}
		if(vol_fadeout_ing == 1)
		{
			DPRINTK("[DRIVER] now volume is fading out.. -> del timer and fade in start\n");
			del_timer(&volume_fade_timer);
			vol_fadeout_ing = 0;
		}

		init_timer(&volume_fade_timer);
		
		volume_fade_timer.expires=jiffies+1; // 10msec
		volume_fade_timer.function=checkVolumeFade;
		volume_fade_timer.data=0;
		vol_fadein_ing = 1;
		/* register timer */
		add_timer(&volume_fade_timer);
	}
	else if(fade == FADE_OUT)
	{
		//printk("[DRIVER] Timer init for VOL Fade OUT\n");

		if(vol_fadeout_ing == 1)
		{
			DPRINTK("[DRIVER] now volume is fading out.. return\n");
			return;
		}
		if(vol_fadein_ing == 1)
		{
			DPRINTK("[DRIVER] now volume is fading in.. -> del timer and fade out start\n");
			del_timer(&volume_fade_timer);
			vol_fadein_ing = 0;
		}

		init_timer(&volume_fade_timer);
		
		volume_fade_timer.expires=jiffies+1; // 10msec
		volume_fade_timer.function=checkVolumeFade;
		volume_fade_timer.data=0;
		vol_fadeout_ing = 1;
		/* register timer */
		add_timer(&volume_fade_timer);
	}
	else
	{
		if(vol_fadeout_ing == 1)
			vol_fadeout_ing = 0;
		if(vol_fadeout_ing == 1)
			vol_fadeout_ing = 0;
		
		//printk("[DRIVER] End of Volume Fade\n");
		del_timer(&volume_fade_timer);
	}
}

int userVol2DacVol(int user_volume)
{
	int real_value = 0;

	if(stmp_dac_value.sw_ver == VER_COMMON)
	{
		//printk("NO_PROCESSING VER_COMMON %d\n",user_volume);
		if(user_volume==30)
			real_value = (DAC_CM_VOL_TBL[user_volume]<<1)+1;			
		else
			real_value = DAC_CM_VOL_TBL[user_volume]<<1;
	}
	else if(stmp_dac_value.sw_ver == VER_FR)
	{
		//printk("NO_PROCESSING VER_FR %d\n",user_volume);
		real_value = DAC_FR_VOL_TBL[user_volume]<<1;			
	}

	if(stmp_dac_value.current_post == DNSE_PROCESSING)
	{
		if(stmp_dac_value.gain_attenuation == 6)
		{
			if(stmp_dac_value.sw_ver == VER_COMMON)
			{
				//printk("6-DNSE_PROCESSING COMMON %d\n",user_volume);
				if(user_volume == 30) real_value = (99<<1)+1; 
				else if(user_volume == 29) real_value = (99<<1)+1; 
				else if(user_volume == 28) real_value = 99<<1; 
				else if(user_volume == 27) real_value = (98<<1)+1; 
				else if(user_volume == 26) real_value = 98<<1; 
				else if(user_volume >= 1) real_value += 6<<1; 
				else real_value = 2;
			}
			else if(stmp_dac_value.sw_ver == VER_FR)
			{
				//printk("6-DNSE_PROCESSING FR %d\n",user_volume);
				if(user_volume >= 1) real_value += 6<<1;
				else real_value = 2;
			}
		}
		else if(stmp_dac_value.gain_attenuation == 9)
		{
			if(stmp_dac_value.sw_ver == VER_COMMON)
			{
				//printk("9-DNSE_PROCESSING COMMON %d\n",user_volume);
				if(user_volume == 30) real_value = (99<<1)+1; 
				else if(user_volume == 29) real_value = (99<<1)+1; 
				else if(user_volume == 28) real_value = 99<<1;
				else if(user_volume == 27) real_value = (98<<1)+1; 
				else if(user_volume == 26) real_value = 98<<1;
				else if(user_volume == 25) real_value = (97<<1)+1; 
				else if(user_volume == 24) real_value = (97<<1);
				else if(user_volume == 23) real_value = (96<<1)+1;
				else if(user_volume == 22) real_value = (96<<1);
				else if(user_volume == 21) real_value = (95<<1)+1;
				else if(user_volume >= 1) real_value += 9<<1; 
				else real_value = 2;
			}
			else if(stmp_dac_value.sw_ver == VER_FR)
			{
				//printk("9-DNSE_PROCESSING FR %d\n",user_volume);
				if(user_volume == 30) real_value = (99<<1)+1; 
				else if(user_volume == 29) real_value = 99<<1;
				else if(user_volume >= 1) real_value += 9<<1;
				else real_value = 2;
			}
		}
		else if(stmp_dac_value.gain_attenuation == 12)
		{
			if(stmp_dac_value.sw_ver == VER_COMMON)
			{
				//printk("12-DNSE_PROCESSING COMMON %d\n",user_volume);
				if(user_volume == 30) 
				{
					setHPVolume(0x06); // +3.0dB
					real_value = (99<<1)+1;
				}
				else if(user_volume == 29)
				{
					setHPVolume(0x06); // +3.0dB
					real_value = (99<<1)+1;
				}
				else if(user_volume == 28)
				{
					setHPVolume(0x07); // +2.5dB
					real_value = (99<<1)+1;
				}
				else if(user_volume == 27)
				{
					setHPVolume(0x08); // +2.0dB
					real_value = (99<<1)+1;
				}
				else if(user_volume == 26)
				{
					setHPVolume(0x09); // +1.5dB
					real_value = (99<<1)+1;
				}
				else if(user_volume == 25)
				{
					setHPVolume(0x0A); // +1.0dB
					real_value = (99<<1)+1;
				}
				else if(user_volume == 24)
				{
					setHPVolume(0x0B); // +0.5dB
					real_value = (99<<1)+1;
				}
				else if(user_volume == 23) real_value = (99<<1)+1; 
				else if(user_volume == 22) real_value = 99<<1;
				else if(user_volume == 21) real_value = (98<<1)+1;
				else if(user_volume >= 1) real_value += 12<<1;
				else real_value = 2;
			}
			else if(stmp_dac_value.sw_ver == VER_FR)
			{
				//printk("12-DNSE_PROCESSING FR %d\n",user_volume);
				if(user_volume == 30) real_value = (90<<1)+1;
				else if(user_volume == 29) real_value = 90<<1;
				else if(user_volume == 28) real_value = (89<<1)+1;
				else if(user_volume == 27) real_value = 89<<1;
				else if(user_volume == 26) real_value = (88<<1)+1;
				else if(user_volume == 25) real_value = 88<<1;
				else if(user_volume == 24) real_value = (87<<1)+1;
				else if(user_volume == 23) real_value = 87<<1;
				else if(user_volume == 22) real_value = (86<<1)+1;
				else if(user_volume >= 1) real_value += 10<<1;
				else real_value = 2;
			}
		}
	}
	return real_value;
}

int updateDacBytes(void *audio_s)
{
	audio_stream_t *audioData = (audio_stream_t *)audio_s;

	audio_buf_t *dac_b = audioData->audio_buffers;
	decoder_buf_t *decoder_b = audioData->decoder_buffers;

	unsigned int currentPosition=0, lastPosition=0;
	unsigned int consumeBytes1=0, consumeBytes2=0;
 	unsigned int buffer_size = 0;

 	buffer_size = getBufSize();

	dac_b->IsFillDac = 1;
	lastPosition = dac_b->lastDMAPosition;

	currentPosition = getDacPointer(dac_b->phyStartaddr); //Since in theory, the pointer is always moving, lets just grab a snapshot of it.
	//printk("[DRIVER] DacPtr[%d]\n", currentPosition);

	if((currentPosition == lastPosition) && (currentPosition != 0))
	{
		printk(" the DAC DMA has stopped !! \n");
		return -1;
	}

	if(currentPosition >= lastPosition)
	{
		 consumeBytes1 = currentPosition - lastPosition;
		 consumeBytes2 = 0;
	}
	else
	{
		consumeBytes1 = buffer_size - lastPosition;
		consumeBytes2 = currentPosition;
	}
	
	memset((void *)dac_b->pStart+dac_b->lastDMAPosition, 0, consumeBytes1+consumeBytes2);
	subDecoderValidBytes(decoder_b, consumeBytes1+consumeBytes2);
	//printk(KERN_DEBUG"[ISR] CP[%d] LP[%d] SB[%d] DB[%d]\n", currentPosition, dac_b->lastDMAPosition, consumeBytes1+consumeBytes2, decoder_b->decoderValidBytes);

	dac_b->lastDMAPosition=currentPosition;
	return 0;
}


void set_pcm_audio_bit(int bit)
{
	if (bit == 8)
		DPRINTK("8 bit is not support\n");
	else if (bit == 16)
		HW_AUDIOOUT_CTRL.B.WORD_LENGTH = 1;
	else if (bit == 32)
		HW_AUDIOOUT_CTRL.B.WORD_LENGTH = 0;
}

