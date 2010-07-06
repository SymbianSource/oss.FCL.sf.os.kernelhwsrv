// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32cia.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __E32CIA_H__
#define __E32CIA_H__

// CIA symbols for USER side code
// CIA symbol macros for Gcc98r2
#if defined(__GCC32__)
#define CSM_ZN4Exec12PopTrapFrameEv " PopTrapFrame__4Exec"
#define CSM_ZN4User7ReAllocEPvii " ReAlloc__4UserPvii"
#define CSM_Z30PanicCObjectConIndexOutOfRangev " PanicCObjectConIndexOutOfRange__Fv"
#define CSM_ZN4User15HandleExceptionEPv " HandleException__4UserPv"
#define CSM_ZN4User12AfterHighResE27TTimeIntervalMicroSeconds32 " AfterHighRes__4UserG27TTimeIntervalMicroSeconds32"
#define CSM_ZN4Exec13ServerReceiveEiR14TRequestStatusPv " ServerReceive__4ExeciR14TRequestStatusPv"
#define CSM_ZN5TTrap6UnTrapEv " UnTrap__5TTrap"
#define CSM_Z15PanicStrayEventv " PanicStrayEvent__Fv"
#define CSM_ZN8CServer210BadMessageERK9RMessage2 " BadMessage__8CServer2RC9RMessage2"
#define CSM_ZN10RSemaphore4WaitEv " Wait__10RSemaphore"
#define CSM_ZN10RSemaphore4WaitEi " Wait__10RSemaphorei"
#define CSM_Z34PanicCObjectConFindIndexOutOfRangev " PanicCObjectConFindIndexOutOfRange__Fv"
#define CSM_ZN7TRegion5ClearEv " Clear__7TRegion"
#define CSM_ZN4User5AllocEi " Alloc__4Useri"
#define CSM_ZN4User5LeaveEi " Leave__4Useri"
#define CSM_ZN4Exec13PushTrapFrameEP5TTrap " PushTrapFrame__4ExecP5TTrap"
#define CSM_ZN8CServer213PolicyActionLERK9RMessage2RKNS_14TPolicyElementE " PolicyActionL__8CServer2RC9RMessage2RCQ28CServer214TPolicyElement"
#define CSM_Z16AllocAnotherRectP7TRegion " AllocAnotherRect__FP7TRegion"
#define CSM_ZN7TRegion7AddRectERK5TRect " AddRect__7TRegionRC5TRect"
#define CSM_Z28PanicCObjectConFindBadHandlev " PanicCObjectConFindBadHandle__Fv"
#define CSM_ZN10RSemaphore6SignalEv " Signal__10RSemaphore"
#define CSM_ZN8CServer212NotConnectedERK9RMessage2 " NotConnected__8CServer2RC9RMessage2"
#define CSM_ZN7TRegion11SetListSizeEi " SetListSize__7TRegioni"
#define CSM_ZN4User9InvariantEv " Invariant__4User"
#define CSM_ZN8CServer210DisconnectERK9RMessage2 " Disconnect__8CServer2RC9RMessage2"
#define CSM_Z16PanicNoTrapFramev " PanicNoTrapFrame__Fv"
#define CSM_ZN7TRegion10ForceErrorEv " ForceError__7TRegion"
#define CSM_ZN5TTrap4TrapERi " Trap__5TTrapRi"
#define CSM_Z29PanicCObjectIxIndexOutOfRangev " PanicCObjectIxIndexOutOfRange__Fv"
#define CSM_Z30PanicOverUnderflowDividebyZeroi " PanicOverUnderflowDividebyZero__Fi"
#define CSM_ZN8CServer27ConnectERK9RMessage2 " Connect__8CServer2RC9RMessage2"
#define CSM_ZN8CCleanup5CheckEPv " Check__8CCleanupPv"
#define CSM_ZN2PP20InitSuperPageFromRom " InitSuperPageFromRom__2PPUlUl"
#elif defined(__ARMCC__)
// CIA symbol macros for RVCT
#define CSM_ZN4Exec12PopTrapFrameEv " __cpp(Exec::PopTrapFrame)"
#define CSM_ZN4User7ReAllocEPvii " __cpp(User::ReAlloc)"
#define CSM_Z30PanicCObjectConIndexOutOfRangev " __cpp(PanicCObjectConIndexOutOfRange)"
#define CSM_ZN4User15HandleExceptionEPv " __cpp(User::HandleException)"
#define CSM_ZN4User12AfterHighResE27TTimeIntervalMicroSeconds32 " __cpp(User::AfterHighRes)"
#define CSM_ZN4Exec13ServerReceiveEiR14TRequestStatusPv " __cpp(Exec::ServerReceive)"
#define CSM_ZN5TTrap6UnTrapEv " __cpp(TTrap::UnTrap)"
#define CSM_Z15PanicStrayEventv " __cpp(PanicStrayEvent)"
#define CSM_ZN8CServer210BadMessageERK9RMessage2 " __cpp(CServer2::BadMessage)"
#define CSM_ZN10RSemaphore4WaitEv " __cpp(static_cast<void (RSemaphore::*) ()>(&RSemaphore::Wait))"
#define CSM_ZN10RSemaphore4WaitEi " __cpp(static_cast<TInt (RSemaphore::*) (TInt)>(&RSemaphore::Wait))"
#define CSM_Z34PanicCObjectConFindIndexOutOfRangev " __cpp(PanicCObjectConFindIndexOutOfRange)"
#define CSM_ZN7TRegion5ClearEv " __cpp(TRegion::Clear)"
#define CSM_ZN4User5AllocEi " __cpp(User::Alloc)"
#define CSM_ZN4User5LeaveEi " __cpp(User::Leave)"
#define CSM_ZN4Exec13PushTrapFrameEP5TTrap " __cpp(Exec::PushTrapFrame)"
#define CSM_ZN8CServer213PolicyActionLERK9RMessage2RKNS_14TPolicyElementE " __cpp(CServer2::PolicyActionL)"
#define CSM_Z16AllocAnotherRectP7TRegion " __cpp(AllocAnotherRect)"
#define CSM_ZN7TRegion7AddRectERK5TRect " __cpp(TRegion::AddRect)"
#define CSM_Z28PanicCObjectConFindBadHandlev " __cpp(PanicCObjectConFindBadHandle)"
#define CSM_ZN10RSemaphore6SignalEv " __cpp(static_cast<void (RSemaphore::*) ()>(&RSemaphore::Signal))"
#define CSM_ZN8CServer212NotConnectedERK9RMessage2 " __cpp(CServer2::NotConnected)"
#define CSM_ZN7TRegion11SetListSizeEi " __cpp(TRegion::SetListSize)"
#define CSM_ZN4User9InvariantEv " __cpp(User::Invariant)"
#define CSM_ZN8CServer210DisconnectERK9RMessage2 " __cpp(CServer2::Disconnect)"
#define CSM_Z16PanicNoTrapFramev " __cpp(PanicNoTrapFrame)"
#define CSM_ZN7TRegion10ForceErrorEv " __cpp(TRegion::ForceError)"
#define CSM_ZN5TTrap4TrapERi " __cpp(TTrap::Trap)"
#define CSM_Z29PanicCObjectIxIndexOutOfRangev " __cpp(PanicCObjectIxIndexOutOfRange)"
#define CSM_Z30PanicOverUnderflowDividebyZeroi " __cpp(PanicOverUnderflowDividebyZero)"
#define CSM_ZN8CServer27ConnectERK9RMessage2 " __cpp(CServer2::Connect)"
#define CSM_ZN8CCleanup5CheckEPv " __cpp(&CCleanup::Check)"
#else
// CIA symbol macros for EABI assemblers
#define CSM_ZN4Exec12PopTrapFrameEv " _ZN4Exec12PopTrapFrameEv"
#define CSM_ZN4User7ReAllocEPvii " _ZN4User7ReAllocEPvii"
#define CSM_Z30PanicCObjectConIndexOutOfRangev " _Z30PanicCObjectConIndexOutOfRangev"
#define CSM_ZN4User15HandleExceptionEPv " _ZN4User15HandleExceptionEPv"
#define CSM_ZN4User12AfterHighResE27TTimeIntervalMicroSeconds32 " _ZN4User12AfterHighResE27TTimeIntervalMicroSeconds32"
#define CSM_ZN4Exec13ServerReceiveEiR14TRequestStatusPv " _ZN4Exec13ServerReceiveEiR14TRequestStatusPv"
#define CSM_ZN5TTrap6UnTrapEv " _ZN5TTrap6UnTrapEv"
#define CSM_Z15PanicStrayEventv " _Z15PanicStrayEventv"
#define CSM_ZN8CServer210BadMessageERK9RMessage2 " _ZN8CServer210BadMessageERK9RMessage2"
#define CSM_ZN10RSemaphore4WaitEv " _ZN10RSemaphore4WaitEv"
#define CSM_ZN10RSemaphore4WaitEi " _ZN10RSemaphore4WaitEi"
#define CSM_Z34PanicCObjectConFindIndexOutOfRangev " _Z34PanicCObjectConFindIndexOutOfRangev"
#define CSM_ZN7TRegion5ClearEv " _ZN7TRegion5ClearEv"
#define CSM_ZN4User5AllocEi " _ZN4User5AllocEi"
#define CSM_ZN4User5LeaveEi " _ZN4User5LeaveEi"
#define CSM_ZN4Exec13PushTrapFrameEP5TTrap " _ZN4Exec13PushTrapFrameEP5TTrap"
#define CSM_ZN8CServer213PolicyActionLERK9RMessage2RKNS_14TPolicyElementE " _ZN8CServer213PolicyActionLERK9RMessage2RKNS_14TPolicyElementE"
#define CSM_Z16AllocAnotherRectP7TRegion " _Z16AllocAnotherRectP7TRegion"
#define CSM_ZN7TRegion7AddRectERK5TRect " _ZN7TRegion7AddRectERK5TRect"
#define CSM_Z28PanicCObjectConFindBadHandlev " _Z28PanicCObjectConFindBadHandlev"
#define CSM_ZN10RSemaphore6SignalEv " _ZN10RSemaphore6SignalEv"
#define CSM_ZN8CServer212NotConnectedERK9RMessage2 " _ZN8CServer212NotConnectedERK9RMessage2"
#define CSM_ZN7TRegion11SetListSizeEi " _ZN7TRegion11SetListSizeEi"
#define CSM_ZN4User9InvariantEv " _ZN4User9InvariantEv"
#define CSM_ZN8CServer210DisconnectERK9RMessage2 " _ZN8CServer210DisconnectERK9RMessage2"
#define CSM_Z16PanicNoTrapFramev " _Z16PanicNoTrapFramev"
#define CSM_ZN7TRegion10ForceErrorEv " _ZN7TRegion10ForceErrorEv"
#define CSM_ZN5TTrap4TrapERi " _ZN5TTrap4TrapERi"
#define CSM_Z29PanicCObjectIxIndexOutOfRangev " _Z29PanicCObjectIxIndexOutOfRangev"
#define CSM_Z30PanicOverUnderflowDividebyZeroi " _Z30PanicOverUnderflowDividebyZeroi"
#define CSM_ZN8CServer27ConnectERK9RMessage2 " _ZN8CServer27ConnectERK9RMessage2"
#define CSM_ZN8CCleanup5CheckEPv " _ZN8CCleanup5CheckEPv"
#endif

