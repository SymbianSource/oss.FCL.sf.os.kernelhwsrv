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

#include <e32kpan.h>
#include "t_property.h"

_LIT(KDefineName, "RProperty::Define() Basics");
 
CPropDefine::CPropDefine(TUid aCategory, TUint aKey, RProperty::TType aType) : 
	  CTestProgram(KDefineName), iCategory(aCategory), iKey(aKey), iType(aType)
	{
	}

void CPropDefine::Run(TUint aCount)
	{
	TUid mySid;
	mySid.iUid = RProcess().SecureId();

	for(TUint i = 0; i < aCount; ++i)
		{
		RProperty prop;

		//	Defines the attributes and access control for a property. This can only be done 
		//	once for each property. Subsequent attempts to define the same property will return
		//	KErrAlreadyExists.
		TInt r = prop.Define(iCategory, iKey, iType, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrNone);
		r = prop.Define(iCategory, iKey, iType, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrAlreadyExists);
		r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);

		// Test defining properties in the default category (==our SID)
		r = prop.Define(iKey, iType, KFailPolicy, KFailPolicy);
		TF_ERROR(r, r == KErrNone);
		r = prop.Define(iKey, iType, KFailPolicy, KFailPolicy);
		TF_ERROR(r, r == KErrAlreadyExists);
		r = prop.Define(mySid, iKey, iType, KFailPolicy, KFailPolicy);
		TF_ERROR(r, r == KErrAlreadyExists);
		r = prop.Delete(mySid, iKey);
		TF_ERROR(r, r == KErrNone);

		// Test deprecated method without policies
		r = prop.Define(iCategory, iKey, iType);
		TF_ERROR(r, r == KErrNone);
		r = prop.Define(iCategory, iKey, iType);
		TF_ERROR(r, r == KErrAlreadyExists);
		r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);

		// Test re-definition doesn't change security settings
		// Defect DEF050961 - Re-defining an RProperty causes the security policy to be overwritten
		{
		TInt expectedResult = PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement)?KErrPermissionDenied:KErrNone;
		_LIT(KTestBytes,"abcd");
		r = prop.Define(iCategory, iKey, iType, KFailPolicy, KFailPolicy);
		TF_ERROR(r, r == KErrNone);
		r = prop.Attach(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);
		if (iType == RProperty::EInt)
			r = prop.Set(1);
		else
			r = prop.Set(KTestBytes);
		TF_ERROR(r, r == expectedResult);
		r = prop.Define(iCategory, iKey, iType, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrAlreadyExists);
		if (iType == RProperty::EInt)
			r = prop.Set(1);
		else
			r = prop.Set(KTestBytes);
		TF_ERROR(r, r == expectedResult);
		r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);
		prop.Close();
		}

		// Define fails with KErrArgument if wrong type or attribute was specified.
		r = prop.Define(iCategory, iKey, RProperty::ETypeLimit, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrArgument);
		const TInt removed_KPersistent_attribute = 0x100;
		r  = prop.Define(iCategory, iKey, iType | removed_KPersistent_attribute, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrArgument);

		TSecurityPolicy badPolicy;
		*(TInt*)&badPolicy = -1;
		r = prop.Define(iCategory, iKey, iType, badPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrArgument);
		r = prop.Define(iCategory, iKey, iType, KPassPolicy, badPolicy);
		TF_ERROR(r, r == KErrArgument);

		if (iType == RProperty::EInt)
			{
			// Define fails with KErrArgument if aType is TInt and aPreallocate is not 0
			r = prop.Define(iCategory, iKey, RProperty::EInt, KPassPolicy, KPassPolicy, 16);
			TF_ERROR(r, r == KErrArgument);

			// Following defintion the property has a default value, 0 for integer properties
			r = prop.Define(iCategory, iKey, RProperty::EInt, KPassPolicy, KPassPolicy);
			TF_ERROR(r, r == KErrNone);
			TInt value;
			r = prop.Get(iCategory, iKey, value);
			TF_ERROR(r, r == KErrNone);
			TF_ERROR(value, value == 0);
			r = prop.Delete(iCategory, iKey);
			TF_ERROR(r, r == KErrNone);
			}
		else 
			{
			// Defne fails with KErrTooBig if aPeallocate is grater than KMaxPropertySize.
			r = prop.Define(iCategory, iKey, RProperty::EByteArray, KPassPolicy, KPassPolicy, RProperty::KMaxPropertySize);
			TF_ERROR(r, r == KErrNone);
			r = prop.Delete(iCategory, iKey);
			TF_ERROR(r, r == KErrNone);
			r = prop.Define(iCategory, iKey, RProperty::EByteArray, KPassPolicy, KPassPolicy, RProperty::KMaxPropertySize + 1);
			TF_ERROR(r, r == KErrTooBig);

			// Following defintion the property has a default value, zero-length data for byte-array and text 
			// properties. 
			r = prop.Define(iCategory, iKey, RProperty::EByteArray, KPassPolicy, KPassPolicy);
			TF_ERROR(r, r == KErrNone);
			TBuf<16> buf;
			r = prop.Get(iCategory, iKey, buf);
			TF_ERROR(r, r == KErrNone);
			TF_ERROR(buf.Size(), buf.Size() == 0);

			TBuf8<16> buf8;
			r = prop.Get(iCategory, iKey, buf8);
			TF_ERROR(r, r == KErrNone);
			TF_ERROR(buf8.Size(), buf8.Size() == 0);
			r = prop.Delete(iCategory, iKey);
			TF_ERROR(r, r == KErrNone);
			}

		// Pending subscriptions for this property will not be completed until a new value is published.
		r = prop.Attach(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);
		TRequestStatus status;
		prop.Subscribe(status);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		r = prop.Define(iCategory, iKey, iType, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrNone);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);
		User::WaitForRequest(status);
		TF_ERROR(status.Int(), status.Int() == KErrNotFound);
		prop.Close();
		}
	}

