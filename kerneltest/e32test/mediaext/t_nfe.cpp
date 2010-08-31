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
// e32test\mediext\t_nfe.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <f32file.h>


RTest test(_L("t_nfe"));

#include <d32locd.h>
#include <e32property.h>
#include "nfe.h"


TBusLocalDrive Drive;
TBool TheWaitFlag = EFalse;				// wait for drive to be encrypted before exiting test
TBool TheFinaliseDriveFlag = EFalse;
TBool TheDisplayStatusFlag = EFalse;	// display drive status and then exit (i.e. don't encrypt)
TBool TheEncryptDriveFlag = ETrue;

TInt FindNfeDrive(TInt aDriveNum)
/** 
Find the next NFE drive 

@return		Local drive identifier.
*/
	{
	TInt drive = KErrNotFound;
	
//	test.Printf(_L("Searching for NFE drive:\n"));
	
	for (TInt i = aDriveNum; i < KMaxLocalDrives && drive < 0; ++i)
		{
		RLocalDrive	d;
		TBool		change = EFalse;
		
		if(d.Connect(i, change) == KErrNone)
			{
//			test.Printf(_L("Connected to local drive %d\n"), i);
			TLocalDriveCapsV4			dc;
			TPckg<TLocalDriveCapsV4>	capsPack(dc);
			capsPack.FillZ();
			
			if(d.Caps(capsPack) != KErrNone)
				continue;
			if (dc.iType == EMediaNANDFlash || dc.iType == EMediaHardDisk)
				{
				TNfeDeviceInfo nfeDeviceInfo;
				TPtr8 nfeDeviceInfoBuf((TUint8*) &nfeDeviceInfo, sizeof(nfeDeviceInfo));
				nfeDeviceInfoBuf.FillZ();

				TInt r = d.QueryDevice((RLocalDrive::TQueryDevice) EQueryNfeDeviceInfo, nfeDeviceInfoBuf);

//				test.Printf(_L("EQueryNfeDeviceInfo on local drive %d returned %d\n"), i, r);
				if (r == KErrNone)
					{
					test.Printf(_L("\nFound NFE on local drive %d\n"), i);
					drive = i;
					}
				}
			d.Close();
			}
		}
	return drive;
	}



const TDesC* DriveStatus(TNfeDiskStatus aStatus)
	{
	_LIT(KNfeUnmounted, "Unmounted");
	_LIT(KNfeDecrypted, "Decrypted");
	_LIT(KNfeDecrypting, "Decrypting");
	_LIT(KNfeEncrypted, "Encrypted");
	_LIT(KNfeEncrypting, "Encrypting");
	_LIT(KNfeWiping, "Wiping");
	_LIT(KNfeCorrupted, "Corrupted");
	_LIT(KNfeUnrecognised, "Unrecognised");

	switch(aStatus)
		{
		case ENfeUnmounted:
			return &KNfeUnmounted;
		case ENfeDecrypted:
			return &KNfeDecrypted;
		case ENfeDecrypting:
			return &KNfeDecrypting;
		case ENfeEncrypted:
			return &KNfeEncrypted;
		case ENfeEncrypting:
			return &KNfeEncrypting;
		case ENfeWiping:
			return &KNfeWiping;
		case ENfeCorrupted:
			return &KNfeCorrupted;
		default:
			return &KNfeUnrecognised;

		}
	}

TInt DriveStatus(TInt aNfeDrive, TNfeDiskStatus& aStatus, TInt &aProgress)
	{
	TInt r = RProperty::Get(
		KNfeUID, 
		NFE_KEY(aNfeDrive, KNfeStatusToUiKey),
		*(TInt*) &aStatus); 
	if (r != KErrNone)
		return r;
	r = RProperty::Get(
		KNfeUID, 
		NFE_KEY(aNfeDrive, KNfeProgressToUiKey),
		*(TInt*) &aProgress); 
	return r;
	}

