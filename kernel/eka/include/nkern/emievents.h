// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __EMIEVENTS_H__
#define __EMIEVENTS_H__

/**
@publishedPartner
@released

Defines the format of a task event record.

An item of this type is passed to:

-# EMI::GetTaskEvent()
-# EMI::AddTaskEvent()
*/
struct TTaskEventRecord
	{
	/**
	Defines the type of event:
	
    0 = Reschedule 
    1..127 = Reserved
	> 127 = User Defined
	*/
	TUint8	iType;	
				
						
	/**
	Flag bits.
	
    Bit 0 - Events have been lost before this event. (All types) 
	Bit 1 - Previous thread is now waiting. (Reschedule only)
	*/					
	TUint8	iFlags;
	
	
	/**
	This has no use in reschedule events, but may be used by other event types.
	*/
	TUint16	iExtra;
	
	
	/**
	The state variable at the time of the event, which will
	probably indicate the clock frequency at the time of the event.
	*/
	TUint32	iUserState;
	
	
	/**
	The time that the event occurred.
	
	The units are defined by the GET_HIGH_RES_TICK macro.
	*/
	TUint32	iTime;
	
	
	/**
	The NThread that was executing before the switch.
	*/
	TAny*	iPrevious;
	
	
	/**
	The NThread that was executing after the switch.
	*/
	TAny*	iNext;
	};

const TUint8 KTskEvtFlag_EventLost  =1;
const TUint8 KTskEvtFlag_PrevWaiting=2;

#endif
