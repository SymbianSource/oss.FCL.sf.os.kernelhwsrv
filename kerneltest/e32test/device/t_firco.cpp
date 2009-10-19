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
// e32test\device\t_firco.cpp
// 
//

#include "t_fir.h"

#if defined(__VC32__) && _MSC_VER==1100
// Disable MSVC++ 5.0 aggressive warnings about non-expansion of inline functions. 
#pragma warning(disable : 4710)	// function '...' not expanded
#endif

#ifdef __WINS__
	#include <es_sock.h>
	RSocketServ ss;
#endif

TBuf8<2060> WriteBuf;
TBuf8<2060> ReadBuf;
TInt iTimeDelay=1000000;
TInt iBufSz=2000;

CActiveConsole::CActiveConsole(CConsoleBase* aConsole) 
	: CActive(EPriorityNormal)
	{
	iConsole=aConsole;
	iInit1  =EFalse;
	iInit2  =EFalse;
	iInit3	=EFalse;
	}

CActiveConsole* CActiveConsole::NewLC(CConsoleBase* aConsole)
	{
	CActiveConsole* self = new (ELeave) CActiveConsole(aConsole);
	self->ConstructL();
	return self;
	}

void CActiveConsole::ConstructL ()
	{ 
	TFirCaps aCapsBuf;
	TFirCapsV01& aCaps=aCapsBuf();
	TFirConfig aConfigBuf;
	TFirConfigV01& aConfig=aConfigBuf();

	iConsole->Printf(_L("\r\n"));
	CActiveScheduler::Add(this);			// Add to active scheduler

	// Load Driver
	TInt ret=User::LoadPhysicalDevice(_L("Difir"));
	if (ret!=KErrNone)
		iConsole->Printf(_L("Error %d on loading Fir PDD\r\n"),ret);
	else
		iConsole->Printf(_L("Successfully loaded Fir PDD\r\n"));

	ret=User::LoadLogicalDevice(_L("Efir"));
	if (ret!=KErrNone)
		iConsole->Printf(_L("Error %d on loading Fir LDD\r\n"),ret);
	else
		iConsole->Printf(_L("Successfully loaded Fir LDD\r\n"));

	SetUpBuffers();

	ret=iPort.Open(0);
	if (ret!=KErrNone)
		iConsole->Printf(_L("Error %d on opening Fastir port\r\n"),ret);
	else
		iConsole->Printf(_L("Successfully opened Fastir port\r\n"));


	ret=iPort.Caps(aCapsBuf);
	if (ret!=KErrNone)
		iConsole->Printf(_L("Error %d on getting caps\r\n"),ret);
	else
		iConsole->Printf(_L("Fir Caps: %d\r\n"),aCaps.iRate);

/*	ret=iPort.Config(aConfigBuf);
	if (ret!=KErrNone)
		iConsole->Printf(_L("Error %d getting config\r\n"),ret);
	else
		{
		if(aConfig.iRate==EBps4000000)
			iConsole->Printf(_L("Fir config is 4Mbps\r\n"));
		}

	aConfig.iRate=EBps4000000;
	ret=iPort.SetConfig(aConfigBuf);
	iConsole->Printf(_L("Error %d on SetConfig\r\n"),ret);*/

	iWriter=CActiveWriter::NewL(iConsole,&iPort);
	if(iWriter)
		iConsole->Printf(_L("Have created writer\r\n"));
	else
		iConsole->Printf(_L("Failed to create writer\r\n"));

	iReader=CActiveReader::NewL(iConsole,&iPort);
	if(iReader)
		iConsole->Printf(_L("Have created reader\r\n"));
	else
		iConsole->Printf(_L("Failed to create reader\r\n"));
	}

