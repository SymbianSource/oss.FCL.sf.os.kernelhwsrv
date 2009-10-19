// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_kucopy.cpp
// Test copying between kernel and user side.
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include "d_kucopy.h"
#include "../mmu/mmudetect.h"

//#define QUICK

_LIT(KLddFileName,"D_KUCOPY.LDD");
_LIT(KLitKernExec,"KERN-EXEC");

RTest test(_L("T_KUCOPY"));
RKUCopy KU;
TInt BufSize;
TInt RandSize;
TUint8* Buf1;
TUint8* Buf2;
TUint8* Random;
TBool KernProt=EFalse;

_LIT8(KTest8_1,"1");
_LIT8(KTest8_2,"Test2");
_LIT8(KTest8_3,"extensionality emptyset pairset separation powerset unionset infinity foundation replacement choice");
_LIT8(KTest8_4,"And darkness and decay and the red death held illimitable dominion over all.");

_LIT16(KTest16_1,"39");
_LIT16(KTest16_2,"Test16");
_LIT16(KTest16_3,"extensionality emptyset pairset separation powerset unionset infinity foundation replacement choice.");
_LIT16(KTest16_4,"And darkness and decay and the red death held illimitable dominion over all.");

static const TDesC8* const TestStrings8[]=
	{
	&KTest8_1,
	&KTest8_2,
	&KTest8_3,
	&KTest8_4,
	};

static const TDesC16* const TestStrings16[]=
	{
	&KTest16_1,
	&KTest16_2,
	&KTest16_3,
	&KTest16_4,
	};

#define NTESTS8		((TInt)(sizeof(TestStrings8)/sizeof(const TDesC8*)))
#define NTESTS16	((TInt)(sizeof(TestStrings16)/sizeof(const TDesC16*)))

enum TTest
	{
	EPut,
	EGet,
	EPut32,
	EGet32,
	ESet,
	EDesPut8,
	EDesGet8,
	EDesInfo8,
	EDesPut16,
	EDesGet16,
	EDesInfo16,
	};

void DoPutGetTest(TInt aSrcOffset, TInt aDestOffset, TInt aLen, TTest aTest)
	{
	// Use the first BufSize of random data as background, the rest as copy data.
	// Use Buf1 as target
//	test.Printf(_L("s=%4d d=%4d l=%4d t=%d\n"),aSrcOffset,aDestOffset,aLen,aTest);
	switch (aTest)
		{
		case EPut:
			Mem::Move(Buf1,Random,BufSize);
			KU.Put(Buf1+aDestOffset,aSrcOffset+BufSize,aLen);
			break;
		case EGet:
			KU.Get(Random+BufSize+aSrcOffset,aDestOffset,aLen);
			KU.Read(Buf1);
			break;
		case EPut32:
			Mem::Move(Buf1,Random,BufSize);
			KU.Put32((TUint32*)(Buf1+aDestOffset),aSrcOffset+BufSize,aLen);
			break;
		case EGet32:
			KU.Get32((const TUint32*)(Random+BufSize+aSrcOffset),aDestOffset,aLen);
			KU.Read(Buf1);
			break;
		default:
			User::Panic(_L("PutGetTest"),aTest);
		}

	// Should now have:
	// Buf1[0...aDestOffset-1]=Random[0...aDestOffset-1]
	// Buf1[aDestOffset...aDestOffset+aLen-1]=Random[BufSize+aSrcOffset...BufSize+aSrcOffset+aLen-1]
	// Buf1[aDestOffset+aLen...BufSize-1]=Random[aDestOffset+aLen...BufSize-1]
	test(Mem::Compare(Buf1,aDestOffset,Random,aDestOffset)==0);
	test(Mem::Compare(Buf1+aDestOffset,aLen,Random+BufSize+aSrcOffset,aLen)==0);
	test(Mem::Compare(Buf1+aDestOffset+aLen,BufSize-aDestOffset-aLen,Random+aDestOffset+aLen,BufSize-aDestOffset-aLen)==0);
	}

void DoSetTest(TInt aOffset, TInt aLen, TUint8 aValue)
	{
	Mem::Move(Buf1,Random,BufSize);
	KU.Set(Buf1+aOffset,aLen,aValue);
	test(Mem::Compare(Buf1,aOffset,Random,aOffset)==0);
	TUint8* p=Buf1+aOffset;
	TUint8* pE=p+aLen;
	while (p<pE)
		test(*p++==aValue);
	test(Mem::Compare(Buf1+aOffset+aLen,BufSize-aOffset-aLen,Random+aOffset+aLen,BufSize-aOffset-aLen)==0);
	}

