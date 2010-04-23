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
// e32test\pipe\t_pipe.cpp
// Overview:
// Test pipe mechanism
// API Information:
// RPipe
// Details:
// - Create various leagal and illegal pipe and verify that pipe
// functionality works as stated in requirements.
// - Create Named and UnNamed Pipes and verify.
// - Test Pipes communication in multiprocess , multithreaded , single process
// single threaded environment.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Refer Pipes design and requirement document.
// 1. SGL.GT0314.202 PREQ1460 Design Doc
// 2. SGL.GT0314.203 PREQ1460 Pipe Functional Specification
// Refer Pipe test specification document.
// 1. SGL.GT0314.601 Pipes_Test_Specifications
// Failures and causes:
// Base Port information:
// MMP File:
// t_pipe.mmp
// 
//

/**
 @STMTestCaseID			KBASE-T_PIPE-0217
 @SYMPREQ				PREQ1460
 @SYMREQ					REQ6141
 @SYMCR					CR0923
 @SYMTestCaseDesc		Pipe functional tests
 @SYMTestPriority		High
 @SYMTestActions			Tests the functionality of the pipe. Success and failure tests are performed.
 @SYMTestExpectedResults	Test should pass
*/

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <e32des8.h>
#include <e32des8_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <e32math.h>
#include <hal.h>

#include "RPipe.h"

LOCAL_D RTest test(_L("t_pipe"));


//if the test is to run under the debugger, uncomment the following line



const TInt KHeapSize=0x2000;



// Test Data
_LIT8(KTestData,"Pipe Data To Be Passed");
_LIT8(KTestData1,"P");
_LIT8(KTestData2,"Pipe Data To Be Passed");
_LIT8(KTestData3,"ipe Data To Be Passed");


// Test Pipe Names
_LIT(KPipe1Name,"TestPipe1");
_LIT(KPipe3Name,"TestPipe3");

//Global semaphore name
_LIT(KSemaphoreName,"Semaphore1");

// Pipename of max pipe length 80 Charecters.
_LIT(KMaxPipeName,"abcdefghijklmnopqrstabcdefghijklmnopqrstabcdefghijklmnopqrstabcdefghijklmnopqrst");
// PipeName of max pipe length plus one ,81 charecters
_LIT(KMaxPipeNamePlusOne,"abcdefghijklmnopqrstabcdefghijklmnopqrstabcdefghijklmnopqrstabcdefghijklmnopqrst1");

// Thread Name Constants
_LIT(KThread2Name, "thread2");
_LIT(KThread3Name, "thread3");
_LIT(KThread4Name, "thread4");
_LIT(KThread5Name, "thread5");
_LIT(KReaderThread, "ReaderThread");
_LIT(KWriterThread, "WriterThread");
_LIT(KThread8Name, "thread8");
_LIT(KThread9Name, "thread9");
_LIT(KThread11Name, "thread11");

// Test Process Name Constants
_LIT(KProcessName, "t_pipe2.exe");


// Following class is used to pass thread handle information to different threads.
class TData
	{
public:
	TData(RPipe* aReadEnd, RPipe *aWriteEnd); 
	TData(RPipe* aReadEnd, RPipe *aWriteEnd, const TDesC8* aPipeData, TInt aIterations=1);
	RPipe* iReadEnd;
	RPipe* iWriteEnd;
	const TDesC8* iPipeData;
	TInt iIterations;
	};

TData::TData(RPipe* aReadEnd , RPipe *aWriteEnd)
	:iReadEnd(aReadEnd),iWriteEnd(aWriteEnd), iPipeData(NULL), iIterations(NULL)
	{}

TData::TData(RPipe* aReadEnd , RPipe *aWriteEnd, const TDesC8* aPipeData, TInt aIterations)
	:iReadEnd(aReadEnd),iWriteEnd(aWriteEnd), iPipeData(aPipeData), iIterations(aIterations)
	{}

/**
A utility class for running functions in other threads/processes
*/
class TTestRemote
	{
public:
	virtual TInt WaitForExitL()=0;
	virtual ~TTestRemote()
		{}

	virtual void Rendezvous(TRequestStatus& aStatus) =0;

protected:
	TTestRemote()
		{}

	static TInt RunFunctor(TAny* aFunctor)
		{
		TFunctor& functor = *(TFunctor*)aFunctor;
		functor();
		return KErrNone;
		}

	TRequestStatus iLogonStatus;
	static TInt iCount;
	};
TInt TTestRemote::iCount=0;

class TTestThread : public TTestRemote
	{
public:
	TTestThread(const TDesC& aName, TThreadFunction aFn, TAny* aData, TBool aAutoResume=ETrue)
		{
		Init(aName, aFn, aData, aAutoResume);
		}

	/**
	Run aFunctor in another thread
	*/
	TTestThread(const TDesC& aName, TFunctor& aFunctor, TBool aAutoResume=ETrue)
		{
		Init(aName, RunFunctor, &aFunctor, aAutoResume);
		}

	~TTestThread()
		{
		//RTest::CloseHandleAndWaitForDestruction(iThread);
		iThread.Close();
		}

	void Resume()
		{
		iThread.Resume();
		}

	/**
	If thread exited normally, return its return code
	Otherwise, leave with exit reason
	*/
	virtual TInt WaitForExitL()
		{
		User::WaitForRequest(iLogonStatus);
		const TInt exitType = iThread.ExitType();
		const TInt exitReason = iThread.ExitReason();

		__ASSERT_ALWAYS(exitType != EExitPending, User::Panic(_L("TTestThread"),0));

		if(exitType != EExitKill)
			User::Leave(exitReason);

		return exitReason;
		}

	virtual void Rendezvous(TRequestStatus& aStatus)
		{
		iThread.Rendezvous(aStatus);
		}

private:
	void Init(const TDesC& aName, TThreadFunction aFn, TAny* aData, TBool aAutoResume)
		{
		TKName name(aName);
		name.AppendFormat(_L("-%d"), iCount++);	
		TInt r=iThread.Create(name, aFn, KDefaultStackSize, KHeapSize, KHeapSize, aData);
		User::LeaveIfError(r);
		
		iThread.Logon(iLogonStatus);
		__ASSERT_ALWAYS(iLogonStatus == KRequestPending, User::Panic(_L("TTestThread"),0));

		if(aAutoResume)
			iThread.Resume();
		}



	RThread iThread;
	};


/**
Non blocking reads, verifying data as expected
*/
TInt TestThread2(TAny* aData)
	{
	RTest test(_L("t_pipe_t2"));

	test.Start(_L("Thread 2"));
	test.Printf(_L("THREAD 2 called by TestMultiThreadNamedPipes And TestMultiThreadUnNamedPipes\n"));
	
	TBuf8<50>		cPipeReadData;
	TInt 			ret,readsize;
	


	TData& data = *(TData *)aData; 					// aData will have pipe handles and size.


	test.Next(_L("PIPE TEST:Thread 2-1 Read 1 byte of data from the pipe : Test for success\n"));
	readsize = 1;
		
	ret = data.iReadEnd->Read(cPipeReadData ,readsize);
	test_Equal(readsize, ret);


	test.Next(_L("PIPE TEST:Thread 2-2 Validate 1 byte received is correct\n"));
	ret = cPipeReadData.Compare(KTestData1);
	test_KErrNone(ret);


	test.Next(_L("PIPE TEST:Thread 2-3 Read remaining data from the pipe\n"));
	readsize = 21;
	ret = data.iReadEnd->Read(cPipeReadData , readsize);
	test_Equal(readsize, ret);

	test.Next(_L("PIPE TEST:Thread 2-4 Validate received data\n"));
	ret = cPipeReadData.Compare(KTestData3);
	test_KErrNone(ret);
	
	test.End();
	test.Close();
	return KErrNone;


}
/****************************************************************************
	This function is used as thread to test Unnamed pipes.
	TestMultiThreadUnNamedPipes() will use this function.
	TestMultiThreadNamedPipes() will use this function.
	@aData 			: Used to pass the pipe and handle its size information.

	Return Value	: TInt

******************************************************************************/
TInt TestThread3(TAny* aData) {

	
	TInt 				ret, aWriteSize;

	TBufC8<50> 			cTestData3(KTestData2); 	// Test Data

	
	
	TData& data = *(TData *)aData; 					// aData will have pipe handles and size.
	RTest test(_L("t_pipe_t3"));

	test.Start(_L("Thread 3"));
	
	test.Printf(_L(" THREAD 3 called by TestMultiThreadNamedPipes And TestMultiThreadUnNamedPipes\n"));
		
	test.Next(_L("PIPE TEST:Thread 3-1 Call Write blocking and write data\n"));
	
	// Call Writeblocking function. Write one byte of data.
		
	aWriteSize = cTestData3.Length();
	ret = data.iWriteEnd->WriteBlocking(cTestData3,aWriteSize);
	test_Equal(aWriteSize, ret);
	
	// Call Writeblocking function. Write aSize bye data.

	// Write data so that pipe get filled.
	test.Next(_L("PIPE TEST:Thread 3-2 Write data till pipe is filled up \n"));
	ret = data.iWriteEnd->WriteBlocking(cTestData3,aWriteSize);
	test_Equal(aWriteSize, ret);
	
	test.End();
	test.Close();
	return KErrNone;
}
/****************************************************************************
	This function is used as thread to test Unnamed pipes.
	TestMultiThreadUnNamedPipes() will use this function.

	@aData 			: Used to pass the pipe and handle its size information.

	Return Value	: TInt

******************************************************************************/
//TRequestStatus			stat1;
TInt TestThread4(TAny* aData) {

	TData& data = *(TData *)aData; 					// aData will have pipe handles and size.
	TRequestStatus			stat1;
	TBuf8<150>				cPipeReadData;
	TInt					ret;
	RTest test(_L("t_pipe_t4"));
	test.Start(_L("Thread 4"));
	
	RSemaphore				sem;					//	Handle to the global semaphore
	ret = sem.OpenGlobal(KSemaphoreName);					// Access to the global semaphore identified by its name.
	test(ret == KErrNone);


	
	test.Printf(_L("Thread 4:Created by TestNotifyMechanismPipes.\n"));
	test.Next(_L("PIPE TEST:Thread 4-1 Register Notify Data available request.\n"));
	data.iReadEnd->NotifyDataAvailable(stat1);
	test_Equal(KRequestPending, stat1.Int());
	sem.Signal(); //signal to say that we have issued notification request
		
	test.Next(_L("PIPE TEST:Thread 4-2 Wait till notified for data. Check for Available.\n"));
	User::WaitForRequest(stat1);
	test ( stat1.Int() == KErrNone);
		
	test.Next(_L("PIPE TEST:Thread 4-3 Read One byte of data from the pipe.\n"));
	sem.Wait();	//wait for signal that 1 byte should be read
	ret = data.iReadEnd->Read(cPipeReadData,1);
	test (ret == 1);
	
	test.Next(_L("PIPE TEST:Thread 4-4 Verify data is correct ?.\n"));
	test (KErrNone == cPipeReadData.Compare(KTestData1));

	test.Next(_L("PIPE TEST:Thread 4-5 Read remaining data from the pipe.\n"));
	ret = data.iReadEnd->Read(cPipeReadData,21);
	test (ret == 21);
	
	
	test.Next(_L("PIPE TEST:Thread 4-6 Verify data is correct ?.\n"));
	test (KErrNone == cPipeReadData.Compare(KTestData3));
	
	sem.Signal();	//signalling to the main thread to continue its operation
	sem.Close();	//closing the handle to the semaphore
	test.End();
	test.Close();

	return KErrNone;

}

/****************************************************************************
	This function is used as thread to test Unnamed pipes.
	TestWaitMechanismPipes() will use this function.

	@aData 			: Used to pass the pipe and handle its size information.

	Return Value	: TInt

******************************************************************************/

TInt TestThread5(TAny* aData) {

	TData& data = *(TData *)aData; 							// aData will have pipe handles and size.
	TRequestStatus			stat1;
	TInt					ret;

	RTest test(_L("t_pipe_t5"));
	
	test.Start(_L("Thread 5"));
	test.Printf(_L("PIPE TEST:Thread 5:Created by TestWaitMechanismPipes.\n"));
	
	test.Next(_L("PIPE TEST:Thread 5-1:Register one more request to wait till open to read. It shall not allow\n"));
	data.iWriteEnd->Wait(KPipe3Name, stat1);
	test (stat1.Int() == KErrInUse);
	data.iWriteEnd->WaitForReader(KPipe3Name, stat1);
	test (stat1.Int() == KErrInUse);


	test.Next(_L("PIPE TEST:Thread 5-2:Open Pipe handle to Read.\n"));
	ret = data.iReadEnd->Open(KPipe3Name,RPipe::EOpenToRead);
	test(ret == KErrNone);
	
	test.End();
	test.Close();
	return KErrNone;
}

/**
The reader thread will wait till there is data in the pipe
and then continuously read till it has read the total length
of the pipe.
*/
TInt ReaderThread(TAny* aData) {
// Read data from Pipe
	RTest test(KReaderThread);
	test.Title();
	test.Start(_L("Reader Thread"));

	TData& data = *(TData *)aData;
	TBuf8<10> pipeReadData;
	
	const TInt sizeToRead = data.iReadEnd->MaxSize(); //attempt to read everything from pipe
	TRequestStatus status(KErrGeneral);
	
	//do read in loop in case thread is notified before pipe is full
	TInt totalDataRead=0;
	do
		{
		data.iReadEnd->NotifyDataAvailable(status);
		test.Printf(_L("notify data request status is %d\n"), status.Int());
		if(status==KRequestPending)
			User::WaitForRequest(status);
		test(status==KErrNone);
		test.Printf(_L("ready to read data\n"), status.Int());

		const TInt sizeRead = data.iReadEnd->Read(pipeReadData, sizeToRead);
		test.Printf(_L("Read %d bytes from pipe\n"), sizeRead);
		test(sizeRead>0);
		totalDataRead+=sizeRead;
		} 
	while(totalDataRead<sizeToRead);

	test(totalDataRead==sizeToRead);
	test.End();
	test.Close();

	return KErrNone;
}