_LIT(KDeleteName, "RProperty::Delete() Basics");
 
CPropDelete::CPropDelete(TUid aCategory, TUint aKey, RProperty::TType aType) : 
	  CTestProgram(KDeleteName), iCategory(aCategory), iKey(aKey), iType(aType)
	{
	}

void CPropDelete::Run(TUint aCount)
	{
	TUid mySid;
	mySid.iUid = RProcess().SecureId();
	for(TUint i = 0; i < aCount; ++i)
		{
		RProperty prop;

		// If the property has not been defined Delete fails with KErrNotFound.
		TInt r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNotFound);
	
		// Test deleting properties in the default category (==our SID)
		//deleting of property in the default category (==our SID) should fail until the property is defined
		r = prop.Delete(iKey);
		TF_ERROR(r, r == KErrNotFound);
		
		r = prop.Define(iKey, iType, KFailPolicy, KFailPolicy);
		TF_ERROR(r, r == KErrNone);
		r = prop.Delete(iKey);
		TF_ERROR(r, r == KErrNone);
				
		r = prop.Define(mySid, iKey, iType, KFailPolicy, KFailPolicy);
		TF_ERROR(r, r == KErrNone);
		r = prop.Delete(mySid, iKey);
		TF_ERROR(r, r == KErrNone);
		r = prop.Delete( iKey);
		TF_ERROR(r, r == KErrNotFound);
		
		r = prop.Define(mySid, iKey, iType, KFailPolicy, KFailPolicy);
		TF_ERROR(r, r == KErrNone);
		r = prop.Delete( iKey);
		TF_ERROR(r, r == KErrNone);
		r = prop.Delete(mySid, iKey);
		TF_ERROR(r, r == KErrNotFound);
	
		// Any pending subscriptions for this property will be completed with KErrNotFound.
		r = prop.Define(iCategory, iKey, iType, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrNone);
		r = prop.Attach(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);
		TRequestStatus status;
		prop.Subscribe(status);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);
		User::WaitForRequest(status);
		TF_ERROR(status.Int(), status.Int() == KErrNotFound);

		// Any new request will not complete until the property is defined and published again.
		prop.Subscribe(status);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		r = prop.Define(iCategory, iKey, iType, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrNone);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		if (iType == RProperty::EInt)
			{
			r = prop.Set(1);
			TF_ERROR(r, r == KErrNone);
			}
		else
			{
			r = prop.Set(_L("Foo"));
			TF_ERROR(r, r == KErrNone);
			}
		User::WaitForRequest(status);
		TF_ERROR(status.Int(), status.Int() == KErrNone);
		r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);
		prop.Close();
		}
	}

