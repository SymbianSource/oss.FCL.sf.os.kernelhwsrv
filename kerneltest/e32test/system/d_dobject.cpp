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
// e32test\system\d_dobject.cpp
// LDD for testing RObjectIx class
// 
//

#include <kernel/kern_priv.h>
#include "d_dobject.h"
#include "../misc/prbs.h"


//
// Handle Mutex referenced from ObjectIx.cpp (normally from sglobals.cpp).
//
DMutex* RObjectIx::HandleMutex;

//
// Constants for test sizes and stepings...
//
const TInt KDObjectTestMaxTestSize  = 2000;	// Bigest size to test
const TInt KDObjectTestStepStart    = 19;	// Test all values below this
const TInt KDObjectTestStep         = 31;	// Step value used when above KDObjectTestStepStart.
const TInt KDObjectTestLoopCount    = 3;	// Loop count used in some tests to repeat random testing.


//
// LDD factory
//

class DObjectTestFactory : public DLogicalDevice
	{
public:
	DObjectTestFactory();
	~DObjectTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
public:
	};

//
// Logical Channel
//
const TInt KObjectIndexMask=0x7fff;

class DObjectTest : public DLogicalChannelBase
	{
public:
	virtual ~DObjectTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
public:
	DObjectTestFactory* iFactory;

private:
	TInt RObjectIxTest1(TInt aSize);
	TInt RObjectIxTest2(TInt aSize);
	TInt RObjectIxTest3(TInt aSize, TBool aPerformanceTest);
	TInt RObjectIxTest4(TInt aSize);
	TInt RObjectIxTestExerciseIx(RObjectIx& aObjectIx, TInt aSize);
	TInt RObjectIxInvalidHandleLookupTest(TInt aSize);
	TInt DObjectNameTest();

	inline TInt Index(TInt aHandle)	{return(aHandle&KObjectIndexMask);}

private:
	struct DOBjAndHandle
		{
		DObject* iObject;
		TInt	 iHandle;
		};

	TUint iSeed[2];
	DOBjAndHandle* iObjAndHandle;	
	};

//
// LDD factory
//

DObjectTestFactory::DObjectTestFactory()
	{
	Kern::MutexCreate(RObjectIx::HandleMutex, _L("HandleMutex"), KMutexOrdHandle);
	}

DObjectTestFactory::~DObjectTestFactory()
	{
	delete RObjectIx::HandleMutex;
	RObjectIx::HandleMutex = NULL;
	}

TInt DObjectTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DObjectTest;
	return KErrNone;
	}

TInt DObjectTestFactory::Install()
	{
	return SetName(&KDObjectTestLddName);
	}

void DObjectTestFactory::GetCaps(TDes8& /* aDes */) const
	{
	//aDes.FillZ(aDes.MaxLength());
	}

DECLARE_STANDARD_LDD()
	{
	return new DObjectTestFactory;
	}

//
// Logical Channel
//

TInt DObjectTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	if (NULL == (iObjAndHandle = (DOBjAndHandle*)Kern::Alloc(KDObjectTestMaxTestSize * sizeof(DOBjAndHandle))))
		return KErrNoMemory;
	return KErrNone;
	}

DObjectTest::~DObjectTest()
	{
	delete[] iObjAndHandle;
	}


//Adds a number of DObjects to RObjectIx, then removes them in the same order.
TInt DObjectTest::RObjectIxTest1(TInt aSize)
{
	TInt i,r;
	DObject* object;
	RObjectIx  myIx;

	myIx.Check(0);

	for (i=0; i<aSize; i++) //Add
		{
		if( (iObjAndHandle[i].iObject = new DObject) == NULL) 
			{
			myIx.Close(NULL);
			return KErrNoMemory;
			}
		iObjAndHandle[i].iObject->iContainerID = 0;
		r = myIx.Add(iObjAndHandle[i].iObject, 0);
		if (r<0)
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return r;
			}
		iObjAndHandle[i].iHandle = r;
		}

	myIx.Check(0);
	
	for (i=0; i<aSize; i++)		//Remove
		{
		TUint32  attr = 0; // Receives the attributes of the removed handle...

		if (KErrNone != (r=myIx.Remove(iObjAndHandle[i].iHandle, object, attr))) 
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return r;
			}
		iObjAndHandle[i].iObject->Close(NULL);
		}

	myIx.Check(0);
	myIx.Close(NULL);

	return KErrNone;
}

