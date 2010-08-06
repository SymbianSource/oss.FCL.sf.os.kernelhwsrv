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
// e32test\dmav2\d_dma2.h
// User-side API for LDD used to test DMAv2 framework.
// 
//

#ifndef __D_DMA2_H__
#define __D_DMA2_H__

#include <e32cmn.h>
#include <drivers/dmadefs.h>


#define ARRAY_LENGTH(ARRAY) sizeof(ARRAY)/sizeof(ARRAY[0])

#ifdef __KERNEL_MODE__
	#include <nkern.h>
	#include <kernel.h>
	#define TEST_FAULT FAULT();
	#define PRINT(N) Kern::Printf("%s = 0x%08x (%d)", #N, (N), (N))
	#define PRINTF(X) Kern::Printf X ;
#else
	#include <e32std.h>
	#include <e32debug.h>
	#define TEST_FAULT {RDebug::Printf("Assertion failure in %s, %d", __FILE__, __LINE__); User::Invariant();}
	#define PRINT(N) RDebug::Printf("%s = 0x%08x (%d)", #N, (N), (N))
	#define PRINTF(X) RDebug::Printf X ;
#endif

#define TEST_ASSERT(C) if(!(C)) {TEST_FAULT;}

const TUint KPhysAddrInvalidUser=0xFFFFFFFFu; // KPhysAddrInvalid is not defined on the user side
#ifdef __KERNEL_MODE__
//if this fails then KPhysAddrInvalidUser must be updated to match
//KPhysAddrInvalid
__ASSERT_COMPILE(KPhysAddrInvalidUser == KPhysAddrInvalid);
#else
const TUint KPhysAddrInvalid = KPhysAddrInvalidUser;
#endif

#ifdef __KERNEL_MODE__
//Function to format the output.
inline void DmaAppendFormat(TDes8& aBuf, const char* aFmt, ...)
	{
	if(!(&aBuf))
		return;
	VA_LIST list;
	VA_START(list,aFmt);
	Kern::AppendFormat(aBuf,aFmt,list);
	}
#endif

_LIT(KTestDmaLddNameSim, "TestDmaV2Sim");
_LIT(KTestDmaLddNameHw, "TestDmaV2");
#ifdef __DMASIM__
const TPtrC KTestDmaLddName = KTestDmaLddNameSim();
#else
const TPtrC KTestDmaLddName = KTestDmaLddNameHw();
#endif

inline TVersion TestDmaLddVersion() { return TVersion(1, 0, 1); }

TInt Log2(TInt aNum);

/**
Indicates the number of each type of call back received
and their context

@note It does not indicate the context of each callback, only
the final one
*/
const TInt KNumberOfCallbacks = 12;

class TCallbackRecord
	{
public:
	enum TCbContext
		{ EInvalid, EThread, EIsr };

	TCallbackRecord(
			TCbContext aContext = EThread,
			TInt aReq = 0,
			TInt aReqSrc = 0,
			TInt aReqDst = 0,

			TInt aDes = 0,
			TInt aDesSrc = 0,
			TInt aDesDst = 0,

			TInt aFrame = 0,
			TInt aFrameSrc = 0,
			TInt aFrameDst = 0,

			TInt aPause = 0,
			TInt aPauseSrc = 0,
			TInt aPauseDst = 0,
			TDmaResult aResult = EDmaResultOK
		);

	static TCallbackRecord Empty();

	void Reset();

	/**
	Allows 2 callback records to be compared
	*/
	TBool operator == (const TCallbackRecord aOther) const;
	void Print() const;

	/**
	Get the number of callbacks for callback aCbType
	*/
	TInt GetCount(TDmaCallbackType aCbType) const;

	void SetCount(TDmaCallbackType aCbType, TInt aCount);

	/**
	Set the result (expected or actual) from
	TDmaChannel::IsrRedoRequest
	 */
	inline TCallbackRecord& IsrRedoResult(TInt aResult) {iIsrRedoRequestResult = aResult; return *this;}

	/**
	Reports the context in which the callback occurred.
	*/
	inline TCbContext GetContext()
		{return iContext;}

	/**
	Updates data based on callback mask aCallbackMask
	@param aCallbackMask Bitmask of callback events @see TDmaCallbackType
	@oaram aResult The result reported by the current callback
	*/
	void ProcessCallback(TUint aCallbackMask, TDmaResult aResultaContext);

	static void SelfTest();

	// The below methods are setters, which may be chained together
	// ie. The Named Parameter Idiom
	// @see http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.18
	TCallbackRecord& Context(TCbContext aContext) {iContext = aContext; return *this;}

private:
	TInt BitToIndex(TDmaCallbackType aCbType) const;

	TCbContext CurrentContext() const;

	TInt iCallbackLog[KNumberOfCallbacks];

	TDmaResult iResult;
	TCbContext iContext;
	/** Result of the most recent redo request call */
	TInt iIsrRedoRequestResult;
	};

