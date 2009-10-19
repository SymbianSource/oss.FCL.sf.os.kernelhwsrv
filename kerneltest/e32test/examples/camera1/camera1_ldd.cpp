// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// its implementation.
// 
//

/**
 @file An example camera device driver which uses Shared Chunks in
 @publishedPartner
 @prototype 9.1
*/

#include <kernel/kern_priv.h>
#include <kernel/cache.h>
#include "camera1.h"
#include "camera1_dev.h"

#if 0  // Set true for tracing
#define TRACE(x) x
#else
#define TRACE(x)
#endif

//
// DCamera1Factory
//

/**
  Standard export function for LDDs. This creates a DLogicalDevice derived object,
  in this case, our DCamera1Factory
*/
DECLARE_STANDARD_LDD()
	{
	return new DCamera1Factory;
	}

/**
  Constructor
*/
DCamera1Factory::DCamera1Factory()
	{
	// Set version number for this device
	iVersion=RCamera1::VersionRequired();
	// Indicate that do support units or a PDD
	iParseMask=0;
	}

/**
  Second stage constructor for DCamera1Factory.
  This must at least set a name for the driver object.

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DCamera1Factory::Install()
	{
	return SetName(&RCamera1::Name());
	}

/**
  Destructor
*/
DCamera1Factory::~DCamera1Factory()
	{
	}

/**
  Return the drivers capabilities.
  Called in the response to an RDevice::GetCaps() request.

  @param aDes User-side descriptor to write capabilities information into
*/
void DCamera1Factory::GetCaps(TDes8& aDes) const
	{
	// Create a capabilities object
	RCamera1::TCaps caps;
	caps.iVersion = iVersion;
	// Write it back to user memory
	Kern::InfoCopy(aDes,(TUint8*)&caps,sizeof(caps));
	}

/**
  Called by the kernel's device driver framework to create a Logical Channel.
  This is called in the context of the user thread (client) which requested the creation of a Logical Channel
  (E.g. through a call to RBusLogicalChannel::DoCreate)
  The thread is in a critical section.

  @param aChannel Set to point to the created Logical Channel

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DCamera1Factory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DCamera1Channel;
	if(!aChannel)
		return KErrNoMemory;

	return KErrNone;
	}

//
// Logical Channel
//

/**
  Default configuration for driver (640x480 pixels of 32bits captured at 15 frames/sec)
*/
static const RCamera1::TConfig DefaultConfig = {{640,480},4,15};

/**
  Constructor
*/
DCamera1Channel::DCamera1Channel()
	:	iDfcQ(Kern::TimerDfcQ()),  // This test uses the timer DFC queue for DFCs
		iStateChangeDfc(StateChangeDfcTrampoline,this,1),  // DFC is priority '1'
		iConfig(DefaultConfig),
		iCaptureTimer(CaptureDfcTrampoline,this)
	{
	}

/**
  Second stage constructor called by the kernel's device driver framework.
  This is called in the context of the user thread (client) which requested the creation of a Logical Channel
  (E.g. through a call to RBusLogicalChannel::DoCreate)
  The thread is in a critical section.

  @param aUnit The unit argument supplied by the client to RBusLogicalChannel::DoCreate
  @param aInfo The info argument supplied by the client to RBusLogicalChannel::DoCreate
  @param aVer The version argument supplied by the client to RBusLogicalChannel::DoCreate

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DCamera1Channel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	// Check client has EMultimediaDD capability
	if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by CAPTURE1")))
		return KErrPermissionDenied;

	// Check version
	if (!Kern::QueryVersionSupported(RCamera1::VersionRequired(),aVer))
		return KErrNotSupported;

	// Setup DFCs
	iStateChangeDfc.SetDfcQ(iDfcQ);

	// Done
	return Kern::MutexCreate(iStateChangeMutex,KNullDesC,KMutexOrdGeneral7);
	}

/**
  Destructor
*/
DCamera1Channel::~DCamera1Channel()
	{
	DoCancel(RCamera1::EAllRequests);
	EndCapture();
	iStateChangeDfc.Cancel();
	if(iStateChangeMutex)
		iStateChangeMutex->Close(0);
	if(iCaptureBuffers)
		iCaptureBuffers->Close();
	}

