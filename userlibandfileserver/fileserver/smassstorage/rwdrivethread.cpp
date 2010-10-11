// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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


#include <e32std.h>
#include "smassstorage.h"
#include "mprotocol.h"
#include "scsiprot.h"       // KMaxBufSize
#include "drivemanager.h"
#include "usbmsshared.h"
#include "rwdrivethread.h"


#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "rwdrivethreadTraces.h"
#endif


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
    OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_10, "Creating Critical Section");
    User::LeaveIfError(iCritSect.CreateLocal());
    OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_11, "Creating RThread");

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
        OstTraceData(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_13,
                     "CThreadContext::ConstructL Created thread %s err=%d", threadName.Ptr(), threadName.Length());
        OstTrace1(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_14,
                     "err=%d", err);


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
    }

/**
Destructor
*/
CThreadContext::~CThreadContext()
    {
    OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_20, "Closing Critical Section");
    iCritSect.Close();
    OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_21, "Killing ThreadContext");
    iThread.Kill(0);
    OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_22, "Closing ThreadContext");
    iThread.Close();
    }

//-----------------------------------------------

/**
Construct a CWriteDriveThread object
*/
CWriteDriveThread* CWriteDriveThread::NewL()
    {
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
    }

/**
Destructor
*/
CWriteDriveThread::~CWriteDriveThread()
    {
    delete iThreadContext;
    }

/**
This function is called when the thread is initially scheduled.

@param aSelf Pointer to self to facilitate call to member method.
*/
TInt CWriteDriveThread::ThreadFunction(TAny* aSelf)
    {
    CWriteDriveThread* self = static_cast<CWriteDriveThread*>(aSelf);
    return self->WriteToDrive();
    }

/**
Writes the data pointed to by iDescWritePtr to the drive.
*/
TInt CWriteDriveThread::WriteToDrive()
    {
    // One-off convenience variable assignment
    TBlockDesc* &desc = iThreadContext->iBuffer.iDescWritePtr;

    for(;;)
        {
        iConsumerSem.Wait();
        OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_30, "Waiting on Write CS...");
        iThreadContext->iCritSect.Wait();
        // +++ WRITE CS STARTS HERE +++
        OstTrace1(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_31,
                  "Now using as write buffer: iBuf%d", iThreadContext->iBuffer.GetBufferNumber(&desc->iBuf));
        OstTrace1(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_32,
                  "SCSI: writing %d bytes", desc->iBuf.Length());
        // Write buffer to disk
        iThreadContext->iError = iThreadContext->iDrive->Write(desc->iByteOffset, desc->iBuf,iThreadContext->iDrive->IsWholeMediaAccess());

        iCallback((TUint8*) (desc->iBuf.Ptr()), iCallbackParameter);
        iWriteCounter--;
        __ASSERT_DEBUG(iWriteCounter >= 0, User::Panic(KUsbMsSvrPncCat, EMsThreadWriteToDrive));

        OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_33, "Signalling Write CS");
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
        OstTrace1(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_40, "Error after previous write = %d", r);
        return KErrAbort;
        }

    // Swap the two buffer pointers
    iProducerSem.Wait();
    OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_41, "Waiting on Write CS...");
    // +++ WRITE CS STARTS HERE +++
    iThreadContext->iCritSect.Wait();

    // New DB First read into the iDescReadPtr pointer,
    // then swap,so that write pointer points to correct location, as the ptr pointed to by iDescWritePtr is what is written from in WriteToDrive
    iThreadContext->iBuffer.iDescReadPtr->iBuf.Set((TUint8*)aDes.Ptr(), aDes.Length(), KMaxBufSize);

    iCallback = aFunc;
    iCallbackParameter = aPtr;

    iWriteCounter++;
    iThreadContext->iBuffer.SwapDesc();
    // Prepare variables for next write
    iThreadContext->iDrive = aDrive;
    iThreadContext->iBuffer.iDescWritePtr->iByteOffset = aOffset;
    // +++ WRITE CS ENDS HERE +++
    OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_42, "Signalling Write CS...");
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
    __ASSERT_DEBUG(iWriteCounter == 0, User::Panic(KUsbMsSvrPncCat, EMsThreadIsRecentlyWritten));
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
    }

/**
Destructor
*/
CReadDriveThread::~CReadDriveThread()
    {
    delete iThreadContext;
    }

/**
This function is called when the thread is initially scheduled.

@param aSelf Pointer to self to facilitate call to member method.
*/
TInt CReadDriveThread::ThreadFunction(TAny* aSelf)
    {
    CReadDriveThread* self = static_cast<CReadDriveThread*>(aSelf);
    return self->ReadFromDrive();
    }

