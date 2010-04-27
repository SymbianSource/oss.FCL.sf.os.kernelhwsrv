// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
*/

#include <e32base.h>

#include "usbmshostpanic.h"
#include "debug.h"
#include "msdebug.h"

#include "msctypes.h"
#include "mprotocol.h"
#include "mblocktransferprotocol.h"
#include "tspcclientinterface.h"
#include "cscsiprotocol.h"
#include "cmassstoragefsm.h"


/**
Constructor

@param aScsiProtocol A reference to CScsiProtocol
*/
TMassStorage::TMassStorage(CScsiProtocol& aScsiProtocol)
:   iScsiProtocol(aScsiProtocol)
    {
	__MSFNLOG
    }


/**
Clear StatusCheck flag. Periodic status checking is used for removable media.
*/
void CMassStorageFsm::ClearStatusCheck()
{
	__MSFNLOG
	iStatusCheck = EFalse;
}


/**
Set StatusCheck flag. Periodic status checking is used for removable media.
*/
void CMassStorageFsm::SetStatusCheck()
{
	__MSFNLOG
	iStatusCheck = ETrue;
}


/**
Get the boolean state of StatusCheck flag.

@return TBool ETrue if StatusCheck flag is set
*/
TBool CMassStorageFsm::IsStatusCheck() const
{
	__MSFNSLOG
	return iStatusCheck;
}


/**
Device supports SCSI BLOCK COMMANDS.

@return TBool Returns ETrue if the SBC interface is initialised
*/
TBool TMassStorage::IsSbcSet() const
    {
	__MSFNSLOG
    return iScsiProtocol.MsIsSbcSet();
    }

/**
Device is removable media

@return TBool Returns ETrue if removable
*/
TBool TMassStorage::IsRemovableMedia() const
    {
	__MSFNSLOG
    return iScsiProtocol.MsIsRemovableMedia();
    }

