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
// e32test\buffer\t_que.cpp
// Overview:
// Test double linked list functionality.
// API Information:
// TDblQueLinkBase, TDeltaQueLink, TDblQueLink, TPriQueLink,
// TDblQueIterBase, TDblQueIter
// Details:
// - Create many TDblQueLinkBase links, insert links at specified locations
// and check previous and next links are as expected.
// - Create many TDeltaQueLink links, insert these links at specified locations
// and check previous and next links are as expected.
// - Create many TDblQueLink links, insert, remove these links at specified
// locations and check previous and next links are as expected.
// - Create many TPriQueLink links, insert, remove these links at specified
// locations and check previous and next links are as expected.
// - Create TDblQueBase based object without offset, with specified offset 
// and check it is constructed as expected.
// - Create TDblQueBase based object, insert and remove TPriQueLink list 
// element, set priorities of TPriQueLink list elements, call IsEmpty 
// and Reset methods.
// - Create TDblQueBase based object without, with offset and check 
// it is constructed as expected
// - Create TDblQueBase based object, insert TDblQueLink links at 
// specified locations and check that they are added as expected.
// - Initialise TPriQueLink link with different priorities, insert 
// the elements in priority order and check that they are added 
// as expected.
// - Create TDblQueBase based object, check the double linked list for empty
// before inserting, after inserting, before/after Deque, before/after Enque,
// after Reset. Verify that results are as expected.
// - Create TDeltaQueBase object without offset, with specified offset
// and check it is constructed as expected. Insert TDeltaQueLink list 
// elements at specified distance from the zero point, remove the elements 
// at specified distance, decrement the delta value.
// - Create TDeltaQueBase based object, insert TDblQueLink link at specified 
// locations and check the previous, next link are as expected.
// - Check the linked list for empty before inserting, after inserting, 
// before/after Deque, before/after Enque, after Reset. Verify that results 
// are as expected.
// - Create TDeltaQueBase based object, insert links using DoAddDelta method,
// check that links are as expected. Delete links using DoRemove and 
// DoRemoveFirst methods and check that links are as expected.
// - Create TDeltaQueBase based object, insert links using DoAddDelta method,
// check the return value of CountDown is as expected. Delete links using 
// DoRemoveFirst method and check the return value of CountDown is as expected.
// - Create TDblQue based object, insert links at front and last, call IsHead, 
// IsFirst, IsLast, First and Last methods.
// - Create TDblQue object, check list for empty before inserting, after 
// inserting, before/after Deque, before/after Enque, after Reset. 
// Verify that results are as expected.
// - Create TDblQue based object with offset constructor, insert links at
// specified locations and check it is added as specified.
// - Create TDblQue based object, insert links using AddFirst and AddLast and
// check the links are as expected.
// - Create TDblQue based object, insert links using AddFirst and AddLast and
// check the result of the IsHead, IsFirst and IsLast methods are as expected.
// - Create TDblQue based object, insert links using AddFirst and AddLast, check 
// the results are as expected.
// - Create TPriQueLink list without offset, with different offset, check the 
// construction is as expected.
// - Create TPriQueLink list and insert many links at different specified location
// and check it is added as specified.
// - Create TPriQueLink link with different priorities, insert the elements in 
// priority order and check that they are added as expected.
// - Create TPriQueLink list, check the double linked list for empty before 
// inserting, after inserting, before/after Deque, before/after Enque, after Reset. 
// Verify that results are as expected.
// - Create TPriQueLink list with different offset, get the list offset and check it
// is as expected.
// - Create TPriQueLink list, insert many links with different priorities check it is 
// as expected.
// - Create TDeltaQueLink list, add ,remove links and check the links and check 
// it is as expected.
// - Create TDeltaQueLink list and insert many links at different specified location
// and check it is added as specified.
// - Create TDeltaQueLink list, check the double linked list for empty before, after 
// inserting, before Deque, after Enque, Reset method call is as expected.
// - Create TDeltaQueLink list, insert links using DoAddDelta method,check that 
// links are as expected. Delete links using DoRemove and DoRemoveFirst methods
// and check that links are as expected.
// - Create TDeltaQueLink based object, insert links using DoAddDelta method,
// check the return value of CountDown is as expected. Delete links using 
// DoRemoveFirst method and check the return value of CountDown is as expected.
// - Create TDeltaQueLink list with different offset, get and check the offset is 
// as expected.
// - Create TDeltaQueLink list, add, remove links at different specified location 
// and check it is added and removed successfully.
// - Initialaize TDblQueIterBase based iterator, get the current item in the queue, 
// move the current position forward, backward, set the iterator to point to the 
// first element, last item and check it is as expected.
// - Create TDblQueIterBase object with offset constructor, insert links at
// specified locations and check it is added as specified.
// - Create TDblQueIterBase object, iterate the list using operators, DoPostInc, 
// DoPostDec, DoCurrent and check it is as expected.
// - Create TDblQue based link with specified offset, initialize TDblQueIter 
// based iterator, iterate the link and check the offset is as expected.
// - Create TDblQueIter object with offset constructor, insert links at
// specified locations and check it is added as specified.
// - Create TDblQueIter object, iterate the list using operators, DoPostInc, 
// DoPostDec, DoCurrent and check it is as expected.
// - Create TDblQueIter based object, insert links using AddFirst and SetToLast,
// using IsHead, check the results are as expected.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_QUE"));

#ifndef _DEBUG
#pragma warning(disable : 4710) //fuction not expanded
#endif

#define SIZE 10
#define MAX_OFFSET 10

struct Item
	{
	TDblQueLink iLink;
	TInt iSpace[MAX_OFFSET]; // Reserve some space
	};

class CItem : public CBase
	{
public:
	TDblQueLink iLink;
	//int iSpac[MAX_OFFSET]; // Reserve some space
	};

template<class T>
class TestTQueLink
	{
public:
	void TestQueLinkBase();	// Calls Test: 1.
	void TestQueLink();	// Calls Test: 1,2.
	void Test1();	// Test Enque
	void Test2();	// Test Deque
protected:
	void CreateObjects(TInt aBaseLink);
	void DestroyObjects();
private:
	void CallTest1();
	T* iLink[SIZE];
	};

template<class T>
void TestTQueLink<T>::CallTest1()
	{
	test.Start(_L("Test Enque"));
	Test1();
	}

template<class T>
void TestTQueLink<T>::CreateObjects(TInt aBaseLink)
	{
	TInt i;

	for (i=0;i<SIZE;i++)
		iLink[i]=new T;
	if (aBaseLink>=0&&aBaseLink<SIZE)
		(iLink[aBaseLink])->iNext=(iLink[aBaseLink])->iPrev=iLink[aBaseLink];
	}

template<class T>
void TestTQueLink<T>::DestroyObjects()
	{
	TInt i;

	for (i=0;i<SIZE;i++)
		delete iLink[i];
	}

template<class T>
void TestTQueLink<T>::TestQueLinkBase()
	{
	CallTest1();
	test.Next(_L("Finished"));
	test.End();
	}

