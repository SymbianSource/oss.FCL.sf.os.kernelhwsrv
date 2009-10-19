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
// e32test\dll\t_tdll12.cpp
// Overview:
// Test DLL Thread Local Storage data and DLL Global data access.
// API Information:
// Dll
// Details:
// - Test that the local storage of two different DLLs, when accessed from
// two different threads is unique. Verify that results are as expected.
// - Test the access of DLL Global data including Alloc, Read and Write. Test
// the protection of the global data. Verify results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include "t_dll.h"
#include "../mmu/mmudetect.h"

const TInt KHeapSize=0x2000;

LOCAL_D RTest test(_L("T_TDLL12"));

TBool KernProt=EFalse;
TUint8* Kern;
TUint8* Garbage;

void SetupAddresses()
	{
	KernProt=HaveDirectKernProt();
	Kern=KernData();
	TUint32 mm_attr=MemModelAttributes();
	TUint32 mm_type=mm_attr & EMemModelTypeMask;
	switch (mm_type)
		{
		case EMemModelTypeDirect:
			Garbage=(TUint8*)0xa8000000;
			break;
		case EMemModelTypeMoving:
			Garbage=(TUint8*)0x60f00000;
			break;
		case EMemModelTypeMultiple:
			Garbage=(TUint8*)0xfe000000;
			break;
		case EMemModelTypeFlexible:
			Garbage=(TUint8*)0x8ff00000;
			break;
		case EMemModelTypeEmul:
			Garbage=(TUint8*)0xf0000000;
			break;
		default:
			test(0);
			break;
		}
	}

void RunTestInThread(TThreadFunction aFn, TAny* aParameter, const TDesC* aPanicCat, TInt aExitCode)
	{
	RThread t;
	TInt r=t.Create(KNullDesC(),aFn,0x2000,NULL,aParameter);
	test(r==KErrNone);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	if (aPanicCat)
		{
		test(t.ExitType()==EExitPanic);
		test(t.ExitCategory()==*aPanicCat);
		test(t.ExitReason()==aExitCode);
		}
	else
		{
		test(t.ExitType()==EExitKill);
		test(t.ExitReason()==aExitCode);
		}
	CLOSE_AND_WAIT(t);
	}

TInt GlobalReadThread(TAny* a)
	{
	return TestDll1::GlobalRead(122,*(TDes8*)a);
	}

TInt GlobalWriteThread(TAny* a)
	{
	return TestDll1::GlobalWrite(0,*(TDes8*)a);
	}

_LIT(KLitKernExec,"KERN-EXEC");
void TestProtection()
	{
	test.Next(_L("Test protection"));
	TBool jit=User::JustInTime();
	User::SetJustInTime(EFalse);
	TUint x=0xffffffff;
	TBuf8<64> ubuf;
	TPtrC8 uptrc(ubuf.Ptr(),11);
	TPtr8 uptr((TUint8*)ubuf.Ptr(),1,20);
	TPtrC8 kptrc(Kern,1);
	TPtr8 kptr(Kern,10,256);
	TPtrC8 gptrc(Garbage,1);
	TPtr8 gptr(Garbage,10,256);
	RunTestInThread(GlobalReadThread,&x,&KLitKernExec,EKUDesInfoInvalidType);
	RunTestInThread(GlobalReadThread,&ubuf,NULL,KErrNone);
	RunTestInThread(GlobalReadThread,&uptr,NULL,KErrNone);
	RunTestInThread(GlobalReadThread,&uptrc,&KLitKernExec,EKUDesInfoInvalidType);
	RunTestInThread(GlobalReadThread,&kptrc,&KLitKernExec,EKUDesInfoInvalidType);
	RunTestInThread(GlobalReadThread,&gptrc,&KLitKernExec,EKUDesInfoInvalidType);
	RunTestInThread(GlobalReadThread,&gptr,&KLitKernExec,ECausedException);
	if (KernProt)
		{
		RunTestInThread(GlobalReadThread,Kern,&KLitKernExec,ECausedException);
		RunTestInThread(GlobalReadThread,&kptr,&KLitKernExec,ECausedException);
		}
	RunTestInThread(GlobalWriteThread,&x,&KLitKernExec,EKUDesInfoInvalidType);
	RunTestInThread(GlobalWriteThread,&ubuf,NULL,KErrNone);
	RunTestInThread(GlobalWriteThread,&uptr,NULL,KErrNone);
	RunTestInThread(GlobalWriteThread,&uptrc,NULL,KErrNone);
	RunTestInThread(GlobalWriteThread,&gptrc,&KLitKernExec,ECausedException);
	RunTestInThread(GlobalWriteThread,&gptr,&KLitKernExec,ECausedException);
	if (KernProt)
		{
		RunTestInThread(GlobalWriteThread,Kern,&KLitKernExec,ECausedException);
		RunTestInThread(GlobalWriteThread,&kptrc,&KLitKernExec,ECausedException);
		RunTestInThread(GlobalWriteThread,&kptr,&KLitKernExec,ECausedException);
		}
	User::SetJustInTime(jit);
	}

