// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pccd\t_pccdsk.cpp
// Soak test the Compact Flash card (ATA).
// 
//


// One of these
#define USE_MEDIA_CHANGE
//#define USE_POWER_OFF_ON

#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <hal.h>

#define ATA_PDD_NAME _L("MEDATA")

const TInt KAtaSectorSize=512;
const TInt KMaxSectors=16;
const TInt KMaxRdWrBufLen=(KAtaSectorSize*KMaxSectors); // 8K
const TInt KMaxErrPos=5;

LOCAL_D	TBusLocalDrive TheDrive;
LOCAL_D TBool ChangedFlag;
RTest test(_L("Local Drive Soak Test"));
LOCAL_D TBuf8<KMaxRdWrBufLen> wrBuf1,wrBuf2,rdBuf;

class TResult
	{
public:
	enum TResTest {EWrite,ERead,ECompare,EFormat,EReMount};
	TResult();
	void Display(CConsoleBase *aConsole, TInt aCycles);
	void Add(TResTest aTst,TInt anErr,TInt anErrPos);
	inline void SetMemStillFree(TInt aVal)
		{iFreeMem=aVal;}
	inline void WriteAborted()
		{iAbortedWrites++;}
	inline TInt WriteFails()
		{return(iWriteTimeoutFails+iWriteWriteFails+iWriteGeneralFails+iWriteCorruptFails+iWriteBatLowFails+iWriteOtherFails);}
	inline TInt ReadFails()
		{return(iReadTimeoutFails+iReadCorruptFails+iReadOtherFails);}
	inline TInt CompareFails()
		{return(iCompareFails);}
	inline TInt FormatFails()
		{return(iFormatTimeoutFails+iFormatEmergencyFails+iFormatBatLowFails+iFormatOtherFails);}
	inline TInt ReMountFails()
		{return(iReMountFails);}
public:
	TInt iWriteTimeoutFails;
	TInt iWriteWriteFails;
	TInt iWriteGeneralFails;
	TInt iWriteCorruptFails;
	TInt iWriteBatLowFails;
	TInt iWriteOtherFails;
	TInt iReadTimeoutFails;
	TInt iReadCorruptFails;
	TInt iReadOtherFails;
	TInt iCompareFails;
	TInt iFormatTimeoutFails;
	TInt iFormatEmergencyFails;
	TInt iFormatBatLowFails;
	TInt iFormatOtherFails;
	TInt iReMountFails;
	TInt iLastErrorPos[KMaxErrPos];
	TInt iLastErrorPtr;
	TInt iFreeMem;
	TInt iAbortedWrites;
	};


LOCAL_C void StatusBar(TInt aPos,TInt anEndPos,TInt aYPos,const TPtrC &aTitle)
//
// Display progress of local drive operation on screen (1-16 dots)
//
	{
	static TInt prev;
	TInt curr;
	if ((curr=(aPos-1)/(anEndPos>>4))>prev)
		{ // Update progress bar
		test.Console()->SetPos(0,aYPos);
		test.Printf(_L("                              "));
		test.Console()->SetPos(2);
		test.Printf(_L("%S "),&aTitle);
		for (TInt i=curr;i>=0;i--)
			test.Printf(_L("."));
		}
	prev=curr;
	}

TResult::TResult()
//
// Constructor
//
	{

	iWriteTimeoutFails=0;
	iWriteWriteFails=0;
	iWriteGeneralFails=0;
	iWriteCorruptFails=0;
	iWriteBatLowFails=0;
	iWriteOtherFails=0;
	iReadTimeoutFails=0;
	iReadCorruptFails=0;
	iReadOtherFails=0;
	iCompareFails=0;
	iFormatTimeoutFails=0;
	iFormatEmergencyFails=0;
	iFormatBatLowFails=0;
	iFormatOtherFails=0;
	iReMountFails=0;
	for (TInt i=0;i<KMaxErrPos;i++)
		iLastErrorPos[i]=0;
	iLastErrorPtr=0;
	iFreeMem=0;
	iAbortedWrites=0;
	}