/**
Retrieve the sense info returned by a call to SenseL

@return const TSenseInfo& Returns the SCSI SENSE info
*/
const TSenseInfo& TMassStorage::MsSenseInfo() const
    {
	__MSFNSLOG
    return iScsiProtocol.MsSenseInfo();
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::InquiryL()
    {
	__MSFNLOG
    TEvent event = EEvCommandFailed;

    switch (iScsiProtocol.MsInquiryL())
        {
    case KErrNone:
        event = EEvCommandPassed;
        break;
    case KErrCommandFailed:
        event = EEvCommandFailed;
        break;
    case KErrNotSupported:
        __SCSIPRINT(_L("INQUIRY Command returned NOT SUPPORTED"));
        User::Leave(KErrNotSupported);
        break;
    default:
        __SCSIPRINT(_L("INQUIRY Command returned KErrUnknown"));
        User::Leave(KErrUnknown);
        break;
        }
    return event;
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::SenseL()
    {
	__MSFNLOG
    TEvent event = EEvCommandPassed;
    switch (iScsiProtocol.MsRequestSenseL())
        {
    case KErrNone:
        break;
    case KErrCommandFailed:
        __SCSIPRINT(_L("REQUEST SENSE Command Failed"));
        User::Leave(KErrNotSupported);
        break;
    default:
        __SCSIPRINT(_L("INQUIRY Command returned KErrUnknown"));
        User::Leave(KErrUnknown);
        break;
        }
    return event;
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::TestUnitReadyL()
    {
	__MSFNLOG
    // KErrCommandFailed indictates that device is NOT READY
    TInt err = iScsiProtocol.MsTestUnitReadyL();
    return (err == KErrNone) ? EEvCommandPassed : EEvCommandFailed;
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::StartStopUnitL(TBool aStart)
    {
	__MSFNLOG
    TInt err = iScsiProtocol.MsStartStopUnitL(aStart);
    return (err == KErrNone) ? EEvCommandPassed : EEvCommandFailed;
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::PreventAllowMediumRemovalL(TBool aPrevent)
    {
	__MSFNLOG
    // KErrCommandFailed indictates that device is NOT READY
    TInt err = iScsiProtocol.MsPreventAllowMediaRemovalL(aPrevent);
    return (err == KErrNone) ? EEvCommandPassed : EEvCommandFailed;
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::ReadCapacityL()
    {
	__MSFNLOG
    TInt err = iScsiProtocol.MsReadCapacityL();
    return ErrToEvent(err);
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::ModeSense10L()
    {
	__MSFNLOG
    TInt err = iScsiProtocol.MsModeSense10L();
    return ErrToEvent(err);
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::ModeSense6L()
    {
	__MSFNLOG
    TInt err = iScsiProtocol.MsModeSense6L();
    return ErrToEvent(err);
    }


/**
Creates and returns a pointer to CMassStorageFsm.

@param aScsiProtocol A reference to the protocol providing the interface to the
mass storage device.

@return CMassStorageFsm* A pointer to the newly created object
*/
CMassStorageFsm* CMassStorageFsm::NewL(CScsiProtocol& aScsiProtocol)
{
	__MSFNSLOG
	CMassStorageFsm* r = new (ELeave) CMassStorageFsm(aScsiProtocol);

	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
}

void CMassStorageFsm::ConstructL()
    {
	__MSFNLOG
    TInt i = 0;
    // EInquiryState,
    iStateTable[i++] = new (ELeave) TInquiryState;
    // EInquirySenseState,
    iStateTable[i++] = new (ELeave) TInquirySenseState;
    // ENotReadyState,
    iStateTable[i++] = new (ELeave) TNotReadyState;
    // ENotReadySenseState,
    iStateTable[i++] = new (ELeave) TNotReadySenseState;
    // EStartUnitState
    iStateTable[i++] = new (ELeave) TStartUnitState;
    //EStartUnitSenseState,
    iStateTable[i++] = new (ELeave) TStartUnitSenseState;
    //EPreventAllowMediumRemovalState,
    iStateTable[i++] = new (ELeave) TPreventMediumRemovalState;
    //EPreventAllowMediumRemovalSenseState,
    iStateTable[i++] = new (ELeave) TPreventMediumRemovalSenseState;
    //EReadCapacity10State,
    iStateTable[i++] = new (ELeave) TReadCapacity10State;
    //EModeSense10State,
    iStateTable[i++] = new (ELeave) TModeSense10State;
    //EModeSense10SenseState,
    iStateTable[i++] = new (ELeave) TModeSense10SenseState;
    //EModeSense6State,
    iStateTable[i++] = new (ELeave) TModeSense6State;
    //EModeSense6SenseState,
    iStateTable[i++] = new (ELeave) TModeSense6SenseState;
    //EConnectedState,
    iStateTable[i++] = new (ELeave) TConnectedState;
    //EStatusCheck
    iStateTable[i++] = new (ELeave) TStatusCheckState;
    //EAllowMediumRemovalState,
    iStateTable[i++] = new (ELeave) TAllowMediumRemovalState;
    //EStopUnitState,
    iStateTable[i++] = new (ELeave) TStopUnitState;
    //ESenseState,
    iStateTable[i++] = new (ELeave) TSenseState;

#ifdef __DEBUG
    // verify state table
    for (TInt i = 0; i < iStateTable.Count(), i++)
        {
        __ASSERT_DEBUG(i == iStateTable[i].iStateId, User::Invariant());
        }
#endif

    SetState(TMassStorageState::EInquiryState);
    }


CMassStorageFsm::CMassStorageFsm(CScsiProtocol& aScsiProtocol)
:   TMassStorage(aScsiProtocol),
    iStartStopUnitRequired(EFalse)
    {
	__MSFNLOG
    }

CMassStorageFsm::~CMassStorageFsm()
    {
	__MSFNLOG

    for (TInt i = 0; i < iStateTable.Count(); i++)
        {
        delete iStateTable[i];
        }
    }


TMassStorage::TEvent CMassStorageFsm::EntryL()
    {
	__MSFNLOG

    return iState->EntryL(*this);
    }


/**
Run FSM to connect device.

@return TInt KErrCompletion if successful otherwise KErrNotSupported
*/
TInt CMassStorageFsm::ConnectLogicalUnitL()
    {
	__MSFNLOG
    __HOSTPRINT(_L("CMassStorageFsm::ConnectLogicalUnitL()"));        
    TInt err = KErrNone;
    for (;;)
         {
         err = ProcessStateL();
         if (err)
             break;
         }
    return err;
    }


/**
Run FSM to disconnect the device.

@return TInt KErrCompletion if successful otherwise KErrNotSupported
*/
TInt CMassStorageFsm::DisconnectLogicalUnitL()
    {
	__MSFNLOG
    __HOSTPRINT(_L("CMassStorageFsm::DisconnectLogicalUnitL()"));
    TInt err = KErrNone;
    for (;;)
         {
         TInt err = ProcessStateL();
         if (err)
             break;
         }
    return err;
    }


/**
Return current FSM state.

@return TBool ETrue if FSM state is Connected State
*/
TBool CMassStorageFsm::IsConnected() const
    {
	__MSFNSLOG
    return iState->iStateId == TMassStorageState::EConnectedState ? ETrue : EFalse;
    }


TInt CMassStorageFsm::ProcessStateL()
    {
	__MSFNLOG
    TMassStorage::TEvent event = TMassStorage::EEvCommandFailed;
    TRAPD(err,  event = EntryL());
    if (err == KErrNotSupported)
        {
        __HOSTPRINT(_L("FSM ProcessState returning with KErrNotSupported"));
        return KErrNotSupported;
        }
    User::LeaveIfError(err);

    __HOSTPRINT1(_L("FSM event=%d"), event);
    switch (event)
        {
    case TMassStorage::EEvCommandPassed:
        err = ScsiCommandPassed();
        break;
    case TMassStorage::EEvCommandFailed:
        err = ScsiCommandFailed();
        break;
    case TMassStorage::EEvCommandError:
        err = ScsiCommandError();
        break;
    default:
        User::Panic(KUsbMsHostPanicCat, EMsFsmEvent);
        break;
        }

    __HOSTPRINT1(_L("FSM ProcessState completed=%d"), err);
    return err;
    }


TInt CMassStorageFsm::ScsiCommandPassed()
    {
	__MSFNLOG
    return iState->ScsiCommandPassed(*this);
    }


TInt CMassStorageFsm::ScsiCommandFailed()
    {
	__MSFNLOG
    return iState->ScsiCommandFailed(*this);
    }


TInt CMassStorageFsm::ScsiCommandError()
    {
	__MSFNLOG
    return iState->ScsiCommandError(*this);
    }


/**
Constructor

@param aStateId The state id of the state implementation
*/
TMassStorageState::TMassStorageState(TStateId aStateId)
:   iStateId(aStateId)
    {
	__MSFNLOG
    }


/** 
   Default state does nothing. Used by states where the stalled event is not 
   applicable 
   
   @param aFsm
   
   @return TInt
 */
TInt TMassStorageState::ScsiCommandError(CMassStorageFsm& /* aFsm */)
    {
    return KErrNone;
    }


TInt TMassStorageState::SenseError(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    TInt ret = KErrNone;
    const TSenseInfo& senseInfo = aFsm.MsSenseInfo();

    if (senseInfo.iSenseCode == TSenseInfo::EUnitAttention)
        {
        aFsm.SetState(TMassStorageState::EInquiryState);
        }
    else
        {
        aFsm.SetState(TMassStorageState::ENotReadyState);
        ret = KErrCompletion;
        }
    return ret;
    }


/**

*/
TInquiryState::TInquiryState()
:   TMassStorageState(EInquiryState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TInquiryState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.InquiryL();
    };


TInt TInquiryState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(ENotReadyState);
    return KErrNone;
    };


TInt TInquiryState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(EInquirySenseState);
    return KErrNone;
    }


/**

*/
TInquirySenseState::TInquirySenseState()
:   TMassStorageState(EInquirySenseState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TInquirySenseState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.SenseL();
    };


TInt TInquirySenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    // SENSE ERROR
    aFsm.SetState(TMassStorageState::EInquiryState);
    return KErrCompletion;
    };


TInt TInquirySenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::EInquiryState);
    return KErrCompletion;
    }


/**

*/
TNotReadyState::TNotReadyState()
:   TMassStorageState(ENotReadyState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TNotReadyState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.TestUnitReadyL();
    }


TInt TNotReadyState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    if (aFsm.IsSbcSet())
        {
        if (aFsm.IsRemovableMedia())
            aFsm.SetState(TMassStorageState::EPreventRemovalState);
        else
            aFsm.SetState(TMassStorageState::EReadCapacityState);
        }
    else
        {
        __HOSTPRINT(_L("SBC is not set !!"));
        aFsm.SetState(TMassStorageState::EReadCapacityState);
        }
        
    return KErrNone;
    }


TInt TNotReadyState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::ENotReadySenseState);
    return KErrNone;
    }


