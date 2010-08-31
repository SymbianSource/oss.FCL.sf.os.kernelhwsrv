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
// os\kernelhwsrv\kernel\eka\drivers\power\smppower\idlehelper.cpp
// Impelentation of helper classes required to implement CPU idle
// functionality in a SMP BSP.

/**
 @file
 @prototype
*/

#include <kernel/arm/arm.h>
#include <smppower/idlehelper.h>

#ifdef __SMP__
//-/-/-/-/-/-/-/-/-/ class TIdleSupport/-/-/-/-/-/-/-/-/-/

TUint TIdleSupport::iGlobalIntDistAddress=0;
TUint TIdleSupport::iBaseIntIfAddress=0;
volatile TUint32* TIdleSupport::iTimerCount=0;
volatile TUint32 TIdleSupport::iIdlingCpus=0;
volatile TUint32 TIdleSupport::iAllEngagedCpusMask=0;
volatile TUint32 TIdleSupport::iRousingCpus=0;
volatile TUint32 TIdleSupport::iExitRequired=EFalse;

/**
   Setup interrupt access for static library  by setting up
   interrupt distributor and CPU interrupt interface addresses
   aGlobalIntDistAddress = interrupt distributor base address
   aBaseIntIfAddress = CPU interrupt base address
   aTimerCount = optional pointer to hw timer counter reg from bsp (only used for btrace)   
   @pre 
 */
 
void TIdleSupport::SetupIdleSupport(TUint32 aGlobalIntDistAddress, TUint32 aBaseIntIfAddress, TUint32* aTimerCount)
	{
	iGlobalIntDistAddress=aGlobalIntDistAddress;
	iBaseIntIfAddress=aBaseIntIfAddress;
	iTimerCount=aTimerCount; /*NULL by default*/
    iAllEngagedCpusMask=AllCpusMask();
	}
/**
   Returns the current HW timer count reg value by default
   Only used for btrace. If this is not set NKern::FastCounter is
   returned.
*/	
	
TUint32 TIdleSupport::GetTimerCount()
	{
    if(iTimerCount)
        return *iTimerCount;
    else
        return NKern::FastCounter();
	}

/**
   Returns TRUE if any interrupt is pending,FALSE otherwise 
*/	

TBool TIdleSupport::IsIntPending()
	{
	return ((TUint32)IntPending()!=KNoInterruptsPending);
	}
		
/**
   Set the piroity of the Idle IPI to be the highest
   @pre 
*/		
	
void TIdleSupport::SetIdleIPIToHighestPriority()
	{	
    // Set Idle IPI to highest priority
    NKern::ThreadEnterCS();
    TInt frz = NKern::FreezeCpu();
    __PM_IDLE_ASSERT_ALWAYS(!frz);
    TInt orig_cpu = NKern::CurrentCpu();
    TInt ncpu = NKern::NumberOfCpus();
    TInt cpu = orig_cpu;
    TUint32 orig_affinity = 0;
    do	
        {
        TUint32 affinity = NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), (TUint32)cpu);
        if (cpu == orig_cpu)
            {
            orig_affinity = affinity;
            NKern::EndFreezeCpu(frz);
            }
        TInt cpu_now = NKern::CurrentCpu();
        __PM_IDLE_ASSERT_ALWAYS(cpu_now == cpu);
          
        // here we can set the priority of the IPI vector for each CPU in turn
        GicDistributor* theGIC = (GicDistributor*) TIdleSupport::iGlobalIntDistAddress;
        TUint8* priorities = (TUint8*) &(theGIC->iPriority);
        priorities[IDLE_WAKEUP_IPI_VECTOR]=0x0;
        __e32_io_completion_barrier();
        if (++cpu == ncpu)
            cpu = 0;
        } while (cpu != orig_cpu);
    NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), orig_affinity);
    NKern::ThreadLeaveCS(); 
	}


/**
   Atomically clears the current cpu idle mask bit to indicate current core has woken 
   up from an interrupt or IPI.   
   return TRUE only if all other cores are in idle and we were woken from an IPI from the last 
   core going idle (otherwisw FALSE). 
   aCpuMask- Bit mask with only current CPU bit set
   Normal usage:use in idle handler after waking from all cores down IPI    
      
   @pre 
 */	
TBool TIdleSupport::ClearLocalAndCheckGlobalIdle(TUint32 aCpuMask)
    {
    return (__e32_atomic_and_ord32(&iIdlingCpus,~aCpuMask) & KGlobalIdleFlag);
    }


/**
	Atomically sets the cpu bit rousing mask only to indicate current CPU has woken. 
	return TRUE only if this is first CPU awake.(otherwise FALSE).
	aCMask- Bit mask with only current CPU bit set	
	Normal usage: use in idle handler just after core is woken
	
   @pre  */		
	
	
TBool TIdleSupport::FirstCoreAwake(TUint32 aCMask)
	{
	//TInt c = NKern::CurrentCpu();
    //TUint32 cMask = (1<<c);//only current cpu mask is set 
	return (!__e32_atomic_ior_acq32(&iRousingCpus,aCMask));	
	}

