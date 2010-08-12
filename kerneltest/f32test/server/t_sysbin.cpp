// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_sysbin.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <f32file.h>
#include <e32ldr.h>
#include <e32ldr_private.h>

_LIT(KCheckFailed,"Check failed %d != %d\n");

#define CHECK_EQ(a,b) { if(a!=b) { test.Printf(KCheckFailed,a,b); test(0); } }

RTest test(_L("T_SYSBIN"));

TBool SysBinEnforced = 0;
TBool RunningFromROM;

RFs TheFs;
CFileMan* TheFileMan;

void CheckFileName(RProcess aProcess, const TDesC& aFileName)
	{
	TFileName fileName = aProcess.FileName();
	test(fileName.MatchF(aFileName)==0);
	}

_LIT(KExeAInSysBin,"z:\\sys\\bin\\t_sysbina.exe");
_LIT(KExeBInSystemPrograms,"z:\\system\\programs\\t_sysbinb.exe");
_LIT(KDllAInSysBin,"z:\\sys\\bin\\t_sysbin_dlla.dll");
_LIT(KDllBInSystemLibs,"z:\\system\\libs\\t_sysbin_dllb.dll");
#ifndef __WINS__
_LIT(KExeBInSysBin,"z:\\sys\\bin\\t_sysbinb.exe");
_LIT(KDllBInSysBin,"z:\\sys\\bin\\t_sysbin_dllb.dll");
#endif
_LIT(KRamDll,"z:\\sys\\bin\\t_sysbin_dll_ram.dll");
_LIT(KDllCInCTest,"c:\\sysbin_test\\t_sysbin_dllc.dll");
_LIT(KDllC,"t_sysbin_dllc.dll");
_LIT(KDllDInCSysBinTest,"c:\\sys\\bin\\test\\t_sysbin_dlld.dll");
_LIT(KDllD,"t_sysbin_dlld.dll");


void CheckFileName(RLibrary aLibrary, const TDesC& aFileName)
	{
	TFileName fileName = aLibrary.FileName();
	test(fileName.MatchF(aFileName)==0);
	}



void TestExeB(const TDesC& aFileName)
	{
	RProcess p;
	TInt r = p.Create(aFileName,KNullDesC);
	if(SysBinEnforced)
		{
#ifdef __WINS__
		CHECK_EQ(r,KErrNotFound)
#else
		CHECK_EQ(r,KErrNone)
		CheckFileName(p,KExeBInSysBin);
		p.Terminate(0);
		p.Close();
#endif
		}
	else
		{
		CHECK_EQ(r,KErrNone)
		CheckFileName(p,KExeBInSystemPrograms);
		p.Terminate(0);
		p.Close();
		}
	}



void TestDllB(const TDesC& aFileName)
	{
	RLibrary l;
	TInt r = l.Load(aFileName);
	if(SysBinEnforced)
		{
#ifdef __WINS__
		CHECK_EQ(r,KErrNotFound)
#else
		CHECK_EQ(r,KErrNone)
		CheckFileName(l,KDllBInSysBin);
		l.Close();
#endif
		}
	else
		{
		CHECK_EQ(r,KErrNone)
		CheckFileName(l,KDllBInSystemLibs);
		l.Close();
		}
	}



void TestDllC(const TDesC& aFileName,const TDesC& aPath)
	{
	RLibrary l;
	test.Printf(_L("Load(%S,%S)\n"),&aFileName,&aPath);
	TInt r = l.Load(aFileName,aPath);
	if(SysBinEnforced)
		CHECK_EQ(r,KErrNotFound)
	else
		{
		CHECK_EQ(r,KErrNone)
		CheckFileName(l,KDllCInCTest);
		l.Close();
		}
	}



void TestDllD(const TDesC& aFileName,const TDesC& aPath)
	{
	RLibrary l;
	test.Printf(_L("Load(%S,%S)\n"),&aFileName,&aPath);
	TInt r = l.Load(aFileName,aPath);
	CHECK_EQ(r,KErrNone)
	CheckFileName(l,KDllDInCSysBinTest);
	l.Close();
	}



