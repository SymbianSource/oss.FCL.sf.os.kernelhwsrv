// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_thread.cpp
// 
//

#include "sf_std.h"
#include <u32exec.h>
#include "sf_file_cache.h"

#define __CHECK_DRVNUM(d) {__ASSERT_DEBUG(d>=EDriveA && d<=EDriveZ,Fault(EFsThreadBadDrvNum));}

#ifdef __X86__
const TInt KRequestThreadStackSize = 0x4000;
#else
const TInt KRequestThreadStackSize = 0x3000;
#endif

const TInt KFinaliseTimerPeriod = 10 * 1000 * 1000;	// default 10S finalisation timeout

TFsDriveThread FsThreadManager::iFsThreads[KMaxDrives];
TUint FsThreadManager::iMainId=0;

TFsDriveThread::TFsDriveThread()
//
//
//
:iIsAvailable(EFalse),iIsSync(EFalse),iThread(NULL),iId(0),iIsHung(EFalse),iMediaChangePending(EFalse)
	{
	TInt r=iFSLock.CreateLocal();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EFsThreadConstructor));
	}

TFsPluginThread::TFsPluginThread()
//
//
//
:iIsAvailable(ETrue),iThread(NULL),iId(0)
	{
	TInt r=iPluginLock.CreateLocal();
	iDriveNumber= KMaxDrives+1;
	__ASSERT_ALWAYS(r==KErrNone,Fault(EFsThreadConstructor));
	}


TInt FsThreadManager::InitDrive(TInt aDrvNumber,TBool aIsSync)
//
// Create a drive thread
// Should only by called from main file server thread with drive thread unavailable
// 
//
	{
	__PRINT1(_L("FsThreadManager::InitDrive() drive=%d"),aDrvNumber);
	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);
	__ASSERT_ALWAYS(!t.iIsAvailable,Fault(EThreadManagerInitDrive));
	t.iIsSync=ETrue;
	
	return ChangeSync(aDrvNumber, aIsSync);
	}


TInt FsThreadManager::ChangeSync(TInt aDrvNumber,TBool aIsSync)
//
// Change if a drive is syncronouse after it has been inishalised.
// Should be called from the main thread.  
// Any pending oporations will be compleated.
//

	{
	__PRINT1(_L("FsThreadManager::ChangeSync() drive=%d"),aDrvNumber);
	__CHECK_DRVNUM(aDrvNumber);
	__CHECK_MAINTHREAD();

	LockDrive(aDrvNumber);
	TFsDriveThread& t=FsThreadManager::GetFsDriveThread(aDrvNumber);
	TInt r=KErrNone;

	if (aIsSync!=t.iIsSync)
		{
		if (!aIsSync)
			{
			if(!t.iThread)
				{
				TRAP(r,t.iThread=CDriveThread::NewL());
				if(r!=KErrNone)
					{
					UnlockDrive(aDrvNumber);
					return(r);
					}
				}
			TRAP(r,t.iId=t.iThread->StartL(aDrvNumber));
			__THRD_PRINT2(_L("Starting thread 0x%x returned %d"),&t,r);
			if(r!=KErrNone)
				aIsSync=ETrue;
			else
				{
				t.iIsSync=EFalse;
				__THRD_PRINT1(_L("drive thread id=0x%x"),t.iId);
				}
			}
		if (aIsSync)
			{
			if (t.iThread)
				{
				t.iThread->CompleteAllRequests(KErrNotReady);
				t.iThread->iExit=ETrue;
				t.iThread=NULL;
				}
			t.iIsSync=ETrue;
			}
		}
	if (r==KErrNone)
		t.iIsAvailable=ETrue;
	
	UnlockDrive(aDrvNumber);	
	return r;
	}

