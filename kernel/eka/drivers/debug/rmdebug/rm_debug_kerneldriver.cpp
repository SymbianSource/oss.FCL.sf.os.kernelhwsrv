// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Device driver for kernel side debug assist
//

#ifdef __WINS__
#error - this driver cannot be built for emulation
#endif

#include <e32def.h>
#include <e32def_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <e32ldr.h>
#include <u32std.h>
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include <nk_trace.h>
#include <arm.h>
#include <kernel/cache.h>
#include <platform.h>
#include <nkern.h>
#include <u32hal.h>
#include <kernel/kdebug.h>
#include <rm_debug_api.h>

#include "debug_logging.h"
#include "d_rmd_breakpoints.h"	// moved breakpoints code lives here
#include "d_rmd_stepping.h"		// moved stepping code lives here
#include "rm_debug_kerneldriver.h"
#include "d_list_manager.h"
#include "rm_debug_driver.h"
#include "rm_debug_eventhandler.h"
#include "d_debug_functionality.h"
#include "d_process_tracker.h"
#include "debug_utils.h"
#include "d_buffer_manager.h"

using namespace Debug;

/////////////////////////////////////////////////////////////////////////
//
// DRM_DebugDriverFactory implementation
//
/////////////////////////////////////////////////////////////////////////

//
// DRM_DebugDriverFactory constructor
//
DRM_DebugDriverFactory::DRM_DebugDriverFactory()
	{
	iVersion = TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

//
// DRM_DebugDriverFactory::Create
//
TInt DRM_DebugDriverFactory::Create(DLogicalChannelBase*& aChannel)
	{
	if (iOpenChannels != 0)
		return KErrInUse; // a channel is already open

	aChannel = new DRM_DebugChannel(this);

	return aChannel ? KErrNone : KErrNoMemory;
	}

//
// DRM_DebugDriverFactory::Install
//
TInt DRM_DebugDriverFactory::Install()
	{
	return(SetName(&KRM_DebugDriverName));
	}

//
// DRM_DebugDriverFactory::Install
//
void DRM_DebugDriverFactory::GetCaps(TDes8& aDes) const
	{
	TCapsRM_DebugDriver b;
	b.iVersion = TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber);

	Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
	}

/////////////////////////////////////////////////////////////////////////
//
// DRM_DebugChannel implementation
//
/////////////////////////////////////////////////////////////////////////

//
// DRM_DebugChannel constructor
//
DRM_DebugChannel::DRM_DebugChannel(DLogicalDevice* aLogicalDevice)
	: iExcludedROMAddressStart(ROM_LINEAR_BASE),
	iExcludedROMAddressEnd(0),
	iPageSize(0x1000),
	iBreakManager(0),
	iStepper(0),
	iStepLock(0),
	iDfcQ(NULL),
	iInitialisedCodeModifier(0),
	iAsyncGetValueRequest(NULL)
	{
	LOG_MSG("DRM_DebugChannel::DRM_DebugChannel()");

	iDevice = aLogicalDevice;

	iClientThread = &Kern::CurrentThread();
	iClientThread->Open();

	LOG_MSG3("DRM_DebugChannel::DRM_DebugChannel() clientThread = 0x%08x, id=%d", 
	            iClientThread, iClientThread->iId );


	iPageSize = Kern::RoundToPageSize(1);
	}

//
// DRM_DebugChannel destructor
//
DRM_DebugChannel::~DRM_DebugChannel()
	{
	LOG_MSG("DRM_DebugChannel::~DRM_DebugChannel()");

	if (iAsyncGetValueRequest)
		{
		Kern::QueueRequestComplete(iClientThread, iAsyncGetValueRequest, KErrCancel); // does nothing if request not pending
		Kern::DestroyClientRequest(iAsyncGetValueRequest);
		}

	NKern::ThreadEnterCS();
	Kern::SafeClose((DObject*&)iClientThread, NULL);
	NKern::ThreadLeaveCS();

	// Close breakpoint manager
	if (iBreakManager)
		{
		NKern::ThreadEnterCS();
		delete iBreakManager;
		NKern::ThreadLeaveCS();
		}

	// Close stepping manager
	if (iStepper)
		{
		NKern::ThreadEnterCS();
		delete iStepper;
		NKern::ThreadLeaveCS();
		}

	//close the debug process list
	iDebugProcessList.Close();

	DestroyDfcQ();

	//close the code modifier
	if (iInitialisedCodeModifier)
		{
		DebugSupport::CloseCodeModifier();
		}
	}

void DRM_DebugChannel::DestroyDfcQ()
	{
	LOG_MSG("DRM_DebugChannel::DestroyDfcQ()");
	if (iDfcQ)
		{
		NKern::ThreadEnterCS();
		iDfcQ->Destroy();
		NKern::ThreadLeaveCS();
		}
	}

//
// DRM_DebugChannel::DoCreate
//
TInt DRM_DebugChannel::DoCreate(TInt /*aUnit*/, const TDesC* anInfo, const TVersion& aVer)
	{
	LOG_MSG("DRM_DebugChannel::DoCreate()");
	TInt err = Kern::CreateClientDataRequest(iAsyncGetValueRequest);
	if(err != KErrNone)
		return err;

	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber), aVer))
		return KErrNotSupported;

	// Do the security check here so that any arbitrary application doesn't make
	// use of Trk kernel driver.
	if (!DoSecurityCheck())
		{
		LOG_MSG("DRM_DebugChannel::DoCreate() - permission denied!");
			return KErrPermissionDenied;
		}

	if (anInfo)
		{
		// this is the end address of the user library.
		// this doesn't seem to be valid for EKA2.
		// right now we dont need this for EKA2 since we are not worried
		// about kernel being stopped as kernel is multithreaded.
		// just retaining this for future use.
		TBuf8<32> buf;
		TInt err = Kern::ThreadRawRead(iClientThread, anInfo, &buf, 32);
		if(err != KErrNone)
			return err;
		}

	// Allocate a D_RMD_Breakpoints class as a breakpoint manager
	NKern::ThreadEnterCS();
	iBreakManager = new D_RMD_Breakpoints(this);
	NKern::ThreadLeaveCS();
	if (iBreakManager == NULL)
		{
		LOG_MSG("DRM_DebugChannel::DRM_DebugChannel - could not construct breakpoint manager");
		return KErrNoMemory;
		}

	// Initialise the new breakpoint manager object
	iBreakManager->Init();

	// Allocate a DRMDStepping class as the stepping manager
	NKern::ThreadEnterCS();
	iStepper = new DRMDStepping(this);
	NKern::ThreadLeaveCS();
	if (iStepper == NULL)
		{
		LOG_MSG("DRM_DebugChannel::DRM_DebugChannel - could not construct stepper manager");
		return KErrNoMemory;
		}

	// Initialize the code modifier for managing breakpoints.
	TUint caps; //ignored for now
	err = DebugSupport::InitialiseCodeModifier(caps, NUMBER_OF_MAX_BREAKPOINTS);
	//if code modifier initializer failed,
	//return here, since we can't set an breakpoints
	if(err != KErrNone)
		{
		return err;
		}
	else
		{
		iInitialisedCodeModifier = ETrue;
		}

	//create and set the driver's Dfc queue
	err = CreateDfcQ();
	if(err != KErrNone)
		{
		LOG_MSG("DRM_DebugChannel::DoCreate() Creating Dfc queue failed.");
		}
	SetDfcQ(iDfcQ);

	iMsgQ.Receive();

	iEventHandler = new DRM_DebugEventHandler;
	if (!iEventHandler)
		return KErrNoMemory;
	err = iEventHandler->Create(iDevice, this, iClientThread);
	if (err != KErrNone)
		return err;

	//return KErrNone;
	return iEventHandler->Start();
	}

/**
Forward call to either synch or asynch methods while serialising all calls via lock.
 
Protect access via a the event handler lock to 
serialise all calls and protect concurrent access to data structures

@param aMsg pointer to a TMessageBase object 

@return error returned by called methods

@see DRM_DebugEventHandler::HandleSpecificEvent where lock is also used
@see DRM_DebugEventHandler::iProtectionLock

*/
TInt DRM_DebugChannel::SendMsg(TMessageBase* aMsg)
	{
	DThread * currThread = &Kern::CurrentThread();
	LOG_MSG3("DRM_DebugChannel::SendMsg() currThread = 0x%08x, iClientThread=0x%08x", currThread, iClientThread );

	iEventHandler->LockDataAccess();
	
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = m.iValue;
	TInt err = KErrNone;

	if (id != (TInt)ECloseMsg && id != KMaxTInt && id < 0)
		{
		// DoRequest
		TRequestStatus* pStatus = (TRequestStatus*)m.Ptr0();
		err = SendRequest(aMsg);
		if (err != KErrNone)
			Kern::RequestComplete(pStatus,err);
		}
	else
		{
		err = DLogicalChannel::SendMsg(aMsg);
		}
	
	iEventHandler->ReleaseDataAccess();
	return err;
	}

//
// DRM_DebugChannel::SendRequest
//
TInt DRM_DebugChannel::SendRequest(TMessageBase* aMsg)
	{
	LOG_MSG("DRM_DebugChannel::SendRequest()");

	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt function = ~m.iValue;
	TRequestStatus* pStatus = (TRequestStatus*)m.Ptr0();
	TAny* a1 = m.Ptr1();

	TInt err = KErrNotSupported;
	switch(function)
		{
		case RRM_DebugDriver::ERequestGetEvent:
			err = PreAsyncGetValue((TEventInfo*)a1,pStatus);
			break;
		}
	if (err == KErrNone)
		err = DLogicalChannel::SendMsg(aMsg);
	return err;
	}

//
// DRM_DebugChannel::PreAsyncGetValue
//
TInt DRM_DebugChannel::PreAsyncGetValue(TEventInfo* aValue, TRequestStatus* aStatus)
	{
	LOG_MSG3("DRM_DebugChannel::PreAsyncGetValue() TEventInfo=0x%08x, TRequestStatus=0x%08x",
		aValue, aStatus );
	
	iAsyncGetValueRequest->Reset();
	
	TInt err = iAsyncGetValueRequest->SetStatus(aStatus);
	if (err != KErrNone)
		return err;
	
	iAsyncGetValueRequest->SetDestPtr(aValue);
	return KErrNone;
	}

/**
  Create the Dfc queue for receiving messages
  */
TInt DRM_DebugChannel::CreateDfcQ()
	{
	LOG_MSG("DRM_DebugChannel::CreateDfcQ()");
	TInt r = Kern::DynamicDfcQCreate(iDfcQ, KRmDebugDriverThreadPriority, KRM_DebugDriverName);
	// Fix to stop t_rmdebug2 etc crashing the device.
	// This should be removed once the rm debug driver has been updated for WDP.
	if (r == KErrNone)
		iDfcQ->SetRealtimeState(ERealtimeStateOff);
	return r;
	}

//
// DRM_DebugChannel::DoCancel
//
// New: The cancel call does not take an enum parameter describing
// the request to be cancelled. Rather it supplies a pointer
// to a user-side struct defining the cancellation
//
void DRM_DebugChannel::DoCancel(TInt aReqNo)
	{
	LOG_MSG("DRM_DebugChannel::DoCancel()");

	TRMD_DebugCancelInfo info;

	TInt err = Kern::ThreadRawRead(iClientThread,(TAny*)aReqNo,(TAny*)&info,sizeof(info));
	if (err != KErrNone)
		{
		// How do we cancel something we know nothing about???
		LOG_MSG("DRM_DebugChannel::DoCancel - bad arguments");
		return;
		}

	// Find the process
	DTargetProcess* pProcess = TheDProcessTracker.FindProcess(info.iProcessName);
	if (pProcess == NULL)
		{
		// We are doomed. We don't know which event to cancel..
		LOG_MSG2("Cannot determine which process is being debugged: %S", &(info.iProcessName));

		return;
		}

	// Find the agent
	DDebugAgent* debugAgent = pProcess->Agent(info.iAgentId);
	if (debugAgent == NULL)
		{
		// Bad agent means there is no tracking agent
		LOG_MSG2("Cannot locate debug agent with pid 0x%0x16lx",info.iAgentId);
		return;
		}

	// Agent completes/pends the request as appropriate.
	debugAgent->CancelGetEvent();

	}

//
// DRM_DebugChannel::DoRequest
//
void DRM_DebugChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	LOG_MSG4("DRM_DebugChannel::DoRequest(), iClientThread=0x%08x, tid=0x%08x, TRequestStatus=0x%08x", 
		iClientThread, I64LOW(iClientThread->iId), aStatus);

	switch(aReqNo)
		{
		case RRM_DebugDriver::ERequestGetEvent:
			{
			TEventMetaData eventMetaData;
			TInt err = Kern::ThreadRawRead(iClientThread, a2, (TUint8 *)&eventMetaData, sizeof(TEventMetaData) );
			if (err != KErrNone)
				{
				LOG_MSG("Error: could not read argument data from the DSS (TEventMetaData)");

				// We could not read information from the user, so the a2 argument is probably wrong
				Kern::RequestComplete(iClientThread, aStatus, KErrArgument);
				return;
				}

			// Find the process
			DTargetProcess* pProcess = TheDProcessTracker.FindProcess(eventMetaData.iTargetProcessName);
			if (pProcess == NULL)
				{
				LOG_MSG("Cannot identify process being debugged");

				// We could not locate the process, so the user asked for the wrong one.
				Kern::RequestComplete(iClientThread, aStatus, KErrArgument);
				return;
				}

			// Find the agent
			DDebugAgent* debugAgent = pProcess->Agent(eventMetaData.iDebugAgentProcessId);
			LOG_MSG5(" For agent pid=%d, DTargetProcess=0x%08x, Agent=0x%08x, iAsyncGetValueRequest0x%08x", 
				I64LOW(eventMetaData.iDebugAgentProcessId), pProcess, debugAgent, iAsyncGetValueRequest );

			if (debugAgent == NULL)
				{
				// Bad agent means there is no tracking agent
				LOG_MSG2("Cannot locate debug agent with pid 0x%0x16lx",eventMetaData.iDebugAgentProcessId);
				return;
				}
			// Agent completes/pends the request as appropriate.
			debugAgent->GetEvent(iAsyncGetValueRequest, iClientThread);

			break;
			}
		default:
			{
			// Don't know what to do, should not get here!
			LOG_MSG("DRM_DebugChannel::DoRequest was passed an unsupported request aReqNo");

			Kern::RequestComplete(iClientThread, aStatus, KErrNotSupported);
			}
		}
	}

