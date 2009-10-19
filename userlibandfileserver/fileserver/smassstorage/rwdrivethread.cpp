// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include "scsiprot.h"
#include "usbmsshared.h"
#include "rwdrivethread.h"
#include "massstoragedebug.h"

// ---

#ifdef PRINT_MSDC_MULTITHREADED_READ_INFO
#define __MT_READ_PRINT(t) {RDebug::Print(t);}
#define __MT_READ_PRINT1(t,a) {RDebug::Print(t,a);}
#define __MT_READ_PRINT2(t,a,b) {RDebug::Print(t,a,b);}
#else
#define __MT_READ_PRINT(t)
#define __MT_READ_PRINT1(t,a)
#define __MT_READ_PRINT2(t,a,b)
#endif // PRINT_MSDC_MULTITHREADED_READ_INFO


#ifdef MSDC_MULTITHREADED 

TBlockDesc::TBlockDesc()
	:iBuf((TUint8 *)NULL,0,0)
	{
	}

void TBlockDesc::SetPtr(TPtr8& aDes)
	{
	iBuf.Set(aDes);
	}


TBlockDescBuffer::TBlockDescBuffer()
	{ 
	iDescReadPtr = &iDesc1;
	iDescWritePtr = &iDesc2;
	}

void TBlockDescBuffer::SetUpReadBuf(TPtr8& aDes1, TPtr8& aDes2)
	{
	iDesc1.SetPtr(aDes1);
	iDesc2.SetPtr(aDes2);
	iDescReadPtr = &iDesc1;
	iDescWritePtr = &iDesc2;
	}


//-----------------------------------------------

/**
Construct a CThreadContext object.

@param aName The name to be assigned to this thread.
@param aThreadFunction Function to be called when thread is initially scheduled.
@param aOwner Pointer to the object owning the thread. Used as the parameter to aThreadFunction.
*/
CThreadContext* CThreadContext::NewL(const TDesC& aName,
									 TThreadFunction aThreadFunction,
									 TAny* aOwner)
	{
	__FNLOG("CThreadContext::NewL");
	CThreadContext* self = new (ELeave) CThreadContext();
	CleanupStack::PushL(self);
	self->ConstructL(aName, aThreadFunction, aOwner);
	CleanupStack::Pop();
	return self;
	}

/**
Construct a CThreadContext object

@param aName The name to be assigned to this thread.
@param aThreadFunction Function to be called when thread is initially scheduled.
@param aOwner Pointer to the object owning the thread. Used as the parameter to aThreadFunction.
*/
void CThreadContext::ConstructL(const TDesC& aName,
								TThreadFunction aThreadFunction,
								TAny* aOwner)
	{
	__FNLOG("CThreadContext::ConstructL");
	__PRINT(_L("Creating Critical Section"));
	User::LeaveIfError(iCritSect.CreateLocal());
	__PRINT(_L("Creating RThread"));

	TUint serial(0); // Used to retry creation of a thread in case
					 // one with the same name already exists

	RBuf threadName;
	threadName.CreateMaxL(aName.Length() + 8);
	CleanupClosePushL(threadName);
	threadName = aName;

	TInt err;
	for (;;)
		{
		err = iThread.Create(threadName, aThreadFunction, 0x1000, NULL, aOwner);
		__PRINT2(_L("CThreadContext::ConstructL Created thread %S err=%d"), &threadName, err);

        // for a restart wait and retry until old thread is gone
		if (err == KErrAlreadyExists)
			{
			User::After(10 * 1000);     // 10 mS
			threadName = aName;
			threadName.AppendNumFixedWidth(serial, EDecimal, 8);
			++serial;
			}
		else
			{
			break;
			}
		}

    User::LeaveIfError(err);
    CleanupStack::Pop(); // threadName
    threadName.Close();

	// set priority
	iThread.SetPriority(EPriorityMore);
	}


/**
Construct a CThreadContext object
*/
CThreadContext::CThreadContext()
	:
	iError(KErrNone)
	{
	__FNLOG("CThreadContext::CThreadContext");
	}

