/**
 * Version: $Id: mep_config.h,v 1.10 2004/06/23 00:51:13 rhodgson Exp $
 *
 * Copyright (c) 1993, 1994 Synaptics, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 **/

/*****************************************************************
 * This is a MEP configuration file used to configure the sample
 * Atmel AVR port of the MepLib.
 *
 * Copyright 2003 Synaptics Incorporated.  All Rights Reserved.
 *
 * The information contained in this file is subject to change.
 *
 */

#ifndef MEP_CONFIG_H
#define MEP_CONFIG_H

/*****************************************************************
 * Select the basic MepLib implementation configuration.  There
 * are three choices:
 *
 * 1. Interrupt-driven
 * 2. Polled
 * 3. Hybrid (a combined Interrupt & Polled approach)
 *
 * See the porting guide for a discussion on choosing a MepLib
 * configuration.
 * 
 * Define the MEP implementation by uncommenting ONE (and only
 * one!) of the following modes:
 */
//#define MEP_CONFIG_INTERRUPT_MODE
#define MEP_CONFIG_POLLED_MODE
/*#define MEP_CONFIG_HYBRID_MODE*/



/*****************************************************************
 * These definitions are used to control timeouts on CLK events
 * inside the mep state machine.  Leaving them undefined in this
 * file means that the timeouts will be disabled in the MEP
 * machine.  See the Porting Guide for more info.
 *
 * The contents of the MEP_WAIT_FOR_CLK macro can be replaced
 * with anything function call that performs some sort of
 * timeout.  The function must return MEP_NOERR or MEP_TIMEOUT.
 *
 * For this simple implementation, the AVR timeouts are specified
 * in terms of loop iterations of the loop watching for the
 * desired CLK edge.  Measurements on the AVR system indicate
 * that CLK normally responds in about 8 iterations.  The value
 * of 1000 is used to be positive that CLK is not responding.
 */

#if 1
#  define MEP_CLK_TIMEOUT_ITERS    1000
   /* Define a prototype for our CLK watcher replacement */
#  define MEP_WAIT_FOR_CLK_PROTO \
          mep_err_t mep_waitForCLK(unsigned char desiredClkLevel, unsigned int timeoutIterations)
#  define MEP_WAIT_FOR_CLK(CLK_desiredLevel) \
          mep_waitForCLK((CLK_desiredLevel), (MEP_CLK_TIMEOUT_ITERS))
#endif



/*****************************************************************
 * If MEP_DEBUG is #define'd, it configures MepLib to display
 * debug ouput as it changes states, and sends/receives packets.
 * The output will be displayed using a host-supplied routine
 * host_putc().
 *
 * See mepLib.h or the "MepLib Porting Guide" for more info.
 */
#undef MEP_DEBUG


/*****************************************************************
 * This def defines whether the AVR interrupt handler is
 * interruptable by non-MEP interrupts or not.  In the
 * interruptable mode, the ISR code in <mep_hostHw.c>
 * demonstrates how the MEP interrupt flags need to be
 * manipulated.
 *
 * This def does not affect MepLib, but since it affects the
 * specific AVR MEP configuration, it is included here to keep
 * all the configuration information together.
 */
#define MEP_INTERRUPTABLE_ISR


/*****************************************************************
 * This optional define is used to calculate perform packet
 * transfer performance.  It is not required for general MepLib
 * operation.
 */
//#define MEP_T0_EVENT   do {markTimeZero();} while (0)
//#undef MEP_TO_EVENT
#ifdef MEP_T0_EVENT
  void markTimeZero(void);
#endif

#endif /* MEP_CONFIG_H */

