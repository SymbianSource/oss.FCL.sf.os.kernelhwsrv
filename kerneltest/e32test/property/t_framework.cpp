// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <e32test.h>
#include "t_framework.h"

RTest test(RProcess().FileName());

CTestProgram::CTestProgram(const TDesC& aName) : iName(aName) 
	{
	iThreadId = RThread().Id();
	}

void CTestProgram::Panic()
	{
	TPtrC8 fd((TUint8*)iFile);
	TPtrC8 cd((TUint8*)iCond);

	HBufC* fhb = HBufC::NewMax(fd.Length());
	test(fhb != 0);
	HBufC* chb = HBufC::NewMax(cd.Length());
	test(chb != 0);

	fhb->Des().Copy(fd);
	chb->Des().Copy(cd);

	test.Panic(iError, _L("Test '%S' Fails;\nCond: '%S'; File: '%S'; Line %d;\n"), &iName, chb, fhb, iLine);
	}

void CTestProgram::Panic(TInt aError, char* aCond, char* aFile, TInt aLine)
	{
	iError = aError;
	iCond = aCond;
	iFile = aFile;
	iLine = aLine;
	if (iThreadId != RThread().Id())
		{
		RThread thr;
		thr.Panic(_L("FAIL"), aError);
		}
	Panic();
	}

void CTestProgram::Launch(TUint aCount)
	{
	// Remember the number of open handles. Just for a sanity check ....
	TInt start_thc, start_phc;
	RThread().HandleCount(start_phc, start_thc);
	TF_ERROR(RThread().RequestCount(), RThread().RequestCount() == 0);

	test.Next(iName);
	Run(aCount);

	// Sanity check for open handles
	TInt end_thc, end_phc;
	RThread().HandleCount(end_phc, end_thc);
	TF_ERROR(end_thc - start_thc, start_thc == end_thc);
	TF_ERROR(end_phc - start_phc, start_phc == end_phc);
		// and also for pending requests ...
	TF_ERROR(RThread().RequestCount(), RThread().RequestCount() == 0);
	}

void CTestProgram::LaunchGroup(CTestProgram** aGroup, TUint aCount)
	{
	for (CTestProgram** progP = aGroup; *progP; ++progP)
		{
		CTestProgram* prog = *progP;
		prog->Launch(aCount);
		}
	}

TInt CTestProgram::ThreadEntry(TAny* a)
	{
	CTestProgram* prog = (CTestProgram*) a;
	prog->Run(prog->iCount);
	return KErrNone;
	}

void CTestProgram::Spawn(TUint aCount)
	{
	test.Next(iName); 
	iCount = aCount;
	iStatus = KRequestPending;
	RThread thr;
	TInt r = thr.Create(KNullDesC, ThreadEntry, 0x2000, NULL, this);
	TF_ERROR(r, r == KErrNone);
	thr.NotifyDestruction(iDestroyStatus);
	thr.Logon(iStatus);
	thr.Resume();
	thr.Close();
	}

void CTestProgram::Wait()
	{
	User::WaitForRequest(iStatus);
	if (iStatus.Int() != KErrNone)
		{
		Panic();
		}
	User::WaitForRequest(iDestroyStatus);
	if (iDestroyStatus.Int() != KErrNone)
		{
		Panic();
		}
	}

void CTestProgram::SpawnGroup(CTestProgram** aGroup, TUint aCount)
	{
	test.Start(_L("=== Start Parallel Testing ==="));
	CTestProgram** progP;
	for (progP = aGroup; *progP; ++progP)
		{
		CTestProgram* prog = *progP;
		prog->Spawn(aCount);
		}
	for (progP = aGroup; *progP; ++progP)
		{
		CTestProgram* prog = *progP;
		prog->Wait();
		}
	test.Next(_L("=== End Parallel Testing ==="));
	test.End();
	}

void CTestProgram::Exec(const TDesC& aFile, TAny* args, TInt size)
	{
	//
	// Create the child process and pass args as a UNICODE command line.
	// (we suppose that the args size is multiple of sizeof(TUint16))
	//
	RProcess proc;
	TInt r = proc.Create(aFile, TPtrC((TUint16*) args, size/sizeof(TUint16)));
	TF_ERROR(r, r == KErrNone);
	TRequestStatus status;
	proc.Logon(status);
	proc.Resume();
	User::WaitForRequest(status);
	TF_ERROR(status.Int(), status.Int() == KErrNone);
	proc.Close();
	}

void CTestProgram::Start()
	{
	test.Title();
	test.Start(_L("GO!"));
	}

void CTestProgram::End()
	{
	test.End();
	}
