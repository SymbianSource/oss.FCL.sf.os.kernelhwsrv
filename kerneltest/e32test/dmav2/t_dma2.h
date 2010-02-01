/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

void ApiTests();

class CSingleTransferTest;
class CIsrRequeTest;
class CMultiTransferTest;


/**
An interface to a classs that sets up the buffers before a test
*/
//TODO both pre and post transfer checks should perhaps derive from an
//abstract visitor base
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
Base class for all DMA tests
*/
class CDmaTest : public CTest
	{
public:
	CDmaTest(const TDesC& aName, TInt aIterations, const MPreTransfer* aPreTransfer, const MPostTransferCheck* aPostTransfer)
		: CTest(aName, aIterations), iPreTransfer(aPreTransfer), iPostTransferCheck(aPostTransfer)
		{}

	void OpenDmaSession();
	void CloseDmaSession();

	virtual void PrintTestInfo() const;
	virtual TBool Result() = 0;

	const RChunk& Chunk() const
		{return iChunk;}

	/**
	Tells the test which DMA channel it should run on
	*/
	void SetChannelCookie(TUint32 aCookie)
		{iChannelCookie = aCookie;}

	virtual void PreTransferSetup() =0;
	virtual TInt DoPostTransferCheck() =0;
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
		:iCreate(aCreate), iFragmentCount(aFragmentCount), iFragmentationResult(aFragmentationResult), iQueueResult(aQueueResult)
		{}

	/**
	Constructs with error results
	*/
	TRequestResults(TFalse)
		:iCreate(KErrUnknown), iFragmentCount(0), iFragmentationResult(KErrUnknown), iQueueResult(KErrUnknown)
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
protected:
	virtual void Setup(const TAddressParms& aParams) const;
	TBool CheckBuffers(const CIsrRequeTest& aTest) const;
	TBool CheckBuffers(const RArray<const TAddressParms> aTransferParams) const;
	};

const TPreTransferIncrBytes KPreTransferIncrBytes;
const TCompareSrcDst KCompareSrcDst;
const TCompare2D KCompare2D;


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

	TInt iBytes; //!< The number of bytes traversed
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
This class will be used for tests which benchmark certain DMA operations
*/
class CDmaBenchmark : public CSingleTransferTest
	{
public:
	CDmaBenchmark(const TDesC& aName, TInt aIterations, const TResultSet& aExpectedResults, const TDmaTransferArgs& aTransferArgs, TUint aMaxFragmentSize);
	~CDmaBenchmark();

	virtual TBool Result();

	static void SelfTest();

protected:
	/**
	@return The mean average of the result array
	*/
	TUint64 MeanResult();

	//TODO must be included within copy ctor or all instances will
	//share on result set!
	RArray<TUint64> iResultArray;

	};

/**
Fragments requests (only) and records duration
TODO make sure we are using old style DDmaRequest
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

	/**
	Compares the actual vs the exepected results and reports
	of the test passed
	@return ETrue for a pass, EFalse for a fail
	 */
	//virtual TBool Result();


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
	//TODO it might be better to group sets of TDmaCapability
	//into their own class eg. TDmaCapSet.
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
	This function will populate TTestRunner with an array of test cases which 
	would be a collection of DMA test,its hardware prerequisites,and other 
	information about how the test	

	@aTTestCases on return, this contains an the DMA test cases 
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


#endif // #ifndef __T_DMA2_H__