/**

*/
TNotReadySenseState::TNotReadySenseState()
:   TMassStorageState(ENotReadySenseState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TNotReadySenseState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.SenseL();
    }


TInt TNotReadySenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    const TSenseInfo& senseInfo = aFsm.MsSenseInfo();
    TInt ret = KErrNone;

    if (senseInfo.iSenseCode == TSenseInfo::ENotReady &&
        senseInfo.iAdditional == TSenseInfo::EAscLogicalUnitNotReady &&
        senseInfo.iQualifier == TSenseInfo::EAscqInitializingCommandRequired)
        {
        aFsm.SetStartStopUnitRequired(ETrue);
        aFsm.SetState(TMassStorageState::EStartUnitState);
        }
    else
        {
        ret = TMassStorageState::SenseError(aFsm);
        }

    return ret;
    }


TInt TNotReadySenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return TMassStorageState::SenseError(aFsm);
    }


/**

*/
TStartUnitState::TStartUnitState()
:   TMassStorageState(EPreventRemovalState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TStartUnitState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.StartStopUnitL(ETrue);
    }


TInt TStartUnitState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    if (aFsm.IsRemovableMedia())
        aFsm.SetState(TMassStorageState::EPreventRemovalState);
    else
        aFsm.SetState(TMassStorageState::EReadCapacityState);
    return KErrNone;
    }


