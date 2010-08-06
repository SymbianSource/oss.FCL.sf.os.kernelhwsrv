// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// Test driver for DMA V2 framework
//
//

#include <kernel/kern_priv.h>
#include <drivers/dma.h>
#include "d_dma2.h"

_LIT(KClientPanicCat, "D_DMA2");
_LIT(KDFCThreadName,"D_DMA_DFC_THREAD");
_LIT(KIsrCbDfcThreadName,"D_DMA_IsrCb_thread");
const TInt KDFCThreadPriority=26;

class TStopwatch
	{
public:
	TStopwatch()
		:iStart(0), iStop(0)
		{}

	void Start()
		{iStart = NKern::FastCounter();}

	void Stop()
		{
		iStop = NKern::FastCounter();

		__KTRACE_OPT(KDMA, Kern::Printf(">TStopwatch::Stop FastCounter ticks: iStart=0x%lx iStop=0x%lx", iStart, iStop));
		}

	TUint64 ReadMicroSecs() const
		{
#ifndef __SMP__
		TUint64 diff = 0;
		if(iStart > iStop)
			{
			diff = (KMaxTUint64 - iStart) + iStop;
			}
		else
			{
			diff = iStop - iStart;
			}
		return FastCountToMicroSecs(diff);
#else
		//On SMP it is possible for the value returned from
		//NKern::FastCounter to depend on the current CPU (ie.
		//NaviEngine)
		//
		//One solution would be to tie DFC's and ISR's to the same
		//core as the client, but this would reduce the usefulness of
		//SMP testing.
		return 0;
#endif
		}
private:

	TUint64 FastCountToMicroSecs(TUint64 aCount) const
		{
		const TUint64 countsPerS = NKern::FastCounterFrequency();

		TUint64 timeuS = (aCount*1000000)/countsPerS;
		__KTRACE_OPT(KDMA, Kern::Printf(">TStopwatch::FastCountToMicroSecs FastCounter ticks: aCount=0x%lx countsPerS=0x%lx time=0x%lx", aCount, countsPerS, timeuS));
		return timeuS;
		}

	TUint64 iStart;
	TUint64 iStop;
	};

//////////////////////////////////////////////////////////////////////////////

class DClientDmaRequest;
/**
Driver channel. Only accessible by a single client thread
*/
class DDmaTestSession : public DLogicalChannelBase
	{
public:
	DDmaTestSession();
	virtual ~DDmaTestSession();
protected:
	// from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
private:
	TInt DoGetInfo(TAny* aInfo);

	TInt OpenDmaChannel(TUint aPslCookie, TUint& aDriverCookie);
	TInt OpenDmaChannel(TUint& aDriverCookie, TDmaChannel::SCreateInfo& aInfo);
	TInt LinkDmaChannelByCookie(TUint aDriverCookie);	
	TInt UnlinkDmaChannelByCookie(TUint aDriverCookie);
	TInt CloseDmaChannelByCookie(TUint aDriverCookie);
	TInt PauseDmaChannelByCookie(TUint aDriverCookie);
	TInt ResumeDmaChannelByCookie(TUint aDriverCookie);
	TInt GetChannelCapsByCookie(TUint aDriverCookie, SDmacCaps& aChannelCaps);
	TInt GetChannelCapsByCookie(TUint aDriverCookie, TDmacTestCaps& aChannelCaps);
	TInt CancelAllByCookie(TUint aDriverCookie);
	TInt IsrRedoRequestByCookie(TUint aDriverCookie,TUint32 aSrcAddr,TUint32 aDstAddr,TInt aTransferCount,TUint32 aPslRequestInfo,TBool aIsrCb);
	TInt IsQueueEmptyByCookie(TUint aDriverCookie, TBool& aQueueEmpty);		
	TInt ChannelIsOpenedByCookie(TUint aDriverCookie, TBool& aChannelOpen);	
	TInt EnableDstElementCountingByCookie(TUint aDriverCookie);
	TInt EnableSrcElementCountingByCookie(TUint aDriverCookie);
	TInt DisableDstElementCountingByCookie(TUint aDriverCookie);
	TInt DisableSrcElementCountingByCookie(TUint aDriverCookie);
	TInt TotalNumDstElementsTransferredByCookie(TUint aDriverCookie);
	TInt TotalNumSrcElementsTransferredByCookie(TUint aDriverCookie);
	void CloseDmaChannelByIndex(TInt aIndex);
	void CancelAllByIndex(TInt aIndex);
	TInt LinkDmaChannelByIndex(TInt aIndex);
	TInt UnlinkDmaChannelByIndex(TInt aIndex);
	TInt PauseDmaChannelByIndex(TInt aIndex);
	TInt ResumeDmaChannelByIndex(TInt aIndex);		
	TInt IsrRedoRequestByIndex(TInt aIndex,TUint32 aSrcAddr,TUint32 aDstAddr,TInt aTransferCount,TUint32 aPslRequestInfo,TBool aIsrCb);
	void EnableDstElementCountingByIndex(TInt aIndex);
	void EnableSrcElementCountingByIndex(TInt aIndex);
	void DisableDstElementCountingByIndex(TInt aIndex);
	void DisableSrcElementCountingByIndex(TInt aIndex);
	TInt TotalNumDstElementsTransferredByIndex(TInt aIndex);
	TInt TotalNumSrcElementsTransferredByIndex(TInt aIndex);
	TInt CreateSharedChunk();
	TUint OpenSharedChunkHandle();

	/**
	Creates a new kernel-side DMA request object, associated with a previously
	opened channel

	@param aChannelCookie - A channel cookie as returned by OpenDmaChannel
	@param aRequestCookie - On success will be a cookie by which the dma request can be referred to
	@param aNewCallback - If true, then a new style DMA callback will be used
	*/
	TInt CreateDmaRequest(TUint aChannelCookie, TUint& aRequestCookie, TBool aNewCallback = EFalse, TInt aMaxFragmentSizeBytes=0);

	/**
	Destroys a previously created dma request object
	*/
	TInt DestroyDmaRequestByCookie(TUint aRequestCookie);

	void DestroyDmaRequestByIndex(TInt aIndex);


	TInt CookieToChannelIndex(TUint aDriverCookie) const;
	TInt CookieToRequestIndex(TUint aRequestCookie) const;

	void MakeAddressesAbsoulute(TDmaTransferArgs& aTransferArgs) const;
	TInt FragmentRequest(TUint aRequestCookie, const TDmaTransferArgs& aTransferArgs, TBool aLegacy=ETrue);

	TInt QueueRequest(TUint aRequestCookie, TRequestStatus* aStatus, TCallbackRecord* aRecord, TUint64* aDurationMicroSecs);
	DClientDmaRequest* RequestFromCookie(TUint aRequestCookie) const;
	TInt RequestFragmentCount(TUint aRequestCookie);
	TDmaV2TestInfo ConvertTestInfo(const TDmaTestInfo& aOldInfo) const;
private:
	DThread* iClient;
	TDynamicDfcQue* iDfcQ;
	TDynamicDfcQue* iIsrCallbackDfcQ; // Will be used by requests which complete with an ISR callback
	static const TInt KMaxChunkSize;
	TLinAddr iChunkBase;
	DChunk* iChunk;

	RPointerArray<TDmaChannel> iChannels;
	RPointerArray<DClientDmaRequest> iClientDmaReqs;
	};


