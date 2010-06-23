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
// os\kernelhwsrv\kernel\eka\include\drivers\smppower\idlehelper.h
// Helper classes required to implement CPU idle
// functionality in a SMP BSP.


/**
 @file
 @prototype
*/

#ifndef __SMP_IDLE_HELPER_H__
#define __SMP_IDLE_HELPER_H__

#define __PM_IDLE_ASSERT_ALWAYS(aCond) \
	__ASSERT_ALWAYS( (aCond), \
		( \
			Kern::Printf("Assertion '" #aCond "' failed;\nFile: '" __FILE__ "' Line: %d\n", __LINE__), \
			Kern::Fault("TIdleSupport", 1) \
		) )

#define __PM_IDLE_ASSERT_DEBUG(aCond) \
	__ASSERT_DEBUG( (aCond), \
		( \
			Kern::Printf("Assertion '" #aCond "' failed;\nFile: '" __FILE__ "' Line: %d\n", __LINE__), \
			Kern::Fault("TIdleSupport", 1) \
		) )



#ifdef __SMP__

#include <kernel/kpower.h>
#include <e32btrace.h>
#include <arm_gic.h>
#include <kernel.h>
#include <nk_priv.h>
#include <nk_plat.h>


const TUint32 KNoInterruptsPending = 1023;


// Temp place holder for TRACE Categories
const TInt KIsrPendingCat = 128;
const TInt KPrintReg = 129;
const TInt KIdleEntry = 130;
const TUint8 KIdleEntryNormalCpu = 0x10;
const TUint8 KIdleEntryLastCpu = 0;
const TInt KIdleeXit = 131;
const TUint8 KIdleeXitLastCpu0 = 0x1;
const TUint8 KIdleeXitLastCpu1 = 0x1;
const TUint8 KIdleeXitNormalCpu0 = 0x10;
const TUint8 KIdleeXitNormalCpu1 = 0x11;
const TInt KSyncPoint = 132;
const TUint8 KSignalAndWaitFnEntry = 0x10;
const TUint8 KSignalAndWaitFneXit = 0x11;
const TUint8 KSignalAndWaitEntry = 0x0;
const TUint8 KSignalAndWaiteXit = 0x1;
const TInt KClearIPI = 133;
const TInt KSendIPI = 134;
const TInt KMisc=135;
const TInt KIdleTickSupression=136;
const TUint8 KCyclesInTickCyclesFullTick = 0x0;
const TUint8 KNextInterrupt = 0x1;
const TUint8 KTimeSleptTimeNextInt = 0x2;
const TUint8 KTIcksSlept = 0x3;
const TInt  KRetireCore=137;
const TUint8 KRetireCoreEntry = 0x1;
const TUint8 KRetireCoreeXit = 0x2;
const TUint8 KRetireMarkCoreRetired = 0x3;
const TInt  KEngageCore=138;
const TUint8 KEngageMarkCoreEngaged = 0x3;
const TInt KNTICK = 139;

// End of Trace categories

//#define DISABLE_TRACE
#if defined(_DEBUG) && !defined(DISABLE_TRACE)

#define PMBTRACE0(c,s) BTrace0((c),(s))
#define PMBTRACE4(c,s,a1) BTrace4((c),(s),(a1))
#define PMBTRACE8(c,s,a1,a2) BTrace8((c),(s),(a1),(a2))

#else

#define PMBTRACE0(c,s)
#define PMBTRACE4(c,s,a1) 
#define PMBTRACE8(c,s,a1,a2) 

#endif

//This will be defined in kernel header file nk_plat.h in the future
#ifndef IDLE_WAKEUP_IPI_VECTOR
#define IDLE_WAKEUP_IPI_VECTOR 0x07
#endif


#ifdef _DEBUG

#define SYNCPOINT(obj,stage) (obj).SignalAndWait(stage) 

#else

#define SYNCPOINT(obj,stage) (obj).SignalAndWait()

#endif//_DEBUG

#define PROPER_WFI		//if defined uses ARM_WFI state otherwise a wait in loop
#define SYNCPOINT_WFE 	//Sync Points use WFE and SEV

//Base class for all sync points
class TSyncPointBase
    {
public:
    TSyncPointBase();
#ifdef _DEBUG
    void SignalAndWait(TUint32 aStage);
#else
    void SignalAndWait();
#endif
    void Reset();
protected:
    virtual void DoSW(TUint32 aCpuMask) = 0;
public:
    volatile TUint32 iStageAndCPUWaitingMask;    // upper 16 are the stage and lower 16 are cpus waiting mask
    volatile TUint32* iAllEnagedCpusMask;
   };

    
// Auto reseting sync point. Can not be broken. 
class TSyncPoint : public TSyncPointBase
    {
private:
    void DoSW(TUint32 aCpuMask);
    };

// Very similar to normal breakpoint except that:
// 1. It does not autoreset between calls, therefore a call to Reset is required between
//    syncs.
// 2. It can be broken. That is a call to Break will result in any cpu currently waiting on the point
//    to be freed inmediatelly, and attempt to start to wait on the point to return inmediatelly. This
//    condition remains until reset is called once more
class TBreakableSyncPoint : public TSyncPointBase
    {
public:
    void Break();
private:
    void DoSW(TUint32 aCpuMask);
};

//Helper class for idle handler support to be used in smp bsp
class TIdleSupport
	{
public:
	static void SetupIdleSupport(TUint32 aGlobalIntDistAddress, TUint32 aBaseIntIfAddress, TUint32* aTimerCount=0);//setup GIC gid and cif base addresses
	static void SetIdleIPIToHighestPriority();//sets idle IPI priority to be the highest
	static void DoIdleIPI(TUint32);
	static void ClearIdleIPI();
	static void DoWFI();//puts current CPU in wait for interrupt state
	static TBool IsIntPending();
	static TInt	IntPending();
	static TUint32 GetTimerCount();//HW timer can be used for tracing
	//Atomic checks used to synchronise cores going idle
	static TBool ClearLocalAndCheckGlobalIdle(TUint32);
	static TBool SetLocalAndCheckSetGlobalIdle(TUint32);
	static TBool FirstCoreAwake(TUint32);
    // Retiring Cores
    static void MarkCoreRetired(TUint32);
    static void MarkCoreEngaged(TUint32);
	//Exit methods for sync points
	static void SetExitRequired(TBreakableSyncPoint* aBreakSyncPoint=0);
	static TBool GetExitRequired();	
	static void  ResetLogic();//Reset helper class flags
	static TUint32 GetCpusIdleMask();//gets bit mask containing idling CPU's
    static volatile TUint32* EngagedCpusMaskAddr();
    static TUint32 AllCpusMask();
private:
    static TInt DoClearIdleIPI();
private:
    static const TUint32 KGlobalIdleFlag = 0x8000000;
	static volatile TUint32 iAllEngagedCpusMask;
	static volatile TUint32 iIdlingCpus;//contains CPU's idle handler waiting to go idle
    static volatile TUint32 iRousingCpus;////contains CPU's waking up
	static volatile TUint32 iExitRequired;
	static TUint iGlobalIntDistAddress;//base address of gloabl interrupt dispatcher
	static TUint iBaseIntIfAddress;//base address of CPu interrupt interface
	static volatile TUint32* iTimerCount;//timer count register provided by bsp for tracing	
	};


#endif //__SMP__

#endif //__SMP_IDLE_HELPER_H__

