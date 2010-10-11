// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/rpmb/d_rpmb.cpp
// LDD for testing RPMB kernel extension
// 
//

/**
 @file
 @internalComponent
 @prototype
*/

#include "d_rpmb.h"
#include "../t_rpmb.h"

_LIT(KRpmbTestThreadName,"RpmbTestThread");
const TInt KRpmbTestPriority = 24;

TDfcQue RpmbTestDfcQ;
TDfc* DfcPtr;

// please read this before uncommenting out #define RPMBTESTS_TRY_TO_WRITE_KEY:
// Don't want to write key 'pre production' on real device as this will lock 
// the system out of the RPMB partition (the key should be preprogrammed as 
// part of the production process so that the system has access to the key - 
// if the key has already been written it can't be written again).
// #define RPMBTESTS_TRY_TO_WRITE_KEY


/****************************************
 Implementation of DRpmbTestFactory class
*****************************************/

DRpmbTestFactory::DRpmbTestFactory()
	{
	}

DRpmbTestFactory::~DRpmbTestFactory()
	{
	}

TInt DRpmbTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = new DRpmbTest;
	return KErrNone;
	}

TInt DRpmbTestFactory::Install()
	{
	return SetName(&KRpmbTestLddName);
	}

void DRpmbTestFactory::GetCaps(TDes8& /*aDes*/) const
// implementation required since DLogicalDevice::GetCaps is pure virtual
	{
	// do nothing 
	}


static void RpmbTestDfcFunction(TAny* /*aPtr*/)
	{
	DRpmbDevice * rpmb;
	rpmb = new DRpmbDevice;
	TInt r = rpmb->Open(0);
	Kern::Printf("%S Early on DRpmbDevice::Open test returned %d",&KDRpmbTestBanner, r);
	// ASSERT to indicate test failure if DRpmbDevice::Open() returned error
	__ASSERT_ALWAYS((r==KErrNone), Kern::Fault(__FILE__, __LINE__));
	rpmb->Close();
	delete rpmb;
	// run more extensive tests early on 
	// note the unconventional use of a class derived from DLogicalChannelBase which is justified
	// in the current context to provide more startup test coverage
	DRpmbTest * test;
	test = new DRpmbTest;
	// while a call to DRpmbTest::DRpmbDeviceTests() works in this context
	// a call to DRpmbTest::RpmbStackTests() would fail as DRpmbTest::DoCreate() wouldn't be
	// called so that DRpmbTest::iStackSemPtr wouldn't get set up properly so that when it 
	// was uaed an exception would be generated
	r = test->DRpmbDeviceTests();
	Kern::Printf("%S Early on DRpmbTest::DRpmbDeviceTests() returned %d",&KDRpmbTestBanner, r);
	// ASSERT to indicate test failure
	__ASSERT_ALWAYS((r==KErrNone), Kern::Fault(__FILE__, __LINE__));
	// not safe to delete test as  not setup conventionally DRpmbTest::DoCreate() wasn't called 
	// at setup and DLogicalChannelBase::~DLogicalChannelBase__sub_object() would generate an 
	// exception
	}


/********************************
 Entry point for RPMB test driver
*********************************/

DECLARE_STANDARD_EXTENSION()
    {     
	TInt r = Kern::DfcQInit(&RpmbTestDfcQ,KRpmbTestPriority,&KRpmbTestThreadName);;      
	if (r != KErrNone)
		return r;
	DfcPtr = new TDfc(RpmbTestDfcFunction,NULL, &RpmbTestDfcQ, KMaxDfcPriority-7);
	DfcPtr->Enque();     
	return KErrNone;
    }

DECLARE_EXTENSION_LDD()
	{
	return new DRpmbTestFactory;
	}

/*********************************
 Implementation of DRpmbTest class
**********************************/

DRpmbTest::DRpmbTest():
// initialise parameters
	iStackSemPtr(NULL),
	iPowerSemPtr(NULL),
	iSessionEndCallBack(DRpmbTest::StackCallBack, this)
	{
	}

DRpmbTest::~DRpmbTest()
	{
	
	iBusCallBack.Remove();
		
	if (iPowerSemPtr)
		{
		iPowerSemPtr->Close(NULL);
		}
	if (iStackSemPtr)
		{
		iStackSemPtr->Close(NULL);
		}
	if (iSession)
		{
		delete iSession;
		}
	}

TInt DRpmbTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	TInt r = KErrNone;
	r = Kern::SemaphoreCreate(iStackSemPtr, _L("DRpmbStackSem"), 0);
	return r;
	}

