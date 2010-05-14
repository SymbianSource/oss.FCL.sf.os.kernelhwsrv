// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\t_sque.cpp
// Overview:
// Test single linked list functionality.
// API Information:
// TSglQueLink, TSglQueBase.
// Details:
// - Create specified number of TSglQueLink objects, then delete them. 
// - Create TSglQueBase object with specified offset and without an offset,
// add and remove links to the list, check that the list is empty.
// - Create TSglQueBase object with and without an offset, verify offset, head 
// and last member values are correct.
// - Create TSglQueBase object, insert links using DoAddFirst and DoAddLast and
// remove links using DoRemove, check the results are as expected.
// - Create TSglQueBase object, insert links using DoAddFirst and DoAddLast and
// verify IsEmpty return the correct value. Set and verify the offset.
// - Create TSglQue object, insert TSglQueLink link at front and end of list, test 
// whether the links are inserted at specified location, remove some links.
// - Create TSglQue object, insert links using DoAddFirst and DoAddLast and
// remove links using DoRemove, check the results are as expected.
// - Create TSglQue object, insert links using DoAddFirst and DoAddLast and
// verify IsEmpty return the correct value. Set and verify the offset.
// - Create TSglQue object with and without an offset, verify offset, head 
// and last member values are correct.
// - Create TSglQue object, insert links using AddFirst and AddLast and
// verify results are as expected.
// - Create TSglQue object, insert links using AddFirst and AddLast and
// verify results using IsFirst and IsLast are as expected.
// - Create TSglQue object, insert links using AddFirst and AddLast and
// verify results using First and Last are as expected.
// - Create TSglQue object, insert links using AddLast and delete using Remove,
// verify results are as expected.
// - Create TSglQueIterBase object, with and without an offset, call the 
// DoCurrent, DoPostInc and SetToFirst methods.
// - Create TSglQueIterBase object, with and without an offset, and TSglQue 
// with different offsets, verify results are as expected.
// - Create TSglQueIterBase object, , with and without an offset, use the 
// DoCurrent and DoPostInc methods and verify the results are as expected.
// - Create TSglQueIterBase object, , with and without an offset, use the 
// DoPostInc and SetToFirst methods and verify the results are as expected.
// - Create TSglQueIter object, with and without an offset, iterate using 
// the ++ operator and delete the object.
// - Create TSglQueIter object, with and without an offset, and TSglQue 
// object with different offsets, verify results are as expected.
// - Create TSglQueIter object, with and without an offset, use the 
// DoCurrent and DoPostInc methods and verify the results are as expected.
// - Create TSglQueIter object, with and without an offset, use the 
// DoPostInc and SetToFirst methods and verify the results are as expected.
// - Create TSglQueIter object, with and without an offset, iterate using 
// the ++ operator, use the conversion operator, verify the results are 
// as expected.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>

LOCAL_D RTest test(_L("T_SQUE"));

#define SIZE 10
#define MAX_OFFSET 10

struct Item
	{
	TSglQueLink iLink;
	TInt iSpace[MAX_OFFSET]; // Reserve some space
	};

struct ItemWithOffset
	{
	TInt iSpace[MAX_OFFSET];
	TSglQueLink iLink;
	};

template<class T>
class TestTQueLink
	{
public:
	void TestQueLink();	// Calls Test: 1
	void Test1();
protected:
	void CreateObjects(TInt aBaseLink);
	void DestroyObjects();
private:
	T* iLink[SIZE];
	};

template<class T>
void TestTQueLink<T>::CreateObjects(TInt aBaseLink)
	{
	TInt i;

	for (i=0;i<SIZE;i++)
		iLink[i]=new T;
	if (aBaseLink>=0&&aBaseLink<SIZE)
		(iLink[aBaseLink])->iNext=iLink[aBaseLink];
	}

template<class T>
void TestTQueLink<T>::DestroyObjects()
	{
	TInt i;

	for (i=0;i<SIZE;i++)
		delete iLink[i];
	}

