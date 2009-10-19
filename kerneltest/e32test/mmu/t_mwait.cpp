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
// e32test\mmu\t_mwait.cpp
// Overview:
// Memory Wait State tests
// API Information:
// RChunk
// Details:
// - Load and open the logical device driver ("D_SHADOW.LDD"). Verify
// results.
// - Create a user writable code chunk and copy a small "SpeedTest" function
// into the chunk.
// - Perform a speed test in shadow ROM and RAM and report the relative performance.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include "u32std.h"
#include "d_shadow.h"

RTest test(_L("T_MWAIT"));

typedef TLinAddr (*TMemSpeedTest)(TInt,TInt&);

extern TLinAddr MemSpeedTest(TInt /*aLoopSize*/, TInt& /*aCount*/);

TMemSpeedTest TheSpeedTestFunction;
TInt TheLoopSize;
TInt Count;
RSemaphore TheSemaphore;
TUint8* RamSpeedTestCodePtr;	// speed test code will be copied here to run from RAM

void CopyCode(RChunk& aC)
	{
	TLinAddr code_begin=(TLinAddr)&MemSpeedTest;
	TInt dummy;
	TLinAddr code_end=MemSpeedTest(0,dummy);
	TInt code_size=TInt(code_end-code_begin);
	test.Printf(_L("ROM Code start: %08x\n"), code_begin);
	test.Printf(_L("ROM Code end  : %08x\n"), code_end);
	test.Printf(_L("ROM Code size : %08x\n"), code_size);
	TInt r = aC.CreateLocalCode(code_size, code_size);
	test(r==KErrNone);
	RamSpeedTestCodePtr = aC.Base();
	Mem::Copy(RamSpeedTestCodePtr, (const TAny*)code_begin, code_size);
	User::IMB_Range(RamSpeedTestCodePtr, RamSpeedTestCodePtr + code_size - 1);
	test.Printf(_L("RAM Code base : %08x\n"), RamSpeedTestCodePtr);
	}

TInt MemSpeedTestThread(TAny*)
	{
	TheSemaphore.Signal();
	(*TheSpeedTestFunction)(TheLoopSize,Count);
	return 0;
	}

const TInt KHeapSize=4096;
TInt KMemSpeedTestAddress = (TInt)(&MemSpeedTest);

TInt DoMemSpeedTest(TBool aRam, TInt aLoopSize)
	{
	if (aRam)
#ifdef __MARM__
		TheSpeedTestFunction=(TMemSpeedTest)(RamSpeedTestCodePtr+(KMemSpeedTestAddress&1));	// add 1 if THUMB function
#else
		TheSpeedTestFunction=(TMemSpeedTest)RamSpeedTestCodePtr;
#endif
	else
		TheSpeedTestFunction=MemSpeedTest;
	Count=0;
	TheLoopSize=aLoopSize;
	RShadow sh;
	TInt r=sh.Open();
	test(r==KErrNone);
	RThread t;
	r=t.Create(_L("Speedy"),MemSpeedTestThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
//	t.SetPriority(EPriorityLess);
	sh.SetPriority(t.Handle(),25);
	sh.SetPriority(RThread().Handle(),26);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	TheSemaphore.Wait();
	User::After(1000000);
	t.Kill(0);
	sh.SetPriority(RThread().Handle(),12);	// so thread gets cleaned up
	User::WaitForRequest(s);
	test(s.Int()==KErrNone);
	test(t.ExitType()==EExitKill);
	CLOSE_AND_WAIT(t);
	sh.Close();
	return Count;
	}

void DoFullTest(TBool aRam)
	{
	TInt count[32];
	TInt i;
	for (i=0; i<32; i++)
		{
		TInt loopSize=(i+1)*2048;
		test.Printf(_L("Loop size %dK "),loopSize>>10);
		count[i]=DoMemSpeedTest(aRam,loopSize);
		TInt64 inst = TInt64(count[i]) * TInt64(loopSize>>2);
		test.Printf(_L("%d loops %ld instructions\n"),count[i],inst);
		}
	test.Printf(_L("\n"));
	}

GLDEF_C TInt E32Main()
	{
	test.Title();

	test.Start(_L("Memory Wait State tests"));

	TInt r=TheSemaphore.CreateLocal(0);
	test(r==KErrNone);

	r=User::LoadLogicalDevice(_L("D_SHADOW"));
	test(r==KErrNone || r==KErrAlreadyExists);

	RChunk c;
	CopyCode(c);

	test.Next(_L("Testing ROM wait states..."));
	DoFullTest(EFalse);

	test.Next(_L("Testing RAM wait states..."));
	DoFullTest(ETrue);

	c.Close();
	test.End();
	return 0;
	}
