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
 @file An example data converter device driver which uses Shared Chunks in
 @publishedPartner
 @prototype 9.1
*/

#include <kernel/kern_priv.h>
#include <kernel/cache.h>
#include "convert1.h"
#include "convert1_dev.h"


#if 0  // Set true for tracing
#define TRACE(x) x
#else
#define TRACE(x)
#endif


_LIT(KConvert1PanicCategory,"CONVERT1");


//
// DConvert1Factory
//

/**
  Number of hardware 'resources' available to driver.
  E.g. the number of simultaneous channels it can support.
*/
const TInt KTotalConvert1Resources = 4;

/**
  A resource ID representing no resources
*/
const TInt KNullConvert1ResourceId = -1;

/**
  Standard export function for LDDs. This creates a DLogicalDevice derived object,
  in this case, our DConvert1Factory
*/
DECLARE_STANDARD_LDD()
	{
	return new DConvert1Factory;
	}

/**
  Constructor
*/
DConvert1Factory::DConvert1Factory()
	{
	// Set version number for this device
	iVersion=RConvert1::VersionRequired();
	// Indicate that do support units or a PDD
	iParseMask=0;
	// Mark all resources available
	iResourceFlags = (1<<KTotalConvert1Resources)-1;
	}

/**
  Second stage constructor for DConvert1Factory.
  This must at least set a name for the driver object.

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DConvert1Factory::Install()
	{
	return SetName(&RConvert1::Name());
	}

/**
  Destructor
*/
DConvert1Factory::~DConvert1Factory()
	{
	}

/**
  Return the drivers capabilities.
  Called in the response to an RDevice::GetCaps() request.

  @param aDes User-side descriptor to write capabilities information into
*/
void DConvert1Factory::GetCaps(TDes8& aDes) const
	{
	// Create a capabilities object
	RConvert1::TCaps caps;
	caps.iVersion = iVersion;
	caps.iMaxChannels = KTotalConvert1Resources;

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
TInt DConvert1Factory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DConvert1Channel(this);
	if(!aChannel)
		return KErrNoMemory;

	return KErrNone;
	}

/**
  Claim a hardware resource. This example driver has KTotalConvert1Resources
  'hardware resources' and returns the ID of the next unallocated one.
*/
TInt DConvert1Factory::ClaimResource(TInt& aResourceId)
	{
	// Wait on mutex protecting resource allocation
	NKern::FMWait(&iResourceMutex);

	// Search for a free resource
	TUint resourceFlags = iResourceFlags;
	TUint mask = 1;
	TInt id = 0;
	do
		{
		if(resourceFlags&mask)
			break;
		mask <<= 1;
		}
	while(++id<KTotalConvert1Resources);

	if(resourceFlags&mask)
		iResourceFlags = resourceFlags&~mask; // Found resource, so mark it in use
	else
		id = KNullConvert1ResourceId; // No resource available

	// Set returned resource id
	aResourceId = id;

	// Release mutex protecting resource allocation
	NKern::FMSignal(&iResourceMutex);

	return id<0 ? KErrInUse : KErrNone;
	}

/**
  Released the hardware resource indicated by the given id
*/
void DConvert1Factory::ReleaseResource(TInt aResourceId)
	{
	// Do nothing if the null id was given
	if(aResourceId==KNullConvert1ResourceId)
		return;

	// Wait on mutex protecting resource allocation
	NKern::FMWait(&iResourceMutex);

	// Check for valid resource and that it is not already free
	__NK_ASSERT_DEBUG(TUint(aResourceId)<TUint(KTotalConvert1Resources));
	__NK_ASSERT_DEBUG((iResourceFlags&(1<<aResourceId))==0);

	// Flag resource free again
	iResourceFlags |= 1<<aResourceId;

	// Release mutex protecting resource allocation
	NKern::FMSignal(&iResourceMutex);
	}

//
// Logical Channel
//

/**
  Default configuration (4k buffer, No Input Chunk, 1MB/sec speed)
*/
static const RConvert1::TConfig DefaultConfig = {4<<10,EFalse,1<<20};