void TestDesGet8(const TDesC8& aSrc)
	{
	TPtr8 dest(Buf2,0,BufSize);
	KU.DesGet(dest,aSrc);
	test(dest==aSrc);
	}

void TestDesGet8()
	{
	TInt i;
	for (i=0; i<NTESTS8; ++i)
		{
		const TDesC8* orig=TestStrings8[i];
		TestDesGet8(*orig);	// test EBufC
		TBuf8<256> buf=*orig;
		TestDesGet8(buf);	// test EBuf
		TPtrC8 ptrc(orig->Ptr(),orig->Length());
		TestDesGet8(ptrc);	// test EPtrC
		TPtr8 ptr((TUint8*)orig->Ptr(),orig->Length(),orig->Length());
		TestDesGet8(ptr);	// test EPtr
		HBufC8* pH=orig->Alloc();
		test(pH!=NULL);
		TPtr8 bufCptr(pH->Des());
		TestDesGet8(bufCptr);	// test EBufCPtr
		User::Free(pH);
		}
	}

void TestDesPut8(TDesC8& aDest, const TDesC8* aExtraDest, const TDesC8& aSrc)
	{
	KU.DesPut((TDes8&)aDest,aSrc);
	test(aDest==aSrc);
	if (aExtraDest)
		test(*aExtraDest==aSrc);	// for testing EBufCPtr
	}

void TestDesPut8()
	{
	TInt i;
	for (i=0; i<NTESTS8; ++i)
		{
		const TDesC8* orig=TestStrings8[i];
		TBuf8<256> buf;
		TestDesPut8(buf,NULL,*orig);	// test EBuf
		Mem::FillZ((TAny*)buf.Ptr(),256);
		TPtr8 ptr((TUint8*)buf.Ptr(),0,256);
		TestDesPut8(ptr,NULL,*orig);	// test EPtr
		HBufC8* pH=HBufC8::New(256);
		test(pH!=NULL);
		TPtr8 bufCptr(pH->Des());
		TestDesPut8(bufCptr,pH,*orig);	// test EBufCPtr
		User::Free(pH);
		}
	}

void TestDesInfo8(const TDesC8& aDes, TInt aLength, TInt aMaxLength, const TUint8* aPtr)
	{
	SDesInfo info;
	KU.DesInfo(aDes,info);
	test(aLength==info.iLength);
	test(aMaxLength==info.iMaxLength);
	test((TAny*)aPtr==info.iPtr);
	}

void TestDesInfo8()
	{
	TInt i;
	for (i=0; i<NTESTS8; ++i)
		{
		const TDesC8* orig=TestStrings8[i];
		TestDesInfo8(*orig,orig->Length(),-1,orig->Ptr());	// test EBufC
		TBuf8<256> buf;
		TestDesInfo8(buf,0,256,buf.Ptr());				// test EBuf
		buf=*orig;
		TestDesInfo8(buf,orig->Length(),256,buf.Ptr());	// test EBuf
		TBuf8<203> buf2;
		TestDesInfo8(buf2,0,203,buf2.Ptr());			// test EBuf
		buf2=*orig;
		TestDesInfo8(buf2,orig->Length(),203,buf2.Ptr());	// test EBuf
		TPtrC8 ptrc(orig->Ptr(),orig->Length());
		TestDesInfo8(ptrc,orig->Length(),-1,orig->Ptr());	// test EPtrC
		TPtr8 ptr((TUint8*)orig->Ptr(),orig->Length(),orig->Length());
		TestDesInfo8(ptr,orig->Length(),orig->Length(),orig->Ptr());	// test EPtr
		HBufC8* pH=orig->Alloc();
		test(pH!=NULL);
		TPtr8 bufCptr(pH->Des());
		TestDesInfo8(*pH,orig->Length(),-1,pH->Ptr());
		TestDesInfo8(bufCptr,orig->Length(),User::AllocLen(pH)-sizeof(TDesC8),pH->Ptr());
		User::Free(pH);
		}
	}