TInt DRpmbTest::Request(TInt aFunction, TAny* /*a1*/, TAny* /*a2*/)
	{
	TInt r = KErrNotSupported;
	switch (aFunction)
		{
		case RTestRpmb::ERunTests:
			{
			// respond to user side request 
			// execute tests for RPMB kernel extension
			// run standatd tests that use the DRpmbDevice interface
            Kern::Printf("	");
            Kern::Printf("%S << START DRpmbDevice TESTS >>", &KDRpmbTestBanner);
			r = DRpmbDeviceTests();
            if (r != KErrNone)
                {
                Kern::Printf("%S DPrmbDevice test FAILED", &KDRpmbTestBanner);
                break;
                }
            Kern::Printf("%S >> ALL DRpmbDevice TESTS PASSED", &KDRpmbTestBanner);
			// run tests that use the MMC stack bypassing the RPMB kernel extension
            Kern::Printf("	");
	        Kern::Printf("%S << START STACK TESTS >>", &KDRpmbTestBanner);
			r = RpmbStackTests();
			if (r != KErrNone)
			    {
                Kern::Printf("%S stack test FAILED", &KDRpmbTestBanner);
                break;
			    }
            Kern::Printf("%S >> ALL STACK TESTS PASSED", &KDRpmbTestBanner);
			break;
			}
		default:
			break;
		}
	return r;
	}


// test RPMB using the DRpmbDevice interface
TInt DRpmbTest::DRpmbDeviceTests()
// open interface and make data write, ket write, counter read and data write accesses
	{				
	TInt size = (TInt)(2*KRpmbOneFramePacketLength);

	NKern::ThreadEnterCS();
	iRequest = (TUint8 *)Kern::Alloc(size);
	NKern::ThreadLeaveCS();

	iResponse = iRequest + KRpmbOneFramePacketLength;

	TInt r = SendAccessRequestNotOpen();
	
	if (r==KErrNone)
		{
		r = OpenWithBadIndex();
		}

	if (r==KErrNone)
		{
		r = MultipleOpen();
		}
	
	if (r==KErrNone)
		{
		r = iRpmb.Open(0);
		}
	
	if (r!=KErrNone)
		{
		Kern::Printf("%S DRpmbTest::DRpmbDeviceTest DRpmbDevice open error = %d", 
		        &KDRpmbTestBanner, r);
		}
	else
		{
		r  = SendAccessRequestBadParms();
		}

	if (r==KErrNone)
		{
		r = InvalidRequestId();
		}   
	
	if (r==KErrNone)
		{
		r = IsKeyProgrammed();
		}   
	
// Don't want to write key 'pre production' on real device as this will lock 
// the system out of the RPMB partition (the key should be preprogrammed as 
// part of the production process so that the system has access to the key - 
// if the key has already been written it can't be written again)
#ifdef RPMBTESTS_TRY_TO_WRITE_KEY
	if (r==KErrNone)
		{
		r = WriteKey();
		}
#endif

	if (r==KErrNone)
		{  
		r = ReadWriteCounter();
		}

	if (r==KErrNone)
		{
		r = ReadData();
		}

	iRpmb.Close();
	
	NKern::ThreadEnterCS();	

	Kern::Free(iRequest);
	
	NKern::ThreadLeaveCS();
	
	return r;
}

TInt DRpmbTest::SendAccessRequestNotOpen()
// test cases where DRpmbDevice::SendAccessRequest() is called 
// 1.) before DRpmbDevice::Open() is ever called
// 2,) after DRpmbDevice::Close() has been called but before DRpmbDevice::Open() is called
	{	
	Kern::Printf("%S DRpmbDevice::SendAccessRequest not open test", &KDRpmbTestBanner);     
	// wrap up IO buffers in descriptors to pass across interface

	TPtr8 request(iRequest,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);
	TPtr8 response(iResponse,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);


	// test call DRpmbDevice::SendAccessRequest before EVER calling open

	TInt r = iRpmb.SendAccessRequest(request, response);
	if (r != KErrNotReady)
		{
        Kern::Printf("%S SendAccessRequest not open test, before EVER opening, unexpected error = %d", 
                &KDRpmbTestBanner, r);     
		return KErrGeneral;
		}

	// test open and close and call again
	r = iRpmb.Open(0);
	if (r != KErrNone)
		{
        Kern::Printf("%S SendAccessRequest not open test, opening, unexpected error = %d", 
                &KDRpmbTestBanner, r);     
		return KErrGeneral;
		}
	iRpmb.Close();

	r = iRpmb.SendAccessRequest(request, response);
	if (r != KErrNotReady)
		{
        Kern::Printf("%S SendAccessRequest not open test, before opening AFTER closing, unexpected error = %d", 
                &KDRpmbTestBanner, r);     
		return KErrGeneral;
		}
	
	Kern::Printf("%S DRpmbDevice::SendAccessRequest not open test PASS", &KDRpmbTestBanner);     
	return KErrNone;
}


TInt DRpmbTest::OpenWithBadIndex()
// test case where DRpmbDevice::Open() is called with a device index of more than zero
	{
	Kern::Printf("%S DRpmbDevice::Open bad index test", &KDRpmbTestBanner);     
	
	// test index > 0
	TInt r = iRpmb.Open(1);
	if (r != KErrGeneral)
		{
        Kern::Printf("%S RpmbDevice::Open bad index test, unexpected error = %d", 
                &KDRpmbTestBanner, r);     
		return KErrGeneral;
		}
	
	Kern::Printf("%S DRpmbDevice::Open bad index test PASS", &KDRpmbTestBanner);     
	return KErrNone;
}