void DisplayNfeDeviceInfo(TInt aNfeDrive, TNfeDeviceInfo& aDeviceInfo, TBool (&aNfeDrives)[KMaxLocalDrives])
	{
//	test.Printf(_L("Stats: \n"));

	RLocalDrive	d;
	TBool change = EFalse;
	TInt r = d.Connect(aNfeDrive, change);
	test (r == KErrNone);
		
	TPtr8 nfeDeviceInfoBuf((TUint8*) &aDeviceInfo, sizeof(aDeviceInfo));
	nfeDeviceInfoBuf.FillZ();
	r = d.QueryDevice((RLocalDrive::TQueryDevice) EQueryNfeDeviceInfo, nfeDeviceInfoBuf);
	test (r == KErrNone || r == KErrNotSupported);

	d.Close();

	test.Printf(_L("iDriveCount %d\n"), aDeviceInfo.iDriveCount);
	test.Printf(_L("iMediaSizeInBytes %lx\n"), aDeviceInfo.iMediaSizeInBytes);

	for (TInt i=0; i<aDeviceInfo.iDriveCount; i++)
		{
		TNfeDriveInfo& di = aDeviceInfo.iDrives[i];

		TInt localDriveNum = di.iLocalDriveNum;
		test_Value(localDriveNum, di.iLocalDriveNum < KMaxLocalDrives);
		
		if (aNfeDrives[localDriveNum])
			continue;
		aNfeDrives[localDriveNum] = 1;

		test.Printf(_L("*** drive index %d ***\n"), i);
		test.Printf(_L("iLocalDriveNum %x\n"), di.iLocalDriveNum);
		test.Printf(_L("iDriveLetter %c\n"), di.iDriveLetter >= 0 && di.iDriveLetter <= 25 ? di.iDriveLetter +'A' : '?');
		test.Printf(_L("iState %d\n"), di.Status());

		test.Printf(_L("State = %S\n"), DriveStatus(di.Status()));

		test.Printf(_L("iEncryptStartPos %lx\n"), di.iEncryptStartPos);
		test.Printf(_L("iEncryptEndPos %lx\n"), di.iEncryptEndPos);
		test.Printf(_L("iPartitionBaseAddr %lx\n"), di.iEntry.iPartitionBaseAddr);
		test.Printf(_L("iPartitionLen %lx\n"), di.iEntry.iPartitionLen);
		test.Printf(_L("iPartitionType %x\n"), di.iEntry.iPartitionType);
		
		test.Printf(_L("iReadRequestCount %d\n"), di.iReadRequestCount);
		test.Printf(_L("iWriteRequestCount %d\n"), di.iWriteRequestCount);
		test.Printf(_L("iCodePagingRequesCount %d\n"), di.iCodePagingRequesCount);
		test.Printf(_L("iDataPagingReadRequestCount %d\n"), di.iDataPagingReadRequestCount);
		test.Printf(_L("iDataPagingWriteRequestCount %d\n"), di.iDataPagingWriteRequestCount);
		test.Printf(_L("iUniqueID %08X\n"), di.iUniqueID);
		}
	}

void EncryptDrive(TInt aNfeDrive)
	{
	// subscribe to cmd acknowledgement property - KNfeToUiKey
    RProperty propToUi;
    test.Printf(_L("Attaching ToUi property")); 
    TInt r = propToUi.Attach(KNfeUID,NFE_KEY(aNfeDrive,KNfeToUiKey));
    test.Printf(_L("Attaching returned %d"), r);    
	if (r != KErrNone)
		return;


    TRequestStatus status;
    propToUi.Subscribe( status );
    

	// Issue command
	test.Printf(_L("Encrypting drive %c...\n"), aNfeDrive+'A');
	r = RProperty::Set(
		KNfeUID, 
		NFE_KEY(aNfeDrive, KNfeToThreadKey),
		ENfeEncryptDisk); 
	test.Printf(_L("Encrypting drive %c, r %d\n"), aNfeDrive+'A', r);
	test (r == KErrNone);

	// wait for ack
	User::WaitForRequest( status );
    r = status.Int();
    test.Printf(_L("cmd status %d"), r);    
	test (r == KErrNone);
	}

void DecryptDrive(TInt aNfeDrive)
	{
	// subscribe to cmd acknowledgement property - KNfeToUiKey
    RProperty propToUi;
    test.Printf(_L("Attaching ToUi property")); 
    TInt r = propToUi.Attach(KNfeUID,NFE_KEY(aNfeDrive,KNfeToUiKey));
    test.Printf(_L("Attaching returned %d"), r);    
	if (r != KErrNone)
		return;


    TRequestStatus status;
    propToUi.Subscribe( status );
    

	// Issue command
	test.Printf(_L("Decrypting drive %c...\n"), aNfeDrive+'A');
	r = RProperty::Set(
		KNfeUID, 
		NFE_KEY(aNfeDrive, KNfeToThreadKey),
		ENfeDecryptDisk); 
	test.Printf(_L("Decrypting drive %c, r %d\n"), aNfeDrive+'A', r);
	test (r == KErrNone);

	// wait for ack
	User::WaitForRequest( status );
    r = status.Int();
    test.Printf(_L("cmd status %d"), r);    
	test (r == KErrNone);
	}