_LIT8(KTestDataNum1 , "12345");
_LIT8(KTestDataNum , "1234567890");



/**
Write into pipe to completely fill it.
*/
TInt WriterThread(TAny* aData)
	{
// Write data to pipe
	RPipe* writeEnd = static_cast<TData*>(aData)->iWriteEnd;
	RTest test(_L("WriterThread"));
	test.Start(_L("WriterThread"));
	writeEnd->Flush(); //make sure pipe is empty
	const TInt sizeToWrite = writeEnd->MaxSize();
	test.Printf(_L("Writing %d bytes in to pipe\n"), sizeToWrite);
	TInt length=writeEnd->WriteBlocking(KTestDataNum1,sizeToWrite);
	test(length==sizeToWrite);
	test.End();
	test.Close();
	return KErrNone;
	}

/**
The FlusherThread waits till the supplied pipe
is full before flushing it. 
*/
TInt FlusherThread(TAny* aData)
	{
	TData& data = *(TData *)aData; 							// aData will have pipe handles and size.
	
	//wait for pipe to fill, then flush
	TRequestStatus status;
	const TInt maxSize=data.iReadEnd->MaxSize();
	do
		{	
		data.iReadEnd->NotifyDataAvailable(status);
		if(status==KRequestPending)
			User::WaitForRequest(status);
		if(status!=KErrNone)
			return status.Int();
		} while(data.iReadEnd->Size()<maxSize);
	data.iReadEnd->Flush();
	return KErrNone;
	}

/****************************************************************************
	This function is used as thread to test Unnamed pipes.
	TestWaitMechanismPipes() will use this function.

	@aData 			: Used to pass the pipe and handle its size information.

	Return Value	: TInt

******************************************************************************/
TInt	CloseFlag;

TInt TestThread9(TAny* aData) {

	TData& data = *(TData *)aData; 							// aData will have pipe handles and size.
	
	if (CloseFlag == 1)
	data.iReadEnd->Close();
	if (CloseFlag == 0)
	data.iWriteEnd->Close();
	
			
	return 0;
	}

/**
The test will create 2 threads running this function. They will
race to placing the blocking read request. The first
will succeed and wait, the second will write to pipe
*/
TInt ReadBlockThenWrite(TAny* aData) {

	TData& data = *(TData *)aData; 							// aData will have pipe handles and size.
	TBuf8<10>		cReadData;
	RTest test(_L("Concurrent blocking read"));
	test.Start(_L("Call blocking read on pipe\n"));

	TInt ret = data.iReadEnd->ReadBlocking(cReadData,5);
	if(ret == KErrNone)
		{
		test_KErrNone(cReadData.Compare(KTestDataNum1));
		}

	if(ret == KErrInUse)
		{
		test.Next(_L("Other thread beat us - write to pipe so it may proceed"));
		TInt write = data.iWriteEnd->Write(KTestDataNum,5);
		test_Equal(5, write);
		}
	
	test.End();
	test.Close();
				
	return ret;
}

/**
The test will create 2 threads running this function. They will
race to placing the blocking write request. The first
will succeed and wait, the second will read from pipe
*/
TInt WriteBlockThenRead(TAny* aData) {

	TData& data = *(TData *)aData; 							// aData will have pipe handles and size.
	TBuf8<10>		cReadData;
	RTest test(_L("Concurrent blocking write"));
	test.Start(_L("test writing blocked pipe\n"));

	TInt ret = data.iWriteEnd->WriteBlocking(KTestDataNum,10);
	if(ret == KErrInUse)
		{
		test.Next(_L("The other thread beat us - read from pipe so it may proceed"));
		TInt read = data.iReadEnd->Read(cReadData,5);
		test_Equal(5, read);
		test_KErrNone(cReadData.Compare(KTestDataNum1));
		}

	test.End();
	test.Close();
				
	return ret;
}

/****************************************************************************
	This function is used as thread to test Unnamed pipes.
	TestWaitMechanismPipes() will use this function.

	@aData 			: Used to pass the pipe and handle its size information.

	Return Value	: TInt

******************************************************************************/

TInt TestThread11(TAny* aData) {

	TData& data = *(TData *)aData; 							// aData will have pipe handles and size.
	TRequestStatus			stat1;
	TBufC<50>				cPipeName(KPipe3Name);			// Descriptor to hold data for Writing.
	TInt					ret;
	RTest test(_L("t_pipe_t11"));

	test.Start(_L("PIPE TEST:Thread 11:Created by TestWaitMechanismPipes.\n"));

	test.Next(_L("PIPE TEST:Thread 11-1:Register one more request to wait till open to read. It shall not allow\n"));
	data.iReadEnd->WaitForWriter(cPipeName, stat1);
	test_Equal(KErrInUse, stat1.Int());


	test.Next(_L("PIPE TEST:Thread 11-2:Open Pipe handle to write.\n"));
	ret = data.iWriteEnd->Open(cPipeName,RPipe::EOpenToWrite);
	test_KErrNone(ret);
	
	test.End();
	test.Close();
	return KErrNone;
}

/****************************************************************************
	This is a function to test Named/UnNamed pipes Performace.
	Check the functionality of following library functions
		-
		-



******************************************************************************/
/**
	- test that WriteBlocking unblocks
		- when data is read
		- whed date is flushed
	- test that notify data available request is completed as data is read
	- test that ReadBlocking unblocks
	- test that data available notification works

*/
void TestBlockingAndNotify() {
		
		TRequestStatus	stat1;
		RPipe		aReader,aWriter;
		TInt		aSize,ret;
		TBufC8<10>	cPipeTestDataNum(KTestDataNum);

		TBuf8<10>	cReadData;
		
		
		aSize = 5;
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess, //
								EOwnerProcess //
							 ); 
		test_KErrNone(ret);

		TData data(	&aReader, &aWriter);

		{
		TTestThread readerThread(KReaderThread, ReaderThread, &data);

		test.Start(_L("Test that WriteBlock unblocks as data is read from pipe\n"));
		ret = aWriter.WriteBlocking(cPipeTestDataNum,10);
		test(ret == 10);
		test(aWriter.Size()==5);
		aWriter.Flush();
		test(aWriter.Size()==0);

		ret = readerThread.WaitForExitL();
		test_KErrNone(ret);
		}

		{
		TTestThread flusherThread(KThread8Name,	FlusherThread, &data);
		test.Next(_L("Test that WriteBlock unblocks as data is flushed from read end of pipe\n"));
		
		ret = aWriter.WriteBlocking(cPipeTestDataNum,10);
		test (ret == 10);
		ret = flusherThread.WaitForExitL();
		test_KErrNone(ret);
		}
		
		test.Next(_L("Test that NotifyDataAvailable request is completed as data is read from pipe\n"));
		
		aWriter.NotifySpaceAvailable(aSize,stat1);
		test(stat1==KRequestPending);
		
		{
		TTestThread readerThread2(KReaderThread, ReaderThread, &data);
		
		User::WaitForRequest(stat1);
		test(stat1==KErrNone);
	
		ret = readerThread2.WaitForExitL();
		test_KErrNone(ret);
		}

		aReader.Flush();

		test.Next(_L("PIPE TEST: Test that ReadBlocking unblocks\n"));
		{
		TTestThread writeThread(KWriterThread, WriterThread, &data);

		ret = aReader.ReadBlocking(cReadData,5);		
		test(ret == 5);

		ret = writeThread.WaitForExitL();
		test_KErrNone(ret);
		}
		
		test.Next(_L("PIPE TEST: Test NotifyDataAvailable\n"));
		aReader.Flush();
		aReader.NotifyDataAvailable(stat1);
		test(stat1==KRequestPending);

		{	
		TTestThread writeThread2(KWriterThread,WriterThread, &data);

		User::WaitForRequest(stat1);		
		test(stat1==KErrNone);

		aReader.Flush();

		ret = writeThread2.WaitForExitL();
		test_KErrNone(ret);
		}	
		test.Next(_L("PIPE TEST: Test reading from pipe closed by the writer\n"));

		CloseFlag = 0; // 0 Close Write Handle	
		//CloseFlag = 1; // 1 Close Read Handle							
		test.Next(_L("PIPE TEST: TestBlockingAndNotify 10.6\n"));
		TTestThread closeThread(KThread9Name, TestThread9, &data);

		ret = closeThread.WaitForExitL();
		test_KErrNone(ret);

		ret = aReader.ReadBlocking(cReadData,5);
		test_Equal(KErrNotReady, ret);
				
		aWriter.Close();
		aReader.Close();
		aSize = 5;
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess, //
								EOwnerProcess //
							 ); 
		test_KErrNone(ret);

		TData data1(&aReader,&aWriter);
				
		
		//CloseFlag = 0; // 0 Close Write Handle	
		CloseFlag = 1; // 1 Close Read Handle							
		test.Printf(_L("PIPE TEST: TestBlockingAndNotify 10.7\n"));
		TTestThread closeThread2(KThread9Name, TestThread9, &data1);
		ret = aWriter.WriteBlocking(cPipeTestDataNum,10);
		test_Equal(KErrNotReady, ret);

		ret = closeThread2.WaitForExitL();
		test_KErrNone(ret);
		
		aWriter.Close();
		aReader.Close();

		test.End();
}


/****************************************************************************
	TestPipesPermissionCheck	:
			This function is used to test Permission and Access related
			Errors of Pipes API.
			APIs tested are Define , Create , Destroy , Read and Write.
			

******************************************************************************/
// Different pipes for different capability , VID values.
_LIT(KPipeName2, "PipeWithNoCap");
_LIT(KPipeName3, "PipeWithNoCapVID");
_LIT(KPipeName4, "PipeWithRWCap");
_LIT(KPipeName5, "PipeWithComDDCap");
_LIT(KPipeName6, "PipeWithRWComDDCap");
_LIT(KPipeName7, "PipeWithRWComDDCapVID");
_LIT(KPipeName8, "PipeWithRWRUCap");

// Different processes with different capability , VID  values.
_LIT(KProcessNoCap, "t_pipe3.exe");
_LIT(KProcessRCap, "t_pipe5.exe");

// My VID and SID
_LIT_VENDOR_ID(MyVID,0x70000001);
void TestPipesPermissionCheck() {

		RProcess			proc;
		TInt 				ret;
			
		// Define TSecurity objects with different capabilities and VID .
		TSecurityPolicy 	NoCapVID(MyVID,ECapability_None,ECapability_None,ECapability_None);
		TSecurityPolicy 	RWCap(ECapabilityReadDeviceData,ECapabilityWriteDeviceData);
		TSecurityPolicy 	ComDDCap(ECapabilityCommDD);
		TSecurityPolicy 	RWComDDCap(ECapabilityReadDeviceData,ECapabilityCommDD,ECapabilityWriteDeviceData);
		TSecurityPolicy 	RWComDDCapVID(MyVID,ECapabilityCommDD,ECapabilityReadDeviceData,ECapabilityWriteDeviceData);
		TSecurityPolicy 	RWRUCap(ECapabilityReadUserData,ECapabilityReadDeviceData,ECapabilityWriteDeviceData);

		// Define Named pipes with No Cap , combination of Cap and VID 
		
		TInt 	aSize = 10;
		ret = RPipe::Define(KPipeName2, aSize);
		test (ret == KErrNone);
		ret = RPipe::Define(KPipeName3, aSize,NoCapVID);
		test (ret == KErrNone);
		ret = RPipe::Define(KPipeName4, aSize,RWCap);
		test (ret == KErrNone);
		ret = RPipe::Define(KPipeName5, aSize,ComDDCap);
		test (ret == KErrNone);
		ret = RPipe::Define(KPipeName6, aSize,RWComDDCap);
		test (ret == KErrNone);
		ret = RPipe::Define(KPipeName7, aSize,RWComDDCapVID);
		test (ret == KErrNone);
		ret = RPipe::Define(KPipeName8, aSize,RWRUCap);
		test (ret == KErrNone);
		
		//Lets see who can use pipes. Check for Permissions and Access
		test.Next(_L("PIPE TEST:8.1	Create Process with No Cap t_pipe3.exe\n"));
		ret = proc.Create	(
								KProcessNoCap,			// Launch t_pipe3.exe process
					 			KNullDesC				// No arguments passed to t_pipe3.exe
					 		);

		if (ret != KErrNone) 
		{
			test.Printf(_L(" ***** t_pipe3.exe could not start ****"));
		}
		test_KErrNone(ret);
		test.Printf(_L("Process Created successfully"));
		
		TRequestStatus procLogon;
		proc.Logon(procLogon);
		test(procLogon==KRequestPending);
		proc.Resume();
		User::WaitForRequest(procLogon);
		proc.Close();
		
		// Lets see what happens with Read Capability.
		test.Next(_L("PIPE TEST:8.2	Create Process with Read-Write-CommDD Cap t_pipe5.exe\n"));
		ret = RPipe::Define(KPipeName2, aSize);
		test_KErrNone(ret);
		ret = RPipe::Define(KPipeName3, aSize,NoCapVID);
		test_KErrNone(ret);
	
		ret = proc.Create	(
								KProcessRCap,			// Launch t_pipe3.exe process
					 			KNullDesC				// No arguments passed to t_pipe3.exe
					 		);

		if (ret != KErrNone) 
		{
			test.Printf(_L(" ***** t_pipe5.exe could not start ****"));
		}
		test_KErrNone(ret);
		test.Printf(_L("Process Created successfully"));
		proc.Logon(procLogon);
		test(procLogon==KRequestPending);
		proc.Resume();
		User::WaitForRequest(procLogon);
		proc.Close();
		
		//the t_pipe5.exe process should destroy most of these
		//but we call destroy again to verify this
		ret = RPipe::Destroy (KPipeName2);
		test_Equal(KErrNotFound, ret);
		ret = RPipe::Destroy (KPipeName3);
		test_KErrNone(ret); //KPipeName3 is not destroyed by the other process.
		ret = RPipe::Destroy (KPipeName4);
		test_Equal(KErrNotFound, ret);
		ret = RPipe::Destroy (KPipeName5);
		test_Equal(KErrNotFound, ret);
		ret = RPipe::Destroy (KPipeName6);
		test_Equal(KErrNotFound, ret);
		ret = RPipe::Destroy (KPipeName7);
		test_KErrNone(ret); //t_pipe5.exe does not have permission to delete
		ret = RPipe::Destroy (KPipeName8);
		test_KErrNone(ret); //t_pipe5.exe does not have permission to delete
				
				

		return;

}