template <class T>
void TestTQueLink<T>::TestQueLink()
	{
	CallTest1();
	test.Next(_L("Text Deque"));
	Test2();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQueLink<T>::Test1()
	{
	CreateObjects(1);
	test.Start(_L("Check Next and Prev pointers set corectly."));
	iLink[2]->Enque(iLink[1]);
	test(iLink[1]->iPrev==iLink[2]);
	test(iLink[1]->iNext==iLink[2]);
	test(iLink[2]->iPrev==iLink[1]);
	test(iLink[2]->iNext==iLink[1]);
	iLink[3]->Enque(iLink[2]);
	test(iLink[1]->iPrev==iLink[3]);
	test(iLink[1]->iNext==iLink[2]);
	test(iLink[2]->iPrev==iLink[1]);
	test(iLink[2]->iNext==iLink[3]);
	test(iLink[3]->iPrev==iLink[2]);
	test(iLink[3]->iNext==iLink[1]);
	iLink[4]->Enque(iLink[3]);
	test(iLink[1]->iPrev==iLink[4]);
	test(iLink[1]->iNext==iLink[2]);
	test(iLink[2]->iPrev==iLink[1]);
	test(iLink[2]->iNext==iLink[3]);
	test(iLink[3]->iPrev==iLink[2]);
	test(iLink[3]->iNext==iLink[4]);
	test(iLink[4]->iPrev==iLink[3]);
	test(iLink[4]->iNext==iLink[1]);
	iLink[5]->Enque(iLink[2]);
	test(iLink[1]->iPrev==iLink[4]);
	test(iLink[1]->iNext==iLink[2]);
	test(iLink[2]->iPrev==iLink[1]);
	test(iLink[2]->iNext==iLink[5]);
	test(iLink[5]->iPrev==iLink[2]);
	test(iLink[5]->iNext==iLink[3]);
	test(iLink[3]->iPrev==iLink[5]);
	test(iLink[3]->iNext==iLink[4]);
	test(iLink[4]->iPrev==iLink[3]);
	test(iLink[4]->iNext==iLink[1]);
	test.Next(_L("Finished"));
	DestroyObjects();
	test.End();
	}

template<class T>
void TestTQueLink<T>::Test2()
	{
	CreateObjects(1);
	test.Start(_L("Check Next and Prev pointers set corectly"));
	iLink[2]->Enque(iLink[1]);
	iLink[3]->Enque(iLink[2]);
	iLink[4]->Enque(iLink[3]);
	iLink[5]->Enque(iLink[4]);
	iLink[5]->Deque();
	iLink[5]->Enque(iLink[2]);
	test(iLink[1]->iPrev==iLink[4]);
	test(iLink[1]->iNext==iLink[2]);
	test(iLink[2]->iPrev==iLink[1]);
	test(iLink[2]->iNext==iLink[5]);
	test(iLink[5]->iPrev==iLink[2]);
	test(iLink[5]->iNext==iLink[3]);
	test(iLink[3]->iPrev==iLink[5]);
	test(iLink[3]->iNext==iLink[4]);
	test(iLink[4]->iPrev==iLink[3]);
	test(iLink[4]->iNext==iLink[1]);
	iLink[3]->Deque();
	test(iLink[1]->iPrev==iLink[4]);
	test(iLink[1]->iNext==iLink[2]);
	test(iLink[2]->iPrev==iLink[1]);
	test(iLink[2]->iNext==iLink[5]);
	test(iLink[5]->iPrev==iLink[2]);
	test(iLink[5]->iNext==iLink[4]);
	test(iLink[4]->iPrev==iLink[5]);
	test(iLink[4]->iNext==iLink[1]);
	iLink[1]->Deque();
	test(iLink[2]->iPrev==iLink[4]);
	test(iLink[2]->iNext==iLink[5]);
	test(iLink[5]->iPrev==iLink[2]);
	test(iLink[5]->iNext==iLink[4]);
	test(iLink[4]->iPrev==iLink[5]);
	test(iLink[4]->iNext==iLink[2]);
	iLink[4]->Deque();
	test(iLink[2]->iPrev==iLink[5]);
	test(iLink[2]->iNext==iLink[5]);
	test(iLink[5]->iPrev==iLink[2]);
	test(iLink[5]->iNext==iLink[2]);
	test.Next(_L("Finished"));
	DestroyObjects();
	test.End();
	}

class VDblQueBase : public TDblQueBase
	{
public:
	VDblQueBase();
	VDblQueBase(TInt anOffset);
	inline void sDoAddFirst(TAny* aPtr) {DoAddFirst(aPtr);}
	inline void sDoAddLast(TAny* aPtr) {DoAddLast(aPtr);}
	inline void sDoAddPriority(TAny* aPtr) {DoAddPriority(aPtr);}
	inline void sTestEmpty() const {__DbgTestEmpty();}
	TDblQueLink* sHead;
	TInt* sOffset;
private:
	void SetMembers();
	};

class VDeltaQueBase : public TDeltaQueBase
	{
public:
	VDeltaQueBase();
	VDeltaQueBase(TInt anOffset);
	inline void sDoAddDelta(TAny* aPtr,TInt aDelta) {DoAddDelta(aPtr,aDelta);}		//From TDeltaQueBase
	inline void sDoRemove(TAny* aPtr) {this->DoRemove(aPtr);}
	inline TAny* sDoRemoveFirst() {return this->DoRemoveFirst();}
	TInt** sFirstDelta;
	inline void sDoAddFirst(TAny* aPtr) {DoAddFirst(aPtr);}		//From TDblQueBase
	inline void sDoAddLast(TAny* aPtr) {DoAddLast(aPtr);}
	inline void sDoAddPriority(TAny* aPtr) {DoAddPriority(aPtr);}
	TDblQueLink* sHead;
	TInt* sOffset;
private:
	void SetMembers();
	};

template <class T>
class VDblQue : public TDblQue<T>
	{
public:
	VDblQue();
	VDblQue(TInt anOffset);
	/*inline void sDoAddDelta(TAny* aPtr,TInt aDelta) {DoAddDelta(aPtr,aDelta);}		//From TDeltaQueBase
	inline void sDoRemove(TAny* aPtr) {this->DoRemove(aPtr);}
	inline TAny* sDoRemoveFirst() {return this->DoRemoveFirst();}
	TInt** sFirstDelta;*/
	inline void sDoAddFirst(TAny* aPtr) {this->DoAddFirst(aPtr);}		//From TDblQueBase
	inline void sDoAddLast(TAny* aPtr) {this->DoAddLast(aPtr);}
	inline void sDoAddPriority(TAny* aPtr) {this->DoAddPriority(aPtr);}
	TDblQueLink* sHead;
	TInt* sOffset;
private:
	void SetMembers();
	};

template <class T>
class VPriQue : public TPriQue<T>
	{
public:
	VPriQue();
	VPriQue(TInt anOffset);
	/*inline void sDoAddDelta(TAny* aPtr,TInt aDelta) {DoAddDelta(aPtr,aDelta);}		//From TDeltaQueBase
	inline void sDoRemove(TAny* aPtr) {this->DoRemove(aPtr);}
	inline TAny* sDoRemoveFirst() {return this->DoRemoveFirst();}
	TInt** sFirstDelta;*/
	inline void sDoAddFirst(TAny* aPtr) {this->DoAddFirst(aPtr);}		//From TDblQueBase
	inline void sDoAddLast(TAny* aPtr) {this->DoAddLast(aPtr);}
	inline void sDoAddPriority(TAny* aPtr) {this->DoAddPriority(aPtr);}
	TDblQueLink* sHead;
	TInt* sOffset;
private:
	void SetMembers();
	};

template <class T>
class VDeltaQue : public TDeltaQue<T>
	{
public:
	VDeltaQue();
	VDeltaQue(TInt anOffset);
	inline void sDoAddDelta(TAny* aPtr,TInt aDelta) {this->DoAddDelta(aPtr,aDelta);}		//From TDeltaQueBase
	inline void sDoRemove(TAny* aPtr) {this->DoRemove(aPtr);}
	inline TAny* sDoRemoveFirst() {return this->DoRemoveFirst();}
	TInt** sFirstDelta;
	inline void sDoAddFirst(TAny* aPtr) {this->DoAddFirst(aPtr);}		//From TDblQueBase
	inline void sDoAddLast(TAny* aPtr) {this->DoAddLast(aPtr);}
	inline void sDoAddPriority(TAny* aPtr) {this->DoAddPriority(aPtr);}
	TDblQueLink* sHead;
	TInt* sOffset;
private:
	void SetMembers();
	};

VDblQueBase::VDblQueBase()
	{
	SetMembers();
	}

VDblQueBase::VDblQueBase(TInt anOffset)
	:TDblQueBase(anOffset)
	{
	SetMembers();
	}

void VDblQueBase::SetMembers()
	{
	sHead=&iHead;
	sOffset=&iOffset;
	}

VDeltaQueBase::VDeltaQueBase()
	{
	SetMembers();
	}

VDeltaQueBase::VDeltaQueBase(TInt anOffset)
	:TDeltaQueBase(anOffset)
	{
	SetMembers();
	}

void VDeltaQueBase::SetMembers()
	{
	sFirstDelta=&iFirstDelta;
	sHead=&iHead;
	sOffset=&iOffset;
	}

template <class T>
VDblQue<T>::VDblQue()
	{
	SetMembers();
	}

template <class T>
VDblQue<T>::VDblQue(TInt anOffset)
	:TDblQue<T>(anOffset)
	{
	SetMembers();
	}

template <class T>
void VDblQue<T>::SetMembers()
	{
	//sFirstDelta=&iFirstDelta;
	sHead=&this->iHead;
	sOffset=&this->iOffset;
	}

template <class T>
VPriQue<T>::VPriQue()
	{
	SetMembers();
	}

template <class T>
VPriQue<T>::VPriQue(TInt anOffset)
	:TPriQue<T>(anOffset)
	{
	SetMembers();
	}

template <class T>
void VPriQue<T>::SetMembers()
	{
	//sFirstDelta=&iFirstDelta;
	sHead=&this->iHead;
	sOffset=&this->iOffset;
	}

template <class T>
VDeltaQue<T>::VDeltaQue()
	{
	SetMembers();
	}

template <class T>
VDeltaQue<T>::VDeltaQue(TInt anOffset)
	:TDeltaQue<T>(anOffset)
	{
	SetMembers();
	}

template <class T>
void VDeltaQue<T>::SetMembers()
	{
	sFirstDelta=&this->iFirstDelta;
	sHead=&this->iHead;
	sOffset=&this->iOffset;
	}

template<class T>
class TestTQue
	{
friend class TestTQueLink<TDblQueLinkBase>;
public:
	void TestQueBase();
	void TestDeltaBase();
	void TestDblQue();
	void TestPriQue();
	void TestDeltaQue();
	void Test1();	// All functions		//TDblQueBase functions
	void Test2();	// Constructors
	void Test3(TBool aTestPri);	// DoAdd's
	void Test4();	// Public functions
	void Test5();	// All functions		//TDblDeltaQueBase
	void Test6();	// Constructors
	void Test7();	// Do's
	void Test8();	// CountDown
	void Test9();	// All functions		//TDblQueBase
	void Test10();	// Constructors
	void Test11();	// Add's
	void Test12();	// Is's
	void Test13();	// Get's
	void Test14();	// All functions		//TPriQue
	void Test15();	// Constructors
	void Test16();	// Add
	void Test17();	// All functions		//TDeltaQue
	void Test18();	// Constructors
	void Test19();	// Add/Remove
private:
	void CallTest3_4(TBool aTestPri);	
	void CallTest7_8();	
	};

template<class T>
void TestTQue<T>::CallTest3_4(TBool aTestPri)
	{
	test.Next(_L("Test DoAdd's"));
	Test3(aTestPri);
	test.Next(_L("Test public functions"));
	Test4();
	}

template<class T>
void TestTQue<T>::CallTest7_8()
	{
	test.Next(_L("Test Do's"));
	Test7();
	test.Next(_L("CountDown"));
	Test8();
	}

template<class T>
void TestTQue<T>::TestQueBase()
	{
	test.Start(_L("Test all member functions (simply)"));
	Test1();
	test.Next(_L("Test Constructors"));
	Test2();
	CallTest3_4(ETrue);
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::TestDeltaBase()
	{
	test.Start(_L("Test all member functions (simply)"));
	Test5();
	CallTest3_4(EFalse);
	test.Next(_L("Test Constructors"));
	Test6();
	CallTest7_8();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::TestDblQue()
	{
	test.Start(_L("Test all member functions (simply)"));
	Test9();
	CallTest3_4(EFalse);
	test.Next(_L("Test Constructor"));
	Test10();
	test.Next(_L("Test Add's"));
	Test11();
	test.Next(_L("Test Is's"));
	Test12();
	test.Next(_L("Get's"));
	Test13();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::TestPriQue()
	{
	test.Start(_L("Test all member functions (simply)"));
	Test14();
	CallTest3_4(ETrue);
	test.Next(_L("Test Constructor"));
	Test15();
	test.Next(_L("Test Add"));
	Test16();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::TestDeltaQue()
	{
	test.Start(_L("Test all member functions (simply)"));
	Test17();
	CallTest3_4(EFalse);
	CallTest7_8();
	test.Next(_L("Test Constructor"));
	Test18();
	test.Next(_L("Test Add/Removes"));
	Test19();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test1()
	{
	T* que;
	TPriQueLink link1,link2;
	TInt offset=4;

	test.Start(_L("Constructors"));
	que=new VDblQueBase(offset);
	delete que;
	que=new VDblQueBase;
	//delete que;
	test.Next(_L("DoAdd's"));
	que->sDoAddFirst(&link1);
	link1.Deque();
	que->sDoAddLast(&link1);
	link1.iPriority=1;
	link2.iPriority=2;
	que->sDoAddPriority(&link2);
	test.Next(_L("Public"));
	que->IsEmpty();
	que->SetOffset(offset);
	que->Reset();
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
	que=new VDblQueBase();
	test(*(que->sOffset)==0);
	test(que->sHead->iNext==que->sHead);
	test(que->sHead->iPrev==que->sHead);
	delete que;
	test.Next(_L("Offset constructor"));
	for (offset=0;offset<40;offset+=4)
		{
		que=new VDblQueBase(offset);
		test(*(que->sOffset)==offset);
		test(que->sHead->iNext==que->sHead);
		test(que->sHead->iPrev==que->sHead);
		delete que;
		}
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test3(TBool aTestPri)
	{
	T* que;
	TDblQueLink link1,link2,link3,link4;

	test.Start(_L("AddFirst"));
	que=new T();
	que->sDoAddFirst(&link1);
	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link1);
	test(link1.iNext==que->sHead);
	test(link1.iPrev==que->sHead);
 	que->sDoAddFirst(&link2);
	test(que->sHead->iNext==&link2);
	test(que->sHead->iPrev==&link1);
	test(link1.iNext==que->sHead);
	test(link1.iPrev==&link2);
	test(link2.iNext==&link1);
	test(link2.iPrev==que->sHead);
  	que->sDoAddFirst(&link3);
	test(que->sHead->iNext==&link3);
	test(que->sHead->iPrev==&link1);
	test(link1.iNext==que->sHead);
	test(link1.iPrev==&link2);
	test(link2.iNext==&link1);
	test(link2.iPrev==&link3);
	test(link3.iNext==&link2);
	test(link3.iPrev==que->sHead);
  	que->sDoAddFirst(&link4);
	test(que->sHead->iNext==&link4);
	test(que->sHead->iPrev==&link1);
	test(link1.iNext==que->sHead);
	test(link1.iPrev==&link2);
	test(link2.iNext==&link1);
	test(link2.iPrev==&link3);
	test(link3.iNext==&link2);
	test(link3.iPrev==&link4);
	test(link4.iNext==&link3);
	test(link4.iPrev==que->sHead);
	link1.Deque();
	link2.Deque();
	link3.Deque();
	link4.Deque();
	delete que;
	test.Next(_L("AddLast"));
	que=new T();
	que->sDoAddLast(&link1);
	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link1);
	test(link1.iNext==que->sHead);
	test(link1.iPrev==que->sHead);
	que->sDoAddLast(&link2);
	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link2);
	test(link1.iNext==&link2);
	test(link1.iPrev==que->sHead);
	test(link2.iNext==que->sHead);
	test(link2.iPrev==&link1);
	que->sDoAddLast(&link3);
	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link3);
	test(link1.iNext==&link2);
	test(link1.iPrev==que->sHead);
	test(link2.iNext==&link3);
	test(link2.iPrev==&link1);
	test(link3.iNext==que->sHead);
	test(link3.iPrev==&link2);
	que->sDoAddLast(&link4);
	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link4);
	test(link1.iNext==&link2);
	test(link1.iPrev==que->sHead);
	test(link2.iNext==&link3);
	test(link2.iPrev==&link1);
	test(link3.iNext==&link4);
	test(link3.iPrev==&link2);
	test(link4.iNext==que->sHead);
	test(link4.iPrev==&link3);
	link1.Deque();
	link2.Deque();
	link3.Deque();
	link4.Deque();
 	delete que;
	test.Next(_L("Combined AddFirst and AddLast"));
 	que=new T();
	que->sDoAddFirst(&link1);
	que->sDoAddLast(&link2);
 	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link2);
	test(link1.iNext==&link2);
	test(link1.iPrev==que->sHead);
	test(link2.iNext==que->sHead);
	test(link2.iPrev==&link1);
	que->sDoAddFirst(&link3);
 	test(que->sHead->iNext==&link3);
	test(que->sHead->iPrev==&link2);
	test(link1.iNext==&link2);
	test(link1.iPrev==&link3);
	test(link2.iNext==que->sHead);
	test(link2.iPrev==&link1);
	test(link3.iNext==&link1);
	test(link3.iPrev==que->sHead);
	que->sDoAddLast(&link4);
  	test(que->sHead->iNext==&link3);
	test(que->sHead->iPrev==&link4);
	test(link1.iNext==&link2);
	test(link1.iPrev==&link3);
	test(link2.iNext==&link4);
	test(link2.iPrev==&link1);
	test(link3.iNext==&link1);
	test(link3.iPrev==que->sHead);
	test(link4.iNext==que->sHead);
	test(link4.iPrev==&link2);
	link1.Deque();
	link2.Deque();
	link3.Deque();
	link4.Deque();
 	delete que;
	if (aTestPri)
	{
		TPriQueLink link5,link6,link7,link8,link9,link10;

		test.Next(_L("AddPriority"));
	 	que=new T();
		link5.iPriority=4;
		link6.iPriority=6;
		link7.iPriority=8;
		que->sDoAddPriority(&link5);
		que->sDoAddPriority(&link6);
		que->sDoAddPriority(&link7);
  		test(que->sHead->iNext==&link7);
		test(que->sHead->iPrev==&link5);
		test(link5.iNext==que->sHead);
		test(link5.iPrev==&link6);
		test(link6.iNext==&link5);
		test(link6.iPrev==&link7);
		test(link7.iNext==&link6);
		test(link7.iPrev==que->sHead);
		link8.iPriority=7;
		que->sDoAddPriority(&link8);
  		test(que->sHead->iNext==&link7);
		test(que->sHead->iPrev==&link5);
		test(link5.iNext==que->sHead);
		test(link5.iPrev==&link6);
		test(link6.iNext==&link5);
		test(link6.iPrev==&link8);
		test(link7.iNext==&link8);
		test(link7.iPrev==que->sHead);
		test(link8.iPrev==&link7);
		test(link8.iNext==&link6);
		link9.iPriority=5;
		que->sDoAddPriority(&link9);
  		test(que->sHead->iNext==&link7);
		test(que->sHead->iPrev==&link5);
		test(link5.iNext==que->sHead);
		test(link5.iPrev==&link9);
		test(link6.iNext==&link9);
		test(link6.iPrev==&link8);
		test(link7.iNext==&link8);
		test(link7.iPrev==que->sHead);
		test(link8.iPrev==&link7);
		test(link8.iNext==&link6);
		test(link9.iPrev==&link6);
		test(link9.iNext==&link5);
		link10.iPriority=3;
		que->sDoAddPriority(&link10);
  		test(que->sHead->iNext==&link7);
		test(que->sHead->iPrev==&link10);
		test(link5.iNext==&link10);
		test(link5.iPrev==&link9);
		test(link6.iNext==&link9);
		test(link6.iPrev==&link8);
		test(link7.iNext==&link8);
		test(link7.iPrev==que->sHead);
		test(link8.iPrev==&link7);
		test(link8.iNext==&link6);
		test(link9.iPrev==&link6);
		test(link9.iNext==&link5);
 		test(link10.iNext==que->sHead);
		test(link10.iPrev==&link5);
		link5.Deque();
		link6.Deque();
		link7.Deque();
		link8.Deque();
	 	delete que;
		}
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
	TDblQueLink link1,link2;
	que->sDoAddFirst(&link1);
	test(que->IsEmpty()==FALSE);
	link1.Deque();
	test(que->IsEmpty()==TRUE);
	que->sDoAddLast(&link2);
	test(que->IsEmpty()==FALSE);
	link1.Enque(&link2);
	test(que->IsEmpty()==FALSE);
	link2.Deque();
	test(que->IsEmpty()==FALSE);
 	link1.Deque();
	test(que->IsEmpty()==TRUE);
	test.Next(_L("Reset"));
	que->sDoAddFirst(&link1);
	test(que->IsEmpty()==FALSE);
	que->Reset();
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
	TDeltaQueLink link1,link2,link3;
	TInt offset=4;

	test.Start(_L("Constructors"));
	que=new VDeltaQueBase(offset);
	delete que;
	que=new VDeltaQueBase;
	test.Next(_L("Do's"));
	que->sDoAddDelta(&link1,3);
	que->sDoAddDelta(&link2,2);
	que->sDoAddDelta(&link3,0);
	que->sDoRemoveFirst();
	que->sDoRemove(&link2);
	test.Next(_L("CountDown"));
	que->CountDown();
	test.Next(_L("Finished"));
	delete que;
	test.End();
	}

template<class T>
void TestTQue<T>::Test6()
	{
	T* que;
	TInt offset;

	test.Start(_L("Default constructor"));
	que=new VDeltaQueBase();
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
	}

template<class T>
void TestTQue<T>::Test7()
	{
	T* que;
	TDeltaQueLink link1,link2,link3,link4,link5,link6;

	test.Start(_L("DoAddDelta"));
	que=new T();
	que->sDoAddDelta(&link2,3);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	que->sDoAddDelta(&link5,15);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link5.iDelta==12);
	que->sDoAddDelta(&link3,6);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link3.iDelta==3);
	test(link5.iDelta==9);
	que->sDoAddDelta(&link4,10);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link3.iDelta==3);
	test(link4.iDelta==4);
	test(link5.iDelta==5);
	que->sDoAddDelta(&link1,1);
	test(*(que->sFirstDelta)==&link1.iDelta);
	test(link1.iDelta==1);
	test(link2.iDelta==2);
	test(link3.iDelta==3);
	test(link4.iDelta==4);
	test(link5.iDelta==5);
	que->sDoAddDelta(&link6,21);
	test(*(que->sFirstDelta)==&link1.iDelta);
	test(link1.iDelta==1);
	test(link2.iDelta==2);
	test(link3.iDelta==3);
	test(link4.iDelta==4);
	test(link5.iDelta==5);
	test(link6.iDelta==6);
	test.Next(_L("DoRemove"));
	que->sDoRemove(&link6);
	test(*(que->sFirstDelta)==&link1.iDelta);
	test(link1.iDelta==1);
	test(link2.iDelta==2);
	test(link3.iDelta==3);
	test(link4.iDelta==4);
	test(link5.iDelta==5);
 	que->sDoRemove(&link1);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link3.iDelta==3);
	test(link4.iDelta==4);
	test(link5.iDelta==5);
	que->sDoRemove(&link4);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link3.iDelta==3);
	test(link5.iDelta==9);
	que->sDoRemove(&link3);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link5.iDelta==12);
	que->sDoRemove(&link5);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test.Next(_L("DoRemoveFirst"));
	test(NULL==que->sDoRemoveFirst());
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	link2.iDelta=1;
	test(NULL==que->sDoRemoveFirst());
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==1);
	link2.iDelta=0;
	test(&link2==que->sDoRemoveFirst());
	delete que;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test8()
	{
	T* que;
	TDeltaQueLink link1,link2,link3;

	que=new T();
	que->sDoAddDelta(&link1,1);
	que->sDoAddDelta(&link2,3);
	que->sDoAddDelta(&link3,6);
	test(que->CountDown()==TRUE);
	que->sDoRemoveFirst();
	test(que->CountDown()==FALSE);
	test(que->CountDown()==TRUE);
	que->sDoRemoveFirst();
	test(que->CountDown()==FALSE);
	test(que->CountDown()==FALSE);
	test(que->CountDown()==TRUE);
	que->sDoRemoveFirst();
	delete que;
	}