/**
Allows a TClientRequest to be associated with a DDmaRequest
*/
class DClientDmaRequest : public DDmaRequest
	{
public:
	static DClientDmaRequest* Construct(DThread* aClient, TDfcQue* const aDfcQ, TDmaChannel& aChannel, TBool aNewStyle=EFalse, TInt aMaxTransferSize=0);
	~DClientDmaRequest();

	TInt Queue(TRequestStatus* aRequestStatus, TCallbackRecord* aRecord, TUint64* aDurationMicroSecs);
	void AddRequeArgs(const TIsrRequeArgsSet& aRequeArgSet);

	TUint64 GetDuration()
		{return iStopwatch.ReadMicroSecs();}

	/**
	Store a copy of the TDmaTransferArgs which was used for fragmentation
	for argument checking
	*/
	void SetAddressParms(const TDmaTransferArgs& aAddressParms)
		{iFragmentedTransfer = aAddressParms;}

	/**
	Retrieve stored TDmaTransferArgs
	*/
	const TDmaTransferArgs& GetAddressParms() const
		{return iFragmentedTransfer;}

protected:
	TInt Create();
	/** Construct with old style callback */
	DClientDmaRequest(DThread* aClient, TDfcQue* const aDfcQ, TDmaChannel& aChannel, TInt aMaxTransferSize);

	/** Construct with new style callback */
	DClientDmaRequest(DThread* aClient, TDfcQue* const aDfcQ, TDmaChannel& aChannel, TBool aNewStyle, TInt aMaxTransferSize);

private:
	static void CallbackOldStyle(TResult aResult, TAny* aRequest);
	static void Callback(TUint, TDmaResult, TAny*, SDmaDesHdr*);
	static void CompleteCallback(TAny* aRequest);

	void DoCallback(TUint, TDmaResult);
	TBool RedoRequest();

	//!< Used to return a TCallbackRecord and transfer time
	TClientDataRequest2<TCallbackRecord, TUint64>* iClientDataRequest;

	DThread* const iClient;
	TDfcQue* const iDfcQ; //!< Use the DDmaTestSession's dfc queue
	TDfc iDfc;

	TStopwatch iStopwatch;
	TIsrRequeArgsSet iIsrRequeArgSet;

	/**
	This will be updated each time fragment is called.
	It is required so that, at queue time, if ISR re-queue
	arguments are added, they can be checked for sanity
	*/
	TDmaTransferArgs iFragmentedTransfer;
	};

DClientDmaRequest* DClientDmaRequest::Construct(DThread* aClient, TDfcQue* const aDfcQ, TDmaChannel& aChannel, TBool aNewStyle, TInt aMaxTransferSize)
	{
	DClientDmaRequest* dmaRequest = NULL;
	if(aNewStyle)
		{
#ifdef DMA_APIV2
		dmaRequest = new DClientDmaRequest(aClient, aDfcQ, aChannel, aNewStyle, aMaxTransferSize);
#else
		TEST_FAULT; // if a new style dma request was requested it should have been caught earlier
#endif
		}
	else
		{
		dmaRequest = new DClientDmaRequest(aClient, aDfcQ, aChannel, aMaxTransferSize);
		}

	if(dmaRequest == NULL)
		{
		return dmaRequest;
		}

	const TInt r = dmaRequest->Create();
	if(r != KErrNone)
		{
		delete dmaRequest;
		dmaRequest = NULL;
		}
	return dmaRequest;
	}

DClientDmaRequest::DClientDmaRequest(DThread* aClient, TDfcQue* const aDfcQ, TDmaChannel& aChannel, TInt aMaxFragmentSize)
	:DDmaRequest(aChannel, &CallbackOldStyle, this, aMaxFragmentSize),
	iClientDataRequest(NULL),
	iClient(aClient),
	iDfcQ(aDfcQ),
	iDfc(CompleteCallback,NULL, iDfcQ, KMaxDfcPriority)
	{
	}
#ifdef DMA_APIV2
DClientDmaRequest::DClientDmaRequest(DThread* aClient, TDfcQue* const aDfcQ, TDmaChannel& aChannel, TBool /*aNewStyle*/, TInt aMaxFragmentSize)
	:DDmaRequest(aChannel, &Callback, this, aMaxFragmentSize),
	iClientDataRequest(NULL),
	iClient(aClient),
	iDfcQ(aDfcQ),
	iDfc(CompleteCallback,NULL, iDfcQ, KMaxDfcPriority)
	{
	}
#endif

TInt DClientDmaRequest::Create()
	{
	return Kern::CreateClientDataRequest2(iClientDataRequest);
	}

DClientDmaRequest::~DClientDmaRequest()
	{
	__KTRACE_OPT(KDMA, Kern::Printf(">DClientDmaRequest::~DClientDmaRequest")); 
	if(iClientDataRequest)
		{
		Kern::DestroyClientRequest(iClientDataRequest);
		}
	}

/**
Queue the DClientDmaRequest.

@param aRequestStatus Pointer to the client's request status
@param aRecord Pointer to the user's TCallbackRecord, may be null
@return
   -KErrInUse The client request is in use
   -KErrNone success
*/
TInt DClientDmaRequest::Queue(TRequestStatus* aRequestStatus, TCallbackRecord* aRecord, TUint64* aDurationMicroSecs)
	{
	__NK_ASSERT_ALWAYS(aRecord);
	__NK_ASSERT_ALWAYS(aDurationMicroSecs);

	//erase results from last transfer
	iClientDataRequest->Data1().Reset();
	iClientDataRequest->SetDestPtr1(aRecord);

	iClientDataRequest->SetDestPtr2(aDurationMicroSecs);


	TInt r = iClientDataRequest->SetStatus(aRequestStatus);
	if(r != KErrNone)
		{
		return r;
		}

	iStopwatch.Start();
#ifdef DMA_APIV2
	r = DDmaRequest::Queue();
#else
	// old version of queue did not return an error code
	DDmaRequest::Queue();
	r = KErrNone;
#endif

	return r;
	}

void DClientDmaRequest::AddRequeArgs(const TIsrRequeArgsSet& aRequeArgSet)
	{
	iIsrRequeArgSet = aRequeArgSet;
	}

/**
If a transfer complete callback in ISR context s received this will be
called to redo the request with the first entry in the array

@return ETrue If the redo was successful - indicates that another callback is comming
*/
TBool DClientDmaRequest::RedoRequest()
	{
	TIsrRequeArgs args = iIsrRequeArgSet.GetArgs();
	const TInt r = args.Call(iChannel);
	TCallbackRecord& record = iClientDataRequest->Data1();
	record.IsrRedoResult(r);
	return (r == KErrNone);
	}