/**
   Sets the exit required flag in TIdleSupport. Exit required is
   normaly required be set if an interrupt is pending on a Core
   aBreakSyncPoint- TBreakableSyncPoint* that all cores were waiting on
   before interrupt occured. Normal usage: after interrupt pending check	
      
   @pre */
   
void TIdleSupport::SetExitRequired(TBreakableSyncPoint* aBreakSyncPoint)	
	{
	iExitRequired=ETrue;
	if(aBreakSyncPoint)
		aBreakSyncPoint->Break();
	}
	
/**
   Sets the exit required flag in TIdleSupport. Exit required is
   normaly required be set if an interrupt is pending on a Core
   aBreakSyncPoint- TBreakableSyncPoint that all cores were waiting on
   before interrupt occured.  
      
   @pre */	

TBool TIdleSupport::GetExitRequired()
	{
	return iExitRequired;
	}
	
/**
   Resets all the control flags/syncpoints. This is normally done by the 
   last core when all cores are confirmed to be idle.
   
      
   @pre */		
	
void TIdleSupport::ResetLogic()	
	{
    iIdlingCpus = 0;         // clear idle CPUs
    iRousingCpus = 0;         // clear rousing CPUs
    iExitRequired = EFalse; 
	}


/**
   mark a core as retired

   @pre called by idle handler as part of idle entry before 
          any syncpoint or calls to SetLocalAndCheckSetGlobalIdle
*/	
void TIdleSupport::MarkCoreRetired(TUint32 aCpuMask)
    {
    __e32_atomic_and_rlx32(&iAllEngagedCpusMask,~aCpuMask);
    PMBTRACE4(KRetireCore,KRetireMarkCoreRetired,aCpuMask);
    }

/**
   mark a core as enaged
   @pre called outside idle handler ( can be called in idle entry before 
        any syncpoint or calls to SetLocalAndCheckSetGlobalIdle
 */	
void TIdleSupport::MarkCoreEngaged(TUint32 aCpuMask)
    {
    __e32_atomic_ior_rlx32(&iAllEngagedCpusMask,aCpuMask);
    PMBTRACE4(KEngageCore,KEngageMarkCoreEngaged,aCpuMask);
    }

/**
   Returns the current cpu idling bit mask
   @pre */	

TUint32 TIdleSupport::GetCpusIdleMask()
	{
	return iIdlingCpus;
	}

/**
   Returns address of enaged cpus mask, needed for synch point construction

   */	

volatile TUint32* TIdleSupport::EngagedCpusMaskAddr()
    { 
    return &iAllEngagedCpusMask; 
    }

/**
   Returns address of enaged cpus mask, needed for synch point construction

   */	

TUint32 TIdleSupport::AllCpusMask()
    { 
    return ((0x1<<NKern::NumberOfCpus())-1); 
    }

/**
   clears IPI and asserts so in 
   @pre */	
#ifdef _DEBUG
void TIdleSupport::ClearIdleIPI()
    {
    __PM_IDLE_ASSERT_ALWAYS((DoClearIdleIPI()&0x1ff)==IDLE_WAKEUP_IPI_VECTOR);
    }
#endif


//-/-/-/-/-/-/-/-/-/ class TSyncPointBase /-/-/-/-/-/-/-/-/-/
TSyncPointBase::TSyncPointBase()
    :iStageAndCPUWaitingMask(0),
     iAllEnagedCpusMask(TIdleSupport::EngagedCpusMaskAddr())
    {
    }


#ifdef _DEBUG
void TSyncPointBase::SignalAndWait(TUint32 aStage)
    {	
    PMBTRACE8(KSyncPoint,KSignalAndWaitEntry,aStage,*iAllEnagedCpusMask);    
#else
void TSyncPointBase::SignalAndWait()
    {
#endif	
    TInt c = NKern::CurrentCpu();
    DoSW(1<<c);
#ifdef _DEBUG
	PMBTRACE0(KSyncPoint,KSignalAndWaiteXit);	
#endif
    }


/**
   Resets a syncpoint. 
   No barriers are used in function so add them if required. For breakable synchpoints this must be called before sync point can be used, 
   for normal syncpoints this must be called whenever a CPU gets enaged
   @pre Should be called from one CPU. 
 */
void TSyncPointBase::Reset()
    {
    // Could assert it is already broken // not using atomics because this must be called from only one cpu before
    // and be synchronised
    iStageAndCPUWaitingMask = 0;
    }


//-/-/-/-/-/-/-/-/-/ class TBreakableSyncPoint /-/-/-/-/-/-/-/-/-/

/**
   Breaks the sync point until it is reset again. Any attempt to wait on the point will return inmediatelly until the point is reset
 */
void TBreakableSyncPoint::Break()
    {
    __e32_atomic_ior_ord32(&iStageAndCPUWaitingMask,0x80000000);
    }


#endif //__SMP__
