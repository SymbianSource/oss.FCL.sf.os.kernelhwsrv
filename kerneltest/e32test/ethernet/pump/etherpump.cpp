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
// e32test\ethernet\etherpump.cpp
// Abbreviations - PSP (Professional Symbian Programming)
// 
//

#define __USE_LDDPDD__
#define __USE_TIMER__

#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32svr.h>

#include <d32ethernet.h>

#include "activeio.h"

#define LDD_NAME _L("Enet")
#if (!defined __WINS__)
#define PDD_NAME _L("Ethernet")
#else

//#define PDD_NAME _L("EthernetWins")
#define PDD_NAME _L("Ethernet")
#endif


// changed from const to static
//const TUint8 DestMacAddr[] = {0x00,0x50,0xDA,0xE9,0x69,0xCA};

// MAC address with second bit 1
static TUint8 DestMacAddr[] = {0x02,0xB0,0xD0,0x64,0x98,0x02};


// for random functions
#include <e32math.h>


void StripeMem(TUint8 *aBuf, TInt aStartPos, TInt anEndPos, TUint aStartChar, TUint anEndChar, TInt aOffset)
//
// Mark a buffer with repeating byte pattern
//
    {
    TUint character=aStartChar+(aOffset%((anEndChar+1)-aStartChar));

    for (TInt i=aStartPos;i<anEndPos;i++)
	{
	aBuf[i]=(TText8)character;
	if(++character>anEndChar)
	    character=aStartChar;
	}
    }

void StripeDes(TDes8 &aBuf, TInt aStartPos, TInt anEndPos, TUint aStartChar, TUint anEndChar, TInt aOffset)
    {
    StripeMem((TUint8 *)aBuf.Ptr(), aStartPos, anEndPos, aStartChar, anEndChar, aOffset);
    }

TBool CheckMem(TUint8 *aBuf, TInt aStartPos, TInt anEndPos, TUint aStartChar, TUint anEndChar, TInt aOffset)
//
// Mark a buffer with repeating byte pattern
//
    {
    TUint character=aStartChar+(aOffset%((anEndChar+1)-aStartChar));

    for (TInt i=aStartPos;i<anEndPos;i++)
	{
	if (aBuf[i]!=(TText8)character)
	    return EFalse;
	if(++character>anEndChar)
	    character=aStartChar;
	}
    return ETrue;
    }

TBool CheckDes(TDes8 &aBuf, TInt aStartPos, TInt anEndPos, TUint aStartChar, TUint anEndChar, TInt aOffset)
    {
    return CheckMem((TUint8 *)aBuf.Ptr(), aStartPos, anEndPos, aStartChar, anEndChar, aOffset);
    }


void StripeMem32(TUint *aBuf, TInt aStartPos, TInt aEndPos)
//
// Mark a buffer with repeating byte pattern
//
    {
    aStartPos >>= 2;
    aEndPos >>= 2;
	
    for (TInt i=aStartPos;i<aEndPos;i++)
	{
	aBuf[i]=i<<2;
	}
    }

void StripeDes32(TDes8 &aBuf, TInt aStartPos, TInt anEndPos)
    {
    StripeMem32((TUint*)aBuf.Ptr(), aStartPos, anEndPos);
    }

// Standard Epoc32 Library Console class
static CConsoleBase* console;


// xxxLC means method can Leave and has pushed something on the Cleanup stack
// Caller is responsible for popping it.
CDemoControl* CDemoControl::NewLC()
    {
    CDemoControl* self = new (ELeave) CDemoControl(EPriorityNormal);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }
TInt CDemoControl::Callback(TAny* aControl)
// Callback function for timer expiry
// Just pump another packet at the server
// It's a static so call a class member to access private data
    {
    CIOBuffer* buf = ((CDemoControl*)aControl)->CreateSendPacketL();

    ((CDemoControl*)aControl)->iWriteQueue.AddLast(*buf);
    ((CDemoControl*)aControl)->iWriter->WriteL(buf->Ptr());

    return KErrNone;
    }


