// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\resmanus\d_resmanusbtraceconst.h
// 
//

#ifndef D_RESMANUSBTRACECONST_H_
#define D_RESMANUSBTRACECONST_H_


const TUint KClientId = 0x12345678;
const TInt KClientHandle = 0x10;
const TUint8 KStatsRes1 = 0x20;
const TUint8 KStatsRes2 = 0x21;
const TUint8 KStatsRes3 = 0x22;
const TUint KResourceId = 0x80;
const TUint KLevel = 0x90;
const TUint KClient = 0xC800FFFF;
const TUint KResult = 0x100;
const TInt KMinLevel = -99;
const TInt KMaxLevel = 99;
const TInt KDefaultLevel = 50;
const TInt KResCount = 20;
const TInt KSize = 999;
const TInt KRetVal = KErrNoMemory;
const TUint KFlags = 0xAAAAAAAA;

_LIT8(KRESOURCENAME, "ResourceName");
_LIT8(KCLIENTNAME, "ClientName");

struct TLogInfo
    {
    TAny* iPR;
    TAny* iPC;
    TAny* iPN;
    TAny* iPCb;
    TAny* iPClient;
    TAny* iPCallback;
    };



#endif /* D_RESMANUSBTRACECONST_H_ */
