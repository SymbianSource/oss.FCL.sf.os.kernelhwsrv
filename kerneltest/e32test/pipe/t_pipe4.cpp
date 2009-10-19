// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pipe\t_pipe4.cpp
// This is very similar to User::WaitForRequest() but allows the User
// to specify multiple TRequestStatus objects to wait on. The function
// will return when the first TRequestStatus object completes.
// / Header Files////////
// 
//

/**
 @STMTestCaseID			KBASE-T_PIPE4-0218
 @SYMPREQ				PREQ1460
 @SYMREQ					REQ6142
 @SYMCR					CR0923
 @SYMTestCaseDesc		User::WaitForNRequests functional test
 @SYMTestPriority		High
 @SYMTestActions			Tests the operation of the API User::WaitForNRequests().
 @SYMTestExpectedResults	Test should pass
*/

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32svr.h>
#include <e32des8.h>
#include <e32des8_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <e32std.h>
#include <e32std_private.h>

#include "rpipe.h"

//// Test Objects for All threads////

LOCAL_D RTest test(_L("t_pipe4"));
LOCAL_D RTest test2(_L("t_pipe_t2"));
LOCAL_D RTest test3(_L("t_pipe_t3"));
LOCAL_D RTest test4(_L("t_pipe_t4"));
LOCAL_D RTest test5(_L("t_pipe_t5"));

//// Thread Name Constants //////
_LIT(KThread2Name, "WaitThread");
_LIT(KThread3Name, "NotifyDataThread");
_LIT(KThread4Name, "NotifySpaceThread");
_LIT(KThread5Name, "CancelNotifyThread");

_LIT(KPipe1Name,"TestPipeP");
_LIT(KPipe2Name,"TestPipeQ");
_LIT(KPipe3Name,"TestPipeR");
_LIT(KPipe4Name,"TestPipeS");
_LIT(KPipe5Name,"TestPipeT");


_LIT8(KTestDataNum,"1234567890");



const TInt KHeapSize=0x2000;


// Following class is used to pass thread handle information to different threads.
class TData
	{
public:
	TData(RPipe* aPipe, RPipe *bPipe, TInt aSize);
	RPipe* rPipe;								// Pipe Read handle
	RPipe* wPipe;								// Pipe Write handle
	TInt iSize;


	};

TData::TData(RPipe * aPipe , RPipe *bPipe, TInt aSize) {
	rPipe = aPipe;
	wPipe = bPipe;
	iSize = aSize;

}

////////////////////////////////////////////////////
//// WaitThread : Function opens the Read handle
////			for the pipe passed to it.
////
////			CPipeName --> Pipe Name
////			aData 	  --> Read Write Handle
////
////////////////////////////////////////////////////
TBufC<100> 		cPipeName;

TInt WaitThread(TAny* aData) {

	test2.Start(_L("PIPE TEST:WaitThread Entry"));
	TInt 		ret;

	TData& data = *(TData *)aData; 					// aData will have pipe handles and size.

	ret = data.rPipe->Open(cPipeName,RPipe::EOpenToRead);
	test2 (ret == KErrNone);

	test2.Printf(_L("PIPE TEST:WaitThread Exit\n"));
	test2.End();
	test2.Close();
	return KErrNone;
}

////////////////////////////////////////////////////
//// NotifyDataThread :
////			Function writes data into the pipe.
////
////			aData 	  --> Read Write Handle
////
////////////////////////////////////////////////////

TInt NotifyDataThread(TAny* aData) {

	TBufC8<50> 			cTestData(KTestDataNum); 	// Test Data
	TInt 				ret;

	test3.Start(_L("PIPE TEST:NotifyDataThread Entry"));

	TData& data = *(TData *)aData; 					// aData will have pipe handles and size.

	User::After(10000);

	ret = data.wPipe->Write(cTestData,cTestData.Length());
	test3(ret == cTestData.Length());

	test3.Printf(_L("PIPE TEST:NotifyDataThread Exit\n"));
	test3.End();
	test3.Close();
	return KErrNone;
}