template <class T>
void TestTQueLink<T>::TestQueLink()
	{
	test.Start(_L("Test Enque"));
	Test1();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQueLink<T>::Test1()
	{
	test.Start(_L("Create objects"));
	CreateObjects(1);
	test.Next(_L("Destroy objects"));
	DestroyObjects();
	test.End();
	}

class VSglQueBase : public TSglQueBase
	{
public:
	inline VSglQueBase();
	inline VSglQueBase(TInt anOffset);
	inline void sDoAddFirst(TAny* aPtr) {this->DoAddFirst(aPtr);}
	inline void sDoAddLast(TAny* aPtr) {this->DoAddLast(aPtr);}
	inline void sDoRemove(TAny* aPtr) {this->DoRemove(aPtr);}
public:
	TSglQueLink** sHead;
	TSglQueLink** sLast;
	TInt* sOffset;
private:
	void SetMembers();
	};

template <class T>
class VSglQue : public TSglQue<T>
	{
public:
	inline VSglQue();
	inline VSglQue(TInt anOffset);
	inline void sDoAddFirst(TAny* aPtr) {this->DoAddFirst(aPtr);}
	inline void sDoAddLast(TAny* aPtr) {this->DoAddLast(aPtr);}
	inline void sDoRemove(TAny* aPtr) {this->DoRemove(aPtr);}
public:
	TSglQueLink** sHead;
	TSglQueLink** sLast;
	TInt* sOffset;
private:
	void SetMembers();
	};

inline VSglQueBase::VSglQueBase()
	{
	SetMembers();
	}

inline VSglQueBase::VSglQueBase(TInt anOffset)
	:TSglQueBase(anOffset)
	{
	SetMembers();
	}

void VSglQueBase::SetMembers()
	{
	sHead=&iHead;
	sLast=&iLast;
	sOffset=&iOffset;
	}

template <class T>
VSglQue<T>::VSglQue()
	{
	SetMembers();
	}

template <class T>
VSglQue<T>::VSglQue(TInt anOffset)
	:TSglQue<T>(anOffset)
	{
	SetMembers();
	}

template <class T>
void VSglQue<T>::SetMembers()
	{
	sHead=&this->iHead;
	sLast=&this->iLast;
	sOffset=&this->iOffset;
	}

template<class T>
class TestTQue
	{
public:
	void TestQueBase();
	void TestSglQue();
	void Test1();	// All functions		//TSglQueBase functions
	void Test2();	// Constructors
	void Test3();	// DoAdd's
	void Test4();	// Public functions
	void Test5();	// All functions		//TSglQue
	//void Test6();	// Constructors
	void Test7();	// Add's
	void Test8();	// Is's
	void Test9();	// Get's
	void Test10();	// Add's
private:
	void CallTest3_4();	
	};

template<class T>
void TestTQue<T>::CallTest3_4()
	{
	test.Next(_L("Test DoAdd's"));
	Test3();
	test.Next(_L("Test public functions"));
	Test4();
	}

template<class T>
void TestTQue<T>::TestQueBase()
	{
	test.Start(_L("Test all member functions (simply)"));
	Test1();						 
	test.Next(_L("Test Constructors"));
	Test2();
	CallTest3_4();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::TestSglQue()
	{
	test.Start(_L("Test all member functions (simply)"));
	Test5();
	test.Next(_L("Test Super Class functions"));
	CallTest3_4();
	test.Next(_L("Test Constructors"));
	Test2();
	test.Next(_L("Test Add's"));
	Test7();
	test.Next(_L("Test Is's"));
	Test8();
	test.Next(_L("Test Get's"));
	Test9();
	test.Next(_L("Test Remove"));
	Test10();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test1()
	{
	T* que;
	TSglQueLink link1,link2;
	TInt offset=4;

	test.Start(_L("Constructors"));
	que=new VSglQueBase(offset);
	delete que;
	que=new VSglQueBase;
	//delete que;
	test.Next(_L("DoAdd's"));
	que->sDoAddFirst(&link1);
	que->sDoAddLast(&link2);
	que->sDoRemove(&link1);
	que->sDoRemove(&link2);
	test.Next(_L("Public"));
	que->IsEmpty();
	que->SetOffset(offset);
	test.Next(_L("Finished"));
	delete que;
	test.End();
	}

template<class T>
void TestTQue<T>::Test2()
	{
	T* que;
	TInt offset;

	test.Start(_L("Default constructor"));
	que=new T();
	test(*(que->sOffset)==0);
	test(*(que->sHead)==NULL);
	test(*(que->sLast)==(TSglQueLink*) que->sHead);
	delete que;
	test.Next(_L("Offset constructor"));
	for (offset=0;offset<40;offset+=4)
		{
		que=new T(offset);
		test(*(que->sOffset)==offset);
		test(*(que->sHead)==NULL);
		test(*(que->sLast)==(TSglQueLink*) que->sHead);
		delete que;
		}
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test3()
	{
	T* que;

		{
		TSglQueLink link1,link2,link3,link4;
		test.Start(_L("AddFirst"));
		que=new T();
		test(*(que->sHead)==NULL);
		test(*(que->sLast)==(TSglQueLink*) que->sHead);
		que->sDoAddFirst(&link1);
		test(*(que->sHead)==&link1);
		test(*(que->sLast)==&link1);
		test(link1.iNext==NULL);
		que->sDoAddFirst(&link2);
		test(*(que->sHead)==&link2);
		test(*(que->sLast)==&link1);
		test(link1.iNext==NULL);
		test(link2.iNext==&link1);
		que->sDoAddFirst(&link3);
		test(*(que->sHead)==&link3);
		test(*(que->sLast)==&link1);
		test(link1.iNext==NULL);
		test(link2.iNext==&link1);
		test(link3.iNext==&link2);
		que->sDoAddFirst(&link4);
		test(*(que->sHead)==&link4);
		test(*(que->sLast)==&link1);
		test(link1.iNext==NULL);
		test(link2.iNext==&link1);
		test(link3.iNext==&link2);
		test(link4.iNext==&link3);
		delete que;
		}
	TSglQueLink link1,link2,link3,link4;
	test.Next(_L("AddLast"));
	que=new T();
	test(*(que->sHead)==NULL);
	test(*(que->sLast)==(TSglQueLink*) que->sHead);
	que->sDoAddLast(&link1);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link1);
	test(link1.iNext==NULL);
	que->sDoAddLast(&link2);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link2);
	test(link1.iNext==&link2);
	test(link2.iNext==NULL);
	que->sDoAddLast(&link3);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link3);
	test(link1.iNext==&link2);
	test(link2.iNext==&link3);
	test(link3.iNext==NULL);
	que->sDoAddLast(&link4);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link4);
	test(link1.iNext==&link2);
	test(link2.iNext==&link3);
	test(link3.iNext==&link4);
	test(link4.iNext==NULL);
	test.Next(_L("Remove"));
	que->sDoRemove(&link3);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link4);
	test(link1.iNext==&link2);
	test(link2.iNext==&link4);
	test(link4.iNext==NULL);
	que->sDoRemove(&link4);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link2);
	test(link1.iNext==&link2);
	test(link2.iNext==NULL);
	que->sDoRemove(&link1);
	test(*(que->sHead)==&link2);
	test(*(que->sLast)==&link2);
	test(link2.iNext==NULL);
	que->sDoRemove(&link2);
	test(*(que->sHead)==NULL);
	test(*(que->sLast)==(TSglQueLink*) que->sHead);
 	delete que;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test4()
	{
	T* que;
	TInt offset;

	test.Start(_L("IsEmpty"));
	que=new T();
	test(que->IsEmpty()==TRUE);
	TSglQueLink link1,link2;
	que->sDoAddFirst(&link1);
	test(que->IsEmpty()==FALSE);
	que->sDoRemove(&link1);
	test(que->IsEmpty()==TRUE);
	que->sDoAddLast(&link2);
	test(que->IsEmpty()==FALSE);
	que->sDoAddFirst(&link1);
	test(que->IsEmpty()==FALSE);
	que->sDoRemove(&link2);
	test(que->IsEmpty()==FALSE);
	que->sDoRemove(&link1);
	test(que->IsEmpty()==TRUE);
	test.Next(_L("SetOffset"));
	for (offset=0;offset<40;offset+=4)
		{
		que->SetOffset(offset);
		test(*(que->sOffset)==offset);
		}
	test.Next(_L("Finished"));
 	delete que;
	test.End();
	}

