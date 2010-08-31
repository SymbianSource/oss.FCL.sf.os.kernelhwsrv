// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/iic/t_iic.cpp
//

// This file interacts with test-specific LDD to instigate tests of functionality
// that would normally be invoked by kernel-side device driver clients of the IIC.
#include <e32test.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <e32def.h>
#include <e32def_private.h>
#include "t_iic.h"

//for memory leak checking
#include <e32svr.h>
#include <u32hal.h>

_LIT(testName,"t_iic");

_LIT(KIicProxyFileNameCtrlLess, "iic_client_ctrless.ldd");		// Kernel-side proxy LDD acting as a client of the IIC
_LIT(KIicProxyFileNameRootCtrlLess, "iic_client_ctrless");
_LIT(KIicProxySlaveFileNameCtrlLess, "iic_slaveclient_ctrless.ldd");	// Kernel-side proxy LDD acting as a slave client of the IIC
_LIT(KIicProxySlaveFileNameRootCtrlLess, "iic_slaveclient_ctrless");
_LIT(KIicProxyFileName, "iic_client.ldd");		// Kernel-side proxy LDD acting as a client of the IIC
_LIT(KIicProxyFileNameRoot, "iic_client");
_LIT(KIicProxySlaveFileName, "iic_slaveclient.ldd");	// Kernel-side proxy LDD acting as a slave client of the IIC
_LIT(KIicProxySlaveFileNameRoot, "iic_slaveclient");
//These are used to exercise stub functions.
_LIT(KIicProxyFileNameStubs, "iic_client_stubs.ldd");
_LIT(KIicProxyFileNameRootStubs, "iic_client_stubs");
_LIT(KIicProxySlaveFileNameStubs, "iic_slaveclient_stubs.ldd");
_LIT(KIicProxySlaveFileNameRootStubs, "iic_slaveclient_stubs");


#ifdef IIC_SIMULATED_PSL
_LIT(KSpiFileNameCtrlLess, "spi_ctrless.pdd");	// Simulated PSL bus implementation
_LIT(KI2cFileNameCtrlLess, "i2c_ctrless.pdd");  // Simulated PSL bus implementation
_LIT(KIicPslFileName, "iic_testpsl.pdd");	// Simulated PSL implementation
_LIT(KSpiFileName, "spi.pdd");	// Simulated PSL bus implementation
_LIT(KI2cFileName, "i2c.pdd");  // Simulated PSL bus implementation
//These are used to exercise stubs. The I2C pdd to use for stub tests will depend on
//whether Master, Slave mode has been selected. 
#if defined(MASTER_MODE)&&!defined(SLAVE_MODE)
_LIT(KI2cFileNameStubs, "i2c_slavestubs_ctrless.pdd");
#elif !defined(MASTER_MODE)&& defined(SLAVE_MODE)
_LIT(KI2cFileNameStubs, "i2c_masterstubs_ctrless.pdd");
#else
_LIT(KI2cFileNameStubs, "i2c_ctrless.pdd");
#endif
#endif

_LIT(KIicPslFileNameRoot, "iic.pdd");

// Specify a stand-alone channel
GLDEF_D TBool aStandAloneChan;

GLDEF_D RTest gTest(testName);


// SPI has Master channel numbers 1,2 and 4, Slave channel number 3
GLDEF_D RBusDevIicClient gChanMasterSpi;
GLDEF_D RBusDevIicClient gChanSlaveSpi;

// I2C has Master channel numbers 10 and 11, if built with MASTER_MODE, only
// I2C has Slave channel numbers 12 and 13, if built with SLAVE_MODE, only
// I2C has Master channel number 10 and Slave channel number 11 if built with both MASTER_MODE and SLAVE_MODE
GLDEF_D RBusDevIicClient gChanMasterI2c;
GLDEF_D RBusDevIicClient gChanSlaveI2c;

LOCAL_C TInt CreateSingleUserSideTransfer(TUsideTferDesc*& aTfer, TInt8 aType, TInt8 aBufGran, TDes8* aBuf, TUsideTferDesc* aNext)
// Utility function to create a single transfer
	{
	aTfer = new TUsideTferDesc();
	if(aTfer==NULL)
		return KErrNoMemory;
	aTfer->iType=aType;
	aTfer->iBufGranularity=aBufGran;
	aTfer->iBuffer = aBuf;
	aTfer->iNext = aNext;
	return KErrNone;
	}

