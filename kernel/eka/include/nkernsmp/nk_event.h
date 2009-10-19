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
// e32\include\nkernsmp\nk_event.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __NK_EVENT_H__
#define __NK_EVENT_H__

#include <nklib.h>

class NSchedulable;
class TSpinLock;
class TDfcQue;

/********************************************
 * Event handler
 ********************************************/

/**
@internalComponent

General event handler callback
*/
typedef void (*NEventFn)(TAny*);

/**
@internalComponent

General event handler structure
*/
struct NEventHandler : public SDblQueLink		// link into active queue
	{
	inline NEventHandler()
		:	iHState(0), iTied(0), iPtr(0), iFn(0)
		{
		iTiedLink.iNext = 0;
		}

	enum TEventHandlerType
		{
		EEventHandlerIrq=0xFCu,
		EEventHandlerNTimer=0xFDu,
		EEventHandlerIDFC=0xFEu,
		EEventHandlerDummy=0xFFu,
		};
	struct N8816
		{
		TUint8	iHType;
		TUint8	iHState8;
		TUint16	iHState16;
		};
	struct N8888
		{
		TUint8	iHType;
		TUint8	iHState0;
		TUint8	iHState1;
		TUint8	iHState2;
		};
	union
		{
		volatile TUint32		iHState;		// state information
		volatile TUint8			iHType;			// type of event handler
		volatile N8816			i8816;
		volatile N8888			i8888;
		};
	union
		{
		NSchedulable* volatile	iTied;			// pointer to tied thread/group
		TDfcQue* volatile		iDfcQ;			// doubles as pointer to DFC queue
		};										// since DFCs can't be tied
	TAny* volatile				iPtr;			// argument to callback
	volatile NEventFn			iFn;			// callback function
	SDblQueLink					iTiedLink;		// link to tied/group for cleanup purposes

	static TSpinLock			TiedLock;		// Protects iTied member of all NEventHandlers
	};

#endif
