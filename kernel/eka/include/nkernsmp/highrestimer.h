// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32/include/nkernsmp/highrestimer.h
// The highrestimer.h header file defines how to access the high resoltion
// timer, if one is supported.  This file is used by default if the variant does
// not export one to \epoc32\include\nkern. 
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __HIGHRESTIMER_H__
#define __HIGHRESTIMER_H__

/**
 * Macro indicating that a high resolution timer is supported.
 */
//#define HAS_HIGH_RES_TIMER

/**
 * Assembler macro to get the the current value of the high res timer and place
 * it in the specified register.
 */
//#define GET_HIGH_RES_TICK_COUNT(Rd) ...

/**
 * The frequency of the timer in Hz.
 */
//const TInt KHighResTimerFrequency = ...

/**
 * Macro indicating that the timer counts up if defined.
 */
//#define HIGH_RES_TIMER_COUNTS_UP

#endif