void TResult::Display(CConsoleBase *aConsole, TInt aCycles)
//
// Display test results
//
	{

	TInt xStartPos=3;
	TInt yStartPos=8;

	aConsole->SetPos(xStartPos,yStartPos);
	test.Printf(_L("Cycles(%08xH) : %d"),iFreeMem,aCycles);

	aConsole->SetPos(xStartPos,yStartPos+1);
	if (WriteFails())
		test.Printf(_L("Write Fails       : %d (TO:%d BT:%d WR:%d GE:%d CU:%d OT:%d)"),WriteFails(),iWriteTimeoutFails,\
					   iWriteBatLowFails,iWriteWriteFails,iWriteGeneralFails,iWriteCorruptFails,iWriteOtherFails);
	else
		test.Printf(_L("Write Fails       : 0"));

	aConsole->SetPos(xStartPos,yStartPos+2);
	if (ReadFails())
		test.Printf(_L("Read Fails        : %d (TO:%d CU:%d OT:%d)"),ReadFails(),iReadTimeoutFails,iReadCorruptFails,iReadOtherFails);
	else
		test.Printf(_L("Read Fails        : 0"));

	aConsole->SetPos(xStartPos,yStartPos+3);
	test.Printf(_L("Compare Fails     : %d"),CompareFails());

	aConsole->SetPos(xStartPos,yStartPos+4);
	if (FormatFails())
		test.Printf(_L("Format Fails      : %d (TO:%d EM:%d BT:%d OT:%d)"),FormatFails(),iFormatTimeoutFails,iFormatEmergencyFails,iFormatBatLowFails,iFormatOtherFails);
	else
		test.Printf(_L("Format Fails      : 0"));

	aConsole->SetPos(xStartPos,yStartPos+5);
#if defined (USE_MEDIA_CHANGE)
	test.Printf(_L("MediaChange Fails : %d"),ReMountFails());
#else
	test.Printf(_L("Pwr off/on Fails  : %d"),ReMountFails());
#endif

	aConsole->SetPos(xStartPos,yStartPos+6);
	test.Printf(_L("Last failures at  : "));
	for (TInt i=iLastErrorPtr;i>0;i--)
		test.Printf(_L("%xH "),iLastErrorPos[i-1]);
	aConsole->SetPos(xStartPos,yStartPos+7);
	test.Printf(_L("Writes aborted    : %d"),iAbortedWrites);
	test.Printf(_L("\r\n"));
	}

void TResult::Add(TResTest aTst,TInt anErr,TInt anErrPos)
//
// Add a test result
//
	{

	if (anErr!=KErrNone)
		{
		RDebug::Print(_L("%d) %d(%x)"),aTst,anErr,anErrPos);
		// Save start sector involved in operation which failed
		if (anErrPos>=0)
			{
			if (iLastErrorPtr>=KMaxErrPos)
				{
				TInt i;
				for (i=0;i<(KMaxErrPos-1);i++)
					iLastErrorPos[i]=iLastErrorPos[i+1];
				iLastErrorPos[i]=anErrPos;
				}
			else
				{
				iLastErrorPtr++;
				iLastErrorPos[iLastErrorPtr-1]=anErrPos;
				}
			}

		// Save error type
		switch (aTst)
			{
			case EWrite:
				if (anErr==KErrTimedOut)
					iWriteTimeoutFails++;
				else if (anErr==KErrWrite)
					iWriteWriteFails++;
				else if (anErr==KErrGeneral)
					iWriteGeneralFails++;
				else if (anErr==KErrCorrupt)
					iWriteCorruptFails++;
				else if (anErr==KErrBadPower)
					iWriteBatLowFails++;
				else
					iWriteOtherFails++;
				break;
			case ERead:
				if (anErr==KErrTimedOut)
					iReadTimeoutFails++;
				else if (anErr==KErrCorrupt)
					iReadCorruptFails++;
				else
					iReadOtherFails++;
				break;
			case ECompare:
				iCompareFails++;
				break;
			case EFormat:
				if (anErr==KErrTimedOut)
					iFormatTimeoutFails++;
				else if (anErr==KErrAbort)
					iFormatEmergencyFails++;
				else if (anErr==KErrBadPower)
					iFormatBatLowFails++;
				else
					iFormatOtherFails++;
				break;
			case EReMount:
				iReMountFails++;
				break;
			}
		}
	}