// CIA symbols for KERNEL side code
// CIA symbol macros for Gcc98r2
#if defined(__GCC32__)
#define CSM_ZN1K8MsgInfoE " _1K.MsgInfo"
#define CSM_ZN14NFastSemaphore6SignalEv " Signal__14NFastSemaphore"
#define CSM_ZN4Kern4ExitEi " Exit__4Kerni"
#define CSM_ZN5NKern4ExitEv " Exit__5NKern"
#define CSM_ZN11NThreadBase12DoCsFunctionEv " DoCsFunction__11NThreadBase"
#define CSM_Z24KUDesInfoPanicBadDesTypev " KUDesInfoPanicBadDesType__Fv"
#define CSM_Z29KUDesSetLengthPanicBadDesTypev " KUDesSetLengthPanicBadDesType__Fv"
#define CSM_ZN1K27LockedPlatformSecurityPanicEv " LockedPlatformSecurityPanic__1K"
#define CSM_ZN4Kern8SafeReadEPKvPvi " SafeRead__4KernPCvPvi"
#define CSM_ZN5NKern4LockEv " Lock__5NKern"
#define CSM_ZN5NKern15PreemptionPointEv " PreemptionPoint__5NKern"
#define CSM_ZN5Cache16AtomicSyncMemoryEv " AtomicSyncMemory__5Cache"
#define CSM_ZN11NThreadBase4ExitEv " Exit__11NThreadBase"
#define CSM_ZN5NKern19ThreadRequestSignalEP7NThreadP10NFastMutex " ThreadRequestSignal__5NKernP7NThreadP10NFastMutex"
#define CSM_ZN5NKern6UnlockEv " Unlock__5NKern"
#define CSM_Z21PanicSyncMsgSentTwicev " PanicSyncMsgSentTwice__Fv"
#define CSM_ZN8DSession12AllocAndSendEiPKiP14TRequestStatus " AllocAndSend__8DSessioniPCiP14TRequestStatus"
#define CSM_Z20FastMutexSignalErrorv " FastMutexSignalError__Fv"
#define CSM_Z27KUDesSetLengthPanicOverflowv " KUDesSetLengthPanicOverflow__Fv"
#define CSM_ZN5NKern11FlashSystemEv " FlashSystem__5NKern"
#define CSM_ZN16CacheMaintenance15OnProcessSwitchEv " OnProcessSwitch__16CacheMaintenance"
#define CSM_ZN1K18PanicCurrentThreadEi " PanicCurrentThread__1Ki"
#define CSM_ZN13DSessionShare17CloseOnCompletionEv " CloseOnCompletion__13DSessionShare"
#define CSM_ZN10NFastMutex6SignalEv " Signal__10NFastMutex"
#define CSM_ZN10NFastMutex4WaitEv " Wait__10NFastMutex"
#define CSM_Z9TBmaFaulti " TBmaFault__Fi"
#define CSM_ZN4Kern6PrintfEPKcz " Printf__4KernPCce"
#define CSM_ZN5TDes89SetLengthEi " SetLength__5TDes8i"
#define CSM_Z20FastMutexNestAttemptv " FastMutexNestAttempt__Fv"
#define CSM_ZN8DSession14SendOnNewShareEP9RMessageKi " SendOnNewShare__8DSessionP9RMessageKi"
#define CSM_ZN1K20TheFileServerProcessE " _1K.TheFileServerProcess"
#define CSM_ZN5NKern12UnlockSystemEv " UnlockSystem__5NKern"
#define CSM_ZN2PP17MonitorEntryPointE " _2PP.MonitorEntryPoint"
#define CSM_ZN4Kern9SafeWriteEPvPKvi " SafeWrite__4KernPvPCvi"
#define CSM_Z25CompleteDisconnectMessageR9RMessageK " CompleteDisconnectMessage__FR9RMessageK"
#define CSM_ZN1K5FaultENS_6TFaultE " Fault__1KQ21K6TFault"
#define CSM_ZN4TDfc3AddEv " Add__4TDfc"
#define CSM_ZN4TDfc6RawAddEv " RawAdd__4TDfc"
#define CSM_ZN10TScheduler7YieldToEP11NThreadBase " YieldTo__10TSchedulerP11NThreadBase"
#define CSM_Z14CheckLockStatev " CheckLockState__Fv"
#define CSM_Z18InvalidExecHandlerv " InvalidExecHandler__Fv"
#define CSM_Z15InvalidFastExecv " InvalidFastExec__Fv"
#define CSM_ZN2PP11NanoWaitCalE " _2PP.NanoWaitCal"
#define CSM_Z22PanicMesAlreadyPendingv " PanicMesAlreadyPending__Fv"
#define CSM_ZN10TScheduler10RescheduleEv " Reschedule__10TScheduler"
#define CSM_ZN10TScheduler9QueueDfcsEv " QueueDfcs__10TScheduler"
#define CSM_ZN13TSubScheduler9QueueDfcsEv " QueueDfcs__13TSubScheduler"
#define	CSM_ZN13TSubScheduler16SelectNextThreadEv " SelectNextThread__13TSubScheduler"
#define CSM_ZN5NKern20DisableAllInterruptsEv " DisableAllInterrupts__5NKern"
#define CSM_ZN5NKern10LockSystemEv " LockSystem__5NKern"
#define CSM_ZN7Monitor4InitEPvi " Init__7MonitorPvi"
#define CSM_ZN7Monitor5LeaveEi " Leave__7Monitori"
#define CSM_Z10KernelMainv " KernelMain__Fv"
#define CSM_ZN4Kern9SuperPageEv " SuperPage__4Kern"
#define CSM_ZN8DSession19CloseFromDisconnectEv " CloseFromDisconnect__8DSession"
#define	CSM_ZN5NKern13ThreadEnterCSEv " ThreadEnterCS__5NKern"
#define	CSM_ZN5NKern13ThreadLeaveCSEv " ThreadLeaveCS__5NKern"
#define	CSM_ZN5NKern14_ThreadEnterCSEv " _ThreadEnterCS__5NKern"
#define	CSM_ZN5NKern14_ThreadLeaveCSEv " _ThreadLeaveCS__5NKern"
#define	CSM_ZN7DObject10AsyncCloseEv " AsyncClose__7DObject"
#define CSM_ZN3EMI16CallStartHandlerEP7DThread " CallStartHandler__3EMIP7DThread"
#define CSM_ZN2KL5PanicENS_13TKernLibPanicE " Panic__2KLQ22KL13TKernLibPanic"
#define CSM_ZN5NKern11FastCounterEv " FastCounter__5NKern"
#define CSM_ZN4TDfc7DoEnqueEv " DoEnque__4TDfc"
#define CSM_ZN6BTrace8DoOutBigEmmPKvimm "DoOutBig__6BTraceUlUlPCviUlUl"
#define CSM_ZN6BTrace4OutXEmmmm "OutX__6BTraceUlUlUlUl"
#define CSM_ZN6BTrace3OutEmmmm "Out__6BTraceUlUlUlUl"
#define CSM_ZN5NKern9TickCountEv " TickCount__5NKern"
#define CSM_ZN3Mmu14MoveKernelPageEP6DChunkmmRmji "MoveKernelPage__3MmuP6DChunkUlUlRUlUii"
#define CSM_ZN12TPriListBase15HighestPriorityEv "HighestPriority__12TPriListBase"
#define CSM_ZN12TPriListBase5FirstEv "First__12TPriListBase"
#define CSM_ZN12TPriListBase3AddEP12TPriListLink "Add__12TPriListBaseP12TPriListLink"
#define	CSM_ZN12TPriListBase6RemoveEP12TPriListLink "Remove__12TPriListBaseP12TPriListLink"
#define	CSM_ZN12TPriListBase14ChangePriorityEP12TPriListLinki "ChangePriority__12TPriListBaseP12TPriListLinki"
#define CSM_ZN4Kern9AsyncFreeEPv "AsyncFree__4KernPv"
#define CSM_ZN9RMessageK4FreeEv "Free__9RMessageK"
#define CSM_ZN1K9ObjDeleteEP7DObject "ObjDelete__1KP7DObject"
#define CSM_ZN5NKern19ThreadRequestSignalEP7NThread "ThreadRequestSignal__5NKernP7NThread"
#define CSM_ZN1K16ObjectFromHandleEii "ObjectFromHandle__1Kii"
#define CSM_ZN5NKern6FMWaitEP10NFastMutex "FMWait__5NKernP10NFastMutex"
#define CSM_ZN5NKern8FMSignalEP10NFastMutex "FMSignal__5NKernP10NFastMutex"
#define CSM_ZN14TClientRequest9SetStatusEP14TRequestStatus "SetStatus__14TClientRequestP14TRequestStatus"
#define	CSM_ZN9TSpinLock7LockIrqEv "LockIrq__9TSpinLock"
#define	CSM_ZN9TSpinLock9UnlockIrqEv "UnlockIrq__9TSpinLock"
#define	CSM_ZN9TSpinLock8LockOnlyEv "LockOnly__9TSpinLock"
#define	CSM_ZN9TSpinLock10UnlockOnlyEv "UnlockOnly__9TSpinLock"
#define	CSM_ZN9TSpinLock11LockIrqSaveEv "LockIrqSave__9TSpinLock"
#define	CSM_ZN9TSpinLock16UnlockIrqRestoreEi "UnlockIrqRestore__9TSpinLocki"
#define	CSM_ZN11TRWSpinLock8LockIrqREv "LockIrqR__11TRWSpinLock"
#define	CSM_ZN11TRWSpinLock10UnlockIrqREv "UnlockIrqR__11TRWSpinLock"
#define	CSM_ZN11TRWSpinLock9LockOnlyREv "LockOnlyR__11TRWSpinLock"
#define	CSM_ZN11TRWSpinLock11UnlockOnlyREv "UnlockOnlyR__11TRWSpinLock"
#define	CSM_ZN11TRWSpinLock17UnlockIrqRestoreREi "UnlockIrqRestoreR__11TRWSpinLocki"
#define	CSM_ZN11TRWSpinLock8LockIrqWEv "LockIrqW__11TRWSpinLock"
#define	CSM_ZN11TRWSpinLock10UnlockIrqWEv "UnlockIrqW__11TRWSpinLock"
#define	CSM_ZN11TRWSpinLock9LockOnlyWEv "LockOnlyW__11TRWSpinLock"
#define	CSM_ZN11TRWSpinLock11UnlockOnlyWEv "UnlockOnlyW__11TRWSpinLock"
#define	CSM_ZN11TRWSpinLock17UnlockIrqRestoreWEi "UnlockIrqRestoreW__11TRWSpinLocki"
#define	CSM_ZN3Arm9SaveStateER14SFullArmRegSet "SaveState__3ArmR14SFullArmRegSet"
#define	CSM_ZN3Arm3RegER14SFullArmRegSetim "Reg__3ArmR14SFullArmRegSetiUl"
#define	CSM_Z9KDebugNumi " KDebugNum__Fi"
#define CSM_ZN5NKern9TimestampEv " Timestamp__5NKern"
#define	CSM_ZN5NKern18TimestampFrequencyEv " TimestampFrequency__5NKern"
#define CSM_ZN6NTimer7OneShotEii " OneShot__6NTimerii"
#define CSM_ZN4Kern20QueueRequestCompleteEP7DThreadP14TClientRequesti "QueueRequestComplete__4KernP7DThreadP14TClientRequesti"
#define CSM_ZN1M18IsDataPagedAddressEPKvRi "IsDataPagedAddress__1MPCvRi"
#define CSM_ZN8DSession14PinDescriptorsEPiPA3_mPP17TVirtualPinObject "PinDescriptors__8DSessionPiPA2_UlPP17TVirtualPinObject"
#define CSM_ZN14TClientRequest5ResetEv "Reset__14TClientRequest"
#define CSM_ZN9RMessageK8CloseRefEv "CloseRef__9RMessageK"
#elif defined(__ARMCC__)
// CIA symbol macros for RVCT
#define CSM_ZN1K8MsgInfoE " __cpp(&K::MsgInfo)"
#define CSM_ZN14NFastSemaphore6SignalEv " __cpp(NFastSemaphore::Signal)"
#define CSM_ZN4Kern4ExitEi " __cpp(Kern::Exit)"
#define CSM_ZN5NKern4ExitEv " __cpp(NKern::Exit)"
#define CSM_ZN11NThreadBase12DoCsFunctionEv " __cpp(NThreadBase::DoCsFunction)"
#define CSM_Z24KUDesInfoPanicBadDesTypev " __cpp(KUDesInfoPanicBadDesType)"
#define CSM_Z29KUDesSetLengthPanicBadDesTypev " __cpp(KUDesSetLengthPanicBadDesType)"
#define CSM_ZN1K27LockedPlatformSecurityPanicEv " __cpp(K::LockedPlatformSecurityPanic)"
#define CSM_ZN4Kern8SafeReadEPKvPvi " __cpp(Kern::SafeRead)"
#define CSM_ZN5NKern4LockEv " __cpp(NKern::Lock)"
#define CSM_ZN5NKern15PreemptionPointEv " __cpp(NKern::PreemptionPoint)"
#define CSM_ZN5Cache16AtomicSyncMemoryEv " __cpp(static_cast<void (*) ()>(&Cache::AtomicSyncMemory))"
#define CSM_ZN11NThreadBase4ExitEv " __cpp(NThreadBase::Exit)"
#define CSM_ZN5NKern19ThreadRequestSignalEP7NThreadP10NFastMutex " __cpp(static_cast<void (*) (NThread *, NFastMutex *)>(&NKern::ThreadRequestSignal))"
#define CSM_ZN5NKern6UnlockEv " __cpp(NKern::Unlock)"
#define CSM_Z21PanicSyncMsgSentTwicev " __cpp(PanicSyncMsgSentTwice)"
#define CSM_ZN8DSession12AllocAndSendEiPKiP14TRequestStatus " __cpp(DSession::AllocAndSend)"
#define CSM_Z20FastMutexSignalErrorv " __cpp(FastMutexSignalError)"
#define CSM_Z27KUDesSetLengthPanicOverflowv " __cpp(KUDesSetLengthPanicOverflow)"
#define CSM_ZN5NKern11FlashSystemEv " __cpp(NKern::FlashSystem)"
#define CSM_ZN16CacheMaintenance15OnProcessSwitchEv " __cpp(CacheMaintenance::OnProcessSwitch)"
#define CSM_ZN1K18PanicCurrentThreadEi " __cpp(K::PanicCurrentThread)"
#define CSM_ZN13DSessionShare17CloseOnCompletionEv " __cpp(DSessionShare::CloseOnCompletion)"
#define CSM_ZN10NFastMutex6SignalEv " __cpp(NFastMutex::Signal)"
#define CSM_ZN10NFastMutex4WaitEv " __cpp(NFastMutex::Wait)"
#define CSM_Z9TBmaFaulti " __cpp(TBmaFault)"
#define CSM_ZN4Kern6PrintfEPKcz " __cpp(Kern::Printf)"
#define CSM_ZN5TDes89SetLengthEi " __cpp(TDes8::SetLength)"
#define CSM_Z20FastMutexNestAttemptv " __cpp(FastMutexNestAttempt)"
#define CSM_ZN8DSession14SendOnNewShareEP9RMessageKi " __cpp(DSession::SendOnNewShare)"
#define CSM_ZN1K20TheFileServerProcessE " __cpp(&K::TheFileServerProcess)"
#define CSM_ZN5NKern12UnlockSystemEv " __cpp(NKern::UnlockSystem)"
#define CSM_ZN2PP17MonitorEntryPointE " __cpp(&PP::MonitorEntryPoint)"
#define CSM_ZN4Kern9SafeWriteEPvPKvi " __cpp(Kern::SafeWrite)"
#define CSM_Z25CompleteDisconnectMessageR9RMessageK " __cpp(CompleteDisconnectMessage)"
#define CSM_ZN1K5FaultENS_6TFaultE " __cpp(K::Fault)"
#define CSM_ZN4TDfc3AddEv " __cpp(TDfc::Add)"
#define CSM_ZN4TDfc6RawAddEv " __cpp(TDfc::RawAdd)"
#define CSM_ZN10TScheduler7YieldToEP11NThreadBase " __cpp(TScheduler::YieldTo)"
#define CSM_Z14CheckLockStatev " __cpp(CheckLockState)"
#define CSM_Z18InvalidExecHandlerv " __cpp(InvalidExecHandler)"
#define CSM_Z15InvalidFastExecv " __cpp(InvalidFastExec)"
#define CSM_ZN2PP11NanoWaitCalE " __cpp(&PP::NanoWaitCal)"
#define CSM_Z22PanicMesAlreadyPendingv " __cpp(PanicMesAlreadyPending)"
#define CSM_ZN10TScheduler10RescheduleEv " __cpp(TScheduler::Reschedule)"
#define CSM_ZN10TScheduler9QueueDfcsEv " __cpp(TScheduler::QueueDfcs)"
#define CSM_ZN13TSubScheduler9QueueDfcsEv " __cpp(TSubScheduler::QueueDfcs)"
#define	CSM_ZN13TSubScheduler16SelectNextThreadEv " __cpp(TSubScheduler::SelectNextThread)"
#define CSM_ZN5NKern20DisableAllInterruptsEv " __cpp(NKern::DisableAllInterrupts)"
#define CSM_ZN5NKern10LockSystemEv " __cpp(NKern::LockSystem)"
#define CSM_ZN7Monitor4InitEPvi " __cpp(Monitor::Init)"
#define CSM_ZN7Monitor5LeaveEi " __cpp(Monitor::Leave)"
#define CSM_Z10KernelMainv " __cpp(KernelMain)"
#define CSM_ZN4Kern9SuperPageEv " __cpp(Kern::SuperPage)"
#define CSM_ZN8DSession19CloseFromDisconnectEv " __cpp(DSession::CloseFromDisconnect)"
#define	CSM_ZN5NKern13ThreadEnterCSEv " __cpp(NKern::ThreadEnterCS)"
#define	CSM_ZN5NKern13ThreadLeaveCSEv " __cpp(NKern::ThreadLeaveCS)"
#define	CSM_ZN5NKern14_ThreadEnterCSEv " __cpp(NKern::_ThreadEnterCS)"
#define	CSM_ZN5NKern14_ThreadLeaveCSEv " __cpp(NKern::_ThreadLeaveCS)"
#define	CSM_ZN7DObject10AsyncCloseEv " __cpp(DObject::AsyncClose)"
#define CSM_ZN3EMI16CallStartHandlerEP7DThread " __cpp(EMI::CallStartHandler)"
#define CSM_ZN2KL5PanicENS_13TKernLibPanicE " __cpp(KL::Panic)"
#define CSM_ZN5NKern11FastCounterEv " __cpp(NKern::FastCounter)"
#define CSM_ZN4TDfc7DoEnqueEv " __cpp(TDfc::DoEnque)"
#define CSM_ZN6BTrace8DoOutBigEmmPKvimm " __cpp(BTrace::DoOutBig)"
#define CSM_ZN6BTrace4OutXEmmmm " __cpp(BTrace::OutX)"
#define CSM_ZN6BTrace3OutEmmmm " __cpp(BTrace::Out)"
#define CSM_ZN5NKern9TickCountEv " __cpp(NKern::TickCount)"
#define CSM_ZN3Mmu14MoveKernelPageEP6DChunkmmRmji " __cpp(Mmu::MoveKernelPage)"
#define CSM_ZN12TPriListBase15HighestPriorityEv "__cpp(TPriListBase::HighestPriority)"
#define CSM_ZN12TPriListBase5FirstEv "__cpp(TPriListBase::First)"
#define CSM_ZN12TPriListBase3AddEP12TPriListLink "__cpp(TPriListBase::Add)"
#define	CSM_ZN12TPriListBase6RemoveEP12TPriListLink "__cpp(TPriListBase::Remove)"
#define	CSM_ZN12TPriListBase14ChangePriorityEP12TPriListLinki "__cpp(TPriListBase::ChangePriority)"
#define CSM_ZN4Kern9AsyncFreeEPv " __cpp(Kern::AsyncFree)"
#define CSM_ZN9RMessageK4FreeEv " __cpp(RMessageK::Free)"
#define CSM_ZN1K9ObjDeleteEP7DObject " __cpp(K::ObjDelete)"
#define CSM_ZN5NKern19ThreadRequestSignalEP7NThread " __cpp(static_cast<void (*) (NThread *)>(&NKern::ThreadRequestSignal))"
#define CSM_ZN1K16ObjectFromHandleEii " __cpp(static_cast<DObject* (*) (TInt, TInt)>(&K::ObjectFromHandle))"
#define CSM_ZN5NKern6FMWaitEP10NFastMutex " __cpp(NKern::FMWait)"
#define CSM_ZN5NKern8FMSignalEP10NFastMutex " __cpp(NKern::FMSignal)"
#define CSM_ZN14TClientRequest9SetStatusEP14TRequestStatus " __cpp(TClientRequest::SetStatus)"
#define	CSM_ZN9TSpinLock7LockIrqEv "__cpp(TSpinLock::LockIrq)"
#define	CSM_ZN9TSpinLock9UnlockIrqEv "__cpp(TSpinLock::UnlockIrq)"
#define	CSM_ZN9TSpinLock8LockOnlyEv "__cpp(TSpinLock::LockOnly)"
#define	CSM_ZN9TSpinLock10UnlockOnlyEv "__cpp(TSpinLock::UnlockOnly)"
#define	CSM_ZN9TSpinLock11LockIrqSaveEv "__cpp(TSpinLock::LockIrqSave)"
#define	CSM_ZN9TSpinLock16UnlockIrqRestoreEi "__cpp(TSpinLock::UnlockIrqRestore)"
#define	CSM_ZN11TRWSpinLock8LockIrqREv "__cpp(TRWSpinLock::LockIrqR)"
#define	CSM_ZN11TRWSpinLock10UnlockIrqREv "__cpp(TRWSpinLock::UnlockIrqR)"
#define	CSM_ZN11TRWSpinLock9LockOnlyREv "__cpp(TRWSpinLock::LockOnlyR)"
#define	CSM_ZN11TRWSpinLock11UnlockOnlyREv "__cpp(TRWSpinLock::UnlockOnlyR)"
#define	CSM_ZN11TRWSpinLock17UnlockIrqRestoreREi "__cpp(TRWSpinLock::UnlockIrqRestoreR)"
#define	CSM_ZN11TRWSpinLock8LockIrqWEv "__cpp(TRWSpinLock::LockIrqW)"
#define	CSM_ZN11TRWSpinLock10UnlockIrqWEv "__cpp(TRWSpinLock::UnlockIrqW)"
#define	CSM_ZN11TRWSpinLock9LockOnlyWEv "__cpp(TRWSpinLock::LockOnlyW)"
#define	CSM_ZN11TRWSpinLock11UnlockOnlyWEv "__cpp(TRWSpinLock::UnlockOnlyW)"
#define	CSM_ZN11TRWSpinLock17UnlockIrqRestoreWEi "__cpp(TRWSpinLock::UnlockIrqRestoreW)"
#define	CSM_ZN3Arm9SaveStateER14SFullArmRegSet "__cpp(Arm::SaveState)"
#define	CSM_ZN3Arm3RegER14SFullArmRegSetim "__cpp(Arm::Reg)"
#define	CSM_Z9KDebugNumi "__cpp(KDebugNum)"
#define CSM_ZN5NKern9TimestampEv " __cpp(NKern::Timestamp)"
#define	CSM_ZN5NKern18TimestampFrequencyEv " __cpp(NKern::TimestampFrequency)"
#define CSM_ZN6NTimer7OneShotEii " __cpp(static_cast<int (NTimer::*)(int, int)>(&NTimer::OneShot))"
#define CSM_ZN4Kern20QueueRequestCompleteEP7DThreadP14TClientRequesti " __cpp(Kern::QueueRequestComplete)"
#define CSM_ZN1M18IsDataPagedAddressEPKvRi " __cpp(M::IsDataPagedAddress)"
#define CSM_ZN8DSession14PinDescriptorsEPiPA3_mPP17TVirtualPinObject " __cpp(DSession::PinDescriptors)"
#define CSM_ZN14TClientRequest5ResetEv "__cpp(TClientRequest::Reset)"
#define CSM_ZN9RMessageK8CloseRefEv "__cpp(RMessageK::CloseRef)"
#else
// CIA symbol macros for EABI assemblers
#define CSM_ZN1K8MsgInfoE " _ZN1K9MsgInfoE"
#define CSM_ZN14NFastSemaphore6SignalEv " _ZN14NFastSemaphore6SignalEv"
#define CSM_ZN4Kern4ExitEi " _ZN4Kern4ExitEi"
#define CSM_ZN5NKern4ExitEv " _ZN5NKern4ExitEv"
#define CSM_ZN11NThreadBase12DoCsFunctionEv " _ZN11NThreadBase12DoCsFunctionEv"
#define CSM_Z24KUDesInfoPanicBadDesTypev " _Z24KUDesInfoPanicBadDesTypev"
#define CSM_Z29KUDesSetLengthPanicBadDesTypev " _Z29KUDesSetLengthPanicBadDesTypev"
#define CSM_ZN1K27LockedPlatformSecurityPanicEv " _ZN1K27LockedPlatformSecurityPanicEv"
#define CSM_ZN4Kern8SafeReadEPKvPvi " _ZN4Kern8SafeReadEPKvPvi"
#define CSM_ZN5NKern4LockEv " _ZN5NKern4LockEv"
#define CSM_ZN5NKern15PreemptionPointEv " _ZN5NKern15PreemptionPointEv"
#define CSM_ZN5Cache16AtomicSyncMemoryEv " _ZN5Cache16AtomicSyncMemoryEv"
#define CSM_ZN11NThreadBase4ExitEv " _ZN11NThreadBase4ExitEv"
#define CSM_ZN5NKern19ThreadRequestSignalEP7NThreadP10NFastMutex " _ZN5NKern19ThreadRequestSignalEP7NThreadP10NFastMutex"
#define CSM_ZN5NKern6UnlockEv " _ZN5NKern6UnlockEv"
#define CSM_Z21PanicSyncMsgSentTwicev " _Z21PanicSyncMsgSentTwicev"
#define CSM_ZN8DSession12AllocAndSendEiPKiP14TRequestStatus " _ZN8DSession12AllocAndSendEiPKiP14TRequestStatus"
#define CSM_Z20FastMutexSignalErrorv " _Z20FastMutexSignalErrorv"
#define CSM_Z27KUDesSetLengthPanicOverflowv " _Z27KUDesSetLengthPanicOverflowv"
#define CSM_ZN5NKern11FlashSystemEv " _ZN5NKern11FlashSystemEv"
#define CSM_ZN16CacheMaintenance15OnProcessSwitchEv " _ZN16CacheMaintenance15OnProcessSwitchEv"
#define CSM_ZN1K18PanicCurrentThreadEi " _ZN1K18PanicCurrentThreadEi"
#define CSM_ZN13DSessionShare17CloseOnCompletionEv " _ZN13DSessionShare17CloseOnCompletionEv"
#define CSM_ZN10NFastMutex6SignalEv " _ZN10NFastMutex6SignalEv"
#define CSM_ZN10NFastMutex4WaitEv " _ZN10NFastMutex4WaitEv"
#define CSM_Z9TBmaFaulti " _Z9TBmaFaulti"
#define CSM_ZN4Kern6PrintfEPKcz " _ZN4Kern6PrintfEPKcz"
#define CSM_ZN5TDes89SetLengthEi " _ZN5TDes89SetLengthEi"
#define CSM_Z20FastMutexNestAttemptv " _Z20FastMutexNestAttemptv"
#define CSM_ZN8DSession14SendOnNewShareEP9RMessageKi " _ZN8DSession14SendOnNewShareEP9RMessageKi"
#define CSM_ZN1K20TheFileServerProcessE " _ZN1K20TheFileServerProcessE"
#define CSM_ZN5NKern12UnlockSystemEv " _ZN5NKern12UnlockSystemEv"
#define CSM_ZN2PP17MonitorEntryPointE " _ZN2PP17MonitorEntryPointE"
#define CSM_ZN4Kern9SafeWriteEPvPKvi " _ZN4Kern9SafeWriteEPvPKvi"
#define CSM_Z25CompleteDisconnectMessageR9RMessageK " _Z25CompleteDisconnectMessageR9RMessageK"
#define CSM_ZN1K5FaultENS_6TFaultE " _ZN1K5FaultENS_6TFaultE"
#define CSM_ZN4TDfc3AddEv " _ZN4TDfc3AddEv"
#define CSM_ZN4TDfc6RawAddEv " _ZN4TDfc6RawAddEv"
#define CSM_ZN10TScheduler7YieldToEP11NThreadBase " _ZN10TScheduler7YieldToEP11NThreadBase"
#define CSM_Z14CheckLockStatev " _Z14CheckLockStatev"
#define CSM_Z18InvalidExecHandlerv " _Z18InvalidExecHandlerv"
#define CSM_Z15InvalidFastExecv " _Z15InvalidFastExecv"
#define CSM_ZN2PP11NanoWaitCalE " _ZN2PP11NanoWaitCalE"
#define CSM_Z22PanicMesAlreadyPendingv " _Z22PanicMesAlreadyPendingv"
#define CSM_ZN10TScheduler10RescheduleEv " _ZN10TScheduler10RescheduleEv"
#define CSM_ZN10TScheduler9QueueDfcsEv " _ZN10TScheduler9QueueDfcsEv"
#define CSM_ZN13TSubScheduler9QueueDfcsEv " _ZN13TSubScheduler9QueueDfcsEv"
#define	CSM_ZN13TSubScheduler16SelectNextThreadEv "_ZN13TSubScheduler16SelectNextThreadEv"
#define CSM_ZN5NKern20DisableAllInterruptsEv " _ZN5NKern20DisableAllInterruptsEv"
#define CSM_ZN5NKern10LockSystemEv " _ZN5NKern10LockSystemEv"
#define CSM_ZN7Monitor4InitEPvi " _ZN7Monitor4InitEPvi"
#define CSM_ZN7Monitor5LeaveEi " _ZN7Monitor5LeaveEi"
#define CSM_Z10KernelMainv " _Z10KernelMainv"
#define CSM_ZN4Kern9SuperPageEv " _ZN4Kern9SuperPageEv"
#define CSM_ZN8DSession19CloseFromDisconnectEv " _ZN8DSession19CloseFromDisconnectEv"
#define	CSM_ZN5NKern13ThreadEnterCSEv " _ZN5NKern13ThreadEnterCSEv"
#define	CSM_ZN5NKern13ThreadLeaveCSEv " _ZN5NKern13ThreadLeaveCSEv"
#define	CSM_ZN5NKern14_ThreadEnterCSEv " _ZN5NKern14_ThreadEnterCSEv"
#define	CSM_ZN5NKern14_ThreadLeaveCSEv " _ZN5NKern14_ThreadLeaveCSEv"
#define	CSM_ZN7DObject10AsyncCloseEv " _ZN7DObject10AsyncCloseEv"
#define CSM_ZN3EMI16CallStartHandlerEP7DThread " _ZN3EMI16CallStartHandlerEP7DThread"
#define CSM_ZN2KL5PanicENS_13TKernLibPanicE " _ZN2KL5PanicENS_13TKernLibPanicE"
#define CSM_ZN5NKern11FastCounterEv " _ZN5NKern11FastCounterEv"
#define CSM_ZN4TDfc7DoEnqueEv " _ZN4TDfc7DoEnqueEv"
#define CSM_ZN6BTrace8DoOutBigEmmPKvimm "_ZN6BTrace8DoOutBigEmmPKvimm"
#define CSM_ZN6BTrace4OutXEmmmm "_ZN6BTrace4OutXEmmmm"
#define CSM_ZN6BTrace3OutEmmmm "_ZN6BTrace3OutEmmmm"
#define CSM_ZN5NKern9TickCountEv "_ZN5NKern9TickCountEv"
#define CSM_ZN3Mmu14MoveKernelPageEP6DChunkmmRmji "_ZN3Mmu14MoveKernelPageEP6DChunkmmRmji"
#define CSM_ZN12TPriListBase15HighestPriorityEv "_ZN12TPriListBase15HighestPriorityEv"
#define CSM_ZN12TPriListBase5FirstEv "_ZN12TPriListBase5FirstEv"
#define CSM_ZN12TPriListBase3AddEP12TPriListLink "_ZN12TPriListBase3AddEP12TPriListLink"
#define	CSM_ZN12TPriListBase6RemoveEP12TPriListLink "_ZN12TPriListBase6RemoveEP12TPriListLink"
#define	CSM_ZN12TPriListBase14ChangePriorityEP12TPriListLinki "_ZN12TPriListBase14ChangePriorityEP12TPriListLinki"
#define CSM_ZN4Kern9AsyncFreeEPv "_ZN4Kern9AsyncFreeEPv"
#define CSM_ZN9RMessageK4FreeEv "_ZN9RMessageK4FreeEv"
#define CSM_ZN1K9ObjDeleteEP7DObject "_ZN1K9ObjDeleteEP7DObject"
#define CSM_ZN5NKern19ThreadRequestSignalEP7NThread "_ZN5NKern19ThreadRequestSignalEP7NThread"
#define CSM_ZN1K16ObjectFromHandleEii "_ZN1K16ObjectFromHandleEii"
#define CSM_ZN5NKern6FMWaitEP10NFastMutex "_ZN5NKern6FMWaitEP10NFastMutex"
#define CSM_ZN5NKern8FMSignalEP10NFastMutex "_ZN5NKern8FMSignalEP10NFastMutex"
#define CSM_ZN14TClientRequest9SetStatusEP14TRequestStatus "_ZN14TClientRequest9SetStatusEP14TRequestStatus"
#define	CSM_ZN9TSpinLock7LockIrqEv "_ZN9TSpinLock7LockIrqEv"
#define	CSM_ZN9TSpinLock9UnlockIrqEv "_ZN9TSpinLock9UnlockIrqEv"
#define	CSM_ZN9TSpinLock8LockOnlyEv "_ZN9TSpinLock8LockOnlyEv"
#define	CSM_ZN9TSpinLock10UnlockOnlyEv "_ZN9TSpinLock10UnlockOnlyEv"
#define	CSM_ZN9TSpinLock11LockIrqSaveEv "_ZN9TSpinLock11LockIrqSaveEv"
#define	CSM_ZN9TSpinLock16UnlockIrqRestoreEi "_ZN9TSpinLock16UnlockIrqRestoreEi"
#define	CSM_ZN11TRWSpinLock8LockIrqREv "_ZN11TRWSpinLock8LockIrqREv"
#define	CSM_ZN11TRWSpinLock10UnlockIrqREv "_ZN11TRWSpinLock10UnlockIrqREv"
#define	CSM_ZN11TRWSpinLock9LockOnlyREv "_ZN11TRWSpinLock9LockOnlyREv"
#define	CSM_ZN11TRWSpinLock11UnlockOnlyREv "_ZN11TRWSpinLock11UnlockOnlyREv"
#define	CSM_ZN11TRWSpinLock17UnlockIrqRestoreREi "_ZN11TRWSpinLock17UnlockIrqRestoreREi"
#define	CSM_ZN11TRWSpinLock8LockIrqWEv "_ZN11TRWSpinLock8LockIrqWEv"
#define	CSM_ZN11TRWSpinLock10UnlockIrqWEv "_ZN11TRWSpinLock10UnlockIrqWEv"
#define	CSM_ZN11TRWSpinLock9LockOnlyWEv "_ZN11TRWSpinLock9LockOnlyWEv"
#define	CSM_ZN11TRWSpinLock11UnlockOnlyWEv "_ZN11TRWSpinLock11UnlockOnlyWEv"
#define	CSM_ZN11TRWSpinLock17UnlockIrqRestoreWEi "_ZN11TRWSpinLock17UnlockIrqRestoreWEi"
#define	CSM_ZN3Arm9SaveStateER14SFullArmRegSet "_ZN3Arm9SaveStateER14SFullArmRegSet"
#define	CSM_ZN3Arm3RegER14SFullArmRegSetim "_ZN3Arm3RegER14SFullArmRegSetim"
#define	CSM_Z9KDebugNumi " _Z9KDebugNumi"
#define CSM_ZN5NKern9TimestampEv " _ZN5NKern9TimestampEv"
#define	CSM_ZN5NKern18TimestampFrequencyEv " _ZN5NKern18TimestampFrequencyEv"
#define CSM_ZN6NTimer7OneShotEii " _ZN6NTimer7OneShotEii"
#define CSM_ZN4Kern20QueueRequestCompleteEP7DThreadP14TClientRequesti "_ZN4Kern20QueueRequestCompleteEP7DThreadP14TClientRequesti"
#define CSM_ZN1M18IsDataPagedAddressEPKvRi "_ZN1M18IsDataPagedAddressEPKvRi"
#define CSM_ZN8DSession14PinDescriptorsEPiPA3_mPP17TVirtualPinObject "_ZN8DSession14PinDescriptorsEPiPA3_mPP17TVirtualPinObject"
#define CSM_ZN14TClientRequest5ResetEv "_ZN14TClientRequest5ResetEv"
#define CSM_ZN9RMessageK8CloseRefEv "_ZN9RMessageK8CloseRefEv"
#endif