_LIT(KPanicName, "RProperty Panics");

CPropPanic::CPropPanic(TUid aCategory, TUint aKey) : 
	  CTestProgram(KPanicName), iCategory(aCategory), iKey(aKey)
	{
	}

TInt CPropPanic::DoubleSubscribeThreadEntry(TAny* ptr)
	{
	CPropPanic* prog = (CPropPanic*) ptr;
	RProperty prop;
	TInt r = prop.Attach(prog->iCategory, prog->iKey, EOwnerThread);
	TF_ERROR_PROG(prog, r, r == KErrNone);
	TRequestStatus status;
	prop.Subscribe(status);
	// Next statement shall Panic.	
	prop.Subscribe(status);
	// Never get here
	return KErrNone;
	}

TInt CPropPanic::BadHandleSubscribeThreadEntry(TAny* /*ptr*/)
	{
	RProperty prop;
	TRequestStatus status;
	prop.Subscribe(status);
	return KErrNone;
	}

TInt CPropPanic::BadHandleCancelThreadEntry(TAny* /*ptr*/)
	{
	RProperty prop;
	prop.Cancel();
	return KErrNone;
	}

TInt CPropPanic::BadHandleGetIThreadEntry(TAny* /*ptr*/)
	{
	RProperty prop;
	TInt i;
	prop.Get(i);
	return KErrNone;
	}

TInt CPropPanic::BadHandleGetBThreadEntry(TAny* /*ptr*/)
	{
	RProperty prop;
	TBuf<64> buf;
	prop.Get(buf);
	return KErrNone;
	}

TInt CPropPanic::BadHandleSetIThreadEntry(TAny* /*ptr*/)
	{
	RProperty prop;
	TInt i = 1;
	prop.Set(i);
	return KErrNone;
	}

TInt CPropPanic::BadHandleSetBThreadEntry(TAny* /*ptr*/)
	{
	RProperty prop;
	TBuf<64> buf;
	prop.Set(buf);
	return KErrNone;
	}

TThreadFunction CPropPanic::BadHandles[] = {
	CPropPanic::BadHandleSubscribeThreadEntry,
	CPropPanic::BadHandleCancelThreadEntry,
	CPropPanic::BadHandleGetIThreadEntry,
	CPropPanic::BadHandleGetBThreadEntry,
	CPropPanic::BadHandleSetIThreadEntry,
	CPropPanic::BadHandleSetBThreadEntry,
	NULL
};

void CPropPanic::Run(TUint /* aCount */)
	{
	// Only one subscriptoin per RProperty object is allowed, the caller will be paniced if
	// there is already a subscription on this object.
	TRequestStatus status;
	TExitType exit;
	RThread thr;
	TInt r = thr.Create(KNullDesC, DoubleSubscribeThreadEntry, 0x2000, NULL, this);
	TF_ERROR(r, r == KErrNone);
	thr.Logon(status);

	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);

	thr.Resume();
	User::WaitForRequest(status);
	thr.Close();

	User::SetJustInTime(jit);

	TF_ERROR(status.Int(), status.Int() == ERequestAlreadyPending);	

	for (TInt i = 0; BadHandles[i]; ++i)
		{
		r = thr.Create(KNullDesC, BadHandles[i], 0x2000, NULL, this);
		TF_ERROR(r, r == KErrNone);
		thr.Logon(status);

		jit = User::JustInTime();
		User::SetJustInTime(EFalse);

		thr.Resume();
		User::WaitForRequest(status);
		exit = thr.ExitType();
		thr.Close();

		User::SetJustInTime(jit);

		TF_ERROR(status.Int(), status.Int() == EBadHandle);	
		TF_ERROR(exit, exit == EExitPanic);
		}
	}

