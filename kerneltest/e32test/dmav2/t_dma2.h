/*
* Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#ifndef __T_DMA2_H__
#define __T_DMA2_H__

#include "cap_reqs.h"
#include "test_thread.h"
#include "d_dma2.h"
#include <e32std.h>

class TTestCase;
// Global array of test cases
extern RPointerArray<TTestCase> TestArray;
extern RPointerArray<TTestCase> TestArrayCallback;
extern RPointerArray<TTestCase> TestArrayIsrReque;
extern RPointerArray<TTestCase> TestArrayMultiPart;
extern RPointerArray<TTestCase> TestArrayIsrAndDfc;
extern RPointerArray<TTestCase> TestArrayBenchmark;
extern RPointerArray<TTestCase> TestArray2DTest;
extern RPointerArray<TTestCase> TestArrayIsrAndDfc;
extern RPointerArray<TTestCase> TestArrayChannel;
extern RPointerArray<TTestCase> TestArraySuspend;
extern RPointerArray<TTestCase> TestArrayQueue;
extern RPointerArray<TTestCase>	TestArraySimple;
extern RPointerArray<TTestCase>	TestArrayRequest;
extern RPointerArray<TTestCase>	TestArrayFragment;

extern TBool gVerboseOutput;   // Verbose output control

const TInt KParameterTextLenMax = 80;	// command-line param length

/**
This function prints out the PSL test Information
*/
void Print(const TDmaV2TestInfo& aInfo);

/**
Runs all framework self tests
*/
void SelfTests();

class CSingleTransferTest;
class CIsrRequeTest;
class CMultiTransferTest;

/**
An interface to a classs that sets up the buffers before a test
*/
class MPreTransfer
	{
public:
	virtual ~MPreTransfer()
		{}
	virtual void Setup(const CSingleTransferTest& aTest) const = 0;
	virtual void Setup(const CIsrRequeTest& aTest) const = 0;
	virtual void Setup(const CMultiTransferTest& aTest) const = 0;
	};

/**
An interface for a check which takes place at the end of a DMA
transfer test to verify the transfer was as expected.
*/
class MPostTransferCheck
	{
public:
	virtual ~MPostTransferCheck()
		{}
	virtual TInt Check(const CSingleTransferTest& aTest) const = 0;
	virtual TInt Check(const CIsrRequeTest& aTest) const = 0;
	virtual TInt Check(CMultiTransferTest& aTest) const = 0;
	};

class TCompare2D : public MPostTransferCheck
	{
public:
	TCompare2D()
		{}

	virtual TInt Check(const CSingleTransferTest& aTest) const;
	virtual TInt Check(const CIsrRequeTest& aTest) const;
	virtual TInt Check(CMultiTransferTest& aTest) const;

	};

class TAlwaysFail : public MPostTransferCheck
	{
public:
	virtual TInt Check(const CSingleTransferTest& /*aTest*/) const
		{return KErrUnknown;}
	virtual TInt Check(const CIsrRequeTest&) const
		{return KErrUnknown;}
	virtual TInt Check(CMultiTransferTest&) const
		{return KErrUnknown;}
	};

class TAlwaysPass : public MPostTransferCheck
	{
public:
	virtual TInt Check(const CSingleTransferTest& /*aTest*/) const
		{return KErrNone;}
	virtual TInt Check(const CIsrRequeTest&) const
		{return KErrNone;}
	virtual TInt Check(CMultiTransferTest&) const
		{return KErrNone;}
	};

/**
Compare that all the various source buffers of a test match
its destination buffers
*/
class TCompareSrcDst : public MPostTransferCheck
	{
public:
	TCompareSrcDst()
		{}

	virtual TInt Check(const CSingleTransferTest& aTest) const;
	virtual TInt Check(const CIsrRequeTest& aTest) const;
	virtual TInt Check(CMultiTransferTest& aTest) const;

protected:
	TInt Check(const TIsrRequeArgsSet& aRequeueArgSet, TUint8* aChunkBase, const TDmaTransferArgs& aTferArgs) const;
	TInt Check(const TIsrRequeArgs& aRequeueArgs) const;
	TInt Check(const TDmaTransferArgs& aTransferArgs, TUint8* aChunkBase) const;
	};

