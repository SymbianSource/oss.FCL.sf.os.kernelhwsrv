// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\eventq.cpp
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"
#include <e32btrace.h>
#include <kerncorestats.h>
/******************************************************
 * Event Queue
 ******************************************************/

/** Release the event queue mutex and panic the current thread with a KERN-EXEC panic.
*/ 
void EventQueuePanic(TInt aReason)
	{
	NKern::FMSignal(&K::EventQueueMutex);
	Kern::PanicCurrentThread(KLitKernExec, aReason);
	}

void K::CreateEventQueue(TInt aSize)
//
// Create the event queue
//
	{

	__KTRACE_OPT(KBOOT,Kern::Printf("CreateEventQueue"));
	K::EventBufferStart=(TRawEvent *)Kern::Alloc(sizeof(TRawEvent)*aSize);
	if (!K::EventBufferStart)
		K::Fault(K::ECreateEventQueueFailed);
	K::EventBufferEnd=K::EventBufferStart+aSize;
	K::EventHeadPtr=K::EventTailPtr=K::EventBufferStart;
	if (Kern::CreateClientDataRequest(K::EventRequest) != KErrNone)
		K::Fault(K::ECreateEventQueueFailed);
	}

TInt ExecHandler::AddEvent(const TRawEvent &aEvent)
//
// Add an event to the queue.
//
	{
	__KTRACE_OPT(KEVENT,Kern::Printf("Exec::AddEvent"));
	if(!Kern::CurrentThreadHasCapability(ECapabilitySwEvent,__PLATSEC_DIAGNOSTIC_STRING("Checked by UserSvr::AddEvent")))
		return KErrPermissionDenied;
	TRawEvent event;
	kumemget32(&event,&aEvent,sizeof(TRawEvent));
	TRawEvent::TType type = event.Type();
	if( (type==TRawEvent::ESwitchOff || type==TRawEvent::ECaseOpen || type==TRawEvent::ECaseClose || type==TRawEvent::ERestartSystem) && 
		!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by UserSvr::AddEvent")))
		return KErrPermissionDenied;
#ifdef BTRACE_TRAWEVENT		
	BTraceContext4(BTrace::ERawEvent, BTrace::EUserAddEvent ,(TUint32)type);
#endif
	NKern::ThreadEnterCS();	
	TInt r=Kern::AddEvent(event);
	NKern::ThreadLeaveCS();
	return r;
	}

/**	Adds an event to the event queue.
	Invoking this will reset the user inactivity timer.
	@param	"const TRawEvent& aEvent" a reference to the event to be added to the event queue.

	@return KErrNone, if successful; KErrOverflow if event queue is already full.

	@pre No fast mutex can be held.
	@pre Calling thread must be in a critical section.
	@pre Kernel must be unlocked
	@pre interrupts enabled
	@pre Call in a thread context
*/
EXPORT_C TInt Kern::AddEvent(const TRawEvent& aEvent)
	{
	return Kern::AddEvent(aEvent, ETrue);
	}

/**	Adds an event to the event queue.

	@param	"const TRawEvent& aEvent" a reference to the event to be added to the event queue.
	@param	"TBool aResetUserActivity" Specifies whether this event should reset the user inactivity timer. 
			For most cases this should be set to ETrue

	@return KErrNone, if successful; KErrOverflow if event queue is already full.

	@pre No fast mutex can be held.
	@pre Calling thread must be in a critical section.
	@pre Kernel must be unlocked
	@pre interrupts enabled
	@pre Call in a thread context
*/
EXPORT_C TInt Kern::AddEvent(const TRawEvent& aEvent, TBool aResetUserActivity)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::AddEvent");	
	__KTRACE_OPT(KEVENT,Kern::Printf("Kern::AddEvent %x, %x",K::EventHeadPtr,K::EventTailPtr));

	if (aResetUserActivity)
		{
		K::InactivityQ->Reset();
		if (K::PowerModel)
			K::PowerModel->RegisterUserActivity(aEvent);
		}
#ifdef BTRACE_TRAWEVENT		
	BTraceContext4(BTrace::ERawEvent, BTrace::EKernelAddEvent ,(TUint32)aEvent.Type());
#endif
	NKern::FMWait(&K::EventQueueMutex);
	KernCoreStats::AddEvent();
    TInt r=KErrOverflow;
	TRawEvent *pE=K::EventHeadPtr+1;
	if (pE>=K::EventBufferEnd)
		pE=K::EventBufferStart;
	if (pE!=K::EventTailPtr)
		{
		*K::EventHeadPtr=aEvent;
		K::EventHeadPtr=pE;
        r=KErrNone;
		}
	K::TryDeliverEvent();
	return r;
	}