void CDemoControl::ConstructL()
// Second Phase construction
    {
    // Add us to the Active Scheduler for the thread
    CActiveScheduler::Add(this);
    // Create the Read and Write Active Objects
    // The 'this' pointer is the MxxxNotify callback interface that CDemoControl is derived from
    // Pass a reference to the Server session so they can make read and write requests

    iWriter = CDemoWriter::NewL(*this,iCard);
    iReader = CDemoReader::NewL(*this,iCard);

    User::LoadPhysicalDevice(PDD_NAME);
    User::LoadLogicalDevice(LDD_NAME);

#if (defined __USE_TIMER__)
    iIfState = EIdle;
    iTimer = CPeriodic::NewL(EPriorityNormal);
#endif

    iWriteQueue.SetOffset(CIOBuffer::LinkOffset());

    HelpText();

    }

void CDemoControl::EmptyWriteQueue()
    {
    TSglQueIter<CIOBuffer> iter(iWriteQueue);
    CIOBuffer* buf;
    while (buf = iter++, buf!=NULL)
	{
	iWriteQueue.Remove(*buf);
	delete buf;
	}
    }

CDemoControl::~CDemoControl()
    {
    // Cancel this classes read request to the Console
    Cancel();
    if(iIfState != EIdle)
	{
	EmptyWriteQueue();
	StopCard();
	}

    User::FreeLogicalDevice(LDD_NAME);
    User::FreePhysicalDevice(PDD_NAME);


#if (defined __USE_TIMER__)
    iTimer->Cancel();
    delete iTimer;
#endif

    delete iWriter;
    delete iReader;
    }

void CDemoControl::RequestCharacter()
// Request a character from the CConsoleBase class and set us to Active
    {
    // Read() will result in our iStatus being set to KRequestPending 0x80000001
    console->Read(iStatus);
    // SetActive sets our iActive to ETrue
    SetActive();
    }

void CDemoControl::RunL()
// Mandatory override of pure virtual called from active scheduler Start()
// Key method called when the Active Scheduler semaphore is
// signalled and the iStatus has been completed for this Active Object
    {
    ProcessKeyPress(TChar(console->KeyCode()));
    RequestCharacter();
    // We now return control to the scheduler loop
    }

void CDemoControl::DoCancel()
// Mandatory override of pure virtual, called from Cancel()
    {
    // Cancels an outstanding request to the console
    console->ReadCancel();
    }

static const TUint KEscChar = 0x1b;
void CDemoControl::ProcessKeyPress(TChar aChar)
// Process commands from the console
// Executes in the context of the class RunL()
    {
    TInt err = KErrNone;
    if(aChar == KEscChar)
	{
	// Modifies loop control flag value so the scheduler loop exits
	CActiveScheduler::Stop();
	}
    else if(aChar == 'h' || aChar == 'H')
	{
	HelpText();
	}
    else
	{
	// Add Command Handler Methods here
	switch(aChar)
	    {
	    case 'p'	:
	    case 'P'	:
		TRAP(err,PumpL());
		break;
	    case 'e'	:
	    case 'E'	:
		TRAP(err,EchoL());
		break;
	    case 'r'	:
	    case 'R'	:
		TRAP(err,ReadL());
		break;
	    case 's'	:
	    case 'S'	:
		TRAP(err,StopL());
		break;
		case 'c'	:
		case 'C'	:
		TRAP(err,SendAndCompareEchoL());
		break;
		case 'd'	:
		case 'D'	:
		TRAP(err,ReadAndSetDestMacL());
		break;
		case 'm'	:
		case 'M'	:
			ReadAndDisplaySettings();
		break;				
	    default		:
		break;
	    }

	if(err != KErrNone)
	    {
	    PrintError(aChar);
	    }
	else
	    {
	    _LIT(KMess,"State = %d\r\n");
	    console->ClearScreen();
	    console->SetPos(0,0);
	    console->Printf(KMess,iIfState);
	    }
	}
    }

