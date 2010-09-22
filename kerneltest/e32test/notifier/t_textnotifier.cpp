// Copyright (c) 2003-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\notifier\t_textnotifier.cpp
// Overview:
// Test the RNotifier class.
// API Information:
// RNotifier
// Details:	
// - For IPC Ver. 1 and IPC Ver 2, connect to and start anotifier server.
// Perform a variety of tests including CancelNotifier, StartNotifier,
// UpdateNotifier, UpdateNotifierAndGetResponse, StartNotifierAndGetResponse.
// Verify results are as expected. Check for memory leaks and cleanup.
// - For IPC Ver. 1 and IPC Ver 2, using MNotifierManager, connect to and 
// start anotifier server. Perform a variety of tests including CancelNotifier, 
// StartNotifier, UpdateNotifier, UpdateNotifierAndGetResponse, 
// StartNotifierAndGetResponse, Notify and NotifyCancnel.
// Verify results are as expected. Check for memory leaks and cleanup.
// - Tests also removed but exported methods that they return expected values
// - Do interactive tests as requested.
// Platforms/Drives/Compatibility:
// Hardware (Automatic).
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include "textnotifier.h"
#include <e32debug.h>
#include <e32event.h>
#include <e32svr.h>

LOCAL_D RTest test(_L("T_TEXTNOTIFIER"));

const TUint KTimeOut=3000000; // 3 seconds

void DoMemoryLeakTests(TUid aUid,TBool aCheckMNotifierManager)
	{
	TInt r;
	TRequestStatus stat;

	test.Start(_L("Connect to notifier server"));
	RNotifier n;
	r = n.Connect();
	test(r==KErrNone);

	test.Next(_L("Get Notifier Server Heap Info"));
	static TBuf8<128> heapInfo1;
	heapInfo1.Zero();
	n.StartNotifierAndGetResponse(stat,aUid,KHeapData,heapInfo1);
	User::WaitForRequest(stat);
	n.CancelNotifier(aUid);
	TInt heapCellCount=stat.Int();
	test(heapCellCount>0);

	test.Next(_L("Repeated StartNotifierAndGetResponse"));
	for(TInt i=0; i<1000; i++)
		{
		TBuf8<128> response;
		response.SetMax();
		response.FillZ();
		response.Zero();
		n.StartNotifierAndGetResponse(stat,aUid,aCheckMNotifierManager?*&KMNotifierManager:*&KStartData,response);
		User::WaitForRequest(stat);
		n.CancelNotifier(aUid);
		test(stat==KErrNone);
		test(response==KResponseData);
		}

	test.Next(_L("Check Notifier Server Heap Info"));
	static TBuf8<128> heapInfo2;
	heapInfo2.Zero();
	n.StartNotifierAndGetResponse(stat,aUid,KHeapData,heapInfo2);
	User::WaitForRequest(stat);
	n.CancelNotifier(aUid);
	test(stat==heapCellCount);
	
	TInt size1, size2;
	TLex8 lex(heapInfo1);
	r = lex.Val(size1);
	test(r==KErrNone);
	lex.Assign(heapInfo2);
	r = lex.Val(size2);
	test(r==KErrNone);
	//allocated size after should not be greater than before BUT may be less with new allocator
	test(size2 <= size1); 

	test.Next(_L("Close connection to notifier server"));
	n.Close();

	test.End();
	}

void DoCleanumpTests(TUid aUid,TBool aCheckMNotifierManager)
	{
	TInt r;

	test.Start(_L("Connect to notifier server"));
	RNotifier n;
	r = n.Connect();
	test(r==KErrNone);

	test.Next(_L("StartNotifierAndGetResponse"));
	TBuf8<128> response;
	response.SetMax();
	response.FillZ();
	response.Zero();
	TRequestStatus stat;
	n.StartNotifierAndGetResponse(stat,aUid,aCheckMNotifierManager?*&KMNotifierManager:*&KStartData,response);
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(response==KResponseData);

	test.Next(_L("Close connection to notifier server"));
	n.Close();

	test.Next(_L("Connect to notifier server"));
	r = n.Connect();
	test(r==KErrNone);

	test.Next(_L("StartNotifierAndGetResponse (to check previous notifier was cancelled)"));
	response.SetMax();
	response.FillZ();
	response.Zero();
	n.StartNotifierAndGetResponse(stat,aUid,aCheckMNotifierManager?*&KMNotifierManagerWithCancelCheck:*&KStartWithCancelCheckData,response);
	User::WaitForRequest(stat);
	test(stat==KTestNotifierWasPreviouselyCanceled);
	test(response==KResponseData);

	test.Next(_L("Close connection to notifier server"));
	n.Close();

	test.End();
	}