CActiveConsole::~CActiveConsole()
	{
	// Make sure we're cancelled
	Cancel();

	if(iWriter)
		delete iWriter;
	if(iReader)
		delete iReader;

	iPort.Close();
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
	if(!iInit3)
		{
		Options3();
		return;
		}
	  // A request is issued to the CConsoleBase to accept a
	  // character from the keyboard.
	iConsole->Printf(_L("*********************************\r\n"));
	iConsole->Printf(_L("press Escape to quit\r\n"));
	iConsole->Printf(_L("press '1'/'2' to start/stop reader\r\n"));
	iConsole->Printf(_L("press '3'/'4' to start/stop writer\r\n"));
	iConsole->Printf(_L("press '8' to show FIR regs\r\n"));
	iConsole->Printf(_L("press '9' to show Dma reader regs\r\n"));
	iConsole->Printf(_L("press '0' to show Dma writer regs\r\n"));
	iConsole->Printf(_L("press 'a' to show TxBuf info\r\n"));
	iConsole->Printf(_L("press 'b' to show RxBuf info\r\n"));
	iConsole->Printf(_L("press 'c' to show Chunk info\r\n"));
	iConsole->Printf(_L("press 'd' to show misc info\r\n"));
	iConsole->Read(iStatus); 
	SetActive();
	}

void CActiveConsole::Options1()
	{
	iConsole->Printf(_L("*****Choose Delay*****\r\n"));
	iConsole->Printf(_L("press '1'  576000 baud\r\n"));
	iConsole->Printf(_L("press '2' 1152000 baud\r\n"));
	iConsole->Printf(_L("press '3' 4000000 baud\r\n"));
	iConsole->Read(iStatus); 
	SetActive();	
	}

void CActiveConsole::Options2()
	{
	iConsole->Printf(_L("*****Choose Delay*****\r\n"));
	iConsole->Printf(_L("press '1' 1.00 sec delay\r\n"));
	iConsole->Printf(_L("press '2' 0.10 sec delay\r\n"));
	iConsole->Printf(_L("press '3' 0.01 sec delay\r\n"));
	iConsole->Printf(_L("press '4' 0.00 sec delay\r\n"));
	iConsole->Read(iStatus); 
	SetActive();	
	}

void CActiveConsole::Options3()
	{
	iConsole->Printf(_L("****Choose Buf Sz*****\r\n"));
	iConsole->Printf(_L("press '1' 1    byte \r\n"));
	iConsole->Printf(_L("press '2' 4    bytes\r\n"));
	iConsole->Printf(_L("press '3' 16   bytes\r\n"));
	iConsole->Printf(_L("press '4' 64   bytes\r\n"));
	iConsole->Printf(_L("press '5' 128  bytes\r\n"));
	iConsole->Printf(_L("press '6' 2000 bytes\r\n"));
	iConsole->Printf(_L("press '7' 2051 bytes\r\n"));
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
		case '1':
			iBaudRate=EBps576000;
			break;
		case '2':
			iBaudRate=EBps1152000;
			break;
		case '3':
			iBaudRate=EBps4000000;
			break;
		default:
			iBaudRate=EBps4000000;
			break;
			}
		iConsole->Printf(_L("Baud rate: %d\r\n"),iBaudRate);
		iInit1=ETrue;
		TFirConfig aConfigBuf;
		TFirConfigV01& aConfig=aConfigBuf();
		aConfig.iRate=iBaudRate;
		TInt ret=iPort.SetConfig(aConfigBuf);
		iConsole->Printf(_L("Error %d on SetConfig\r\n"),ret);
		RequestCharacter();
		return;
		}

	if(!iInit2)
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
		iInit2=ETrue;
		RequestCharacter();
		return;
		}
	if(!iInit3)
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
			iBufSz=16;
			break;
		case '4':
			iBufSz=64;
			break;
		case '5':
			iBufSz=128;
			break;
		case '6':
			iBufSz=2000;
			break;
		case '7':
			iBufSz=2052;
			break;
		default:
			iBufSz=2000;
			break;
			}
		iConsole->Printf(_L("Buffer size: %d\r\n"),iBufSz);
		iInit3=ETrue;
		RequestCharacter();
		return;
		}

	switch (aChar)
		{
	case '1'://start reader
		iReader->Start();
		break;
	case '2'://stop reader
		iReader->Stop();
		break;
	case '3'://start writer
		iWriter->Start();
		break;
	case '4'://stop writer
		iWriter->Stop();
		break;
	case '8'://get fir regs
		GetFirRegs();
		break;
	case '9'://get dma reader regs
		GetDmaReaderRegs();
		break;
	case '0'://get dma writer regs
		GetDmaWriterRegs();
		break;
	case 'a'://get TxBuf info
		GetWriteBufInfo();
		break;
	case 'b'://get RxBuf info
		GetReadBufInfo();
		break;
	case 'c'://get RxBuf info
		GetChunkInfo();
		break;
	case 'd':
		GetMiscInfo();
		break;
	default:
		iConsole->Printf(_L("\r\nUnknown Command\r\n\r\n"));
		break;
		}
	RequestCharacter ();
	return;
	}