//
// DRM_DebugChannel::DoControl
//
TInt DRM_DebugChannel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	LOG_MSG("DRM_DebugChannel::DoControl()");

	LOG_MSG2("DoControl Function %d", aFunction);

	TInt err = KErrBadHandle;
	DThread* threadObj = NULL;

	switch(aFunction)
		{
		/* Security first */
		case RRM_DebugDriver::EControlIsDebuggable:
			{
			err = IsDebuggable((TUint32)a1);
			break;
			}
		case RRM_DebugDriver::EControlSetBreak:
			{
			err = SetBreak((TSetBreakInfo*)a1);
			break;
			}
		case RRM_DebugDriver::EControlClearBreak:
			{
			err = iBreakManager->DoClearBreak((TInt32)a1);
			break;
			}
		case RRM_DebugDriver::EControlModifyBreak:
			{
			err = iBreakManager->DoModifyBreak((TModifyBreakInfo*)a1);
			break;
			}
		case RRM_DebugDriver::EControlModifyProcessBreak:
			{
			err = iBreakManager->DoModifyProcessBreak((TModifyProcessBreakInfo*)a1);
			break;
			}
		case RRM_DebugDriver::EControlBreakInfo:
			{
			err = iBreakManager->DoBreakInfo((TGetBreakInfo*)a1);
			break;
			}
		case RRM_DebugDriver::EControlSuspendThread:
			{
			threadObj = DebugUtils::OpenThreadHandle((TUint32)a1);
			if (threadObj)
			{
				err = DoSuspendThread(threadObj);
			}
			break;
			}
		case RRM_DebugDriver::EControlResumeThread:
			{
			threadObj = DebugUtils::OpenThreadHandle((TUint32)a1);
			if (threadObj)
				{
				err = DoResumeThread(threadObj);
				}
			break;
			}
		case RRM_DebugDriver::EControlStepRange:
			{
			threadObj = DebugUtils::OpenThreadHandle((TUint32)a1);
			if (threadObj)
				{
				err = StepRange(threadObj, (TRM_DebugStepInfo*)a2);
				}
			break;
			}
		case RRM_DebugDriver::EControlReadMemory:
			{
			threadObj = DebugUtils::OpenThreadHandle((TUint32)a1);
			if (threadObj)
				{
				err = ReadMemory(threadObj, (TRM_DebugMemoryInfo*)a2);
				}
			break;
			}
		case RRM_DebugDriver::EControlWriteMemory:
			{
			threadObj = DebugUtils::OpenThreadHandle((TUint32)a1);
			if (threadObj)
				{
				err = WriteMemory(threadObj, (TRM_DebugMemoryInfo*)a2);
				}
			break;
			}
		case RRM_DebugDriver::EControlReadRegistersLegacy:
			{
			threadObj = DebugUtils::OpenThreadHandle((TUint32)a1);
			if (threadObj)
				{
				err = ReadRegistersLegacy(threadObj, (TRM_DebugRegisterInfo*)a2);
				}
			break;
			}
		case RRM_DebugDriver::EControlWriteRegistersLegacy:
			{
			threadObj = DebugUtils::OpenThreadHandle((TUint32)a1);
			if (threadObj)
				{
				err = WriteRegistersLegacy(threadObj, (TRM_DebugRegisterInfo*)a2);
				}
			break;
			}
		case RRM_DebugDriver::EControlReadRegisters:
			{
			threadObj = DebugUtils::OpenThreadHandle((TUint32)a1);
			if (threadObj)
				{
				err = ReadRegisters(threadObj, (TRM_DebugRegisterInformation*)a2);
				}
			break;
			}
		case RRM_DebugDriver::EControlWriteRegisters:
			{
			threadObj = DebugUtils::OpenThreadHandle((TUint32)a1);
			if (threadObj)
				{
				err = WriteRegisters(threadObj, (TRM_DebugRegisterInformation*)a2);
				}
			break;
			}
		case RRM_DebugDriver::EControlGetDebugFunctionalityBufSize:
			{
			LOG_MSG("RRM_DebugDriver::EControlGetDebugFunctionalityBufSize\n");

			TDebugFunctionality df;

			TUint size = df.GetDebugFunctionalityBufSize();

			// Return size to user-side in a safe manner
			err = Kern::ThreadRawWrite(iClientThread, a1, (TUint8*)&size, sizeof(TUint), iClientThread);
			break;
			}
		case RRM_DebugDriver::EControlGetDebugFunctionality:
			{
			LOG_MSG("RRM_DebugDriver::EControlGetDebugFunctionality\n");

			TDebugFunctionality df;

			TUint32 dfsize = df.GetDebugFunctionalityBufSize();

			// Alloc tmp buffer for Debug Functionality data
			NKern::ThreadEnterCS();
			TUint8* dfbuffer = (TUint8*)Kern::AllocZ(dfsize);
			NKern::ThreadLeaveCS();
			if (dfbuffer==NULL)
				{
				LOG_MSG2("Could not allocate memory for %d bytes\n",dfsize);

				// could not allocate memory
				return KErrNoMemory;
				}

			// Temporary descriptor to hold DF data
			TPtr8 tmpPtr(dfbuffer,0,dfsize);

			// Obtain the DF data
			if (df.GetDebugFunctionality(tmpPtr) )
				{
				// Return the DF data to the user-side
				err = Kern::ThreadDesWrite(iClientThread, a1, tmpPtr, 0, KChunkShiftBy0, iClientThread);
				}
			else
				{
				// Failed.
				err = KErrGeneral;
				}

			// Free tmp buffer
			NKern::ThreadEnterCS();
			Kern::Free(dfbuffer);
			NKern::ThreadLeaveCS();
			break;
			}
		case RRM_DebugDriver::EControlAttachProcess:
			{
			LOG_MSG("RRM_DebugDriver::EControlAttachProcess");

			err = AttachProcess(a1,a2);
			break;
			}
		case RRM_DebugDriver::EControlDetachProcess:
			{
			LOG_MSG("RRM_DebugDriver::EControlDetachProcess");

			err = DetachProcess(a1,a2);
			break;
			}
		case RRM_DebugDriver::EControlDetachAgent:
			{
			LOG_MSG("RRM_DebugDriver::EControlDetachAgent");

			err = DetachAgent(a1,a2);
			break;
			}
		case RRM_DebugDriver::EControlSetEventAction:
			{
			LOG_MSG("RRM_DebugDriver::EControlSetEventAction");

			err = SetEventAction(a1,a2);
			break;
			}
		case RRM_DebugDriver::EControlGetMemoryOperationMaxBlockSize:
			{
			LOG_MSG("RRM_DebugDriver::EControlGetMemoryOperationMaxBlockSize\n");

			TUint32 maxSize = TDebugFunctionality::GetMemoryOperationMaxBlockSize();

			// Return size to user-side in a safe manner
			err = Kern::ThreadRawWrite(iClientThread, a1, (TUint8*)&maxSize, sizeof(TUint32), iClientThread);
			break;
			}
		case RRM_DebugDriver::EControlGetList:
			{
			LOG_MSG("RRM_DebugDriver::EControlGetList\n");
			err = GetList((TListInformation*)a1);
			break;
			}
		case RRM_DebugDriver::EControlStep:
			{
			LOG_MSG("RRM_DebugDriver::EControlStep\n");

			err = Step((TUint32)a1,(TUint32)a2);
			break;
			}
		case RRM_DebugDriver::EControlKillProcess:
			{
			LOG_MSG("RRM_DebugDriver::EControlKillProcess\n");

			err = KillProcess((TUint32)a1,(TUint32)a2);
			break;
			}
		default:
			{
			err = KErrGeneral;
			}
		}

	if (KErrNone != err)
		{
		LOG_MSG2("Error %d from control function", err);
		}

	if (threadObj)
		{
		// Close the thread handle which has been opened by DebugUtils::OpenThreadHandle
		threadObj->Close(NULL);
		}

	return err;
	}

void DRM_DebugChannel::HandleMsg(TMessageBase* aMsg)
	{
	LOG_MSG("DRM_DebugChannel::HandleMsg()");

	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = m.iValue;

	if (id == (TInt)ECloseMsg)
		{
		if (iEventHandler)
			{
			iEventHandler->Stop();
			iEventHandler->Close();
			iEventHandler = NULL;
			}
		m.Complete(KErrNone, EFalse);
		return;
		}

	if (id == KMaxTInt)
		{
		// DoCancel
		DoCancel(m.Int0());
		m.Complete(KErrNone, ETrue);
		return;
		}

	if (id < 0)
		{
		// DoRequest
		TRequestStatus* pStatus = (TRequestStatus*)m.Ptr0();
		DoRequest(~id, pStatus, m.Ptr1(), m.Ptr2());
		m.Complete(KErrNone, ETrue);
		}
	else
		{
		// DoControl
		TInt err = DoControl(id, m.Ptr0(), m.Ptr1());
		m.Complete(err, ETrue);
		}
	}

//
// DRM_DebugChannel::RemoveProcess
//
TBool DRM_DebugChannel::RemoveProcess(TAny* a1, TAny* a2)
	{
	LOG_MSG("DRM_DebugChannel::RemoveProcess()");

	DProcess *aProcess = (DProcess*)a1;

	// Sanity check
	if (!aProcess)
		{
		// No process was specified!
		LOG_MSG("DRM_DebugChannel::RemoveProcess was called with an invalid process ID");
		return EFalse;
		}

	// this is called when a process dies.  we want to mark any breakpoints in this
	// process space as obsolete.  the main reason for this is so we don't return
	// an error when the host debugger tries to clear breakpoints for the process

	TUint32 codeAddress = 0;
	TUint32 codeSize = 0;

	LOG_EVENT_MSG2("Process being removed, Name %S", aProcess->iName);

	DCodeSeg* codeSeg = aProcess->iCodeSeg;

	if (codeSeg)
		{
		TModuleMemoryInfo processMemoryInfo;
		TInt err = codeSeg->GetMemoryInfo(processMemoryInfo, aProcess);
		if (err != KErrNone)
			{
			codeAddress = processMemoryInfo.iCodeBase;
			codeSize = processMemoryInfo.iCodeSize;
			}
		else
			{
			LOG_MSG2("Error in getting memory info: %d", err);
			}
		}

	if (!codeAddress || !codeSize)
		{
		LOG_EVENT_MSG2("Code segment not available for process %d", aProcess->iId);
		// make sure there is not already a breakpoint at this address
		for (TInt i = 0; i < iDebugProcessList.Count(); i++)
			{
			if (iDebugProcessList[i].iId == aProcess->iId)
				{
				codeAddress = iDebugProcessList[i].iCodeAddress;
				codeSize = iDebugProcessList[i].iCodeSize;

				//now remove from the list
				iDebugProcessList.Remove(i);
				break;
				}
			}
		}

	if (!codeAddress || !codeSize)
		{
		return EFalse;
		}

	iBreakManager->RemoveBreaksForProcess(aProcess->iId, codeAddress, codeSize);
	return EFalse;
	}

//
// DRM_DebugChannel::StartThread
//
TBool DRM_DebugChannel::StartThread(TAny* a1, TAny* a2)
	{
	LOG_EVENT_MSG("DRM_DebugChannel::StartThread()");

	DThread *aThread = (DThread*)a1;
	if(!aThread)
		{
		LOG_MSG("Error getting DThread object");
		__NK_ASSERT_DEBUG(aThread);
		return EFalse;
		}

	//a2 points to the thread creating the new thread.
	//We have no use for it at the moment so just ignore it for now

	TDriverEventInfo info;
	info.iEventType = EEventsStartThread;
	info.iThreadId = aThread->iId;
	info.iThreadIdValid = ETrue;
	DProcess* owningProcess = aThread->iOwningProcess;
	if(owningProcess)
		{
		info.iProcessId = owningProcess->iId;
		info.iProcessIdValid = ETrue;
		DCodeSeg* p = owningProcess->iCodeSeg;
		if(p && p->iFileName)
			{
			info.iFileName.Copy(*(p->iFileName));
			DTargetProcess* foundProcess = TheDProcessTracker.FindProcess(*(p->iFileName));
			if(foundProcess)
				{
				foundProcess->NotifyEvent(info);
				}
			else
				{
				LOG_EVENT_MSG2("Couldn't find process with name [%S]", p->iFileName);
				}
			}
		else
			{
			if(p)
				{
				LOG_EVENT_MSG("\tCode segment name missing");
				}
			else
				{
				LOG_EVENT_MSG("\tCode segment is NULL");
				}
			}
		}
	return EFalse;
	}

//
// DRM_DebugChannel::HandleAddProcessEvent
//
TBool DRM_DebugChannel::HandleAddProcessEvent(TAny* a1, TAny* a2)
	{
	LOG_EVENT_MSG("DRM_DebugChannel::AddProcess()");

	DProcess *aProcess = (DProcess*)a1;
	// a2 points to the thread creating the new process.
	DThread *aThread = (DThread*)a2;

	if(!aProcess)
		{
		LOG_MSG("Error getting DProcess object");
		__NK_ASSERT_DEBUG(aProcess);
		return EFalse;
		}

	TDriverEventInfo info;
	info.iEventType = EEventsAddProcess;
	info.iProcessId = aProcess->iId;

	info.iCreatorThreadId  = aThread ? aThread->iId : 0;
	info.iProcessIdValid = ETrue;

	// Copy TUids
	info.iUids = aProcess->iUids;

	info.iUidsValid = ETrue;

	// copy name of the process
	if (aProcess->iName)
		{
		// copy the name of the process
		info.iFileName.Copy(*aProcess->iName);

		DTargetProcess* foundProcess = TheDProcessTracker.FindProcess(*(aProcess->iName));
		if(foundProcess)
			{
			foundProcess->NotifyEvent(info);
			}
		else
			{
			// AddProcess event does not have fully-qualified path, it has "filename.exe"
			// So we try a less-precise match
			DTargetProcess* foundProcess = TheDProcessTracker.FuzzyFindProcess(*(aProcess->iName));
			if(foundProcess)
				{
				foundProcess->NotifyEvent(info);
				}
			else
				{
				LOG_EVENT_MSG2("Couldn't find process with name [%S]", aProcess->iName);
				}
			}
		}
	else
		{
		LOG_EVENT_MSG("DRM_DebugChannel::AddProcess - No iName for this process");
		}

	return EFalse;
	}

//
// DRM_DebugChannel::HandleRemoveProcessEvent
//
TBool DRM_DebugChannel::HandleRemoveProcessEvent(TAny* a1, TAny* a2)
	{
	LOG_EVENT_MSG("DRM_DebugChannel::RemoveProcess()");

	DProcess *aProcess = (DProcess*)a1;
	if(!aProcess)
		{
		LOG_MSG("Error getting DProcess object");
		__NK_ASSERT_DEBUG(aProcess);
		return EFalse;
		}

	// a2 points to the thread creating the new process.
	// We have no use for it at the moment so just ignore it for now
	// Also, it may not be known and therefore NULL

	TDriverEventInfo info;
	info.iEventType = EEventsRemoveProcess;
	info.iProcessId = aProcess->iId;
	info.iProcessIdValid = ETrue;

	// copy name of the process
	if (aProcess->iName)
		{
		// copy the name of the process
		info.iFileName.Copy(*aProcess->iName);

		DTargetProcess* foundProcess = TheDProcessTracker.FindProcess(*(aProcess->iName));
		if(foundProcess)
			{
			foundProcess->NotifyEvent(info);
			}
		else
			{
			// RemoveProcess event does not have fully-qualified path, it has "filename.exe"
			// So we try a less-precise match
			DTargetProcess* foundProcess = TheDProcessTracker.FuzzyFindProcess(*(aProcess->iName));
			if(foundProcess)
				{
				foundProcess->NotifyEvent(info);
				}
			else
				{
				LOG_EVENT_MSG2("Couldn't find process with name [%S]", aProcess->iName);
				}
			}

		}
	else
		{
		LOG_EVENT_MSG("DRM_DebugChannel::AddProcess - No iName for this process");
		}

	return EFalse;
	}