/**
Check whether destination buffers are zero filled

Used to check that a transfer hasn't taken place.
*/
class TCheckNoTransfer : public MPostTransferCheck
	{
public:
	TCheckNoTransfer()
		{}

	virtual TInt Check(const CSingleTransferTest& aTest) const;
	virtual TInt Check(const CIsrRequeTest& aTest) const;
	virtual TInt Check(CMultiTransferTest& aTest) const;

protected:
	TBool IsZeroed(const TDmaTransferArgs& aTransferArgs, TUint8* aChunkBase) const;
	};

/**
Base class for all DMA tests
*/
class CDmaTest : public CTest
	{
public:
	CDmaTest(const TDesC& aName, TInt aIterations, const MPreTransfer* aPreTransfer, const MPostTransferCheck* aPostTransfer)
		: CTest(aName, aIterations), iPreTransfer(aPreTransfer), iPostTransferCheck(aPostTransfer)
		{}

	void OpenDmaSession();
	/* Duplicate aSession */
	void OpenDmaSession(const RDmaSession& aSession);
	void CloseDmaSession();
	void ChannelPause(const TUint aChannelSessionCookie);
	void ChannelResume(const TUint aChannelSessionCookie);
	virtual void PrintTestInfo() const;
	virtual TBool Result() = 0;

	const RChunk& Chunk() const
		{return iChunk;}

	/**
	Tells the test which DMA channel it should run on
	*/
	void SetChannelCookie(TUint32 aCookie)
		{iChannelCookie = aCookie;}

	virtual void PreTransferSetup();
	virtual TInt DoPostTransferCheck();
protected:
	RDmaSession iDmaSession;
	RChunk iChunk;

	/**
	Identifies the channel to open (as understood by a DMA PSL)
	*/
	TUint iChannelCookie;
	const MPreTransfer* iPreTransfer;

	const MPostTransferCheck* iPostTransferCheck; //!< Some check to be run after the transfer
	};

/**
The Decorator Pattern is used allowing test classes to be optionally extended
using wrapper/decorator classes.
This is the base class for test decorators
*/
class CDmaTestDecorator : public CDmaTest
	{
public:

protected:
	CDmaTestDecorator(CDmaTest* aDecoratedTest);
	CDmaTestDecorator(const CDmaTestDecorator& aOther);

	CDmaTest* iDecoratedTest;
	};

/**
Will run the wrapped test against both versions of the DMA
API if available, otherwise just the old version.
*/
class CMultiVersionTest : public CDmaTestDecorator
	{
public:
	CMultiVersionTest(CSingleTransferTest* aDmaTest); 
	CMultiVersionTest(const CMultiVersionTest& aOther);
	~CMultiVersionTest();

	virtual void Announce() const;
	virtual void PrintTestType() const;
	virtual void PrintTestInfo() const; 

	virtual CTest* Clone() const {return new CMultiVersionTest(*this);}
	virtual void SetupL();

	virtual void RunTest();
	virtual TBool Result();

protected:
	void Configure();
	TBool Version2PILAvailable();
	CSingleTransferTest* iNewVersionTest;
	};

/**
Holds return codes for the various functions which must be called
to create, fragment, and queue a DMA request
*/
struct TRequestResults
	{
	TRequestResults
		(
		TInt aCreate = KErrNone,
		TInt aFragmentCount = 0,
		TInt aFragmentationResult = KErrNone,
		TInt aQueueResult = KErrNone
		)
		:iCreate(aCreate), 
		 iFragmentCount(aFragmentCount), 
		 iFragmentationResult(aFragmentationResult), 
		 iQueueResult(aQueueResult)
		{}

	/**
	Constructs with error results
	*/
	TRequestResults(TFalse)
		:iCreate(KErrUnknown), 
		 iFragmentCount(0), 
		 iFragmentationResult(KErrUnknown), 
		 iQueueResult(KErrUnknown)
		{}

	inline TRequestResults& CreationResult(TInt aErrorCode) {iCreate = aErrorCode; return *this;}
	inline TRequestResults& FragmentCount(TInt aCount) {iFragmentCount = aCount; return *this;}
	inline TRequestResults& FragmentationResult(TInt aErrorCode) {iFragmentationResult = aErrorCode; return *this;}
	inline TRequestResults& QueueResult(TInt aErrorCode) {iQueueResult = aErrorCode; return *this;}

	TInt iCreate;
	TInt iFragmentCount; //!< 0 means any result permitted
	TInt iFragmentationResult;
	TInt iQueueResult;
	};