/**
Destructor
*/
CThreadContext::~CThreadContext()
	{
	__FNLOG("CThreadContext::~CThreadContext");
	__PRINT(_L("Closing Critical Section"));
	iCritSect.Close();
	__PRINT(_L("Killing ThreadContext"));
	iThread.Kill(0);
	__PRINT(_L("Closing ThreadContext"));
	iThread.Close();
	}

//-----------------------------------------------

/**
Construct a CWriteDriveThread object
*/
CWriteDriveThread* CWriteDriveThread::NewL()
	{
	__FNLOG("CWriteDriveThread::NewL");
	CWriteDriveThread* self = new (ELeave) CWriteDriveThread();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

/**
Construct a CWriteDriveThread object
*/
void CWriteDriveThread::ConstructL()
	{
	__FNLOG("CWriteDriveThread::ConstructL");
	TBuf<16> name = _L("MassStorageWrite");
	iThreadContext = CThreadContext::NewL(name, ThreadFunction, this);
	// There are two free pointers to start with so initialise the semaphore with 1
	User::LeaveIfError(iProducerSem.CreateLocal(1));
	User::LeaveIfError(iConsumerSem.CreateLocal(0));
	
	iThreadContext->Resume();
	}

/**
Construct a CWriteDriveThread object
*/
CWriteDriveThread::CWriteDriveThread() 
	: iIsCommandWrite10(EFalse)
	{
	__FNLOG("CWriteDriveThread::CWriteDriveThread");
	}

/**
Destructor
*/
CWriteDriveThread::~CWriteDriveThread()
	{
	__FNLOG("CWriteDriveThread::~CWriteDriveThread");
	delete iThreadContext;
	}

/**
This function is called when the thread is initially scheduled.

@param aSelf Pointer to self to facilitate call to member method.
*/
TInt CWriteDriveThread::ThreadFunction(TAny* aSelf)
	{
	__FNLOG("CWriteDriveThread::ThreadFunction");
	CWriteDriveThread* self = static_cast<CWriteDriveThread*>(aSelf);
	return self->WriteToDrive();
	}

/**
Writes the data pointed to by iDescWritePtr to the drive.
*/
TInt CWriteDriveThread::WriteToDrive()
	{
	__FNLOG("\tCWriteDriveThread::WriteToDrive");

	// One-off convenience variable assignment
	TBlockDesc* &desc = iThreadContext->iBuffer.iDescWritePtr;
	
	for(;;)
		{
		iConsumerSem.Wait();
		__PRINT(_L("\tWaiting on Write CS..."));
		iThreadContext->iCritSect.Wait();
		// +++ WRITE CS STARTS HERE +++
		__PRINT1(_L("\tNow using as write buffer: iBuf%d"), iThreadContext->iBuffer.GetBufferNumber(&desc->iBuf));
#ifdef MEASURE_AND_DISPLAY_WRITE_TIME
		RDebug::Print(_L("\tSCSI: writing %d bytes\n"), desc->iBuf.Length());
		TTime t0, t1;
		t0.HomeTime();
#else
		__PRINT1(_L("\tSCSI: writing %d bytes\n"), desc->iBuf.Length());
#endif
		// Write buffer to disk

#ifdef INJECT_ERROR
		if (desc->iBuf[0] == '2')
		{
			desc->iBuf[0] = 'x';
			RDebug::Printf("Injecting error");
		}

		
		RDebug::Printf("%08lx %x [%x] [%x]", desc->iByteOffset, desc->iBuf.Length(), 
			desc->iBuf[0],
			desc->iBuf[desc->iBuf.Length()-1]);
#endif

		iThreadContext->iError = iThreadContext->iDrive->Write(desc->iByteOffset, desc->iBuf,iThreadContext->iDrive->IsWholeMediaAccess());
#ifdef INJECT_ERROR
		if (desc->iBuf[0] == 'x')
		{
			iThreadContext->iError = KErrUnknown;
		}
#endif

#ifdef MEASURE_AND_DISPLAY_WRITE_TIME
		t1.HomeTime();
		const TTimeIntervalMicroSeconds time = t1.MicroSecondsFrom(t0);
		const TUint time_ms = I64LOW(time.Int64() / 1000);
		RDebug::Print(_L("SCSI: write took %d ms\n"), time_ms);
#endif
		iCallback((TUint8*) (desc->iBuf.Ptr()), iCallbackParameter);
		iWriteCounter--;
		ASSERT(iWriteCounter >= 0);

		__PRINT(_L("\tSignalling Write CS"));
		iThreadContext->iCritSect.Signal();
		// +++ WRITE CS ENDS HERE +++
		iProducerSem.Signal();
		}
	}

/**
Initiates writing data pointed to by iReadBuf to the drive and resumes the thread. Writing 
is completed by the ThreadFunction when the thread is resumed.

@param aDrive Drive to write to.
@param aOffset Write offset.
*/
TInt CWriteDriveThread::WriteDriveData(CMassStorageDrive* aDrive, const TInt64& aOffset, TPtrC8& aDes, ProcessWriteCompleteFunc aFunc, TAny* aPtr)
	{
	// Check error code from previous write
	const TInt r = iThreadContext->iError;
	if (r != KErrNone)
		{
		__PRINT1(_L("Error after previous write = 0x%x \n"), r);
		return KErrAbort;
        }

	// Swap the two buffer pointers
	iProducerSem.Wait();
	__PRINT(_L("Waiting on Write CS..."));
	// +++ WRITE CS STARTS HERE +++
	iThreadContext->iCritSect.Wait();

	// New DB First read into the iDescReadPtr pointer,
	// then swap,so that write pointer points to correct location, as the ptr pointed to by iDescWritePtr is what is written from in WriteToDrive 
	iThreadContext->iBuffer.iDescReadPtr->iBuf.Set((TUint8*)aDes.Ptr(), aDes.Length(), KMaxBufSize );
	
	iCallback = aFunc;
	iCallbackParameter = aPtr;

	iWriteCounter++;
	iThreadContext->iBuffer.SwapDesc();
	// Prepare variables for next write
	iThreadContext->iDrive = aDrive;
	iThreadContext->iBuffer.iDescWritePtr->iByteOffset = aOffset;
	// +++ WRITE CS ENDS HERE +++
	__PRINT(_L("Signalling Write CS..."));
	iThreadContext->iCritSect.Signal();

	iConsumerSem.Signal();
	return KErrNone;
}


void CWriteDriveThread::WaitForWriteEmpty()
{
	while(iWriteCounter > 0)
		{
		User::After(100);
		}
}

// Check if the target address range was recently written to, this is to force a
// cache miss when reading from the same sectors that were just written. 
// Optimisation note: this is only needed if the read precache was started
// before the write was completed.
TBool CWriteDriveThread::IsRecentlyWritten(TInt64 aOffset, TInt aLength)
{
	ASSERT(iWriteCounter == 0);
	if (iIsCommandWrite10) //If the previous command is Write10, then discard pre-read as the same buffers are used and will be over written by Write10 
		return ETrue;
	if(aOffset <= iThreadContext->iBuffer.iDescReadPtr->iByteOffset &&
			aOffset + aLength >= iThreadContext->iBuffer.iDescReadPtr->iByteOffset)
		return ETrue;
	if(aOffset >= iThreadContext->iBuffer.iDescReadPtr->iByteOffset &&
			aOffset <= iThreadContext->iBuffer.iDescReadPtr->iByteOffset + iThreadContext->iBuffer.iDescReadPtr->iLength)
		return ETrue;
	if(aOffset <= iThreadContext->iBuffer.iDescWritePtr->iByteOffset &&
			aOffset + aLength >= iThreadContext->iBuffer.iDescReadPtr->iByteOffset)
		return ETrue;
	if(aOffset >= iThreadContext->iBuffer.iDescWritePtr->iByteOffset &&
			aOffset <= iThreadContext->iBuffer.iDescReadPtr->iByteOffset + iThreadContext->iBuffer.iDescReadPtr->iLength)
		return ETrue;
	return EFalse;
}

//-----------------------------------------------

/**
Construct a CReadDriveThread object
*/
CReadDriveThread* CReadDriveThread::NewL()
	{
	__FNLOG("CReadDriveThread::NewL");
	CReadDriveThread* self = new (ELeave) CReadDriveThread();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

/**
Construct a CReadDriveThread object

@param aName The name to be assigned to this thread.
@pram aThreadFunction Function to be called when thread is initially scheduled.
*/
void CReadDriveThread::ConstructL()
	{
	__FNLOG("CReadDriveThread::ConstructL");
	TBuf<15> name = _L("MassStorageRead");
	iThreadContext = CThreadContext::NewL(name, ThreadFunction, this);
	}

/**
Construct a CReadDriveThread object
*/
CReadDriveThread::CReadDriveThread()
	:
	iThreadRunning(EFalse)
	{
	__FNLOG("CReadDriveThread::CReadDriveThread");
	}

/**
Destructor
*/
CReadDriveThread::~CReadDriveThread()
	{
	__FNLOG("CReadDriveThread::~CReadDriveThread");
	delete iThreadContext;
	}

/**
This function is called when the thread is initially scheduled.

@param aSelf Pointer to self to facilitate call to member method.
*/
TInt CReadDriveThread::ThreadFunction(TAny* aSelf)
	{
	__FNLOG("CReadDriveThread::ThreadFunction");
	CReadDriveThread* self = static_cast<CReadDriveThread*>(aSelf);
	return self->ReadFromDrive();
	}

/**
Reads data from the drive with iOffset and iReadLength into memory pointer iReadBuffer
and suspends the thread.
*/
TInt CReadDriveThread::ReadFromDrive()
	{
	__FNLOG("\tCReadDriveThread::ReadFromDrive");

	// One-off convenience variable assignment
	TBlockDesc* &desc = iThreadContext->iBuffer.iDescWritePtr;

	for (;;)
		{
		__PRINT(_L("\tWaiting on Read CS..."));
		iThreadContext->iCritSect.Wait();
		// +++ READ CS STARTS HERE +++
		iThreadRunning = ETrue;
		iCompleted = EFalse;

		__PRINT1(_L("\tNow using as read buffer: iBuf%d"), iThreadContext->iBuffer.GetBufferNumber(&desc->iBuf));
 
#ifdef MEASURE_AND_DISPLAY_READ_TIME
		RDebug::Print(_L("\tSCSI: reading %d bytes\n"), desc->iBuf.Length());
		TTime t0, t1;
		t0.HomeTime();
#else
		__PRINT1(_L("\tSCSI: reading %d bytes\n"), desc->iBuf.Length());
#endif
		// Fill read buffer from disk
		iThreadContext->iError = iThreadContext->iDrive->Read(desc->iByteOffset,
															  desc->iLength,
															  desc->iBuf,
															  iThreadContext->iDrive->IsWholeMediaAccess());

#ifdef MEASURE_AND_DISPLAY_READ_TIME
		t1.HomeTime();
		const TTimeIntervalMicroSeconds time = t1.MicroSecondsFrom(t0);
		const TUint time_ms = I64LOW(time.Int64() / 1000);
		RDebug::Print(_L("SCSI: read took %d ms\n"), time_ms);
#endif

		iCompleted = ETrue;
		iThreadRunning = EFalse;
		__PRINT(_L("\tSignalling Read CS"));
		// +++ READ CS ENDS HERE +++
		iThreadContext->iCritSect.Signal();
		// Suspend self
		__PRINT(_L("\tSuspending Read Thread"));
		RThread().Suspend();
		}
	}

/**
Client read request of a data block from the specified drive. 
If there is no pre-read data that matches the requested Offset and Length then the drive
is read and the next pre-read is setup. If there is matching pre-read data available then
the next pre-read is setup. Finishes by resuming the thread and the ThreadFunciton runs.

@param aDrive Drive to read from.
@param aOffset Read offset
@param aLength Length 
*/
TBool CReadDriveThread::ReadDriveData(CMassStorageDrive* aDrive,
									  const TInt64& aOffset,
									  TUint32 aLength,
									  TBool aIgnoreCache)
	{
	__MT_READ_PRINT2(_L("\nRead10: offs %ld len %d"), aOffset, aLength);

	__PRINT(_L("Waiting on Read CS..."));
	iThreadContext->iCritSect.Wait();
	// +++ READ CS STARTS HERE +++
	__ASSERT_DEBUG(!iThreadRunning, User::Panic(_L("MSDC-THREAD"), 666));

	TBlockDesc* &desc = iThreadContext->iBuffer.iDescReadPtr;
	TBlockDesc* &bgDesc = iThreadContext->iBuffer.iDescWritePtr;

	if ((!aIgnoreCache) &&
		(iCompleted) &&
		(iThreadContext->iError == KErrNone) &&
		(iThreadContext->iDrive == aDrive) &&
		(bgDesc->iByteOffset == aOffset) &&
		(bgDesc->iLength == aLength))
		{
		// Good: We pre-read the correct data :-)
		__MT_READ_PRINT(_L("Match: Using pre-read data :-) :-) :-) :-)"));
		}
	else
		{
		__MT_READ_PRINT(_L("Not using pre-read data"));
		if (iThreadContext->iError != KErrNone)
			{
			__MT_READ_PRINT1(_L("Pre-read failed: %d"), iThreadContext->iError);
			}
		if (iThreadContext->iDrive != aDrive)
			{
			__MT_READ_PRINT2(_L("Pre-read drive mismatch: pre 0x%08x / act 0x%08x"),
							 iThreadContext->iDrive, aDrive);
			}
		if (desc->iByteOffset != aOffset)
			{
			__MT_READ_PRINT2(_L("Pre-read offset mismatch: pre %ld / act %ld"),
							 desc->iByteOffset, aOffset);
			}
		if (desc->iLength != aLength)
			{
			__MT_READ_PRINT2(_L("Pre-read length mismatch: pre %d / act %d"),
							 desc->iLength, aLength);
			// Potential optimization: If the pre-read was OK but for more data
			// than the host is now asking for, we could still satisfy that
			// request from the pre-read data by shortening the buffer.
			}
		// No valid pre-read data was available - so we have to read it now
		bgDesc->iByteOffset = aOffset;
		bgDesc->iLength = aLength;
		TInt err = aDrive->Read(aOffset,
								aLength,
								bgDesc->iBuf,
								aDrive->IsWholeMediaAccess());
		if (err != KErrNone)
			{
			__PRINT1(_L("Read failed, err=%d\n"), err);
			// +++ READ CS ENDS HERE +++
			__PRINT(_L("Signalling Read CS..."));
			iThreadContext->iCritSect.Signal();
			return EFalse;
			}
		}

	// Prepare thread variables for next pre-read attempt by the ReadThread
	const TInt64 offs_new = aOffset + aLength;
	iThreadContext->iDrive = aDrive;	// same drive
	desc->iByteOffset = offs_new;		// next block
	desc->iLength = aLength;			// same length
	iCompleted = EFalse;
	iThreadContext->iBuffer.SwapDesc();

	// +++ READ CS ENDS HERE +++
	__PRINT(_L("Signalling Read CS..."));
	iThreadContext->iCritSect.Signal();
	// Start background read
	__PRINT(_L("Resuming Read Thread"));
	iThreadContext->Resume();
	return ETrue;
	}

/**
Discard the read buffer. This is used to force a cache miss when reading from
the same sectors that were just written.
*/
void CReadDriveThread::DiscardRead()
{
	__PRINT(_L("Waiting on Read CS in DiscardRead..."));
	iThreadContext->iCritSect.Wait();
	// +++ READ CS STARTS HERE +++
	__PRINT(_L("Discarding pre-read buffer"));
	iCompleted = EFalse;
	iThreadContext->iBuffer.iDescReadPtr->iLength = 0;

	// +++ READ CS ENDS HERE +++
	__PRINT(_L("Signalling Read CS in DiscardRead..."));
	iThreadContext->iCritSect.Signal();
}
#endif // MSDC_MULTITHREADED

