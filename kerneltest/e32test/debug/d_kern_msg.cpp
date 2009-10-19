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
// LDD for testing nanokernel debug trace bits
// 
//

//#undef __REMOVE_PLATSEC_DIAGNOSTICS__
#include "platform.h"
#include <kernel/kern_priv.h>
#include "u32std.h"
#include "d_kern_msg.h"


const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

class DKernMsgTestFactory : public DLogicalDevice
//
// Kernel msg test LDD factory
//
	{
public:
	DKernMsgTestFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DKernMsgTest : public DLogicalChannelBase
//
// Trace test LDD channel
//
	{
public:
	DKernMsgTest();
	~DKernMsgTest();
protected:
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);

private:
	DThread* iClient;
	TInt iShotNumber;
	TInt iCurShot;
	NTimer iNTimer;
	static void NTimerCallBack(TAny*);
	};


DECLARE_STANDARD_LDD()
	{
	//=== load
    return new DKernMsgTestFactory;
    }

DKernMsgTestFactory::DKernMsgTestFactory()
//
// Constructor
//
    {
	//=== load
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    }

TInt DKernMsgTestFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DNKTraceTest on this logical device
//
    {
	//=== open
	aChannel=new DKernMsgTest;
    return aChannel?KErrNone:KErrNoMemory;
    }

TInt DKernMsgTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
	//=== load
    return SetName(&KLddName);
    }


void DKernMsgTestFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsTraceTestV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

DKernMsgTest::DKernMsgTest()
	: iNTimer(NTimerCallBack, this)
//
// Constructor
//
	//=== open
    {
	// Get pointer to client threads DThread object
	iClient=&Kern::CurrentThread();

	// Open a reference on client thread so it's control block can't dissapear until
	// this driver has finished with it.
	// Note, this call to Open can't fail since its the thread we are currently running in
	iClient->Open();
    }

/**
  Destructor
*/
DKernMsgTest::~DKernMsgTest()
	{
	iNTimer.Cancel();
	// Close our reference on the client thread
	Kern::SafeClose((DObject*&)iClient,NULL);
	}

TInt DKernMsgTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {

	//=== open
    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	return KErrNone;
	}

TInt DKernMsgTest::Request(TInt aReqNo, TAny* a1, TAny*)
	{
	TInt r=KErrNotSupported;
	switch (aReqNo)
		{
		case RKernMsgTest::EControlKDebug:
			{

			TBuf8<1024> msg;
			
			TInt ret=Kern::ThreadDesRead(iClient,a1,msg,0,0);
			if (ret != KErrNone)
				return ret;

			*((char*)msg.Ptr() + msg.Length()) = '\0';

			Kern::Printf("%s", msg.Ptr());
			r = Kern::CurrentThread().iId;

			}
			break;
		case RKernMsgTest::EControlIsrContextTest:
			{
			iShotNumber = (TInt) a1;

			iCurShot = 0;

			iNTimer.OneShot(5);
			
			r = KErrNone; 
			}
			break;

		default:
			break;
		}
	return r;
	}

void DKernMsgTest::NTimerCallBack(TAny* ptr)
	{
	DKernMsgTest* p = (DKernMsgTest*) ptr;

	if (p->iCurShot++ == p->iShotNumber)
		return;

	Kern::Printf("%d", p->iCurShot);
	
	p->iNTimer.Again(10);
	}
