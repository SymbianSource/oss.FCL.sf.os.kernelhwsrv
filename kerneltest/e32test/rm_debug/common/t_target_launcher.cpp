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
// Helper app to launch debug targets. Uses command-line parameters as follows using a + sign:
//  +n<number of applications to launch>
//  +m<number of times to launch each application>
//  +o<order of launch, 1 means launch in reverse order>
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
#include <hal.h>
#include <u32hal.h>
#include <e32property.h>

#include "t_target_launcher.h"


/**
  Launch a process

  @param aProcess The RProcess object to use to create the process
  @param aExeName File name of the executable to create the process from
  @param aCommandLine The command line to pass to the new process
  @return KErrNone on success, or one of the other system wide error codes
  */
TInt LaunchProcess(RProcess& aProcess, TDesC & aExeName, TPtr & aCommandLine )
    {

    TPtrC commandLine( aCommandLine );

    TInt err = aProcess.Create( aExeName, commandLine );

    // check that there was no error raised
    if(err != KErrNone)
        {
        return err;
        }

    TRequestStatus status = KRequestPending;
    aProcess.Rendezvous(status);

    if(KRequestPending != status.Int())
        {
        // startup failed so kill the process
        RDebug::Printf( "> RProcess Rendezvous() failed with %d. Killing process", status.Int() );
        aProcess.Kill(KErrNone);
        return status.Int();
        }
    else
        {
        // start up succeeded so resume the process
        aProcess.Resume();
        User::WaitForRequest(status);
        if(KErrNone != status.Int())
            {
            RDebug::Printf( "> RProcess Resume() failed with %d. Killing process", status.Int() );
            aProcess.Kill(KErrNone);
            }
        return status.Int();
        }
    }

/**
 * Read command line parameters and control the launching of targets. 
 * Create global launch semaphore KLaunchMutexName
 */
void MainL()
    {

    TInt numApps = KNumApps;
    TInt numLaunches = KNumLaunches;
    TInt launchControl = 0;

    TInt argc = User::CommandLineLength();
    HBufC* commandLine = NULL;
    RDebug::Printf( ">Launcher Process() argc=%d", argc );

    if( argc )
        {
        commandLine = HBufC::NewLC(argc);
        TPtr commandLineBuffer = commandLine->Des();
        User::CommandLine(commandLineBuffer);

        RBuf printCommandLine;
        CleanupClosePushL( printCommandLine );
        printCommandLine.CreateL( commandLine->Des().Length() );
        printCommandLine.Copy( commandLine->Des() );
        printCommandLine.Collapse();
        RDebug::Printf( ">command line = %S", &printCommandLine );
        CleanupStack::PopAndDestroy( &printCommandLine );

        // create a lexer and read through the command line
        TLex lex(*commandLine);
        while (!lex.Eos())
            {
            // only look for options with first character '+', other switches are for the targets
            if (lex.Get() == '+')
                {
                TChar arg = lex.Get();
                switch (arg)
                    {
                    case 'n':
                        lex.Val( numApps );
                        RDebug::Printf("parsed numApps as %d", numApps);
                        break;
                    case 'm':
                        lex.Val( numLaunches );
                        RDebug::Printf("parsed numLaunches as %d", numLaunches );
                        break;
                    case 'o':
                        lex.Val( launchControl );
                        RDebug::Printf("parsed launchControl as %d", launchControl);
                        break;
                    default:
                        // unknown argument ignore it
                        break;             
                    }//switch
                }// if +
            }//while
        }//if argc

   RSemaphore launchMutex;
   TInt ret = KErrNone;
   CleanupClosePushL( launchMutex );
   ret = launchMutex.CreateGlobal( KLaunchMutexName, 0 );
   RDebug::Printf( ">Target Launcher : RSemaphore.CreateGlobal ret %d", ret);
   User::LeaveIfError( ret );

   ret = launchMutex.OpenGlobal( KLaunchMutexName );
   RDebug::Printf( ">Target Launcher : RSemaphore.OpenGlobal ret %d", ret);
   User::LeaveIfError( ret );

   //Now launch the requested number of apps for the requested number of launches
   for( ; numLaunches > 0; numLaunches-- )
       { 
       for( TInt launchIndex = numApps; launchIndex > 0; launchIndex-- )  
           {
           RDebug::Printf( ">Target Launcher:  Mutex wait app %d, launch %d", launchIndex, numLaunches );
           launchMutex.Wait();

           RBuf targetName;
           CleanupClosePushL( targetName );
           RDebug::Printf( ">Target Launcher:  targetName.Create %d, launch %d", launchIndex, numLaunches );
           targetName.Create( KTargetExe().Length() + 2 );

           if( launchControl == 1 )
               {
               // Reverse the order of the apps launched by reversing the index in the name
               RDebug::Printf( ">Target Launcher:  targetName.Format %d, launch %d", numApps - launchIndex + 1, numLaunches );
               targetName.Format( KTargetExe(), numApps - launchIndex + 1 );
               }
           else
               {
               RDebug::Printf( ">Target Launcher:  targetName.Format %d, launch %d", launchIndex, numLaunches );
               targetName.Format( KTargetExe(), launchIndex );
               }

           RProcess aProc;
           CleanupClosePushL( aProc ); 
    
           RDebug::Printf( ">Target Launcher: LaunchProcess %d, launch %d", launchIndex, numLaunches );
           RDebug::Printf( ">LaunchProcess %lS", &targetName );
           TPtr cmdLinePtr( commandLine->Des() );
           ret = LaunchProcess( aProc, targetName, cmdLinePtr );
           CleanupStack::PopAndDestroy( &aProc );

           RDebug::Printf( "<Target Launcher: LaunchProcess returned %d", ret );
           CleanupStack::PopAndDestroy( &targetName );

           User::LeaveIfError( ret );

           //By now the add proc event should have been delivered to the
           //test app agent.
           }
       }

    launchMutex.Wait( 500000 );

    CleanupStack::PopAndDestroy( &launchMutex );

    if( commandLine )
       CleanupStack::PopAndDestroy( commandLine );
 
    }


GLDEF_C TInt E32Main()
	{
	RProcess thisProcess;
	thisProcess.Rendezvous(KErrNone);
	RDebug::Printf( ">Launcher Process()" );

	CTrapCleanup* trap = CTrapCleanup::New();
	if (!trap)
		return KErrNoMemory;

	TRAPD(err, MainL());
	RDebug::Printf( "< Target launching returned %d", err);
	
	delete trap;
	
	return err;
	}