void K::TryDeliverEvent()
//
// Deliver an event if one is pending.
// Enter with event queue mutex held, leave with it released.
//
	{

	if (K::EventHeadPtr!=K::EventTailPtr && K::EventRequest->IsReady())
		{
		K::EventRequest->Data() = *K::EventTailPtr;
		K::EventTailPtr++;
		if (K::EventTailPtr>=K::EventBufferEnd)
			K::EventTailPtr=K::EventBufferStart;
		Kern::QueueRequestComplete(K::EventThread,K::EventRequest,KErrNone);
		__KTRACE_OPT(KEVENT,Kern::Printf("Delivered event"));
		}
	NKern::FMSignal(&K::EventQueueMutex);
	}

void ExecHandler::CaptureEventHook()
//
// Capture the event hook.
// Enter and leave with system unlocked.
//
	{
	__KTRACE_OPT(KEVENT,Kern::Printf("Exec::CaptureEventHook"));

	if(TheCurrentThread->iOwningProcess!=K::TheWindowServerProcess)
		{
		NKern::LockSystem();
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by UserSvr::CaptureEventHook"));
		}
	NKern::FMWait(&K::EventQueueMutex);
	if (!K::EventThread)
		K::EventThread=TheCurrentThread;
	else
		EventQueuePanic(EEventAlreadyCaptured);
	NKern::FMSignal(&K::EventQueueMutex);
	}

void ExecHandler::ReleaseEventHook()
//
// Release the event hook.
// Enter and leave with system unlocked.
//
	{
	__KTRACE_OPT(KEVENT,Kern::Printf("Exec::ReleaseEventHook"));
	DThread* pT=TheCurrentThread;
	NKern::FMWait(&K::EventQueueMutex);
	if (K::EventThread==pT)
		{
		K::EventThread=NULL;
		Kern::QueueRequestComplete(K::EventThread,K::EventRequest,KErrCancel);
		}
	else
		EventQueuePanic(EEventNotCaptured);
	NKern::FMSignal(&K::EventQueueMutex);
	}

void ExecHandler::RequestEvent(TRawEventBuf& aEvent, TRequestStatus& aStatus)
//
// Request the next event.
// Enter and leave with system unlocked.
//
	{
   	__KTRACE_OPT(KEVENT,Kern::Printf("Exec::RequestEvent"));
	NKern::FMWait(&K::EventQueueMutex);
	if (K::EventThread!=TheCurrentThread)
		EventQueuePanic(EEventNotCaptured);
	if (K::EventRequest->SetStatus(&aStatus) != KErrNone)
		EventQueuePanic(EEventRequestPending);
	
	// This could be simpler if the API was changed to pass a pointer to a TRawEvent instead of a
	// TRawEventBuf descriptor.  We cheat here and assume the descriptor is always a TRawEventBuf;
	// this should be safe since the API specifies a TRawEventBuf.	
	K::EventRequest->SetDestPtr(((TUint8*)&aEvent) + sizeof(TDes8));
	
	K::TryDeliverEvent();
	}

void ExecHandler::RequestEventCancel()
//
// Cancel the event request.
// Enter and leave with system unlocked.
//
	{
	__KTRACE_OPT(KEVENT,Kern::Printf("Exec::RequestEventCancel"));
	NKern::FMWait(&K::EventQueueMutex);
	if (K::EventThread!=TheCurrentThread)
		EventQueuePanic(EEventNotCaptured);
	Kern::QueueRequestComplete(K::EventThread,K::EventRequest,KErrCancel);
	NKern::FMSignal(&K::EventQueueMutex);
	}


/** Gets the mouse or digitiser X-Y position.

	@return The position.
	
	@pre	Call in any context.
 */