_LIT(KSetGetName, "RProperty::Set()/Get() Basics");

CPropSetGet::CPropSetGet(TUid aCategory, TUint aKey, RProperty::TType aType) : 
	  CTestProgram(KSetGetName), iCategory(aCategory), iKey(aKey), iType(aType)
	{
	}

void CPropSetGet::Run(TUint aCount)
	{
	for(TUint i = 0; i < aCount; ++i)
		{
		TInt r;
		RProperty prop;

		r = prop.Attach(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);

		// If the property has not been defined this fails with KErrNotFound.
			{
			TInt value;
			TBuf<16> buf;
			TBuf8<16> buf8;
			if (iType == RProperty::EInt)
				{
				r = prop.Get(iCategory, iKey, value);
				TF_ERROR(r, r == KErrNotFound);
				r = prop.Set(iCategory, iKey, value);
				TF_ERROR(r, r == KErrNotFound);
				}
			else
				{
				r = prop.Get(iCategory, iKey, buf);
				TF_ERROR(r, r == KErrNotFound);
				r = prop.Set(iCategory, iKey, buf);
				TF_ERROR(r, r == KErrNotFound);
				r = prop.Get(iCategory, iKey, buf8);
				TF_ERROR(r, r == KErrNotFound);
				r = prop.Set(iCategory, iKey, buf8);
				TF_ERROR(r, r == KErrNotFound);
				}

			if (iType == RProperty::EInt)
				{
				r = prop.Get(value);
				TF_ERROR(r, r == KErrNotFound);
				r = prop.Set(value);
				TF_ERROR(r, r == KErrNotFound);
				}
			else
				{
				r = prop.Get(buf);
				TF_ERROR(r, r == KErrNotFound);
				r = prop.Set(buf);
				TF_ERROR(r, r == KErrNotFound);
				r = prop.Get(buf8);
				TF_ERROR(r, r == KErrNotFound);
				r = prop.Set(buf8);
				TF_ERROR(r, r == KErrNotFound);
				}
			}

		r = prop.Define(iCategory, iKey, iType, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrNone);

		// Can set property to zero length
			{
			if (iType ==  RProperty::EByteArray)
				{
				TBuf8<20> buf8(20);
				r = prop.Set(iCategory, iKey, KNullDesC8);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(iCategory, iKey, buf8);
				TF_ERROR(r, r == KErrNone);
				TF_ERROR(buf8.Length(), buf8.Length() == 0);
				}
			}

		// If the property is larger than KMaxPropertySize this fails with KErrTooBig
			{
			if (iType ==  RProperty::EByteArray)
				{
				TBuf<RProperty::KMaxPropertySize/2 + 1> buf(RProperty::KMaxPropertySize/2 + 1);
				TBuf8<RProperty::KMaxPropertySize + 1> buf8(RProperty::KMaxPropertySize + 1);
				r = prop.Set(iCategory, iKey, buf);
				TF_ERROR(r, r == KErrTooBig);
				r = prop.Set(iCategory, iKey, buf8);
				TF_ERROR(r, r == KErrTooBig);
				r = prop.Set(buf);
				TF_ERROR(r, r == KErrTooBig);
				r = prop.Set(buf8);
				TF_ERROR(r, r == KErrTooBig);
				}
			}

		// When type of operation mismatch with the property type this fails with KErrArgument.
			{
			TInt value;
			TBuf<16> buf;
			TBuf8<16> buf8;
			if (iType !=  RProperty::EInt)
				{
				r = prop.Get(iCategory, iKey, value);
				TF_ERROR(r, r == KErrArgument);
				r = prop.Set(iCategory, iKey, value);
				TF_ERROR(r, r == KErrArgument);
				r = prop.Get(value);
				TF_ERROR(r, r == KErrArgument);
				r = prop.Set(value);
				TF_ERROR(r, r == KErrArgument);
				}
			else
				{
				r = prop.Get(iCategory, iKey, buf);
				TF_ERROR(r, r == KErrArgument);
				r = prop.Set(iCategory, iKey, buf);
				TF_ERROR(r, r == KErrArgument);
				r = prop.Get(iCategory, iKey, buf8);
				TF_ERROR(r, r == KErrArgument);
				r = prop.Set(iCategory, iKey, buf8);
				TF_ERROR(r, r == KErrArgument);
				r = prop.Get(buf);
				TF_ERROR(r, r == KErrArgument);
				r = prop.Set(buf);
				TF_ERROR(r, r == KErrArgument);
				r = prop.Get(buf8);
				TF_ERROR(r, r == KErrArgument);
				r = prop.Set(buf8);
				TF_ERROR(r, r == KErrArgument);
				}
			}

		// Get/Set
		if (iType == RProperty::EInt)
			{
				{
				r = prop.Set(1);
				TF_ERROR(r, r == KErrNone);
				TInt value = 0;
				r = prop.Get(value);
				TF_ERROR(r, r == KErrNone);
				TF_ERROR(value, value == 1);
				}
				{
				TInt value = 0;
				r = prop.Set(iCategory, iKey, 1);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(iCategory, iKey, value);
				TF_ERROR(r, r == KErrNone);
				TF_ERROR(value, value == 1);
				}
			}
		else 
			{
				{
				TBuf<16> ibuf(_L("Foo"));
				TBuf<16> obuf;
				r = prop.Set(ibuf);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(obuf);
				TF_ERROR(r, r == KErrNone);
				r = obuf.Compare(ibuf);
				TF_ERROR(r, r == 0);
				}
				{
				TBuf8<16> ibuf8((TUint8*)"Foo");
				TBuf8<16> obuf8;
				r = prop.Set(ibuf8);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(obuf8);
				TF_ERROR(r, r == KErrNone);
				r = obuf8.Compare(ibuf8);
				TF_ERROR(r, r == 0);
				}
				{
				TBuf<16> ibuf(_L("Foo"));
				TBuf<16> obuf;
				r = prop.Set(iCategory, iKey, ibuf);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(iCategory, iKey, obuf);
				TF_ERROR(r, r == KErrNone);
				r = obuf.Compare(ibuf);
				TF_ERROR(r, r == 0);
				}
				{
				TBuf8<16> ibuf8((TUint8*)"Foo");
				TBuf8<16> obuf8;
				r = prop.Set(iCategory, iKey, ibuf8);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(iCategory, iKey, obuf8);
				TF_ERROR(r, r == KErrNone);
				r = obuf8.Compare(ibuf8);
				TF_ERROR(r, r == 0);
				}
			}

		// If the supplied buffer is too small this fails with KErrOverflow and the truncated value is reported.
		if (iType == RProperty::EByteArray)
			{
				{
				TBuf<16> ibuf(_L("0123456789012345"));
				TBuf<16> obuf(_L("abcdefghigklmnop"));
				TPtr optr((TUint16*) obuf.Ptr(), 0, 15);
				r = prop.Set(iCategory, iKey, ibuf);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(iCategory, iKey, optr);
				TF_ERROR(r, r == KErrOverflow);
				TF_ERROR(optr.Length(), optr.Length() == 15); 
				TF_ERROR(obuf[14], obuf[14] == TText('4')); 
				TF_ERROR(obuf[15], obuf[15] == TText('p'));
				}
				{
				TBuf8<16> ibuf8((TUint8*) "0123456789012345");
				TBuf8<16> obuf8((TUint8*) "abcdefghigklmnop");
				TPtr8 optr8((TUint8*) obuf8.Ptr(), 0, 15);
				r = prop.Set(iCategory, iKey, ibuf8);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(iCategory, iKey, optr8);
				TF_ERROR(r, r == KErrOverflow);
				TF_ERROR(optr8.Length(), optr8.Length() == 15); 
				TF_ERROR(obuf8[14], obuf8[14] == '4'); 
				TF_ERROR(obuf8[15], obuf8[15] == 'p');
				}
				{
				TBuf<16> ibuf(_L("0123456789012345"));
				TBuf<16> obuf(_L("abcdefghigklmnop"));
				TPtr optr((TUint16*) obuf.Ptr(), 0, 15);
				r = prop.Set(ibuf);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(optr);
				TF_ERROR(r, r == KErrOverflow);
				TF_ERROR(optr.Length(), optr.Length() == 15); 
				TF_ERROR(obuf[14], obuf[14] == TText('4')); 
				TF_ERROR(obuf[15], obuf[15] == TText('p')); 
				}
				{
				TBuf8<16> ibuf8((TUint8*) "0123456789012345");
				TBuf8<16> obuf8((TUint8*) "abcdefghigklmnop");
				TPtr8 optr8((TUint8*) obuf8.Ptr(), 0, 15);
				r = prop.Set(ibuf8);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(optr8);
				TF_ERROR(r, r == KErrOverflow);
				TF_ERROR(optr8.Length(), optr8.Length() == 15); 
				TF_ERROR(obuf8[14], obuf8[14] == '4'); 
				TF_ERROR(obuf8[15], obuf8[15] == 'p');
				}
			}

		// Get/Set zero-length data
		if (iType == RProperty::EByteArray)
			{
				{
				TBuf<16> ibuf(_L("Foo"));
				TBuf<16> obuf;
				TPtr nullbuf(NULL, 0);

				r = prop.Set(ibuf);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(nullbuf);
				TF_ERROR(r, r == KErrOverflow);
				TF_ERROR(nullbuf.Length(), (nullbuf.Length() == 0));

				r = prop.Set(nullbuf);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(obuf);
				TF_ERROR(r, r == KErrNone);
				TF_ERROR(obuf.Length(), (obuf.Length() == 0));
				}
				{
				TBuf8<16> ibuf((TUint8*) "Foo");
				TBuf8<16> obuf;
				TPtr8 nullbuf(NULL, 0);

				r = prop.Set(ibuf);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(nullbuf);
				TF_ERROR(r, r == KErrOverflow);
				TF_ERROR(nullbuf.Length(), (nullbuf.Length() == 0));

				r = prop.Set(nullbuf);
				TF_ERROR(r, r == KErrNone);
				r = prop.Get(obuf);
				TF_ERROR(r, r == KErrNone);
				TF_ERROR(obuf.Length(), (obuf.Length() == 0));
				}
			}

		r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);
		prop.Close();
		}
	}

		
