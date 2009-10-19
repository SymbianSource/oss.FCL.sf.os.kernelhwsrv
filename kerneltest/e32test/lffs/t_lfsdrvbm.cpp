// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\lffs\t_lfsdrvbm.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include "..\misc\prbs.h"


LOCAL_D TBuf<16384> DataBuf;
LOCAL_D	TBusLocalDrive TheDrive;
LOCAL_D TBool ChangedFlag;

const TInt KBufferSize=4096;
const TInt KBigBufferSize=4096*4;
TUint8 Buffer[KBigBufferSize];


#define LFFS_PDD_NAME _L("MEDLFS")


RTest test(_L("LFFS Driver BenchMark Test"));

LOCAL_C void DoRead(TInt aReadBlockSize)
//
// Do Read benchmark
//
	{
    TInt msgHandle = KLocalMessageHandle;
	TLocalDriveCapsV7 info;
	TPckg<TLocalDriveCapsV7> capsPckg(info);
  	TheDrive.Caps(capsPckg);
	TInt maxSize;
	maxSize=I64LOW(info.iSize);
	TInt count,pos,err;
	count=pos=err=0;

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;
	timer.After(reqStat,10000000); // After 10 secs
	while(reqStat==KRequestPending)
		{
		if (TheDrive.Read(pos,aReadBlockSize,&DataBuf,msgHandle,0)==KErrNone)
			count++;
		else
			err++;
		pos+=aReadBlockSize;
		if (pos>=(maxSize-aReadBlockSize))
			pos=0;
		}
#if defined (__WINS__)
	test.Printf(_L("Read %d %d byte blocks in 10 secs\n"),count,aReadBlockSize);
#else
	TBuf<60> buf;
	TReal32 rate=((TReal32)(count*aReadBlockSize))/10240.0F;
	TRealFormat rf(10,2);
	buf.Format(_L("Read %d %d byte blocks in 10 secs ("),count,aReadBlockSize);
	buf.AppendNum(rate,rf);
	buf.Append(_L("Kb/s)\n"));
	test.Printf(buf);
#endif
	test.Printf(_L("Errors:%d\n"),err);
	}

LOCAL_C void DoWrite(TInt aWriteBlockSize)
//
// Do write benchmark
//
	{
	TLocalDriveCapsV7 info;
	TPckg<TLocalDriveCapsV7> capsPckg(info);
  	TheDrive.Caps(capsPckg);
	TInt maxSize;
	maxSize=I64LOW(info.iSize);
	TInt count,err;
	TUint pos;
	count=pos=err=0;

	// Erase the first 16 blocks to ensure write completes OK
	TUint32 EbSz=(TInt)info.iEraseBlockSize;
	TInt r=KErrNone;
	for (pos=0; pos<16*EbSz; pos+=EbSz)
		{
		TInt64 pos64 = MAKE_TINT64(0, pos);
		r=TheDrive.Format(pos64,EbSz);
		test(r==KErrNone);
		}

	pos=0;
	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;
	TPtrC8 ptr(Buffer,aWriteBlockSize);
	timer.After(reqStat,10000000); // After 10 secs
	while(reqStat==KRequestPending)
		{
		TInt64 pos64 = MAKE_TINT64(0, pos);
		TInt r=TheDrive.Write(pos64,ptr);
		if (r==KErrNone)
			count++;
		else
			err++;
		pos+=aWriteBlockSize;
		if ((TInt)pos>=(maxSize-aWriteBlockSize))
			pos=0;
		}

#if defined (__WINS__)
	test.Printf(_L("Write %d %d byte blocks in 10 secs\n"),count,aWriteBlockSize);
#else
	TBuf<60> buf;
	TReal32 rate=((TReal32)(count*aWriteBlockSize))/10240.0F;
	TRealFormat rf(10,2);
	buf.Format(_L("Write %d %d byte blocks in 10 secs ("),count,aWriteBlockSize);
	buf.AppendNum(rate,rf);
	buf.Append(_L("Kb/s)\n"));
	test.Printf(buf);
#endif
	test.Printf(_L("Errors:%d\n"),err);
	}