/**
Holds all the results for a DMA CSingleTransferTest
*/
struct TResultSet
	{
	/**
	No errors expected
	*/
	TResultSet(TInt aChannelOpenResult = KErrNone,
			const TRequestResults aRequestResults = TRequestResults(),
			TInt aPostTransferCheck = KErrNone,
			const TCallbackRecord aCallbackRecord = TCallbackRecord(TCallbackRecord::EThread,1)
			)
		:
		iChannelOpenResult(aChannelOpenResult),
		iRequestResult(aRequestResults),
		iPostTransferCheck(aPostTransferCheck),
		iCallbackRecord(aCallbackRecord)
		{}
	
	explicit TResultSet(const TCallbackRecord& aRecord)
		:iChannelOpenResult(KErrNone),
		 iRequestResult(),
		 iPostTransferCheck(KErrNone),
		 iCallbackRecord(aRecord)
		{}

	/**
	Errors expected
	*/
	TResultSet(TFalse)
		:iChannelOpenResult(KErrUnknown), 
		iRequestResult(EFalse),
		iPostTransferCheck(KErrUnknown),
		iCallbackRecord(TCallbackRecord::Empty())
		{}		
	
	void Print() const;
	TBool operator == (const TResultSet& aOther) const;

	/** Set channel opening result */
	TResultSet& ChannelOpenResult(TInt aResult) {iChannelOpenResult = aResult; return *this;}
	TResultSet& PostTransferResult(TInt aResult) {iPostTransferCheck = aResult; return *this;}
	/** Set request results */
	TResultSet& RequestResult(const TRequestResults& aResults) {iRequestResult = aResults; return *this;}
	/** Set Callback record */
	TResultSet& CallbackRecord(const TCallbackRecord& aCallbackRecord) {iCallbackRecord = aCallbackRecord; return *this;}

	TInt iChannelOpenResult;
	TRequestResults iRequestResult;
	TInt iPostTransferCheck;
	TCallbackRecord iCallbackRecord;
	};

/**
Fills each source buffer with an increasing value and clears each destination
*/
class TPreTransferIncrBytes : public MPreTransfer
	{
public:
	TPreTransferIncrBytes()
		{}

	virtual void Setup(const CSingleTransferTest& aTest) const;
	virtual void Setup(const CIsrRequeTest& aTest) const;
	virtual void Setup(const CMultiTransferTest& aTest) const;

	static void SelfTest();
protected:
	virtual void Setup(const TAddressParms& aParams) const;

	TBool CheckBuffers(const CIsrRequeTest& aTest) const;
	TBool CheckBuffers(const CMultiTransferTest& aTest) const;

	TBool CheckBuffers(const RArray<const TAddressParms>& aTransferParams, TBool aAllowExactRepeat=ETrue) const;

	// This function is part of the unit test
	friend TBool DoTferParmTestL(const TAddressParms* aParms, TInt aCount, TBool aAllowRepeat, TBool aPositive);
	};

const TPreTransferIncrBytes KPreTransferIncrBytes;
const TCompareSrcDst KCompareSrcDst;
const TCompare2D KCompare2D;
const TCheckNoTransfer KCheckNoTransfer;