template<class T>
void TestTQue<T>::Test5()
	{
	T* que;
	TSglQueLink link1,link2;
	TInt offset=4;

	test.Start(_L("Constructors"));
	que=new VSglQue<TSglQueLink>(offset);
	delete que;
	que=new VSglQue<TSglQueLink>;
	test.Next(_L("Add's"));
	que->AddFirst(link1);
	que->AddLast(link2);
	test.Next(_L("Is's"));
	que->IsFirst(&link1);
	que->IsLast(&link1);
	test.Next(_L("Get's"));
	que->First();
	que->Last();
	test.Next(_L("Remove"));
	que->Remove(link1);
	que->Remove(link2);
	test.Next(_L("Finished"));
	delete que;
	test.End();
	}

/*template<class T>
void TestTQue<T>::Test6()
	{
	T* que;
	TInt offset;

	test.Start(_L("Default constructor"));
	que=new VSglQue<TSglQueBase>();
	test(*(que->sFirstDelta)==NULL);
	delete que;
	test.Next(_L("Offset constructor"));
	for (offset=0;offset<40;offset+=4)
		{
		que=new VDeltaQueBase(offset);
		test(*(que->sOffset)==offset);
		test(*(que->sFirstDelta)==NULL);
		delete que;
		}
	test.Next(_L("Finished"));
	test.End();
	}*/

