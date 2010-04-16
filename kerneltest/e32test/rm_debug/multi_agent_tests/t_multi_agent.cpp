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
// Tests the run mode debug device component by launching multiple targets 
// on different CPUs. On a single core the targets run on the same CPU.  
//

#include <e32base.h>
#include <e32property.h>
#include <hal.h>
#include <e32test.h>
#include <e32def.h>

#include "t_rmdebug_app.h"
#include "t_multi_agent.h"
#include "t_agent_eventhandler.h"
#include "t_debug_logging.h"

const TVersion securityServerVersion(0,1,1);

/**
 * First phase constructor
 */
CMultiAgent* CMultiAgent::NewL()
	{
	CMultiAgent* self = new(ELeave) CMultiAgent();
	self->ConstructL();
	return self;
	}

/**
  * Destructor
  */
CMultiAgent::~CMultiAgent()
	{
	LOG_MSG("~CMultiTargetAgent\n");
	iServSession.Close();
	}

/**
 * Constructor
 */
CMultiAgent::CMultiAgent() 
	{
	}

/**
 * Second phase constructor
 */
void CMultiAgent::ConstructL()
	{
	}

/**
  Parse the command line, set agent cpu affinity and call main test function
  */
void CMultiAgent::ClientAppL()
	{
	LOG_MSG("ENTER: CMultiTargetAgent::ClientAppL"); 

	iNumApps = KNumApps;
	iAgentCpuNo = KAgentCpu;
	iTargetNameOffset = KTargetOffset;

	TInt argc = User::CommandLineLength();
	HBufC* commandLine = NULL;
	LOG_MSG2(">Launcher Process() argc=%d", argc);
	
	if(argc)
		{
		commandLine = HBufC::NewLC(argc);
		TPtr commandLineBuffer = commandLine->Des();
		User::CommandLine(commandLineBuffer);

		RBuf printCommandLine;
		CleanupClosePushL(printCommandLine);
		printCommandLine.CreateL(commandLine->Des().Length());
		printCommandLine.Copy(commandLine->Des());
		printCommandLine.Collapse();
		LOG_MSG2(">command line = %S", &printCommandLine );
		CleanupStack::PopAndDestroy( &printCommandLine );

		// create a lexer and read through the command line
		TLex lex(*commandLine);
	
		while (!lex.Eos())
		{
			// only look for options with first character '-'
			if (lex.Get() == '-')
			{
			TChar arg = lex.Get();
			
				switch ( arg )
				{
				case 'n':
					lex.Val( iNumApps );
					LOG_MSG2("parsed numApps as %d", iNumApps); 
					break;
		
				case 'a':
					lex.Val( iAgentCpuNo );
					LOG_MSG2("parsed agentCpuNo as %d", iAgentCpuNo);                        
					break;

				case 'o':
					lex.Val( iTargetNameOffset );
					LOG_MSG2("parsed iTargetNameOffset as %d", iTargetNameOffset);        
					break;

				default:
					LOG_MSG("Bad argument from user"); 
					break;                 
				}
			}
		}
	}
	// Create active scheduler (to run active objects)
	CActiveScheduler* scheduler = new (ELeave) CActiveScheduler();
	CleanupStack::PushL(scheduler);
	CActiveScheduler::Install(scheduler);
	
	if (iAgentCpuNo)
		{
		LOG_MSG2("CMultiAgent::ClientAppL() - setting agent to cpu %d", iAgentCpuNo);
		UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny *)iAgentCpuNo, 0);
		}

	TInt err = iServSession.Connect(securityServerVersion);
	
	if (err != KErrNone)
		{
		User::Panic(_L("Can't open server session"), err);
		}

	StartTest();

	// Note: below is a workaround to overcome an issue with RTest server crashing 
	// when writing to the windows console from different agents (on different CPUs 
	// at the same time). To overcome this we signal the launcher using a global 
	// semaphore to indicate a RTest complete instead
	RSemaphore launchSemaphore;
	CleanupClosePushL(launchSemaphore);
            
	TFindSemaphore launchSemFinder(KLaunchSemaphoreSearchString);
	TFullName semaphoreResult;
	TInt ret = launchSemFinder.Next(semaphoreResult);
	LOG_MSG3( "> Find Launch Semaphote.Next ret=%d, %lS", ret, &semaphoreResult);
         
	ret = launchSemaphore.OpenGlobal(semaphoreResult);
	LOG_MSG2( ">OpenGlobal semaphore ret=%d", ret );         
    
	LOG_MSG( ">Signalling semaphore" );
	launchSemaphore.Signal();
	CleanupStack::PopAndDestroy(&launchSemaphore); // launchSemaphore

	// Delete active scheduler
	CleanupStack::PopAndDestroy(scheduler);

	if (commandLine)
	CleanupStack::PopAndDestroy(commandLine);
	
	LOG_MSG( "EXIT: CMultiTargetAgent::ClientAppL"); 
	}