TInt DRpmbTest::MultipleOpen()
// test case where DRpmbDevice::Open() is called twice with the same index parameter and
// the same instance of DRpmbDevice
// test case where DRpmbDevice::Open() is called twice with the same index parameter and
// a different instance of DRpmbDevice
	{
	Kern::Printf("%S DRpmbDevice::Open use more than once test", &KDRpmbTestBanner);     
	
	TInt r = iRpmb.Open(0);
	if (r != KErrNone)
		{
        Kern::Printf("%S RpmbDevice::Open use more than once test first open, unexpected error = %d", 
                &KDRpmbTestBanner, r);     
		return KErrGeneral;
		}

	r = iRpmb.Open(0);
	if (r != KErrNone)
		{
        Kern::Printf("%S RpmbDevice::Open use more than once test second open, unexpected error = %d", 
                &KDRpmbTestBanner, r);     
		return KErrGeneral;	
		}

	r = iRpmbSecondInstance.Open(0);
	if (r != KErrInUse)
		{
        Kern::Printf("%S RpmbDevice::Open use more than once test third open, unexpected error = %d", 
                &KDRpmbTestBanner, r);     
		return KErrGeneral;	
		}

	iRpmb.Close();
	iRpmbSecondInstance.Close();

	Kern::Printf("%S DRpmbDevice::Open use more than once test PASS", &KDRpmbTestBanner);     

	return KErrNone;
	}


TInt DRpmbTest::SendAccessRequestBadParms()
// test cases where DRpmbDevice::SendAccessRequest is called with secriptor arguments with invalid lengths
	{
	Kern::Printf("%S DRpmbDevice::SendAccessRequest bad parms test", &KDRpmbTestBanner);     
	
	// test both lengths !=	KRpmbOneFramePacketLength
	TBuf<KRpmbOneFramePacketLength-1> request;
	TBuf<KRpmbOneFramePacketLength+1> response;
	request.SetLength(KRpmbOneFramePacketLength-1);
	response.SetLength(KRpmbOneFramePacketLength+1);

	TInt r = iRpmb.SendAccessRequest(request, response);
	if (r != KErrArgument)
		{
        Kern::Printf("%S SendAccessRequest bad parms test, both params bad, unexpected error = %d", 
                &KDRpmbTestBanner, r);     
		return KErrGeneral;
		}

	// test request length != KRpmbOneFramePacketLength
	TBuf<KRpmbOneFramePacketLength+1> request1;
	TBuf<KRpmbOneFramePacketLength> response1;
	request1.SetLength(KRpmbOneFramePacketLength+1);
	response1.SetLength(KRpmbOneFramePacketLength);

	r = iRpmb.SendAccessRequest(request1, response1);
	if (r != KErrArgument)
		{
        Kern::Printf("%S SendAccessRequest bad parms test, request param bad, unexpected error = %d", 
                &KDRpmbTestBanner, r);     
		return KErrGeneral;
		}

	// test response length !=KRpmbOneFramePacketLength
	TBuf<KRpmbOneFramePacketLength> request2;
	TBuf<KRpmbOneFramePacketLength-1> response2;
	request2.SetLength(KRpmbOneFramePacketLength);
	response2.SetLength(KRpmbOneFramePacketLength-1);

	r = iRpmb.SendAccessRequest(request1, response1);
	if (r != KErrArgument)
		{
        Kern::Printf("%S SendAccessRequest bad parms test, response param bad, unexpected error = %d", 
                &KDRpmbTestBanner, r);     
		return KErrGeneral;
		}
	
	
	Kern::Printf("%S DRpmbDevice::SendAccessRequest bad parms test PASS", &KDRpmbTestBanner);     
	return KErrNone;
}

TInt DRpmbTest::InvalidRequestId()
// send a request with an invalid request ID to the RPMB partition
// when SendAccessRequest returns iResponse should not have reponse or result fields filled in
	{
    Kern::Printf("%S invalid request ID test", &KDRpmbTestBanner);     
    
	// set up write request packet with blank MAC
	memset(iRequest, 0, KRpmbOneFramePacketLength);
	* (iRequest + KRpmbRequestLsbOffset) = KRpmbRequestReadResultRegister + 1;
   
	// wrap up IO buffers in descriptors to pass across interface
	TPtr8 request(iRequest,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);
	TPtr8 response(iResponse,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);

	TInt r = iRpmb.SendAccessRequest(request, response);
	if (r != KErrNone)
		{
        Kern::Printf("%S invalid request ID test, send request error = %d", 
                &KDRpmbTestBanner, r);     
		return r;
		}

	// check that response or result fields have not been set in response
	TUint resp = DecodeResponse(iResponse);
	TUint result = DecodeResult(iResponse);

	if (resp != 0x0006)
		{
        Kern::Printf("%S invalid request ID test, unexpected response = %d error", 
                &KDRpmbTestBanner, resp);
		return KErrGeneral;
		}

	if (result != 0x0000)
		{
		Kern::Printf("%S invalid request ID test, unexpected result = %d error", 
                &KDRpmbTestBanner, result);
		return KErrGeneral;
		}

	Kern::Printf("%S invalid requeset ID test PASS", &KDRpmbTestBanner);     
	return KErrNone;
	}