template<class T>
void TestTQue<T>::Test7()
	{
	T* que;

		{
		TSglQueLink link1,link2,link3,link4;
		test.Start(_L("AddFirst"));
		que=new T();
		test(*(que->sHead)==NULL);
		test(*(que->sLast)==(TSglQueLink*) que->sHead);
		que->AddFirst(link1);
		test(*(que->sHead)==&link1);
		test(*(que->sLast)==&link1);
		test(link1.iNext==NULL);
		que->AddFirst(link2);
		test(*(que->sHead)==&link2);
		test(*(que->sLast)==&link1);
		test(link1.iNext==NULL);
		test(link2.iNext==&link1);
		que->AddFirst(link3);
		test(*(que->sHead)==&link3);
		test(*(que->sLast)==&link1);
		test(link1.iNext==NULL);
		test(link2.iNext==&link1);
		test(link3.iNext==&link2);
		que->AddFirst(link4);
		test(*(que->sHead)==&link4);
		test(*(que->sLast)==&link1);
		test(link1.iNext==NULL);
		test(link2.iNext==&link1);
		test(link3.iNext==&link2);
		test(link4.iNext==&link3);
		delete que;
		}
	TSglQueLink link1,link2,link3,link4;
	test.Next(_L("AddLast"));
	que=new T();
	test.Next(_L("AddLast"));
	que=new T();
	test(*(que->sHead)==NULL);
	test(*(que->sLast)==(TSglQueLink*) que->sHead);
	que->AddLast(link1);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link1);
	test(link1.iNext==NULL);
	que->AddLast(link2);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link2);
	test(link1.iNext==&link2);
	test(link2.iNext==NULL);
	que->AddLast(link3);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link3);
	test(link1.iNext==&link2);
	test(link2.iNext==&link3);
	test(link3.iNext==NULL);
	que->AddLast(link4);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link4);
	test(link1.iNext==&link2);
	test(link2.iNext==&link3);
	test(link3.iNext==&link4);
	test(link4.iNext==NULL);
	delete que;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test8()
	{
	T* que;

		{
		TSglQueLink link1,link2,link3,link4;
		test.Start(_L("IsFirst"));
		que=new T();
		test(que->IsFirst((TSglQueLink*) que->sHead)==FALSE);
		test(que->IsFirst((TSglQueLink*) *(que->sHead))==TRUE);
		test(que->IsFirst((TSglQueLink*) *(que->sLast))==FALSE);
		que->AddFirst(link1);
		test(que->IsFirst((TSglQueLink*) que->sHead)==FALSE);
		test(que->IsFirst((TSglQueLink*) *(que->sHead))==TRUE);
		test(que->IsFirst((TSglQueLink*) *(que->sLast))==TRUE);
		test(que->IsFirst(&link1)==TRUE);
		que->AddFirst(link2);
		test(que->IsFirst((TSglQueLink*) que->sHead)==FALSE);
		test(que->IsFirst((TSglQueLink*) *(que->sHead))==TRUE);
		test(que->IsFirst((TSglQueLink*) *(que->sLast))==FALSE);
		test(que->IsFirst(&link1)==FALSE);
		test(que->IsFirst(&link2)==TRUE);
		que->AddFirst(link3);
		test(que->IsFirst((TSglQueLink*) que->sHead)==FALSE);
		test(que->IsFirst((TSglQueLink*) *(que->sHead))==TRUE);
		test(que->IsFirst((TSglQueLink*) *(que->sLast))==FALSE);
		test(que->IsFirst(&link1)==FALSE);
		test(que->IsFirst(&link2)==FALSE);
		test(que->IsFirst(&link3)==TRUE);
		que->AddFirst(link4);
		test(que->IsFirst((TSglQueLink*) que->sHead)==FALSE);
		test(que->IsFirst((TSglQueLink*) *(que->sHead))==TRUE);
		test(que->IsFirst((TSglQueLink*) *(que->sLast))==FALSE);
		test(que->IsFirst(&link1)==FALSE);
		test(que->IsFirst(&link2)==FALSE);
		test(que->IsFirst(&link3)==FALSE);
		test(que->IsFirst(&link4)==TRUE);
		delete que;
		}
	TSglQueLink link1,link2,link3,link4;
	test.Next(_L("IsLast"));
	que=new T();
	test(que->IsLast((TSglQueLink*) que->sHead)==TRUE);
	test(que->IsLast((TSglQueLink*) *(que->sHead))==FALSE);
	test(que->IsLast((TSglQueLink*) *(que->sLast))==TRUE);
	que->AddLast(link1);
	test(que->IsLast((TSglQueLink*) que->sHead)==FALSE);
	test(que->IsLast((TSglQueLink*) *(que->sHead))==TRUE);
	test(que->IsLast((TSglQueLink*) *(que->sLast))==TRUE);
	test(que->IsLast(&link1)==TRUE);
	que->AddLast(link2);
	test(que->IsLast((TSglQueLink*) que->sHead)==FALSE);
	test(que->IsLast((TSglQueLink*) *(que->sHead))==FALSE);
	test(que->IsLast((TSglQueLink*) *(que->sLast))==TRUE);
	test(que->IsLast(&link1)==FALSE);
	test(que->IsLast(&link2)==TRUE);
	que->AddLast(link3);
	test(que->IsLast((TSglQueLink*) que->sHead)==FALSE);
	test(que->IsLast((TSglQueLink*) *(que->sHead))==FALSE);
	test(que->IsLast((TSglQueLink*) *(que->sLast))==TRUE);
	test(que->IsLast(&link1)==FALSE);
	test(que->IsLast(&link2)==FALSE);
	test(que->IsLast(&link3)==TRUE);
	que->AddLast(link4);
	test(que->IsLast((TSglQueLink*) que->sHead)==FALSE);
	test(que->IsLast((TSglQueLink*) *(que->sHead))==FALSE);
	test(que->IsLast((TSglQueLink*) *(que->sLast))==TRUE);
	test(que->IsLast(&link1)==FALSE);
	test(que->IsLast(&link2)==FALSE);
	test(que->IsLast(&link3)==FALSE);
	test(que->IsLast(&link4)==TRUE);
	test.Next(_L("Finished"));
	delete que;
	test.End();
	}

