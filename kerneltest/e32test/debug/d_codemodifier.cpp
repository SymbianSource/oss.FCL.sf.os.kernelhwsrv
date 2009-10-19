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
// e32test\debug\d_codemodifier.cpp
// See e32test\debug\t_codemodifier.cpp for details
// 
//

#include "d_codemodifier.h"
#include <kernel/kern_priv.h>
#include <kernel/cache.h>

class DCodeModifier : public DLogicalChannelBase
	{
public:
	DCodeModifier();
	~DCodeModifier();
	static TBool Handler (const TDesC8& aText, TTraceSource aTraceSource);
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	DThread* iThread[2];
	};

DCodeModifier* CodeModifierDriver;

DCodeModifier::DCodeModifier() 
	{
	}

DCodeModifier::~DCodeModifier()
	{
	CodeModifierDriver = NULL;
	}


/**Creates the channel*/
TInt DCodeModifier::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

/**User side request entry point.*/
TInt DCodeModifier::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r = KErrNone;
	RCodeModifierDevice::TData data;

	kumemget(&data,a1,sizeof(data));
	switch (aFunction)
	{
		case RCodeModifierDevice::EControlThreadId:
			{
			DObjectCon* threads=Kern::Containers()[EThread];
			NKern::ThreadEnterCS();
			threads->Wait();
			iThread[data.iServer]=Kern::ThreadFromId(data.iThreadId);
			if(!iThread[data.iServer])
				r=KErrNotFound;
			threads->Signal();
			NKern::ThreadLeaveCS();
			}
			break;

		case RCodeModifierDevice::EControlReadWord:
			{	
			TInt val;
			r = Kern::ThreadRawRead(iThread[data.iServer], (const TAny*) data.iAddress, (TAny*) &val, sizeof(TInt));
			kumemput(a2,&val,sizeof(TInt));
			}
			break;

		case RCodeModifierDevice::EControlWriteWord:
			r = Kern::ThreadRawWrite(iThread[data.iServer], (TAny*)data.iAddress,(const TAny*) &a2, sizeof(TInt));
			break;

		case RCodeModifierDevice::EControlWriteCode:
			r = DebugSupport::ModifyCode(iThread[data.iServer], (TLinAddr) data.iAddress, data.iSize, (TUint)a2, DebugSupport::EBreakpointGlobal);
			if (r == DebugSupport::EBreakpointGlobal)
				r = KErrNone;	
			break;

		case RCodeModifierDevice::EControlRestoreCode:
			r = DebugSupport::RestoreCode(iThread[data.iServer], (TLinAddr)data.iAddress);	
			break;

		case RCodeModifierDevice::EInitialiseCodeModifier:
			{
			TUint cap;
			r = DebugSupport::InitialiseCodeModifier(cap,data.iSize);
			if (r && (cap!=DebugSupport::EBreakpointGlobal))
				r = KErrGeneral;
			}
			break;

		case RCodeModifierDevice::ECloseCodeModifier:
			DebugSupport::CloseCodeModifier();
			break;
			
		case RCodeModifierDevice::EControlAllocShadowPage:
			NKern::ThreadEnterCS();
			r = Epoc::AllocShadowPage(data.iAddress & ~(Kern::RoundToPageSize(1)-1));
			NKern::ThreadLeaveCS();
			break;

		case RCodeModifierDevice::EControlFreeShadowPage:
			NKern::ThreadEnterCS();
			r = Epoc::FreeShadowPage(data.iAddress & ~(Kern::RoundToPageSize(1)-1));
			NKern::ThreadLeaveCS();
			break;

		default:
			r=KErrNotSupported;
		}
	return r;
	}

//////////////////////////////////////////
class DTestFactory : public DLogicalDevice
	{
public:
	DTestFactory();
	// from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

DTestFactory::DTestFactory()
    {
    iParseMask = KDeviceAllowUnit;
    iUnitsMask = 0x3;
    }

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
    {
	CodeModifierDriver = new DCodeModifier;
	aChannel = CodeModifierDriver;
	return (aChannel ? KErrNone : KErrNoMemory);
    }

TInt DTestFactory::Install()
    {
    return SetName(&KCodeModifierName);
    }

void DTestFactory::GetCaps(TDes8& /*aDes*/) const
    {
    }

DECLARE_STANDARD_LDD()
	{
    return new DTestFactory;
	}