LOCAL_C TUint GetTUintFromConsole(const TDesC &aText)
//
// Get a TUint value from the console
//
    {

    TBuf<10> buf(0);
    TKeyCode kc;
    TUint pos=0;
	test.Printf(aText);
    TUint linePos=(aText.Length()+2);
	test.Console()->SetPos(linePos);
    FOREVER
        {
		switch((kc=test.Getch()))
			{
            case EKeyEscape: case EKeyEnter:
				{
                TLex lex(buf);
	            TUint v;
                if (lex.Val(v,EDecimal)==KErrNone)
                    return(v);
                return(0);
				}
            case EKeyBackspace: case EKeyDelete:
                pos--;
				buf.Delete(pos,1);
                linePos--;
	            test.Console()->SetPos(linePos);
	            test.Printf(_L(" "));
	            test.Console()->SetPos(linePos);
				break;
            default:
				TChar ch=(TUint)kc;
                if (ch.IsDigit() && pos<9)
                    {
                    buf.Append(ch);
					pos++;
	                test.Printf(_L("%c"),(TUint)ch);
					linePos++;
                    }
                break;
			}
        }
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

	test.Printf(_L("Select Test Sequence (Sector 1-R,2-WR,3-WRF)/(SubSector 4-R,5-WR,6-WRF): "));
	TInt testSeq;
	FOREVER
		{
		c=(TUint)test.Getch();
		testSeq=((TUint)c)-'0';
		if (testSeq>=0&&testSeq<=6)
			break;
		}
	test.Printf(_L("%d\r\n"),testSeq);

	TInt RdWrLen=(TInt)GetTUintFromConsole(_L("Select Buffer Size In Sectors: "));
	RdWrLen*=KAtaSectorSize;

	test.Start(_L("Load Ata Media Driver"));
	TInt r;
	r=User::LoadPhysicalDevice(ATA_PDD_NAME);
	test(r==KErrNone || r==KErrAlreadyExists);
#if defined (USE_POWER_OFF_ON)
	RTimer timer;
	test(timer.CreateLocal()==KErrNone);
	TRequestStatus prs;
	TTime tim;
#endif
	TInt muid=0;
	r=HAL::Get(HAL::EMachineUid, muid);
	test(r==KErrNone);
	TBool reMountTestSupported=ETrue;
//	if (machineName.MatchF(_L("SNOWBALL*"))>=0)	// snowball is ancient history
//		reMountTestSupported=EFalse;

	b.Format(_L("Connect to drive %c:"),'C'+drv);
	test.Next(b);
	ChangedFlag=EFalse;
	TheDrive.Connect(drv,ChangedFlag);

	test.Next(_L("ATA drive: Capabilities"));
	TLocalDriveCapsV2Buf info;
	test(TheDrive.Caps(info)==KErrNone);
	test(info().iType==EMediaHardDisk);
	TInt diskSize=I64LOW(info().iSize);

	wrBuf1.SetLength(RdWrLen);
	TInt j;
	for (j=0;j<RdWrLen;j++)
		wrBuf1[j]=(TUint8)j;
	wrBuf2.SetLength(RdWrLen);
	for (j=0;j<RdWrLen;j++)
		wrBuf2[j]=(TUint8)((RdWrLen-1)-j);

	TUint *p;
	TDes8* wrBuf;
	TInt cycles=0;
	TResult results;
	TBool decendPat=EFalse;

	TRequestStatus kStat;
	test.Console()->Read(kStat);
	FOREVER
		{
		wrBuf=(decendPat)?&wrBuf2:&wrBuf1;
		p=(decendPat)?(TUint*)&wrBuf2[0]:(TUint*)&wrBuf1[0];
		TInt i,j,len,res;

		// Recalculate amount of free memory
    	TMemoryInfoV1Buf membuf;
    	UserHal::MemoryInfo(membuf);
    	TMemoryInfoV1 &memoryInfo=membuf();
		results.SetMemStillFree(memoryInfo.iFreeRamInBytes);
		results.Display(test.Console(),cycles);

		// Write test
		RDebug::Print(_L("0"));
		if (testSeq==2||testSeq==3||testSeq==5||testSeq==6)
			{
			for (i=0,j=0;i<diskSize;i+=len,j++)
				{
				StatusBar(i,diskSize,16,_L("WRITING   "));
				if (testSeq>3)
					len=Min(RdWrLen-3,(diskSize-i)); // Not on sector boundary
				else
					len=Min(RdWrLen,(diskSize-i));
				(*p)=j;
				wrBuf->SetLength(len);
				do
					{
					res=TheDrive.Write(i,*wrBuf);
					if (res==KErrAbort)
						{
						results.WriteAborted();
						results.Display(test.Console(),cycles);
						}
					} while (res==KErrNotReady||res==KErrAbort);
				results.Add(TResult::EWrite,res,i);
				if (res!=KErrNone)
					break;
				}
			results.Display(test.Console(),cycles);
			}

		// Read test
		RDebug::Print(_L("1"));
		if (testSeq>=1)
			{
			for (i=0,j=0;i<diskSize;i+=len,j++)
				{
				StatusBar(i,diskSize,16,_L("READING   "));
				if (testSeq>3)
					len=Min(RdWrLen-3,(diskSize-i)); // Not on sector boundary
				else
					len=Min(RdWrLen,(diskSize-i));
				rdBuf.Fill(0,len);
				do
					{
					res=TheDrive.Read(i,len,rdBuf);
					} while (res==KErrNotReady);

				results.Add(TResult::ERead,res,i);
				if (res!=KErrNone)
					break;
				if (testSeq==2||testSeq==3||testSeq==5||testSeq==6)
					{
					(*p)=j;
					wrBuf->SetLength(len);
					if (rdBuf.Compare(*wrBuf)!=0)
						{
						results.Add(TResult::ECompare,KErrGeneral,-1);
						break;
						}
					}
				}
			results.Display(test.Console(),cycles);
			}

		// Format test
		RDebug::Print(_L("3"));
		if (testSeq==3||testSeq==6)
			{ 
			TFormatInfo fi;
			FOREVER
				{
				StatusBar((fi.i512ByteSectorsFormatted<<9),diskSize,16,_L("FORMATTING"));
				do
					{
					res=TheDrive.Format(fi);
					} while (res==KErrNotReady);
				if (res==KErrEof)
					break;
				results.Add(TResult::EFormat,res,(fi.i512ByteSectorsFormatted<<9));
				if (res!=KErrNone)
					break;
				}
			results.Display(test.Console(),cycles);
			}

		RDebug::Print(_L("4"));
		if (reMountTestSupported)
			{
			// Media change test / power off-on test
#if defined (USE_MEDIA_CHANGE)
			TheDrive.ForceMediaChange();
			if (ChangedFlag==EFalse)
				results.Add(TResult::EReMount,KErrGeneral,-1);
#else
			tim.HomeTime();
			tim+=TTimeIntervalSeconds(8);
			timer.At(prs,tim);
			UserHal::SwitchOff();		// Switch off
			User::WaitForRequest(prs);	// Switch back on
			if (prs.Int()!=KErrNone)
				results.Add(TResult::EReMount,KErrGeneral,-1);
#endif
			else
				{
				do
					{
					res=TheDrive.Caps(info);
					} while (res==KErrNotReady);
				if (res==KErrNone)
					{
					if (info().iType!=EMediaHardDisk)
						results.Add(TResult::EReMount,KErrGeneral,-1);
					}
				else
					results.Add(TResult::EReMount,res,-1);
				}
			ChangedFlag=EFalse;
			}

		cycles++;
		decendPat^=0x01;

		if (kStat!=KRequestPending)
			{
			TKeyCode c=test.Console()->KeyCode();
			if (c==EKeySpace)
				break;
			test.Console()->Read(kStat);
			}
		RDebug::Print(_L("<<"));
		}

	b.Format(_L("Disconnect from local drive (%c:)"),'C'+drv);
	test.Next(b);
	TheDrive.Disconnect();

	test.End();
	return(0);
	}
  
