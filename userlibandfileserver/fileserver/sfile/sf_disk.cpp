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
// f32\sfile\sf_disk.cpp
// 
//

#include "sf_std.h"

#if defined(_LOCKABLE_MEDIA)

LOCAL_C TInt DelayedWriter(TAny *aPtr);
LOCAL_C void DelayedWriterL(const TDelayedWriter *aDW);


EXPORT_C void WriteToDisk(const TDesC& aFileName, const TDesC8& aBuf)
// 
// Launches as separate thread that writes the contents of the pbus pswd
// store to disk.  It is possible that this function will be called again
// before the thread has finished writing.  In that case, a new thread
// will be created and it will wait on the global DelayedWriteSem semaphore.
// 
	{
	static TInt32 ctr = 0x00000000;				// ctr to create unique thd names
	
	__PRINT(_L("WriteToDisk"));
	__PRINT1(_L("wtd:afn%S"), &aFileName);

	TDelayedWriterInit dwi;
	dwi.iFileName = &aFileName;
	dwi.iData = &aBuf;

	// Create local semaphore this thread can wait on until the child thread has
	// made copies of the file name and the store data.

	__PRINT(_L("wtd:cr sem"));
	TBuf<3 + 8> semName;
	semName.Format(_L("dws%08x"), ctr++);
	dwi.iSemName = &semName;
	RSemaphore svrSem;
	if (svrSem.CreateGlobal(semName, 0) != KErrNone)
		return;

	// Spin off a thread with a unique name.

	__PRINT(_L("wtd:cr thd"));
	TName nm;
	nm.Format(_L("dw%08x"), ctr);
	RThread t;
	TInt hminsz = Max(KHeapMinSize, aFileName.Length() + aBuf.Length() + 1024);
	if (t.Create(
		nm, DelayedWriter, KDefaultStackSize,
		hminsz /* aHeapMinSize */, hminsz /* aHeapMaxSize */, &dwi) == KErrNone)
		{
		__PRINT(_L("wtd:set pri"));
		t.SetPriority(EPriorityMuchLess);		// run as low priority task
		__PRINT(_L("wtd:res"));
		t.Resume();
		__PRINT(_L("wtd:wait"));
		svrSem.Wait();
		__PRINT(_L("wtd:cls thd"));
		t.Close();								// get rid of our handle
		}
	
	__PRINT(_L("wtd:cls sem"));
	svrSem.Close();
	}


LOCAL_D TInt DelayedWriter(TAny *aPtr)
//
// Main thread function for thread that is spun off from WriteToDisk().
// After local copies of the data have been allocated (or failed), tell
// the server to continue.
// 
	{
	__PRINT(_L("DelayedWriter"));

	User::SetCritical(User::ESystemCritical);

	TInt r;

	TDelayedWriterInit *dwi = (TDelayedWriterInit *) aPtr;
	RSemaphore svrSem;							// signal svr when data copied
	CTrapCleanup *th = NULL;					// thread trap handler
	TDelayedWriter *dw = NULL;					// thread copy of data
	RSemaphore queueSem;						// queued delayed write threads

	// Allocate a trap handler.
	__PRINT(_L("dlw:alc tp"));
	if ((th = CTrapCleanup::New()) == NULL)
		{
		r = KErrNoMemory;
		goto cleanup;
		}

	// Make copies of the filename and store data.
	__PRINT(_L("dlw:cp dat"));
	TRAP(r, dw = TDelayedWriter::NewL(dwi));
	if (r != KErrNone)
		goto cleanup;
	
	// Tell file server made local copies of data and so can continue.
	__PRINT(_L("dlw:sg cp dat"));
	if ((r = svrSem.OpenGlobal(*dwi->iSemName)) != KErrNone)
		goto cleanup;
	svrSem.Signal();

	// Wait for the other store threads to finish.
	__PRINT(_L("dlw:wait"));
	if ((r = queueSem.OpenGlobal(_L("dwsem"))) != KErrNone)
		goto cleanup;
	queueSem.Wait();

	// Write the data and signal the global semaphore so follow up threads can run.
	__PRINT(_L("dlw:wrt"));
	TRAP(r, DelayedWriterL(dw));
	__PRINT1(_L("dlw:wrt r = %d"), r);
	queueSem.Signal();

cleanup:										// free any opened resources
	__PRINT(_L("dlw:cln"));
	svrSem.Close();
	delete th;
	delete dw;
	queueSem.Close();

	return KErrNone;
	}


LOCAL_D void DelayedWriterL(const TDelayedWriter *aDW)
//
// Replace any existing store file; write data and set file as hidden and system.
//
	{
	__PRINT(_L("DelayedWriterL"));

	RFs fs;										// connect to the file server
	CleanupClosePushL(fs);
	User::LeaveIfError(fs.Connect());

	RFile f;									// replace any existing file
	CleanupClosePushL(f);

	__PRINT(_L("dlw: opn"));

	// Create the directory if it doesn't already exist
	TInt r;
	r = fs.MkDirAll(*aDW->iFileName);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		__PRINT(_L("dlw: MkDirAll err"));
		User::Leave(r);
		}

	User::LeaveIfError(f.Replace(fs, *aDW->iFileName, EFileShareExclusive | EFileStream | EFileWrite));
	__PRINT(_L("dlw: wrt"));
	User::LeaveIfError(f.Write(*aDW->iData));
	__PRINT(_L("dlw: sat"));
#ifndef __WINS__								// cannot replace hidden | system file in WINS.
	User::LeaveIfError(f.SetAtt(KEntryAttHidden | KEntryAttSystem, 0x00000000));
#endif
	__PRINT(_L("dlw: dst"));
	CleanupStack::PopAndDestroy(2);				// f, fs
	}

//
// TDelayedWriter
// 

TDelayedWriter *TDelayedWriter::NewL(const TDelayedWriterInit *dwi)
//
// static
// Allocates a TDelayedWriter structure on the thread's heap that is used to
// persist the data that the writer thread will need during its lifetime.
// 
	{
	__PRINT(_L("TDelayedWriter::NewL"));

	TDelayedWriter *self = new(ELeave) TDelayedWriter();
	CleanupStack::PushL(self);
	self->ConstructL(dwi);
	CleanupStack::Pop();						// self
	return self;
	}


TDelayedWriter::TDelayedWriter()
	{
	__PRINT(_L("TDelayedWriter::TDelayedWriter"));

	// empty.
	}


void TDelayedWriter::ConstructL(const TDelayedWriterInit *dwi)
//
// 2y initialisation.  Makes own copy of filename and data.
// Fields are not popped onto CleanupStack because will be deleted in dtor
// 
	{
	__PRINT(_L("TDelayedWriter::ConstructL"));

	iFileName = dwi->iFileName->AllocL();
	iData = dwi->iData->AllocL();
	}

TDelayedWriter::~TDelayedWriter()
//
// dtor - frees filename and store data that was allocated in ConstructL().
//
	{
	__PRINT(_L("TDelayedWriter::~TDelayedWriter"));

	delete iFileName;							// alloc in ConstructL()
	delete iData;								// alloc in server
	}

#else

EXPORT_C void WriteToDisk(const TDesC& /*aFileName*/, const TDesC8& /*aBuf*/)
//
//
//
	{}

#endif
