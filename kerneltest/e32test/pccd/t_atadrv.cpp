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
// e32test\pccd\t_atadrv.cpp
// Test the Compact Flash card (ATA) media driver
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <hal.h>
#include <e32def.h>
#include <e32def_private.h>

const TInt KAtaSectorSize=512;
const TInt KAtaSectorShift=9;
const TUint KAtaSectorMask=0xFFFFFE00;
const TInt KSectBufSizeInSectors=8;
const TInt KSectBufSizeInBytes=(KSectBufSizeInSectors<<KAtaSectorShift);
const TInt KRdWrBufLen=(KSectBufSizeInBytes+KAtaSectorSize); // 4.5K - exceeds driver local buffer size

const TInt KShortFormatInSectors=1;
const TInt KShortFormatInBytes=(KShortFormatInSectors<<KAtaSectorShift);
const TInt KLongFormatInSectors=KSectBufSizeInSectors+1;	// 4.5K - exceeds driver local buffer size
const TInt KLongFormatInBytes=(KLongFormatInSectors<<KAtaSectorShift);

const TInt KHeapSize=0x4000;

#undef USE_IDLE_CURRENT
#ifdef USE_IDLE_CURRENT
const TInt KAtaIdleCurrentInMilliAmps=1; 
#endif


#define PDD_NAME _L("MEDATA")

LOCAL_D RTest test(_L("T_ATADRV"));
LOCAL_D RTest nTest(_L("This thread doesn't disconnect"));
LOCAL_D TBool ChangeFlag;
LOCAL_D TBool SecThreadChangeFlag;
LOCAL_D TBuf8<KRdWrBufLen> wrBuf,rdBuf;
LOCAL_D TInt DriveNumber;

const TInt KSingSectorNo=1;
void singleSectorRdWrTest(TBusLocalDrive &aDrv,TInt aSectorOffset,TInt aLen)
//
// Perform a write / read test on a single sector (KSingSectorNo). Verify that the
// write / read back is successful and that the rest of the sector is unchanged.
//
	{

	TBuf8<KAtaSectorSize> saveBuf;
	test.Start(_L("Single sector write/read test"));
	test(aSectorOffset+aLen<=KAtaSectorSize);

	// Now save state of sector before we write to it
	TInt secStart=(KSingSectorNo<<KAtaSectorShift);
 	test(aDrv.Read(secStart,KAtaSectorSize,saveBuf)==KErrNone);

	// Write zero's to another sector altogether (to ensure drivers 
	// local buffer hasn't already got test pattern we expect).
	wrBuf.Fill(0,KAtaSectorSize);
	test(aDrv.Write((KSingSectorNo+4)<<KAtaSectorShift,wrBuf)==KErrNone);

	// Write / read back sector in question
	wrBuf.SetLength(aLen);
	for (TInt i=0;i<aLen;i++)
		wrBuf[i]=(TUint8)(0xFF-i);
	test(aDrv.Write((secStart+aSectorOffset),wrBuf)==KErrNone);
	rdBuf.Fill(0,aLen);
 	test(aDrv.Read((secStart+aSectorOffset),aLen,rdBuf)==KErrNone);
  	test(rdBuf.Compare(wrBuf)==0);

	// Now check the rest of the sector is unchanged
	rdBuf.Fill(0,KAtaSectorSize);
 	test(aDrv.Read(secStart,KAtaSectorSize,rdBuf)==KErrNone);
	saveBuf.Replace(aSectorOffset,aLen,wrBuf);
  	test(rdBuf.Compare(saveBuf)==0);
	test.End();
	}

