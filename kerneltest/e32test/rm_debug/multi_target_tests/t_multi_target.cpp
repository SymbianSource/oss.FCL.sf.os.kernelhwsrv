// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Test the ability of the debug system to handle events from several debug targets
//
//

#include <e32base.h>
#include <e32property.h>

#include <hal.h>
#include <e32test.h>

#include "t_multi_target.h"
#include "t_target_launcher.h"
#include "t_rmdebug_app.h"

#ifdef KERNEL_OOM_TESTING
  #ifdef USER_OOM_TESTING
    #error "Cannot define both KERNEL_OOM_TESTING and USER_OOM_TESTING"
  #endif
#endif


using namespace Debug;

const TVersion securityServerVersion(0,1,1);

const TVersion testVersion(2,1,0);

#ifdef SYMBIAN_STANDARDDEBUG
LOCAL_D RTest test(_L("T_RMDEBUG_MULTI_TARGET"));
#endif

#ifdef SYMBIAN_OEMDEBUG
LOCAL_D RTest test(_L("T_RMDEBUG_MULTI_TARGET_OEM"));
#endif

#ifdef SYMBIAN_OEM2DEBUG
LOCAL_D RTest test(_L("T_RMDEBUG_MULTI_TARGET_OEM2"));
#endif



CMultiTargetAgent* CMultiTargetAgent::NewL()
//
// CMultiTargetAgent::NewL
//
  {
  CMultiTargetAgent* self = new(ELeave) CMultiTargetAgent();

  self->ConstructL();

  return self;
  }


CMultiTargetAgent::~CMultiTargetAgent()
//
// CMultiTargetAgent destructor
//
    {
    RDebug::Printf("~CMultiTargetAgent\n");
    iServSession.Close();
    }


CMultiTargetAgent::CMultiTargetAgent() : 
    iEventPtr( (TUint8*)&iEventInfo, sizeof(TEventInfo) )
    {
    }


void CMultiTargetAgent::ConstructL()
//
// CMultiTargetAgent::ConstructL
//
  {
  }

/**
 * Helper code for the stepping tests. Returns the number of nanokernel ticks in one second.
 *
 * @return Number of nanokernel ticks. 0 if unsuccesful.
 */
TInt CMultiTargetAgent::HelpTicksPerSecond(void)
  {
  TInt nanokernel_tick_period;
  HAL::Get(HAL::ENanoTickPeriod, nanokernel_tick_period);

  ASSERT(nanokernel_tick_period != 0);

  static const TInt KOneMillion = 1000000;

  return KOneMillion/nanokernel_tick_period;
  }

void CMultiTargetAgent::ClientAppL()
//
// Performs each test in turn
//
  {
  test.Start(_L("ClientAppL"));
  TInt err = iServSession.Connect(securityServerVersion);
  if (err != KErrNone)
      {
      User::Panic(_L("Can't open server session"), err);
      }
  SetupDebugServerL();
  LaunchTargetsInOrderL();
  RDebug::Printf( "returning from CMultiTargetAgent::ClientAppL" );
  test.End();
  }

/**
  Launch a process

  @param aProcess The RProcess object to use to create the process
  @param aExeName File name of the executable to create the process from
  @param aCommandLine The command line to pass to the new process
  @return KErrNone on success, or one of the other system wide error codes
  */
TInt CMultiTargetAgent::LaunchProcess(RProcess& aProcess, TDesC & aExeName, TDesC & aCommandLine )
    {    
    TInt err = aProcess.Create( aExeName, aCommandLine );    
    if(err != KErrNone)
        {
        RDebug::Printf( "aProcess.Create ret %d", err);
        return err;
        }

    TRequestStatus status = KRequestPending;
    aProcess.Rendezvous(status);
    if(KRequestPending != status.Int())
        {
        // startup failed so kill the process
        aProcess.Kill(KErrNone);
        return status.Int();
        }
    else
        {
        // start up succeeded so resume the process
        aProcess.Resume();
        // Give the process a chance to run
        User::After( 500000 );
        return KErrNone;
        }
    }

