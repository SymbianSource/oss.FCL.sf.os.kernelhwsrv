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
// e32test\emul\d_guiconfig.cpp
// LDD for testing emulator GUI config API
// 
//

#include <kernel/kernel.h>
#include <wins/winsgui.h>
#include <e32keys.h>
#include "d_guiconfig.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

class DTest;

class DTestFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	DTestFactory();
	virtual TInt Install(); 					//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

class DTest : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	};

DECLARE_STANDARD_LDD()
	{
	return new DTestFactory;
	}

DTestFactory::DTestFactory()
//
// Constructor
//
	{
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	//iParseMask=0;//No units, no info, no PDD
	//iUnitsMask=0;//Only one thing
	}

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DTest on this logical device
//
	{
	aChannel=new DTest;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
	{
	return SetName(&KLddName);
	}

void DTestFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
	{
	TCapsTestV01 b;
	b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
	}

TInt DTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
//
// Create channel
//
	{

	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return KErrNotSupported;
	return KErrNone;
	}

DTest::~DTest()
//
// Destructor
//
	{
	}

TInt DTest::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	(void)a1;
	(void)a2;
	TInt r=KErrNone;
	switch (aFunction)
		{
		case RGuiConfigTest::EGetConfig:
			{
			r = WinsGui::CurrentConfiguration();
			break;
			}
		case RGuiConfigTest::EGenerateKeyEvent:
			{
			r = WinsGui::CurrentConfiguration();

			TRawEvent eventDown, eventUp;
			eventDown.Set(TRawEvent::EKeyDown, (EKeyScreenDimension0 + r)<<16);
			eventUp.Set(TRawEvent::EKeyUp, (EKeyScreenDimension0 + r)<<16);

			NKern::ThreadEnterCS();
			r = Kern::AddEvent(eventDown);
			if (r == KErrNone)
				r = Kern::AddEvent(eventUp);
			NKern::ThreadLeaveCS();
			break;
			}
		default:
			{
			r=KErrNotSupported;
			break;
			}
		}
	return r;
	}