//Adds a number of DObjects to RObjectIx, then removes them in the reverse order.
TInt DObjectTest::RObjectIxTest2(TInt aSize)
{
	TInt i,r;
	DObject* object;
	RObjectIx myIx;
	
	myIx.Check(0);

	for (i=0; i<aSize; i++) //Add
		{
		if( (iObjAndHandle[i].iObject = new DObject) == NULL) 
			{
			myIx.Close(NULL);
			return KErrNoMemory;
			}
		iObjAndHandle[i].iObject->iContainerID = 0;
		r = myIx.Add(iObjAndHandle[i].iObject, 0);
		if (r<0)
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return r;
			}
		iObjAndHandle[i].iHandle = r;
		}

	myIx.Check(0);

	for (i=aSize-1; i>=0; i--)		//Remove in reverse
		{
		TUint32  attr = 0; // Receives the attributes of the removed handle...

		if (KErrNone != (r=myIx.Remove(iObjAndHandle[i].iHandle, object, attr))) 
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return r;
			}
		iObjAndHandle[i].iObject->Close(NULL);
		}

	myIx.Check(0);
	myIx.Close(NULL);

	return KErrNone;
}

//Adds and removes random number of DObjects to/from RObjectIx.
TInt DObjectTest::RObjectIxTest3(TInt aSize, TBool aPerformanceTest)
{
	TInt index, x, r;
	DObject* object;
	RObjectIx myIx;

	//---Create & init the objects we need
	for (x=0; x<aSize; x++) iObjAndHandle[x].iObject = NULL; //initialize the array

	if (!aPerformanceTest)
		myIx.Check(0);

	for (x = 0;  x < KDObjectTestLoopCount;  x++)
		{
		
		//---Add the random number of objects (in random order)---
		TInt toAdd=Random(iSeed)%(aSize-myIx.ActiveCount()+1);
		while (toAdd--)
			{
			index=Random(iSeed)%aSize;
			while (iObjAndHandle[index].iObject) index=(index+1)%aSize; //Find the next NULL pointer 
			if( (iObjAndHandle[index].iObject = new DObject) == NULL) 
				{
				myIx.Close(NULL);
				return KErrNoMemory;
				}
			r = myIx.Add(iObjAndHandle[index].iObject, 0);
			if (r<0)
				{
				if (!aPerformanceTest)
					myIx.Check(0);
				myIx.Close(NULL);
				return r;
				}
			iObjAndHandle[index].iHandle = r;
			}

		if (!aPerformanceTest)
			myIx.Check(0);

		//---Remove the random number of objects (in random order)---
		TInt toRemove=Random(iSeed)%(myIx.ActiveCount()+1);
		while (toRemove--)
			{
			index=Random(iSeed)%aSize;
			while (!iObjAndHandle[index].iObject) index=(index+1)%aSize; //Find the next non-NULL pointer 

			TUint32  attr = 0; // Receives the attributes of the removed handle...

			if (KErrNone != (r=myIx.Remove(iObjAndHandle[index].iHandle, object, attr)))
				{
				if (!aPerformanceTest)
					myIx.Check(0);
				myIx.Close(NULL);
				return r;
				}
			iObjAndHandle[index].iObject->Close(NULL);
			iObjAndHandle[index].iObject=NULL;
			}

		if(aPerformanceTest) continue;

		myIx.Check(0);

		//---Test data consistency---
		TInt objNum=0;
		for (index=0;index<aSize;index++) 
			{
			if (iObjAndHandle[index].iObject)
				{
				objNum++;
				
				//Test At(TInt aHandle) method
				NKern::LockSystem();
				if (iObjAndHandle[index].iObject != myIx.At(iObjAndHandle[index].iHandle))
					{
					NKern::UnlockSystem();
					myIx.Check(0);
					myIx.Close(NULL);
					return KErrGeneral;
					}
				NKern::UnlockSystem();

				
				//Test Count(CObject* aObject) method
				RObjectIx::Wait();
				if (1!=myIx.Count(iObjAndHandle[index].iObject))
					{
					RObjectIx::Signal();
					myIx.Check(0);
					myIx.Close(NULL);
					return KErrGeneral;
					}
				
				//Test At(CObject* aObject) method
				if (iObjAndHandle[index].iHandle != myIx.At(iObjAndHandle[index].iObject))
					{
					RObjectIx::Signal();
					myIx.Check(0);
					myIx.Close(NULL);
					return KErrGeneral;
					}
				RObjectIx::Signal();

				//Test operator[](TInt index) method
				NKern::LockSystem();
				if (iObjAndHandle[index].iObject != myIx[Index(iObjAndHandle[index].iHandle)])
					{
					NKern::UnlockSystem();
					myIx.Check(0);
					myIx.Close(NULL);
					return KErrGeneral;
					}
				NKern::UnlockSystem();

				}
			}
	
		if (objNum != myIx.ActiveCount())
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return KErrGeneral;
			}
		}

	myIx.Check(0);
	myIx.Close(NULL);

	return KErrNone;
}


