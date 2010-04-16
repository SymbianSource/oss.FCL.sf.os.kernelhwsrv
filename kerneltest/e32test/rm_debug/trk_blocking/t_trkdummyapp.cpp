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
// Ensure that old insecure Trk debug agent cannot be installed
// as it should be blocked from SWInstall'ing by a trkdummyapp.exe contained within
// the base OS with the same SID as the insecure Trk.
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32test.h>
#include <e32ldr.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <f32dbg.h>
#include <f32file.h>
#include "t_trkdummyapp.h"

LOCAL_D RTest test(_L("T_TRKDUMMYAPP"));

CTrkDummyAppTest::CTrkDummyAppTest()
//
// CTrkDummyAppTest constructor
//
	{
	// nothing to do
	}

CTrkDummyAppTest* CTrkDummyAppTest::NewL()
//
// CRunModeAgent::NewL
//
	{
	CTrkDummyAppTest* self = new(ELeave) CTrkDummyAppTest();

  	self->ConstructL();

	return self;
	}

CTrkDummyAppTest::~CTrkDummyAppTest()
//
// CTrkDummyAppTest destructor
//
	{
	// Nothing to do
	}

void CTrkDummyAppTest::ConstructL()
//
// CTrkDummyAppTest::ConstructL
//
	{
	// nothing to do here
	}


CTrkDummyAppTest *TrkDummyTest;

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-TRKDUMMYAPP-0792
//! @SYMTestType        
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests that a known insecure debug agent trkapp.sis cannot be installed
//!                     by ensuring the existence of a Symbian OS common app called trkdummyapp.exe
//!                     already exists with the same Secure ID as the insecure app.
//! @SYMTestActions     
//!    
//!     1.              Calls RProcess.Create() on z:\sys\bin\trkdummyapp.exe. Fail if unsuccessful.
//!     
//!     2.              Obtain the Secure ID of the process derived from z:\sys\bin\trkdummyapp.exe.
//!
//!     3.              Close the process derived from z:\sys\bin\trkdummyapp.exe.
//!
//! @SYMTestExpectedResults 
//!
//!     1.              Fails if unable to create a process from z:\sys\bin\trkdummyapp.exe.
//!
//!     2.              The Secure ID of trkdummyapp.exe has the Secure ID 0x101F7159. Fail otherwise.
//! 
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

// Names of some test programs used for testing security
_LIT(KRMDebugSecurityTrkDummyApp,"z:\\sys\\bin\\trkdummyapp.exe");