void FsThreadManager::CloseDrive(TInt aDrvNumber)
//
// Close a drive thread
// Assumes already locked or safe
// If file system in not synchronous then should be called from a drive thread request
//
	{
	__PRINT1(_L("FsThreadManager::CloseDrive() drive=%d"),aDrvNumber);
	__CHECK_DRVNUM(aDrvNumber);

	// no need to cancel requests if synchronous since queued
	if(!FsThreadManager::IsDriveSync(aDrvNumber,EFalse))
		{
		CDriveThread* pT=NULL;
		TInt r=FsThreadManager::GetDriveThread(aDrvNumber,&pT);
		__ASSERT_ALWAYS(r==KErrNone && pT,Fault(EDismountFsDriveThread));
		pT->CompleteAllRequests(KErrNotReady);
		}

	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);
	__ASSERT_ALWAYS(t.iIsAvailable,Fault(EFsThreadDriveClose1));
	if(!t.iIsSync)
		{
		__ASSERT_ALWAYS(FsThreadManager::IsDriveThread(aDrvNumber,EFalse),Fault(EFsThreadDriveClose2));
		
		StopFinalisationTimer(aDrvNumber);

		// drive thread will exit when request completed
		t.iThread->iExit=ETrue;

		// Ensure that subsequent remounts use a new thread AND a new CDriveThread object - 
		// re-use of the CDriveThread object can lead to deadlock while both old & new threads are active.
		t.iThread = NULL;	

		// Empty the closed file queue for this drive before the thread ends and the CDriveThread object 
		// is deleted because the closed file queue contains a list of CFileCache objects which will 
		// call CRequestThread::RemoveTimer() when closed
		TClosedFileUtils::Remove(aDrvNumber);
		}
	else
		{
		__CHECK_MAINTHREAD();
		t.iIsSync=EFalse;
		}
	t.iIsAvailable=EFalse;
	t.iId=0;
	}


TBool FsThreadManager::IsDriveAvailable(TInt aDrvNumber,TBool aIsLock)
//
//
//
	{
	__CHECK_DRVNUM(aDrvNumber);
	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);
	if(aIsLock)
		t.iFSLock.Wait();
	TBool b=t.iIsAvailable;
	if(aIsLock)
		t.iFSLock.Signal();
	__THRD_PRINT2(_L("drive thread %d iIsAvailable=%d"),aDrvNumber,b);
	return(b);
	}

TBool FsThreadManager::IsDriveSync(TInt aDrvNumber,TBool aLock)
//
// 
//
	{
	__CHECK_DRVNUM(aDrvNumber);
	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);
	if(aLock)
		t.iFSLock.Wait();
	TBool b=(t.iIsAvailable&&t.iIsSync);
	if(aLock)
		t.iFSLock.Signal();
	__THRD_PRINT2(_L("drive thread %d iIsSync=%d"),aDrvNumber,b);
	return(b);
	}

TInt FsThreadManager::GetDriveThread(TInt aDrvNumber, CDriveThread** aDrvThread)
//
// Assumes locked or called from the drive thread
//
	{
	__CHECK_DRVNUM(aDrvNumber);
	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);
	*aDrvThread=NULL;
	TInt r=KErrNone;
	if(!t.iIsAvailable)
		r=KErrNotReady;
	else if(t.iIsSync)
		r=KErrAccessDenied;
	else
		{
		*aDrvThread=t.iThread;
		__ASSERT_DEBUG(*aDrvThread,Fault(EFsThreadGetThread));
		}
	__THRD_PRINT4(_L("GetDriveThread(%d) r %d id=0x%x *aDrvThread=0x%x"),aDrvNumber, r, t.iId, *aDrvThread);
	return r;
	}


void FsThreadManager::LockDrive(TInt aDrvNumber)
//
// Lock the TFsDriveThread object for the aDrvNumber drive
//
	{
	__CHECK_DRVNUM(aDrvNumber);
	__THRD_PRINT1(_L("FsThreadManager::LockDrive(%d)"),aDrvNumber);
	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);
	t.iFSLock.Wait();
	}

void FsThreadManager::UnlockDrive(TInt aDrvNumber)
//
// Unlock the TFsDriveThread object for the aDrvNumber drive
//
	{
	__CHECK_DRVNUM(aDrvNumber);
	__THRD_PRINT1(_L("FsThreadManager::UnlockDrive(%d)"),aDrvNumber);
	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);
	t.iFSLock.Signal();
	}

void FsThreadManager::SetDriveHung(TInt aDrvNumber, TBool aIsHung)
	{
	if (aDrvNumber < EDriveA || aDrvNumber > EDriveZ)
		return;

	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);

	// quick exit if hung state not changing or drive thread not available
	if ((!t.iIsAvailable) || (t.iIsHung == aIsHung))
		return;

	t.iFSLock.Wait();

	// Don't clear the hung state if this is a synchronous request
	// and the drive is asynchronous - we need to wait for whatever 
	// asynchronous request caused the hang to complete first.
	TUint id=RThread().Id();
	TBool isDriveThread = t.iIsSync || (!t.iIsSync && t.iId == id);
	__THRD_PRINT3(_L("Set %d Hung %d. Is Drive thread %d"), aDrvNumber, aIsHung, isDriveThread);
	if (!aIsHung && !isDriveThread)
		{
		t.iFSLock.Signal();
		return;
		}

	t.iIsHung = aIsHung;

	// if we're no longer hung, see if there's a media change pending
	// and if so issue one now
	TBool mediaChangePending = EFalse;
	if(!aIsHung)
		{
		mediaChangePending = t.iMediaChangePending;
		t.iMediaChangePending = EFalse;
		}
	t.iFSLock.Signal();

	// If the drive is now hung we must complete all requests in the drive thread's
	// queue - and all subsequent requests - with KErrNotReady to prevent deadlock.
	// For example, the notifier server may try to access the loader but one of the
	// requests in the queue may already belong to the loader.
	if (aIsHung && t.iThread)
		t.iThread->CompleteClientRequests(KErrNotReady);

	if(mediaChangePending)
		FsNotify::DiskChange(aDrvNumber);
	}