/**
Iterates over the bytes in buffer, in the order
the supllied DMA config would access them
*/
class TTransferIter
	{
public:
	TTransferIter()
		:iCfg(NULL), iPtr(NULL)
		{}

	TTransferIter(const TDmaTransferConfig& aCfg, TUint8* aChunkBase=NULL)
		:iElem(0), iFrame(0), iCfg(&aCfg), iChunkBase(aChunkBase), iPtr(Start()), iBytes(0)
		{}

	void operator++ ();
	TUint8& operator* ()
		{
		Invariant();
		return *iPtr;
		}

	TBool operator!= (const TTransferIter& aOther)
		{
		return (iPtr != aOther.iPtr);
		}

	static void SelfTest();
private:
	TUint8* Start() const
		{
		return iChunkBase + iCfg->iAddr;
		}

	void Invariant() const;

	TUint iElem; //!< The current element
	TUint iFrame; //!< The current frame

	const TDmaTransferConfig* const iCfg;
	TUint8* iChunkBase;

	TUint8* iPtr; //<! Pointer to the current byte

	TUint iBytes; //!< The number of bytes traversed
	};

/**
Performs a single DMA transfer using the member TDmaTransferArgs on
one channel. At each stage of the transfer results are recorded in a
TResultSet struct: at the end these are compared with a set of expected
results.
*/
class CSingleTransferTest : public CDmaTest
	{
public:
	CSingleTransferTest(
			const TDesC& aName, TInt aIterations,
			const TDmaTransferArgs& aArgs,
			const TResultSet& aExpected,
			TUint aMaxFragmentSize = 0,
			const MPostTransferCheck* aPostTferChk = &KCompareSrcDst,
			const MPreTransfer* aPreTfer = &KPreTransferIncrBytes
			)
		: CDmaTest(aName, aIterations, aPreTfer, aPostTferChk),
		iTransferArgs(aArgs),iExpected(aExpected),iActual(EFalse),
		iUseNewRequest(ETrue),
		iUseNewFragment(ETrue),
		iMaxFragmentSize(aMaxFragmentSize)
		{}

	/**
	Perform each stage of trasnfer
	*/
	virtual void RunTest();
	virtual void PrintTestType() const;
	virtual void PrintTestInfo() const;

	virtual CTest* Clone() const {return new CSingleTransferTest(*this);}

	/**
	Compares the actual vs the exepected results and reports
	of the test passed
	@return ETrue for a pass, EFalse for a fail
	 */
	virtual TBool Result();

	/**
	An accessor function for the object's TDmaTransferArgs
	*/
	const TDmaTransferArgs& TransferArgs() const
		{return iTransferArgs;}

	// The below methods are setters, which may be chained together
	// ie. The Named Parameter Idiom
	// @see http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.18
	inline CSingleTransferTest& UseNewRequest(TBool aFlag) {iUseNewRequest=aFlag; return *this;}
	inline CSingleTransferTest& UseNewFragment(TBool aFlag) {iUseNewFragment=aFlag; return *this;}
	inline CSingleTransferTest& UseNewDmaApi(TBool aFlag) {UseNewRequest(aFlag); UseNewFragment(aFlag); return *this;}

protected:
	virtual void OpenChannel();
	virtual void PreTransferSetup();
	virtual void CreateDmaRequest();
	virtual void Fragment();
	virtual void Queue();
	virtual void PostTransferCheck();
	virtual TInt DoPostTransferCheck();
	virtual void FreeRequest();
	virtual void CloseChannel();

protected:
	/**
	A handle to kernel side TDmaChannel object received after a channel is opened.
	*/
	TUint iChannelSessionCookie;
	/**
	A handle to kernel side DDmaRequest object.
	*/
	TUint iRequestSessionCookie;

	const TDmaTransferArgs& iTransferArgs;

	/**
	Expected transfer results
	*/
	TResultSet iExpected;

	/**
	Filled with actual transfer results
	*/
	TResultSet iActual;

	TBool iUseNewRequest; //!< If true then CSingleTransferTest will create a DDmaRequest with the v2 ctor
	TBool iUseNewFragment; //!< If true then CSingleTransferTest will use v2 Fragment API
	const TUint iMaxFragmentSize;
	};

