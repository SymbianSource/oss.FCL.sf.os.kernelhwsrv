// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\d_dobject.h
// 
//

#ifndef __D_DOBJECT_H__
#define __D_DOBJECT_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif


struct SParam
	{
	TInt duration;
	TUint iSeed[2];
	TBool iPerformanceTest; 
	};

class RTestDObject : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ERObjectIxTest1,
		ERObjectIxTest2,
		ERObjectIxTest3,
		ERObjectIxTest4,
		ERObjectIxThreadTestCreateIx,
		ERObjectIxThreadTestExerciseIx,
		ERObjectIxThreadTestFreeIx,
		ERObjectIxInvalidHandleLookupTest,
		EDObjectNameTest,
		};
public:
	inline TInt Open();

public:
	inline TInt RObjectIxTest1(TInt& aTime);
	inline TInt RObjectIxTest2(TInt& aTime);
	inline TInt RObjectIxTest3(SParam& aParam);
	inline TInt RObjectIxTest4(TInt& aTime);
	inline TInt RObjectIxThreadTestCreateIx(void*& aRObjectIxPtr);
	inline TInt RObjectIxThreadTestExerciseIx(void* aRObjectIxPtr);
	inline TInt RObjectIxThreadTestFreeIx(void* aRObjectIxPtr);
	inline TInt InvalidHandleLookupTest();
	inline TInt DObjectNameTest();
	};

_LIT(KDObjectTestLddName,"D_DOBJECT");


#ifndef __KERNEL_MODE__
inline TInt RTestDObject::Open()
	{ return DoCreate(KDObjectTestLddName,TVersion(),KNullUnit,NULL,NULL); }

inline TInt RTestDObject::RObjectIxTest1(TInt& aTime)
	{
	TRequestStatus aStatus;
	DoRequest(ERObjectIxTest1,aStatus, &aTime);
	User::WaitForRequest(aStatus);
	return aStatus.Int();
	}

inline TInt RTestDObject::RObjectIxTest2(TInt& aTime)
	{
	TRequestStatus aStatus;
	DoRequest(ERObjectIxTest2,aStatus, &aTime);
	User::WaitForRequest(aStatus);
	return aStatus.Int();
	}

inline TInt RTestDObject::RObjectIxTest3(SParam& aParam)
	{
	TRequestStatus aStatus;
	DoRequest(ERObjectIxTest3,aStatus, &aParam);
	User::WaitForRequest(aStatus);
	return aStatus.Int();
	}
	
inline TInt RTestDObject::RObjectIxTest4(TInt& aTime)
	{
	TRequestStatus aStatus;
	DoRequest(ERObjectIxTest4,aStatus, &aTime);
	User::WaitForRequest(aStatus);
	return aStatus.Int();
	} // RTestDObject::RObjectIxTest4

inline TInt RTestDObject::RObjectIxThreadTestCreateIx(void*& aRObjectIxPtr)
	{
	TRequestStatus aStatus;
	DoRequest(ERObjectIxThreadTestCreateIx,aStatus, &aRObjectIxPtr);
	User::WaitForRequest(aStatus);
	return aStatus.Int();
	} // RTestDObject::RObjectIxThreadTestCreateIx

inline TInt RTestDObject::RObjectIxThreadTestExerciseIx(void* aRObjectIxPtr)
	{
	TRequestStatus aStatus;
	DoRequest(ERObjectIxThreadTestExerciseIx, aStatus, aRObjectIxPtr);
	User::WaitForRequest(aStatus);
	return aStatus.Int();
	} // RTestDObject::RObjectIxThreadTestExerciseIx

inline TInt RTestDObject::RObjectIxThreadTestFreeIx(void* aRObjectIxPtr)
	{
	TRequestStatus aStatus;
	DoRequest(ERObjectIxThreadTestFreeIx,aStatus, aRObjectIxPtr);
	User::WaitForRequest(aStatus);
	return aStatus.Int();
	} // RTestDObject::RObjectIxThreadTestFreeIx

inline TInt RTestDObject::InvalidHandleLookupTest()
	{
	TRequestStatus aStatus;
	DoRequest(ERObjectIxInvalidHandleLookupTest, aStatus);
	User::WaitForRequest(aStatus);
	return aStatus.Int();
	} // RTestDObject::InvalidHandleLookupTest

inline TInt RTestDObject::DObjectNameTest(void)
	{
	TRequestStatus aStatus;
	DoRequest(EDObjectNameTest, aStatus);
	User::WaitForRequest(aStatus);
	return aStatus.Int();
	}
#endif

#endif