GLDEF_C TInt E32Main()
    {
	TBuf<32> b;

	test.Title();
	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();
	TInt r;
	TInt drv;

	test.Printf(_L("DRIVES PRESENT  :%d\r\n"),di.iTotalSupportedDrives);
	test.Printf(_L("C:(1ST) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[0]);
	test.Printf(_L("D:(2ND) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[1]);
	test.Printf(_L("E:(3RD) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[2]);
	test.Printf(_L("F:(4TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[3]);
	test.Printf(_L("G:(5TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[4]);
	test.Printf(_L("H:(6TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[5]);
	test.Printf(_L("I:(7TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[6]);
	test.Printf(_L("J:(8TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[7]);
	test.Printf(_L("K:(9TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[8]);

	test.Printf(_L("Select Local Drive (C-%c):\r\n"),'C'+ 8);
	TChar c;
	FOREVER
		{
		c=(TUint)test.Getch();
		c.UpperCase();
		drv=((TUint)c)-'C';
		if (drv>=0&&drv<='C'+ 8)
			break;
		}

	r=User::LoadPhysicalDevice(LFFS_PDD_NAME);
	test(r==KErrNone || r==KErrAlreadyExists);


	b.Format(_L("Connect to drive %c:"),'C'+drv);
	test.Next(b);
	TheDrive.Connect(drv,ChangedFlag);

	TLocalDriveCapsV7 info;
	TPckg<TLocalDriveCapsV7> capsPckg(info);
  	TheDrive.Caps(capsPckg);

	test.Start(_L("Starting write tests\n"));

	// Full buffer write test - pre-load the buffer and write 
	TUint32* pB=(TUint32*)(Buffer);
	TUint32* pE=(TUint32*)(Buffer+KBufferSize);
	TUint seed[2];
	seed[0]=0xb17217f8;
	seed[1]=0;
	while (pB<pE)
		*pB++=Random(seed);

	pB=(TUint32*)(Buffer);
	if(info.iWriteBufferSize)
		DoWrite(info.iWriteBufferSize);

	// Erase test
	// Get the current time
	TInt64 zeroTime=MAKE_TINT64(0, 0);
	TTime TheTimer=TTime(zeroTime);

	// Invoke the erase sequuence

	// Report the time interval
	TInt64 currentTime=TheTimer.Int64();
test.Printf(_L("currentTime now = %d\n"),(TInt)currentTime);

currentTime=TheTimer.Int64();
test.Printf(_L("currentTime now = %d\n"),(TInt)currentTime);

currentTime=TheTimer.Int64();
test.Printf(_L("currentTime now = %d\n"),(TInt)currentTime);


	if ((info.iWriteBufferSize)<1024)
	/* 
	these tests would cause errors on M18 Intel Strataflash. this type
	of Strataflash operates in object and control mode. when writing 16
	byte blocks the first write puts puts each M18 1024 byte programming 
	region into control mode so that all susequent even numbered writes 
	succeed but all subsequent odd numbered writes fail (in control 
	mode writes to bytes 16 - 31, 48 - 63 aren't allowed). successes match
	failures. when writing 256 byte blocks only 1 in 4 writes succeeds -
	the first write in each programming region succeeds and puts the 
	region into object mode so that the subsequent three writes to the 
	same programming region fail (in object mode a programming region may
	only be written to once). when writing 512 byte blocks 1 in 2 writes 
	fail with the first write to each programming region succeeding and the 
	second failing. with 513 byte writes the analysis is slightly more 
	complex than with 512 byte writes but the failure rate of 1 in 2 still
	applies. 
	*/		
		{
		DoWrite(16);
		DoWrite(256);
		DoWrite(512);
		DoWrite(513); 
		}
	DoWrite(1024); 
	DoWrite(2048);
	DoWrite(16384);


	DoRead(16);
	DoRead(256);
	DoRead(512);
	DoRead(513);
	DoRead(2048);
	DoRead(16384);

   test.End();

	return(0);
	}
  