//
// DRM_DebugChannel::AddLibrary
//
TBool DRM_DebugChannel::AddLibrary(TAny* a1, TAny* a2)
	{
	LOG_EVENT_MSG("DRM_DebugChannel::AddLibrary()");

	DLibrary *aLibrary = (DLibrary*)a1;
	DThread *aThread = (DThread*)a2;

	// sanity check
	if (!aLibrary)
		{
		LOG_EVENT_MSG("DRM_DebugChannel::AddLibrary called with no library specified");
		return EFalse;
		}

	if (!aThread)
		{
		LOG_EVENT_MSG("DRM_DebugChannel::AddLibrary called with no thread specified");
		return EFalse;
		}

	LOG_EVENT_MSG2(("Lib loaded: %S"), aLibrary->iName);

	if (aThread)
		{
		// make sure this is not the debugger thread
		if ((aThread != iClientThread) && (aThread->iOwningProcess->iId != iClientThread->iOwningProcess->iId))
			{
			TDriverEventInfo info;

			info.iEventType = EEventsAddLibrary;
			info.iProcessId = aThread->iOwningProcess->iId;
			info.iProcessIdValid = ETrue;
			info.iThreadId = aThread->iId;
			info.iThreadIdValid = ETrue;

			//get the code address
			DCodeSeg* codeSeg = aLibrary->iCodeSeg;
			if (!codeSeg)
				{
				LOG_EVENT_MSG2("Code segment not available for library %S", aLibrary->iName);
				return EFalse;
				}

			// Uid3
			info.iUids = codeSeg->iUids;
			info.iUidsValid = ETrue;

			TModuleMemoryInfo memoryInfo;
			TInt err = codeSeg->GetMemoryInfo(memoryInfo, NULL); //NULL for DProcess should be ok;
			if (err != KErrNone)
				{
				LOG_EVENT_MSG2("Error in getting memory info: %d", err);
				return EFalse;
				}

			info.iCodeAddress = memoryInfo.iCodeBase;
			info.iDataAddress = memoryInfo.iInitialisedDataBase;

			info.iFileName.Copy(*(aLibrary->iName)); //just the name, without uid info.

			//queue up or complete the event
			info.iArg1 = a1;
			info.iArg2 = a2;
			NotifyEvent(info);
			}

		}
	return EFalse;
	}

//
// DRM_DebugChannel::RemoveLibrary
//
TBool DRM_DebugChannel::RemoveLibrary(TAny* a1, TAny* a2)
	{
	LOG_EVENT_MSG("DRM_DebugChannel::RemoveLibrary()");
	DLibrary *aLibrary = (DLibrary*)a1;

	// sanity check
	if (!aLibrary)
		{
		LOG_EVENT_MSG("DRM_DebugChannel::RemoveLibrary called with no library specified");
		return EFalse;
		}

	LOG_EVENT_MSG2(("Lib unloaded: %S"), aLibrary->iName);

	// this is called when all handles to this library have been closed.  this can happen when a process dies, or when a dll is
	// unloaded while the process lives on.  in former case, we don't need to notify the host debugger because that process is
	// dying anyway.  for the latter case, we do need to notify the host so it can unload the symbolics, etc.

	DThread* aThread = &Kern::CurrentThread();

	if ((aThread) &&
			(aThread != iClientThread) &&
			(aThread->iOwningProcess->iId != iClientThread->iOwningProcess->iId))
		{
		//the library gets unloaded only when the mapcount is 0.
		if (aLibrary->iMapCount != 0)
			return EFalse;

		DCodeSeg* codeSeg = aLibrary->iCodeSeg;
		if (!codeSeg)
			{
			LOG_EVENT_MSG2("Code segment not available for library %S", aLibrary->iName);
			return EFalse;
			}

		TModuleMemoryInfo processMemoryInfo;
		TInt err = codeSeg->GetMemoryInfo(processMemoryInfo, NULL); //passing NULL for the DProcess argument should be ok;
		if (err != KErrNone)
			{
			LOG_EVENT_MSG2("Error in getting memory info: %d", err);
			return EFalse;
			}

		TUint32 codeAddress = processMemoryInfo.iCodeBase;
		TUint32 codeSize = processMemoryInfo.iCodeSize;

		// first invalidate all breakpoints that were set in the library code
		iBreakManager->InvalidateLibraryBreakPoints(codeAddress, codeSize);
		DProcess *process = &Kern::CurrentProcess();
		RArray<SCodeSegEntry>* dynamicCode = &(process->iDynamicCode);

		for (TInt j=0; j<dynamicCode->Count(); j++)
			{
			if ((*dynamicCode)[j].iLib == aLibrary)
				{
				TDriverEventInfo info;

				info.iEventType = EEventsRemoveLibrary;
				info.iFileName.Copy(*(aLibrary->iName)); //lib name without uid info
				//info.iFileName.ZeroTerminate();
				info.iProcessId = process->iId;
				info.iProcessIdValid = ETrue;
				info.iThreadId = 0xFFFFFFFF; // don't care!
				info.iThreadIdValid = EFalse;
				// Uid3
				info.iUids = codeSeg->iUids;
				info.iUidsValid = ETrue;

				//queue up or complete the event
				info.iArg1 = a1;
				info.iArg2 = a2;
				NotifyEvent(info);
				}
			}
		}
	return EFalse;
	}

//
// DRM_DebugChannel::HandleEventKillThread
//
TBool DRM_DebugChannel::HandleEventKillThread(TAny* a1, TAny* a2)
	{

	LOG_MSG2("DRM_DebugChannel::HandleEventKillThread(Thread a1=0x%08x)", a1 );

	DThread* currentThread = &Kern::CurrentThread();
	if (!currentThread)
		{
		LOG_MSG("Error getting current thread");
		__NK_ASSERT_DEBUG(currentThread);
		return EFalse;
		}

	// a1 should point to the current thread, check this to make sure it does
	__NK_ASSERT_DEBUG((DThread*)a1 == currentThread);

	TDriverEventInfo info;

	info.iProcessId = currentThread->iOwningProcess->iId;
	info.iProcessIdValid = ETrue;
	info.iThreadId = currentThread->iId;
	info.iThreadIdValid = ETrue;
	// 14 should probably be replaced by PC_REGISTER, for some reason PC_REGISTER had been replaced with 14 in the code
	TInt err = ReadKernelRegisterValue(currentThread, 14, info.iCurrentPC);
	if(err != KErrNone)
		{
		LOG_EVENT_MSG2("DRM_DebugChannel::HandleEventKillThread - Non-zero error code discarded: %d", err);
		}

	if (currentThread->iExitType == EExitPanic)
		{
		info.iPanicCategory.Copy(currentThread->iExitCategory);
		}
	info.iExceptionNumber = currentThread->iExitReason;
	info.iExitType = currentThread->iExitType;
	info.iEventType = EEventsKillThread;

	// Are we debugging this process - decide based on iFileName
	DCodeSeg* codeSeg = currentThread->iOwningProcess->iCodeSeg;

	// remove all the breakpoints in this thread, whether we are debugging it or not.
	iBreakManager->DoRemoveThreadBreaks(info.iThreadId);

	// if the code seg exists then get the file name from it and check we're debugging it
	if(codeSeg)
		{
		DTargetProcess* foundProcess = TheDProcessTracker.FindProcess(*(codeSeg->iFileName));
		if(!foundProcess)
			{
			// not debugging this process so return false
			return EFalse;
			}
		}
	else
		{
		// can't validate that we are debugging the thread
		return EFalse;
		}

	info.iArg1 = a1;
	info.iArg2 = a2;
	NotifyEvent(info);

	return ETrue;
	}

//
// DRM_DebugChannel::HandleSwException
//
TBool DRM_DebugChannel::HandleSwException(TAny* a1, TAny* a2)
	{
	LOG_EVENT_MSG("DRM_DebugChannel::HandleSwException");
	TExcType aExcType = (TExcType)(TInt)a1;

	TDriverEventInfo info;

	DThread* currentThread = &Kern::CurrentThread();
	if (!currentThread)
		{
		LOG_MSG("Error getting current thread");
		__NK_ASSERT_DEBUG(currentThread);
		return EFalse;
		}

	info.iProcessId = currentThread->iOwningProcess->iId;
	info.iProcessIdValid = ETrue;
	info.iThreadId = currentThread->iId;
	info.iThreadIdValid = ETrue;
	TInt err = ReadKernelRegisterValue(currentThread, PC_REGISTER, info.iCurrentPC);
	if(err != KErrNone)
		{
		LOG_EVENT_MSG2("DRM_DebugChannel::HandleSwException - Non-zero error code discarded: %d", err);
		}
	info.iExceptionNumber = aExcType;
	info.iEventType = EEventsSwExc;
	info.iArg1 = a1;
	info.iArg2 = a2;

	NotifyEvent(info);

	return EFalse;
	}

//
// DRM_DebugChannel::HandleHwException
//
TBool DRM_DebugChannel::HandleHwException(TAny* a1, TAny* a2)
	{
	TArmExcInfo* aExcInfo = (TArmExcInfo*)a1;

	// sanity check
	if (!aExcInfo)
		{
		LOG_MSG("DRM_DebugChannel::HandleHwException called with no aExcInfo");
		__NK_ASSERT_DEBUG(aExcInfo);
		return EFalse;
		}

	TDriverEventInfo info;

	DThread* currentThread = &Kern::CurrentThread();

	if (!currentThread)
		{
		LOG_MSG("Error getting current thread");
		__NK_ASSERT_DEBUG(currentThread);
		return EFalse;
		}

	info.iProcessId = currentThread->iOwningProcess->iId;
	info.iProcessIdValid = ETrue;
	info.iThreadId = currentThread->iId;
	info.iThreadIdValid = ETrue;
	info.iRmdArmExcInfo.iFaultAddress= aExcInfo->iFaultAddress;
	info.iRmdArmExcInfo.iFaultStatus= aExcInfo->iFaultStatus;

	LOG_MSG5("DRM_DebugChannel::HandleHwException current thread = 0x%08x, CritSect count=%d,\n"
		" iFaultAddress=0x%08x, iFaultStatus=0x%08x",
		currentThread, currentThread->iNThread.iCsCount, aExcInfo->iFaultAddress, aExcInfo->iFaultStatus);

	info.iRmdArmExcInfo.iR0= aExcInfo->iR0;
	info.iRmdArmExcInfo.iR1= aExcInfo->iR1;
	info.iRmdArmExcInfo.iR2= aExcInfo->iR2;
	info.iRmdArmExcInfo.iR3= aExcInfo->iR3;

	info.iRmdArmExcInfo.iR4= aExcInfo->iR4;
	info.iRmdArmExcInfo.iR5= aExcInfo->iR5;
	info.iRmdArmExcInfo.iR6= aExcInfo->iR6;
	info.iRmdArmExcInfo.iR7= aExcInfo->iR7;
	info.iRmdArmExcInfo.iR8= aExcInfo->iR8;
	info.iRmdArmExcInfo.iR9= aExcInfo->iR9;
	info.iRmdArmExcInfo.iR10= aExcInfo->iR10;
	info.iRmdArmExcInfo.iR11= aExcInfo->iR11;
	info.iRmdArmExcInfo.iR12= aExcInfo->iR12;

	info.iRmdArmExcInfo.iR13= aExcInfo->iR13;
	info.iRmdArmExcInfo.iR14= aExcInfo->iR14;
	info.iRmdArmExcInfo.iR15= aExcInfo->iR15;

	info.iRmdArmExcInfo.iCpsr= aExcInfo->iCpsr;
	info.iRmdArmExcInfo.iR13Svc= aExcInfo->iR13Svc;
	info.iRmdArmExcInfo.iR14Svc= aExcInfo->iR14Svc;
	info.iRmdArmExcInfo.iSpsrSvc= aExcInfo->iSpsrSvc;
	LOG_MSG5(" iCpsr=0x%x, iExcCode=0x%x, R14=0x%x, R15=0x%x",
			aExcInfo->iCpsr, aExcInfo->iExcCode, aExcInfo->iR14, aExcInfo->iR15);

	switch (aExcInfo->iExcCode)
		{
		case 0:
			info.iExceptionNumber = EExcCodeAbort;
			LOG_EVENT_MSG(" iExcCode == 0 => EExcCodeAbort");
			break;
		case 1:
			info.iExceptionNumber = EExcDataAbort;
			LOG_EVENT_MSG(" iExcCode == 1 => EExcDataAbort");
			break;
		case 2:
			info.iExceptionNumber = EExcInvalidOpCode;
			LOG_EVENT_MSG(" iExcCode == 2 => EExcInvalidOpCode");
			break;
		default:
			// new event? Something gone wrong?
			__NK_ASSERT_DEBUG(EFalse);
			return EFalse;
		}

	info.iEventType = EEventsHwExc;

	info.iArg1 = a1;
	info.iArg2 = a2;

	if(EExcInvalidOpCode == info.iExceptionNumber)
		{
		return HandleInvalidOpCodeException(info, currentThread);
		}

	NotifyEvent(info);
	return EFalse;
	}

//
// DRM_DebugChannel::HandUserTrace
//
TBool DRM_DebugChannel::HandleUserTrace(TAny* a1, TAny* a2)
	{
	LOG_EVENT_MSG("DRM_DebugChannel::HandleUserTrace()");

	DThread* currentThread = &Kern::CurrentThread();
	if (!currentThread)
		{
		LOG_EVENT_MSG("Error getting current thread");
		__NK_ASSERT_DEBUG(currentThread);
		return EFalse;
		}

	TDriverEventInfo info;
	info.iProcessId = currentThread->iOwningProcess->iId;
	info.iProcessIdValid = ETrue;
	info.iThreadId = currentThread->iId;
	info.iThreadIdValid = ETrue;
	info.iEventType = EEventsUserTrace;
	info.iArg1 = a1;
	info.iArg2 = a2;

	TInt err = KErrNone;

	//User Trace info
	XTRAP(err, XT_DEFAULT, kumemget(info.iUserTraceText, info.iArg1, (TInt)a2));
	if(KErrNone != err)
		{
		return EFalse;
		}

	info.iMessageStatus = ESingleMessage;

	NotifyEvent(info);

	return EFalse;
	}