/**
Calls TDmaChannel::IsrRedoRequest on aChannel
with this object's parameters
*/
TInt TIsrRequeArgs::Call(TDmaChannel& aChannel)
	{
#ifdef DMA_APIV2
	return aChannel.IsrRedoRequest(iSrcAddr, iDstAddr, iTransferCount, iPslRequestInfo, iIsrCb);
#else
	TEST_FAULT;
	return KErrNotSupported;
#endif
	}

/** Translate an old style dma callback to a new-style one
*/
void DClientDmaRequest::CallbackOldStyle(TResult aResult, TAny* aArg)
	{
	__KTRACE_OPT(KDMA, Kern::Printf(">DClientDmaRequest::CallBackOldStyle: TResult result=%d", aResult));
	TEST_ASSERT(aResult != EBadResult);
	//translate result code
	const TDmaResult result = (aResult == EOk) ? EDmaResultOK : EDmaResultError;

	//call the new-style callback
	Callback(EDmaCallbackRequestCompletion, result, aArg, NULL);
	}


/**
The new style callback called by the DMA framework
may be called in either thread or ISR context
*/
void DClientDmaRequest::Callback(TUint aCallbackType, TDmaResult aResult, TAny* aArg, SDmaDesHdr* /*aHdr*/)
	{
	const TInt context = NKern::CurrentContext();
	__KTRACE_OPT(KDMA, Kern::Printf(">DClientDmaRequest::CallBack: TDmaResult result = %d, NKern::TContext context = %d", aResult, context));
	
	DClientDmaRequest& self = *reinterpret_cast<DClientDmaRequest*>(aArg);
	self.DoCallback(aCallbackType, aResult);

	// decide if callback is complete
	const TBool transferComplete = aCallbackType & EDmaCallbackRequestCompletion;
	if(!transferComplete)
		{
		return;
		}

	// If there are reque args then redo this request
	// another callback would then be expected.
	// Requests can only be re-queued in ISR context, but we
	// do not check that here as it is up to the client to get
	// it right - also, we want to test that the PIL catches this
	// error
	if(!self.iIsrRequeArgSet.IsEmpty())
		{
		// If redo call was succesful, return and wait for next call back
		if(self.RedoRequest())
			return;
		}

	switch(context)
		{
	case NKern::EThread:
		{
		CompleteCallback(aArg);
		break;
		}
	case NKern::EInterrupt:
		{
		self.iDfc.iPtr = aArg;
		self.iDfc.Add();
		break;
		}
	//Fall-through: If context is IDFC or the EEscaped marker occur
	//it is an error
	case NKern::EIDFC:
	case NKern::EEscaped:
	default:
		TEST_FAULT;
		}
	}

/**
Log results of callback. May be called in either thread or ISR context
*/
void DClientDmaRequest::DoCallback(TUint aCallbackType, TDmaResult aResult)
	{
	iStopwatch.Stop(); //sucessive calls will simply over write the stop time

	// This will always be done whether the client requested a
	// callback record or not
	TCallbackRecord& record = iClientDataRequest->Data1();
	record.ProcessCallback(aCallbackType, aResult);
	}

/**
This function may either be called directly or queued as a DFC
*/
void DClientDmaRequest::CompleteCallback(TAny* aArg)
	{
	__KTRACE_OPT(KDMA, Kern::Printf(">DClientDmaRequest::CompleteCallBack thread %O", &Kern::CurrentThread()));
	__ASSERT_NOT_ISR;

	DClientDmaRequest& self = *reinterpret_cast<DClientDmaRequest*>(aArg);

	self.iClientDataRequest->Data2() = self.iStopwatch.ReadMicroSecs();

	//Assert that we called SetRequestStatus on this object before
	//queueing
	__NK_ASSERT_DEBUG(self.iClientDataRequest->IsReady());

	// This is an inelegant, temporary, solution to the following problem:
	//
	// If a dma request completes with an ISR callback the test
	// framework will queue this function as a DFC which
	// will then signal the user-side client. As a consequence of
	// this the user side client may then decide to destroy this
	// request. However, untill the DMA framework's DFC has run
	// and called OnDeque() on this request, it is still considered as
	// queued. Since it is possible that this DFC could run
	// before the DMA fw's DFC, this request could get destroyed while
	// it is stil queued, triggering a PIL assertion.
	//
	// The real fix is likely be for the PIL to call the callback
	// twice, but with different arguments, once to annonunce the
	// ISR and again to announce the dequeue.
	//
	// Here we poll and wait for this request to be dequeued. Note,
	// this DFC is currently run on a separate DFC queue, otherwise
	// it could get deadlocked. An alternative to polling would be
	// to use DCondVar, but that would require PIL modification

	if(NKern::CurrentThread() == self.iDfcQ->iThread)
		{
		// Only need to poll if we aren't on the channel's DFC queue
		for(;;)
			{
			// once the request has been unqueued it
			// can only be queued again by the client
			const TBool queued = __e32_atomic_load_acq32(&self.iQueued);
			if(!queued)
				break;
			__KTRACE_OPT(KDMA, Kern::Printf("Waiting for requeuest to be dequeued"));
			NKern::Sleep(10);
			}
		}
	else
		{
		// If we are on the channel's DFCQ we should be dequeued
		// already
		__NK_ASSERT_DEBUG(!__e32_atomic_load_acq32(&self.iQueued));
		}

	// We can always complete with KErrNone, the actual DMA result is
	// logged in the TCallbackRecord
	Kern::QueueRequestComplete(self.iClient, self.iClientDataRequest, KErrNone);
	}

const TInt DDmaTestSession::KMaxChunkSize = 8 * KMega;

TInt DDmaTestSession::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
	}

DDmaTestSession::DDmaTestSession()
	: iClient(NULL), iDfcQ(NULL), iIsrCallbackDfcQ(NULL), iChunkBase(0), iChunk(NULL)
	{}

// called in thread critical section
TInt DDmaTestSession::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	__NK_ASSERT_ALWAYS(iDfcQ == NULL);
	__NK_ASSERT_ALWAYS(iIsrCallbackDfcQ == NULL);

	TInt r = Kern::DynamicDfcQCreate(iDfcQ, KDFCThreadPriority, KDFCThreadName);
	if (r != KErrNone)
		{
		Kern::Printf("DDmaTestSession::DoCreate D_DMA_DFC_THREAD returned (%d)\n", r);
		return r;
		}
	NKern::ThreadSetCpuAffinity((NThread*)(iDfcQ->iThread), KCpuAffinityAny);

	r = Kern::DynamicDfcQCreate(iIsrCallbackDfcQ, KDFCThreadPriority, KIsrCbDfcThreadName);
	if (r != KErrNone)
		{
		Kern::Printf("DDmaTestSession::DoCreate D_DMA_IsrCb_thread returned (%d)\n", r);
		return r;
		}
	NKern::ThreadSetCpuAffinity((NThread*)(iIsrCallbackDfcQ->iThread), KCpuAffinityAny);

	iClient = &Kern::CurrentThread();

	r = CreateSharedChunk();
	Kern::Printf("DDmaTestSession::DoCreate CreateSharedChunk returned (%d)\n", r);
	return r;
	}

