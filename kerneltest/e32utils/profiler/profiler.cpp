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
// e32utils\profiler\profiler.cpp
// 
//

#include <e32cons.h>
#include <f32file.h>
#include "profiler.h"
#include "sampler.h"

// The name of the output file use to save the sample data
_LIT(KFileName,"?:\\PROFILER.DAT");
const TInt KFileNameLen=15;

// The name of the DLL used as an alternative UI controller
_LIT(KProfilerKeysDll,"ProfilerKeys");

// The size of the buffers used for reading sample data and writing to file
const TInt KBufferSize = 0x800;

// The sample rate used by the profiler
const TInt KSampleRate = 1000;

const TInt KCommandMask  = 0x00ff;
const TInt KCommandNone  = 0x0010;
const TInt KCommandNoUi  = 0x0100;
const TInt KCommandXIPOnly = 0x0200;

// The controller class used to provide the Profiler functions.
// This runs as a server in the engine thread
class CPServer : public CServer2, public MProfilerController
	{
public:
	static MProfilerController* NewL(TInt aPriority, MProfilerEngine& aEngine);
private:
	CPServer(TInt aPriority, MProfilerEngine& aEngine);
	void Release();
	CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	};

// The session class used by the server controller
class CPSession : public CSession2
	{
private:
	inline const CPServer& Server() const;
	void ServiceL(const RMessage2& aMessage);
	};


// The default UI controller class which uses a Console
class CConsole : public CActive, private MProfilerController
	{
public:
	static MProfilerController* NewL(TInt aPriority, MProfilerEngine& aEngine);
private:
	CConsole(TInt aPriority, MProfilerEngine& aEngine);
	void ConstructL();
	~CConsole();
	void Release();
//
	void Help();
	void Queue();
//
	void RunL();
	void DoCancel();
private:
	CConsoleBase* iConsole;
	};


// The buffers used for transferring data from the device driver to the file
struct TBuffer
	{
	TBuffer* iNext;
	TBuf8<KBufferSize> iBuf;
	};

class CProfiler;

// The active object responsible for reading data from the device
class CReader : public CActive
	{
public:
	CReader(TInt aPriority, CProfiler& aProfiler);
	~CReader();
//
	void ConstructL();
	void Queue(TBuffer* aBuf);
private:
	void RunL();
	void DoCancel();
private:
	CProfiler& iProfiler;
	TBuffer* iBuf;
public:
	RSampler iSampler;
	};

// The active object responsible for writing data out (to file)
class CWriter : public CActive
	{
public:
	CWriter(TInt aPriority, CProfiler& aProfiler);
	~CWriter();
	void ConstructL();
	TInt Open(const TDesC& aFile);
	void Close();
	void Queue(TBuffer* aBuf);
private:
	void RunL();
	void DoCancel();
private:
	CProfiler& iProfiler;
	TBuffer* iBuf;
	RFile iFile;
	RFs iFs;
	};


// The profiler engine itself.
class CProfiler : public CBase, private MProfilerEngine
	{
	enum {EControlPriority = 10, EReaderPriority = 0, EWriterPriority = -10};
	
	/** Specifies the state of the engine*/
	enum TState 
		{
		/**
		Initial state. The file is closed. Driver is inactive
		*/
		EClosed,
		/**
		Engine enters this state on client's Start request (if -xiponly is not specified).
		Opens the file.
		Resets the driver and nonXIP code segments.
		Sends GetSegments calls to the driver until driver returns zero length reply.
		Leaves this state (goes into ERunning) when the last data (obtained by GetSegment) is
		written into the file.		 
		*/
		EGettingSegments,
		/**
		Sends async. read request to the driver. Once completed, it immediately sends another while 
		writing the collected records into the file.
		*/
		ERunning,
		/**
		Get into this state from ERunning on the client's Stop, Close or Exit request.
		Sends Drain calls to the driver until driver returns zero length reply.
		Leaves this state when all records are written into the file.
		*/
		EDraining,
		/**
		No active calls to the driver. On the client's Start request, will go back into ERunning mode.
		*/
		EStopped,
		/**
		Get into this state on client's Close or Exit request.
		Sends a single GetErrorReport request to the driver. After data has been recorded into the file,
		it closes the file and goes into EClosed state or terminates application..
		*/
		EGettingErrors
		};
public:
	static CProfiler* NewLC(TInt aCmd, TDesC* aDrive);
//
	TInt Control(Profiler::TState aCommand);
//
	void ReadComplete(TBuffer* aBuf);
	void WriteComplete(TBuffer* aBuf);
private:
	CProfiler();
	~CProfiler();
	void ConstructL(TInt aCmd, TDesC* aDrive);
	MProfilerController* CreateUiL();
//
	void Read();
	void Write();
	TBool GetSegments();
	TBool Drain();
	void GetErrors();
//
	Profiler::TState State() const;
private:
	CReader* iReader;
	CWriter* iWriter;
	MProfilerController* iServer;
	RLibrary iUiCode;
	MProfilerController* iUi;
	TState iState;
	TBool iXIPOnly;
	Profiler::TState iLastCommand;
//
	// The FIFO queue of data that has to be written
	TBuffer* iHead;
	TBuffer* iTail;
//
	// The LIFO list of free buffers 
	TBuffer* iFree;
	TDesC* iDrive;
	};


