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
// e32test\pccd\t_media.cpp
// Test the Compact Flash card (ATA) media driver
// 
//

#include <e32test.h>
#include <e32svr.h>
#include "u32std.h"
#include "../misc/prbs.h"

const TInt KSectorSize=512;

LOCAL_D RTest test(_L("T_MEDIA"));
LOCAL_D TBusLocalDrive TheDrive;
LOCAL_D TBool MediaChange=EFalse;
LOCAL_D TUint Seed[2];

LOCAL_D TUint8 Background1[64*KSectorSize];
LOCAL_D TUint8 Background2[64*KSectorSize];
LOCAL_D TUint8 Foreground1[64*KSectorSize];
LOCAL_D TUint8 Foreground2[64*KSectorSize];
LOCAL_D TUint8 VerifyBuffer[64*KSectorSize];

inline TUint RoundDownToSector(TUint aPos)
	{ return aPos&~0x1ff; }
inline TUint RoundUpToSector(TUint aPos)
	{ return (aPos+0x1ff)&~0x1ff; }

LOCAL_C void TestPattern(TUint8* aBuf, TInt aLength)
	{
	while(aLength--)
		*aBuf++=(TUint8)Random(Seed);
	}

LOCAL_C void Write(TUint aPos, TInt aLength, const TUint8* aBuffer)
	{
	TPtrC8 p(aBuffer,aLength);
	TInt r=TheDrive.Write(aPos,p);
	if (r!=KErrNone)
		{
		test.Printf(_L("Write failed with error %d\n"),r);
		test.Printf(_L("Pos=%08x, Length=%x\n"),aPos,aLength);
		test(0);
		}
	}

LOCAL_C void DebugDump(TUint aPos, TInt aLength, const TUint8* aBuf, const TDesC& aTitle)
	{
	RDebug::Print(aTitle);
	TUint end=aPos+aLength;
	TInt i;
	TInt j=0;
	while(aPos<end)
		{
		TBuf<80> buf;
		buf.NumFixedWidthUC(aPos,EHex,8);
		buf+=_L(": ");
		for (i=0; i<16; i++)
			{
			buf.AppendNumFixedWidthUC(aBuf[j+i],EHex,2);
			buf+=_L(" ");
			}
		RDebug::Print(buf);
		aPos+=16;
		j+=16;
		if ((aPos&(KSectorSize-1))==0)
			RDebug::Print(_L(""));
		}
	}

LOCAL_C void Verify(TUint aPos, TInt aLength, const TUint8* aRef)
	{
	TPtr8 p(VerifyBuffer,0,64*KSectorSize);
	TInt r=TheDrive.Read(aPos,aLength,p);
	if (r!=KErrNone)
		{
		test.Printf(_L("Read failed with error %d\n"),r);
		test.Printf(_L("Pos=%08x, Length=%x\n"),aPos,aLength);
		test(0);
		}
	if (p.Length()!=aLength)
		{
		test.Printf(_L("Incorrect length after read: Was %08x Expected %08x\n"),p.Length(),aLength);
		test.Printf(_L("Pos=%08x, Length=%x\n"),aPos,aLength);
		test(0);
		}
	r=Mem::Compare(VerifyBuffer,aLength,aRef,aLength);
	if (r==0)
		return;
	TInt i=0;
	while(i<aLength && VerifyBuffer[i]==aRef[i])
		i++;
	test.Printf(_L("Verify error: aPos=%08x, aLength=%08x\n"),aPos,aLength);
	test.Printf(_L("First difference at offset %x\n"),i);
	test.Printf(_L("Press <ENTER> for debug dump "));
	TInt k=test.Getch();
	if (k==EKeyEnter)
		{
		DebugDump(aPos,aLength,VerifyBuffer,_L("Actual:"));
		DebugDump(aPos,aLength,aRef,_L("Expected:"));
		}
	test(0);
	}

LOCAL_C void DoTest(TUint aBasePos, TInt anOffset, TInt aSize)
	{
	TBuf<80> buf;
	buf.Format(_L("Offset %3x Size %04x"),anOffset,aSize);
	test.Next(buf);
	TUint block1=aBasePos;
	TUint block2=aBasePos+64*KSectorSize;
	TUint totalSectorSize=RoundUpToSector(anOffset+aSize);
	TestPattern(Background1,totalSectorSize);
	TestPattern(Background2,totalSectorSize);
	TestPattern(Foreground1,totalSectorSize);
	TestPattern(Foreground2,totalSectorSize);
	Write(block1,totalSectorSize,Background1);
	Write(block2,totalSectorSize,Background2);
	Verify(block1,totalSectorSize,Background1);
	Verify(block2,totalSectorSize,Background2);
	Write(block1+anOffset,aSize,Foreground1);
	Write(block2+anOffset,aSize,Foreground2);
	Mem::Copy(Background1+anOffset,Foreground1,aSize);
	Mem::Copy(Background2+anOffset,Foreground2,aSize);
	Verify(block1,totalSectorSize,Background1);
	Verify(block2,totalSectorSize,Background2);
	}

GLDEF_C TInt E32Main()
	{
	Seed[0]=0xadf85458;
	Seed[1]=0;
	test.Title();
	
	TChar driveToTest;

	// Get the list of drives
	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();
	TInt driveCount = di.iTotalSupportedDrives;
	
	test.Printf(_L("\nDRIVES USED AT PRESENT :\r\n"));
	for (TInt i=0; i < driveCount; i++)
		{
		TBool flag=EFalse;
		RLocalDrive d;
		TInt r=d.Connect(i,flag);
		//Not all the drives are used at present
		if (r == KErrNotSupported)
			continue;

		test.Printf(_L("%d : DRIVE NAME  :%- 16S\r\n"), i, &di.iDriveName[i]);
		}

	test.Printf(_L("\n<<<Hit required drive number to continue>>>\r\n"));

	driveToTest=(TUint)test.Getch();
	
	TInt driveNumber=((TUint)driveToTest) - '0';

	TBuf<0x100> buf;
	buf.Format(_L("Connect to local drive (%d)"),driveNumber);
	test.Start(buf);
	
	TInt r=TheDrive.Connect(driveNumber,MediaChange);
	test(r==KErrNone);
	
	test.Next(_L("Get capabilities"));
	TLocalDriveCapsV2 driveCaps;
	TPckg<TLocalDriveCapsV2> capsPckg(driveCaps);
	r=TheDrive.Caps(capsPckg);
	test(r==KErrNone);
	TUint driveSize=I64LOW(driveCaps.iSize);
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
	TUint basePos=RoundDownToSector(driveSize)-128*KSectorSize;
	test.Printf(_L("Base position    = %08x\n"),basePos);

	TInt offset;
	TInt size;
	for (size=KSectorSize/4; size<=23*KSectorSize/2; size+=KSectorSize/4)
		{
		for (offset=0; offset<KSectorSize; offset+=KSectorSize/2)
			{
			DoTest(basePos,offset,size);
			}
		}

	for (size=12*KSectorSize; size<=33*KSectorSize; size+=KSectorSize/2)
		{
		for (offset=0; offset<KSectorSize; offset+=KSectorSize/2)
			{
			DoTest(basePos,offset,size);
			}
		}

	buf.Format(_L("Disconnect from local drive (%d)"),driveNumber);
	test.Next(buf);
	TheDrive.Disconnect();
	test.End();
	return 0;
	}
