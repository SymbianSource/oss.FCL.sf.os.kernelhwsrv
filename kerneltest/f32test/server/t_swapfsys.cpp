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
// f32test\server\t_swapfilesystem.cpp
// 
//

#include <f32file.h>
#include <e32test.h>

_LIT(KTestString,"t_swapfilesystem");

LOCAL_D RTest test(KTestString);

LOCAL_D RFs TheFs;

LOCAL_D TChar gDriveToTest;

_LIT(KFsNameComposite,"Composite");
_LIT(KFsNameFat,      "Fat");

//---------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_SWAPFSYS-0602
//! @SYMTestCaseDesc    Swap current file system with current file system
//! @SYMDEF 			DEF101639 RFs::SwapFileSystem is not tested by f32test
//! @SYMTestPriority    Medium
//! @SYMTestActions     1. Get name of the file system currently mounted on the drive
//! 					2. Swap current file system with the same one
//! @SYMTestExpectedResults 1. Error code KErrAlreadyExists if Composite file is
//!							   current file system
//!							2. Error code KErrNotSupported if the drive is Z as on this drive,
//!							   only composite file system can be swapped only once
//!							3. Error codes KErrNone, KErrNotSupported or KErrInUse, otherwise.
//---------------------------------------------------------------------------------------------

LOCAL_C TInt TestSwapFSCurrent(TChar aDriveLetter)
// 
// If no file system is mounted / error in getting file system name, skip the test
// 
	{
	TInt 		err = KErrNone;
	TInt 		driveNumber;
	TFileName 	curFSName;
	TFileName 	newFSName;
	
	test.Next(_L("\nSwap current file system with same file system"));
	
	test.Printf(_L("\nTest is run on drive %c"),TUint(aDriveLetter));
	
	err = TheFs.CharToDrive(aDriveLetter, driveNumber);
	if(KErrNone != err)
		{
		test.Printf(_L("\nInvalid drive"));
		test(EFalse);
		}
	
	// Find current file system name
	err = TheFs.FileSystemName(curFSName,driveNumber);
	if(KErrNone != err)
		{
		test.Printf(_L("\nSkipping current test as File System is NOT mounted / error in getting the name."));
		return (err);
		}
	
	test.Printf(_L("\nName of the file system currently mounted on this drive is %s"),curFSName.Ptr());
	
	newFSName = curFSName;
	
	test.Printf(_L("\nName of the file system to be mounted on this drive is %s"),newFSName.Ptr());
	
	err = TheFs.SwapFileSystem(curFSName,newFSName,driveNumber);
	
	test.Printf(_L("\nError code is %d"),err);
	
	if(curFSName.CompareF(KFsNameComposite) == 0)
		{
		// This is unique case: 
		// Swap Composite (current) with Composite (new)
		test(KErrAlreadyExists == err);
		}
	else if(driveNumber == EDriveZ)
		{
		// This is another such case: 
		// On Z Drive, only Swap with Composite is allowed
		test(KErrNotSupported  == err);
		}
	else
		{
		// Other / generic cases
		test(KErrNone          == err || 
			 KErrNotSupported  == err ||
		     KErrInUse         == err   );
		}
	
	test.Printf(_L("\nCurrent test is completed."));
	
	return (KErrNone);
	}

//---------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_SWAPFSYS-0603
//! @SYMTestCaseDesc    Swap current file system with FAT file system
//! @SYMDEF 			DEF101639 RFs::SwapFileSystem is not tested by f32test
//! @SYMTestPriority    Medium
//! @SYMTestActions     1. Get name of the file system currently mounted on the drive
//! 					2. Swap the current file system with FAT file system
//! @SYMTestExpectedResults 1. Error code KErrNotSupported on drive Z
//!							2. Error codes KErrNone, KErrNotSupported or KErrInUse, otherwise
//---------------------------------------------------------------------------------------------
LOCAL_C TInt TestSwapFSFat(TChar aDriveLetter)
// 
// It is always assumed that FAT is always available!!
// If no file system is mounted / error in getting file system name, skip the test
// 
	{
	TInt 		err = KErrNone;
	TInt 		driveNumber;
	TFileName 	curFSName;
	TFileName 	newFSName;
	
	test.Next(_L("\nSwap current file system with FAT File System"));
	
	test.Printf(_L("\nTest is run on drive %c"),TUint(aDriveLetter));
	err = TheFs.CharToDrive(aDriveLetter, driveNumber);
	if(KErrNone != err)
		{
		test.Printf(_L("\nInvalid drive"));
		test(EFalse);
		}
	
	// Find current file system name
	err = TheFs.FileSystemName(curFSName,driveNumber);
	if(KErrNone != err)
		{
		test.Printf(_L("\nSkipping current test as File System is NOT mounted / error in getting the name."));
		return (err);
		}
	
	test.Printf(_L("\nName of the file system currently mounted on this drive is %s"),curFSName.Ptr());
	
	newFSName = KFsNameFat;
	
	test.Printf(_L("\nName of the file system to be mounted on this drive is %s"),newFSName.Ptr());
	
	err = TheFs.SwapFileSystem(curFSName,newFSName,driveNumber);
	
	test.Printf(_L("\nError code is %d"),err);
	
	if(driveNumber == EDriveZ)
		{
		// This is unique case: 
		// On Z Drive, only Swap with Composite is allowed
		test(KErrNotSupported  == err);
		}
	else
		{
		// Other / generic cases
		test(KErrNone          == err || 
			 KErrNotSupported  == err ||
		     KErrInUse         == err   );		
		}
	
	test.Printf(_L("\nCurrent test is completed."));
	
	return (KErrNone);
	}