GLDEF_C TInt E32Main()
    {
	SysBinEnforced=PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin);

	TInt r;
	RProcess p;
	RLibrary l;

 	test.Title();

	test( KErrNone == TheFs.Connect() );
	RunningFromROM = TheFs.IsFileInRom(RProcess().FileName())!=0;

	// Turn off evil lazy dll unloading
	{
	RLoader l;
	test_KErrNone(l.Connect());
	test_KErrNone(l.CancelLazyDllUnload());
	l.Close();
	}

	CTrapCleanup* ct = CTrapCleanup::New();
	test_NotNull(ct);
	TRAP(r, TheFileMan=CFileMan::NewL(TheFs));
	test_KErrNone(r);

	test.Start(_L("Check loading an EXE which lives in \\SYS\\BIN"));

		test.Start(_L("Load without path or ext"));
		r = p.Create(_L("T_SYSBINa"),KNullDesC);
		CHECK_EQ(r,KErrNone)
		CheckFileName(p,KExeAInSysBin);
		p.Terminate(0);
		p.Close();

		test.Next(_L("Load without path"));
		r = p.Create(_L("T_SYSBINa.EXE"),KNullDesC);
		CHECK_EQ(r,KErrNone)
		CheckFileName(p,KExeAInSysBin);
		p.Terminate(0);
		p.Close();

		test.Next(_L("Load with path but without ext"));
		r = p.Create(_L("\\Sys\\Bin\\T_SYSBINa"),KNullDesC);
		CHECK_EQ(r,KErrNone)
		CheckFileName(p,KExeAInSysBin);
		p.Terminate(0);
		p.Close();

		test.Next(_L("Load with path and ext"));
		r = p.Create(_L("\\Sys\\Bin\\T_SYSBINa.EXE"),KNullDesC);
		CHECK_EQ(r,KErrNone)
		CheckFileName(p,KExeAInSysBin);
		p.Terminate(0);
		p.Close();

		test.Next(_L("Load with SYSTEM\\PROGRAMS path and without ext"));
		r = p.Create(_L("\\SYSTEM\\PROGRAMS\\T_SYSBINa"),KNullDesC);
		if(!SysBinEnforced)
			CHECK_EQ(r,KErrNotFound)
		else
			{
			CHECK_EQ(r,KErrNone)
			CheckFileName(p,KExeAInSysBin);
			p.Terminate(0);
			p.Close();
			}

		test.Next(_L("Load with SYSTEM\\PROGRAMS path and with ext"));
		r = p.Create(_L("\\SYSTEM\\PROGRAMS\\T_SYSBINa.EXE"),KNullDesC);
		if(!SysBinEnforced)
			CHECK_EQ(r,KErrNotFound)
		else
			{
			CHECK_EQ(r,KErrNone)
			CheckFileName(p,KExeAInSysBin);
			p.Terminate(0);
			p.Close();
			}

		test.End();

	test.Next(_L("Check loading an EXE which lives in \\SYSTEM\\PROGRAMS"));

	if(!RunningFromROM)
		{
		test.Printf(_L("TESTS NOT RUN - Not running from ROM"));
		}
	else
		{
		test.Start(_L("Load without path or ext"));
		TestExeB(_L("T_SYSBINb"));

		test.Next(_L("Load without path"));
		TestExeB(_L("T_SYSBINb.EXE"));

		test.Next(_L("Load with path but without ext"));
		TestExeB(_L("\\System\\programs\\T_SYSBINb"));

		test.Next(_L("Load with path and ext"));
		TestExeB(_L("\\System\\programs\\T_SYSBINb.EXE"));

		test.End();
		}

	test.Next(_L("Check loading an DLL which lives in \\SYS\\BIN"));

		test.Start(_L("Load without path or ext"));
		r = l.Load(_L("T_SYSBIN_DLLa"));
		CHECK_EQ(r,KErrNone)
		CheckFileName(l,KDllAInSysBin);
		l.Close();

		test.Next(_L("Load without path"));
		r = l.Load(_L("T_SYSBIN_DLLa.DLL"));
		CHECK_EQ(r,KErrNone)
		CheckFileName(l,KDllAInSysBin);
		l.Close();

		test.Next(_L("Load with path but without ext"));
		r = l.Load(_L("\\Sys\\Bin\\T_SYSBIN_DLLa"));
		CHECK_EQ(r,KErrNone)
		CheckFileName(l,KDllAInSysBin);
		l.Close();

		test.Next(_L("Load with path and ext"));
		r = l.Load(_L("\\Sys\\Bin\\T_SYSBIN_DLLa.DLL"));
		CHECK_EQ(r,KErrNone)
		CheckFileName(l,KDllAInSysBin);
		l.Close();

		test.Next(_L("Load with SYSTEM\\LIBS path and without ext"));
		r = l.Load(_L("\\SYSTEM\\LIBS\\T_SYSBIN_DLLa"));
		if(!SysBinEnforced)
			CHECK_EQ(r,KErrNotFound)
		else
			{
			CHECK_EQ(r,KErrNone)
			CheckFileName(l,KDllAInSysBin);
			l.Close();
			}

		test.Next(_L("Load with SYSTEM\\LIBS path and with ext"));
		r = l.Load(_L("\\SYSTEM\\LIBS\\T_SYSBIN_DLLa.DLL"));
		if(!SysBinEnforced)
			CHECK_EQ(r,KErrNotFound)
		else
			{
			CHECK_EQ(r,KErrNone)
			CheckFileName(l,KDllAInSysBin);
			l.Close();
			}

		test.End();

	test.Next(_L("Check loading an DLL which lives in \\SYSTEM\\LIBS"));

	if(!RunningFromROM)
		{
		test.Printf(_L("TESTS NOT RUN - Not running from ROM"));
		}
	else
		{
		test.Start(_L("Load without path or ext"));
		TestDllB(_L("T_SYSBIN_DLLb"));

		test.Next(_L("Load without path"));
		TestDllB(_L("T_SYSBIN_DLLb.DLL"));

		test.Next(_L("Load with path but without ext"));
		TestDllB(_L("\\System\\Libs\\T_SYSBIN_DLLb"));

		test.Next(_L("Load with path and ext"));
		TestDllB(_L("\\System\\Libs\\T_SYSBIN_DLLb.DLL"));

		test.End();
		}

	test.Next(_L("Check loading an DLL which lives in \\SYSBIN_TEST"));

	r = TheFileMan->Copy(KRamDll,KDllCInCTest,CFileMan::ERecurse);
	test_KErrNone(r);

		test.Start(_L("Load using full path+name"));
		TestDllC(KDllCInCTest,KNullDesC);

		test.Next(_L("Load using separate search path"));
		TestDllC(KDllC,_L("\\sysbin_test\\"));