/**
  Launch a process
  @param aProcess the RProcess object used to create the process
  @param aFileName file name of the executable used to create the process
  @return KErrNone on success, or one of the other system wide error codes
  */
TInt CMultiAgent::LaunchProcess(RProcess& aProcess, const TDesC& aExeName, const TDesC& aCommandLine)    
	{
	LOG_MSG( "ENTER: CMultiAgent::LaunchProcess");
    LOG_MSG2("%S", &TPtr8((TUint8*)aExeName.Ptr(), 2*aExeName.Length(), 2*aExeName.Length()));	
	
	// wait for 0.5 seconds due to issue with creating several processes in smp quickly
	User::After(500000);
	
	TInt err = aProcess.Create( aExeName, aCommandLine );
	LOG_MSG2( "CMultiAgent::LaunchProcess, aProcess.Create err = %d", err); 

	// check that there was no error raised
	if (err != KErrNone)
		return err;
	
	// rendezvous with process
	TRequestStatus status = KRequestPending;
	aProcess.Rendezvous(status);

	if (KRequestPending != status.Int())
		{
		// startup failed so kill the process
		LOG_MSG2( "> RProcess Rendezvous() failed with %d. Killing process", status.Int() );
		aProcess.Kill(KErrNone);
		LOG_MSG( "EXIT: CMultiAgent::LaunchProcess");
		return status.Int();
		}
	else
		{
		// start the test target
		aProcess.Resume();
		User::WaitForRequest(status);
	
		LOG_MSG2( "> CMultiAgent::LaunchProcess: RProcess Resume() Rendezvous successful %d: ", status.Int() );

		if(KErrNone != status.Int())
			{
			LOG_MSG2( "> RProcess Resume() failed with %d. Killing process", status.Int() );
			aProcess.Kill(KErrNone);
			}

		LOG_MSG( "EXIT: CMultiAgent::LaunchProcess");
		return status.Int();
		}
	}

/**
  Handle Event
  @param aEventInfo object containing event information from the DSS 
  */
