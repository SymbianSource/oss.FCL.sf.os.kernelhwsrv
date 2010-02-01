// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\demandpaging\t_paginginfo.cpp
// 
//

#include <e32test.h>

RTest test(_L("t_paginginfo"));

#include <e32rom.h>
#include <e32svr.h>
#include <u32hal.h>
#include <f32file.h>
#include <f32dbg.h>
#include <d32locd.h>
#include "testdefs.h"
#include <hal.h>


TInt DriveNumber=-1;   // Parameter - Which drive?  -1 = autodetect.
TInt locDriveNumber;

TBusLocalDrive Drive;
TBool DisplayStats = ETrue;
TBool ManualTest = EFalse;

TInt findDataPagingDrive()
/** 
Find the drive containing a swap partition.

@return		Local drive identifier.
*/
	{
	TInt drive = KErrNotFound;
	
	test.Printf(_L("Searching for data paging drive:\n"));
	
	for(TInt i = 0; i < KMaxLocalDrives && drive < 0; ++i)
		{
		RLocalDrive	d;
		TBool		change = EFalse;
		
		if(d.Connect(i, change) == KErrNone)
			{
			test.Printf(_L("Connected to local drive %d\n"), i);
			TLocalDriveCapsV4			dc;
			TPckg<TLocalDriveCapsV4>	capsPack(dc);
			
			if(d.Caps(capsPack) == KErrNone)
				{
				if ((dc.iMediaAtt & KMediaAttPageable) &&
					(dc.iPartitionType == KPartitionTypePagedData))
					{
					test.Printf(_L("Found swap partition on local drive %d\n"), i);
					drive = i;

					TPageDeviceInfo pageDeviceInfo;

					TPtr8 pageDeviceInfoBuf((TUint8*) &pageDeviceInfo, sizeof(pageDeviceInfo));
					pageDeviceInfoBuf.FillZ();

					TInt r = d.QueryDevice(RLocalDrive::EQueryPageDeviceInfo, pageDeviceInfoBuf);

					test.Printf(_L("EQueryPageDeviceInfo on local drive %d returned %d\n"), i, r);
					}
				}
			d.Close();
			}
		}
	return drive;
	}



void DisplayPageDeviceInfo(TInt aDataPagingDrive)
	{
	test.Printf(_L("Stats: \n"));

	SMediaPagingInfo info;
	TInt r = UserSvr::HalFunction(EHalGroupMedia,EMediaHalGetPagingInfo,(TAny*) aDataPagingDrive, &info);
	test.Printf(_L("HAL: EMediaHalGetPagingInfo returned %d\n"), r);
	if (r == KErrNone)
		{
		test.Printf(_L("iRomPageInCount %d\n"), info.iRomPageInCount);
		test.Printf(_L("iCodePageInCount %d\n"), info.iCodePageInCount);
		test.Printf(_L("iDataPageInCount %d\n"), info.iDataPageInCount);
		test.Printf(_L("iDataPageOutCount %d\n"), info.iDataPageOutCount);
		test.Printf(_L("iDataPageOutBackgroundCount %d\n"), info.iDataPageOutBackgroundCount);
		}

	
	RLocalDrive	d;
	TBool change = EFalse;
	r = d.Connect(aDataPagingDrive, change);
	test (r == KErrNone);
		
	TPageDeviceInfo pageDeviceInfo;
	TPtr8 pageDeviceInfoBuf((TUint8*) &pageDeviceInfo, sizeof(pageDeviceInfo));
	pageDeviceInfoBuf.FillZ();
	r = d.QueryDevice(RLocalDrive::EQueryPageDeviceInfo, pageDeviceInfoBuf);
	test (r == KErrNone || r == KErrNotSupported);

	d.Close();
	test.Printf(_L("iReservoirBlockCount %d\n"),		pageDeviceInfo.iReservoirBlockCount);
	test.Printf(_L("iBadBlockCount %d\n"),	pageDeviceInfo.iBadBlockCount);

	}

//
// The gubbins that starts all the tests
//
// ParseCommandLine reads the arguments and sets globals accordingly.
//

void ParseCommandLine()
	{
	TBuf<32> args;
	User::CommandLine(args);
	TLex lex(args);
	
	FOREVER
		{
		
		TPtrC token=lex.NextToken();
		if(token.Length()!=0)
			{
			if ((token.Length()==2) && (token[1]==':'))
				DriveNumber=User::UpperCase(token[0])-'A';
			else if (token.Length()==1)
				{
				TChar driveLetter = User::UpperCase(token[0]); 
				if ((driveLetter>='A') && (driveLetter<='Z'))
					DriveNumber=driveLetter - (TChar) 'A';
				else 
					test.Printf(_L("Unknown argument '%S' was ignored.\n"), &token);
				}
			else if ((token==_L("help")) || (token==_L("-h")) || (token==_L("-?")))
				{
				test.Printf(_L("\nUsage:  t_paginginfo [enable] [disable] [stats]\n\n"));
				test.Getch();
				}
			else if (token==_L("stats"))
				{
				DisplayStats = ETrue;
				}
			else if (token==_L("-m"))
				{
				ManualTest = ETrue;
				}
			else
				test.Printf(_L("Unknown argument '%S' was ignored.\n"), &token);
			}
		else
			break;
		
		}
	}

//
// E32Main
//

TInt E32Main()
	{
	test.Title();

	test.Start(_L("Check that the rom is paged"));

	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();

	if (romHeader->iPageableRomStart==NULL)
		{
		test.Printf(_L("Test ROM is not paged - test skipped!\r\n"));
		test.End();
		return 0;
		}
	ParseCommandLine();	

	TInt dataPagingDrive = findDataPagingDrive();
	if (dataPagingDrive == KErrNotFound)
		{
		test.Printf(_L("Swap partition not found - test skipped!\r\n"));
		test.End();
		return 0;
		}



	if (DisplayStats)
		{
		DisplayPageDeviceInfo(dataPagingDrive);

		if (ManualTest)
			test.Getch();

		test.End();
		return 0;
		}



	test.End();
	return 0;
	}


