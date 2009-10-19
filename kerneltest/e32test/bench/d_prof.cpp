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
// e32test\bench\d_prof.cpp
// LDD for thread time profiling
// 
//


#ifndef __WINS__
#include "platform.h"
#else
#include <kernel/kernel.h>
#endif
#include "d_prof.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

class DProfile;

class DProfileFactory : public DLogicalDevice
//
// Profile LDD factory
//
	{
public:
	DProfileFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual DLogicalChannel* CreateL();			//overriding pure virtual
	};

class DProfile : public DLogicalChannel
//
// Profile logical channel
//
	{
public:
	DProfile(DLogicalDevice* aLogicalDevice);
	~DProfile();
protected:
	virtual void DoCancel(TInt aReqNo);						//overriding pure virtual
	virtual void DoRequest(TInt aReqNo,TAny* a1,TAny* a2);	//overriding pure virtual
	virtual void DoCreateL(TInt aUnit,CBase* aPdd,const TDesC* anInfo,const TVersion& aVer);
	virtual TInt DoControl(TInt aFunction,TAny *a1,TAny *a2);
	};

DECLARE_STANDARD_LDD()
	{
    return new DProfileFactory;
    }

DProfileFactory::DProfileFactory()
//
// Constructor
//
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }

DLogicalChannel* DProfileFactory::CreateL()
//
// Create a new DProfile on this logical device
//
    {
    return(new(ELeave) DProfile(this));
    }

TInt DProfileFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
    TPtrC name=_L("Profile");
    return(SetName(&name));
    }

void DProfileFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsProfileV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

DProfile::DProfile(DLogicalDevice* aLogicalDevice)
//
// Constructor
//
    : DLogicalChannel(aLogicalDevice)
    {
    }

void DProfile::DoCreateL(TInt /*aUnit*/,CBase* /*aPdd*/,const TDesC* /*anInfo*/,const TVersion& aVer)
//
// Create channel
//
    {

    if (!User::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	User::Leave(KErrNotSupported);
	}

DProfile::~DProfile()
//
// Destructor
//
    {
    }

void DProfile::DoCancel(TInt /*aReqNo*/)
//
// Cancel an outstanding request - overriding pure virtual
//
    {
	// not used
	}

void DProfile::DoRequest(TInt /*aReqNo*/, TAny* /*a1*/, TAny* /*a2*/)
//
// Asynchronous requests - overriding pure virtual
//
    {
	// not used
    }

LOCAL_C void Reset()
	{
	CObjectCon& threads=*Kern::Threads();
	TInt i;
	TInt c=threads.Count();
	for (i=0; i<c; i++)
		{
		DPlatThread *pT=(DPlatThread*)threads[i];
		pT->iTotalCpuTime=0;
		pT->iMaxContinuousCpuTime=0;
		pT->iLastYieldTotal=0;
		pT->iMaxTimeBeforeYield=0;
		}
	}

LOCAL_C TInt Read(TInt aHandle, TProfileData& aData)
	{
	DPlatThread *pT=(DPlatThread*)Kern::ThreadFromHandle(aHandle);
	if (!pT)
		return KErrArgument;
	aData.iTotalCpuTime=(pT->iTotalCpuTime * 125)>>6;
	aData.iMaxContinuousCpuTime=(pT->iMaxContinuousCpuTime * 125)>>6;
	aData.iMaxTimeBeforeYield=(pT->iMaxTimeBeforeYield * 125)>>6;
	return KErrNone;
	}

#if defined(__MARM__)
TInt DProfile::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
	switch (aFunction)
		{
		case RProfile::EControlResetProfile:
			Reset();
			break;
		case RProfile::EControlReadProfile:
			r=Read(TInt(a1),*(TProfileData*)a2);
			break;
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}
#else
TInt DProfile::DoControl(TInt /*aFunction*/, TAny* /*a1*/, TAny* /*a2*/)
	{

	return KErrNotSupported;
	}
#endif