/**
Extends SDmacCaps to contain the DMA PIL
version being used
*/
struct TDmacTestCaps : public SDmacCaps
	{
	TDmacTestCaps();
	TDmacTestCaps(const SDmacCaps& aDmacCaps, TInt aVersion = 2);

	TInt iPILVersion;
	};


/** SCreateInfo for opening DMA - Reused for testing */
struct SCreateInfoTest
		{
		/** Default constructor. Initializes all fields with meaningful default
			values.

			Must be inline (for now) because exporting it would break existing
			custom DMA libs as their clients would need the export which would
			be missing from the custom .def files.
		*/
		SCreateInfoTest() : iPriority(KDmaPriorityNone), iDynChannel(EFalse) {};

		/** Identifier used by PSL to select channel to open */
		TUint32 iCookie;
		/** Number of descriptors this channel can use.

			This number is not used in the upgraded version of the DMA
			framework and is kept there only for source compatibility. If the
			client is certain that it will only ever use that version, then the
			value passed here doesn't matter - the framework will ignore it.

			@deprecated
		 */
		TInt iDesCount;
		/** DFC queue used to service DMA interrupts.

			The DFC thread priority must be higher than any client thread
			priority to avoid a situation where a transfer completes while
			being cancelled and another transfer is started before the DFC
			thread gets a chance to run. This would lead to a stray DFC.
		*/
		//TDfcQue* iDfcQ;

		TAny* iDfcQ;

		/** DFC priority */
		TUint8 iDfcPriority;
		/** Used by PSL to configure a channel priority (if possible).

			The default is KDmaPriorityNone (the don't care value).

		    @see TDmaPriority
		*/
		TUint iPriority;
		/** Request a dynamic DMA channel.

			If this is set to ETrue then the Open call is for a 'dynamic' as
			opposed to a static and solely owned DMA channel. A number of
			properties of the opened TDmaChannel object will be different in
			that case.

			The default value is EFalse.
		 */
		TBool iDynChannel;
		};


class TDmaChannel;

struct TAddrRange
	{
	TAddrRange(TUint aStart, TUint aLength);
	inline TUint End() const {return (iStart + iLength -1);}
	inline TUint Start() const {return iStart;}

	inline TBool Contains(TUint aValue) const {return Rng(iStart, aValue, End());}
	TBool Contains(TAddrRange aRange) const;

	TBool Overlaps(const TAddrRange& aRange) const;
	TBool IsFilled(TUint8 aValue) const;

	static void SelfTest();

private:
	TUint iStart;
	TUint iLength;
	};


