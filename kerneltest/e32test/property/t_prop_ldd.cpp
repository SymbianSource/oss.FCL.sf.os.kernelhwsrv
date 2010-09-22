// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include "t_prop_ldd.h"
#include <kernel/kernel.h>
#include "nk_priv.h"

class DPropLDevice : public DLogicalDevice
	{
public:
	DPropLDevice();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DPropLChannel : public DLogicalChannelBase
	{
public:
	DPropLChannel();
	~DPropLChannel();

	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);

private:
	TInt Basic(RPropChannel::TBasicInfo* aInfo);

	static void CompleteFn(TAny* aPtr, TInt aReason);
	TInt iReason;
	NFastSemaphore iSem;

	};

DECLARE_STANDARD_LDD()
//
// Create a new device
//
	{
	return new DPropLDevice;
	}

DPropLDevice::DPropLDevice()
//
// Constructor
//
	{
	//iUnitsMask=0;
	iVersion = TVersion(1,0,1);
	// iParseMask = 0;
	}

TInt DPropLDevice::Install()
//
// Install the device driver.
//
	{
	TInt r = SetName(&KPropLdName);
	return r;
	}

void DPropLDevice::GetCaps(TDes8&) const
//
// Return the Comm capabilities.
//
	{
	}

TInt DPropLDevice::Create(DLogicalChannelBase*& aChannel)
//
// Create a channel on the device.
//
	{
	aChannel = new DPropLChannel;
	return aChannel ? KErrNone : KErrNoMemory;
	}

DPropLChannel::DPropLChannel() 
	{
	NKern::FSSetOwner(&iSem, NKern::CurrentThread());
	}

TInt DPropLChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /* aInfo*/ , const TVersion& aVer)
//
// Create the channel from the passed info.
//
	{
	if (!Kern::QueryVersionSupported(TVersion(1,0,1),aVer))
		return KErrNotSupported;
	return KErrNone;
	}

DPropLChannel::~DPropLChannel()
	{
	}