/**
  Process a request on this logical channel.

  @param aReqNo Request number:
  	            ==KMaxTInt, a 'DoCancel' message
	            >=0, a 'DoControl' message with function number equal to iValue
	            <0, a 'DoRequest' message with function number equal to ~iValue
  @param a1     First argument. For DoRequest requests this is a pointer to the TRequestStatus.
  @param a2     Second argument. For DoRequest this is a pointer to the 2 actual TAny* arguments.

  @return       Result. Ignored by device driver framework for DoRequest requests.
*/
TInt DCamera1Channel::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
	// Decode the message type and dispatch it to the relevent handler function...
	if ((TUint)aReqNo<(TUint)KMaxTInt)
		return DoControl(aReqNo,a1,a2);
	if(aReqNo==KMaxTInt)
		return DoCancel((TInt)a1);
	return DoRequest(aReqNo,a1,a2);
	}

/**
  Process synchronous 'control' requests
*/
TInt DCamera1Channel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	TRACE(Kern::Printf(">DCamera1Channel::DoControl fn=%d\n",aFunction);)

	(void)a2;   // a2 not used in this example

	TInt r = KErrNotSupported;
	switch (aFunction)
		{
		case RCamera1::EGetConfig:
			r = GetConfig((TDes8*)a1);
			break;

		case RCamera1::ESetConfig:
			r = SetConfig((const TDesC8*)a1);
			break;

		case RCamera1::EStartCapture:
			r = StartCapture();
			break;

		case RCamera1::EEndCapture:
			r = EndCapture();
			break;

		case RCamera1::EReleaseImage:
			r = ImageRelease((TInt)a1);
			break;

		case RCamera1::ECaptureImage:
			CaptureImage((TRequestStatus*)a1,(TInt)a2);
			break;
		}

	TRACE(Kern::Printf("<DCamera1Channel::DoControl result=%d\n",r);)

	return r;
	}

/**
  Process asynchronous requests.
  This driver doesn't have any 'DoRequest' requests because we handle asyncronous
  requests using 'DoControl' for performance reasons. I.e. to avoid having to read
  the arguments with kumemget()
*/
TInt DCamera1Channel::DoRequest(TInt aNotReqNo, TAny* a1, TAny* a2)
	{
	TRACE(Kern::Printf(">DCamera1Channel::DoRequest req=%d\n",aNotReqNo);)

	// Get arguments
	TAny* a[2];
	kumemget32(a,a2,sizeof(a)); 
	TRequestStatus* status=(TRequestStatus*)a1;
	TInt reqNo = ~aNotReqNo;

	// Do the request
	TInt r;
	switch(reqNo)
		{
		case RCamera1::ECaptureImage:
			// Not used because we do 'ECaptureImage' as a DoControl rather than
			// a DoRequest for performance reasons

		default:
			r = KErrNotSupported;
			break;
		}

	// Complete request if there was an error
	if (r!=KErrNone)
		Kern::RequestComplete(&Kern::CurrentThread(),status,r);

	TRACE(Kern::Printf("<DCamera1Channel::DoRequest result=%d\n",r);)

	return KErrNone;  // Result is ignored by device driver framework for DoRequest requests
	}

/**
  Process cancelling of asynchronous requests.
*/
TInt DCamera1Channel::DoCancel(TUint aMask)
	{
	TRACE(Kern::Printf(">DCamera1Channel::DoCancel mask=%08x\n",aMask);)

	if(aMask&(1<<RCamera1::ECaptureImage))
		CaptureImageCancel();

	TRACE(Kern::Printf("<DCamera1Channel::DoCancel\n");)

	return KErrNone;
	}

//
// Methods for processing configuration control messages
//

/**
  Process a GetConfig control message. This writes the current driver configuration to a
  RCamera1::TConfigBuf supplied by the client.
*/
TInt DCamera1Channel::GetConfig(TDes8* aConfigBuf)
	{
	// Write the config to the client
	Kern::InfoCopy(*aConfigBuf,(const TUint8*)&iConfig,sizeof(iConfig));
	return KErrNone;
	}

