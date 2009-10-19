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
// e32\e32test\demandpaging\d_pagingexample.h
// Examples for demand paging device driver migration
// This header defines the interface to four drivers:
// d_pagingexample_1_pre		DLogicalChannel-dervied driver, pre-migration
// d_pagingexample_1_post		DLogicalChannel-dervied driver, post-migration
// d_pagingexample_2_pre		DLogicalChannelBase-dervied driver, pre-migration
// d_pagingexample_2_post		DLogicalChannelBase-dervied driver, post-migration
// 
//

#ifndef __D_PAGINGEXAMPLE_H__
#define __D_PAGINGEXAMPLE_H__

#include <e32cmn.h>

#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KPagingExample1PreLdd,"D_PAGINGEXAMPLE_1_PRE");
_LIT(KPagingExample1PostLdd,"D_PAGINGEXAMPLE_1_POST");
_LIT(KPagingExample2PreLdd,"D_PAGINGEXAMPLE_2_PRE");
_LIT(KPagingExample2PostLdd,"D_PAGINGEXAMPLE_2_POST");

const TInt KMaxTransferSize = 256;
const TInt KAsyncDelay = 100;   // simulated async operations take 100ms

// Common interface to all versions of the example driver
class RPagingExample : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ESetRealtimeState,
		EGetConfig,
		ESetConfig,
		};
	
	enum TRequest
		{
		ERequestRead,
		ERequestReadDes,
		ERequestWrite,
		ERequestWriteDes,
		ERequestNotify,
		ERequestAsyncGetValue,
		ERequestAsyncGetValue2,
		ERequestCancel,
		};

	struct TConfigData
		{
		TInt iParam1;
		TInt iParam2;
		TInt iParam3;
		TInt iParam4;
		};

	struct TValueStruct
		{
		TInt     iValue1;
		TBuf8<4> iValue2;
		};
	
#ifndef __KERNEL_MODE__
public:
	inline TInt Open(const TDesC& aLddName)
		{ return DoCreate(aLddName, TVersion(), KNullUnit, NULL, NULL, EOwnerThread, EFalse); }
	inline TInt SetDfcThreadRealtimeState(TBool aRealtime)  // only supported on "1_pre" driver
		{ return DoControl(ESetRealtimeState, (TAny*)aRealtime); }
	inline TInt GetConfig(TConfigData& aConfigOut)
		{ return DoControl(EGetConfig, (TAny*)&aConfigOut); }
	inline TInt SetConfig(const TConfigData& aConfigIn)
		{ return DoControl(ESetConfig, (TAny*)&aConfigIn); }
	inline void Notify(TRequestStatus& aStatus)
		{ DoRequest(ERequestNotify, aStatus); }
	inline void AsyncGetValue(TRequestStatus& aStatus, TValueStruct& aValueOut)
		{ DoRequest(ERequestAsyncGetValue, aStatus, (TAny*)&aValueOut); }
	inline void AsyncGetValue2(TRequestStatus& aStatus, TInt& aValueOut1, TInt& aValueOut2)
		{ DoRequest(ERequestAsyncGetValue2, aStatus, (TAny*)&aValueOut1, (TAny*)&aValueOut2); }
	inline void Read(TRequestStatus& aStatus, TUint8* aBuffer, TInt aLength)
		{ DoRequest(ERequestRead, aStatus, aBuffer, (TAny*)aLength); }
	inline void ReadDes(TRequestStatus& aStatus, TDes8& aDesOut)
		{ DoRequest(ERequestReadDes, aStatus, &aDesOut); }
	inline void Write(TRequestStatus& aStatus, const TUint8* aBuffer, TInt aLength)
		{ DoRequest(ERequestWrite, aStatus, (TAny*)aBuffer, (TAny*)aLength); }
	inline void WriteDes(TRequestStatus& aStatus, const TDesC8& aDesIn)
		{ DoRequest(ERequestWriteDes, aStatus, (TAny*)&aDesIn); }
	inline void Cancel()
		{ DoCancel(ERequestCancel); }
#endif
	};

#endif
