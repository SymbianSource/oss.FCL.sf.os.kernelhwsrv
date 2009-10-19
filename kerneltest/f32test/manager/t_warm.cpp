// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\manager\t_warm.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include "u32std.h"
#include <f32file.h>

_LIT(KProgressFileName,"C:\\Progress");

#define BUF_SIZE	4096

LOCAL_D RTest test(_L("T_WARM"));
GLDEF_D RFs Session;
GLDEF_D RFile File;
TBuf8<BUF_SIZE> Buffer;
TUint Seed[2];

#if !defined(__WINS__)
LOCAL_C TInt Output()
	{
	TInt err = File.Write(Buffer);
	if (err!=KErrNone && err!=KErrDiskFull)
		{
		File.Close();
		Session.Close();
		test.Printf(_L("Error writing to file\n"));
		test(1==0);
		}
	return err;
	}
/*

LOCAL_C TInt OutputByte(TInt i)
	{
	TPtrC8 bytePtr( Buffer.Ptr()+i, 1 );
	TInt err = File.Write(bytePtr);
	if (err!=KErrNone && err!=KErrDiskFull)
		{
		File.Close();
		Session.Close();
		test.Printf(_L("Error writing to file\n"));
		test(1==0);
		}
	return err;
	}

  */

LOCAL_C TInt Input()
	{
	TInt err = File.Read(Buffer);
	if (err!=KErrNone && err!=KErrEof)
		{
		File.Close();
		Session.Close();
		test.Printf(_L("Error reading from file\n"));
		test(1==0);
		}
	return err;
	}

LOCAL_C void CreateFile()
	{
	if ( File.Create(Session, _L("C:\\TESTFILE.BIN"), EFileWrite) == KErrNone )
		{
		test.Printf(_L("C:\\TESTFILE.BIN created\n"));
		}
	else
		{
		File.Close();
		Session.Close();
		test.Printf(_L("Error opening file\n"));
		test(1==0);
		}
	}

LOCAL_C void OpenFile()
	{
	if ( File.Open(Session, _L("C:\\TESTFILE.BIN"), EFileRead) != KErrNone )
		{
		File.Close();
		Session.Close();
		test.Printf(_L("Error reading input file\n"));
		test(1==0);
		}
	else
		{
		test.Printf(_L("C:\\TESTFILE.BIN opened\n"));
		}
	}

__NAKED__ TUint Random( TUint * /*aSeed*/ )
	{
#ifdef __MARM__
	// Random number generator
	//
	// This uses a 33-bit feedback shift register to generate a pseudo-randomly
	// ordered sequence of numbers which repeats in a cycle of length 2^33 - 1
	//
	__SWITCH_TO_ARM;
	asm("LDMIA	r0, {r1, r2} ");		// get seed value
	asm("MOV	r3, r1, LSR#1 ");		// 33-bit rotate right
	asm("ORR	r3, r3, r2, LSL#31 ");	// LSB of r2 into MSB of r3
	asm("AND	r2, r1, #1 ");			// LSB of r1 into LSB of r2
	asm("EOR	r3, r3, r1, LSL#12 ");	// (involved!)
	asm("EOR	r1, r3, r3, LSR#20 ");	// (similarly involved!)
	asm("STMIA	r0, {r1, r2} ");		// store new seed
	asm("MOV	r0, r1 ");				// return low word of new seed
	__JUMP(,lr);
	__END_ARM;
#endif
#if defined(__WINS__) || defined(__X86__)
	_asm push ebx
	_asm mov ecx, [esp+8]				// get seed pointer
	_asm mov eax, [ecx]					// get seed value into edx:eax
	_asm mov edx, [ecx+4]
	_asm mov ebx, eax					// original eax into ebx
	_asm shr edx, 1						// edx lsb into CF
	_asm rcr eax, 1						// rotate into eax
	_asm adc edx, edx					// LSB into edx lsb
	_asm shl ebx, 12
	_asm xor eax, ebx					// first involved step
	_asm mov ebx, eax
	_asm shr ebx, 20
	_asm xor eax, ebx					// second involved step
	_asm mov [ecx+4], edx				// store top bit of result
	_asm mov [ecx], eax					// store bottom 32 bits of result
	_asm pop ebx
	_asm ret							// return with low word of result in eax
#endif
	}

