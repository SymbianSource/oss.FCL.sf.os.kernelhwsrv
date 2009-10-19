// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkern\d_crazyints.h
//

#ifndef __CRAZYINTS_H__
#define __CRAZYINTS_H__

#include <e32cmn.h>
#include <e32ver.h>

const TInt KIntTestThreadPriority = 24;

const TInt KIntTestMajorVersionNumber = 1;
const TInt KIntTestMinorVersionNumber = 0;
const TInt KIntTestBuildVersionNumber = KE32BuildVersionNumber;

const TInt KNumOfTimes = 100; // number of times we kick off the time
const TInt KTimerWaitValue = 3; //number of timer ticks we use to set the timer..

class RBusIntTestClient: public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ETestCrazyInts
		};

#ifndef __KERNEL_MODE__
public:
	TInt Open(TDesC& aProxyName)
		{return (DoCreate(aProxyName,TVersion(KIntTestMajorVersionNumber,KIntTestMinorVersionNumber,KIntTestBuildVersionNumber),-1,NULL,NULL,EOwnerThread));}

	// test-cases
	TInt TestCrazyInts()
		{return DoControl(ETestCrazyInts, NULL);}


#endif
	};

#ifdef __KERNEL_MODE__
struct TCapsProxyClient
	{
	TVersion version;
	};

class DDeviceIntsTest: public DLogicalDevice
	{
public:
	DDeviceIntsTest();
	~DDeviceIntsTest();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

// declaration for the client channel
class DChannIntsTest: public DLogicalChannel
	{
public:
	DChannIntsTest();
	~DChannIntsTest();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
	static void Handler(TAny *aParam);


protected:
	virtual void HandleMsg(TMessageBase* aMsg); // Note: this is a pure virtual in DLogicalChannel

	TInt DoControl(TInt aId, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aId, TRequestStatus* aStatus, TAny* a1, TAny* a2);

private:
#ifdef __SMP__
	TInt TestCrazyInts();
	TBool CrazyInterruptsEnabled();

	TUint iStatus;
	NTimer iTimer;
	TInt iRunCount;
#endif
	TDynamicDfcQue* iDfcQue; // Kernel thread for the test driver
	DThread* iClient;
	};


#endif /* __KERNEL_MODE__ */

#endif /* __CRAZYINTS_H__ */