//Adds a number of DObjects to RObjectIx using reserved slots, then removes
// them in the same order. Repeat using the reverse order.
TInt DObjectTest::RObjectIxTest4(TInt aSize)
	{
	TInt i,r;
	DObject* object;
	RObjectIx myIx;

	myIx.Check(0);
	myIx.Reserve(aSize);
	myIx.Check(0);
	myIx.Reserve(-aSize);
	myIx.Check(0);
	myIx.Reserve(aSize);
	myIx.Check(0);
	
	for (i=0; i<aSize; i++) //Add
		{
		if( (iObjAndHandle[i].iObject = new DObject) == NULL) 
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return KErrNoMemory;
			}
		iObjAndHandle[i].iObject->iContainerID = 0;
		r = myIx.Add(iObjAndHandle[i].iObject, (TUint32)RObjectIx::EReserved);
		if (r<0)
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return r;
			}
		iObjAndHandle[i].iHandle = r;
		}

	myIx.Check(0);

	TInt toRemove=Random(iSeed)%(myIx.ActiveCount()+1);
	TInt toAdd=toRemove; // will put them all back again...
	while (toRemove--)
		{
		i=Random(iSeed)%aSize;
		while (!iObjAndHandle[i].iObject) i=(i+1)%aSize; //Find the next non-NULL pointer 

		TUint32  attr = 0; // Receives the attributes of the removed handle...

		if (KErrNone != (r=myIx.Remove(iObjAndHandle[i].iHandle, object, attr))) 
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return r;
			}
		if ((attr & RObjectIx::EReserved) == 0) 
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return KErrBadHandle;
			}
		iObjAndHandle[i].iObject->Close(NULL);
		iObjAndHandle[i].iObject = NULL;
		}

	myIx.Check(0);

	while (toAdd--)
		{
		i=Random(iSeed)%aSize;
		while (iObjAndHandle[i].iObject) i=(i+1)%aSize; //Find the next NULL pointer 

		if( (iObjAndHandle[i].iObject = new DObject) == NULL) 
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return KErrNoMemory;
			}
		iObjAndHandle[i].iObject->iContainerID = 0;
		r = myIx.Add(iObjAndHandle[i].iObject, (TUint32)RObjectIx::EReserved);
		if (r<0)
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return r;
			}
		iObjAndHandle[i].iHandle = r;
		}

	myIx.Check(0);

	for (i=aSize-1; i>=0; i--)		//Remove in reverse
		{
		TUint32  attr = 0; // Receives the attributes of the removed handle...

		if (KErrNone != (r=myIx.Remove(iObjAndHandle[i].iHandle, object, attr))) 
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return r;
			}
		if ((attr & RObjectIx::EReserved) == 0) 
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return KErrBadHandle;
			}
		iObjAndHandle[i].iObject->Close(NULL);
		iObjAndHandle[i].iObject = NULL;
		}

	myIx.Check(0);
	myIx.Close(NULL);

	return KErrNone;
	} // DObjectTest::RObjectIxTest4


