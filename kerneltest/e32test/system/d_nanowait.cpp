// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\d_nanowait.cpp
// LDD for testing nanosecond blocking
// 
//

#include "plat_priv.h"
#include "d_nanowait.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;


// global Dfc Que
TDynamicDfcQue* gDfcQ;


class DNanoWaitFactory : public DLogicalDevice
//
// NanoWait LDD factory
//
	{
public:
	DNanoWaitFactory();
	~DNanoWaitFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DNanoWait : public DLogicalChannel
//
// nanowait LDD channel
//
	{
public:
	DNanoWait();
	~DNanoWait();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	virtual void HandleMsg(TMessageBase* aMsg);
public:
	inline DThread* Client() { return iThread; }
public:
	DThread* iThread;
	TDynamicDfcQue* iDfcQ;
	};



DECLARE_STANDARD_LDD()
	{
    return new DNanoWaitFactory;
    }

DNanoWaitFactory::DNanoWaitFactory()
//
// Constructor
//
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }

TInt DNanoWaitFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DMsTim on this logical device
//
    {
	aChannel=new DNanoWait;
	return aChannel?KErrNone:KErrNoMemory;
    }

const TInt KDNanoWaitThreadPriority = 27;
_LIT(KDNanoWaitThread,"DNanoWaitThread");

TInt DNanoWaitFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(gDfcQ, KDNanoWaitThreadPriority, KDNanoWaitThread);

#ifdef CPU_AFFINITY_ANY
	NKern::ThreadSetCpuAffinity((NThread*)(gDfcQ->iThread), KCpuAffinityAny);			
#endif

	if (r != KErrNone)
		return r; 	

    return SetName(&KNanoWaitLddName);
    }

void DNanoWaitFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsNanoWaitV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

/**
  Destructor
*/
DNanoWaitFactory::~DNanoWaitFactory()
	{
	if (gDfcQ)
		gDfcQ->Destroy();
	}

DNanoWait::DNanoWait()
//
// Constructor
//
    {
	iThread=&Kern::CurrentThread();
	iThread->Open();
    }

TInt DNanoWait::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {

    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	SetDfcQ(gDfcQ);
	iMsgQ.Receive();
	return KErrNone;
	}

DNanoWait::~DNanoWait()
//
// Destructor
//
    {
	Kern::SafeClose((DObject*&)iThread, NULL);
    }

void DNanoWait::HandleMsg(TMessageBase* aMsg)
	{
	TInt r=KErrNone;
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;
	if (id==(TInt)ECloseMsg)
		{
		m.Complete(KErrNone,EFalse);
		iMsgQ.CompleteAll(KErrServerTerminated);
		return;
		}
	else
		{
		r=DoControl(id,m.Ptr0(),m.Ptr1());
		}
	m.Complete(r,ETrue);
	}

TInt DNanoWait::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
	TInt interval=(TInt)a2;
	switch (aFunction)
		{
		case RNanoWait::EControlStartNanoWait:
			{
			TInt loopCount=(TInt)a1;
			for( int loop = 0; loop < loopCount; loop++)
				{
				Kern::NanoWait(interval);	
				}
			break;
			}
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

