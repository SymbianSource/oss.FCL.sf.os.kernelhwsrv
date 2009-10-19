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
// bootldr\src\ymodemb.cpp
// 
//

#define FILE_ID		0x594D4442

#include "bootldr.h"
#include <d32comm.h>
#include "ymodemu.h"

_LIT(KSerialLddName, "ECOMM");
_LIT(KSerialPddName, "EUART");

#if 0
GLREF_C void RequestSignal(TInt aCount);
GLREF_C void HandleClose(TInt aHandle);
GLREF_C TInt OpenCommPort(TInt aPort, TInt& aHandle);
GLREF_C void WriteToCommPort(TInt aHandle, TRequestStatus& aStatus, const TDesC8& aDes);
GLREF_C void CommReadOneOrMore(TInt aHandle, TRequestStatus& aStatus, TDes8& aDes);
GLREF_C void CommRead(TInt aHandle, TRequestStatus& aStatus, TDes8& aDes);
GLREF_C void CommReadCancel(TInt aHandle);
#endif 

YModemU* TheYModem;
TInt DownloadMode;

YModemU::YModemU(TBool aG)
	: YModem(aG)
	{
	}

YModemU::~YModemU()
	{
	iComm.Close();
	iTimer.Close();
	}

TInt YModemU::Create(TInt aPort)
	{
	TInt r=iComm.Open(aPort);
	if (r!=KErrNone)
		return r;
	TCommConfig cfg;
	TCommConfigV01& c=cfg();
	iComm.Config(cfg);
	c.iRate=SerialBaud;
	c.iDataBits=EData8;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
//	c.iHandshake=KConfigFreeRTS|KConfigFreeDTR;
	c.iHandshake=0;
	c.iFifo=EFifoEnable;
	c.iTerminatorCount=0;
	r=iComm.SetConfig(cfg);
	if (r!=KErrNone)
		return r;
	r=iComm.SetReceiveBufferLength(8192);
	if (r!=KErrNone)
		return r;
	return iTimer.CreateLocal();
	}

YModemU* YModemU::NewL(TInt aPort, TBool aG)
	{
	YModemU* p=new YModemU(aG);
	TInt r=p->Create(aPort);
	if (r!=KErrNone)
		{
		delete p;
		User::Leave(r);
		}
	return p;
	}

void YModemU::WriteC(TUint aChar)
	{
	TBuf8<1> b;
	b.SetLength(1);
	b[0]=(TUint8)aChar;
	TRequestStatus s;
	iComm.Write(s,b);
	User::WaitForRequest(s);
	}

TInt YModemU::ReadBlock(TDes8& aDest)
	{
	aDest.Zero();
	TRequestStatus s1, s2;
	iTimer.After(s1,iTimeout);
//	test.Printf(_L("@%dms %d\n"),User::FastCounter(),iComm.QueryReceiveBuffer());
	iComm.Read(s2,aDest);
	User::WaitForRequest(s1,s2);
	if (s2!=KRequestPending)
		{
		iTimer.Cancel();
		User::WaitForRequest(s1);
		return s2.Int();
		}
	iComm.ReadCancel();
	User::WaitForRequest(s2);
	return KErrTimedOut;
	}

TInt ReadYModemInputData(TUint8* aDest, TInt& aLength)
	{
	if( TheYModem->IsHeaderStored() )
	    {
	    DEBUG_PRINT((_L(">>ReadYModemInputData(aDest:0x%08x, aLength:%d)\r\n"), aDest, aLength))
	    TInt r = TheYModem->GetHeaderBufferContent(aDest, aLength);
	    DEBUG_PRINT((_L("<<ReadYModemInputData(aDest:0x%08x, aLength:%d):%d\r\n"), aDest, aLength, r))
	    return r;
	    }
    else
        {
        TUint8* pD=aDest;
    	TInt r=TheYModem->ReadPackets(pD,aLength);
    	aLength=pD-aDest;
    	return r;    
        }	    
	
	}

void CloseYModem()
	{
	DEBUG_PRINT((_L(">>CloseYModem()\r\n")));
	TBuf<256> name;
	TInt size=-1;
	TheYModem->StartDownload(DownloadMode, size, name);
	DEBUG_PRINT((_L("<<CloseYModem()\r\n")));
	}

GLDEF_C TInt InitSerialDownload(TInt aPort)
	{
//	RThread().SetSystem(ETrue);

	TInt r=LoadDriver(KSerialLddName,0);
	if (r!=KErrNone)
		BOOT_FAULT();
	r=LoadDriver(KSerialPddName,1);
	if (r!=KErrNone)
		BOOT_FAULT();

	TInt PortNumber=aPort;
	DownloadMode=KYModemGMode;
	
	TRAP(r,TheYModem=YModemU::NewL(PortNumber,ETrue));
	if (r!=KErrNone || TheYModem==NULL)
		BOOT_FAULT();

//	r=YModemB::New(TheYModem,PortNumber,DownloadMode);
//	TEST(r==KErrNone && TheYModem!=NULL);
	YModemU* pY=TheYModem;

	TBuf<256> name;
	r=pY->StartDownload(DownloadMode, FileSize, name);
	if (r!=KErrNone)
		return r;

#ifdef __SUPPORT_UNZIP__
	if (name.Length()>=4 && name.Right(4).MatchF(_L(".zip"))==0 && FileSize!=0)
		ImageZip=ETrue;
	else
#endif
		ImageZip=EFalse;

#ifdef __SUPPORT_FLASH_REPRO__
	if (name.Length()>=8 && (name.Left(8).MatchF(_L("FLASHIMG"))==0 || name.Left(8).MatchF(_L("FLASHLDR"))==0) && FileSize!=0)
		{
		LoadToFlash=ETrue;
		if (name.Left(8).MatchF(_L("FLASHLDR"))==0)
			FlashBootLoader=ETrue;

		}
	else
#endif
		LoadToFlash=EFalse;

    if (!ImageZip)
        {
        r = pY->GetInnerCompression(ImageDeflated, RomLoaderHeaderExists);
        if(KErrNone != r)
            {
            PrintToScreen(_L("Unable to determine the compression!\r\n"));
		    BOOT_FAULT();    
            }
        }
	LoadDevice=ELoadSerial;
	InputFunction=ReadYModemInputData;
	CloseInputFunction=CloseYModem;
	FileName=name;

	return KErrNone;
	}

