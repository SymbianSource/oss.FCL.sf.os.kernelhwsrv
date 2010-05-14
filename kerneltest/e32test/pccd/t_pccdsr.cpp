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
// e32test\pccd\t_pccdsr.cpp
// Stress test a single sector of Compact Flash card (ATA).
// 
//


//#define USE_F32_ACCESS
#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#if defined (USE_F32_ACCESS)
#include <f32fsys.h>
#include <f32file.h>
#endif

#define ATA_PDD_NAME _L("MEDATA")

const TInt KAtaSectorSize=512;
const TInt KMaxSectors=8;
const TInt KMaxRdWrBufLen=(KAtaSectorSize*KMaxSectors); // 4K
const TInt KMaxErr=8;

#if defined (USE_F32_ACCESS)
GLDEF_D	RFs TheFs;
#else
LOCAL_D	TBusLocalDrive TheDrive;
LOCAL_D TBool ChangedFlag;
#endif
RTest test(_L("Local Drive Stress test"));
LOCAL_D TBuf8<KMaxRdWrBufLen> wrBufPat1,wrBufPat2,rdBuf;

enum TOper {ENone,EWrite,ERead,ECompare};
class TErrInfo
	{
public:
	TErrInfo();
public:
	TInt iError;
	TOper iOperation;
	TInt iCycle;
	};

class TResult
	{
public:
	TResult();
	void Display(CConsoleBase *aConsole, TInt aCycles);
	void Add(TInt anError,TOper anOperation,TInt aCycle);
public:
	TInt iTotalErrs;
	TErrInfo iFirstErr;
	TErrInfo iLastErrs[KMaxErr];
	TInt iNextFreeErr;
	TBool iHadAnError;
	};


LOCAL_C TUint OperationToChar(TOper anOperation)
//
// Convert operation enum to corresponding display character
//
	{

	switch(anOperation)
		{
		case EWrite:
			return('W');
		case ERead:
			return('R');
		case ECompare:
			return('C');
		default:
			return('?');
		}
	}

TErrInfo::TErrInfo()
//
// Constructor
//
	{

	iError=KErrNone;
	iOperation=ENone;
	iCycle=0;
	}

TResult::TResult()
//
// Constructor
//
	{

	iNextFreeErr=0;
	iTotalErrs=0;
	iHadAnError=EFalse;
	}

void TResult::Display(CConsoleBase *aConsole, TInt aCycles)
//
// Display test results
//
	{

	TInt xStartPos=0;
	TInt yStartPos=7;

	aConsole->SetPos(xStartPos,yStartPos);
	test.Printf(_L("CYCLES-> %07d   ERRORS-> %d"),aCycles,iTotalErrs);

	aConsole->SetPos(xStartPos,yStartPos+1);
	test.Printf(_L("FIRST ERROR-> "));
	if (iHadAnError)
		test.Printf(_L("Error:%d Oper:%c Cycle:%07d"),iFirstErr.iError,OperationToChar(iFirstErr.iOperation),iFirstErr.iCycle);

	aConsole->SetPos(xStartPos,yStartPos+2);
	test.Printf(_L("LAST ERRORS->"));
	if (iHadAnError)
		{
		TInt i;
		aConsole->SetPos(xStartPos+3,yStartPos+3);
		test.Printf(_L("Error:  "));
		for (i=0;(i<KMaxErr && iLastErrs[i].iOperation!=ENone);i++)
			test.Printf(_L("% 7d,"),iLastErrs[i].iError);

		aConsole->SetPos(xStartPos+3,yStartPos+4);
		test.Printf(_L("Oper:   "));
		for (i=0;(i<KMaxErr && iLastErrs[i].iOperation!=ENone);i++)
			test.Printf(_L("      %c,"),OperationToChar(iLastErrs[i].iOperation));
 
		aConsole->SetPos(xStartPos+3,yStartPos+5);
		test.Printf(_L("Cycle:  "));
		for (i=0;(i<KMaxErr && iLastErrs[i].iOperation!=ENone);i++)
			test.Printf(_L("% 7d,"),iLastErrs[i].iCycle);
		}

	test.Printf(_L("\r\n"));
	}

void TResult::Add(TInt anError,TOper anOperation,TInt aCycle)
//
// Add a test result
//
	{

	iTotalErrs++;
	if (!iHadAnError)
		{
		iFirstErr.iError=anError;
		iFirstErr.iOperation=anOperation;
		iFirstErr.iCycle=aCycle;
		iHadAnError=ETrue;
		}
	if (iNextFreeErr>=KMaxErr)
		{
		for (TInt i=0;i<(KMaxErr-1);i++)
			iLastErrs[i]=iLastErrs[i+1];
		iNextFreeErr=(KMaxErr-1);
		}
	iLastErrs[iNextFreeErr].iError=anError;
	iLastErrs[iNextFreeErr].iOperation=anOperation;
	iLastErrs[iNextFreeErr].iCycle=aCycle;
	iNextFreeErr++;
	}

