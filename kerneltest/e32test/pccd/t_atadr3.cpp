// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pccd\t_atadr3.cpp
// Test the Compact Flash card (ATA) media driver
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include "u32std.h"
#include "../misc/prbs.h"

//#define __USE_MUTEX__
//#define __DISABLE_KILLER__
#define SYSTEM	ETrue

const TInt KErrVerify=-100;

const TInt KSectorSize=512;
const TInt KSectorShift=9;
const TInt KVerifyBlockSize=8;	// in sectors

LOCAL_D TBusLocalDrive WriterDrive;
LOCAL_D RTest test(_L("T_ATADR3"));
LOCAL_D TInt DriveNumber;
LOCAL_D TInt DriveSizeInSectors;
LOCAL_D RThread TheWriterThread;
LOCAL_D RThread TheKillerThread;
LOCAL_D RSemaphore Sem;
LOCAL_D TInt CurrentSector=0;
LOCAL_D TInt MediaChanges=0;
LOCAL_D TInt PowerDowns=0;
LOCAL_D TInt Kills=0;
LOCAL_D TInt SectorsWritten=0;
LOCAL_D TInt Aborts=0;
LOCAL_D TUint WritePattern=0;

#ifdef __USE_MUTEX__
LOCAL_D RMutex Mutex;
#define WAIT		Mutex.Wait()
#define SIGNAL		Mutex.Signal()
#else
#define WAIT
#define SIGNAL
#endif

inline TUint RoundDownToSector(TUint aPos)
	{ return aPos&~0x1ff; }
inline TUint RoundUpToSector(TUint aPos)
	{ return (aPos+0x1ff)&~0x1ff; }

LOCAL_C TInt WriteSectors(TBusLocalDrive& aDrive, TInt aSector, TInt aCount, TUint8* aBuf)
	{
	WAIT;
	TInt r=KErrNotReady;
	TInt pos=aSector<<KSectorShift;
	TPtrC8 p(aBuf,aCount*KSectorSize);
	while (r==KErrNotReady || (r==KErrAbort && (++Aborts,1)))
		r=aDrive.Write(pos,p);
	SIGNAL;
	return r;
	}

LOCAL_C TInt ReadSectors(TBusLocalDrive& aDrive, TInt aSector, TInt aCount, TUint8* aBuf)
	{
	WAIT;
	TInt r=KErrNotReady;
	TInt pos=aSector<<KSectorShift;
	TInt len=aCount*KSectorSize;
	TPtr8 p(aBuf,0,len);
	while (r==KErrNotReady || r==KErrAbort)
		r=aDrive.Read(pos,len,p);
	if (r==KErrNone && p.Length()!=len)
		r=KErrUnderflow;
	SIGNAL;
	return r;
	}

LOCAL_C void GenerateTestPattern(TInt aSector, TUint aState, TUint8* aBuf)
	{
	TUint seed[2];
	seed[1]=0;
	seed[0]=TUint(aSector)^(aState<<16);
	*aBuf++=TUint8(aState&0xff);
	*aBuf++=TUint8((aState>>8)&0xff);
	TInt i;
	for (i=0; i<KSectorSize-2; i++)
		*aBuf++=TUint8(Random(seed));
	}

LOCAL_C void Write(TBusLocalDrive& aDrive, TInt aSector, TInt aCount, TUint8* aBuf)
	{
	TInt n;
	for (n=0; n<aCount; n++)
		GenerateTestPattern(aSector+n,WritePattern,aBuf+n*KSectorSize);
	TInt r=WriteSectors(aDrive,aSector,aCount,aBuf);
	if (r!=KErrNone)
		User::Panic(_L("WRITE"),r);
	}

LOCAL_C void Write(TBusLocalDrive& aDrive, TInt aSector, TInt aCount)
	{
	const TInt KMaxLen = KSectorSize * 8;
	TInt len=aCount*KSectorSize;
	test (len <= KMaxLen);
	TUint8 buf[KMaxLen];
	Write(aDrive,aSector,aCount,buf);
	}

