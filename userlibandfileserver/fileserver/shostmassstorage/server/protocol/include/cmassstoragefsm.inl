// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
 
 Sets the StateId to be entered next time the FSM is called.
 
 @param aStateId The State ID
*/
inline void CMassStorageFsm::SetState(TMassStorageState::TStateId aStateId)
    {
    __HOSTPRINT1(_L("CMassStorage::SetState[%d]"), aStateId);
    iState = iStateTable[aStateId];
    }


/**
Sets the flag to indicate that the device requires a SCSI START STOP UNIT
command.

@param aRequired ETrue if SCSI START STOP UNIT command is required
*/
inline void CMassStorageFsm::SetStartStopUnitRequired(TBool aRequired)
    {
    iStartStopUnitRequired = aRequired;
    }


/**
Returns boolean state of SCSI START STOP UNIT Required flag.

@return TBool ETrue if SCSI START STOP UNIT command is required
*/
inline TBool CMassStorageFsm::StartStopUnitRequired() const
    {
    return iStartStopUnitRequired;
    }



