// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dma\dma_api_tests.cpp
// 
// Overview:
//  This file contains API tests for the new DMA framework
//

#define __E32TEST_EXTENSION__
#include "d_dma2.h"
#include "u32std.h"
#include "t_dma2.h"
#include "cap_reqs.h"

#include <e32test.h>
#include <e32debug.h>
#include <e32svr.h>

static RTest test(_L("DMA Test Framework API"));
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2564
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    This test checks the correct behaviour of Open API in the new DMA framework
//!
//! @SYMTestActions     
//!						1.  Open a DMA channel
//!						2.	Verify that channel is really open.
//!
//! @SYMTestExpectedResults 
//!						1.  DMA channel opens and KErrNone returned
//!						2.  Call to ChannelIsOpened() return as ETrue.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void test_open_api()
{
	//TO DO : Expose TInt Open(const SCreateInfo& aInfo, TDmaChannel*& aChannel)
	//TO DO : Implement more test cases
	test.Start(_L("*** Testing Open() API  ***"));

	test.Next(_L("Open session"));
	RDmaSession session;
	TInt r = session.Open();
	test_KErrNone(r);

	TUint channelCookie_open_api=0;
	
	test.Next(_L("Open DMA Channel"));
	channelCookie_open_api=0;
	r = session.ChannelOpen(16, channelCookie_open_api);
	test.Printf(_L("cookie recieved = 0x%08x\n"), channelCookie_open_api);
	test_KErrNone(r);

	//Check if channel is open
	// test.Printf(_L("Verify that the specified DMA channel is opened\n"));	
	// TBool channelOpened;
	// TBool channelNotOpened = EFalse;
	// r = session.ChannelIsOpened(channelCookie_open_api,  channelOpened);
	// test_KErrNone(r);	
 	// TEST_ASSERT(channelOpened != channelNotOpened)
	
	//close channel
	test.Next(_L("Channel close"));
	r = session.ChannelClose(channelCookie_open_api);
	test_KErrNone(r);
	
	RTest::CloseHandleAndWaitForDestruction(session);
	test.End();
}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2568
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    This test checks the correct behaviour of Close API in the new DMA framework
//!
//! @SYMTestActions     
//!						1.  Open a DMA channel
//!						2.	Open DMA Channel again
//!						3	Close the DMA channel.
//!						4	Open DMA channel to verify that the DMA channel closed.
//!						5.	Open DMA channel again.
//!						6.	Queue a request on the channel.
//!						7.	Close DMA channel while request is still queued on it.
//!
//! @SYMTestExpectedResults 
//!						1.  DMA channel opens and KErrNone returned.
//!						2.	DMA Framework returns KErrInUse as channel is already open.					
//!						3.	DMA channel closes and KErrNone returned.
//!						4.	DMA channel opens and KErrNone returned.
//!						5.	DMA Framework returns KErrInUse as channel is already open.
//!						6.	DMA request queued and KErrNone returned.
//!						7.	DMA channel closes and DMA framework flags an error.
//!							
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void test_close_api()
{
	test.Start(_L("*** Testing Close() API  ***"));
	
	test.Next(_L("Open session"));
	RDmaSession session;
	TInt r = session.Open();
	test_KErrNone(r);

	const TInt size = 64 * KKilo;
	TUint reqCookieNewStyle_close_api=0;	
	TUint channelCookie_close_api=0;
	
	test.Next(_L("Open a single DMA channel"));
	r = session.ChannelOpen(16, channelCookie_close_api);
	test.Printf(_L("cookie recieved = 0x%08x\n"), channelCookie_close_api);
	test_KErrNone(r);

	// test.Next(_L("Open DMA channel again"));
	// TUint channelCookie_close_api_1=0;
	// r = session.ChannelOpen(16, channelCookie_close_api_1);
	// test.Printf(_L("Verify that DMA channel is already opened\n"));
	// test_Equal(KErrInUse,r); 

	test.Next(_L("Close the DMA channel"));
	r = session.ChannelClose(channelCookie_close_api);
	test_KErrNone(r);

	test.Next(_L("Open DMA channel again"));
	r = session.ChannelOpen(16, channelCookie_close_api);
	test.Printf(_L("Verify that DMA channel was closed\n"));
	test_KErrNone(r); 

	//Fails if a request is created and cancel
	test.Next(_L("Queue a request on the channel"));
  	r = session.RequestCreateNew(channelCookie_close_api, reqCookieNewStyle_close_api); //Create Dma request (with new-style callback)
  	test.Printf(_L("cookie recieved for open channel = 0x%08x\n"), reqCookieNewStyle_close_api);
  	test_KErrNone(r);
  
  	TDmaTransferArgs transferArgs_close_api;
  	transferArgs_close_api.iSrcConfig.iAddr = 0;
  	transferArgs_close_api.iDstConfig.iAddr = size;
  	transferArgs_close_api.iSrcConfig.iFlags = KDmaMemAddr;
  	transferArgs_close_api.iDstConfig.iFlags = KDmaMemAddr;
  	transferArgs_close_api.iTransferCount = size;
  	r = session.FragmentRequest(reqCookieNewStyle_close_api, transferArgs_close_api);
  	test_KErrNone(r);
  	
  	test.Next(_L("Queue DMA Request"));
  	TCallbackRecord record_close_api;
  	r = session.QueueRequest(reqCookieNewStyle_close_api, &record_close_api);
  	test_KErrNone(r);
	
	test.Next(_L("Destroy Dma request"));
	r = session.RequestDestroy(reqCookieNewStyle_close_api);
	test_KErrNone(r);

	test.Next(_L("Close the DMA channel"));
	r = session.ChannelClose(channelCookie_close_api);
	test_KErrNone(r);

	test.End();
	RTest::CloseHandleAndWaitForDestruction(session);
}

void RDmaSession::ApiTest()
	{
    test_open_api();     // Verify that Open() opens a DMA channel
    test_close_api();    // Verify that Close() closes a DMA channel
	}

void ApiTests()
	{
	test.Next(_L("Running framework API tests"));
	RDmaSession::ApiTest();	
	test.Close();
	}