void CDemoControl::HelpText() const
    {
	_LIT(KMess,"Press 'Esc' to exit \r\nPress 'H' for Help \r\nPress 'P' for Data Pump \r\nPress 'E' for Echo \r\nPress 'R' for Read \r\nPress 'C' for send and Compare echo \r\nPress 'D' to set dest MAC\r\nPress 'M' to display Settings\r\nPress 'S' to Stop");
	console->ClearScreen();
    console->SetPos(0,5);
    console->Printf(KMess);
    }

void CDemoControl::PrintError(TChar aChar)
    {
    //_LIT(KMess,"Command Error = %c State = %d\r\n1 = Idle\r\n2 = Echo\r\n3 = Read\r\n4 = Pump");
	_LIT(KMess,"Command Error = %c State = %d\r\n0 = Idle\r\n1 = Echo\r\n2 = Read\r\n3 = Pump\r\n4 = send & Compare echo");
	console->ClearScreen();
    console->SetPos(0,5);
    console->Printf(KMess,(char)aChar,iIfState);	
    }

void CDemoControl::EchoL()
    {
    if(iIfState != EIdle)
	{
	User::Leave(KErrInUse);
	}
    StartCardL();

    iReadBuffer.SetMax();
    iReadBuffer.FillZ();
    iReader->ReadL(iReadBuffer);

    iIfState = EEcho;
    }

void CDemoControl::PumpL()
    {
    console->ClearScreen();

    if(iIfState != EIdle)
	{
	User::Leave(KErrInUse);
	}
#if (defined __USE_TIMER__)
    iTimer->Start(0,1,TCallBack(Callback,this));
#endif
    StartCardL();
	
    iReadBuffer.SetMax();
    iReadBuffer.FillZ();
    iReader->ReadL(iReadBuffer);

#if (!defined __USE_TIMER__)
    CIOBuffer* buf = CreateSendPacketL();

    iWriteQueue.AddLast(*buf);
    iWriter->WriteL(buf->Ptr());
#endif

    iIfState = EPump;
    }

void CDemoControl::ReadAndDisplaySettings()
//
// Read and display the current config
//
    {
    TBuf8<32> config;

    User::LeaveIfError(iCard.Open(0));
    User::After(2000);

    // MAC Address starts at the 4th byte
    config.SetMax();
    iCard.Config(config);


    console->Printf(_L("\n\nEthernet Speed :"));
    switch (config[0])
	{
	case KEthSpeedUnknown:
	    console->Printf(_L(" Unknown\n"));
	    break;
	case KEthSpeedAuto:
	    console->Printf(_L(" Auto\n"));
	    break;
	case KEthSpeed10BaseT:
	    console->Printf(_L(" 10 MBit\n"));
	    break;
	case KEthSpeed100BaseTX:
	    console->Printf(_L(" 100 MBit\n"));
	    break;
	default:
	    console->Printf(_L(" ERROR\n"));
	}

    console->Printf(_L("Duplex Setting :"));
    switch (config[1])
	{
	case KEthDuplexUnknown:
	    console->Printf(_L(" Unknown\n"));
	    break;
	case KEthDuplexAuto:
	    console->Printf(_L(" Auto\n"));
	    break;
	case KEthDuplexFull:
	    console->Printf(_L(" Full\n"));
	    break;
	case KEthDuplexHalf:
	    console->Printf(_L(" Half\n"));
	    break;
	default:
	    console->Printf(_L(" ERROR\n"));
	}

    console->Printf(_L("MAC :"));
    console->Printf(_L(" %2x:%2x:%2x:%2x:%2x:%2x\n\n"),
		 config[2], config[3],
		 config[4], config[5],
		 config[6], config[7]);

	console->Printf(_L("\nPress any key to continue..\n") );

	console->Getch();

	iCard.Close();
    }