////////////////////////////////////////////////////
//// NotifySpaceThread :
////			Function flush the pipe and makes
////			it free.
////
////			aData 	  --> Read Write Handle
////
////////////////////////////////////////////////////

TInt NotifySpaceThread(TAny* aData) {


	test4.Start(_L("PIPE TEST:NotifySpaceThread Entry"));

	TData& data = *(TData *)aData; 					// aData will have pipe handles and size.

//	User::After(10000000);

	data.rPipe->Flush();

	test4.Printf(_L("PIPE TEST:NotifySpaceThread Exit\n"));
	test4.End();
	test4.Close();
	return KErrNone;
}


////////////////////////////////////////////////////
//// CancelNotifyThread :
////			Function cancels Space available request
////			or Cancels Data available request.
////
////			CancelFlag  --> 1 CancelSpaceAvailable
////							0 for CancelDataAvailable
////
////			aData 	  --> Read Write Handle
////
////////////////////////////////////////////////////

TInt		CancelFlag; // 1 CancelSpaceAvailable ; 0 for CancelDataAvailable
TInt CancelNotifyThread(TAny* aData) {

	test5.Start(_L("PIPE TEST:CancelNotifyThread Entry"));

	TData& data = *(TData *)aData; 					// aData will have pipe handles and size.

	if ( CancelFlag == 1)
		data.wPipe->CancelSpaceAvailable();
	else if ( CancelFlag == 0)
		data.rPipe->CancelDataAvailable();
	else
		test5.Printf(_L("PIPE TEST: *** Illegal Cancel\n"));

	test5.Printf(_L("PIPE TEST:CancelNotifyThread Exit\n"));
	test5.End();
	test5.Close();
	return KErrNone;
}

////////////////////////////////////////////////////
//// Main Thread
////
////
////////////////////////////////////////////////////
/*


1. Create 5 (UN)Pipes.
2. Register NotifyData AVailable on 5 pipes.
3. Create Thread 1
4. Wait for n request.
5. In thread 1 write data into any one pipe. Exit
6. Main should show 1 stat variable updated.

7. Again call Wait for n request.
8. It shall come out as one stat is Available.

9. Close all the pipes. Repeat same for NotifySpace available.

10. Define 5 N-Pipes
11. Open write end and call wait till read end open.
12. Create thread 2
12. Wait for n request
13. In thread 2 , open read end of one of the pipe.
14. Main should show 1 stat variable updated.

15. Again call Wait for n request.
16. It shall come out as one stat is Available.

17. Close all the pipes. Destroy pipes.

18. Create 3 N pipe and 2 UN pipes.
19. Register 2 NDA , 2 NSA , 2 Read end wait.
20. Create thread 3 make any request successful.


*/