//---------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_SWAPFSYS-0604
//! @SYMTestCaseDesc    Swap current file system with Composite File System
//! @SYMDEF 			DEF101639 RFs::SwapFileSystem is not tested by f32test
//! @SYMTestPriority    Medium
//! @SYMTestActions     1. Add Composite file system to the file server
//!						   to know its availability
//! 					2. Get the current file system name mounted on the test drive
//! 					3. Swap the current file system with Composite file system
//! @SYMTestExpectedResults 1. KErrNotFound is composite file system is not available
//!							2. Error codes KErrAlreadyExists, KErrInUse or KErrNone, on drive Z
//!							3. Error codes KErrNotSupported, KErrAlreadyExists or KErrInUse, 
//!                            on other drives
//---------------------------------------------------------------------------------------------
LOCAL_C TInt TestSwapFSComposite(TChar aDriveLetter)
// 
// If no file system is mounted / error in getting file system name, skip the test
// 
	{
	TInt 		err = KErrNone;
	TInt 		driveNumber;
	TFileName 	curFSName;
	TFileName 	newFSName;
	TBool		compFSAvailable = EFalse;
	
	test.Next(_L("\nSwap current file system with Composite File System"));
	
	test.Printf(_L("\nTest is run on drive is %c"),TUint(aDriveLetter));
	err = TheFs.CharToDrive(aDriveLetter, driveNumber);
	if(KErrNone != err)
		{
		test.Printf(_L("\nInvalid drive"));
		test(EFalse);
		}
	
	err = TheFs.AddFileSystem(_L("ecomp.fsy"));
	
	if (KErrNone == err || KErrAlreadyExists == err)
		{
		compFSAvailable = ETrue;
		}
	
	// Find current file system name
	err = TheFs.FileSystemName(curFSName,driveNumber);
	if(KErrNone != err)
		{
		test.Printf(_L("\nFile System NOT mounted. Quiting current test"));
		return (err);
		}
	
	test.Printf(_L("\nName of the file system currently mounted on this drive is %s"),curFSName.Ptr());
	
	newFSName = KFsNameComposite;
	
	test.Printf(_L("\nName of the file system to be mounted on this drive is %s"),newFSName.Ptr());
	
	err = TheFs.SwapFileSystem(curFSName,newFSName,driveNumber);
	
	test.Printf(_L("\nError code is %d"),err);
	
	if(compFSAvailable)
		{
		//Composite is available!!
		if(driveNumber == EDriveZ)
			{
			test(KErrAlreadyExists == err ||
				 KErrInUse         == err ||
				 KErrNone		   == err   );
			}
		else
			{
			test(KErrNotSupported  == err ||
			 	 KErrAlreadyExists == err ||
			     KErrInUse         == err   );			
			}
		}
	else
		{
			//Composote NOT available!!
			test(KErrNotFound == err);		
		}
	
	test.Printf(_L("\nCurrent test is completed."));
	
	return (KErrNone);	
	}

LOCAL_C void ParseCommandArguments()
//
//
//
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	TFileName thisfile=RProcess().FileName();
	if (token.MatchF(thisfile)==0)
		{
		token.Set(lex.NextToken());
		}
	test.Printf(_L("CLP=%S"),&token);

	if(token.Length()!=0)		
		{
		gDriveToTest=token[0];
		gDriveToTest.UpperCase();
		}
	else						
		gDriveToTest='C';
	}


GLDEF_C TInt E32Main()
//
// 
//
	{
	TInt err;
	
	test.Title();
	
	err = TheFs.Connect();
	
	test(KErrNone == err);
	
	ParseCommandArguments();
	
#if defined(__WINS__)
		// The emulator z: drive maps onto the PCs local file system
		// and swapping / unmounting it is not allowed.
		// Hence, skip the test on emulated Z: drive.
	if (gDriveToTest != 'Z')
		{
		TestSwapFSCurrent(gDriveToTest);
		TestSwapFSFat(gDriveToTest);
		TestSwapFSComposite(gDriveToTest);
		}
#else
	TestSwapFSCurrent(gDriveToTest);
	TestSwapFSFat(gDriveToTest);
	TestSwapFSComposite(gDriveToTest);
#endif
	
	TheFs.Close();
	
	test.Printf(_L("t_swapfilesystem completed successfully"));
	
	test.Close();
	
	return KErrNone;
	}
