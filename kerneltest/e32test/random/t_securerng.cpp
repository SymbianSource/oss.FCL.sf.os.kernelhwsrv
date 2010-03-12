// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
//
// Description:
// e32test\random\t_securerng.cpp
// 
//

//system include
#include <e32test.h>
#include <e32math.h>
#include <e32cmn.h>
#include "../mmu/mmudetect.h"

//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-securerng-2702
//! @SYMTestType				UT
//! @SYMTestCaseDesc			Verifies correct operation of the Secure RNG
//! @SYMPREQ					PREQ211
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1. 	TestRandomNumberGeneration: tests that random data is generated correctly.
//!
//! 	2. 	SecureRNGTestWithMultiThread: tests that random data can be provided to multiple threads simultaneously
//!
//! 	3. 	TestSecureRNGForPanicScenarios: tests that the correct panics are issued for common error conditions
//! 		
//! 
//! @SYMTestExpectedResults
//! 	1.	Properties checked:
//! 		1) checks that requests for random data with a buffer length of zero do not cause an error.
//!         2) checks that requests for a large amount of random data do not cause an error.
//!         3) checks that some random data has been returned (comparison to zero filled buffer).
//!         4) checks that the new Math::RandomL() API either returns some random data or correctly indicates the RNG
//!            as not secure (KErrNotReady).
//! 	
//! 	2. Properties checked:
//! 		5) checks that making requests for random data from multiple threads simultaneously does not cause an error.
//!
//! 	3. Properties checked:
//!         6) checks passing a user non-writable memory address to the random function through a valid pointer results
//!            results in the correct panic.
//!         7) checks passing a null pointer to the random function results in the correct panic.
//!         8) checks passing a non-modifiable descriptor to the random function results in the correct panic.
//---------------------------------------------------------------------------------------------------------------------


// RTest for testing Secure RNG unit
RTest test(_L("Secure RNG Unit Test"));

// Threads required for multi thread testing
RThread SecureRNGTestThread1;
RThread SecureRNGTestThread2;
RThread SecureRNGTestThread3;

// Threads name to identify
_LIT(KSecureRNGTestThread1, "SecureRNGTestThread1");
_LIT(KSecureRNGTestThread2, "SecureRNGTestThread2");
_LIT(KSecureRNGTestThread3, "SecureRNGTestThread3");

//length of the buffer data
const TInt KDataLength = 10;

/*
 *Test Secure RNG with invalid and non-writable and non modifiable descriptor type
 */
TInt PanicFuncForBadDesc(TAny* aDes)
    {
    Math::Random(*(TDes8*)aDes);
    return KErrNone;
    }

void CreatePanicThreads(TAny* aThreadData, TInt aExpectedStatusOfThread)
    {
    RThread panicThread;
    _LIT(KPanicThreadName, "SecureRNGTestPanicThread");
        
    test(panicThread.Create(KPanicThreadName(),PanicFuncForBadDesc,KDefaultStackSize,0x200,0x900,aThreadData) == KErrNone); 
    TRequestStatus status;
    panicThread.Logon(status);
    panicThread.Resume();
    User::WaitForRequest(status);
    test.Printf(_L("Exit Reason %d\r\n"), status.Int());
    test.Printf(_L("Exit Type %d\r\n"),(TInt)panicThread.ExitType());
    //Test for expected result from thread
    test(status.Int()== aExpectedStatusOfThread); //(status of thread = thread Exit Reason)
    test(panicThread.ExitCategory() == _L("KERN-EXEC"));
    test(panicThread.ExitType()==EExitPanic);
    
    CLOSE_AND_WAIT(panicThread);
    }

/*
 * Panic test cases for testing Secure RNG
 */
void TestSecureRNGForPanicScenarios()
    {
    test.Printf(_L("Passing user non-writable memory address to the random function through a valid pointer \n"));
    TPtr8 tptr(KernData(), KDataLength, KDataLength);
    CreatePanicThreads(&tptr, 3);
            
    test.Printf(_L("Passing null pointer to random function \n"));
    tptr.Set(NULL, KDataLength, KDataLength);
    CreatePanicThreads(&tptr, 3);
    
    test.Printf(_L("Passing non-modifiable descriptor to the random function \n"));
    HBufC8* randbuf =HBufC8::New(25);
    TPtr8 ptr = randbuf->Des();
    ptr.SetMax();
    CreatePanicThreads(randbuf, 34);
    delete randbuf;
    }

TInt GenerateRandomNumber(TAny*)
    {
    HBufC8* randbuf = HBufC8::New(3000);
    TPtr8 ptr = randbuf->Des();
    ptr.SetMax();
    for(;;)
        {
        Math::Random(ptr);
        }
    }

/*
 * Test Secure RNG with multi threads requesting for random numbers
 */