DDmaTestSession::~DDmaTestSession()
	{
	//Destroy requests before channels
	//or we will trigger an assertion
	while(iClientDmaReqs.Count())
		{
		DestroyDmaRequestByIndex(0);
		}
	iClientDmaReqs.Close();

	while(iChannels.Count())
		{
		CloseDmaChannelByIndex(0);
		}
	iChannels.Close();


	if (iDfcQ)
		{
		iDfcQ->Destroy();
		}

	if (iIsrCallbackDfcQ)
		{
		iIsrCallbackDfcQ->Destroy();
		}

	if(iChunk)
		{
		Kern::ChunkClose(iChunk);
		iChunk = NULL;
		}
	}

TInt DDmaTestSession::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	__NK_ASSERT_DEBUG(&Kern::CurrentThread() == iClient);

	switch (aFunction)
		{
	case RDmaSession::EOpenChannel:
			{
			TUint pslCookie = (TUint)a1;
			TUint driverCookie = 0;
			TInt r = OpenDmaChannel(pslCookie, driverCookie);
			umemput32(a2, &driverCookie, sizeof(TAny*));
			return r;
			}
	case RDmaSession::EOpenChannelExposed:
			{
			TDmaChannel::SCreateInfo openInfo;
			TUint driverCookie = 0;

			TPckgBuf<SCreateInfoTest> openArgsBuf;
			Kern::KUDesGet(openArgsBuf, *reinterpret_cast<TDes8*>(a2));

			SCreateInfoTest& openTestInfo = openArgsBuf();
			openInfo.iCookie = openTestInfo.iCookie;
			openInfo.iDesCount = openTestInfo.iDesCount;
			openInfo.iDfcQ = iDfcQ;
			openInfo.iDfcPriority = openTestInfo.iDfcPriority;

			#ifdef DMA_APIV2
				openInfo.iPriority = openTestInfo.iPriority;
				openInfo.iDynChannel = openTestInfo.iDynChannel;
			#endif

			TInt r = OpenDmaChannel(driverCookie, openInfo);
			umemput32(a1, &driverCookie, sizeof(TAny*));
			Kern::KUDesPut(*reinterpret_cast<TDes8*>(a2), openArgsBuf);
			return r;
			}
	case RDmaSession::ECloseChannel:
			{
			TUint driverCookie = reinterpret_cast<TUint>(a1);
			TInt r = CloseDmaChannelByCookie(driverCookie);
			return r;
			}
	case RDmaSession::EChannelCaps:
			{
			TUint driverCookie = reinterpret_cast<TUint>(a1);
			TPckgBuf<TDmacTestCaps> capsBuf;
			TInt r = GetChannelCapsByCookie(driverCookie, capsBuf());
			Kern::KUDesPut(*reinterpret_cast<TDes8*>(a2), capsBuf);
			return r;
			}
	case RDmaSession::EPauseChannel:
			{
			TUint driverCookie = reinterpret_cast<TUint>(a1);
			TInt r = PauseDmaChannelByCookie(driverCookie);
			return r;
			}
	case RDmaSession::EResumeChannel:
			{
			TUint driverCookie = reinterpret_cast<TUint>(a1);
			TInt r = ResumeDmaChannelByCookie(driverCookie);
			return r;
			}
	case RDmaSession::ELinkChannel:
			{
			TUint driverCookie = reinterpret_cast<TUint>(a1);
			TInt r = LinkDmaChannelByCookie(driverCookie);
			return r;
			}
	case RDmaSession::EUnlinkChannel:
			{
			TUint driverCookie = reinterpret_cast<TUint>(a1);
			TInt r = UnlinkDmaChannelByCookie(driverCookie);
			return r;
			}
	case RDmaSession::EFragmentCount:
			{
			TUint requestCookie = reinterpret_cast<TUint>(a1);
			TInt r = RequestFragmentCount(requestCookie);
			return r;
			}
	case RDmaSession::EEnableDstElementCounting:
			{		
			TUint requestCookie = reinterpret_cast<TUint>(a1);
			TInt r = EnableDstElementCountingByCookie(requestCookie);
			return r;
			}
	case RDmaSession::EEnableSrcElementCounting:
			{
			TUint requestCookie = reinterpret_cast<TUint>(a1);		
			TInt r = EnableSrcElementCountingByCookie(requestCookie);
			return r;
			}
	case RDmaSession::EDisableDstElementCounting:
			{
			TUint requestCookie = reinterpret_cast<TUint>(a1);
			TInt r = DisableDstElementCountingByCookie(requestCookie);
			return r;
			}
	case RDmaSession::EDisableSrcElementCounting:
			{
			TUint requestCookie = reinterpret_cast<TUint>(a1);
			TInt r = DisableSrcElementCountingByCookie(requestCookie);
			return r;
			}
	case RDmaSession::ETotalNumDstElementsTransferred:
			{
			TUint requestCookie = reinterpret_cast<TUint>(a1);
			TInt r = TotalNumDstElementsTransferredByCookie(requestCookie);
			return r;
			}
	case RDmaSession::ETotalNumSrcElementsTransferred:
			{
			TUint requestCookie = reinterpret_cast<TUint>(a1);
			TInt r = TotalNumSrcElementsTransferredByCookie(requestCookie);
			return r;
			}
	case RDmaSession::ERequestOpen:
			{
			RDmaSession::TRequestCreateArgs createArgs(0, EFalse, 0);
			TPckg<RDmaSession::TRequestCreateArgs> package(createArgs);
			Kern::KUDesGet(package, *reinterpret_cast<TDes8*>(a1));

			const TUint channelCookie = createArgs.iChannelCookie;
			TUint requestCookie = 0;

			TInt r = CreateDmaRequest(channelCookie, requestCookie, createArgs.iNewStyle, createArgs.iMaxFragmentSize);

			umemput32(a2, &requestCookie, sizeof(TAny*));
			return r;
			}
	case RDmaSession::ERequestClose:
			{
			const TUint requestCookie = reinterpret_cast<TUint>(a1);
			return DestroyDmaRequestByCookie(requestCookie);
			}
	case RDmaSession::EFragmentLegacy:
	case RDmaSession::EFragment:
			{
			TPckgBuf<RDmaSession::TFragmentArgs> argsBuff;
			Kern::KUDesGet(argsBuff, *reinterpret_cast<TDes8*>(a1));
			const TUint requestCookie = argsBuff().iRequestCookie;

			//must remove constness as we actually need to
			//convert the src and dst offsets to addresses
			TDmaTransferArgs& transferArgs = const_cast<TDmaTransferArgs&>(argsBuff().iTransferArgs);

			//convert address offsets in to kernel virtual addresses
			MakeAddressesAbsoulute(transferArgs);

			TInt r = KErrGeneral;
			if (!TAddressParms(transferArgs).CheckRange(iChunkBase, iChunk->Size()))
			{
				// Return error code for invalid src and destination arguments used in tranferArgs
				r=KErrArgument;
				return r;
			}

			TStopwatch clock;
			clock.Start();
			switch (aFunction)
				{
			case RDmaSession::EFragmentLegacy:
				r = FragmentRequest(requestCookie, transferArgs, ETrue); break;
			case RDmaSession::EFragment:
				r = FragmentRequest(requestCookie, transferArgs, EFalse); break;
			default:
				TEST_FAULT;
				}
			clock.Stop();

			const TUint64 time = clock.ReadMicroSecs();

			TUint64* const timePtr = argsBuff().iDurationMicroSecs;
			if(timePtr)
				{
				umemput(timePtr, &time, sizeof(time));
				}
			return r;
			}
	case RDmaSession::EQueueRequest:
			{
			TPckgBuf<RDmaSession::TQueueArgs> argsBuff;
			Kern::KUDesGet(argsBuff, *reinterpret_cast<TDes8*>(a1));

			//this is an Asynchronous request
			const TUint requestCookie = argsBuff().iRequestCookie;
			TRequestStatus* requestStatus = argsBuff().iStatus;
			TCallbackRecord* callbackRec = argsBuff().iCallbackRecord;
			TUint64* duration = argsBuff().iDurationMicroSecs;

			TInt r = QueueRequest(requestCookie, requestStatus, callbackRec, duration);
			if(r != KErrNone)
				{
				Kern::RequestComplete(requestStatus, r);
				}
			return r;
			}	
	case RDmaSession::EQueueRequestWithReque:
			{
			TPckgBuf<RDmaSession::TQueueArgsWithReque> argsBuff;
			Kern::KUDesGet(argsBuff, *reinterpret_cast<TDes8*>(a1));

			//this is an Asynchronous request
			const TUint requestCookie = argsBuff().iRequestCookie;
			TRequestStatus* requestStatus = argsBuff().iStatus;
			TCallbackRecord* callbackRec = argsBuff().iCallbackRecord;
			TUint64* duration = argsBuff().iDurationMicroSecs;

			TInt r = KErrNotFound;

			DClientDmaRequest* const request = RequestFromCookie(requestCookie);
			if(request != NULL)
				{
				TIsrRequeArgsSet& requeArgs = argsBuff().iRequeSet;
				requeArgs.Fixup(iChunkBase);

				TEST_ASSERT(requeArgs.CheckRange(iChunkBase, iChunk->Size(), request->GetAddressParms() ));
				request->AddRequeArgs(requeArgs);

				r = QueueRequest(requestCookie, requestStatus, callbackRec, duration);
				}

			if(r != KErrNone)
				{
				Kern::RequestComplete(requestStatus, r);
				}
			return r;
			}
	case RDmaSession::EIsOpened:
			{
			TUint driverCookie = (TUint)a1;
			TBool channelOpen = EFalse;;
			TInt r = ChannelIsOpenedByCookie(driverCookie,channelOpen);	
			umemput32(a2, &channelOpen, sizeof(TAny*));
			return r;		
			}
	case RDmaSession::EIsQueueEmpty:
			{
			TUint driverCookie = (TUint)a1;
			TBool queueEmpty = EFalse;;
			TInt r = IsQueueEmptyByCookie(driverCookie,queueEmpty);	
			umemput32(a2, &queueEmpty, sizeof(TAny*));
			return r;
			}
	case RDmaSession::ECancelAllChannel:
			{
			TUint driverCookie = reinterpret_cast<TUint>(a1);
			TInt r = CancelAllByCookie(driverCookie);
			return r;
			}
	case RDmaSession::EOpenSharedChunk:
			{
			return OpenSharedChunkHandle();
			}
	case RDmaSession::EGetTestInfo:
			{
#ifdef DMA_APIV2
			TPckgC<TDmaV2TestInfo> package(DmaTestInfoV2());
#else
			TPckgC<TDmaV2TestInfo> package(ConvertTestInfo(DmaTestInfo()));
#endif
			Kern::KUDesPut(*reinterpret_cast<TDes8*>(a1), package);
			return KErrNone;
			}
	default:
		Kern::PanicCurrentThread(KClientPanicCat, __LINE__);
		return KErrGeneral;
		}
	}