LOCAL_C TInt CreateSingleUserSideTransaction(TUsideTracnDesc*& aTracn, TBusType aType, TDes8* aHdr, TUsideTferDesc* aHalfDupTrans, TUsideTferDesc* aFullDupTrans, TUint8 aFlags, TAny* aPreambleArg, TAny* aMultiTranscArg)
// Utility function to create a single transaction
	{
	aTracn = new TUsideTracnDesc();
	if(aTracn==NULL)
		return KErrNoMemory;
	aTracn->iType=aType;
	aTracn->iHeader=aHdr;
	aTracn->iHalfDuplexTrans=aHalfDupTrans;
	aTracn->iFullDuplexTrans=aFullDupTrans;
	aTracn->iFlags=aFlags;
	aTracn->iPreambleArg = aPreambleArg;
	aTracn->iMultiTranscArg = aMultiTranscArg;
	return KErrNone;
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_IIC-2402
//! @SYMTestType        UT
//! @SYMPREQ            PREQ2128,2129
//! @SYMTestCaseDesc    This test case test the Master channel basic functionality
//! @SYMTestActions     0) Create a transaction and invoke the synchronous Queue Transaction API
//!
//!						1) Re-use the transaction and invoke asynchronous Queue Transaction API. Wait for
//|						   the TRequestStatus to be completed.
//!
//!						2) Instruct the Kernel-side proxy client to instigate testing of priority queuing.
//!						   The proxy uses controlIO to block the transaction queue, then queues 5 transactions in reverse
//!						   priority order. The proxy then uses controlIO to unblock the transaction queue and checks that
//!						   the transactions complete in priority order.
//!
//!						3) Attempt to cancel a previously-completed asynchronous request for a queued transaction
//!
//!						4) Use controlio to block request completion. Issue two asynchronous Queue Transaction requests.
//!						   Request cancellation of the second transaction. Wait for completion of the TRequestStatus for
//!						   the second request. Attempt to de-register the channel. Use controlio to unblock request completion.
//!						   Wait for completion of the TRequestStatus for the first request.
//!
//!						5) Attempt to de-register a channel that is not busy.
//!
//!						6) Attempt to queue a transaction on an invalid (de-registered) channel
//!
//!						7) Instruct the Kernel-side proxy client to instigate construction of a valid full duplex transaction.
//!
//!						8) Instruct the Kernel-side proxy client to instigate construction of a invalid full duplex transaction,
//!						   where both transfer in same direction
//!
//!						9) Instruct the Kernel-side proxy client to instigate construction of a invalid full duplex transaction,
//!						   where with different node length (not the number of node on opposite linklist ) at the same
//!						   position on the opposite transfer linklist
//!
//!						10) Instruct the Kernel-side proxy client to instigate construction of a valid full duplex transaction,
//!						   with different size for the last node
//!
//!						11) Instruct the Kernel-side proxy client to instigate construction of a valid full duplex transaction,
//!						   with different number of transfer
//!
//!
//! @SYMTestExpectedResults 0) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!						1) Kernel-side proxy client should return with KErrNone, exits otherwise. TRequestStatus should
//!						   be set to KErrNone, exits otherwise.
//!						2) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!						3) Kernel-side proxy client should return with KErrNone, exits otherwise.TRequestStatus should
//!						   be set to KErrNone, exits otherwise.
//!						4) The TRequestStatus for the cancelled request should be set to KErrCancel, exits otherwise.
//!						   The attempt to de-register the channel should return KErrInUse, exits otherwise. The
//!						   TRequestStatus for the first request should be set to KErrNone, exits otherwise.
//!						5) Kernel-side proxy client should return with KErrNone or KErrArgument, exits otherwise.
//!						6) Kernel-side proxy client should return with KErrArgument, exits otherwise.
//!						7) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!						8) Kernel-side proxy client should return with KErrNotSupported, exits otherwise.
//!						9) Kernel-side proxy client should return with KErrNotSupported, exits otherwise.
//!						10) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!						11) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt MasterBasicTests()
//
//	Exercise the Master Channel API with trivial data
//
	{
	gTest.Printf(_L("\n\nStarting MasterBasicTests\n"));

	TInt r=KErrNone;

	TUint32 busIdSpi = 0;

	// Use the SPI bus
	// SPI uses channel numbers 1,2,3 and 4
	SET_BUS_TYPE(busIdSpi,ESpi);
	SET_CHAN_NUM(busIdSpi,2);
	TConfigSpiBufV01* spiBuf = NULL;
	// aDeviceId=1 ... 100kHz ... aTimeoutPeriod=100 ... aTransactionWaitCycles=10 - arbitrary paarmeters.
	r = CreateSpiBuf(spiBuf, ESpiWordWidth_8, 100000, ESpiPolarityLowRisingEdge, 100 ,ELittleEndian, EMsbFirst, 10, ESpiCSPinActiveLow);
	gTest(r==KErrNone);

	// Use a single transfer
	_LIT(halfDuplexText,"Half Duplex Text");
	TBuf8<17> halfDuplexBuf_8;
	halfDuplexBuf_8.Copy(halfDuplexText);
	TUsideTferDesc* tfer = NULL;
	r = CreateSingleUserSideTransfer(tfer, EMasterWrite, 8, &halfDuplexBuf_8, NULL);
	gTest(r==KErrNone);

	// Create the transaction object
	TUsideTracnDesc* tracn = NULL;
	r = CreateSingleUserSideTransaction(tracn, ESpi, spiBuf, tfer, NULL, 0, NULL, NULL);
	gTest(r==KErrNone);

	// Test basic queueing operations
	// inline TInt QueueTransaction(TInt aBusId, TUsideTracnDesc* aTransaction)
	gTest.Printf(_L("\n\nStarting synchronous QueueTransaction \n"));
	r = gChanMasterSpi.QueueTransaction(busIdSpi, tracn);
	gTest.Printf(_L("Synchronous QueueTransaction returned = %d\n"),r);
	gTest(r==KErrNone); 
    // inline void QueueTransaction(TRequestStatus& aStatus, TInt aBusId, TUsideTracnDesc* aTransaction)
	gTest.Printf(_L("\n\nStarting asynchronous QueueTransaction \n"));
	TRequestStatus status;

	gChanMasterSpi.QueueTransaction(status, busIdSpi, tracn);
	User::WaitForRequest(status);
	if(status != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after queue = %d\n"), status.Int());
		gTest(EFalse);
		}

	// Test message with priorities
	gTest.Printf(_L("\n\nStarting test for message with priorities\n\n"),r);
	r = gChanMasterSpi.TestPriority(busIdSpi);
	gTest(r==KErrNone);

	// Test cancel operation (on previously completed request)

	// inline void CancelAsyncOperation(TRequestStatus* aStatus, TInt aBusId)	{TInt* parms[2]; parms[0]=(TInt*)aStatus; parms[1]=(TInt*)aBusId;DoCancel((TInt)&parms[0]);}
	gTest.Printf(_L("\n\nStarting CancelAsyncOperation \n"));
	gChanMasterSpi.CancelAsyncOperation(&status, busIdSpi);
	if(status == KRequestPending)
		User::WaitForRequest(status);
	if(status != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after (belated) cancel = %d\n"), status.Int());
		gTest(EFalse);
		}

	// Test cancel operation (on pending request)
	// Also test that a channel with a transaction queued can not be de-registered.
	// For this:
	// (1) create a second transaction object
	// (2) use controlio/StaticExtension to block request completion
	// (3) use asynchronous queue transaction for the two transaction objects
	// (4) request cancellation of the second request
	// (5) check that the TRequestStatus object associated with the second request is completed with KErrCancel
	// (6) check that attempt to de-register the channel fails with KErrInUse
	// (7) use controlio/StaticExtension to unblock request completion
	// (8) check that the TRequestStatus object associated with the first request is completed with KErrNone
	//
	gTest.Printf(_L("\n\nStarting (successful) cancellation test\n\n"),r);
	_LIT(halfDuplexText2,"2 Half Duplex Text 2");
	TBuf8<21> halfDuplexBuf2_8;
	halfDuplexBuf2_8.Copy(halfDuplexText2);
	TUsideTferDesc* tfer2 = NULL;
	r = CreateSingleUserSideTransfer(tfer2, EMasterRead, 16, &halfDuplexBuf2_8, NULL);
	gTest(r == KErrNone);

	TUsideTracnDesc* tracn2 = NULL;
	delete spiBuf;
	spiBuf = NULL;

	// aDeviceId=1 ... 100kHz ... aTimeoutPeriod=100 ... aTransactionWaitCycles=10 - arbitrary paarmeters.
	r = CreateSpiBuf(spiBuf, ESpiWordWidth_8, 100000, ESpiPolarityLowRisingEdge, 100 ,ELittleEndian, EMsbFirst, 10, ESpiCSPinActiveLow);
	gTest(r == KErrNone);

	r = CreateSingleUserSideTransaction(tracn2, ESpi, spiBuf, tfer2, NULL, 0, NULL, NULL);
	gTest(r == KErrNone);

	//
	gTest.Printf(_L("Invoking BlockReqCompletion\n"));
	r = gChanMasterSpi.BlockReqCompletion(busIdSpi);
	gTest.Printf(_L("BlockReqCompletion returned = %d\n"),r);
	gTest(r == KErrNone);

	//
	gTest.Printf(_L("Queueing first transaction \n"));
	gChanMasterSpi.QueueTransaction(status, busIdSpi, tracn);
	TRequestStatus status2;

	gTest.Printf(_L("Queueing second transaction \n"));
	gChanMasterSpi.QueueTransaction(status2, busIdSpi, tracn2);
	//
	User::After(50000);
	//
	gTest.Printf(_L("Issuing Cancel for second transaction\n"));
	gChanMasterSpi.CancelAsyncOperation(&status2, busIdSpi);
	gTest.Printf(_L("Returned from Cancel for second transaction\n"));
	if(status2 == KRequestPending)
		User::WaitForRequest(status2);
	if(status2 != KErrCancel)
		{
		gTest.Printf(_L("TRequestStatus (2) value after cancel = %d\n"), status2.Int());
		gTest(EFalse);
		}

	// If it is stand-alone channel, the client is reponsible for channel creation.
	// So the RegisterChan and DeRegisterChan are not needed.
	if (aStandAloneChan == 0)
		{
		gTest.Printf(_L("Invoking DeRegisterChan\n"));
		r = gChanMasterSpi.DeRegisterChan(busIdSpi);

		gTest.Printf(_L("DeRegisterChan returned = %d\n"),r);
		gTest(r==KErrInUse);
		}
	//
	gTest.Printf(_L("Invoking UnlockReqCompletion\n"));
	r = gChanMasterSpi.UnblockReqCompletion(busIdSpi);
	gTest.Printf(_L("UnblockReqCompletion returned = %d\n"),r);
	//
	User::After(50000);
	//
	User::WaitForRequest(status);
	if(status != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after queue = %d\n"), status.Int());
		gTest(EFalse);
		}

	// Clean up
	delete spiBuf;
	delete tfer;
	delete tracn;
	delete tfer2;
	delete tracn2;

	gTest.Printf(_L("\n\nStarting full duplex transaction creation test\n\n"),r);

	TUint32 busIdSpiFd = 0;

	// Use the SPI bus
	// SPI uses channel numbers 1,2,3 and 4
	SET_BUS_TYPE(busIdSpi,ESpi);
	SET_CHAN_NUM(busIdSpi,4);

	// Test creating a valid full duplex transaction
	gTest.Printf(_L("\n\nStarting valid full duplex transaction test\n\n"),r);
	r = gChanMasterSpi.TestValidFullDuplexTrans(busIdSpiFd);
	gTest(r==KErrNone);

	// Test creating a full duplex transaction with both transfer in same direction (invalid)
	gTest.Printf(_L("\n\nStarting invalid direction full duplex transaction test\n\n"),r);
	r = gChanMasterSpi.TestInvalidFullDuplexTrans1(busIdSpiFd);
	gTest.Printf(_L("Full duplex transaction with invalid direction returned = %d\n"),r);
	gTest(r==KErrNotSupported);

	// Test creating a full duplex transaction with different node length (not the number of node on opposite linklist )
	// at the same position on the opposite transfer linklist
	gTest.Printf(_L("\n\nStarting invalid transfer length full duplex transaction test\n\n"),r);
	r = gChanMasterSpi.TestInvalidFullDuplexTrans2(busIdSpiFd);
	gTest(r==KErrNotSupported);

	// Test creating a valid full duplex transaction with different size for the last node
	gTest.Printf(_L("\n\nStarting valid full duplex transaction test with diff size last node\n\n"),r);
	r = gChanMasterSpi.TestLastNodeFullDuplexTrans(busIdSpiFd);
	gTest(r==KErrNone);

	// Test creating a valid full duplex transaction with different number of transfer
	gTest.Printf(_L("\n\nStarting valid full duplex transaction test with diff number of transfer\n\n"),r);
	r = gChanMasterSpi.TestDiffNodeNumFullDuplexTrans(busIdSpiFd);
	gTest(r==KErrNone);

	return KErrNone;
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_IIC-2403
//! @SYMTestType        UT
//! @SYMPREQ            PREQ2128,2129
//! @SYMTestCaseDesc    This test case tests the Master channel data handling for transactions
//! @SYMTestActions     0) Instruct the kernel-side proxy to construct a transaction of pre-defined data
//!						   and inform the simulated bus to expect to receive this data. Then the proxy invokes
//!						   the synchronous Queue Transaction API. On receipt of the transaction, the simulated bus
//!						   checks the header and transafer content of the transaction to confirm that it is correct.
//!
//! @SYMTestExpectedResults 0) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt MasterTransactionTests()
//
//	Exercise the Master Channel API with trivial data
//
	{
	gTest.Printf(_L("\n\nStarting MasterTransactionTests\n"));

	TInt r = KErrNone;

	// Prove that the simulated bus can access the transfer data contained within a transaction
	// Do this by instructing the proxy client to:
	// (1) Inform the bus of the test about to be informed
	// (2) Send a transaction with a known number of transfers with known data
	// (3) Check the result announced by the bus.
	//
	// Use the SPI bus
	// SPI uses channel numbers 1,2,3 and 4
	TUint32 busIdSpi = 0;
	SET_BUS_TYPE(busIdSpi,ESpi);
	SET_CHAN_NUM(busIdSpi,4);	// Master, Full-duplex - required by TestBufferReUse
	r = gChanMasterSpi.TestTracnOne(busIdSpi);
	gTest.Printf(_L("TestTracnOne returned = %d\n"),r);
	gTest(r==KErrNone);

	// Test that transfer and transaction buffers can be modifed for re-use
	// This test modifies the content of a full-duplex transaction - so a full-duplex channel must be used
	TRequestStatus status;
	gChanMasterSpi.TestBufferReUse(busIdSpi, status);
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after CaptureChannel = %d\n"),r);
		gTest(r==KErrCompletion);
		}

	return KErrNone;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_IIC-2401
//! @SYMTestType        UT
//! @SYMPREQ            PREQ2128,2129
//! @SYMTestCaseDesc    This test case test the Master channel preamble and multi-transaction functionality.
//! @SYMTestActions     0) Create a transaction that requires preamble support, and queue it for processing
//!
//!						1) If the test has been invoked for preamble testing, wait for the preamble-specific
//!						   TRequestStatus to be completed.
//!
//!						2) If the test has been invoked for multi-transaction testing, wait for the multi-transaction
//!						   -specific TRequestStatus to be completed.
//!
//!
//! @SYMTestExpectedResults 0) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!						1) If waiting on the preamble-specific TRequestStatus, it should be set to KErrNone, exists otherwise.
//!						2) If waiting on the multi-transaction-specific TRequestStatus, it should be set to KErrNone, exists otherwise.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt MasterExtTests(TUint8 aFlags)
//
//	Exercise the Master Channel API for Preamble functionality
//

//  For the multi-transaction test, a bus Master might not know 
//  how much data to write to a Slave until it performs a single read on it. 
//  However, specifying a read separately from the subsequent write 
//  introduces the risk of allowing another transaction to go ahead of the 
//  following write and thus invalidating it. The multi-transaction feature of IIC
//  allows a callback to be called(in the context of the bus channel) after 
//  the transfers of a preliminary transaction have taken place 
//  (could be a single read), without completing the overall transaction,
//  then extend the delayed transaction by inserting more transfers
//
	{
	gTest.Printf(_L("\n\nStarting MasterExtTests\n"));

	TInt r = KErrNone;

	// Create a transaction that requires preamble support
	// To prove required operation has executed, make callback complete a TRequestStatus object
	TRequestStatus preamblestatus;
	TRequestStatus multitranscstatus;

	// Use the SPI bus
	// SPI uses channel numbers 1,2,3 and 4
	TUint32 busIdSpi = 0;
	SET_BUS_TYPE(busIdSpi, ESpi);
	SET_CHAN_NUM(busIdSpi, 1);
	TConfigSpiBufV01* spiBuf = NULL;
	// aDeviceId=1 ... 100kHz ... aTimeoutPeriod=100 ... aTransactionWaitCycles=10 - arbitrary paarmeters.
	r = CreateSpiBuf(spiBuf, ESpiWordWidth_8, 100000,
	        ESpiPolarityLowRisingEdge, 100, ELittleEndian, EMsbFirst, 10,
	        ESpiCSPinActiveLow);
	if (r != KErrNone)
		return r;

	// Use a single transfer
	_LIT(extText, "Ext Text");
	TBuf8<14> extBuf_8;
	extBuf_8.Copy(extText);
	TUsideTferDesc* tfer = NULL;
	r = CreateSingleUserSideTransfer(tfer, EMasterRead, 8, &extBuf_8, NULL);
	if (r != KErrNone)
		{
		delete spiBuf;
		return r;
		}

	// Create the transaction object
	TUsideTracnDesc* tracn = NULL;
	r = CreateSingleUserSideTransaction(tracn, ESpi, spiBuf, tfer, NULL,
	        aFlags, (TAny*) &preamblestatus, (TAny*) &multitranscstatus);

	if (r != KErrNone)
		{
		delete spiBuf;
		delete tfer;
		return r;
		}

	// Send the transaction to the kernel-side proxy
	// inline TInt QueueTransaction(TInt aBusId, TUsideTracnDesc* aTransaction)
	gTest.Printf(_L("\nInvoke synchronous QueueTransaction for preamble test %x\n"), tracn);

	r = gChanMasterSpi.QueueTransaction(busIdSpi, tracn);
	gTest.Printf(_L("synchronous QueueTransaction returned = %d\n"), r);

	if (r == KErrNone)
		{
		// ... and wait for the TRequestStatus object to be completed
		if (aFlags & KTransactionWithPreamble)
			{
			User::WaitForRequest(preamblestatus);
			r = preamblestatus.Int();
			if (r != KErrNone)
				{
				gTest.Printf(_L("MasterPreambleTests: TRequestStatus completed with = %d\n"), r);
				}
			}


		if (aFlags & KTransactionWithMultiTransc)
			{
			User::WaitForRequest(multitranscstatus);
			if (r != KErrNone)
				{
				gTest.Printf(_L("MasterMultiTranscTests: TRequestStatus completed with = %d\n"), r);
				}
			}
		}

	delete spiBuf;
	delete tfer;
	delete tracn;

	return r;
	}

#ifdef SLAVE_MODE
LOCAL_C TInt CreateSlaveChanI2cConfig(TConfigI2cBufV01*& aI2cBuf, TUint32& aBusIdI2c, TUint8 aChanNum)
	{
	// Initialise TConfigI2cBufV01 and the Bus Realisation Config for gChanSlaveI2c.
	// Customised:
	// - token containing the bus realisation variability.
	// - pointer to a descriptor containing the device specific configuration option applicable to all transactions.
	// - reference to variable to hold a platform-specific cookie that uniquely identifies the channel instance to be
	//   used by this client
	aBusIdI2c = 0;
	SET_BUS_TYPE(aBusIdI2c,EI2c);
	SET_CHAN_NUM(aBusIdI2c,aChanNum);
	//
	// clock speed=36Hz, aTimeoutPeriod=100 - arbitrary parameter
	TInt r=CreateI2cBuf(aI2cBuf, EI2cAddr7Bit, 36, ELittleEndian, 100);
	return r;
	}

LOCAL_C TInt SyncCaptureGChanSlaveI2c(TInt& aChanId, TConfigI2cBufV01* aI2cBuf, TUint32 aBusIdI2c)
	{
	// Synchronous capture of a Slave channel. Need to provide:
	// - token containing the bus realisation variability.
	// - pointer to a descriptor containing the device specific configuration option applicable to all transactions.
	// - reference to variable to hold a platform-specific cookie that uniquely identifies the channel instance to be used by this client
	gTest.Printf(_L("\n\nStarting synchronous CaptureChannel \n"));
	TInt r = gChanSlaveI2c.CaptureChannel(aBusIdI2c, aI2cBuf, aChanId );
	gTest.Printf(_L("Synchronous CaptureChannel returned = %d, aChanId=0x%x\n"),r,aChanId);
	return r;
	}


LOCAL_C TInt AsyncCaptureGChanSlaveI2c(TInt& aChanId, TConfigI2cBufV01* aI2cBuf, TUint32 aBusIdI2c)
	{
	// Asynchronous capture of a Slave channel. Need to provide:
	// - token containing the bus realisation variability.
	// - pointer to a descriptor containing the device specific configuration option applicable to all transactions.
	// - reference to variable to hold a platform-specific cookie that uniquely identifies the channel instance to be used by this client
	// - pointer to TRequestStatus used to indicate operation completion
	gTest.Printf(_L("\n\nStarting asynchronous CaptureChannel \n"));
	TRequestStatus status;
	TInt r = gChanSlaveI2c.CaptureChannel(aBusIdI2c, aI2cBuf, aChanId, status );
	gTest(r==KErrNone);
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrCompletion)
		{
		gTest.Printf(_L("TRequestStatus value after CaptureChannel = %d\n"),r);
		gTest(r==KErrCompletion);
		}
	gTest.Printf(_L("Asynchronous CaptureChannel gave aChanId=0x%x\n"),aChanId);
	return KErrNone;
	}
#endif
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_IIC-2399
//! @SYMTestType        UT
//! @SYMPREQ            PREQ2128,2129
//! @SYMTestCaseDesc    This test case tests Slave channel capture and release APIs.
//! @SYMTestActions     0) Perform synchronous capture of a channel
//!
//!						1) Release the channel
//!
//!						2) Perform asynchronous capture of a channel
//!
//!						3) Attempt synchronous capture of a channel that is already captured
//!
//!						4) Attempt asynchronous capture of a channel that is already captured
//!
//!						5) Release the channel
//!
//! @SYMTestExpectedResults 0) Kernel-side proxy client should return with KErrCompletion, exits otherwise.
//!						1) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!						2) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!						3) Kernel-side proxy client should return with KErrInUse, exits otherwise.
//!						4) Kernel-side proxy client should return with KErrNone, exits otherwise. The associated
//!						   TRequestStatus should be set to KErrInUse, exits otherwise.
//!						5) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt SlaveChannelCaptureReleaseTests()
//
//	Exercise the Slave Channel API for channel capture and release
//
	{
	gTest.Printf(_L("\n\nStarting SlaveChannelCaptureReleaseTests\n"));
	TInt r=KErrNone;
#ifdef SLAVE_MODE

	// Create a I2C configuration buffer and the configuration data for use in capturing gChanSlaveI2c
	TUint32 busIdI2c = 0;
	TConfigI2cBufV01* i2cBuf=NULL;
	r=CreateSlaveChanI2cConfig(i2cBuf, busIdI2c, 11);	// 11 is the Slave channel number
	gTest(r==KErrNone);

	// Synchronous capture of a Slave channel.
	TInt chanId = 0; // Initialise to zero to silence compiler ...
	r=SyncCaptureGChanSlaveI2c(chanId, i2cBuf, busIdI2c);
	gTest(r==KErrNone);
	//
	// Release the channel
	gTest.Printf(_L("\n\nInvoke ReleaseChannel for chanId=0x%x \n"),chanId);
	r = gChanSlaveI2c.ReleaseChannel( chanId );
	gTest.Printf(_L("ReleaseChannel returned = %d\n"),r);
	gTest(r==KErrNone);
	//
	// Asynchronous capture of a Slave channel.
	chanId = 0; // Re-initialise to zero to silence compiler ...
	r=AsyncCaptureGChanSlaveI2c(chanId, i2cBuf, busIdI2c);
	gTest(r==KErrNone);

	// Try capturing a slave channel that is already captured
	//
	// Create another instance of a client, and use to attempt duplicated capture
	TInt dumChanId = 0; // Initialise to zero to silence compiler ...
	RBusDevIicClient tempChanSlaveI2c;
	TBufC<24> proxySlaveName;
	if(aStandAloneChan == 0)
		proxySlaveName = KIicProxySlaveFileNameRoot;
	else
		proxySlaveName = KIicProxySlaveFileNameRootCtrlLess;
	r = tempChanSlaveI2c.Open(proxySlaveName);
	gTest(r==KErrNone);
	r = tempChanSlaveI2c.InitSlaveClient();
	gTest(r==KErrNone);
	//
	// Synchronous capture
	gTest.Printf(_L("\n\nStarting attempted synchronous CaptureChannel of previously-captured channel\n"));
	r = tempChanSlaveI2c.CaptureChannel(busIdI2c, i2cBuf, dumChanId );
	gTest.Printf(_L("Synchronous CaptureChannel returned = %d, dumChanId=0x%x\n"),r,dumChanId);
	gTest(r==KErrInUse);
	//
	// Asynchronous capture
	dumChanId = 0;
	gTest.Printf(_L("\n\nStarting attempted asynchronous CaptureChannel of previously-captured channel\n"));
	TRequestStatus status;
	r = tempChanSlaveI2c.CaptureChannel(busIdI2c, i2cBuf, dumChanId, status );
	gTest(r==KErrNone);
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrInUse)
		{
		gTest.Printf(_L("TRequestStatus value after attempted CaptureChannel of previously-captured channel = %d\n"),r);
		gTest(r==KErrInUse);
		}
	gTest.Printf(_L("Asynchronous CaptureChannel gave dumChanId=0x%x\n"),dumChanId);

	tempChanSlaveI2c.Close();
	//
	// Clean up, release the channel
	r = gChanSlaveI2c.ReleaseChannel( chanId );
	gTest.Printf(_L("ReleaseChannel returned = %d\n"),r);
	gTest(r==KErrNone);

	delete i2cBuf;
#else
	gTest.Printf(_L("\nSlaveChannelCaptureReleaseTests only supported when SLAVE_MODE is defined\n"));
#endif
	return r;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_IIC-2400
//! @SYMTestType        UT
//! @SYMPREQ            PREQ2128,2129
//! @SYMTestCaseDesc    This test case tests Slave channel capture operation for receive and transmit of data
//! @SYMTestActions     0) Check that the timeout threshold values can be updated
//!
//!						1) Check that an Rx Buffer can be registered, and that a replacement buffer can be registered in its place
//!						   if a notification has not been requested.
//!
//!						2) Specify a notification trigger for Rx events
//!
//!						3) Attempt to register a replacement Rx buffer
//!
//!						4) Use controlIO to instruct the simulated bus to indicate that it has received the required number of words
//!						   and wait for the TRequestStatus to be completed.
//!
//!						5) Specify a notification trigger for Rx events, use controlIO to instruct the simulated bus to indicate that
//!						   it has received less than the required number of words and wait for the TRequestStatus to be completed.
//!
//!						6) Specify a notification trigger for Rx events, use controlIO to instruct the simulated bus to indicate that
//!						   it has received more than the required number of words and wait for the TRequestStatus to be completed.
//!
//!						7) Repeat steps 1-6, but for Tx
//!
//!						8) Specify a notification trigger for Rx and Tx events. Use controlIO to instruct the simulated bus to indicate that
//!						   it has received the required number of words, then that it has transmitted the required number of words, and wait
//!						   for the TRequestStatus to be completed.
//!
//!						9) Repeat step 8, but simulate Tx, then Rx.
//!
//!						10) Specify a notification trigger for bus error events. Use controlIO to instruct the simulated bus to indicate that
//!						    it has encountered a bus error, and wait for the TRequestStatus to be completed.
//!
//!						11) Use controlIO to instruct the simulated bus to block Master response. Specify a notification trigger for bus error
//!						    events. Use controlIO to instruct the simulated bus to indicate that it has received more than the required number
//!						    of words. Wait for the TRequestStatus to be completed (with KErrNone). Specify a notification trigger for Tx and
//!							Tx Overrun, then use controlIO to instruct the simulated bus to unblock Master responses.Wait for the TRequestStatus
//!							to be completed.
//!         
//!                     12) Test the PIL behavior for a client timeout: request notification of an event but deliberately delay the client response.  
//!                         The PIL should return KErrTimedOut when a subsequent request for a notification is made.
//!
//! @SYMTestExpectedResults 0) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!						1) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!						2) Kernel-side proxy client should return with KErrNone, exits otherwise.
//!						3) Kernel-side proxy client should return with KErrAlreadyExists, exits otherwise.
//!						4) Kernel-side proxy client should return with KErrNone, exits otherwise. The associated
//!						   TRequestStatus should be set to KErrNone, exits otherwise.
//!						5) Kernel-side proxy client should return with KErrNone for both API calls, exits otherwise. The associated
//!						   TRequestStatus should be set to KErrNone, exits otherwise.
//!						6) Kernel-side proxy client should return with KErrNone for both API calls, exits otherwise. The associated
//!						   TRequestStatus should be set to KErrNone, exits otherwise.
//!						7) Results should be the same as for steps 1-6.
//!						8) Kernel-side proxy client should return with KErrNone for each API call, exits otherwise. The associated
//!						   TRequestStatus should be set to KErrNone, exits otherwise.
//!						9) Kernel-side proxy client should return with KErrNone for each API call, exits otherwise. The associated
//!						   TRequestStatus should be set to KErrNone, exits otherwise.
//!						10) Kernel-side proxy client should return with KErrNone for each API call, exits otherwise. The associated
//!						   TRequestStatus should be set to KErrNone, exits otherwise.
//!						11) Kernel-side proxy client should return with KErrNone for each API call, exits otherwise. The associated
//!						   TRequestStatus should be set to KErrNone in both cases, exits otherwise.
//!                     12) Kernel-side proxy client should return with KErrNone for each API call, exits otherwise. The associated
//!                        TRequestStatus should be set to KErrNone in both cases, exits otherwise, except for when the client response
//!                        exceeds the timeout period, and the next request for a notification expects KErrTimedOut.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

LOCAL_C TInt SlaveRxTxNotificationTests()
//
//	Exercise the Slave channel operation for receive and transmit of data
//

// The means to supply a buffer to be filled with data received from the Master, and the number of words expected.
// It is only after the reception of the number of words specified that the notification should be issued
// (or on under-run/overrun/timeout/bus specific error).
//
// The means to supply a buffer with data to be transmitted to the Master, and the number of words to transmit.
// It is only after the transmission of the number of words specified that the notification should be issued
// (or under-run/overrun/timeout/bus specific error).
//
// The means to enable and disable the events which will trigger the notification callback. These events are:
// 1)	the complete reception of the number of words specified,
// 2)	the complete transmission of the number of words specified,
// 3)	errors: receive buffer under-run (the Master terminates the transaction or reverts the direction of
//		transfer before all expected data has been received), receive buffer overrun
//		(Master attempts to write more data than this channel expected to receive), transmit buffer overrun
//		(Master attempts to read more data than supplied by client), transmit buffer under-run
//		(the Master terminates the transaction or reverts the direction of transfer before all expected data
//		has been transmitted to it), access timeout(1) error, or bus specific error (e.g. collision, framing).
  {
	gTest.Printf(_L("\n\nStarting SlaveRxTxNotificationTests\n"));
	TInt r=KErrNone;
#ifdef SLAVE_MODE

	//Configure and capture a channel
	gTest.Printf(_L("Create and capture channel\n"));
	TUint32 busIdI2c;
	TConfigI2cBufV01* i2cBuf=NULL;
	r=CreateSlaveChanI2cConfig(i2cBuf, busIdI2c, 11);	// 11 is the Slave channel number
	gTest(r==KErrNone);

	TInt chanId = 0; // Initialise to zero to silence compiler ...
	r=SyncCaptureGChanSlaveI2c(chanId, i2cBuf, busIdI2c);
	gTest(r==KErrNone);

	//		Update wait times for Master and Client
	// Delegate the operation of this test to the proxy client (iic_client). The proxy will read, modify, and reinstate
	// the timeout values.
	gTest.Printf(_L("Starting UpdateTimeoutValues\n"));
	r=gChanSlaveI2c.UpdateTimeoutValues(busIdI2c, chanId);
	gTest(r==KErrNone);


	// Receive and transmit buffers must be created by the client in Kernel heap and remain in their ownership throughout.
	// Therefore, the kernel-side proxy will provide the buffer
	// The buffers are of size KRxBufSizeInBytes and KRxBufSizeInBytes (currently 64)

	//
	//		Rx tests
	//

	// For Rx, specify buffer granularity=4 (32-bit words), 8 words to receive, offset of 16 bytes
	// 64 bytes as 16 words: words 0-3 offset, words 4-11 data, words 12-15 unused
	gTest.Printf(_L("Starting RegisterRxBuffer\n"));
	r=gChanSlaveI2c.RegisterRxBuffer(chanId, 4, 8, 16);
	gTest(r==KErrNone);
	//
	// If a buffer is already registered but a notification has not yet been requested the API should return KErrNone
	gTest.Printf(_L("Starting (repeated) RegisterRxBuffer\n"));
	r=gChanSlaveI2c.RegisterRxBuffer(chanId, 4, 8, 16);
	gTest(r==KErrNone);
	//
	// Now set the notification trigger
	TRequestStatus status;
	TInt triggerMask=ERxAllBytes;
	gTest.Printf(_L("Starting SetNotificationTrigger with ERxAllBytes\n"));
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	//
	// If a buffer is registered and a notification has been requested the API should return KErrAlreadyExists
	gTest.Printf(_L("Starting RegisterRxBuffer (to be rejected)\n"));
	r=gChanSlaveI2c.RegisterRxBuffer(chanId, 4, 8, 16);
	gTest(r==KErrAlreadyExists);
	//
	// Now instruct the bus implementation to represent receipt of the required number of words from the bus master.
	gTest.Printf(_L("Starting SimulateRxNWords\n"));
	r=gChanSlaveI2c.SimulateRxNWords(busIdI2c, chanId, 8);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after receiving data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Starting Rx test completed OK\n"));
	//
	// Repeat for each error condition. Re-use the buffer previously registered.
	//
	//
	triggerMask=ERxAllBytes|ERxUnderrun;
	gTest.Printf(_L("Starting SetNotificationTrigger with ERxAllBytes\n"));
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	// Now instruct the bus implementation to represent the bus master transmitting less words than anticipated (Rx Underrun)
	gTest.Printf(_L("Starting SimulateRxNWords for Underrun\n"));
	r=gChanSlaveI2c.SimulateRxNWords(busIdI2c, chanId, 6);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after receiving data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Rx Underrun test completed OK\n"));
	// Re-set the notification trigger
	triggerMask=ERxAllBytes|ERxOverrun;
	gTest.Printf(_L("Starting SetNotificationTrigger\n"));
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	// Now instruct the bus implementation to represent the bus master attempting to transmit more words than
	// anticipated (Rx Overrun)
	gTest.Printf(_L("Starting SimulateRxNWords for Overrun\n"));
	r=gChanSlaveI2c.SimulateRxNWords(busIdI2c, chanId, 10);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after receiving data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Rx Overrun test completed OK\n"));

	//
	//		Tx tests
	//

	// For Tx, specify buffer granularity=4 (32-bit words), 12 words to transmit, offset of 8 bytes
	// 64 bytes as 16 words: words 0-1 offset, words 2-13 data, words 14-15 unused
	gTest.Printf(_L("\nStarting RegisterTxBuffer\n"));
	r=gChanSlaveI2c.RegisterTxBuffer(chanId, 4, 12, 8);
	gTest(r==KErrNone);
	//
	// If a buffer is already registered but a notification has not yet been requested the API should return KErrNone
	gTest.Printf(_L("Starting (repeated) RegisterTxBuffer\n"));
	r=gChanSlaveI2c.RegisterTxBuffer(chanId, 4, 12, 8);
	gTest(r==KErrNone);
	//

	// Re-set the notification trigger
	// Now set the notification trigger
	gTest.Printf(_L("Starting SetNotificationTrigger\n"));
	triggerMask=ETxAllBytes;
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	//
	// If a buffer is already registered, a subsequent request to do the same should return KErrAlreadyExists
	gTest.Printf(_L("Starting RegisterTxBuffer (to be rejected)\n"));
	r=gChanSlaveI2c.RegisterTxBuffer(chanId, 4, 12, 8);
	gTest(r==KErrAlreadyExists);
	//
	// Now instruct the bus implementation to represent transmission of the required number of words to the bus master.
	gTest.Printf(_L("Starting SimulateTxNWords (to be rejected)\n"));
	r=gChanSlaveI2c.SimulateTxNWords(busIdI2c, chanId, 12);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after transmitting data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Tx test completed OK\n"));
	//
	// Repeat for each error condition. Re-use the buffer previously registered
	//
	// Re-set the notification trigger
	gTest.Printf(_L("Starting SetNotificationTrigger\n"));
	triggerMask=ETxAllBytes|ETxOverrun;
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	// Now instruct the bus implementation to represent transmission of less than the required number of words
	// to the bus master (Tx Overrun)
	gTest.Printf(_L("Starting SimulateTxNWords for Tx Overrun\n"));
	r=gChanSlaveI2c.SimulateTxNWords(busIdI2c, chanId, 10);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after transmitting data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Tx Overrun test completed OK\n"));
	// Re-set the notification trigger
	triggerMask=ETxAllBytes|ETxUnderrun;
	gTest.Printf(_L("Starting SetNotificationTrigger\n"));
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	// Now instruct the bus implementation to represent the bus master attempting to read more words than
	// anticipated (Tx Underrun)
	gTest.Printf(_L("Starting SimulateTxNWords for Tx Underrun\n"));
	r=gChanSlaveI2c.SimulateTxNWords(busIdI2c, chanId, 14);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after transmitting data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Tx Underrun test completed OK\n"));

	//
	//		Simultaneous Rx,Tx tests
	//
	// For these tests, the proxy client (iic_slaveclient) will check that the expected results are witnessed
	// in the required order, and will complete the TRequestStatus when the sequence is complete (or error occurs).
	//
	// Set the notification trigger for both Rx and Tx
	triggerMask=ERxAllBytes|ETxAllBytes;
	gTest.Printf(_L("\nStarting SetNotificationTrigger with ERxAllBytes|ETxAllBytes\n"));
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	// Now instruct the bus implementation to represent receipt of the required number of words from the bus master.
	gTest.Printf(_L("Starting SimulateRxNWords\n"));
	r=gChanSlaveI2c.SimulateRxNWords(busIdI2c, chanId, 8);
	gTest(r==KErrNone);
	// Now instruct the bus implementation to represent transmission of the required number of words to the bus master.
	gTest.Printf(_L("Starting SimulateTxNWords\n"));
	r=gChanSlaveI2c.SimulateTxNWords(busIdI2c, chanId, 12);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after receiving and transmitting data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Rx, Tx test completed OK\n"));
	//
	// Set the notification trigger for both Rx and Tx
	gTest.Printf(_L("Starting SetNotificationTrigger with ERxAllBytes|ETxAllBytes\n"));
	triggerMask=ERxAllBytes|ETxAllBytes;
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	// Now instruct the bus implementation to represent transmission of the required number of words to the bus master.
	gTest.Printf(_L("Starting SimulateTxNWords\n"));
	r=gChanSlaveI2c.SimulateTxNWords(busIdI2c, chanId, 12);
	gTest(r==KErrNone);
	// Now instruct the bus implementation to represent receipt of the required number of words from the bus master.
	gTest.Printf(_L("Starting SimulateRxNWords\n"));
	r=gChanSlaveI2c.SimulateRxNWords(busIdI2c, chanId, 8);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after receiving and transmitting data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Tx, Rx test completed OK\n"));
	//
	// Set the notification trigger for both Rx and Tx
	gTest.Printf(_L("Starting SetNotificationTrigger with ERxAllBytes|ETxAllBytes\n"));
	triggerMask=ERxAllBytes|ETxAllBytes;
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	// Now instruct the bus implementation to represent simultaneous transmission of the required number of words (12)
	// to the bus master and receipt of the required number of words (8) from the bus master
	gTest.Printf(_L("Starting SimulateRxTxNWords\n"));
	r=gChanSlaveI2c.SimulateRxTxNWords(busIdI2c, chanId, 8, 12);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after receiving and transmitting data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Tx with Rx test completed OK\n"));

	// Clear the trigger mask - this is just invoking SetNotificationTrigger with a zero trigger
	// so that no subsequent triggers are expected (and so no TRequestStatus is provided)
	gTest.Printf(_L("Starting SetNotificationTrigger with 0\n"));
	triggerMask=0;
	r=gChanSlaveI2c.SetNotifNoTrigger(chanId,triggerMask);
	gTest(r==KErrNone);

	//
	//		Rx Overrun and Tx Underrun when both Rx and Tx notifications are requested
	//
	gTest.Printf(_L("Starting RxOverrun-TxUnderrun with simultaneous Rx,Tx notification requests\n"));
	gChanSlaveI2c.TestOverrunUnderrun(busIdI2c,chanId,status);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after RxOverrun-TxUnderrun with simultaneous Rx,Tx notification requests= %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("RxOverrun-TxUnderrun with simultaneous Rx,Tx notification requests test completed OK\n"));


	//
	//		Bus Error tests
	//

	// Simulate a bus error
	// A bus error will cause all pending bus activity to be aborted.
	// Request a notification, then simulate a bus error
	triggerMask=ERxAllBytes|ETxAllBytes;
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	gTest.Printf(_L("Starting SimulateBusErr\n"));
	r = gChanSlaveI2c.SimulateBusErr(busIdI2c,chanId);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after receiving data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Bus error test completed OK\n"));

	// Clear the trigger mask and prepare for the next test
	// This is unnecessary if the SetNotificationTrigger for the following test
	// is called within the timeout period applied for Client responses ...
	// but it represents a Client ending a transaction cleanly, and so is
	// left here as an example
	gTest.Printf(_L("\nStarting SetNotificationTrigger with 0\n"));
	triggerMask=0;
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);

	// Simulate Master timeout
	// Do this by:
	// - Requesting a trigger for Tx
	// - simulating the Master performing a read (ie the PSL indicates a Tx event) to start the transaction
	// - provide a buffer for Tx, and request notification of Tx events, ie wait for Master response
	// - block the PSL Tx notification to the PIL, so that the PIL timeout timer expires when a simulated Tx event
	//   is next requested
	//
	// Indicate the test to be performed
	gTest.Printf(_L("\nStarting BlockNotification\n"));
	// Register a buffer for Tx, then set the notification trigger
	gTest.Printf(_L("RegisterTxBuffer - for Master to start the transaction\n"));
	r=gChanSlaveI2c.RegisterTxBuffer(chanId, 4, 12, 8);
	gTest(r==KErrNone);
	gTest.Printf(_L("SetNotificationTrigger - for Master to start the transaction\n"));
	triggerMask=ETxAllBytes;
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	// Now instruct the bus implementation to simulate the Master reading the expected number of words
	gTest.Printf(_L("Starting SimulateTxNWords\n"));
	r=gChanSlaveI2c.SimulateTxNWords(busIdI2c, chanId, 12);
	gTest(r==KErrNone);
	// Wait for the notification
	User::WaitForRequest(status);
	gTest.Printf(_L("Status request completed\n"));
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after receiving data = %d\n"),r);
		gTest(r==KErrNone);
		}
	// Client is now expected to perform its part of the transaction - so pretend we need another Tx
	//  - but block completion of the Tx so that we generate  a bus error
	gTest.Printf(_L("SetNotificationTrigger - for second part of the transaction\n"));
	triggerMask=ETxAllBytes;
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	gTest.Printf(_L("BlockNotification\n"));
	r=gChanSlaveI2c.BlockNotification(busIdI2c, chanId);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after receiving data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("Blocked notification test completed OK\n"));
    // Now instruct the bus implementation to represent the bus master attempting to read the required number of words
    gTest.Printf(_L("\nStarting SimulateTxNWords\n"));
    r=gChanSlaveI2c.SimulateTxNWords(busIdI2c, chanId, 12);
    gTest(r==KErrNone);
	// Re-set the notification trigger - for the 'blocked' Tx
	// This is required because, in the event of a bus error, the set of requested Rx,Tx
	// flags are cleared
	gTest.Printf(_L("Starting SetNotificationTrigger with ETxAllBytes\n"));
	triggerMask=ETxAllBytes;
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);
	// Remove the block
	gTest.Printf(_L("Starting UnblockNotification\n"));
	r=gChanSlaveI2c.UnblockNotification(busIdI2c, chanId);
	gTest(r==KErrNone);
	//
	// Wait for the notification
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("TRequestStatus value after receiving data = %d\n"),r);
		gTest(r==KErrNone);
		}
	gTest.Printf(_L("UnBlocked notification test completed OK\n"));
	// Clear the trigger mask
	gTest.Printf(_L("Starting SetNotificationTrigger with 0\n"));
	triggerMask=0;
	r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
	gTest(r==KErrNone);

	//Test the PIL behavior for a client timeout: request notification of an event
	//but deliberately delay the client response. The PIL should return KErrTimedOut 
	//when a subsequent request for a notification is made.
	gTest.Printf(_L("Starting test for SendBusErrorAndReturn.\n"));
	
	// For Rx, specify buffer granularity=4 (32-bit words), 8 words to receive, offset of 16 bytes
	// 64 bytes as 16 words: words 0-3 offset, words 4-11 data, words 12-15 unused
	gTest.Printf(_L("Starting RegisterRxBuffer\n"));
	r=gChanSlaveI2c.RegisterRxBuffer(chanId, 4, 8, 16);
	gTest(r==KErrNone);

	// Now set the notification trigger
	//TRequestStatus status;
	triggerMask=ERxAllBytes;
	
    gTest.Printf(_L("Starting SetNotificationTrigger with ERxAllBytes\n"));
    r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
    gTest(r==KErrNone);
    // Now instruct the bus implementation to represent receipt of the required number of words from the bus master.
    gTest.Printf(_L("Starting SimulateRxNWords\n"));
    r=gChanSlaveI2c.SimulateRxNWords(busIdI2c, chanId, 8);
    gTest(r==KErrNone);
    //
    // Wait for the notification
    User::WaitForRequest(status);
    r=status.Int();
    if(r != KErrNone)
        {
        gTest.Printf(_L("TRequestStatus value after receiving data = %d\n"),r);
        gTest(r==KErrNone);
        }
    //Delay the client response to exceed the timeout period, and check that the next
    //request for a notification encounters the expected error code.
    User::After(1000 * 1000);
    r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
    gTest(r==KErrNone);
    User::WaitForRequest(status);
	r = status.Int();
    if(r!=KErrTimedOut)
        {
        gTest.Printf(_L("TRequestStatus value = %d\n"),status.Int());
        gTest(r==KErrTimedOut);
        } 
    gTest.Printf(_L("The test for SendBusErrorAndReturn is completed OK\n"));
	// Release the channel
	r = gChanSlaveI2c.ReleaseChannel( chanId );
	gTest(r==KErrNone);

	delete i2cBuf;