TInt TStartUnitState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::EPreventRemovalSenseState);
    return KErrNone;
    }

/**

*/
TStartUnitSenseState::TStartUnitSenseState()
:   TMassStorageState(EPreventRemovalSenseState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TStartUnitSenseState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.SenseL();
    }


TInt TStartUnitSenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    if (aFsm.IsRemovableMedia())
        aFsm.SetState(TMassStorageState::EPreventRemovalState);
    else
        aFsm.SetState(TMassStorageState::EReadCapacityState);

    return KErrNone;
    }


TInt TStartUnitSenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    TInt ret = KErrCompletion;
    const TSenseInfo& senseInfo = aFsm.MsSenseInfo();

    aFsm.SetState(TMassStorageState::EInquiryState);
    if (senseInfo.iSenseCode == TSenseInfo::EIllegalRequest)
        {
        aFsm.SetState(TMassStorageState::EReadCapacityState);
        ret = KErrNone;
        }
    else
        {
        ret = TMassStorageState::SenseError(aFsm);
        }

    return ret;
    }


/**

*/
TPreventMediumRemovalState::TPreventMediumRemovalState()
:   TMassStorageState(EPreventRemovalState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TPreventMediumRemovalState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.PreventAllowMediumRemovalL(ETrue);
    }


TInt TPreventMediumRemovalState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::EReadCapacityState);
    return KErrNone;
    }


TInt TPreventMediumRemovalState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::EPreventRemovalSenseState);
    return KErrNone;
    }

/**

*/
TPreventMediumRemovalSenseState::TPreventMediumRemovalSenseState()
:   TMassStorageState(EPreventRemovalSenseState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TPreventMediumRemovalSenseState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.SenseL();
    }


TInt TPreventMediumRemovalSenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::EReadCapacityState);
    return KErrNone;
    }




TInt TPreventMediumRemovalSenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    TInt ret = KErrCompletion;
    const TSenseInfo& senseInfo = aFsm.MsSenseInfo();

    if (senseInfo.iSenseCode == TSenseInfo::EIllegalRequest)
        {
        aFsm.SetState(TMassStorageState::EReadCapacityState);
        ret = KErrNone;
        }
    else
        {
        ret = TMassStorageState::SenseError(aFsm);
        }
    return ret;
    }


/**

*/
TReadCapacity10State::TReadCapacity10State()
:   TMassStorageState(EReadCapacityState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TReadCapacity10State::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.ReadCapacityL();
    };


TInt TReadCapacity10State::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(EModeSense10State);
    return KErrNone;
    };


TInt TReadCapacity10State::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::ESenseState);
    return KErrCompletion;
    }


TInt TReadCapacity10State::ScsiCommandError(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::ENotReadyState);
    return KErrCompletion;
    }


/**

*/
TModeSense10State::TModeSense10State()
:   TMassStorageState(EModeSense10State)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TModeSense10State::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.ModeSense10L();
    };


TInt TModeSense10State::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(EConnectedState);
    return KErrCompletion;
    };


TInt TModeSense10State::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(EModeSense10SenseState);
    return KErrNone;
    }


TInt TModeSense10State::ScsiCommandError(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(EModeSense6State);
    return KErrNone;
    }

/**

*/
TModeSense10SenseState::TModeSense10SenseState()
:   TMassStorageState(EModeSense10SenseState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TModeSense10SenseState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.SenseL();
    };


TInt TModeSense10SenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    TInt ret = KErrCompletion;
    const TSenseInfo& senseInfo = aFsm.MsSenseInfo();

    aFsm.SetState(TMassStorageState::EInquiryState);
    if (senseInfo.iSenseCode == TSenseInfo::EIllegalRequest)
        {
        aFsm.SetState(TMassStorageState::EModeSense6State);
        ret = KErrNone;
        }
    else
        {
        ret = TMassStorageState::SenseError(aFsm);
        }
    return ret;
    };