CIOBuffer* CDemoControl::CreateSendPacketL()
    {
    CIOBuffer* buf = CIOBuffer::NewL(1500);
    // Copy in the Destination mac address
    buf->Ptr().SetLength(6);
    buf->Ptr().Copy(DestMacAddr,6);

    // Copy in the source mac address read from the driver
    //buf->Ptr().Append(&iConfig[3],6);
	buf->Ptr().Append(&iConfig[2],6);
	
    // EtherII framing
    buf->Ptr().Append(0x08);
    buf->Ptr().Append(0x06);
    buf->Ptr().SetMax();
    StripeDes(buf->Ptr(), 14, buf->Ptr().Length(), '@', 'Z',0);
    return buf;
    }

void CDemoControl::ReadL()
    {
    if(iIfState != EIdle)
	{
	User::Leave(KErrInUse);
	}
	
    StartCardL();
	
    iReadBuffer.SetMax();
    iReadBuffer.FillZ();
    iReader->ReadL(iReadBuffer);

    iIfState = ERead;
    }




CIOBuffer* CDemoControl::CreateRandomPacketL(TInt aOffset)
	{
	CIOBuffer* buf = CIOBuffer::NewL(1500);
	// Copy in the Destination mac address
	buf->Ptr().SetLength(6);
	buf->Ptr().Copy(DestMacAddr,6);
#if (defined __USE_LDDPDD__)
	// Copy in the source mac address read from the driver
	//buf->Ptr().Append(&iConfig[3],6);
	buf->Ptr().Append(&iConfig[2],6);
#else
	buf->Ptr().Append(DummyMac,6);
#endif
	// EtherII framing
	buf->Ptr().Append(0x08);
	buf->Ptr().Append(0x06);
	buf->Ptr().SetMax();
	
	StripeDes(buf->Ptr(), 14, buf->Ptr().Length(), '@', 'Z',aOffset);
	return buf;
}


TInt CDemoControl::iSendAndEchoCmpCounter = 0;

void CDemoControl::CompareEcho()
{
	iSendAndEchoSame = 
		CheckDes(iReadBuffer, 14, /*iReadBuffer.Length() - 4*/ 1500 - 4, '@', 'Z', iIntRandomOffset);
										// - 4 for trailer
	console->Printf(_L("\r\nSent & Received Random Packet no: %d \r\n"), iSendAndEchoCmpCounter );
	
	if( iSendAndEchoSame )
		console->Printf( _L("Echo Same: TRUE \r\n") );
	else
		console->Printf( _L("Echo Same: FALSE \r\n") );
	

}

void CDemoControl::SendAndCompareEchoL()
{
	if(iIfState != EIdle)
		{
		User::Leave(KErrInUse);
		}

	iSendAndEchoSame = EFalse;

	StartCardL();
	
	// empty write buffer before start - nothing else should write 
	// when iIfState = ESendAndCmpEcho
	EmptyWriteQueue();
	
	iIfState = ESendAndCmpEcho;

	// time for generating seed for rand function
	TTime time;
	time.HomeTime(); 

	// change seed after 10 frames sent
	if( 0 == (iSendAndEchoCmpCounter % 10) )
	{
		iIntSeed = time.Int64();
	}

	iIntRandomOffset = Math::Rand( iIntSeed );

	CIOBuffer* buf = CreateRandomPacketL( iIntRandomOffset );
	
	iWriteQueue.AddLast(*buf);
	iWriter->WriteL(buf->Ptr());
}

void CDemoControl::HandleWriteCompleteSndCmpEchoModeL()
{
	
	CIOBuffer* buf = iWriteQueue.First();
	iWriteQueue.Remove(*buf);
	delete buf;

	iSendAndEchoCmpCounter = ++iSendAndEchoCmpCounter;
		
	// empty read buffer
	iReadBuffer.SetMax();
	iReadBuffer.FillZ();	
	// read echo
	iReader->ReadL(iReadBuffer);
}