/**
This class will be used for testing DMA Close() and Open() API

Extends CDmaTest by implemeting a RunTest() with a sequence of operations 
to test Close() and Open() API 
*/
class COpenCloseTest : public CDmaTest 
	{
public:
	COpenCloseTest(
			const TDesC& aName, TInt aIterations,	
			const MPostTransferCheck* aPostTferChk = NULL,
			const MPreTransfer* aPreTfer = NULL
			)
		: CDmaTest(aName, aIterations, aPreTfer, aPostTferChk), iOpenCloseResult(EFalse) , iRunOpen(EFalse)
		{}

	~COpenCloseTest();

	virtual void RunTest();
	virtual void PrintTestType() const;

	virtual CTest* Clone() const {return new COpenCloseTest(*this);}
	
	/**
	Checks the results of the sequeunce of  sequence of operations 
	to test Close() and Open() API, return ETrue for a pass, EFalse for a fail
	 */
	virtual TBool Result();

	// The methods below is a setters ie. The Named Parameter Idiom
	// @see http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.18
	inline COpenCloseTest& RunOpenApiTest(TBool aFlag) {iRunOpen=aFlag; return *this;}

protected:	
	TBool DoRunClose();
	TBool DoRunOpen();
	TBool DoRunOpenExposed();

protected:
	/**
	A handle to kernel side TDmaChannel object received after a channel is opened.
	*/
	TUint iChannelSessionCookie;
	/**
	A handle to kernel side DDmaRequest object.
	*/
	TUint iRequestSessionCookie;

	/**
	If true then Close/Open  API test passed
	*/
	TBool iOpenCloseResult;
	
	/**
	 If true then run Open API test otherwise run Close API test
	*/
	TBool iRunOpen;
	};

/**
Used for testing Pause and Resume

Extends CSingle transfer by adding the capability to test
Pause  & Resume() API.
*/
class CPauseResumeTest : public CSingleTransferTest
	{
public:
	 CPauseResumeTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs& aArgs, const TResultSet& aExpected)
		:CSingleTransferTest(aName, aIterations, aArgs, aExpected)
	 {}

	~CPauseResumeTest();

	virtual void RunTest();
	virtual void PrintTestType() const;

	virtual CTest* Clone() const {return new  CPauseResumeTest(*this);}

	// The methods below is a setters ie. The Named Parameter Idiom
	// @see http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.18
	inline CPauseResumeTest& UseNewDmaApi(TBool aFlag) {CSingleTransferTest::UseNewDmaApi(aFlag); return *this;}

protected:
	void DoCalibrationTransfer(TUint64 &atime);
	TInt QueueAsyncRequest(TRequestStatus &aRequestState,TUint64 &atime);
	};

/**
Used for testing Pause and Resume ( Negative Testing)

Extends CSingle transfer by adding the capability to test
Pause  & Resume() API. Expects that Pause and Resume is not supported
*/
class CPauseResumeNegTest : public CSingleTransferTest
	{
public:
	 CPauseResumeNegTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs& aArgs, const TResultSet& aExpected)
		:CSingleTransferTest(aName, aIterations, aArgs, aExpected)
	 {}

	~CPauseResumeNegTest();

	virtual void RunTest();
	virtual void PrintTestType() const;

	virtual CTest* Clone() const {return new  CPauseResumeNegTest(*this);}

	// The methods below is a setters ie. The Named Parameter Idiom
	// @see http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.18
	inline CPauseResumeNegTest& UseNewDmaApi(TBool aFlag) {CSingleTransferTest::UseNewDmaApi(aFlag); return *this;}
	};


/**
Used for testing element counting 

Extends CSingle transfer by adding the capability to test
Element Counting APIs
*/
class CElementCountingTest : public CSingleTransferTest
	{
public:
	 CElementCountingTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs& aArgs, const TResultSet& aExpected)
		:CSingleTransferTest(aName, aIterations, aArgs, aExpected)
	 {}

	~CElementCountingTest();

	virtual void RunTest();
	virtual void PrintTestType() const;

	virtual CTest* Clone() const {return new CElementCountingTest(*this);}

	// The methods below is a setters ie. The Named Parameter Idiom
	// @see http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.18
	inline CElementCountingTest& UseNewDmaApi(TBool aFlag) {CSingleTransferTest::UseNewDmaApi(aFlag); return *this;}
	};