TInt DRpmbTest::IsKeyProgrammed()
// send a data write request to the RPMB partition
// because the request has a blank MAC (not authenticated) 
// the partition returns operation result authentication failure if the key is programmed
// else the partition returns authentication key not programmed
	{
    Kern::Printf("%S data write test", &KDRpmbTestBanner);     
    
	// set up write request packet with blank MAC
	memset(iRequest, 0, KRpmbOneFramePacketLength);
	* (iRequest + KRpmbRequestLsbOffset) = KRpmbRequestWriteData;
   
	// wrap up IO buffers in descriptors to pass across interface
	TPtr8 request(iRequest,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);
	TPtr8 response(iResponse,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);

	TInt r = iRpmb.SendAccessRequest(request, response);
	if (r != KErrNone)
		{
        Kern::Printf("%S data write test, send request error = %d", 
                &KDRpmbTestBanner, r);     
		return r;
		}
		
	TUint resp = DecodeResponse(iResponse);
	TUint result = DecodeResult(iResponse);

	if (resp != KRpmbResponseWriteData)
		{
        Kern::Printf("%S data write test, unexpected response = %d error", 
                &KDRpmbTestBanner, resp);
		return KErrGeneral;
		}

	if (result == KRpmbResultKeyNotProgrammed)
		{
		iKeySet = EFalse;
	    Kern::Printf("%S data write test, key NOT programmed", 
	            &KDRpmbTestBanner);     
		}
	else if (result == KRpmbResultAuthenticationFailure)
		{
		iKeySet = ETrue;
	    Kern::Printf("%S data write test, key ALREADY programmed", 
	            &KDRpmbTestBanner);     
		}
	else
		{
        Kern::Printf("%S data write test, unexpected result = %d error", 
                &KDRpmbTestBanner, result);
		return KErrGeneral;
		}
	Kern::Printf("%S data write test PASS", &KDRpmbTestBanner);     
	return KErrNone;
	}

TInt DRpmbTest::WriteKey()
	{
    Kern::Printf("%S write key test", &KDRpmbTestBanner);     

    // set up write key request
	memset(iRequest, 0, KRpmbOneFramePacketLength);
	* (iRequest + KRpmbRequestLsbOffset) = KRpmbRequestWriteKey;
    // key = 0x0101010101010101010101010101010101010101010101010101010101010101
	memset(iRequest+KRpmbKeyOffset,1, KRpmbKeyLength); 

	// wrap up IO buffers in descriptors to pass across interface
	TPtr8 request(iRequest,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);
	TPtr8 response(iResponse,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);

	TInt r = iRpmb.SendAccessRequest(request, response);
	if (r != KErrNone)
		{
        Kern::Printf("%S write key test, send request error = %d", 
                &KDRpmbTestBanner, r);     
		return r;
		}

	TUint resp = DecodeResponse(iResponse);
	TUint result = DecodeResult(iResponse);

	if (resp != KRpmbResponseWriteKey)
		{
        Kern::Printf("%S write key test, unexpected repsponse = %d error", 
             &KDRpmbTestBanner, resp);
		return KErrGeneral;
		}

	if (iKeySet)
		{
		if (result == KRpmbResultGeneralFailure)
			{
            Kern::Printf("%S write key test, key ALREADY written", 
                    &KDRpmbTestBanner);     
            Kern::Printf("%S write key test PASS", 
                    &KDRpmbTestBanner);     
			return KErrNone;
			}
		else
			{
            Kern::Printf("%S write key test, unexpected result = %d error", 
                    &KDRpmbTestBanner, result);
			return KErrGeneral; 
			}
		}
	else
		{
		if(result == KRpmbResultOk)
			{
            Kern::Printf("%S write key test, key JUST written", 
		            &KDRpmbTestBanner);     
            Kern::Printf("%S write key test PASS", &KDRpmbTestBanner);     
			return KErrNone;
			}
		else
			{
	       Kern::Printf("%S write key test unexpected result = %d error", 
	                &KDRpmbTestBanner, result);
			return KErrGeneral;
			}
		}
	}

TInt DRpmbTest::ReadWriteCounter()
	{
    Kern::Printf("%S read write counter test", &KDRpmbTestBanner);     

	// set up read write counter request packet
	memset(iRequest, 0, KRpmbOneFramePacketLength);
	* (iRequest + KRpmbRequestLsbOffset) = KRpmbRequestReadWriteCounter;
   
	// wrap up IO buffers in descriptors to pass across interface
	TPtr8 request(iRequest,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);
	TPtr8 response(iResponse,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);

	TInt r = iRpmb.SendAccessRequest(request, response);
	if (r!=KErrNone)
		{
        Kern::Printf("%S read write counter test, send request error = %d", 
                &KDRpmbTestBanner, r);     
		return r;
		}

	TUint resp = DecodeResponse(iResponse);
	TUint result = DecodeResult(iResponse);
	TUint32 counter = DecodeCounter(iResponse);

	if (resp != KRpmbResponseReadWriteCounter)
		{
        Kern::Printf("%S read write counter test, unexpected repsponse = %d error", 
                &KDRpmbTestBanner, resp);
		return KErrGeneral;
		}

#ifdef  RPMBTESTS_TRY_TO_WRITE_KEY
	if (result == KRpmbResultOk)
#else
	if (result == KRpmbResultOk || result == KRpmbResultKeyNotProgrammed)
#endif
		{
		Kern::Printf("%S read write counter test write counter = %d", 
			&KDRpmbTestBanner, counter);     
        Kern::Printf("%S read write counter test PASS", &KDRpmbTestBanner);     
		return KErrNone;
		}
	else
		{
        Kern::Printf("%S read write counter test, unexpected result = %d error", 
                &KDRpmbTestBanner, result);
		return KErrGeneral;
		}	
	}

