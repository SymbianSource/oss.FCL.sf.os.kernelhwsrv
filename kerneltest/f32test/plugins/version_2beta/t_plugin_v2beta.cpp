// Copyright (c) 2006-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\plugins\version_2beta\encrypt\t_plugin_v2beta.cpp
//
//

#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include <f32dbg.h>
#include "f32_test_utils.h"
#include "t_server.h"
#include "encrypt.h"
#include "hex.h"

GLREF_C void TestIfEqual( TInt aValue, TInt aExpected, TInt aLine, const char aFileName[]);

#define TEST_FOR_ERROR( r )	{ TInt _r = (r); if (_r < 0) HandleError(_r, __LINE__); }
#define TEST_FOR_VALUE( r, expected ) TestIfEqual( r, expected, __LINE__, __FILE__)

LOCAL_C TBool gFileSystemExtensionLoaded = EFalse;

LOCAL_C void HandleError(TInt aError, TInt aLine)
	{
	test.Printf(_L("Error %d\n"), aError);
	test.operator()(EFalse, aLine);
	}

_LIT(KLitTestFileName,"encryptedfile.txt");

_LIT8( KSampleText, "line 0 abcdefghijklmnopqrstuvwxyz\n"
					"line 1 abcdefghijklmnopqrstuvwxyz\n"
					"line 2 abcdefghijklmnopqrstuvwxyz\n"
					"line 3 abcdefghijklmnopqrstuvwxyz\n"
					"line 4 abcdefghijklmnopqrstuvwxyz\n"
					"line 5 abcdefghijklmnopqrstuvwxyz\n"
					"line 6 abcdefghijklmnopqrstuvwxyz\n"
					"line 7 abcdefghijklmnopqrstuvwxyz\n"
					"line 8 abcdefghijklmnopqrstuvwxyz\n"
					"line 9 abcdefghijklmnopqrstuvwxyz\n");

_LIT( KValueTestFailMsg, "ERROR Got %d expected %d" );
GLDEF_C void TestIfEqual( TInt aValue, TInt aExpected, TInt aLine, const char aFileName[])
	{
	if( aExpected != aValue )
		{
		TText filenameU[512];
		TUint i = 0;
		for (; (i < sizeof(filenameU)) && (aFileName[i] != (char)0); i++)
			{
			filenameU[i]=aFileName[i];
			}
		filenameU[i]=0;
		test.Printf( KValueTestFailMsg, aValue, aExpected );
		test.operator()( EFalse, aLine, &filenameU[0]);
		}
	}

GLDEF_D RTest test(_L("t_plugin_v2beta"));


void TestFileAccessBeforeEncryptionPlugin()
	{

	//Check that we can read all the files we want before the
	//encryption plugin is loaded.
	test.Next(_L("Test opening files before encryption plugin is installed"));

	RFile file;
	TInt r = file.Replace(TheFs, KLitTestFileName, EFileShareAny);
	TEST_FOR_ERROR(r);

	TBuf8<512> binBuffer;
	binBuffer.Copy(KSampleText);
	Encrypt(binBuffer);

	TBuf8<512*2> hexBuffer;
	Hex(binBuffer, hexBuffer);

	r = file.Write(hexBuffer);
	TEST_FOR_ERROR(r);

	r = file.Flush();
	TEST_FOR_ERROR(r);

	file.Close();
	}

void TestLoadingOfEncryptionPlugin()
	{
	test.Next(_L("Test the loading of the encryption plugin"));

	// Try loading the encryption plugin.
	TInt r = TheFs.AddPlugin(_L("t_enchook"));
	TEST_FOR_ERROR(r);

	// Try loading the encryption plugin again.
	r = TheFs.AddPlugin(_L("t_enchook"));
	TEST_FOR_VALUE(r, KErrAlreadyExists);


	// Try mounting the plugin
	r = TheFs.MountPlugin(_L("EncHook"));
	TEST_FOR_ERROR(r);

	//Test the name functions
	TFullName encName;
	r = TheFs.PluginName(encName,0,1);
	test.Printf(_L("Encryption plugin name is: %S\n"), &encName);

	}

void TestUnloadingOfEncryptionPlugin()
	{
	test.Next(_L("Test unloading the encryption plugin"));

	//Unload the encryption plugin
	//Wait for it to empty it's input queue.
	User::After(3000000);

	TInt r = TheFs.DismountPlugin(_L("EncHook"));
	TEST_FOR_ERROR(r);

	r = TheFs.DismountPlugin(_L("EncHook"));
	TEST_FOR_VALUE(r, KErrNotFound);

	r = TheFs.RemovePlugin(_L("EncHook"));
	TEST_FOR_ERROR(r);

	r = TheFs.RemovePlugin(_L("EncHook"));
	TEST_FOR_VALUE(r, KErrNotFound);
	}


