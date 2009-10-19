// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// msdebug.cpp
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef MSDEBUG_H
#define MSDEBUG_H

// #define _MSFN_DEBUG_PRINT_

#if defined(_MSFN_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define MSFN_TRACE_ENABLED
#endif

#ifdef MSFN_TRACE_ENABLED
#include <e32debug.h>


/**
Logging function to be used with static functions. Prints the function string on
function entry and exit.
*/
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


/**
Logging function which prints the function string on function entry and exit.
*/
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

#endif // MSDEBUG_H