EXPORT_C TPoint TRawEvent::Pos() const
	{
	TPoint p;
	p.iX=iU.pos.x;
	p.iY=iU.pos.y;
	return p;
	}

/** Gets the 3D pointing device Cartesian coordinates.

	@return The position.
	
	@pre	Call in any context.
 */
EXPORT_C TPoint3D TRawEvent::Pos3D() const
	{
	TPoint3D p;
	p.iX=iU.pos.x;
	p.iY=iU.pos.y;
	p.iZ=iU.pos3D.z;
	return p;
	}

/** Gets the angular spherical polar coordinates of the 3D pointer end that is closer to the screen.
    
    @return The angular spherical polar coordinates of the point defined by the end of the 3D pointing device that is closer to the screen.
	
	@pre	Call in any context.
 */
EXPORT_C TAngle3D TRawEvent::Tilt() const
	{
	TAngle3D p;
	p.iPhi=iU.pos3D.phi;
	p.iTheta=iU.pos3D.theta;
	return p;
	}

/** Gets the rotation angle of 3D pointing device.
    
    @return The rotation angle of the 3D pointing device.
	
	@pre	Call in any context.
 */
EXPORT_C TInt TRawEvent::Rotation() const
	{
	return (iU.pos3D.alpha);
	}

/** Sets the event up as a scancode event.

	@param	aType	The event type.
	@param	aScanCode The scancode.
	
	@pre	Call in any context.
 */
EXPORT_C void TRawEvent::Set(TType aType,TInt aScanCode)
	{
	iType=(TUint8)aType;
	iU.key.scanCode=aScanCode;
	iU.key.repeats=0;
#ifdef BTRACE_TRAWEVENT	
	BTraceContext8(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aScanCode);
#endif
	iTicks=Kern::TickCount();
	}


/** Sets the event up as a repeating scancode event.

	@param	aType	The event type.
	@param	aScanCode The scancode.
	@param	aRepeats  The repeat count.
	
	@pre	Call in any context.
 */
EXPORT_C void TRawEvent::SetRepeat(TType aType,TInt aScanCode,TInt aRepeats)
	{
	iType=(TUint8)aType;
	iU.key.scanCode=aScanCode;
	iU.key.repeats=aRepeats;
#ifdef BTRACE_TRAWEVENT	
	BTraceContext12(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aScanCode, (TUint32)aRepeats);
#endif
	iTicks=Kern::TickCount();
	}


/** Sets the event up as a mouse/pen event.

	@param	aType	The event type.
	@param	aX		The X position.
	@param	aY		The Y position.
	
	@pre	Call in any context.
 */
EXPORT_C void TRawEvent::Set(TType aType,TInt aX,TInt aY)
	{
	iType=(TUint8)aType;
	iU.pos.x=aX;
	iU.pos.y=aY;
#ifdef BTRACE_TRAWEVENT	
	BTraceContext12(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aX,(TUint32)aY);
#endif
	iTicks=Kern::TickCount();
	}


/** Sets the event up as a 3D pointer linear move event.

	@param	aType	The event type.
	@param	aX		The X position.
	@param	aY		The Y position.
	@param	aZ		The Z position (defaults to 0 on 2D detection devices).
	
	@pre	Call in any context.
 */
EXPORT_C void TRawEvent::Set(TType aType,TInt aX,TInt aY,TInt aZ)
	{
	iType=(TUint8)aType;
	iU.pos.x=aX;
	iU.pos.y=aY;
	iU.pos3D.z=aZ;

#ifdef BTRACE_TRAWEVENT	
	//Form the Trace Data
	TUint32 traceData[2];
    traceData[0] =aY;
	traceData[1] =aZ;
    BTraceContextN(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aX,traceData, sizeof(traceData));    
#endif
	iTicks=Kern::TickCount();
	}

/** Sets up an event without parameters.

	@param	aType	The event type.
	
	@pre	Call in any context.
 */
EXPORT_C void TRawEvent::Set(TType aType)
	{
	iType=(TUint8)aType;
#ifdef BTRACE_TRAWEVENT	
	BTraceContext4(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType);
#endif 
	iTicks=Kern::TickCount();
	}

