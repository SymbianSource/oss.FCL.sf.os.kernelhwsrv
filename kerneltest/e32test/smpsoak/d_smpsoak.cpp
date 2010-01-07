// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\smpsoak\d_smpsoak.cpp
//

// LDD for smpsoak - setting Thread CPU Affinity
//

#include "d_smpsoak.h"
#include <platform.h>
#include <kernel/kern_priv.h>

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

class DSmpSoakFactory : public DLogicalDevice
//
// IPC copy LDD factory
//
	{
public:
	DSmpSoakFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DSmpSoak : public DLogicalChannelBase
	{
public:
	DSmpSoak();
	virtual ~DSmpSoak();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
public:
	static void IDfcFn(TAny* aPtr);
public:
	void OccupyCpus();
	};

DECLARE_STANDARD_LDD()
	{
	Kern::Printf("DSmpSoak called");
    return new DSmpSoakFactory;
    }

DSmpSoakFactory::DSmpSoakFactory()
//
// Constructor
//
    {
	Kern::Printf("DSmpSoakFactory::DSmpSoakFactory called");
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    }

TInt DSmpSoakFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DSmpSoak on this logical device
//
    {
	Kern::Printf("DSmpSoakFactory::Create called");
	aChannel=new DSmpSoak;
    return aChannel?KErrNone:KErrNoMemory;
    }

TInt DSmpSoakFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
	Kern::Printf("DSmpSoakFactory::Install called");
    return SetName(&KSmpSoakLddName);
    }

void DSmpSoakFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    Kern::Printf("DSmpSoakFactory::GetCaps called");
    }

DSmpSoak::DSmpSoak()
//
// Constructor
//
	{
	Kern::Printf("DSmpSoak::DSmpSoak called");
	}

DSmpSoak::~DSmpSoak()
	{
	Kern::Printf("DSmpSoak::~DSmpSoak called");
	}

TInt DSmpSoak::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {
	Kern::Printf("DSmpSoak::DoCreate called");

    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	
	return KErrNone;
	}


TInt DSmpSoak::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	DThread *pT = NULL;
	NThread *pMyNThread = NULL;
	TInt handle = (TInt)a1;
	TInt priority = (TInt)a2;

	TInt r = KErrNotSupported;
	Kern::Printf("DSmpSoak::Request called aFunction = %d, a1 = %d, a2 = %d", aFunction, a1, a2);

	switch (aFunction)
		{
		case RSMPSoak::KGETPROCESSORCOUNT:
			r = NKern::NumberOfCpus();
			Kern::Printf("DSmpSoak::Request Processor count = %d", r);
			break;
		case RSMPSoak::KGETCURRENTCPU:
			r = NKern::CurrentCpu();
			Kern::Printf("DSmpSoak::Request Current CPU = %d", r);
			break;
		case RSMPSoak::KGETCURRENTTHREAD:
			r = (TInt)NKern::CurrentThread();
			Kern::Printf("DSmpSoak::Request Current Thread %02x", r);
			break;
		case RSMPSoak::KTHREADSETCPUAFFINITY:
			r = NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), (TInt)a1);
			r = (TInt)NKern::CurrentCpu();
			Kern::Printf("DSmpSoak::Request Current Cpu = %d", r);
			break;
		case RSMPSoak::KOCCUPYCPUS:
			Kern::Printf("DSmpSoak::Request OCCUPYCPUS: called");
			OccupyCpus();
			break;
		case RSMPSoak::KCHANGEAFFINITY:
			Kern::Printf("DSmpSoak::Request CHANGEAFFINITY");
			NKern::LockSystem();
			pT=(DThread*)Kern::CurrentThread().ObjectFromHandle(handle);
			pMyNThread=(NThread*)&pT->iNThread;
			NKern::ThreadSetCpuAffinity((NThread*)pMyNThread, (TInt)a2);
			NKern::UnlockSystem();
			break;
		case RSMPSoak::KCHANGETHREADPRIORITY:
			Kern::Printf("DSmpSoak::Request CHANGETHREADPRIORITY");
			NKern::LockSystem();
			pT=(DThread*)Kern::CurrentThread().ObjectFromHandle(handle);
			Kern::Printf("DSmpSoak::Request Current Thread %d", pT);
			pT->SetThreadPriority(priority);
			Kern::Printf("DSmpSoak::CHANGETHREADPRIORITY now  %d", pT->iThreadPriority);
			NKern::UnlockSystem();
			break;
		default:
			Kern::Printf("DSmpSoak::Request default: called");
			break;
		}
	return r;
	}

void DSmpSoak::OccupyCpus()
	{
	Kern::Printf(">>>DSmpSoak::OccupyCpus()");
	}



