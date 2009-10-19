// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//



/**
 @file
 @internalTechnology
*/


#ifndef DEBUG_H
#define DEBUG_H

//#define _USBOTG_DEBUG_PRINT_
//#define _USBHOST_DEBUG_PRINT_

#if (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#include <e32debug.h>
#endif

#if defined(_USBOTG_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
/** Trace - format string  */
#define __USBOTGPRINT(t) {RDebug::Print(t);}
/** Trace - format string with 1 param */
#define __USBOTGPRINT1(t,a) {RDebug::Print(t,a);}
/** Trace - format string with 2 params */
#define __USBOTGPRINT2(t,a,b) {RDebug::Print(t,a,b);}
/** Trace - format string with 3 params */
#define __USBOTGPRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
/** Trace - format string with 4 params */
#define __USBOTGPRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
/** Trace - format string with 5 params */
#define __USBOTGPRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#else
/** NULL definition */
#define __USBOTGPRINT(t)
/** NULL definition */
#define __USBOTGPRINT1(t,a)
/** NULL definition */
#define __USBOTGPRINT2(t,a,b)
/** NULL definition */
#define __USBOTGPRINT3(t,a,b,c)
/** NULL definition */
#define __USBOTGPRINT4(t,a,b,c,d)
/** NULL definition */
#define __USBOTGPRINT5(t,a,b,c,d,e)
#endif


#if defined(_USBHOST_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
/** Trace - format string  */
#define __USBHOSTPRINT(t) {RDebug::Print(t);}
/** Trace - format string with 1 param */
#define __USBHOSTPRINT1(t,a) {RDebug::Print(t,a);}
/** Trace - format string with 2 params */
#define __USBHOSTPRINT2(t,a,b) {RDebug::Print(t,a,b);}
/** Trace - format string with 3 params */
#define __USBHOSTPRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
/** Trace - format string with 4 params */
#define __USBHOSTPRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
/** Trace - format string with 5 params */
#define __USBHOSTPRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#else
/** NULL definition */
#define __USBHOSTPRINT(t)
/** NULL definition */
#define __USBHOSTPRINT1(t,a)
/** NULL definition */
#define __USBHOSTPRINT2(t,a,b)
/** NULL definition */
#define __USBHOSTPRINT3(t,a,b,c)
/** NULL definition */
#define __USBHOSTPRINT4(t,a,b,c,d)
/** NULL definition */
#define __USBHOSTPRINT5(t,a,b,c,d,e)
#endif




#endif // DEBUG_H