/****************************************************************************

	TestMiscPipes :
	This is a function to test Named/UnNamed pipes for all Misc test cases.
	

******************************************************************************/

void TestMiscPipes() {

_LIT(KInvalidPipeName,"PipeNotExist");
		TInt			ret,aSize;
		RPipe			aReader,aWriter;
		TBufC<50>		cPipeName(KInvalidPipeName);			// Descriptor to hold data for Writing.
		TBufC8<50>		cPipeTestData2(KTestData2);
		
		
		// Try to create unnamed pipe with Negative size
		test.Next(_L("PIPE TEST:9.1 Create Pipe of Negative Size.\n"));
		aSize = -1;
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess, //
								EOwnerProcess //
							 ); 
		test( ret == KErrArgument);
		
		
		// Try to create unnamed pipe with zero size
		test.Next(_L("PIPE TEST:9.2 Create Pipe with of Zero size.\n"));
		aSize = 0;
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess, //
								EOwnerProcess //
							 );
		test( ret == KErrArgument);
		

		// Try to define named pipe with Negative size
		test.Next(_L("PIPE TEST:9.3 Define Pipe of Negative size.\n"));
		aSize = -1;
		ret = RPipe::Define(cPipeName, aSize); 
		test( ret == KErrArgument);
		
		

		// Try to define named pipe with Zero size
		test.Next(_L("PIPE TEST:9.4 Define Pipe of Zero size.\n"));
		aSize = 0;
		ret = RPipe::Define(cPipeName, aSize);
		test( ret == KErrArgument);
		

		// Try to destroy pipe which does not exists
		test.Next(_L("PIPE TEST:9.5 Try to destroy named pipe which do not exist.\n"));
		ret = RPipe::Destroy (cPipeName);
		test (ret == KErrNotFound);

		
		// Try to read from pipe with invalid length data to be read
		RPipe			aReaderUN,aWriterUN;
		TBuf8<150>		cPipeReadData;
		aSize = 10;
		ret = RPipe::Create(	aSize,
								aReaderUN,
								aWriterUN,
								EOwnerProcess, //
								EOwnerProcess //
							 );
		test (ret == KErrNone );

		
		test.Next(_L("PIPE TEST:9.6 Try calling ReadBlocking using write handle and WriteBlocking using Read handle.\n"));
		ret = aWriterUN.ReadBlocking(cPipeReadData, aSize);
		test (ret == KErrAccessDenied);
		ret = aReaderUN.WriteBlocking(cPipeTestData2,aSize);
		test (ret == KErrAccessDenied);
		
		
		
		
		test.Next(_L("PIPE TEST:9.7 Read negative size data from un-named pipe.\n"));
		aSize = -1;
		ret = aReaderUN.Read(cPipeReadData, aSize);
		test( ret == KErrArgument);
		ret = aWriterUN.Write(cPipeTestData2,aSize);
		test( ret == KErrArgument);
		

		
		test.Next(_L("PIPE TEST:9.8 Read/Write zero size data from/to un-named pipe\n"));
		aSize = 0;
		ret = aReaderUN.Read(cPipeReadData, aSize);
		test( ret == KErrNone);

		ret = aWriterUN.Write(cPipeTestData2,aSize);
		test( ret == KErrNone);
		

		test.Next(_L("PIPE TEST:9.9 Call ReadBlocking and WriteBlocking to Read and Write negative size data.\n"));
		// Try to readblocking from pipe with invalid length data to be read
		aSize = -1;
		ret = aReaderUN.ReadBlocking(cPipeReadData, aSize);
		test( ret == KErrArgument);
		ret = aWriterUN.WriteBlocking(cPipeTestData2,aSize);
		test( ret == KErrArgument);

		test.Next(_L("PIPE TEST:9.10 ReadBlocking/WriteBlocking to read/write zero size data.\n"));
		aSize = 0;
		ret = aReaderUN.ReadBlocking(cPipeReadData, aSize);
		test( ret == KErrArgument);
		ret = aWriterUN.WriteBlocking(cPipeTestData2,aSize);
		test( ret == KErrArgument);
		
		
		test.Next(_L("PIPE TEST:9.11 Try calling ReadBlocking and WriteBlocking using un opened RPipe handles.\n"));		
		RPipe		aReaderT,aWriterT;
		ret = aReaderT.ReadBlocking(cPipeReadData, aSize);
		test (ret == KErrBadHandle);
		ret = aWriterT.WriteBlocking(cPipeTestData2,aSize);
		test (ret == KErrBadHandle);
		aReaderUN.Close();
		aWriterUN.Close();
		

		return;
}

/****************************************************************************
	This is a function to test notify mechanism of pipes.
	Check the functionality of following library functions
		- Notify...()


******************************************************************************/
void TestWaitMechanismPipes() {

		RPipe			aReader,aWriter;			// Used to pass to thread.
		RPipe			aWriter2;
		TInt			ret;
		TBufC<50>		cPipeName(KPipe3Name);		// Descriptor to hold data for Writing.
		TInt			aSize;
		TRequestStatus	stat1;

		aSize = 22 * 10; // Sizeof(KTestData) * 10

		ret = RPipe::Define( cPipeName,aSize);
		ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
		test (ret == KErrNone);


		
		test.Next(_L("PIPE TEST:7.1 Try calling Wait function on BadPipe name.\n"));
		
		_LIT(KBadPipeName , "***R$@#$@#%#$%#^.12-.");
		aWriter.Wait(KBadPipeName,stat1);
		test(stat1.Int() == KErrBadName);
		aWriter.WaitForReader(KBadPipeName,stat1);
		test(stat1.Int() == KErrBadName);
		
			
		_LIT(KBadPipeName2 , "");
		aWriter.Wait(KBadPipeName2,stat1);
		test(stat1.Int() == KErrBadName);
		aWriter.WaitForReader(KBadPipeName2,stat1);
		test(stat1.Int() == KErrBadName);
		
		
		
		test.Next(_L("PIPE TEST:7.2 Try calling Wait function on non existing Pipe\n"));
		_LIT(KInvalidPipe , "NotExistingPipe");
		aWriter.Wait(KInvalidPipe,stat1);
		test(stat1.Int() == KErrNotFound);
		aWriter.WaitForReader(KInvalidPipe,stat1);
		test(stat1.Int() == KErrNotFound);
		
		
		test.Next(_L("PIPE TEST:7.3 Try calling Wait function on Pipe name length more than maxlength.\n"));
		aWriter.Wait(KMaxPipeNamePlusOne,stat1);
		test(stat1.Int() == KErrBadName);
		aWriter.WaitForReader(KMaxPipeNamePlusOne,stat1);
		test(stat1.Int() == KErrBadName);
			
	
		test.Next(_L("PIPE TEST:7.4 Try calling Wait function from unopened handle.\n"));
		aWriter2.Wait(cPipeName, stat1);
		test (stat1.Int() == KErrInUse);
		aWriter2.WaitForReader(cPipeName, stat1);
		test (stat1.Int() == KErrInUse);
				
		
		test.Next(_L("PIPE TEST:7.5 Register a valid Wait Request .\n"));
		aWriter.Wait(cPipeName, stat1);

		TData data(	&aReader, &aWriter);

		// Create Thread 5
		// Pass TData object with Write and Read handle both
		TTestThread thread5(KThread5Name, TestThread5, &data);
		User::WaitForRequest(stat1);

		ret = thread5.WaitForExitL();
		test_KErrNone(ret);
		
		test.Next(_L("PIPE TEST:7.6 After Wait finish check the value of status register.\n"));
		test_KErrNone(stat1.Int());
		aWriter.Close();
		aReader.Close();
		ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
		test_KErrNone(ret);
		ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
		test_KErrNone(ret);
		
		test.Next(_L("PIPE TEST:7.7 After Read handle is open , register request to Wait\n"));
		
		aWriter.Wait(cPipeName, stat1);
		test (stat1.Int() == KErrNone);
	
		test.Next(_L("PIPE TEST:7.8 Check for CancelWait.\n"));
		aWriter.Close();
		aReader.Close();
		ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
		TRequestStatus stat2;
		aWriter.Wait(cPipeName, stat1);
		aWriter.Wait(cPipeName, stat2);
		test(stat2.Int() == KErrInUse);
		aWriter.CancelWait();
		test(stat1.Int() == KErrCancel);
		test(stat2.Int() == KErrInUse);
		aWriter.Wait(cPipeName, stat1);
		ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
		test(stat1.Int() == KErrNone);
		
		test.Next(_L("PIPE TEST:7.9 Check Wait and CancelWait from reader end\n"));
		aWriter.Close();
		aReader.Close();
		ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
		aReader.Wait(cPipeName, stat1);
		test (stat1.Int() ==  KErrAccessDenied);
		aReader.CancelWait();

	
		aWriter.Close();
		aReader.Close();
		RPipe::Destroy(cPipeName);
		
		/*****************Newly added tests for CR 1114 - WaitForReader *********/
		
		ret = RPipe::Define( cPipeName,aSize);
		ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
		test (ret == KErrNone);


		test.Next(_L("PIPE TEST:7.10 Register a valid Wait Request .\n"));
		aWriter.WaitForReader(cPipeName, stat1);
		test(stat1==KRequestPending);
						
		// Create Thread 5
		// Pass TData object with Write and Read handle both
		{
		TTestThread thread5_1(KThread5Name, TestThread5, &data);
		User::WaitForRequest(stat1);

		ret = thread5_1.WaitForExitL();
		test_KErrNone(ret);
		}
		
		test.Next(_L("PIPE TEST:7.11 After Wait finish check the value of status register.\n"));
		aWriter.Close();
		aReader.Close();
		ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
		ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
		
		aReader.WaitForReader(cPipeName, stat1);
		test (stat1.Int() ==  KErrAccessDenied);
		
		test.Next(_L("PIPE TEST:7.12 After Read handle is open , register request to Wait\n"));
				
		aWriter.WaitForReader(cPipeName, stat1);
		test (stat1.Int() == KErrNone);
		
		test.Next(_L("PIPE TEST:7.13 Check for CancelWait.\n"));
		aWriter.Close();
		aReader.Close();
		ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
		
		aWriter.WaitForReader(cPipeName, stat1);
		aWriter.WaitForReader(cPipeName, stat2);
		test(stat2.Int() == KErrInUse);
		aWriter.CancelWait();
		test(stat1.Int() == KErrCancel);
		test(stat2.Int() == KErrInUse);
		aWriter.WaitForReader(cPipeName, stat1);
		ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
		test(stat1.Int() == KErrNone);
				
		// Release all the resources.
		aWriter.Close();
		aReader.Close();
		
		// Create Thread 5
		// Pass TData object with Write and Read handle both
		
		test.Next(_L("PIPE TEST:7.14 Register a valid Wait Request .\n"));
		aWriter.WaitForReader(cPipeName, stat1);
						
		{
		TTestThread thread5_2(KThread5Name, TestThread5, &data);
		User::WaitForRequest(stat1);
		
		ret = thread5_2.WaitForExitL();
		test_KErrNone(ret);
		}
		test.Next(_L("PIPE TEST:7.15 After Wait finish close the handles.\n"));
		test (stat1.Int() == KErrNone);
		aWriter.Close();
		aReader.Close();
		// Release all the resources.
		ret = RPipe::Destroy(cPipeName);
		test_KErrNone(ret);
		
		/*****************Newly added tests for CR 1114 - WaitForWriter *********/
		
		ret = RPipe::Define( cPipeName,aSize);
		test_KErrNone(ret);	
		ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
		test (ret == KErrNone);


		// Create Thread 11
		// Pass TData object with Write and Read handle both
		
		test.Next(_L("PIPE TEST:7.16 Register a valid Wait Request .\n"));
		aReader.WaitForWriter(cPipeName, stat1);
		test(stat1==KRequestPending);
			
		{	
		TTestThread thread11(KThread11Name, TestThread11, &data);
		User::WaitForRequest(stat1);

		ret = thread11.WaitForExitL();
		test_KErrNone(ret);
		}

		test.Next(_L("PIPE TEST:7.17 After Wait finish check the value of status register.\n"));
		aWriter.Close();
		aReader.Close();
		ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
		test_KErrNone(ret);	
		ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
		test_KErrNone(ret);	
		
		aWriter.WaitForWriter(cPipeName, stat1);
		test (stat1.Int() ==  KErrAccessDenied);
		
		test.Next(_L("PIPE TEST:7.18 After Write handle is open , register request to Wait\n"));
		
		aReader.WaitForWriter(cPipeName, stat1);
		test (stat1.Int() == KErrNone);
		
		test.Next(_L("PIPE TEST:7.19 Check for CancelWait.\n"));
		aWriter.Close();
		aReader.Close();
		ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
	
		aReader.WaitForWriter(cPipeName, stat1);
		aReader.WaitForWriter(cPipeName, stat2);
		test(stat2.Int() == KErrInUse);
		aReader.CancelWait();
		test(stat1.Int() == KErrCancel);
		test(stat2.Int() == KErrInUse);
		aReader.WaitForWriter(cPipeName, stat1);
		ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
		test(stat1.Int() == KErrNone);
		
		// Release all the resources.
		aWriter.Close();
		aReader.Close();
		
		// Create Thread 11
		// Pass TData object with Write and Read handle both

		test.Next(_L("PIPE TEST:7.20 Register a valid Wait Request .\n"));
		aReader.WaitForWriter(cPipeName, stat1);
				
		{
		TTestThread thread11_2(KThread11Name, TestThread11, &data);
		User::WaitForRequest(stat1);
		
		test.Next(_L("PIPE TEST:7.21 After Wait finish , close the hadles.\n"));
		test (stat1.Int() == KErrNone);
		ret = thread11_2.WaitForExitL();
		test_KErrNone(ret);

		// Release all the resources.
		aWriter.Close();
		aReader.Close();
		}

		ret = RPipe::Destroy(cPipeName);
		test_KErrNone(ret);
		
		/**********************************************************/
		
		// Define the pipe.
		ret = RPipe::Define( cPipeName,aSize);
		test(ret == KErrNone);
		
		// Wait for Writer.
		aReader.WaitForWriter(cPipeName, stat1);
		// Try to open read end again. It should not open because WaitForWriter
		// will has already opened the Read End of the pipe.
		ret = aReader.Open(cPipeName ,RPipe::EOpenToRead );
		test(ret = KErrInUse );
		// Status of the wait request is pending because writer is not opened yet.
		test (stat1.Int() == KRequestPending);
		
		// Wait on Reader.
		aWriter.WaitForReader(cPipeName , stat2);
		// Reader was already opened so status is KErrNone.
		test ( stat2.Int() == KErrNone);
		
		// Try to open Write end. It should not allow because WaitForReader has 
		// already opened the write end of the pipe.
		ret = aWriter.Open(cPipeName ,RPipe::EOpenToWrite);
		test (ret == KErrInUse);
		
		// Check the status of the WaitForWriter  request. It should be KErrNone now.
		test (stat1.Int() == KErrNone);
		
		// Let's check for pipe attributes.
		ret = aReader.MaxSize();
		test (ret = aSize);
		ret = aWriter.MaxSize();
		test (ret = aSize);
		ret = aReader.Size();
		test (ret = aSize);
		ret = aWriter.Size();
		test (ret = aSize);
		
		// Close the Reade handle.
		aReader.Close();
		
		// Put request to wait for Reader.
		aWriter.WaitForReader(cPipeName , stat2);
		// It should be pending.
		test ( stat2.Int() == KRequestPending);
		// Open the reader end of the pipe. It should allow , KErrNone
		ret = aReader.Open(cPipeName ,RPipe::EOpenToRead );
		test ( stat2.Int() == KErrNone);
		test (ret == KErrNone);
		
		aReader.Close();
		aWriter.Close();
		ret=RPipe::Destroy(cPipeName);
		test_KErrNone(ret);
									
		return;


}

