// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <f32file.h>
#include <e32test.h>
#include <e32std.h>
#include <e32svr.h>
#include <e32property.h>
#include <usbmsshared.h>
#include "t_ms_main.h"

_LIT(KMsFsyName, "MassStorageFileSystem");
_LIT(KMsFs, "MSFS.FSY");
_LIT(KPassword, "hello");

LOCAL_D TChar driveLetter;

//
// Parses the command line arguments
//
LOCAL_C void ParseCommandArguments()
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	
	TPtrC token;
	token.Set(lex.NextToken());
	if (token.Length() != 0)
		{
		driveLetter = token[0];
		driveLetter.UpperCase();
		test.Printf(_L("CmdLine Param=%S"),&token);
		}
	else
		{
		test.Printf(_L("Not enough command line arguments"));
		test(EFalse);
		}
	}
	
LOCAL_C void doTest()
	{
	ParseCommandArguments();
	
	TInt err;
	TBuf8<16> password;
	password.Copy(KPassword);
	_LIT(KEmpty, "");
	TBuf8<16> emptyString;
	emptyString.Copy(KEmpty);
	
	RFs fs;
	test(KErrNone == fs.Connect());
	
	TInt drive;
	err = fs.CharToDrive(driveLetter, drive);
	test(KErrNone == err);
	
	//Load MS file system
	err = fs.AddFileSystem(KMsFs);
    test(err == KErrNone);
	
	//Lock drive and save password
	err = fs.LockDrive(drive, emptyString, password, ETrue);
	test(err == KErrNone);
	
	//If the drive has a filesystem mounted then dismount it
	TBuf<128> oldFs;
	err = fs.FileSystemName(oldFs, drive);
	if (err == KErrNone)
		{
		err = fs.DismountFileSystem(oldFs, drive);
		test(err == KErrNone);
		}
	
	//Mount MsFs on the drive
	err = fs.MountFileSystem(KMsFsyName, drive);
	test(err == KErrNone);
	
	//Try to unlock, the drive should have already been unlocked by the file system
	err = fs.UnlockDrive(drive, password, ETrue);
	test(err == KErrAlreadyExists);
	
	//Dismount the drive
	err = fs.DismountFileSystem(KMsFsyName, drive);
	test(err == KErrNone);
	
	//Remount the original file system
	err = fs.MountFileSystem(oldFs, drive);
	test(err == KErrNone);
	
	//Relock the drive and clear the password
	err = fs.LockDrive(drive, emptyString, password, ETrue);
	test(err == KErrNone);
	err = fs.ClearPassword(drive, password);
	test(err == KErrNone);
	
	//Dismount the drive
	err = fs.DismountFileSystem(oldFs, drive);
	test(err == KErrNone);
	 
	//Mount MsFs on the drive
	err = fs.MountFileSystem(KMsFsyName, drive);
	test(err == KErrNone);
	
	//Check P&S variable, drive state should be locked
	TBuf8<16> driveStates;
	TBool stateCorrect = EFalse;
	RProperty::Get(KUsbMsDriveState_Category, EUsbMsDriveState_DriveStatus, driveStates);
	TInt i = 0;
	for (; i < 8; i++)
		{
		if (driveStates[2*i] == drive && driveStates[2*i+1] == EUsbMsDriveState_Locked)
			{
			stateCorrect = ETrue;
			}
		}
	test(stateCorrect);
	
	//Unlock drive.
	err = fs.UnlockDrive(drive, password, ETrue);
	test(err == KErrNone);
	
	//Check P&S variable, drive state should not be locked
	stateCorrect = EFalse;
	RProperty::Get(KUsbMsDriveState_Category, EUsbMsDriveState_DriveStatus, driveStates);

	for (i=0; i < 8; i++)
		{
		if (driveStates[2*i] == drive && driveStates[2*i+1] != EUsbMsDriveState_Locked)
			{
			stateCorrect = ETrue;
			}
		}
	test(stateCorrect);

	//Lock the drive
	err = fs.LockDrive(drive, emptyString, password, ETrue);
	test(err == KErrNone);
	
	//Dismount the drive
	err = fs.DismountFileSystem(KMsFsyName, drive);
	test(err == KErrNone);
	
	//Remount MsFs
	err = fs.MountFileSystem(KMsFsyName, drive);
	test(err == KErrNone);
	
	//Check P&S variable, drive state should not be locked
	stateCorrect = EFalse;
	RProperty::Get(KUsbMsDriveState_Category, EUsbMsDriveState_DriveStatus, driveStates);
	for (i = 0; i < 8; i++)
		{
		if (driveStates[2*i] == drive && driveStates[2*i+1] != EUsbMsDriveState_Locked)
			{
			stateCorrect = ETrue;
			}
		}
	test(stateCorrect);
	}
	
GLDEF_C void CallTestsL()
//
// Do all tests
//
	{
    doTest();
    }
