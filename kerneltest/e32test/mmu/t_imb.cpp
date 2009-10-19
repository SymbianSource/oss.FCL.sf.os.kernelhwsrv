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
// e32test\mmu\t_imb.cpp
// Overview:
// Test the RChunk Create Local Code and Instruction Memory Barrier 
// control interface.
// API Information:
// RChunk::CreateLocalCode & User::IMB_Range
// Details:
// - Create a code chunk, write a small test function to the chunk, use
// User::IMB_Range to prepare the virtual address range for code execution.
// - Verify the success and failure of the IMB with various processes and with
// different base and size values.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include "u32std.h"
#include <e32math.h>

#ifdef __CPU_ARM
typedef TInt (*TSqrtFn)(TReal&, const TReal&);
extern TInt Sqrt(TReal& /*aDest*/, const TReal& /*aSrc*/);
extern TUint Sqrt_Length();

typedef TInt (*TDivideFn)(TRealX&, const TRealX&);
extern TInt Divide(TRealX& /*aDividend*/, const TRealX& /*aDivisor*/);
extern TUint Divide_Length();

extern TInt SDummy(TInt);
extern TUint SDummy_Length();

extern TInt Increment(TInt);
extern TUint Increment_Length();

typedef TInt (*PFI)(TInt);

class RTestHeap : public RHeap
	{
public: 
	TUint8* GetTop() {return iTop;}
	};

TInt Thread2(TAny* aPtr)
	{
	TSqrtFn pSqrt=(TSqrtFn)aPtr;
	TReal x,y;
	x=2.0;
	return pSqrt(y,x);
	}

TInt Thread3(TAny* aPtr)
	{
	return *(TInt*)aPtr;
	}

TInt Thread4(TAny* aPtr)
	{
	*(TInt*)aPtr=0xe7ffffff;
	return 0;
	}

void SecondaryProcess(const TDesC& aCmd, RTest& test)
	{
	test.Start(_L("Secondary Process"));
	TLex lex(aCmd);
	TUint32 addr;
	TInt r=lex.Val(addr,EHex);
	test(r==KErrNone);
	test.Printf(_L("Main process RAM code at %08x\n"),addr);
	TInt n=0;
	FOREVER
		{
		RThread t;
		TRequestStatus s;
		if (n==0)
			{
			// Create another thread which attempts to execute code from the other process
			r=t.Create(_L("Thread2"),Thread2,0x1000,NULL,(TAny*)addr);
			}
		else if (n==1)
			{
			// Create another thread which attempts to read code from the other process
			r=t.Create(_L("Thread3"),Thread3,0x1000,NULL,(TAny*)addr);
			}
		else if (n==2)
			{
			// Create another thread which attempts to write to the the other process' code
			r=t.Create(_L("Thread4"),Thread4,0x1000,NULL,(TAny*)addr);
			}
		test(r==KErrNone);
		t.SetPriority(EPriorityMore);
		t.Logon(s);
		t.Resume();
		User::WaitForRequest(s);
		TInt exitType=t.ExitType();
		TInt exitReason=t.ExitReason();
		TBuf<32> exitCat=t.ExitCategory();
		CLOSE_AND_WAIT(t);
		test(exitType==EExitPanic);
		test(exitReason==ECausedException);
		test(exitCat==_L("KERN-EXEC"));
		if (++n==3)
			n=0;
		User::After(0);//Force rescheduling of the primary process's thread.
		}
	}

void Fill32(TUint32* aBase, TUint aSize, TUint32 aValue)
	{
	for (; aSize; aSize-=4)
		*aBase++=aValue;
	}

