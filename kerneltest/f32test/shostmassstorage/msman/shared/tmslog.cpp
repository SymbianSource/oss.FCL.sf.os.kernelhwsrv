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

#include "tmslog.h"

#ifdef TRACE_ENABLED

_LIT8(KMsgSIn, ">>\t%S\r\n");
_LIT8(KMsgSOut,"<<\t%S\r\n");
_LIT8(KMsgIn, ">>[0x%08x]\t%S\r\n");
_LIT8(KMsgOut,"<<[0x%08x]\t%S\r\n");


TMsLogStaticFn::TMsLogStaticFn(const TDesC8& aFunctionName)
    {
    iFunctionName.Set(aFunctionName);
    buf.AppendFormat(KMsgSIn, &iFunctionName);
    RDebug::RawPrint(buf);
    };


TMsLogStaticFn::~TMsLogStaticFn()
    {
    buf.Zero();
    buf.AppendFormat(KMsgSOut, &iFunctionName);
    RDebug::RawPrint(buf);
    };


TMsLogFn::TMsLogFn(const TDesC8& aFunctionName, void* aThisPointer)
:   iThisPointer(aThisPointer)
    {
    iFunctionName.Set(aFunctionName);
    buf.AppendFormat(KMsgIn, iThisPointer, &iFunctionName);
    RDebug::RawPrint(buf);
    };


TMsLogFn::~TMsLogFn()
    {
    buf.Zero();
    buf.AppendFormat(KMsgOut, iThisPointer, &iFunctionName);
    RDebug::RawPrint(buf);
    };

#endif