CProfiler* CProfiler::NewLC(TInt aCmd, TDesC* aDrive)
	{
	CProfiler* self = new(ELeave) CProfiler;
	CleanupStack::PushL(self);
	self->ConstructL(aCmd, aDrive);
	return self;
	}

CProfiler::CProfiler()
	{}

CProfiler::~CProfiler()
	{
	delete iReader;
	delete iWriter;
	if (iServer)
		iServer->Release();
	if (iUi)
		iUi->Release();
	iUiCode.Close();

	// discard the buffers in the free list
	TBuffer* b=iFree;
	while (b)
		{
		TBuffer* n = b->iNext;
		delete b;
		b = n;
		}

	// discard any buffers in the holding queue
	b=iHead;
	while (b)
		{
		TBuffer* n = b->iNext;
		delete b;
		b = n;
		}
	}

void CProfiler::ConstructL(TInt aCmd, TDesC* aDrive)
//
// Build the profiler engine
//
	{
	// Set drive letter of where to store profiler data
	iDrive = aDrive;
	
	// Run the engine at maximum priority to try and ensure that the sampler device
	// does not get choked and start dropping samples
	RThread me;
	me.SetPriority(EPriorityRealTime);
	User::LeaveIfError(User::RenameThread(KProfilerName));

	CActiveScheduler::Install(new(ELeave) CActiveScheduler);
	iReader = new(ELeave) CReader(EReaderPriority,*this);
	iReader->ConstructL();
	iWriter = new(ELeave) CWriter(EWriterPriority,*this);
	iWriter->ConstructL();
	iServer = CPServer::NewL(EControlPriority,*this);
	if (!(aCmd & KCommandNoUi))
		iUi = CreateUiL();

	// Start off with two buffers in the free list for sample data
	TBuffer* buf = new(ELeave) TBuffer;
	buf->iNext = 0;
	iFree = buf;
	buf = new(ELeave) TBuffer;
	buf->iNext = iFree;
	iFree = buf;
	
	// idenify the running mode
	iXIPOnly = aCmd & KCommandXIPOnly;

	// start profiling if requested
	if ((aCmd & KCommandMask) == Profiler::EStart)
		User::LeaveIfError(Control(Profiler::EStart));
			
	}

MProfilerController* CProfiler::CreateUiL()
//
// deal with the UI acquisition part of construction
// If ProfilerKeys.Dll is available, use it; otherwise create a text console
//
	{
	_LIT(KWindowServerName,"*WindowServer");
	TFindServer find(KWindowServerName);
	TFullName n;
	if (find.Next(n) == KErrNotFound)
		{
		// No UI on this device [yet]. Run without one.
		return 0;
		}

	if (iUiCode.Load(KProfilerKeysDll,TUidType(KNullUid, KUidProfilerKeys)) == KErrNone)
		{
		TProfilerControllerFactoryL factoryL = TProfilerControllerFactoryL(iUiCode.Lookup(1));
		MProfilerController* ui = NULL;
		TRAPD(error, ui = factoryL(EControlPriority, *this));
		if (error == KErrNone)
			return ui;

		// Couldn't create alternative UI, so use the console
		iUiCode.Close();
		}
	return CConsole::NewL(EControlPriority, *this);
	}