TBool FsThreadManager::IsDriveHung(TInt aDrvNumber)
	{
	if (aDrvNumber < EDriveA || aDrvNumber > EDriveZ)
		return EFalse;

	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);
//	__THRD_PRINT3(_L("Is %d Hung = %d"), aDrvNumber, t.iIsHung);
	return t.iIsHung;
	}


// If the drive is hung, then don't complete any disk change 
// notifications until the request causing the hang completes.
void FsThreadManager::SetMediaChangePending(TInt aDrvNumber)
	{
	if (aDrvNumber < EDriveA || aDrvNumber > EDriveZ)
		return;

	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);

	if (!t.iIsAvailable)
		return;

	t.iFSLock.Wait();
	t.iMediaChangePending = ETrue;
	t.iFSLock.Signal();
	}

void FsThreadManager::SetMainThreadId()
//
// called at file server startup, assumes called from main file server thread
//
	{
	iMainId=RThread().Id();
	__THRD_PRINT1(_L("Main thread id = 0x%x"),iMainId);
	}

TBool FsThreadManager::IsDriveThread(TInt aDrvNumber,TBool aIsLock)
//
// Return ETrue if the calling thread is the aDrvNumber drive thread
//
	{
	__CHECK_DRVNUM(aDrvNumber);
	TFsDriveThread& t=GetFsDriveThread(aDrvNumber);
	TUint id=RThread().Id();
	if(aIsLock)
		t.iFSLock.Wait();
	TBool b = t.iIsAvailable && (!t.iIsSync && t.iId==id || t.iIsSync);
	if(aIsLock)
		t.iFSLock.Signal();
	return(b);
	}
	
TBool FsThreadManager::IsMainThread()
//
// Returns ETrue if calling thread is same as main file server thread
//
	{
	return((TUint)(RThread().Id())==iMainId);
	}


void FsThreadManager::StartFinalisationTimer(TInt aDrvNumber)
	{
	if (aDrvNumber < EDriveA || aDrvNumber > EDriveZ)
		return;

	// If the message could cause disk modification, make sure that the finalisation
	// timer is queued so that we can mark the disk consistent at some point in the future
	CDriveThread* driveThread=NULL;
	TInt r = GetDriveThread(aDrvNumber, &driveThread);
	if(r == KErrNone && driveThread != NULL)
		driveThread->StartFinalisationTimer();
	}

void FsThreadManager::StopFinalisationTimer(TInt aDrvNumber)
	{
	if (aDrvNumber < EDriveA || aDrvNumber > EDriveZ)
		return;

	// If the message could cause disk modification, make sure that the finalisation
	// timer is queued so that we can mark the disk consistent at some point in the future
	CDriveThread* dT=NULL;
	TInt r = GetDriveThread(aDrvNumber, &dT);
	if(r == KErrNone && dT != NULL)
		{
		dT->StopFinalisationTimer();
		}
	}

CRequestThread::CRequestThread()
//
//
//
:iList(_FOFF(CFsRequest,iLink))
	{
	//iRequest=NULL;
	//iIsWaiting=EFalse;
	iExit=EFalse;
	}

TInt CRequestThread::Initialise()
//
// Initialise
//
	{
	TInt r=iListLock.CreateLocal();
	return(r);
	}

CRequestThread::~CRequestThread()
//
//
//
	{
	__ASSERT_ALWAYS(iList.IsEmpty(),Fault(ERequestThreadDestructor));
	iListLock.Close();

	if(iThread.Handle() != 0)
		{
		iThread.Close();
		}
	delete iTimer;
	}

LOCAL_C TInt ThreadFunction(TAny* aPtr)
//
//
//
	{
	__THRD_PRINT(_L("ThreadFunction()"));
	User::SetCritical(User::ESystemCritical);
	CRequestThread* pT=(CRequestThread*)aPtr;
	TInt r = pT->ThreadFunction();
	delete pT;
	return r;
	}