void CDemoControl::HandleReadCompleteSndCmpEchoModeL()
{
	CompareEcho();

	// empty read buffer
	iReadBuffer.SetMax();
    iReadBuffer.FillZ();

//	iIfState = EIdle;
}

void CDemoControl::ReadAndSetDestMacL()
{
	
	TUint8 upper=0;
	TInt i =0;
	//TInt consPos = 0;
	TChar c;
	TInt pos; 
	TUint8 value;

	TBuf<20> validChars(_L("0123456789abcdef"));

	TUint8 newDestMacAddr[] = {0x00,0x00,0x00,0x00,0x00,0x00};

	_LIT(KMess,"Type new dest MAC (12 hexagonal digits):\r\n");
	console->ClearScreen();
	console->SetPos(0,0);
	console->Printf(KMess,iIfState);

	for(i = 0; i < 12; i++)
	{
		c = console->Getch();
		c.LowerCase();
		if((pos = validChars.Locate(c))==KErrNotFound)
		    {
		    //pos = upper;
			User::Leave(KErrNotFound); 
		    //break;
		    }
		console->SetPos(i, 1);
		console->Printf(_L("%c"), (char)c);
		if(i%2)
		{
			upper = newDestMacAddr[i / 2];
			value = (TUint8)pos;
			//value = (TUint8)((upper<<4) | value);
			newDestMacAddr[i / 2] = (TUint8)((upper<<4) | value);
		}
		else
			newDestMacAddr[i / 2] = (TUint8)pos;
		
	}

	for(i = 0; i < 6; i++)
		DestMacAddr[i] = newDestMacAddr[i];

	console->Printf(_L("\nSetting MAC to %2x:%2x:%2x:%2x:%2x:%2x\n"),
			 DestMacAddr[0], DestMacAddr[1], DestMacAddr[2],
			 DestMacAddr[3], DestMacAddr[4], DestMacAddr[5]);

	console->Printf(_L("\nPress any key to continue..\n") );

	console->Getch();

   return; 
}

//-jk

void CDemoControl::StopL()
    {
    if(iIfState == EIdle)
		{
		User::Leave(KErrInUse);
		}

    EmptyWriteQueue();
#if (defined __USE_TIMER__)
    if(iIfState == EPump)
	{
		iTimer->Cancel();
		_LIT(KMess,"\r\nPackets Pumped = %d\r\n");
		console->Printf(KMess,iPacketsWritten);
		console->Printf(_L("\r\nPress any key to continue..\r\n") );
		console->Getch();
	}
#endif
    StopCard();

    iIfState = EIdle;
    }

void CDemoControl::StartCardL()
    {
//	User::LeaveIfError(iCard.Open(iCard.VersionRequired(),0,NULL));
    User::LeaveIfError(iCard.Open(0));
    User::After(2000000);
//	TBuf8<8> ioctlBuf;
//	ioctlBuf.SetLength(1);
//	ioctlBuf[0] = KIoControlGetStatus;
//	TRequestStatus status;
//	iCard.IOControl(iStatus,ioctlBuf);
//	User::WaitForRequest(status);
//	if(ioctlBuf[0] != KEventPCCardReady)
//		{
//		iCard.Close();
//		User::Leave(KErrNotReady);
//		}
    // MAC Address starts at the 2nd byte
    iConfig.SetMax();
    iCard.Config(iConfig);

    iPacketsRead = 0;
    iPacketsWritten = 0;
    console->ClearScreen();
    }

void CDemoControl::StopCard()
    {
	
    iCard.ReadCancel();
    iCard.WriteCancel();
    iWriter->Cancel();
    iReader->Cancel();
	
    iCard.Close();
		
    }