// Adds a number of DObjects to RObjectIx using both normal and reserved slots, plus
// unused reserved slots then removes
// them in the same order. Repeat using the reverse order. Used in concurrent
// testing with multiple threads.
TInt DObjectTest::RObjectIxTestExerciseIx(RObjectIx& aObjectIx, TInt aSize)
	{
	TInt i,r;
	DObject* object;

	aObjectIx.Check(0);

	for (i=0; i<aSize; i++) //Add and reserve (causing a Grow())...
		{
		// we reserve handles because it encourages grow and shrinking of the pool
		aObjectIx.Reserve(1);

		if( (iObjAndHandle[i].iObject = new DObject) == NULL)
			{
			return KErrNoMemory;
			}
		iObjAndHandle[i].iObject->iContainerID = 0;
		r = aObjectIx.Add(iObjAndHandle[i].iObject, i&0x1 ? (TUint32)RObjectIx::EReserved : 0);
		if (r<0)
			{
			return r;
			}
		iObjAndHandle[i].iHandle = r;
		}

	//---Test data consistency---
	for (i=0;i<aSize;i++)
		{
		if (iObjAndHandle[i].iObject)
			{
			//Test At(TInt aHandle) method
			NKern::LockSystem();
			if (iObjAndHandle[i].iObject != aObjectIx.At(iObjAndHandle[i].iHandle))
				{
				NKern::UnlockSystem();
				return KErrGeneral;
				}
			NKern::UnlockSystem();
			}
		}

	aObjectIx.Check(0);

	for (i=0; i<aSize; i++)		//Remove
		{
		TUint32  attr = 0; // Receives the attributes of the removed handle...
		if (KErrNone != (r=aObjectIx.Remove(iObjAndHandle[i].iHandle, object, attr)))
			{
			return r;
			}

		if ((i&0x1) && ((attr & RObjectIx::EReserved) == 0))
			{
			return KErrBadHandle;
			}

		iObjAndHandle[i].iObject->Close(NULL);
		iObjAndHandle[i].iObject = NULL;
		}

	aObjectIx.Check(0);

	for (i=0; i<aSize; i++) //Add
		{
		if( (iObjAndHandle[i].iObject = new DObject) == NULL)
			{
			return KErrNoMemory;
			}
		iObjAndHandle[i].iObject->iContainerID = 0;
		r = aObjectIx.Add(iObjAndHandle[i].iObject, i&0x1 ?  (TUint32)RObjectIx::EReserved : 0);
		if (r<0)
			{
			return r;
			}
		iObjAndHandle[i].iHandle = r;
		}

	//---Test data consistency---
	for (i=0;i<aSize;i++)
		{
		if (iObjAndHandle[i].iObject)
			{
			NKern::LockSystem();
			//Test At(TInt aHandle) method
			if (iObjAndHandle[i].iObject != aObjectIx.At(iObjAndHandle[i].iHandle))
				{
				NKern::UnlockSystem();
				return KErrGeneral;
				}
			NKern::UnlockSystem();
			}
		}

	aObjectIx.Check(0);

	for (i=aSize-1; i>=0; i--)		//Remove in reverse
		{
		TUint32  attr = 0; // Receives the attributes of the removed handle...
		if (KErrNone != (r=aObjectIx.Remove(iObjAndHandle[i].iHandle, object, attr)))
			{
			return r;
			}
		if ((i&0x1) && ((attr & RObjectIx::EReserved) == 0))
			{
			return KErrBadHandle;
			}

		iObjAndHandle[i].iObject->Close(NULL);

		aObjectIx.Reserve(-1); // Remove a reserved object causing a TidyAndCompact()...
		}

	aObjectIx.Check(0);

	return KErrNone;
	} // DObjectTest::RObjectIxTestExerciseIx


/**
 *  Adds a number of DObjects to RObjectIx, tries random invalid handle to access them,
 *  then attempts removes them in the same order.
 * 
 *  @param aSize  Size of handle array to use.
 * 
 *  @return KErrNone or standard error code.
 */