LOCAL_C void GenerateBuffer()
	{
	TInt i;
	TUint *p = (TUint*)Buffer.Ptr();
	for (i=0; i<BUF_SIZE/4; i++)
		{
		*p++ = Random(Seed);
		}
	Buffer.SetLength(BUF_SIZE);
	}

LOCAL_C TInt VerifyBuffer()
	{
	TInt i;
	TUint *p = (TUint*)Buffer.Ptr();
	for (i=0; i<Buffer.Length()/4; i++)
		{
		if (*p++ != Random(Seed))
			{
			return(i*4);
			}
		}
	return -1;
	}

LOCAL_C void GenerateFile()
	{
	TInt size=0;
	TInt err = KErrNone;
	test.Next(_L("Creating large file containing pseudo-random sequence"));
	test( Session.Connect()==KErrNone );
	Seed[0] = 0xB504F334;
	Seed[1] = 0;

	// write PRBS sequence to file
	CreateFile();
	while(err==KErrNone)
		{
		GenerateBuffer();
		err=Output();
		if (err==KErrNone)
			size+=BUF_SIZE;
		}
/*
//	Fill RAM Drive to last byte
	err=KErrNone;
	TInt i=0;
	while(err==KErrNone)
		{
		err=OutputByte(i++);
		if (err==KErrNone)
			size++;
		}
*/
	File.Close();
	Session.Close();
	test.Printf(_L("Total bytes written %d\n"), size);
	User::After(2000000);
	}

LOCAL_C void VerifyFile()
	{
	TInt err = KErrNone;
	TInt i,j;
	test.Next(_L("Verifying file containing pseudo-random sequence"));
	test( Session.Connect()==KErrNone );
	Seed[0] = 0xB504F334;
	Seed[1] = 0;

	// read and verify contents of file
	OpenFile();
	i=0;
	FOREVER
		{
		err=Input();
		if (err==KErrNone && Buffer.Length()!=0)
			{
			j = VerifyBuffer();
			if (j>=0)
				{
				File.Close();
				Session.Close();
				test.Printf(_L("Verify error at 0x%08X\n"), j+i );
				test(1==0);
				}
			i+=Buffer.Length();
			}
		else
			break;
		}
	test.Printf(_L("File verified OK\n"));
	test.Printf(_L("Total bytes read %d\n"), i);
	File.Close();
	Session.Close();
	}

void CauseKernelException()
	{
	RThread().SetSystem(ETrue);
	*((TUint*)0x58000900)=0xDEADDEAD;
	}
#endif

#if defined(__WINS__)

TInt E32Main()
//
// This test doesn't make sense under WINS
//
	{
	test.Start(_L("No tests under WINS"));
	test.End();
	return KErrNone;
	}

#else