/**
Reads data from the drive with iOffset and iReadLength into memory pointer iReadBuffer
and suspends the thread.
*/
TInt CReadDriveThread::ReadFromDrive()
    {
    // One-off convenience variable assignment
    TBlockDesc* &desc = iThreadContext->iBuffer.iDescWritePtr;

    for (;;)
        {
        OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_50, "Waiting on Read CS...");
        iThreadContext->iCritSect.Wait();
        // +++ READ CS STARTS HERE +++
        iThreadRunning = ETrue;
        iCompleted = EFalse;

        OstTrace1(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_51,
                  "\tNow using as read buffer: iBuf%d", iThreadContext->iBuffer.GetBufferNumber(&desc->iBuf));
        OstTrace1(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_52,
                  "\tSCSI: reading %d bytes", desc->iBuf.Length());

        // Fill read buffer from disk
        iThreadContext->iError = iThreadContext->iDrive->Read(desc->iByteOffset,
                                                              desc->iLength,
                                                              desc->iBuf,
                                                              iThreadContext->iDrive->IsWholeMediaAccess());

        iCompleted = ETrue;
        iThreadRunning = EFalse;
        OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_53, "Signalling Read CS");
        // +++ READ CS ENDS HERE +++
        iThreadContext->iCritSect.Signal();
        // Suspend self
        OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_54, "Suspending Read Thread");
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
    OstTraceExt2(TRACE_SMASSSTORAGE_MEDIADB, CTHREADCONTEXT_60, "Read10: offs %ld len %d", aOffset, aLength);

    OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_61, "Waiting on Read CS...");
    iThreadContext->iCritSect.Wait();
    // +++ READ CS STARTS HERE +++
    __ASSERT_DEBUG(!iThreadRunning, User::Panic(KUsbMsSvrPncCat, EMsThreadReadDriveThread));

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
        OstTrace0(TRACE_SMASSSTORAGE_MEDIADB, CTHREADCONTEXT_70, "Match: Using pre-read data :-) :-) :-) :-)");
        }
    else
        {
        OstTrace0(TRACE_SMASSSTORAGE_MEDIADB, CTHREADCONTEXT_71, "Not using pre-read data");
        if (iThreadContext->iError != KErrNone)
            {
            OstTrace1(TRACE_SMASSSTORAGE_MEDIADB, CTHREADCONTEXT_72, "Pre-read failed: %d", iThreadContext->iError);
            }
        if (iThreadContext->iDrive != aDrive)
            {
            OstTraceExt2(TRACE_SMASSSTORAGE_MEDIADB, CTHREADCONTEXT_73,
                         "Pre-read drive mismatch: pre 0x%08x / act 0x%08x",
                         (TUint32)iThreadContext->iDrive, (TUint32)aDrive);
            }
        if (desc->iByteOffset != aOffset)
            {
            OstTraceExt4(TRACE_SMASSSTORAGE_MEDIADB, CTHREADCONTEXT_74,
                         "Pre-read offset mismatch: pre 0x%x 0x%x  / act 0x%x 0x%x",
                         I64HIGH(desc->iByteOffset), I64LOW(desc->iByteOffset), I64HIGH(aOffset), I64LOW(aOffset));
            }
        if (desc->iLength != aLength)
            {
            OstTraceExt2(TRACE_SMASSSTORAGE_MEDIADB, CTHREADCONTEXT_75,
                         "Pre-read length mismatch: pre 0x%x / act 0x%x",
                         (TUint32)desc->iLength, aLength);
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
            OstTrace1(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_76, "Read failed, err=%d", err);
            // +++ READ CS ENDS HERE +++
            OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_77, "Signalling Read CS...");
            iThreadContext->iCritSect.Signal();
            return EFalse;
            }
        }

    // Prepare thread variables for next pre-read attempt by the ReadThread
    const TInt64 offs_new = aOffset + aLength;
    iThreadContext->iDrive = aDrive;    // same drive
    desc->iByteOffset = offs_new;       // next block
    desc->iLength = aLength;            // same length
    iCompleted = EFalse;
    iThreadContext->iBuffer.SwapDesc();

    // +++ READ CS ENDS HERE +++
    OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_78, "Signalling Read CS...");
    iThreadContext->iCritSect.Signal();
    // Start background read
    OstTrace0(TRACE_SMASSSTORAGE_MEDIA, CTHREADCONTEXT_79, "Resuming Read Thread");
    iThreadContext->Resume();
    return ETrue;
    }

#endif // MSDC_MULTITHREADED