LOCAL_C TInt Dll1Thread2(TAny* /*anArg*/)
//
// The entry point for thread2.
//
	{

    test(TestDll1::Attach(ETrue)==KErrNone);
	test((TUint)TestDll1::Data()==0x12345678);
	TestDll1::SetData(0xfedcba98);
	test((TUint)TestDll1::Data()==0xfedcba98);
    test(TestDll1::Attach(EFalse)==KErrNone);
	return KErrNone;
	}
	
LOCAL_C TInt Dll2Thread2(TAny* /*anArg*/)
//
// The entry point for thread2.
//
	{

    test(TestDll2::Attach(ETrue)==KErrNone);
	test((TUint)TestDll2::Data()==0xABCDABCD);
	TestDll2::SetData(0x12341234);
	test((TUint)TestDll2::Data()==0x12341234);
    test(TestDll2::Attach(EFalse)==KErrNone);
	return KErrNone;
	}

void testGlobalAlloc()
//
//
//
	{

	__KHEAP_MARK;
	test.Start(_L("Test Dll::GlobalAlloc"));
	TInt r;
	test(TestDll1::GlobalAllocated()==EFalse);
	test(TestDll2::GlobalAllocated()==EFalse);
	r=TestDll2::GlobalAlloc(0);
	test(r==KErrNone);
	r=TestDll1::GlobalAlloc(256);
	test(r==KErrNone);
	test(TestDll1::GlobalAllocated());
	test(TestDll2::GlobalAllocated()==EFalse);
	r=TestDll2::GlobalAlloc(256);
	test(r==KErrNone);
	test(TestDll1::GlobalAllocated());
	test(TestDll2::GlobalAllocated());

	test.Next(_L("Write"));
	// Write 256 bytes
	TBuf8<0x100> buf100;
	TInt i;
	buf100.SetLength(0x100);
	for (i=0; i<256; i++)
		buf100[i]=(TText8)('A'+i%26);
	r=TestDll1::GlobalWrite(0, buf100);
	test(r==KErrNone);
	buf100.Fill('X');
	r=TestDll2::GlobalWrite(0, buf100);
	test(r==KErrNone);

	test.Next(_L("Read"));
	// Read 256 bytes
	r=TestDll1::GlobalRead(0, buf100);
	test(r==KErrNone);
	for (i=0; i<256; i++)
		test(buf100[i]=='A'+i%26);
	buf100.Fill('D');
	r=TestDll2::GlobalRead(0, buf100);
	test(r==KErrNone);
	for (i=0; i<256; i++)
		test(buf100[i]=='X');

	test.Next(_L("Realloc"));
	r=TestDll1::GlobalAlloc(128);
	test(r==KErrNone);
	test(TestDll1::GlobalAllocated());
	test(TestDll2::GlobalAllocated());
	test.Next(_L("Read"));
	r=TestDll1::GlobalRead(0,buf100);
	for (i=0; i<128; i++)
		test(buf100[i]=='A'+i%26);
	test(buf100.Length()==128);
	r=TestDll2::GlobalRead(0,buf100);
	test(r==KErrNone);
	for (i=0; i<256; i++)
		test(buf100[i]=='X');
	test(buf100.Length()==256);

	test.Next(_L("Read @ pos"));
	// Read from position
	r=TestDll1::GlobalRead(1, buf100);
	test(r==KErrNone);
	test(buf100.Length()==127);
	for (i=0; i<127; i++)
		test(buf100[i]=='A'+(i+1)%26);
	test.Next(_L("Write @ pos"));
	buf100=_L8("LALALALALALA");
	r=TestDll1::GlobalWrite(5, buf100);
	test(r==KErrNone);
	buf100=_L8("POPOPOPOPO");
	r=TestDll2::GlobalWrite(4, buf100);
	test(r==KErrNone);
	r=TestDll1::GlobalRead(0, buf100);
	buf100.SetLength(20);
	test(buf100==_L8("ABCDELALALALALALARST"));
	r=TestDll2::GlobalRead(0, buf100);
	buf100.SetLength(20);
	test(buf100==_L8("XXXXPOPOPOPOPOXXXXXX"));

	TestProtection();

	test.Next(_L("Free Global Alloc"));
	r=TestDll1::GlobalAlloc(0);
	test(r==KErrNone);
	test(TestDll1::GlobalAllocated()==EFalse);
	test(TestDll2::GlobalAllocated());

	r=TestDll2::GlobalWrite(0, _L8("WEEEEEEEEEE"));
	test(r==KErrNone);
	r=TestDll2::GlobalRead(0, buf100);
	buf100.SetLength(11);
	test(buf100==_L8("WEEEEEEEEEE"));
	r=TestDll1::GlobalAlloc(0);
	test(r==KErrNone);
	r=TestDll2::GlobalAlloc(0);
	test(r==KErrNone);
	test(TestDll1::GlobalAllocated()==EFalse);
	test(TestDll2::GlobalAllocated()==EFalse);
	__KHEAP_MARKEND;
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test DLL Thread Local Storage data.
//
    {

	test.Title();
	SetupAddresses();
//
	test.Start(_L("Dll1 Thread 1"));
    test(TestDll1::Attach(ETrue)==KErrNone);
	test((TUint)TestDll1::Data()==0x12345678);
	TestDll1::SetData(0x87654321);
	test((TUint)TestDll1::Data()==0x87654321);
//
	test.Next(_L("Dll1 Thread 2"));
	RThread t;
	TInt r=t.Create(_L("Dll1 Thread2"),Dll1Thread2,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	TRequestStatus tStat;
	t.Logon(tStat);
	test(tStat==KRequestPending);
	t.Resume();
	User::WaitForRequest(tStat);
	test(tStat==KErrNone);
//
	test.Next(_L("Dll1 Thread 1 again"));
	test((TUint)TestDll1::Data()==0x87654321);
	TestDll1::SetData(0x12345678);
	test((TUint)TestDll1::Data()==0x12345678);
//
    test(TestDll1::Attach(EFalse)==KErrNone);
//
	test.Next(_L("Dll2 Thread 1"));
    test(TestDll2::Attach(ETrue)==KErrNone);
	test((TUint)TestDll2::Data()==0xABCDABCD);
	TestDll2::SetData(0xDCBADCBA);
	test((TUint)TestDll2::Data()==0xDCBADCBA);
//
	test.Next(_L("Dll2 Thread 2"));
	r=t.Create(_L("Dll2 Thread2"),Dll2Thread2,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	t.Logon(tStat);
	test(tStat==KRequestPending);
	t.Resume();
	User::WaitForRequest(tStat);
	test(tStat==KErrNone);
//
	test.Next(_L("Dll2 Thread 1 again"));
	test((TUint)TestDll2::Data()==0xDCBADCBA);
	TestDll2::SetData(0xABCDABCD);
	test((TUint)TestDll2::Data()==0xABCDABCD);
//
    test(TestDll2::Attach(EFalse)==KErrNone);
//
	test.Next(_L("Dll1 Thread 1"));
    test(TestDll1::Attach(ETrue)==KErrNone);
	test((TUint)TestDll1::Data()==0x12345678);
	TestDll1::SetData(0x87654321);
	test((TUint)TestDll1::Data()==0x87654321);
//
	test.Next(_L("Dll2 Thread 1"));
    test(TestDll2::Attach(ETrue)==KErrNone);
	test((TUint)TestDll2::Data()==0xABCDABCD);
	TestDll2::SetData(0xDCBADCBA);
	test((TUint)TestDll2::Data()==0xDCBADCBA);
//
	test((TUint)TestDll1::Data()==0x87654321);
	TestDll1::SetData(0x12345678);
	test((TUint)TestDll1::Data()==0x12345678);
//
	test((TUint)TestDll2::Data()==0xDCBADCBA);
	TestDll2::SetData(0xABCDABCD);
	test((TUint)TestDll2::Data()==0xABCDABCD);
//
    test(TestDll1::Attach(EFalse)==KErrNone);
    test(TestDll2::Attach(EFalse)==KErrNone);
//

	test.End();
	return(0);
    }