template<class T>
void TestTQue<T>::Test9()
	{
	T* que;
	TDblQueLink link1,link2;
	TInt offset=4;

	test.Start(_L("Constructors"));
	que=new VDblQue<TDblQueLink>(offset);
	delete que;
	que=new VDblQue<TDblQueLink>;
	test.Next(_L("Add's"));
	que->AddFirst(link1);
	que->AddLast(link2);
	test.Next(_L("Is's"));
	que->IsHead(que->sHead);
	que->IsFirst(&link1);
	que->IsLast(&link2);
	test.Next(_L("Get's"));
	que->First();
	que->Last();
	test.Next(_L("Finished"));
	delete que;
	test.End();
	}

template<class T>
void TestTQue<T>::Test10()
	{
	T* que;
	TInt offset;

	test.Start(_L("Offset constructor"));
	for (offset=0;offset<40;offset+=4)
		{
		que=new VDblQue<TDblQueLink>(offset);
		test(*(que->sOffset)==offset);
		delete que;
		}
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test11()
	{
	T* que;
	TDblQueLink link1,link2,link3,link4;

	test.Start(_L("AddFirst"));
	que=new T();
	que->AddFirst(link1);
	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link1);
	test(link1.iNext==que->sHead);
	test(link1.iPrev==que->sHead);
 	que->AddFirst(link2);
	test(que->sHead->iNext==&link2);
	test(que->sHead->iPrev==&link1);
	test(link1.iNext==que->sHead);
	test(link1.iPrev==&link2);
	test(link2.iNext==&link1);
	test(link2.iPrev==que->sHead);
  	que->AddFirst(link3);
	test(que->sHead->iNext==&link3);
	test(que->sHead->iPrev==&link1);
	test(link1.iNext==que->sHead);
	test(link1.iPrev==&link2);
	test(link2.iNext==&link1);
	test(link2.iPrev==&link3);
	test(link3.iNext==&link2);
	test(link3.iPrev==que->sHead);
  	que->AddFirst(link4);
	test(que->sHead->iNext==&link4);
	test(que->sHead->iPrev==&link1);
	test(link1.iNext==que->sHead);
	test(link1.iPrev==&link2);
	test(link2.iNext==&link1);
	test(link2.iPrev==&link3);
	test(link3.iNext==&link2);
	test(link3.iPrev==&link4);
	test(link4.iNext==&link3);
	test(link4.iPrev==que->sHead);
	link1.Deque();
	link2.Deque();
	link3.Deque();
	link4.Deque();
	delete que;
	test.Next(_L("AddLast"));
	que=new T();
	que->AddLast(link1);
	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link1);
	test(link1.iNext==que->sHead);
	test(link1.iPrev==que->sHead);
	que->AddLast(link2);
	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link2);
	test(link1.iNext==&link2);
	test(link1.iPrev==que->sHead);
	test(link2.iNext==que->sHead);
	test(link2.iPrev==&link1);
	que->AddLast(link3);
	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link3);
	test(link1.iNext==&link2);
	test(link1.iPrev==que->sHead);
	test(link2.iNext==&link3);
	test(link2.iPrev==&link1);
	test(link3.iNext==que->sHead);
	test(link3.iPrev==&link2);
	que->AddLast(link4);
	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link4);
	test(link1.iNext==&link2);
	test(link1.iPrev==que->sHead);
	test(link2.iNext==&link3);
	test(link2.iPrev==&link1);
	test(link3.iNext==&link4);
	test(link3.iPrev==&link2);
	test(link4.iNext==que->sHead);
	test(link4.iPrev==&link3);
	link1.Deque();
	link2.Deque();
	link3.Deque();
	link4.Deque();
 	delete que;
	test.Next(_L("Combined AddFirst and AddLast"));
 	que=new T();
	que->AddFirst(link1);
	que->AddLast(link2);
 	test(que->sHead->iNext==&link1);
	test(que->sHead->iPrev==&link2);
	test(link1.iNext==&link2);
	test(link1.iPrev==que->sHead);
	test(link2.iNext==que->sHead);
	test(link2.iPrev==&link1);
	que->AddFirst(link3);
 	test(que->sHead->iNext==&link3);
	test(que->sHead->iPrev==&link2);
	test(link1.iNext==&link2);
	test(link1.iPrev==&link3);
	test(link2.iNext==que->sHead);
	test(link2.iPrev==&link1);
	test(link3.iNext==&link1);
	test(link3.iPrev==que->sHead);
	que->AddLast(link4);
  	test(que->sHead->iNext==&link3);
	test(que->sHead->iPrev==&link4);
	test(link1.iNext==&link2);
	test(link1.iPrev==&link3);
	test(link2.iNext==&link4);
	test(link2.iPrev==&link1);
	test(link3.iNext==&link1);
	test(link3.iPrev==que->sHead);
	test(link4.iNext==que->sHead);
	test(link4.iPrev==&link2);
	link1.Deque();
	link2.Deque();
	link3.Deque();
	link4.Deque();
 	delete que;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test12()
	{
	T* que;
	TDblQueLink link1,link2,link3,link4,*head;

	que=new T();
	que->AddFirst(link1);
 	que->AddLast(link2);
 	que->AddLast(link3);
  	que->AddLast(link4);
	head=que->sHead;
	test.Start(_L("IsHead"));
	test(que->IsHead(head)==TRUE);
	test(que->IsHead(&link1)==FALSE);
	test(que->IsHead(&link2)==FALSE);
	test(que->IsHead(&link3)==FALSE);
	test(que->IsHead(&link4)==FALSE);
	test.Next(_L("IsFirst"));
	test(que->IsFirst(head)==FALSE);
	test(que->IsFirst(&link1)==TRUE);
	test(que->IsFirst(&link2)==FALSE);
	test(que->IsFirst(&link3)==FALSE);
	test(que->IsFirst(&link4)==FALSE);
	test.Next(_L("IsLast"));
	test(que->IsLast(head)==FALSE);
	test(que->IsLast(&link1)==FALSE);
	test(que->IsLast(&link2)==FALSE);
	test(que->IsLast(&link3)==FALSE);
	test(que->IsLast(&link4)==TRUE);
 	delete que;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test13()
	{
	T* que;
	TDblQueLink link1,link2,link3,link4;

	test.Start(_L("First"));
	que=new T();
	que->AddFirst(link1);
	test(que->First()==&link1);
 	que->AddFirst(link2);
	test(que->First()==&link2);
 	que->AddFirst(link3);
	test(que->First()==&link3);
  	que->AddFirst(link4);
	test(que->First()==&link4);
	link1.Deque();
	link2.Deque();
	link3.Deque();
	link4.Deque();
 	delete que;
	test.Next(_L("Last"));
	que=new T();
	que->AddLast(link1);
	test(que->Last()==&link1);
 	que->AddLast(link2);
	test(que->Last()==&link2);
 	que->AddLast(link3);
	test(que->Last()==&link3);
  	que->AddLast(link4);
	test(que->Last()==&link4);
	link1.Deque();
	link2.Deque();
	link3.Deque();
	link4.Deque();
 	delete que;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test14()
	{
	T* que;
	TPriQueLink link;
	TInt offset=4;

	test.Start(_L("Constructors"));
	que=new VPriQue<TPriQueLink>(offset);
	delete que;
	que=new VPriQue<TPriQueLink>;
	test.Next(_L("Add"));
	que->Add(link);
	test.Next(_L("Finished"));
	delete que;
	test.End();
	}

template<class T>
void TestTQue<T>::Test15()
	{
	T* que;
	TInt offset;

	test.Start(_L("Offset constructor"));
	for (offset=0;offset<40;offset+=4)
		{
		que=new VPriQue<TPriQueLink>(offset);
		test(*(que->sOffset)==offset);
		delete que;
		}
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test16()
	{
	T* que;
	TPriQueLink link1,link2,link3,link4,link5,link6;

	que=new T();
	link1.iPriority=4;
	link2.iPriority=6;
	link3.iPriority=2;
	que->sDoAddPriority(&link1);
	que->sDoAddPriority(&link2);
	que->sDoAddPriority(&link3);
  	test(que->sHead->iNext==&link2);
	test(que->sHead->iPrev==&link3);
	test(link1.iNext==&link3);
	test(link1.iPrev==&link2);
	test(link2.iNext==&link1);
	test(link2.iPrev==que->sHead);
	test(link3.iNext==que->sHead);
	test(link3.iPrev==&link1);
	link4.iPriority=3;
	que->sDoAddPriority(&link4);
  	test(que->sHead->iNext==&link2);
	test(que->sHead->iPrev==&link3);
	test(link1.iNext==&link4);
	test(link1.iPrev==&link2);
	test(link2.iNext==&link1);
	test(link2.iPrev==que->sHead);
	test(link3.iNext==que->sHead);
	test(link3.iPrev==&link4);
	test(link4.iNext==&link3);
	test(link4.iPrev==&link1);
	link5.iPriority=5;
	que->sDoAddPriority(&link5);
  	test(que->sHead->iNext==&link2);
	test(que->sHead->iPrev==&link3);
	test(link1.iNext==&link4);
	test(link1.iPrev==&link5);
	test(link2.iNext==&link5);
	test(link2.iPrev==que->sHead);
	test(link3.iNext==que->sHead);
	test(link3.iPrev==&link4);
	test(link4.iNext==&link3);
	test(link4.iPrev==&link1);
	test(link5.iNext==&link1);
	test(link5.iPrev==&link2);
	link6.iPriority=1;
	que->sDoAddPriority(&link6);
  	test(que->sHead->iNext==&link2);
	test(que->sHead->iPrev==&link6);
	test(link1.iNext==&link4);
	test(link1.iPrev==&link5);
	test(link2.iNext==&link5);
	test(link2.iPrev==que->sHead);
	test(link3.iNext==&link6);
	test(link3.iPrev==&link4);
	test(link4.iNext==&link3);
	test(link4.iPrev==&link1);
	test(link5.iNext==&link1);
	test(link5.iPrev==&link2);
	test(link6.iNext==que->sHead);
	test(link6.iPrev==&link3);
	delete que;
	}

template<class T>
void TestTQue<T>::Test17()
	{
	T* que;
	TDeltaQueLink link1,link2;
	TInt offset=4;

	test.Start(_L("Constructors"));
	que=new VDeltaQue<TDeltaQueLink>(offset);
	delete que;
	que=new VDeltaQue<TDeltaQueLink>;
	test.Next(_L("Add/Remove"));
	que->Add(link1,0);
	que->Add(link2,2);
	que->Remove(link2);
	que->RemoveFirst();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test18()
	{
	T* que;
	TInt offset;

	test.Start(_L("Offset constructor"));
	for (offset=0;offset<40;offset+=4)
		{
		que=new VDeltaQue<TDeltaQueLink>(offset);
		test(*(que->sOffset)==offset);
		delete que;
		}
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQue<T>::Test19()
	{
  	T* que;
	TDeltaQueLink link1,link2,link3,link4,link5,link6;

	test.Start(_L("Add"));
	que=new T();
	que->Add(link2,3);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	que->Add(link5,15);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link5.iDelta==12);
	que->Add(link3,6);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link3.iDelta==3);
	test(link5.iDelta==9);
	que->Add(link4,10);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link3.iDelta==3);
	test(link4.iDelta==4);
	test(link5.iDelta==5);
	que->Add(link1,1);
	test(*(que->sFirstDelta)==&link1.iDelta);
	test(link1.iDelta==1);
	test(link2.iDelta==2);
	test(link3.iDelta==3);
	test(link4.iDelta==4);
	test(link5.iDelta==5);
	que->Add(link6,21);
	test(*(que->sFirstDelta)==&link1.iDelta);
	test(link1.iDelta==1);
	test(link2.iDelta==2);
	test(link3.iDelta==3);
	test(link4.iDelta==4);
	test(link5.iDelta==5);
	test(link6.iDelta==6);
	test.Next(_L("Remove"));
	que->Remove(link6);
	test(*(que->sFirstDelta)==&link1.iDelta);
	test(link1.iDelta==1);
	test(link2.iDelta==2);
	test(link3.iDelta==3);
	test(link4.iDelta==4);
	test(link5.iDelta==5);
 	que->Remove(link1);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link3.iDelta==3);
	test(link4.iDelta==4);
	test(link5.iDelta==5);
	que->Remove(link4);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link3.iDelta==3);
	test(link5.iDelta==9);
	que->Remove(link3);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test(link5.iDelta==12);
	que->Remove(link5);
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	test.Next(_L("RemoveFirst"));
	test(NULL==que->RemoveFirst());
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==3);
	link2.iDelta=1;
	test(NULL==que->RemoveFirst());
	test(*(que->sFirstDelta)==&link2.iDelta);
	test(link2.iDelta==1);
	link2.iDelta=0;
	test(&link2==que->RemoveFirst());
	test.Next(_L("Finished"));
	delete que;
	test.End();
	}