// CIA symbols for COMMON code (used both USER and KERNEL side)
// CIA symbol macros for Gcc98r2
#if defined(__GCC32__)
#define CSM_CFUNC(f)	#f
#define CSM_Z26Des16PanicLengthOutOfRangev " Des16PanicLengthOutOfRange__Fv"
#define CSM_Z20Des8PanicDesOverflowv " Des8PanicDesOverflow__Fv"
#define CSM_ZN10RArrayBase4GrowEv " Grow__10RArrayBase"
#define CSM_ZN3Mem8CompareCEPKtiS0_i " CompareC__3MemPCUsiT1i"
#define CSM_Z20Des16PanicBadDesTypev " Des16PanicBadDesType__Fv"
#define CSM_Z24Des16PanicLengthNegativev " Des16PanicLengthNegative__Fv"
#define CSM_Z19Des8PanicBadDesTypev " Des8PanicBadDesType__Fv"
#define CSM_ZN3Mem7CompareEPKtiS0_i " Compare__3MemPCUsiT1i"
#define CSM_Z23Des8PanicLengthNegativev " Des8PanicLengthNegative__Fv"
#define CSM_ZN17RPointerArrayBase4GrowEv " Grow__17RPointerArrayBase"
#define CSM_Z28Des16PanicDesIndexOutOfRangev " Des16PanicDesIndexOutOfRange__Fv"
#define CSM_ZN3Mem8CompareFEPKhiS0_i " CompareF__3MemPCUciT1i"
#define CSM_Z5Panic9TCdtPanic " Panic__F9TCdtPanic"
#define CSM_Z27Des8PanicDesIndexOutOfRangev " Des8PanicDesIndexOutOfRange__Fv"
#define CSM_ZN3Mem8CompareFEPKtiS0_i " CompareF__3MemPCUsiT1i"
#define CSM_Z26Des8PanicMaxLengthNegativev " Des8PanicMaxLengthNegative__Fv"
#define CSM_Z25Des8PanicLengthOutOfRangev " Des8PanicLengthOutOfRange__Fv"
#define CSM_Z21Des16PanicDesOverflowv " Des16PanicDesOverflow__Fv"
#define CSM_Z27Des16PanicMaxLengthNegativev " Des16PanicMaxLengthNegative__Fv"
#define CSM_Z18PanicBadArrayIndexv " PanicBadArrayIndex__Fv"
#define CSM_Z22Des8PanicPosOutOfRangev " Des8PanicPosOutOfRange__Fv"
#define CSM_Z23Des16PanicPosOutOfRangev " Des16PanicPosOutOfRange__Fv"
#define CSM_ZN3Mem8CompareCEPKhiS0_i " CompareC__3MemPCUciT1i"
#define CSM_Z22RHeap_PanicBadNextCellv " RHeap_PanicBadNextCell__Fv"
#define CSM_Z30PanicEWordMoveSourceNotAlignedv " PanicEWordMoveSourceNotAligned__Fv"
#define CSM_Z30PanicEWordMoveTargetNotAlignedv " PanicEWordMoveTargetNotAligned__Fv"
#define CSM_Z34PanicEWordMoveLengthNotMultipleOf4v " PanicEWordMoveLengthNotMultipleOf4__Fv"
#define CSM_ZN16DMemModelProcess13TryOpenOsAsidEv "TryOpenOsAsid__16DMemModelProcess"
#define CSM_ZN16DMemModelProcess11CloseOsAsidEv " CloseOsAsid__16DMemModelProcess"