TInt CProfiler::Control(Profiler::TState aCommand)
//
// Handle a command from one of the controllers.
// This method specifies the flow of the engine state (iState attr).
// The most of transtions is not performed immediately but after all 
// current data are recorded into the file - see WriteComplete method.
//
	{
	
	DEBUG_PROFILER(RDebug::Printf("*CTRL %d",iState);)
	
	TInt r = KErrNone;
	Profiler::TState oldCommand = iLastCommand;
	
	//Record the command. In most cases, it is WriteComplete method
	//to perform state transition (based on this value)
	iLastCommand = aCommand;
	
	switch (aCommand)
		{
	case Profiler::EStart:
		switch (iState)
			{
		case EClosed:
			{
			// Set the path of the output file to include the drive letter
			// specified at the command line or the default
			TBuf<KFileNameLen> path;
			path.Copy(KFileName);
			path[0] = (*iDrive)[0];
		 	r = iWriter->Open(path);
			}
			if (KErrNone != r)	 	// Re-open the file
				return r;
			iReader->iSampler.Reset(iXIPOnly);				// Reset the sampler
			if(iXIPOnly)
				{
				iState = ERunning;
				iReader->iSampler.Start(KSampleRate);		// Start sampler
				if (!iReader->IsActive())
					Read();									// Start reading
				}
			else	
				{
				iState = EGettingSegments;
				iReader->iSampler.ResetSegments();			// Reset segments
				GetSegments();								// Start getting segments
				}
			break;
		case EStopped:
			iState = ERunning;
			iReader->iSampler.Start(KSampleRate);			// Start sampler
			if (!iReader->IsActive())
				Read();										//Start reading
			break;
		case ERunning:			//Already started. No action required.
		case EGettingSegments:	//Already started. No action required.
		case EDraining:			//Will restart after draining is completed.
		case EGettingErrors:    //Will restart after getting errors is completed;
			break;
			}
		break; //end of case Profiler::EStart
		
	case Profiler::EStop:
		switch (iState)
			{
		case EClosed:
		case EGettingErrors:
			iLastCommand = oldCommand; 		
			return KErrGeneral; 			//The command makes no sense in this state
		case ERunning:
			iReader->iSampler.Stop();		//Stop sampling.
			break;
		case EGettingSegments:	//Will do GettingSegments->Running->Stopped transitions
		case EDraining:			//Stopping already in progress
		case EStopped:			//Already stopped.
			break;
			}
		break; //end of case Profiler::EStop
		
	case Profiler::EClose:
		switch (iState)
			{
		case EStopped:
			iState = EGettingErrors;
			GetErrors();
			break;
		case ERunning:
			iReader->iSampler.Stop();
			break;
		case EClosed:   		//Already closed.
		case EGettingErrors:	//Closing in progress
		case EGettingSegments:
		case EDraining:
			break;
			}
		break; //end of case Profiler::EStop

	case Profiler::EUnload:
		switch (iState)
			{
		case EClosed:
			CActiveScheduler::Stop();	// Terminate application.
			break;
		case EStopped:
			iState = EGettingErrors;
			GetErrors();
			break;
		case ERunning:
			iReader->iSampler.Stop();
			break;
		case EDraining:
		case EGettingErrors:
		case EGettingSegments:
			break;
			}
		break;//end of case Profiler::Unload
		}

	DEBUG_PROFILER(RDebug::Printf("*CTRL end %d",iState);)
	return KErrNone;
	}

Profiler::TState CProfiler::State() const
//
// Report the current state of the engine
//
	{
	switch (iState)
		{
	case EGettingErrors:
	case EStopped:
		return Profiler::EStop;
	case EClosed:
		return Profiler::EClose;
	default:
		return Profiler::EStart;
		}
	}

void CProfiler::Read()
//
// Pass a free buffer to the reader, allocating one if necessary
//
	{
	TBuffer* buf = iFree;
	if (buf)
		iFree = buf->iNext;
	else
		{
		buf = new TBuffer;
		if(!buf)
			{
			RDebug::Print(_L("PROFILER: No more memory ... stopping"));
			CProfiler::Control(Profiler::EStop);
			return;
			}
		}
	iReader->Queue(buf);
	}

