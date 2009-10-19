#ifndef __BASE_TEST_CASE_H
#define __BASE_TEST_CASE_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* @file BaseTestCase.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <d32usbdi.h>
#include "FDFActor.h"
#include "testdebug.h"

namespace NUnitTesting_USBDI
	{

#ifdef _DEBUG
#define _LITDBG(a)	_LIT(lit, a)
#else
#define _LITDBG(a)
#endif
	
// Constants for CBaseTestCase::GetEndpointAddress

extern const TUint8 KEpDirectionIn;
extern const TUint8 KEpDirectionOut;
extern const TUint8 KTransferTypeControl;
extern const TUint8 KTransferTypeIsoch;
extern const TUint8 KTransferTypeBulk;
extern const TUint8 KTransferTypeInterrupt;
	
// Forward declarations

class CBasicTestPolicy;

#define WS_FILENAME(f)	_LIT(KFileName, f);
#define WS_CONDITION(c)	_LIT(KCondition, c);
_LIT(KFailText, "CHECK fails ---> %S, in file %S @ line %i");

// Utility macro to tidy up the test
#define CHECK(aCondition)\
	if(!(aCondition))\
		{\
		WS_FILENAME(__FILE__);\
		WS_CONDITION(#aCondition);\
		CBaseTestCase::LogWithCondAndInfo(KCondition, KFileName, __LINE__);\
		return TestFailed(KErrAbort);\
		}\
		
// Utility macro to tidy up the test
#define CHECK_RET_BOOL(aCondition)\
	if(!(aCondition))\
		{\
		WS_FILENAME(__FILE__);\
		WS_CONDITION(#aCondition);\
		CBaseTestCase::LogWithCondAndInfo(KCondition, KFileName, __LINE__);\
		return EFalse;\
		}\
/**
Base policy class for test cases.  Test cases are active objects that execute their respective test cases
and asynchronously complete when test case has finished
*/
class CBaseTestCase : public CActive
	{
public:
	/**
	Destructor
	*/
	virtual ~CBaseTestCase();
	
	/**
	*/
	void SetTestPolicy(CBasicTestPolicy* aTestPolicy);
	
	/**
	Performs the test case
	*/
	void PerformTestL();
	
	/**
	Provides to the caller the identity of this test case
	@return descriptor referrence to the test case identity
	*/
	TDesC& TestCaseId();
	
	/**
	Retrieve the test result for the executed test case
	@return the test case result
	*/
	TInt TestResult() const;
		
	/**
	Check if the test is only be run on the host(no synch. with client needed)
	@return True if test to be run in host only.
	*/
	TBool IsHostOnly() const;
	
	/**
	Check if the test is only be run on the host(no synch. with client needed)
	@return True if test to be run in host only.
	*/
	TBool IsHost() const;
	
	/** 
	static method(util), used to log a test condition.
	@param[in] aCondition checked condition	
	@param[in] aFileName __FILE__	
	@param[in] aLine __LINE__ 	
	*/	 
	static void LogWithCondAndInfo(const TDesC& aCondition, const TDesC& aFileName, TInt aLine);
	
protected: // From CActive 
	
	/**
	Called when Cancel is called (if active)
	*/
	void DoCancel();
	
	/**
	Called when the this AO has been scheduled
	*/
	void RunL();
	
	/**
	This default implementation just informs the test controller that this specific test case RunL
	left with the error code so that the active scheduler will not Panic.  This indicates that the test case failed.
	Derived test cases are expected to override for exact knowledge of error
	@param aError the error from a derived test case execution
	@return KErrNone
	*/
	virtual TInt RunError(TInt aError);
	
protected: // Tree checks

	/**
	Store dev. and config. desc into internal buffer and check if it is identical to the reference one. 
	@param[in] aDevDesc parsed device descriptor
	@param[in] aConfigDesc parsed config. descriptor
	@param[in] aFileName file to be compared with the current tree(doesn't contain the path nor extension)
	or to build a reference tree(after being manually checked once, it could be used as a ref.)
	@return KErrNone if tree is identical to the ref. tree
	*/
	TInt CheckTree(TUsbGenericDescriptor& aDevDesc, TUsbGenericDescriptor& aConfigDesc, const TDesC& aFileName);
	 
	/**
	Parse configuration descriptor raw data and check if the generated tree is identical to the ref. one.
	FAILS the test if there is any difference.
	@param[in] devDesc already parsed device descriptor
	@param[in] configSet configuration descriptor raw data to be parsed
	@param[in] indexTest index of the test(used to append a number to the generated file name)
	@return KErrNone if parsing is ok and tree is identical to the ref. tree
	*/
	TInt ParseConfigDescriptorAndCheckTree(TUsbDeviceDescriptor *devDesc, const TDesC8& configSet, TUint indexTest);
    	/**
	Parse configuration descriptor raw data and check if the generated tree is identical to the ref. one.
	FAILS the test if there is any difference.
	@param[in] aTestDevice device being tested tested
	@param[in] aFileName file to be compared with the current tree(doesn't contain the path nor extension)
	@return KErrNone if tree is identical to the ref. tree after a real device has been inserted.
	*/
    TInt CheckTreeAfterDeviceInsertion(CUsbTestDevice& aTestDevice, const TDesC& aFileName);

protected:
	/**
	Constructor
	@param aTestCaseId the identity that this test case is to represent
	@param aHostFlag the flag for host role
	*/
	CBaseTestCase(const TDesC& aTestCaseId,TBool aHostFlag, TBool aHostOnly = EFalse);
	
	/**
	Base class 2nd phase constructor
	*/
	void BaseConstructL();	
	virtual void ConstructL() = 0;
	
	/**
	State that the test case has failed with the given error code
	@param aFailResult the failiure result for the test case
	*/
	void TestFailed(TInt aFailResult);
	
	/**
	Instruct the test case that it has passed
	*/
	void TestPassed();
	
	/**
	Instruct the test case to asynchronously complete itself now
	*/
	void SelfComplete();
	
	/**
	Instruct the test case to asynchronously complete itself now with the supplied error code.
	@param aError the error code to complete this test case with.
	*/
	void SelfComplete(TInt aError);

	/**
	Timeout in the specified number of seconds
	@param aTimeoutPeriod the timeout interval 
	*/
	void TimeoutIn(TInt aTimeoutPeriod);
	
	/**
	Cancels the timeout for the test case step
	*/
	void CancelTimeout();
	
	/**
	*/	
	CBasicTestPolicy& TestPolicy();
	
	/**
	*/
	virtual void ExecuteHostTestCaseL() = 0;
	virtual void HostRunL() = 0;
	virtual void HostDoCancel() = 0;
	
	/**
	*/
	virtual void ExecuteDeviceTestCaseL() = 0;
	virtual void DeviceRunL() = 0;
	virtual void DeviceDoCancel() = 0;

protected:

/**
Finds the address of the first endpoint with the specification of direction and type
@param aUsbInterface the interface that has the correct interface setting
@param aInterfaceSetting the alternate interface setting which has the endpoint
@param aTransferType the type of transfer for this endpoint
@param aDirection the direction of the endpoint in the host transfer
@param[out] on return, the first endpoint address found
@return KErrNone if successful or a system wide error code
*/
TInt GetEndpointAddress(RUsbInterface& aUsbInterface,TInt aInterfaceSetting,
		TUint8 aTransferType,TUint8 aDirection,TInt& aEndpointAddress);

/**
Finds the address of the (index+1)th endpoint with the specification of direction and type
@param aUsbInterface the interface that has the correct interface setting
@param aInterfaceSetting the alternate interface setting which has the endpoint
@param aTransferType the type of transfer for this endpoint
@param aDirection the direction of the endpoint in the host transfer
@param[out] on return, the first endpoint address found
@return KErrNone if successful or a system wide error code
*/
TInt GetEndpointAddress(RUsbInterface& aUsbInterface,TInt aInterfaceSetting,
		TUint8 aTransferType,TUint8 aDirection,TUint8 aIndex, TInt& aEndpointAddress);

protected:

	/**
	The timer resource for timeout of test actions
	and possibly other uses
	*/
	RTimer iTimer;	
	
	/**
	The execution result for the test case
	*/
	TInt iTestResult;
	
private:  // Tree 

	/**
	Print the tree(logging) and store it into iTreeBuffer.
	@param[in] aDesc usb descriptor(likely to be device descriptor or configuration one)
	@param[in] aDepth tree depth
	*/
    void PrintAndStoreTree(TUsbGenericDescriptor& aDesc, TInt aDepth = 0);
    
    /**
	Print a blob(logging) and store it into iTreeBuffer.
	@param[in] aBuf usb descriptor(likely to be device descriptor or configuration one)
	@param[in] aBlob tree depth
	*/	
	void PrintAndStoreBlob(TDes8& aBuf, TPtrC8& aBlob);
	
	/** 
	Print a chunk(logging) and store it into iTreeBuffer.
	@param[in] aChunk chunk buffer
	@param[in] aSize nb of data(for writtings in one go)
	@param[in] aBlob whole Blob
	@param[in] aOffset offset of blob(for writtings in several times)
	@param[in] aIter index of chunk(for writtings in several times)
	@param[in] aBuf buffer needed for proper indentation
	*/
    void PrintAndStoreChunk(HBufC8* aChunk, TUint aSize, TPtrC8& aBlob, TUint aOffset, TUint aIter, TDes8& aBuf);
   	
   	/** 
	Generate a Ref. file representing the tree(flushing iTreeBuffer)
	@param[in] aFileName file name(doesn't contain the path nor extension)
	@return KErrNone if successfull.
	*/
	TInt GenerateRefFile(const TDesC& aFileName);
	
	/** 
	Compare the current tree(iTreeBuffer) to a reference.
	@param[in] aFileName ref. file name(doesn't contain the path nor extension)
	@param[out] aIsIdentical True if equal to the ref.
	@return KErrNone if successfull.
	*/
	TInt CompareCurrentTreeToRef(const TDesC& aFileName, TBool& aIsIdentical);
	
private :

	/**
	The identity of the test case
	*/
	TBuf<KMaxName> iTestCaseId;
	
	/**
	*/
	CBasicTestPolicy* iTestPolicy;
	
	/**
	The flag to indicate if this is a test case that performs host test actions
	*/
	TBool iHost;
	
	/** 
	The flag to indicate if this is a test case that is only run in host, and doesn't need synch. with a device.
	*/
	TBool iHostOnly;
	
	/** 
	Heap buffer which contains the descriptors tree.
	*/
	RBuf8 iTreeBuffer;
	
	/**
	Hanlde to a file session server
	*/
	RFs iFs;
	};

	}
	
#endif