/**
  Constructor
*/
DConvert1Channel::DConvert1Channel(DConvert1Factory* aFactory)
	:	iFactory(aFactory), 
		iResourceId(KNullConvert1ResourceId),
		iConfig(DefaultConfig),
		iConvertTimer(ConvertDfcTrampoline,this)
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
TInt DConvert1Channel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	// Check client has EMultimediaDD capability
	if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by CAPTURE1")))
		return KErrPermissionDenied;

	// Check version
	if (!Kern::QueryVersionSupported(RConvert1::VersionRequired(),aVer))
		return KErrNotSupported;

	// Claim ownership of a hardware resource
	TInt r=iFactory->ClaimResource(iResourceId);
	if(r!=KErrNone)
		return r;

	// Set client thread with which channel will be used by
	iClient = &Kern::CurrentThread();

	// Done
	return KErrNone;
	}

/**
  Destructor
*/
DConvert1Channel::~DConvert1Channel()
	{
	// Cancel outsatnding requests
	DoCancel(RConvert1::EAllRequests);

	// Release hardware resource which we own
	iFactory->ReleaseResource(iResourceId);
	}

/**
  Called when a user thread requests a handle to this channel.
*/
TInt DConvert1Channel::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	// Make sure that only our client can get a handle
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
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
TInt DConvert1Channel::Request(TInt aReqNo, TAny* a1, TAny* a2)
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
TInt DConvert1Channel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	TRACE(Kern::Printf(">DConvert1Channel::DoControl fn=%d\n",aFunction);)

	TInt r = KErrNotSupported;
	switch (aFunction)
		{
		case RConvert1::EGetConfig:
			r = GetConfig((TDes8*)a1);
			break;

		case RConvert1::ESetConfig:
			r = SetConfig((const TDesC8*)a1,(RConvert1::TBufferInfo*)a2);
			break;

		case RConvert1::EConvertDes:
			ConvertDes((const TDesC8*)a1,(TRequestStatus*)a2);
			break;

		case RConvert1::EConvertChunk:
			ConvertChunk((const RConvert1::TConvertArgs*)a1,(TRequestStatus*)a2);
			break;

		case RConvert1::EConvertInChunk:
			ConvertInChunk((TInt)a1,(TRequestStatus*)a2);
			break;
		}

	TRACE(Kern::Printf("<DConvert1Channel::DoControl result=%d\n",r);)

	return r;
	}

/**
  Process asynchronous requests.
  This driver doesn't have any 'DoRequest' requests because we handle asyncronous
  requests using 'DoControl' for performance reasons. I.e. to avoid having to read
  the arguments with kumemget()
*/
TInt DConvert1Channel::DoRequest(TInt aNotReqNo, TAny* a1, TAny* a2)
	{
	TRACE(Kern::Printf(">DConvert1Channel::DoRequest req=%d\n",aNotReqNo);)

	// Get arguments
	TAny* a[2];
	kumemget32(a,a2,sizeof(a)); 
	TRequestStatus* status=(TRequestStatus*)a1;
	TInt reqNo = ~aNotReqNo;

	// Do the request
	TInt r;
	switch(reqNo)
		{
		case RConvert1::EConvertDes:
		case RConvert1::EConvertChunk:
		case RConvert1::EConvertInChunk:
			// Not used because we do these asyncronous request as a
			// DoControl rather than a DoRequest for performance reasons.

		default:
			r = KErrNotSupported;
			break;
		}

	// Complete request if there was an error
	if (r!=KErrNone)
		Kern::RequestComplete(&Kern::CurrentThread(),status,r);

	TRACE(Kern::Printf("<DConvert1Channel::DoRequest result=%d\n",r);)

	return KErrNone;  // Result is ignored by device driver framework for DoRequest requests
	}

/**
  Process cancelling of asynchronous requests.
*/
TInt DConvert1Channel::DoCancel(TUint aMask)
	{
	TRACE(Kern::Printf(">DConvert1Channel::DoCancel mask=%08x\n",aMask);)

	if(aMask&( (1<<RConvert1::EConvertDes) | (1<<RConvert1::EConvertChunk) | (1<<RConvert1::EConvertInChunk) ) )
		ConvertCancel();

	TRACE(Kern::Printf("<DConvert1Channel::DoCancel\n");)

	return KErrNone;
	}

//
// Methods for processing configuration control messages
//