TInt DRpmbTest::ReadData()
	{
    Kern::Printf("%S read data test", &KDRpmbTestBanner);     
    
	// set up read request packet
	memset(iRequest, 0, KRpmbOneFramePacketLength);
	* (iRequest + KRpmbRequestLsbOffset) = KRpmbRequestReadData;
   
	// wrap up IO buffers in descriptors to pass across interface
	TPtr8 request(iRequest,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);
	TPtr8 response(iResponse,KRpmbOneFramePacketLength,KRpmbOneFramePacketLength);

	TInt r=iRpmb.SendAccessRequest(request, response);
	if (r!=KErrNone)
		{
        Kern::Printf("%S read data test, send request error = %d", 
                &KDRpmbTestBanner, r);     
		return r;
		}

	TUint resp = DecodeResponse(iResponse);
	TUint result = DecodeResult(iResponse);

	if (resp != KRpmbResponseReadData)
		{
        Kern::Printf("%S read data test, unexpected repsponse = %d error", 
                &KDRpmbTestBanner, resp);
		return KErrGeneral;
		}

#ifdef  RPMBTESTS_TRY_TO_WRITE_KEY
	if (result == KRpmbResultOk)
#else
	if (result == KRpmbResultOk || result == KRpmbResultKeyNotProgrammed)
#endif
		{
 		DisplayReadData(iResponse);
        Kern::Printf("%S read data test PASS", &KDRpmbTestBanner);     
		return KErrNone;
		}
	else
		{
        Kern::Printf("%S read data test, unexpected result = %d error", 
             &KDRpmbTestBanner, result);
        return KErrGeneral;
		}		
	}

TUint DRpmbTest::DecodeResponse(TUint8 * aResp)
	{
	return (* (aResp + KRpmbResponseLsbOffset) + ((* (aResp + KRpmbResponseMsbOffset)) << 8));
	}

TUint DRpmbTest::DecodeResult(TUint8 * aResp)
	{
	return ((* (aResp + KRpmbResultLsbOffset) + ((* (aResp + KRpmbResultMsbOffset)) << 8)) 
	        & KRpmbResultCounterExpiredMask);
	}

TUint32 DRpmbTest::DecodeCounter(TUint8 * aResp)
	{

	return ((* (aResp + KRpmbCounterByteOneOffset)) + ((* (aResp + KRpmbCounterByteTwoOffset)) << 8)
		+ ((* (aResp + KRpmbCounterByteThreeOffset)) << 16) + ((* (aResp + KRpmbCounterByteFourOffset)) << 24));
	}

void DRpmbTest::DisplayReadData(TUint8 * aResp)
	{
	TUint8 * displayPtr = aResp + KRpmbDataOffset;
    Kern::Printf("%S data field:", &KDRpmbTestBanner);
	for (TInt i=0; i<8; i++)
		{
		Kern::Printf("%x%x%x%x %x%x%x%x %x%x%x%x %x%x%x%x %x%x%x%x %x%x%x%x %x%x%x%x %x%x%x%x",
			* displayPtr       , * (displayPtr +  1), * (displayPtr +  2), * (displayPtr +  3),
			* (displayPtr +  4), * (displayPtr +  5), * (displayPtr +  6), * (displayPtr +  7),
			* (displayPtr +  8), * (displayPtr +  9), * (displayPtr + 10), * (displayPtr + 11),
			* (displayPtr + 12), * (displayPtr + 13), * (displayPtr + 14), * (displayPtr + 15),
			* (displayPtr + 16), * (displayPtr + 17), * (displayPtr + 18), * (displayPtr + 19),
			* (displayPtr + 20), * (displayPtr + 21), * (displayPtr + 22), * (displayPtr + 23),
			* (displayPtr + 24), * (displayPtr + 25), * (displayPtr + 26), * (displayPtr + 27),
			* (displayPtr + 28), * (displayPtr + 29), * (displayPtr + 30), * (displayPtr + 31));
		displayPtr +=32;
		}
	}

void DRpmbTest::StackCallBack(TAny * aSelf)
// call back from MMC stack session 
    {
	// dereference any pointer
	DRpmbTest& self = *static_cast<DRpmbTest*>(aSelf);
	// signal semaphore so that calling code progresses
	Kern::SemaphoreSignal(*(self.iStackSemPtr));
	}