const TInt KMultSectorNo=2; 
void MultipleSectorRdWrTest(TBusLocalDrive &aDrv,TInt aFirstSectorOffset,TInt aLen)
//
// Perform a write / read test over multiple sectors (starting within sector KMultSectorNo).
// Verify that the write / read back is successful and that the remainder of the first and
// last sectors are not affected.
//
	{

	TBuf8<KAtaSectorSize> saveBuf1;
	TBuf8<KAtaSectorSize> saveBuf2;
	test.Start(_L("Multiple sector write/read test"));
	test(aFirstSectorOffset<KAtaSectorSize&&aLen<=KRdWrBufLen);

	// If not starting on sector boundary then save 1st sector to check rest of 1st sector is unchanged
	TInt startSecPos=(KMultSectorNo<<KAtaSectorShift);
	if (aFirstSectorOffset!=0)
 		test(aDrv.Read(startSecPos,KAtaSectorSize,saveBuf1)==KErrNone);

	// If not ending on sector boundary then save last sector to check rest of last sector is unchanged
	TInt endOffset=(aFirstSectorOffset+aLen)&(~KAtaSectorMask);
	TInt endSecPos=((startSecPos+aFirstSectorOffset+aLen)&KAtaSectorMask);
	if (endOffset)
 		test(aDrv.Read(endSecPos,KAtaSectorSize,saveBuf2)==KErrNone);
	
	// Write zero's to another sector altogether (to ensure drivers 
	// local buffer hasn't already got test pattern we expect).
	wrBuf.Fill(0,KSectBufSizeInBytes);
	test(aDrv.Write((KMultSectorNo+20)<<KAtaSectorShift,wrBuf)==KErrNone);
	
	wrBuf.SetLength(aLen);
	for (TInt i=0;i<aLen;i++)
		wrBuf[i]=(TUint8)(0xFF-i);
	test(aDrv.Write((startSecPos+aFirstSectorOffset),wrBuf)==KErrNone);
	rdBuf.Fill(0,aLen);
 	test(aDrv.Read((startSecPos+aFirstSectorOffset),aLen,rdBuf)==KErrNone);
  	test(rdBuf.Compare(wrBuf)==0);

	// Check rest of first sector involved is unchanged (if offset specified)
	if (aFirstSectorOffset!=0)
		{
		rdBuf.Fill(0,KAtaSectorSize);
 		test(aDrv.Read(startSecPos,KAtaSectorSize,rdBuf)==KErrNone);
		wrBuf.SetLength(KAtaSectorSize-aFirstSectorOffset);
		saveBuf1.Replace(aFirstSectorOffset,(KAtaSectorSize-aFirstSectorOffset),wrBuf);
  		test(rdBuf.Compare(saveBuf1)==0);
		}

	// Check rest of last sector involved is unchanged (if not ending on sector boundary)
	if (endOffset)
		{
		rdBuf.Fill(0,KAtaSectorSize);
 		test(aDrv.Read(endSecPos,KAtaSectorSize,rdBuf)==KErrNone);
		wrBuf.SetLength(aLen);
		wrBuf.Delete(0,aLen-endOffset);
		saveBuf2.Replace(0,endOffset,wrBuf);
  		test(rdBuf.Compare(saveBuf2)==0);
		}
	test.End();
	}

LOCAL_C TInt dontDisconnectThread(TAny*)
	{

	TBusLocalDrive anotherAtaDrive;
	nTest.Title();

	nTest.Start(_L("Connect to internal drive"));
	anotherAtaDrive.Connect(DriveNumber,SecThreadChangeFlag);

	nTest.Next(_L("Capabilities"));
	TLocalDriveCapsV2 info;
	TPckg<TLocalDriveCapsV2> infoPckg(info);
	nTest(anotherAtaDrive.Caps(infoPckg)==KErrNone);
	nTest(info.iType==EMediaHardDisk);

    nTest.End();
	return(KErrNone);
	}

LOCAL_C void ProgressBar(TInt aPos,TInt anEndPos,TInt anXPos)
//
// Display progress of local drive operation on screen (1-16 dots)
//
	{
	static TInt prev;
	TInt curr;
	if ((curr=(aPos-1)/(anEndPos>>4))>prev)
		{ // Update progress bar
		test.Console()->SetPos(anXPos);
		for (TInt i=curr;i>=0;i--)
			test.Printf(_L("."));
		}
	prev=curr;
	}

#pragma warning( disable : 4702 ) // unreachable code