/**
  Process a GetConfig control message. This writes the current driver configuration to a
  RConvert1::TConfigBuf supplied by the client.
*/
TInt DConvert1Channel::GetConfig(TDes8* aConfigBuf)
	{
	// Write the config to the client
	Kern::InfoCopy(*aConfigBuf,(const TUint8*)&iConfig,sizeof(iConfig));
	return KErrNone;
	}

/**
  Process a SetConfig control message. This sets the driver configuration using a
  RConvert1::TConfigBuf supplied by the client.
*/
TInt DConvert1Channel::SetConfig(const TDesC8* aConfigBuf,RConvert1::TBufferInfo* aBufferInfo)
	{
	// Create a config structure.
	RConvert1::TConfig config(DefaultConfig);

	// Note: We have constructed a config using DefaultConfig, this is to allow
	// backwards compatibility when a client gives us an old (and shorter) version
	// of the config structure.

	// Read the config structure from client
	TPtr8 ptr((TUint8*)&config,sizeof(config));
	Kern::KUDesGet(ptr,*aConfigBuf);

	// 'info' is the data we will return to client at the end
	RConvert1::TBufferInfo info;
	memclr(&info,sizeof(info));

	TInt r;

	// Need to be in critical section whilst allocating objects
	NKern::ThreadEnterCS();

	// Check we aren't in the middle of converting data
	if(iConvertRequestStatus)
		{
		r = KErrInUse;
		goto done;
		}

	// Note: The above check is enough to ensure we have exclusive access
	// to this channels buffer and hardware resources because:
	// 1. The covert DFC can't run because we haven't started converting yet.
	// 2. No other request can come in because the channel only allows one
	//    client thread to use it. See DConvert1Channel::Request()
	// 3. The channel destructor can't be called whilst we are processing a request.


	// For some settings we allow zero to mean default...
	if(!config.iBufferSize)
		config.iBufferSize = DefaultConfig.iBufferSize;
	if(!config.iSpeed)
		config.iSpeed = DefaultConfig.iSpeed;

	// Validate configuration
	if(config.iBufferSize<=0)
		{
		r = KErrArgument;
		goto done;
		}
	if(config.iSpeed<=0)
		{
		r = KErrArgument;
		goto done;
		}

	// Change the config
	iConfig = config; 

	{

	// Calculate buffer size
	TInt bufferSize = Kern::RoundToPageSize(config.iBufferSize);

	// Destroy old buffers
	iClientBuffer.Destroy();
	iInBuffer.Destroy();
	iOutBuffer.Destroy();

	// Setup iClientBuffer
	r = iClientBuffer.SetMaxSize(bufferSize);
	if(r!=KErrNone)
		goto done;

	// Create output buffer
	r = iOutBuffer.Create(bufferSize);
	if(r!=KErrNone)
		goto done;
	// Make handle for output buffer
	r = Kern::MakeHandleAndOpen(NULL, iOutBuffer.iChunk);
	if(r<0) // -ve value is error, +ve value is a handle
		goto done;
	info.iOutChunkHandle = r;
	r = KErrNone;

	// Create input buffer if requested
	if(iConfig.iCreateInputChunk)
		{
		r = iInBuffer.Create(bufferSize);
		if(r!=KErrNone)
			goto done;
		// Make handle for input buffer
		r = Kern::MakeHandleAndOpen(NULL, iInBuffer.iChunk);
		if(r<0) // -ve value is error, +ve value is a handle
			goto done;
		info.iInChunkHandle = r;
		r = KErrNone;
		// Set info about input buffer
		//
		// Note we don't set iInBufferPtr because this is the address in
		// client process which it must set for itself
		info.iInBufferOffset = iInBuffer.iChunkOffset;
		info.iInBufferSize = iInBuffer.iMaxSize;
		}
	}
done:
	// Cleanup if there was an error
	if(r!=KErrNone)
		{
		iClientBuffer.Destroy();
		iInBuffer.Destroy();
		iOutBuffer.Destroy();
		if(info.iOutChunkHandle)
			Kern::CloseHandle(NULL,info.iOutChunkHandle);
		if(info.iInChunkHandle)
			Kern::CloseHandle(NULL,info.iInChunkHandle);
		memclr(&info,sizeof(info));
		}

	NKern::ThreadLeaveCS();

	// Write chunk handles and other info back to client memory
	kumemput32(aBufferInfo,&info,sizeof(info));

	return r;
	}

//
// Methods for processing Convert requests
//