void CMultiTargetAgent::SetupDebugServerL()
    {
    RDebug::Printf( "CMultiTargetAgent::SetupDebugServerL" );
    test.Next(_L("SetupDebugServerL\n"));
    iTargets.ReserveL( KNumApps );

    RBuf targetName;
    CleanupClosePushL( targetName );

    for( TInt numApps = 0; numApps < KNumApps; numApps++ )
        {
        iTargets.AppendL( targetName );
        RDebug::Printf( "Attach to DSS for app %d ", numApps );

        iTargets[numApps].CreateL( KTargetExe().Length() + 2 );
        iTargets[numApps].Format( KTargetExe(), numApps+1 );

        TInt ret = iServSession.AttachExecutable( iTargets[numApps], EFalse );
        test( ret == KErrNone );

        RDebug::Printf( ">SetEventAction app %d,  EEventsStartThread EActionSuspend", numApps );
        ret = iServSession.SetEventAction( iTargets[numApps], EEventsStartThread, EActionSuspend );
        test( ret == KErrNone );

        RDebug::Printf( ">SetEventAction app %d,  EEventsAddProcess EActionContinue", numApps );
        ret = iServSession.SetEventAction( iTargets[numApps], EEventsAddProcess, EActionContinue );
        test( ret == KErrNone );

        RDebug::Printf( ">SetEventAction app %d,  EEventsRemoveProcess EActionContinue", numApps );
        ret = iServSession.SetEventAction( iTargets[numApps], EEventsRemoveProcess, EActionContinue );
        test( ret == KErrNone );
        }

    CleanupStack::PopAndDestroy( &targetName ); // targetName

    }