void DoTests(TUid aUid,TBool aCheckMNotifierManager)
	{	
	TInt r;
	
	test.Start(_L("Connect to notifier server"));
	RNotifier n;
	r = n.Connect();
	test(r==KErrNone);

	test.Next(_L("StartNotifier (without response)"));
	r = n.StartNotifier(aUid,aCheckMNotifierManager?*&KMNotifierManager:*&KStartData);
	RDebug::Printf("r=%d", r);
	test(r==KErrNone);

	test.Next(_L("CancelNotifier"));
	r = n.CancelNotifier(aUid);
	test(r==KErrNone);

	test.Next(_L("StartNotifier"));
	TBuf8<128> response;
	response.SetMax();
	response.FillZ();
	response.Zero();
	r = n.StartNotifier(aUid,aCheckMNotifierManager?*&KMNotifierManager:*&KStartData,response);
	test(r==KErrNone);
	test(response==KResponseData);

	test.Next(_L("UpdateNotifier"));
	response.SetMax();
	response.FillZ();
	response.Zero();   // EKA1 text notifier dies if current length < length of response
	r = n.UpdateNotifier(aUid,aCheckMNotifierManager?*&KMNotifierManager:*&KUpdateData,response);
	test(r==KErrNone);
	test(response==KResponseData);

	test.Next(_L("UpdateNotifierAndGetResponse"));
	response.SetMax();
	response.FillZ();
	response.Zero();   // EKA1 text notifier dies if current length < length of response
	TRequestStatus updateStat;
	n.UpdateNotifierAndGetResponse(updateStat,aUid,aCheckMNotifierManager?*&KMNotifierManager:*&KUpdateData,response);
	User::WaitForRequest(updateStat);
	test(updateStat==KErrNone);
	test(response==KResponseData);
	
	test.Next(_L("CancelNotifier"));
	r = n.CancelNotifier(aUid);
	test(r==KErrNone);

	test.Next(_L("StartNotifierAndGetResponse (to check previous notifier was cancelled)"));
	response.SetMax();
	response.FillZ();
	response.Zero();
	TRequestStatus stat;
	n.StartNotifierAndGetResponse(stat,aUid,aCheckMNotifierManager?*&KMNotifierManagerWithCancelCheck:*&KStartWithCancelCheckData,response);
	User::WaitForRequest(stat);
	test(stat==KTestNotifierWasPreviouselyCanceled);
	test(response==KResponseData);

	test.Next(_L("CancelNotifier"));
	r = n.CancelNotifier(aUid);
	test(r==KErrNone);

	test.Next(_L("Close connection to notifier server"));
	n.Close();

	test.Next(_L("Memory leak tests"));
	DoMemoryLeakTests(aUid,aCheckMNotifierManager);

	test.Next(_L("Session cleanup test"));
	DoCleanumpTests(aUid,aCheckMNotifierManager);

	test.End();
	}


void TestNotify()
//
// Test Notify by launching a simple notifier. Gets closed
// using timer and simulated keypress.
//
	{
	TInt r;
	test.Start(_L("Connect to notifier server"));
	RNotifier n;
	r = n.Connect();
	test(r==KErrNone);
	TInt button=0;
	TRequestStatus status;
	TRequestStatus timerStatus;
	RTimer timer;
	timer.CreateLocal();

	test.Next(_L("Launching simple notifier"));
	_LIT(KLine1,"Line1 - Select Button2");
	_LIT(KLine2,"Line2 - or press enter");
	_LIT(KButton1,"Button1");
	_LIT(KButton2,"Button2");

	n.Notify(KLine1,KLine2,KButton1,KButton2,button,status);
	timer.After(timerStatus,KTimeOut); // launch timer for getting control back after timeout
	User::WaitForRequest(status, timerStatus);
	if (status==KRequestPending)
		{
		test.Printf(_L("Timeout in waiting for keypress, continuing\n"));

		// make the notifier to disappear
		TRawEvent eventDown;
		eventDown.Set(TRawEvent::EKeyDown,EStdKeyEnter);
		TRawEvent eventUp;
		eventUp.Set(TRawEvent::EKeyUp,EStdKeyEnter);
		UserSvr::AddEvent(eventDown);
		UserSvr::AddEvent(eventUp);
		User::WaitForRequest(status); // wait again
		}
	else
		{
		timer.Cancel();
		}
	
	timer.Close();

	test(status.Int()==KErrNone);

	test.Next(_L("Close connection to notifier server"));
	n.Close();

	test.End();
	}