/**
  Process Convert request where the source data is specified by a descriptor
*/
void DConvert1Channel::ConvertDes(const TDesC8* aSrc,TRequestStatus* aRequestStatus)
	{
	TInt r;

	// Get descriptor info
	TInt len;
	TInt maxLen;
	TAny* uptr = (TAny*)Kern::KUDesInfo(*aSrc,len,maxLen);

	// Check there isn't an outstanding request
	if(iConvertRequestStatus)
		Kern::ThreadKill(NULL,EExitPanic,ERequestAlreadyPending,KConvert1PanicCategory);

	// Check output buffer has been created
	if(!iOutBuffer.iChunk)
		{
		r = KErrNotReady;
		goto done;
		}

	// Check chunk has been created (TConfig::iCreateInputChunk True when SetConfig was called)
	if(!iInBuffer.iChunk)
		{
		r = KErrNotSupported;
		goto done;
		}

	// See if client data is in a shared chunk
	r = iClientBuffer.Open(uptr,len);
	if(r==KErrNone)
		iSource = &iClientBuffer; // use iClientBuffer as input buffer
	else
		{
		// Copy data from client descriptor into our iInBuffer
		r = iInBuffer.Copy(uptr,len);
		if(r==KErrNone)
			iSource = &iInBuffer; // use iInBuffer as input buffer
		}

	// Start convert if no error
	if(r==KErrNone)
		{
		iConvertRequestStatus = aRequestStatus;
		DoConvertStart(0,len);
		}
done:
	// Complete request if there was an error
	if (r!=KErrNone)
		Kern::RequestComplete(&Kern::CurrentThread(),aRequestStatus,r);
	}

/**
  Process Convert request where the source data is specified by a chunk
*/
void DConvert1Channel::ConvertChunk(const RConvert1::TConvertArgs* aSrcArgs,TRequestStatus* aRequestStatus)
	{
	TInt r;

	// Check there isn't an outstanding request
	if(iConvertRequestStatus)
		Kern::ThreadKill(NULL,EExitPanic,ERequestAlreadyPending,KConvert1PanicCategory);

	// Check output buffer has been created
	if(!iOutBuffer.iChunk)
		{
		r = KErrNotReady;
		goto done;
		}

	// Unpackage arguments
	RConvert1::TConvertArgs args;
	kumemget32(&args,aSrcArgs,sizeof(args));

	// Make buffer by opening chunk
	r=iClientBuffer.Open(args.iChunkHandle,args.iOffset,args.iSize);

	// Start convert if no error
	if(r==KErrNone)
		{
		iSource = &iClientBuffer;
		iConvertRequestStatus = aRequestStatus;
		DoConvertStart(0,args.iSize);
		}
done:
	// Complete request if there was an error
	if (r!=KErrNone)
		Kern::RequestComplete(&Kern::CurrentThread(),aRequestStatus,r);
	}

/**
  Process Convert request where the source data is contained in the input chunk
*/
void DConvert1Channel::ConvertInChunk(TInt aSize,TRequestStatus* aRequestStatus)
	{
	TInt r;

	// Check there isn't an outstanding request
	if(iConvertRequestStatus)
		Kern::ThreadKill(NULL,EExitPanic,ERequestAlreadyPending,KConvert1PanicCategory);

	// Check output buffer has been created
	if(!iOutBuffer.iChunk)
		{
		r = KErrNotReady;
		goto done;
		}

	// Check chunk has been created (TConfig::iCreateInputChunk True when SetConfig was called)
	if(!iInBuffer.iChunk)
		{
		r = KErrNotSupported;
		goto done;
		}

	// Check size of data really fits within chunk
	if(TUint(aSize)>=TUint(iInBuffer.iMaxSize))
		{
		r = KErrArgument;
		goto done;
		}

	// Start the convert 
	iSource = &iInBuffer;
	iConvertRequestStatus = aRequestStatus;
	DoConvertStart(iInBuffer.iChunkOffset,aSize);
	r = KErrNone;

done:
	// Complete request if there was an error
	if (r!=KErrNone)
		Kern::RequestComplete(&Kern::CurrentThread(),aRequestStatus,r);
	}