#else
	gTest.Printf(_L("\nSlaveRxTxNotificationTests only supported when SLAVE_MODE is defined\n"));
#endif

	return r;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_IIC-2404
//! @SYMTestType        UT
//! @SYMPREQ            PREQ2128,2129
//! @SYMTestCaseDesc    This test case tests that MasterSlave channels can only be used in one mode at a time, and that
//!						if captured for Slave operation or with transactions queued for Master operation the channel can
//!						not be de-registered.
//! @SYMTestActions     0) 	Capture the channel for Slave operation. Attempt to synchronously queue a transaction
//!							on the channel. Attempt to asynchronously queue a transaction on the channel. Attempt
//!						    to de-register the channel.Release the Slave channel
//!
//!						1) Use controlio to block completion of queued transactions. Request asynchronous queue
//!						   transaction. Attempt to capture the channel for Slave operation. Attempt to de-register
//!						   the channel. Unblock completion of transactions and wait for the TRequestStatus for the
//!						   transaction to be completed.
//!
//! @SYMTestExpectedResults 0) 	Once captured for Slave operation, attempts to queue a transaction or de-register the channel
//!							    return KErrInUse, exits otherwise.
//!						1) With a transaction queued, attempt to capture the channel returns KErrInUse, exits otherwise.
//!						   Attempt to de-register channel returns KErrInUse, exits otherwise. The TRequestStatus should
//!						   be set to KErrTimedOut, exits otherwise.
//!
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt MasterSlaveAcquisitionTests()
//
//	Test to check that:
//	(1) A Master-Slave channel that has been captured for use in Slave mode will not allow requests for
//		queing transactions to be accepted
//	(2) A Master-Slave channel that has been captured for use in Slave mode can not be de-registered
//	(3) A Master-Slave channel that has one or more transactions queued in its Master channel transaction queue
//		can not be captured for use in Slave Made
//	(4) A Master-Slave channel that has one or more transactions queued in its Master channel transaction queue
//		can not be de-registered
//
	{
	gTest.Printf(_L("\n\nStarting MasterSlaveAcquisitionTests\n"));
	TInt r=KErrNone;

#if defined(MASTER_MODE) && defined(SLAVE_MODE)
	//	Create a Master-Slave channel
	RBusDevIicClient chanMasterSlaveI2c;
	TBufC<18> proxyName;
	if(!aStandAloneChan)
		proxyName = KIicProxyFileNameRoot;
	else
		proxyName = KIicProxyFileNameRootCtrlLess;
	r = chanMasterSlaveI2c.Open(proxyName);
	gTest(r==KErrNone);
	r = chanMasterSlaveI2c.InitSlaveClient();	// Initialise callback used for Slave processing
	gTest(r==KErrNone);
	//
	//	Capture the channel for Slave operation
	//  Attempt to synchronously queue a transaction on the channel - expect KErrInUse as a response
	//  Attempt to asynchronously queue a transaction on the channel - expect KErrInUse as a response
	//  Attempt to de-register the channel - expect KErrInUse as a response
	//  Release the Slave channel
	//
	// Create a I2C configuration buffer and the configuration data for use in capturing gChanSlaveI2c
	TUint32 busIdI2c = 0;
	TConfigI2cBufV01* i2cBuf=NULL;
	r=CreateSlaveChanI2cConfig(i2cBuf, busIdI2c, 12);	// 12 is the MasterSlave channel number
	gTest(r==KErrNone);
	TInt chanId;

	gTest.Printf(_L("\nStarting synchronous CaptureChannel \n"));
	r = chanMasterSlaveI2c.CaptureChannel(busIdI2c, i2cBuf, chanId );
	gTest.Printf(_L("Synchronous CaptureChannel returned = %d, chanId=0x%x\n"),r,chanId);
	gTest(r==KErrNone);
	//
	_LIT(halfDuplexText,"Half Duplex Text");
	TBuf8<17> halfDuplexBuf_8;
	halfDuplexBuf_8.Copy(halfDuplexText);
	TUsideTferDesc* tfer = NULL;
	r=CreateSingleUserSideTransfer(tfer, EMasterWrite, 8, &halfDuplexBuf_8, NULL);
	if(r!=KErrNone)
		return r;
	if(tfer==NULL)
		return KErrGeneral;
	//
	TUsideTracnDesc* tracn = NULL;
	r = CreateSingleUserSideTransaction(tracn, EI2c, i2cBuf, tfer, NULL, 0, NULL, NULL);
	if(r!=KErrNone)
		return r;
	if(tracn==NULL)
		return KErrGeneral;


	gTest.Printf(_L("\nStarting synchronous QueueTransaction \n"));
	r = chanMasterSlaveI2c.QueueTransaction(busIdI2c, tracn);
	gTest.Printf(_L("Synchronous QueueTransaction returned = %d\n"),r);
	gTest(r==KErrInUse);
	gTest.Printf(_L("\nStarting asynchronous QueueTransaction \n"));
	TRequestStatus status;
	chanMasterSlaveI2c.QueueTransaction(status, busIdI2c, tracn);
	User::WaitForRequest(status);
	if(status != KErrInUse)
		{
		gTest.Printf(_L("TRequestStatus value after queue = %d\n"),status.Int());
		gTest(r==KErrInUse);
		}
//
//	// If it is stand-alone channel, the client is responsible for channel creation.
//	// So the RegisterChan and DeRegisterChan are not needed.
	if(aStandAloneChan == 0)
		{
		gTest.Printf(_L("\nStarting deregistration of captured channel\n"));
		r = chanMasterSlaveI2c.DeRegisterChan(busIdI2c);
		gTest.Printf(_L("DeRegisterChan returned = %d\n"),r);
		gTest(r==KErrInUse);
		}

	gTest.Printf(_L("\nInvoke ReleaseChannel for chanId=0x%x \n"),chanId);
	r = chanMasterSlaveI2c.ReleaseChannel( chanId );
	gTest.Printf(_L("ReleaseChannel returned = %d\n"),r);
	gTest(r==KErrNone);

	//
	//	Use ControlIO/StaticExtension to block transactions on the Master Channel
	//  Queue an asynchronous transaction on the channel
	//  Attempt to capture the channel for Slave operation - expect KErrInUse as a response
	//  Attempt to de-register the channel - expect KErrInUse as a response
	//  Unblock the channel
	//  Check for (timed out) completion of the transaction
	//
	gTest.Printf(_L("Invoking BlockReqCompletion\n"));
	r = chanMasterSlaveI2c.BlockReqCompletion(busIdI2c);
	gTest.Printf(_L("BlockReqCompletion returned = %d\n"),r);
	//
	gTest.Printf(_L("Queueing first transaction \n"));
	chanMasterSlaveI2c.QueueTransaction(status, busIdI2c, tracn);
	//
	User::After(50000);
	//
	gTest.Printf(_L("\nStarting synchronous CaptureChannel \n"));
	r = chanMasterSlaveI2c.CaptureChannel(busIdI2c, i2cBuf, chanId );
	gTest.Printf(_L("Synchronous CaptureChannel returned = %d, chanId=0x%x\n"),r,chanId);
	gTest(r==KErrInUse);

	// If it is stand-alone channel, the client is responsible for channel creation.
	// So the RegisterChan and DeRegisterChan are not needed.
	if(aStandAloneChan == 0)
		{
		gTest.Printf(_L("\nStarting deregistration of channel\n"));
		r = chanMasterSlaveI2c.DeRegisterChan(busIdI2c);
		gTest.Printf(_L("DeRegisterChan returned = %d\n"),r);
		gTest(r==KErrInUse);
		}
	gTest.Printf(_L("Invoking UnlockReqCompletion\n"));
	r = chanMasterSlaveI2c.UnblockReqCompletion(busIdI2c);
	gTest.Printf(_L("UnblockReqCompletion returned = %d\n"),r);
	//
	User::After(50000);
	//
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrTimedOut)
		{
		gTest.Printf(_L("TRequestStatus value after queue = %d\n"),r);
		gTest(r==KErrTimedOut);
		}
	r=KErrNone; // Ensure error code is not propagated

	delete i2cBuf;
	delete tfer;
	delete tracn;
	chanMasterSlaveI2c.Close();
