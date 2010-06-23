// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL " http://www.eclipse.org/legal/epl-v10.html ".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// eka\drivers\power\smppower\sample_idlehandler\smpidlehandler.cpp
// implements a basic smp idle handler generic layer that can be derived from 
// to create platform specific SMP idle handlers

/**
 @file
 @prototype
*/
#ifndef __WINS__
#include <kernel/arm/arm.h>
#endif
#include "smpidlehandler.h"

#ifdef __SMP__

//-/-/-/-/-/-/-/-/-/ class DDSMPIdleHandler /-/-/-/-/-/-/-/-/-/

DSMPIdleHandler* gTheIdleHandler = NULL;

DSMPIdleHandler::DSMPIdleHandler ()
    :iAllCpusMask(TIdleSupport::AllCpusMask()),
     iStartAfterExtInitDfc(StartAfterExtIntDfcFn,this,Kern::SvMsgQue(),0),
     iInitialised(EFalse)
      {
      // singleton class
      __PM_IDLE_ASSERT_DEBUG(!gTheIdleHandler);
      gTheIdleHandler = this;
      }


DSMPIdleHandler::~DSMPIdleHandler()
	{
	}


/**
   To be called after construction in a thread context with interrupts enabled. Power extension entry point ideal
   @pre thread context ints enable no kernel locks or fast mutexes
   @param aGlobalIntDistAddress GIC address
   @param aBaseIntIfAddress GIC CPU interface address
 */
void DSMPIdleHandler::Initialise(TUint32 aGlobalIntDistAddress, TUint32 aBaseIntIfAddress)
    {
	TIdleSupport::SetupIdleSupport(aGlobalIntDistAddress,aBaseIntIfAddress);
    //Set Idle IPI to highest priority
	TIdleSupport::SetIdleIPIToHighestPriority();
    Arm::SetIdleHandler((TCpuIdleHandlerFn)DSMPIdleHandler::IdleHandler, this);
    iStartAfterExtInitDfc.Enque();
    }


/**
   Must be called when cores are enaged or retired
   @pre calling code must be outside idle handler, or in DoEnterIdle call
*/
void DSMPIdleHandler::ResetSyncPoints()
    {
    iIdleSync.Reset();
    iStage2.Reset();
    }


/**
   Called by idle handler inmediatelly after idle entry. Can be used for things such as checking 
   if a core is going to be retired. Can return EFalse to force the idle handler to return 
   at that point
   @param aCpuMask mask of current cpu
   @param aStage passed from kernel indicated things such core retiring or postamble
   @param aU points to some per-CPU uncached memory used for handshaking in 
       during power up/power down of a core for support of core retiring. This 
       memory is provided by baseport to the kernel via the VIB

   @return EFalse if the cpu should exit idle, ETrue otherwise
 */
TBool DSMPIdleHandler::DoEnterIdle(TInt aCpuMask, TInt aStage,  volatile TAny* aU)
    {
    return ETrue;
    }

 /**
    Called by idle handler all cpus to enter idle once all have entered idle but
    before NTimeQ::IdleTime is called to check time to next timer expiry. Can be used 
    for things such as idle timer pre-idle processing. In such cases only the last CPU
    should do the idle timer processing
    @param aCpuMask mask of current cpu
    @param aLastCPu indicates if this is last CPU.
 */
void DSMPIdleHandler::CpusHaveEnteredIdle(TInt aCpuMask, TInt aLastCpu)
    {
    }


/**
   Called after wakeup can be used for status restoring
   a sync point is placed after the call to this function
   This can be a good place to do idle tick restoration
   @param TInt aCpuMask indicates calling CPU
   @param aLastCPu indicates if this is last CPU.
   @see EnterLowPowerMode
*/
void DSMPIdleHandler::PostWakeup(TInt aCpuMask, TBool aLastCpu)
    {
    }

/**
   Called at exit of idle handler not synchronised in any way
   @param aExitPoint point at which you might exit the idle handler
   @param TInt aCpuMask indicates calling CPU
   @param aLastCPu indicates if this is last CPU.
*/
void DSMPIdleHandler::DoExitIdle(TIdleExit aExitPoint, TInt aCpuMask, TBool aLastCpu)
    {
    }

/**
   IdleHandler just calls DoIdle
*/
void DSMPIdleHandler::IdleHandler(TAny* /*aPtr*/, TInt aStage, volatile TAny* aU)
    {
    gTheIdleHandler->DoIdle(aStage,aU);
    }