#elif defined(__ARMCC__)
// CIA symbol macros for RVCT
#define	CSM_CFUNC(f)	"__cpp(" #f ")"
#define CSM_Z26Des16PanicLengthOutOfRangev " __cpp(Des16PanicLengthOutOfRange)"
#define CSM_Z20Des8PanicDesOverflowv " __cpp(Des8PanicDesOverflow)"
#define CSM_ZN10RArrayBase4GrowEv " __cpp(RArrayBase::Grow)"
#define CSM_ZN3Mem8CompareCEPKtiS0_i " __cpp(static_cast<int (*) (const unsigned short*, int, const unsigned short*, int)>(&Mem::CompareC))"
#define CSM_Z20Des16PanicBadDesTypev " __cpp(Des16PanicBadDesType)"
#define CSM_Z24Des16PanicLengthNegativev " __cpp(Des16PanicLengthNegative)"
#define CSM_Z19Des8PanicBadDesTypev " __cpp(Des8PanicBadDesType)"
#define CSM_ZN3Mem7CompareEPKtiS0_i " __cpp(static_cast<int (*) (const unsigned short*, int, const unsigned short*, int)>(&Mem::Compare))"
#define CSM_Z23Des8PanicLengthNegativev " __cpp(Des8PanicLengthNegative)"
#define CSM_ZN17RPointerArrayBase4GrowEv " __cpp(RPointerArrayBase::Grow)"
#define CSM_Z28Des16PanicDesIndexOutOfRangev " __cpp(Des16PanicDesIndexOutOfRange)"
#define CSM_ZN3Mem8CompareFEPKhiS0_i " __cpp(static_cast<int (*) (const unsigned char*, int, const unsigned char*, int)>(&Mem::CompareF))"
#define CSM_Z5Panic9TCdtPanic " __cpp(Panic)"
#define CSM_Z27Des8PanicDesIndexOutOfRangev " __cpp(Des8PanicDesIndexOutOfRange)"
#define CSM_ZN3Mem8CompareFEPKtiS0_i " __cpp(static_cast<int (*) (const unsigned short*, int, const unsigned short*, int)>(&Mem::CompareF))"
#define CSM_Z26Des8PanicMaxLengthNegativev " __cpp(Des8PanicMaxLengthNegative)"
#define CSM_Z25Des8PanicLengthOutOfRangev " __cpp(Des8PanicLengthOutOfRange)"
#define CSM_Z21Des16PanicDesOverflowv " __cpp(Des16PanicDesOverflow)"
#define CSM_Z27Des16PanicMaxLengthNegativev " __cpp(Des16PanicMaxLengthNegative)"
#define CSM_Z18PanicBadArrayIndexv " __cpp(PanicBadArrayIndex)"
#define CSM_Z22Des8PanicPosOutOfRangev " __cpp(Des8PanicPosOutOfRange)"
#define CSM_Z23Des16PanicPosOutOfRangev " __cpp(Des16PanicPosOutOfRange)"
#define CSM_ZN3Mem8CompareCEPKhiS0_i " __cpp(static_cast<int (*) (const unsigned char*, int, const unsigned char*, int)>(&Mem::CompareC))"
#define CSM_Z22RHeap_PanicBadNextCellv " __cpp(RHeap_PanicBadNextCell)"
#define CSM_Z30PanicEWordMoveSourceNotAlignedv " __cpp(PanicEWordMoveSourceNotAligned)"
#define CSM_Z30PanicEWordMoveTargetNotAlignedv " __cpp(PanicEWordMoveTargetNotAligned)"
#define CSM_Z34PanicEWordMoveLengthNotMultipleOf4v " __cpp(PanicEWordMoveLengthNotMultipleOf4)"
#define CSM_ZN16DMemModelProcess13TryOpenOsAsidEv " __cpp(DMemModelProcess::TryOpenOsAsid)"
#define CSM_ZN16DMemModelProcess11CloseOsAsidEv " __cpp(DMemModelProcess::CloseOsAsid)"
#else
// CIA symbol macros for EABI assemblers
#define CSM_CFUNC(f)	#f
#define CSM_Z26Des16PanicLengthOutOfRangev " _Z26Des16PanicLengthOutOfRangev"
#define CSM_Z20Des8PanicDesOverflowv " _Z20Des8PanicDesOverflowv"
#define CSM_ZN10RArrayBase4GrowEv " _ZN10RArrayBase4GrowEv"
#define CSM_ZN3Mem8CompareCEPKtiS0_i " _ZN3Mem8CompareCEPKtiS0_i"
#define CSM_Z20Des16PanicBadDesTypev " _Z20Des16PanicBadDesTypev"
#define CSM_Z24Des16PanicLengthNegativev " _Z24Des16PanicLengthNegativev"
#define CSM_Z19Des8PanicBadDesTypev " _Z19Des8PanicBadDesTypev"
#define CSM_ZN3Mem7CompareEPKtiS0_i " _ZN3Mem7CompareEPKtiS0_i"
#define CSM_Z23Des8PanicLengthNegativev " _Z23Des8PanicLengthNegativev"
#define CSM_ZN17RPointerArrayBase4GrowEv " _ZN17RPointerArrayBase4GrowEv"
#define CSM_Z28Des16PanicDesIndexOutOfRangev " _Z28Des16PanicDesIndexOutOfRangev"
#define CSM_ZN3Mem8CompareFEPKhiS0_i " _ZN3Mem8CompareFEPKhiS0_i"
#define CSM_Z5Panic9TCdtPanic " _Z5Panic9TCdtPanic"
#define CSM_Z27Des8PanicDesIndexOutOfRangev " _Z27Des8PanicDesIndexOutOfRangev"
#define CSM_ZN3Mem8CompareFEPKtiS0_i " _ZN3Mem8CompareFEPKtiS0_i"
#define CSM_Z26Des8PanicMaxLengthNegativev " _Z26Des8PanicMaxLengthNegativev"
#define CSM_Z25Des8PanicLengthOutOfRangev " _Z25Des8PanicLengthOutOfRangev"
#define CSM_Z21Des16PanicDesOverflowv " _Z21Des16PanicDesOverflowv"
#define CSM_Z27Des16PanicMaxLengthNegativev " _Z27Des16PanicMaxLengthNegativev"
#define CSM_Z18PanicBadArrayIndexv " _Z18PanicBadArrayIndexv"
#define CSM_Z22Des8PanicPosOutOfRangev " _Z22Des8PanicPosOutOfRangev"
#define CSM_Z23Des16PanicPosOutOfRangev " _Z23Des16PanicPosOutOfRangev"
#define CSM_ZN3Mem8CompareCEPKhiS0_i " _ZN3Mem8CompareCEPKhiS0_i"
#define CSM_Z22RHeap_PanicBadNextCellv " _Z22RHeap_PanicBadNextCellv"
#define CSM_Z30PanicEWordMoveSourceNotAlignedv " _Z30PanicEWordMoveSourceNotAlignedv"
#define CSM_Z30PanicEWordMoveTargetNotAlignedv " _Z30PanicEWordMoveTargetNotAlignedv"
#define CSM_Z34PanicEWordMoveLengthNotMultipleOf4v " _Z34PanicEWordMoveLengthNotMultipleOf4v"
#define CSM_ZN16DMemModelProcess13TryOpenOsAsidEv " _ZN16DMemModelProcess13TryOpenOsAsidEv"
#define CSM_ZN16DMemModelProcess11CloseOsAsidEv " _ZN16DMemModelProcess11CloseOsAsidEv"
#endif