TBool CProfiler::GetSegments()
//
// Gets the list of the current non-XIP segments from device.
// Returns true if zero-length desc is returned, otherwise ...
// ...passes the buffer to write engine and returns false.
	{
	TBuffer* buf = iFree;
	if (buf)
		iFree = buf->iNext;
	else
		{
		RDebug::Printf("PROFILER: No available buffer for GetSegments");
		User::Invariant();
		}
		
	iReader->iSampler.GetSegments(buf->iBuf);
	if (!buf->iBuf.Length())
		{
		buf->iNext = iFree;//Return empty buffer to the free list
		iFree = buf;
		return ETrue;
		}
		
	iWriter->Queue(buf);//Pass the buffer to the write engine.
	return EFalse;
	}

TBool CProfiler::Drain()
//
// Drains all remaining records from the device
// Returns true if zero-length desc is returned, otherwise ...
// ...passes the buffer to the write engine and returns false.
	{
	TBuffer* buf = iFree;
	if (buf)
		iFree = buf->iNext;
	else
		{
		RDebug::Printf("PROFILER: No available buffer for Drain");
		User::Invariant();
		}
		
	iReader->iSampler.Drain(buf->iBuf);

	if (!buf->iBuf.Length())
		{
		buf->iNext = iFree;//Return empty buffer to the free list
		iFree = buf;
		return ETrue;
		}
	iWriter->Queue(buf); //Pass the buffer to the write engine.
	return EFalse;
	}


void CProfiler::GetErrors()
//
// Gets error report from the device and pass the buffer to the write engine
//
	{
	TBuffer* buf = iFree;
	if (buf)
		iFree = buf->iNext;
	else
		{
		RDebug::Printf("PROFILER: No available buffer for GetErrors");
		User::Invariant();
		}
	iReader->iSampler.GetErrors(buf->iBuf);
	iWriter->Queue(buf);
	}

void CProfiler::Write()
//
// Pass a queued buffer to the writer
//
	{
	TBuffer* buf = iHead;
	iHead = buf->iNext;
	if (iHead == 0)
		iTail = 0;
	iWriter->Queue(buf);
	}

void CProfiler::ReadComplete(TBuffer* aBuf)
//
// Handle a completed read buffer
//
	{
	DEBUG_PROFILER(RDebug::Printf("*RC %d",iState);)

	//Add the buffer to the queue
	aBuf->iNext = 0;
	if (iTail)
		iTail->iNext = aBuf;
	else
		iHead = aBuf;
	iTail = aBuf;

	if (!iWriter->IsActive())
		Write();
	
	if (iLastCommand == Profiler::EStart)
		Read();	//Request another read

	DEBUG_PROFILER(RDebug::Printf("*RC end %d",iState);)
	}

void CProfiler::WriteComplete(TBuffer* aBuf)
//
// Handle a flushed write buffer.
//
	{
	DEBUG_PROFILER(RDebug::Printf("*WC %d",iState);)
	
	aBuf->iNext = iFree;//Return empty buffer to the free list
	iFree = aBuf;

	switch (iState)
		{
	case EGettingSegments:
		if (!GetSegments())
			break;//More code segments to be completed

		//Always go to the running state after the segments are collected....
		iState = ERunning;
		iReader->iSampler.Start(KSampleRate);
		Read();
		
		//...but stop sampler immediately if we got another user command 
		if (iLastCommand != Profiler::EStart)
			{
			iReader->iSampler.Stop();
			}
		break; //the end of EGettingSegments case

	case ERunning:
		if (iHead)
			{
			Write(); // There are more buffers to go to the file.
			break;
			}
		if (iLastCommand != Profiler::EStart)
			{//The user has stopped the profiler.
			iState = EDraining;
			if (!Drain())
				break;//More data to drain.
				
			//Drain returned empty. May progress further with the engine state
			if (iLastCommand == Profiler::EStop)
				iState = EStopped;
			else
				{
				iState = EGettingErrors;
				GetErrors();
				}
			}
		break;//the end of ERunning case
		
	case EDraining:
		if (!Drain())
			break; //still draining;
		
		//Drain is completed
		switch (iLastCommand)
			{
		case Profiler::EStart:
			//While draining, we received another Start command	
			iState = ERunning;
			iReader->iSampler.Start(KSampleRate);
			Read();
			break;
		case Profiler::EStop:
			iState = EStopped;
			break;
		default:			
			iState = EGettingErrors;
			GetErrors();
			}
		break; //the end of EDraining case
		
	case EGettingErrors:
		iWriter->Close();
		iState = EClosed;
		switch (iLastCommand)
			{
		case Profiler::EUnload:
			CActiveScheduler::Stop(); //Terminate application.
			break;
		case Profiler::EStart:
			Control(Profiler::EStart);
			break;			
		default:
			break;			
			}
		break; //the end of EGettingErrors case
		
	default:
		RDebug::Printf("PROFILER: WriteComplete in %d state", iState);
		User::Invariant();
		break;
		
		}

	DEBUG_PROFILER(RDebug::Printf("*WC end %d",iState);)
	}



