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
//

/**
 @file
 @internalTechnology
*/

#include "debug.h"

#ifdef FUNCTION_TRACE_ENABLED

_LIT8(KFnMsgSIn, ">>\t%S\r\n");
_LIT8(KFnMsgSOut,"<<\t%S\r\n");
_LIT8(KFnMsgIn, ">>[0x%08x]\t%S\r\n");
_LIT8(KFnMsgOut,"<<[0x%08x]\t%S\r\n");

TLogStaticFn::TLogStaticFn(const TDesC8& aFunctionName)
    {
    iFunctionName.Set(aFunctionName);
    buf.AppendFormat(KFnMsgSIn, &iFunctionName);
    RDebug::RawPrint(buf);
    };


TLogStaticFn::~TLogStaticFn()
    {
    buf.Zero();
    buf.AppendFormat(KFnMsgSOut, &iFunctionName);
    RDebug::RawPrint(buf);
    };


TLogFn::TLogFn(const TDesC8& aFunctionName, void* aThisPointer)
:   iThisPointer(aThisPointer)
    {
    iFunctionName.Set(aFunctionName);
    buf.AppendFormat(KFnMsgIn, iThisPointer, &iFunctionName);
    RDebug::RawPrint(buf);
    };


TLogFn::~TLogFn()
    {
    buf.Zero();
    buf.AppendFormat(KFnMsgOut, iThisPointer, &iFunctionName);
    RDebug::RawPrint(buf);
    };

#endif