void CActiveConsole::GetFirRegs()
	{
/*	TInt r=0;
	TDebugFirRegs FirRegs;
	r=iPort.GetFirRegs(FirRegs);
	iConsole->Printf(_L("RxFrameStatus  : 0x%x\r\n"),FirRegs.RxFrameStatus);
	iConsole->Printf(_L("RxBufferEmpty  : 0x%x\r\n"),FirRegs.RxBufferEmpty);
	iConsole->Printf(_L("RxError        : 0x%x\r\n"),FirRegs.RxError);
	iConsole->Printf(_L("RxOverrun      : 0x%x\r\n"),FirRegs.RxOverrun);
	iConsole->Printf(_L("CrcError       : 0x%x\r\n"),FirRegs.CrcError);
	iConsole->Printf(_L("TxFrameStatus  : 0x%x\r\n"),FirRegs.TxFrameStatus);
	iConsole->Printf(_L("TxBufferEmpty  : 0x%x\r\n"),FirRegs.TxBufferEmpty);
	iConsole->Printf(_L("TxBufferSz     : 0x%x\r\n"),FirRegs.TxBufferSz);
	iConsole->Printf(_L("TxOverrun      : 0x%x\r\n"),FirRegs.TxOverrun);*/
	}

void CActiveConsole::GetDmaReaderRegs()
	{
/*	TInt r=0;
	TDebugDmaChannelRegs DmaRxRegs;
	r=iPort.GetDmaRxRegs(DmaRxRegs);
	iConsole->Printf(_L("Rx Chan       : %d\n"),DmaRxRegs.DmaRxChannel);
	iConsole->Printf(_L("Rx DmaMode    : %d  "),DmaRxRegs.DmaMode);
	iConsole->Printf(_L("Rx DmaState   : %d  "),DmaRxRegs.DmaState);
	iConsole->Printf(_L("Rx DmaBuffer  : %x\n"),DmaRxRegs.DmaBuffer);
	iConsole->Printf(_L("Rx DmGauge    : %d\n"),DmaRxRegs.DmaGauge);
	iConsole->Printf(_L("Rx DmaSrcAddr : %x  "),DmaRxRegs.DmaSrcAddr);
	iConsole->Printf(_L("Rx DmaSrcInc  : %d\n"),DmaRxRegs.DmaSrcInc);
	iConsole->Printf(_L("Rx DmaDestAddr: %x  "),DmaRxRegs.DmaDestAddr);
	iConsole->Printf(_L("Rx DmaDestInc : %d\n"),DmaRxRegs.DmaDestInc);
	iConsole->Printf(_L("Rx DmaCount   : %d\n"),DmaRxRegs.DmaCount);
	//iConsole->Printf(_L("Rx MatchClear : %x\n"),DmaRxRegs.MatchClear);
	//iConsole->Printf(_L("Rx MatchSet   : %x\n"),DmaRxRegs.MatchSet);*/
	}