template<class T>
void TestTQue<T>::Test9()
	{
	T* que;

		{
		TSglQueLink link1,link2,link3,link4;
		test.Start(_L("First"));
		que=new T();
		test(que->First()==NULL);
		que->AddFirst(link1);
		test(que->First()==&link1);
	 	que->AddFirst(link2);
		test(que->First()==&link2);
	 	que->AddFirst(link3);
		test(que->First()==&link3);
	  	que->AddFirst(link4);
		test(que->First()==&link4);
	 	delete que;
		}
	TSglQueLink link1,link2,link3,link4;
	test.Next(_L("Last"));
	que=new T();
	test(que->Last()==(TSglQueLink*) que->sHead);
	que->AddLast(link1);
	test(que->Last()==&link1);
 	que->AddLast(link2);
	test(que->Last()==&link2);
 	que->AddLast(link3);
	test(que->Last()==&link3);
  	que->AddLast(link4);
	test(que->Last()==&link4);
	test.Next(_L("Finished"));
 	delete que;
	test.End();
	}

template<class T>
void TestTQue<T>::Test10()
	{
	T* que;
	TSglQueLink link1,link2,link3,link4;

	que=new T();
	que->AddLast(link1);
	que->AddLast(link2);
	que->AddLast(link3);
	que->AddLast(link4);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link4);
	test(link1.iNext==&link2);
	test(link2.iNext==&link3);
	test(link3.iNext==&link4);
	test(link4.iNext==NULL);
	que->Remove(link3);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link4);
	test(link1.iNext==&link2);
	test(link2.iNext==&link4);
	test(link4.iNext==NULL);
	que->Remove(link4);
	test(*(que->sHead)==&link1);
	test(*(que->sLast)==&link2);
	test(link1.iNext==&link2);
	test(link2.iNext==NULL);
	que->Remove(link1);
	test(*(que->sHead)==&link2);
	test(*(que->sLast)==&link2);
	test(link2.iNext==NULL);
	que->Remove(link2);
	test(*(que->sHead)==NULL);
	test(*(que->sLast)==(TSglQueLink*) que->sHead);
 	delete que;
	}