TInt DDmaTestSession::OpenDmaChannel(TUint& aDriverCookie, TDmaChannel::SCreateInfo& aInfo)
	{
	//cs so thread can't be killed between
	//opening channel and adding to array
	NKern::ThreadEnterCS();
	TDmaChannel* channel = NULL;
	TInt r = TDmaChannel::Open(aInfo, channel);
	if(KErrNone == r)
		{
		__NK_ASSERT_ALWAYS(channel);

		__KTRACE_OPT(KDMA, Kern::Printf("OpenDmaChannel: channel@ 0x%08x", channel));

		r = iChannels.Append(channel);
		if(KErrNone == r)
			{
			aDriverCookie = reinterpret_cast<TUint>(channel);
			}
		else
			{
			channel->Close();
			r = KErrNoMemory;
			}
		}
	NKern::ThreadLeaveCS();

	return r;
	}

/**
Open a DMA channel with arbitrary default parameters
*/
TInt DDmaTestSession::OpenDmaChannel(TUint aPslCookie, TUint& aDriverCookie )
	{
	TDmaChannel::SCreateInfo info;
	info.iCookie = aPslCookie;
	info.iDfcQ = iDfcQ;
	info.iDfcPriority = 3;
	info.iDesCount = 128;

	return OpenDmaChannel(aDriverCookie, info);
	}

TInt DDmaTestSession::CookieToChannelIndex(TUint aDriverCookie) const
	{
	const TInt r = iChannels.Find(reinterpret_cast<TDmaChannel*>(aDriverCookie));

	if(r < 0)
		{
		__KTRACE_OPT(KDMA, Kern::Printf("CookieToChannelIndex: cookie 0x%08x not found!", aDriverCookie)); 
		}
	return r;
	}

TInt DDmaTestSession::CookieToRequestIndex(TUint aRequestCookie) const
	{
	const TInt r = iClientDmaReqs.Find(reinterpret_cast<DClientDmaRequest*>(aRequestCookie));

	if(r < 0)
		{
		__KTRACE_OPT(KDMA, Kern::Printf("CookieToRequestIndex: cookie 0x%08x not found!", aRequestCookie)); 
		}
	return r;
	}

void DDmaTestSession::CloseDmaChannelByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("CloseDmaChannelByIndex: %d", aIndex)); 
	__NK_ASSERT_DEBUG(aIndex < iChannels.Count()); 
	// cs so client thread can't be killed between removing channel from
	// array and closing it.
	NKern::ThreadEnterCS();
	TDmaChannel* channel = iChannels[aIndex];
	iChannels.Remove(aIndex);
	channel->Close();
	NKern::ThreadLeaveCS();
	}