struct TAddressParms
	{
	TAddressParms(TUint32 aSrcAddr=0, TUint32 aDstAddr=0, TUint aTransferCount=0)
		:iSrcAddr(aSrcAddr), iDstAddr(aDstAddr), iTransferCount(aTransferCount)
		{}

	TAddressParms(const TDmaTransferArgs& aArgs)
		:iSrcAddr(aArgs.iSrcConfig.iAddr),
		iDstAddr(aArgs.iDstConfig.iAddr),
		iTransferCount(aArgs.iTransferCount)
		{}

	/**
	If addresses have been left as KPhysAddrInvalid or the count as 0 (ie.
	the default values used for IsrRedoRequest) then substitute the values from
	aTransferArgs.
	*/
	void Substitute(const TDmaTransferArgs& aTransferArgs);

	/**
	If addresses have been left as KPhysAddrInvalid or the count as 0 (ie.
	the default values used for IsrRedoRequest) then substitute the values from
	aTransferArgs.
	*/
	void Substitute(const TAddressParms& aTransferArgs);

	/**
	When received by the test driver, src and dst
	addresses will be offsets from the dma test session's
	chunk base. They must be converted to absolute, *physical* addresses
	*/
	void Fixup(TLinAddr aChunkBase);

	/**
	Check that both the src and destination lie within the area
	defined by aStart and aSize
	*/
	TBool CheckRange(TLinAddr aStart, TUint aSize);

	TAddrRange SourceRange() const;
	TAddrRange DestRange() const;

	TBool Overlaps(const TAddrRange aRange) const;
	TBool Overlaps(const TAddressParms aParm) const;

	TBool operator==(const TAddressParms& aOther) const;


	/**
	Produce a printable representation
	*/
	void AppendString(TDes& aBuf) const
		{
		_LIT(KOutput, "TAddressParms: src=0x%08x (%d) dst=0x%08x (%d) count=0x%08x (%d)\0");
#ifdef __KERNEL_MODE__
		DmaAppendFormat(aBuf, (const char*)KOutput().Ptr(), iSrcAddr, iSrcAddr, iDstAddr, iDstAddr, iTransferCount, iTransferCount);
#else
		aBuf.AppendFormat(KOutput, iSrcAddr, iSrcAddr, iDstAddr, iDstAddr, iTransferCount, iTransferCount);
#endif
		}

	void MakePhysical();

	static void SelfTest();

	TUint32 iSrcAddr;
	TUint32 iDstAddr;
	TUint iTransferCount;
	};

// These functions can be used for accessing TDmaTransferArgs in
// terms of TAddressParms. (TAddressParms would be a natural base
// class for TDmaTransferArgs but changing the production code
// is undesirable)
TAddressParms GetAddrParms(const TDmaTransferArgs&);
void SetAddrParms(TDmaTransferArgs&, const TAddressParms&);

/**
This struct holds the arguments which can be used with TDmaChannel::IsrRedoRequest
*/
struct TIsrRequeArgs : public TAddressParms
	{
	TIsrRequeArgs(TUint32 aSrcAddr=KPhysAddrInvalidUser, TUint32 aDstAddr=KPhysAddrInvalidUser,
			TUint aTransferCount=0, TUint32 aPslRequestInfo=0,
			TBool aIsrCb=ETrue)
		: TAddressParms(aSrcAddr, aDstAddr, aTransferCount), iPslRequestInfo(aPslRequestInfo), iIsrCb(aIsrCb)
		{}


	TInt Call(TDmaChannel& aChannel);

	TBool CheckRange(TLinAddr aStart, TUint aSize) const;

	TUint32 iPslRequestInfo;
	TBool iIsrCb;
	};
class CISrRequeTest;
/**
A collection of TIsrRequeArgs
*/
struct TIsrRequeArgsSet
	{
	friend class CIsrRequeTest;
	TIsrRequeArgsSet(TIsrRequeArgs* aRequeueArgs=NULL, TInt aCount =0)
		:iCount(aCount), iIndex(0)
		{
		TEST_ASSERT(iCount <= MaxCount);
		for(TInt i=0; i<iCount; i++)
			{
			iRequeArgs[i] = aRequeueArgs[i];
			}

		}

	TBool IsEmpty() const
		{return iCount == 0;}

	TIsrRequeArgs GetArgs();

	/**
	If addresses have been left as KPhysAddrInvalid or the count as 0 (ie.
	the default values used for IsrRedoRequest) then substitute the appropriate
	value from the previous argument struct. aTransferArgs is used to
	substitute values for the initial argument struct.
	*/
	void Substitute(const TDmaTransferArgs& aTransferArgs);
	void Fixup(TLinAddr aChunkBase);
	TBool CheckRange(TLinAddr aAddr, TUint aSize) const;

	/**
	Check that all re-queue parameters will remain within the region defined
	by aAddr and aSize. This overload assumes that the requeue parameters have
	not been substituted hence the data in aInitialParms is required.

	@param aInitialParms The original transfer that the re-queues in this set are based on
	*/
	TBool CheckRange(TLinAddr aAddr, TUint aSize, const TDmaTransferArgs& aInitialParms) const;

	static void SelfTest();
private:
	enum {MaxCount=6};
	TInt iCount;
	TInt iIndex;
	TIsrRequeArgs iRequeArgs[MaxCount];
	};

