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
// e32test\device\t_sirco2.cpp
// 
//

#include "t_slowir.h"
#include <e32svr.h> 
#include <e32std.h>
#include <e32std_private.h> 
#include <e32hal.h>
#include <hal.h>

#if defined(__VC32__) && _MSC_VER==1100
// Disable MSVC++ 5.0 aggressive warnings about non-expansion of inline functions. 
#pragma warning(disable : 4710)	// function '...' not expanded
#endif

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV.PDD")
#define LDD_NAME _L("ECOMM.LDD")
#else
#define PDD_NAME _L("EUART")
#define LDD_NAME _L("ECOMM")
#endif

#define PDD2_NAME _L("EUART")

#ifdef __WINS__
	#include <es_sock.h>
	RSocketServ ss;
#endif

#define FIND_NAME _L("Comm.*")

const TInt KUnit1=1;
const TInt KUnit2=2;

//LOCAL_D RTest test(_L("T_SlowIR"));

TBuf8<2060> WriteBuf;
TBuf8<2060> ReadBuf;
TInt iTimeDelay=1000000;
TInt iBufSz=2000;
TBool iRWToWrite=EFalse;

void ResetReadBuffer()
	{
	TInt i=0;
	ReadBuf.SetLength(2060);
	for(i=0;i<2050;i++)
		ReadBuf[i] ='R';
	}

void ResetWriteBuffer()
//
// Mark a buffer with repeating byte pattern
//
	{
	TUint startChar='A';
	TUint endChar='z';
	WriteBuf.SetLength(2060);

	TUint character=startChar;
	for (TInt i=0;i<WriteBuf.Length();i++)
		{
		WriteBuf[i]=(TText8)character;
		if(++character>endChar)
			character=startChar;
		}
	}

TInt CActiveRW::ErrorStats()
	{ // Computes error %ge
		if(iRxCount != 0)
			return (iRxErrCount*200+iRxCount)/(2*iRxCount);
		else
			return 0;
	}

TBool CActiveRW::CompareBuffers(TInt aLen)
	{
	TInt i=0;
	if(aLen !=ReadBuf.Length() || aLen !=WriteBuf.Length())
		return EFalse;

	while(i<aLen)
		{
		if(ReadBuf[i]!=WriteBuf[i]){
			return EFalse;}
		i++;
		}
	return ETrue;
	}

void SetUpBuffers()
	{
	ResetReadBuffer();
	ResetWriteBuffer();
	}

CActiveConsole::CActiveConsole(CConsoleBase* aConsole) 
	: CActive(EPriorityNormal)
	{
	iConsole=aConsole;
	iInit1  =EFalse;
	iInit2  =EFalse;
	}

CActiveConsole* CActiveConsole::NewLC(CConsoleBase* aConsole)
	{
	CActiveConsole* self = new (ELeave) CActiveConsole(aConsole);
	self->ConstructL();
	return self;
	}

