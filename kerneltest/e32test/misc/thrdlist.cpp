// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\thrdlist.cpp
// 
//

#include <e32svr.h>
#include <f32file.h>

_LIT(KFileName,"C:\\THRDLIST.TXT");
_LIT(KLitAsterisk,"*");
_LIT(KLitProblem,"Could not open thread");

struct SArmRegSet
	{
	TUint32 iR0;
	TUint32 iR1;
	TUint32 iR2;
	TUint32 iR3;
	TUint32 iR4;
	TUint32 iR5;
	TUint32 iR6;
	TUint32 iR7;
	TUint32 iR8;
	TUint32 iR9;
	TUint32 iR10;
	TUint32 iR11;
	TUint32 iR12;
	TUint32 iR13;
	TUint32 iR14;
	TUint32	iR15;
	TUint32 iFlags;
	TUint32 iDacr;
	};

TUint ThreadId(const RThread& aThread)
	{
	TThreadId id=aThread.Id();
	TUint* p=(TUint*)&id;
	return *p;
	}

TUint ProcessId(const RProcess& aProcess)
	{
	TProcessId id=aProcess.Id();
	TUint* p=(TUint*)&id;
	return *p;
	}

GLDEF_C TInt E32Main()
	{
	RThread().SetPriority(EPriorityAbsoluteHigh);
	RFs fs;
	TInt r=fs.Connect();
	if (r!=KErrNone)
		User::Panic(_L("THRDLIST FS"),r);
	RFile file;
	r=file.Open(fs,KFileName,EFileWrite);
	if (r==KErrNotFound)
		r=file.Create(fs,KFileName,EFileWrite);
	if (r==KErrNone)
		{
		TInt p=0;
		r=file.Seek(ESeekEnd,p);
		}
	if (r!=KErrNone)
		User::Panic(_L("THRDLIST FILE"),r);
	TTime now;
	now.HomeTime();
	TBuf<1024> buf;
	TDateTime dt=now.DateTime();
	buf.Format(_L("Time %02d:%02d:%02d:%06d Date %02d/%02d/%04d\n"),dt.Hour(),dt.Minute(),dt.Second(),dt.MicroSecond(),dt.Day()+1,dt.Month()+1,dt.Year());
	r=file.Write(buf);
	if (r!=KErrNone)
		User::Panic(_L("THRDLIST WRITE"),r);
	TFindThread ft(KLitAsterisk);
	TFullName fn;
	while (ft.Next(fn)==KErrNone)
		{
		RThread t;
		r=t.Open(ft);
		TExitType exitType=EExitKill;
		TInt exitReason=0;
		TBuf<32> exitCat;
		TFullName procName;
		TUint tid=0xffffffff;
		TUint pid=0xffffffff;
		SArmRegSet regs;
		Mem::FillZ(&regs,sizeof(regs));
		TPckg<SArmRegSet> regPckg(regs);
		if (r==KErrNone)
			{
			t.Context(regPckg);
			exitType=t.ExitType();
			exitReason=t.ExitReason();
			exitCat=t.ExitCategory();
			tid=ThreadId(t);
			RProcess p;
			r=t.Process(p);
			if (r==KErrNone)
				{
				procName=p.FullName();
				pid=ProcessId(p);
				p.Close();
				}
			}
		else
			fn=KLitProblem;
		buf.Format(_L("Thread %S (id=%d) in process %S (id=%d)\n"),&fn,tid,&procName,pid);
		file.Write(buf);
		buf.Format(_L("Exit info %d,%d,%S\n"),exitType,exitReason,&exitCat);
		file.Write(buf);
		buf.Format(_L("  R0=%08x  R1=%08x  R2=%08x  R3=%08x\n"),regs.iR0,regs.iR1,regs.iR2,regs.iR3);
		file.Write(buf);
		buf.Format(_L("  R4=%08x  R5=%08x  R6=%08x  R7=%08x\n"),regs.iR4,regs.iR5,regs.iR6,regs.iR7);
		file.Write(buf);
		buf.Format(_L("  R8=%08x  R9=%08x R10=%08x R11=%08x\n"),regs.iR8,regs.iR9,regs.iR10,regs.iR11);
		file.Write(buf);
		buf.Format(_L(" R12=%08x R13=%08x R14=%08x R15=%08x\n"),regs.iR12,regs.iR13,regs.iR14,regs.iR15);
		file.Write(buf);
		buf.Format(_L("CPSR=%08x DACR=%08x\n\n"),regs.iFlags,regs.iDacr);
		file.Write(buf);
		t.Close();
		}
	file.Close();
	fs.Close();
	return KErrNone;
	}
	
