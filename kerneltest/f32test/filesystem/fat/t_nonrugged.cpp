// Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\filesystem\fat\t_nonrugged.cpp
// Functional tests for the non-Rugged file mode (also called the File Sequential mode)
// Only perform tests on the Rugged FAT file system
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>

#include "t_server.h"
#include "fat_utils.h"
using namespace Fat_Test_Utils;


RTest test(_L("T_NONRUGGED"));

TInt gDriveNum;

_LIT(KTestPath, ":\\F32-TST\\T_NONRUGGED\\");	// 22


enum TNonRuggedFileMode
	{
	ENormalFileMode,	// File is in normal (Rugged) mode
	ENonRuggedFileMode	// File is in non-Rugged file mode (EFileSequential)
	};

enum TNonRuggedControlIO
/*
 * ControlIo enum values
 */
    {
    EControlIOIsRuggedFSys		= 4,			// Defined as EIsRuggedFSys in \fileserver\sfat32\common_constants.h
    EControlIOIsFileSequential	= KMaxTInt-23	// Defined as KControlIoIsFileSequential in \fileserver\inc\f32dbg.h
    };


void CreateFile(RFile& aFile, const TDesC& aPath, TUint aFileMode)
/*
 * Creates/opens a file in Rugged/non-Rugged mode after emptying the trace buffer
 */
	{
	TInt r = TheFs.MkDirAll(aPath);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	r = aFile.Replace(TheFs, aPath, aFileMode);
	test_KErrNone(r);
	}


void DeleteFileAndDir(const TDesC& aPath)
/*
 * Deletes a single file and the directory containing it
 */
	{
	TInt r = TheFs.Delete(aPath);
	test_KErrNone(r);
	r = TheFs.RmDir(aPath);
	test_KErrNone(r);
	}


void SingleClientTest()
/*
 * Unit test for Non-Rugged File mode with a single client
	 ***********************************************************************************
	 * 1. Create a file in non-Rugged file mode and then close the file.
	 * 2. Open the file in normal Rugged file mode and then close it.
	 * 3. Open the file in non-Rugged file mode again and then close it.
	 * Expected Results:
	 * 1. File is in non-Rugged file mode.
	 * 2. File is in normal Rugged file mode.
	 * 3. File is in non-Rugged file mode.
	 ***********************************************************************************
 */
	{
	test.Next(_L("Test single client"));

	TBuf<34> singleFile;
	singleFile.Append(gDriveToTest);		//  1
	singleFile.Append(KTestPath);			// 22
	singleFile.Append(_L("file.single"));	// 11
	TInt r = TheFs.Delete(singleFile);
	test_Value(r, r == KErrNone || r == KErrNotFound || r == KErrPathNotFound);
	TBuf8<KMaxPath> singleFilePkg;
	singleFilePkg.Copy(singleFile.Mid(2));
	TUint8 fileMode = 0;
	TPtr8 fileModePkg(&fileMode, 1, 1);

	test.Printf(_L("Create file in non-Rugged file mode\n"));
	RFile file;
	CreateFile(file, singleFile, (EFileWrite | EFileSequential));
	r = TheFs.ControlIo(gDriveNum, EControlIOIsFileSequential, singleFilePkg, fileModePkg);
	test_KErrNone(r);
	test_Equal(ENonRuggedFileMode, fileMode);
	file.Close();

	test.Printf(_L("Open file in normal mode\n"));
	CreateFile(file, singleFile, EFileWrite);
	r = TheFs.ControlIo(gDriveNum, EControlIOIsFileSequential, singleFilePkg, fileModePkg);
	test_KErrNone(r);
	test_Equal(ENormalFileMode, fileMode);
	file.Close();

	test.Printf(_L("Open file in non-Rugged file mode again\n"));
	CreateFile(file, singleFile, (EFileWrite | EFileSequential));
	r = TheFs.ControlIo(gDriveNum, EControlIOIsFileSequential, singleFilePkg, fileModePkg);
	test_KErrNone(r);
	test_Equal(ENonRuggedFileMode, fileMode);
	file.Close();

	DeleteFileAndDir(singleFile);
	}


