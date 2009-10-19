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
// e32test\pccd\t_meddrv.cpp
// General media driver tests
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
const TInt KHeapSize=0x4000;
const TInt KTestDriverMediaSize=0x1000;	// 4K
const TInt KShortBufferSize=0x100;

#define MEDDRV_ATA_NAME _L("MEDATA")
#define MEDDRV_SINGLE_REQ_NAME _L("MEDT1")
#define MEDDRV_SIMULTANEOUS_REQ_NAME _L("MEDT2")

LOCAL_D RTest test(_L("T_MEDDRV"));
LOCAL_D TInt TheDriveNumber=3;	


LOCAL_C TInt formatThread(TAny*)
	{

	TBusLocalDrive anotherDrive;
	RTest ftest(_L("Formatting thread"));
	ftest.Title();

	ftest.Start(_L("Connect to local drive"));
	TBool changeFlag=EFalse;
	ftest(anotherDrive.Connect(TheDriveNumber,changeFlag)==KErrNone);

	ftest.Next(_L("Format disk"));
	ftest(anotherDrive.Format(0,KTestDriverMediaSize)==KErrNone);
	anotherDrive.Disconnect();

    ftest.End();
	return(KErrNone);
	}

GLDEF_C TInt E32Main()
    {
	TBuf<64> b;
	TInt r;
	TInt i;

	test.Title();
	test.Start(_L("Check loader running"));

#if !defined (__WINS__)
    test.Next(_L("Read drive information"));
	TDriveInfoV1Buf dinfo;
	r=UserHal::DriveInfo(dinfo);
	test(r==KErrNone);
	if (dinfo().iTotalSockets==2)
        TheDriveNumber=3;
    else if (dinfo().iTotalSockets==1)
        TheDriveNumber=2;
    else
		{
		test.Printf(_L("Test not supported on this platform"));
		test.End();
		return(0);
		}
#else
    TheDriveNumber=3;
#endif

	test.Next(_L("Load 1st Media Driver"));
	r=User::LoadPhysicalDevice(MEDDRV_ATA_NAME);
	test(r==KErrNone||r==KErrAlreadyExists);
	r=User::LoadPhysicalDevice(MEDDRV_SINGLE_REQ_NAME);
	test(r==KErrNone||r==KErrAlreadyExists);
	test.Printf(_L("Media drivers installed:\n\r"));
	TFindPhysicalDevice findMediaDrv(_L("Media.*"));
	TFullName findResult;
	r=findMediaDrv.Next(findResult);
	while (r==KErrNone)
		{
		test.Printf(_L("   %S\n\r"),&findResult);
		r=findMediaDrv.Next(findResult);
		}

	b.Format(_L("Connect to local drive %d"),TheDriveNumber);
	test.Next(b);
	TBusLocalDrive theDrive;
	TBool changeFlag=EFalse;
	test(theDrive.Connect(TheDriveNumber,changeFlag)==KErrNone);

	test.Next(_L("Local drive: Capabilities"));
	// Generate a media change to open the profiler media driver rather than the standard one
//	UserSvr::ForceRemountMedia(ERemovableMedia0);
	User::After(300000); // Wait 0.3Sec
	TLocalDriveCapsV2Buf info;
	test(theDrive.Caps(info)==KErrNone);
	test(I64LOW(info().iSize)==(TUint)KTestDriverMediaSize);
	test(info().iType==EMediaRam);

	test.Next(_L("Local drive: Write/Read"));
	TInt len;
	TBuf8<KShortBufferSize> rdBuf,wrBuf;
	wrBuf.SetLength(KShortBufferSize);
	for (i=0;i<KShortBufferSize;i++)
		wrBuf[i]=(TUint8)i;
	for (i=0;i<KTestDriverMediaSize;i+=len)
		{
		len=Min(KShortBufferSize,(KTestDriverMediaSize-i));
		wrBuf.SetLength(len);
		test(theDrive.Write(i,wrBuf)==KErrNone);
		}
	for (i=0;i<KTestDriverMediaSize;i+=len)
		{
		len=Min(KShortBufferSize,(KTestDriverMediaSize-i));
		rdBuf.Fill(0,len);
 		test(theDrive.Read(i,len,rdBuf)==KErrNone);
		wrBuf.SetLength(len);
  	    test(rdBuf.Compare(wrBuf)==0);
		}

	test.Next(_L("Start 2nd thread to format drive"));
	RThread thread;
	TRequestStatus stat;
	test(thread.Create(_L("Thread"),formatThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	thread.Logon(stat);
	thread.Resume();
  
	test.Next(_L("Drive read/write during format"));
	User::After(2000000); // Wait 2Secs for format to get going
    rdBuf.SetLength(KShortBufferSize);
 	test(theDrive.Read(0,KShortBufferSize,rdBuf)==KErrInUse);
	wrBuf.SetLength(KShortBufferSize);
	test(theDrive.Write(0,wrBuf)==KErrInUse);
    
    // Verify format unaffected
    User::WaitForRequest(stat);
	test(stat==KErrNone);
	CLOSE_AND_WAIT(thread);
	wrBuf.Fill(0xFF,KShortBufferSize);
	for (i=0;i<KTestDriverMediaSize;i+=len)
		{
		len=Min(KShortBufferSize,(KTestDriverMediaSize-i));
		rdBuf.Fill(0,len);
 		test(theDrive.Read(i,len,rdBuf)==KErrNone);
		wrBuf.SetLength(len);
  	    test(rdBuf.Compare(wrBuf)==0);
		}

	test.Next(_L("Load 2nd Media Driver"));
	r=User::FreePhysicalDevice(_L("Media.Tst1"));
	test(r==KErrNone);
	r=User::LoadPhysicalDevice(MEDDRV_SIMULTANEOUS_REQ_NAME);
	test(r==KErrNone||r==KErrAlreadyExists);
	test.Printf(_L("Media drivers installed:\n\r"));
	findMediaDrv.Find(_L("Media.*"));
	r=findMediaDrv.Next(findResult);
	while (r==KErrNone)
		{
		test.Printf(_L("   %S\n\r"),&findResult);
		r=findMediaDrv.Next(findResult);
		}

	test.Next(_L("Local drive: Capabilities"));
	// Generate a media change to open the profiler media driver rather than the standard one
//	UserSvr::ForceRemountMedia(ERemovableMedia0);
	User::After(300000); // Wait 0.3Sec
	test(theDrive.Caps(info)==KErrNone);
	test(I64LOW(info().iSize)==(TUint)KTestDriverMediaSize);
	test(info().iType==EMediaFlash);

	test.Next(_L("Local drive: Write/Read"));
	wrBuf.SetLength(KShortBufferSize);
	for (i=0;i<KShortBufferSize;i++)
		wrBuf[i]=(TUint8)i;
	for (i=0;i<KTestDriverMediaSize;i+=len)
		{
		len=Min(KShortBufferSize,(KTestDriverMediaSize-i));
		wrBuf.SetLength(len);
		test(theDrive.Write(i,wrBuf)==KErrNone);
		}
	for (i=0;i<KTestDriverMediaSize;i+=len)
		{
		len=Min(KShortBufferSize,(KTestDriverMediaSize-i));
		rdBuf.Fill(0,len);
 		test(theDrive.Read(i,len,rdBuf)==KErrNone);
		wrBuf.SetLength(len);
  	    test(rdBuf.Compare(wrBuf)==0);
		}

	test.Next(_L("Start 2nd thread again to format drive"));
	test(thread.Create(_L("Thread"),formatThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	thread.Logon(stat);
	thread.Resume();
  
	test.Next(_L("Drive read/write during format"));
	User::After(2000000); // Wait 2Secs for format to get going
    rdBuf.SetLength(KShortBufferSize);
 	test(theDrive.Read(0,KShortBufferSize,rdBuf)==KErrNone);
	wrBuf.Fill(0xFF,KShortBufferSize);
  	test(rdBuf.Compare(wrBuf)==0);
	
    for (i=0;i<KShortBufferSize;i++)
		wrBuf[i]=(TUint8)i;
	test(theDrive.Write(0,wrBuf)==KErrNone);
 	test(theDrive.Read(0,KShortBufferSize,rdBuf)==KErrNone);
  	test(rdBuf.Compare(wrBuf)==0);
    
    // Verify remaining part of format
    User::WaitForRequest(stat);
	test(stat==KErrNone);
	CLOSE_AND_WAIT(thread);
	wrBuf.Fill(0xFF,KShortBufferSize);
	for (i=KShortBufferSize;i<KTestDriverMediaSize;i+=len)
		{
		len=Min(KShortBufferSize,(KTestDriverMediaSize-i));
		rdBuf.Fill(0,len);
 		test(theDrive.Read(i,len,rdBuf)==KErrNone);
		wrBuf.SetLength(len);
  	    test(rdBuf.Compare(wrBuf)==0);
		}

	test.Next(_L("Unload 2nd Media Driver"));
	User::FreePhysicalDevice(_L("Media.Tst2"));
	// Generate a media change to restore the standard one next mount
//	UserSvr::ForceRemountMedia(ERemovableMedia0);
	User::After(300000); // Wait 0.3Sec

	theDrive.Disconnect();
	test.End();
	return(0);
	}
  
