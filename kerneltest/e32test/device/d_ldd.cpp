// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\d_ldd.cpp
// LDD for testing LDD static data
// 
//

#include <kernel/kernel.h>
#include "d_ldd.h"
#include "d_ldd2.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

TInt AFunction()
	{
	return KErrNone;
	}

TInt data=0x100;
TAny* dataptr=(TAny*)&AFunction;
TInt TheBss;

class TGlobal
	{
public:
	TGlobal();
	~TGlobal();
	void Update(TUint32 a);
	TInt Verify();
public:
	TUint32 iInt;
	TAny* iPtr;
	};

TGlobal Global;

TGlobal::TGlobal()
	{
	__KTRACE_OPT(KDEVICE,Kern::Printf("TGlobal::TGlobal()"));
	iPtr = Kern::Alloc(65536);
	Update(487);
	}

TGlobal::~TGlobal()
	{
	__KTRACE_OPT(KDEVICE,Kern::Printf("TGlobal::~TGlobal()"));
	Kern::Free(iPtr);
	}

void TGlobal::Update(TUint32 a)
	{
	iInt = a;
	if (iPtr)
		{
		TUint32* p = (TUint32*)iPtr;
		TUint32* pE = p + 65536/4;
		while (p<pE)
			{
			a = (a*69069u)+41;
			*p++ = a;
			}
		}
	}

TInt TGlobal::Verify()
	{
	TUint32 x = iInt;
	if (iPtr)
		{
		TUint32* p = (TUint32*)iPtr;
		TUint32* pE = p + 65536/4;
		while (p<pE)
			{
			x = (x*69069u)+41;
			if (*p++ != x)
				return KErrGeneral;
			}
		return KErrNone;
		}
	return KErrNoMemory;
	}

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

class DKInstallTestFactory : public DLogicalDevice
//
// Extra test device which will be installed from kernel side using
// Kern::InstallLogicalDevice
//
	{
public:
	DKInstallTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DKInstallTest : public DLogicalChannelBase
//
// Extra test logical channel
//
	{
public:
	virtual ~DKInstallTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	};

DECLARE_STANDARD_LDD()
	{
	return Global.iPtr ? new DTestFactory : NULL;
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
		case RLddTest::EControlTest1:
			r=(TInt)dataptr;
			break;
		case RLddTest::EControlTest2:
			r=data++;
			break;
		case RLddTest::EControlTest3:
			r=data--;
			break;
		case RLddTest::EControlTest4:
			r=data;
			break;
		case RLddTest::EControlTest5:
			r=TheBss;
			break;
		case RLddTest::EControlTest6:
			TheBss=(TInt)a1;
			break;
		case RLddTest::EControlTest7:
			r = (TInt)Global.iInt;
			break;
		case RLddTest::EControlTest8:
			Global.Update((TUint32)a1);
			break;
		case RLddTest::EControlTest9:
			r = Global.Verify();
			break;
		case RLddTest::EControlLinkedTest1:
			r=LinkedTest1();
			break;
		case RLddTest::EControlLinkedTest2:
			r=LinkedTest2();
			break;
		case RLddTest::EControlLinkedTest3:
			r=LinkedTest3();
			break;
		case RLddTest::EControlLinkedTest4:
			r=LinkedTest4();
			break;
		case RLddTest::EControlLinkedTest5:
			r=LinkedTest5();
			break;
		case RLddTest::EControlLinkedTest6:
			r=LinkedTest6((TInt)a1);
			break;
		case RLddTest::EControlLinkedTest7:
			r = LinkedTest7();
			break;
		case RLddTest::EControlLinkedTest8:
			LinkedTest8((TUint32)a1);
			break;
		case RLddTest::EControlLinkedTest9:
			r = LinkedTest9();
			break;
		case RLddTest::EControlTestKInstall:
				{
				r = KErrNoMemory;
				NKern::ThreadEnterCS();
				DLogicalDevice* device = new DKInstallTestFactory;
				if (device!=NULL)
					r = Kern::InstallLogicalDevice(device);
				NKern::ThreadLeaveCS();
				}
			break;
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

DKInstallTestFactory::DKInstallTestFactory()
//
// Constructor
//
	{
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	//iParseMask=0;//No units, no info, no PDD
	//iUnitsMask=0;//Only one thing
	}

TInt DKInstallTestFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DKInstallTest on this logical device
//
	{
	aChannel=new DKInstallTest;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DKInstallTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
	{
	return SetName(&KKInstallLddName);
	}

void DKInstallTestFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
	{
	TCapsTestV01 b;
	b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
	}

TInt DKInstallTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
//
// Create channel
//
	{

	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return KErrNotSupported;
	return KErrNone;
	}

DKInstallTest::~DKInstallTest()
//
// Destructor
//
	{
	}
	
TInt DKInstallTest::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	(void)aFunction;
	(void)a1;
	(void)a2;
	return KErrNotSupported;
	}