/**
Used for testing Linking of DMA Channels ( Negative Testing)

Extends CSingle transfer by adding the capability to test DMA channel linking
Expects that channel linking is not supported
*/
class CLinkChannelTest : public CSingleTransferTest
	{
public:
	 CLinkChannelTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs& aArgs, const TResultSet& aExpected)
		:CSingleTransferTest(aName, aIterations, aArgs, aExpected)
	 {}

	~CLinkChannelTest();

	virtual void RunTest();
	virtual void PrintTestType() const;

	virtual CTest* Clone() const {return new  CLinkChannelTest(*this);}

	// The methods below is a setters ie. The Named Parameter Idiom
	// @see http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.18
	inline CLinkChannelTest& UseNewDmaApi(TBool aFlag) {CSingleTransferTest::UseNewDmaApi(aFlag); return *this;}
	};


/**
This class will be used for tests which benchmark certain DMA operations
*/
class CDmaBenchmark : public CSingleTransferTest
	{
public:
	CDmaBenchmark(const TDesC& aName, TInt aIterations, const TResultSet& aExpectedResults, const TDmaTransferArgs& aTransferArgs, TUint aMaxFragmentSize);
	CDmaBenchmark(const CDmaBenchmark& aOriginal);
	~CDmaBenchmark();

	virtual TBool Result();

	static void SelfTest();

protected:
	/**
	@return The mean average of the result array
	*/
	TUint64 MeanResult();

	RArray<TUint64> iResultArray;
	};

/**
Fragments requests (only) and records duration
*/
class CDmaBmFragmentation : public CDmaBenchmark
	{
public:
	CDmaBmFragmentation(const TDesC& aName, TInt aIterations, const TDmaTransferArgs& aTransferArgs, TUint aMaxFragmentSize);
	virtual CTest* Clone() const {return new CDmaBmFragmentation(*this);}
	virtual TInt DoPostTransferCheck()
		{TEST_FAULT; return KErrNotSupported;}

	virtual void RunTest();
	virtual void PrintTestType() const;

protected:
	void Fragment();
	static const TResultSet ExpectedResults;
	};

/**
Performs a transfer using an old style DDmaRequest and
records the duration
*/
class CDmaBmTransfer : public CDmaBenchmark
	{
public:
	CDmaBmTransfer(const TDesC& aName, TInt aIterations, const TDmaTransferArgs& aTransferArgs, TUint aMaxFragmentSize);
	virtual CTest* Clone() const {return new CDmaBmTransfer(*this);}
	virtual TInt DoPostTransferCheck()
		{TEST_FAULT; return KErrNotSupported;}

	virtual void RunTest();
	virtual void PrintTestType() const;

	inline CDmaBmTransfer& UseNewDmaApi(TBool aFlag) {CSingleTransferTest::UseNewDmaApi(aFlag); return *this;}
	inline CDmaBmTransfer& ExpectedResults(const TResultSet& aArgs) {iExpected=aArgs; return *this;}
protected:
	void Queue();
	};



/**
Will create and queue multiple requests

Unlike CSingleTransferTest the class does not permit the use of TResultSet to
define expected results (for neagative testing)
*/
class CMultiTransferTest : public CDmaTest
	{
public:
	CMultiTransferTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs* aTransferArgs, const TResultSet* aResultSets, TInt aCount);
	CMultiTransferTest(const CMultiTransferTest& aOther);
	virtual ~CMultiTransferTest();
	virtual CTest* Clone() const {return new CMultiTransferTest(*this);}

	virtual TBool Result();
	virtual void RunTest();
	virtual void PrintTestType() const;

	inline CMultiTransferTest& PauseWhileQueuing() {iPauseWhileQueuing = ETrue; return *this;}
	inline CMultiTransferTest& SetPreTransferTest(const MPreTransfer* aPreTfer) {iPreTransfer = aPreTfer; return *this;}
	inline CMultiTransferTest& SetPostTransferTest(const MPostTransferCheck* aPostTfer) {iPostTransferCheck = aPostTfer; return *this;}

	const TDmaTransferArgs& TransferArgs(TInt aIndex) const;
	inline TInt TransferCount() const {return iTransferArgsCount;}

	void SetPostTransferResult(TInt aIndex, TInt aErrorCode);
