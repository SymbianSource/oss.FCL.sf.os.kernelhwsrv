/*
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* e32test\personality\example\ifcldd.h
* Test code for example RTOS personality.
* 
*
*/



#if !defined(__IFCLDD_H__)
#define __IFCLDD_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KRtosIfcLddName,"RtosIfc");

class TCapsRtosIfcV01
	{
public:
	TVersion	iVersion;
	};

struct SRxData
	{
	TInt iLength;
	TUint8 iChecksum;
	TUint8 iData[1];
	};

struct SReport
	{
	enum TType {ESem=8, ERcv=9, ETm=10};
	TInt iType;
	TUint iCount;
	TUint iOkCount;
	TUint iBadCount;
	};

class RRtosIfc : public RBusLogicalChannel
	{
public:
	enum TRequest
		{
		ERequestWaitInitialTests,
		ERequestReceive,
		ERequestReport,
		};

	enum TRequestCancel
		{
		ECancelWaitInitialTests=1,
		ECancelReceive=2,
		ECancelReport=4,
		};

	enum TControl
		{
		EControlInit,
		EControlSend,
		EControlFlush,
		EControlFinish,
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KRtosIfcLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }
	inline void Init()
		{ DoControl(EControlInit); }
	inline void WaitInitialTests(TRequestStatus& aStatus)
		{ DoRequest(ERequestWaitInitialTests, aStatus); }
	inline void Receive(TRequestStatus& aStatus, SRxData& aData)
		{ DoRequest(ERequestReceive, aStatus, &aData); }
	inline void Report(TRequestStatus& aStatus, SReport& aData)
		{ DoRequest(ERequestReport, aStatus, &aData); }
	inline TInt SendData(const TDesC8& aData)
		{ return DoControl(EControlSend, (TAny*)&aData); }
	inline void FlushData()
		{ DoControl(EControlFlush); }
	inline void Finish()
		{ DoControl(EControlFinish); }
	inline void Cancel(TUint aMask)
		{ DoCancel(aMask); }
#endif
	};

#endif
