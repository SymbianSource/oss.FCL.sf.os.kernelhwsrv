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
// t_prop_define.cpp.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>

#include "t_property.h"

TSecureId MySecureId;
TBool IHaveWriteDeviceData;

_LIT(KSecurityOwnerName, "RProperty Security: Owner Basics");

class CPropSecurityOwner : public CTestProgram
	{
public:
	CPropSecurityOwner(TUid aCategory, TUint aMasterKey, TUint aSlaveKey, RProperty::TType	aType) :
		CTestProgram(KSecurityOwnerName), iCategory(aCategory), iMasterKey(aMasterKey), iSlaveKey(aSlaveKey), 
			iType(aType)
		{
		}

	void Run(TUint aCount);

private:
	TUid				iCategory;
	TUint				iMasterKey;
	TUint				iSlaveKey;
	RProperty::TType	iType;
	};

void CPropSecurityOwner::Run(TUint aCount)
	{
	for (TUint i = 0; i < aCount; ++i)
		{
		// Delete() can only be called by the property owner, as defined by the process Security ID,
		// any other process will get a KErrPermissionDenied error.
		RProperty	mProp;
		TInt r = mProp.Delete(iCategory, iMasterKey);
		TF_ERROR(r, r == KErrPermissionDenied);
		}
	}



_LIT(KPropDefineSecurityName, "RProperty Security: Define category security");

class CPropDefineSecurity : public CTestProgram
	{
public:
	CPropDefineSecurity(TUid aCategory, TUint aMasterKey, TUint aSlaveKey, RProperty::TType	aType) :
		CTestProgram(KPropDefineSecurityName), iCategory(aCategory), iMasterKey(aMasterKey), iSlaveKey(aSlaveKey), 
			iType(aType)
		{
		}

	void Run(TUint aCount);

private:
	TUid				iCategory;
	TUint				iMasterKey;
	TUint				iSlaveKey;
	RProperty::TType	iType;
	};

void CPropDefineSecurity::Run(TUint aCount)
	{
	for (TUint i = 0; i < aCount; ++i)
		{
		// Test defining in the system category
		RProperty	prop;
		TInt r = prop.Define(KUidSystemCategory, iSlaveKey, iType, KPassPolicy, KPassPolicy);
		RDebug::Printf("CPropDefineSecurity define with System Category returns %d",r);
		TF_ERROR(r, r == IHaveWriteDeviceData ? KErrNone : KErrPermissionDenied);
		if(r==KErrNone)
			{
			r = prop.Delete(KUidSystemCategory, iSlaveKey);
			TF_ERROR(r, r == KErrNone);
			}

		// Tesk defining properties with categories above and below security threshold
		TUid categoryA = { KUidSecurityThresholdCategoryValue-1 };
		r = prop.Define(categoryA, iSlaveKey, iType, KPassPolicy, KPassPolicy);
		RDebug::Printf("CPropDefineSecurity define with Category A returns %d",r);
		TF_ERROR(r, r == (categoryA==MySecureId || IHaveWriteDeviceData) ? KErrNone : KErrPermissionDenied);
		if(r==KErrNone)
			{
			r = prop.Delete(categoryA, iSlaveKey);
			TF_ERROR(r, r == KErrNone);
			}
		TUid categoryB = { KUidSecurityThresholdCategoryValue };
		r = prop.Define(categoryB, iSlaveKey, iType, KPassPolicy, KPassPolicy);
		RDebug::Printf("CPropDefineSecurity define with Category B returns %d",r);
		TF_ERROR(r, r == (categoryB==MySecureId) ? KErrNone : KErrPermissionDenied);
		if(r==KErrNone)
			{
			r = prop.Delete(categoryB, iSlaveKey);
			TF_ERROR(r, r == KErrNone);
			}
		TUid categoryC = { KUidSecurityThresholdCategoryValue+1 };
		r = prop.Define(categoryC, iSlaveKey, iType, KPassPolicy, KPassPolicy);
		RDebug::Printf("CPropDefineSecurity define with Category C returns %d",r);
		TF_ERROR(r, r == KErrPermissionDenied);

		}
	}




GLDEF_C TInt E32Main()
	{
	TSecurityInfo info;
	info.Set(RProcess());
	MySecureId = info.iSecureId;
	IHaveWriteDeviceData = info.iCaps.HasCapability(ECapabilityWriteDeviceData);

	TInt len = User::CommandLineLength();
	__ASSERT_ALWAYS(len, User::Panic(_L("t_prop_sec: bad args"), 0));

	// Get arguments for the command line
	TInt size = len * sizeof(TUint16);
	HBufC8* hb = HBufC8::NewMax(size);
	__ASSERT_ALWAYS(hb, User::Panic(_L("t_prop_sec: no memory"), 0));
	TPtr cmd((TUint16*) hb->Ptr(), len);
	User::CommandLine(cmd);
	CPropSecurity::TArgs* args = (CPropSecurity::TArgs*) hb->Ptr();	

	CTestProgram::Start();

	CTestProgram* progs[] = 
		{ 
		new CPropSecurityOwner(args->iCategory, args->iMasterKey, args->iSlaveKeySlot, RProperty::EInt),
		new CPropSecurityOwner(args->iCategory, args->iMasterKey, args->iSlaveKeySlot + 1, RProperty::EByteArray),
		new CPropDefineSecurity(args->iCategory, args->iMasterKey, args->iSlaveKeySlot, RProperty::EInt),
		NULL 
		};

	TInt i;
	TInt n = (sizeof(progs)/sizeof(*progs)) - 1;
	for (i = 0; i < n; ++i)
		{
		__ASSERT_ALWAYS(progs[i], User::Panic(_L("t_property: no memory"), 0));
		}
	
	CTestProgram::LaunchGroup(progs, 2);

	for (i = 0; i < n; ++i)
		{
		delete progs[i];
		}

	CTestProgram::End();

	return KErrNone;
	}