void CRequestThread::CompleteAllRequests(TInt aValue)
    {
    __THRD_PRINT(_L("CRequestThread::CompleteAllRequests()"));
    iListLock.Wait();
    while(!iList.IsEmpty())
        {
        CFsRequest* pR=iList.First();
        pR->iLink.Deque();
        iListLock.Signal();
        pR->Complete(aValue);
        iListLock.Wait();
        }
    iListLock.Signal();
    __THRD_PRINT(_L("all requests completed"));
    }

TInt CRequestThread::ThreadFunction()
//
// entry point for the thread
//
	{
	iTimer = CFsDeltaTimer::New(*this, EPriorityLess);
	if (iTimer == NULL)
		{
		RThread::Rendezvous(KErrNoMemory);
		return(KErrNone);
		}
	iTimer->iStatus = KErrNotReady;

	CTrapCleanup* trapHandler=CTrapCleanup::New();
	if (trapHandler==NULL)
		{
		RThread::Rendezvous(KErrNoMemory);
		delete iTimer;
		return(KErrNone);
		}

	RThread::Rendezvous(KErrNone);

	TInt err = DoThreadInitialise();
	if(err != KErrNone)
		{
		delete trapHandler;
		return(KErrNone);
		}

	iExit=EFalse;
	iIsWaiting=EFalse;
	// start receiving
	Receive();
	CompleteAllRequests(KErrNotReady);
	
	delete trapHandler;
	return(KErrNone);
	}

TInt CRequestThread::DoThreadInitialise()
	{
	return KErrNone;
	}

TInt CRequestThread::DoStart(RThread& aThread)
//
// create thread and return handle
// necessary for client to close thread handle if successful
//
	{
	TInt r=aThread.Create(KNullDesC,::ThreadFunction,KRequestThreadStackSize,NULL,(TAny*)this);
	__PRINT1(_L("CRequestThread::DoStart() r=%d"),r);
	if(r!=KErrNone)
		return(r);
	TRequestStatus status;
	aThread.Rendezvous(status);
	if(status==KRequestPending)
		{
		aThread.SetPriority(EPriorityLess);
		aThread.Resume();
		}
	else
		{
		aThread.Kill(0);
		}
	User::WaitForRequest(status);
	r = status.Int();
	if(r!=KErrNone)
		aThread.Close();
	else
		iThread = aThread;

	return(r);
	}
	

void CRequestThread::Receive()
//
// Receive and process requests
//
	{
	FOREVER
		{
		iListLock.Wait();
		if(!iList.IsEmpty())
			{
			iRequest=iList.First();
			iRequest->iLink.Deque();
			__THRD_PRINT(_L("CRequestThread::Receive() dequeing"));
			iListLock.Signal();
			}
		else
			{
			iIsWaiting=ETrue;
			iRequest = NULL;	// set to NULL so we can distinguish between a timer and a request signal
			iListLock.Signal();
			__THRD_PRINT(_L("CRequestThread::Receive() waiting"));
			User::WaitForAnyRequest();
			iIsWaiting=EFalse;	// force main thread to post new requests on queue to avoid suspending this thread unnecessarily
			}
		__THRD_PRINT2(_L("received req 0x%x, func 0x%x"),iRequest, iRequest ? iRequest->Operation()->iFunction : -1);

		iTimer->RunL();

		if (iRequest)
			iRequest->Process();

		if(iExit)
		    {
		    //Any requests that sneaked on to
		    //the queue are cancelled in 
		    //CRequestThread::ThreadFunction()
		    break;
		    }
		}
	}

void CRequestThread::Deliver(CFsRequest* aRequest,TBool aIsFront, TBool aLowPriority)
//
// Deliver a request to the list from calling thread
// Write request directly to current request if thread is waiting
//
	{
	__THRD_PRINT4(_L("Deliver req %08x to threadId %lx aIsFront=%d iIsWaiting=%d"), aRequest, iThread.Id().Id(), aIsFront, iIsWaiting);
	iListLock.Wait();
	if (iList.IsEmpty())
		{
		// if this is a low priority request (and this is the only request in the queue),
		// reduce the thread's priority to EPriorityAbsoluteBackground
		if (iLowPriority != aLowPriority)
			{
			__THRD_PRINT(_L("LOWERING THREAD PRIORITY"));
			iThread.SetPriority(aLowPriority?EPriorityAbsoluteBackground:EPriorityLess);
			iLowPriority = aLowPriority;
			}
		}
	else
		{
		// there's more than one request in the queue, so rather than go throught the entire queue
		// to determine what the thread's priority should be, assume that it should be "high"
		if (iLowPriority)
			{
			iThread.SetPriority(EPriorityLess);
			iLowPriority = EFalse;
			}
		}

	if(iIsWaiting)
		{
		// the request thread must be waiting on the iWaitLock
		iIsWaiting=EFalse;
		iListLock.Signal();
		iRequest=aRequest;

		iThread.RequestSignal();
		}
	else
		{
		if(aIsFront)
			iList.AddFirst(*aRequest);
		else
			iList.AddLast(*aRequest);
		iListLock.Signal();
		}
	}