TInt DDmaTestSession::CloseDmaChannelByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("CloseDmaChannelByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToChannelIndex(aDriverCookie);
	
	if(index >= 0)
		{
		CloseDmaChannelByIndex(index);
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

TInt DDmaTestSession::CancelAllByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("CancelAllByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToChannelIndex(aDriverCookie);
	
	if(index >= 0)
		{
		CancelAllByIndex(index);
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

void DDmaTestSession::CancelAllByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("CancelAllByIndex: %d", aIndex)); 
	__NK_ASSERT_DEBUG(aIndex < iChannels.Count()); 
	
	TDmaChannel* channel = iChannels[aIndex];	
	channel->CancelAll();
	}

TInt DDmaTestSession::LinkDmaChannelByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("LinkDmaChannelByIndex: %d", aIndex)); 
	__NK_ASSERT_DEBUG(aIndex < iChannels.Count()); 

#ifdef DMA_APIV2
	TDmaChannel* channel = iChannels[aIndex];
	return channel->LinkToChannel(channel);
#else
	return KErrNotSupported;
#endif	
	}

TInt DDmaTestSession::LinkDmaChannelByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("LinkDmaChannelByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToChannelIndex(aDriverCookie);
	
	if(index >= 0)
		{
		TInt r = LinkDmaChannelByIndex(index);
		return r;
		}
	else
		{
		return KErrNotFound;
		}
	}

TInt DDmaTestSession::UnlinkDmaChannelByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("UnlinkDmaChannelByIndex: %d", aIndex)); 
	__NK_ASSERT_DEBUG(aIndex < iChannels.Count()); 

#ifdef DMA_APIV2
	TDmaChannel* channel = iChannels[aIndex];
	return channel->LinkToChannel(NULL);
#else
	return KErrNotSupported;
#endif	
	}

TInt DDmaTestSession::UnlinkDmaChannelByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("UnlinkDmaChannelByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToChannelIndex(aDriverCookie);
	
	if(index >= 0)
		{
		TInt r = UnlinkDmaChannelByIndex(index);
		return r;
		}
	else
		{
		return KErrNotFound;
		}
	}

TInt DDmaTestSession::PauseDmaChannelByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("PauseDmaChannelByIndex: %d", aIndex)); 
	__NK_ASSERT_DEBUG(aIndex < iChannels.Count()); 

#ifdef DMA_APIV2
	TDmaChannel* channel = iChannels[aIndex];
	return channel->Pause();
#else
	return KErrNotSupported;
#endif	
	}

TInt DDmaTestSession::PauseDmaChannelByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("PauseDmaChannelByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToChannelIndex(aDriverCookie);
	
	if(index >= 0)
		{
		TInt r = PauseDmaChannelByIndex(index);
		return r;
		}
	else
		{
		return KErrNotFound;
		}
	}

TInt DDmaTestSession::ResumeDmaChannelByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("ResumeDmaChannelByIndex: %d", aIndex)); 
	__NK_ASSERT_DEBUG(aIndex < iChannels.Count()); 

#ifdef DMA_APIV2
	TDmaChannel* channel = iChannels[aIndex];
	return channel->Resume();
#else
	return KErrNotSupported;
#endif
	}

TInt DDmaTestSession::ResumeDmaChannelByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("ResumeDmaChannelByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToChannelIndex(aDriverCookie);
	
	if(index >= 0)
		{
		TInt r = ResumeDmaChannelByIndex(index);
		return r;
		}
	else
		{
		return KErrNotFound;
		}
	}

TInt DDmaTestSession::EnableDstElementCountingByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("EnableDstElementCountingByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToRequestIndex(aDriverCookie);
	
	if(index >= 0)
		{
		EnableDstElementCountingByIndex(index);
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

void DDmaTestSession::EnableDstElementCountingByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("EnableDstElementCountingByIndex: %d", aIndex)); 		
	__NK_ASSERT_DEBUG(aIndex < iClientDmaReqs.Count()); 
#ifdef DMA_APIV2	
	iClientDmaReqs[aIndex]->EnableDstElementCounting();
#endif
	}

TInt DDmaTestSession::EnableSrcElementCountingByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("EnableSrcElementCountingByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToRequestIndex(aDriverCookie);
	
	if(index >= 0)
		{
		EnableSrcElementCountingByIndex(index);
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

void DDmaTestSession::EnableSrcElementCountingByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("EnableSrcElementCountingByIndex: %d", aIndex)); 		
	__NK_ASSERT_DEBUG(aIndex < iClientDmaReqs.Count()); 
	
#ifdef DMA_APIV2
	iClientDmaReqs[aIndex]->EnableSrcElementCounting();
#endif
	}

TInt DDmaTestSession::DisableDstElementCountingByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DisableDstElementCountingByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToRequestIndex(aDriverCookie);
	
	if(index >= 0)
		{
		DisableDstElementCountingByIndex(index);
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

void DDmaTestSession::DisableDstElementCountingByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DisableDstElementCountingByIndex: %d", aIndex)); 		
	__NK_ASSERT_DEBUG(aIndex < iClientDmaReqs.Count()); 
#ifdef DMA_APIV2
	iClientDmaReqs[aIndex]->DisableDstElementCounting();
#endif
	}

TInt DDmaTestSession::DisableSrcElementCountingByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DisableSrcElementCountingByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToRequestIndex(aDriverCookie);
	
	if(index >= 0)
		{
		DisableSrcElementCountingByIndex(index);
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

void DDmaTestSession::DisableSrcElementCountingByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DisableSrcElementCountingByIndex: %d", aIndex)); 		
	__NK_ASSERT_DEBUG(aIndex < iClientDmaReqs.Count()); 
#ifdef DMA_APIV2
	iClientDmaReqs[aIndex]->DisableSrcElementCounting();
#endif
	}

TInt DDmaTestSession::TotalNumDstElementsTransferredByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TotalNumDstElementsTransferredByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToRequestIndex(aDriverCookie);
	
	if(index >= 0)
		{
		TInt r = TotalNumDstElementsTransferredByIndex(index);
		return r;
		}
	else
		{
		return KErrNotFound;
		}
	}

TInt DDmaTestSession::TotalNumDstElementsTransferredByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TotalNumDstElementsTransferredByIndex: %d", aIndex)); 		
	__NK_ASSERT_DEBUG(aIndex < iClientDmaReqs.Count()); 
	
#ifdef DMA_APIV2
	TInt r = iClientDmaReqs[aIndex]->TotalNumDstElementsTransferred();
	return r;
#else
	return KErrNotSupported;
#endif
	}