//
// DRM_DebugChannel::HandleException
//
TBool DRM_DebugChannel::HandleInvalidOpCodeException(TDriverEventInfo& aEventInfo, DThread* aCurrentThread)
	{
	LOG_EVENT_MSG("DRM_DebugChannel::HandleInvalidOpCodeException()");

	TInt err = KErrNone;

	TUint32 inst = KArmBreakPoint;
	TInt instSize = 4;

	// change these for thumb mode
	TUint32 regValue;
	err = ReadKernelRegisterValue(aCurrentThread, STATUS_REGISTER, regValue);
	if(err != KErrNone)
		{
		LOG_EVENT_MSG2("DRM_DebugChannel::HandleInvalidOpCodeException - Non-zero error code discarded: %d", err);
		}

	if (regValue & ECpuThumb)
		{
		inst = KThumbBreakPoint;
		instSize = 2;
		}

	TUint32 instruction = 0;
	err = Kern::ThreadRawRead(aCurrentThread, (TUint32 *)aEventInfo.iRmdArmExcInfo.iR15, (TUint8 *)&instruction, instSize);

	if (KErrNone != err)
		LOG_MSG2("Error reading instruction at currentpc: %d", err);

	if (!memcompare((TUint8 *)&inst, instSize, (TUint8 *)&instruction, instSize))
		{
		TInt err = DoSuspendThread(aCurrentThread);
		if(! ((KErrNone == err) || (KErrAlreadyExists == err)) )
			{
			LOG_MSG2("DRM_DebugChannel::HandleInvalidOpCodeException() Thread with id 0x%08x could not be suspended.", aCurrentThread->iId);
			return EFalse;
			}

		// the exception was a breakpoint instruction.  see if we have a breakpoint at that address
		TBreakEntry* breakEntry = NULL;
		do
			{
			breakEntry = iBreakManager->GetNextBreak(breakEntry);
			if (breakEntry && ((breakEntry->iThreadSpecific && breakEntry->iId == aEventInfo.iThreadId) || (!breakEntry->iThreadSpecific && breakEntry->iId == aEventInfo.iProcessId)) && breakEntry->iAddress == aEventInfo.iRmdArmExcInfo.iR15)
				{
				LOG_EVENT_MSG2("Breakpoint with Id %d has been hit", breakEntry->iBreakId);

				TBreakEntry tempBreakEntry = *breakEntry;

				//change the event type to breakpoint type
				aEventInfo.iEventType = breakEntry->iThreadSpecific ? EEventsBreakPoint : EEventsProcessBreakPoint;

				// enable any breakpoints we had to disable for this thread
				err = iBreakManager->DoEnableDisabledBreak(aEventInfo.iThreadId);
				if (KErrNone != err)
					LOG_MSG2("Error %d enabling disabled breakpoints", err);

				// see if this is a temp breakpoint
				if (iBreakManager->IsTemporaryBreak(*breakEntry))
					{
					// this was a temp breakpoint, so we need to clear it now
					err = iBreakManager->DoClearBreak(breakEntry->iBreakId);
					if (KErrNone != err)
						LOG_MSG2("Error %d clearing temp breakpoint", err);

					// Find out how many steps remain to be done

					// reduce the number of steps to complete by 1
					tempBreakEntry.iNumSteps--;

					LOG_EVENT_MSG2("There are %d steps remaining\n", tempBreakEntry.iNumSteps);

					// New. If we have not finished do all the steps, continue stepping and don't notify event
					if (tempBreakEntry.iNumSteps)
						{
						LOG_EVENT_MSG("Continuing stepping...not telling the agent yet\n");
						err = DoStepRange(aCurrentThread, aEventInfo.iRmdArmExcInfo.iR15, aEventInfo.iRmdArmExcInfo.iR15, ETrue, tempBreakEntry.iResumeOnceOutOfRange /*EFalse*/, tempBreakEntry.iNumSteps, ETrue);
						if (err != KErrNone)
							{
							LOG_EVENT_MSG("Failed to continue stepping\n");

							// what do we do? might as well stop here and tell the user
							NotifyEvent(aEventInfo);

							return ETrue;
							}

						// continue as though no event occured. No need to suspend/resume anything...
						LOG_EVENT_MSG("Continuing to step\n");
						return ETrue;
						}

					// Is this a case where we just want to continue?
					if (tempBreakEntry.iResumeOnceOutOfRange)
						{
						LOG_EVENT_MSG("PC is out of range, continuing thread");
						DoResumeThread(aCurrentThread);

						return ETrue;
						}
					}

				// if the breakpoint is thread specific, make sure it's the right thread
				// if not, just continue the thread.  take special care if it's the debugger
				// thread.  if it hits a regular breakpoint, we NEVER want to stop at it.  if
				// it hits a temp breakpoint, we're probably just stepping past a real breakpoint
				// and we do need to handle it.
				TBool needToResume = (tempBreakEntry.iThreadSpecific && tempBreakEntry.iId != aEventInfo.iThreadId) ||
					(!tempBreakEntry.iThreadSpecific && tempBreakEntry.iId != aEventInfo.iProcessId);

				if (needToResume)
					{
					LOG_EVENT_MSG("breakpoint does not match threadId, calling DoResumeThread");
					err = DoResumeThread(aCurrentThread);
					if (KErrNone != err)
						LOG_MSG2("Error in DoResumeThread: %d", err);

					return EFalse;
					}

				//normal user break point, just notify the event
				break;
				}
			} while(breakEntry);
		}

	NotifyEvent(aEventInfo);

	return (aEventInfo.iEventType == EEventsBreakPoint) || (aEventInfo.iEventType == EEventsProcessBreakPoint);
	}

//
// DRM_DebugChannel::SetBreak
//
TInt DRM_DebugChannel::SetBreak(TSetBreakInfo* aBreakInfo)
	{
	LOG_MSG("DRM_DebugChannel::SetBreak()");

	TInt err = KErrNone;

	if (!aBreakInfo)
		{
		LOG_MSG("DRM_DebugChannel::SetBreak() was passed a NULL argument");
		return KErrArgument;
		}

	//User side memory is not accessible directly
	TSetBreakInfo info;
	err = Kern::ThreadRawRead(iClientThread, aBreakInfo, (TUint8*)&info, sizeof(TSetBreakInfo));
	if (err != KErrNone)
		{
		LOG_MSG("DRM_DebugChannel::SetBreak() was passed a bad argument");
		return err;
		}

	DProcess* process = NULL;
	if(info.iThreadSpecific)
		{
		// if the target thread is not suspended then return KErrInUse
		if(!TheDProcessTracker.CheckSuspended(info.iId))
			{
			LOG_MSG2("DRM_DebugChannel::SetBreak() Thread with id 0x%08x not suspended.", info.iId);
			return KErrInUse;
			}
		DThread* thread = DebugUtils::OpenThreadHandle(info.iId);
		if(!thread)
			{
			LOG_MSG2("DRM_DebugChannel::SetBreak() Thread with id 0x%08x not found", info.iId);
			return KErrNotFound;
			}
		process = DebugUtils::OpenProcessHandle(thread->iOwningProcess->iId);
		thread->Close(NULL);
		}
	else
		{
		process = DebugUtils::OpenProcessHandle(info.iId);
		}

	if(!process)
		{
		LOG_MSG2("DRM_DebugChannel::SetBreak() Process with id 0x%08x not found", process->iId);
		return KErrNotFound;
		}

	TBool found = EFalse;
	for(TInt i=0; i<iDebugProcessList.Count(); i++)
		{
		if(process->iId == iDebugProcessList[i].iId)
			{
			found = ETrue;
			}
		}

	if(!found)
		{
		DCodeSeg* codeSeg = process->iCodeSeg;
		if (!codeSeg)
			{
			LOG_MSG2("DRM_DebugChannel::SetBreak() Code seg for process with id 0x%08x not found", process->iId);
			return KErrNotFound;
			}

		TModuleMemoryInfo memoryInfo;
		TInt err = codeSeg->GetMemoryInfo(memoryInfo, process);
		if (err != KErrNone)
			{
			LOG_MSG2("DRM_DebugChannel::SetBreak() Error getting memory info for process with id 0x%08x", process->iId);
			return err;
			}

		//add this process to the list of processes that we are debugging
		TProcessInfo processInfo(process->iId, memoryInfo.iCodeBase, memoryInfo.iCodeSize, memoryInfo.iInitialisedDataBase);
		iDebugProcessList.Append(processInfo);
		process->Close(NULL);
		}

	if (!info.iBreakId) //first check if the iId address is valid
		return KErrArgument;

	if (err == KErrNone)
		{
		TInt32 iBreakId;

		err = iBreakManager->DoSetBreak(iBreakId, info.iId, info.iThreadSpecific, info.iAddress, info.iMode );

		if (err == KErrNone)
			{
			err = Kern::ThreadRawWrite(iClientThread, (TUint8 *)info.iBreakId, &iBreakId, sizeof(TInt32), iClientThread);
			}
		}

	return err;
	}

//
// DRM_DebugChannel::StepRange
//
TInt DRM_DebugChannel::StepRange(DThread* aThread, TRM_DebugStepInfo* aStepInfo)
	{
	LOG_MSG("DRM_DebugChannel::StepRange()");

	TInt err = KErrNone;

	if(!TheDProcessTracker.CheckSuspended(aThread))
		{
		LOG_MSG2("DRM_DebugChannel::StepRange() Thread with id 0x%08x not suspended.", aThread->iId);
		return KErrInUse;
		}

	if (!aStepInfo)
		return KErrArgument;

	TRM_DebugStepInfo info(0, 0, 0);
	err = Kern::ThreadRawRead(iClientThread, aStepInfo, (TUint8*)&info, sizeof(TRM_DebugStepInfo));

	if (err != KErrNone)
		return err;

	err = DoStepRange(aThread, info.iStartAddress, info.iStopAddress, info.iStepInto, EFalse, ETrue);

	return err;
	}

/**
Read memory from a target thread and return the data to the client. If the
memory block has breakpoints in it then the correct values are placed in the
returned data

@param aThread pointer to thread whose memory space the memory is to be read from
@param aMemoryInfo information about what memory to read

@return KErrNone if memory read successfully,
        KErrArgument if aMemoryInfo is not initialised correctly,
        KErrNoMemory if a temporary buffer could not be allocated,
        KErrBadHandle if aThread is invalid,
        or another of the system wide error codes
*/
TInt DRM_DebugChannel::ReadMemory(DThread* aThread, TRM_DebugMemoryInfo* aMemoryInfo)
	{
	LOG_MSG("DRM_DebugChannel::ReadMemory()");

	TInt err = KErrNone;

	if (!aMemoryInfo)
		return KErrArgument;

	TRM_DebugMemoryInfo info(0, 0, 0);
	err = Kern::ThreadRawRead(iClientThread, aMemoryInfo, (TUint8*)&info, sizeof(TRM_DebugMemoryInfo));
	if (err != KErrNone)
		return err;

	if (!info.iData)
		return KErrArgument;

	NKern::ThreadEnterCS();
	TUint8 *data = (TUint8*)Kern::Alloc(info.iLength);
	NKern::ThreadLeaveCS();
	if (!data)
		{
		return KErrNoMemory;
		}

	TPtr8 dataDes(data, info.iLength);

	err = DoReadMemory(aThread, info.iAddress, info.iLength, dataDes);
	if (err == KErrNone)
		{
		err = Kern::ThreadDesWrite(iClientThread, info.iData, dataDes, 0, KChunkShiftBy0, iClientThread);
		}

	NKern::ThreadEnterCS();
	Kern::Free(data);
	NKern::ThreadLeaveCS();

	return err;
	}

/**
Attempt to write memory to aThread's address space

@param aThread thread to whose address space memory is to be written
@param aMemoryInfo memory info object representing the data to write

@return KErrNone if memory written successfully,
        KErrNoMemory if memory could not be allocated
        KErrArgument if aMemoryInfo is NULL, if aMemoryInfo.iData is NULL,
        if aMemoryInfo.iLength is greater than than the length of the passed
        in descrptor
        KErrBadHandle if aThread is invalid,
	or another of the system wide error codes
*/
TInt DRM_DebugChannel::WriteMemory(DThread* aThread, TRM_DebugMemoryInfo* aMemoryInfo)
	{
	LOG_MSG("DRM_DebugChannel::WriteMemory()");

	TInt err = KErrNone;

	if (!aMemoryInfo)
		return KErrArgument;

	TRM_DebugMemoryInfo info(0, 0, 0);
	err = Kern::ThreadRawRead(iClientThread, aMemoryInfo, (TUint8*)&info, sizeof(TRM_DebugMemoryInfo));
	if (err != KErrNone)
		return err;

	if (!info.iData)
		return KErrArgument;

	NKern::ThreadEnterCS();
	TUint8 *data = (TUint8*)Kern::Alloc(info.iLength);
	NKern::ThreadLeaveCS();
	if (!data)
		{
		return KErrNoMemory;
		}

	TPtr8 dataDes(data, info.iLength);

	err = Kern::ThreadDesRead(iClientThread, info.iData, dataDes, 0);
	if (err == KErrNone)
		{
		err = DoWriteMemory(aThread, info.iAddress, info.iLength, dataDes);
		}

	NKern::ThreadEnterCS();
	Kern::Free(data);
	NKern::ThreadLeaveCS();

	return err;
	}

//
// DRM_DebugChannel::ReadRegisters
//
TInt DRM_DebugChannel::ReadRegistersLegacy(DThread* aThread, TRM_DebugRegisterInfo* aRegisterInfo)
	{
	LOG_MSG("DRM_DebugChannel::ReadRegistersLegacy()");

	TInt err = KErrNone;

	if (!aRegisterInfo)
		return KErrArgument;

	TRM_DebugRegisterInfo info(0, 0, 0);
	err = Kern::ThreadRawRead(iClientThread, aRegisterInfo, (TUint8*)&info, sizeof(TRM_DebugRegisterInfo));
	if (err != KErrNone)
		return err;

	if (!info.iValues)
		return KErrArgument;

	TUint length = (info.iLastRegister - info.iFirstRegister + 1) * 4;

	NKern::ThreadEnterCS();
	TUint8 *values = (TUint8*)Kern::Alloc(length);
	NKern::ThreadLeaveCS();
	if (!values)
		{
		return KErrNoMemory;
		}

	TPtr8 valuesDes(values, length);

	err = DoReadRegisters(aThread, info.iFirstRegister, info.iLastRegister, valuesDes);
	if (err == KErrNone)
		{
		err = Kern::ThreadDesWrite(iClientThread, info.iValues, valuesDes, 0, KChunkShiftBy0, iClientThread);
		}

	NKern::ThreadEnterCS();
	Kern::Free(values);
	NKern::ThreadLeaveCS();

	return err;
	}