/****************************************************************************
	This is a function to test notify mechanism of pipes.
	Check the functionality of following library functions
		- Notify...()


******************************************************************************/
void TestNotifyMechanismPipes() {
		// Test NotifyDataAvailable , NotifySpaceAvailable
		RSemaphore 					globalSem;						// Handle to a global semaphore. Semaphore is used to maintain synchronisation between thread4 and the main thread 
		TInt						ret;							// Return Value variable.
		RPipe						aReader,aWriter;				// Used to pass to thread.
		RPipe						aReader2,aWriter2;				// Used to pass to thread.
		
		TInt						aSize;							// Used to pass to thread.
		const TBufC<50>				cPipeName(KPipe1Name);			// Descriptor to hold data for Writing.

		TRequestStatus				stat1;
		TBuf8<50>					cPipeTestData1(KTestData2);
		
		const TBufC<50>             cSemaphoreName(KSemaphoreName); // Descriptor conataining the name of the global semaphore.

		
		ret = globalSem.CreateGlobal(cSemaphoreName,0);		//create and open a global semaphore with initial count as 0.
		test (ret == KErrNone);
		
		aSize = 22; 											// Size of KTestData * 10

		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess, //
								EOwnerProcess //
							 );

		TData data(	&aReader, &aWriter);

		// Create Thread 4
		// Pass TData object with Write and Read handle both
		test.Next(_L("PIPE TEST:6.1 Call CancelDataAvailable/CancelSpaceAvailable using unopened RPipe handles.\n"));
		ret = aReader2.CancelDataAvailable();
		test ( ret == KErrBadHandle);
		ret = aWriter2.CancelSpaceAvailable();
		test (ret == KErrBadHandle);
		
		test.Next(_L("PIPE TEST:6.2 Call NotifySpaceAvailable with Negative size \n"));
		// Call Notfifyspace
		TInt	tempsize = -1;
		aWriter.NotifySpaceAvailable(tempsize, stat1);
		test ( stat1.Int() == KErrArgument);

		test.Next(_L("PIPE TEST:6.2a Call NotifySpaceAvailable for space larger than pipe.\n"));
		aWriter.NotifySpaceAvailable(aSize+1, stat1);
		test_Equal( KErrArgument, stat1.Int());
		
		
		// Start the thread
		{
		TTestThread thread4(KThread4Name, TestThread4, &data);

																	//	Thread 4
																	// 	Call Notifydata available
																	//	Loop for Available
																	//	Read the data and validate

		test.Next(_L("PIPE TEST:6.3 Write data into the pipe , verify return value.\n"));
		// Write one byte data into the pipe.
		globalSem.Wait(); //wait before writing to ensure that other thread can register Data Available request while pipe is empty. 
		ret = aWriter.Write(cPipeTestData1,cPipeTestData1.Length());
		test (ret == cPipeTestData1.Length());
		
						
		test.Next(_L("PIPE TEST:6.4 Call NotifySpaceAvailable with valid parameters \n"));
		tempsize = 1;
		aWriter.NotifySpaceAvailable(tempsize, stat1);
		test(stat1==KRequestPending);
		globalSem.Signal();	//signal that thread 4 may go ahead and read 1 byte
		
		
		test.Next(_L("PIPE TEST:6.5 Wait till request says AVAILABLE. Available ?\n"));
		User::WaitForRequest(stat1);
		test ( stat1.Int() == KErrNone || stat1.Int() == KErrCompletion);
		
					// Thread 4
					// Notify data available
					// Loop for available
					// Flush the buffer
					// Register two request for Notifydata available
					// Register two request for Notifyspace available
					// Call Notifydata available using Read handle
					// Call Notifydata available using write handle
					// return


		// Check for Error conditions

		// Flush buffer
		globalSem.Wait(); // this is waiting for a signal from thread4
		test.Next(_L("PIPE TEST:6.6 Register Notification for space availability. Allows ?\n"));
		ret = aWriter.Write(cPipeTestData1,cPipeTestData1.Length());
		ret = aWriter.Write(cPipeTestData1,cPipeTestData1.Length());
		// Register two request for Notifyspace available
		
		aWriter.NotifySpaceAvailable(tempsize, stat1);
		test_Equal(KRequestPending, stat1.Int() );
		test.Next(_L("PIPE TEST:6.7 Register One more Notification for space availability. Allows ?\n"));
		TRequestStatus tempstat1;
		aWriter.NotifySpaceAvailable(tempsize, tempstat1);
		test ( tempstat1.Int() == KErrInUse);
	
		test.Next(_L("PIPE TEST:6.7a Cancellation of non-outstanding requests must not disrupt oustanding requests\n"));
		aWriter.CancelWait();
		aWriter.NotifySpaceAvailable(tempsize, tempstat1);
		test_Equal( KErrInUse, tempstat1.Int());

		test.Next(_L("PIPE TEST:6.8 Cancel all the pending Notifications.\n"));
		ret = aWriter.CancelSpaceAvailable();
		test ( ret == KErrNone);
		test_Equal(KErrCancel, stat1.Int() );
		
		test.Next(_L("PIPE TEST:6.9 Try to cancel some more notifications. It should not allow\n"));
		ret = aWriter.CancelSpaceAvailable();
		test ( ret == KErrNone);
		
		test.Next(_L("PIPE TEST:6.10 Register Notification for Data availability. Allows ?\n"));
		// Register two request for Notifydata available
		aWriter.Flush();
		aReader.NotifyDataAvailable(stat1);
			
		test.Next(_L("PIPE TEST:6.11 Register One More Notification for Data availability. Allows ?\n"));
				
		aReader.NotifyDataAvailable(tempstat1);
		test ( ( tempstat1.Int() == KErrInUse) || (tempstat1.Int() == KErrCompletion));
		
		test.Next(_L("PIPE TEST:6.12 Cancel all the pending Notifications for Data.\n"));
		ret = aReader.CancelDataAvailable();
		test ( ( ret == KErrNone) || (ret == KErrNotFound));
		test (stat1.Int() == KErrCancel);
		
		test.Next(_L("PIPE TEST:6.13 Try to cancel some more notifications. It should not allow\n"));
		ret = aReader.CancelDataAvailable();
		test ( ret == KErrNone);

		test.Next(_L("PIPE TEST:6.14 Try to register Data available notification using Write handle. Should not allow.\n"));
		aWriter.NotifyDataAvailable(stat1);
		test ( stat1.Int() == KErrAccessDenied);
		
		test.Next(_L("PIPE TEST:6.15 Try to register Space available notification using Read handle. Should not allow.\n"));
		aReader.NotifySpaceAvailable(tempsize, stat1);
		test ( stat1.Int() == KErrAccessDenied);
		
		test.Next(_L("PIPE TEST:6.16 Try to Cancel Data available notification using Write handle. Should not allow.\n"));
		ret = aWriter.CancelDataAvailable();
		test ( ret == KErrAccessDenied);
		
		test.Next(_L("PIPE TEST:6.17 Try to Cancel Space available notification using Write handle. Should not allow.\n"));
		ret = aReader.CancelSpaceAvailable();
		test ( ret == KErrAccessDenied);

		// Stop the thread and Close the Thread
		ret = thread4.WaitForExitL();
		}
		test_KErrNone(ret);
		// Close all the pipe handles.
		aReader.Close();
		aWriter.Close();
	
	
		
		test.Printf(_L(" TEST NOTIFICATION MECHNISM FOR NAMED PIPES\n"));
		
		aSize = 22; // Size of KTestData
		
		test.Next(_L("PIPE TEST:6.18 Create Named pipe and Open Read/Write Handles.\n"));
		ret = RPipe::Define( cPipeName,aSize);
		test (ret == KErrNone);
		ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
		test (ret == KErrNone);
		ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
		test (ret == KErrNone);
		
		
	
		TData data2(	&aReader, &aWriter);

		// Create Thread 4
		// Pass TData object with Write and Read handle both
		TTestThread thread4a(KThread4Name, TestThread4, &data2);

														//	Thread 4
														// 	Call Notifydata available
														//	Loop for Available
														//	Read the data and validate
		
		// Write one byte data into the pipe.
		test.Next(_L("PIPE TEST:6.19 Write Data and check for return value.\n"));
		globalSem.Wait(); //wait before writing to ensure that other thread can register Data Available request while pipe is empty. 
		ret = aWriter.Write(cPipeTestData1,cPipeTestData1.Length());
		test (ret == cPipeTestData1.Length());

		test.Next(_L("PIPE TEST:6.20 Register Notification for Space Available.\n"));
		// Call Notfifyspace
		tempsize = 1;
		aWriter.NotifySpaceAvailable(tempsize, stat1);
		test(stat1==KRequestPending);
		globalSem.Signal();	//signal that thread 4 may go ahead and read byte
		
		test.Next(_L("PIPE TEST:6.21 Wait till notified for Space Availanle.\n"));
		User::WaitForRequest(stat1);
		test ( stat1.Int() == KErrNone);

														// Thread 4
														// Read one byte of data
														// Thread 4
														// Notify data available
														// Loop for available
														// Flush the buffer
														// Register two request for Notifydata available
														// Register two request for Notifyspace available
														// Call Notifydata available using Read handle
														// Call Notifydata available using write handle
														// return
														
		globalSem.Wait();	//waiting for a signal from thread4.
		test.Next(_L("PIPE TEST:6.22 Notify for Space available.\n"));
		// Register two request for Notifydata available
		ret = aWriter.Write(cPipeTestData1,cPipeTestData1.Length());
		aWriter.NotifySpaceAvailable(tempsize, stat1);
		
		test.Next(_L("PIPE TEST:6.23 Notify one more notifier for Space available.\n"));
		// Register two request for Notifyspace available
		TRequestStatus notifyStatus;
		aWriter.NotifySpaceAvailable(tempsize, notifyStatus);
		test ( notifyStatus.Int() == KErrInUse);
		
		aWriter.Flush();
		aWriter.NotifySpaceAvailable(tempsize, stat1);
		test ( stat1.Int() == KErrNone);
		
		test.Next(_L("PIPE TEST:6.24 Cancel Notify for Space available.\n"));
		ret = aWriter.CancelSpaceAvailable();
		test (( ret == KErrNotFound) ||( ret == KErrNone));
		test.Next(_L("PIPE TEST:6.25 Cancel one more Notify for Space available.\n"));
		ret = aWriter.CancelSpaceAvailable();
		test ( ret == KErrNone);
		
		test.Next(_L("PIPE TEST:6.26 Register Notify for Data available.\n"));
		// Register two request for Notifydata available
		
		aWriter.Flush();
		TRequestStatus stat5,stat6;
		aReader.NotifyDataAvailable(stat5);
			
		test.Next(_L("PIPE TEST:6.27 Register One more Notify for Data available.\n"));
		// Register two request for Notifyspace available
		aReader.NotifyDataAvailable(notifyStatus);
		test ( notifyStatus.Int() == KErrInUse);
		test.Next(_L("PIPE TEST:6.28 Cancel Notify for Data available.\n"));
		ret = aReader.CancelDataAvailable();
		test ( ret == KErrNone);
		test (stat5.Int() == KErrCancel);
		
		test.Next(_L("PIPE TEST:6.29 Cancel One more Notify for Data available.\n"));
		ret = aReader.CancelDataAvailable();
		test ( ret == KErrNone);

		test.Next(_L("PIPE TEST:6.30 Register Notify for Data available using Write handle\n"));
		aWriter.NotifyDataAvailable(stat6);
		test ( stat6.Int() == KErrAccessDenied);
		
		test.Next(_L("PIPE TEST:6.31 Register Notify for Space available using Read handle\n"));
		aReader.NotifySpaceAvailable(tempsize, stat6);
		test ( stat6.Int() == KErrAccessDenied);
		
		test.Next(_L("PIPE TEST:6.32 Cancel Notify for Data available using Write handle\n"));
		ret = aWriter.CancelDataAvailable();
		test ( ret == KErrAccessDenied);
		
		test.Next(_L("PIPE TEST:6.33 Cancel Notify for Space available using Read handle\n"));
		ret = aReader.CancelSpaceAvailable();
		test ( ret == KErrAccessDenied);

	//close the handle to the global semaphore
	
	globalSem.Close();
	// Stop the thread and Close the Thread
		ret = thread4a.WaitForExitL();
		test_KErrNone(ret);
	aReader.Close();
	aWriter.Close();
	RPipe::Destroy(cPipeName);
	

	return;



} // End TestNotifyMechanismPipes()