_LIT(KSubsCancelName, "RProperty::Subscribe()/Cancel() Basics");
 
CPropSubsCancel::CPropSubsCancel(TUid aCategory, TUint aKey, RProperty::TType aType) : 
	  CTestProgram(KSubsCancelName), iCategory(aCategory), iKey(aKey), iType(aType)
	{
	}


void CPropSubsCancel::Run(TUint aCount)
	{

	for(TUint i = 0; i < aCount; ++i)
		{
		TRequestStatus status;
		RProperty prop;

		TInt r = prop.Attach(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);

		// The calling thread will have the specified request status signalled when the property is next updated.
		r = prop.Define(iCategory, iKey, iType, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrNone);
		prop.Subscribe(status);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		if (iType == RProperty::EInt)
			{
			r = prop.Set(1);
			TF_ERROR(r, r == KErrNone);
			}
		else
			{
			r = prop.Set(_L("Foo"));
			TF_ERROR(r, r == KErrNone);
			}
		User::WaitForRequest(status);
		TF_ERROR(status.Int(), status.Int() == KErrNone);
		r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);
	
		// If the property has not been defined, the request will not complete until the property
		// is defined and published.
		prop.Subscribe(status);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		r = prop.Define(iCategory, iKey, iType, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrNone);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		if (iType == RProperty::EInt)
			{
			r = prop.Set(1);
			TF_ERROR(r, r == KErrNone);
			}
		else
			{
			r = prop.Set(_L("Foo"));
			TF_ERROR(r, r == KErrNone);
			}
		User::WaitForRequest(status);
		TF_ERROR(status.Int(), status.Int() == KErrNone);
		r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);

		// Cancel an outstanding subscription request for this property handle. 
		// If it has not already completed, the request is completed with KErrCancelled.
		prop.Subscribe(status);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		prop.Cancel();		
		User::WaitForRequest(status);
		TF_ERROR(status.Int(), status.Int() == KErrCancel);

		r = prop.Define(iCategory, iKey, iType, KPassPolicy, KPassPolicy);
		TF_ERROR(r, r == KErrNone);
		prop.Subscribe(status);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		if (iType == RProperty::EInt)
			{
			r = prop.Set(1);
			TF_ERROR(r, r == KErrNone);
			}
		else
			{
			r = prop.Set(_L("Foo"));
			TF_ERROR(r, r == KErrNone);
			}
		User::WaitForRequest(status);
		TF_ERROR(status.Int(), status.Int() == KErrNone);
		prop.Cancel();		
		TF_ERROR(status.Int(), status.Int() == KErrNone);

		prop.Subscribe(status);
		TF_ERROR(status.Int(), status.Int() == KRequestPending);
		prop.Cancel();		
		User::WaitForRequest(status);
		TF_ERROR(status.Int(), status.Int() == KErrCancel);

		r = prop.Delete(iCategory, iKey);
		TF_ERROR(r, r == KErrNone);

		prop.Close();
		}
	}