/**
Get listing information.

@param aListInformation pointer to a TListInformation object containing the
       user specified listings information

@return KErrNone on success,
        KErrTooBig if the kernel's data is too big to fit in the passed buffer,
        KErrArgument if aListInformation is NULL,
	or one of the other system-wide error codes
*/
TInt DRM_DebugChannel::GetList(TListInformation* aListInformation) const
	{
	LOG_MSG("DRM_DebugChannel::GetList()");

	TInt err = KErrNone;

	if(aListInformation == NULL)
		{
		return KErrArgument;
		}

	//read DSS' data into local structure
	TListInformation info;
	err = Kern::ThreadRawRead(iClientThread, aListInformation, (TUint8*)&info, sizeof(TListInformation));
	if(err != KErrNone)
		{
		return err;
		}

	//check arguments
	TPtr8 buffer(NULL, 0);
	err = AllocAndReadDes(iClientThread, *info.iBuffer, buffer);
	if(err != KErrNone)
		{
		//need to free the buffer if it was allocated
		if(err != KErrNoMemory)
			{
			NKern::ThreadEnterCS();
			Kern::Free((TAny*)buffer.Ptr());
			NKern::ThreadLeaveCS();
			}
		return err;
		}

	//get the list
	TUint32 dataSize = 0;
	TListManager manager;
	err = KErrArgument;
	switch(info.iType)
		{
		case EXipLibraries:
			if(Debug::EScopeGlobal == info.iListScope)
				{
				err = manager.GetXipLibrariesList(buffer, dataSize);
				}
			break;

		case EThreads:
			if(Debug::EScopeGlobal == info.iListScope)
				{
				err = manager.GetGlobalThreadList(buffer, dataSize);
				}
			else if(Debug::EScopeProcessSpecific == info.iListScope)
				{
				err = manager.GetThreadListForProcess(buffer, dataSize, info.iTargetId);
				}
			else if(Debug::EScopeThreadSpecific == info.iListScope)
				{
				err = manager.GetThreadListForThread(buffer, dataSize, info.iTargetId);
				}
			break;

		case EProcesses:
			if(Debug::EScopeGlobal == info.iListScope)
				{
				err = manager.GetProcessList(buffer, dataSize);
				}
			break;

		case ECodeSegs:
			if(Debug::EScopeGlobal == info.iListScope)
				{
				err = manager.GetGlobalCodeSegList(buffer, dataSize);
				}
			else if(Debug::EScopeProcessSpecific == info.iListScope)
				{
				err = manager.GetCodeSegListForProcess(buffer, dataSize, info.iTargetId);
				}
			else if(Debug::EScopeThreadSpecific == info.iListScope)
				{
				err = manager.GetCodeSegListForThread(buffer, dataSize, info.iTargetId);
				}
			break;

		default:
			err = KErrNotSupported;
		}

	if(err == KErrNone)
		{
		//if no error then write the buffer back
		err = Kern::ThreadDesWrite(iClientThread, info.iBuffer, buffer, 0, KChunkShiftBy0, iClientThread);
		}

	//write back the size of the data regardless of any error
	TInt writeErr = Kern::ThreadRawWrite(iClientThread, info.iDataSize, (TUint8*)&dataSize, sizeof(TUint32), iClientThread);
	if(writeErr != KErrNone)
		{
		//if there was an error writing the size return that error instead
		err = writeErr;
		}

	//free the buffer
	NKern::ThreadEnterCS();
	Kern::Free((TAny*)buffer.Ptr());
	NKern::ThreadLeaveCS();

	return err;
	}

/**
Read registers and store register data in aRegisterInfo

@param aThread thread to read registers from
@param aRegisterInfo structure specifying which registers to read and providing
       descriptors to write the register data into

@return KErrNone if registers were read successfully. Note that this does not
        mean that all the registers could be read, the
        aRegisterInfo.iRegisterFlags array should be checked as to whether each
        individual register could be read,
        KErrArgument if aRegisterInfo is NULL, or if any of the pointers that
        are members of aRegisterInfo are NULL, if an unknown register is
        specified or if the passed in register values buffer is too small
        KErrNoMemory if there is insufficient memory,
        KErrDied, if the thread with thread ID aThreadId is dead
*/
TInt DRM_DebugChannel::ReadRegisters(DThread* aThread, TRM_DebugRegisterInformation* aRegisterInfo) const
	{
	LOG_MSG("DRM_DebugChannel::ReadRegisters()");

	TInt err = KErrNone;

	if (!aRegisterInfo)
		return KErrArgument;

	TRM_DebugRegisterInformation info;
	err = Kern::ThreadRawRead(iClientThread, aRegisterInfo, (TUint8*)&info, sizeof(TRM_DebugRegisterInformation));
	if (err != KErrNone)
		return err;

	if ((!info.iRegisterIds) || (!info.iRegisterValues) || (!info.iRegisterFlags))
		return KErrArgument;

	//read ids from client thread
	TPtr8 ids(NULL, 0);
	err = AllocAndReadDes(iClientThread, *info.iRegisterIds, ids);
	if(err != KErrNone)
		{
		if(err == KErrNoMemory)
			{
			NKern::ThreadEnterCS();
			Kern::Free((TAny*)ids.Ptr());
			NKern::ThreadLeaveCS();
			}
		return err;
		}

	//read values from client thread
	TPtr8 values(NULL, 0);
	err = AllocAndReadDes(iClientThread, *info.iRegisterValues, values, EFalse);
	if(err != KErrNone)
		{
		if(err == KErrNoMemory)
			{
			NKern::ThreadEnterCS();
			Kern::Free((TAny*)values.Ptr());
			NKern::ThreadLeaveCS();
			}

		NKern::ThreadEnterCS();
		Kern::Free((TAny*)ids.Ptr());
		NKern::ThreadLeaveCS();
		return err;
		}

	//read flags from client thread
	TPtr8 flags(NULL, 0);
	err = AllocAndReadDes(iClientThread, *info.iRegisterFlags, flags, EFalse);
	if(err != KErrNone)
		{
		if(err == KErrNoMemory)
			{
			NKern::ThreadEnterCS();
			Kern::Free((TAny*)flags.Ptr());
			NKern::ThreadLeaveCS();
			}
		NKern::ThreadEnterCS();
		Kern::Free((TAny*)ids.Ptr());
		Kern::Free((TAny*)values.Ptr());
		NKern::ThreadLeaveCS();
		return err;
		}

	err = DoReadRegisters(aThread, ids, values, flags);
	if (err == KErrNone)
		{
		err = Kern::ThreadDesWrite(iClientThread, info.iRegisterValues, values, 0, KChunkShiftBy0, iClientThread);
		if(err == KErrNone)
			{
			err = Kern::ThreadDesWrite(iClientThread, info.iRegisterFlags, flags, 0, KChunkShiftBy0, iClientThread);
			}
		}

	NKern::ThreadEnterCS();
	Kern::Free((TAny*)ids.Ptr());
	Kern::Free((TAny*)values.Ptr());
	Kern::Free((TAny*)flags.Ptr());
	NKern::ThreadLeaveCS();

	return err;
	}

/**
@deprecated use DRM_DebugChannel::WriteRegisters(DThread* aThread, TRM_DebugRegisterInformation* aRegisterInfo) instead
*/
TInt DRM_DebugChannel::WriteRegistersLegacy(DThread* aThread, const TRM_DebugRegisterInfo* aRegisterInfo)
	{
	LOG_MSG("DRM_DebugChannel::WriteRegistersLegacy()");

	TInt err = KErrNone;

	if (!aRegisterInfo)
		return KErrArgument;

	TRM_DebugRegisterInfo info(0, 0, 0);
	err = Kern::ThreadRawRead(iClientThread, aRegisterInfo, (TUint8*)&info, sizeof(TRM_DebugRegisterInfo));
	if (err != KErrNone)
		return err;

	if (!info.iValues)
		return KErrArgument;

	TUint length = (info.iLastRegister - info.iFirstRegister + 1) * 4;

	NKern::ThreadEnterCS();
	TUint8 *values = (TUint8*)Kern::Alloc(length);
	NKern::ThreadLeaveCS();
	if (!values)
		{
		return KErrNoMemory;
		}

	TPtr8 valuesDes(values, length);

	err = Kern::ThreadDesRead(iClientThread, info.iValues, valuesDes, 0);
	if (err == KErrNone)
		{
		err = DoWriteRegisters(aThread, info.iFirstRegister, info.iLastRegister, valuesDes);
		}

	NKern::ThreadEnterCS();
	Kern::Free(values);
	NKern::ThreadLeaveCS();

	return err;
	}

/**
Write registers and store flags data in aRegisterInfo

@param aThread thread to write registers to
@param aRegisterInfo structure specifying which registers to write and providing
       descriptors to write the register flags data into

@return KErrNone if registers were written successfully. Note that this does not
        mean that all the registers could be written, the flags array
        should be checked as to whether each individual register could be read,
        KErrArgument if aRegisterInfo is NULL, or if any of the pointers that
        are members of aRegisterInfo are NULL, if an unknown register is
        specified or if the passed in register values buffer is too small, or
        if aThread is NULL,
        KErrGeneral if there was a problem initialising the register set,
        KErrNoMemory if there is insufficient memory,
        KErrDied, if the thread with thread ID aThreadId is dead
*/
TInt DRM_DebugChannel::WriteRegisters(DThread* aThread, TRM_DebugRegisterInformation* aRegisterInfo) const
	{
	LOG_MSG("DRM_DebugChannel::WriteRegisters()");

	TInt err = KErrNone;

	if (!aRegisterInfo)
		return KErrArgument;

	TRM_DebugRegisterInformation info;
	err = Kern::ThreadRawRead(iClientThread, aRegisterInfo, (TUint8*)&info, sizeof(TRM_DebugRegisterInformation));
	if (err != KErrNone)
		return err;

	if ((!info.iRegisterIds) || (!info.iRegisterValues) ||(!info.iRegisterFlags))
		return KErrArgument;

	//read ids from client thread
	TPtr8 ids(NULL, 0);
	err = AllocAndReadDes(iClientThread, *info.iRegisterIds, ids);
	if(err != KErrNone)
		{
		if(err == KErrNoMemory)
			{
			NKern::ThreadEnterCS();
			Kern::Free((TAny*)ids.Ptr());
			NKern::ThreadLeaveCS();
			}
		return err;
		}

	//read values from client thread
	TPtr8 values(NULL, 0);
	err = AllocAndReadDes(iClientThread, *info.iRegisterValues, values);
	if(err != KErrNone)
		{
		if(err == KErrNoMemory)
			{
			NKern::ThreadEnterCS();
			Kern::Free((TAny*)values.Ptr());
			NKern::ThreadLeaveCS();
			}
		NKern::ThreadEnterCS();
		Kern::Free((TAny*)ids.Ptr());
		NKern::ThreadLeaveCS();
		return err;
		}

	//read flags from client thread
	TPtr8 flags(NULL, 0);
	err = AllocAndReadDes(iClientThread, *info.iRegisterFlags, flags, EFalse);
	if(err != KErrNone)
		{
		if(err == KErrNoMemory)
			{
			NKern::ThreadEnterCS();
			Kern::Free((TAny*)flags.Ptr());
			NKern::ThreadLeaveCS();
			}
		NKern::ThreadEnterCS();
		Kern::Free((TAny*)ids.Ptr());
		Kern::Free((TAny*)values.Ptr());
		NKern::ThreadLeaveCS();
		return err;
		}

	err = DoWriteRegisters(aThread, ids, values, flags);
	if(err == KErrNone)
		{
		err = Kern::ThreadDesWrite(iClientThread, info.iRegisterFlags, flags, 0, KChunkShiftBy0, iClientThread);
		}

	NKern::ThreadEnterCS();
	Kern::Free((TAny*)ids.Ptr());
	Kern::Free((TAny*)values.Ptr());
	Kern::Free((TAny*)flags.Ptr());
	NKern::ThreadLeaveCS();

	return err;
	}

/**
Suspends execution of the specified thread.

@param aThread thread to resume

@return KErrNone if there were no problems or KErrArgument if aThread is NULL
*/
TInt DRM_DebugChannel::DoSuspendThread(DThread *aThread)
	{
	LOG_MSG("DRM_DebugChannel::DoSuspendThread()");

	if (!aThread)
		{
		LOG_MSG("Invalid dthread object");
		return KErrArgument;
		}

	return TheDProcessTracker.SuspendThread(aThread);
	}

/**
Resumes execution of the specified thread.

@param aThread thread to resume

@return KErrNone if there were no problems, KErrArgument if aThread is NULL
        or an error value returned from DoStepRange()
*/
TInt DRM_DebugChannel::DoResumeThread(DThread *aThread)
	{
	LOG_MSG("DRM_DebugChannel::DoResumeThread()");

	if (!aThread)
		return KErrArgument;

	// get the current PC
	TUint32 currentPC;
	TInt err = ReadKernelRegisterValue(aThread, PC_REGISTER, currentPC);
	if(err != KErrNone)
		{
		LOG_MSG2("DRM_DebugChannel::DoResumeThread - Non-zero error code discarded: %d", err);
		}

	// if there is a breakpoint at the current PC, we need to single step past it
	TBreakEntry* breakEntry = NULL;
	do
		{
		breakEntry = iBreakManager->GetNextBreak(breakEntry);
		if(breakEntry && !iBreakManager->IsTemporaryBreak(*breakEntry))
			{
			if (breakEntry->iAddress == currentPC)
				{
				return DoStepRange(aThread, currentPC, currentPC+1, ETrue, 1, ETrue);
				}
			}
		} while(breakEntry);

	return TheDProcessTracker.ResumeThread(aThread);
	}

//
// DRM_DebugChannel::DoStepRange
//
TInt DRM_DebugChannel::DoStepRange(DThread *aThread, const TUint32 aStartAddress, const TUint32 aStopAddress, TBool aStepInto, TBool aResumeOnceOutOfRange, const TUint32 aNumSteps, TBool aUserRequest)
	{
	LOG_MSG("DRM_DebugChannel::DoStepRange()");

	if (!aThread)
		return KErrArgument;

	//check that the thread is suspended
	if(!TheDProcessTracker.CheckSuspended(aThread))
		{
		LOG_MSG2("DRM_DebugChannel::DoStepRange() Thread with id 0x%08x not suspended.", aThread->iId);
		return KErrInUse;
		}

	TUint32 startAddress = (aStartAddress & 0x1) ? aStartAddress + 1 : aStartAddress;
	TUint32 stopAddress = (aStopAddress & 0x1) ? aStopAddress + 1 : aStopAddress;;

	// don't allow the user to step in the excluded ROM region.  this could be called
	// internally however.  for example, the the special breakpoints we set to handle
	// panics, exceptions, and library loaded events are in the user library, and we
	// will need to step past the breakpoint before continuing the thread.
	//if (aUserRequest && (startAddress >= iExcludedROMAddressStart) && (startAddress < iExcludedROMAddressEnd))
	//{
	//	return KErrNotSupported;
	//}

	// set the temp breakpoint, and disable the breakpoint at the current PC if necessary
	// if its not a user request, and we are just trying to resume from a breakpoint,
	// then we don't need to check for stubs. The last parameter aUserRequest tells
	// ModifyBreaksForStep to check for stubs or not. In some cases, the check for stubs
	// is true even if its not a user request.For example, this is true in cases where
	// we are doing a step range and the instruction in the range modified PC.
	// in this case, DoStepRange will be called from the exception handler where
	// we need to check for the stubs for the valid behavior. So truly, we don't need to check
	// for stubs only when resuming from  a breakpoint.
	ReturnIfError(iStepper->ModifyBreaksForStep(aThread, startAddress, stopAddress, aResumeOnceOutOfRange, aUserRequest, aNumSteps));

	LOG_MSG("DRM_DebugChannel::DoStepRange() - resuming thread\n");

	return TheDProcessTracker.ResumeThread(aThread);
	}