void RunTestInThread(TThreadFunction aFn, TAny* aParameter, const TDesC* aPanicCat, TInt aExitCode, TInt aLine)
	{
	test.Printf(_L("Line %d\n"),aLine);
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

TInt DesGet8Thread(TAny* aPtr)
	{
	TBuf8<256> dest;
	KU.DesGet(dest, *(const TDesC8*)aPtr);
	return 0;
	}

TInt DesPut8Thread(TAny* aPtr)
	{
	KU.DesPut(*(TDes8*)aPtr,KTest8_4);
	return 0;
	}

TInt DesInfo8Thread(TAny* aPtr)
	{
	SDesInfo info;
	KU.DesInfo(*(TDesC8*)aPtr,info);
	return 0;
	}

void TestErrors()
	{
	TBool jit=User::JustInTime();
	User::SetJustInTime(EFalse);
	TUint8* Kern=KU.KernelBufferAddress();
	TUint x=0xffffffff;
	TBuf8<256> ubuf;
	TPtrC8 uptrc(Buf1,100);
	TPtrC8 kptrc(Kern,1);
	TPtr8 kptr(Kern,10,256);
	RunTestInThread(DesGet8Thread,&x,&KLitKernExec,EKUDesInfoInvalidType,__LINE__);
	RunTestInThread(DesGet8Thread,&ubuf,NULL,KErrNone,__LINE__);
	RunTestInThread(DesGet8Thread,&ubuf,NULL,KErrNone,__LINE__);
	if (KernProt)
		{
		RunTestInThread(DesGet8Thread,Kern,&KLitKernExec,ECausedException,__LINE__);
		RunTestInThread(DesGet8Thread,&kptrc,&KLitKernExec,ECausedException,__LINE__);
		RunTestInThread(DesGet8Thread,&kptr,&KLitKernExec,ECausedException,__LINE__);
		}
	RunTestInThread(DesPut8Thread,&x,&KLitKernExec,EKUDesInfoInvalidType,__LINE__);
	RunTestInThread(DesPut8Thread,&ubuf,NULL,KErrNone,__LINE__);
	RunTestInThread(DesPut8Thread,&uptrc,&KLitKernExec,EKUDesSetLengthInvalidType,__LINE__);
	if (KernProt)
		{
		RunTestInThread(DesPut8Thread,Kern,&KLitKernExec,ECausedException,__LINE__);
		RunTestInThread(DesPut8Thread,&kptrc,&KLitKernExec,EKUDesSetLengthInvalidType,__LINE__);
		RunTestInThread(DesPut8Thread,&kptr,&KLitKernExec,ECausedException,__LINE__);
		}
	RunTestInThread(DesInfo8Thread,&x,&KLitKernExec,EKUDesInfoInvalidType,__LINE__);
	RunTestInThread(DesInfo8Thread,&ubuf,NULL,KErrNone,__LINE__);
	RunTestInThread(DesInfo8Thread,&uptrc,NULL,KErrNone,__LINE__);
	RunTestInThread(DesInfo8Thread,&kptrc,NULL,KErrNone,__LINE__);
	RunTestInThread(DesInfo8Thread,&kptr,NULL,KErrNone,__LINE__);
	if (KernProt)
		{
		RunTestInThread(DesInfo8Thread,Kern,&KLitKernExec,ECausedException,__LINE__);
		}

	User::SetJustInTime(jit);
	}

void RequestCompleteBadPointer(TUint aBadAddress)
	{
	TRequestStatus* badStatus = (TRequestStatus*)aBadAddress;
	test.Printf(_L("Test Kern::RequestComplete to %08x\n"), aBadAddress);
	KU.RequestComplete(badStatus);
	test.Printf(_L("Test Kern::RequestComplete to %08x (in current thread)\n"), aBadAddress);
	KU.RequestCompleteLocal(badStatus);

	// Check the thread hasn't been signalled
	TRequestStatus status;
	RTimer timer;
	test_KErrNone(timer.CreateLocal());
	timer.After(status, 100000); // 100ms
	test_Equal(KRequestPending, status.Int());
	User::WaitForAnyRequest();
	test_Equal(KErrNone, status.Int());
	timer.Close();	
	
	test.Printf(_L("Test Kern::QueueRequestComplete to %08x\n"), aBadAddress);
	TInt r = KU.QueueRequestComplete(badStatus);

	// Kern::QueueRequestComplete will signal even if the pointer is bad, unless:
	//  - the pointer is null
	//  - the call to Setup returned an error
	if (badStatus != NULL && r == KErrNone)
		User::WaitForAnyRequest();
	}

void TestRequestCompleteTrapped()
	{
	TRequestStatus status;
	test.Printf(_L("Test Kern::RequestComplete to %08x\n"), &status);
	status = KRequestPending;
	KU.RequestComplete(&status);
	User::WaitForRequest(status);
	test_Equal(KErrNone, status.Int());
	
	test.Printf(_L("Test Kern::RequestComplete to %08x (in current thread)\n"), &status);
	status = KRequestPending;
	KU.RequestCompleteLocal(&status);
	User::WaitForRequest(status);
	test_Equal(KErrNone, status.Int());

	test.Printf(_L("Test Kern::QueueRequestComplete to %08x\n"), &status);
	status = KRequestPending;
	KU.QueueRequestComplete(&status);
	User::WaitForRequest(status);
	test_Equal(KErrNone, status.Int());

	RequestCompleteBadPointer(0x00000000);
	RequestCompleteBadPointer(0x00000001);
	RequestCompleteBadPointer(0x00000004);
	RequestCompleteBadPointer(0x00000008);
	if (KernProt)
		{
		RequestCompleteBadPointer(0x80000000);
		RequestCompleteBadPointer(0xfffffff0);
		RequestCompleteBadPointer((TUint)KU.KernelBufferAddress());
		}
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Load LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);
	test.Next(_L("Open channel"));
	r=KU.Open();
	test(r==KErrNone);
	test.Next(_L("Create chunk"));
	RChunk c;
	r=c.CreateDisconnectedLocal(0,0,0x100000);
	test(r==KErrNone);
	BufSize=KU.Length();
	RandSize=KU.RandomLength();
	test.Printf(_L("BufSize=%x, RandSize=%x\n"),BufSize,RandSize);
	r=c.Commit(0,BufSize);
	test(r==KErrNone);
	r=c.Commit(0x10000,BufSize);
	test(r==KErrNone);
	r=c.Commit(0x20000,RandSize);
	test(r==KErrNone);
	Buf1=c.Base();
	Buf2=c.Base()+0x10000;
	Random=c.Base()+0x20000;
	test.Printf(_L("Buf1 at %08x, Buf2 at %08x, Random at %08x\n"),Buf1,Buf2,Random);
	Mem::Fill(Buf1,BufSize,0xb1);
	Mem::Fill(Buf2,BufSize,0xb2);
	Mem::Fill(Random,RandSize,0x8b);
	KU.ReadRandom(Random);

	KernProt=HaveDirectKernProt();

#ifndef QUICK
	TInt s;
	TInt d;
	TInt l;
	test.Next(_L("Put/Get 1"));
	for (l=1; l<300; l+=3)
		{
		test.Printf(_L("PG1: l=%d\n"),l);
		for (s=0; s<=BufSize-l; s+=227)
			{
			for (d=0; d<=BufSize-l; d+=229)
				{
				DoPutGetTest(s,d,l,EPut);
				DoPutGetTest(s,d,l,EGet);
				}
			}
		}

	test.Next(_L("Put/Get 2"));
	for (l=1; l<300; l+=3)
		{
		test.Printf(_L("PG2: l=%d\n"),l);
		for (s=BufSize-l; s>=0; s-=227)
			{
			for (d=BufSize-l; d>=0; d-=229)
				{
				DoPutGetTest(s,d,l,EPut);
				DoPutGetTest(s,d,l,EGet);
				}
			}
		}

	test.Next(_L("Put32/Get32 1"));
	for (l=4; l<300; l+=12)
		{
		test.Printf(_L("PG32,1: l=%d\n"),l);
		for (s=0; s<=BufSize-l; s+=4*59)
			{
			for (d=0; d<=BufSize-l; d+=4*61)
				{
				DoPutGetTest(s,d,l,EPut32);
				DoPutGetTest(s,d,l,EGet32);
				}
			}
		}

	test.Next(_L("Put32/Get32 2"));
	for (l=4; l<300; l+=12)
		{
		test.Printf(_L("PG32,2: l=%d\n"),l);
		for (s=BufSize-l; s>=0; s-=4*59)
			{
			for (d=BufSize-l; d>=0; d-=4*61)
				{
				DoPutGetTest(s,d,l,EPut32);
				DoPutGetTest(s,d,l,EGet32);
				}
			}
		}
	test.Next(_L("Set"));
	for (l=1; l<300; l+=3)
		{
		test.Printf(_L("Set: l=%d\n"),l);
		for (s=0; s<=BufSize-l; s+=83)
			{
			DoSetTest(s,l,0x1b);
			DoSetTest(s,l,0xaa);
			}
		for (s=BufSize-l; s>=0; s-=79)
			{
			DoSetTest(s,l,0x1b);
			DoSetTest(s,l,0xaa);
			}
		}
#endif
	test.Next(_L("DesGet8"));
	TestDesGet8();
	test.Next(_L("DesPut8"));
	TestDesPut8();
	test.Next(_L("DesInfo8"));
	TestDesInfo8();
	test.Next(_L("Errors"));
	TestErrors();
	test.Next(_L("Test Kern::RequestComplete functions trapped"));
	TestRequestCompleteTrapped();

	c.Close();
	KU.Close();
	test.End();
	return 0;
	}