TInt DDmaTestSession::TotalNumSrcElementsTransferredByCookie(TUint aDriverCookie)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TotalNumSrcElementsTransferredByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToRequestIndex(aDriverCookie);
	
	if(index >= 0)
		{
		TInt r = TotalNumSrcElementsTransferredByIndex(index);
		return r;
		}
	else
		{
		return KErrNotFound;
		}
	}

TInt DDmaTestSession::TotalNumSrcElementsTransferredByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TotalNumSrcElementsTransferredByIndex: %d", aIndex)); 		
	__NK_ASSERT_DEBUG(aIndex < iClientDmaReqs.Count()); 
	
#ifdef DMA_APIV2
	TInt r = iClientDmaReqs[aIndex]->TotalNumSrcElementsTransferred();
	return r;
#else
	return KErrNotSupported;
#endif
	}
TInt DDmaTestSession::IsrRedoRequestByCookie(TUint aDriverCookie,TUint32 aSrcAddr,TUint32 aDstAddr,TInt aTransferCount,TUint32 aPslRequestInfo,TBool aIsrCb)
{
	__KTRACE_OPT(KDMA, Kern::Printf("IsrRedoRequestByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToChannelIndex(aDriverCookie);
	
	if(index >= 0)
		{
		TInt r = IsrRedoRequestByIndex(index,aSrcAddr,aDstAddr,aTransferCount,aPslRequestInfo,aIsrCb);
		return r;
		}
	else
		{
		return KErrNotFound;
		}
}

TInt DDmaTestSession::IsrRedoRequestByIndex(TInt aIndex,TUint32 aSrcAddr,TUint32 aDstAddr,TInt aTransferCount,TUint32 aPslRequestInfo,TBool aIsrCb)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("IsrRedoRequestByIndex: %d", aIndex)); 
	__NK_ASSERT_DEBUG(aIndex < iChannels.Count()); 

#ifdef DMA_APIV2
	TDmaChannel* channel = iChannels[aIndex];
	return channel->IsrRedoRequest(aSrcAddr,aDstAddr,aTransferCount,aPslRequestInfo,aIsrCb);
#else
	return KErrNotSupported;
#endif
	}

/**
aChannelCaps will be set to "NULL" values
*/
TInt DDmaTestSession::GetChannelCapsByCookie(TUint aDriverCookie, TDmacTestCaps& aChannelCaps)
	{
	SDmacCaps caps = {0,}; //initialise with NULL values
	TInt r = GetChannelCapsByCookie(aDriverCookie, caps);

	if(r == KErrNotSupported)
		{
		//If we can not query caps it means
		//that we are using the v1 driver
		//we construct a empty TDmacTestCaps
		//but with an iPILVersion of 1
		const TDmacTestCaps nullCapsV1(caps, 1);
		aChannelCaps = nullCapsV1;
		r = KErrNone;
		}
	else if(r == KErrNone)
		{
		const TDmacTestCaps capsV2(caps, 2);
		aChannelCaps = capsV2;
		}

	return r;
	}

/**
Will return the capabilities of the DMA channel.
Querying SDmacCaps is not possible on V1 of the DMA framework.
In that case an error of KErrNotSupported will be returned
*/
TInt DDmaTestSession::GetChannelCapsByCookie(TUint aDriverCookie, SDmacCaps& aChannelCaps)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("GetChannelCapsByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToChannelIndex(aDriverCookie);
	if(index >= 0)
		{
#ifdef DMA_APIV2
		aChannelCaps = iChannels[index]->DmacCaps();
		return KErrNone;
#else
		return KErrNotSupported;
#endif
		}
	else
		{
		return KErrNotFound;
		}
	}