void CMultiAgent::HandleEvent(TEventInfo& aEventInfo)
	{
	LOG_MSG( "ENTER: CMultiAgent::HandleEvent" ); 
	TInt ret = KErrNone;
	const TInt idValid = 1;
	
	switch ( aEventInfo.iEventType )
		{
		case EEventsAddProcess:
			{
			LOG_MSG(">> EEventsAddProcess");                        
			TPtrC8 exeNamePtr8(aEventInfo.iAddProcessInfo.iFileName, aEventInfo.iAddProcessInfo.iFileNameLength);
	
			RBuf8 exeName8;
			CleanupClosePushL(exeName8);
			exeName8.CreateL(exeNamePtr8);
			LOG_MSG2("From event: exeName8=%S", &exeName8);
			CleanupStack::PopAndDestroy(&exeName8);
			LOG_MSG("Testing if event process id is valid");

			LOG_MSG2("Got aEventInfo.iProcessId=%d", I64LOW( aEventInfo.iProcessId));
			__ASSERT_ALWAYS((aEventInfo.iProcessIdValid==idValid), User::Panic(_L("ProcessId Invalid"), aEventInfo.iProcessIdValid));
	
			RProcess targetProc;
			ret = targetProc.Open(TProcessId(aEventInfo.iProcessId));
			LOG_MSG2("RProcess open ret=%d", ret);
			targetProc.Close();

			__ASSERT_ALWAYS((ret == KErrNone), User::Panic(_L("ProcessId Invalid"), aEventInfo.iProcessIdValid));
			break;
			}
	
		case EEventsStartThread:
			{
			LOG_MSG(">> EEventsStartThread");                
			TPtrC8 exeNamePtr8(aEventInfo.iStartThreadInfo.iFileName, aEventInfo.iStartThreadInfo.iFileNameLength);
			RBuf8 exe8Name;
			CleanupClosePushL(exe8Name);
			exe8Name.CreateL(exeNamePtr8);
			LOG_MSG2("From event: exeName8=%S", &exe8Name);
			CleanupStack::PopAndDestroy(&exe8Name);
	
			LOG_MSG("Testing if event process id is valid" );

			__ASSERT_ALWAYS((aEventInfo.iProcessIdValid==idValid), User::Panic(_L("ProcessId Invalid"), aEventInfo.iProcessIdValid));

			LOG_MSG2("Got aEventInfo.iProcessId=%d", I64LOW(aEventInfo.iProcessId));

			LOG_MSG("Testing if event thread id is valid");

			__ASSERT_ALWAYS((aEventInfo.iThreadIdValid==idValid), User::Panic(_L("ThreadId Invalid"), aEventInfo.iThreadIdValid));

			LOG_MSG2("Got aEventInfo.iThreadId=%d", I64LOW(aEventInfo.iThreadId));
			break;                    
			}                       

		case EEventsUserTrace:
			{
			LOG_MSG(">> EEventsUserTrace");  
			break;
			}

		case EEventsRemoveProcess:
			{
			LOG_MSG( ">> EEventsRemoveProcess");                        
			iLaunchCompleted++; 
			break;
			}
	
		default:   
			{
			LOG_MSG( ">> Unknown event - probably due to DSS busy?");
			break;
			}	
		}
 	 
	LOG_MSG("EXIT: CMultiAgent::HandleEvent"); 
	}

/**
 * Main test function which launches several targets and stresses the DSS 
 */