void TestFileAccessDuringEncryptionPluginL()
	{

	RFile file;
	TBuf8<512> buf;
	TInt len = KSampleText().Length();
	TPtr8 buffer((TUint8*) buf.Ptr(), len, len);

	test.Next(_L("Test opening & reading files with the encryption plugin installed"));

	TInt r = file.Open(TheFs, KLitTestFileName, EFileShareAny);
	TEST_FOR_ERROR(r);


	buffer.SetLength(KSampleText().Length());
	r = file.Read(buffer);

	TInt driveNum = CurrentDrive();
	TFSName name;
	TInt err = TheFs.FileSystemName(name, driveNum);

	if (err != KErrNone)
		{

		test.Printf(_L("Drive %C: is not ready!"), 'A'+driveNum);
		test(EFalse);

		}
	else
		if ((gFileSystemExtensionLoaded || F32_Test_Utils::Is_Win32(TheFs,driveNum)) && r == KErrNotSupported)
			{
			test.Printf(_L("File system extension does not support local buffers\n"));
			file.Close();
			return;
			}

	TEST_FOR_ERROR(r);

	r = buffer.Compare(KSampleText);
	TEST_FOR_VALUE(r, KErrNone);


	// read again - this should be read from cache
	TInt startPos = 0;
	file.Seek(ESeekStart, startPos);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	r = buffer.Compare(KSampleText);
	TEST_FOR_VALUE(r, KErrNone);


	file.Close();
	}


void TestFileAccessAfterEncryptionPlugin()
	{

	test.Next(_L("Test opening files after encryption plugin is uninstalled"));

	}

void TestFormatDriveIntercept()
    {
    test.Next(_L("Test intercepting of formatting of the drive"));
    RFormat format;

    TInt tracksRemaining;
    gSessionPath = _L("?:\\F32-TST\\");
    gSessionPath[0] = (TText) gDriveToTest;
    TInt r = format.Open(TheFs, gSessionPath, EQuickFormat, tracksRemaining);
    TEST_FOR_VALUE(r, KErrNone);

    // Don't format the whole drive to save time since
    // it is plugin intercept which is being tested and not
    // the format operation itself
    r = format.Next(tracksRemaining);
    TEST_FOR_VALUE(r, KErrNone);

    format.Close();
    }

void TestFormatDriveAfterFormatPlugin()
    {
    test.Next(_L("Test formatting of the drive after plugin unloaded"));
    RFormat format;

    TInt tracksRemaining;
    gSessionPath = _L("?:\\F32-TST\\");
    gSessionPath[0] = (TText) gDriveToTest;

    TInt r = format.Open(TheFs, gSessionPath, EQuickFormat, tracksRemaining);
    TEST_FOR_VALUE(r, KErrNone);


    while(tracksRemaining)
        {
        r = format.Next(tracksRemaining);
        TEST_FOR_VALUE(r, KErrNone);
        }

    format.Close();
    }

void TestLoadingOfHexPlugin()
	{
	test.Next(_L("Test the loading of the hex plugin"));

	// Try loading the hex plugin.
	TInt r = TheFs.AddPlugin(_L("t_hexhook"));
	TEST_FOR_ERROR(r);

	// Try loading the hex plugin again.
	r = TheFs.AddPlugin(_L("t_hexhook"));
	TEST_FOR_VALUE(r, KErrAlreadyExists);


	// Try mounting the plugin
	r = TheFs.MountPlugin(_L("HexHook"));
	TEST_FOR_ERROR(r);

	//Test the name functions
	TFullName hexName;
	r = TheFs.PluginName(hexName,0,2);
	test.Printf(_L("Hex plugin name is: %S\n"), &hexName);

	}

void TestUnloadingOfHexPlugin()
	{
	test.Next(_L("Test unloading the hex plugin"));

	//Unload the hex plugin
	//Wait for it to empty it's input queue.
	User::After(3000000);

	TInt r = TheFs.DismountPlugin(_L("HexHook"));
	TEST_FOR_ERROR(r);

	r = TheFs.DismountPlugin(_L("HexHook"));
	TEST_FOR_VALUE(r, KErrNotFound);

	r = TheFs.RemovePlugin(_L("HexHook"));
	TEST_FOR_ERROR(r);

	r = TheFs.RemovePlugin(_L("HexHook"));
	TEST_FOR_VALUE(r, KErrNotFound);
	}


void TestLoadingOfTracePlugin()
	{
	test.Next(_L("Test the loading of the trace plugin"));

	// Try loading the trace plugin.
	TInt r = TheFs.AddPlugin(_L("t_tracehook"));
	TEST_FOR_ERROR(r);

	// Try loading the trace plugin again.
	r = TheFs.AddPlugin(_L("t_tracehook"));
	TEST_FOR_VALUE(r, KErrAlreadyExists);


	// Try mounting the plugin
	r = TheFs.MountPlugin(_L("TraceHook"));
	TEST_FOR_ERROR(r);

	//Test the name functions
	TFullName traceName;
	r = TheFs.PluginName(traceName,0,0);
	test.Printf(_L("Trace plugin name is: %S\n"), &traceName);

	}