void CRequestThread::DeliverFront(CFsRequest* aRequest)
//
//
//
	{
	Deliver(aRequest,ETrue);
	}

void CRequestThread::DeliverBack(CFsRequest* aRequest, TBool aLowPriority)
//
//
//
	{
	Deliver(aRequest,EFalse,aLowPriority);
	}



CFsDeltaTimer* CRequestThread::Timer()
	{
	__ASSERT_ALWAYS(iTimer,Fault(ERequestThreadNotInitialised));
	return iTimer;
	}


CDriveThread::CDriveThread()
	: iFinaliseTimer(FinaliseTimerEvent, this)
	{
	}

CDriveThread* CDriveThread::NewL()
//
//
//
	{
	__PRINT(_L("CDriveThread::NewL()"));
	CDriveThread* pT=new(ELeave) CDriveThread;
	TInt r=pT->Initialise();
	if(r!=KErrNone)
		{
		delete(pT);
		User::Leave(r);
		}
	return(pT);
	}

TUint CDriveThread::StartL(TInt aDrvNumber)
//
//
//
	{
	__PRINT1(_L("CDriveThread::StartL() on drive %d"),aDrvNumber);
	iDriveNumber=aDrvNumber;
	RThread t;
	User::LeaveIfError(DoStart(t));
	TUint id=t.Id();
	return(id);
	}

TInt CDriveThread::DoThreadInitialise()
//
// Initialise function for the drive thread
//  - Renames the thread to contain the drive number.
//  - Note: Drive mappings are not available at this time, so we can't show the actual drive letter.
//
	{
	__PRINT1(_L("CDriveThread::DoThreadInitialise() on drive %d"), iDriveNumber);
	
	TBuf<16> name;
	name.Format(_L("DriveThread_%02d"), iDriveNumber);
	return(RThread::RenameMe(name));
	}


void CDriveThread::CompleteReadWriteRequests()
	{
	__THRD_PRINT1(_L("CDriveThread::CompleteReadWriteRequests() drive=%d"),iDriveNumber);

	iListLock.Wait();

	TDblQueIter<CFsRequest> q(iList);
	CFsRequest* pR;
	while((pR=q++)!=NULL)
		{
		TInt func = pR->Operation()->Function();
		if (func == EFsFileRead || func == EFsFileWrite || func == EFsFileWriteDirty)
			{
			pR->iLink.Deque();
			pR->Complete(KErrNotReady);
			}
		}
	iListLock.Signal();

	__THRD_PRINT(_L("file read/write requests completed"));
	}

/*
This function is called by FsThreadManager::SetDriveHung() and attempts to purge the request queue 
of all requests which MIGHT belong to the critical notifier server (or to the loader) to avoid causing 
a deadlock when calling the server.

All requests are completed with KErrNotReady apart from :
- EFsFileWriteDirty requests, to avoid losing dirty data
- KDispatchObjectClose requests as they are raised by the file server only and therefore cannot belong to the critical notifier server
- EFsFileSubClose requests, to avoid closing files containing dirty data. These requests have their message completed
  so that clients are unblocked, but the request itself is not processed until later. (If the request WAS processed
  and completed, then this might result in the CFileCB and CMountCB object being deleted, leading to problems 
  dereferencing invalid pointers).
*/
void CDriveThread::CompleteClientRequests(TInt aValue)
	{
	__THRD_PRINT1(_L("CDriveThread::CompleteClientRequests() drive=%d"),iDriveNumber);

	iListLock.Wait();

	TDblQueIter<CFsRequest> q(iList);
	CFsRequest* pR;
	while((pR=q++)!=NULL)
		{
		TInt func = pR->Operation()->Function();
		if(func == EFsFileSubClose)
			{
			TInt msgHandle = pR->Message().Handle();
			if ((msgHandle != KLocalMessageHandle) && (msgHandle != 0))
				pR->Message().Complete(KErrNone);
			}
		else if (func != EFsFileWriteDirty && func != KDispatchObjectClose)
			{
			pR->iLink.Deque();
			iListLock.Signal();
			pR->Complete(aValue);
			iListLock.Wait();
			}
		}
	iListLock.Signal();

	__THRD_PRINT(_L("client read requests completed"));
	}