/**
  Signal ConvertData request completed
*/
void DConvert1Channel::ConvertCancel()
	{
	// Tell hardware to stop
	DoConvertCancel();

	// Complete client request
	NKern::ThreadEnterCS();
	ConvertComplete(KErrCancel);
	NKern::ThreadLeaveCS();
	}

/**
  DFC callback called after data has been converted.
*/
void DConvert1Channel::ConvertDfcTrampoline(TAny* aSelf)
	{
	// Just call non-static method
	((DConvert1Channel*)aSelf)->ConvertDfc();
	}

/**
  DFC callback called after data has been converted
*/
void DConvert1Channel::ConvertDfc()
	{
	TRACE(Kern::Printf(">DConvert1Channel::ConvertDfc\n");)

	// The result value will be the chunk offset of the data we've converted
	TInt result = iOutBuffer.iChunkOffset;
	ConvertComplete(result);

	TRACE(Kern::Printf("<DConvert1Channel::ConvertDfc\n");)
	}

/**
  Complete a Convert request
  @pre In thread critical section or DFC thread
*/
void DConvert1Channel::ConvertComplete(TInt aResult)
	{
	// Hold mutex to avoid concurrency
	NKern::FMWait(&iConvertMutex);

	// Claim the client request
	TRequestStatus* status = iConvertRequestStatus;
	iConvertRequestStatus = NULL;

	// Claim chunk handle if we need to close it
	DChunk* chunk = NULL;
	if(status && iSource==&iClientBuffer)
		{
		chunk = iClientBuffer.iChunk;
		iClientBuffer.iChunk = NULL;
		}

	// Clear iSource to help show up bugs
	iSource = NULL;

	// Can release mutex now we own the pointers
	NKern::FMSignal(&iConvertMutex);

	// Must be in a critical section so we can't die whilst owning 'chunk' and 'status'
	__ASSERT_CRITICAL;

	// Close chunk if required
	if(chunk)
		Kern::ChunkClose(chunk);

	// Complete the request
	if(status)
		Kern::RequestComplete(iClient,status,aResult);
	}


//
// DChunkBuffer
//

/**
  Constructor
*/
DChunkBuffer::DChunkBuffer()
	: iChunk(NULL), iPhysicalPages(NULL)
	{
	}

/**
  Create chunk and commit memory for buffer
*/
TInt DChunkBuffer::Create(TInt aBufferSize)
	{
	// Initialise member data for the size of buffer we want
	TInt r=SetMaxSize(aBufferSize);
	if(r!=KErrNone)
		return r;

	// Create chunk
	__NK_ASSERT_DEBUG(!iChunk);
	TChunkCreateInfo info;
	info.iType = TChunkCreateInfo::ESharedKernelMultiple;
	info.iMaxSize = (TInt)aBufferSize;
#ifndef __WINS__
	info.iMapAttr = EMapAttrCachedMax;
#else
	info.iMapAttr = 0;
#endif
	info.iOwnsMemory = ETrue;
	r = Kern::ChunkCreate(info,iChunk,iChunkBase,iChunkMapAttr);

	if(r==KErrNone)
		{
		// Commit memory to chunk
		iChunkOffset = 0;
		r = Kern::ChunkCommit(iChunk,iChunkOffset,Kern::RoundToPageSize(iMaxSize));
		if(r==KErrNone)
			{
			// Setup physical address info for memory in the buffer
			r = SetPhysicalAddresses(iMaxSize);
			}
		}

	if(r!=KErrNone)
		Destroy(); // Cleanup

	return r;
	}

/**
  Free all resources
*/
void DChunkBuffer::Destroy()
	{
	delete [] iPhysicalPages;
	iPhysicalPages = 0;
	Close();
	}

/**
  Destructor
*/
DChunkBuffer::~DChunkBuffer()
	{
	Destroy();
	}

/**
  Set maximum size for buffer.
  (Allocates heap resources for this max size.)
*/
TInt DChunkBuffer::SetMaxSize(TInt aMaxSize)
	{
	// Create array to hold address of physical pages
	__NK_ASSERT_DEBUG(!iPhysicalPages);
	iPhysicalPages = new TPhysAddr[Kern::RoundToPageSize(aMaxSize)/Kern::RoundToPageSize(1)+1];
	if(!iPhysicalPages)
		return KErrNoMemory;

	iMaxSize = aMaxSize;
	return KErrNone;
	}