#else
	gTest.Printf(_L("\nMasterSlaveAcquisitionTests only supported when both MASTER_MODE and SLAVE_MODE are defined\n"));
#endif

	return r;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_IIC-2404
//! @SYMTestType        UT
//! @SYMDEF             DEF141732
//! @SYMTestCaseDesc    This test case tests the inline functions of DIicBusChannel interface.
//! @SYMTestActions     Call Kernel-side proxy client function to perform interface tests.
//! @SYMTestExpectedResults Kernel-side proxy client should return with KErrNone.
//! @SYMTestPriority        Medium
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt IicInterfaceInlineTests()
    {
    if(aStandAloneChan == 1)
        {
        gTest.Printf(_L("\n\nStarting IicInterfaceInlineTests\n"));
        TInt r=KErrNone;
        r = gChanMasterSpi.TestIiicChannelInlineFunc();
        return r;  
        }
    else
        {
        gTest.Printf(_L("\nIicInterfaceInlineTests can only be run in Standalone mode\n"));
        return KErrNone;
        }
    }

//Only get called in stand alone mode
LOCAL_C TInt IicTestStubs()
    {
    //Function to call the stub methods for Master and slave channels
    //when Master and Slave functionality has not been built. The stubs
    //return KErrNotSupported.
    TInt r=KErrNone;
    
    TUint32 busIdI2c = 0;
    TConfigI2cBufV01* i2cBuf=NULL;
    TRequestStatus status;

    //Starting master channel stubs test.
    //a valid transaction is required when calling the Master QueueTransaction stub.
    //Use I2C channel here. In MASTER_MODE, channelId starting from 10, 
    //and 10 is a master channel
    SET_BUS_TYPE(busIdI2c,EI2c);
    SET_CHAN_NUM(busIdI2c,10);
    // aDeviceId=1 ... 100kHz ... aTimeoutPeriod=100 ... aTransactionWaitCycles=10 - arbitrary paarmeters.
    r=CreateI2cBuf(i2cBuf, EI2cAddr7Bit, 36, ELittleEndian, 100);
    gTest(r==KErrNone);

    // Use a single transfer
    _LIT(halfDuplexText,"Half Duplex Text");
    TBuf8<17> halfDuplexBuf_8;
    halfDuplexBuf_8.Copy(halfDuplexText);
    TUsideTferDesc* tfer = NULL;
    r = CreateSingleUserSideTransfer(tfer, EMasterWrite, 8, &halfDuplexBuf_8, NULL);
    gTest(r==KErrNone);

    // Create the transaction object
    TUsideTracnDesc* tracn = NULL;
    r = CreateSingleUserSideTransaction(tracn, EI2c, i2cBuf, tfer, NULL, 0, NULL, NULL);
    gTest(r==KErrNone);

    // queue a synchronous transaction
    gTest.Printf(_L("\n\nStarting synchronous QueueTransaction \n"));
    r = gChanMasterI2c.QueueTransaction(busIdI2c, tracn);
    gTest.Printf(_L("Synchronous QueueTransaction returned = %d\n"),r);
    //Queueing a transaction in SLAVE_MODE should return KErrNotSupported 
    gTest(r==KErrNotSupported); 
    
    // queue an asynchronous transaction and cancel the trasnaction
    // QueueTransaction actually completes before CancelAsyncOperation with KErrNotSupported
    // In test driver, we pretend the request is still in the queue and then cancel it.
    gChanMasterI2c.QueueTransaction(status, busIdI2c, tracn);
    gChanMasterI2c.CancelAsyncOperation(&status, busIdI2c);
    User::WaitForRequest(status);
    if(status != KErrNotSupported)
        {
        gTest.Printf(_L("TRequestStatus value after queue = %d\n"),status.Int());
        gTest(r==KErrNotSupported);
        }
    //spare1 is an unused method that is present to provide for future extension,
    //which just returns KErrNotSupported. 
    r = gChanMasterI2c.TestSpare1(busIdI2c);
    gTest(r == KErrNotSupported);
    //StaticExtension is present for PSL implementations to override, the default
    //implementation just returns KErrNotSupported
    r = gChanMasterI2c.TestStaticExtension(busIdI2c);
    gTest(r == KErrNotSupported);
    //free the memory
    delete i2cBuf;
    delete tfer;
    delete tracn;

    //Start to test slave channel operations
    SET_BUS_TYPE(busIdI2c,EI2c);
    SET_CHAN_NUM(busIdI2c,11); // 11 is the Slave channel number
    //
    // clock speed=36Hz, aTimeoutPeriod=100 - arbitrary parameter
    r=CreateI2cBuf(i2cBuf, EI2cAddr7Bit, 36, ELittleEndian, 100);
    gTest(r==KErrNone);

    // Synchronous capture of a Slave channel.
    TInt chanId = 0; // Initialise to zero to silence compiler ...
    gTest.Printf(_L("\n\nStarting synchronous CaptureChannel \n"));
    r = gChanSlaveI2c.CaptureChannel(busIdI2c, i2cBuf, chanId );
    gTest.Printf(_L("Synchronous CaptureChannel returned = %d, aChanId=0x%x\n"),r,chanId);
    gTest(r==KErrNotSupported);
    
    gTest.Printf(_L("Starting RegisterRxBuffer\n"));
    r=gChanSlaveI2c.RegisterRxBuffer(chanId, 4, 8, 16);
    gTest(r==KErrNotSupported);
    
    gTest.Printf(_L("\nStarting RegisterTxBuffer\n"));
    r=gChanSlaveI2c.RegisterTxBuffer(chanId, 4, 12, 8);
    gTest(r==KErrNotSupported);
    //
    // Now set the notification trigger
    TInt triggerMask=ERxAllBytes;

    gTest.Printf(_L("Starting SetNotificationTrigger with ERxAllBytes\n"));
    r=gChanSlaveI2c.SetNotificationTrigger(chanId,triggerMask,&status);
    gTest(r==KErrNotSupported);
    
    r = gChanSlaveI2c.TestSpare1(busIdI2c);
    gTest(r == KErrNotSupported);
    r = gChanSlaveI2c.TestStaticExtension(busIdI2c);
    gTest(r == KErrNotSupported);
    delete i2cBuf;

    //Start to test MasterSlave channel operations
    //Create a Master-Slave channel
    RBusDevIicClient chanMasterSlaveI2c;
    TBufC<18> proxyName;
    proxyName = KIicProxyFileNameRootStubs;
    r = chanMasterSlaveI2c.Open(proxyName);
    gTest(r==KErrNone);
    
    SET_BUS_TYPE(busIdI2c,EI2c);
    SET_CHAN_NUM(busIdI2c,12); // 12 is the MasterSlave channel number
    r = chanMasterSlaveI2c.TestStaticExtension(busIdI2c);
    gTest(r==KErrNotSupported);
    chanMasterSlaveI2c.Close();

    return KErrNone;
    }