void TestIMB(RTest& test, TUint32* aBase, TUint aOffset, TUint aSize)
	{
	test.Printf(_L("TestIMB: Base %08x Offset %x Size %x\n"),aBase,aOffset,aSize);
	// First fill entire area
	Fill32(aBase,0x20000,0xe3a00000);			// mov r0, #0
#ifdef __SUPPORT_THUMB_INTERWORKING
	aBase[0x8000]=0xe12fff1e;					// bx lr
#else
	aBase[0x8000]=0xe1a0f00e;					// mov pc, lr
#endif
	PFI pBase=(PFI)aBase;
	PFI pCode=(PFI)((TUint8*)aBase+aOffset);
	User::IMB_Range(aBase,aBase+0x8001);
	TInt r=pBase(0);
	test(r==0);

	TUint32* p32=(TUint32*)pCode;
	TUint32* pEnd32=p32+aSize/4;
	Fill32(p32,aSize-4,0xe2800001);				// add r0, r0, #1
#ifdef __SUPPORT_THUMB_INTERWORKING
	pEnd32[-1]=0xe12fff1e;						// bx lr
#else
	pEnd32[-1]=0xe1a0f00e;						// mov pc, lr
#endif
	User::IMB_Range(p32,pEnd32);
	r=pCode(0);
	if (r!=(TInt)(aSize/4-1))
		{
		test.Printf(_L("f(0) expected %d got %d\n"),aSize/4-1,r);
		test(0);
		}
	r=pCode(487);
	if (r!=(TInt)(487+aSize/4-1))
		{
		test.Printf(_L("f(487) expected %d got %d\n"),487+aSize/4-1,r);
		test(0);
		}
	}