LOCAL_C void test_MainThread()
{

		RPipe			aReader1,aWriter1;			// Used to pass to thread.
		RPipe			aReader2,aWriter2;			// Used to pass to thread.
		RPipe			aReader3,aWriter3;			// Used to pass to thread.
		RPipe			aReader4,aWriter4;			// Used to pass to thread.
		RPipe			aReader5,aWriter5;			// Used to pass to thread.

		TInt			ret,aSize;
		RThread			thread2;
		RThread			thread3;
		RThread			thread4;
		RThread			thread5;

		TRequestStatus	stat1;
		TRequestStatus	stat2;
		TRequestStatus	stat3;
		TRequestStatus	stat4;
		TRequestStatus	stat5;

		TRequestStatus	s;

		TRequestStatus	*ReqArray[] =
		{
			&stat1,&stat2,&stat3,&stat4,&stat5
		};
		const TBufC<100> 		cPipeName1(KPipe1Name);
		TBufC8<50> 				cTestData(KTestDataNum); 	// Test Data



		test.Printf(_L("PIPE TEST:Main Thread Entry\n"));

//Test	1:

		aSize = 10;
		// Create 5 Pipes.
		ret = RPipe::Create( aSize,	aReader1,aWriter1,EOwnerProcess, EOwnerProcess);
		test_KErrNone(ret);
		ret = RPipe::Create( aSize,	aReader2,aWriter2,EOwnerProcess, EOwnerProcess);
		test_KErrNone(ret);
		ret = RPipe::Create( aSize,	aReader3,aWriter3,EOwnerProcess, EOwnerProcess);
		test_KErrNone(ret);
		ret = RPipe::Create( aSize,	aReader4,aWriter4,EOwnerProcess, EOwnerProcess);
		test_KErrNone(ret);
		ret = RPipe::Create( aSize,	aReader5,aWriter5,EOwnerProcess, EOwnerProcess);
		test_KErrNone(ret);
		// Register Notification for Data Available for all 5 pipes.
		aReader1.NotifyDataAvailable(stat1);
		aReader2.NotifyDataAvailable(stat2);
		aReader3.NotifyDataAvailable(stat3);
		aReader4.NotifyDataAvailable(stat4);
		aReader5.NotifyDataAvailable(stat5);


		// Pass handles for Pipe# 3
		TData data1(&aReader3,&aWriter3,aSize);

		// Create thread for Writing data into pipe.
		// This will make status variable of respective pipe as AVAILABLE
		ret = thread3.Create(
								KThread3Name, 		// Thread name
								NotifyDataThread,			// Function to be called
								KDefaultStackSize,
								KHeapSize,
								KHeapSize,
								(TAny *)&data1		// Data object containing Pipe information.
							);
		test_KErrNone(ret);
		thread3.Logon(s);
		test_Equal(KRequestPending, s.Int());

		// Start the Thread
		thread3.Resume();

		// Wait till any of the request status is Avaialble
		User::WaitForNRequest(ReqArray,5);

		// As WaitForNRequest returned. This proves test pass.
		test.Printf(_L("PIPE TEST:Test 1: Pass\n"));

		// Cancel all pending requests for other tests.
		aReader1.CancelDataAvailable();
		aReader2.CancelDataAvailable();
		aReader3.CancelDataAvailable();
		aReader4.CancelDataAvailable();
		aReader5.CancelDataAvailable();

		// Close thread.
		User::WaitForRequest(s);
		CLOSE_AND_WAIT(thread3);


//Test	2:

		// Pipes created in Test 1.
		// Write data into all the pipes and Fill it.
		aWriter1.Write(cTestData,cTestData.Length());
		aWriter2.Write(cTestData,cTestData.Length());
		aWriter3.Write(cTestData,cTestData.Length());
		aWriter4.Write(cTestData,cTestData.Length());
		aWriter5.Write(cTestData,cTestData.Length());

		// Register notification for Space availability for all pipes.
			
		aWriter1.NotifySpaceAvailable(aSize,stat1);
		aWriter2.NotifySpaceAvailable(aSize,stat2);
		aWriter3.NotifySpaceAvailable(aSize,stat3);
		aWriter4.NotifySpaceAvailable(aSize,stat4);
		aWriter5.NotifySpaceAvailable(aSize,stat5);


		// Create data object for Pipe# 5.
		TData data2(&aReader5,&aWriter5,aSize);

		// Create thread which will make space available in Pipe.
		//
		ret = thread4.Create(
								KThread4Name, 		// Thread name
								NotifySpaceThread,			// Function to be called
								KDefaultStackSize,
								KHeapSize,
								KHeapSize,
								(TAny *)&data2		// Data object containing Pipe information.
							);
		test_KErrNone(ret);
		thread4.Logon(s);
		test_Equal(KRequestPending, s.Int());

		// Start the thread.
		thread4.Resume();

		// Wait till any of the status variable status change to Avaialble
		User::WaitForNRequest(ReqArray,5);

		test.Printf(_L("PIPE TEST:Test 2: Pass\n"));

		// Cancel all pending requests.
		aWriter1.CancelSpaceAvailable();
		aWriter2.CancelSpaceAvailable();
		aWriter3.CancelSpaceAvailable();
		aWriter4.CancelSpaceAvailable();
		aWriter5.CancelSpaceAvailable();

		// Close the thread.
		User::WaitForRequest(s);
		CLOSE_AND_WAIT(thread4);

//Test	3:
		// Flush all the Pipes.
		// This is needed for NotifyDataAvailable request.
		aReader1.Flush();
		aReader2.Flush();
		aReader3.Flush();
		aReader4.Flush();
		aReader5.Flush();

		// Register NotifyDataAvailable request for all the pipes.
		
		CancelFlag = 0;	// 0 = CanceldataAvailable()
		aReader1.NotifyDataAvailable(stat1);
		aReader2.NotifyDataAvailable(stat2);
		aReader3.NotifyDataAvailable(stat3);
		aReader4.NotifyDataAvailable(stat4);
		aReader5.NotifyDataAvailable(stat5);

		TData data3(&aReader2,&aWriter2,aSize);

		ret = thread5.Create(
								KThread5Name, 		// Thread name
								CancelNotifyThread,			// Function to be called
								KDefaultStackSize,
								KHeapSize,
								KHeapSize,
								(TAny *)&data3		// Data object containing Pipe information.
							);
		test_KErrNone(ret);
		thread5.Logon(s);
		test_Equal(KRequestPending, s.Int());
		thread5.Resume();

		User::WaitForNRequest(ReqArray,5);

		test.Printf(_L("PIPE TEST:Test 3: Pass\n"));
		aReader1.CancelDataAvailable();
		aReader2.CancelDataAvailable();
		aReader3.CancelDataAvailable();
		aReader4.CancelDataAvailable();
		aReader5.CancelDataAvailable();
		User::WaitForRequest(s);
		CLOSE_AND_WAIT(thread5);


//Test	4:
		aReader1.Close();
		aWriter1.Close();
		aReader2.Close();
		aWriter2.Close();
		aReader3.Close();
		aWriter3.Close();
		aReader4.Close();
		aWriter4.Close();
		aReader5.Close();
		aWriter5.Close();

		ret = RPipe::Define(KPipe1Name,aSize);
		test_KErrNone(ret);
		ret = RPipe::Define(KPipe2Name,aSize);
		test_KErrNone(ret);
		ret = RPipe::Define(KPipe3Name,aSize);
		test_KErrNone(ret);
		ret = RPipe::Define(KPipe4Name,aSize);
		test_KErrNone(ret);
		ret = RPipe::Define(KPipe5Name,aSize);
		test_KErrNone(ret);

		aWriter1.Open(KPipe1Name,RPipe::EOpenToWrite);
		aWriter2.Open(KPipe2Name,RPipe::EOpenToWrite);
		aWriter3.Open(KPipe3Name,RPipe::EOpenToWrite);
		aWriter4.Open(KPipe4Name,RPipe::EOpenToWrite);
		aWriter5.Open(KPipe5Name,RPipe::EOpenToWrite);

		aWriter1.Wait(KPipe1Name,stat1);
		aWriter2.Wait(KPipe2Name,stat2);
		aWriter3.Wait(KPipe3Name,stat3);
		aWriter4.Wait(KPipe4Name,stat4);
		aWriter5.Wait(KPipe5Name,stat5);

		const TBufC<100> 		cPipeName2(KPipe2Name);

		TData data4(&aReader2,&aWriter2,aSize);
		cPipeName = cPipeName2;

		ret = thread2.Create(
								KThread2Name, 		// Thread name
								WaitThread,			// Function to be called
								KDefaultStackSize,
								KHeapSize,
								KHeapSize,
								(TAny *)&data4		// Data object containing Pipe information.
							);
		test_KErrNone(ret);
		thread2.Logon(s);
		test_Equal(KRequestPending, s.Int());
		thread2.Resume();
		User::WaitForNRequest(ReqArray,5);
		test.Printf(_L("PIPE TEST:Test 4: Pass\n"));
		aWriter1.CancelWait();
		aWriter2.CancelWait();
		aWriter3.CancelWait();
		aWriter4.CancelWait();
		aWriter5.CancelWait();

		aReader1.Close();		aWriter1.Close();
		aReader2.Close();		aWriter2.Close();
		aReader3.Close();		aWriter3.Close();
		aReader4.Close();		aWriter4.Close();
		aReader5.Close();		aWriter5.Close();

		User::WaitForRequest(s);
		CLOSE_AND_WAIT(thread2);

//Test	5:

		ret =aWriter1.Open(KPipe1Name,RPipe::EOpenToWrite);
		test_KErrNone(ret);
		aReader1.Close();
		//ret =aWriter1.Wait(KPipe1Name,stat1);
		aWriter1.Wait(KPipe1Name,stat1);

		ret =aWriter2.Open(KPipe2Name,RPipe::EOpenToWrite);
		test_KErrNone(ret);
		ret =aReader2.Open(KPipe2Name,RPipe::EOpenToRead);
		test_KErrNone(ret);
		ret =aWriter2.Write(cTestData,cTestData.Length());
		//ret =aWriter2.NotifySpaceAvailable(aSize,stat2);
		aWriter2.NotifySpaceAvailable(aSize,stat2);

		ret = RPipe::Create( aSize,	aReader3,aWriter3,EOwnerProcess, EOwnerProcess);
		test_KErrNone(ret);
		aReader3.Flush();
		//ret =aReader3.NotifyDataAvailable(stat3);
		aReader3.NotifyDataAvailable(stat3);

		ret = RPipe::Create( aSize,	aReader4,aWriter4,EOwnerProcess, EOwnerProcess);
		test_KErrNone(ret);
		ret =aWriter4.Write(cTestData,cTestData.Length());
		//ret =aWriter4.NotifySpaceAvailable(aSize,stat4);
		aWriter4.NotifySpaceAvailable(aSize,stat4);

		ret = RPipe::Create( aSize,	aReader5,aWriter5,EOwnerProcess, EOwnerProcess);
		test_KErrNone(ret);
		aReader5.Flush();
		//ret =aReader5.NotifyDataAvailable(stat5);
		aReader5.NotifyDataAvailable(stat5);

		TData data5(&aReader3,&aWriter3,aSize);
		cPipeName = cPipeName2;

		RThread thread6;

		ret = thread6.Create(
								KThread3Name, 		// Thread name
								NotifyDataThread,			// Function to be called
								KDefaultStackSize,
								KHeapSize,
								KHeapSize,
								(TAny *)&data5		// Data object containing Pipe information.
							);
		test_KErrNone(ret);
		thread6.Logon(s);
		test_Equal(KRequestPending, s.Int());
		thread6.Resume();
		User::WaitForNRequest(ReqArray,5);
		test.Printf(_L("PIPE TEST:Test 5: Pass\n"));

		User::WaitForRequest(s);
		CLOSE_AND_WAIT(thread6);
//Test	6:

		TData data6(&aReader2,&aWriter2,aSize);
		cPipeName = cPipeName2;

		RThread thread7;

		ret = thread7.Create(
								KThread3Name, 		// Thread name
								NotifySpaceThread,			// Function to be called
								KDefaultStackSize,
								KHeapSize,
								KHeapSize,
								(TAny *)&data6		// Data object containing Pipe information.
							);
		test_KErrNone(ret);
		thread7.Logon(s);
		test_Equal(KRequestPending, s.Int());
		thread7.Resume();

		User::WaitForNRequest(ReqArray,5);
		test.Printf(_L("PIPE TEST:Test 6: Pass\n"));


		User::WaitForRequest(s);
		CLOSE_AND_WAIT(thread7);

//Test	7:
		TData data7(&aReader1,&aWriter1,aSize);
		cPipeName = cPipeName1;

		RThread thread8;

		ret = thread8.Create(
								KThread2Name, 		// Thread name
								WaitThread,			// Function to be called
								KDefaultStackSize,
								KHeapSize,
								KHeapSize,
								(TAny *)&data7		// Data object containing Pipe information.
							);
		test_KErrNone(ret);
		thread8.Logon(s);
		test_Equal(KRequestPending, s.Int());
		thread8.Resume();

		User::WaitForNRequest(ReqArray,5);
		test.Printf(_L("PIPE TEST:Test 7: Pass\n"));


		User::WaitForRequest(s);
//		CLOSE_AND_WAIT(thread8);	DOESN'T WORK SINCE PIPE DRIVER KEEPS THE THREAD OPEN
		thread8.Close();

//Test	8:
		TData data8(&aReader4,&aWriter4,aSize);

		RThread thread9;
		
		CancelFlag = 1;	// 1 = CancelSpaceAvailable()

		ret = thread9.Create(
								KThread4Name, 		// Thread name
								CancelNotifyThread,			// Function to be called
								KDefaultStackSize,
								KHeapSize,
								KHeapSize,
								(TAny *)&data8		// Data object containing Pipe information.
							);
		test_KErrNone(ret);
		thread9.Logon(s);
		test_Equal(KRequestPending, s.Int());
		thread9.Resume();

		User::WaitForNRequest(ReqArray,5);
		test.Printf(_L("PIPE TEST:Test 8: Pass\n"));

		User::WaitForRequest(s);
		CLOSE_AND_WAIT(thread9);

//Test	9:
		TData data9(&aReader5,&aWriter5,aSize);

		RThread thread10;

		CancelFlag = 0;	// 0 = CancelDataAvailable()

		ret = thread10.Create(
								KThread5Name, 		// Thread name
								CancelNotifyThread,			// Function to be called
								KDefaultStackSize,
								KHeapSize,
								KHeapSize,
								(TAny *)&data9		// Data object containing Pipe information.
							);
		test_KErrNone(ret);
		thread10.Logon(s);
		test_Equal(KRequestPending, s.Int());
		thread10.Resume();

		
		User::WaitForNRequest(ReqArray,5);
		test.Printf(_L("PIPE TEST:Test 9: Pass\n"));

		User::WaitForRequest(s);
		CLOSE_AND_WAIT(thread10);


		aReader1.Close();		aWriter1.Close();
		aReader2.Close();		aWriter2.Close();
		aReader3.Close();		aWriter3.Close();
		aReader4.Close();		aWriter4.Close();
		aReader5.Close();		aWriter5.Close();
		ret = RPipe::Destroy(KPipe1Name);
		test_KErrNone(ret);
		ret = RPipe::Destroy(KPipe2Name);
		test_KErrNone(ret);
		ret = RPipe::Destroy(KPipe3Name);
		test_KErrNone(ret);
		ret = RPipe::Destroy(KPipe4Name);
		test_KErrNone(ret);
		ret = RPipe::Destroy(KPipe5Name);
		test_KErrNone(ret);
		
		
		test.Printf(_L("PIPE TEST:Main Thread Exit\n"));
		return;



}

////////////////////////////////////////////////////
//// RunTests:
////
////
////////////////////////////////////////////////////
LOCAL_C void RunTests(void)
{

	test.Start(_L("PIPE TEST: Testing WaitForNRequest"));

	test_MainThread();

	test.Printf(_L("PIPE TEST: Ending test.\n"));

	test.End();
	test.Close();
	return;
}


////////////////////////////////////////////////////
//// E32Main : Main entry point to test.
////
////
////////////////////////////////////////////////////


GLDEF_C TInt E32Main()

{
	TInt		ret = 0;

	ret = RPipe::Init();
	
	if (ret != KErrNone && ret != KErrAlreadyExists)
	{
	test.Printf(_L(" t_pipe4.exe PIPE TEST: Error loading driver %d\n"),ret);
	test.Printf(_L(" Exiting t_pipe4.exe\n"));

	return KErrNone;

	}
	RunTests();

	TName pddName1(RPipe::Name());
	
	ret= User::FreeLogicalDevice(pddName1);
	
	return KErrNone;
	
}
