// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Helper app to launch debug targets
//
//

#include "t_multi_agent_launcher.h"

#include "t_debug_logging.h"

/**
 * Launch a process
 * @param aProcess the RProcess object used for creating the process
 * @param aExeName the name of the executable to run 
 * @param aCommandLine command line parameters to pass when creating the process 
 * @return KErrNone on success, or one of the other system wide error codes
 */
TInt LaunchProcess(RProcess& aProcess, TDesC& aExeName, TDesC& aCommandLine )    
	{
	LOG_MSG("ENTER: t_multi_agent_launcher: launchProcess"); 

	LOG_MSG2("aExeName %S ", &TPtr8((TUint8*)aExeName.Ptr(), 2*aExeName.Length(), 2*aExeName.Length()));
	LOG_MSG2("aCommandLine %S", &TPtr8((TUint8*)aCommandLine.Ptr(), 2*aCommandLine.Length(), 2*aCommandLine.Length()));

	TInt err = aProcess.Create( aExeName, aCommandLine );
	LOG_MSG2("t_multi_agent_launcher launchProcess, aProcess.Create err = %d", err); 

	// check that there was no error raised
	if(err != KErrNone)
		{
		return err;
		}

	// rendezvous with process
	TRequestStatus status = KRequestPending;
	aProcess.Rendezvous(status);

	if(KRequestPending != status.Int())
		{
		// startup failed so kill the process
		LOG_MSG2("t_multi_agent_launcher: launchProcess: RProcess Rendezvous() failed with %d. Killing process", status.Int());
		aProcess.Kill(KErrNone);
		return status.Int();
		}
	else
		{
		aProcess.Resume();
		User::WaitForRequest(status);

		LOG_MSG2("t_multi_agent_launcher: launchProcess: RProcess Resume() Rendezvous successful %d: ", status.Int());

		if(KErrNone != status.Int())
			{
			LOG_MSG2("t_multi_agent_launcher: RProcess Resume() failed with %d. Killing process", status.Int());
			aProcess.Kill(KErrNone);
			}

		LOG_MSG("EXIT: t_multi_agent_launcher launchProcess");
		return status.Int();
		}
	}

/**
 * Read command line parameters and control the launching of the agents. 
 */
void MainL()
	{	
	LOG_MSG( "ENTER: t_multi_agent_launcher MainL()");

	TInt ret = KErrNone;
	TInt numAgents = KNumAgents;
	TInt numTargets = KNumTargets;
	TInt numTestRuns = KNumTestRuns;

	TInt argc = User::CommandLineLength();
	HBufC* commandLine = NULL;
	LOG_MSG2("t_multi_agent_launcher: MainL(): argc=%d", argc);
    
	if(argc)
		{
		commandLine = HBufC::NewLC(argc);
		TPtr commandLineBuffer = commandLine->Des();
		User::CommandLine(commandLineBuffer);

		RBuf printCommandLine;
		CleanupClosePushL( printCommandLine );
		printCommandLine.CreateL( commandLine->Des().Length() );
		printCommandLine.Copy( commandLine->Des() );
		printCommandLine.Collapse();
		LOG_MSG2("t_multi_agent_launcher: command line = %S", &printCommandLine);
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
							lex.Val( numAgents );
							LOG_MSG2("t_multi_agent_launcher: parsed numAgents as %d", numAgents);
							break;
						case 'm':
							lex.Val( numTargets );
							LOG_MSG2("t_multi_agent_launcher: parsed numTargets as %d", numTargets);                        
							break;  
						case 't':
							lex.Val( numTestRuns );
							LOG_MSG2("t_multi_agent_launcher: parsed numTestRuns as %d", numTestRuns);                        
							break;                    
						default:
							LOG_MSG("t_multi_agent_launcher: unknown argument ignoring it");
							break;                 
						}
				}
			}
		}

	// Note: below is a workaround to overcome an issue with RTest server crashing 
	// when writing to the windows console from different agents (on different CPUs 
	// at the same time). To overcome this we get signaled by the agents when they have 
	// completed their tests so that we can do a RTest complete
	RSemaphore launchSemaphore;
	CleanupClosePushL(launchSemaphore);
	ret = launchSemaphore.CreateGlobal(KLaunchSemaphoreName, 0);
	LOG_MSG2( ">Target Launcher : RSemaphore.CreateGlobal ret %d", ret);
	User::LeaveIfError( ret );

	ret = launchSemaphore.OpenGlobal(KLaunchSemaphoreName);
	LOG_MSG2( ">Target Launcher : RSemaphore.OpenGlobal ret %d", ret);
	User::LeaveIfError( ret );

	//Now launch the requested number of apps for the requested number of test runs
	for( TInt j = 0; j < numTestRuns; j++ )
		{ 
			for( TInt i = 0; i < numAgents; i++ )  
				{
					RBuf targetName;
					targetName.CleanupClosePushL();
					targetName.CreateL(KAgentExe());

					RProcess aProc;
					CleanupClosePushL(aProc); 
					RBuf launcherOptions;
					CleanupClosePushL(launcherOptions);
				    const TInt additionalWords = 2;	
					launcherOptions.CreateL( KAgentOptions().Length() + additionalWords );
		
					// Apply offset: launcherOptions.Format( .., .., i * numTargets, ..)
					// workaround to ensure we have the same binary for multiple agents. 
					// e.g. So if offset = 0, agent attaches to app1, app2, app3, app4, app5
					// if offset = 5, agent attached to app6, app7, app8, app9, app10 etc.
					// Note: apps need to be in rom otherwise the agent will fail on an assert 
					// (with KErrNotFound)
					launcherOptions.Format( KAgentOptions(), (TUint)numTargets, i * numTargets, 0);
			
					ret = LaunchProcess( aProc, targetName, launcherOptions );	
					CleanupStack::PopAndDestroy(3,&targetName);
					User::LeaveIfError(ret);
				}
		}

	// Wait for all agents to do their testing before checking the semaphore
	User::After(12000000);

	LOG_MSG( ">Target Launcher:  Semaphore wait");

	for (TInt i = 0; i < numAgents; i ++)
		{
		//We need this delay just in case an agent crashes and never signals the sem
		ret = launchSemaphore.Wait(100000);
		if( ret != KErrNone )
			{
			LOG_MSG3("launchSemaphore.Wait ret %d for agent %d", ret, i);
			break;
			}
		}

	LOG_MSG2( "testing for Semaphore ret %d", ret);

	// We only want to have one RTest instance at any one time since otherwise RTest can panic
	RTest test(_L("T_MULTI_AGENT_LAUNCHER"));
	test.Start(_L("t_multi_agent_launcher Check for agents finishing correctly"));
	test(ret == KErrNone);
	test.End();
	test.Close();

	CleanupStack::PopAndDestroy(&launchSemaphore); // launchSemaphore

	if( commandLine )
	CleanupStack::PopAndDestroy(commandLine);
	
	LOG_MSG("EXIT: t_multi_agent_launcher MainL()");
	}
 
GLDEF_C TInt E32Main()
	{
	LOG_MSG("ENTER: Multi_agent_launcher E32Main()");
	__UHEAP_MARK;

	CTrapCleanup* trap = CTrapCleanup::New();
	if (!trap)
		return KErrNoMemory;

	TRAPD(err, MainL());
	LOG_MSG2("Multi_agent_launcher: returning from MainL(), err = %d", err);
	
	delete trap;
	LOG_MSG("EXIT: Multi_agent_launcher E32Main()");
	__UHEAP_MARKEND;

	return err;
	}