TInt DDmaTestSession::IsQueueEmptyByCookie(TUint aDriverCookie, TBool& aQueueEmpty)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("IsQueueEmptyByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToChannelIndex(aDriverCookie);
	
	if(index >= 0)
		{
		aQueueEmpty=iChannels[index]->IsQueueEmpty();
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

TInt DDmaTestSession::ChannelIsOpenedByCookie(TUint aDriverCookie, TBool& aChannelOpen)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("ChannelIsOpenedByCookie: 0x%08x", aDriverCookie)); 
	const TInt index = CookieToChannelIndex(aDriverCookie);
	
	if(index >= 0)
		{
		aChannelOpen=iChannels[index]->IsOpened();
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

TInt DDmaTestSession::CreateDmaRequest(TUint aChannelCookie, TUint& aRequestCookie, TBool aNewCallback, TInt aMaxFragmentSizeBytes)
	{
#ifndef DMA_APIV2
	if(aNewCallback)
		return KErrNotSupported;
#endif

	TInt channelIndex = CookieToChannelIndex(aChannelCookie);
	if(channelIndex < 0)
		return channelIndex;

	NKern::ThreadEnterCS();
	DClientDmaRequest* request = DClientDmaRequest::Construct(iClient, iIsrCallbackDfcQ, *iChannels[channelIndex], aNewCallback, aMaxFragmentSizeBytes);
	if(request == NULL)
		{
		NKern::ThreadLeaveCS();
		return KErrNoMemory;
		}

	TInt r = iClientDmaReqs.Append(request);
	if(r == KErrNone)
		{
		aRequestCookie = reinterpret_cast<TUint>(request);
		}
	else
		{
		delete request;
		}
	NKern::ThreadLeaveCS();
	
	return r;
	}

TInt DDmaTestSession::DestroyDmaRequestByCookie(TUint aRequestCookie)
	{
	TInt requestIndex = CookieToRequestIndex(aRequestCookie);
	if(requestIndex < 0)
		return requestIndex;

	DestroyDmaRequestByIndex(requestIndex);

	return KErrNone;
	}

void DDmaTestSession::DestroyDmaRequestByIndex(TInt aIndex)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DestroyDmaRequestByIndex: %d", aIndex)); 
	__NK_ASSERT_DEBUG(aIndex < iClientDmaReqs.Count()); 
	NKern::ThreadEnterCS();

	DClientDmaRequest* request = iClientDmaReqs[aIndex];
	iClientDmaReqs.Remove(aIndex);
	delete request;

	NKern::ThreadLeaveCS();
	}

TInt DDmaTestSession::CreateSharedChunk()
	{
    // Enter critical section so we can't die and leak the objects we are creating
    // I.e. the TChunkCleanup and DChunk (Shared Chunk)
    NKern::ThreadEnterCS();

    // Create the chunk
    TChunkCreateInfo info;
    info.iType         = TChunkCreateInfo::ESharedKernelSingle;
    info.iMaxSize      = KMaxChunkSize;
#ifndef __WINS__
    info.iMapAttr      = EMapAttrFullyBlocking | EMapAttrUserRw;
#endif

    info.iOwnsMemory   = ETrue;
    info.iDestroyedDfc = NULL;

    DChunk* chunk;
	TUint32 mapAttr;
    TInt r = Kern::ChunkCreate(info, chunk, iChunkBase, mapAttr);
    if(r!=KErrNone)
        {
        NKern::ThreadLeaveCS();
        return r;
        }

    // Map our device's memory into the chunk (at offset 0)
	TUint32 physicalAddr;
	r = Kern::ChunkCommitContiguous(chunk,0,KMaxChunkSize, physicalAddr);
    if(r!=KErrNone)
        {
        // Commit failed so tidy-up...
        Kern::ChunkClose(chunk);
        }
    else
        {
        iChunk = chunk;
        }

    // Can leave critical section now that we have saved pointers to created objects
    NKern::ThreadLeaveCS();

    return r;
	}

TUint DDmaTestSession::OpenSharedChunkHandle()
	{
	NKern::ThreadEnterCS();
	const TInt r = Kern::MakeHandleAndOpen(NULL, iChunk);
	NKern::ThreadLeaveCS();
	return r;
	}

/**
Replace addresses specified as an offset from the chunk base with absolute
virtual addresses.
*/
void DDmaTestSession::MakeAddressesAbsoulute(TDmaTransferArgs& aTransferArgs) const
	{
	aTransferArgs.iSrcConfig.iAddr += iChunkBase;
	aTransferArgs.iDstConfig.iAddr += iChunkBase;
	}

#ifndef DMA_APIV2
static TInt FragmentCount(DDmaRequest* aRequest)
	{
	TInt count = 0;
	for (SDmaDesHdr* pH = aRequest->iFirstHdr; pH != NULL; pH = pH->iNext)
		count++;
	return count;
	}
#endif

TInt DDmaTestSession::RequestFragmentCount(TUint aRequestCookie)
	{
	TInt requestIndex = CookieToRequestIndex(aRequestCookie);
	if(requestIndex < 0)
		return requestIndex;
#ifdef DMA_APIV2
	TInt r = iClientDmaReqs[requestIndex]->FragmentCount();
#else
	TInt r = FragmentCount(iClientDmaReqs[requestIndex]);
#endif

	return r;
	}

TInt DDmaTestSession::FragmentRequest(TUint aRequestCookie, const TDmaTransferArgs& aTransferArgs, TBool aLegacy)
	{
	__KTRACE_OPT(KDMA, Kern::Printf(">FragmentRequest: cookie=0x%08x, legacy=%d", aRequestCookie, aLegacy)); 
	TInt requestIndex = CookieToRequestIndex(aRequestCookie);
	if(requestIndex < 0)
		return requestIndex;

	DClientDmaRequest& request = *iClientDmaReqs[requestIndex];
	request.SetAddressParms(aTransferArgs);

	TInt r = KErrNotSupported;

	if (aTransferArgs.iTransferCount < 1)
		{
		// Return error code for invalid transfer size used in tranferArgs
		r=KErrArgument;
		return r;
		}

	if(aLegacy)
		{
		TUint flags = KDmaMemSrc | KDmaIncSrc | KDmaMemDest | KDmaIncDest;
		const TUint src = aTransferArgs.iSrcConfig.iAddr;
		const TUint dst = aTransferArgs.iDstConfig.iAddr;
		r = request.Fragment(src, dst, aTransferArgs.iTransferCount, flags, NULL);
		}
	else
		{
#ifdef DMA_APIV2
		r = request.Fragment(aTransferArgs);
#endif
		}
	return r;
	}

/**
Queue the request refered to by aRequestCookie

@param aRequestCookie Client identifier for the DDmaRequest
@param aStatus Pointer to the client's TRequestStatus
@param aRecord Pointer to the client's TCallbackRecord
@return
   - KErrNotFound - aRequestCookie was invalid
   - KErrNone - Success
*/
TInt DDmaTestSession::QueueRequest(TUint aRequestCookie, TRequestStatus* aStatus, TCallbackRecord* aRecord, TUint64* aDurationMicroSecs)
	{
	__KTRACE_OPT(KDMA, Kern::Printf(">QueueRequest: 0x%08x", aRequestCookie)); 

	DClientDmaRequest* request = RequestFromCookie(aRequestCookie);
	if(request == NULL)
		return KErrNotFound;

	return request->Queue(aStatus, aRecord, aDurationMicroSecs);
	}

DClientDmaRequest* DDmaTestSession::RequestFromCookie(TUint aRequestCookie) const
	{
	TInt requestIndex = CookieToRequestIndex(aRequestCookie);
	if(requestIndex < 0)
		return NULL;

	return (iClientDmaReqs[requestIndex]);
	}

TDmaV2TestInfo DDmaTestSession::ConvertTestInfo(const TDmaTestInfo& aOldInfo) const
	{
	TDmaV2TestInfo newInfo;
	newInfo.iMaxTransferSize = aOldInfo.iMaxTransferSize;
	newInfo.iMemAlignMask = aOldInfo.iMemAlignMask;
	newInfo.iMemMemPslInfo = aOldInfo.iMemMemPslInfo;

	newInfo.iMaxSbChannels = aOldInfo.iMaxSbChannels;
		{
		for(TInt i=0; i<aOldInfo.iMaxSbChannels; i++)
			{
			newInfo.iSbChannels[i] = aOldInfo.iSbChannels[i];
			}
		}

	newInfo.iMaxDbChannels = aOldInfo.iMaxDbChannels;
		{
		for(TInt i=0; i<aOldInfo.iMaxDbChannels; i++)
			{
			newInfo.iDbChannels[i] = aOldInfo.iDbChannels[i];
			}
		}

	newInfo.iMaxSgChannels = aOldInfo.iMaxSgChannels;
		{
		for(TInt i=0; i<aOldInfo.iMaxSgChannels; i++)
			{
			newInfo.iSgChannels[i] = aOldInfo.iSgChannels[i];
			}
		}

	return newInfo;
	}
//////////////////////////////////////////////////////////////////////////////

class DDmaTestFactory : public DLogicalDevice
	{
public:
	DDmaTestFactory();
	// from DLogicalDevice
	virtual ~DDmaTestFactory()
		{
		__KTRACE_OPT(KDMA, Kern::Printf(">DDmaTestFactory::~DDmaTestFactory"));
		}
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};


DDmaTestFactory::DDmaTestFactory()
    {
    iVersion = TestDmaLddVersion();
    iParseMask = KDeviceAllowUnit;							// no info, no PDD    
    }


TInt DDmaTestFactory::Create(DLogicalChannelBase*& aChannel)
    {
	aChannel=new DDmaTestSession;
	Kern::Printf("DDmaTestFactory::Create %d\n", aChannel?KErrNone : KErrNoMemory);
	return aChannel ? KErrNone : KErrNoMemory;
    }


TInt DDmaTestFactory::Install()
    {
    TInt r = SetName(&KTestDmaLddName);
	Kern::Printf("DDmaTestFactory::Install %d\n",r);
	return r;
    }


void DDmaTestFactory::GetCaps(TDes8& /*aDes*/) const
    {
    }

//////////////////////////////////////////////////////////////////////////////

DECLARE_STANDARD_LDD()
	{
    return new DDmaTestFactory;
	}