void CTrkDummyAppTest::TestSecurityCheckPreventInsecureTrkDebugAgent(void)
	{

	test.Next(_L("TestSecurityCheckPreventInsecureTrkDebugAgent, SID 0x101F7159\n"));

	RProcess process;
	TInt err = process.Create(KRMDebugSecurityTrkDummyApp, KNullDesC, EOwnerProcess);
	test (err == KErrNone);

	// rendezvous with process
	TRequestStatus status;
	process.Rendezvous(status);

	// obtain the secure ID for the process
	TSecurityInfo secInfo(process);

	static const TSecureId KTrkDummyAppSID = 0x101F7159;

	test(secInfo.iSecureId.iId == KTrkDummyAppSID);

	// Kill the process, as we don't need it anymore
	process.Kill(KErrNone);

	process.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-TRKDUMMYAPP-0793
//! @SYMTestType        
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests that a known insecure debug agent trkapp.sis cannot be installed
//!                     by ensuring the existence of a Symbian OS common app called trkdummyapp.exe
//!                     already exists with the same Secure ID as the insecure app.
//! @SYMTestActions     
//!    
//!     1.              Calls RProcess.Create() on z:\sys\bin\trkdummyapp2.exe. Fail if unsuccessful.
//!     
//!     2.              Obtain the Secure ID of the process derived from z:\sys\bin\trkdummyapp2.exe.
//!
//!     3.              Close the process derived from z:\sys\bin\trkdummyapp2.exe.
//!
//! @SYMTestExpectedResults 
//!
//!     1.              Fails if unable to create a process from z:\sys\bin\trkdummyapp2.exe.
//!
//!     2.              The Secure ID of trkdummyapp.exe has the Secure ID 0x2000a7dd. Fail otherwise.
//! 
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

// Names of some test programs used for testing security
_LIT(KRMDebugSecurityTrkDummyApp2,"z:\\sys\\bin\\trkdummyapp2.exe");

void CTrkDummyAppTest::TestSecurityCheckPreventInsecureTrkDebugAgent2(void)
	{

	test.Next(_L("TestSecurityCheckPreventInsecureTrkDebugAgent2, SID 0x2000a7dd\n"));

	RProcess process;
	TInt err = process.Create(KRMDebugSecurityTrkDummyApp2, KNullDesC, EOwnerProcess);
	test (err == KErrNone);

	// rendezvous with process
	TRequestStatus status;
	process.Rendezvous(status);

	// obtain the secure ID for the process
	TSecurityInfo secInfo(process);

	static const TSecureId KTrkDummyAppSID2 = 0x2000a7dd;

	test(secInfo.iSecureId.iId == KTrkDummyAppSID2);

	// Kill the process, as we don't need it anymore
	process.Kill(KErrNone);

	process.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-TRKDUMMYAPP-2396
//! @SYMTestType        
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests that a known insecure debug agent trkapp.sis cannot be installed
//!                     by ensuring the existence of a Symbian OS common app called trkdummyapp200159D8.exe
//!                     already exists with the same Secure ID as the insecure app.
//! @SYMTestActions     
//!    
//!     1.              Calls RProcess.Create() on z:\sys\bin\trkdummyapp200159D8.exe. Fail if unsuccessful.
//!     
//!     2.              Obtain the Secure ID of the process derived from z:\sys\bin\trkdummyapp200159D8.exe.
//!
//!     3.              Close the process derived from z:\sys\bin\trkdummyapp200159D8.exe.
//!
//! @SYMTestExpectedResults 
//!
//!     1.              Fails if unable to create a process from z:\sys\bin\trkdummyapp200159D8.exe.
//!
//!     2.              The Secure ID of trkdummyapp200159D8.exe has the Secure ID 0x200159D8. Fail otherwise.
//! 
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

// Names of some test programs used for testing security
_LIT(KRMDebugSecurityTrkDummyApp200159D8,"z:\\sys\\bin\\trkdummyapp200159D8.exe");

void CTrkDummyAppTest::TestSecurityCheckPreventInsecureTrkDebugAgent200159D8(void)
	{
	test.Next(_L("TestSecurityCheckPreventInsecureTrkDebugAgent, SID 0x200159D8\n"));

	RProcess process;
	TInt err = process.Create(KRMDebugSecurityTrkDummyApp200159D8, KNullDesC, EOwnerProcess);
	test (err == KErrNone);

	// rendezvous with process
	TRequestStatus status;
	process.Rendezvous(status);

	// obtain the secure ID for the process
	TSecurityInfo secInfo(process);

	static const TSecureId KTrkDummyAppSID2 = 0x200159D8;

	test(secInfo.iSecureId.iId == KTrkDummyAppSID2);

	// Kill the process, as we don't need it anymore
	process.Kill(KErrNone);

	process.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-TRKDUMMYAPP-2397
//! @SYMTestType        
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests that a known insecure debug agent trkapp.sis cannot be installed
//!                     by ensuring the existence of a Symbian OS common app called trkdummyapp200170BC.exe
//!                     already exists with the same Secure ID as the insecure app.
//! @SYMTestActions     
//!    
//!     1.              Calls RProcess.Create() on z:\sys\bin\trkdummyapp200170BC.exe. Fail if unsuccessful.
//!     
//!     2.              Obtain the Secure ID of the process derived from z:\sys\bin\trkdummyapp200170BC.exe.
//!
//!     3.              Close the process derived from z:\sys\bin\trkdummyapp200170BC.exe.
//!
//! @SYMTestExpectedResults 
//!
//!     1.              Fails if unable to create a process from z:\sys\bin\trkdummyapp200170BC.exe.
//!
//!     2.              The Secure ID of trkdummyapp.exe has the Secure ID 0x200170BC. Fail otherwise.
//! 
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

// Names of some test programs used for testing security
_LIT(KRMDebugSecurityTrkDummyApp200170BC,"z:\\sys\\bin\\trkdummyapp200170BC.exe");

void CTrkDummyAppTest::TestSecurityCheckPreventInsecureTrkDebugAgent200170BC(void)
	{

	test.Next(_L("TestSecurityCheckPreventInsecureTrkDebugAgent, SID 0x200170BC\n"));

	RProcess process;
	TInt err = process.Create(KRMDebugSecurityTrkDummyApp200170BC, KNullDesC, EOwnerProcess);
	test (err == KErrNone);

	// rendezvous with process
	TRequestStatus status;
	process.Rendezvous(status);

	// obtain the secure ID for the process
	TSecurityInfo secInfo(process);

	static const TSecureId KTrkDummyAppSID2 = 0x200170BC;

	test(secInfo.iSecureId.iId == KTrkDummyAppSID2);

	// Kill the process, as we don't need it anymore
	process.Kill(KErrNone);

	process.Close();
	}

void CTrkDummyAppTest::ClientAppL()
//
// Performs each test in turn
//
	{
	test.Start(_L("ClientAppL"));

	TestSecurityCheckPreventInsecureTrkDebugAgent();

	TestSecurityCheckPreventInsecureTrkDebugAgent2();

	TestSecurityCheckPreventInsecureTrkDebugAgent200159D8();

	TestSecurityCheckPreventInsecureTrkDebugAgent200170BC();

	test.End();
	}



GLDEF_C TInt E32Main()
//
// Entry point for run mode debug driver test
//
	{
   TInt ret = KErrNone;

	// client
	CTrapCleanup* trap = CTrapCleanup::New();
	if (!trap)
		return KErrNoMemory;

   	test.Title();
   TrkDummyTest = CTrkDummyAppTest::NewL();
   if (TrkDummyTest != NULL)
       {
        __UHEAP_MARK;
	    TRAP(ret,TrkDummyTest->ClientAppL());
	    __UHEAP_MARKEND;

	    delete TrkDummyTest;
       }
       
	delete trap;

	return ret;
	}

// End of file - t_trkdummyapp.cpp





































































































































































