TInt TModeSense10SenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(EInquirySenseState);
    return KErrCompletion;
    }

/**

*/
TModeSense6State::TModeSense6State()
:   TMassStorageState(EModeSense6State)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TModeSense6State::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.ModeSense6L();
    };


TInt TModeSense6State::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(EConnectedState);
    return KErrCompletion;
    };


TInt TModeSense6State::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(EModeSense6SenseState);
    return KErrNone;
    }


TInt TModeSense6State::ScsiCommandError(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    // If device responds with protocol error, ignore the error and assume the
    // device is not write protected
    aFsm.SetState(EConnectedState);
    return KErrCompletion;
    }


/**

*/
TModeSense6SenseState::TModeSense6SenseState()
:   TMassStorageState(EModeSense6SenseState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TModeSense6SenseState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.SenseL();
    };


TInt TModeSense6SenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    TInt ret = KErrCompletion;
    const TSenseInfo& senseInfo = aFsm.MsSenseInfo();

    aFsm.SetState(TMassStorageState::EInquiryState);
    if (senseInfo.iSenseCode == TSenseInfo::EIllegalRequest)
        {
        aFsm.SetState(TMassStorageState::EConnectedState);
        }
    else
        {
        ret = TMassStorageState::SenseError(aFsm);
        }
    return ret;
    };


TInt TModeSense6SenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(EInquirySenseState);
    return KErrCompletion;
    }



/**

*/
TConnectedState::TConnectedState()
:   TMassStorageState(EConnectedState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TConnectedState::EntryL(CMassStorageFsm& /* aFsm */)
    {
	__MSFNLOG
    return TMassStorage::EEvCommandPassed;
    };


TInt TConnectedState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    TInt ret = KErrNone;

    if (aFsm.IsRemovableMedia())
        {
		if(aFsm.IsStatusCheck())
			{
			aFsm.SetState(TMassStorageState::EStatusCheckState);
			}
		else
            {
            aFsm.SetState(TMassStorageState::EAllowRemovalState);
            }
        }
    else if (aFsm.StartStopUnitRequired())
        {
        aFsm.SetState(TMassStorageState::EStopUnitState);
        }
    else
        {
		aFsm.SetState(TMassStorageState::ENotReadyState);
		ret = KErrCompletion;
        }
    return ret;
    };


TInt TConnectedState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::EInquiryState);
    return KErrCompletion;
    }


/**

*/
TStatusCheckState::TStatusCheckState()
:   TMassStorageState(EStatusCheckState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TStatusCheckState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.TestUnitReadyL();
    };


TInt TStatusCheckState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(EConnectedState);
    return KErrCompletion;
    };


TInt TStatusCheckState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
	aFsm.SetState(ESenseState);
    return KErrNone;
    }


/**

*/
TAllowMediumRemovalState::TAllowMediumRemovalState()
:   TMassStorageState(EAllowRemovalState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TAllowMediumRemovalState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.PreventAllowMediumRemovalL(EFalse);
    }


TInt TAllowMediumRemovalState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    TInt ret = KErrNone;
    if (aFsm.StartStopUnitRequired())
        {
        aFsm.SetState(TMassStorageState::EStopUnitState);
        }
    else
        {
        aFsm.SetState(ENotReadyState);
        ret = KErrCompletion;
        }
    return ret;
    };


TInt TAllowMediumRemovalState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::EInquiryState);
    return KErrCompletion;
    }


/**

*/
TStopUnitState::TStopUnitState()
:   TMassStorageState(EStopUnitState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TStopUnitState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.StartStopUnitL(EFalse);
    }


TInt TStopUnitState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(ENotReadyState);
    return KErrCompletion;
    };


TInt TStopUnitState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(TMassStorageState::EInquiryState);
    return KErrCompletion;
    }


/**

*/
TSenseState::TSenseState()
:   TMassStorageState(EConnectedState)
    {
	__MSFNLOG
    }


TMassStorage::TEvent TSenseState::EntryL(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    return aFsm.SenseL();
    };


TInt TSenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    aFsm.SetState(ENotReadyState);
    return KErrCompletion;
    };


TInt TSenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
	__MSFNLOG
    // This event should not happen
    aFsm.SetState(EInquiryState);
    return KErrCompletion;
    }

