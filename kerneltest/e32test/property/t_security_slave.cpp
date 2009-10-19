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
// Description:
//

#include <e32test.h>
#include "t_property.h"

TSecureId TestSecureId;
TVendorId TestVendorId;
TCapability TestCaps[ECapability_Limit];
TCapability TestMissingCaps[ECapability_Limit];

TSecurityPolicy CorrectCapPolicy;
TSecurityPolicy IncorrectCapPolicy;

TUint TestPolicyCount=0;

#include <e32svr.h>

TBool MakeTestSecurityPolicy()
	{
	TCapability* c =TestCaps;
	TCapability* m =TestMissingCaps;
	TUint t=++TestPolicyCount;
	RDebug::Print(_L("Test policy %d\n"),t);
	if(t==1)
		{
		_LIT_SECURITY_POLICY_PASS(KSecurityPolicyPass);
		CorrectCapPolicy = KSecurityPolicyPass;
		_LIT_SECURITY_POLICY_FAIL(KSecurityPolicyFail);
		IncorrectCapPolicy = KSecurityPolicyFail;
		return ETrue;
		}
	t -= 1;
	if(t<7)
		{
		switch(t)
			{
		case 0:
			CorrectCapPolicy = TSecurityPolicy(c[0]);
			IncorrectCapPolicy = TSecurityPolicy(m[0]);
			break;
		case 1:
			CorrectCapPolicy = TSecurityPolicy(c[0],c[1]);
			IncorrectCapPolicy = TSecurityPolicy(m[0],c[1]);
			break;
		case 2:
			CorrectCapPolicy = TSecurityPolicy(c[0],c[1],c[2]);
			IncorrectCapPolicy = TSecurityPolicy(c[0],m[1],c[2]);
			break;
		case 3:
			CorrectCapPolicy = TSecurityPolicy(c[0],c[1],c[2],c[3]);
			IncorrectCapPolicy = TSecurityPolicy(c[0],c[1],m[2],c[3]);
			break;
		case 4:
			CorrectCapPolicy = TSecurityPolicy(c[0],c[1],c[2],c[3],c[4]);
			IncorrectCapPolicy = TSecurityPolicy(c[0],c[1],c[2],m[3],c[4]);
			break;
		case 5:
			CorrectCapPolicy = TSecurityPolicy(c[0],c[1],c[2],c[3],c[4],c[5]);
			IncorrectCapPolicy = TSecurityPolicy(c[0],c[1],c[2],c[3],m[4],c[5]);
			break;
		case 6:
			CorrectCapPolicy = TSecurityPolicy(c[0],c[1],c[2],c[3],c[4],c[5],c[6]);
			IncorrectCapPolicy = TSecurityPolicy(c[0],c[1],c[2],c[3],c[4],m[5],c[6]);
			break;
			}
		return ETrue;
		}
	t -= 7;
	if(t<4)
		{
		TSecureId id = TestSecureId;
		TSecureId id2 = TSecureId((TUint32)TestVendorId);
		switch(t)
			{
		case 0:
			CorrectCapPolicy = TSecurityPolicy(id);
			IncorrectCapPolicy = TSecurityPolicy(id2);
			break;
		case 1:
			CorrectCapPolicy = TSecurityPolicy(id,c[0]);
			IncorrectCapPolicy = TSecurityPolicy(id2,c[0]);
			break;
		case 2:
			CorrectCapPolicy = TSecurityPolicy(id,c[0],c[1]);
			IncorrectCapPolicy = TSecurityPolicy(id2,c[0],c[1]);
			break;
		case 3:
			CorrectCapPolicy = TSecurityPolicy(id,c[0],c[1],c[2]);
			IncorrectCapPolicy = TSecurityPolicy(id2,c[0],c[1],c[2]);
			break;
			}
		return ETrue;
		}
	t -= 4;
	if(t<4)
		{
		TVendorId id = TestVendorId;
		TVendorId id2 = TVendorId((TUint32)TestSecureId);
		switch(t)
			{
		case 0:
			CorrectCapPolicy = TSecurityPolicy(id);
			IncorrectCapPolicy = TSecurityPolicy(id2);
			break;
		case 1:
			CorrectCapPolicy = TSecurityPolicy(id,c[0]);
			IncorrectCapPolicy = TSecurityPolicy(id2,c[0]);
			break;
		case 2:
			CorrectCapPolicy = TSecurityPolicy(id,c[0],c[1]);
			IncorrectCapPolicy = TSecurityPolicy(id2,c[0],c[1]);
			break;
		case 3:
			CorrectCapPolicy = TSecurityPolicy(id,c[0],c[1],c[2]);
			IncorrectCapPolicy = TSecurityPolicy(id2,c[0],c[1],c[2]);
			break;
			}
		return ETrue;
		}
	t -= 4;
	TestPolicyCount = 0;
	return EFalse;
	}