TInt CMultiTargetAgent::LaunchTargetsInOrderL()
    {
    RDebug::Printf( "CMultiTargetAgent::LaunchTargetsInOrderL" );
    
    RBuf launcher;
    CleanupClosePushL( launcher );
    launcher.CreateL( KLauncherExe() );
    
    RBuf launcherOptions;
    CleanupClosePushL( launcherOptions ); 
    launcherOptions.CreateL( KTargetOptions().Length() + 2 );
    launcherOptions.Format( KTargetOptions(), (TUint)ENormalExit );

    RDebug::Printf( ">LaunchProcess()" );
    RProcess launcherProc; 
    CleanupClosePushL( launcherProc );
    
    TInt ret = LaunchProcess( launcherProc, launcher, launcherOptions );
    RDebug::Printf( "<LaunchProcess() ret %d", ret );
    
    CleanupStack::PopAndDestroy( &launcherProc ); // launcherProc
    CleanupStack::PopAndDestroy( &launcherOptions ); // launcherOptions
    CleanupStack::PopAndDestroy( &launcher ); //launcher 

    test( ret == KErrNone );
    
    RSemaphore launchSemaphore;   
    CleanupClosePushL( launchSemaphore );
            
    TFindSemaphore launchSemFinder( KLaunchMutexNameSearchString );
    TFullName mutexResult;
    ret = launchSemFinder.Next(mutexResult);
    RDebug::Printf( ">  Find Launch Semaphote.Next ret=%d, %lS", ret, &mutexResult );
    test( ret == KErrNone );   
     
    ret = launchSemaphore.OpenGlobal( mutexResult );
    RDebug::Printf( "> OpenGlobal mutex ret=%d", ret );         
    test( ret == KErrNone );    
    
    TBool thisLaunchCompleted; 

    test.Next(_L("LaunchTargetsInOrderL\n"));
    for( TInt numLaunches = KNumLaunches; numLaunches > 0; numLaunches-- )
        {
        for( TInt numApps = KNumApps; numApps > 0; numApps-- )
            {
            thisLaunchCompleted = EFalse;
            // This will trigger the launcher app to launch the next target
            RDebug::Printf( " >Mutex.Signal app=%d, launch=%d", numApps, numLaunches);
            launchSemaphore.Signal();
            
            RBuf8 tgt8Name; 
            CleanupClosePushL( tgt8Name );
           
            RBuf tgtCollapseName;
            CleanupClosePushL( tgtCollapseName );
                    
            tgtCollapseName.CreateL( iTargets[numApps-1] );
            tgt8Name.CreateL( tgtCollapseName.Collapse() );
            

            while( ! thisLaunchCompleted )
                {
                RDebug::Printf( ">GetEvent app %d for %S", numApps, &tgt8Name );
                iServSession.GetEvent( iTargets[numApps-1], iStatus, iEventPtr );
          
                // Wait for the target to get started.
                RDebug::Printf( " >Wait for event from target app=%d, launch=%d\n", numApps, numLaunches);
                User::WaitForRequest( iStatus );
                RDebug::Printf( " <Wait for request returned with status %d", iStatus.Int() );
                test( iStatus==KErrNone );
    
                RDebug::Printf( " > Got iEventType =%d, app=%d", iEventInfo.iEventType, numApps );
                switch( iEventInfo.iEventType )
                    {
                    case EEventsAddProcess:
                        {
                        RDebug::Printf( "Got EEventsAddProcess" );                        
                        TPtrC8 exeNamePtr8( iEventInfo.iAddProcessInfo.iFileName, iEventInfo.iAddProcessInfo.iFileNameLength );
                        
                        RBuf8 exeName8;
                        CleanupClosePushL( exeName8 );
                        exeName8.CreateL( exeNamePtr8 );
                        RDebug::Printf( " from event: exeName8=%S", &exeName8 );
                        CleanupStack::PopAndDestroy( &exeName8 );
                        
                        RBuf8 compareName8;
                        CleanupClosePushL( compareName8 );
                        compareName8.CreateL( KTargetExeName().Length() + 10 );
                        compareName8.Format( KTargetExeName(), numApps );
                        RDebug::Printf( " comparing to: compareName8=%S", &compareName8 );
                        
                        test( compareName8.CompareC( exeNamePtr8 ) == 0 );
                        CleanupStack::PopAndDestroy( &compareName8 );

                        RDebug::Printf( "Testing if event process id is valid" );
                        test( iEventInfo.iProcessIdValid );
                        RDebug::Printf( "Got iEventInfo.iProcessId=%d", I64LOW( iEventInfo.iProcessId ) );
                        
                        RProcess targetProc;
                        ret = targetProc.Open( TProcessId( iEventInfo.iProcessId ) );
                        RDebug::Printf( "RProcess open ret=%d",ret );
                        targetProc.Close();
                        test( ret == KErrNone );
                        
                        break;
                        }//EEventsAddProcess
                        
                    case EEventsStartThread:
                        {
                        RDebug::Printf( "Got EEventsStartThread" );
                         
                        TPtrC8 exeNamePtr8( iEventInfo.iStartThreadInfo.iFileName, iEventInfo.iStartThreadInfo.iFileNameLength );
                        RBuf8 exe8Name;
                        CleanupClosePushL( exe8Name );
                        exe8Name.CreateL( exeNamePtr8 );
                        RDebug::Printf( " from event: exeName8=%S", &exe8Name );
                        CleanupStack::PopAndDestroy( &exe8Name );
                        
                        test( tgt8Name.CompareC( exeNamePtr8 ) == 0 );
                        
                        RDebug::Printf( "Testing if event process id is valid" );
                        test( iEventInfo.iProcessIdValid );
                        RDebug::Printf( "Got iEventInfo.iProcessId=%d", I64LOW( iEventInfo.iProcessId ) );
                         
                        RDebug::Printf( "Testing if event thread id is valid" );
                        test( iEventInfo.iThreadIdValid );
                        RDebug::Printf( "Got iEventInfo.iThreadId=%d", I64LOW( iEventInfo.iThreadId ) );
                        
                        RThread targetThread;
                        CleanupClosePushL( targetThread );
                        
                        ret = targetThread.Open( TThreadId( iEventInfo.iThreadId ) );
                        RDebug::Printf( "RThread open ret=%d", ret );
                        test( ret == KErrNone ); 
                         
                        test( iEventInfo.iThreadId == targetThread.Id() );  

                        RDebug::Printf( "Resuming thread for app=%d, id=%d", numApps, I64LOW( targetThread.Id() ));
                        ret = iServSession.ResumeThread( iEventInfo.iThreadId );
                        CleanupStack::PopAndDestroy( &targetThread );
                        
                        test( ret == KErrNone );
                        
                        ret = iServSession.ResumeThread( iEventInfo.iThreadId );
                        break;                    
                        }//case EEventsStartThread                        

                    case ( EEventsRemoveProcess ):
                        {
                        RDebug::Printf( "*** Got EEventsRemoveProcess. app%d has exited. Moving on to next app", numApps );                        
                        thisLaunchCompleted = ETrue;
                        break;
                        }
                        
                    default :   
                        RDebug::Printf( "Got unknown event" );
                        test( EFalse );
                        break;
                    }
                }//while

            CleanupStack::PopAndDestroy( &tgtCollapseName ); // tgtCollapseName
            CleanupStack::PopAndDestroy( &tgt8Name ); // tgt8Name 
            }
        }    

    launchSemaphore.Signal();
    
	CleanupStack::PopAndDestroy( &launchSemaphore ); // launchSemaphore
  
	for( TInt i = iTargets.Count()-1; i>=0; i-- )
		{
		RDebug::Printf( "Closing target %d", i );
		iTargets[ i ].Close();
		}

	iTargets.Close();
	
	return KErrNone;
    }


GLDEF_C TInt E32Main()
    {
    TInt ret = KErrNone;
   
  
    CTrapCleanup* trap = CTrapCleanup::New();
    if (!trap)
      return KErrNoMemory;
    test.Title();
   
    CMultiTargetAgent *runModeAgent = CMultiTargetAgent::NewL();
    if (runModeAgent != NULL)
        {
        __UHEAP_MARK;
        TRAP(ret,runModeAgent->ClientAppL());
        __UHEAP_MARKEND;
        
        RDebug::Printf( "ClientAppL returned %d", ret );
        delete runModeAgent;
        }

    delete trap;
    return ret;
    }
