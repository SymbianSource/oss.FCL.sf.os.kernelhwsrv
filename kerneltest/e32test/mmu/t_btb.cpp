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
// e32test\mmu\t_btb.cpp
// 
//

//! @SYMTestCaseID KBASE/T_BTB
//! @SYMTestType UT
//! @SYMTestCaseDesc Make sure processes can't interfere with each other via the BTB.  Make sure the BTB is turned on.
//! @SYMREQ REQ4095
//! @SYMTestActions Make concurent processes with code areas and thrash them.
//! @SYMTestExpectedResults  They shouldn't interfere.  The BTB should be enabled.
//! @SYMTestPriority Low
//! @SYMTestStatus Defined

#include <e32test.h>
#include "u32std.h"

#ifdef __CPU_ARM

extern TInt BranchTest1();
extern TInt BranchTest2();
extern TInt BranchTest3();
void BranchTest4(TInt);
extern void BranchTest1End();
extern void BranchTest2End();
extern void BranchTest3End();
extern void BranchTest4End();

typedef TInt (*PFI)(void);
typedef void (*PFV)(TInt);

void SecondaryProcess(const TDesC& aCmd, RTest& test)
	{
	test.Start(_L("Secondary Process"));
	TLex lex(aCmd);
	TUint32 addr;
	TInt r=lex.Val(addr,EHex);
	test(r==KErrNone);
	test.Printf(_L("Primary process says: RAM code at %08x\n"),addr);
	TInt pageSize;
	r=UserHal::PageSizeInBytes(pageSize);
	test(r==KErrNone);
	RChunk c;
	r=c.CreateLocalCode(pageSize,0x100000);
	test(r==KErrNone);
	PFI pBranchTest2=(PFI)c.Base();
	test.Printf(_L("Secondary test loop function at %08x\n"),pBranchTest2);
	
	test((TUint32)pBranchTest2==addr);

	TInt fnLength = (TInt)&BranchTest2End-(TInt)&BranchTest2;

	test.Printf(_L("SecProc: Copying %d bytes from %08x...\n"), (TInt)&BranchTest2End-(TInt)&BranchTest2, (TAny*)&BranchTest2 );
	Mem::Copy((TAny*)pBranchTest2, (TAny*)&BranchTest2, fnLength);
	User::IMB_Range((TAny*)pBranchTest2, fnLength+(TUint8*)pBranchTest2);

	TTime begin;
	begin.HomeTime();

	test.Printf(_L("Running secondary loop...\n"));

	TInt n=0;
	FOREVER
		{
		n=pBranchTest2();

		if (n)
			break;

		TTime now;
		now.HomeTime();
		if (now.MicroSecondsFrom(begin).Int64()>20000000)  // ten seconds for each test ought to be long enough
			break;
		}
		test.Printf(_L("Ending secondary loop.\n"));
		test(n==0);
		//test.End();
	}