/**
  Process a SetConfig control message. This sets the driver configuration using a
  RCamera1::TConfigBuf supplied by the client.
*/
TInt DCamera1Channel::SetConfig(const TDesC8* aConfigBuf)
	{
	// Create a config structure.
	RCamera1::TConfig config(DefaultConfig);

	// Note: We have constructed a config using DefaultConfig, this is to allow
	// backwards compatibility when a client gives us an old (and shorter) version
	// of the config structure.

	// Read the config structure from client
	TPtr8 ptr((TUint8*)&config,sizeof(config));
	Kern::KUDesGet(ptr,*aConfigBuf);

	// For some settings we allow zero to mean default...
	if(!config.iImageSize.iWidth)
		config.iImageSize.iWidth = DefaultConfig.iImageSize.iWidth;
	if(!config.iImageSize.iHeight)
		config.iImageSize.iHeight = DefaultConfig.iImageSize.iHeight;
	if(!config.iImageBytesPerPixel)
		config.iImageBytesPerPixel = DefaultConfig.iImageBytesPerPixel;

	// Validate configuration
	TInt scale = DefaultConfig.iImageSize.iWidth/config.iImageSize.iWidth;
	if(scale*config.iImageSize.iWidth != DefaultConfig.iImageSize.iWidth)
		return KErrArgument;
	if(scale*config.iImageSize.iHeight != DefaultConfig.iImageSize.iHeight)
		return KErrArgument;
	if(config.iImageBytesPerPixel<=0 || config.iImageBytesPerPixel>4)
		return KErrArgument;

	if(config.iFrameRate<0)
		return KErrArgument;
	if(config.iNumImageBuffers<1)
		return KErrArgument;

	TInt imageSize;
	DCaptureBuffers* buffers;
	TInt r;

	// Need to be in critical section whilst holding a DMutex
	NKern::ThreadEnterCS();

	// Claim state change mutex. Note, the return value is ignored because a Wait
	// can only fail if the mutex is destroyed whilst waiting for it, this can't 
	// happen in our driver.
	Kern::MutexWait(*iStateChangeMutex);

	// Check we aren't in the middle of capturing images
	if(iCapturing)
		{
		r = KErrInUse;
		goto done;
		}

	// Change the config
	iConfig = config; 
	iCaptureRateTicks = config.iFrameRate ? 1000000/config.iFrameRate/NKern::TickPeriod() : KMaxTInt;
	if(iCaptureRateTicks<1)
		iCaptureRateTicks = 1;

	// Claim ownership of old buffers
	NKern::FMWait(&iCaptureMutex);
	buffers = iCaptureBuffers;
	iCaptureBuffers = NULL;
	NKern::FMSignal(&iCaptureMutex);

	// Delete old buffers
	if(buffers)
		buffers->Close();

	// Contruct new buffer object
	imageSize = iConfig.iImageSize.iWidth*iConfig.iImageSize.iHeight*iConfig.iImageBytesPerPixel;
	buffers = DCaptureBuffers::New(2+iConfig.iNumImageBuffers,imageSize);
	if(!buffers)
		{
		r = KErrNoMemory;
		goto done;
		}

	// Use the new buffers if another thread didn't create them first
	NKern::FMWait(&iCaptureMutex);
	iCaptureBuffers = buffers;
	NKern::FMSignal(&iCaptureMutex);

	// Create handle for chunk
	r = Kern::MakeHandleAndOpen(NULL, iCaptureBuffers->iChunk);

done:
	// Release state change mutex
	Kern::MutexSignal(*iStateChangeMutex);

	NKern::ThreadLeaveCS();

	return r;
	}

//
// Methods for processing start/end capture
//

/**
   Start image capturing
*/
TInt DCamera1Channel::StartCapture()
	{
	// Need to be in critical section whilst holding a DMutex
	NKern::ThreadEnterCS();

	// Claim state change mutex. Note, the return value is ignored because a Wait
	// can only fail if the mutex is destroyed whilst waiting for it, this can't 
	// happen in our driver.
	Kern::MutexWait(*iStateChangeMutex);

	NKern::FMWait(&iCaptureMutex);

	TInt r;
	if(!iCaptureBuffers)
		r = KErrNotReady;  // SetConfig not yet been called
	else if(iCapturing)
		r = KErrInUse;     // StartCapture has already been called
	else
		{
		// Initialise image buffer state for capturing images
		iCaptureBuffers->Reset();

		// Flag capturing started
		iCapturing = ETrue;
		r = KErrNone;
		}

	NKern::FMSignal(&iCaptureMutex);

	// Get state change DFC to initialise camera hardware for capture
	if(r==KErrNone)
		StateChange(ETrue);

	// Release state change mutex
	Kern::MutexSignal(*iStateChangeMutex);

	NKern::ThreadLeaveCS();

	return r;
	}