/**
Read memory from the specified addres into the aData descriptor. If there is a
breakpoint set in the region of memory returned then the correct data value is
inserted into the descriptor

@param aThread pointer to thread whose address space memory is to be read from
@param aAddress address to start reading memory from
@param aLength length of memory block to read
@param aData descriptor to read memory into

@return KErrNone if memory read successfully,
        KErrNotSupported if reading from the rom section is not supported,
        KErrBadHandle if aThread is invalid,
        or one of the other system wide error codes
*/
TInt DRM_DebugChannel::DoReadMemory(const DThread *aThread, const TUint32 aAddress, const TUint32 aLength, TDes8 &aData) const
	{
	LOG_MSG("DRM_DebugChannel::DoReadMemory()");

	// make sure the parameters are valid
	if (aLength > aData.MaxSize())
		return KErrArgument;

	TInt err = KErrNone;

	// trap exceptions in case the address is invalid
	XTRAPD(r, XT_DEFAULT, err = TryToReadMemory(aThread, (TAny *)aAddress, (TAny *)aData.Ptr(), aLength));

	err = (KErrNone == r) ? err : r;

	if (KErrNone == err)
		{
		aData.SetLength(aLength);

		TPtr8 data((TUint8 *)aData.Ptr(), aLength, aLength);

		// if we have any breakpoints in this range, put the actual instruction in the buffer
		TBreakEntry* breakEntry = NULL;
		do
			{
			breakEntry = iBreakManager->GetNextBreak(breakEntry);
			if(breakEntry && !iBreakManager->IsTemporaryBreak(*breakEntry))
				{
				if ((breakEntry->iAddress >= aAddress) && (breakEntry->iAddress < (aAddress + aLength)))
					{
					TInt instSize;

					switch(breakEntry->iMode)
						{
						case EArmMode:
							instSize = 4;
							break;

						case EThumbMode:
							instSize = 2;
							break;

						case EThumb2EEMode:
						default:
							LOG_MSG("DRM_DebugChannel::DoReadMemory() cannot fixup breakpoints with unsupported architecture");
							return KErrNotSupported;
						}
					memcpy((TAny*)&data[breakEntry->iAddress - aAddress], (TAny *)breakEntry->iInstruction.Ptr(), instSize);
					}
				}
			} while(breakEntry);
		}

	return err;
	}

/**
Attempt to write memory to aThread's address space

@param aThread thread to whose address space memory is to be written
@param aAddress memory location to write memory to
@param aLength number of bytes of data to write
@param aData descriptor containing memory to write

@return KErrNone if memory written successfully,
        KErrArgument if aLength is greater than than the length of the aData
        KErrBadHandle if aThread is invalid,
	or another of the system wide error codes
*/
TInt DRM_DebugChannel::DoWriteMemory(DThread *aThread, const TUint32 aAddress, const TUint32 aLength, TDes8 &aData)
	{
	LOG_MSG("DRM_DebugChannel::DoWriteMemory()");

	// make sure the parameters are valid
	if (aLength > aData.Length())
		return KErrArgument;

	TInt err = KErrNone;

	// trap exceptions in case the address is invalid
	XTRAPD(r, XT_DEFAULT, err = TryToWriteMemory(aThread, (TAny *)aAddress, (TAny *)aData.Ptr(), aLength));

	err = (KErrNone == r) ? err : r;

	// reset any breakpoints we may have just overwritten
	if (KErrNone == err)
		{
		TPtr8 data((TUint8 *)aData.Ptr(), aLength, aLength);

		TBreakEntry* breakEntry = NULL;
		do
			{
			breakEntry = iBreakManager->GetNextBreak(breakEntry);
			if(breakEntry && !iBreakManager->IsTemporaryBreak(*breakEntry))
				{
				if ((breakEntry->iAddress >= aAddress) && (breakEntry->iAddress < (aAddress + aLength)))
					{
					// default to arm mode
					TUint32 inst;
					TInt instSize;

					switch (breakEntry->iMode)
						{
						case EArmMode:
							inst = KArmBreakPoint;
							instSize = 4;
							break;

						case EThumbMode:
							inst = KThumbBreakPoint;
							instSize = 2;
							break;

						case EThumb2EEMode:
						default:
							LOG_MSG("DRM_DebugChannel::DoWriteMemory() cannot fixup breakpoints of unsupported architecture type");

							return KErrNotSupported;
						}

					breakEntry->iInstruction.Copy(&data[breakEntry->iAddress - aAddress], instSize);
					memcpy((TAny*)breakEntry->iAddress, (TAny *)&inst, instSize);
					}
				}

			} while(breakEntry);
		}
	return err;
	}

//
// DRM_DebugChannel::DoReadRegisters
//
TInt DRM_DebugChannel::DoReadRegisters(DThread *aThread, const TInt16 aFirstRegister, const TInt16 aLastRegister, TDes8 &aValues)
	{
	LOG_EVENT_MSG("DRM_DebugChannel::DoReadRegisters()");

	// make sure the parameters are valid
	if (!aThread || (aFirstRegister < 0) || (aLastRegister >= (TInt16)(sizeof(TArmRegSet)/sizeof(TArmReg))))
		return KErrArgument;

	// make sure the descriptor is big enough to hold the requested data
	if ((TInt)((aLastRegister - aFirstRegister + 1) * sizeof(TArmReg)) > (aValues.MaxSize()))
		return KErrArgument;

	TArmRegSet regSet;
	TUint32 unused;

	NKern::ThreadGetUserContext(&aThread->iNThread, &regSet, unused);

	LOG_MSG2( "DRM_DebugChannel::DoReadRegistersLegacy() : unused = 0x%X\n", unused );

	TArmReg *reg = &regSet.iR0;

	if (!reg)
		return KErrGeneral;

	for (TInt16 i = aFirstRegister; i <= aLastRegister; i++)
		aValues.Append((TUint8 *)&reg[i], sizeof(TArmReg));

	return KErrNone;
}

/**
  @prototype

  Experimental function for determining whether a thread is suspended.

  @param aThread thread to check if suspended

  @return ETrue if the thread is suspended, EFalse if it isn't or does not exist
  */
TBool DRM_DebugChannel::CheckSuspended(const DThread *aThread) const
	{
	if(!aThread)
		{
		return EFalse;
		}

	if( (aThread->iNThread.iCsCount>0) && (aThread->iNThread.iCsFunction>0) )
		{
		return ETrue;
		}

	if(aThread->iNThread.iSuspendCount > 0)
		{
		return ETrue;
		}
	return EFalse;
	}

/**
Read registers and store register values in aRegisterValues and the flags
indicating which registers could be read in aRegisterFlags

@param aThread thread to read registers from
@param aRegisterIds array containing register IDs to read
@param aRegisterValues array to store register values in
@param aRegisterFlags array to store flags in

@return KErrNone if registers were read successfully. Note that this does not
        mean that all the registers could be read, the aRegisterFlags array
        should be checked as to whether each individual register could be read,
        KErrArgument if aThread is NULL, if an unknown register is specified in
        aRegisterValues or if aRegisterValues is too small
        KErrGeneral if there was a problem initialising the register set
*/
TInt DRM_DebugChannel::DoReadRegisters(DThread *aThread, const TDesC8 &aRegisterIds, TDes8 &aRegisterValues, TDes8& aRegisterFlags) const
	{
	LOG_MSG("DRM_DebugChannel::DoReadRegisters()");

	// make sure the parameters are valid
	if (!aThread)
		return KErrArgument;

	//Need to revisit this to determine whether there is a way to validate this
#if 0
	if ( !CheckSuspended(aThread) )
		{
		LOG_MSG2("DRM_DebugChannel::DoReadRegisters() thread with id 0x%08x is not suspended", aThread->iId);
		return KErrInUse;
		}
#endif

	//set lengths of output descriptors to 0 prior to filling
	aRegisterValues.SetLength(0);
	aRegisterFlags.SetLength(0);

	TArmRegSet regSet;
	TUint32 flags;

	NKern::ThreadGetUserContext(&aThread->iNThread, &regSet, flags);

	LOG_MSG2( "DRM_DebugChannel::DoReadRegisters() : flags = 0x%X\n", flags );

	TArmReg *regPtr = &regSet.iR0;

	if (!regPtr)
		return KErrGeneral;

	TUint numberOfRegisters = aRegisterIds.Length() / sizeof(TRegisterInfo);

	//iterate through registers setting the relevant aFlags value
	for(TUint i=0; i<numberOfRegisters; i++)
		{
		//get current register id
		TRegisterInfo reg;
		TInt err = GetTRegisterInfo(aRegisterIds, i, reg);
		//exit with the error value if there was an error
		if(err != KErrNone)
			return err;

		//if unknown register then exit as can't know how many bytes this entry will
		//represent in aRegisterValues
		TTag registerTag;
		TDebugFunctionality::GetRegister(reg, registerTag);
		if(registerTag.iValue == EAccessUnknown)
			{
			return KErrArgument;
			}

		//get the current register id as a kernel register
		TArmReg armReg;
		err = GetKernelRegisterId(reg, armReg);
		if((err == KErrNotSupported) || (registerTag.iValue == EAccessNone) || (registerTag.iValue == EAccessWriteOnly))
			{
			//reading this register is not supported
			aRegisterFlags.Append(ENotSupported);
			//just skip over this entry in the values buffer
			if(aRegisterValues.Length() + registerTag.iSize > aRegisterValues.MaxLength())
				{
				//writing this value would cause overflow so exit
				return KErrArgument;
				}
			aRegisterValues.SetLength(aRegisterValues.Length() + registerTag.iSize);
			}
		else
			{
			if(registerTag.iSize == sizeof(TArmReg))
				{
				if(GetFlagAtOffset(flags, armReg))
					{
					//set flag as valid
					aRegisterFlags.Append(EValid);
					}
				else
					{
					// Even though the flag is invalid, we can return the value of the register
					// and let the user decide what to do
					aRegisterFlags.Append(EInValid);
					}

				if(aRegisterValues.Length() + sizeof(TArmReg) > aRegisterValues.MaxLength())
					{
					//writing this value would cause overflow so exit
					return KErrArgument;
					}
				//write value into register into regSet
				aRegisterValues.Append((TUint8 *)&regPtr[armReg], registerTag.iSize);
				}
			else
				{
				//currently all kernel supported registers are 4 bytes so
				//return EBadSize. Would need updating if/when other register
				//value sizes are supported
				aRegisterFlags.Append(EBadSize);
				aRegisterValues.SetLength(aRegisterValues.Length() + registerTag.iSize);
				}
			}
		}
	return KErrNone;
	}

//
// DRM_DebugChannel::DoWriteRegisters
//
TInt DRM_DebugChannel::DoWriteRegisters(DThread *aThread, const TInt16 aFirstRegister, const TInt16 aLastRegister, TDesC8 &aValues)
	{
	LOG_MSG("DRM_DebugChannel::DoWriteRegisters()");

	// make sure the parameters are valid
	if (!aThread || (aFirstRegister < 0) || (aLastRegister >= (TInt16)(sizeof(TArmRegSet)/sizeof(TArmReg))))
		return KErrArgument;

	// make sure the descriptor is big enough to hold the data to write
	if ((TInt)((aLastRegister - aFirstRegister + 1) * sizeof(TArmReg)) > (aValues.Length()))
		return KErrArgument;

	TArmRegSet regSet;
	TUint32 unused;

	NKern::ThreadGetUserContext(&aThread->iNThread, &regSet, unused);

	TArmReg *reg = &regSet.iR0;

	for (TInt16 i = aFirstRegister; i <= aLastRegister; i++)
		reg[i] = *(TUint32 *)&aValues[(i-aFirstRegister)*sizeof(TArmReg)];

	NKern::ThreadSetUserContext(&aThread->iNThread, &regSet);

	return KErrNone;
	}

/**
Write registers and store flags indicating which registers could be read in
aRegisterFlags

@param aThread thread to write registers to
@param aRegisterIds array containing register IDs to write
@param aRegisterValues array containing register values to write
@param aRegisterFlags array to store flags in

@return KErrNone if registers were written successfully. Note that this does not
        mean that all the registers could be written, the aRegisterFlags array
        should be checked as to whether each individual register could be read,
        KErrArgument if aThread is NULL, if the buffer passed in as
        aRegisterValue is too small, or if an unknown register is requested,
        KErrGeneral if there was a problem initialising the register set
*/
TInt DRM_DebugChannel::DoWriteRegisters(DThread *aThread, const TDesC8 &aRegisterIds, TDesC8 &aRegisterValues, TDes8 &aRegisterFlags) const
	{
	LOG_MSG("DRM_DebugChannel::DoWriteRegisters()");

	// make sure the parameters are valid
	if (!aThread)
		return KErrArgument;

	//check that the thread is suspended before reading the registers
	if(!TheDProcessTracker.CheckSuspended(aThread))
		{
		LOG_MSG2("DRM_DebugChannel::DoWriteRegisters() thread with id 0x%08x is not suspended", aThread->iId);
		return KErrInUse;
		}

	//get register values from kernel
	TArmRegSet regSet;
	TUint32 flags;
	NKern::ThreadGetUserContext(&aThread->iNThread, &regSet, flags);

	//set lengths of output descriptors to 0 prior to filling
	aRegisterFlags.SetLength(0);

	//pointer to first kernel register
	TArmReg *regPtr = &regSet.iR0;

	if (!regPtr)
		return KErrGeneral;

	//calculate number of registers
	TUint numberOfRegisters = aRegisterIds.Length() / sizeof(TRegisterInfo);

	//iterate through registers setting the relevant aRegisterFlags value and
	//setting the necessary value in regSet ready to write to kernel
	for(TUint i=0, offset = 0; i<numberOfRegisters; i++)
		{
		//get current register id
		TRegisterInfo reg;
		TInt err = GetTRegisterInfo(aRegisterIds, i, reg);
		//exit with the error value if there was an error
		if(err != KErrNone)
			{
			return err;
			}

		//if unknown register then exit as can't know how many bytes this entry will
		//represent in aRegisterValues
		TTag registerTag;
		TDebugFunctionality::GetRegister(reg, registerTag);
		if(registerTag.iValue == EAccessUnknown)
			{
			return KErrArgument;
			}

		//get the current register id as a kernel register
		TArmReg armReg;
		err = GetKernelRegisterId(reg, armReg);
		if((err == KErrNotSupported) || (registerTag.iValue == EAccessNone) || (registerTag.iValue == EAccessReadOnly))
			{
			//writing to this register is not supported
			aRegisterFlags.Append(ENotSupported);
			}
		else if(GetFlagAtOffset(flags, armReg))
			{
			if(registerTag.iSize == sizeof(TArmReg))
				{
				//set flag as valid
				aRegisterFlags.Append(EValid);
				if(offset + sizeof(TArmReg) > aRegisterValues.Length())
					{
					//getting this value would cause overflow so exit
					return KErrArgument;
					}
				//write value into register into regSet
				regPtr[armReg] = *(TUint32 *)&aRegisterValues[offset];
				}
			else
				{
				//currently all kernel supported registers are 4 bytes so
				//return EBadSize. Would need updating if/when other register
				//value sizes are supported
				aRegisterFlags.Append(EBadSize);
				}

			}
		else
			{
			//set flag as invalid as register value couldn't be read
			aRegisterFlags.Append(EInValid);
			}
		offset+=registerTag.iSize;
		}

	//write the input data into the registers
	NKern::ThreadSetUserContext(&aThread->iNThread, &regSet);

	//return normally
	return KErrNone;
	}

//
// DRM_DebugChannel::DoSecurityCheck
//
TBool DRM_DebugChannel::DoSecurityCheck()
	{
	LOG_MSG("DRM_DebugChannel::DoSecurityCheck");
	DProcess* clientProcess = iClientThread->iOwningProcess;
	if (clientProcess)
		{
		SSecurityInfo secureInfo = clientProcess->iS;

		LOG_MSG2("DoSecurityCheck - client secure id is 0x%08x",secureInfo.iSecureId);

		// Ensure we really are communicating with the Debug Security Server
		if (secureInfo.iSecureId == KUidDebugSecurityServer.iUid )
			{
			return ETrue;
			}
		}
	return EFalse;
	}

