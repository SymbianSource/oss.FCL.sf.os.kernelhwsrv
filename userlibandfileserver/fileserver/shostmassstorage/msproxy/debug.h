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
//

/**
 @file
 @internalTechnology
*/

#ifndef PXY_DEBUG_H
#define PXY_DEBUG_H

// #define _HOST_DEBUG_PRINT_
// #define _PROXY_DEBUG_PRINT_
// #define _PROXY_FN_TRACE_

#if defined(_PROXY_FN_TRACE_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define FUNCTION_TRACE_ENABLED
#endif

#if (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#include <e32debug.h>
#endif

#ifdef FUNCTION_TRACE_ENABLED
/**
Logging function to be used with static functions. Prints the function string on
function entry and exit.
*/
class TLogStaticFn
    {
public:
    static const TInt KLogBufferSize = 0xFF;

    TLogStaticFn(const TDesC8& aFunctionName);

	~TLogStaticFn();

protected:
    TBuf8<KLogBufferSize> buf;
	TPtrC8 iFunctionName;
    };


/**
Logging function which prints the function string on function entry and exit.
*/
class TLogFn
    {
public:
    static const TInt KLogBufferSize = 0xFF;
	TLogFn(const TDesC8& aFunctionName, void* aThisPointer);

	~TLogFn();

private:
    TBuf8<KLogBufferSize> buf;
	TPtrC8 iFunctionName;
	void* iThisPointer;
    };

#define __MSFNLOG TLogFn funcLog(TPtrC8((TUint8*)__PRETTY_FUNCTION__), this);
#define __MSFNSLOG TLogStaticFn funcLog(TPtrC8((TUint8*)__PRETTY_FUNCTION__));
#else	// FUNCTION_TRACE_ENABLED
#define __MSFNSLOG
#define __MSFNLOG
#endif		// FUNCTION_TRACE_ENABLED


#if defined (_PROXY_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define __PXYPRINT(t) {RDebug::Print(t);}
#define __PXYPRINT1(t,a) {RDebug::Print(t,a);}
#define __PXYPRINT2(t,a,b) {RDebug::Print(t,a,b);}
#define __PXYPRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
#define __PXYPRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
#define __PXYPRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#define __PXYPRINT8BIT1(t,a) {TFileName temp;temp.Copy(a);RDebug::Print(t,&temp);}
#define __PXYPRINT1TEMP(t,a) {TBuf<KMaxFileName>temp(a);RDebug::Print(t,&temp);}
#else
#define __PXYPRINT(t)
#define __PXYPRINT1(t,a)
#define __PXYPRINT2(t,a,b)
#define __PXYPRINT3(t,a,b,c)
#define __PXYPRINT4(t,a,b,c,d)
#define __PXYPRINT5(t,a,b,c,d,e)
#define __PXYPRINT8BIT1(t,a)
#define __PXYPRINT1TEMP(t,a)
#endif // _PROXY_DEBUG_PRINT_

#if defined(_HOST_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define __HOSTPRINT(t) {RDebug::Print(t);}
#define __HOSTPRINT1(t,a) {RDebug::Print(t,a);}
#define __HOSTPRINT2(t,a,b) {RDebug::Print(t,a,b);}
#define __HOSTPRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
#define __HOSTPRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
#define __HOSTPRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#define __HOSTPRINT8BIT1(t,a) {TFileName temp;temp.Copy(a);RDebug::Print(t,&temp);}
#define __HOSTPRINT1TEMP(t,a) {TBuf<KMaxFileName>temp(a);RDebug::Print(t,&temp);}
static const TUint KBlockSize = 0x200;
#else
#define __HOSTPRINT(t)
#define __HOSTPRINT1(t,a)
#define __HOSTPRINT2(t,a,b)
#define __HOSTPRINT3(t,a,b,c)
#define __HOSTPRINT4(t,a,b,c,d)
#define __HOSTPRINT5(t,a,b,c,d,e)
#define __HOSTPRINT8BIT1(t,a)
#define __HOSTPRINT1TEMP(t,a)
#endif // _HOST_DEBUG_PRINT_

#endif // PXY_DEBUG_H


