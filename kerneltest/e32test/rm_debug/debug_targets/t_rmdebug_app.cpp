// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <e32debug.h>
#include <e32property.h> 
#include <u32hal.h>
#include <f32file.h>
#include <e32svr.h>


#include "t_rmdebug_app.h"

IMPORT_C extern void RMDebug_BranchTst2();

LOCAL_C void ParseCommandLineL(TInt32& aFunctionType, TUint& aDelay, TUint& aExtraThreads, TInt32& aCpuNumber)
	{
	// get the length of the command line arguments
	TInt argc = User::CommandLineLength();
	RDebug::Printf(" t_rmdebug_app: ParseCommandLineL argc=%d", argc);

	// allocate a buffer for the command line arguments and extract the data to it
	HBufC* commandLine = HBufC::NewLC(argc);
	TPtr commandLineBuffer = commandLine->Des();
	User::CommandLine(commandLineBuffer);

	// create a lexer and read through the command line
	TLex lex(*commandLine);
	while (!lex.Eos())
		{
		// expecting the first character to be a '-'
		if (lex.Get() == '-')
			{
			TChar arg = lex.Get();
			switch (arg)
				{
				case 'f':
					// the digits following '-f' give the function type
					User::LeaveIfError(lex.Val(aFunctionType));
					RDebug::Printf(" t_rmdebug_app: setting aFunctionType=%d", aFunctionType);
					break;
				case 'd':
					// the digits following '-d' give the delay
					User::LeaveIfError(lex.Val(aDelay));
					RDebug::Printf(" t_rmdebug_app: setting aDelay=%d", aDelay);
					break;
				case 'e':
					// the digits following '-e' give the number of extra threads to launch
					User::LeaveIfError(lex.Val(aExtraThreads));
					RDebug::Printf(" t_rmdebug_app: setting aExtraThreads=%d", aExtraThreads);
					break;

				case 'a':
						// the digits following '-a' gives the cpu on which this thread will execute on
					User::LeaveIfError(lex.Val(aCpuNumber));
					RDebug::Printf(" t_rmdebug_app: CPU Number=%d", aCpuNumber);
					break;

				default:
					// unknown argument so leave
					User::Leave(KErrArgument);
				}
			lex.SkipSpace();
			}
		else
			{
			// unknown argument so leave
			User::Leave(KErrArgument);
			}
		}

	// do clean up
	CleanupStack::PopAndDestroy(commandLine);
	}

typedef void (*TPfun)();

// test function to call corresponding to EPrefetchAbortFunction
void PrefetchAbort()
	{
	TPfun f = NULL;
	f();
	}

// test function to call corresponding to EUserPanicFunction
void UserPanic()
	{
	User::Panic(KUserPanic, KUserPanicCode);
	}

// calls self repeatedly until stack is used up. Slightly convoluted to prevent UREL optimising this out...
TUint32 StackOverFlowFunction(TUint32 aInt=0)
	{
	TUint32 unusedArray[150];
	for(TInt i=0; i<150; i++)
		{
		unusedArray[i] = StackOverFlowFunction(i);
		}
	return unusedArray[0];
	}

void DataAbort()
	{
	TInt* r = (TInt*) 0x1000;
	*r = 0x42;              
	}

void UndefInstruction()
	{
	TUint32 undef = 0xE6000010;
	TPfun f = (TPfun) &undef;
	f();
	}

TInt DataRead()
	{
	TInt* r = (TInt*) 0x1000;
	TInt rr = (TInt)*r;
	//include the following line to ensure that rr doesn't get optimised out
	RDebug::Printf("Shouldn't see this being printed out: %d", rr);

	// Stop compilation warning. Should not get here anyway.
	rr++;
	return rr;
	}

void DataWrite()
	{
	TInt* r = (TInt*) 0x1000;
	*r = 0x42;                
	}

void UserException()
	{
	User::RaiseException(EExcGeneral);
	}

void SpinForeverWithBreakPoint()
	{

    // finding the process t_rmdebug2/t_rmdebug2_oem/t_rmdebug2_oem2
    // we find the process.SID to attach to the property
	_LIT(KThreadWildCard, "t_rmdebug2*");

	TInt err = KErrNone;
	TUid propertySid = KNullUid;
	TFindThread find(KThreadWildCard);
	TFullName name;
	TBool found = EFalse;
	while(find.Next(name)==KErrNone && !found)
		{
		RThread thread;
		err = thread.Open(find);
		if (err == KErrNone)
			{
			RProcess process;
			thread.Process(process);
			TFullName fullname = thread.FullName();
		    //RDebug::Printf("SID Search Match Found Name %lS Process ID%ld Thread Id %ld", &fullname, process.Id().Id(), thread.Id().Id());
			found = ETrue;
			//SID saved so that the property can be attached to
			propertySid = process.SecureId();
			process.Close();
			}
		thread.Close();
	}

    //attach to the property to publish the address of the RMDebug_BranchTst2 with the correct SID value
	RProperty integerProperty;
	err = integerProperty.Attach(propertySid, EMyPropertyInteger, EOwnerThread);
	if(KErrNone != err)
		RDebug::Printf("Error Attach to the property %d", err);

	TInt address = (TInt)&RMDebug_BranchTst2;
	
	// publish the address where the breakpoint would be set
	err = integerProperty.Set(address);
	if(KErrNone != err)
		RDebug::Printf("Error Set of the property %d", err);
	integerProperty.Close();
	
	//open semaphore to signal the fact we have reached the point where we have to set the property
	RSemaphore globsem;
	globsem.OpenGlobal(_L("RMDebugGlobSem"));
	globsem.Signal();
	globsem.Close();

	RProcess thisProcess;
	TFileName thisProcessName = thisProcess.FileName();
	RDebug::Printf("App Process Name %lS process id %ld thread id %ld", &thisProcessName, thisProcess.Id().Id(), RThread().Id().Id());

	TInt i=0;
	RThread::Rendezvous(KErrNone);
	while(i<0xffffffff)
		{
		RMDebug_BranchTst2();
		User::After(10000);
		}
	}

