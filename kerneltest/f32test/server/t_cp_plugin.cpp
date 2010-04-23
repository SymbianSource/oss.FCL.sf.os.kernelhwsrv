// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//
//


#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include <e32test.h>
#include <hal.h>
#include "t_server.h"
#include "f32dbg.h"


GLDEF_D RTest test(_L("T_CP_PLUGIN"));

// function decalarations
void DoCodePagePluginTest();
void TestUnicodeVolumeLabel();
void TestShortNameBoundary();
void TestConsistentShortNameGeneration();
void TestConsistentShortNameExtGeneration();
void TestDuplicateLongFileNames();
void TestDuplicateLongDirNames();
void TestLeadingE5Handling();
void TestDEF130334();
void TestINC127905();
void DeleteTestDirectory();
void TestCompatibility();
void TestINC126563();


TInt LoadCodePageDll(const TDesC& aCodePageDllName)
	{
	RDebug::Printf("++ T_CP_PLUGIN.CPP LoadCodePageDll");
	
	TInt err = KErrNone;
	if(aCodePageDllName.Length())
		{
		RDebug::Print(_L("IS: codepageDllName = %S"), &aCodePageDllName);
		RLoader loader;
		err = loader.Connect();
		if (err==KErrNone)
			{
			err = loader.SendReceive(ELoadCodePage, TIpcArgs(0, &aCodePageDllName, 0));
			RDebug::Printf("\n T_CP_PLUGIN.CPP : LoadCodePageDll : loader.SendReceive == %d", err);
			loader.Close();
			}
		}
	RDebug::Printf("-- T_CP_PLUGIN.CPP LoadCodePageDll");
	return err;
	}

void DoCodePagePluginTest()
	{
	RDebug::Printf("++ T_CP_PLUGIN.CPP DoCodePagePluginTestL");

	CreateTestDirectory(_L("\\F32-TST\\T_CP_PLUGIN\\"));
	TestUnicodeVolumeLabel();
	TestShortNameBoundary();
	TestConsistentShortNameGeneration();
	TestConsistentShortNameExtGeneration();
	TestDuplicateLongFileNames();
	TestDuplicateLongDirNames();
	TestLeadingE5Handling();
	TestDEF130334();
	TestINC127905();
	DeleteTestDirectory();
	TestCompatibility();
	TestINC126563();
	
	RDebug::Printf("-- T_CP_PLUGIN.CPP DoCodePagePluginTestL");
	}


void CallTestsL(void)
	{

	test.Title();
	test.Start(_L("Starting T_CP_PLUGIN tests"));

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// Codepage dll name
	_LIT(KCP932Name,"T_CP932");
	// Test only runs on Fat file systems
	TheFs.SessionPath(gSessionPath);
	TInt driveNum = CurrentDrive();
	TFSName name;
	TInt r = TheFs.FileSystemName(name, driveNum);
	if (KErrNone == r)
		{
		if (name.Compare(_L("Fat")) != 0)
			{
			test.Printf(_L("Test only runs on 'FAT' drives"));
			}
		else
			{
			TBuf<16> CodepageDllName(KCP932Name);
			r = LoadCodePageDll(CodepageDllName);
			test_Value(r, r == KErrNone || r == KErrAlreadyExists);

			if(r == KErrNone)
				{
				// should not allow loading again codepage dll.
				r = LoadCodePageDll(CodepageDllName);
				test_Value(r, r == KErrAlreadyExists);

				}

			// Enables codepage dll implementation of LocaleUtils functions
			TInt r = TheFs.ControlIo(CurrentDrive(), KControlIoEnableFatUtilityFunctions);
			test(KErrNone == r);

			DoCodePagePluginTest();

			// Disables codepage dll implementation of LocaleUtils functions for other base tests
			r = TheFs.ControlIo(driveNum, KControlIoDisableFatUtilityFunctions);
			test_KErrNone(r);
			}
		}
	else
		{
		test.Printf(_L("Drive %C: is not ready!"), 'A'+driveNum);
		test(EFalse);
		}
#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	test.End();
	}

