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
// e32test\pccd\t_idrv.cpp
// Overview:
// Tests for the internal RAM drive
// API Information:
// TBusLocalDrive
// Details:
// - Load a Physical Device Driver for the RAM Media Driver.
// - Find the internal drive: type == EMediaRam
// - Display and adjust various drive capabilities, verify results
// are as expected.
// - Read and write the drive using various drive sizes, verify results
// are as expected.
// - Format the drive, verify results.
// - Set original size and reformat. 
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include "../mmu/mmudetect.h"
#include <f32file.h>

#define PDD_NAME _L("MEDINT")

const TInt KTestDriveLen=0x00040000;	//256K
const TInt KSmallDriveInc=0x00000400;	//1K
#if defined (__WINS__)
const TInt KBigDriveLen=0x00100000;		//1M - WINS
#endif
const TInt KTestBufLen=256;


RTest test(_L("T_IDRV"));

void Format(TInt aDrive, RFs& aFs)
//
// Format current drive
//
	{
	test.Next(_L("Format"));
	TBuf<4> driveBuf=_L("?:\\");
	driveBuf[0]=(TText)(aDrive+'A');
	RFormat format;
	TInt count;
	TInt r=format.Open(aFs,driveBuf,EHighDensity,count);
	test(r==KErrNone);
	while(count)
		{
		TInt r=format.Next(count);
		test(r==KErrNone);
		}
	format.Close();
	}