TBool CDriveThread::IsRequestWriteable()
//
// return if current request may cause write to disk
// must be called from drive thread
//
	{
	__ASSERT_ALWAYS(FsThreadManager::IsDriveThread(iDriveNumber,EFalse),Fault(EDriveThreadWriteable));
	return(iRequest->Operation()->IsWrite());
	}

TBool CDriveThread::IsSessionNotifyUser()
//
// return if request's session has notify user set
// must be called from drive thread and request have a session set
//
	{
	__ASSERT_ALWAYS(FsThreadManager::IsDriveThread(iDriveNumber,EFalse),Fault(EDriveThreadNotifyUser1));
	// NB For read-ahead or a flush-dirty write request generated by the file cache, the request or the session may be NULL: 
	// in this case assume that notify user is set (as it's the safest option)
	return iRequest && iRequest->Session() ?  iRequest->Session()->GetNotifyUser() : ETrue;
	}

void CDriveThread::StartFinalisationTimer()
	{
	if(IsProxyDrive(iDriveNumber))
		iFinaliseTimer.Start(this, KFinaliseTimerPeriod);
	}
	

void CDriveThread::StopFinalisationTimer()
	{
	iFinaliseTimer.Stop();
	}

TInt CDriveThread::FinaliseTimerEvent(TAny* aSelfP)
	{
	CDriveThread& self = *(CDriveThread*)aSelfP;

	TDrive& drive = TheDrives[self.iDriveNumber];
	if(drive.IsMounted())
        {
        (void)drive.FinaliseMount(RFs::EFinal_RW);
        }

	return KErrNone;
	}



CPluginThread::CPluginThread(CFsPlugin& aPlugin, RLibrary aLibrary)
  : iPlugin(aPlugin), iLib(aLibrary)
	{
	iPlugin.Open();
	
	/* 
	Duplicate the handle to the DLL which created the plugin to prevent 
	TFsRemovePlugin::DoRequestL() from unmapping the DLL's code segment before
	this thread's destructor has been called as the destructor closes the plugin 
	which results in a call to the plugin's derived destructor contained in the DLL (!)
	*/ 
	TInt r = iLib.Duplicate(iThread, EOwnerProcess);
	__ASSERT_ALWAYS(r==KErrNone, Fault(EFsThreadConstructor));
	}

CPluginThread::~CPluginThread()
	{
	iPlugin.Close();
	iLib.Close();
	iOperationLock.Close();
	}


CPluginThread* CPluginThread::NewL(CFsPlugin& aPlugin, RLibrary aLibrary)
	{
	__PRINT(_L("CPluginThread::NewL()"));
	CPluginThread* pT=new(ELeave) CPluginThread(aPlugin, aLibrary);
	TInt r=pT->Initialise();

	if(r == KErrNone)
		r=pT->iOperationLock.CreateLocal(0);

	if(r!=KErrNone)
		{
		delete(pT);
		User::Leave(r);
		}
	return(pT);
	}

TUint CPluginThread::StartL()
	{
	__PRINT(_L("CPluginThread::StartL()"));
	RThread t;
	User::LeaveIfError(DoStart(t));
	TUint id=t.Id();
	return(id);
	}


TInt CPluginThread::DoThreadInitialise()
	{
	__PRINT(_L("CPluginThread::DoThreadInitialise()"));
 	TRAPD(err, iPlugin.InitialiseL());

	return err;
	}

/** @prototype */
void CPluginThread::OperationLockWait()
	{
	iOperationLock.Wait();
	}

/** @prototype */
void CPluginThread::OperationLockSignal()
	{
	iOperationLock.Signal();
	}

// Class TTickCountQue
/**
@internalComponent
@released

Constructs an empty list header
*/
TTickCountQue::TTickCountQue()
	{}




/**
@internalComponent
@released

Adds the specified list element.

The element is added into the list in order of its tick count.

@param aRef The list element to be inserted.
*/
void TTickCountQue::Add(TTickCountQueLink& aRef)
	{
	TTickCountQueLink* currentLink = (TTickCountQueLink*)(iHead.iNext);
	TTickCountQueLink* addLink = &aRef;

	while (	(currentLink != (TTickCountQueLink*)&iHead) &&
			(((TInt)(addLink->iTickCount - currentLink->iTickCount)) >= 0)
		)
		{
		currentLink = (TTickCountQueLink*)currentLink->iNext;
		}

	addLink->Enque(currentLink->iPrev);
	}