// These macros define 'hooks' that maybe useful to tie various functions in the kernel
// to compiler helper functions. Typically they just allow for the avoidance of a jump.
#ifndef KMEMSETHOOK
#if defined(__ARMCC__)
#if (__ARMCC_VERSION > 210000)
#define KMEMSETHOOK     asm(".global __rt_memset ");  asm("__rt_memset: ");\
                        asm(".global _memset ");  asm("_memset: ");\
                        asm(".global _memset_w ");  asm("_memset_w: "); 

#else
#define KMEMSETHOOK     asm(".global __rt_memset ");  asm("__rt_memset: ");\
                        asm(".global _memset_w ");  asm("_memset_w: "); 
#endif
#else
#define KMEMSETHOOK
#endif
#endif

#ifndef KMEMMOVEHOOK
#if defined(__ARMCC__)
#if (__ARMCC_VERSION > 210000)
#define KMEMMOVEHOOK         asm(".global __aeabi_memmove8 ");  asm("__aeabi_memmove8: ");\
                             asm(".global __aeabi_memmove4 ");  asm("__aeabi_memmove4: "); \
                             asm(".global __aeabi_memmove ");  asm("__aeabi_memmove: "); \
                             asm(".global __rt_memmove "); asm("__rt_memmove: ");\
                             asm(".global __rt_memmove_w "); asm("__rt_memmove_w: ");