GLDEF_C TInt E32Main()
    {
	TInt i;
	TBuf<64> b;

	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();
	test.Title();
	test.Start(_L("Test the Compact Flash card (ATA) media drive"));
	test.Printf(_L("DRIVES PRESENT  :%d\r\n"),di.iTotalSupportedDrives);
	test.Printf(_L("1ST DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[0]);
	test.Printf(_L("2ND DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[1]);
	test.Printf(_L("3RD DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[2]);
	test.Printf(_L("4TH DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[3]);
	test.Printf(_L("5TH DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[4]);
	test.Printf(_L("6TH DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[5]);
	test.Printf(_L("7TH DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[6]);
	test.Printf(_L("8TH DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[7]);
	test.Printf(_L("9TH DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[8]);

	test.Printf(_L("\r\nWarning - all data on removable drive will be lost.\r\n"));
	test.Printf(_L("<<<Hit D to continue>>>\r\n"));
	TChar c=(TUint)test.Getch();
	c.UpperCase();
	DriveNumber=((TUint)c)-'C';
	test(DriveNumber >= 1 && DriveNumber < di.iTotalSupportedDrives);

#if defined (__WINS__)
	// Connect to all the local drives first as will be the case in ARM
	TBusLocalDrive Drive[KMaxLocalDrives];
	TBool DriveFlag[KMaxLocalDrives];
	for (i=0;i<KMaxLocalDrives;i++)
		Drive[i].Connect(i,DriveFlag[i]);
#endif

	test.Next(_L("Load ATA Media Driver"));
	TInt r=User::LoadPhysicalDevice(PDD_NAME);
	test(r==KErrNone||r==KErrAlreadyExists);

    test.Next(_L("Read machine information"));
	TInt mid;
	r=HAL::Get(HAL::EMachineUid,mid);
	test(r==KErrNone);
	TBool mediaChangeSupported=EFalse;

	b.Format(_L("Connect to local drive (%c:)"),DriveNumber+'C');
	test.Next(b);
	TBusLocalDrive theAtaDrive;
	ChangeFlag=EFalse;
	test(theAtaDrive.Connect(DriveNumber,ChangeFlag)==KErrNone);
	if (mediaChangeSupported)
		{
		theAtaDrive.ForceMediaChange();	// Generate media change to reset PC Card current consumption
		User::After(300000);			// Allow 0.3s after power down for controller to detect door closed.
		}
//	TSupplyInfoV1Buf supply1;
//	test(UserHal::SupplyInfo(supply1)==KErrNone);

	test.Next(_L("ATA drive: Capabilities"));
	TInt diskSize;
	TTime startTime;
	startTime.HomeTime();
	TLocalDriveCapsV2 info;
	TPckg<TLocalDriveCapsV2> infoPckg(info);
	test(theAtaDrive.Caps(infoPckg)==KErrNone);
	diskSize=I64LOW(info.iSize);
	test.Printf( _L("Check drive size: %d\r\n"),diskSize);
#if defined (__WINS__)
	test.Printf(_L("Check hidden sectors (=0): %d\r\n"),info.iHiddenSectors);
#else
	test.Printf(_L("Check hidden sectors (=16/32): %d\r\n"),info.iHiddenSectors);
#endif
	// test.Getch();
	test(info.iType==EMediaHardDisk);
	test(info.iConnectionBusType==EConnectionBusInternal);
	test(info.iDriveAtt==(TUint)(KDriveAttLocal|KDriveAttRemovable));
	test(info.iMediaAtt==KMediaAttFormattable);
	test(info.iFileSystemId==KDriveFileSysFAT);
#undef USE_IDLE_CURRENT
#ifdef USE_IDLE_CURRENT
	TSupplyInfoV1Buf supply2;
	test(UserHal::SupplyInfo(supply2)==KErrNone);
	if (mediaChangeSupported)
		test(supply2().iCurrentConsumptionMilliAmps==supply1().iCurrentConsumptionMilliAmps+KAtaIdleCurrentInMilliAmps); // Snowball idle current is zero
#endif

	b.Format(_L("ATA drive: Sector RdWr(%d)"),KAtaSectorSize);
	test.Next(b);
	TInt len;
	wrBuf.SetLength(KAtaSectorSize);
	TUint *p=(TUint*)&wrBuf[0];
	for (i=0;i<KAtaSectorSize;i++)
		wrBuf[i]=(TUint8)i;

	test.Printf(_L("Writing    "));
	for (i=0;i<diskSize;i+=len)	 // B - Sector wr/rd on sector boundary
		{
		ProgressBar(i,diskSize,11);
		len=Min(KAtaSectorSize,(diskSize-i));
		(*p)=(i/KAtaSectorSize);
		wrBuf.SetLength(len);
		test(theAtaDrive.Write(i,wrBuf)==KErrNone);
		}
	test.Printf(_L("\r\nReading    "));
	for (i=0;i<diskSize;i+=len)
		{
		ProgressBar(i,diskSize,11);
		len=Min(KAtaSectorSize,(diskSize-i));
		rdBuf.Fill(0,len);
 		test(theAtaDrive.Read(i,len,rdBuf)==KErrNone);
		(*p)=(i/KAtaSectorSize);
		wrBuf.SetLength(len);
  	    test(rdBuf.Compare(wrBuf)==0);
		}
	test.Printf(_L("\r\n"));

	b.Format(_L("ATA drive: Short RdWr(1) (%dbytes at %d)"),25,0); 
	test.Next(b);
	singleSectorRdWrTest(theAtaDrive,0,25); // A - Sub-sector wr/rd at sector start

	b.Format(_L("ATA drive: Short RdWr(2) (%dbytes at %d)"),16,277); 
	test.Next(b);
	singleSectorRdWrTest(theAtaDrive,277,16); // E - Sub-sector wr/rd in mid sector

	b.Format(_L("ATA drive: Short RdWr(3) (%dbytes at %d)"),100,412); 
	test.Next(b);
	singleSectorRdWrTest(theAtaDrive,412,100); // F - Sub-sector wr/rd at sector end

	b.Format(_L("ATA drive: Long RdWr(1) (%dbytes at %d)"),KAtaSectorSize+15,0);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,0,KAtaSectorSize+15); // C - Long wr/rd starting on sector boundary

	b.Format(_L("ATA drive: Long RdWr(2) (%dbytes at %d)"),(KAtaSectorSize<<1),0);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,0,(KAtaSectorSize<<1)); // D - Long wr/rd starting/ending on sector boundary

	b.Format(_L("ATA drive: Long RdWr(3) (%dbytes at %d)"),KAtaSectorSize+3,509);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,509,KAtaSectorSize+3); // H -  - Long wr/rd ending on sector boundary

	b.Format(_L("ATA drive: Long RdWr(4) (%dbytes at %d)"),(KAtaSectorSize<<1),508);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,508,(KAtaSectorSize<<1));

	b.Format(_L("ATA drive: Sector RdWr across sector boundary(%dbytes at %d)"),KAtaSectorSize,508);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,508,KAtaSectorSize); // G - Sector wr/rd over sector boundary

  	b.Format(_L("ATA drive: Very long RdWr(1) (%dbytes at %d)"),KRdWrBufLen,0);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,0,KRdWrBufLen); // Exceeds driver's buffer, starts/ends on sector boundary

  	b.Format(_L("ATA drive: Very long RdWr(2) (%dbytes at %d)"),(KRdWrBufLen-KAtaSectorSize+5),507);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,507,(KRdWrBufLen-KAtaSectorSize+5)); // Exceeds driver's buffer, ends on sector boundary

  	b.Format(_L("ATA drive: Very long RdWr(3) (%dbytes at %d)"),KRdWrBufLen,10);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,10,KRdWrBufLen); // Exceeds driver's buffer, starts/ends off sector boundary

  	b.Format(_L("ATA drive: Very long RdWr(4) (%dbytes at %d)"),(KRdWrBufLen-3),0);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,0,KRdWrBufLen-3); // Exceeds driver's buffer, starts on sector boundary

  	b.Format(_L("ATA drive: Very long RdWr(5) (%dbytes at %d)"),(KRdWrBufLen-KAtaSectorSize),27);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,27,(KRdWrBufLen-KAtaSectorSize)); // Exceeds driver's buffer (due to start offset), starts/ends off sector boundary

  	b.Format(_L("ATA drive: Very long RdWr(6) (%dbytes at %d)"),(KRdWrBufLen-KAtaSectorSize-3),0);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,0,KRdWrBufLen-KAtaSectorSize-3); // Equals driver's buffer, starts on sector boundary

  	b.Format(_L("ATA drive: Very long RdWr(7) (%dbytes at %d)"),(KRdWrBufLen-3),3);
	test.Next(b);
	MultipleSectorRdWrTest(theAtaDrive,3,KRdWrBufLen-3); // Equals driver's buffer, ends on sector boundary
/*
	test.Next(_L("ATA drive: Inter-thread RdWr"));
	RThread dummyThread;
	dummyThread.Duplicate(RThread());
  	TInt threadHandle=dummyThread.Handle();
	wrBuf.SetLength(KAtaSectorSize);
	for (i=0;i<KAtaSectorSize;i++)
		wrBuf[i]=(TUint8)i;
	test(theAtaDrive.Write(10,KAtaSectorSize,&wrBuf,threadHandle,0)==KErrNone);
	rdBuf.Fill(0,KAtaSectorSize);
 	test(theAtaDrive.Read(10,KAtaSectorSize,&rdBuf,threadHandle,0)==KErrNone);
  	test(rdBuf.Compare(wrBuf)==0);
	dummyThread.Close();
*/
	test.Next(_L("ATA drive: Format sectors (short)"));
	TBuf8<KAtaSectorSize> savBuf1,savBuf2;
	TInt fmtTestPos=(10<<KAtaSectorShift);
	// Save sectors surrounding those which will be formatted
 	test(theAtaDrive.Read((fmtTestPos-KAtaSectorSize),KAtaSectorSize,savBuf1)==KErrNone);
 	test(theAtaDrive.Read((fmtTestPos+KShortFormatInBytes),KAtaSectorSize,savBuf2)==KErrNone);
	test(theAtaDrive.Format(fmtTestPos,KShortFormatInBytes)==KErrNone);
 	test(theAtaDrive.Read(fmtTestPos,KShortFormatInBytes,rdBuf)==KErrNone);
	wrBuf.Fill(0xFF,KShortFormatInBytes);
  	test(rdBuf.Compare(wrBuf)==0);
    // Check that surrounding sectors unaffected
 	test(theAtaDrive.Read((fmtTestPos-KAtaSectorSize),KAtaSectorSize,rdBuf)==KErrNone);
  	test(rdBuf.Compare(savBuf1)==0);
 	test(theAtaDrive.Read((fmtTestPos+KShortFormatInBytes),KAtaSectorSize,rdBuf)==KErrNone);
  	test(rdBuf.Compare(savBuf2)==0);

	test.Next(_L("ATA drive: Format sectors (long)"));
	fmtTestPos+=(4<<KAtaSectorShift);
	// Save sectors surrounding those which will be formatted
 	test(theAtaDrive.Read((fmtTestPos-KAtaSectorSize),KAtaSectorSize,savBuf1)==KErrNone);
 	test(theAtaDrive.Read((fmtTestPos+KLongFormatInBytes),KAtaSectorSize,savBuf2)==KErrNone);
	test(theAtaDrive.Format(fmtTestPos,KLongFormatInBytes)==KErrNone);
 	test(theAtaDrive.Read(fmtTestPos,KLongFormatInBytes,rdBuf)==KErrNone);
	wrBuf.Fill(0xFF,KLongFormatInBytes);
  	test(rdBuf.Compare(wrBuf)==0);
    // Check that surrounding sectors unaffected
 	test(theAtaDrive.Read((fmtTestPos-KAtaSectorSize),KAtaSectorSize,rdBuf)==KErrNone);
  	test(rdBuf.Compare(savBuf1)==0);
 	test(theAtaDrive.Read((fmtTestPos+KLongFormatInBytes),KAtaSectorSize,rdBuf)==KErrNone);
  	test(rdBuf.Compare(savBuf2)==0);

	test.Next(_L("ATA drive: Format entire disk"));
	TFormatInfo fi;
	test.Printf(_L("Formatting "));
	TInt ret;
	while((ret=theAtaDrive.Format(fi))!=KErrEof)
		{
		ProgressBar((fi.i512ByteSectorsFormatted<<9),diskSize,11);
		test(ret==KErrNone);
		}
	test.Printf(_L("\r\nReading    "));
	for (i=0;i<diskSize;i+=len)
		{
		ProgressBar(i,diskSize,11);
		len=Min(KAtaSectorSize,(diskSize-i));
		rdBuf.Fill(0x55,len);
 		test(theAtaDrive.Read(i,len,rdBuf)==KErrNone);
		wrBuf.SetLength(len);
  		test(rdBuf.Compare(wrBuf)==0);
		}

	TTime endTime;
	endTime.HomeTime();
	TTimeIntervalMicroSeconds elapsed=endTime.MicroSecondsFrom(startTime);
	test.Printf(_L("   (Elapsed time: %dmS)\r\n"),(elapsed.Int64()/1000));

	if (!mediaChangeSupported)
		{
		// Remainder of tests involve media change so stop now
		test.End();
		return(0);
		}
	
	test.Next(_L("ATA drive: Media change"));
#if defined (__WINS__)
	test.Printf( _L("<<<Hit F5 - then any other key>>>\r\n"));
#else
	test.Printf( _L("<<<Generate Media change - then hit a key>>>\r\n"));
#endif
	test.Getch();
	User::After(300000);	// Allow 0.3s after power down for controller to detect door closed.
	test(ChangeFlag);
//	test(UserHal::SupplyInfo(supply2)==KErrNone);
//	test(supply2().iCurrentConsumptionMilliAmps==supply1().iCurrentConsumptionMilliAmps);
	__KHEAP_MARK;

	test.Next(_L("ATA drive: Caps following media change"));
	test(theAtaDrive.Caps(infoPckg)==KErrNone);
	test(info.iType==EMediaHardDisk);
#undef USE_IDLE_CURRENT
#ifdef USE_IDLE_CURRENT
	test(UserHal::SupplyInfo(supply2)==KErrNone);
	test(supply2().iCurrentConsumptionMilliAmps==supply1().iCurrentConsumptionMilliAmps+KAtaIdleCurrentInMilliAmps);
#endif

	test.Next(_L("ATA drive: Caps while OOM"));
	TInt err=KErrNoMemory;
	test.Printf(_L("Mount returns:"));
	for (TInt j=1; err!=KErrNone && j<16; j++)
		{
		theAtaDrive.ForceMediaChange();	// Generate media change
		User::After(300000);	// Allow 0.3s after power down for controller to detect door closed.
//		__KHEAP_MARK;
		__KHEAP_SETFAIL(RHeap::EDeterministic,j);
		err=theAtaDrive.Caps(infoPckg);
		test.Printf(_L("(%d)"),err);
		test(err==KErrNoMemory || err==KErrNone);
//		__KHEAP_MARKEND;		// fails because card functions only released by media change or power down
		__KHEAP_RESET;
		}
	test(err==KErrNone);
	test.Printf(_L("\r\n"));
	theAtaDrive.ForceMediaChange();	// Generate media change
	User::After(300000);	// Allow 0.3s after power down for controller to detect door closed.
	__KHEAP_MARKEND;		// test memory released after media change

//	__KHEAP_MARK;
	test.Next(_L("ATA drive: Caps before power off"));
	test(theAtaDrive.Caps(infoPckg)==KErrNone);
	test(info.iType==EMediaHardDisk);

	test.Next(_L("ATA drive: Machine power-off."));
	ChangeFlag=EFalse;
	RTimer timer;
	test(timer.CreateLocal()==KErrNone);
	TRequestStatus timerStat;
	TTime tim;
	tim.HomeTime();
	tim+=TTimeIntervalSeconds(8);
	timer.At(timerStat,tim);
	UserHal::SwitchOff();
	User::WaitForRequest(timerStat);
	test(!ChangeFlag);		// ie machine power off hasn't updated it
	timer.Close();
//	__KHEAP_MARKEND;		// test memory released on power off

	test.Next(_L("ATA drive: Caps following power off"));
	test(theAtaDrive.Caps(infoPckg)==KErrNone);
	test(info.iType==EMediaHardDisk);

	test.Next(_L("Starting 2nd thread"));
	SecThreadChangeFlag=EFalse;
	RThread thread;
	TRequestStatus stat;
	test(thread.Create(_L("Thread"),dontDisconnectThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("ATA drive: 2nd media change"));
	theAtaDrive.ForceMediaChange();		// Generate media change
	test(ChangeFlag);
	test(!SecThreadChangeFlag);	// Closed 2nd thread so shouldn't have been updated

	b.Format(_L("Disconnect from local drive (%c:)"),DriveNumber+'C');
	test.Next(b);
	theAtaDrive.Disconnect();

	test.End();

#if defined (__WINS__)
	for (i=0;i<KMaxLocalDrives;i++)
		Drive[i].Disconnect();
#endif
	return(0);
	}
  
