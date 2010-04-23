// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#if (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#include <e32debug.h>
#endif

// #define _USBMS_DEBUG_PRINT_ 
// #define _MSDEVICE_DEBUG_PRINT_

#if defined(_USBMS_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
/** Trace - format string  */
#define __PRINT(t) {RDebug::Print(t);}
/** Trace - format string with 1 param */
#define __PRINT1(t,a) {RDebug::Print(t,a);}
/** Trace - format string with 2 params */
#define __PRINT2(t,a,b) {RDebug::Print(t,a,b);}
/** Trace - format string with 3 params */
#define __PRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
/** Trace - format string with 4 params */
#define __PRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
/** Trace - format string with 5 params */
#define __PRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}


_LIT(KMsgIn, ">>%S\n");
_LIT(KMsgOut,"<<%S\n");

class TMSLogFn
{
	protected:
	TBuf<100> iName;

	public:
	TMSLogFn(const TDesC& aName){iName = aName; RDebug::Print(KMsgIn, &iName);};
	~TMSLogFn(){RDebug::Print(KMsgOut, &iName);};
};

#define __FNLOG(name) TMSLogFn __fn_log__(_L(name))

#else

/** NULL definition */
#define __PRINT(t)
/** NULL definition */
#define __PRINT1(t,a)
/** NULL definition */
#define __PRINT2(t,a,b)
/** NULL definition */
#define __PRINT3(t,a,b,c)
/** NULL definition */
#define __PRINT4(t,a,b,c,d)
/** NULL definition */
#define __PRINT5(t,a,b,c,d,e)
/** NULL definition */
#define __FNLOG(name)
#endif


#if defined (_MSDEVICE_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define __MSDEVPRINT(t) {RDebug::Print(t);}
#define __MSDEVPRINT1(t,a) {RDebug::Print(t,a);}
#define __MSDEVPRINT2(t,a,b) {RDebug::Print(t,a,b);}
#else
#define __MSDEVPRINT(t)
#define __MSDEVPRINT1(t,a)
#define __MSDEVPRINT2(t,a,b)
#endif // _MSDEVICE_DEBUG_PRINT_

#endif // DEBUG_H
