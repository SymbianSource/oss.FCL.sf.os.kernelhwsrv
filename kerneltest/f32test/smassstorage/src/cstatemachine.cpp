// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implementation of finite state machine
// 
//

/**
 @file
 @internalTechnology
*/

#include "t_ms_main.h"
#include "cstatemachine.h"
#include "tstate.h"

CStateMachine* 
CStateMachine::NewL()
	{
	return new (ELeave) CStateMachine;
	}
	
CStateMachine::CStateMachine()
	{
    // Intentionally left blank
	}

CStateMachine::~CStateMachine()
	{
    allStates.ResetAndDestroy();
	}

void
CStateMachine::MoveTo(int aStateId)
	{
    if (!iCurrentState)
    	{
        test.Printf(_L("The current state is undefined\n"));
        test(EFalse);
    	}

    iCurrentState->MoveTo(aStateId);
    iCurrentState = FindState(aStateId);
	}

const TState*
CStateMachine::AddState(int aStateId)
	{
    const TState* state = FindState(aStateId);

    if (state == 0)
    	{
         switch (aStateId)
         	{
         case EUsbMsDriveState_Disconnected:
             state = new TDisconnected();
             break;
         case EUsbMsDriveState_Connecting:
             state = new TConnecting();
             break;
         case EUsbMsDriveState_Connected:
             state = new TConnected();
             break;     
         case EUsbMsDriveState_Disconnecting:
             state = new TDisconnecting();
             break;
         case EUsbMsDriveState_Active:
             state = new TActive();
             break;
         case EUsbMsDriveState_Locked:
             state = new TLocked();
             break;
         case EUsbMsState_Read:
             state = new TRead();
             break;
         case EUsbMsState_Written:
             state = new TWritten();
             break;
         default:
             break;
         	}
    
         if (state)
         	{
             allStates.Append(state);
         	}
    	}

    return state;
	}

void
CStateMachine::SetInitState(int aStateId)
	{
    iCurrentState = FindState(aStateId);
    iFromStateId = aStateId;
	}

const TState* 
CStateMachine::FindState(int aStateId) const
	{
    TInt count = allStates.Count();
    for (TInt i = 0; i < count; i++)
    	{
        if (allStates[i]->GetStateId() == aStateId)
        	{
            return allStates[i];
        	}
    	}

    return 0;
	}

TInt CStateMachine::CurrentStateId() const
	{
	return iCurrentState->GetStateId();
	}

TInt CStateMachine::FromStateId() const
	{
	return iFromStateId;
	}

void CStateMachine::SetFromStateId(TInt aStateId)
	{
	iFromStateId = aStateId;
	}