/**
   Idle handling per se
*/
void DSMPIdleHandler::DoIdle(TInt aStage, volatile TAny* aU)
    {
    // wait after all extensions have initialsed before running
    if (!iInitialised) return;
    
    TInt c = NKern::CurrentCpu();
	TUint32 cMask = (1<<c);
	TUint32 allButC = (~cMask) & iAllCpusMask;

    if (!DoEnterIdle(cMask,aStage,aU)) return;

	if (TIdleSupport::SetLocalAndCheckSetGlobalIdle(cMask)) // check if last down
		{
        //LastCpuIdle:                               
        PMBTRACE4(KIdleEntry,KIdleEntryLastCpu,TIdleSupport::GetCpusIdleMask());
        // we have now managed to lock the system into all cpus waiting, by setting MSB we are now informing any leaving CPUs 
        // that we intend to take the system down
        // other CPUs with an STI so they can proceed with power down
        // when we get there, all other CPUs can be in of the following states:
        // 1. In the DoWFI of the section above (allButC!=_....) 
        // 2. They could have already got past the DoWFI due to another interrupt waking them up
        TIdleSupport::DoIdleIPI(allButC);          // wake all sleeping CPUs   
        PMBTRACE0(KSendIPI,0);
        SYNCPOINT(iIdleSync,0);  // After this point all should be awake and then other will clear the pending IPI 
        CpusHaveEnteredIdle(cMask,ETrue);
        iStage2.Reset();
        TIdleSupport::ResetLogic(); // 
        SYNCPOINT(iIdleSync,1);  // sync on IPI clearing

        // here do we have
        // a/ Interrupt pending?
        // b/ Timer coming up soon? 
        // if (a or b) just allow all idle handlers to return
        TInt nextTimer = NTimerQ::IdleTime();
        TBool exitNow = !GetLowPowerMode(nextTimer,iLowPowerMode);
        
        PMBTRACE8(KMisc,2,nextTimer,iLowPowerMode);
        
        if (TIdleSupport::IsIntPending() || exitNow)
            {
            // clear idling cpus this will cause all cpus to exit idle handler
            TIdleSupport::SetExitRequired(&iStage2);		
            }
        
        SYNCPOINT(iStage2,2);
        if(TIdleSupport::GetExitRequired())            
            {
            DoExitIdle(EExitBeforeLPM,cMask,ETrue);
            PMBTRACE4(KIdleeXit,KIdleeXitLastCpu1,TIdleSupport::GetCpusIdleMask());
            return;
            }
        
        // We enter LPM here!!!
        TBool exitRouseRequired = EnterLowPowerMode(iLowPowerMode,cMask,ETrue);

        TBool firstToWake = EFalse;

        if (exitRouseRequired)
            {
            firstToWake = TIdleSupport::FirstCoreAwake(cMask);
            if (firstToWake)
                {
                // we are probaly first to rouse so wake the others
                PMBTRACE0(KMisc,0x50);
                TIdleSupport::DoIdleIPI (allButC);
                }
            }

        PostWakeup(cMask,ETrue);
        PMBTRACE4(KMisc,3,exitRouseRequired);
        SYNCPOINT(iIdleSync,3);  
        if (exitRouseRequired)
            {
            if (!firstToWake)
                {
                TIdleSupport::ClearIdleIPI();
                PMBTRACE0(KMisc,0x51);
                }
            }
        DoExitIdle(EExitAfterLPM,cMask,ETrue);    
        PMBTRACE4(KMisc,0x52,TIdleSupport::IntPending()&0x1ff);
        PMBTRACE4(KIdleeXit,KIdleeXitLastCpu0,TIdleSupport::GetCpusIdleMask());
        }
	else
        {
        //NormalIdle:                               
        PMBTRACE4(KIdleEntry,KIdleEntryNormalCpu,TIdleSupport::GetCpusIdleMask());	
        TIdleSupport::DoWFI();	// CPU in WFI
        // here we could have woken up because of the Idle IPI or because of another interrupt
        if (!TIdleSupport::ClearLocalAndCheckGlobalIdle(cMask)) 
            {
            // not all other cores are idling we were woken up by some interrupt so exit
            DoExitIdle(EExitOtherCPUsNotIdle,cMask,EFalse);
            PMBTRACE4(KIdleeXit,KIdleeXitNormalCpu0,TIdleSupport::GetCpusIdleMask());
            return;
            }

        // we are all idling
        SYNCPOINT(iIdleSync,0x10);
        // clear IdleIPI, we could actually have another interrupt pending but 
        // we ignore this for now as it is checked later
        
        TIdleSupport::ClearIdleIPI(); 
        PMBTRACE0(KClearIPI,0x10);
        CpusHaveEnteredIdle(cMask,EFalse);
        SYNCPOINT(iIdleSync,0x11);

        if (TIdleSupport::IsIntPending())
				{
                // clear idling cpus this will cause all cpus to exit idle handler
                TIdleSupport::SetExitRequired(&iStage2);				           
				}

        SYNCPOINT(iStage2,0x12);			
        if (TIdleSupport::GetExitRequired())
            {
            DoExitIdle(EExitBeforeLPM,cMask,EFalse);
            PMBTRACE4(KIdleeXit,KIdleeXitNormalCpu1,TIdleSupport::GetCpusIdleMask());
            return;
            }

        // We enter LPM here!!!
        TBool exitRouseRequired = EnterLowPowerMode(iLowPowerMode,cMask,EFalse);

        TBool firstToWake = EFalse;
        
        if (exitRouseRequired)
            {
            firstToWake = TIdleSupport::FirstCoreAwake(cMask);
            if (firstToWake)
                {
                // we are probaly first to rouse so wake the others
                PMBTRACE0(KMisc,0x50);
                TIdleSupport::DoIdleIPI (allButC);
                }
            }

        PostWakeup(cMask,EFalse);
        PMBTRACE4(KMisc,3,exitRouseRequired);
        SYNCPOINT(iIdleSync,0x13);  
        if (exitRouseRequired)
            {
            if (!firstToWake)
                {
                TIdleSupport::ClearIdleIPI();
                PMBTRACE0(KMisc,0x51);
                }
            }
        
        DoExitIdle(EExitAfterLPM,cMask,EFalse);    
        PMBTRACE4(KMisc,0x52,TIdleSupport::IntPending()&0x1ff);
        PMBTRACE4(KIdleeXit,KIdleeXitNormalCpu0,TIdleSupport::GetCpusIdleMask());
        }
    }


/**
   Dfc queued on init to delay idle handler entry until after all
   extensions have initialised
*/
void DSMPIdleHandler::StartAfterExtIntDfcFn(TAny* aPtr)
    {
    DSMPIdleHandler* pD = (DSMPIdleHandler*) aPtr;
    pD->iInitialised = ETrue;
    }


#endif //__SMP__