TInt DRpmbTest::RpmbStackTests()
// Test code verfies all four access types through a direct connection to the MMC protocol stack 
	{
	TInt r = SetupForStackTests();
	if (r == KErrNone)
		{
		r = StackBadIndex();
		}
	if (r == KErrNone)
		{
		r = StackIsKeyProgrammed();
		}
// Don't want to write key 'pre production' on a real device as this will lock 
// the system out of the RPMB partition (the key should be preprogrammed as 
// part of the production process so that the system has access to the key - 
// if the key has already been written it can't be written again)
#ifdef RPMBTESTS_TRY_TO_WRITE_KEY
	if (r == KErrNone)
		{
		r = StackWriteKey();
		}
#endif
	if (r == KErrNone)
		{
		r = StackReadWriteCounter();
		}
	if (r == KErrNone)
		{
		r = StackReadData();
		}
	return r;
	}


TInt DRpmbTest::StackBadIndex()
// Call MMCGetExtInterface with invalid device id
//   - don't expect params to be returned
//   - expect KErrGeneral return code
	{
	Kern::Printf("%S stack device index test", &KDRpmbTestBanner);     

	MRpmbInfo* rpmbInterface = NULL;
	TRpmbDeviceParms parms;

	TInt r = MMCGetExtInterface(KInterfaceRpmb, (MMCMExtInterface*&) rpmbInterface, this); //this pointer not used in the case of Rpmb
    if (r!=KErrNone)
        {
        Kern::Printf("%S stack device index, MMCGetInterface returned error = %d",
             &KDRpmbTestBanner, r);
        return KErrGeneral;
        }
	if (rpmbInterface == NULL)
		{
		// unexpected error since MMCGetExtInterface didn't return an error
        Kern::Printf("%S stack device index, MMCGetInterface returned NULL interface pointer",
             &KDRpmbTestBanner, r);
		return KErrGeneral;
		}

    else
        {
		r = rpmbInterface->RpmbInfo(1, parms);
		if (r == KErrNone)
			{
			Kern::Printf("%S stack device index test, parmas returned for index 1 error",
				&KDRpmbTestBanner, r);
			return KErrGeneral;
			}
		else if (r != KErrGeneral)
			{
			Kern::Printf("%S stack device index test, unexpected rc for index 1 error",
				&KDRpmbTestBanner, r);
			return KErrGeneral;
			}
		}
    Kern::Printf("%S stack device index test PASS", &KDRpmbTestBanner);     
	return KErrNone;
}


void DRpmbTest::BusCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2)
{
    DRpmbTest* lddPtr = (DRpmbTest*)aPtr;
    TPBusState busState = (TPBusState) (TInt) a1;
	TInt busError = (TInt) a2;

    if(aReason == TPBusCallBack::EPBusStateChange 
		&& busState == EPBusOn && busError == KErrNone)
		{
		Kern::SemaphoreSignal(*(lddPtr->iPowerSemPtr));
        }
}

TInt DRpmbTest::SetupForStackTests()
	{
	Kern::Printf("%S setup for stack tests", &KDRpmbTestBanner);     

	TRpmbDeviceParms parms;
	
	parms.iCardNumber = 0;
	parms.iSocketPtr = NULL;
	
	DMMCStack* stack = NULL;
	TUint cardNumber = 0;
	MRpmbInfo* rpmbInterface = NULL;

    TInt r = MMCGetExtInterface(KInterfaceRpmb, (MMCMExtInterface*&) rpmbInterface, this); //this pointer not used in the case of Rpmb
    if (r!=KErrNone)
        {
        Kern::Printf("%S setup for stack tests, MMCGetInterface returned error = %d",
             &KDRpmbTestBanner, r);
        return KErrGeneral;
        }
	if (rpmbInterface == NULL)
		{
		// unexpected error since MMCGetExtInterface didn't return an error
		return KErrGeneral;
		}

    else
        {
		r = rpmbInterface->RpmbInfo(0, parms);
		if (r != KErrNone)
			{
			// requested index non zero or bseport not configured with RPMB capable MMC device
			return r;
			}
        
        cardNumber = parms.iCardNumber;        
        iSocket = parms.iSocketPtr;
        if(iSocket == NULL)
            {
            Kern::Printf("%S setup for stack tests, socket pointer is NULL",
                 &KDRpmbTestBanner);
              return KErrNoMemory;
            }
        
		// set up to be informed of changes on bus
		iBusCallBack.iFunction = BusCallBack;
		iBusCallBack.iPtr=this;
		iBusCallBack.SetSocket(iSocket->iSocketNumber);
		iBusCallBack.Add();

        // power up the stack
        // media drivers don't have to do this at this stage because already done
 		NKern::ThreadEnterCS();	

		r = Kern::SemaphoreCreate(iPowerSemPtr,_L("DpmbPowerSem"), 0);

		if (r!=KErrNone)
			{		
			NKern::ThreadLeaveCS();
            Kern::Printf("%S sese you below thentup for stack tests, SemaphoreCreate returned %d",
                    &KDRpmbTestBanner, r);
            return KErrGeneral;
			}
		
		r = iSocket->PowerUp();
				
        if (r!=KErrNone && r!= KErrCompletion)
            {
			NKern::ThreadLeaveCS();
            Kern::Printf("%S setup for stack tests, PowerUp returned %d",
                    &KDRpmbTestBanner, r);
            return KErrGeneral;
            }
		
        if (r == KErrNone)
			{
            // wait for socket to power up
			Kern::SemaphoreWait(*iPowerSemPtr);
			}

		NKern::ThreadLeaveCS();
  
        stack = iSocket->Stack(0);
        if (stack == NULL)
            {
            // baseport poorly configured
            Kern::Printf("%S setup for stack tests, stack pointer is NULL",
                &KDRpmbTestBanner);
            return KErrGeneral;
            }
        }

    TMMCard* card = stack->CardP(cardNumber);

	if (card == NULL)
		{
		// baseport poorly configured
        Kern::Printf("%S setup for stack tests, card pointer is NULL",
            &KDRpmbTestBanner);
		return KErrGeneral;
		}

	NKern::ThreadEnterCS();	

 	iSession = stack->AllocSession(iSessionEndCallBack);
	
	NKern::ThreadLeaveCS();
	
	if (iSession == NULL)
		{
		Kern::Printf("%S setup for stack tests, session pointer is NULL",
		        &KDRpmbTestBanner);
		return(KErrNoMemory);
		}
 
	iSession->SetStack(stack);

	iSession->SetCard(card);
		
	TUint32 revision = 0;

	// loop until extended csd becomes available in case earlier pause wasn't long enough
	// this shouldn't need to be done
	// a reliable method not depending on delays and or polling is required
	do {
		stack = iSocket->Stack(0);
		if (stack)
		{
			card = stack->CardP(cardNumber);	
			if (card)
			{
				revision = card->ExtendedCSD().ExtendedCSDRev();
			}
		}
	} while (revision==0);
		
	TUint size = card->ExtendedCSD().RpmbSize();
		
	if(revision < 5 || size == 0)
		{
		Kern::Printf("%S setup for stack test, rpmb partition NOT detected", &KDRpmbTestBanner); 
		return KErrNotSupported;
		}
	
	// use memory from stack
    TInt bufLen, minorBufLen;
    stack->BufferInfo(iBufPtr, bufLen, minorBufLen);
	// mmc media driver reserved the first KRpmbOneFramePacketLength bytes of the 
	// PSL buffer to be used for RPMB requests / responses
	iBufPtr += minorBufLen;

	return KErrNone;
	}