/****************************************************************************
	This is a function to test Named pipes in Mutli process environment.
	Check the functionality of following library functions
		- Define ()
		-


******************************************************************************/


_LIT8(KTestDataIP, "ABCDEFGHIJ");

void TestMultiProcessNamedPipes() {

_LIT(KPipeName5, "InterProcessPipe1");
	TInt 						ret;								// Return value variable
	RProcess					proc;								// Process Handle
	RPipe 						aWriter;
	RPipe						aWriterUN,aReaderUN;
	const	TBufC<150>			cPipeName1(KPipeName5);

	TBufC8<150>					cPipeWriteData1(KTestDataIP);
	TInt						aSize;
	TRequestStatus				stat1;
	TBuf8<150>					cPipeReadData1;

	aSize = 10;

	// Define Pipe with valid size.

	test.Next(_L("PIPE TEST:5.1 Define Pipe and Create UnNAmed pipe and pass handle to other process.\n"));
	ret = RPipe::Define(cPipeName1, aSize);
	test_KErrNone(ret);	
	ret = proc.Create(
						KProcessName,								// Launch t_pipe2.exe process
					 	KNullDesC									// No arguments passed to t_pipe2.exe
					 );

	if (ret != KErrNone) {
		// Check for process successfully launched
		test.Printf(_L("Error : Could not start the process t_pipe2.exe \n"));
	}
	test_KErrNone(ret);	
	TRequestStatus procLogon;
	proc.Logon(procLogon);
	test(procLogon==KRequestPending);

	aSize = 512;
	ret = RPipe::Create(	aSize,
							aReaderUN,
							aWriterUN,
							EOwnerProcess,//TOwnerType aTypeW
							EOwnerProcess //TOwnerType aTypeW
						);
	test_KErrNone(ret);	
						
	ret = aWriterUN.Write(KTestData,22);
	test (ret == 22);
	ret = aReaderUN.Read(cPipeReadData1,10);
	test ( ret == 10);
	aWriterUN.Close();
	ret = aReaderUN.Read(cPipeReadData1,6);
	test (ret == 6);
	aReaderUN.NotifyDataAvailable(stat1);
	ret = stat1.Int();
	test (ret == KErrNone);
	ret = aReaderUN.ReadBlocking(cPipeReadData1,6);
	test (ret == 6);
	ret = aReaderUN.Read(cPipeReadData1,1);
	test ( ret == KErrNotReady);
	ret = aReaderUN.ReadBlocking(cPipeReadData1,1);
	test ( ret == KErrNotReady);
	aReaderUN.Close();
		
	ret = RPipe::Create(	aSize,
							aReaderUN,
							aWriterUN,
							EOwnerProcess,//TOwnerType aTypeW
							EOwnerProcess //TOwnerType aTypeW
						);
	test_KErrNone(ret);	
	proc.SetParameter(3,aReaderUN);
	aReaderUN.Close();
	ret = aWriterUN.Write(KTestData,22);
	test (ret == 22 );
	
	proc.Resume();

	aSize = 10;
	test.Next(_L("PIPE TEST:5.2 Open Write handle to Pipe. Wait till Read handle Opened\n"));
	ret = aWriter.Open(cPipeName1, RPipe::EOpenToWrite);
	test_KErrNone(ret);	
	aWriter.Wait(cPipeName1,stat1);
	User::WaitForRequest(stat1);
	test (stat1.Int() == KErrNone);
	
	test.Next(_L("PIPE TEST:5.3 Write data to Pipe.\n"));
	ret = aWriter.Write(cPipeWriteData1,cPipeWriteData1.Length());
	test ( ret == cPipeWriteData1.Length());

	test.Next(_L("PIPE TEST:5.4 Wait till Space Available in Pipe.\n"));
	aWriter.NotifySpaceAvailable(aSize,stat1);
	User::WaitForRequest(stat1);
	
	test.Next(_L("PIPE TEST:5.5 Write the data using WriteBlocking call\n"));
	test_Equal(0, aWriter.Size());
	ret = aWriter.WriteBlocking(cPipeWriteData1,cPipeWriteData1.Length());
	test ( ret ==cPipeWriteData1.Length() );

	User::WaitForRequest(procLogon);
	
	aWriter.Close();
	ret=RPipe::Destroy(cPipeName1);
	test_KErrNone(ret);
	
	proc.Close();
	aWriterUN.Close();
	aReaderUN.Close();
	return;
	
} // End TestMultiProcessNamedPipes ()
/****************************************************************************
	This is a function to test Named pipes in Multi threaded environment.
	Check the functionality of following library functions
		- Define ()
		- Read()
		- Write()
		- ReadBlocking()
		- WriteBlocking()


******************************************************************************/
//
// Test defining and opening both ends of pipe
// Attempt to readblock from pipe closed at far end (should fail)
// Attemp writeblock to pipe closed at far end
//
// Do some normal reading and writing between threads
// do some blocking read/write between threads
void TestMultiThreadNamedPipes() {

	TInt					ret;							// Return Value variable.
	RPipe					aReader,aWriter;				// Used to pass to thread.
	TInt					aSize;							// Used to pass to thread.
	const TBufC<50>			cPipeName(KPipe1Name);			// Descriptor to hold data for Writing.
	TInt					aReadSize;
	TBuf8<150>				cPipeReadData;
	TBufC8<50> 				cTestData(KTestData); 			// Test Data




///////////////////////////////////////////////////////////
//  PART : 1											///
//	Test Read and Write Functionality					///
///////////////////////////////////////////////////////////

	test.Next(_L("PIPE TEST:4.1 Define Pipe , Open Read and Write Handle.\n"));
	aSize = 22;
	ret = RPipe::Define( cPipeName,aSize);
	test (ret == KErrNone);
	
	ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
	test (ret == KErrNone);
	
	ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
	test (ret == KErrNone);
	
	aWriter.Close();
	
	test.Next(_L("PIPE TEST:4.2 ReadBlocking: Check for KErrNotReady.\n"));
	aReadSize = 1;
	ret = aReader.ReadBlocking(cPipeReadData,aReadSize);
	test (ret == KErrNotReady);
	
	ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
	test (ret == KErrNone);
	
	
	aReader.Close();
	test.Next(_L("PIPE TEST:4.3 WriteBlocking: Check for KErrNotReady.\n"));
	ret = aWriter.WriteBlocking(cTestData,cTestData.Length());
	test (ret == KErrNotReady);
		
	ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
	test (ret == KErrNone);
	

	TData data(	&aReader, &aWriter);




	// Write data to Pipe using valid handle
	test.Next(_L("PIPE TEST:4.4 Write into the pipe and verify return value.\n"));
	ret = aWriter.Write(cTestData,cTestData.Length());
	test (ret == cTestData.Length() );

	
	// Keep writing the data into pipe till it overflows
	
	test.Next(_L("PIPE TEST:4.5 Write into the pipe till it overflows.\n"));
	ret = aWriter.Write(cTestData,cTestData.Length());
	test (( ret == KErrOverflow)) ;
	
	// Start the thread.
	TTestThread thread2(KThread2Name, TestThread2, &data);
	// Read 1 byte data from pipe
	// Validate data
	// Read aByte size data
	// Validate data
	// Thread 2
	// Read aByte size data
	// User:: After (10000)
	// Keep reading data till zero return
	// return

	// Stop the thread and Close the Thread
	ret = thread2.WaitForExitL();
	test_KErrNone(ret);

	aReader.Close();
	aWriter.Close();
	ret = RPipe::Destroy(cPipeName);
	test_KErrNone(ret);


///////////////////////////////////////////////////////////
//  PART : 2											///
//	Test ReadBlocking and WriteBlocking Functionality	///
///////////////////////////////////////////////////////////

	// Test Read and Write blocking call

	test.Next(_L("PIPE TEST:4.6 Create UnNamed Pipe with valid size.\n"));
	
	aSize = 22;
	ret = RPipe::Define( cPipeName,aSize);
	test (ret == KErrNone);
	
	ret = aReader.Open(cPipeName,RPipe::EOpenToRead);
	test (ret == KErrNone);
	
	ret = aWriter.Open(cPipeName,RPipe::EOpenToWrite);
	test (ret == KErrNone);
	
	// Create TData object to pass Pipe handles.
	TData data2(	&aReader, &aWriter);

	
	// Create Thread ( Note : Thread is in pending state now)
	// Flush the data if any in pipe to make sure its empty.
	aWriter.Flush();
	
	// Start the thread
	TTestThread thread3(KThread3Name, TestThread3, &data2);
											// Thread 3
											// Write one byte of data
											// Write one one byte of data using WriteBlocking call
											// Write aByte size data using Writblocking call

	
	// Call Readblocking function.Read one byte of data.
	test.Next(_L("PIPE TEST:4.7 Flush the buffer and Call ReadBlocking one byte into the pipe.\n"));
		aReadSize = 1;
		ret = aReader.ReadBlocking(cPipeReadData,aReadSize);
		test_Equal(aReadSize, ret);
	
	test.Next(_L("PIPE TEST:4.8 Verify the data received.\n"));
		test ( KErrNone == cPipeReadData.Compare(KTestData1));
		
	
	test.Next(_L("PIPE TEST:4.9 Call ReadBlocking and read complete data from Pipe. As big as Pipe Size\n"));
	// Call Readblocking function.Read aSize bytes of data.
		aReadSize = aSize-aReadSize; //read rest of data
		ret = aReader.ReadBlocking(cPipeReadData,aReadSize);
		test_Equal(aReadSize, ret);
		test ( KErrNone == cPipeReadData.Compare(KTestData3));
		
											// Thread 3
											// Wait for some time
											// Call Readblocking and read 1 byte of data
											// Call Readblocking and Read abyte size data
											// Call flush buffer
											// return
	test.Next(_L("PIPE TEST:4.10 Call ReadBlocking and read complete data from Pipe. As big as Pipe Size\n"));
	// Call Readblocking function.Read aSize bytes of data.
		aReadSize = aSize;
		ret = aReader.ReadBlocking(cPipeReadData,aReadSize);
		test_Equal(aReadSize, ret);
		test ( KErrNone == cPipeReadData.Compare(KTestData2));
		
	// Stop the thread and Close the Thread
	ret = thread3.WaitForExitL();
	test_KErrNone(ret);

	aReader.Close();
	aWriter.Close();
	ret=RPipe::Destroy(cPipeName);
	test_KErrNone(ret);
	return ;

} // End TestMultiThreadNamedPipes ()