/** Sets the event up as a 3D pointer linear move and tilt and rotation change event.

	@param	aType	The event type.
	@param	aX		The X position (or TPhi polar coordinate).
	@param	aY		The Y position (or Theta polar coordinate).
	@param	aZ		The Z position (or rotation).
	@param	aPhi	The Phi polar coordinate.
	@param	aTheta	The Theta polar coordinate.
	@param	aAlpha	The rotation angle.
	
	@pre	Call in any context.
 */
EXPORT_C void TRawEvent::Set(TType aType,TInt aX,TInt aY,TInt aZ,TInt aPhi,TInt aTheta,TInt aAlpha)
	{
	iType=(TUint8)aType;
	iU.pos.x=aX;
	iU.pos.y=aY;
	iU.pos3D.z=aZ;
	iU.pos3D.phi=aPhi;
	iU.pos3D.theta=aTheta;
	iU.pos3D.alpha=aAlpha;
#ifdef BTRACE_TRAWEVENT	
	//Form the Trace Data
	TUint32 traceData[5];
    traceData[0] =aY;
	traceData[1] =aZ;
	traceData[2] =aPhi;
	traceData[3] =aTheta;
	traceData[4] =aAlpha;
	BTraceContextN(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aX,traceData, sizeof(traceData));		
#endif
	iTicks=Kern::TickCount();
	}

/** Sets the event up as 3D pointer tilt change event.

	@param	aType	The event type.
	@param	aPhi	The Phi coordinate.
	@param	aTheta	The Theta coordinate.
	
	@pre	Call in any context.
*/
EXPORT_C void TRawEvent::SetTilt(TType aType,TInt aPhi,TInt aTheta)
	{
	iType=(TUint8)aType;
	iU.pos3D.phi=aPhi;
	iU.pos3D.theta=aTheta;
#ifdef BTRACE_TRAWEVENT	
	BTraceContext12(BTrace::ERawEvent, BTrace::ESetTiltEvent ,(TUint32)aType, (TUint32)aPhi,(TUint32)aTheta);
#endif
	iTicks=Kern::TickCount();
	}

/** Sets the event up as 3D pointer rotation event.

	@param	aType	The event type.
	@param	aAlpha	The rotation angle.
	
	@pre	Call in any context.
*/
EXPORT_C void TRawEvent::SetRotation(TType aType,TInt aAlpha)
	{
	iType=(TUint8)aType;
	iU.pos3D.alpha=aAlpha;
#ifdef BTRACE_TRAWEVENT	
	BTraceContext8(BTrace::ERawEvent, BTrace::ESetRotationtEvent,(TUint32)aType, (TUint32)aAlpha);
#endif
	iTicks=Kern::TickCount();
	}



/** Returns the scancode.

	@return The scancode.
	
	@pre	Call in any context.
 */
EXPORT_C TInt TRawEvent::ScanCode() const
	{
	return iU.key.scanCode;
	}


/** Returns the repeat count.

	@return The repeat count.
	
	@pre	Call in any context.
 */
EXPORT_C TInt TRawEvent::Repeats() const
	{
	return iU.key.repeats;
	}


/** Sets the event up as a 3D pointer linear move event with pointer number 
	@param	aType	The event type.
	@param	aX		The X position.
	@param	aY		The Y position.
	@param	aZ		The Z position .
	@param  aPointerNumber   The pointer number for the event 

	@pre	Call in any context.
 
*/

EXPORT_C void TRawEvent::Set (TType aType, TInt aX, TInt aY, TInt aZ, TUint8 aPointerNumber)
	{

	iType=(TUint8)aType;
	iU.pos3D.x=aX;
	iU.pos3D.y=aY;
	iU.pos3D.z=aZ;
	iPointerNumber=aPointerNumber;	
#ifdef BTRACE_TRAWEVENT	
	//Form the Trace Data
	TUint32 traceData[3];
    traceData[0] =aY;
	traceData[1] =aZ;
	traceData[2] =aPointerNumber;
    BTraceContextN(BTrace::ERawEvent, BTrace::ESetEvent ,(TUint32)aType, (TUint32)aX,traceData, sizeof(traceData));    	
#endif
	iTicks=Kern::TickCount();
	}