class VSglQueIterBase : public TSglQueIterBase
	{
public:
	VSglQueIterBase(TSglQueBase& aQue);
	inline TAny* sDoPostInc() {return DoPostInc();}
	inline TAny* sDoCurrent() {return DoCurrent();}
public:
	TInt* sOffset;
	TSglQueLink** sHead;
	TSglQueLink** sNext;
private:
	void SetMember();
	};

template <class T>
class VSglQueIter : public TSglQueIter<T>
	{
public:
	VSglQueIter(TSglQue<T>& aQue);
	inline TAny* sDoPostInc() {return this->DoPostInc();}
	inline TAny* sDoCurrent() {return this->DoCurrent();}
public:
	TInt* sOffset;
	TSglQueLink** sHead;
	TSglQueLink** sNext;
private:
	void SetMember();
	};

VSglQueIterBase::VSglQueIterBase(TSglQueBase& aQue)
	:TSglQueIterBase(aQue)
	{
	SetMember();
	}

void VSglQueIterBase::SetMember()
	{
	sOffset=&iOffset;
	sHead=&iHead;
	sNext=&iNext;
	}

template <class T>
VSglQueIter<T>::VSglQueIter(TSglQue<T>& aQue)
	:TSglQueIter<T>(aQue)
	{
	SetMember();
	}

template <class T>
void VSglQueIter<T>::SetMember()
	{
	sOffset=&this->iOffset;
	sHead=&this->iHead;
	sNext=&this->iNext;
	}
	
template<class T,class Iter>
class TestTQueIter
	{
public:
	void TestIterBase();
	void TestQueIter();
	void Test1();	//All functions			//TSglQueIterBase
	void Test2();	//Constructor
	void Test3();	//Do's
	void Test4();	//Set
	void Test5();	//All functions			//TDblQueIter
	//void Test6();	//Constructors									//Redundant
	void Test7();	//Iterators
private:
	void CallTest2_4();
	};

template<class T,class Iter>
void TestTQueIter<T,Iter>::CallTest2_4()
	{
	test.Next(_L("Constructors"));
	Test2();
	test.Next(_L("Do's"));
	Test3();
	test.Next(_L("Sets"));
	Test4();
	}