/**
   End image capturing
*/
TInt DCamera1Channel::EndCapture()
	{
	// Need to be in critical section whilst holding a DMutex
	NKern::ThreadEnterCS();

	// Claim state change mutex. Note, the return value is ignored because a Wait
	// can only fail if the mutex is destroyed whilst waiting for it, this can't 
	// happen in our driver.
	Kern::MutexWait(*iStateChangeMutex);

	if(iCapturing)
		{
		// Get state change DFC to reset camera hardware
		StateChange(EFalse);

		// Flag capture ended
		NKern::FMWait(&iCaptureMutex);
		iCapturing = EFalse;
		NKern::FMSignal(&iCaptureMutex);

		// Cancel any pending caoture request
		CaptureImageCancel();
		}

	// Release state change mutex
	Kern::MutexSignal(*iStateChangeMutex);

	NKern::ThreadLeaveCS();

	return KErrNone;
	}

/**
  Performs state change on Start/EndCapture by calling state change DFC
  Call with iStateChangeMutex held.

  @param aNewState True to start image capture, false to stop image capture.
*/
void DCamera1Channel::StateChange(TBool aNewState)
	{
	iNewState = aNewState;
	NKern::FSSetOwner(&iStateChangeSemaphore,NULL);
	iStateChangeDfc.Enque();
	NKern::FSWait(&iStateChangeSemaphore);
	}

/**
  DFC callback called when Start/EndCapture requests are made.
*/
void DCamera1Channel::StateChangeDfcTrampoline(TAny* aSelf)
	{
	// Just call non-static method
	((DCamera1Channel*)aSelf)->StateChangeDfc();
	}

/**
  DFC callback called when Start/EndCapture requests are made.
*/
void DCamera1Channel::StateChangeDfc()
	{
	TRACE(Kern::Printf(">DCamera1Channel::StateChangeDfc\n");)

	// Call relevent state change function
	if(iNewState)
		DoStartCapture();
	else
		DoEndCapture();

	// Signal completion
	NKern::FSSignal(&iStateChangeSemaphore);

	TRACE(Kern::Printf("<DCamera1Channel::StateChangeDfc\n");)
	}

//
// Methods for processing CaptureImage
//

/**
  Process Capture Image request 
*/
void DCamera1Channel::CaptureImage(TRequestStatus* aRequestStatus,TInt aReleaseImage)
	{
	TInt r=KErrNone;

	// Get the thread making the request
	DThread* requestThread = &Kern::CurrentThread();

	// Release image (if one was specified)
	if(aReleaseImage!=-1)
		{
		r = ImageRelease(aReleaseImage);
		if(r!=KErrNone)
			goto done;
		}

	NKern::FMWait(&iCaptureMutex);

	if(!iCapturing)
		r = KErrNotReady;     // StartCapture hasn't yet been called
	else if(iCaptureRequestStatus)
		r = KErrInUse;        // There is already a pending CaptureImage request
	else
		{
		// See if an image is already available...
		DImageBuffer* buffer=iCaptureBuffers->ImageForClient();
		if(buffer)
			{
			// Return offset of buffer to client
			r = buffer->iChunkOffset;
			}
		else
			{
			// Image not found...
			if(!iCaptureBuffers->iFreeBuffers[0])
				r = KErrOverflow;  // Out of buffers
			else
				{
				// Wait for new image to become available
				iCaptureRequestStatus = aRequestStatus;
				requestThread->Open(); // can't fail because this is the current thread
				iCaptureRequestThread = requestThread;
				r = KErrNone;
				}
			}
		}

	NKern::FMSignal(&iCaptureMutex);

done:
	// Complete request if there was an error
	if (r!=KErrNone)
		Kern::RequestComplete(requestThread,aRequestStatus,r);
	}

