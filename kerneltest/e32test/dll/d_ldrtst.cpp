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
// f32test\loader\d_ldrtst.cpp
// LDD for testing loader
// 
//

#include <kernel/kern_priv.h>
#include "d_ldrtst.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

class DLdrTest;
class DLdrTestFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	DLdrTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DLdrTest : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DLdrTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
public:
	TInt GetCodeSegInfo(TAny* aHandle, TAny* aDest);
	TAny* ModuleCodeSeg(TModuleHandle aModuleHandle);
	TAny* ProcessCodeSeg(TInt aProcessHandle);
	TAny* LibraryCodeSeg(TInt aLibraryHandle);
	TInt GetCodeSegList(RLdrTest::SEntry* aList, TInt aMax);
	TAny* CodeSegFromAddr(TLinAddr aAddr);
	TModuleHandle ModuleHandleFromAddr(TLinAddr aAddr);
	TInt ProcessSMPUnsafeCount(TInt aProcessHandle);
private:
	SDblQueLink* FindCodeSegQueueAnchor();
	};

DECLARE_STANDARD_LDD()
	{
	return new DLdrTestFactory;
	}

DLdrTestFactory::DLdrTestFactory()
//
// Constructor
//
	{
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

TInt DLdrTestFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DLdrTest on this logical device
//
	{
	aChannel=new DLdrTest;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DLdrTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
	{
	return SetName(&KLdrTestLddName);
	}

void DLdrTestFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
	{
	TCapsTestV01 b;
	b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
	}

TInt DLdrTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
	{

	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return KErrNotSupported;
	return KErrNone;
	}

DLdrTest::~DLdrTest()
//
// Destructor
//
	{
	}

TInt DLdrTest::GetCodeSegInfo(TAny* aHandle, TAny* aDest)
	{
	TCodeSegCreateInfo info;
	DCodeSeg* pS=DCodeSeg::VerifyHandle(aHandle);
	if (pS)
		{
		Kern::AccessCode();
		pS->Info(info);
		Kern::EndAccessCode();
		kumemput32(aDest, &info, sizeof(info));
		return KErrNone;
		}
	return KErrArgument;
	}

TAny* DLdrTest::ModuleCodeSeg(TModuleHandle aModuleHandle)
	{
	DCodeSeg* pS=DCodeSeg::VerifyHandle(aModuleHandle);
	if(!pS)
		{
		Kern::AccessCode();
		DCodeSeg::CodeSegFromEntryPoint((TLinAddr)aModuleHandle);  // ignore returned DCodeSeg*
		Kern::EndAccessCode();
		}
	return pS;
	}

TAny* DLdrTest::CodeSegFromAddr(TLinAddr aAddr)
	{
	Kern::AccessCode();
	DCodeSeg* s = Kern::CodeSegFromAddress(aAddr, Kern::CurrentThread().iOwningProcess);
	Kern::EndAccessCode();
	return s;
	}

TModuleHandle DLdrTest::ModuleHandleFromAddr(TLinAddr aAddr)
	{
	TModuleHandle h = (TModuleHandle)CodeSegFromAddr(aAddr);
	if(!h)
		h = (TModuleHandle)aAddr;
	return h;
	}

TAny* DLdrTest::ProcessCodeSeg(TInt aProcessHandle)
	{
	DCodeSeg* pS=NULL;
	DThread& t=Kern::CurrentThread();
	NKern::LockSystem();
	DProcess* pP=(DProcess*)t.ObjectFromHandle(aProcessHandle, EProcess);
	if (pP)
		{
		pS=pP->iCodeSeg;
		if (!pS)
			pS=pP->iTempCodeSeg;
		}
	NKern::UnlockSystem();
	return pS;
	}

TAny* DLdrTest::LibraryCodeSeg(TInt aLibraryHandle)
	{
	DCodeSeg* pS=NULL;
	DThread& t=Kern::CurrentThread();
	NKern::LockSystem();
	DLibrary* pL=(DLibrary*)t.ObjectFromHandle(aLibraryHandle, ELibrary);
	if (pL)
		pS=pL->iCodeSeg;
	NKern::UnlockSystem();
	return pS;
	}

SDblQueLink* DLdrTest::FindCodeSegQueueAnchor()
	{
	SDblQueLink* p=&iDevice->iCodeSeg->iLink;	// this device driver's code segment
	for (;;)
		{
		p=p->iPrev;
		DCodeSeg* s=_LOFF(p, DCodeSeg, iLink);
		if (s->iExeCodeSeg==s && (s->iAttr & ECodeSegAttKernel))
			{
			// s is the kernel's code segment, which is the first one to be created
			return s->iLink.iPrev;
			}
		}
	}

TInt DLdrTest::GetCodeSegList(RLdrTest::SEntry* aList, TInt aMax)
	{
	if (aMax<=0)
		return KErrArgument;
	RLdrTest::SEntry list[128];
	Kern::AccessCode();
	SDblQueLink* anchor=FindCodeSegQueueAnchor();
	SDblQueLink* p=anchor->iNext;
	if (aMax>128)
		aMax=128;
	TInt n=0;
	for(; p!=anchor && n<aMax; p=p->iNext, ++n)
		{
		DCodeSeg* s=_LOFF(p, DCodeSeg, iLink);
		list[n].iHandle=s;
		list[n].iUid3=(TUint32)s->iUids.iUid[2].iUid;
		}
	Kern::EndAccessCode();
	if (n>0)
		kumemput32(aList, list, n*sizeof(RLdrTest::SEntry));
	return n;
	}

TInt DLdrTest::ProcessSMPUnsafeCount(TInt aProcessHandle)
	{
	TInt count=KErrNotFound;
	DThread& t=Kern::CurrentThread();
	NKern::LockSystem();
	DProcess* pP=(DProcess*)t.ObjectFromHandle(aProcessHandle, EProcess);
	if (pP)
		count=pP->iSMPUnsafeCount;
	NKern::UnlockSystem();
	return count;
	}

TInt DLdrTest::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
	switch (aFunction)
		{
		case RLdrTest::EControlGetCodeSegInfo:
			r=GetCodeSegInfo(a1,a2);
 			break;
		case RLdrTest::EControlProcessCodeSeg:
			r=(TInt)ProcessCodeSeg((TInt)a1);
 			break;
		case RLdrTest::EControlLibraryCodeSeg:
			r=(TInt)LibraryCodeSeg((TInt)a1);
 			break;
		case RLdrTest::EControlModuleCodeSeg:
			r=(TInt)ModuleCodeSeg((TModuleHandle)a1);
 			break;
		case RLdrTest::EControlGetCodeSegList:
			r=GetCodeSegList( (RLdrTest::SEntry*)a1, (TInt)a2 );
			break;
		case RLdrTest::EControlCodeSegFromAddr:
			r=(TInt)CodeSegFromAddr((TLinAddr)a1);
 			break;
		case RLdrTest::EControlModuleHandleFromAddr:
			r=(TInt)ModuleHandleFromAddr((TLinAddr)a1);
 			break;
		case RLdrTest::EControlProcessSMPUnsafeCount:
			r=ProcessSMPUnsafeCount((TInt)a1);
			break;
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