void TestUnloadingOfTracePlugin()
	{
	test.Next(_L("Test unloading the trace plugin"));

	//Unload the trace plugin
	//Wait for it to empty it's input queue.
	User::After(3000000);

	TInt r = TheFs.DismountPlugin(_L("TraceHook"));
	TEST_FOR_ERROR(r);

	r = TheFs.DismountPlugin(_L("TraceHook"));
	TEST_FOR_VALUE(r, KErrNotFound);

	r = TheFs.RemovePlugin(_L("TraceHook"));
	TEST_FOR_ERROR(r);

	r = TheFs.RemovePlugin(_L("TraceHook"));
	TEST_FOR_VALUE(r, KErrNotFound);
	}

void TestLoadingOfFormatPlugin()
    {
    test.Next(_L("Test the loading of the format plugin"));

    // Try loading the format plugin.
    TInt r = TheFs.AddPlugin(_L("t_formathook"));
    TEST_FOR_ERROR(r);

    // Try loading the format plugin again.
    r = TheFs.AddPlugin(_L("t_formathook"));
    TEST_FOR_VALUE(r, KErrAlreadyExists);


    // Try mounting the plugin
    r = TheFs.MountPlugin(_L("FormatHook"));
    TEST_FOR_ERROR(r);

    //Test the name functions
    TFullName formatName;
    r = TheFs.PluginName(formatName,0,0);
    test.Printf(_L("Format plugin name is: %S\n"), &formatName);

    }

void TestUnloadingOfFormatPlugin()
    {
    test.Next(_L("Test unloading the format plugin"));

    //Unload the format plugin
    //Wait for it to empty it's input queue.
    User::After(3000000);

    TInt r = TheFs.DismountPlugin(_L("FormatHook"));
    TEST_FOR_ERROR(r);

    r = TheFs.DismountPlugin(_L("FormatHook"));
    TEST_FOR_VALUE(r, KErrNotFound);

    r = TheFs.RemovePlugin(_L("FormatHook"));
    TEST_FOR_ERROR(r);

    r = TheFs.RemovePlugin(_L("FormatHook"));
    TEST_FOR_VALUE(r, KErrNotFound);
    }

void DeleteFiles()
	{
	test.Next(_L("Cleanup files"));

	TInt r = TheFs.Delete(KLitTestFileName);
	TEST_FOR_ERROR(r);
	}




GLDEF_C void CallTestsL()
	{
	TInt theDrive;
	TInt r = TheFs.CharToDrive(gDriveToTest,theDrive);
	test(r == KErrNone);
	TVolumeInfo volInfo;
	r = TheFs.Volume(volInfo, theDrive);
	test (r == KErrNone);
	if (volInfo.iDrive.iType == EMediaRam)
		{
#if defined(__WINS__)
		if(gDriveToTest != 'C')
			{
#endif
			test.Printf(_L("Plugin not supported on RAM drive\n"));
			return;
#if defined(__WINS__)
			}
#endif
		}


	TFullName extName;
	r = TheFs.ExtensionName(extName,theDrive, 0);
	if (r == KErrNone)
		{
		test.Printf(_L("File system extension is present (%S)\n"), &extName);
		gFileSystemExtensionLoaded = ETrue;
		}
	else
		{
		test.Printf(_L("File system extension not present.\n"));
		}


	TestFileAccessBeforeEncryptionPlugin();

	TestLoadingOfTracePlugin();
	TestLoadingOfEncryptionPlugin();
	TestLoadingOfHexPlugin();

	TestFileAccessDuringEncryptionPluginL();

	TestUnloadingOfHexPlugin();
	TestUnloadingOfEncryptionPlugin();
	TestUnloadingOfTracePlugin();

	TestFileAccessAfterEncryptionPlugin();

	DeleteFiles();


	// run T_FILE with trace plugin installed

#if defined(__WINS__)	// only in WINS to save time
	TestLoadingOfTracePlugin();

	RProcess p;

	TBuf<4> driveBuf=_L("?");
	driveBuf[0] = (TText) gDriveToTest;

	test.Next(_L("Test running T_FILE with plugin installed"));

	r = p.Create(_L("T_FILE.exe"), driveBuf);
	test(r==KErrNone);
	TRequestStatus status;
	p.Logon(status);
	p.Resume();
	User::WaitForRequest(status);
	TestUnloadingOfTracePlugin();
#endif // __WINS__

	// Cannot format drive C: so skip this test on that drive
	if (!F32_Test_Utils::Is_SimulatedSystemDrive(TheFs, EDriveC))
	    {
        TestLoadingOfFormatPlugin();
        TestFormatDriveIntercept();
        TestUnloadingOfFormatPlugin();
        TestFormatDriveAfterFormatPlugin();
        }
	}