LOCAL_C TInt Verify(TInt aSector, TInt aCount, TUint8* aBuf)
	{
	TUint32 buf[KSectorSize/4];
	TUint8* pB=(TUint8*)buf;
	while(aCount--)
		{
		TUint state=aBuf[0]|(aBuf[1]<<8);
		GenerateTestPattern(aSector,state,pB);
		if (Mem::Compare(aBuf,KSectorSize,pB,KSectorSize)!=0)
			return KErrVerify;
		++aSector;
		aBuf+=KSectorSize;
		}
	return KErrNone;
	}

LOCAL_C TInt Verify(TBusLocalDrive& aDrive, TInt aSector, TInt aCount)
	{
	const TInt KMaxLen = KSectorSize * 8;
	TInt len=aCount*KSectorSize;
	test (len <= KMaxLen);
	TUint8 buf[KMaxLen];
	TInt r=ReadSectors(aDrive,aSector,aCount,buf);
	if (r!=KErrNone)
		return r;
	return Verify(aSector,aCount,buf);
	}

LOCAL_C TInt WriterThread(TAny*)
	{
/* 
 * SetSystem() API was removed by __SECURE_API__	
 *	"Systemize" will be implemented later calling a LDD which will set thread's "system" flag
	RThread().SetSystem(SYSTEM);
 */
	TUint seed[2];
	seed[0]=User::NTickCount();
	seed[1]=0;
	TUint32 buf[8*KSectorSize/4];
	TUint8* pB=(TUint8*)buf;
	TBool medChg=EFalse;
	WriterDrive.Close();	// close handle from previous incarnation of this thread
	TInt r=WriterDrive.Connect(DriveNumber,medChg);
	if (r!=KErrNone)
		User::Panic(_L("WRITER-CONNECT"),r);
	FOREVER
		{
		TInt remain=DriveSizeInSectors-CurrentSector;
		TInt n=Random(seed)&15;
		if (n>8 || n==0)
			n=1;
		if (n>remain)
			n=remain;
		if (n==1)
			{
			Write(WriterDrive,CurrentSector,1,pB);
			++WritePattern;
			Write(WriterDrive,CurrentSector,1,pB+KSectorSize);
			SectorsWritten+=2;
			}
		else
			{
			Write(WriterDrive,CurrentSector,n,pB);
			SectorsWritten+=n;
			}
		CurrentSector+=n;
		if (CurrentSector==DriveSizeInSectors)
			{
			CurrentSector=0;
			++WritePattern;
			}
		}
	}

