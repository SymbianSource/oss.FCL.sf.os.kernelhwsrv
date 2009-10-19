/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* @file tmslog.h
* 
*
*/






#ifndef TMSLOG_H
#define TMSLOG_H

//#define _TESTAPP_DEBUG_PRINT_

#if (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#include <e32debug.h>
#endif

#if defined(_TESTAPP_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define TRACE_ENABLED
#endif

#ifdef TRACE_ENABLED

class TMsLogStaticFn
    {
public:
    static const TInt KLogBufferSize = 0xFF;

    TMsLogStaticFn(const TDesC8& aFunctionName);

	~TMsLogStaticFn();

protected:
    TBuf8<KLogBufferSize> buf;
	TPtrC8 iFunctionName;
    };




class TMsLogFn
    {
public:
    static const TInt KLogBufferSize = 0xFF;
	TMsLogFn(const TDesC8& aFunctionName, void* aThisPointer);

	~TMsLogFn();

private:
    TBuf8<KLogBufferSize> buf;
	TPtrC8 iFunctionName;
	void* iThisPointer;
    };




#define __MSFNLOG TMsLogFn funcLog(TPtrC8((TUint8*)__PRETTY_FUNCTION__), this);
#define __MSFNSLOG TMsLogStaticFn funcLog(TPtrC8((TUint8*)__PRETTY_FUNCTION__));
#else
#define __MSFNSLOG
#define __MSFNLOG
#endif



#ifdef TRACE_ENABLED
#define __PRINT(t) {RDebug::Print(t);}
#define __PRINT1(t,a) {RDebug::Print(t,a);}
#define __PRINT2(t,a,b) {RDebug::Print(t,a,b);}
#define __PRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
#else
#define __PRINT(t)
#define __PRINT1(t,a)
#define __PRINT2(t,a,b)
#define __PRINT3(t,a,b,c)

#endif


#endif // TMSLOG_H