LOCAL_C TInt RunTests()
//
//	Utility method to invoke the separate tests
//
	{
	TInt r =KErrNone;
	r = IicInterfaceInlineTests();
    if(r!=KErrNone)
        return r;
    
	r = MasterBasicTests();
	if(r!=KErrNone)
		return r;

	r = SlaveRxTxNotificationTests();
	if(r!=KErrNone)
		return r;

	r = SlaveChannelCaptureReleaseTests();
	if(r!=KErrNone)
		return r;

	r = MasterExtTests(KTransactionWithPreamble);
	if(r!=KErrNone)
		return r;

	r = MasterExtTests(KTransactionWithMultiTransc);
	if(r!=KErrNone)
		return r;

	r = MasterExtTests(KTransactionWithMultiTransc|KTransactionWithPreamble);
	if(r!=KErrNone)
		return r;

	r = MasterTransactionTests();
	if(r!=KErrNone)
		return r;

	r = MasterSlaveAcquisitionTests();
	if(r!=KErrNone)
		return r;

	return KErrNone;
	}

GLDEF_C TInt E32Main()
//
// Main
//
    {
	gTest.Title();
	gTest.Start(_L("Test IIC API\n"));

	TInt r = KErrNone;

    // Turn off lazy dll unloading
    RLoader l;
    gTest(l.Connect()==KErrNone);
    gTest(l.CancelLazyDllUnload()==KErrNone);
    l.Close();

#ifdef IIC_SIMULATED_PSL
	gTest.Next(_L("Start the IIC with controller test\n"));
	aStandAloneChan = 0;
	gTest.Next(_L("Load Simulated IIC PSL bus driver"));
	r = User::LoadPhysicalDevice(KIicPslFileName);
	gTest.Printf(_L("return value r=%d"),r);
	gTest(r==KErrNone || r==KErrAlreadyExists);

	gTest.Next(_L("Load Simulated PSL SPI bus driver"));
	r = User::LoadPhysicalDevice(KSpiFileName);
	gTest.Printf(_L("return value r=%d"),r);
	gTest(r==KErrNone || r==KErrAlreadyExists);

	gTest.Next(_L("Load Simulated PSL I2C bus driver"));
	r = User::LoadPhysicalDevice(KI2cFileName);
	gTest.Printf(_L("return value r=%d"),r);
	gTest(r==KErrNone || r==KErrAlreadyExists);

	gTest.Next(_L("Load kernel-side proxy IIC client"));
	r = User::LoadLogicalDevice(KIicProxyFileName);
	gTest(r==KErrNone || r==KErrAlreadyExists);

	gTest.Next(_L("Load kernel-side proxy IIC slave client"));
	r = User::LoadLogicalDevice(KIicProxySlaveFileName);
	gTest(r==KErrNone || r==KErrAlreadyExists);

	__KHEAP_MARK;
	// First ascertain what bus options are available.

	// SPI has Master channel numbers 1,2 and 4, Slave channel number 3
	// Open a Master SPI channel to the kernel side proxy
	TBufC<30> proxyName(KIicProxyFileNameRoot);
	r = gChanMasterSpi.Open(proxyName);
	gTest(r==KErrNone);

	// I2C has Master channel numbers 10 and 11, if built with MASTER_MODE, only
	// I2C has Slave channel numbers 12 and 13, if built with SLAVE_MODE, only
	// I2C has Master channel number 10 and Slave channel numer 11 if built with both MASTER_MODE and SLAVE_MODE
	// Open a Master I2C channel to the kernel side proxy
	r = gChanMasterI2c.Open(proxyName);
	gTest(r==KErrNone);
	TBufC<15> proxySlaveName(KIicProxySlaveFileNameRoot);
	r = gChanSlaveI2c.Open(proxySlaveName);
	gTest(r==KErrNone);
	r = gChanSlaveI2c.InitSlaveClient();
	gTest(r==KErrNone);

	// Instigate tests
	r = RunTests();
	gTest(r==KErrNone);

	gTest.Printf(_L("Tests completed OK, about to close channel\n"));

	gChanMasterSpi.Close();
	gChanMasterI2c.Close();
	gChanSlaveI2c.Close();
	
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
// Not safe to assume that heap clean-up has completed for the channels just closed, so insert a delay.(DEF145202)
	User::After(20 * 1000);
	__KHEAP_MARKEND;

	gTest.Next(_L("Free kernel-side proxy IIC client"));
	TInt err = User::FreeLogicalDevice(KIicProxyFileNameRoot);
	gTest(err==KErrNone || err==KErrAlreadyExists);

	gTest.Next(_L("Free kernel-side proxy IIC slave client"));
	err = User::FreeLogicalDevice(KIicProxySlaveFileNameRoot);
	gTest(err==KErrNone || err==KErrAlreadyExists);

	gTest.Next(_L("Free Simulated PSL I2C bus driver"));
	err = User::FreePhysicalDevice(KI2cFileName);
	gTest(err==KErrNone);

	gTest.Next(_L("Free Simulated PSL SPI bus driver"));
	err = User::FreePhysicalDevice(KSpiFileName);
	gTest(err==KErrNone);

	gTest.Next(_L("Free Simulated IIC PSL bus driver"));
	err = User::FreePhysicalDevice(KIicPslFileNameRoot);
	gTest(err==KErrNone);

	gTest.Next(_L("Start the controller-less IIC test\n"));
	aStandAloneChan = 1;

	gTest.Next(_L("Load Simulated PSL SPI bus driver"));
	r = User::LoadPhysicalDevice(KSpiFileNameCtrlLess);
	gTest.Printf(_L("return value r=%d"),r);
	gTest(r==KErrNone || r==KErrAlreadyExists);

	gTest.Next(_L("Load Simulated PSL I2C bus driver"));
	r = User::LoadPhysicalDevice(KI2cFileNameCtrlLess);
	gTest.Printf(_L("return value r=%d"),r);
	gTest(r==KErrNone || r==KErrAlreadyExists);

	gTest.Next(_L("Load kernel-side proxy IIC client"));
	r = User::LoadLogicalDevice(KIicProxyFileNameCtrlLess);
	gTest(r==KErrNone || r==KErrAlreadyExists);

	gTest.Next(_L("Load kernel-side proxy IIC slave client"));
	r = User::LoadLogicalDevice(KIicProxySlaveFileNameCtrlLess);
	gTest(r==KErrNone || r==KErrAlreadyExists);

	// First ascertain what bus options are available.
	__KHEAP_MARK;
	// SPI has Master channel numbers 1,2 and 4, Slave channel number 3
	// Open a Master SPI channel to the kernel side proxy
	TBufC<30> proxyNameCtrlLess(KIicProxyFileNameRootCtrlLess);
	r = gChanMasterSpi.Open(proxyNameCtrlLess);
	gTest(r==KErrNone);

	// I2C has Master channel numbers 10 and 11, if built with MASTER_MODE, only
	// I2C has Slave channel numbers 12 and 13, if built with SLAVE_MODE, only
	// I2C has Master channel number 10 and Slave channel numer 11 if built with both MASTER_MODE and SLAVE_MODE
	// Open a Master I2C channel to the kernel side proxy
	r = gChanMasterI2c.Open(proxyNameCtrlLess);

	gTest(r==KErrNone);
	TBufC<35> proxySlaveNameCtrlLess(KIicProxySlaveFileNameRootCtrlLess);

	r = gChanSlaveI2c.Open(proxySlaveNameCtrlLess);
	gTest(r==KErrNone);
	r = gChanSlaveI2c.InitSlaveClient();
	gTest(r==KErrNone);

	// Instigate tests
	r = RunTests();
	gTest(r==KErrNone);

	gTest.Printf(_L("Tests completed OK, about to close channel\n"));

	gChanMasterSpi.Close();
	gChanMasterI2c.Close();
	gChanSlaveI2c.Close();

	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
// Not safe to assume that heap clean-up has completed for the channels just closed, so insert a delay.(DEF145202)
	User::After(20 * 1000);
	__KHEAP_MARKEND;

	gTest.Next(_L("Free kernel-side proxy IIC client"));

	err = User::FreeLogicalDevice(KIicProxyFileNameRootCtrlLess);
	gTest(err==KErrNone || err==KErrAlreadyExists);
	gTest.Next(_L("Free kernel-side proxy IIC slave client"));
	err = User::FreeLogicalDevice(KIicProxySlaveFileNameRootCtrlLess);
	gTest(err==KErrNone || err==KErrAlreadyExists);

	gTest.Next(_L("Free Simulated PSL I2C bus driver"));
	err = User::FreePhysicalDevice(KI2cFileNameCtrlLess);
	gTest(err==KErrNone);

	gTest.Next(_L("Free Simulated PSL SPI bus driver"));
	err = User::FreePhysicalDevice(KSpiFileNameCtrlLess);
	gTest(err==KErrNone);
	
    //For simplicity, the code coverage tests are executed in STANDALONE_CHANNEL mode
	//All the changes are made in test code, and not affect PIL.
	gTest.Next(_L("Start the code coverage tests"));

    gTest.Next(_L("Load Simulated PSL I2C bus driver"));
    r = User::LoadPhysicalDevice(KI2cFileNameStubs);
    gTest.Printf(_L("return value r=%d"),r);
    gTest(r==KErrNone || r==KErrAlreadyExists);

    gTest.Next(_L("Load kernel-side proxy IIC client"));
    r = User::LoadLogicalDevice(KIicProxyFileNameStubs);
    gTest(r==KErrNone || r==KErrAlreadyExists);

    gTest.Next(_L("Load kernel-side proxy IIC slave client"));
    r = User::LoadLogicalDevice(KIicProxySlaveFileNameStubs);
    gTest(r==KErrNone || r==KErrAlreadyExists);

    __KHEAP_MARK;
    TBufC<30> proxyNameStubs(KIicProxyFileNameRootStubs);
    // Open a Master I2C channel to the kernel side proxy
    r = gChanMasterI2c.Open(proxyNameStubs);

    gTest(r==KErrNone);
    TBufC<35> proxySlaveNameStubs(KIicProxySlaveFileNameRootStubs);

    r = gChanSlaveI2c.Open(proxySlaveNameStubs);
    gTest(r==KErrNone);
    r = gChanSlaveI2c.InitSlaveClient();
    gTest(r==KErrNone);

    // Instigate tests
    r = IicTestStubs();
    gTest(r==KErrNone);

    gTest.Printf(_L("Tests completed OK, about to close channel\n"));
    gChanMasterI2c.Close();
    gChanSlaveI2c.Close();

    UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
    // Not safe to assume that heap clean-up has completed for the channels just closed, so insert a delay.(DEF145202)
    User::After(20 * 1000);
    __KHEAP_MARKEND;

    gTest.Next(_L("Free kernel-side proxy IIC client"));

    err = User::FreeLogicalDevice(KIicProxyFileNameRootStubs);
    gTest(err==KErrNone || err==KErrAlreadyExists);
    gTest.Next(_L("Free kernel-side proxy IIC slave client"));
    err = User::FreeLogicalDevice(KIicProxySlaveFileNameRootStubs);
    gTest(err==KErrNone || err==KErrAlreadyExists);

    gTest.Next(_L("Free Simulated PSL I2C bus driver"));
    err = User::FreePhysicalDevice(KI2cFileNameStubs);
    gTest(err==KErrNone);

	gTest.Next(_L("End the code coverage tests"));
#else
	gTest.Printf(_L("Don't do the test if it is not IIC_SIMULATED_PSL"));
#endif
	gTest.End();
	return r;
    }