void CActiveConsole::GetDmaWriterRegs()
	{
/*	TInt r=0;
	TDebugDmaChannelRegs DmaTxRegs;
	r=iPort.GetDmaTxRegs(DmaTxRegs);
	iConsole->Printf(_L("Tx Chan       : %d\n"),DmaTxRegs.DmaTxChannel);
	iConsole->Printf(_L("Tx DmaMode    : %d"),DmaTxRegs.DmaMode);
	iConsole->Printf(_L("Tx DmaState   : %d"),DmaTxRegs.DmaState);
	iConsole->Printf(_L("Tx DmaBuffer  : %x\n"),DmaTxRegs.DmaBuffer);
	iConsole->Printf(_L("Tx DmGauge    : %d\n"),DmaTxRegs.DmaGauge);
	iConsole->Printf(_L("Tx DmaSrcAddr : %x"),DmaTxRegs.DmaSrcAddr);
	iConsole->Printf(_L("Tx DmaSrcInc  : %d\n"),DmaTxRegs.DmaSrcInc);
	iConsole->Printf(_L("Tx DmaDestAddr: %x"),DmaTxRegs.DmaDestAddr);
	iConsole->Printf(_L("Tx DmaDestInc : %d\n"),DmaTxRegs.DmaDestInc);
	iConsole->Printf(_L("Tx DmaCount   : %d\n"),DmaTxRegs.DmaCount);
	//iConsole->Printf(_L("Tx MatchClear : %x\n"),DmaTxRegs.MatchClear);
	//iConsole->Printf(_L("Tx MatchSet   : %x\n"),DmaTxRegs.MatchSet);*/
	}

void CActiveConsole::GetReadBufInfo()
	{
/*	TInt r=0;
	TDebugBufInfo RxBufInfo;
	r=iPort.GetRxBufInfo(RxBufInfo);
	iConsole->Printf(_L("Rx no frames: %d\r\n"),RxBufInfo.iNoFrames);
	iConsole->Printf(_L("Rx insert pt: %d "),RxBufInfo.iInsertPt);
	iConsole->Printf(_L("Rx remove pt: %d\r\n"),RxBufInfo.iRemovePt);
	iConsole->Printf(_L("Rx head     : %d "),RxBufInfo.iHead);
	iConsole->Printf(_L("Rx tail     : %d\r\n"),RxBufInfo.iTail);
	iConsole->Printf(_L("Rx active   : %x "),RxBufInfo.iActive);
	iConsole->Printf(_L("Rx cancelled: %x\r\n"),RxBufInfo.iCancelled);
	iConsole->Printf(_L("Client read pending: %d\r\n"),RxBufInfo.iClientPending);*/
	}

void CActiveConsole::GetWriteBufInfo()
	{
/*	TInt r=0;
	TDebugBufInfo TxBufInfo;
	r=iPort.GetTxBufInfo(TxBufInfo);
	iConsole->Printf(_L("Tx no frames: %d\r\n"),TxBufInfo.iNoFrames);
	iConsole->Printf(_L("Tx insert pt: %d "),TxBufInfo.iInsertPt);
	iConsole->Printf(_L("Tx remove pt: %d\r\n"),TxBufInfo.iRemovePt);
	iConsole->Printf(_L("Tx head     : %d "),TxBufInfo.iHead);
	iConsole->Printf(_L("Tx tail     : %d\r\n"),TxBufInfo.iTail);
	iConsole->Printf(_L("Tx active   : %x "),TxBufInfo.iActive);
	iConsole->Printf(_L("Tx cancelled: %x\r\n"),TxBufInfo.iCancelled);
	iConsole->Printf(_L("Client write pending: %d\r\n"),TxBufInfo.iClientPending);*/
	}