_LIT(KSecurityName, "RProperty Security Basics (Master)");
 
CPropSecurity::CPropSecurity(TUid aCategory, TUint aMasterKey, RProperty::TType aType, TUint aSlaveKeySlot) : 
		CTestProgram(KSecurityName), iCategory(aCategory), iMasterKey(aMasterKey), 
		iSlaveKeySlot(aSlaveKeySlot), iType(aType)
	{
	}

_LIT(KSecuritySlavePath, "t_prop_sec.exe");

void CPropSecurity::Run(TUint aCount)
	{
	for(TInt i=0; i<ECapability_Limit; i++)
		if(!PlatSec::IsCapabilityEnforced((TCapability)i))
			{
			// System isn't configured with platform security enforced
			// so don't bother running tests
			return;
			}

	TArgs args;
	args.iCount = aCount;
	args.iCategory = iCategory;
	args.iMasterKey = iMasterKey;
	args.iSlaveKeySlot = iSlaveKeySlot;

	RProperty prop;

	TInt r = prop.Define(iCategory, iMasterKey, iType, KPassPolicy, KPassPolicy);
	TF_ERROR(r, r == KErrNone);

	Exec(KSecuritySlavePath, &args, sizeof(args));

	Exec(_L("t_prop_define0.exe"), &args, sizeof(args));
	Exec(_L("t_prop_define1.exe"), &args, sizeof(args));
	Exec(_L("t_prop_define2.exe"), &args, sizeof(args));
	Exec(_L("t_prop_define3.exe"), &args, sizeof(args));

	r = prop.Delete(iCategory, iMasterKey);
	TF_ERROR(r, r == KErrNone);
	}