GLDEF_C TInt E32Main()
    {

	test.Title();
	if (!HaveVirtMem())
		{
		test.Printf(_L("Needs MMU\n"));
		return 0;
		}
#if defined(__EPOC32__) && defined(__CPU_X86)
	test.Printf(_L("Doesn't run on X86\n"));
#else

	TBusLocalDrive theInternalDrive;
    TInt msgHandle = KLocalMessageHandle;
	
	UserSvr::UnlockRamDrive();
	
	test.Printf(_L("Warning - this will destroy internal drive.\r\n"));
	TChar c= 'C';
	c.UpperCase();
	if (c!='C')
		return(0);

	test.Start(_L("Check loader running"));

	test.Next(_L("Load Internal Ram Media Driver"));
	TInt r=User::LoadPhysicalDevice(PDD_NAME);
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Find internal drive"));
	
	TInt drive;
	for (drive = 0; drive < KMaxLocalDrives; drive++)
		{
		TBool changedFlag;
		if (theInternalDrive.Connect(drive, changedFlag) != KErrNone)
			continue;

		TLocalDriveCapsV2 info;
		TPckg<TLocalDriveCapsV2> infoPckg(info);
		theInternalDrive.Caps(infoPckg);

		if (info.iType == EMediaRam)
			break;						// found it

		theInternalDrive.Disconnect();
		}
	test(drive < KMaxLocalDrives);		// iterated over all, found none

	test.Next(_L("Capabilities"));
	TLocalDriveCapsV2 info;
	TPckg<TLocalDriveCapsV2> infoPckg(info);
	test(theInternalDrive.Caps(infoPckg)==KErrNone);
	TUint saveSize=I64LOW(info.iSize);
	test(info.iType==EMediaRam);
	test(info.iConnectionBusType==EConnectionBusInternal);
	test(info.iDriveAtt==(KDriveAttLocal|KDriveAttInternal));
	test(info.iMediaAtt==(KMediaAttVariableSize|KMediaAttFormattable));
	test(info.iFileSystemId==KDriveFileSysFAT);

	test.Printf(_L("Current drive size: %lx\n"),info.iSize);

	test.Next(_L("Set size to zero"));
	test(theInternalDrive.ReduceSize(0,saveSize)==KErrNone);
	test(theInternalDrive.Caps(infoPckg)==KErrNone);
	test(info.iSize==0);
	test(theInternalDrive.ReduceSize(0,-1)==KErrArgument);
	test(theInternalDrive.Enlarge(-1)==KErrArgument);

	test.Next(_L("Increase to large size"));
#if defined (__WINS__)
	TUint cSize=KBigDriveLen;
#else
	TMemoryInfoV1Buf memBuf;
	TMemoryInfoV1 &mi=memBuf();
	UserHal::MemoryInfo(memBuf);
//	TUint cSize=(mi.iTotalRamInBytes-KTestDriveLen); // Leave last 256K - used by Kernel etc.
//	TUint cSize=mi.iTotalRamInBytes>>1; 			 // Half ram
//	TUint cSize=mi.iTotalRamInBytes>>2; 			 // Quarter ram
	TUint cSize=mi.iTotalRamInBytes>>3; 			 // Eighth ram
#endif
	test.Printf(_L("(Increasing to %dbytes)\r\n"),cSize);
	test(theInternalDrive.Enlarge(cSize)==KErrNone);
//	test(theInternalDrive.Enlarge(cSize-saveSize)==KErrNone); // ???
	test(theInternalDrive.Caps(infoPckg)==KErrNone);
	test(I64LOW(info.iSize)==cSize);

	test.Next(_L("Increase by 1K"));
	cSize+=KSmallDriveInc;
	test(theInternalDrive.Enlarge(KSmallDriveInc)==KErrNone);
	test(theInternalDrive.Caps(infoPckg)==KErrNone);
	test(I64LOW(info.iSize)==cSize);

	test.Next(_L("Reduce to 256K"));
	test(theInternalDrive.ReduceSize(0,(cSize-KTestDriveLen))==KErrNone);
	cSize=KTestDriveLen;
	test(theInternalDrive.Caps(infoPckg)==KErrNone);
	test(I64LOW(info.iSize)==(TUint)KTestDriveLen);

	test.Next(_L("Write/Read"));
	TBuf8<KTestBufLen> wrBuf(KTestBufLen),rdBuf;
	TUint i,j,len;
	for (i=0 ; i<(TUint)KTestBufLen ; i++)
		wrBuf[i]=(TUint8)i;
	for (i=0,j=0;i<(TUint)KTestDriveLen;i+=len,j++)
		{
		len=Min(KTestBufLen,(KTestDriveLen-i));
		rdBuf.Fill(0,len);
		wrBuf[0]=(TUint8)j;
		test(theInternalDrive.Write(i,len,&wrBuf,msgHandle,0)==KErrNone);
 		test(theInternalDrive.Read(i,len,&rdBuf,msgHandle,0)==KErrNone);
		wrBuf.SetLength(len);
  	    test(rdBuf.Compare(wrBuf)==0);
		}

	test.Next(_L("Reduce size - 256 bytes from start"));
 	test(theInternalDrive.ReduceSize(0,KTestBufLen)==KErrNone);
	test(theInternalDrive.Caps(infoPckg)==KErrNone);
	cSize-=KTestBufLen;
	test(I64LOW(info.iSize)==(TUint)cSize);
	for (i=0,j=1;i<cSize;i+=len,j++)
		{
		len=Min(KTestBufLen,(cSize-i));
		rdBuf.Fill(0,len);
		wrBuf[0]=(TUint8)j;
 		test(theInternalDrive.Read(i,len,&rdBuf,msgHandle,0)==KErrNone);
		wrBuf.SetLength(len);
  	    test(rdBuf.Compare(wrBuf)==0);
		}

	test.Next(_L("Reduce size - (4K+127) bytes from middle"));
	TInt reduction=((KTestBufLen<<4)+((KTestBufLen>>1)-1)); 
 	test(theInternalDrive.ReduceSize(KTestBufLen,reduction)==KErrNone); 
	test(theInternalDrive.Caps(infoPckg)==KErrNone);
	cSize-=reduction;
	test(I64LOW(info.iSize)==(TUint)cSize);
	TBuf8<KTestBufLen> odBuf(KTestBufLen); // To verify new pattern 
	for (i=0 ; i<(TUint)KTestBufLen ; i++)
		{
		if (i<=(KTestBufLen>>1))
			odBuf[i]=(TUint8)(i+((KTestBufLen>>1)-1));
		else
			odBuf[i]=(TUint8)(i-((KTestBufLen>>1)+1));
		}
	for (i=0,j=1;i<cSize;i+=len,j++)
		{
		len=Min(KTestBufLen,(cSize-i));
		rdBuf.Fill(0,len);
 		test(theInternalDrive.Read(i,len,&rdBuf,msgHandle,0)==KErrNone);
		if (j==2)
			j+=17;
		if (j==1)
			{
			wrBuf[0]=(TUint8)j;
			wrBuf.SetLength(len);
  	    	test(rdBuf.Compare(wrBuf)==0);
			}
		else
			{
			odBuf.SetLength(KTestBufLen);
			odBuf[((KTestBufLen>>1)+1)]=(TUint8)j;
			odBuf.SetLength(len);
  	    	test(rdBuf.Compare(odBuf)==0);
			}
		}

	test.Next(_L("Reduce size - (8K-1) bytes from end"));
	reduction=((KTestBufLen<<5)-1); 
	test(theInternalDrive.ReduceSize((cSize-reduction),reduction)==KErrNone);
	test(theInternalDrive.Caps(infoPckg)==KErrNone);
	cSize-=reduction;
	test(info.iSize==cSize);
	for (i=0,j=1;i<cSize;i+=len,j++)
		{
		len=Min(KTestBufLen,(cSize-i));
		rdBuf.Fill(0,len);
 		test(theInternalDrive.Read(i,len,&rdBuf,msgHandle,0)==KErrNone);
		if (j==2)
			j+=17;
		if (j==1)
			{
			wrBuf[0]=(TUint8)j;
			wrBuf.SetLength(len);
  	    	test(rdBuf.Compare(wrBuf)==0);
			}
		else
			{
			odBuf.SetLength(KTestBufLen);
			odBuf[((KTestBufLen>>1)+1)]=(TUint8)j;
			odBuf.SetLength(len);
  	    	test(rdBuf.Compare(odBuf)==0);
			}
		}

	test.Next(_L("Format"));
	wrBuf.Fill(0,KTestBufLen);
	TFormatInfo fi;
	TInt ret;
	while((ret=theInternalDrive.Format(fi))!=KErrEof)
		test(ret==KErrNone);
	for (i=0;i<cSize;i+=len)
		{
		len=Min(KTestBufLen,(cSize-i));
		rdBuf.Fill(0xAA,len);
 		test(theInternalDrive.Read(i,len,&rdBuf,msgHandle,0)==KErrNone);
		wrBuf.SetLength(len);
  	    test(rdBuf.Compare(wrBuf)==0);
		}

	test.Next(_L("Restore original size"));
	TInt sizeDif=cSize-saveSize;
	if (sizeDif>0)
		test(theInternalDrive.ReduceSize(0,sizeDif)==KErrNone);
	else
		test(theInternalDrive.Enlarge(sizeDif*-1)==KErrNone);

	test.Next(_L("Disconnect from internal drive"));
	theInternalDrive.Disconnect();

	RFs fs;
	test(fs.Connect()==KErrNone);
	for(drive=25 ; drive>=0; --drive)
		{
		TDriveInfo info;
		if(fs.Drive(info,drive)==KErrNone)
			if(info.iType==EMediaRam)
				{
				TBuf<256> text;
				text.Append(_L("Formatting drive "));
				text.Append(TText(drive+'A'));
				text.Append(_L(": ..."));
				test.Next(text);
				Format(drive,fs);
				break;
				}
		}

    test.End();

#endif	// x86
	return(0);
	}
  