/**
  Signal Capture Image request completed
*/
void DCamera1Channel::CaptureImageCancel()
	{
	// Need to be in critical section so we don't die whilst owning the capture image request
	NKern::ThreadEnterCS();

	// Claim the capture image request
	NKern::FMWait(&iCaptureMutex);
	DThread* thread = iCaptureRequestThread;;
	TRequestStatus* status = iCaptureRequestStatus;
	iCaptureRequestStatus = NULL;
	NKern::FMSignal(&iCaptureMutex);

	// Signal completion
	if(status)
		{
		Kern::RequestComplete(thread,status,KErrCancel);
		thread->Close(0);
		}

	NKern::ThreadLeaveCS();
	}

/**
  DFC callback called when after a new image has been captured
  In this example code this is called by
*/
void DCamera1Channel::CaptureDfcTrampoline(TAny* aSelf)
	{
	// Just call non-static method
	((DCamera1Channel*)aSelf)->CaptureDfc();
	}

/**
  DFC callback called when a new image has been captured
*/
void DCamera1Channel::CaptureDfc()
	{
	TRACE(Kern::Printf(">DCamera1Channel::CaptureDfc\n");)

	NKern::FMWait(&iCaptureMutex);

	// Update image buffers state
	iCaptureBuffers->ImageCaptured();

	// Did client request an image and is one available?
	DImageBuffer* clientBuffer;
	if(iCaptureRequestStatus && (clientBuffer=iCaptureBuffers->ImageForClient())!=NULL )
		{
		// Claim the client request
		DThread* thread = iCaptureRequestThread;
		TRequestStatus* status = iCaptureRequestStatus;
		iCaptureRequestStatus = NULL;

		NKern::FMSignal(&iCaptureMutex);

		// We now own the client request but we don't have to worry about
		// being in a critical section because we are running in a DFC thread
		// which can't be killed

		// Complete client request with the chunk offset for a captured image
		// (We use AsyncClose() here because we are running in a high priority DFC and
		// don't want to take the penalty for possibly deleting a thread in this context.)
		Kern::RequestComplete(thread,status,clientBuffer->iChunkOffset);
		thread->AsyncClose();
		}
	else
		NKern::FMSignal(&iCaptureMutex);

	// Get camera hardware to capture next image
	DoNextCapture();

	TRACE(Kern::Printf("<DCamera1Channel::CaptureDfc\n");)
	}

/**
  Release a buffer which was being used by client

  @param aChunkOffset The chunk offset corresponding to the buffer to be freed

  @return KErrNone if successful.
		  KErrNotFound if no 'in use' buffer had the specified chunk offset.
*/
TInt DCamera1Channel::ImageRelease(TInt aChunkOffset)
	{
	// Need to be in critical section so we don't die whilst holding reference on buffers
	NKern::ThreadEnterCS();

	// Get reference to buffers object and find the buffer we want
	NKern::FMWait(&iCaptureMutex);
	DCaptureBuffers* buffers = iCaptureBuffers;
	DImageBuffer* buffer = NULL;
	if(buffers)
		{
		buffers->Open();
		buffer = buffers->InUseImage(aChunkOffset);
		}
	NKern::FMSignal(&iCaptureMutex);

	TInt r;
	if(!buffer)
		r = KErrNotFound;	// Buffer not found
	else
		{
		// Purge the CPU cache for the buffer.
		// Note, we don't do this whilst holding iCaptureMutex because it can
		// take a long time.
		// Also, it doesn't mater that e aren't holding the mutex because:
		// 1. The buffer can't be delete because we have a reference count on iCaptureBuffers
		// 2. Reentrancy of the Purge method is safe 
		buffers->Purge(buffer);

		// Release buffer (move it to the free list)
		NKern::FMWait(&iCaptureMutex);
		r = buffers->ImageRelease(aChunkOffset) ? KErrNone : KErrArgument;
		NKern::FMSignal(&iCaptureMutex);
		}

	// Close reference on buffers
	if(buffers)
		buffers->Close();

	NKern::ThreadLeaveCS();

	return r;
	}

//
// DCaptureBuffers
//

/**
  Construct a new set of buffers

  @param aNumBuffers Number of buffers
  @param aBufferSize Size of each buffer in bytes

  @return Pointer to the created DCaptureBuffers or NULL if the system ran out of memory
*/
DCaptureBuffers* DCaptureBuffers::New(TInt aNumBuffers,TInt aBufferSize)
	{
	DCaptureBuffers* buffers = new DCaptureBuffers;
	if(buffers)
		{
		TInt r = buffers->Create(aNumBuffers,aBufferSize);
		if(r==KErrNone)
			return buffers;
		delete buffers;
		// An error other than 'no memory' must be a programming error in the driver
		__NK_ASSERT_ALWAYS(r==KErrNoMemory);
		}
	return NULL;
	}