class DDmaTestSession;
class RDmaSession : public RBusLogicalChannel
	{
	friend class DDmaTestSession;
public:
#ifndef __KERNEL_MODE__
	TInt ChannelIsQueueEmpty(TUint aDriverCookie,TBool& aQueueEmpty)
		{
		return DoControl(EIsQueueEmpty, reinterpret_cast<TAny*>(aDriverCookie),	&aQueueEmpty);		
		}

	TInt ChannelIsOpened(TUint aDriverCookie,TBool &aChannelOpen)
		{
		return DoControl(EIsOpened, reinterpret_cast<TAny*>(aDriverCookie), &aChannelOpen);		
		}

	TInt ChannelCancelAll(TUint aDriverCookie)
		{	
		return DoControl(ECancelAllChannel, reinterpret_cast<TAny*>(aDriverCookie));
		}

	TInt ChannelOpen(TUint aPslCookie,  TUint& aDriverCookie)
		{
		return DoControl(EOpenChannel, reinterpret_cast<TAny*>(aPslCookie), &aDriverCookie);
		}

	TInt ChannelOpen(TUint& aDriverCookie, SCreateInfoTest& aInfo)
		{
		TPckg<SCreateInfoTest> package(aInfo);
		return DoControl(EOpenChannelExposed,&aDriverCookie, &package);
		}

	TInt ChannelClose(TUint aDriverCookie)
		{	
		return DoControl(ECloseChannel, reinterpret_cast<TAny*>(aDriverCookie));
		}

	TInt ChannelPause(TUint aDriverCookie)
		{	
		return DoControl(EPauseChannel, reinterpret_cast<TAny*>(aDriverCookie));
		}
	
	TInt ChannelResume(TUint aDriverCookie)
		{	
		return DoControl(EResumeChannel, reinterpret_cast<TAny*>(aDriverCookie));
		}

	TInt ChannelLinking(TUint aDriverCookie)
		{	
		return DoControl(ELinkChannel, reinterpret_cast<TAny*>(aDriverCookie));
		}

	TInt ChannelUnLinking(TUint aDriverCookie)
		{	
		return DoControl(EUnlinkChannel, reinterpret_cast<TAny*>(aDriverCookie));
		}

	TInt ChannelCaps(TUint aDriverCookie, SDmacCaps& aChannelCaps)
		{
		TDmacTestCaps caps;
		TInt r = ChannelCaps(aDriverCookie, caps);
		aChannelCaps = caps;
		return r;
		}

	TInt ChannelCaps(TUint aDriverCookie, TDmacTestCaps& aChannelCaps)
		{
		TPckg<TDmacTestCaps> package(aChannelCaps);
		return DoControl(EChannelCaps, reinterpret_cast<TAny*>(aDriverCookie), &package);
		}
	
	TInt Open()
		{
		TInt r = KErrNone;
		r = DoCreate(KTestDmaLddNameHw,TestDmaLddVersion(), 0, NULL, NULL, EOwnerThread);
		RDebug::Printf("RDmaSession::Open returned %d", r);
		return r;
		}

	TInt OpenSim()
		{
		return DoCreate(KTestDmaLddNameSim,TestDmaLddVersion(), 0, NULL, NULL, EOwnerThread);
		}

	TInt RequestCreateOld(TUint aChannelCookie, TUint& aRequestCookie, TUint aMaxTransferSize=0)
		{	
		return DoRequestCreate(aChannelCookie, EFalse, aMaxTransferSize, aRequestCookie);
		}


	TInt RequestCreate(TUint aChannelCookie, TUint& aRequestCookie, TUint aMaxTransferSize=0)
		{
		return DoRequestCreate(aChannelCookie, ETrue, aMaxTransferSize, aRequestCookie);
		}

	TInt RequestDestroy(TUint aRequestCookie)
		{	
		return DoControl(ERequestClose, reinterpret_cast<TAny*>(aRequestCookie));
		}

	TInt RequestFragmentCount(TUint aRequestCookie)
		{	
		return DoControl(EFragmentCount, reinterpret_cast<TAny*>(aRequestCookie));
		}

	TInt RequestEnableDstElementCounting(TUint aRequestCookie)
		{					
		return DoControl(EEnableDstElementCounting, reinterpret_cast<TAny*>(aRequestCookie));		
		}

	TInt RequestEnableSrcElementCounting(TUint aRequestCookie)
		{		
		return DoControl(EEnableSrcElementCounting, reinterpret_cast<TAny*>(aRequestCookie));
		}

	TInt RequestDisableDstElementCounting(TUint aRequestCookie)
		{	
		return DoControl(EDisableDstElementCounting, reinterpret_cast<TAny*>(aRequestCookie));
		}

	TInt RequestDisableSrcElementCounting(TUint aRequestCookie)
		{	
		return DoControl(EDisableSrcElementCounting, reinterpret_cast<TAny*>(aRequestCookie));
		}

	TInt RequestTotalNumDstElementsTransferred(TUint aRequestCookie)
		{	
		return DoControl(ETotalNumDstElementsTransferred, reinterpret_cast<TAny*>(aRequestCookie));
		}

	TInt RequestTotalNumSrcElementsTransferred(TUint aRequestCookie)
		{	
		return DoControl(ETotalNumSrcElementsTransferred, reinterpret_cast<TAny*>(aRequestCookie));
		}

	/**
	Will fragment a DMA request using the legacy API
	*/
	TInt FragmentRequestOld(TUint aRequestCookie, const TDmaTransferArgs& aTransferArgs, TUint64* aDurationMicroSecs=NULL)
		{
		const TFragmentArgs args(aRequestCookie, aTransferArgs, aDurationMicroSecs);
		TPckgC<TFragmentArgs> package(args);
		return DoControl(EFragmentLegacy, &package);
		}

	/**
	Will fragment a DMA request using the new API
	*/
	TInt FragmentRequest(TUint aRequestCookie, const TDmaTransferArgs& aTransferArgs, TUint64* aDurationMicroSecs=NULL)
		{
		const TFragmentArgs args(aRequestCookie, aTransferArgs, aDurationMicroSecs);
		TPckgC<TFragmentArgs> package(args);
		return DoControl(EFragment, &package);
		}

	TInt QueueRequest(TUint aRequestCookie, TRequestStatus& aStatus, TCallbackRecord* aRecord = NULL, TUint64* aDurationMicroSecs=NULL)
		{
		//These dummy values can accept the writeback from the driver
		//if the client does not want them.
		//(TClientDataRequest can not be programmed with a NULL to
		//indicate that an argument is unwanted)
		TCallbackRecord dummyRec;
		TUint64 dummyTime=0;

		aStatus = KRequestPending;

		TQueueArgs args(aRequestCookie, &aStatus, aRecord ? aRecord : &dummyRec, aDurationMicroSecs ? aDurationMicroSecs : &dummyTime);
		TPckgC<TQueueArgs> package(args);
		return DoControl(EQueueRequest, &package);
		}

	/**
	Synchronous version of QueueRequest
	*/
	TInt QueueRequest(TUint aRequestCookie, TCallbackRecord* aRecord = NULL, TUint64* aDurationMicroSecs=NULL)
		{
		TRequestStatus status;
		TInt r = QueueRequest(aRequestCookie, status, aRecord, aDurationMicroSecs);
		User::WaitForRequest(status);
		return r;
		}

	/**
	Queue a previously fragmented request.
	Additional request parameters are included in iRequeueArgs, these will be
	transferred from ISR context callback using the TDmaChannel::IsrRedoRequest function

	@pre Isr callback for completion must have been requested at request fragmentation time
	*/
	TInt QueueRequestWithRequeue(TUint aRequestCookie, TIsrRequeArgs* aRequeueArgs, TInt aCount, TRequestStatus& aStatus, TCallbackRecord* aRecord = NULL, TUint64* aDurationMicroSecs=NULL)
		{
		//These dummy values can accept the writeback from the driver
		//if the client does not want them.
		//(TClientDataRequest can not be programmed with a NULL to
		//indicate that an argument is unwanted)
		TCallbackRecord dummyRec;
		TUint64 dummyTime=0;

		aStatus = KRequestPending;

		TQueueArgsWithReque args(aRequeueArgs, aCount, aRequestCookie, &aStatus, aRecord ? aRecord : &dummyRec, aDurationMicroSecs ? aDurationMicroSecs : &dummyTime);
		TPckgC<TQueueArgsWithReque> package(args);
		return DoControl(EQueueRequestWithReque, &package);
		}

	/**
	Synchronous version of QueueRequestWithRequeue
	*/
	TInt QueueRequestWithRequeue(TUint aRequestCookie, TIsrRequeArgs* aRequeueArgs, TInt aCount, TCallbackRecord* aRecord = NULL, TUint64* aDurationMicroSecs=NULL)
		{
		TRequestStatus status;
		TInt r = QueueRequestWithRequeue(aRequestCookie, aRequeueArgs, aCount, status, aRecord, aDurationMicroSecs);
		User::WaitForRequest(status);
		return r;
		}

	TInt OpenSharedChunk(RChunk& aChunk)
		{
		TUint chunkHandle = DoControl(EOpenSharedChunk);
		return aChunk.SetReturnedHandle(chunkHandle);
		}

	TInt GetTestInfo(TDmaV2TestInfo& aInfo)
		{
		TPckg<TDmaV2TestInfo> package(aInfo);
		return DoControl(EGetTestInfo, &package);
		}

	static void SelfTest(TBool aSimulatedDmac);

	static void ApiTest();
#endif // __KERNEL_MODE__

private:

	TInt DoRequestCreate(TUint aChannelCookie, TBool aNewStyle, TUint aMaxTransferSize, TUint& aRequestCookie)
		{
		TRequestCreateArgs args(aChannelCookie, aNewStyle, aMaxTransferSize);
		TPckgC<TRequestCreateArgs> package(args);
		return DoControl(ERequestOpen, &package, &aRequestCookie);
		}
	
	struct TRequestCreateArgs
		{
		TRequestCreateArgs(TUint aChannelCookie, TBool aNewStyle, TUint aMaxFragmentSize)
			:iChannelCookie(aChannelCookie), iNewStyle(aNewStyle), iMaxFragmentSize(aMaxFragmentSize)
			{}

		TUint iChannelCookie;
		TBool iNewStyle;
		TUint iMaxFragmentSize;
		};

	struct TFragmentArgs
		{
		TFragmentArgs()
			:iRequestCookie(0), iTransferArgs(), iDurationMicroSecs(NULL)
			{}
		TFragmentArgs(TUint aRequestCookie, const TDmaTransferArgs& aTransferArgs, TUint64* aDurationMicroSecs = NULL)
			:iRequestCookie(aRequestCookie), iTransferArgs(aTransferArgs), iDurationMicroSecs(aDurationMicroSecs)
			{}

		const TUint iRequestCookie;
		const TDmaTransferArgs iTransferArgs;
		TUint64* const iDurationMicroSecs;
		};

	struct TQueueArgs
		{
		TQueueArgs(TUint aRequestCookie=0, TRequestStatus* aStatus=NULL, TCallbackRecord* aCallbackRecord=NULL, TUint64* aDurationMicroSecs=NULL)
			:iRequestCookie(aRequestCookie), iStatus(aStatus), iCallbackRecord(aCallbackRecord), iDurationMicroSecs(aDurationMicroSecs)
			{}
		TUint iRequestCookie;
		TRequestStatus* iStatus;
		TCallbackRecord* iCallbackRecord;
		TUint64* iDurationMicroSecs;
		};

	/**
	This struct is used for queing and including a set of transfers
	to be setup from ISR context callback
	*/
	struct TQueueArgsWithReque : public TQueueArgs
		{
		TQueueArgsWithReque(TIsrRequeArgs* aRequeueArgs=NULL, TInt aCount=0,
				TUint aRequestCookie=0, TRequestStatus* aStatus=NULL, TCallbackRecord* aCallbackRecord=NULL, TUint64* aDurationMicroSecs=NULL)
			:TQueueArgs(aRequestCookie, aStatus, aCallbackRecord, aDurationMicroSecs), iRequeSet(aRequeueArgs, aCount)
			{
			}

		TIsrRequeArgsSet iRequeSet;
		};

	enum TControl
		{
		EOpenChannel,
		EOpenChannelExposed,
		ECloseChannel,
		EPauseChannel,
		EResumeChannel,
		EChannelCaps,
		ERequestOpen,
		ERequestClose,
		EOpenSharedChunk,
		EFragmentLegacy,
		EFragment,
		EFragmentCount,
		EQueueRequest,
		EGetTestInfo,
		EIsQueueEmpty,
		EIsOpened,
		EIsrRedoRequest,
		ECancelAllChannel,
		EQueueRequestWithReque,
		ELinkChannel,
		EUnlinkChannel,
		EEnableDstElementCounting,
		EEnableSrcElementCounting,
		EDisableDstElementCounting,
		EDisableSrcElementCounting,
		ETotalNumDstElementsTransferred,
		ETotalNumSrcElementsTransferred,
		};
	};
#endif // __D_DMA2_H__