/****************************************************************************
	This is a function to test UnNamed pipes in Multi threaded environment.
	Check the functionality of following library functions
		- Create()
		- Read()
		- Write()
		- ReadBlocking()
		- WriteBlocking()


******************************************************************************/
void TestMultiThreadUnNamedPipes() {

	TInt					ret;							// Return Value variable.
	RPipe					aReader,aWriter;				// Used to pass to thread.
	TInt					aSize;							// Used to pass to thread.
	TBuf8<250>				cPipeReadData;
	TInt					aReadSize;
	TBufC8<50> 				cTestData(KTestData); 			// Test Data


///////////////////////////////////////////////////////////
//  PART : 1											///
//	Test Read and Write Functionality					///
///////////////////////////////////////////////////////////
	
	test.Next(_L("PIPE TEST: 3.1 Create Pipe : Check for no erros on Pipe Creation \n"));
	ret = 100;
	aSize = 22; 										// Size of KTestData * 10

	ret = RPipe::Create(		aSize,
								aReader,
								aWriter,
								EOwnerProcess ,//TOwnerType aTypeW
								EOwnerProcess //TOwnerType aTypeW
							 );

	test (ret == KErrNone);

	// Create TData object to pass Pipe handles.
	TData data1(	&aReader, &aWriter);

	// Create Thread ( Note : Thread is in pending state now)

	// Create test data stream.
	test.Next(_L("PIPE TEST: 3.2 Write Function test : Write data into the pipe \n"));
	ret = aWriter.Write(cTestData,cTestData.Length()); 		// Write ""Pipe Data To Be Passed"
	test (ret == cTestData.Length());

	// Keep writing the data into pipe till it overflows
	test.Next(_L("PIPE TEST: 3.3 Write Data till the Pipe returns KErrOverFlow\n"));
	ret = aWriter.Write(cTestData,cTestData.Length());
	test ( ret == KErrOverflow);
	
	TTestThread thread2(KThread2Name, TestThread2, &data1);
	// Thread2
	// Read 1 byte data from pipe
	// Validate data
	// Read aByte size data
	// Validate data
	// Thread 2
	// Read aByte size data
	// User:: After (10000)
	// Keep reading data till zero return
	// return
	
	// Stop the thread , Close it.
	
	ret = thread2.WaitForExitL();
	test_KErrNone(ret);

	aReader.Close();
	aWriter.Close();


///////////////////////////////////////////////////////////
//  PART : 2											///
//	Test ReadBlocking and WriteBlocking Functionality	///
///////////////////////////////////////////////////////////


	aSize = 22; 									// Size of KTestData

	ret = RPipe::Create(		aSize,
								aReader,
								aWriter,
								EOwnerProcess ,			//TOwnerType aTypeW
								EOwnerProcess 			//TOwnerType aTypeW
					   );
	test_KErrNone(ret);	

	// Create TData object to pass Pipe handles.

	TData data2(&aReader, &aWriter);

	// Flush the data if any in pipe to make sure its empty.
	 aWriter.Flush();
	 
	// Start the thread
	TTestThread thread3(KThread3Name, TestThread3, &data2);
						// Thread 3
						// Write one byte of data
						// Write one one byte of data using WriteBlocking call
						// Write aByte size data using Writblocking call


	// Call Readblocking function.Read one byte of data.
	
	aReadSize = 1;
	test.Next(_L("PIPE TEST: 3.4 : Call readblocking and read one byte of data \n"));
	ret = aReader.ReadBlocking(cPipeReadData,aReadSize);
	// Call goes to Thread 3
	test_Equal(aReadSize, ret);
	
	test.Next(_L("PIPE TEST: 3.5 : Validate the data \n"));
	test_Equal(0, cPipeReadData.Compare(KTestData1));
	

	// Call Readblocking function.Read aSize bytes of data.
	test.Next(_L("PIPE TEST: 3.6 : Read remaining data \n"));
	aReadSize = 21;
	ret = aReader.ReadBlocking(cPipeReadData,aReadSize);
	test_Equal(aReadSize, ret);
	test_Equal(0, cPipeReadData.Compare(KTestData3));
	
	
	test.Next(_L("PIPE TEST: 3.7 : Read complete pipe size data \n"));
	aReadSize = 22;
	ret = aReader.ReadBlocking(cPipeReadData,aReadSize);
	test_Equal(aReadSize, ret);
	test_Equal(0, cPipeReadData.Compare(KTestData2));
	
	// Wait for thread to end and Close
	ret = thread3.WaitForExitL();
	test_KErrNone(ret);

	aReader.Close();
	aWriter.Close();

	return ;


}// End TestMultiThreadUnNamedPipes()

/****************************************************************************
	This is a function to test Named pipes in Single threaded environment.
	Check the functionality of following library functions
		- Define ()
		-


******************************************************************************/

void TestSingleThreadNamedPipes()

{


	const TBufC<50> 		cPipeName(KPipe1Name); 		// Descriptor to hold data for Writing.
	TInt 					aSize, ret;
	RPipe 					testPipe1;
	RPipe 					testPipe2;
	RPipe					testPipe3;
	RPipe					testPipe4;
	RPipe					testPipe5;
	const TBufC<100> 		cPipeNameMax(KMaxPipeName);
	const TBufC<100> 		cPipeNameMaxPlusOne(KMaxPipeNamePlusOne);

	_LIT(KBadName , "***?SFSDFWE?*_-");
	_LIT(KBadName2 , "");

	test.Next(_L("PIPE TEST: 2.1 Define Function test : Check for No Error\n"));
		aSize = 10;

		ret = RPipe::Define(cPipeName , aSize);
		test (ret == KErrNone);
	
		ret = RPipe::Destroy (cPipeName);
		test (ret == KErrNone);

	

	test.Next(_L("PIPE TEST: 2.2 Define Function test : Check for Max length \n"));
	
		aSize = 10;

		ret = RPipe::Define(cPipeNameMax , aSize);
		test (ret == KErrNone);
	
		ret = RPipe::Destroy (cPipeNameMax);
		test (ret == KErrNone);

	test.Next(_L("PIPE TEST: 2.3 Define Function test : Check for Max length \n"));
		
		aSize = 10;
		ret = RPipe::Define(cPipeNameMaxPlusOne , aSize);
		test (ret == KErrBadName);
		ret = RPipe::Destroy (cPipeNameMaxPlusOne);
		test (ret == KErrBadName);

	test.Next(_L("PIPE TEST: 2.4 Open Function test : Test Open  \n"));
		aSize = 10;
		ret = RPipe::Define(cPipeName , aSize);
		test_KErrNone(ret);

		ret = testPipe1.Open(KBadName,RPipe::EOpenToRead);
		test (ret == KErrBadName);
		
		ret = testPipe1.Open(KBadName2,RPipe::EOpenToRead);
		test (ret == KErrBadName);
		
		ret = testPipe1.Open(cPipeName,RPipe::EOpenToRead);
		test (ret == KErrNone);
		
		ret = testPipe1.Open(cPipeName,RPipe::EOpenToWrite);
		test (ret == KErrInUse);
		
		
		
	test.Next(_L("PIPE TEST: 2.5 Destroy Function test : Destroy opened pipe error  \n"));
		ret = RPipe::Destroy (cPipeName);
		test (ret == KErrInUse);

	test.Next(_L("PIPE TEST: 2.6 Open Function test : Reopen pipe for reading\n"));
		ret = testPipe2.Open(cPipeName,RPipe::EOpenToRead);
		test (ret == KErrInUse);

	test.Next(_L("PIPE TEST: 2.7 Open Function test : Bad name test  \n"));
		ret = testPipe2.Open(cPipeNameMaxPlusOne,RPipe::EOpenToRead);
		test (ret == KErrBadName);
			

	test.Next(_L("PIPE TEST: 2.8 Open Function test : Write mode test\n"));

		ret = testPipe3.Open(cPipeName,RPipe::EOpenToWrite);
		test (ret == KErrNone);
		
		ret = testPipe3.Open(cPipeName,RPipe::EOpenToRead);
		test (ret == KErrInUse);

	test.Next(_L("PIPE TEST: 2.9 Open Function test : Bad name test  \n"));
	
		ret = testPipe4.Open(cPipeNameMaxPlusOne,RPipe::EOpenToWrite);
		test (ret == KErrBadName);
	

	test.Next(_L("PIPE TEST: 2.10 Open Function test : Reopen for writing  \n"));
		ret = testPipe4.Open(cPipeName,RPipe::EOpenToWrite);
		test (ret == KErrInUse);
		

	// Do we have pipes created ? Close and Destroy them ....!!

	testPipe1.Close();
	testPipe2.Close();
	testPipe3.Close();
	testPipe4.Close();
	ret = RPipe::Destroy (cPipeName);
	test_KErrNone(ret);



	test.Next(_L("PIPE TEST: 2.11a Open Function test : Write But Fail on no Readers mode before pipe defined\n"));
	ret = testPipe1.Open(cPipeName,RPipe::EOpenToWriteNamedPipeButFailOnNoReaders);
	test_Equal(KErrNotFound,ret); 

	test.Next(_L("PIPE TEST: 2.11 Open Function test : Write But Fail on no Readers mode Error test\n"));
		ret = RPipe::Define(cPipeName , aSize);
		test (ret == KErrNone);

		ret = testPipe1.Open(cPipeName,RPipe::EOpenToWriteNamedPipeButFailOnNoReaders);
		test (ret == KErrNotReady); 
		

	test.Next(_L("PIPE TEST: 2.12 Open Function test : Write But Fail on no Readers mode Success test\n"));
		ret = testPipe1.Open(cPipeName,RPipe::EOpenToRead);
		test_KErrNone(ret);
		ret = testPipe2.Open(cPipeName,RPipe::EOpenToWriteNamedPipeButFailOnNoReaders);
		test ( ret == KErrNone);
		

	// Do we have pipes created ? Close and Destroy them ....!!

	testPipe1.Close();
	testPipe2.Close();
	testPipe3.Close();
	testPipe4.Close();
	ret = RPipe::Destroy (cPipeName);
	test_KErrNone(ret);


	
	test.Next(_L("	2.13 Define Function test : Check Incorrect Size\n"));
		aSize = -1;
		ret = RPipe::Define(cPipeName , aSize);
		test (ret == KErrArgument); 
	
	
	test.Next(_L("PIPE TEST: 2.14 Define Function test : Check Incorrect Size\n"));
		aSize = 0x1fffffff;
		ret = RPipe::Define(cPipeName , aSize);
		test (ret == KErrNoMemory); 
	

	test.Next(_L("PIPE TEST: 2.15 Size Function test : Size\n"));
		aSize = 10;
		ret = RPipe::Define(cPipeName , aSize);
		
		ret = testPipe5.MaxSize();
		test (ret == KErrBadHandle);


	test.Next(_L("PIPE TEST: 2.16 Size Function test : Size\n"));
		aSize = 10;
		ret = RPipe::Define(cPipeName , aSize);
		testPipe5.Open(cPipeName, RPipe::EOpenToRead);
		testPipe4.Open(cPipeName, RPipe::EOpenToWrite);
		ret = testPipe5.MaxSize();
		test (ret == aSize);
		ret = testPipe4.MaxSize();
		test (ret == aSize);
		

	/* Close all the pipes and Destroy*/
	testPipe1.Close();
	testPipe2.Close();
	testPipe3.Close();
	testPipe4.Close();
	testPipe5.Close();
	ret = RPipe::Destroy (cPipeName);
	test_KErrNone(ret);

	_LIT(KRedefinePipe , "REDEFINEPIPE");

	test.Next(_L("PIPE TEST: 2.17 Check for Redefining same pipe name \n"));
		ret = RPipe::Define(KRedefinePipe , aSize);
		test_KErrNone(ret);
		ret = RPipe::Define(KRedefinePipe , aSize);
		test (ret == KErrAlreadyExists);
	ret = RPipe::Destroy (KRedefinePipe);
	test_KErrNone(ret);
		
	test.Next(_L("PIPE TEST: 2.18 Open Function test : Bad Pipe name\n"));
		aSize = 10;
		RPipe testPipe6;
		ret = testPipe6.Open(cPipeName, RPipe::EOpenToRead);
		test (ret == KErrNotFound);
		

	const TBufC<50> cPipeNameNull;
	test.Next(_L("PIPE TEST: 2.19 Define Function test : Null Pipe name and Bad Pipe name\n"));
		aSize = 10;
		ret = RPipe::Define(cPipeNameNull , aSize);
		test (ret == KErrBadName);


	ret = RPipe::Define(KBadName , aSize);
		test (ret == KErrBadName);
	ret = RPipe::Destroy(KBadName);
		test (ret == KErrBadName);
		

	ret = RPipe::Define(KBadName2 , aSize);
		test (ret == KErrBadName);
	ret = RPipe::Destroy(KBadName2);
		test (ret == KErrBadName);
		
		
	test.Next(_L("PIPE TEST: 2.20 Destroy a pipe while Write end is open\n"));
	ret = RPipe::Define(cPipeName , aSize);

	testPipe1.Close();
	ret = testPipe1.Open(cPipeName,RPipe::EOpenToRead);
	test (ret == KErrNone);
	testPipe2.Close();
	ret = testPipe2.Open(cPipeName,RPipe::EOpenToWrite);
	test (ret == KErrNone);
	testPipe1.Close();
	ret = RPipe::Destroy (cPipeName);
	test (ret == KErrInUse);
	
	testPipe2.Close();
	ret = RPipe::Destroy (cPipeName);
	test (ret == KErrNone);
	
	testPipe1.Close();
	testPipe2.Close();
	testPipe3.Close();
	testPipe4.Close();
	testPipe5.Close();
	
	
	_LIT(KPipe1Name,"TestPipeA");
	_LIT(KPipe2Name,"TestPipeB");
	_LIT(KPipe3Name,"TestPipeC");
	_LIT(KPipe4Name,"TestPipeD");
	
	test.Next(_L("PIPE TEST: 2.21 Define Four pipes and Destroy in different sequence ( Code Coverage test)\n"));
	ret = RPipe::Define(KPipe1Name , aSize);
	test (ret == KErrNone);
	ret = RPipe::Define(KPipe2Name , aSize);
	test (ret == KErrNone);
	ret = RPipe::Define(KPipe3Name , aSize);
	test (ret == KErrNone);
	ret = RPipe::Define(KPipe4Name , aSize);
	test (ret == KErrNone);
	ret = RPipe::Destroy (KPipe2Name);
	test (ret == KErrNone);
	ret = RPipe::Destroy (KPipe4Name);
	test (ret == KErrNone);
	ret = RPipe::Destroy (KPipe1Name);
	test (ret == KErrNone);
	ret = RPipe::Destroy (KPipe3Name);
	test (ret == KErrNone);
	
	return;

} // End TestSingleThreadNamedPipes()

/****************************************************************************
	This is a function to test UnNamed pipes in Single threaded environment.
	Check the functionality of following library functions
		- Create ()
		-


******************************************************************************/

_LIT8(KTxtDataToSend,"*Test data to send**");