template<class T,class Iter>
void TestTQueIter<T,Iter>::TestIterBase()
	{
	test.Start(_L("All Methods"));
	Test1();
	CallTest2_4();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T,class Iter>
void TestTQueIter<T,Iter>::TestQueIter()
	{
	test.Start(_L("All Methods"));
	Test5();
	CallTest2_4();
	test.Next(_L("Iterators"));
	Test7();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T,class Iter>
void TestTQueIter<T,Iter>::Test1()
	{
	T item1,item2;
	TSglQue<T> que(_FOFF(T,iLink));
	Iter* iter;

	que.AddFirst(item2);
	que.AddFirst(item1);
	test.Start(_L("Constructor"));
	iter=new Iter(que);
	test.Next(_L("Do's"));
	iter->sDoCurrent();
	iter->sDoPostInc();
	test.Next(_L("Sets"));
	iter->SetToFirst();
	delete iter;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T,class Iter>
void TestTQueIter<T,Iter>::Test2()
	{
	TSglQue<T>* que;
	TInt offset;
 	Iter* iter;
	TSglQueLink* head;

	for (offset=0;offset<40;offset+=4)
		{
		que=new TSglQue<T>(offset);
		iter=new Iter(*que);
		test(*(iter->sHead)==PtrAdd((TSglQueLink*) que->Last(),offset));
		head=*(iter->sHead);
		test(que->IsFirst((T*) PtrSub(*(iter->sNext),offset)));		//Need to pass a pointer to a item
		test(*(iter->sOffset)==offset);
		delete iter;
		T item;
		que->AddFirst(item);
		iter=new Iter(*que);
		test(*(iter->sHead)==head);
		test(que->IsFirst((T*) PtrSub(*(iter->sNext),offset)));
		test(*(iter->sOffset)==offset);
		delete iter;
		delete que;
		}
	}

template<class T,class Iter>
void TestTQueIter<T,Iter>::Test3()
	{
	T item1,item2,item3,item4;
	TSglQue<T> que(_FOFF(T,iLink));
 	Iter* iter;
				  
	que.AddFirst(item4);
	que.AddFirst(item3);
	que.AddFirst(item2);
	que.AddFirst(item1);
	test.Start(_L("DoPostInc"));
	iter=new Iter(que);
	test(&item1==iter->sDoPostInc());
	test(&item2.iLink==*(iter->sNext));
	test(&item2==iter->sDoPostInc());
	test(&item3.iLink==*(iter->sNext));
	test(&item3==iter->sDoPostInc());
	test(&item4.iLink==*(iter->sNext));
	test(&item4==iter->sDoPostInc());
	test((Item*) *(iter->sNext)==NULL);
	test(iter->sDoPostInc()==NULL);
	delete iter;
	test.Next(_L("DoCurrent"));
	iter=new Iter(que);
	test(&item1==iter->sDoCurrent());
	iter->sDoPostInc();
	test(&item2==iter->sDoCurrent());
	iter->sDoPostInc();
	test(&item3==iter->sDoCurrent());
	iter->sDoPostInc();
	test(&item4==iter->sDoCurrent());
	iter->sDoPostInc();
	test(iter->sDoCurrent()==NULL);
	delete iter;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T,class Iter>
void TestTQueIter<T,Iter>::Test4()
	{
	T item1,item2,item3,item4;
	TSglQue<T> que(_FOFF(T,iLink));
 	Iter* iter;
	TInt i,j;

	que.AddFirst(item4);
	que.AddFirst(item3);
	que.AddFirst(item2);
	que.AddFirst(item1);
	iter=new Iter(que);
	for(i=0;i<5;i++)
		{
		for(j=0;j<i;j++)
			iter->sDoPostInc();
		iter->SetToFirst();
		test(*(iter->sNext)==&item1.iLink);
		}
	delete iter;
	}

template<class T,class Iter>
void TestTQueIter<T,Iter>::Test5()
	{
	T item1,item2;
	TSglQue<T> que(_FOFF(T,iLink));
	Iter* iter;

	que.AddFirst(item2);
	que.AddFirst(item1);
	test.Start(_L("Constructor"));
	iter=new Iter(que);
	test.Next(_L("Iterators"));
	(*iter)++;
	delete iter;
	test.Next(_L("Finished"));
	test.End();
	}

/*template<class T>											//Redundant
void TestTQueIter<T>::Test6()
	{
	Item item;
	TDblQue<Item>* que;
	TInt offset;
 	T* iter;

	for (offset=0;offset<40;offset+=4)
		{
		que=new TDblQue<Item>(offset);
		iter=new T(*que);
		test(que->IsHead((Item*) *(iter->sHead)));
		test(que->IsHead((Item*) *(iter->sNext)));
		test(*(iter->sOffset)==offset);
		delete iter;
		delete que;
		que=new TDblQue<Item>(offset);
		que->AddFirst(item);
		iter=new T(*que);
		test(que->IsHead((Item*) *(iter->sHead)));
		test(*(iter->sNext)==&item.iLink);
		test(*(iter->sOffset)==offset);
		delete iter;
		delete que;
		}
	}*/

template<class T,class Iter>
void TestTQueIter<T,Iter>::Test7()
	{
	T item1,item2,item3,item4;
	TSglQue<T> que(_FOFF(T,iLink));
 	Iter* iter;
				  
	que.AddFirst(item4);
	que.AddFirst(item3);
	que.AddFirst(item2);
	que.AddFirst(item1);
	test.Start(_L("PostFix ++"));
	iter=new Iter(que);
	test(&item1==(*iter)++);
	test(&item2.iLink==*(iter->sNext));
	test(&item2==(*iter)++);
	test(&item3.iLink==*(iter->sNext));
	test(&item3==(*iter)++);
	test(&item4.iLink==*(iter->sNext));
	test(&item4==(*iter)++);
	test((Item*) *(iter->sNext)==NULL);
	test((*iter)++==NULL);
	delete iter;
	test.Next(_L("Conversion Operator"));
	iter=new Iter(que);
	test(&item1==*iter);
	(*iter)++;
	test(&item2==*iter);
	(*iter)++;
	test(&item3==*iter);
	(*iter)++;
	test(&item4==*iter);
	(*iter)++;
	test(*iter==NULL);
	delete iter;
	test.Next(_L("Finished"));
	test.End();
	}
	
#ifndef _DEBUG
#pragma warning (disable: 4710)
#endif

GLDEF_C TInt E32Main()
    {

	TestTQueLink<TSglQueLink>* testSglQueLink;
	TestTQue<VSglQueBase>* testSglQueBase;
	TestTQue<VSglQue<TSglQueLink> >* testSglQue;
	TestTQueIter<Item,VSglQueIterBase>* testSglQueIterBase;
	TestTQueIter<Item,VSglQueIter<Item> >* testSglQueIter;

	TestTQueIter<ItemWithOffset,VSglQueIterBase>* testSglQueIterBaseOffset;
	TestTQueIter<ItemWithOffset,VSglQueIter<ItemWithOffset> >* testSglQueIterOffset;
 
	test.Title();
	test.Start(_L("class TSglQueLink"));
	testSglQueLink=new TestTQueLink<TSglQueLink>;
	testSglQueLink->TestQueLink();
	delete testSglQueLink;

	test.Next(_L("class TSglQueBase"));
	testSglQueBase=new TestTQue<VSglQueBase>;
	testSglQueBase->TestQueBase();
 	delete testSglQueBase;

	test.Next(_L("class TSlgQue"));
	testSglQue=new TestTQue<VSglQue<TSglQueLink> >;
	testSglQue->TestSglQue();
 	delete testSglQue;

	test.Next(_L("class TSglQueIterBase"));
	testSglQueIterBase=new TestTQueIter<Item,VSglQueIterBase>;
	testSglQueIterBase->TestIterBase();
 	delete testSglQueIterBase;

	test.Next(_L("class TSglQueIter"));
	testSglQueIter=new TestTQueIter<Item,VSglQueIter<Item> >;
	testSglQueIter->TestQueIter();
 	delete testSglQueIter;

	test.Next(_L("class TSglQueIterBase with Offset"));
	testSglQueIterBaseOffset=new TestTQueIter<ItemWithOffset,VSglQueIterBase>;
	testSglQueIterBaseOffset->TestIterBase();
 	delete testSglQueIterBaseOffset;

	test.Next(_L("class TSglQueIter with Offset"));
	testSglQueIterOffset=new TestTQueIter<ItemWithOffset,VSglQueIter<ItemWithOffset> >;
	testSglQueIterOffset->TestQueIter();
 	delete testSglQueIterOffset;

	test.Next(_L("Finished"));
	test.End();
	return(KErrNone);
    }
#pragma warning (default: 4710)