protected:
	void OpenChannel();
	TInt CloseChannel();
	void CreateDmaRequests();
	void Fragment();
	void QueueRequests();

	virtual void PreTransferSetup();
	virtual TInt DoPostTransferCheck();

	TBool Result(TInt aTransfer);

	const TDmaTransferArgs* const iTransferArgs; //pointer to an array of transfer args
	const TInt iTransferArgsCount;


	TBool iNewDmaApi; //!< If true then CMultiTransferTest will use new style API

	/**
	A handle to kernel side TDmaChannel object received after a channel is opened.
	*/
	TUint iChannelSessionCookie;
	RArray<TUint> iRequestCookies;

	const TResultSet* const iExpectedArray; // array will be of length iTransferArgsCount
	RArray<TResultSet> iActualResults;

	/**
	If set, the test will pause the channel before queuing requests, and
	resume once they are all queued
	*/
	TBool iPauseWhileQueuing;
	};

/**
Used for testing TDmaChannel::IsQueueEmpty
Extends CMultiTransferTest by adding the capability to test IsQueueEmpty() API. 
*/
class CIsQueueEmptyTest : public  CMultiTransferTest 
	{
public:
	CIsQueueEmptyTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs* aTransferArgs, const TResultSet* aResultSets, TInt aCount);
	CIsQueueEmptyTest(const CIsQueueEmptyTest& aOther);
	~CIsQueueEmptyTest();

	inline CIsQueueEmptyTest& SetPreTransferTest(const MPreTransfer* aPreTfer) {iPreTransfer = aPreTfer; return *this;}
	inline CIsQueueEmptyTest& SetPostTransferTest(const MPostTransferCheck* aPostTfer) {iPostTransferCheck = aPostTfer; return *this;}

	virtual CTest* Clone() const {return new CIsQueueEmptyTest(*this);}
	virtual void RunTest();
	virtual void PrintTestType() const;

protected:
	void DoQueueNotEmpty();	
	void DoIsQueueEmpty();
	void QueueRequests();
	};

/**
Used for testing CancelAll, Will create and queue multiple requests
Extends CSingle transfer by adding the capability to test CancelAll API
*/
class CCancelAllTest : public CMultiTransferTest
	{
public:
	CCancelAllTest(const TDesC& aName, TInt aIterations,
		const TDmaTransferArgs* aTransferArgs, const TResultSet* aResultSets,
		TInt aCount
		);
	//CCancelAllTest(const CCacheNotifyDirCh

	virtual void RunTest();
	virtual void PrintTestType() const;
	virtual CTest* Clone() const {return new CCancelAllTest(*this);}

	inline CCancelAllTest& PauseWhileQueuing()
		{iPauseWhileQueuing = ETrue; return *this;}
	inline CCancelAllTest& SetPreTransferTest(const MPreTransfer* aPreTfer)
		{iPreTransfer = aPreTfer; return *this;}
	inline CCancelAllTest& SetPostTransferTest(const MPostTransferCheck* aPostTfer)
		{iPostTransferCheck = aPostTfer; return *this;}

protected:
	void QueueRequestsAsync();
	TInt CancelAllRequests();
	void PauseChannel();
	void ResumeChannel();

	/**
	A single request status that we use for all
	asynchronously queued requests (we do not intend
	to wait for them)
	*/
	TRequestStatus iDummyRequestStatus;
	};

/**
Used for testing TDmaChannel::IsrRedoRequest

Extends CSingle transfer by adding the capability to queue with
additonal transfer parameters (TIsrRequeArgs) which are passed
to IsrRedoRequest in ISR callback
*/
class CIsrRequeTest : public CSingleTransferTest
	{
public:
	CIsrRequeTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs& aArgs,
			TIsrRequeArgs* aRequeueArgs, TInt aCount,
			const TResultSet& aExpected, const MPreTransfer* aPreTfer,
			const MPostTransferCheck* aPostTferChk, TUint aMaxFragmentSize=0);

	virtual void PrintTestType() const;

	virtual void Queue();
	virtual CTest* Clone() const {return new CIsrRequeTest(*this);}

	const TIsrRequeArgsSet& GetRequeueArgs() const
		{return iRequeArgSet;}