/**
  Construct with access count of one
*/
DCaptureBuffers::DCaptureBuffers()
	: iAccessCount(1)
	{
	}

/**
  Create all buffers and lists
*/
TInt DCaptureBuffers::Create(TInt aNumBuffers,TInt aBufferSize)
	{
	// Allocate buffer lists
	DImageBuffer** lists = (DImageBuffer**)Kern::AllocZ(3*aNumBuffers*sizeof(DImageBuffer*));
	if(!lists)
		return KErrNoMemory;
	iBufferLists = lists;
	iFreeBuffers = lists;
	iCompletedBuffers = lists+aNumBuffers;
	iInUseBuffers = lists+2*aNumBuffers;

	// Calculate sizes
	aBufferSize = Kern::RoundToPageSize(aBufferSize);
	TInt pageSize = Kern::RoundToPageSize(1);
	TUint64 chunkSize = TUint64(aBufferSize+pageSize)*aNumBuffers+pageSize;
	if(chunkSize>(TUint64)KMaxTInt)
		return KErrNoMemory;  // Need more than 2GB of memory!

	// Create chunk
	TChunkCreateInfo info;
	info.iType = TChunkCreateInfo::ESharedKernelMultiple;
	info.iMaxSize = (TInt)chunkSize;
#ifndef __WINS__
	info.iMapAttr = EMapAttrCachedMax;
#else
	info.iMapAttr = 0;
#endif
	info.iOwnsMemory = ETrue;
	TInt r = Kern::ChunkCreate(info,iChunk,iChunkBase,iChunkMapAttr);
	if(r!=KErrNone)
		return r;

	// Construct array of buffers
	iNumBuffers = aNumBuffers;
	iImageBuffer = new DImageBuffer[aNumBuffers];
	if(!iImageBuffer)
		return KErrNoMemory;

	// Create each buffer
	TInt offset = pageSize;
	while(aNumBuffers)
		{
		r = iImageBuffer[--aNumBuffers].Create(iChunk,offset,aBufferSize);
		if(r!=KErrNone)
			return r;
		offset += aBufferSize+pageSize;
		}

	return KErrNone;
	}

/**
  Destructor
*/
DCaptureBuffers::~DCaptureBuffers()
	{
	if(iChunk)
		Kern::ChunkClose(iChunk);
	delete [] iImageBuffer;
	Kern::Free(iBufferLists);
	}

/**
  Increment access count of buffers
*/
void DCaptureBuffers::Open()
	{
	__e32_atomic_tas_ord32(&iAccessCount, 1, 1, 0);
	}

/**
  Decrement access count of buffers.
  Deleting them if the count is decremented to zero.
*/
void DCaptureBuffers::Close()
	{
	__ASSERT_NO_FAST_MUTEX;
	__ASSERT_CRITICAL;
	if(__e32_atomic_tas_ord32(&iAccessCount, 1, -1, 0) == 1)
		AsyncDelete();
	}

/**
  Reset all image buffer lists to reflect the state at the start of image capture process
*/
void DCaptureBuffers::Reset()
	{
	// Purge cache for all buffers in use by client.
	DImageBuffer** list = iInUseBuffers;
	DImageBuffer* buffer;
	while((buffer=*list++)!=NULL)
		Purge(buffer);

	// Get pointers to first buffer
	buffer = iImageBuffer; 

	// Set buffers for current and next images
	iCurrentBuffer = buffer++;
	iNextBuffer = buffer++;

	// Add all other buffers to the free list
	DImageBuffer** free = iFreeBuffers;
	DImageBuffer* bufferLimit = iImageBuffer+iNumBuffers; 
	while(buffer<bufferLimit)
		*free++ = buffer++;
	*free = 0;

	// Start with no completed or used buffers
	iCompletedBuffers[0] = 0;
	iInUseBuffers[0] = 0;
	}