void CDemoControl::ReadCompleteL(const TInt aStatus)
// Read completed by the server
    {
    iPacketsRead++;
    console->SetPos(0,1);
    _LIT(KMess,"Read  Complete Status = %d Packets Read    = %d\r\n");
    console->Printf(KMess,aStatus,iPacketsRead);
	RDebug::Print(KMess,aStatus,iPacketsRead);
    // Validate the received buffer with what we sent

    switch(iIfState)
	{
	case EPump:
	    HandleReadCompletePumpModeL();
	    break;

	case EEcho:
	    HandleReadCompleteEchoModeL();
	    break;
		
	case ERead:
	    HandleReadCompleteReadModeL();
	    break;

	case ESendAndCmpEcho:
		HandleReadCompleteSndCmpEchoModeL();
		break;
		
	default:
	    break;
	}
    iReadBuffer.SetMax();
    iReadBuffer.FillZ();
    iReader->ReadL(iReadBuffer);
    }

void CDemoControl::WriteCompleteL(const TInt aStatus)
// Write completed by the server
    {
    iPacketsWritten++;
    console->SetPos(0,0);
    _LIT(KMess,"Write Complete Status = %d Packets Written = %d\r\n");
    console->Printf(KMess,aStatus,iPacketsWritten);

    switch(iIfState)
	{
	case EPump:
	    HandleWriteCompletePumpModeL();
	    break;

	case EEcho:
	    HandleWriteCompleteEchoModeL();
	    break;

	case ESendAndCmpEcho:
			HandleWriteCompleteSndCmpEchoModeL();
			break;
	
	default:
	    break;
	}
    }

void CDemoControl::HandleWriteCompleteEchoModeL()
    {
    CIOBuffer* buf = iWriteQueue.First();
    iWriteQueue.Remove(*buf);
    delete buf;
    if(!iWriteQueue.IsEmpty())
	{
	buf = iWriteQueue.First();
	iWriter->WriteL(buf->Ptr());
	}
    }

void CDemoControl::HandleReadCompleteEchoModeL()
    // In echo mode we send out what we receive and there could potentialy be a write
    // outstanding.
    // Get a new CIOBuffer copy the read data to the new write buffer
    // Queue it but only WriteL() it if the queue was empty
    {
    TBool sendNow = EFalse;
    (iWriteQueue.IsEmpty()) ? (sendNow = ETrue) : (sendNow = EFalse);
    // Add it to the queue
    CIOBuffer* buf = CIOBuffer::NewL(iReadBuffer.Length());
	
    buf->Ptr() = iReadBuffer;

    // Flip Mac Addresses in buf
    FlipMacAddresses(buf->Ptr());

    iWriteQueue.AddLast(*buf);
    if(sendNow)
	{
	iWriter->WriteL(buf->Ptr());
	}
    }

void CDemoControl::FlipMacAddresses(TDes8& aBuf)
    {
    TUint32 length = aBuf.Length();
    aBuf.SetLength(6);
    TBuf8<6> dest(aBuf);
    aBuf.SetLength(12);
    aBuf.Copy(&aBuf[6],6);
    aBuf.SetLength(6);
    aBuf.Append(dest);
    aBuf.SetLength(length);
    }

void CDemoControl::HandleWriteCompletePumpModeL()
    // In pump mode we never need to queue so just reuse the last buffer
    {
#if (defined __USE_TIMER__)
    CIOBuffer* buf = iWriteQueue.First();
    iWriteQueue.Remove(*buf);
    delete buf;
#else
    CIOBuffer* buf = iWriteQueue.First();
    iWriter->WriteL(buf->Ptr());
#endif
    }

void CDemoControl::HandleReadCompletePumpModeL()
    {
    }

void CDemoControl::HandleReadCompleteReadModeL()
    {
    }

//////////////