/**
@internalComponent
@released

Removes the first list element from the linked list if its tick count
is prior to the current tick count.

@param aTickCount The current tick count.

@return A pointer to the element removed from the linked list. This is NULL 
        if the first element has yet to expire or the queue is empty.
*/
TTickCountQueLink* TTickCountQue::RemoveFirst(TUint aTickCount)
	{
	TTickCountQueLink* firstLink = (TTickCountQueLink*)iHead.iNext;

	if (((TInt)(firstLink->iTickCount - aTickCount)) <= 0)
		{
		return RemoveFirst();
		}
	else
		{
		return NULL;
		}
	}


/**
@internalComponent
@released

Removes the first list element from the linked list, if any.

@return A pointer to the element removed from the linked list. This is NULL, 
        if the queue is empty.
*/
TTickCountQueLink* TTickCountQue::RemoveFirst()
	{
	TTickCountQueLink* firstLink = (TTickCountQueLink*)iHead.iNext;

	if (firstLink != (TTickCountQueLink*)&iHead)
		{
		firstLink->Deque();
		return firstLink;
		}

	return NULL;
	}




/**
@internalComponent
@released

Gets a pointer to the first list element in the doubly linked list.

@return A pointer to the first list element in the doubly linked list. If 
        the list is empty, this pointer is not necessarily NULL and must not
		be assumed to point to a valid object.
*/
TTickCountQueLink* TTickCountQue::First() const
	{
#if defined (_DEBUG)
	__DbgTestEmpty();
#endif
    return((TTickCountQueLink*)iHead.iNext);
    }





CFsDeltaTimer* CFsDeltaTimer::New(CRequestThread& aRequestThread, TInt aPriority)
	{
	TTimeIntervalMicroSeconds32 tickPeriod;
	UserHal::TickPeriod(tickPeriod);

	CFsDeltaTimer* timer = new CFsDeltaTimer(aRequestThread, aPriority, tickPeriod.Int());
	if (timer == NULL)
		return NULL;

	if (timer->iTimer.CreateLocal() != KErrNone || 
		timer->iLock.CreateLocal() != KErrNone)
		{
		delete timer;
		return NULL;
		}

	return timer;
	}

CFsDeltaTimer::CFsDeltaTimer(CRequestThread& aRequestThread, TInt /*aPriority*/, TInt aTickPeriod) : 
	iRequestThread(aRequestThread), iTickPeriod(aTickPeriod)
	{
	iThreadId = RThread().Id(); 
	}

/**
Destructor.

Frees all resources before destruction of the object. Specifically, it cancels 
any outstanding timer requests generated by the RTimer object and then deletes 
all timed event entries from the timed event queue.

@see RTimer

@publishedAll
@released
*/
CFsDeltaTimer::~CFsDeltaTimer()
	{
	Cancel();

	while (!iQueue.IsEmpty())
		{
		iQueue.First()->Deque();
		}

	iLock.Close();
	iTimer.Close();
	}


/**
Start the timer.

@see RTimer

@publishedAll
@released
*/
void CFsDeltaTimer::Start(TThreadTimer& aEntry, TTimeIntervalMicroSeconds32 aTime)
	{
	iLock.Wait();

	// must be already running on this thread or not running at all
	ASSERT(aEntry.iRequestThread == NULL || aEntry.iRequestThread  == &iRequestThread);

	// attach the entry to this thread
	aEntry.iRequestThread = &iRequestThread;

	// Remove the entry from the list (if it's already queued) 
	// and then add it again in the correct order
	aEntry.iLink.Deque();
	QueueLong(TTimeIntervalMicroSeconds(MAKE_TINT64(0, aTime.Int())), aEntry);

	iLock.Signal();
	}

void CFsDeltaTimer::Stop(TThreadTimer& aEntry)
	{
	iLock.Wait();

	aEntry.iLink.Deque();
	aEntry.iRequestThread = NULL;

	iLock.Signal();
	}