#define BASIC_ERROR(aRes, aCond) \
	{\
	if (!(aCond)) \
		{ \
		Kern::Printf("Test '" #aCond "' fails; r = %d;\n\tfile '" __FILE__ "'; line %d;\n", aRes, __LINE__); \
		prop.Close(); \
		return EFalse; \
		} \
	}

void DPropLChannel::CompleteFn(TAny* aPtr, TInt aReason)
	{ // static
	DPropLChannel* self = (DPropLChannel*) aPtr;
	self->iReason = aReason;
	NKern::FSSignal(&self->iSem);
	}

TBool DPropLChannel::Basic(RPropChannel::TBasicInfo* aInfo)
	{

	TUid category = aInfo->iCategory;
	TUint key = aInfo->iKey;
	TUint count = aInfo->iCount;
	RProperty::TType type = aInfo->iType;

	for (TUint i = 0; i < count; ++i)
		{
		RPropertyRef prop;
		TPropertyStatus propStatus;
		TInt r = prop.Open(category, key);
		BASIC_ERROR(r, r == KErrNotFound);
		r = prop.Attach(category, key);
		BASIC_ERROR(r, r == KErrNone);

		//	Defines the attributes and access control for a property. This can only be done 
		//	once for each property. Subsequent attempts to define the same property will return
		//	KErrAlreadyExists.

		TSecurityPolicy policy;

		NKern::LockSystem();
		TBool defined = prop.GetStatus(propStatus);
		NKern::UnlockSystem();
		BASIC_ERROR(defined, !defined); // should be EFALSE when not defined
		
		r = prop.Define(type, policy, policy);
		BASIC_ERROR(r, r == KErrNone);

		NKern::LockSystem();
		defined = prop.GetStatus(propStatus);
		NKern::UnlockSystem();
		BASIC_ERROR(defined, defined);  // should be ETRUE, when defined
		BASIC_ERROR(propStatus.iType, propStatus.iType==type);

		r = prop.Define(type, policy, policy);
		BASIC_ERROR(r, r == KErrAlreadyExists);
		r = prop.Delete();
		BASIC_ERROR(r, r == KErrNone);

		// Define fails with KErrArgument if wrong type or attribute was specified.
		r = prop.Define(RProperty::ETypeLimit, policy, policy);
		BASIC_ERROR(r, r == KErrArgument);

		static _LIT_SECURITY_POLICY_PASS(KPassPolicy);
		TSecurityPolicy badPolicy;
		*(TInt*)&badPolicy = -1;

		r = prop.Define(type, badPolicy, policy);
		BASIC_ERROR(r, r == KErrArgument);
		r = prop.Define(type, KPassPolicy, badPolicy);
		BASIC_ERROR(r, r == KErrArgument);
	
		if (type == RProperty::EInt)
			{
			// Define fails with KErrArgument if aType is TInt and aPreallocate is not 0
			r = prop.Define(type, KPassPolicy, KPassPolicy, 16);
			BASIC_ERROR(r, r == KErrArgument);

			// Following defintion the property has a default value, 0 for integer properties
			r = prop.Define(RProperty::EInt, KPassPolicy, KPassPolicy);
			BASIC_ERROR(r, r == KErrNone);
			TInt value;
			r = prop.Get(value);
			BASIC_ERROR(r, r == KErrNone);
			BASIC_ERROR(value, value == 0);
			r = prop.Delete();
			BASIC_ERROR(r, r == KErrNone);
			}
		else 
			{
			// Defne fails with KErrTooBig if aPeallocate is grater than KMaxPropertySize.
			r = prop.Define(RProperty::EByteArray, KPassPolicy, KPassPolicy, RProperty::KMaxPropertySize);
			BASIC_ERROR(r, r == KErrNone);
			r = prop.Delete();
			BASIC_ERROR(r, r == KErrNone);
			r = prop.Define(RProperty::EByteArray, KPassPolicy, KPassPolicy, RProperty::KMaxPropertySize+1);
			BASIC_ERROR(r, r == KErrTooBig);

			// Following defintion the property has a default value, zero-length data for byte-array and text 
			// properties. 
			r = prop.Define(RProperty::EByteArray, KPassPolicy, KPassPolicy);
			BASIC_ERROR(r, r == KErrNone);
			TBuf8<16> buf;
			r = prop.Get(buf);
			BASIC_ERROR(r, r == KErrNone);
			BASIC_ERROR(buf.Size(), buf.Size() == 0);
			r = prop.Delete();
			BASIC_ERROR(r, r == KErrNone);
			}

		// Pending subscriptions for this property will not be completed until a new value is published.
		TPropertySubsRequest subs(CompleteFn, this);
		iReason = KRequestPending;
		r = prop.Subscribe(subs);
		r = prop.Define(type, KPassPolicy, KPassPolicy);
		BASIC_ERROR(r, r == KErrNone);
		BASIC_ERROR(iReason, iReason == KRequestPending);
		r = prop.Delete();
		BASIC_ERROR(r, r == KErrNone);
		NKern::FSWait(&iSem);
		BASIC_ERROR(iReason, iReason == KErrNotFound);

		// If the property has not been defined Delete() fails with KErrNotFound.
		r = prop.Delete();
		BASIC_ERROR(r, r == KErrNotFound);

		// When deleted any pending subscriptions for the property will be completed with KErrNotFound.
		r = prop.Define(type, KPassPolicy, KPassPolicy);
		BASIC_ERROR(r, r == KErrNone);
		iReason = KRequestPending;
		r = prop.Subscribe(subs);
		BASIC_ERROR(r, r == KErrNone);
		BASIC_ERROR(iReason, iReason == KRequestPending);
		r = prop.Delete();
		BASIC_ERROR(r, r == KErrNone);
		NKern::FSWait(&iSem);
		BASIC_ERROR(iReason, iReason == KErrNotFound);

		// Any new request will not complete until the property is defined and published again.
		iReason = KRequestPending;
		r = prop.Subscribe(subs);
		BASIC_ERROR(r, r == KErrNone);
		BASIC_ERROR(iReason, iReason == KRequestPending);
		r = prop.Define(type, KPassPolicy, KPassPolicy);
		BASIC_ERROR(r, r == KErrNone);
		BASIC_ERROR(iReason, iReason == KRequestPending);
		if (type == RProperty::EInt)
			{
			r = prop.Set(1);
			BASIC_ERROR(r, r == KErrNone);
			}
		else
			{
			TBuf8<16> buf((TUint8*) "Foo");
			r = prop.Set(buf);
			BASIC_ERROR(r, r == KErrNone);
			}
		NKern::FSWait(&iSem);
		BASIC_ERROR(iReason, iReason == KErrNone);
		r = prop.Delete();
		BASIC_ERROR(r, r == KErrNone);

		// If the property has not been defined Set()/Get() fail with KErrNotFound.
			{
			TInt value;
			TBuf8<16> buf;
			if (type == RProperty::EInt)
				{
				r = prop.Get(value);
				BASIC_ERROR(r, r == KErrNotFound);
				r = prop.Set(value);
				BASIC_ERROR(r, r == KErrNotFound);
				}
			else
				{
				r = prop.Get(buf);
				BASIC_ERROR(r, r == KErrNotFound);
				r = prop.Set(buf);
				BASIC_ERROR(r, r == KErrNotFound);
				}
			}

		r = prop.Define(type, KPassPolicy, KPassPolicy);
		BASIC_ERROR(r, r == KErrNone);

		// If the property is larger than KMaxPropertySize Set() fails with KErrTooBig
			{
			if (type ==  RProperty::EByteArray)
				{
				TBuf8<RProperty::KMaxPropertySize + 1> buf(RProperty::KMaxPropertySize + 1);
				r = prop.Set(buf);
				BASIC_ERROR(r, r == KErrTooBig);
				}
			}

		// When type of operation mismatch with the property type Set()/Get() fails with KErrArgument.
			{
			if (type !=  RProperty::EInt)
				{
				TInt value;
				r = prop.Get(value);
				BASIC_ERROR(r, r == KErrArgument);
				r = prop.Set(value);
				BASIC_ERROR(r, r == KErrArgument);
				}
			else
				{
				TBuf8<16> buf;
				r = prop.Get(buf);
				BASIC_ERROR(r, r == KErrArgument);
				r = prop.Set(buf);
				BASIC_ERROR(r, r == KErrArgument);
				}
			}

		// Get/Set
		if (type == RProperty::EInt)
			{
			r = prop.Set(1);
			BASIC_ERROR(r, r == KErrNone);
			TInt value = 0;
			r = prop.Get(value);
			BASIC_ERROR(r, r == KErrNone);
			BASIC_ERROR(value, value == 1);
			}
		else 
			{
			TBuf8<16> ibuf((TUint8*)"Foo");
			TBuf8<16> obuf;
			r = prop.Set(ibuf);
			BASIC_ERROR(r, r == KErrNone);
			r = prop.Get(obuf);
			BASIC_ERROR(r, r == KErrNone);
			r = obuf.Compare(ibuf);
			BASIC_ERROR(r, r == 0);
			}

		// If the supplied buffer is too small Get() fails with KErrOverflow and the truncated value is reported.
		if (type == RProperty::EByteArray)
			{
			TBuf8<16> ibuf((TUint8*) "0123456789012345");
			TBuf8<16> obuf((TUint8*) "abcdefghigklmnop");
			TPtr8 optr((TUint8*) obuf.Ptr(), 0, 15);
			r = prop.Set(ibuf);
			BASIC_ERROR(r, r == KErrNone);

			NKern::LockSystem();
			defined = prop.GetStatus(propStatus);
			NKern::UnlockSystem();
			BASIC_ERROR(propStatus.iSize, propStatus.iSize == ibuf.Length());

			r = prop.Get(optr);
			BASIC_ERROR(r, r == KErrOverflow);
			BASIC_ERROR(optr.Length(), optr.Length() == 15); 
			BASIC_ERROR(obuf[14], obuf[14] == '4'); 
			BASIC_ERROR(obuf[15], obuf[15] == 'p');
			}

		// The calling thread will have the specified request status signalled when the property is next updated.
		iReason = KRequestPending;
		r = prop.Subscribe(subs);
		BASIC_ERROR(r, r == KErrNone);
		BASIC_ERROR(iReason, iReason == KRequestPending);
		if (type == RProperty::EInt)
			{
			r = prop.Set(1);
			BASIC_ERROR(r, r == KErrNone);
			}
		else
			{
			TBuf8<16> buf((TUint8*) "Foo");
			r = prop.Set(buf);
			BASIC_ERROR(r, r == KErrNone);
			}
		NKern::FSWait(&iSem);
		BASIC_ERROR(iReason, iReason == KErrNone);

		r = prop.Delete();
		BASIC_ERROR(r, r == KErrNone);

		// Cancel an outstanding subscription request.
		// If it has not already completed, the request is completed with KErrCancelled.
		iReason = KRequestPending;
		r = prop.Subscribe(subs);
		BASIC_ERROR(r, r == KErrNone);
		BASIC_ERROR(iReason, iReason == KRequestPending);
		prop.Cancel(subs);		
		NKern::FSWait(&iSem);
		BASIC_ERROR(iReason, iReason == KErrCancel);

		r = prop.Define(type, KPassPolicy, KPassPolicy);
		BASIC_ERROR(r, r == KErrNone);

		iReason = KRequestPending;
		r = prop.Subscribe(subs);
		BASIC_ERROR(r, r == KErrNone);
		BASIC_ERROR(iReason, iReason == KRequestPending);
		if (type == RProperty::EInt)
			{
			r = prop.Set(1);
			BASIC_ERROR(r, r == KErrNone);
			}
		else
			{
			TBuf8<16> buf((TUint8*) "Foo");
			r = prop.Set(buf);
			BASIC_ERROR(r, r == KErrNone);
			}
		NKern::FSWait(&iSem);
		BASIC_ERROR(iReason, iReason == KErrNone);
		prop.Cancel(subs);		
		BASIC_ERROR(iReason, iReason == KErrNone);

		iReason = KRequestPending;
		r = prop.Subscribe(subs);
		BASIC_ERROR(r, r == KErrNone);
		BASIC_ERROR(iReason, iReason == KRequestPending);
		prop.Cancel(subs);		
		NKern::FSWait(&iSem);
		BASIC_ERROR(iReason, iReason == KErrCancel);

		r = prop.Delete();
		BASIC_ERROR(r, r == KErrNone);

		prop.Close();
		}
	return ETrue;
	}

//
// Client requests.
//
TBool DPropLChannel::Request(TInt aFunction, TAny* a1, TAny*)
	{
	TBool r;
	switch (aFunction)
		{
		case RPropChannel::EBasicTests:
			RPropChannel::TBasicInfo info;
			kumemget32(&info, a1, sizeof(info));
			NKern::ThreadEnterCS();
			r = Basic(&info);
			NKern::ThreadLeaveCS();
			break;	
		default:
			r = EFalse;
			break;
		}
	return r;
	}
