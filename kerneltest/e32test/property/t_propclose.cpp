// Copyright (c) 2010-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\property\t_propclose
//This test case was written to exercise the use-case 
//before DPropertyRef::Close() was introduced in sproperty.cpp
//Kernel used to crash when a Close() was issued on a property handle by some thread
//when other thread had issued a Cancel() on it.
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32property.h>

RTest test(_L("T_PROPCLOSE"));

const TInt32 KUidTestPropertyCategoryValue = 0xbaadbeef;
const TUid KUidTestPropertyCategory = {KUidTestPropertyCategoryValue};
const TUint KUidTestPropertyKey = 0xfeedfaceu;
const TInt KHeapSize=0x200;

#define TEST_RUNS 1000

class CPropertyTest
	{
public:
	CPropertyTest();
	~CPropertyTest();

	TInt DefineProperty();

	TInt CreateThreadOneAndResume();
	TInt CreateThreadTwoAndResume();
	void CloseAndWait();

	static TInt ThreadFuncOne(TAny* aThreadOnePtr);
	TInt RunThreadFuncOne();

	static TInt ThreadFuncTwo(TAny* aThreadTwoPtr);
	TInt RunThreadFuncTwo();

private:	
	RProperty iP;
	RTest iTest;

	RThread iThreadOne;
	TRequestStatus iThreadOneStatus;

	RThread iThreadTwo;
	TRequestStatus iThreadTwoStatus;

	TInt iLoopCount;
	};


CPropertyTest::CPropertyTest():iTest(_L("PropertyClose Test"))
	{
	iLoopCount=0;
	}


CPropertyTest::~CPropertyTest()
	{
	iTest.End();
	iTest.Close();	
	}


TInt CPropertyTest::DefineProperty()
	{
	iTest.Title();
	iTest.Start(_L("Start PropertyClose Test"));
	iTest.Next(_L("Define Property"));
	TInt r = RProperty::Define(KUidTestPropertyCategory, KUidTestPropertyKey, RProperty::EInt);
	test(r==KErrAlreadyExists || r==KErrNone);
	return r;
	}

TInt CPropertyTest::CreateThreadOneAndResume()
	{
	TInt r=KErrNone;

	r = iThreadOne.Create(KNullDesC,ThreadFuncOne,KDefaultStackSize,KHeapSize,KHeapSize,this);
	test_KErrNone(r);
	iThreadOne.SetPriority(EPriorityAbsoluteHigh); //value 23
	iThreadOne.Logon(iThreadOneStatus);

	iThreadOne.Resume();
	return r;
	}


TInt CPropertyTest::CreateThreadTwoAndResume()
	{
	TInt r=KErrNone;

	r = iThreadTwo.Create(KNullDesC,ThreadFuncTwo,KDefaultStackSize,KHeapSize,KHeapSize,this);
	test_KErrNone(r);
	iThreadTwo.SetPriority(EPriorityMore); //value 23
	iThreadTwo.Logon(iThreadTwoStatus);

	iThreadTwo.Resume();
	return r;
	}

void CPropertyTest::CloseAndWait()
	{
	User::WaitForAnyRequest();
	if( iThreadOne.ExitType() == EExitPanic)
		{
		iThreadOne.Close();
		CreateThreadOneAndResume();
		CloseAndWait();
		}
	else if(iThreadOneStatus == KErrNone)
			{
			iThreadOne.Close();
			}
		else if(iThreadTwoStatus == KErrNone)
					{
					iThreadTwo.Close();
					}
	}


TInt CPropertyTest::ThreadFuncOne(TAny* aThreadOnePtr)
	{
	return ((CPropertyTest*)aThreadOnePtr)->RunThreadFuncOne();
	}


TInt CPropertyTest::ThreadFuncTwo(TAny* aThreadTwoPtr)
	{
	return ((CPropertyTest*)aThreadTwoPtr)->RunThreadFuncTwo();
	}


TInt CPropertyTest::RunThreadFuncOne()
	{
	TInt r=KErrNone;
	TInt attach;
	TRequestStatus sp1;
	while(iLoopCount <= TEST_RUNS)
		{
		attach = iP.Attach(KUidTestPropertyCategory, KUidTestPropertyKey);
		test_KErrNone(attach);
		iP.Subscribe(sp1);
		iP.Cancel();  //should initiate TProperty::CompleteCancellationQDfc()
		iLoopCount++;
		}
	return r;
	}


TInt CPropertyTest::RunThreadFuncTwo()
	{
	while(iLoopCount <= TEST_RUNS)
		{
		//loop adds delay to simulate crash on Naviengine
		for(TInt i=0;i<10000;i++){};
		iP.Close();
		iLoopCount++;
		}
	return KErrNone;
	}


GLDEF_C TInt E32Main()
	{
	//Need to ensure ThreadOne,ThreadTwo and Main are of higher priorities than SVR thread
	RProcess().SetPriority(EPriorityHigh); 
	RThread().SetPriority(EPriorityMore); //value 20

	CPropertyTest propertyTest;
	propertyTest.DefineProperty();
	propertyTest.CreateThreadOneAndResume();
	propertyTest.CreateThreadTwoAndResume();
	propertyTest.CloseAndWait();

	return KErrNone;
	}