class VDblQueIterBase : public TDblQueIterBase
	{
public:
	VDblQueIterBase(TDblQueBase& aQue);
	inline TAny* sDoPostInc() {return DoPostInc();}
	inline TAny* sDoPostDec() {return DoPostDec();}
	inline TAny* sDoCurrent() {return DoCurrent();}
	TInt* sOffset;
	TDblQueLinkBase** sHead;
	TDblQueLinkBase** sNext;
private:
	void SetMember();
	};

template <class T>
class VDblQueIter : public TDblQueIter<T>
	{
public:
	VDblQueIter(TDblQueBase& aQue);
	inline TAny* sDoPostInc() {return this->DoPostInc();}
	inline TAny* sDoPostDec() {return this->DoPostDec();}
	inline TAny* sDoCurrent() {return this->DoCurrent();}
	TInt* sOffset;
	TDblQueLinkBase** sHead;
	TDblQueLinkBase** sNext;
private:
	void SetMember();
	};

VDblQueIterBase::VDblQueIterBase(TDblQueBase& aQue)
	:TDblQueIterBase(aQue)
	{
	SetMember();
	}

void VDblQueIterBase::SetMember()
	{
	sOffset=&iOffset;
	sHead=&iHead;
	sNext=&iNext;
	}

template <class T>
VDblQueIter<T>::VDblQueIter(TDblQueBase& aQue)
	:TDblQueIter<T>(aQue)
	{
	SetMember();
	}