/**
Attempt to read memory from aThread's address space

@param aThread thread from whose address space memory is to be read
@param aSrc pointer to memory location to read memory from
@param aDest pointer to memory location to write memory to
@param aLength number of bytes of data to read

@return KErrNone if memory read successfully,
	or another of the system wide error codes
*/
TInt DRM_DebugChannel::TryToReadMemory(const DThread *aThread, const TAny *aSrc, TAny *aDest, const TUint32 aLength) const
	{
	LOG_MSG("DRM_DebugChannel::TryToReadMemory()");

	// make sure the parameters are valid
	if (!aThread)
		return KErrArgument;

	//Need to revisit this to determine whether there is a way to validate this
#if 0
	//check that the thread is suspended before reading the memory
	if ( !CheckSuspended(aThread) )
		{
		LOG_MSG2("DRM_DebugChannel::TryToReadMemory() thread with id 0x%08x is not suspended", aThread->iId);
		return KErrInUse;
		}
#endif

	LOG_MSG2("Using Kern::ThreadRawRead to read memory at address %x", aSrc);
	return Kern::ThreadRawRead((DThread *)aThread, aSrc, aDest, aLength);
	}

/**
Attempt to write memory to aThread's address space

@param aThread thread to whose address space memory is to be written
@param aDest pointer to memory location to write memory to
@param aSrc pointer to memory location to read memory from
@param aLength number of bytes of data to write

@return KErrNone if memory written successfully, or another of the system wide
        error codes
*/
TInt DRM_DebugChannel::TryToWriteMemory(const DThread *aThread, TAny *aDest, const TAny *aSrc, const TUint32 aLength)
	{
	LOG_MSG("DRM_DebugChannel::TryToWriteMemory()");

	//check that the thread is suspended before writing the memory
	if(!TheDProcessTracker.CheckSuspended((DThread*)aThread))
		{
		LOG_MSG2("DRM_DebugChannel::TryToWriteMemory() thread with id 0x%08x is not suspended", aThread->iId);
		return KErrInUse;
		}

	LOG_MSG2("Using Kern::ThreadRawWrite to write memory at address %x", (TUint32)aDest);
	return Kern::ThreadRawWrite((DThread *)aThread, aDest, aSrc, aLength, iClientThread);
	}

/**
@deprecated use DRM_DebugChannel::ReadKernelRegisterValue(DThread *aThread, const TArmReg aKernelRegisterId, T4ByteRegisterValue &aValue) instead
*/
TInt32 DRM_DebugChannel::ReadRegister(DThread *aThread, TInt aNum)
	{
	LOG_MSG("DRM_DebugChannel::ReadRegister()");

	if (!aThread || (aNum < 0) || (aNum >= (TInt16)(sizeof(TArmRegSet)/sizeof(TArmReg))))
		{
		LOG_MSG2("Invalid register number (%d) passed to ReadRegister", aNum);
		return 0;
		}

	TArmRegSet regSet;
	TUint32 unused;

	NKern::ThreadGetUserContext(&aThread->iNThread, &regSet, unused);

	TArmReg *reg = &regSet.iR0;

	return ((TUint32 *)reg)[aNum];
	}

/**
Given a TArmReg register ID, read the value of the register. The register value
will be stored in aValue if the register could be read.

@param aThread thread to read register from
@param aKernelRegisterId ID of register to read from
@param aValue value read from register

@return KErrNone if value was successfully stored in aValue,
        KErrNotSupported if aKernelRegister is not supported by the debug
	security server,
        or a return value from DRM_DebugChannel::ReadDebugRegisterValue()
*/
TInt32 DRM_DebugChannel::ReadKernelRegisterValue(DThread *aThread, const TArmReg aKernelRegisterId, T4ByteRegisterValue &aValue) const
	{
	//get register ID as a TRegisterInfo ID
	TRegisterInfo regId;
	TInt err = GetDebugRegisterId(aKernelRegisterId, regId);
	if(err != KErrNone)
		return err;

	//get the value for the register
	err = ReadDebugRegisterValue(aThread, regId, aValue);
	return err;
	}

/**
Given a TRegisterInfo register ID, read the value of this register. The
register value will be stored in aValue if the register could be read.

@param aThread thread to read register from
@param aDebugRegisterId ID of register to read from
@param aValue value read from register

@return KErrNone if value was successfully stored in aValue,
        TRegisterFlag::EInValid if value could not be read from the register,
        TRegisterFlag::ENotSupported if the register is not supported,
        KErrNoMemory if temporary memory could not be allocated,
        or a return value from DRM_DebugChannel::DoReadRegisters
*/
TInt32 DRM_DebugChannel::ReadDebugRegisterValue(DThread *aThread, const TRegisterInfo aDebugRegisterId, T4ByteRegisterValue &aValue) const
	{
	//allocate temporary buffers to store data
	NKern::ThreadEnterCS();
	TUint8* id = (TUint8*)Kern::Alloc(sizeof(TRegisterInfo));
	NKern::ThreadLeaveCS();
	if(id == NULL)
		{
		return KErrNoMemory;
		}

	TPtr8 idPtr(id, sizeof(TRegisterInfo));

	NKern::ThreadEnterCS();
	TUint8* value = (TUint8*)Kern::Alloc(sizeof(T4ByteRegisterValue));
	NKern::ThreadLeaveCS();
	if(value == NULL)
		{
		return KErrNoMemory;
		}
	TPtr8 valuePtr(value, sizeof(T4ByteRegisterValue));

	NKern::ThreadEnterCS();
	TUint8* flag = (TUint8*)Kern::Alloc(sizeof(TUint8));
	NKern::ThreadLeaveCS();
	if(flag == NULL)
		{
		return KErrNoMemory;
		}
	TPtr8 flagPtr(flag, sizeof(TUint8));

	//store register id in buffer
	idPtr.Append((TUint8*)&aDebugRegisterId, sizeof(TRegisterInfo));

	//read registers
	TInt err = DoReadRegisters(aThread, idPtr, valuePtr, flagPtr);
	if(err == KErrNone)
		{
		if(*flag == EValid)
			{
			//register could be read so store value
			aValue = *(T4ByteRegisterValue*)value;
			}
		else
			{
			//register couldn't be read for some reason
			err = *flag;
			}
		}

	//free memory
	NKern::ThreadEnterCS();
	Kern::Free(id);
	Kern::Free(value);
	Kern::Free(flag);
	NKern::ThreadLeaveCS();

	return err;
	}

//
// DRM_DebugChannel::NotifyEvent
//
void DRM_DebugChannel::NotifyEvent(const TDriverEventInfo& aEventInfo)
	{
	LOG_EVENT_MSG("DRM_DebugChannel::NotifyEvent()");

	// Look for the relevant DTargetProcess
	// We can find out the relevant process id from aEventInfo
	TUint32 pid = aEventInfo.iProcessId;

	//opening handle to process
	DProcess* targetProcess = DebugUtils::OpenProcessHandle(pid);

	if(!targetProcess)
		{
		LOG_EVENT_MSG("DRM_DebugChannel::NotifyEvent - process does not exist!");
		return;
		}

	// Are we debugging this process - decide based on iFileName
	DCodeSeg* p = targetProcess->iCodeSeg;
	DTargetProcess* foundProcess;
	if (p)
		{
		 foundProcess = TheDProcessTracker.FindProcess(*(p->iFileName));
		}
	else
		{
		// special case: may not have a code seg in some cases. in which case we tell everyone!
		if (targetProcess->iName)
			{
			// copy the name of the process
			foundProcess = TheDProcessTracker.FindProcess(*(targetProcess->iName));
			}
		else
			{
			foundProcess = NULL;
			}
		}

	//close the handle
	targetProcess->Close(NULL);

	if (!foundProcess)
		{
		// No: just ignore this exception
		LOG_EVENT_MSG("DRM_DebugChannel::NotifyEvent - we are not debugging this process!");
		return;
		}

	foundProcess->NotifyEvent(aEventInfo);
	}

#ifndef __LAUNCH_AS_EXTENSION__
DECLARE_STANDARD_LDD()
	{
	return new DRM_DebugDriverFactory;
	}
#else

DStopModeExtension* TheStopModeExtension = NULL;

DECLARE_EXTENSION_LDD()
	{
	return new DRM_DebugDriverFactory;
	}

/**
  This value is used as an initialiser for the size of the Stop-Mode Debug API's
  default request buffer.
  */
const TInt KRequestBufferSize = 0x200;
/**
  This value is used as an initialiser for the size of the Stop-Mode Debug API's
  default response buffer.
  */
const TInt KResponseBufferSize = 0x1000;

DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("Starting RM_DEBUG extension"));

	// get a reference to the DDebuggerInfo and to the DStopModeExtension
	TSuperPage& superPage = Kern::SuperPage();

	if(!superPage.iDebuggerInfo)
		{
		//kdebug has not been installed so create DDebuggerInfo using our stub constructor
		superPage.iDebuggerInfo = new DDebuggerInfo();
		}

	if(!TheStopModeExtension)
		{
		TheStopModeExtension = new DStopModeExtension();
		}

	// create the request buffer and store a reference to it
	TTag tag;
	tag.iTagId = EBuffersRequest;
	tag.iType = ETagTypePointer;
	tag.iSize = KRequestBufferSize;
	TInt err = TheDBufferManager.CreateBuffer(tag);
	if(KErrNone != err)
		{
		return KErrNone;
		}

	// create the response buffer and store a reference to it
	tag.iTagId = EBuffersResponse;
	tag.iSize = KResponseBufferSize;
	err = TheDBufferManager.CreateBuffer(tag);
	if(KErrNone != err)
		{
		return KErrNone;
		}
	// create the debug functionality buffer and store a reference to it
	TDebugFunctionality df;
	TUint dfSize = df.GetStopModeFunctionalityBufSize();
	tag.iTagId = EBuffersFunctionality;
	tag.iSize = dfSize;
	err = TheDBufferManager.CreateBuffer(tag);
	if(KErrNone != err)
		{
		return KErrNone;
		}

	// fill the functionality buffer with the functionality data and store it in
	// the super page
	TPtr8 dfBlockPtr((TUint8*)tag.iValue, dfSize);
	if(!df.GetStopModeFunctionality(dfBlockPtr))
		{
		return KErrNone;
		}
	TheStopModeExtension->iFunctionalityBlock = (DFunctionalityBlock*)tag.iValue;

	DStopModeExtension::Install(TheStopModeExtension);

	return KErrNone;
	}

/**
 * This stub constructor is intended to be used in the case where the old deprecated
 * stop mode api, kdebug, is not in place. It will initialise all values to NULL except
 * the pointer to the new stop mode api extension. This allows the new stop mode solution
 * to both co-exist and exist independantly of the existing one *
 */
DDebuggerInfo::DDebuggerInfo():
	iObjectOffsetTable(NULL),
	iObjectOffsetTableCount(NULL),
	iThreadContextTable(NULL),
	iStopModeExtension(new DStopModeExtension()),
	iContainers(NULL),
	iCodeSegLock(NULL),
	iCodeSegGlobalList(NULL),
	iScheduler(NULL),
	iShadowPages(NULL),
	iShadowPageCount(0),
	iCurrentThread(NULL),
	iEventMask(),
	iEventHandlerBreakpoint(0),
	iMemModelObjectOffsetTable(NULL),
	iMemModelObjectOffsetTableCount(0)
	{
	}

/**
 * Installs the stop-mode debugger extension
 * Make the stop-mode API visible to a JTAG debugger, by publishing its
 * existence in the superpage
*/
void DStopModeExtension::Install(DStopModeExtension* aExt)
	{
	Kern::SuperPage().iDebuggerInfo->iStopModeExtension = aExt;
	}

#endif

/**
Helper function

Allocates memory in current thread with a max length the same as aSrcDes. If
aReadFromClient is true (as it is by default) then the data from aSrdDes is
copied into the allocated aDestDes buffer.

Use of this function should be followed at a later time by a call such as
Kern::Free(aDestDes.Ptr())

@param aThread pointer to thread to read data from
@param aSrcDes descriptor in aThread to read data from
@param aDestDes location to read data to. Memory is allocated at this location,
       if memory is already allocated at this location then the function will
       return KErrArgument
@param aReadFromClient if false then data is not actually read from the
       client, the memory is simply allocated
@param aOffest offset into aSrcDes to start reading from. Default is 0.

@return KErrNone if there were no problems,
        KErrArgument if aDestDes.Ptr() != NULL or aSrcDes has max length 0,
        KErrNoMemory if could not allocate memory,
        or one of the other system wide error codes
*/
TInt DRM_DebugChannel::AllocAndReadDes(DThread *aThread, const TDesC8& aSrcDes, TPtr8& aDestDes, const TBool aReadFromClient, const TUint aOffset) const
	{

	//check thread is not null
	if(!aThread)
		{
		return KErrArgument;
		}

	//check aDestDes is empty
	if(aDestDes.Ptr() != NULL)
		{
		return KErrArgument;
		}

	//get the source descriptor's max length and exit if 0
	TUint srcMaxLength = Kern::ThreadGetDesMaxLength(aThread, &aSrcDes);
	if(srcMaxLength == 0)
		{
		return KErrNone;
		}

	//allocate memory and return if none available
	NKern::ThreadEnterCS();
	TUint8 *destPtr = (TUint8*)Kern::Alloc(srcMaxLength);
	NKern::ThreadLeaveCS();
	if (!destPtr)
		{
		return KErrNoMemory;
		}

	//point the TPtr8 at the target memory
	aDestDes.Set(destPtr, srcMaxLength, srcMaxLength);

	if(aReadFromClient)
		{
		//read data from the client thread and return status code
		return Kern::ThreadDesRead(aThread, &aSrcDes, aDestDes, aOffset);
		}
	else
		{
		return KErrNone;
		}
	}

/**
Helper function to extract a TRegisterInfo value from a descriptor containing
binary data.

@param aRegisterIds descriptor containing register IDs
@param aOffset offset in bytes into the descriptor to start reading data from.
       If this value is not a multiple of sizeof(TRegisterInfo) then a
       KErrArgument error is returned.
@param aValue will contain the returned value

@return KErrNone if aValue was set correctly, KErrArgument if bad arguments
        were passed in
*/
TInt DRM_DebugChannel::GetTRegisterInfo(const TDesC8 &aRegisterIds, const TUint aIndex, TRegisterInfo &aValue) const
	{
	TUint length = aRegisterIds.Length();

	TUint size = sizeof(TRegisterInfo);

	//check that not trying to read past end of descriptor
	if((aIndex + 1) * size > length)
		return KErrArgument;

	//get pointer to descriptor's data
	const TUint8 *dataPtr = aRegisterIds.Ptr();
	const TRegisterInfo *registerId = reinterpret_cast<const TRegisterInfo*>(dataPtr + (aIndex * size));

	aValue = *registerId;

	return KErrNone;
	}

