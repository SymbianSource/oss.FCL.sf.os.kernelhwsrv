// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pccd\t_lfsdrv.cpp
// Test the LFS media driver
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <hal.h>

const TInt KDriveNumber=8;	

#define PDD_NAME _L("MEDLFS")

LOCAL_D RTest test(_L("T_LFSDRV"));


GLDEF_C TInt E32Main()
    {
	TBuf<64> b;
	TInt r;

	test.Title();
	test.Start(_L("Check loader running"));

#if !defined (__WINS__)
    test.Next(_L("Read machine information"));
	TInt muid;
	r=HAL::Get(HAL::EMachineUid, muid);
	test(r==KErrNone);
	if (muid != HAL::EMachineUid_Brutus)
		{
		test.Printf(_L("Test not supported on this platform"));
		test.End();
		return(0);
		}
#endif

	test.Next(_L("Load ATA Media Driver"));
	r=User::LoadPhysicalDevice(PDD_NAME);
	test(r==KErrNone||r==KErrAlreadyExists);

#if defined (__WINS__)
	b.Format(_L("Connect to local drive %d (W)"),KDriveNumber);
#else
	b.Format(_L("Connect to local drive %d (K)"),KDriveNumber);
#endif
	test.Next(b);
	TBusLocalDrive theDrive;
	TBool changeFlag=EFalse;
	test(theDrive.Connect(KDriveNumber,changeFlag)==KErrNone);

	test.Next(_L("LFS drive: Capabilities"));
	TLocalDriveCapsV2Buf info;
	test(theDrive.Caps(info)==KErrNone);
	TInt diskSize=I64LOW(info().iSize);
	test.Printf( _L("Check drive size: %d\r\n"),diskSize);
//	test.Getch();
	test(info().iType==EMediaFlash);
	test(info().iConnectionBusType==EConnectionBusInternal);
	test(info().iDriveAtt==(TUint)(KDriveAttLocal|KDriveAttInternal));
	test(info().iMediaAtt==KMediaAttFormattable);
	test(info().iFileSystemId==KDriveFileSysLFFS);

	test.Next(_L("LFS drive: Read"));
	TBuf8<0x10> rdBuf;
 	test(theDrive.Read(0,4,rdBuf)==KErrNone);
	test.Printf( _L("%x %x %x %x\r\n"),rdBuf[0],rdBuf[1],rdBuf[2],rdBuf[3]);
//	test.Getch();

	test.Next(_L("Free device"));
	r=User::FreePhysicalDevice(_L("Media.Lfs"));
	test(r==KErrNone);

	test.End();
	return(0);
	}
  