/**
  Open a shared chunk given an user address and siae
*/
TInt DChunkBuffer::Open(TAny* aAddress, TInt aSize, TBool aWrite)
	{
	TInt r;

	// Check size
	if(aSize>iMaxSize)
		return KErrTooBig;

	NKern::ThreadEnterCS();

	// Attempt to open chunk
	iChunk = Kern::OpenSharedChunk(NULL,aAddress,aWrite,iChunkOffset);
	if(!iChunk)
		r = KErrArgument;
	else
		{
		// Get physical addresses
		r = SetPhysicalAddresses(aSize);
		if(r!=KErrNone)
			Close();
		}

	NKern::ThreadLeaveCS();

	return r;
	}

/**
  Open a specified shared chunk
*/
TInt DChunkBuffer::Open(TInt aChunkHandle, TInt aOffset, TInt aSize, TBool aWrite)
	{
	TInt r;

	// Check size
	if(aSize>iMaxSize)
		return KErrTooBig;
	iChunkOffset = aOffset;

	NKern::ThreadEnterCS();

	// Attempt to open chunk
	iChunk = Kern::OpenSharedChunk(NULL,aChunkHandle,aWrite);
	if(!iChunk)
		r = KErrArgument;
	else
		{
		// Get physical addresses
		r = SetPhysicalAddresses(aSize);
		if(r!=KErrNone)
			Close();
		}

	NKern::ThreadLeaveCS();

	return r;
	}

/**
  Close chunk
*/
void DChunkBuffer::Close()
	{
	__ASSERT_CRITICAL;
	if(iChunk)
		{
		Kern::ChunkClose(iChunk);
		iChunk = NULL;
		}
	}

/**
  Fill buffer by copying data from the given user address
*/
TInt DChunkBuffer::Copy(TAny* aAddress, TInt aSize)
	{
	// Check size
	if(aSize>iMaxSize)
		return KErrTooBig;

	// Copy data
	kumemget((TAny*)(iChunkBase+iChunkOffset),aAddress,aSize);

	return KErrNone;
	}

/**
  Setup physical address info for memory in the buffer
*/
TInt DChunkBuffer::SetPhysicalAddresses(TInt aSize)
	{
	// Assert that the iPhysicalPages array already allocated will be big enough
	__NK_ASSERT_DEBUG(aSize<=iMaxSize);

	// Get physical addresses
	TLinAddr kaddr;
	TInt r=Kern::ChunkPhysicalAddress(iChunk,iChunkOffset,aSize,kaddr,iChunkMapAttr,iPhysicalAddress,iPhysicalPages);
	// r = 0 or 1 on success. (1 meaning the physical pages are not contiguous)
	if(r>=0) 
		{
		iChunkBase = kaddr-iChunkOffset; // Calculate start of chunk in kernel process address space
		r = KErrNone;
		}
	return r;
	}

//
// Program converter hardware
//

/**
  Initialise hardware to start converting data.
  Input data is in iSource.
  Output data to be placed in iOutBuffer.
*/
void DConvert1Channel::DoConvertStart(TInt aOffset,TInt aSize)
	{
	// For this example test...
	
	TRACE(Kern::Printf("DConvert1Channel::DoConvertStart\n");)

	// 'Convert' data by xoring with 1
	TUint8* src = (TUint8*)iSource->iChunkBase+iSource->iChunkOffset+aOffset;
	TUint8* end = src+aSize;
	TUint8* dst = (TUint8*)iOutBuffer.iChunkBase+iOutBuffer.iChunkOffset;
	while(src<end)
		*dst++ = TUint8(*src++^1);

	// Start the timer
	TInt ticks = TInt((TInt64)1000000*(TInt64)aSize/(TInt64)iConfig.iSpeed)
					/NKern::TickPeriod();
	if(ticks<1)
		ticks = 1;
#ifdef _DEBUG
	TInt r=
#endif
		iConvertTimer.OneShot(ticks,ETrue);
	__NK_ASSERT_DEBUG(r==KErrNone);
	}

/**
  Tell hardware to stop converting.
*/
void DConvert1Channel::DoConvertCancel()
	{
	// For this example test...
	
	TRACE(Kern::Printf("DConvert1Channel::DoConvertCancel\n");)

	// Cancel the timer
	iConvertTimer.Cancel();
	}