GLDEF_C TInt E32Main()
    {
	TBuf<64> b;

	test.Title();
	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();
	test.Printf(_L("Select Local Drive (C-%c): "),'C'+(di.iTotalSupportedDrives-1));
	TChar c;
	TInt drv;
	FOREVER
		{
		c=(TUint)test.Getch();
		c.UpperCase();
		drv=((TUint)c)-'C';
		if (drv>=0&&drv<di.iTotalSupportedDrives)
			break;
		}
	test.Printf(_L("%c:\r\n"),'C'+drv);

	TInt rdWrLen;
#if !defined (USE_F32_ACCESS)
	test.Printf(_L("Select total sectors to write(1-8): "));
	FOREVER
		{
		c=(TUint)test.Getch();
		rdWrLen=((TUint)c)-'0';
		if (rdWrLen>=1&&rdWrLen<=8)
			break;
		}
	test.Printf(_L("%dSector(s)\r\n"),rdWrLen);
	rdWrLen*=KAtaSectorSize;
#else
	rdWrLen=(KAtaSectorSize*2)+1;
#endif

	b.Format(_L("Init test on drive %c:"),'C'+drv);
	test.Start(b);

	TInt r;
#if defined (USE_F32_ACCESS)
	r=TheFs.Connect();
	test(r==KErrNone);

	RFile f;
	b.Format(_L("%c:\\TEMP.BIN"),'C'+drv);
	r=f.Replace(TheFs,b,EFileShareAny|EFileStream|EFileWrite);
	test(r==KErrNone);
	b.Format(_L("Start testing (%c:\\TEMP.BIN):"),'C'+drv);
#else
	r=User::LoadPhysicalDevice(ATA_PDD_NAME);
	test(r==KErrNone || r==KErrAlreadyExists);

	ChangedFlag=EFalse;
	TheDrive.Connect(drv,ChangedFlag);

	TLocalDriveCapsV2Buf info;
	test(TheDrive.Caps(info)==KErrNone);
	test(info().iType==EMediaHardDisk);
	TInt trgPos=I64LOW(info().iSize)-rdWrLen;	  // Hammer the very end of the disk
	b.Format(_L("Start testing (sector %xH):"),trgPos/KAtaSectorSize);
#endif

	test.Next(b);
	wrBufPat1.SetLength(rdWrLen);
	TInt j;
	for (j=0;j<rdWrLen;j++)
		wrBufPat1[j]=(TUint8)j;
	wrBufPat2.SetLength(rdWrLen);
	for (j=0;j<rdWrLen;j++)
		wrBufPat2[j]=(TUint8)((rdWrLen-1)-j);

	TInt cycles=0;
	TResult results;
	TBool toggleTest=EFalse;

	TRequestStatus kStat;
	test.Console()->Read(kStat);
	FOREVER
		{
		if ((cycles%10)==0)
			results.Display(test.Console(),cycles);

		TInt res;
#if defined (USE_F32_ACCESS)
		TInt len=(toggleTest)?rdWrLen:1;

		wrBufPat1.SetLength(len);
		if ((res=f.SetSize(len))==KErrNone)
			res=f.Write(0,wrBufPat1);		// Write test (Pos=0)
		if (res!=KErrNone)
			results.Add(res,EWrite,cycles);

		// Read test
		rdBuf.Fill(0,len);
		res=f.Read(0,rdBuf,len);
		if (res!=KErrNone)
			results.Add(res,ERead,cycles);
		if (rdBuf.Compare(wrBufPat1)!=0)
			results.Add(0,ECompare,cycles);
#else
		TDes8* wrBuf=(toggleTest)?&wrBufPat2:&wrBufPat1; // Change pattern written
		
		res=TheDrive.Write(trgPos,*wrBuf);	// Write test
		if (res!=KErrNone)
			results.Add(res,EWrite,cycles);
		
		rdBuf.Fill(0,rdWrLen);
		res=TheDrive.Read(trgPos,rdWrLen,rdBuf);	// Read test
		if (res!=KErrNone)
			results.Add(res,ERead,cycles);
		if (rdBuf.Compare(*wrBuf)!=0)
			results.Add(0,ECompare,cycles);
#endif
		cycles++;
		toggleTest^=0x01;

		if (kStat!=KRequestPending)
			{
			TKeyCode c=test.Console()->KeyCode();
			if (c==EKeySpace)
				break;
			test.Console()->Read(kStat);
			}
		}

	test.Next(_L("Close"));
#if defined (USE_F32_ACCESS)
	f.Close();
	TheFs.Close();
#else
	TheDrive.Disconnect();
#endif

	test.End();
	return(0);
	}
  