CDemoWriter* CDemoWriter::NewL(MWriterNotify& aNotify,RBusDevEthernet& aCard)
// Standard CBase derived creation of the Writer object
    {
    CDemoWriter* self = new (ELeave) CDemoWriter(EPriorityNormal);
    CleanupStack::PushL(self);
    self->ConstructL(aNotify,aCard);
    CleanupStack::Pop();
    return self;
    }


void CDemoWriter::WriteL(const TDesC8& aBuffer)
// Write data to the server
    {
    // Sanity check on the state of the active object
    if(IsActive())
	{
#if (defined __USE_TIMER__)
	return;
#else
	User::Leave(KErrNotReady);
#endif
	}
    RDebug::Print(_L("About to write\n"));
    iCard->Write(iStatus,aBuffer);

    SetActive();
    }

CDemoWriter::~CDemoWriter()
    {
    // Just in case, does not hurt to call if object is not active
    Cancel();
    }


void CDemoWriter::ConstructL(MWriterNotify& aNotify,RBusDevEthernet& aCard)
// Second phase construction. Does not actually leave 
    {
    CActiveScheduler::Add(this);
    iNotify = &aNotify;
    iCard = &aCard;
    }


void CDemoWriter::RunL()
// Just call back into the parent to notify Write completion
    {
    // Pass the status
    iNotify->WriteCompleteL(iStatus.Int());
    }

void CDemoWriter::DoCancel()
// Called by the CActive base class Cancel()
// Only called if our TRequestStatus is still active 
    {
    }

///////


CDemoReader* CDemoReader::NewL(MReaderNotify& aNotify,RBusDevEthernet& aCard)
// Standard CBase derived creation of the Reader object
    {
    CDemoReader* self = new (ELeave) CDemoReader(EPriorityNormal+1);
    CleanupStack::PushL(self);
    self->ConstructL(aNotify,aCard);
    CleanupStack::Pop();
    return self;
    }


void CDemoReader::ReadL(TDes8& aBuffer)
    {
    // Sanity Check
    if(IsActive())
	{
	User::Leave(KErrNotReady);
	}
    RDebug::Print(_L("About to read\n"));
    iCard->Read(iStatus,aBuffer);

    SetActive();
    }

CDemoReader::~CDemoReader()
    {
    // Just in case, does not hurt to call if object is not active
    Cancel();
    }


void CDemoReader::ConstructL(MReaderNotify& aNotify,RBusDevEthernet& aCard)
// Second phase construction. Does not actually leave 
    {
    CActiveScheduler::Add(this);
    iNotify = &aNotify;
    iCard = &aCard;
    }


void CDemoReader::RunL()
// Just call back into the parent to notify read completion
    {
    // Pass the status
    iNotify->ReadCompleteL(iStatus.Int());
    }

void CDemoReader::DoCancel()
// Called by the CActive base class Cancel()
// Only called if our TRequestStatus is still active 
    {
    }

///////

static void DriveEngineL()
    {
    // Create an Active Scheduler for the thread
    // Only one Active Scheduler per thread
    CActiveScheduler* myActiveScheduler = new(ELeave) CActiveScheduler;
    CleanupStack::PushL(myActiveScheduler);
    // Install the Active Scheduler
    CActiveScheduler::Install(myActiveScheduler);
    // Create of program control class derived from CActive
    // The ConstructL() of CDemoControl adds itself to the Active Scheduler
    RDebug::Print(_L("New demo Cntrol\n"));
    CDemoControl* demo = CDemoControl::NewLC();
    // Request a character from the the console to kick the
    // Active scheduler into life. If this is not done then we will block on the
    // Scheduler loop semaphore forever.
    RDebug::Print(_L("demo Control request char\n"));
    demo->RequestCharacter();
    // Active scheduler now enters its control loop
    // We can exit this loop and hence the program by calling CActiveScheduler::Stop()
    // from a RunL().
    // IMPORTANT :-
    // From now on all this thread's processing takes place from the RunL()'s of
    // the Active objects that have been added to the Active Scheduler
    RDebug::Print(_L("Start scheduler\n"));
    myActiveScheduler->Start();
    // Remove and delete demo and myActiveScheduler
    CleanupStack::PopAndDestroy(2);	
    }

