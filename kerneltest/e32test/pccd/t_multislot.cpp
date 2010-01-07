/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#include <e32test.h>
#include <f32file.h>
#include <d32locd.h>

static RTest test(_L("Testing t_multislot"));
_LIT(KYes, "yes");
_LIT(KNo, "no");
static RFs fs;

// this function was copied from t_sdpartition.cpp
TInt FindMmcLocalDriveNumber(TChar aDriveChar, TInt& aLocalDriveNum, TInt aDriveNum)
	{
	TInt r = fs.CharToDrive(aDriveChar, aDriveNum);
	test(r==KErrNone);

	TDriveInfo driveInfo;
    r = fs.Drive(driveInfo, aDriveNum);
    test(r==KErrNone);


	TVolumeInfo vi;
	r = fs.Volume(vi, aDriveNum);
    test(r==KErrNone);


	TMediaSerialNumber serialNum;
	r = fs.GetMediaSerialNumber(serialNum, aDriveNum);
    test(r==KErrNone);


    test.Printf(_L("Drive %C size %ld\n"), (char) aDriveChar, vi.iSize);
	TInt len = serialNum.Length();
	test.Printf(_L("Serial number (len %d) :"), len);
	TInt n;
	for (n=0; n<len; n+=16)
		{
		TBuf16<16*3 +1> buf;
		for (TInt m=n; m<n+16; m++)
			{
			TBuf16<3> hexBuf;
			hexBuf.Format(_L("%02X "),serialNum[m]);
			buf.Append(hexBuf);
			}
		buf.Append(_L("\n"));
		test.Printf(buf);
		}

	TBusLocalDrive drv;
	TBool chg(EFalse);
	aLocalDriveNum = -1;
	TInt serialNumbersMatched = 0;
	for (n=0; n<KMaxLocalDrives; n++)
		{
		r = drv.Connect(n, chg); //for user area
//RDebug::Print(_L("TBusLocalDrive::Connect(%d) %d"), n, r);

		if(r != KErrNone)
			{
			test.Printf(_L("drive %d: TBusLocalDrive::Connect() failed %d\n"), n, r);
			continue;
			}	

	    TLocalDriveCapsV5Buf capsBuf;
	    TLocalDriveCapsV5& caps = capsBuf();
		r = drv.Caps(capsBuf);
		if(r != KErrNone)
			{
			test.Printf(_L("drive %d: TBusLocalDrive::Caps() failed %d\n"), n, r);
			continue;
			}	

//RDebug::Print(_L("areaSize %ld cardCapacity %ld"), caps.iSize, caps.iFormatInfo.iCapacity);

		TPtrC8 localSerialNum(caps.iSerialNum, caps.iSerialNumLength);
		if (serialNum.Compare(localSerialNum) == 0)
			{
			serialNumbersMatched++;
			TBool sizeMatch = (vi.iSize < caps.iSize);
			test.Printf(_L("drive %d: Serial number match, size match: %S\n"), n, sizeMatch?&KYes:&KNo);
			if (sizeMatch)
				{
				aLocalDriveNum = n;
				drv.Disconnect();
				break;
				}
			}
		drv.Disconnect();
		}


	return aLocalDriveNum == -1?KErrNotFound:KErrNone;
	}


// Manual test - requires user to move a card between two physical slots
extern TInt E32Main()
	{
	test.Start(_L("T_MULTISLOT Test"));
	test(fs.Connect()==KErrNone);
	
	// Get the list of removable drive driver-letters
	TDriveList driveList;
	test(fs.DriveList(driveList,KDriveAttRemovable)==KErrNone);
	
	
	TInt length=driveList.Length();
	TBool pass = EFalse;
	
	// i is drive letter (as int)
	// for every removable media logical drive
	for(TInt i=0; i<length; i++)
		{
		if(driveList[i] == 0)
			{
			continue;
			}

		TChar driveChar = i+'A';
		test.Next(_L("Testing Logical Drive"));
		

		TInt FirstlocDrvNum = KErrNotFound;
		TInt SecondlocDrvNum = KErrNotFound;
		TInt driveNum = -1;
		driveNum = i+'A';
		test.Printf(_L("Logical Drive : %d"), driveNum);
		
		// Get local drive number by gettin ghr Serial number fo the card (RFs call), then 
		// enumerating the TBusLocalDrives and finding one that matches.
		test(FindMmcLocalDriveNumber(driveChar,FirstlocDrvNum,driveNum)==KErrNone);
		// Got first local drive number, now move card into second slot.
		test.Printf(_L("<Move MMC Card to second slot, then press any key>"));
		test.Getch();
		// Get second local drive number for same logical drive (should be different local drive number).
		test(FindMmcLocalDriveNumber(driveChar,SecondlocDrvNum,driveNum)==KErrNone);
		if(FirstlocDrvNum!=SecondlocDrvNum)
			{
			pass=ETrue;
			break;
			}
		// else perhaps this wasn't a multislot drive
		}
	test(pass);
	test.End();
	test.Close();
	
	fs.Close();
	return KErrNone;
	}