protected:
	virtual TInt DoPostTransferCheck();
	virtual void PreTransferSetup();

	TIsrRequeArgsSet iRequeArgSet;
	};

/**
A channel record collects DMA channel capabilities and other PSL information
before running tests.
*/
class TChannelRecord
	{
public:
	TChannelRecord(){}
	~TChannelRecord(){}

	/**
	DMA Channel Cookie
	*/
	TUint   iCookie;

	/**
	DMA Channel Capabilities
	*/
	TDmacTestCaps iChannelCaps;
	};

/**
A test case collects together a DMA test (CDmaTest), its hardware prerequisites,
and other information about how the test should be run.
*/
class TTestCase
	{
public:
	TTestCase(CDmaTest* aTest,
           TBool aConcurrent = EFalse,
		   const TDmaCapability = TDmaCapability(),
		   const TDmaCapability = TDmaCapability(),
		   const TDmaCapability = TDmaCapability(),
		   const TDmaCapability = TDmaCapability(),
		   const TDmaCapability = TDmaCapability()
		   );

	static void SelfTest();

	/**
	Compares the requirements held in the class
	against those described in aChannelCaps and makes a decision
	as to whether this test case should be run, skipped, or failed.
	*/
	TResult TestCaseValid(const SDmacCaps& aChannelCaps) const;
	TResult TestCaseValid(const TDmacTestCaps& aChannelCaps) const;

	enum {KMaxChannelCaps=5};
	TDmaCapability	iChannelCaps[KMaxChannelCaps];
	TUint iChannelType;
	TInt iTimeout;
	CDmaTest* iTest;
	TBool iConcurrentTest;
	TBool iDmaV2Only; //!< If true then this test cannot be run on DMA v1 framework
	};

/**
A TestRunner manages the whole testing process.Before running any test cases it will open its own RDmaSession 
handle, not associated with a DMA channel, so that it can recover the TDmaTestInfo object (as used by the 
existing DMA framework) which says what channels are available to be tested.It will use TTestThread objects 
to run tests in new threads.TTestThread contains a number of useful features such as waiting for thread exit 
and accepting a TFunctor object to be run in a new thread. 
*/
class TTestRunner
{
public:
	TTestRunner();
	~TTestRunner();

	/**
	This function will populate TTestRunner with an array of test cases
	to be run

	@param aTTestCases Array of test cases
	*/
	void AddTestCases(RPointerArray<TTestCase>& aTTestCases);

	/**
	This will iterate over all test cases held by the test runner and
	for each one will judge which DMA channels it can be run on, running
	the test if possible.
	*/
	void RunTests();

private:
	/**
	This functions retrieves the PSL cookies from all the DMA channels
	and stores them in a single array.	It will use information from 
	the PslTestInfo.
	*/
	void GetPslCookie();

	/**
	This function will generate the DMA channel records.i.e channel cookies,Caps.
	*/
	void GenerateChannelRecord();

	/**
	Holds the PslTestInfo
	*/	
	TDmaV2TestInfo iPslTestInfo;

	/**
	A handle to RDmaSession
	*/
	RDmaSession iDmaSession;
	
	/**
	Array of DMA test cases 
	*/
	RPointerArray<TTestCase> iTestCases; 
	
	/**
	Array of DMA channel records,channel capabilities and other PSL information
	*/
	RArray<TChannelRecord> iChannelRecords; 	

	/**
	Array of DMA channel cookies
	*/
	RArray<TUint> iPslCookies;
};

/**
Copy an RArray
*/
template <typename T>
void CopyL(const RArray<T>& aOriginal, RArray<T>& aNew)
	{
	const TInt count = aOriginal.Count();
	for(TInt i=0; i<count; ++i)
		{
		aNew.AppendL(aOriginal[i]);
		}
	}

template <typename T, typename Iterator>
void ArrayAppendL(RArray<T>& aArray, Iterator aBegin, Iterator aEnd)
	{
	for(Iterator begin = aBegin; begin != aEnd; ++begin)
		aArray.AppendL(*begin);
	}


#endif // #ifndef __T_DMA2_H__