template <class T>
void VDblQueIter<T>::SetMember()
	{
	sOffset=&this->iOffset;
	sHead=&this->iHead;
	sNext=&this->iNext;
	}
	
template<class T>
class TestTQueIter
	{
public:
	void TestIterBase();
	void TestQueIter();
	void Test1();	//All functions			//TDblQueIterBase
	void Test2();	//Constructors
	void Test3();	//Do's
	void Test4();	//Set
	void Test5();	//All functions			//TDblQueIter
	void Test1Item();
	//void Test6();	//Constructors										//Redundant
	void Test7();	//Iterators
private:
	void CallTest2_4();
	};

template<class T>
void TestTQueIter<T>::CallTest2_4()
	{
	test.Next(_L("Constructors"));
	Test2();
	test.Next(_L("Do's"));
	Test3();
	test.Next(_L("Sets"));
	Test4();
	}

template<class T>
void TestTQueIter<T>::TestIterBase()
	{
	test.Start(_L("All Methods"));
	Test1();
	CallTest2_4();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQueIter<T>::TestQueIter()
	{
	test.Start(_L("All Methods"));
	Test5();
	CallTest2_4();
	test.Next(_L("One item in queue"));
	Test1Item();
	test.Next(_L("Iterators"));
	Test7();
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQueIter<T>::Test1()
	{
	Item item1,item2;
	TDblQue<Item> que;
	T* iter;

	que.AddFirst(item2);
	que.AddFirst(item1);
	test.Start(_L("Constructor"));
	iter=new VDblQueIterBase(que);
	test.Next(_L("Do's"));
	iter->sDoCurrent();
	iter->sDoPostInc();
	iter->sDoPostDec();
	test.Next(_L("Sets"));
	iter->SetToFirst();
	iter->SetToLast();
	delete iter;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQueIter<T>::Test2()
	{
	Item item;
	TDblQue<Item>* que;
	TInt offset;
 	T* iter;

	for(offset=0;offset<40;offset+=4)
		{
		que=new TDblQue<Item>(offset);
		new(PtrAdd(&item.iLink,offset)) TDblQueLink();	// create the link at this offset
		iter=new T(*que);
		test(que->IsHead((Item*) PtrSub(*(iter->sHead),offset)));		//Need to pass a pointer to a item
		test(que->IsHead((Item*) PtrSub(*(iter->sNext),offset)));
		test(*(iter->sOffset)==offset);
		delete iter;
		delete que;
		que=new TDblQue<Item>(offset);
		new(PtrAdd(&item.iLink,offset)) TDblQueLink();	// create the link at this offset
		que->AddFirst(item);
		iter=new T(*que);
		test(que->IsHead((Item*) PtrSub(*(iter->sHead),offset)));
		test(*(iter->sNext)==PtrAdd(&item.iLink,offset));						//Need a pointer to a link
		test(*(iter->sOffset)==offset);
		PtrAdd(&item.iLink,offset)->Deque();
		delete iter;
		delete que;
		}
	}

template<class T>
void TestTQueIter<T>::Test3()
	{
	Item item1,item2,item3,item4;
	TDblQue<Item> que;
 	T* iter;
				  
	que.AddFirst(item4);
	que.AddFirst(item3);
	que.AddFirst(item2);
	que.AddFirst(item1);
	test.Start(_L("DoPostInc"));
	iter=new T(que);
	test(&item1==iter->sDoPostInc());
	test(&item2.iLink==*(iter->sNext));
	test(&item2==iter->sDoPostInc());
	test(&item3.iLink==*(iter->sNext));
	test(&item3==iter->sDoPostInc());
	test(&item4.iLink==*(iter->sNext));
	test(&item4==iter->sDoPostInc());
	test(que.IsHead((Item*) *(iter->sNext)));
	test(iter->sDoPostInc()==NULL);
	delete iter;
	test.Next(_L("DoPostDec"));
	iter=new T(que);
	iter->sDoPostInc();
	iter->sDoPostInc();
	iter->sDoPostInc();
	test(&item4.iLink==*(iter->sNext));
	test(&item4==iter->sDoPostDec());
	test(&item3.iLink==*(iter->sNext));
	test(&item3==iter->sDoPostDec());
	test(&item2.iLink==*(iter->sNext));
	test(&item2==iter->sDoPostDec());
	test(&item1.iLink==*(iter->sNext));
	test(&item1==iter->sDoPostDec());
	test(que.IsHead((Item*) *(iter->sNext)));
	test(iter->sDoPostDec()==NULL);
	delete iter;
	test.Next(_L("DoCurrent"));
	iter=new T(que);
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

template<class T>
void TestTQueIter<T>::Test4()
	{
	Item item1,item2,item3,item4;
	TDblQue<Item> que;
 	T* iter;
	TInt i,j;

	que.AddFirst(item4);
	que.AddFirst(item3);
	que.AddFirst(item2);
	que.AddFirst(item1);
	test.Start(_L("SetToFirst"));
	iter=new T(que);
	for(i=0;i<5;i++)
		{
		for(j=0;j<i;j++)
			iter->sDoPostInc();
		iter->SetToFirst();
		test(*(iter->sNext)==&item1.iLink);
		}
	delete iter;
	test.Next(_L("SetToLast"));
	iter=new T(que);
	for(i=0;i<5;i++)
		{
		iter->SetToFirst();
		for(j=0;j<i;j++)
			iter->sDoPostInc();
		iter->SetToLast();
		test(*(iter->sNext)==&item4.iLink);
		}
	delete iter;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQueIter<T>::Test5()
	{
	Item item1,item2;
	TDblQue<Item> que;
	T* iter;

	que.AddFirst(item2);
	que.AddFirst(item1);
	test.Start(_L("Constructor"));
	iter=new T(que);
	test.Next(_L("Iterators"));
	test(((*iter)++)==&item1);
	test(((*iter)--)==&item2);
	test(((*iter)++)==&item1);
	test(((*iter)++)==&item2);
	test(((*iter)++)==NULL);
	test(((*iter)++)==NULL);
	test(((*iter)--)==NULL);
	test(((*iter)--)==NULL);
	iter->Set(item2);
	test(((*iter)--)==&item2);
	test(((*iter)++)==&item1);
	test(((*iter)--)==&item2);
	test(((*iter)--)==&item1);
	test(((*iter)++)==NULL);
	test(((*iter)++)==NULL);
	test(((*iter)--)==NULL);
	test(((*iter)--)==NULL);
	delete iter;
	test.Next(_L("Finished"));
	test.End();
	}

template<class T>
void TestTQueIter<T>::Test1Item()
	{
	Item item;
	TDblQue<Item> que;
	T* iter;

	test.Start(_L("Constructor"));
	iter=new T(que);
	que.AddFirst(item);
	iter->Set(item);	
	test.Next(_L("Iterators"));
	test(((*iter)++)==&item);
	test(((*iter)++)==NULL);
	test(((*iter)++)==NULL);
	test(((*iter)++)==NULL);
	test(((*iter)--)==NULL);
	test(((*iter)--)==NULL);
	iter->Set(item);	
	test(((*iter)--)==&item);
	test(((*iter)++)==NULL);
	test(((*iter)--)==NULL);
	test(((*iter)--)==NULL);
	test(((*iter)--)==NULL);
	test(((*iter)++)==NULL);
	test(((*iter)++)==NULL);
	delete iter;
	test.Next(_L("Finished"));
	test.End();
	}

/*template<class T>					//Redundant
void TestTQueIter<T>::Test6()
	{
	Item item;
	TDblQue<Item>* que;
	TInt offset;
 	T* iter;

	for(offset=0;offset<40;offset+=4)
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

template<class T>
void TestTQueIter<T>::Test7()
	{
	Item item1,item2,item3,item4;
	TDblQue<Item> que;
 	T* iter;
				  
	que.AddFirst(item4);
	que.AddFirst(item3);
	que.AddFirst(item2);
	que.AddFirst(item1);
	test.Start(_L("PostFix ++"));
	iter=new T(que);
	test(&item1==(*iter)++);
	test(&item2.iLink==*(iter->sNext));
	test(&item2==(*iter)++);
	test(&item3.iLink==*(iter->sNext));
	test(&item3==(*iter)++);
	test(&item4.iLink==*(iter->sNext));
	test(&item4==(*iter)++);
	test(que.IsHead((Item*) *(iter->sNext)));
	test((*iter)++==NULL);
	delete iter;
	test.Next(_L("PostFix --"));
	iter=new T(que);
	iter->SetToLast();
	test(&item4.iLink==*(iter->sNext));
	test(&item4==(*iter)--);
	test(&item3.iLink==*(iter->sNext));
	test(&item3==(*iter)--);
	test(&item2.iLink==*(iter->sNext));
	test(&item2==(*iter)--);
	test(&item1.iLink==*(iter->sNext));
	test(&item1==(*iter)--);
	test(que.IsHead((Item*) *(iter->sNext)));
	test((*iter)--==NULL);
	delete iter;
	test.Next(_L("Conversion Operator"));
	iter=new T(que);
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
	
GLDEF_C TInt E32Main()
    {

	TestTQueLink<TDblQueLinkBase>* testDblQueLinkBase;
	TestTQueLink<TDeltaQueLink>* testDeltaQueLink;
	TestTQueLink<TDblQueLink>* testDblQueLink;
	TestTQueLink<TPriQueLink>* testPriQueLink;
	TestTQue<VDblQueBase>* testDblQueBase;
	TestTQue<VDeltaQueBase>* testDeltaQueBase;
	TestTQue<VDblQue<TDblQueLink> >* testDblQue;
	TestTQue<VPriQue<TPriQueLink> >* testPriQue;
 	TestTQue<VDeltaQue<TDeltaQueLink> >* testDeltaQue;
	TestTQueIter<VDblQueIterBase>* testDblQueIterBase;
	TestTQueIter<VDblQueIter<Item> >* testDblQueIter;
 
// Test the queue classes.
	test.Title();
	test.Start(_L("class TDblQueLinkBase"));
	testDblQueLinkBase=new TestTQueLink<TDblQueLinkBase>;
	testDblQueLinkBase->TestQueLinkBase();
	delete testDblQueLinkBase;

	test.Next(_L("class TDeltaQueLink"));
	testDeltaQueLink=new TestTQueLink<TDeltaQueLink>;
	testDeltaQueLink->TestQueLinkBase();
	delete testDeltaQueLink;

  	test.Next(_L("class TDblQueLink"));
	testDblQueLink=new TestTQueLink<TDblQueLink>;
	testDblQueLink->TestQueLink();
	delete testDblQueLink;

  	test.Next(_L("class TPriQueLink"));
	testPriQueLink=new TestTQueLink<TPriQueLink>;
	testPriQueLink->TestQueLink();
 	delete testPriQueLink;

	test.Next(_L("class TDblQueBase"));
	testDblQueBase=new TestTQue<VDblQueBase>;
	testDblQueBase->TestQueBase();
 	delete testDblQueBase;

	test.Next(_L("class TDeltaQueBase"));
	testDeltaQueBase=new TestTQue<VDeltaQueBase>;
	testDeltaQueBase->TestDeltaBase();
 	delete testDeltaQueBase;

	test.Next(_L("class TDlbQue"));
	testDblQue=new TestTQue<VDblQue<TDblQueLink> >;
	testDblQue->TestDblQue();
 	delete testDblQue;

	test.Next(_L("class TPriQue"));
	testPriQue=new TestTQue<VPriQue<TPriQueLink> >;
	testPriQue->TestPriQue();
 	delete testPriQue;
 
	test.Next(_L("class TDeltaQue"));
	testDeltaQue=new TestTQue<VDeltaQue<TDeltaQueLink> >;
	testDeltaQue->TestDeltaQue();
 	delete testDeltaQue;

	test.Next(_L("class TDblQueIterBase"));
	testDblQueIterBase=new TestTQueIter<VDblQueIterBase>;
	testDblQueIterBase->TestIterBase();
 	delete testDblQueIterBase;

	test.Next(_L("class TDblQueIter"));
	testDblQueIter=new TestTQueIter<VDblQueIter<Item> >;
	testDblQueIter->TestQueIter();
 	delete testDblQueIter;

	test.Next(_L("Finished"));
	test.End();
	return(0);
    }

#pragma warning(default : 4710) //fuction not expanded