void CActiveConsole::ConstructL ()
	{ 
	TCommCaps aCapsBuf;
	TCommCapsV01& aCaps=aCapsBuf();
	TCommConfig aConfigBuf;
	TCommConfigV01& aConfig=aConfigBuf();
	iConsole->Printf(_L("\r\n"));
	CActiveScheduler::Add(this);			// Add to active scheduler

    TBuf<10> pddName=PDD_NAME;
#if defined (__MARM__)
	const TInt KMaxPdds=10;
	iConsole->Printf(_L("Load MARM PDDs\n\r"));
	RDebug::Print(_L("Load MARM PDDs\n\r"));	
#else
	const TInt KMaxPdds=0;
	iConsole->Printf(_L("Load WINS PDD\n\r"));
	iConsole->Printf(PDD_NAME);
#endif
//		iConsole->Read(iStatus); 

	TInt i;
	TInt r;
	for (i=-1; i<KMaxPdds; ++i)
		{
		if (i==0)
			pddName.Append(TChar('0'));
		else if (i>0)
			pddName[pddName.Length()-1] = (TText)('0'+i);
		r=User::LoadPhysicalDevice(pddName);
		if (r==KErrNone || r==KErrAlreadyExists)
			{
			iConsole->Printf(_L("PDD %S loaded\n"),&pddName);
			RDebug::Print(_L("PDD %S loaded\n"),&pddName);	
			}
		}

	RDebug::Print(_L("Load MARM LDD\n\r"));	
	RDebug::Print(LDD_NAME);	
	iConsole->Printf(_L("Load LDD\n\r"));
	iConsole->Printf(LDD_NAME);
	r=User::LoadLogicalDevice(LDD_NAME);
	RDebug::Print(_L("\n\rReturn %d\n\r"),r);	
	iConsole->Printf(_L("\n\rReturn %d\n\r"),r);
//	test(r==KErrNone || r==KErrAlreadyExists);

	RDebug::Print(_L("\n\rFind PDDs\n\r"));	
	iConsole->Printf(_L("Find PDDs\n"));
	TFindPhysicalDevice findPDD(FIND_NAME);
	TFullName findResult;
	TInt res=findPDD.Next(findResult);
	while (res==KErrNone)
		{
		RDebug::Print(_L("Found Driver: %S\n\r"),&findResult);	
		iConsole->Printf(_L("Found Driver: %S\n\r"),&findResult);
		res=findPDD.Next(findResult);
		}

	iConsole->Printf(_L("Found drivers\n\r"));

	iPort=new RCommDev;
//	test(iPort!=NULL);

	SetUpBuffers();

	TInt muid=0;
	TInt ret=HAL::Get(HAL::EMachineUid, muid);
	if (ret!=KErrNone)
		iConsole->Printf(_L("Error %d reading MachineUid\r\n"),ret);
	if(muid==HAL::EMachineUid_Integrator)
		iConsole->Printf(_L("Platform is Integrator\r\n"));

	if(muid==HAL::EMachineUid_Integrator) ret=iPort->Open(KUnit2); // unit =2 for Integrator
	else ret=iPort->Open(KUnit1); // unit == 1 for brutus
	if (ret!=KErrNone)
		iConsole->Printf(_L("Error %d on opening Slow ir port\r\n"),ret);
	else
		iConsole->Printf(_L("Successfully opened Slow ir port\r\n"));

	iPort->Caps(aCapsBuf);

	if (ret!=KErrNone)
		iConsole->Printf(_L("Error %d on getting caps\r\n"),ret);
	else
		iConsole->Printf(_L("Sir Caps: %d\r\n"),aCaps.iRate);

	iPort->Config(aConfigBuf);
	if (ret!=KErrNone)
		iConsole->Printf(_L("Error %d getting config\r\n"),ret);
	else
		iConsole->Printf(_L("IR port config read\r\n"));

    iConsole->Printf(_L("****Choose Rate*****\r\n"));
    iConsole->Printf(_L("press '1' for   9600\r\n"));
    iConsole->Printf(_L("press '2' for  19200\r\n"));
    iConsole->Printf(_L("press '3' for  38400\r\n"));
    iConsole->Printf(_L("press '4' for  57600\r\n"));
    iConsole->Printf(_L("press '5' for 115200\r\n"));

    TRequestStatus key;
    iConsole->Read(key);
    User::WaitForRequest(key);
    TChar pressedkey = TChar(iConsole->KeyCode());

    switch(pressedkey)
        {
    case '1':
        aConfig.iRate=EBps9600;
        break;
    case '2':
        aConfig.iRate=EBps19200;
        break;
    case '3':
        aConfig.iRate=EBps38400;
        break;
    case '4':
        aConfig.iRate=EBps57600;
        break;
    case '5':
        aConfig.iRate=EBps115200;
        break;
    default:
        aConfig.iRate=EBps115200;
        break;
        }

	aConfig.iHandshake=0;
	aConfig.iSIREnable=ESIREnable;
	aConfig.iParity=EParityNone;
	aConfig.iDataBits=EData8;
	aConfig.iStopBits=EStop1;

	ret=iPort->SetConfig(aConfigBuf);

	if (ret!=KErrNone)
		iConsole->Printf(_L("Error %d setting config\r\n"),ret);
	else
		iConsole->Printf(_L("Have configured port\n"));


	iRW=CActiveRW::NewL(iConsole,iPort);


	if(iRW)
		iConsole->Printf(_L("Have created writer\r\n"));
	else
		iConsole->Printf(_L("Failed to create writer\r\n"));
	}

CActiveConsole::~CActiveConsole()
	{
	// Make sure we're cancelled
	Cancel();

	if(iRW)
		delete iRW;

	iPort->Close();
	}

void  CActiveConsole::DoCancel()
	{
	iConsole->ReadCancel();
	}