/**
Helper function to get the kernel register ID of the TRegisterInfo defined register.

@param aDebugRegister the debug register ID to return the kernel ID for
@param aKernelRegister corresponding value of register aDebugRegister

@return KErrNone if translation occurred without problems
        KErrNotSupported if aDebugRegister is not supported by the kernel
*/
TInt DRM_DebugChannel::GetKernelRegisterId(const TRegisterInfo aDebugRegister, TArmReg& aKernelRegister) const
	{
	if(Register::IsCoreReg(aDebugRegister))
		{
		TUint id = Register::GetCoreRegId(aDebugRegister);
		//first 17 registers match the first 17 kernel registers
		if(id < 17)
			{
			aKernelRegister = id;
			}
		else
			{
			return KErrNotSupported;
			}
		}
	else if(Register::IsCoproReg(aDebugRegister))
		{
		TUint32 crn = Register::GetCRn(aDebugRegister);
		TUint32 crm = Register::GetCRm(aDebugRegister);
		TUint32 opcode1 = Register::GetOpcode1(aDebugRegister);
		TUint32 opcode2 = Register::GetOpcode2(aDebugRegister);
		TUint32 coproNum = Register::GetCoproNum(aDebugRegister);

		//each coprocessor register has potentially different characteristics
		//so need to identify each individually

		//this is the DACR, the ARM ARM specifies that the CRn and the
		//Opcodes are not relevant, section B3-24, point 3.7.3
		if((coproNum == 15) && (crm == 3))
			{
			aKernelRegister = EArmDacr;
			}
		else
			{
			return KErrNotSupported;
			}
		}
	else // might be supported at a later date
		{
		return KErrNotSupported;
		}

	return KErrNone;
	}

/**
Helper function to get the debug register ID of the kernel defined register.

@param aKernelRegister the kernel register ID to return the debug ID for
@param aDebugRegister corresponding value of register aKernelRegister

@return KErrNone if translation occured without problems
        KErrNotSupported if aKernelRegister is not supported by the debug
	security server
*/
TInt DRM_DebugChannel::GetDebugRegisterId(const TArmReg aKernelRegister, TRegisterInfo &aDebugRegister) const
	{

	// registers 0 - 15 and the CPSR share the same values as with the debug enums
	if(aKernelRegister < 17)
		{
		TUint32 id = aKernelRegister;
		aDebugRegister = id << 8;
		}
	//the DACR value is special and corresponds to EDF_Register_DACR
	else if(aKernelRegister == EArmDacr)
		{
		aDebugRegister = 0x00300f01;
		}
	// must be an unsupported register, return an error as such
	else
		{
		return KErrNotSupported;
		}

	//found a supported register so return KErrNone
	return KErrNone;
	}

/**
Helper function to find out whether the aIndex flag is set. This is equivalent
to the aIndex bit of aFlags being non-zero.

@param aFlags set of flags
@param aIndex offset into aFlags to get flag from

@return ETrue if bit is set, EFalse if not
*/
TBool DRM_DebugChannel::GetFlagAtOffset(const TUint32 aFlags, const TArmReg aIndex) const
	{
	return aFlags & (1<<aIndex);
	}

/* Register the attachment of a debug agent to a process to be debugged
 *
 * @param a1 - TDes8 target process name
 * @param a2 - &TUint64 - Debug Agent Id
 *
 * @return - KErrNone if successful. KErrArgument if the filepath is not a valid size.
 * KErrOutOfMemory if there is insufficient memory. Or one of the other system wide error codes
 * if appropriate.
 */
TInt DRM_DebugChannel::AttachProcess(TAny* a1, TAny* a2)
	{
	LOG_MSG("DRM_DebugChannel::AttachProcess()");

	// Validate the supplied TDes8 target process name in a1
	TInt length, maxLength;
	TUint8* aPtr;

	TInt err = Kern::ThreadGetDesInfo(iClientThread,\
		a1,\
		length,\
		maxLength,\
		aPtr,\
		EFalse);
	if (err != KErrNone)
		{
		// Could not read the descriptor information
		return err;
		}

	// Check the processname is a valid size for a filepath
	if (length < 1 || length >= KMaxPath)
		{
		return KErrArgument;
		}

	if (maxLength < 1 || maxLength >= KMaxPath)
		{
		return KErrArgument;
		}

	// Allocate space to store the target process name in a kernel-side TPtr8
	NKern::ThreadEnterCS();
	TUint8* buffer = (TUint8*)Kern::AllocZ(length);
	NKern::ThreadLeaveCS();
	if (buffer==NULL)
		{
		// Out of memory
		return KErrNoMemory;
		}

	// A temporary descriptor to store the target process name
	TPtr8 targetProcessName(buffer,length,length);

	// Read the user-side data into targetProcessName
	err = Kern::ThreadDesRead(iClientThread,a1,targetProcessName,0,KChunkShiftBy0);
	if (err != KErrNone)
		{
		// Could not read the user-side descriptor containing the target process name
		NKern::ThreadEnterCS();
		Kern::Free(buffer);
		NKern::ThreadLeaveCS();

		return err;
		}

	// Obtain the Debug Agent Id
	TUint64 debugAgentId = 0;

	err = Kern::ThreadRawRead(iClientThread,a2,&debugAgentId,sizeof(debugAgentId));
	if (err != KErrNone)
		{
		// Something bad happened so free the memory and return
		NKern::ThreadEnterCS();
		Kern::Free(buffer);
		NKern::ThreadLeaveCS();
		return err;
		}

	// Add the target process to our list of tracked processes
	err = TheDProcessTracker.AttachProcess(targetProcessName, debugAgentId);

	// Free the kernel-side memory containing targetProcessName data
	NKern::ThreadEnterCS();
	Kern::Free(buffer);
	NKern::ThreadLeaveCS();

	return err;
	}

/* Register the detachment of a debug agent to a process to be debugged.
 *
 * @param - a1 TDes8 target process name in a1
 * @param a2 - &TUint64 - Debug Agent Id
 *
 * @return - KErrNone if successful. KErrArgument if the filepath is not a valid size.
 * KErrOutOfMemory if there is insufficient memory. Or one of the other system wide error codes
 * if appropriate.
 */
TInt DRM_DebugChannel::DetachProcess(TAny* a1, TAny* a2)
	{
	// Validate the supplied TDes8 target process name in a1
	TInt length, maxLength;
	TUint8* aPtr;

	TInt err = Kern::ThreadGetDesInfo(iClientThread,\
		a1,\
		length,\
		maxLength,\
		aPtr,\
		EFalse);
	if (err != KErrNone)
		{
		return err;
		}

	if (length < 1 || length >= KMaxPath)
		{
		return KErrArgument;
		}

	if (maxLength < 1 || maxLength >= KMaxPath)
		{
		return KErrArgument;
		}

	// Allocate space to store the target process name in a kernel-side TPtr8
	NKern::ThreadEnterCS();
	TUint8* buffer = (TUint8*)Kern::AllocZ(length);
	NKern::ThreadLeaveCS();
	if (buffer==NULL)
		{
		// Out of memory
		return KErrNoMemory;
		}

	TPtr8 targetProcessName(buffer,length,length);

	// Read the user-side data into targetProcessName
	err = Kern::ThreadDesRead(iClientThread,a1,targetProcessName,0,KChunkShiftBy0);
	if (err != KErrNone)
		{
		// Something bad happened so free the memory and return
		NKern::ThreadEnterCS();
		Kern::Free(buffer);
		NKern::ThreadLeaveCS();

		return err;
		}

	// Obtain the AgentId
	TUint64 debugAgentId = 0;

	err = Kern::ThreadRawRead(iClientThread,a2,&debugAgentId,sizeof(debugAgentId));
	if (err != KErrNone)
		{
		// Something bad happened so free the memory and return
		NKern::ThreadEnterCS();
		Kern::Free(buffer);
		NKern::ThreadLeaveCS();

		return err;
		}

	// Remove the process from our list of tracked processes
	err = TheDProcessTracker.DetachProcess(targetProcessName, debugAgentId);

	// Free the kernel-side memory containing targetProcessName data
	NKern::ThreadEnterCS();
	Kern::Free(buffer);
	NKern::ThreadLeaveCS();

	return err;
	}

/* Register the detachment of a debug agent from all processes being debugged.
 *
 * @param - a1 - &TUint64 Debug Agent Id.
 * @return - KErrNone if successful. One of the system-wide error codes otherwise.
 */
TInt DRM_DebugChannel::DetachAgent(TAny* a1, TAny* a2)
	{
	// Obtain the AgentId
	TUint64 debugAgentId = 0;

	TInt err = Kern::ThreadRawRead(iClientThread,a1,&debugAgentId,sizeof(debugAgentId));
	if (err != KErrNone)
		{
		return err;
		}

	// Remove the process from our list of tracked processes
	return TheDProcessTracker.DetachAgent(debugAgentId);
	}

/* Set the action associated with a particular kernel event for a given agent and target process
 *
 * @param - a1 TDes8 target process name in a1
 * @param - a2 &TRM_DebugEventActionInfo
 * @return - KErrNone if successful. KErrArgument if the filepath is an invalid size. Or one of
 * the other system wide error codes if appropriate.
 */
TInt DRM_DebugChannel::SetEventAction(TAny* a1, TAny* a2)
	{
	// Validate the supplied TDes8 target process name in a1
	TInt length, maxLength;
	TUint8* aPtr;

	TInt err = Kern::ThreadGetDesInfo(iClientThread,\
		a1,\
		length,\
		maxLength,\
		aPtr,\
		EFalse);
	if (err != KErrNone)
		{
		return err;
		}

	if (length < 1 || length >= KMaxPath)
		{
		return KErrArgument;
		}

	if (maxLength < 1 || maxLength >= KMaxPath)
		{
		return KErrArgument;
		}

	// Allocate space to store the target process name in a kernelspace TPtr8
	NKern::ThreadEnterCS();
	TUint8* buffer = (TUint8*)Kern::AllocZ(length);
	NKern::ThreadLeaveCS();
	if (buffer==NULL)
		{
		// Out of memory
		return KErrNoMemory;
		}
	TPtr8 targetProcessName(buffer,length,length);

	// Read the user-side data into targetProcessName
	err = Kern::ThreadDesRead(iClientThread,a1,targetProcessName,0,KChunkShiftBy0);
	if (err != KErrNone)
		{
		// Something bad happened so free the memory and return
		NKern::ThreadEnterCS();
		Kern::Free(buffer);
		NKern::ThreadLeaveCS();

		return err;
		}

	// Read the Event and Action from the user-side
	TRM_DebugEventActionInfo info(0,0,0);

	err = Kern::ThreadRawRead(iClientThread, a2, &info, sizeof(info));
	if (err != KErrNone)
		{
		// Could not read event action data from the user-side

		// Free memory used for targetProcessName
		NKern::ThreadEnterCS();
		Kern::Free(buffer);
		NKern::ThreadLeaveCS();

		return err;
		}

	// Find the target process
	DTargetProcess* pProcess = TheDProcessTracker.FindProcess(targetProcessName);
	if (pProcess == NULL)
		{
		// Could not find this process

		// Free memory used for targetProcessName
		NKern::ThreadEnterCS();
		Kern::Free(buffer);
		NKern::ThreadLeaveCS();

		return KErrArgument;
		}

	TUint64 debugAgentId = info.iAgentId;

	// Find the agent
	DDebugAgent* debugAgent = pProcess->Agent(debugAgentId);
	if (debugAgent == NULL)
		{
		// Bad agent means there is no tracking agent
		LOG_MSG2("Cannot locate debug agent with pid 0x%0x16lx",info.iAgentId);

		// Free memory used for targetProcessName
		NKern::ThreadEnterCS();
		Kern::Free(buffer);
		NKern::ThreadLeaveCS();

		return KErrGeneral;
		}

	// Set the event action
	debugAgent->SetEventAction((TEventType)info.iEvent,(TKernelEventAction)info.iAction);

	// Free memory used for targetProcessName
	NKern::ThreadEnterCS();
	Kern::Free(buffer);
	NKern::ThreadLeaveCS();

	return KErrNone;
	}

TInt DRM_DebugChannel::Step(const TUint32 aThreadId, const TUint32 aNumSteps)
	{
	LOG_MSG3("DRM_DebugChannel::Step(aThreadId = 0x%08x, aNumSteps = 0x%08x)\n",aThreadId,aNumSteps);

	DThread* thread = DebugUtils::OpenThreadHandle(aThreadId);

	if (thread == NULL)
		{
		// The thread terminated before we could open it.
		LOG_MSG2("DRM_DebugChannel::Step - Could not open thread %u", aThreadId);

		return KErrArgument;
		}

	// We simply repeat this for desired number of steps
	TInt err = KErrNone;

	// Need to step from the current location for 'n' steps
	TUint32 startAddress;

	// We always step from the current PC.
	err = ReadKernelRegisterValue(thread, PC_REGISTER, startAddress);
	if(err != KErrNone)
		{
		LOG_MSG2("DRM_DebugChannel::Step - Could not read the PC: %d", err);

		// Close the handle
		thread->Close(NULL);

		return err;
		}

	err = DoStepRange(thread, startAddress, startAddress, ETrue, EFalse, aNumSteps, ETrue);

	if (err != KErrNone)
		{
		// There was a problem, return straightaway
		LOG_MSG("DRM_DebugChannel::Step - failed to step");
		}

	// Close the handle
	thread->Close(NULL);

	return err;
	}

TInt DRM_DebugChannel::KillProcess(const TUint32 aProcessId, const TInt aReason)
	{
	LOG_MSG3("DRM_DebugChannel::KillProcess(aProcessId = 0x%08x, aReason = 0x%08x)\n",aProcessId,aReason);

	DProcess* process = DebugUtils::OpenProcessHandle(aProcessId);

	if (process == NULL)
		{
		// The process terminated before we could open it to kill it ourselves.
		LOG_MSG2("DRM_DebugChannel::KillProcess - Could not open process %u", aProcessId);

		return KErrArgument;
		}

	TInt err = KErrNone;

	DebugSupport::TerminateProcess(process,aReason);

	// Close the handle
	process->Close(NULL);

	return err;
	}

/* Security critical - this checks whether the specified process is debuggable or not
 *
 * @param aProcessId - The process id of the process to check
 * @return KErrNone if debuggable, KErrPermissionDenied if not debuggable.
 */
TInt DRM_DebugChannel::IsDebuggable(const TUint32 aProcessId)
	{
	/* In order to ensure that only processes which are debuggable
	 * can be debugged, this function enables the security server
	 * to read the DProcess.iDebugAttributes field and ensure
	 * the process was created from a debuggable executable.
	 */
	LOG_MSG2("DRM_DebugChannel::IsDebuggable(aProcessId 0x%08x)\n",aProcessId);

	TInt err = KErrPermissionDenied;

	DProcess* process = DebugUtils::OpenProcessHandle(aProcessId);
	if (process)
		{
		if (process->iDebugAttributes & TProcessCreateInfo::EDebugAllowed)
			{
			// Yes this process exists and is debuggable
			err = KErrNone;
			}
		process->Close(NULL);
		}

	if (err == KErrNone)
		{
		LOG_MSG2("DRM_DebugChannel::IsDebuggable(aProcessId 0x%08x) - Yes it is debuggable\n",aProcessId);
		}

	return err;
	}