GLREF_C TInt E32Main()
	{
	RTest test(_L("T_BTB"));
	test.Title();

	TBuf<16> cmd;
	User::CommandLine(cmd);
	if (cmd.Length()!=0)  // if we get a command line, we're the secondary process
		{
		SecondaryProcess(cmd,test);
		return 0;
		}

	test.Start(_L("Create primary process code chunk"));
	TInt pageSize;
	TInt r=UserHal::PageSizeInBytes(pageSize);
	test(r==KErrNone);

	RChunk c;
	r=c.CreateLocalCode(pageSize,0x100000);
	test(r==KErrNone);
	TUint8* pCode=c.Base();
	test.Printf(_L("Primary process code chunk at %08x\n"),pCode);

	// Spawn a secondary process
	RProcess p;
	TRequestStatus s;
	TBuf<16> codeBaseHex;
	codeBaseHex.Format(_L("%08x"),pCode);
	r=p.Create(RProcess().FileName(),codeBaseHex);
	test(r==KErrNone);
	p.Logon(s);
	p.Resume();


	TTime begin, now;
	TInt n=0, m=0, q=0, fnLength;

	PFI pBranchTest1=(PFI)pCode;
	test.Printf(_L("Primary test loop function at %08x\n"),pBranchTest1);

	fnLength = (TInt)&BranchTest1End-(TInt)&BranchTest1;
	
	//test.Printf(_L("PriProc: Copying %d bytes from 0x%08x to 0x%08x for forward-branching test...\n"), fnLength, &BranchTest1, pBranchTest1);
	Mem::Copy((TAny *)pBranchTest1, (const TAny*)&BranchTest1, fnLength);   // copy in the asm test code
	User::IMB_Range((TAny*)pBranchTest1, fnLength+(TUint8*)pBranchTest1);

	test.Printf(_L("Running primary loop...\n"));
	begin.HomeTime();
	FOREVER
		{
		m++;
		n=pBranchTest1();
		if (n)
			break;	
		now.HomeTime();
		if (now.MicroSecondsFrom(begin).Int64()>10000000)  // ten seconds ought to be long enough
			break;
		}
	test(n==0);
	test.Printf(_L("Ending primary loop.  Ran %d times in 10s.\n"), m);

	// run the second test

	PFI pBranchTest3=(PFI)pCode;

	fnLength = (TInt)&BranchTest3End-(TInt)&BranchTest3;
	//test.Printf(_L("PriProc: Copying %d bytes from 0x%08x to 0x%08x for back-branching test...\n"), fnLength, &BranchTest3, pBranchTest3);
	Mem::Copy((TAny *)pBranchTest3, (const TAny*)&BranchTest3, fnLength);   // copy in the asm test code
	User::IMB_Range((TAny*)pBranchTest3, fnLength+(TUint8*)pBranchTest3);

	test.Printf(_L("PriProc: Starting back-branch test loop.\n"));
	m=0;  
	begin.HomeTime();
	FOREVER
		{
		m++;
		n=pBranchTest3();
		if (n)
			break;	
		now.HomeTime();
		if (now.MicroSecondsFrom(begin).Int64()>10000000)  // ten seconds ought to be long enough
			break;
		}
	test(n==0);
	test.Printf(_L("Ending primary back-branching loop.  Ran %d times in 10s.\n"), m);

	p.Kill(0);
	User::WaitForRequest(s);
	TInt exitType=p.ExitType();
	TInt exitReason=p.ExitReason();
	TExitCategoryName exitCat=p.ExitCategory();
	CLOSE_AND_WAIT(p);
	test.Printf(_L("SecProc: %d,%d,%S\n"),exitType,exitReason,&exitCat);
	test(exitType==EExitKill);
	test(exitReason==KErrNone);

	// speed test

	PFV pBranchTest4 = (PFV)pCode;
	fnLength = (TInt)&BranchTest4End-(TInt)&BranchTest4;
	//test.Printf(_L("PriProc: Copying %d bytes from 0x%08x to 0x%08x for forward-branching speed test...\n"), fnLength, &BranchTest4, pBranchTest1);
	Mem::Copy((TAny *)pBranchTest4, (const TAny*)&BranchTest4, fnLength);   // copy in the asm test code
	User::IMB_Range((TAny*)pBranchTest4, fnLength+(TUint8*)pBranchTest4);

	test.Printf(_L("Speed test running with f(0)...\n"));
	m=0;
	begin.HomeTime();
	FOREVER
		{
		m++;
		pBranchTest4(0);
		now.HomeTime();
		if (now.MicroSecondsFrom(begin).Int64()>10000000)  // ten seconds ought to be long enough
			break;
		}
	test.Printf(_L("Ending f(0) test.  Ran %d times in 10s.\n"), m);

	test.Printf(_L("PriProc: Starting f(1) run.\n"));
	q=0;	
	begin.HomeTime();
	FOREVER
		{
		q++;
		pBranchTest4(1);
		now.HomeTime();
		if (now.MicroSecondsFrom(begin).Int64()>10000000)  // ten seconds ought to be long enough
			break;
		}
	test.Printf(_L("Ending test loop with f(1).  Ran %d times in 10s.\n"), q);

	test(m-q>m/4);  // the difference between m and q should be large, indicating successful prediction

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