#ifndef __WINS__
		TestDllC(KDllC,_L("\\sysbin_test"));
		TestDllC(KDllC,_L("sysbin_test\\"));
		TestDllC(KDllC,_L("sysbin_test"));
#endif

		TestDllC(KDllC,_L("c:\\sysbin_test\\"));
#ifndef __WINS__
		TestDllC(KDllC,_L("c:\\sysbin_test"));
		TestDllC(KDllC,_L("c:sysbin_test\\"));
		TestDllC(KDllC,_L("c:sysbin_test"));
#endif

		test.End();

	// All files on the emulator's Z: drive have the KEntryAttReadOnly flag set
	// This flag will have been copied to the C: drive, so we need to remove this before calling CFileMan::RmDir()
#ifdef __WINS__
	r = TheFs.SetAtt(KDllCInCTest, 0, KEntryAttReadOnly);
	test_KErrNone(r);
#endif


	_LIT(KCTestPath,"c:\\sysbin_test\\");
	r = TheFileMan->RmDir(KCTestPath);
	test_KErrNone(r);

	test.Next(_L("Check loading an DLL which lives in \\SYS\\BIN\\TEST"));

	r = TheFileMan->Copy(KRamDll,KDllDInCSysBinTest,CFileMan::ERecurse);
	test_KErrNone(r);

		test.Start(_L("Load using full path+name"));
		TestDllD(KDllDInCSysBinTest,KNullDesC);

		test.Next(_L("Load using separate search path"));
		TestDllD(KDllD,_L("\\sys\\bin\\test\\"));
#ifndef __WINS__
		TestDllD(KDllD,_L("\\sys\\bin\\test"));
		TestDllD(KDllD,_L("sys\\bin\\test"));
		TestDllD(KDllD,_L("sys\\bin\\test\\"));
#endif

		TestDllD(KDllD,_L("c:\\sys\\bin\\test\\"));
#ifndef __WINS__
		TestDllD(KDllD,_L("c:\\sys\\bin\\test"));
		TestDllD(KDllD,_L("c:sys\\bin\\test\\"));
		TestDllD(KDllD,_L("c:sys\\bin\\test"));
#endif

		TestDllD(KDllD,_L("\\dummy;\\sys\\bin\\test\\"));
		TestDllD(KDllD,_L("\\sys\\bin\\test\\;\\dummy2"));
		TestDllD(KDllD,_L("\\dummy1;\\sys\\bin\\test2;\\sys\\bin\\test\\"));

		TestDllD(KDllD,_L("c:\\dummy;c:\\sys\\bin\\test\\"));
		TestDllD(KDllD,_L("c:\\sys\\bin\\test\\;c:\\dummy2"));
		TestDllD(KDllD,_L("c:\\dummy1;c:\\sys\\bin\\test2;c:\\sys\\bin\\test\\"));

		test.End();

	// All files on the emulator's Z: drive have the KEntryAttReadOnly flag set
	// This flag will have been copied to the C: drive, so we need to remove this before calling CFileMan::RmDir()
#ifdef __WINS__
	r = TheFs.SetAtt(KDllDInCSysBinTest, 0, KEntryAttReadOnly);
	test_KErrNone(r);
#endif


	_LIT(KCTestPath2,"c:\\sys\\bin\\test\\");
	r = TheFileMan->RmDir(KCTestPath2);
	test_KErrNone(r);

	test.End();
	return(0);
    }