void CActiveConsole::GetChunkInfo()
	{
/*	TInt r=0;
	TDebugDmaChunkInfo DmaChunkInfo;
	r=iPort.GetDmaChunkInfo(DmaChunkInfo);
	iConsole->Printf(_L("Write Chunk Phys Addr: 0x%x\r\n"),DmaChunkInfo.iWriteChunkPhysAddr);
	iConsole->Printf(_L("Write Chunk Lin  Addr: 0x%x\r\n"),DmaChunkInfo.iWriteChunkLinAddr);
	iConsole->Printf(_L("Read  Chunk Phys Addr: 0x%x\r\n"),DmaChunkInfo.iReadChunkPhysAddr);
	iConsole->Printf(_L("Read  Chunk Lin  Addr: 0x%x\r\n"),DmaChunkInfo.iReadChunkLinAddr);
	//iConsole->Printf(_L("No pages in read chunk  : %d\r\n"),DmaChunkInfo.iReaderNoPages);
	//iConsole->Printf(_L("no pages in write chunk : %d\r\n"),DmaChunkInfo.iWriterNoPages);*/
	}

void CActiveConsole::GetMiscInfo()
	{
/*	TInt r=0;
	TDebugInterruptInfo IntInfo;
	r=iPort.GetInterruptsInfo(IntInfo);
	iConsole->Printf(_L("NoRxDmaInts  : %d\r\n"),IntInfo.NoRxDmaInts);	
	iConsole->Printf(_L("NoTxDmaInts  : %d\r\n"),IntInfo.NoTxDmaInts);	
	iConsole->Printf(_L("NoRxBusyInts : %d\r\n"),IntInfo.NoRxBusyInts);	
	iConsole->Printf(_L("NoRxIdleInts : %d\r\n"),IntInfo.NoRxIdleInts);
	iConsole->Printf(_L("NoRxInts     : %d\r\n"),IntInfo.NoRxInts);
	iConsole->Printf(_L("NoTxIdleInts : %d\r\n"),IntInfo.NoTxIdleInts);
	iConsole->Printf(_L("NoTxInts     : %d\r\n"),IntInfo.NoTxInts);*/
	}

void CActiveConsole::SetUpBuffers()
	{
	TInt i=0;
	WriteBuf.SetLength(2060);
	ReadBuf.SetLength(2060);
	for(i=0;i<2050;i++)
		{
		//WriteBuf[i]='W';
		ReadBuf[i] ='R';
		}

	TInt j=0;
	while(j<2050-16)
		{
		WriteBuf[j  ]='0';
		WriteBuf[j+1]='1';
		WriteBuf[j+2]='2';
		WriteBuf[j+3]='3';

		WriteBuf[j+4]='4';
		WriteBuf[j+5]='5';
		WriteBuf[j+6]='6';
		WriteBuf[j+7]='7';

		WriteBuf[j+8 ]='8';
		WriteBuf[j+9 ]='9';
		WriteBuf[j+10]='A';
		WriteBuf[j+11]='B';

		WriteBuf[j+12]='C';
		WriteBuf[j+13]='D';
		WriteBuf[j+14]='E';
		WriteBuf[j+15]='F';

		j=j+16;
		}

	WriteBuf.SetLength(2000);
	}

//
// class CActiveWriter
//

CActiveWriter::CActiveWriter(CConsoleBase* aConsole,RDevFir* aPort)
	: CActive (EPriorityNormal)
	{
	iConsole   = aConsole;
	iPort=aPort;
	iLength=0;
	}

CActiveWriter* CActiveWriter::NewL(CConsoleBase* aConsole,RDevFir* aPort)
	{
	CActiveWriter* self = new (ELeave) CActiveWriter(aConsole,aPort);

	CleanupStack::PushL (self);
	self->ConstructL();
	CActiveScheduler::Add (self);
	CleanupStack::Pop ();
	return (self);
	}

void CActiveWriter::ConstructL()
	{
	}

