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
// eka/drivers/smppower/sample_idlehandler/smpidlehandler.h
// Example of a generic idle handler layer


#ifndef __SMPIDLEHANDLER_H__
#define __SMPIDLEHANDLER_H__

#include <smppower/idlehelper.h>

#ifdef __SMP__

//Helper class provides a generic idle handler that can derived from. However own idle handler can be used if the version 
//provided here is not appropriate
class DSMPIdleHandler
    {
public:
    // points at which a cpu might exit the idle handler
    enum TIdleExit
        {
        EExitOtherCPUsNotIdle,
        EExitBeforeLPM,
        EExitAfterLPM
        };
    

    DSMPIdleHandler();
	virtual ~DSMPIdleHandler();

    /*
      called to init and bind the idle handler. After this call idle will be directed to idle handler
      @pre thread context, no locks no fast mutexes, interrupt on
    */
    void Initialise(TUint32 aGlobalIntDistAddress, TUint32 aBaseIntIfAddress);
    /**
       Must be called when cores are enaged or retired
       @pre calling code must be outside idle handler, or in DoEnterIdle call
    */
    void ResetSyncPoints();

protected:
    /**
       Called by idle handler inmediatelly after idle entry. Can be used for things 
       such as checking if a core is going to be retired and retiring
       @param aCpuMask mask of current cpu
       @param aStage passed from kernel indicated things such core retiring 
       or postamble
       @param aU points to some per-CPU uncached memory used for handshaking in 
       during power up/power down of a core for support of core retiring. This 
       memory is provided by baseport to the kernel via the VIB

       @return EFalse if the idle handler needs to be exited, ETrue to progress
       further into idle
       @see TIdleSupport::MarkCoreRetired
    */
    virtual TBool DoEnterIdle(TInt aCpuMask, TInt aStage, volatile TAny* aU);
    /**
       Called by idle handler all cpus to enter idle once all have entered idle but
       before NTimeQ::IdleTime is called to check time to next timer expiry. Can be used 
       for things such as idle timer pre-idle processing. In such cases only the last CPU
       should do the idle timer processing
       @param aCpuMask mask of current cpu
       @param aLastCPu indicates if this is last CPU.
    */
    virtual void CpusHaveEnteredIdle(TInt aCpuMask, TInt aLastCpu);
    /**
       This function gets called by the last CPU to go into idle once the system is locked 
       with all CPUs going idle.
       In this function the baseport would fill an opaque integer which determines what low 
       power mode the platform can go to.
       This mode is usually determined by the idle time available, entry and wake latencies 
       of low power modes, and states 
       additionally the function returns EFalse if a there is no point proceeding with idle
       (say next tick is very close for example)
       of other resources in the system
       @param aIdleTime time until next timer is due as obtained from NTimerQ::IdleTime
       @param aLowPowerMode low powe mode to go into, up to baseport on meaning of this 
       @return EFalse to force exit from idle handler if there is no point proceeding with idle
       @see NTimerQ::IdleTIme
     */
    virtual TBool GetLowPowerMode(TInt aIdleTime, TInt &aLowPowerMode) = 0;
    /**
       This function actually enters the low power mode. It should do any state saving and perform
       idle tick suppression. If rousing of other CPUs is required after wake then this function should
       return true. 
       
       @param aMode the low power mode obtained from GetLowPowerMode
       @param TInt aCpuMask indicates calling CPU
       @param aLastCpu true if the calling CPU was the last to enter the idle thread
       @return True if rousing of other cores is required after wakeup
     */
    virtual TBool EnterLowPowerMode(TInt aMode, TInt aCpuMask, TBool aLastCpu) = 0;
    /**
       Called after wakeup can be used for status restoring
       a sync point is placed after the call to this function
       This can be a good place to do idle tick restoration
       @param TInt aCpuMask indicates calling CPU
       @param aLastCPu indicates if this is last CPU.
       @see EnterLowPowerMode
    */
    virtual void PostWakeup(TInt aCpuMask, TBool aLastCpu);
    /**
       Called at exit of idle handler not synchronised in any way
       @param aExitPoint point at which you might exit the idle handler
       @param TInt aCpuMask indicates calling CPU
       @param aLastCPu indicates if this is last CPU.
    */
    virtual void DoExitIdle(TIdleExit aExitPoint, TInt aCpuMask, TBool aLastCpu);

private:
    static void IdleHandler(TAny* aPtr, TInt aStage, volatile TAny* aU);
    void DoIdle(TInt aStage, volatile TAny* aU);
    static void StartAfterExtIntDfcFn(TAny*);

private:
 	const TUint32 iAllCpusMask; 
	TSyncPoint iIdleSync;
	TBreakableSyncPoint iStage2;
	volatile TUint32 iIdlingCpus;
	volatile TUint32 iExitRequired;
	TInt iLowPowerMode;
    TDfc iStartAfterExtInitDfc;
    TBool iInitialised;
    };
    


#endif // __SMP__


#endif //__SMPIDLEHANDLER_H__