LOCAL_C TInt KillerThread(TAny*)
	{
/* 
 * SetSystem() API was removed by __SECURE_API__	
 *	"Systemize" will be implemented later calling a LDD which will set thread's "system" flag
 	RThread().SetSystem(SYSTEM);
 */
	TUint seed[2];
	seed[0]=0xadf85458;
	seed[1]=0;
	TBusLocalDrive drive;
	TBool medChg=EFalse;
	TInt r=drive.Connect(DriveNumber,medChg);
	if (r!=KErrNone)
		User::Panic(_L("KILLER-CONNECT"),r);
	RTimer timer;
	r=timer.CreateLocal();
	if (r!=KErrNone)
		User::Panic(_L("KILLER-TIMER"),r);
	TInt action=0;
	FOREVER
		{
		TUint x=Random(seed);
		TUint ms=1000+(x&4095);
		User::AfterHighRes(ms*1000);
		switch (action)
			{
			case 0:
#ifndef __DISABLE_KILLER__
				drive.ForceMediaChange();
#endif
				++MediaChanges;
				break;
			case 1:
				{
#ifndef __DISABLE_KILLER__
				TTime now;
				now.HomeTime();
				now+=TTimeIntervalSeconds(2);
				TRequestStatus s;
				timer.At(s,now);
				UserHal::SwitchOff();
				User::WaitForRequest(s);
				++PowerDowns;
#endif
				break;
				}
			case 2:
#ifndef __DISABLE_KILLER__
				TheWriterThread.Kill(0);
#endif
				++Kills;
#ifndef __DISABLE_KILLER__
				++WritePattern;
				Sem.Wait();
				TheWriterThread.SetPriority(EPriorityNormal);
#endif
				break;
			case 3:
#ifndef __DISABLE_KILLER__
				drive.ForceMediaChange();
#endif
				++MediaChanges;
#ifndef __DISABLE_KILLER__
				x=Random(seed);
				ms=50+(x&511);
				User::AfterHighRes(ms*1000);
				TheWriterThread.Kill(0);
#endif
				++Kills;
#ifndef __DISABLE_KILLER__
				++WritePattern;
				Sem.Wait();
				TheWriterThread.SetPriority(EPriorityNormal);
#endif
				break;
			case 4:
				break;
			}
		if (++action==5)
			action=0;
#ifndef __DISABLE_KILLER__
		TInt curr=CurrentSector;
		TInt remain=DriveSizeInSectors-curr;
		TInt n=(remain<8)?remain:8;
		r=Verify(drive,curr,n);
		if (r!=KErrNone)
			User::Panic(_L("VERIFY"),r);
#endif
		}
	}

LOCAL_C TInt CreateKillerThread()
	{
	TInt r=TheKillerThread.Create(_L("Killer"),KillerThread,0x2000,NULL,NULL);
	if (r!=KErrNone)
		return r;
	TheKillerThread.SetPriority(EPriorityMore);
	TheKillerThread.Resume();
	return KErrNone;
	}

LOCAL_C TInt CreateWriterThread()
	{
	FOREVER
		{
		TInt r=TheWriterThread.Create(_L("Writer"),WriterThread,0x2000,NULL,NULL);
		if (r==KErrNone)
			break;
		if (r!=KErrAlreadyExists)
			return r;
		test.Printf(_L("Writer thread still exists\n"));
		User::After(200000);
		}
	TheWriterThread.SetPriority(EPriorityLess);
	TheWriterThread.Resume();
	return KErrNone;
	}