void SpinForever()
	{
	TInt i=0;
	RThread::Rendezvous(KErrNone);
	while(i<0xffffffff)
		{
		User::After(10000);
		}
	}

void NormalExit()
    {
    RDebug::Printf("Target app: NormalExit() function. Returning to MainL" );
    }

void LaunchThreads(TUint aNumber)
	{
	_LIT(KDebugThreadName, "DebugThread");
	const TUint KDebugThreadDefaultHeapSize=0x10000;
	for(TInt i=0; i<aNumber; i++)
		{
		RThread thread;
		RBuf threadName;
		threadName.Create(KDebugThreadName().Length()+10); // the 10 is for appending i to the end of the name
		threadName.Append(KDebugThreadName());
		threadName.AppendNum(i);
		TInt err = thread.Create(threadName, (TThreadFunction)SpinForever, KDefaultStackSize, KDebugThreadDefaultHeapSize, KDebugThreadDefaultHeapSize, NULL);
		if(err != KErrNone)
			{
			RDebug::Printf("Couldn't create thread %d", err);
			threadName.Close();
			thread.Close();
			break;
			}
		thread.SetPriority(EPriorityNormal);
		TRequestStatus status;
		thread.Rendezvous(status);
		thread.Resume();
		User::WaitForRequest(status);
		thread.Close();
		threadName.Close();
		}
	}

void WaitFiveSecondsThenExit(void)
	{
	// wait for 5 seconds
	User::After(5000000);
	}

TInt NumberOfCpus()
	{
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	return r;
	}

TInt SetCpuAffinity(TInt aCpuNumber)
	{
    TInt TestCpuCount = NumberOfCpus();
	RDebug::Printf("SetCpuAffinity --> TestCpuCount = %d\n", TestCpuCount);		
	TUint32 cpu = 0;

	if ((aCpuNumber % TestCpuCount) != 0)
	cpu = (TUint32)(aCpuNumber % TestCpuCount);

	RDebug::Printf("SetCpuAffinity --> Setting cpu %3d\n", cpu);
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny *)cpu, 0);
	return r;
	}

// call the function corresponding to aFunctionType
LOCAL_C void CallFunction(TDebugFunctionType aFunctionType, TUint aDelay, TUint aExtraThreads, TInt32 aCpuNumber)
	{
	// pause for aDelay microseconds
	User::After(aDelay);

	// set cpu on which this thread should execute on
	if (aCpuNumber)
		SetCpuAffinity(aCpuNumber);

	// launch the extra threads
	LaunchThreads(aExtraThreads);

	// call appropriate function
	switch( aFunctionType )
		{
		case EPrefetchAbortFunction:
			PrefetchAbort();
			break;
		case EUserPanicFunction:
			UserPanic();
			break;
		case EStackOverflowFunction:
			StackOverFlowFunction();
			break;
		case EDataAbortFunction:
			DataAbort();
			break;
		case EUndefInstructionFunction:
			UndefInstruction();
			break;
		case EDataReadErrorFunction:
			DataRead();
			break;
		case EDataWriteErrorFunction:
			DataWrite();
			break;
		case EUserExceptionFunction:
			UserException();
			break;
		case EWaitFiveSecondsThenExit:
			WaitFiveSecondsThenExit();
			break;
		case ESpinForever:
			SpinForever();
			break;
		case ESpinForeverWithBreakPoint:
			SpinForeverWithBreakPoint();
			break;
        case ENormalExit:
            NormalExit();
            break;			
		case EDefaultDebugFunction:
		default:
			break;
		}
	}

void PrintHelp()
	{
	RDebug::Printf("Invoke with arguments:\n");
	RDebug::Printf("\t-d<delay>\n\t: delay in microseconds before calling target function\n");
	RDebug::Printf("\t-f<function-number>\n\t: enumerator from TDebugFunctionType representing function to call\n");
	RDebug::Printf("\t-e<number>\n\t: number of extra threads to launch, these threads run endlessly\n");
	}


TInt E32Main()
	{
		
	RDebug::Printf("t_rmdebug_app tid=%d,pid=%d", I64LOW(RThread().Id().Id()), I64LOW(RProcess().Id().Id()) ) ;
	// setup heap checking and clean up trap
	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New();
	RThread().SetPriority(EPriorityNormal);
	RProcess::Rendezvous(KErrNone);

	// read arguments from command line
	TUint delay = 0;
	TInt32 functionTypeAsTInt32 = (TInt32)EDefaultDebugFunction;
	TUint extraThreads = 0;
	TInt32 aCpuNumber = -1;

	TRAPD(err, ParseCommandLineL(functionTypeAsTInt32, delay, extraThreads, aCpuNumber));

	if(KErrNone == err)
		{
		// if the command line arguments were successfully read then call the appropriate function
		CallFunction((TDebugFunctionType)functionTypeAsTInt32, delay, extraThreads, aCpuNumber);
		}

	// perform clean up and return any error which was recorded
	delete cleanup;
	__UHEAP_MARKEND;
	return err;
	}