void WaitForFinish(TInt aNfeDrive, TBool aEncrypt)
	{
	TNfeDiskStatus diskStatus = ENfeCorrupted;
	TInt progress = 0;

	TInt r = DriveStatus(aNfeDrive, diskStatus, progress);
	test (r == KErrNone);
	
	// Poll progress status.
    while (diskStatus != (aEncrypt ? ENfeEncrypted : ENfeDecrypted ))
        {
		r = DriveStatus(aNfeDrive, diskStatus, progress);
		test (r == KErrNone);
		test.Printf(_L("Drive %c, r %d progress %3u%% status %S\n"), aNfeDrive+'A', r, progress, DriveStatus((TNfeDiskStatus) diskStatus));


		if (TheFinaliseDriveFlag && progress > 10)
			{
			TheFinaliseDriveFlag = EFalse;
			RFs fs;
			TInt r = fs.Connect();
			test_KErrNone(r);

			r = fs.FinaliseDrive(aNfeDrive, RFs::EFinal_RW);
			test_KErrNone(r);
			return;
			}

		User::After( 1000 * 500 );
        }
	test.Printf( _L("\nFinished\n") );
	}

//
// E32Main
//

TInt ParseCommandArguments()
	{
    TInt tokenCount = 0;
	TChar driveToTest = 'C';;

	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	
    for (TPtrC token=lex.NextToken(); token.Length() != 0;token.Set(lex.NextToken()))
		{
        tokenCount++;
		// Get the drive letter
		if (tokenCount == 1)
			{
			TChar ch = token[0];
			if (ch.IsAlpha())
				{
				if(token.Length() > 0)		
					{
					driveToTest=token[0];
					driveToTest.UpperCase();
					}
				}
			RDebug::Print(_L("drive=%C"), (TUint) driveToTest);
			continue;
			}

		else if (token.CompareF(_L("-d")) == 0)
			{
			TheEncryptDriveFlag = EFalse;
			}
		else if (token.CompareF(_L("-e")) == 0)
			{
			TheEncryptDriveFlag = ETrue;
			}
		else if (token.CompareF(_L("-f")) == 0)
			{
			TheFinaliseDriveFlag = ETrue;
			}
		else if (token.CompareF(_L("-w")) == 0)
			{
			TheWaitFlag = ETrue;
			}
		else if (token.CompareF(_L("-s")) == 0)
			{
			TheDisplayStatusFlag = ETrue;
			}
		}

	return driveToTest;
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("NFE tests"));

	RFs fs;

	TInt r = fs.Connect();
	test_KErrNone(r);

	TChar driveToTest = ParseCommandArguments();

	TInt drive;
	r = fs.CharToDrive(driveToTest,drive);
	test_KErrNone(r);


	TVolumeInfo volumeInfo;
	r = fs.Volume(volumeInfo, drive);
	test(r == KErrNone);



	TNfeDiskStatus diskStatus = ENfeCorrupted;
	TInt progress = 0;

	r = DriveStatus(drive, diskStatus, progress);
	test.Printf(_L("drive %c diskStatus %S, progress %d r %d\n"), drive+'A', DriveStatus(diskStatus), progress, r);

	if (TheDisplayStatusFlag)
		{
		test.Printf(_L("*** press any key ***"));
		test.Getch();
		test.End();
		test.Close();
		return 0;
		}

	if (r == KErrNone && diskStatus == ENfeDecrypted && TheEncryptDriveFlag)
		{
		test.Next(_L("Encrypting NFE drive"));
		EncryptDrive(drive);
		r = DriveStatus(drive, diskStatus, progress);
		test.Printf(_L("drive %c diskStatus %S, progress %d r %d\n"), drive+'A', DriveStatus(diskStatus), progress, r);
		}

	if (r == KErrNone && diskStatus == ENfeEncrypted && !TheEncryptDriveFlag)
		{
		test.Next(_L("Decrypting NFE drive"));
		DecryptDrive(drive);
		r = DriveStatus(drive, diskStatus, progress);
		test.Printf(_L("drive %c diskStatus %S, progress %d r %d\n"), drive+'A', DriveStatus(diskStatus), progress, r);
		}


	if (r == KErrNone && TheWaitFlag)
		{
		test.Next(_L("Waiting for finish"));
		WaitForFinish(drive, TheEncryptDriveFlag);
		}


	TBool nfeDrives[KMaxLocalDrives];
	memclr(nfeDrives, sizeof(nfeDrives));
	
	for(TInt nfeDrive = FindNfeDrive(0); nfeDrive != KErrNotFound; nfeDrive = FindNfeDrive(++nfeDrive))
		{
		TNfeDeviceInfo deviceInfo;
		DisplayNfeDeviceInfo(nfeDrive, deviceInfo, nfeDrives);
		}

	fs.Close();

	test.End();
	test.Close();

	return 0;
	}