CReader::CReader(TInt aPriority, CProfiler& aProfiler)
	:CActive(aPriority), iProfiler(aProfiler)
	{
	CActiveScheduler::Add(this);
	}

CReader::~CReader()
	{
	Cancel();
	delete iBuf;
	iSampler.Close();
	User::FreeLogicalDevice(KSamplerName);
	}

void CReader::ConstructL()
	{
	TInt r=User::LoadLogicalDevice(KSamplerName);
	if (r!=KErrNone && r!=KErrAlreadyExists)
		User::Leave(r);
	User::LeaveIfError(iSampler.Open());
	}

void CReader::RunL()
//
// Pass the full buffer to the engine
//
	{
	TBuffer* data=iBuf;
	iBuf = 0;
	iProfiler.ReadComplete(data);
	}

void CReader::DoCancel()
	{
	iSampler.ReadCancel();
	}

void CReader::Queue(TBuffer* aBuf)
//
// Queue a request to read data into the empty buffer
//
	{
	iBuf = aBuf;
	iSampler.Read(aBuf->iBuf, iStatus);
	SetActive();
	}

CWriter::CWriter(TInt aPriority, CProfiler& aProfiler)
	:CActive(aPriority), iProfiler(aProfiler)
	{
	CActiveScheduler::Add(this);
	}

CWriter::~CWriter()
	{
	Cancel();
	delete iBuf;
	iFile.Close();
	iFs.Close();
	}

void CWriter::ConstructL()
	{
	User::LeaveIfError(iFs.Connect());
	}

TInt CWriter::Open(const TDesC& aFile)
//
// Open the file for saving the sample data
//
	{
	return iFile.Replace(iFs,aFile,EFileWrite);
	}

void CWriter::Close()
//
// Release the file
//
	{
	iFile.Close();
	}

void CWriter::Queue(TBuffer* aBuf)
//
// Queue a request to write the full buffer into the file
//
	{
	iBuf = aBuf;
	iFile.Write(aBuf->iBuf, iStatus);
	SetActive();
	}

void CWriter::RunL()
//
// Return the empty buffer back to the engine
//
	{
	TBuffer* data=iBuf;
	iBuf = 0;
	iProfiler.WriteComplete(data);
	}

void CWriter::DoCancel()
//
// RFile does not provide a WriteCancel() function
//
	{}


// Server controller

inline const CPServer& CPSession::Server() const
	{return *static_cast<const CPServer*>(CSession2::Server());}

void CPSession::ServiceL(const RMessage2& aMessage)
//
// Handle a IPC request to control the profiler
//
	{
	aMessage.Complete(Server().Control(Profiler::TState(aMessage.Function())));
	}

MProfilerController* CPServer::NewL(TInt aPriority, MProfilerEngine& aEngine)
//
// Create and start the server to provide the Profiler interface
//
	{
	CPServer* self = new(ELeave) CPServer(aPriority, aEngine);
	CleanupStack::PushL(self);
	self->StartL(KProfilerName);
	CleanupStack::Pop();
	return self;
	}

CPServer::CPServer(TInt aPriority, MProfilerEngine& aEngine)
	:CServer2(aPriority), MProfilerController(aEngine)
	{}

void CPServer::Release()
	{
	delete this;
	}

CSession2* CPServer::NewSessionL(const TVersion&,const RMessage2&) const
	{
	return new(ELeave) CPSession();
	}


// Console Controller