void TestSingleThreadUnNamedPipes() {

	TInt 				ret = 1; 				// Return value test variable.
	TInt 				aSize;
	TBufC8<40>			wData(KTxtDataToSend); 	// Descriptor to hold data for Writing.
	RPipe				aReader,aWriter;
	RPipe				aReader2,aWriter2;


	// Following tests will verify all the APIs in single thread
	// for all the possible return values. 
	// This is to verify different paths of API
	// not for detail functional verification.
	// Create the Pipe and Check for No Errors when Valid parameters are passed.
	
	test.Next(_L("PIPE TEST:1.1 Create Function test : Check for No Error\n"));
		aSize = 10;

		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess //TOwnerType aTypeW
							 );

		test (ret == KErrNone);
		
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess //TOwnerType aTypeW
							 );

		test (ret == KErrInUse);
				
		aWriter.Close();
		aReader.Close();
	

	// How big pipe we can create ?

	test.Next(_L("PIPE TEST:1.2 Create Function test : Check for No Memory\n"));
	
		aSize = 0x1BCDEFFF;

		ret = RPipe::Create(	aSize,
								aReader2,
								aWriter2,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess //TOwnerType aTypeW
							 );

		test (ret == KErrNoMemory);

		aReader2.Close();
		aWriter2.Close();

	
	test.Next(_L("PIPE TEST:1.3 Create Function test : Check for Reopening pipe\n"));
		aSize = 10;

		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );
		test_KErrNone(ret);	

		
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );

		test (ret == KErrInUse);
		
		aReader.Close();
		aWriter.Close();
		


	test.Next(_L("PIPE TEST:1.4 Read/Write Function test : Check for Writing to pipe\n"));

		aSize = 100;
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );
		test_KErrNone(ret);	

		ret=aWriter.Write(wData , wData.Size());
		test (ret == wData.Size() ); 
		
		ret=aWriter.Write(wData , -1);
		test (ret == KErrArgument ); 
		
		
	test.Next(_L("PIPE TEST:1.5 Read/Write Function test : Check for Reading from pipe\n"));


		TBuf8<100> rData ;// Descriptor for reading data from Pipe.
		
		ret=aReader.Read(rData,wData.Size()); // Length of the data to be read from Pipe
		test (ret == wData.Size());// Length of the data read from the Pipe


	test.Next(_L("PIPE TEST:1.6 Read/Write Function test : Validate data received\n"));

		test (KErrNone == rData.Compare(wData));


	test.Next(_L("PIPE TEST:1.7 Read/Write Function test : Check for Reading from pipe\n"));

		ret=aReader.Read(rData,1);
		test (ret == 0);
		{
			
		
		RPipe		aReaderT, aWriterT;
		aSize = 20;
		ret = RPipe::Create(	aSize,
								aReaderT,
								aWriterT,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );
		test_KErrNone(ret);	
							 
		ret=aWriterT.Write(wData ,15);
		test_Equal(15, ret);
		ret = aReaderT.Read(rData,10);
		test_Equal(10, ret);
		ret=aWriterT.Write(wData ,10);
		test_Equal(10, ret);
		ret = aReaderT.Read(rData,20);
		test (ret ==15);
		ret = aReaderT.Read(rData,5);
		test_Equal(0, ret);
		ret = aReaderT.Read(rData,25);
		test_Equal(0, ret);

		aReaderT.Close();
		aWriterT.Close();
				
		}
		
	
	test.Next(_L("PIPE TEST:1.8 Read/Write Function test : Check for Wrong RPipe Handle\n"));


		ret=aWriter.Read(rData,15 );// Length of the data to be read from Pipe
		test (ret == KErrAccessDenied );

	test.Next(_L("PIPE TEST:1.9 Read/Write Function test : Check for Wrong RPipe Handle\n"));

		ret=aReader.Write(rData,rData.Size());
		test (ret == KErrAccessDenied );
		

	test.Next(_L("PIPE TEST:1.10 Read/Write Function test : Check for write overflow\n"));

		ret=aWriter.Write(wData,wData.Size());
		ret=aWriter.Write(wData,wData.Size());
		ret=aWriter.Write(wData,wData.Size());
		ret=aWriter.Write(wData,wData.Size());
		ret=aWriter.Write(wData,wData.Size());
		ret=aWriter.Write(wData,wData.Size());
		test (ret == KErrOverflow );
		
		

	test.Next(_L("PIPE TEST:1.11 MaxSize Function test : MaxSize Check \n"));
		aSize = 10;
		// Just to be on safer side , close pipes if any.
		aReader.Close();
		aWriter.Close();

		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );
		test (ret == KErrNone); // This error condition is not defined yet.
		ret =aReader.MaxSize();
		test (ret == aSize);
		

	test.Next(_L("PIPE TEST:1.12 Size Function test : Size Check \n"));
		rData.Zero();	
		
		ret = aReader.Size();
		test_Equal(0, ret);
			
		_LIT8(KSizeTestData1,"123456789");

		ret = aWriter.Write(KSizeTestData1,9);
		test_Equal(9, ret);
		ret = aReader.Size();
		test_Equal(9, ret);

		ret = aReader.Read(rData,1);
		test_Equal(1, ret);
		ret = rData.Compare(_L8("1"));
		test_KErrNone(ret);
		ret = aReader.Size();
		test_Equal(8, ret);
		
		_LIT8(KSizeTestData2,"ab");
		ret = aWriter.Write(KSizeTestData2,2);
		test_Equal(2, ret);
		ret = aReader.Size();
		test_Equal(10, ret);
		
		ret = aWriter.Write(KSizeTestData2,1);
		test_Equal(KErrOverflow, ret);
		ret = aReader.Size();
		test_Equal(10, ret);
		
		ret = aReader.Read(rData,9);
		test_Equal(9, ret);
		ret = rData.Compare(_L8("23456789a"));
		test_KErrNone(ret);
		ret = aReader.Size();
		test_Equal(1, ret);
		
		ret = aReader.Read(rData,1);
		test_Equal(1, ret);
		ret = rData.Compare(_L8("b"));
		test_KErrNone(ret);
		ret = aReader.Size();
		test_Equal(0, ret);
		
		ret = aWriter.Size();
		test_Equal(0, ret);
		RPipe WrongPipeHandle;
		
		ret = WrongPipeHandle.Size();
		test ( ret == KErrBadHandle);

	test.Next(_L("PIPE TEST:1.13 Size Function test : Size Function call with Wrong handle \n"));

	

		ret = WrongPipeHandle.MaxSize();
		test (ret == KErrBadHandle);
	
	aReader.Close();
	aWriter.Close();
	aReader2.Close();
	aWriter2.Close();
		
	test.Next(_L("PIPE TEST:1.14 Read Function : KErrNotReady \n"));
		aSize = 10;
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );
		test (ret == KErrNone); // This error condition is not defined yet.
		aWriter.Close();
		ret = aReader.Read(rData,aSize);
		test (ret == KErrNotReady);
		ret = aReader.Read(rData,110);
		test (ret == KErrArgument);
			
		
	test.Next(_L("PIPE TEST:1.15 Check Handle Type function \n"));	
		aReader.Close();
		aWriter.Close();
		aSize = 10;
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );
		test_KErrNone(ret);	
							 
		ret = aReader.HandleType();
		test (ret == RPipe::EReadChannel);
		
		ret = aWriter.HandleType();
		test (ret == RPipe::EWriteChannel);
		
		
		aReader.Close();
		ret = aReader.HandleType();
		test (ret == KErrBadHandle);
		
	test.Next(_L("PIPE TEST:1.16 Write Function : KErrNotReady \n"));	
		aSize = 1;
		ret = aWriter.Write(wData,aSize);
		test (ret == KErrNotReady);
	
	test.Next(_L("PIPE TEST:1.17 Write Function : Write data more than size of Descriptor \n"));			
		ret = aWriter.Write(wData,110);
		test (ret == KErrArgument);
		
		
	test.Next(_L("PIPE TEST:1.18 Write Function : KErrCompletion \n"));
		aWriter.Close();
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );
		test_KErrNone(ret);	
		ret = aWriter.Write(wData,wData.Size());
		test (ret == KErrOverflow);
	
	test.Next(_L("PIPE TEST:1.19 Create Function : KErrInUse \n"));
		aReader.Close();
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );
		test (ret == KErrInUse);
		
		aWriter.Close();
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );
		test (ret == KErrNone);
		aWriter.Close();
		ret = RPipe::Create(	aSize,
								aReader,
								aWriter,
								EOwnerProcess,//TOwnerType aTypeW
								EOwnerProcess//TOwnerType aTypeW
							 );
		test (ret == KErrInUse);
		
	
		
	test.Next(_L("PIPE TEST:1.20 Read / Write using un opened handles \n"));	
		RPipe		aReaderT,aWriterT;
		
		ret = aWriterT.Write(wData,wData.Size());
		test (ret == KErrBadHandle);
		ret = aReaderT.Read(rData,aSize);
		test (ret == KErrBadHandle);
		
			
		

	// Close all the pipes and return the resources.
		aReader.Close();
		aWriter.Close();
		aReader2.Close();
		aWriter2.Close();
	return;

} // End TestSingleThreadUnNamedPipes()

/****************************************************************************
	This is a function to test UnNamed pipes and named pipes in Single 
	threaded environment.
	This test ensures that the memory is actually being released when:
	RPipe::Close() is called for unnamed pipes and
	RPipe::Destroy() is called for named pipes.
******************************************************************************/
void TestCreateClosePipe()
	{
	
	//unnamed pipes
	RPipe readHandle, writeHandle;
	const TInt K3MB = 1024*1024*3;
	// The loop is run ten times to ensure that if memory allocated while pipe
	// creation is not deallocated by close,then creation of pipe will fail with
	// KErrNoMemory in the sixth iteration. Default heap size is assumed.
	TInt i;
	test.Next(_L("PIPE TEST:11.1 Create Function test in a loop : Check for No Error\n"));
	for(i=1; i<10; i++) 
    	{
        TInt r = RPipe::Create(K3MB, readHandle, writeHandle,EOwnerProcess, EOwnerProcess);
        test(KErrNone == r);
        readHandle.Close(); 
        writeHandle.Close();
        }
     
    //named pipes
    _LIT(KPipeName, "testPipe");
    // The loop is run ten times to ensure that if memory allocated while pipe
	// creation is not deallocated by destroy,then creation of pipe will fail with
	// KErrNoMemory in the sixth iteration. Default heap size is assumed.
	test.Next(_L("PIPE TEST:11.2 Define Function test in a loop : Check for No Error\n"));
    for(i=1; i<10; i++) 
   	 	{
    	TInt r = RPipe::Define(KPipeName,K3MB);
    	test(KErrNone == r);
    	r = RPipe::Destroy(KPipeName);
    	test(KErrNone == r);
    	}
    
    }// End TestCreateClosePipe()


struct TStressArgs
	{
	TStressArgs(TInt aIter, TInt aSize)
		:iIter(aIter), iSize(aSize)
		{}

	const TInt iIter;
	const TInt iSize;
	};

struct TDefDesArgs: public TStressArgs
	{
	TDefDesArgs(TInt aIter, TInt aSize, const TDesC& aName)
		:TStressArgs(aIter,aSize), iName(aName)
		{}
	const TDesC& iName;
	};

/**
Repeatedly define and destroy named pipe
*/
TInt DefineDestroy(TAny* aArgs)
	{
	const TDefDesArgs args = *static_cast<TDefDesArgs*>(aArgs);
	
	for(TInt i=0; i<args.iIter; i++)
		{
		TInt r = RPipe::Define(args.iName, args.iSize);
		if(r!=KErrNone && r!=KErrAlreadyExists)
			{
			return r;
			}
		r = RPipe::Destroy(args.iName);
		if(r!=KErrNone && r!=KErrInUse && r!=KErrNotFound)
			{
			return r;
			}
		}
	return KErrNone;
	}


/**
The parent thread will try to repeatedly open and close a named pipe
which is being repeatedly created and destroyed by the child thread.
This attempts to catch race conditions.
*/
void TestRapidDefineDestroy()
	{
	const TInt iterations=1000;
	TDefDesArgs args(iterations, 16, KPipe1Name);

	RPipe pipe;

	TTestThread thread(_L("DefineDestroy"), DefineDestroy, &args);
	
	TInt r=KErrNone;

	for(TInt i=0; i<args.iIter; i++)
		{
		r = pipe.Open(args.iName, RPipe::EOpenToWrite);
		if(r!=KErrNone && r !=KErrNotFound)
			{
			test_KErrNone(r);
			}

		pipe.Close();
		}
	r = thread.WaitForExitL();
	test_KErrNone(r);
	}


/**
Write the descriptor specified in to the pipe repeating
as many times as specified by TData::iIterations
*/
TInt WriteThread(TAny* aData)
	{
	TData& data = *static_cast<TData*>(aData);
	
	const TInt iter = data.iIterations;

	TInt write=0;
	for(TInt i=0; i<iter; i++)
		{
		write = data.iWriteEnd->WriteBlocking(*data.iPipeData, data.iPipeData->Size());
		if(write <KErrNone)
			{
			return write;
			}
		}

	return write*iter;
	}

/**
Fill descriptor with random bytes
*/
void FillRandom(TDes8& aDes)
	{
	aDes.Zero();

	while(aDes.Size()+4<=aDes.MaxSize())
		{
		TUint8 rand[4];
		*(TUint32*)rand = Math::Random();

		aDes.Append(rand, 4);
		} 
	}

/**
@param aTotalBytes Bytes transfered in period
@param aTicks number of ticks elapsed in period
@return The rate of the transfer on kilobytes per second
*/
TReal KiloBytesPerSecond(TInt aTotalBytes, TInt aTicks)
	{
	TInt period=0; //period of nanotick in microseconds
	TInt r = HAL::Get(HAL::ENanoTickPeriod, period);
	User::LeaveIfError(r);

	//we use the definition that a kilobytes is 1000 bytes
	TReal result = (aTotalBytes/(aTicks*period/1000));
	return result;
	}