void TestNotifyCancel()
//
// Simple test to just call NotifyCancel. Just sends a message
// which goes to window server. Window server ignores the cancel event
// and because of this, no notification to be canceled is started.
//
	{
	TInt r;
	test.Start(_L("Connect to notifier server"));
	RNotifier n;
	r = n.Connect();
	test(r==KErrNone);

	test.Next(_L("Call NotifyCancel"));
	n.NotifyCancel();

	test.Next(_L("Close connection to notifier server"));
	n.Close();

	test.End();
	}


void TestRemovedMethods()
//
// Test deprecated but exported methods
//
	{
	test.Start(_L("Test removed methods of RNotifier class"));
	
	// connect to server
	RNotifier n;
	TInt r = n.Connect();
	test(r==KErrNone);

	test.Next(_L("StartNotifierAndGetResponse (5-params with dllUid)"));
	TBuf8<128> response;
	response.SetMax();
	response.FillZ();
	response.Zero();
	TRequestStatus stat;
	n.StartNotifierAndGetResponse(stat,KUidTestTextNotifier1,KUidTestTextNotifier2,*&KStartData,response);
	User::WaitForRequest(stat);
	test(stat==KErrNotSupported);

	test.Next(_L("StartNotifier (4-params with dllUid)"));
	test(n.StartNotifier(KUidTestTextNotifier2,KUidTestTextNotifier1,*&KStartData,response)==KErrNotSupported);
	
	test.Next(_L("Test removed method LoadNotifiers()"));
	test(n.LoadNotifiers(KUidTestTextNotifier1)==KErrNotSupported);
	
	test.Next(_L("Test removed method UnloadNotifiers()"));
	test(n.UnloadNotifiers(KUidTestTextNotifier1)==KErrNotSupported);

	test.Next(_L("Close connection to notifier server"));
	n.Close();
	test.End();
	}


void DoInteractiveTests()
	{
	TInt r;

	test.Start(_L("Connect to notifier server"));
	RNotifier n;
	r = n.Connect();
	test(r==KErrNone);

	test.Next(_L("Launching simple notifier"));
	_LIT(KLine1,"Line1 - Select Button2");
	_LIT(KLine2,"Line2 - or press enter");
	_LIT(KButton1,"Button1");
	_LIT(KButton2,"Button2");
	TInt button=-1;
	TRequestStatus stat;
	n.Notify(KLine1,KLine2,KButton1,KButton2,button,stat);
	User::WaitForRequest(stat);
	test(button==1);

	test.Next(_L("Close connection to notifier server"));
	n.Close();

	test.End();
	}

#include <e32svr.h>

GLDEF_C TInt E32Main()
    {
	test.Title();

	test.Start(_L("Test V1 notifier"));
	if(UserSvr::IpcV1Available())
		DoTests(KUidTestTextNotifier1,EFalse);
	else
		test.Printf(_L("IPC V1 not supported"));

	test.Next(_L("Test V2 notifier"));
	DoTests(KUidTestTextNotifier2,EFalse);

	test.Next(_L("Test V1 notifier using MNotifierManager"));
	if(UserSvr::IpcV1Available())
		DoTests(KUidTestTextNotifier1,ETrue);
	else
		test.Printf(_L("IPC V1 not supported"));

	test.Next(_L("Test V2 notifier using MNotifierManager"));
	if(UserSvr::IpcV1Available())
		DoTests(KUidTestTextNotifier2,ETrue);
	else
		test.Printf(_L("FIX ME! - Can't run because IPC V1 not supported\n"));

	
	test.Next(_L("TestNotify"));
	TestNotify();
	
	test.Next(_L("TestNotifyCancel"));
	TestNotifyCancel();

	test.Next(_L("Test removed methods"));
	TestRemovedMethods();

	test.Next(_L("Interactive Tests"));
	test.Printf(_L("  Do you want to test notifiers interactively? y/n\n"));
	test.Printf(_L("  Waiting 10 seconds for answer...\n"));
	TRequestStatus keyStat;
	test.Console()->Read(keyStat);
	RTimer timer;
	test(timer.CreateLocal()==KErrNone);
	TRequestStatus timerStat;
	timer.After(timerStat,10*1000000);
	User::WaitForRequest(timerStat,keyStat);
	TInt key = 0;
	if(keyStat!=KRequestPending)
		key = test.Console()->KeyCode();
	timer.Cancel();
	test.Console()->ReadCancel();
	User::WaitForAnyRequest();
	if(key=='y' || key=='Y')
		DoInteractiveTests();
	else
		test.Printf(_L("  Interactive Tests Not Run\n"));

	test.End();
	return(0);
	}