_LIT(KSecurityReadCapabilityName, "RProperty Security: Read Capability Basics");

class CPropSecurityReadCapability : public CTestProgram
	{
public:
	CPropSecurityReadCapability(TUid aCategory, TUint aKey1, TUint aKey2, RProperty::TType aType) :
		CTestProgram(KSecurityReadCapabilityName), iCategory(aCategory), iKey1(aKey1), iKey2(aKey2), iType(aType)
		{
		}

	void Run(TUint aCount);

private:
	TUid				iCategory;
	TUint				iKey1;
	TUint				iKey2;
	RProperty::TType	iType;
	};

void CPropSecurityReadCapability::Run(TUint aCount)
	{
	while(MakeTestSecurityPolicy())
	for (TUint i = 0; i < aCount; ++i)
		{
		RProperty	prop1;
		RProperty	prop2;

		TInt r = prop1.Attach(iCategory, iKey1);
		TF_ERROR(r, r == KErrNone);
		r = prop2.Attach(iCategory, iKey2);
		TF_ERROR(r, r == KErrNone);

		// If the property has not been defined, the request will not complete until the property
		// is defined and published.
		// When defined if the caller does not have read capabilities the request completes
		// with KErrPermissionDenied.
		TRequestStatus status1;
		prop1.Subscribe(status1);
		TF_ERROR(status1.Int(), status1.Int() == KRequestPending);
		r = prop1.Define(iCategory, iKey1, iType, IncorrectCapPolicy, CorrectCapPolicy);
		TF_ERROR(r, r == KErrNone);
		User::WaitForRequest(status1);
		TF_ERROR(status1.Int(), status1.Int() == KErrPermissionDenied);

		TRequestStatus status2;
		prop2.Subscribe(status2);
		TF_ERROR(status2.Int(), status2.Int() == KRequestPending);
		r = prop2.Define(iCategory, iKey2, iType, CorrectCapPolicy, IncorrectCapPolicy);
		TF_ERROR(r, r == KErrNone);
		TF_ERROR(status2.Int(), status2.Int() == KRequestPending);
		prop2.Cancel();
		User::WaitForRequest(status2);
		TF_ERROR(status2.Int(), status2.Int() == KErrCancel);

		// If the Subscribe() caller does not have the read capabilities the request completes immediately
		// with KErrPermissionDenied.
		prop1.Subscribe(status1);
		User::WaitForRequest(status1);
		TF_ERROR(status1.Int(), status1.Int() == KErrPermissionDenied);

		prop2.Subscribe(status2);
		TF_ERROR(status2.Int(), status2.Int() == KRequestPending);
		prop2.Cancel();
		User::WaitForRequest(status2);
		TF_ERROR(status2.Int(), status2.Int() == KErrCancel);

		// If the caller does not have the read capabilities Get() fails with KErrPermissionDenied.
		if (iType == RProperty::EInt)
			{
			TInt value;
			r = prop1.Get(iCategory, iKey1, value);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Get(iCategory, iKey2, value);
			TF_ERROR(r, r == KErrNone);
			r = prop1.Get(value);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Get(value);
			TF_ERROR(r, r == KErrNone);
			}
		else 
			{
			TBuf<16> buf;
			TBuf8<16> buf8;
			r = prop1.Get(iCategory, iKey1, buf);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Get(iCategory, iKey2, buf);
			TF_ERROR(r, r == KErrNone);
			r = prop1.Get(iCategory, iKey1, buf8);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Get(iCategory, iKey2, buf8);
			TF_ERROR(r, r == KErrNone);
			r = prop1.Get(buf);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Get(buf);
			TF_ERROR(r, r == KErrNone);
			r = prop1.Get(buf8);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Get(buf8);
			TF_ERROR(r, r == KErrNone);
			}

		r = prop1.Delete(iCategory, iKey1);
		TF_ERROR(r, r == KErrNone);
		r = prop2.Delete(iCategory, iKey2);
		TF_ERROR(r, r == KErrNone);
		prop1.Close();
		prop2.Close();
		}
	}