/**
  Purge cache for an image buffer.
  @param aBuffer The buffer.
*/
void DCaptureBuffers::Purge(DImageBuffer* aBuffer)
	{
	Cache::SyncMemoryBeforeDmaRead(iChunkBase+aBuffer->iChunkOffset,aBuffer->iSize,iChunkMapAttr);
	}

/**
  Remove an image buffer to the start of the given image list.
  @return A pointer to the image buffer or NULL if the list was empty
*/
DImageBuffer* DCaptureBuffers::Remove(DImageBuffer** aList)
	{
	DImageBuffer* buffer=aList[0];
	if(buffer)
		{
		DImageBuffer* b;
		do
			{
			b=aList[1];
			*aList++ = b;
			}
		while(b);
		}
	return buffer;
	}

/**
  Add an image buffer to the end of the given image list.
*/
DImageBuffer* DCaptureBuffers::Add(DImageBuffer** aList, DImageBuffer* aBuffer)
	{
	while(*aList) aList++;
	*aList = aBuffer;
	return aBuffer;
	}

/**
  Update buffer lists after an image has been captured.
  @return A pointer to the catptured image buffer
*/
DImageBuffer* DCaptureBuffers::ImageCaptured()
	{
	// Add captured image to completed list
	DImageBuffer* buffer = iCurrentBuffer;
	DCaptureBuffers::Add(iCompletedBuffers,buffer);

	// Make queued buffer the current one
	iCurrentBuffer = iNextBuffer;

	// Queue a new buffer
	iNextBuffer = DCaptureBuffers::Remove(iFreeBuffers);
	if(!iNextBuffer)
		iNextBuffer = DCaptureBuffers::Remove(iCompletedBuffers);

	TRACE(Kern::Printf("DCaptureBuffers::ImageCaptured  buf=%08x\n",buffer->iChunkOffset);)

	return buffer;
	}

/**
  Get the next image from the completed capture list and make it 'in use' by the client

  @return A pointer to the next completed image buffer
*/
DImageBuffer* DCaptureBuffers::ImageForClient()
	{
	DImageBuffer* buffer=Remove(iCompletedBuffers);
	if(buffer)
		DCaptureBuffers::Add(iInUseBuffers,buffer);

	TRACE(Kern::Printf("DCaptureBuffers::ImageForClient buf=%08x\n",buffer ? buffer->iChunkOffset : -1);)

	return buffer;
	}

/**
  Release (move to free list) the 'in use' image specified by the given chunk offset.

  @param aChunkOffset The chunk offset corresponding to the buffer to be freed

  @return The freed image buffer, or NULL if no 'in use' buffer had the specified chunk offset.
*/
DImageBuffer* DCaptureBuffers::ImageRelease(TInt aChunkOffset)
	{
	// Scan 'in use' list for the image buffer
	DImageBuffer** list = iInUseBuffers;
	DImageBuffer* buffer;
	while((buffer=*list++)!=NULL && buffer->iChunkOffset!=aChunkOffset)
		{};

	// Move buffer to the free list (if found)
	if(buffer)
		buffer = Add(iFreeBuffers,Remove(list-1));

	TRACE(Kern::Printf("DCaptureBuffers::ImageRelease   buf=%08x\n",buffer ? buffer->iChunkOffset : -1);)

	return buffer;
	}

/**
  Find the 'in use' image specified by the given chunk offset

  @param aChunkOffset The chunk offset corresponding to the buffer to be freed

  @return The image buffer, or NULL if no 'in use' buffer had the specified chunk offset
*/
DImageBuffer* DCaptureBuffers::InUseImage(TInt aChunkOffset)
	{
	// Scan 'in use' list for the image buffer
	DImageBuffer** list = iInUseBuffers;
	DImageBuffer* buffer;
	while((buffer=*list++)!=NULL && buffer->iChunkOffset!=aChunkOffset)
		{};

	return buffer;
	}

//
// DImageBuffer
//

/**
  Constructor clears all member data
*/
DImageBuffer::DImageBuffer()
	{
	memclr(this,sizeof(*this));
	}