GLDEF_C TInt E32Main()
	{
/* 
 * SetSystem() API was removed by __SECURE_API__	
 *	"Systemize" will be implemented later calling a LDD which will set thread's "system" flag
	RThread().SetSystem(SYSTEM);
 */
	WriterDrive.SetHandle(0);
	test.Title();
	TInt drv=-1;
	while (drv<0 || drv>=KMaxLocalDrives)
		{
		test.Printf(_L("\nSelect drive C-K: "));
		TChar c=(TUint)test.Getch();
		c.UpperCase();
		if (c.IsAlpha())
			drv=TUint(c)-'C';
		else
			drv=-1;
		}
	TBuf<1> b;
	b.SetLength(1);
	b[0]=(TText)(drv+'C');
	test.Printf(_L("%S\n"),&b);

	TBuf<80> buf=_L("Connect to drive ");
	buf+=b;
	test.Start(buf);
	TBusLocalDrive drive;
	TBool medChg=EFalse;
	TInt r=drive.Connect(drv,medChg);
	test(r==KErrNone);
	DriveNumber=drv;

	test.Next(_L("Get capabilities"));
	TLocalDriveCapsV2 driveCaps;
	TPckg<TLocalDriveCapsV2> capsPckg(driveCaps);
	r=drive.Caps(capsPckg);
	test(r==KErrNone);
	TUint driveSize=I64LOW(driveCaps.iSize);
	DriveSizeInSectors=(driveSize&~0xfff)>>KSectorShift;	// round down to multiple of 8 sectors
	test.Printf(_L("Drive size       = %08x (%dK)\n"),driveSize,driveSize>>10);
	test.Printf(_L("Media type       = %d\n"),driveCaps.iType);
	test.Printf(_L("Connection Bus   = %d\n"),driveCaps.iConnectionBusType);
	test.Printf(_L("Drive attributes = %08x\n"),driveCaps.iDriveAtt);
	test.Printf(_L("Media attributes = %08x\n"),driveCaps.iMediaAtt);
	test.Printf(_L("Base address     = %08x\n"),driveCaps.iBaseAddress);
	test.Printf(_L("File system ID   = %08x\n"),driveCaps.iFileSystemId);
	test.Printf(_L("Hidden sectors   = %08x\n"),driveCaps.iHiddenSectors);
	test.Printf(_L("Press any key...\n"));
	test.Getch();

#ifdef __USE_MUTEX__
	test.Next(_L("Create mutex"));
	r=Mutex.CreateLocal();
	test(r==KErrNone);
#endif

	test.Next(_L("Initialise drive"));
	TInt sector;
	for (sector=0; sector<DriveSizeInSectors; sector+=8)
		{
		Write(drive,sector,8);
		if ((sector&127)==0)
			test.Printf(_L("."));
		}
	test.Printf(_L("\n"));
	test.Next(_L("Verify drive"));
	for (sector=0; sector<DriveSizeInSectors; sector+=8)
		{
		test(Verify(drive,sector,8)==KErrNone);
		if ((sector&127)==0)
			test.Printf(_L("."));
		}

	test.Printf(_L("\n\nPress ENTER to continue..."));
	TKeyCode k=EKeyNull;
	while (k!=EKeyEnter)
		k=test.Getch();
	test.Printf(_L("\n"));

	test.Next(_L("Create semaphore"));
	r=Sem.CreateLocal(0);
	test(r==KErrNone);
	test.Next(_L("Create writer thread"));
	r=CreateWriterThread();
	test(r==KErrNone);
	TheWriterThread.SetPriority(EPriorityNormal);
	test.Next(_L("Create killer thread"));
	r=CreateKillerThread();
	test(r==KErrNone);

	TBool exit=EFalse;
	sector=0;
	TInt verifies=0;
	TInt fails=0;
	while (!exit)
		{
		r=Verify(drive,sector,KVerifyBlockSize);
		if (r!=KErrNone)
			{
			++fails;
			test.Printf(_L("Sector %d Fail %d\n"),sector,r);
			}
		else
			++verifies;
		sector+=KVerifyBlockSize;
		if (sector==DriveSizeInSectors)
			{
			sector=0;
			test.Printf(_L("W %d VER %d FAIL %d MC %d PD %d K %d A %d\n"),SectorsWritten,verifies,fails,MediaChanges,PowerDowns,Kills,Aborts);
			}
		if ((sector&63)==0)
			{
			if (TheKillerThread.ExitType()!=EExitPending)
				{
				const TDesC& cat=TheKillerThread.ExitCategory();
				test.Printf(_L("KillerThread exited %d,%d,%S\n"),TheKillerThread.ExitType(),
														TheKillerThread.ExitReason(),&cat);
				break;
				}
			TExitType xt=TheWriterThread.ExitType();
			if (xt==EExitPanic)
				{
				const TDesC& cat=TheWriterThread.ExitCategory();
				test.Printf(_L("WriterThread Panic %S %d\n"),&cat,TheWriterThread.ExitReason());
				break;
				}
			if (xt!=EExitPending)
				{
				// restart writer thread
				TheWriterThread.Close();
				r=CreateWriterThread();
				if (r!=KErrNone)
					{
					test.Printf(_L("Restart writer thread failed %d\n"),r);
					break;
					}
				Sem.Signal();
				}
			}
		}

	buf=_L("Disconnect from drive ");
	buf+=b;
	test.Next(buf);
	drive.Disconnect();
	test.End();
	return 0;
	}
