// Copyright (c) 1999-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// hal\src\hal_main.cpp
// 
//

#include <kernel/hal_int.h>

_LIT(KLitHal,"HAL");
void HalInternal::Panic(HalInternal::THalPanic aPanic)
	{
	User::Panic(KLitHal,aPanic);
	}

void HalInternal::InitialiseData()
	{
	TInt i;
	_LIT_SECURITY_POLICY_PASS(KPassPolicy);
	_LIT_SECURITY_POLICY_C1(KWriteDevDataPolicy, ECapabilityWriteDeviceData);
	
	for (i=0; i<HAL::ENumHalAttributes; i++)                // for every attribute,
		{
		if (Properties[i]&HAL::EValid && !Implementation[i] && (Properties[i] & HAL::ESettable)) 
			// if it's implemented, not a constant and not a user function
			{
			 TInt r = RProperty::Define(KUidSystemCategory, KUidHalPropertyKeyBase+i, RProperty::EInt,
					KPassPolicy, KWriteDevDataPolicy); 
			 __ASSERT_ALWAYS(r==KErrNone || r==KErrAlreadyExists,Panic(EInitialAllocFailed1)); 
			 if (r==KErrAlreadyExists)
				break;

			 // This will panic if the first instance of the DLL to start does not have WriteDeviceData
			 // capability.  Again, there isn't anything we can do about this.  It shouldn't ever happen.
			 r = RProperty::Set(KUidSystemCategory, KUidHalPropertyKeyBase+i, InitialValue[i]);
			 __ASSERT_ALWAYS(r==KErrNone,Panic(EInitialAllocFailed2));
			}
		}
	}

TInt HalInternal::ReadWord(TInt aKey)
	{
	 TInt result;

	// This is a slow way of doing it, but it is light on memory as it doesn't
	// need an offset array.

	if(aKey>=HAL::ENumHalAttributes)
		return KErrNotFound;
	TInt r=RProperty::Get(KUidSystemCategory, KUidHalPropertyKeyBase+aKey, result);
	if (r!=KErrNone)
		{
		InitialiseData();
		r=RProperty::Get(KUidSystemCategory, KUidHalPropertyKeyBase+aKey, result);
		}
	__ASSERT_ALWAYS(r==KErrNone,Panic(EGetPropFailed)); 
	return (result);
	}

TInt HalInternal::WriteWord(TInt aKey, TInt aValue)
	{
	TInt r;

	if(aKey>=HAL::ENumHalAttributes)
		return KErrArgument;
	r=RProperty::Set(KUidSystemCategory, KUidHalPropertyKeyBase+aKey, aValue);
	if (r!=KErrNone && r!=KErrPermissionDenied) 
		{
		InitialiseData();
		r=RProperty::Set(KUidSystemCategory, KUidHalPropertyKeyBase+aKey, aValue);
		}
	__ASSERT_ALWAYS(r==KErrNone || r==KErrPermissionDenied,Panic(ESetPropFailed));
	return r;
	}


EXPORT_C TInt HAL::Get(HAL::TAttribute aAttribute, TInt& aValue)
	{
	return HAL::Get(0,aAttribute,aValue);
	}


EXPORT_C TInt HAL::Set(HAL::TAttribute aAttribute, TInt aValue)
	{
	return HAL::Set(0,aAttribute,aValue);
	}


EXPORT_C TInt HAL::GetAll(TInt& aNumEntries, SEntry*& aData)
	{
	TInt max_devices=1;

	HAL::Get(EDisplayNumberOfScreens,max_devices);
	TInt size=max_devices*(TInt)ENumHalAttributes*(TInt)sizeof(SEntry);
	SEntry* pE=(SEntry*)User::Alloc(size);
	if (!pE)
		{
		aNumEntries=0;
		aData=NULL;
		return KErrNoMemory;
		}

	TInt device;
	for(device=0;device<max_devices;device++)
		{
		TInt r;
		TInt i=ENumHalAttributes-1;
		for (; i>=0; --i)
			{
			TInt offset = device*(TInt)ENumHalAttributes + i;
			TInt properties=HalInternal::Properties[i];
			// Exclusion of the EDisplayMemoryHandle attribute is a work around
			// to avoid the handle and resources related to it from being 
			// allocated. Callers of this API (halsettings - to save modifiable 
			// atrributes) need to avoid this resource overhead. Clients of 
			// this attribute need to use HAL::Get() directly.
			// HAL should not be used for handle opening and this
			// attribute should be replaced with a better API.
			if ((properties & HAL::EValid) && (i != EDisplayMemoryHandle))
				{
				THalImplementation f=HalInternal::Implementation[i];
				if (f)
					{
					// Initialise the value before getting it, for consistancy
					// when functions take an argument. (-1 is also likely to be
					// an invalid argument and return an error in these cases, which
					// is probably the safest result.)
					pE[offset].iValue = -1;
					r=(*f)(device,i,EFalse,&pE[offset].iValue);
					if (r==KErrNone)
						{
						pE[offset].iProperties=EEntryValid|EEntryDynamic;
						continue;
						}
					// drop through to clear EEntryValid
					}
				else
					{
					//these attributes do not support multiple devices
					if(device==0)
						{
						TInt p;
						if (!(properties & HAL::ESettable))
							{
							pE[offset].iValue = HalInternal::InitialValue[i];
							p=EEntryValid;
							}
						else
							{
							r=RProperty::Get(KUidSystemCategory, KUidHalPropertyKeyBase+i, pE[offset].iValue);
							if (r!=KErrNone)
								{
								HalInternal::InitialiseData();
								r=RProperty::Get(KUidSystemCategory, KUidHalPropertyKeyBase+i, pE[offset].iValue);
								}
							p=(r==KErrNone?EEntryValid:0);
							p|=EEntryDynamic;
							}
						pE[offset].iProperties=p;
						continue;
						}
					// drop through to clear EEntryValid
					}
				}
			pE[offset].iProperties=0;
			pE[offset].iValue=0;
			}
		}
	aNumEntries=max_devices*(TInt)ENumHalAttributes;
	aData=pE;

	return KErrNone;
	}


EXPORT_C TInt HAL::Get(TInt aDeviceNumber, HAL::TAttribute aAttribute, TInt& aValue)
	{

	if (TUint(aAttribute)>=TUint(ENumHalAttributes))
		return KErrNotSupported;
	TUint8 properties=HalInternal::Properties[aAttribute];
	if (!(properties & HAL::EValid))
		return KErrNotSupported;
	THalImplementation f=HalInternal::Implementation[aAttribute];
	if (f)
		return (*f)(aDeviceNumber,aAttribute,EFalse,&aValue);
	if (!(properties & HAL::ESettable))
	{
		aValue=HalInternal::InitialValue[aAttribute];
		return KErrNone;
	}
	aValue=HalInternal::ReadWord(aAttribute);
	return KErrNone;
	}


EXPORT_C TInt HAL::Set(TInt aDeviceNumber, HAL::TAttribute aAttribute, TInt aValue)
	{

     if (TUint(aAttribute)>=TUint(ENumHalAttributes))
		return KErrNotSupported;

	TUint8 properties=HalInternal::Properties[aAttribute];
	if (!(properties & HAL::EValid) || !(properties & HAL::ESettable))
		return KErrNotSupported;
	THalImplementation f=HalInternal::Implementation[aAttribute];
	if (f)
		return (*f)(aDeviceNumber,aAttribute,ETrue,(TAny*)aValue);
	return HalInternal::WriteWord(aAttribute,aValue);
	}

