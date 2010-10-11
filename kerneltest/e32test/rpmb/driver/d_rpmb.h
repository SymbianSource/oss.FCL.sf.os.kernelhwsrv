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
// e32test/rpmb/rpmb.h
// 
//

/**
 @file
 @internalComponent
 @prototype
*/

#include <kernel/kern_priv.h>
#include <drivers/rpmbdevice.h>
#include <drivers/mmc.h>

// banner for reporting progress of these tests 
_LIT(KDRpmbTestBanner,"RPMB TEST DRIVER: ");

// RPMB test driver factory class
class DRpmbTestFactory : public DLogicalDevice
	{
public:
	DRpmbTestFactory();
	~DRpmbTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

// RPMB test driver request handling class
class DRpmbTest : public DLogicalChannelBase
	{
public:
	DRpmbTest();
	virtual ~DRpmbTest();
	TInt DRpmbDeviceTests();
	TInt RpmbStackTests();
protected:
	TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	TInt SendAccessRequestNotOpen();
	TInt OpenWithBadIndex();
	TInt MultipleOpen();
	TInt SendAccessRequestBadParms();
	TInt InvalidRequestId();
	TInt IsKeyProgrammed();
	TInt WriteKey();
	TInt ReadWriteCounter();
	TInt ReadData();
	TUint DecodeResponse(TUint8 * aResp);
	TUint DecodeResult(TUint8 * aResp);
	TUint32 DecodeCounter(TUint8 * aResp);
	void DisplayReadData(TUint8 * aResp);
	static void BusCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2);
	static void StackCallBack(TAny * aSelf);
	TInt StackBadIndex();
	TInt SetupForStackTests();
	TInt StackIsKeyProgrammed();
	TInt StackWriteKey();
	TInt StackReadWriteCounter();
	TInt StackReadData();
	TInt SendToStackAndWait();	
private:
	DRpmbDevice iRpmb;
	DRpmbDevice iRpmbSecondInstance;
	TUint8 * iRequest;
	TUint8 * iResponse;
	TBool iKeySet;
	TUint8* iBufPtr;
	DSemaphore * iStackSemPtr;
	DSemaphore * iPowerSemPtr;
	TPBusCallBack iBusCallBack;
	TMMCCallBack iSessionEndCallBack;
	DMMCSocket* iSocket;
	DMMCSession* iSession;
	};