GLDEF_C TInt E32Main()
//
// T_WARM should be run by
// copying to RAM as C:\System\libs\eshell.exe and pressing the User Reset button
//
	{	 

	test.Title();
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		test.Start(_L("Test T_WARM is running from RAM as C:\\Sys\\Bin\\ESHELL.EXE"));
	else
		test.Start(_L("Test T_WARM is running from RAM as C:\\System\\Bin\\ESHELL.EXE"));
	RProcess p;
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		test(p.FileName().CompareF(_L("C:\\Sys\\Bin\\ESHELL.EXE"))==0);
	else
		test(p.FileName().CompareF(_L("C:\\System\\Bin\\ESHELL.EXE"))==0);
	TBuf8<0x100> peaches=_L8("");
	TInt i;
	for (i=0; i<16; i++)
		peaches.Append(_L8("PeachesAndCream_"));

	test.Next(_L("Check test progress"));
	RFs fs;
	TInt r=fs.Connect();
	test(r==KErrNone);
	RFile progress;
	TBuf8<0x80> buf;
	r=progress.Open(fs,KProgressFileName(),EFileRead|EFileWrite);
	if (r==KErrNotFound)
		{
		test.Next(_L("Setting progress to first test"));
		r=progress.Create(fs,KProgressFileName(),EFileRead|EFileWrite);
		test(r==KErrNone);
		buf=_L8("Warm Reset");
		r=progress.Write(buf);
		test(r==KErrNone);
		}
	else
		{
		r=progress.Read(buf);
		test(r==KErrNone);
		}


	TBuf<0x10> bufU;
	bufU.Copy(buf);
	test.Printf(_L("Performing %S test\n"), &bufU);

	test.Next(_L("Get startup reason"));
	TMachineStartupType reason;
	UserHal::StartupReason(reason);

	if (buf==_L8("Warm Reset"))
		{
		test.Next(_L("Test reason"));
		test(reason==EStartupWarmReset);
		TInt fault;
		test(UserHal::FaultReason(fault)==KErrGeneral);
		GenerateFile();
		}
	else if (buf==_L8("Kernel Fault"))
		{
		test.Next(_L("Test reason"));
		test(reason==EStartupKernelFault);
		TInt fault;
		test(UserHal::FaultReason(fault)==KErrNone);
		test(fault==0x1234);
		VerifyFile();
		}
	else if (buf==_L8("Kernel Exception"))
		{
		test.Next(_L("Test reason"));
		test(reason==EStartupKernelFault);
#if defined(__MARM__)
		TExcInfo excInfo;
		test(UserHal::ExceptionInfo(excInfo)==KErrNone);
		test(((TUint) excInfo.iCodeAddress & 0xf0000000)==0x20000000);
		test((TUint)excInfo.iDataAddress==0x58000900);
		TInt id;
		test(UserHal::ExceptionId(id)==KErrNone);
		test(id==EExcAccessViolation);
#endif
		VerifyFile();
		}
	else if (buf==_L8("Finalise"))
		{
		test.Next(_L("Test reason"));
		test(reason==EStartupWarmReset);
		TInt fault;
		test(UserHal::FaultReason(fault)==KErrGeneral);
		VerifyFile();
		}
	else
		{
		test.Next(_L("It's all gone horribly wrong"));
		test(EFalse);
		}


	//
	test.Next(_L("Move on to the next test"));
	if (buf==_L8("Warm Reset"))
		{
		test(progress.Write(0,_L8("Kernel Fault"))==KErrNone);
		test.Next(_L("Cause Kernel fault"));
		RDebug::Fault(0x1234);
		test(EFalse);
		}
	else if (buf==_L8("Kernel Fault"))
		{
		test(progress.Write(0,_L8("Kernel Exception"))==KErrNone);
		test.Next(_L("Cause Kernel exception"));
		CauseKernelException();
		test(EFalse);
		}
	else if (buf==_L8("Kernel Exception"))
		{
		test(progress.Write(0,_L8("Finalise"))==KErrNone);
		test.Next(_L("Press the user reset button..."));
		FOREVER ;
		}
	else if (buf==_L8("Finalise"))
		{
		test.Next(_L("Removing progress file"));
		progress.Close();
		r=fs.Delete(KProgressFileName());
		test(r==KErrNone);
		}
	else
		{
		test.Next(_L("It's all gone horribly wrong"));
		test(EFalse);
		}

	test.Printf(_L("\n\nTest completed O.K.\nPress LEFT SHIFT, RIGHT SHIFT and USER RESET to test hard reset\n"));
	test.Getch();
	test.End();
	return(KErrNone);
	}

#endif