_LIT(KSecurityWriteCapabilityName, "RProperty Security: Write Cpability Basics");

class CPropSecurityWriteCapability : public CTestProgram
	{
public:
	CPropSecurityWriteCapability(TUid aCategory, TUint aKey1, TUint aKey2, RProperty::TType aType) :
		CTestProgram(KSecurityWriteCapabilityName), iCategory(aCategory), iKey1(aKey1), iKey2(aKey2), iType(aType)
		{
		}

	void Run(TUint aCount);

private:
	TUid				iCategory;
	TUint				iKey1;
	TUint				iKey2;
	RProperty::TType	iType;
	};

void CPropSecurityWriteCapability::Run(TUint aCount)
	{
	while(MakeTestSecurityPolicy())
	for (TUint i = 0; i < aCount; ++i)
		{
		RProperty	prop1;
		RProperty	prop2;

		TInt r = prop1.Attach(iCategory, iKey1);
		TF_ERROR(r, r == KErrNone);
		r = prop2.Attach(iCategory, iKey2);
		TF_ERROR(r, r == KErrNone);

		r = prop1.Define(iCategory, iKey1, iType, CorrectCapPolicy, IncorrectCapPolicy);
		TF_ERROR(r, r == KErrNone);
		r = prop2.Define(iCategory, iKey2, iType, IncorrectCapPolicy, CorrectCapPolicy);
		TF_ERROR(r, r == KErrNone);

		// If the caller does not have the write capabilities Set() fails with KErrPermissionDenied.
		if (iType == RProperty::EInt)
			{
			TInt value = 1;
			r = prop1.Set(iCategory, iKey1, value);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Set(iCategory, iKey2, value);
			TF_ERROR(r, r == KErrNone);
			r = prop1.Set(value);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Set(value);
			TF_ERROR(r, r == KErrNone);
			}
		else 
			{
			TBuf<16> buf(_L("Foo"));
			TBuf8<16> buf8((TUint8*) "Foo");
			r = prop1.Set(iCategory, iKey1, buf);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Set(iCategory, iKey2, buf);
			TF_ERROR(r, r == KErrNone);
			r = prop1.Set(iCategory, iKey1, buf8);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Set(iCategory, iKey2, buf8);
			TF_ERROR(r, r == KErrNone);
			r = prop1.Set(buf);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Set(buf);
			TF_ERROR(r, r == KErrNone);
			r = prop1.Set(buf8);
			TF_ERROR(r, r == KErrPermissionDenied);
			r = prop2.Set(buf8);
			TF_ERROR(r, r == KErrNone);
			}
		
		r = prop1.Delete(iCategory, iKey1);
		TF_ERROR(r, r == KErrNone);
		r = prop2.Delete(iCategory, iKey2);
		TF_ERROR(r, r == KErrNone);
		prop1.Close();
		prop2.Close();
		}
	}

GLDEF_C TInt E32Main()
	{

	TSecurityInfo info;
	info.Set(RProcess());
	TestSecureId = info.iSecureId;
	TestVendorId = info.iVendorId;
	{
	TInt c=0;
	TInt m=0;
	for(TInt i=0; i<ECapability_Limit; i++)
		{
		if(info.iCaps.HasCapability((TCapability)i))
			TestCaps[c++] = (TCapability)i;
		else
			TestMissingCaps[m++] = (TCapability)i;
		}
	__ASSERT_ALWAYS(c>=7,User::Panic(_L("not enough caps"), 1));
	__ASSERT_ALWAYS(m>=6,User::Panic(_L("missing caps <6"), 1));
	}
		
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
		new CPropSecurityReadCapability(args->iCategory, args->iSlaveKeySlot + 0, args->iSlaveKeySlot + 1, 
										RProperty::EInt),
		new CPropSecurityReadCapability(args->iCategory, args->iSlaveKeySlot + 2, args->iSlaveKeySlot + 3, 
										RProperty::EByteArray),
		new CPropSecurityWriteCapability(args->iCategory, args->iSlaveKeySlot + 4, args->iSlaveKeySlot + 5, 
										RProperty::EInt),
		new CPropSecurityWriteCapability(args->iCategory, args->iSlaveKeySlot + 6, args->iSlaveKeySlot + 7, 
										RProperty::EByteArray),
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