TInt CMultiAgent::StartTest()
	{
	LOG_MSG("ENTER: CMultiTargetAgent::StartTest");

	for( TInt i = 0; i < iNumApps; i++ )
		{
		RBuf targetName;
		RBuf launcherOptions;

		CleanupClosePushL(targetName); 
		CleanupClosePushL(launcherOptions); 

		targetName.CreateL( KTargetExe().Length() + 2 );
		targetName.Format( KTargetExe(), i + iTargetNameOffset + 1 );

		LOG_MSG2("App %d: ", i+1);
		LOG_MSG2("%S", &TPtr8((TUint8*)targetName.Ptr(), 2*targetName.Length(), 2*targetName.Length()));	

		launcherOptions.CreateL( KTargetOptions().Length() + 2 );
		launcherOptions.Format( KTargetOptions(), (TUint)ENormalExit, (i+1) );

		LOG_MSG( "AppOptions : ");
		LOG_MSG2("%S", &TPtr8((TUint8*)launcherOptions.Ptr(), 2*launcherOptions.Length(), 2*launcherOptions.Length()));	
		
		// Add each test target to array
		iTargetList.AppendL(CAgentAsyncEvent::NewL(*this, targetName, launcherOptions));
		CleanupStack::PopAndDestroy(2, &targetName );
		}
	
	iLaunchCompleted = 0;
	TInt err = KErrNone;
		
	for (TInt i = 0; i < iNumApps; i++)
		{
		// Attach to process non-passively
		LOG_MSG2( ">AttachExecutable app %d ", i + iTargetNameOffset + 1 );
		LOG_MSG2("%S", &TPtr8((TUint8*)iTargetList[i]->GetExecutable().Ptr(), 2*iTargetList[i]->GetExecutable().Length(), 
					2*iTargetList[i]->GetExecutable().Length()));

		err = iServSession.AttachExecutable( iTargetList[i]->GetExecutable(), EFalse);
		__ASSERT_ALWAYS((err == KErrNone), User::Panic(_L("DSS Attach failed"), err));

		// Continue on interested event actions
		LOG_MSG2( ">SetEventAction app %d,  EEventsStartThread EAcionContinue", i + iTargetNameOffset + 1);

		err = iServSession.SetEventAction( iTargetList[i]->GetExecutable(), EEventsStartThread, EActionContinue);
		__ASSERT_ALWAYS((err==KErrNone), User::Panic(_L("SetEventAction Error"), err));
	
		LOG_MSG2(">SetEventAction app %d,  EEventsAddProcess EActionContinue", i + iTargetNameOffset + 1);
		err = iServSession.SetEventAction( iTargetList[i]->GetExecutable(), EEventsAddProcess, EActionContinue);
		__ASSERT_ALWAYS((err==KErrNone), User::Panic(_L("SetEventAction Error"), err));

		LOG_MSG2(">SetEventAction app %d,  EEventsUserTrace EActionContinue", i + iTargetNameOffset + 1);
		err = iServSession.SetEventAction( iTargetList[i]->GetExecutable(), EEventsUserTrace, EActionContinue);
		__ASSERT_ALWAYS((err==KErrNone), User::Panic(_L("SetEventAction Error"), err));
	
		LOG_MSG2(">SetEventAction app %d,  EEventsRemoveProcess EActionContinue", i + iTargetNameOffset + 1);
		err = iServSession.SetEventAction( iTargetList[i]->GetExecutable(), EEventsRemoveProcess, EActionContinue);
		__ASSERT_ALWAYS((err==KErrNone), User::Panic(_L("SetEventAction Error"), err));

		// Add target object to active schedular
		iTargetList[i]->Watch();
		}

	for (TInt i= 0; i< iNumApps; i++)
		{
		LOG_MSG( ">Calling LaunchProcess function");
		err = LaunchProcess(iTargetList[i]->GetProcHandle(), iTargetList[i]->GetExecutable(), iTargetList[i]->GetExeConfig());
		__ASSERT_ALWAYS((err==KErrNone), User::Panic(_L("LaunchProcess failed"), err));
		}

	LOG_MSG( ">CActiveScheduler::Start()");
	CActiveScheduler::Start();

	for (TInt i= 0; i < iNumApps; i++)
		{
		// Now detach again
		LOG_MSG( "Before iServSession.DetachExecutable" );
		err = iServSession.DetachExecutable(iTargetList[i]->GetExecutable());
		__ASSERT_ALWAYS((err==KErrNone), User::Panic(_L("DetachExecutable failed"), err));
		}
	
	// Free all the memory
	iTargetList.ResetAndDestroy();
	LOG_MSG( "EXIT: CMultiTargetAgent::StartTest" );

	return KErrNone;
	}

/**
  * Entry point for run mode debug driver test
  */
GLDEF_C TInt E32Main()
	{
	LOG_MSG( "ENTER: Multi_agent E32Main ");
	__UHEAP_MARK;

	TInt ret = KErrNone;
	RProcess::Rendezvous(KErrNone);
	
	CTrapCleanup* trap = CTrapCleanup::New();
		
	if (!trap)
		return KErrNoMemory;
	
	CMultiAgent *runModeAgent = CMultiAgent::NewL();

	if (runModeAgent != NULL)
		{
		TRAP(ret,runModeAgent->ClientAppL());
		LOG_MSG2( "ClientAppL returned %d", ret );
		delete runModeAgent;
		}

	delete trap;
	__UHEAP_MARKEND;
	LOG_MSG( "EXIT: Multi_agent E32Main ");
	return ret;
	}