CActiveWriter::~CActiveWriter()
	{
	Cancel();
	}

void CActiveWriter::RunL ()
	{
	if (iStatus != KErrNone)
		{
		iConsole->Printf(_L("Error %d on write completion\r\n"),iStatus.Int());
		return;
		}
	else
		iConsole->Printf(_L("W :%d "),WriteBuf.Length());

	if(iTimeDelay)
		User::After(iTimeDelay);
	iLength=(iLength+1)%10;
	WriteBuf.SetLength(iBufSz+iLength);
	iPort->Write(iStatus, WriteBuf, WriteBuf.Length());
	SetActive();
	}

void CActiveWriter::Start()
	{
	if(IsActive())
		return;
	iConsole->Printf(_L("Starting writer.....\r\n"));
	iPort->Write(iStatus, WriteBuf, WriteBuf.Length());
	SetActive();
	}

void CActiveWriter::Stop()
	{
	iConsole->Printf(_L("Stopping writer.....\r\n"));
	Cancel();
	}

void CActiveWriter::DoCancel()
	{
	iPort->WriteCancel();
	}

//
// class CActiveReader
//

CActiveReader::CActiveReader(CConsoleBase* aConsole,RDevFir* aPort)
	: CActive (EPriorityNormal)//EPriorityMore)
	{
	iConsole=aConsole;
	iPort   =aPort;
	}

CActiveReader* CActiveReader::NewL(CConsoleBase* aConsole,RDevFir* aPort)
	{
	CActiveReader* self = new (ELeave) CActiveReader(aConsole,aPort);

	CleanupStack::PushL(self);
	self->ConstructL();
	CActiveScheduler::Add (self);
	CleanupStack::Pop();
	return (self);
	}

void CActiveReader::ConstructL()
	{
	}

CActiveReader::~CActiveReader ()
	{
	Cancel();
	}

void CActiveReader::Start()
	{
	if(IsActive())
		return;
	iConsole->Printf(_L("Starting reader.....\r\n"));
	ReadBuf.SetLength(2052);
	iPort->Read(iStatus, ReadBuf, ReadBuf.Length());
	SetActive();
	}

void CActiveReader::Stop()
	{
	iConsole->Printf(_L("Stopping reader.....\r\n"));
	Cancel();
	}

void CActiveReader::RunL ()
	{
	if (iStatus != KErrNone)
		iConsole->Printf(_L("Error %d\r\n"),iStatus.Int());
	else
		{
		if(!CompareBuffers(ReadBuf.Length()))
			iConsole->Printf(_L("Buffers dont compare!\r\n"));
		iConsole->Printf(_L("R:%d\r\n"),ReadBuf.Length());
/*		TInt len=ReadBuf.Length();
		for(TInt i=0;i<10;i++)
			{
			for (TInt j=0;j<16;j++)//print 16 bytes
				{
				if ((i*16)+j<len)
					{
					TInt v=ReadBuf[(i*16)+j];
					iConsole->Printf(_L("%02x "),v);
					}
				else
					iConsole->Printf(_L("   "));
				}
			iConsole->Printf(_L("\r\n"));
			}*/
		}

	ResetReadBuffer();
	if(iTimeDelay)
		User::After(iTimeDelay);
	iPort->Read(iStatus, ReadBuf, ReadBuf.Length());
	SetActive();
	}

void CActiveReader::DoCancel()
	{
	iPort->ReadCancel();
	}

void CActiveReader::ResetReadBuffer()
	{
	TInt i=0;
	ReadBuf.SetLength(2060);
	for(i=0;i<2052;i++)
		ReadBuf[i] ='R';
	}

TBool CActiveReader::CompareBuffers(TInt aLen)
	{
	TInt i=0;
	while(i<aLen)
		{
		if(ReadBuf[i]!=WriteBuf[i])
			return EFalse;
		i++;
		}
	return ETrue;
	}
#pragma warning (default:4710)