MProfilerController* CConsole::NewL(TInt aPriority, MProfilerEngine& aEngine)
//
// Create and start the console UI for the profiler
//
	{
	CConsole* self = new(ELeave) CConsole(aPriority, aEngine);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

CConsole::CConsole(TInt aPriority, MProfilerEngine& aEngine)
	:CActive(aPriority), MProfilerController(aEngine)
	{
	CActiveScheduler::Add(this);
	}

void CConsole::ConstructL()
	{
	iConsole = Console::NewL(KProfilerName, TSize(KConsFullScreen,KConsFullScreen));
	Help();
	Queue();
	}

CConsole::~CConsole()
	{
	Cancel();
	delete iConsole;
	}

void CConsole::Release()
	{
	delete this;
	}

void CConsole::Help()
//
// Display the instructions on the console
//
	{
	_LIT(KInstructions,"[S]tart, Sto[p], [C]lose or E[x]it\r\n");
	iConsole->Write(KInstructions);
	}

void CConsole::Queue()
//
// Request a key press from the console
//
	{
	iConsole->Read(iStatus);
	SetActive();
	}

void CConsole::RunL()
//
// Handle a key press from the console
//
	{
	TInt key = iConsole->KeyCode();
	Queue();
	Profiler::TState command;
	switch (key)
		{
	case 's': case 'S':
		command = Profiler::EStart;
		break;
	case 'p': case 'P':
		command = Profiler::EStop;
		break;
	case 'c': case 'C':
		command = Profiler::EClose;
		break;
	case 'x': case 'X':
		command = Profiler::EUnload;
		break;
	case '?': case 'h': case 'H':
		Help();
		return;
	default:
		return;
		}
	Control(command);
	}

void CConsole::DoCancel()
	{
	iConsole->ReadCancel();
	}


void MainL(TInt aCmd, TDesC* aDrive)
//
// Construct and run the profile engine
//
	{
	CProfiler* p = CProfiler::NewLC(aCmd, aDrive);
	CActiveScheduler::Start();
	CleanupStack::PopAndDestroy(p);
	}

TInt GetCommand(TDes &aDrive)
//
// Decode the command line arguments into a profiler control request
//		aDrive is the drive number to store the profiler data on
//
	{
	_LIT(KStart,"start");
	_LIT(KStop,"stop");
	_LIT(KClose,"close");
	_LIT(KUnload,"unload");
	_LIT(KExit,"exit");
	_LIT(KNoUi,"-noui");
	_LIT(KXIPOnly,"-xiponly");
	_LIT(KDrive,"-drive=");
	const TInt KDriveOffset=7;
	TBuf<64> c;
	User::CommandLine(c);
	TInt cmd = 0;
	if (c.FindF(KNoUi) >= 0)
		cmd |= KCommandNoUi;
	if (c.FindF(KXIPOnly) >= 0)
		cmd |= KCommandXIPOnly;
			
	// get the drive letter if any
	TInt pos = c.FindF(KDrive);
	if(pos >= 0)
		{
		pos += KDriveOffset;
		TBuf<1> driveLet;
		driveLet.SetLength(1);
		driveLet[0] = c[pos];
		driveLet.UpperCase();
		if (driveLet[0] >= 'A' && driveLet[0] <= 'Z')
			{
			aDrive[0] = driveLet[0];
			}
		}
	if (c.FindF(KStart) >= 0)
		return cmd | Profiler::EStart;
	if (c.FindF(KStop) >= 0)
		return cmd | Profiler::EStop;
	if (c.FindF(KClose) >= 0)
		return cmd | Profiler::EClose;
	if (c.FindF(KUnload) >= 0)
		return cmd | Profiler::EUnload;
	if (c.FindF(KExit) >= 0)
		return cmd | Profiler::EUnload;
	return cmd | KCommandNone;
	}

TInt E32Main()
//
// Profiler.exe entry point
// Decode any command-line argument - which can be used to control a running profile engine
// Otherwise start the engine in this process
//
	{
	TBuf<1> drive;
	drive.SetLength(1);
	drive[0] = 'C';
	TInt command = GetCommand(drive);
	if ((command & KCommandMask) != KCommandNone)
		{
		TInt r = Profiler::Control(Profiler::TState(command & KCommandMask));
		if (r != KErrNotFound || (command & KCommandMask) != Profiler::EStart)
			return r;
		}
	CTrapCleanup::New();
	TRAPD(r,MainL(command, &drive));
	if (r != KErrNone)
		RDebug::Print(_L("PROFILER: Error starting profiler"));
	return r;
	}