/**
  Commit memory for this buffer.

  @param aChunk  The chunk into which the memory is to be commited
  @param aOffset The offset within aChunk for the start of the comitted memory.
                 Must be a multiple of the MMU page size.
  @param aSize   The number of bytes of memory to commit.
                 Must be a multiple of the MMU page size.

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DImageBuffer::Create(DChunk* aChunk, TInt aOffset, TInt aSize)
	{
	TInt r;

	// Initialise data
	iChunkOffset = aOffset;
	iSize = aSize;

	// Try for physically contiguous memory first
	r = Kern::ChunkCommitContiguous(aChunk,aOffset,aSize,iPhysicalAddress);
	if(r==KErrNone)
		return r;

	// failed to get contiguous memory...

	// Mark physical address invalid
	iPhysicalAddress = KPhysAddrInvalid;

	// Commit discontiguous memory
	r = Kern::ChunkCommit(aChunk,aOffset,aSize);
	if(r!=KErrNone)
		return r;

	// Allocate array for list of physical pages
	iPhysicalPages = new TPhysAddr[aSize/Kern::RoundToPageSize(1)];
	if(!iPhysicalPages)
		return KErrNoMemory;

	// Get physical addresses of pages in buffer
	TUint32 kernAddr;
	TUint32 mapAttr;
	TPhysAddr physAddr;
	r = Kern::ChunkPhysicalAddress(aChunk,aOffset,aSize,kernAddr,mapAttr,physAddr,iPhysicalPages);
	// r = 0 or 1 on success. (1 meaning the physical pages are not-contiguous)
	if(r>=0)
		r = KErrNone;
	return r;
	}

/**
  Destructor
*/
DImageBuffer::~DImageBuffer()
	{
	delete [] iPhysicalPages;
	}

//
// Program camera hardware
//

/**
  Initialise camera hardware to start capturing images
  First buffer to fill is iCaptureBuffers->iCurrentBuffer.
  Next buffer to fill will be iCaptureBuffers->iNextBuffer.
*/
void DCamera1Channel::DoStartCapture()
	{
	// For this example test...
	
	TRACE(Kern::Printf("DCamera1Channel::DoStartCapture buf=%08x cnt=%04d\n",iCaptureBuffers->iCurrentBuffer->iChunkOffset,iCaptureCounter);)

	// Initialise frame counter
	iCaptureCounter = 0;

	// Put frame counter into current image buffer. (This is the 'image' data we capture).
	*(TInt*)(iCaptureBuffers->iChunkBase+iCaptureBuffers->iCurrentBuffer->iChunkOffset) = iCaptureCounter++;

	// Start the timer
	TInt r=iCaptureTimer.OneShot(iCaptureRateTicks,ETrue);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	}

/**
  Reset camera hardware to stop capturing images
*/
void DCamera1Channel::DoEndCapture()
	{
	// For this example test...

	TRACE(Kern::Printf("DCamera1Channel::DoEndCapture\n");)

	// Cancel the timer
	iCaptureTimer.Cancel();
	}

/**
  Setup camera hardware to capture next image
  Next buffer to fill will be iCaptureBuffers->iNextBuffer;

  @param aLastImage The last image just captured. I.e. the completed capture which caused
					this method to be called
*/
void DCamera1Channel::DoNextCapture()
	{
	// For this example test...
	
	TRACE(Kern::Printf("DCamera1Channel::DoNextCapture  cur=%08x cnt=%04d nxt=%08x\n",iCaptureBuffers->iCurrentBuffer->iChunkOffset,iCaptureCounter,iCaptureBuffers->iNextBuffer->iChunkOffset);)

	// Put frame counter into current image buffer. (This is the 'image' data we capture).
	*(TInt*)(iCaptureBuffers->iChunkBase+iCaptureBuffers->iCurrentBuffer->iChunkOffset) = iCaptureCounter++;

	// Restart the timer
	TInt r = iCaptureTimer.Again(iCaptureRateTicks);
	if(r==KErrArgument)
		{
		// Timer would have already expired.
		//
		// In a real device driver this is analogous to iCurrentBuffer already being filled
		// and the DMA queue being emptied. I.e. we have missed some frames.
		//
		// For this test...

		TRACE(Kern::Printf("DCamera1Channel::DoNextCapture frame dropped cnt=%04d\n",iCaptureCounter);)

		// Skip a frame count
		++iCaptureCounter;

		// Restart timer
		r = iCaptureTimer.OneShot(iCaptureRateTicks,ETrue);
		}
	__NK_ASSERT_ALWAYS(r==KErrNone);
	}