/**
Create a source data buffer of aTotal bytes and fill with random data.
Create a pipe and thread (WriteThread) to write the source buffer
into the pipe.
Read from the pipe into destination buffer
and optionally verify that buffers match

@param aTotal Size of data buffer
@param aPipeData Size of pipe to create
@param aIter number of times to repeat transfer
@param aVerify Confirm that destination buffer matches source buffer
@param aPollRead read pipe by polling instead of using blocking read
@return Total number of ticks elapsed
*/
TInt TestLoopBack(TInt aTotal, TInt aPipeSize, TInt aIter, TBool aVerify=ETrue, TBool aPollRead=EFalse)
	{
	const TInt bufferSize = aTotal;

	RBuf8 sourceBuffer;
	sourceBuffer.CreateL(bufferSize);
	FillRandom(sourceBuffer);
	test_Equal(bufferSize,sourceBuffer.Size());

	const TInt pipeSize=aPipeSize;

	RPipe readEnd, writeEnd;
	TInt r = RPipe::Create(pipeSize, readEnd, writeEnd ,EOwnerProcess, EOwnerProcess);
	test_KErrNone(r);


	const TInt maxIter=aIter;
	TData data(NULL, &writeEnd, &sourceBuffer, maxIter);


	RBuf8 destBuffer;
	destBuffer.CreateL(bufferSize);
	
	RBuf8 tempBuffer;
	tempBuffer.CreateL(bufferSize);

	
	TTestThread writer(_L("LoopBack"), WriteThread, &data);
	const TUint32 startTicks=User::NTickCount();

	for(TInt iter=0; iter<maxIter; iter++)
		{
		TInt remainingData = bufferSize;
		do
			{
				const TInt toRead = Min(pipeSize,remainingData);
				if(aPollRead)
					{
					//an inefficient way to read a pipe!
					r = readEnd.Read(tempBuffer, toRead);
					test_NotNegative(r);
					}
				else
					{
					r = readEnd.ReadBlocking(tempBuffer, toRead );
					test_Equal(toRead, r);
					}
				destBuffer+=tempBuffer;
				tempBuffer.Zero();
				remainingData-=r;
			}
		while(remainingData);

		if(aVerify)
			{
			r = sourceBuffer.Compare(destBuffer);
			test_KErrNone(r);
			}

		destBuffer.Zero();
		}
	const TUint32 endTicks = User::NTickCount();

	r = writer.WaitForExitL();
	test_Equal(bufferSize*maxIter, r);

	const TUint32 ticksElapsed= endTicks - startTicks; 

	sourceBuffer.Close();
	tempBuffer.Close();
	destBuffer.Close();

	readEnd.Close();
	writeEnd.Close();

	return ticksElapsed;
	}

/**
Simple test to confirm that data is reproduced after being fed through a pipe
*/
void TestTransferIntegrity()
	{

	TestLoopBack(128*1024, 128, 1, ETrue);
	TestLoopBack(1024, 1, 1, ETrue);

	//read by constantly polling
	TestLoopBack(128*1024, 1024, 1, ETrue, ETrue);
	}


/**
Enable Writeblocking and Readblocking notifications
without actual reads and writes
*/
class RTestPipe: public RPipe
	{
public:
	void RequestWriteBlocking(TInt aNumByte, TRequestStatus& aStatus)
		{
 		DoRequest(EWriteBlocking, aStatus, &aNumByte);
		}
		
	void RequestReadBlocking(TRequestStatus& aStatus)
		{
		DoRequest(EReadBlocking, aStatus);
		}
	};


/**
A test which will request some type of notification
*/
struct CNotificationTest : public TFunctor
	{
	CNotificationTest(RTestPipe& aPipe)
		:iPipe(aPipe)
		{
		TInt r = iParent.Open(iParent.Id());
		test_KErrNone(r);
		}

	virtual ~CNotificationTest()
		{
		}

	void operator()()
		{
		RunTest();

		//set up rendezvous with parent
		iParent.Rendezvous(iRendezvousStatus);

		//announce we have run test
		RThread::Rendezvous(KErrNone);
		
		//wait untill parent has reached rendezvous
		User::WaitForRequest(iRendezvousStatus);
		}

	virtual CNotificationTest* Clone()=0;

	/**
	If necessary, gets pipe into correct state for the start of test
	*/
	virtual void PreparePipe() =0;
	virtual void RunTest() =0;

	/**
	Cancel the notification
	*/
	virtual void Cancel() =0;

	virtual TInt GetReturn()
		{
		return iStatus.Int();
		}

	RTestPipe& iPipe;
	TRequestStatus iStatus;

	TRequestStatus iRendezvousStatus;
	RThread iParent;
	};



/**
Will request free space notification
*/
struct CSpaceNotificationTest : public CNotificationTest
	{
	typedef void (RTestPipe::*TSpaceNotification) (TInt, TRequestStatus&);

	/**
	@param aPipe Pipe handle to use
	@param TSpaceNotification A pointer for the method to test
	@param aNumBytes Amount of space to request
	*/
	CSpaceNotificationTest(RTestPipe& aPipe, TSpaceNotification aFunc, TInt aNumBytes)
		:CNotificationTest(aPipe), iFn(aFunc), iNumBytes(aNumBytes)
		{}
	
	CNotificationTest* Clone()
		{
		return new CSpaceNotificationTest(*this);
		}

	void RunTest()
		{
		(iPipe.*iFn)(iNumBytes, iStatus);
		}

	//Make sure space notification won't complete immediately
	void PreparePipe()
		{
		TInt freeSpace = iPipe.MaxSize() - iPipe.Size();
		if(freeSpace >= iNumBytes)
			{
			TInt r = iPipe.Write(KTestData, freeSpace);
			test_Equal(freeSpace, r);
			}
		}

	void Cancel()
		{
		iPipe.CancelSpaceAvailable();
		};

	TSpaceNotification iFn; 
	TInt iNumBytes;
	};

struct CDataNotificationTest : public CNotificationTest
	{
	typedef void (RTestPipe::*TDataNotification) (TRequestStatus&);

	CDataNotificationTest(RTestPipe& aPipe, TDataNotification aFunc)
		:CNotificationTest(aPipe), iFn(aFunc)
		{}
	
	CNotificationTest* Clone()
		{
		return new CDataNotificationTest(*this);
		}

	void RunTest()
		{
		(iPipe.*iFn)(iStatus);
		}

	//make sure we start with an empty pipe
	void PreparePipe()
		{
		iPipe.Flush();
		}

	void Cancel()
		{
		iPipe.CancelDataAvailable();
		};

	TDataNotification iFn; 
	};


void ProcessNotificationTests(RPointerArray<CNotificationTest>& aArray)
	{
	const TInt count = aArray.Count();
	for(TInt i=0; i < count; i++)
		{
		for(TInt j=0; j < count; j++)
			{
			//need copies as objects contain request states
			CNotificationTest* testA = aArray[i]->Clone();
			test_NotNull(testA);
			CNotificationTest* testB = aArray[j]->Clone();
			test_NotNull(testB);

			testA->PreparePipe(); testB->PreparePipe();

			TTestThread a(_L("CNotificationTestA"), *testA, EFalse);
			TTestThread b(_L("CNotificationTestB"), *testB, EFalse);

			TRequestStatus rendezvousA, rendezvousB;
			a.Rendezvous(rendezvousA);
			b.Rendezvous(rendezvousB);

			a.Resume();	b.Resume();

			//wait till after threads have made notification request.
			User::WaitForRequest(rendezvousA);
			User::WaitForRequest(rendezvousB);

			TInt retA = testA->GetReturn(); TInt retB = testB->GetReturn();

			//announce that we have read status requests, allowing
			//child threads to terminate
			RThread::Rendezvous(KErrNone);

			a.WaitForExitL();
			b.WaitForExitL();

			TBool oneRequestSupported = ((retA == KRequestPending) && (retB == KErrInUse))
										|| ((retB == KRequestPending) && (retA == KErrInUse));
			TBool bothSupported = (retA == KRequestPending) && (retB == KRequestPending);

			if(!(oneRequestSupported || bothSupported))
				{
				test.Printf(_L("Failure: i=%d, j=%d"), i, j);
				test(EFalse);
				}

			testA->Cancel(); testB->Cancel();
			delete testA;
			delete testB;
			}
		}
	}

/**
Test abillity of pipe channels to handle multiple notification requests
simultaneously.
*/
void TestNotifications()
	{
	RTestPipe readEnd, writeEnd;

	TInt pipeSize = 5;
	TInt r = RPipe::Create(pipeSize, readEnd, writeEnd, EOwnerProcess, EOwnerProcess);
	test_KErrNone(r);

	test.Next(_L("Test write end requests"));
	CSpaceNotificationTest writeBlocking(writeEnd, &RTestPipe::RequestWriteBlocking, pipeSize);
	CSpaceNotificationTest spaceAvailable(writeEnd, &RTestPipe::NotifySpaceAvailable, pipeSize);
	RPointerArray<CNotificationTest> writeEndTests;
	
	writeEndTests.AppendL(&writeBlocking);
	writeEndTests.AppendL(&spaceAvailable);

	for(TInt i=0; i<10; i++)
		{
		ProcessNotificationTests(writeEndTests);
		}
	writeEndTests.Close();

	test.Next(_L("Test read end requests"));
	CDataNotificationTest readBlocking(readEnd, &RTestPipe::RequestReadBlocking);
	CDataNotificationTest dataAvailable(readEnd, &RTestPipe::NotifyDataAvailable);
	RPointerArray<CNotificationTest> readEndTests;

	readEndTests.AppendL(&readBlocking);
	readEndTests.AppendL(&dataAvailable);

	for(TInt j=0; j<10; j++)
		{
		ProcessNotificationTests(readEndTests);
		}

	readEndTests.Close();

	readEnd.Close();
	writeEnd.Close();
	}

LOCAL_C void RunTests(void)
	{
	//  Test UnNamed Pipes in Single Thread
	test.Next(_L("PIPE TEST: 1.Un Named pipes in Single Thread\n"));
	TestSingleThreadUnNamedPipes();

	//	Test Named Pipes in Single Thread
	test.Next(_L("PIPE TEST: 2.Named pipes in Single Thread\n"));
	TestSingleThreadNamedPipes();

	//	Test UnNamed Pipes in MultiThread
	test.Next(_L("PIPE TEST: 3.Un Named pipes in Multi Thread\n"));
	TestMultiThreadUnNamedPipes();

	//	Test Named Pipes in MultiThread
	test.Next(_L("PIPE TEST: 4.Named pipes in Multi Thread\n"));
	TestMultiThreadNamedPipes();

	//	Test Named Pipes in Multi Process
	test.Next(_L("PIPE TEST: 5.Named pipes in Multi Process\n"));
	TestMultiProcessNamedPipes();

	//  Test Notify mechanism
	test.Next(_L("PIPE TEST: 6.Pipes Notify mechanism test\n"));
	TestNotifyMechanismPipes();

	//	Test Wait Mechanism
	test.Next(_L("PIPE TEST: 7.Pipes Wait mechanism test\n"));
	TestWaitMechanismPipes();


	//  Test Pipes performance
	test.Next(_L("PIPE TEST: 8.Pipes Permission test\n"));
	TestPipesPermissionCheck ();

	//	Misc Pipe tests
	test.Next(_L("PIPE TEST: 9.Misc Pipe tests\n"));
	TestMiscPipes();
	
	// Blocking and Notify method tests.
	test.Next(_L("PIPE TEST: 10.Blocking and Notification calls test\n"));
	TestBlockingAndNotify();
	
	//Creation and closure of a large number of Pipes
	test.Next(_L("PIPE TEST: 11. Creation and subsequent closure of a large number of pipes\n"));
	TestCreateClosePipe();

	test.Next(_L("Test concurrent notification requests"));
	TestNotifications();

	test.Next(_L("Repeatedly open a named pipe whilst it's being created and destroyed"));
	TestRapidDefineDestroy();

	test.Next(_L("Check integrity of data after transfer\n"));
	TestTransferIntegrity();

	test.Next(_L("PIPE TEST: Ending test.\n"));
	return;
	}

TInt ParseCommandLine()
	{
	TBuf<20> cmdLine;
	//TInt r= cmdLine.Create(User::CommandLineLength());
	//test_KErrNone(r);
	User::CommandLine(cmdLine);
	TLex lex(cmdLine);

	TPtrC token=lex.NextToken();
	if(token.Length()>0)
		{
		TInt os=token.Match(_L("-n*"));
		if(os==0)
			{
			if(token.Length()>2)
				lex.SkipAndMark(2-lex.TokenLength()); //set mark backwards to after the "-n"
			token.Set(lex.NextToken());
			if(token.Length()==0)
				return KErrArgument;

			TLex valLex(token);
			TInt value;
			TInt r=valLex.Val(value);
			if(r<0)
				return r;
			else
				return value;
			}
		else
			{
			return KErrArgument;
			}
		}
	else
		{	
		const TInt KDefaultRuns=1;
		return KDefaultRuns;
		}
	}

GLDEF_C TInt E32Main()
// Main entry point for the test.
    {
	__UHEAP_MARK;
	TInt		ret = 0;

	test.Start(_L("PIPE TEST: Testing"));
	ret = RPipe::Init();
	if ( ret != KErrNone && ret != KErrAlreadyExists) 
		{
		test.Printf(_L("Fail to load RPIPE driver %d\n"),ret);
		return KErrNone;
		}
	TName pddName(RPipe::Name());
	ret = RPipe::Define (KPipe1Name,10);
	test_KErrNone(ret);
	
	User::FreeLogicalDevice(pddName);
	ret = RPipe::Define (KPipe1Name,10);
	test_Equal(KErrNotFound, ret);
		
	ret = RPipe::Init();
	if ( ret != KErrNone && ret != KErrAlreadyExists) 
		{
		test.Printf(_L("Fail to load RPIPE driver %d\n"),ret);
		return KErrNone;
		}
		

	TInt runs=ParseCommandLine();
	if(runs>=0)
		{
		TBool forever=(runs==0);
		for(TInt i=0; forever||(i<runs); ++i)
			{
			if(i!=0)
				test.Next(_L("Next iteration"));

			test.Start(_L("Starting Run")); //sub nest each test iteration
			__UHEAP_MARK;
			TRAP(ret, RunTests());
			test_KErrNone(ret);
			__UHEAP_MARKEND;
			test.End();
			}
		}
	else
		{
		test.Printf(_L("Usage: t_pipe -n N\n N is number of runs to do. If N=0 run forever"));	
		}
	
	TName pddName2(RPipe::Name());
	ret= User::FreeLogicalDevice(pddName2);
	test_KErrNone(ret);
	test.End();
	test.Close();
	__UHEAP_MARKEND;
	return KErrNone;
 }
 