TInt DRpmbTest::StackIsKeyProgrammed()
// Write Data to block 0 without MACing 
//   - expect not MACed error if key set 
//   - expect key not prgrammed error if key not programmed
    {    

	Kern::Printf("%S write data stack test", &KDRpmbTestBanner);     
        
    // set up write request packet in stack memory
    memset(iBufPtr,0,KRpmbOneFramePacketLength);
    * (iBufPtr + KRpmbRequestLsbOffset) = KRpmbRequestWriteData;
        
    TInt r = SendToStackAndWait();
	if (r != KErrNone)
		{
		return r;
		}

	TUint response = DecodeResponse(iBufPtr); 
	TUint result = DecodeResult(iBufPtr);

	if (response == KRpmbResponseWriteData)
		{
		if (result == KRpmbResultAuthenticationFailure)
			{
            Kern::Printf("%S write data stack test, key IS written", 
                    &KDRpmbTestBanner);     
            Kern::Printf("%S write data stack test PASS", &KDRpmbTestBanner);     
			iKeySet = ETrue;
			return KErrNone;
			}
		else if (result == KRpmbResultKeyNotProgrammed)
			{
            Kern::Printf("%S write data stack test, key NOT written", 
                    &KDRpmbTestBanner);     
            Kern::Printf("%S write data stack test PASS",&KDRpmbTestBanner);
            iKeySet = EFalse;
            return KErrNone;
			}
		}
    Kern::Printf("%S write data stack test FAILED, response = %d, result = %d", 
            &KDRpmbTestBanner, response, result);     
    return KErrGeneral;
	}

TInt DRpmbTest::StackWriteKey()
// Write key
//   - expect no error if key not programmed
//   - expect general error if key already programmed
    {    
    Kern::Printf("%S write key stack test", &KDRpmbTestBanner);     

    // set up write key request packet in stack memory
    memset(iBufPtr,0,KRpmbOneFramePacketLength);
    * (iBufPtr + KRpmbRequestLsbOffset) = KRpmbRequestWriteKey;
        
    TInt r = SendToStackAndWait();
	if (r != KErrNone)
		{
		return r;
		}

	TUint response = DecodeResponse(iBufPtr); 
	TUint result = DecodeResult(iBufPtr);

	if (response == KRpmbResponseWriteKey)
		{
		if (iKeySet && result == KRpmbResultGeneralFailure)
			{
            Kern::Printf("%S write key stack test, key ALREADY written", 
                    &KDRpmbTestBanner);     
            Kern::Printf("%S write key stack test PASS", &KDRpmbTestBanner);     
			return KErrNone;
			}
		else if (!iKeySet && result == KRpmbResultOk)
			{
            Kern::Printf("%S write key stack test, key JUST written", 
                    &KDRpmbTestBanner);     
            Kern::Printf("%S write key stack test PASS", &KDRpmbTestBanner);     
			return KErrNone;
			}
		}
    Kern::Printf("%S write key stack test FAILED, response = %d, result = %d", 
            &KDRpmbTestBanner, response, result);     
    return KErrGeneral;	
	}