TInt DObjectTest::RObjectIxInvalidHandleLookupTest(TInt aSize)
	{
	TInt  i, r;
	DObject*  object;
	RObjectIx myIx;

	myIx.Check(0);

	//
	// Add in some DObjects...
	//
	for (i = 0;  i < aSize;  i++)
		{
		if ((iObjAndHandle[i].iObject = new DObject) == NULL) 
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return KErrNoMemory;
			}

		iObjAndHandle[i].iObject->iContainerID = 0;

		r = myIx.Add(iObjAndHandle[i].iObject, 0);
		if (r < 0)
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return r;
			}
		iObjAndHandle[i].iHandle = r;
		}

	myIx.Check(0);

	//
	// Randomly attempt to access handles...
	//
	TInt  handlesToTest = aSize * KDObjectTestLoopCount;
	TInt  count;
	
	for (count = 0;  count < handlesToTest;  count++)
		{
		//
		// A handle looks like this:
		//	Bits 0-14	index
		//	Bit 15		no-close flag (ignored)
		//	Bits 16-29	instance value
		//	Bit 30		thread local flag (ignored)
		//	Bit 31		special handle flag (should be 0)
		//
		TInt  randomHandle = Kern::Random() & 0x3fff7fff;
		TInt  uniqueID = 0;		// any object type!
		
        NKern::LockSystem();
		object = myIx.At(randomHandle, uniqueID);
		NKern::UnlockSystem();

		if (object != NULL)
			{
			//
			// We've picked a valid handle, this is unlikely but check if
			// it is really valid...
			//
			TBool  found = EFalse;
			
			for (i = 0;  i < aSize;  i++)
				{
				if (iObjAndHandle[i].iHandle == randomHandle  &&
					iObjAndHandle[i].iObject == object)
					{
					found = ETrue;
					break;
					}
				}
			
			if (found == EFalse) 
				{
				myIx.Check(0);
				myIx.Close(NULL);
				return KErrBadHandle;
				}
			}
		}
	
	myIx.Check(0);

	//
	// Remove the DObjects...
	//
	for (i = 0;  i < aSize;  i++)
		{
		TUint32  attr = 0; // Receives the attributes of the removed handle...

		if (KErrNone != (r=myIx.Remove(iObjAndHandle[i].iHandle, object, attr))) 
			{
			myIx.Check(0);
			myIx.Close(NULL);
			return r;
			}

		iObjAndHandle[i].iObject->Close(NULL);
		}

	myIx.Check(0);
	myIx.Close(NULL);

	return KErrNone;
	} // DObjectTest::RObjectIxInvalidHandleLookupTest