#else
#define KMEMMOVEHOOK         asm(".global __rt_memmove "); asm("__rt_memmove: ");\
                             asm(".global __rt_memmove_w "); asm("__rt_memmove_w: ");
#endif
#else
#define KMEMMOVEHOOK
#endif
#endif

#ifndef KMEMCPYHOOK
#if defined(__ARMCC__)
#if (__ARMCC_VERSION > 210000)
#define KMEMCPYHOOK             asm(".global __aeabi_memcpy8 ");  asm("__aeabi_memcpy8: ");\
                                asm(".global __aeabi_memcpy4 ");  asm("__aeabi_memcpy4: "); \
                                asm(".global __aeabi_memcpy ");  asm("__aeabi_memcpy: "); \
                                asm(".global _memcpy ");asm("_memcpy: ");\
                                asm(".global __rt_memcpy ");asm("__rt_memcpy: ");\
                                asm(".global __rt_memcpy_w ");asm("__rt_memcpy_w: ");
#else
#define KMEMCPYHOOK             asm(".global __rt_memcpy ");asm("__rt_memcpy: ");\
                                asm(".global __rt_memcpy_w ");asm("__rt_memcpy_w: ");
#endif
#else
#define KMEMCPYHOOK
#endif
#endif

#ifndef KMEMCLRHOOK
#if defined(__ARMCC__)
#if (__ARMCC_VERSION > 210000)
#define KMEMCLRHOOK             asm(".global __aeabi_memclr8 ");  asm("__aeabi_memclr8: ");\
                                asm(".global __aeabi_memclr4 ");  asm("__aeabi_memclr4: "); \
                                asm(".global __aeabi_memclr ");  asm("__aeabi_memclr: "); \
                                asm(".global __rt_memclr ");asm("__rt_memclr: ");\
                                asm(".global __rt_memclr_w ");asm("__rt_memclr_w: ");
#else
#define KMEMCLRHOOK             asm(".global __rt_memclr ");asm("__rt_memclr: ");\
                                asm(".global __rt_memclr_w ");asm("__rt_memclr_w: ");
#endif
#else
#define KMEMCLRHOOK
#endif
#endif

#endif