TInt DRpmbTest::StackReadWriteCounter()
// Read Data Counter
// - expect no error
    {    
    Kern::Printf("%S read write counter stack test", &KDRpmbTestBanner);     

    // set up write key request packet in stack memory
    memset(iBufPtr,0,KRpmbOneFramePacketLength);
    * (iBufPtr + KRpmbRequestLsbOffset) = KRpmbRequestReadWriteCounter;
        
    TInt r = SendToStackAndWait();
	if (r != KErrNone)
		{
		return r;
		}
	
	TUint response = DecodeResponse(iBufPtr); 
	TUint result = DecodeResult(iBufPtr);
	TUint32 counter = DecodeCounter(iBufPtr);

#ifdef  RPMBTESTS_TRY_TO_WRITE_KEY
	if ((response == KRpmbResponseReadWriteCounter) && (result == KRpmbResultOk))
#else
	if ((response == KRpmbResponseReadWriteCounter) 
		&& (result == KRpmbResultOk || result == KRpmbResultKeyNotProgrammed))
#endif
		{
        Kern::Printf("%S read write counter stack test, counter = %d", 
			&KDRpmbTestBanner, counter);     
        Kern::Printf("%S read write counter stack test PASS", 
                &KDRpmbTestBanner);     
		return KErrNone;
		}
    Kern::Printf("%S read write counrter stack test FAILED, response = %d, result = %d", 
            &KDRpmbTestBanner, response, result);     
    return KErrGeneral;	
	}


TInt DRpmbTest::StackReadData()
// Read Data from block 0
// - expect no error
    {    
    Kern::Printf("%S read data stack test", &KDRpmbTestBanner);     

    // set up write key request packet in stack memory
    memset(iBufPtr,0,KRpmbOneFramePacketLength);
    * (iBufPtr + KRpmbRequestLsbOffset) = KRpmbRequestReadData;
        
    TInt r = SendToStackAndWait();
	if (r != KErrNone)
		{
		return r;
		}

	TUint response = DecodeResponse(iBufPtr); 
	TUint result = DecodeResult(iBufPtr);

#ifdef  RPMBTESTS_TRY_TO_WRITE_KEY
	if ((response == KRpmbResponseReadData) && (result == KRpmbResultOk))
#else
	if ((response == KRpmbResponseReadData) 
		&& (result == KRpmbResultOk || result == KRpmbResultKeyNotProgrammed))
#endif
		{
		DisplayReadData(iBufPtr);
        Kern::Printf("%S read data stack test PASS", &KDRpmbTestBanner);     
		return KErrNone;
		}
	Kern::Printf("%S read data stack test FAILED, response = %d, result = %d", 
            &KDRpmbTestBanner, response, result);     
    return KErrGeneral;
	}

TInt DRpmbTest::SendToStackAndWait()
	{
// comment out to get timings with RPMB -> RPMB switches in place of User 
// Area -> RPMB switces
#define RPMBSTACKTEST_FIRST_SWITCH_TO_USER_AREA
#ifdef RPMBSTACKTEST_FIRST_SWITCH_TO_USER_AREA	
	// in normal operation it may be necessary to switch to RPMB partition
	// prior to access
	// so force the switch to occur by pre switching to the user area
    iSession->SetPartition(TExtendedCSD::ESelectUserArea);
#endif

	TInt start, postswitch, end;

	start = NKern::TickCount();

	// lock stack
	TInt r = iSocket->InCritical();
	if (r != KErrNone)
		{
		Kern::Printf("%S DRpmbTest::SendToStackAndWait, error=%d",
		        &KDRpmbTestBanner, r);
		iSocket->EndInCritical();
		return KErrGeneral;
		}

	 // switch to RPMB partition
    iSession->SetPartition(TExtendedCSD::ESelectRPMB);

	postswitch = NKern::TickCount();
	
    // set up for write exchange 
    iSession->ResetCommandStack();
    iSession->FillCommandArgs(0, KRpmbOneFramePacketLength, iBufPtr, KRpmbOneFramePacketLength);
    iSession->iSessionID = ECIMRpmbAccess;
    iSession->Engage();
    
    // wait for stack call to complete
	r = Kern::SemaphoreWait(*iStackSemPtr);

	end = NKern::TickCount();

	Kern::Printf("%S switch to RPMB took %d %d uS timer ticks",
		&KDRpmbTestBanner, start - postswitch, NKern::TickPeriod());
	Kern::Printf("%S RPMB ECIMRpmbAccess took %d %d uS timer ticks",
		&KDRpmbTestBanner, end - postswitch, NKern::TickPeriod());

	if (r != KErrNone)
		{
		Kern::Printf("%S DRpmbTest::SendToStackAndWait, SemaphoreWait return code = %d",
		        &KDRpmbTestBanner, r);
		iSocket->EndInCritical();
		return KErrGeneral;
		}

	// check stack epoc return code
	r = iSession->EpocErrorCode();
	if (r != KErrNone)
		{		
		Kern::Printf("%S DRpmbTest::stack epoc return code = %d",
		        &KDRpmbTestBanner, r);
		iSocket->EndInCritical();
		return KErrGeneral;
		}
		
	// unlock stack
	iSocket->EndInCritical();

	return KErrNone;
	}