TInt DObjectTest::DObjectNameTest()
	{
#define TEST_GOOD_NAME(name) if (Kern::ValidateName(name) != KErrNone) return KErrBadName;
#define TEST_BAD_NAME(name) if (Kern::ValidateName(name) != KErrBadName) return KErrBadName;
#define TEST_GOOD_FULLNAME(name) if (Kern::ValidateFullName(name) != KErrNone) return KErrBadName;
#define TEST_BAD_FULLNAME(name) if (Kern::ValidateFullName(name) != KErrBadName) return KErrBadName;
	
	
	_LIT(KGoodName1,"DObject1 ABCDEFGHIJKLMNOPRSTUVWXYZ0123456789");
	_LIT(KGoodName2,"DObject2 abdefghijklmnoprstuvwxyz!\"#$%&'()+,-./;<=>@[\\]^_`{|}~");
	_LIT(KGoodFullName1,"DObject :3");
	_LIT(KBadName1,"DObject 5 *");
	_LIT(KBadName2,"DObject 6 ?");
	TUint8 badName3[] = {'D','O','b','j','e','c','t',0x00};
	TUint8 badName4[] = {'D','O','b','j','e','c','t',0x1f};
	TUint8 badName5[] = {'D','O','b','j','e','c','t',0x7f};
	TUint8 badName6[] = {'D','O','b','j','e','c','t',0xff};
	TPtr8 badNamePtr(badName3, sizeof(badName3), sizeof(badName3));
	
	// Test Kern::ValidateName for good and bad names
	TEST_GOOD_NAME(KGoodName1);
	TEST_GOOD_NAME(KGoodName2);
	TEST_BAD_NAME(KGoodFullName1);
	TEST_BAD_NAME(KBadName1);
	TEST_BAD_NAME(KBadName2);
	TEST_BAD_NAME(badNamePtr); // already set to badName3 as no TPtr8 default constructor
	badNamePtr.Set(badName4, sizeof(badName4), sizeof(badName4));
	TEST_BAD_NAME(badNamePtr);
	badNamePtr.Set(badName5, sizeof(badName5), sizeof(badName5));
	TEST_BAD_NAME(badNamePtr);
	badNamePtr.Set(badName6, sizeof(badName6), sizeof(badName6));
	TEST_BAD_NAME(badNamePtr);
	
	// Test Kern::ValidateFullName for good and bad full names
	TEST_GOOD_FULLNAME(KGoodName1);
	TEST_GOOD_FULLNAME(KGoodName2);
	TEST_GOOD_FULLNAME(KGoodFullName1);
	TEST_BAD_FULLNAME(KBadName1);
	TEST_BAD_FULLNAME(KBadName2);
	badNamePtr.Set(badName3, sizeof(badName3), sizeof(badName3));
	TEST_BAD_FULLNAME(badNamePtr);
	badNamePtr.Set(badName4, sizeof(badName4), sizeof(badName4));
	TEST_BAD_FULLNAME(badNamePtr);
	badNamePtr.Set(badName5, sizeof(badName5), sizeof(badName5));
	TEST_BAD_FULLNAME(badNamePtr);
	badNamePtr.Set(badName6, sizeof(badName6), sizeof(badName6));
	TEST_BAD_FULLNAME(badNamePtr);

	return KErrNone;
	}