GLREF_C TInt E32Main()
	{
	RTest test(_L("T_IMB"));
	test.Title();

	TBuf<16> cmd;
	User::CommandLine(cmd);
	if (cmd.Length()!=0)
		{
		SecondaryProcess(cmd,test);
		return 0;
		}

	test.Start(_L("Create code chunk"));
	TInt pageSize;
	TInt r=UserHal::PageSizeInBytes(pageSize);
	test(r==KErrNone);

	RChunk c;
	r=c.CreateLocalCode(pageSize,0x100000);
	test(r==KErrNone);
	TUint8* pCode=c.Base();
	test.Printf(_L("Code chunk at %08x\n"),pCode);

	// Copy increment function
	Mem::Copy(pCode, (const TAny*)&Increment, Increment_Length());
	User::IMB_Range(pCode,pCode+Increment_Length());
	PFI pFI=(PFI)pCode;
	r=pFI(29);
	test(r==30);

	// Copy dummy without IMB
	Mem::Copy(pCode, (const TAny*)&SDummy, SDummy_Length());
	r=pFI(29);
	test.Printf(_L("Copy without IMB 1: r=%d\n"),r);

	// Now do IMB
	User::IMB_Range(pCode,pCode+SDummy_Length());
	r=pFI(29);
	test(r==29);

	// Read the code so it's in DCache
	TInt i;
	TInt sum=0;
	for (i=0; i<15; ++i)
		sum+=pCode[i];

	// Copy increment function
	Mem::Copy(pCode, (const TAny*)&Increment, Increment_Length());
	r=pFI(29);
	test.Printf(_L("Copy without IMB 2: r=%d\n"),r);

	// Now do IMB
	User::IMB_Range(pCode,pCode+Increment_Length());
	r=pFI(29);
	test(r==30);

	// Now adjust to 2 pages
	r=c.Adjust(2*pageSize);
	test(r==KErrNone);
	TUint8* pCode2=pCode+pageSize;

	// Create another thread
	RThread t;
	TRequestStatus s;
	r=t.Create(_L("Thread2"),Thread2,0x1000,NULL,pCode2);
	test(r==KErrNone);
	t.SetPriority(EPriorityMore);
	t.Logon(s);

	// Copy Sqrt code to 2nd page
	Mem::Copy(pCode2, (const TAny*)&Sqrt, Sqrt_Length());
	User::IMB_Range(pCode2,pCode2+Sqrt_Length());
	TSqrtFn pSqrt=(TSqrtFn)pCode2;
	TReal x,y,z;
	x=2.0;
	r=Math::Sqrt(y,x);
	test(r==KErrNone);
	r=pSqrt(z,x);
	test(r==KErrNone);
	test(z==y);

	// Unmap the second page
	r=c.Adjust(pageSize);
	test(r==KErrNone);

	// Get the second thread to attempt to execute the unmapped code
	t.Resume();
	User::WaitForRequest(s);
	TInt exitType=t.ExitType();
	TInt exitReason=t.ExitReason();
	TBuf<32> exitCat=t.ExitCategory();
	CLOSE_AND_WAIT(t);
	test.Printf(_L("Thread2: %d,%d,%S\n"),exitType,exitReason,&exitCat);
	test(exitType==EExitPanic);
	test(exitReason==ECausedException);
	test(exitCat==_L("KERN-EXEC"));

	// Copy Sqrt code to 1st page
	Mem::Copy(pCode, (const TAny*)&Sqrt, Sqrt_Length());
	User::IMB_Range(pCode,pCode+Sqrt_Length());
	pSqrt=(TSqrtFn)pCode;

	// Do a long test to allow multiple copies of this process to run concurrently
	// Spawn a secondary process
	RProcess p;
	TBuf<16> codeBaseHex;
	codeBaseHex.Format(_L("%08x"),pCode);
	r=p.Create(RProcess().FileName(),codeBaseHex);
	test(r==KErrNone);
	p.Logon(s);
	p.Resume();

	TTime begin;
	begin.HomeTime();
	i=1;
	for (;;)
		{
		TReal x,y,z;
		x=i;
		r=Math::Sqrt(y,x);
		test(r==KErrNone);
		r=pSqrt(z,x);
		test(r==KErrNone);
		test(z==y);
		++i;
		TTime now;
		now.HomeTime();
		if (now.MicroSecondsFrom(begin).Int64()>10000000)
			break;
		User::After(0);//Force rescheduling of the secondary process's thread
		}
	p.Kill(0);
	User::WaitForRequest(s);
	exitType=p.ExitType();
	exitReason=p.ExitReason();
	exitCat=p.ExitCategory();
	CLOSE_AND_WAIT(p);
	test.Printf(_L("SecProc: %d,%d,%S\n"),exitType,exitReason,&exitCat);
	test(exitType==EExitKill);
	test(exitReason==KErrNone);

	// Test heap in code chunk
	RTestHeap* pCodeHeap=(RTestHeap*) UserHeap::ChunkHeap(c,pageSize,pageSize);
	test(pCodeHeap==(RHeap*)c.Base());
	test(c.Size()==pageSize);
	TUint32* pCode3=(TUint32*)pCodeHeap->Alloc(pageSize);
	test(pCode3!=NULL);
	test(c.Size()==2*pageSize);
	TAny* pCode4=pCodeHeap->Alloc(3*pageSize);
	test(pCode4!=NULL);
	test(c.Size()==5*pageSize);
	pCodeHeap->Free(pCode4);
	test(c.Size()==2*pageSize);
	TUint8 * oldTop = pCodeHeap->GetTop();
	pCodeHeap->Free(pCode3);
	TUint8 * newTop = pCodeHeap->GetTop();
	// Under some conditions (KHeapShrinkRatio value is low and iGrowBy is at its default value of a page size) 
	// heap may be reduced at the end of Free() operation
	if (oldTop==newTop) // heap was not reduced
		test(c.Size()==2*pageSize);

	// Test IMB with various base/size values
	pCode3=(TUint32*)pCodeHeap->Alloc(0x20004);
	test(pCode3!=NULL);

	for (i=8; i<1024; i+=32)
		{
		TestIMB(test,pCode3,0,i);
		TestIMB(test,pCode3,4,i);
		}

	for (i=1024; i<131072; i+=844)
		{
		TestIMB(test,pCode3,0,i);
		TestIMB(test,pCode3,4,i);
		}

	c.Close();

	test.End();
	return 0;
	}
#else
GLREF_C TInt E32Main()
	{
	return 0;
	}
#endif