TInt CFsDeltaTimer::QueueLong(TTimeIntervalMicroSeconds aTimeInMicroSeconds, TThreadTimer& aEntry)
	{
	const TInt64 timeInTicks = (aTimeInMicroSeconds.Int64() + iTickPeriod - 1) / iTickPeriod;

	TInt timeInTicks32 = I64LOW(timeInTicks);

	// We are using deltas on tick values, hence using maximum signed number of ticks
	if (I64HIGH(timeInTicks) || (timeInTicks32 < 0))
		{
		return KErrOverflow;
		}

	// Make sure we queue for at least one tick
	if (timeInTicks32 == 0)
		{
		timeInTicks32 = 1;
		}
	
	// Calculate tick count for new entry
	aEntry.iLink.iTickCount = User::TickCount() + timeInTicks32;

	// Add this entry at the right spot
	iQueue.Add(aEntry.iLink);

	// we only need to re-start the timer if we've added an entry to the head of the queue
	// or the timer is not already running
	if (&aEntry.iLink == iQueue.First() || iStatus == KRequestPending)
		Activate();
	
	return KErrNone;
	}

void CFsDeltaTimer::Activate()
//
// Queue a request on the timer.
//
	{
	if (RThread().Id() != iThreadId)
		{
		iRestartNeeded = ETrue;
		iRequestThread.iThread.RequestSignal();
		return;
		}

	if (iStatus == KRequestPending)
		Cancel();

	if (!iQueue.IsEmpty() && !iQueueBusy)
		{
		const TInt ticksToWait = iQueue.First()->iTickCount - User::TickCount();

		if (ticksToWait > 0)
			{
			iTimer.AfterTicks(iStatus, ticksToWait);
			}
		else
			{
			TRequestStatus* status = &iStatus;
			User::RequestComplete(status, KErrNone);
			}
		}
	}



void CFsDeltaTimer::RunL()
//
// Call all zero delta callbacks
	{
	// if still running and no restart needed, then there's nothing to do
	if (iStatus == KRequestPending && !iRestartNeeded)
		return;


	iLock.Wait();

	// Queue busy
	iQueueBusy = ETrue;

	// Whilst the list of expired timers is being processed, time will pass and
	// the tick count may have increased such that there are now more expired
	// timers.  Loop until we have either emptied the queue or can wait for a
	// timer exipration in the future.
	if (iStatus == KErrNone)
		{
		iStatus = KErrNotReady;
		while (!iQueue.IsEmpty())
			{
			// Calculate how long till first timer expires
			const TUint tickCount = User::TickCount();

			// If the first timer is yet to expire, wait some more
			if (((TInt)(iQueue.First()->iTickCount - tickCount)) > 0)
				{
				break;
				}

			// Remove entry before callback to prevent re-entrancy issues
			TTickCountQueLink* entry = iQueue.RemoveFirst();

			// Iterate through the timers we know have expired based on the
			// last calculation of delta
			while (entry)
				{
				TThreadTimer* threadTimer = reinterpret_cast<TThreadTimer*>(PtrSub(entry, _FOFF(TThreadTimer, iLink)));
				threadTimer->iRequestThread = NULL;	// indicate timer not running

				// Make callback.  This could go reentrant on Queue[Long]() or Remove().
				iLock.Signal();
				threadTimer->iCallBack.CallBack();
				iLock.Wait();

				// Remove the next expired entry, if any
				entry = iQueue.RemoveFirst(tickCount);
				}
			}
		}

	// Queue idle
	iQueueBusy = EFalse;


	// Requeue timer if queue isn't empty
	Activate();

	iRestartNeeded = EFalse;

	iLock.Signal();
	}
	
void CFsDeltaTimer::Cancel()
	{
	if (iStatus == KRequestPending)
		{
		iTimer.Cancel();
		User::WaitForRequest(iStatus);
		}
	}



TThreadTimer::TThreadTimer(TInt (*aCallBackFunction)(TAny*),TAny* aPtr) :
	iCallBack(aCallBackFunction, aPtr),
	iRequestThread(NULL)
	{
	};	


void TThreadTimer::Start(CRequestThread* aRequestThread, TTimeIntervalMicroSeconds32 aTime)
	{
	ASSERT(aRequestThread);

	// NB: There are no locks here, so we have to be aware that CFsDeltaTimer::RunL()
	// may be running in another thread and set iRequestThread to NULL
	CRequestThread* requestThread = iRequestThread;
	if (!requestThread)	// if not already running, use caller's request thread
		requestThread = aRequestThread;


	__ASSERT_DEBUG(requestThread->Timer(),Fault(ERequestThreadNotInitialised));
	requestThread->Timer()->Start(*this, aTime);
	}


void TThreadTimer::Stop()
	{
	// NB: There are no locks here, so we have to be aware that CFsDeltaTimer::RunL()
	// may be running in another thread and set iRequestThread to NULL
	CRequestThread* requestThread = iRequestThread;
	if (requestThread)
		requestThread->Timer()->Stop(*this);
	}