TInt DObjectTest::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
	TInt i;
	TInt duration;
	SParam param;
	TAny* args[2];	

	TRequestStatus* s=(TRequestStatus*)a1;
	kumemget32(args,a2,sizeof(args));

	switch (~aFunction)
		{
	case RTestDObject::ERObjectIxTest1:
		duration = NKern::TickCount();

		NKern::ThreadEnterCS();
		for (i = 1;  i < KDObjectTestMaxTestSize;  i = i<KDObjectTestStepStart?i+1:i+KDObjectTestStep) 
			if (KErrNone != (r=RObjectIxTest1(i)))
				{
				NKern::ThreadLeaveCS();
				Kern::RequestComplete(s, r);
				return KErrNone;
				}
		NKern::ThreadLeaveCS();

		duration = NKern::TickCount() - duration;
		kumemput32(args[0], &duration, sizeof(TInt));

		Kern::RequestComplete(s,KErrNone);
		return KErrNone;

	case RTestDObject::ERObjectIxTest2:
		duration = NKern::TickCount();

		NKern::ThreadEnterCS();
		for (i = 1;  i < KDObjectTestMaxTestSize;  i = i<KDObjectTestStepStart?i+1:i+KDObjectTestStep) 
			if (KErrNone != (r=RObjectIxTest2(i)))
				{
				NKern::ThreadLeaveCS();
				Kern::RequestComplete(s, r);
				return KErrNone;
				}

		NKern::ThreadLeaveCS();

		duration = NKern::TickCount() - duration;
		kumemput32(args[0], &duration, sizeof(TInt));

		Kern::RequestComplete(s,KErrNone);
		return KErrNone;

	case RTestDObject::ERObjectIxTest3:
		kumemget32(&param, args[0], sizeof(param));

		duration = NKern::TickCount();
		iSeed[0] = param.iSeed[0];
		iSeed[1] = param.iSeed[1];
		NKern::ThreadEnterCS();
		for (i = 1;  i < KDObjectTestMaxTestSize;  i = i<KDObjectTestStepStart?i+1:i+KDObjectTestStep) 
			if (KErrNone != (r=RObjectIxTest3(i, param.iPerformanceTest)))
				{
				NKern::ThreadLeaveCS();
				Kern::RequestComplete(s, r);
				return KErrNone;
				}
		NKern::ThreadLeaveCS();
		
		duration = NKern::TickCount() - duration;
		kumemput32(&((SParam*)args[0])->duration, &duration, sizeof(TInt));

		Kern::RequestComplete(s,KErrNone);
		return KErrNone;

	case RTestDObject::ERObjectIxTest4:
		duration = NKern::TickCount();

		NKern::ThreadEnterCS();
		for (i = 1;  i < KDObjectTestMaxTestSize;  i = i<KDObjectTestStepStart?i+1:i+KDObjectTestStep) 
			if (KErrNone != (r=RObjectIxTest4(i)))
				{
				NKern::ThreadLeaveCS();
				Kern::RequestComplete(s, r);
				return KErrNone;
				}
		NKern::ThreadLeaveCS();

		duration = NKern::TickCount() - duration;
		kumemput32(args[0], &duration, sizeof(TInt));

		Kern::RequestComplete(s,KErrNone);
		return KErrNone;

	case RTestDObject::ERObjectIxThreadTestCreateIx:
		//RTestDObject::RObjectIxThreadTestCreateIx(void*& aRObjectIxPtr);
		{
		RObjectIx*  threadTestObjectIx = NULL;
		
		NKern::ThreadEnterCS();
		threadTestObjectIx = new RObjectIx();
		if (threadTestObjectIx == NULL)
			{
			NKern::ThreadLeaveCS();
			r = KErrNoMemory;
			}
		else
			{
			NKern::ThreadLeaveCS();
			
			kumemput32(args[0], &threadTestObjectIx, sizeof(RObjectIx*));
			r = KErrNone;
			}
		Kern::RequestComplete(s, r);
		r = KErrNone;
		}
		break;
		
	case RTestDObject::ERObjectIxThreadTestExerciseIx:
		//RTestDObject::RObjectIxThreadTestExerciseIx(void* aRObjectIxPtr);
		{
		RObjectIx*  threadTestObjectIx = (RObjectIx*) args[0];

		NKern::ThreadEnterCS();
		for (i = 1;  i < KDObjectTestMaxTestSize;  i = i<KDObjectTestStepStart?i+1:i+KDObjectTestStep) 
			{
			if (KErrNone != (r=RObjectIxTestExerciseIx(*threadTestObjectIx, i)))
				{
				NKern::ThreadLeaveCS();
				Kern::RequestComplete(s, r);
				return KErrNone;
				}
			}
		NKern::ThreadLeaveCS();

		Kern::RequestComplete(s, KErrNone);
		r = KErrNone;
		}
		break;
		
	case RTestDObject::ERObjectIxThreadTestFreeIx:
		//RTestDObject::RObjectIxThreadTestFreeIx(void* aRObjectIxPtr)
		{
		RObjectIx*  threadTestObjectIx = (RObjectIx*) args[0];
		
		NKern::ThreadEnterCS();
		threadTestObjectIx->Check(0);
		threadTestObjectIx->Close(NULL);
		delete threadTestObjectIx;
		threadTestObjectIx = NULL;
		NKern::ThreadLeaveCS();
		
		Kern::RequestComplete(s, KErrNone);
		r = KErrNone;
		}
		break;
		
	case RTestDObject::ERObjectIxInvalidHandleLookupTest:
		//RTestDObject::InvalidHandleLookupTest()
		{
		NKern::ThreadEnterCS();
		for (i = 1;  i < KDObjectTestMaxTestSize;  i = i<KDObjectTestStepStart?i+1:i+KDObjectTestStep) 
			{
			if (KErrNone != (r=RObjectIxInvalidHandleLookupTest(i)))
				{
				NKern::ThreadLeaveCS();
				Kern::RequestComplete(s, r);
				return KErrNone;
				}
			}
		NKern::ThreadLeaveCS();

		Kern::RequestComplete(s, r);
		}
		break;
		
	case RTestDObject::EDObjectNameTest:
		
		r = DObjectNameTest();
		Kern::RequestComplete(s, r);
		break;
		
	default:
		r = KErrNotSupported;
		break;
		}
	return r;
	}