void  CActiveConsole::RunL()
	{
	ProcessKeyPressL(TChar(iConsole->KeyCode()));
//	iConsole->Printf(_L("CActiveConsole - Completed with code %d\r\n\r\n"), iStatus.Int ());
	}

void CActiveConsole::RequestCharacter()
	{
	if(!iInit1)
		{
		Options1();
		return;
		}
	if(!iInit2)
		{
		Options2();
		return;
		}
	  // A request is issued to the CConsoleBase to accept a
	  // character from the keyboard.
	iConsole->Printf(_L("*********************************\r\n"));
	iConsole->Printf(_L("press Escape to quit\r\n"));
	iConsole->Printf(_L("press 'r' to start as reader\r\n"));
	iConsole->Printf(_L("press 'w' to start as writer\r\n"));
	iConsole->Printf(_L("press 's' stop \r\n"));

	iConsole->Read(iStatus); 
	SetActive();
	}

void CActiveConsole::Options1()
	{
	iConsole->Printf(_L("*****Choose Delay*****\r\n"));
	iConsole->Printf(_L("press '1' 1.00 sec delay\r\n"));
	iConsole->Printf(_L("press '2' 0.10 sec delay\r\n"));
	iConsole->Printf(_L("press '3' 0.01 sec delay\r\n"));
	iConsole->Printf(_L("press '4' 0.00 sec delay\r\n"));
	iConsole->Read(iStatus); 
	SetActive();	
	}

void CActiveConsole::Options2()
	{
	iConsole->Printf(_L("****Choose Buf Sz*****\r\n"));
	iConsole->Printf(_L("press '1' 1    byte \r\n"));
	iConsole->Printf(_L("press '2' 4    bytes\r\n"));
	iConsole->Printf(_L("press '3' 2000 bytes\r\n"));
	iConsole->Printf(_L("press '4' 2051 bytes\r\n"));
	iConsole->Read(iStatus); 
	SetActive();	
	}

void CActiveConsole::ProcessKeyPressL(TChar aChar)
	{
	if (aChar == EKeyEscape)
		{
		CActiveScheduler::Stop();
		return;
		}

	if(!iInit1)
		{
		switch(aChar)
			{
		case '1'://1 sec
			iTimeDelay=1000000;
			break;
		case '2'://0.1 sec
			iTimeDelay=100000;
			break;
		case '3'://0.01 sec
			iTimeDelay=10000;
			break;
		case '4'://0 sec
			iTimeDelay=0;
			break;
		default:
			iTimeDelay=1000000;
			break;
			}
		iConsole->Printf(_L("Time Delay: %d\r\n"),iTimeDelay);
		iInit1=ETrue;
		RequestCharacter();
		return;
		}
	if(!iInit2)
		{
		switch(aChar)
			{
		case '1':
			iBufSz=1;
			break;
		case '2':
			iBufSz=4;
			break;
		case '3':
			iBufSz=2000;
			break;
		case '4':
			iBufSz=2051;
			break;
		default:
			iBufSz=2000;
			break;
			}
		// check that we have enough space
		if(iBufSz>iPort->ReceiveBufferLength())
			iPort->SetReceiveBufferLength(iBufSz);
		// if it won't do it, then settle for what it is by default
		if(iBufSz>iPort->ReceiveBufferLength())
			iBufSz=iPort->ReceiveBufferLength();

		iConsole->Printf(_L("Buffer size: %d\r\n"),iBufSz);
		iInit2=ETrue;
		RequestCharacter();
		return;
		}

	switch (aChar)
		{
	case 'r'://start reader
	case 'R'://start reader
		iRW->Start(EFalse);
		break;
	case 'w'://start writer
	case 'W'://start writer
		iRW->Start(ETrue);
		break;
	case 's'://stop reader
	case 'S'://stop reader
		iRW->Stop();
		break;
	default:
		iConsole->Printf(_L("\r\nUnknown Command\r\n\r\n"));
		break;
		}
	RequestCharacter ();
	return;
	}



//
// class CActiveRW
//

CActiveRW::CActiveRW(CConsoleBase* aConsole,RCommDev* aPort)
	: CActive (EPriorityNormal)
	{
	iConsole=aConsole;
	iPort=aPort;
	iLength=0;
	iUnrecovered=0;
	iRxCount=0;
	iRxErrCount=0;
	}

CActiveRW* CActiveRW::NewL(CConsoleBase* aConsole,RCommDev* aPort)
	{
	CActiveRW* self = new (ELeave) CActiveRW(aConsole,aPort);

	CleanupStack::PushL (self);
	self->ConstructL();
	CActiveScheduler::Add (self);
	CleanupStack::Pop ();
	return (self);
	}