void MultipleClientsTest()
/*
 * Unit tests for Non-Rugged File mode with multiple clients accessing the same file
 */
	{
	/***********************************************************************************
	 * Use Case 1:
	 * 1. Client1 opens a file in non-Rugged file mode.
	 * 2. Client2 then opens the same file in normal Rugged file mode.
	 * Expected Results:
	 * 1. File is in non-Rugged file mode.
	 * 2. File changed to normal Rugged file mode.
	 ***********************************************************************************
	 */
	test.Next(_L("Test multiple clients - Use case 1"));

	TBuf<33> fileName1;
	fileName1.Append(gDriveToTest);		//  1
	fileName1.Append(KTestPath);		// 22
	fileName1.Append(_L("file1.mult"));	// 10
	TInt r = TheFs.Delete(fileName1);
	test_Value(r, r == KErrNone || r == KErrNotFound || r == KErrPathNotFound);
	TBuf8<31> fileName1Pkg;
	fileName1Pkg.Copy(fileName1.Mid(2));
	TUint8 fileMode = 0;
	TPtr8 fileModePkg(&fileMode, 1, 1);


	// Use Case 1.1, Client 1 (Non-Rugged Client) ------------------------------------
	test.Printf(_L("Client1 create file in non-Rugged file mode\n"));

	RFile file1;
	CreateFile(file1, fileName1, (EFileWrite | EFileSequential | EFileShareAny));
	r = TheFs.ControlIo(gDriveNum, EControlIOIsFileSequential, fileName1Pkg, fileModePkg);
	test_KErrNone(r);
	test_Equal(ENonRuggedFileMode, fileMode);


	// Use Case 1.2, Client 2 (Rugged Client) ----------------------------------------
	test.Printf(_L("Client2 open file in 'normal' Rugged file mode\n"));

	RFile file2;
	CreateFile(file2, fileName1, (EFileWrite | EFileShareAny));
	r = TheFs.ControlIo(gDriveNum, EControlIOIsFileSequential, fileName1Pkg, fileModePkg);
	test_KErrNone(r);
	test_Equal(ENormalFileMode, fileMode);


	file1.Close();
	file2.Close();
	r = TheFs.Delete(fileName1);
	test_KErrNone(r);


	/***********************************************************************************
	 * Use Case 2:
	 * 1. Client1 opens a file in normal Rugged file mode.
	 * 2. Client2 then opens the same file in non-Rugged file mode.
	 * Expected Results:
	 * 1. File is in normal Rugged file mode.
	 * 2. File does not change to non-Rugged file mode.
	 ***********************************************************************************
	 */
	test.Next(_L("Test multiple clients - Use case 2"));

	TBuf<34> fileName2;
	fileName2.Append(gDriveToTest);		//  1
	fileName2.Append(KTestPath);		// 22
	fileName2.Append(_L("file2.mult"));	// 10
	r = TheFs.Delete(fileName2);
	test_Value(r, r == KErrNone || r == KErrNotFound || r == KErrPathNotFound);
	TBuf8<KMaxPath> fileName2Pkg;
	fileName2Pkg.Copy(fileName2.Mid(2));


	// Use Case 2.1, Client 1 (Rugged Client) ----------------------------------------
	test.Printf(_L("Client1 create file in 'normal' Rugged file mode\n"));

	CreateFile(file1, fileName2, (EFileWrite | EFileShareAny));
	r = TheFs.ControlIo(gDriveNum, EControlIOIsFileSequential, fileName2Pkg, fileModePkg);
	test_KErrNone(r);
	test_Equal(ENormalFileMode, fileMode);


	// Use Case 2.2, Client 2 (Non-Rugged Client) ------------------------------------
	test.Printf(_L("Client2 open file in non-Rugged file mode\n"));

	CreateFile(file2, fileName2, (EFileWrite | EFileSequential | EFileShareAny));
	r = TheFs.ControlIo(gDriveNum, EControlIOIsFileSequential, fileName2Pkg, fileModePkg);
	test_KErrNone(r);
	test_Equal(ENormalFileMode, fileMode);


	file1.Close();
	file2.Close();
	DeleteFileAndDir(fileName2);
	}



void CallTestsL()
/*
 * Start point of T_NONRUGGED
 */
	{
#ifndef _DEBUG
    test.Printf(_L("T_NONRUGGED skipped. To run only on debug builds.\n"));
    return;
#else
    
	TInt r = TheFs.CharToDrive(gDriveToTest, gDriveNum);
	test_KErrNone(r);
	
	// Currently only FAT file system supports Rugged drive
    if (!Is_Fat(TheFs, gDriveNum))
		{
		test.Printf(_L("T_NONRUGGED skipped. Requires FAT filesystem to run.\n"));
		return;
		}

// Use this to set filesystem to Rugged if it is not set as such
#if(0)
	{
	// Ensure that the FAT filesystem is Rugged
	TUint8 ruggedVal = 0;
	TPtr8 ruggedPkg(&ruggedVal, 1, 1);
	r = TheFs.ControlIo(gDriveNum, EControlIOIsRuggedFSys, ruggedPkg);
	test_KErrNone(r);
	if (!ruggedVal)
		{
		r = TheFs.ControlIo(gDriveNumber, KControlIoRuggedOn);
		test_KErrNone(r);
		}
	}
#endif
	
    // Test to run only on a rugged FAT drive
    TUint8 ruggedVal = 0;
    TPtr8 ruggedPkg(&ruggedVal, 1, 1);
    r = TheFs.ControlIo(gDriveNum, EControlIOIsRuggedFSys, ruggedPkg);
    test_KErrNone(r);
    if (!ruggedVal)
    	{
		test.Printf(_L("T_NONRUGGED skipped. Requires Rugged FAT to run.\n"));
		return;
    	}

	test.Start(_L("T_NONRUGGED Test Start"));
	
	// Run tests
	SingleClientTest();
	MultipleClientsTest();
	
// Use this to unset filesystem to non-Rugged if it has been set above
#if(0)
	// Set filesystem back to non-Rugged
	if (!ruggedVal)
		{
		r = TheFs.ControlIo(gDriveNumber, KControlIoRuggedOff);
		test_KErrNone(r);
		}
#endif
	
	test.End();
	test.Close();
#endif
	}