static void MainL()
    {
    // String Literal MACRO initialises a Descriptor
    //_LIT(KTitle,"EtherPump");
    //console=Console::NewL(KTitle,TSize(KDefaultConsWidth,KDefaultConsHeight));
    console=Console::NewL(_L("EtherPump"),TSize(KConsFullScreen,KConsFullScreen));
    RDebug::Print(_L("Console created\n"));
    CleanupStack::PushL(console);
    RDebug::Print(_L("and put on cu stack\n"));
    // TRAP
    TRAPD(err,DriveEngineL());
    if(err != KErrNone)
	{
	_LIT(KErrText,"Function Leave Code = %d\r\n");
	console->Printf(KErrText,err);
	}

    _LIT(KAnyKey,"Hit Any Key to Exit");
    console->ClearScreen();
    console->Printf(KAnyKey);
    console->Getch();
    CleanupStack::PopAndDestroy(1);
    }

// Entry point for all Epoc32 executables
// See PSP Chapter 2 Getting Started
GLDEF_C TInt E32Main()
    {
    // Heap balance checking
    // See PSP Chapter 6 Error Handling
    RDebug::Print(_L("create cu stack\n"));
    __UHEAP_MARK;
    CTrapCleanup* cleanup = CTrapCleanup::New();
    if(cleanup == NULL)
	{
	return KErrNoMemory;
	}
    RDebug::Print(_L("Run mainL\n"));
    TRAPD(err,MainL());
    _LIT(KPanic,"Etherpump");
    __ASSERT_ALWAYS(!err, User::Panic(KPanic,err));
    delete cleanup;
    __UHEAP_MARKEND;
    return KErrNone;
    }
///////////////////////

// Generic Buffer class
// Currently used for transmit buffers
CIOBuffer::CIOBuffer() : iBufPtr(NULL,0)
    {
    }

CIOBuffer::~CIOBuffer()
// Free the HBuf if there is one
    {
    FreeData();
    }

TPtr8& CIOBuffer::Ptr()
    {
    return iBufPtr;
    }

CIOBuffer* CIOBuffer::NewL(const TInt aSize)
// Creation where we new the HBuf
    {
    CIOBuffer * self = new (ELeave) CIOBuffer;
    CleanupStack::PushL(self);
    self->ConstructL(aSize);
    CleanupStack::Pop();
    return self;
    }

void CIOBuffer::ConstructL(const TInt aSize)
// Construction where we new the HBuf
    {
    iBuf = HBufC8::NewL(aSize);
    TPtr8 temp=iBuf->Des();
    iBufPtr.Set(temp);
    }

CIOBuffer* CIOBuffer::NewL(HBufC8* aBuf)
// HBuf provided
    {
    CIOBuffer * self = new (ELeave) CIOBuffer;
    CleanupStack::PushL(self);
    self->ConstructL(aBuf);
    CleanupStack::Pop();
    return self;
    }

void CIOBuffer::ConstructL(HBufC8* aBuffer)
    {
    Assign(aBuffer);
    }

TInt CIOBuffer::LinkOffset()
    {
    return _FOFF(CIOBuffer,iLink);	
    }

void CIOBuffer::Assign(HBufC8* aBuffer)
    {
    iBuf = aBuffer;
    if(aBuffer)
	{
	TPtr8 temp=iBuf->Des();
	iBufPtr.Set(temp);
	}
    }

HBufC8*	CIOBuffer::Data() const
    {
    return iBuf;
    }

void CIOBuffer::FreeData()
    {
    if(iBuf)
	{
	delete iBuf;
	iBuf = NULL;
	}
    }