void CActiveRW::ConstructL()
	{
	}

CActiveRW::~CActiveRW()
	{
	Cancel();
	}

void CActiveRW::RunL ()
	{

	TInt i=0;
	if(iNextXfer==EWriteXfer)
		iRxCount++;

	if (iStatus != KErrNone)
	{
		if(iNextXfer==EWriteXfer)
			{
			iRxErrCount++;
			iConsole->Printf(_L("Error %d on reading, error = %d%%\r\n"),iStatus.Int(),ErrorStats());
			iPort->ResetBuffers();
			iNextXfer=EDiscardXfer;

			}
		else
			{
			iConsole->Printf(_L("Error %d on writing\r\n"),iStatus.Int());
			iConsole->Printf(_L("Received %d characters\r\n"),iPort->QueryReceiveBuffer());
			}
	}
	//return;

	if(iTimeDelay)
		User::After(iTimeDelay);

	if(iNextXfer==EReadXfer)
		{
//		iPort->ResetBuffers();
		ResetReadBuffer();
		ReadBuf.SetLength(iBufSz);
		iConsole->Printf(_L("Reading(%d)"),ReadBuf.Length());
		iPort->Read(iStatus, ReadBuf, ReadBuf.Length());
		iNextXfer=EWriteXfer;
		}
	else // EWriteXfer || EDiagXfer || EDiscardXfer
		{
		if(iNextXfer==EWriteXfer)
			{
			if(!CompareBuffers(iBufSz))
				{
				iConsole->Printf(_L("\n***** Buffers don't match *****\n"));
				for (i=0;i<ReadBuf.Length();i++)
					{
					iConsole->Printf(_L("WriteBuf[%d] = %02x, ReadBuf[%d] = %02x\n"),i,WriteBuf[i],i,ReadBuf[i]);
					}

				iUnrecovered=iPort->QueryReceiveBuffer();
				iConsole->Printf(_L("Unrecovered %d characters\r\n"),iUnrecovered);
				if(iUnrecovered)
					{
					ReadBuf.SetLength(Max(iBufSz,iUnrecovered));
					iNextXfer=EDiagXfer;
					iConsole->Printf(_L("\n***** Doing Diagnostic read *****\n"));
					iPort->Read(iStatus, ReadBuf, iUnrecovered);
					SetActive();
					return;
					}
				}
			else
				{
				iConsole->Printf(_L("ok\n"));
				}
			}
		else if(iNextXfer==EDiagXfer)//EDiagXfer
			{
			iConsole->Printf(_L("\n***** Diagnostic dump %d *****\n"),iUnrecovered);

			for (i=0;i<iUnrecovered;i++)
				{
				iConsole->Printf(_L("ReadBuf[%d] = %02x\n"),i,ReadBuf[i]);
				}
			}
		WriteBuf.SetLength(iBufSz);
		iConsole->Printf(_L("\nWriting(%d), "),WriteBuf.Length());
		iPort->Write(iStatus, WriteBuf, WriteBuf.Length());
		iNextXfer=EReadXfer;
		
		}
	iUnrecovered=0;
	SetActive();
	}

void CActiveRW::Start(TBool StartWriting)
	{
	iPort->ResetBuffers();
	if(IsActive())
		return;
	if(StartWriting)
		{
//		iConsole->Printf(_L("Starting with write.%d....\r\n"),iBufSz);
		WriteBuf.SetLength(iBufSz);
		iPort->Write(iStatus, WriteBuf, WriteBuf.Length());
		iNextXfer=EReadXfer;
		}
	else
		{
		ReadBuf.SetLength(iBufSz);
		iPort->Read(iStatus, ReadBuf, ReadBuf.Length());
//		iConsole->Printf(_L("Starting with read.%d....\r\n"),ReadBuf.Length());
		iNextXfer=EWriteXfer;
		}
	SetActive();
	}

void CActiveRW::Stop()
	{
	iConsole->Printf(_L("Stopping.....\r\n"));
	iConsole->Printf(_L("Unrecovered %d characters\r\n"),iPort->QueryReceiveBuffer());
	Cancel();
	}

void CActiveRW::DoCancel()
	{
	iPort->WriteCancel();
	iPort->ReadCancel();
	}

#pragma warning (default:4710)