void SecureRNGTestWithMultiThread()
    {
    test(SecureRNGTestThread1.Create(KSecureRNGTestThread1(),GenerateRandomNumber,KDefaultStackSize,0x200,0x900,NULL)== KErrNone);
    SecureRNGTestThread1.SetPriority(EPriorityLess);
    test(SecureRNGTestThread2.Create(KSecureRNGTestThread2(),GenerateRandomNumber,KDefaultStackSize,0x200,0x900,NULL)== KErrNone);
    SecureRNGTestThread2.SetPriority(EPriorityLess);
    test(SecureRNGTestThread3.Create(KSecureRNGTestThread3(),GenerateRandomNumber,KDefaultStackSize,0x200,0x900,NULL)== KErrNone);
    SecureRNGTestThread3.SetPriority(EPriorityLess);
    
    SecureRNGTestThread1.Resume();
    SecureRNGTestThread2.Resume();
    SecureRNGTestThread3.Resume();
    
    User::After(30 * 1000 * 1000); //30 seconds
    // After waiting for few seconds, kill the threads now.
    SecureRNGTestThread1.Kill(KErrNone);
    test(SecureRNGTestThread1.ExitType()==EExitKill);
    SecureRNGTestThread2.Kill(KErrNone);
    test(SecureRNGTestThread2.ExitType()==EExitKill);
    SecureRNGTestThread3.Kill(KErrNone);
    test(SecureRNGTestThread3.ExitType()==EExitKill);

	CLOSE_AND_WAIT(SecureRNGTestThread1);
	CLOSE_AND_WAIT(SecureRNGTestThread2);
	CLOSE_AND_WAIT(SecureRNGTestThread3);
    }

const TInt KChunkLength = 2048;
void CheckForRandomNumbers(const TUint8* aRandomNumbers, TInt aLength)
    {
    TBuf8<KChunkLength> buf;
    buf.FillZ();
    TInt bytesToCompare = aLength;
    TInt index = 0;
    while(bytesToCompare > 0)
        {
        // check there is at least some random numbers in every chunk
        TInt newLength = bytesToCompare > KChunkLength ? KChunkLength : bytesToCompare;
        test(memcompare(aRandomNumbers+ (index* KChunkLength), newLength, buf.Ptr(), newLength) != 0);
        index++;
        bytesToCompare  = bytesToCompare - KChunkLength;
        }
    }
/*
 * Functionality test for the Random APIs
 */
//required for testing for large number of random numbers request
const TInt KRandomNumsRequired  = 70000;
void TestRandomNumberGeneration()
    {
    test.Printf(_L(" Request for zero random numbers \n"));
    TBuf8<KDataLength> randomBuffer;
    randomBuffer.SetLength(0);
    TRAPD(error, Math::RandomL(randomBuffer));
    test(error == KErrNone);
    
    test.Printf(_L(" Request for huge random numbers of 70000 bytes in length \n"));
    HBufC8* randbuf =HBufC8::New(KRandomNumsRequired);
    TPtr8 ptr = randbuf->Des();
    ptr.SetMax();
    TRAP(error, Math::RandomL(ptr));
    test(error == KErrNotReady || error == KErrNone);
    //check we have some random numbers atleast in every chunk of large randomnumbers received
    CheckForRandomNumbers(ptr.Ptr(), KRandomNumsRequired);
    delete randbuf;
            
    test.Printf(_L(" Request for 32 bit random number using the new leaving function: Math::RandomL() api \n"));
	for (TInt i=0; i<50; ++i)
		{
		// Try to prove it's working by looking for a nonzero value - 50 32-bit zero values
		// in a row from a random source is extremely unlikely. However, if it's not ready
		// we will get 0 every time as the return value is not set when it leaves, so we
		// give up.
	    TUint32 randomNumber = 0;
	    TRAP(error ,randomNumber = Math::RandomL());
	    test.Printf(_L("The generated four byte random number is %d \n" ), randomNumber);
	    test(error == KErrNotReady || error == KErrNone);
		if (error == KErrNotReady || randomNumber != 0)
			break;
		}
    }

/*
 * Test Secure RNG for functionality test, multi-thread tests and panic test cases
 */
void SecureRNGTest()
    {
    test.Printf(_L("\n Functionality test for RNG \n"));
    TestRandomNumberGeneration();
    
    // Test Secure RNG with multi threads
    test.Printf(_L("Testing Secure RNG with Multithreads requesting for random numbers \n"));
    SecureRNGTestWithMultiThread();
        
    //Panic test cases - check with non-writable descriptor type and null pointer
    test.Printf(_L("\n Panic test cases for Secure RNG \n"));
    TestSecureRNGForPanicScenarios();
    }

/*
Gobal Entry Function
*/
GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("\n Starting Secure RNG Unit tests \n"));
	    
	CTrapCleanup* cleanup=CTrapCleanup::New();
	test(cleanup != NULL);
	
	__KHEAP_MARK;
	__UHEAP_MARK;
	SecureRNGTest();
    __UHEAP_MARKEND;
	__KHEAP_MARKEND;
	
	test.End();
	delete cleanup;
	return KErrNone;
	}
