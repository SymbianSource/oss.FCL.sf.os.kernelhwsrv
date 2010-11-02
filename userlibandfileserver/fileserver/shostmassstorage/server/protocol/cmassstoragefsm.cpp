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

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cmassstoragefsmTraces.h"
#endif

#include "usbmshostpanic.h"
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
    }


/**
Clear StatusCheck flag. Periodic status checking is used for removable media.
*/
void CMassStorageFsm::ClearStatusCheck()
{
    iStatusCheck = EFalse;
}


/**
Set StatusCheck flag. Periodic status checking is used for removable media.
*/
void CMassStorageFsm::SetStatusCheck()
{
    iStatusCheck = ETrue;
}


/**
Get the boolean state of StatusCheck flag.

@return TBool ETrue if StatusCheck flag is set
*/
TBool CMassStorageFsm::IsStatusCheck() const
{
    return iStatusCheck;
}


/**
Device supports SCSI BLOCK COMMANDS.

@return TBool Returns ETrue if the SBC interface is initialised
*/
TBool TMassStorage::IsSbcSet() const
    {
    return iScsiProtocol.MsIsSbcSet();
    }

/**
Device is removable media

@return TBool Returns ETrue if removable
*/
TBool TMassStorage::IsRemovableMedia() const
    {
    return iScsiProtocol.MsIsRemovableMedia();
    }

/**
Retrieve the sense info returned by a call to SenseL

@return const TSenseInfo& Returns the SCSI SENSE info
*/
const TSenseInfo& TMassStorage::MsSenseInfo() const
    {
    return iScsiProtocol.MsSenseInfo();
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::InquiryL()
    {
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
        OstTrace0(TRACE_SHOSTMASSSTORAGE_SCSI, CMASSSTORAGEFSM_10,
                  "INQUIRY Command returned NOT SUPPORTED");
        User::Leave(KErrNotSupported);
        break;
    default:
        OstTrace0(TRACE_SHOSTMASSSTORAGE_SCSI, CMASSSTORAGEFSM_11,
                  "INQUIRY Command returned KErrUnknown");
        User::Leave(KErrUnknown);
        break;
        }
    return event;
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::SenseL()
    {
    TEvent event = EEvCommandPassed;
    switch (iScsiProtocol.MsRequestSenseL())
        {
    case KErrNone:
        break;
    case KErrCommandFailed:
        OstTrace0(TRACE_SHOSTMASSSTORAGE_SCSI, CMASSSTORAGEFSM_12,
                  "REQUEST SENSE Command Failed");
        User::Leave(KErrNotSupported);
        break;
    default:
        OstTrace0(TRACE_SHOSTMASSSTORAGE_SCSI, CMASSSTORAGEFSM_13,
                  "INQUIRY Command returned KErrUnknown");
        User::Leave(KErrUnknown);
        break;
        }
    return event;
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::TestUnitReadyL()
    {
    // KErrCommandFailed indictates that device is NOT READY
    TInt err = iScsiProtocol.MsTestUnitReadyL();
    return (err == KErrNone) ? EEvCommandPassed : EEvCommandFailed;
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::StartStopUnitL(TBool aStart)
    {
    TInt err = iScsiProtocol.MsStartStopUnitL(aStart);
    return (err == KErrNone) ? EEvCommandPassed : EEvCommandFailed;
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::PreventAllowMediumRemovalL(TBool aPrevent)
    {
    // KErrCommandFailed indictates that device is NOT READY
    TInt err = iScsiProtocol.MsPreventAllowMediaRemovalL(aPrevent);
    return (err == KErrNone) ? EEvCommandPassed : EEvCommandFailed;
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::ReadCapacityL()
    {
    TInt err = iScsiProtocol.MsReadCapacityL();
    return ErrToEvent(err);
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::ModeSense10L()
    {
    TInt err = iScsiProtocol.MsModeSense10L();
    return ErrToEvent(err);
    }


/** SCSI Request */
TMassStorage::TEvent TMassStorage::ModeSense6L()
    {
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
    CMassStorageFsm* r = new (ELeave) CMassStorageFsm(aScsiProtocol);

    CleanupStack::PushL(r);
    r->ConstructL();
    CleanupStack::Pop();
    return r;
}

void CMassStorageFsm::ConstructL()
    {
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
    }

CMassStorageFsm::~CMassStorageFsm()
    {
    for (TInt i = 0; i < iStateTable.Count(); i++)
        {
        delete iStateTable[i];
        }
    }


TMassStorage::TEvent CMassStorageFsm::EntryL()
    {
    return iState->EntryL(*this);
    }


/**
Run FSM to connect device.

@return TInt KErrCompletion if successful otherwise KErrNotSupported
*/
TInt CMassStorageFsm::ConnectLogicalUnitL()
    {
    OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CMASSSTORAGEFSM_14,
              "Connect Logical Unit");
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
    OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CMASSSTORAGEFSM_15,
              "Disconnect Logical Unit");
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
    return iState->iStateId == TMassStorageState::EConnectedState ? ETrue : EFalse;
    }


TInt CMassStorageFsm::ProcessStateL()
    {
    TMassStorage::TEvent event = TMassStorage::EEvCommandFailed;
    TRAPD(err,  event = EntryL());
    if (err == KErrNotSupported)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CMASSSTORAGEFSM_16,
                  "FSM ProcessState returning with KErrNotSupported");
        return KErrNotSupported;
        }
    User::LeaveIfError(err);

    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, CMASSSTORAGEFSM_17,
              "FSM event=%d", event);
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

    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, CMASSSTORAGEFSM_18,
              "FSM ProcessState completed=%d", err);
    return err;
    }


TInt CMassStorageFsm::ScsiCommandPassed()
    {
    return iState->ScsiCommandPassed(*this);
    }


TInt CMassStorageFsm::ScsiCommandFailed()
    {
    return iState->ScsiCommandFailed(*this);
    }


TInt CMassStorageFsm::ScsiCommandError()
    {
    return iState->ScsiCommandError(*this);
    }


/**
Constructor

@param aStateId The state id of the state implementation
*/
TMassStorageState::TMassStorageState(TStateId aStateId)
:   iStateId(aStateId)
    {
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
    }


TMassStorage::TEvent TInquiryState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.InquiryL();
    };


TInt TInquiryState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(ENotReadyState);
    return KErrNone;
    };


TInt TInquiryState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(EInquirySenseState);
    return KErrNone;
    }


/**

*/
TInquirySenseState::TInquirySenseState()
:   TMassStorageState(EInquirySenseState)
    {
    }


TMassStorage::TEvent TInquirySenseState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.SenseL();
    };


TInt TInquirySenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    // SENSE ERROR
    aFsm.SetState(TMassStorageState::EInquiryState);
    return KErrCompletion;
    };


TInt TInquirySenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(TMassStorageState::EInquiryState);
    return KErrCompletion;
    }


/**

*/
TNotReadyState::TNotReadyState()
:   TMassStorageState(ENotReadyState)
    {
    }


TMassStorage::TEvent TNotReadyState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.TestUnitReadyL();
    }


TInt TNotReadyState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    if (aFsm.IsSbcSet())
        {
        if (aFsm.IsRemovableMedia())
            aFsm.SetState(TMassStorageState::EPreventRemovalState);
        else
            aFsm.SetState(TMassStorageState::EReadCapacityState);
        }
    else
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CMASSSTORAGEFSM_19,
                  "SBC is not set !!");
        aFsm.SetState(TMassStorageState::EReadCapacityState);
        }

    return KErrNone;
    }


TInt TNotReadyState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(TMassStorageState::ENotReadySenseState);
    return KErrNone;
    }


/**

*/
TNotReadySenseState::TNotReadySenseState()
:   TMassStorageState(ENotReadySenseState)
    {
    }


TMassStorage::TEvent TNotReadySenseState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.SenseL();
    }


TInt TNotReadySenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
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
    return TMassStorageState::SenseError(aFsm);
    }


/**

*/
TStartUnitState::TStartUnitState()
:   TMassStorageState(EPreventRemovalState)
    {
    }


TMassStorage::TEvent TStartUnitState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.StartStopUnitL(ETrue);
    }


TInt TStartUnitState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    if (aFsm.IsRemovableMedia())
        aFsm.SetState(TMassStorageState::EPreventRemovalState);
    else
        aFsm.SetState(TMassStorageState::EReadCapacityState);
    return KErrNone;
    }


TInt TStartUnitState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(TMassStorageState::EPreventRemovalSenseState);
    return KErrNone;
    }

/**

*/
TStartUnitSenseState::TStartUnitSenseState()
:   TMassStorageState(EPreventRemovalSenseState)
    {
    }


TMassStorage::TEvent TStartUnitSenseState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.SenseL();
    }


TInt TStartUnitSenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    if (aFsm.IsRemovableMedia())
        aFsm.SetState(TMassStorageState::EPreventRemovalState);
    else
        aFsm.SetState(TMassStorageState::EReadCapacityState);

    return KErrNone;
    }


TInt TStartUnitSenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
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
    }


TMassStorage::TEvent TPreventMediumRemovalState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.PreventAllowMediumRemovalL(ETrue);
    }


TInt TPreventMediumRemovalState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(TMassStorageState::EReadCapacityState);
    return KErrNone;
    }


TInt TPreventMediumRemovalState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(TMassStorageState::EPreventRemovalSenseState);
    return KErrNone;
    }

/**

*/
TPreventMediumRemovalSenseState::TPreventMediumRemovalSenseState()
:   TMassStorageState(EPreventRemovalSenseState)
    {
    }


TMassStorage::TEvent TPreventMediumRemovalSenseState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.SenseL();
    }


TInt TPreventMediumRemovalSenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(TMassStorageState::EReadCapacityState);
    return KErrNone;
    }




TInt TPreventMediumRemovalSenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
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
    }


TMassStorage::TEvent TReadCapacity10State::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.ReadCapacityL();
    };


TInt TReadCapacity10State::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(EModeSense10State);
    return KErrNone;
    };


TInt TReadCapacity10State::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(TMassStorageState::ESenseState);
    return KErrCompletion;
    }


TInt TReadCapacity10State::ScsiCommandError(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(TMassStorageState::ENotReadyState);
    return KErrCompletion;
    }


/**

*/
TModeSense10State::TModeSense10State()
:   TMassStorageState(EModeSense10State)
    {
    }


TMassStorage::TEvent TModeSense10State::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.ModeSense10L();
    };


TInt TModeSense10State::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(EConnectedState);
    return KErrCompletion;
    };


TInt TModeSense10State::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(EModeSense10SenseState);
    return KErrNone;
    }


TInt TModeSense10State::ScsiCommandError(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(EModeSense6State);
    return KErrNone;
    }

/**

*/
TModeSense10SenseState::TModeSense10SenseState()
:   TMassStorageState(EModeSense10SenseState)
    {
    }


TMassStorage::TEvent TModeSense10SenseState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.SenseL();
    };


TInt TModeSense10SenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
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
    aFsm.SetState(EInquirySenseState);
    return KErrCompletion;
    }

/**

*/
TModeSense6State::TModeSense6State()
:   TMassStorageState(EModeSense6State)
    {
    }


TMassStorage::TEvent TModeSense6State::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.ModeSense6L();
    };


TInt TModeSense6State::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(EConnectedState);
    return KErrCompletion;
    };


TInt TModeSense6State::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(EModeSense6SenseState);
    return KErrNone;
    }


TInt TModeSense6State::ScsiCommandError(CMassStorageFsm& aFsm)
    {
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
    }


TMassStorage::TEvent TModeSense6SenseState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.SenseL();
    };


TInt TModeSense6SenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
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
    aFsm.SetState(EInquirySenseState);
    return KErrCompletion;
    }



/**

*/
TConnectedState::TConnectedState()
:   TMassStorageState(EConnectedState)
    {
    }


TMassStorage::TEvent TConnectedState::EntryL(CMassStorageFsm& /* aFsm */)
    {
    return TMassStorage::EEvCommandPassed;
    };


TInt TConnectedState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
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
    aFsm.SetState(TMassStorageState::EInquiryState);
    return KErrCompletion;
    }


/**

*/
TStatusCheckState::TStatusCheckState()
:   TMassStorageState(EStatusCheckState)
    {
    }


TMassStorage::TEvent TStatusCheckState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.TestUnitReadyL();
    };


TInt TStatusCheckState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(EConnectedState);
    return KErrCompletion;
    };


TInt TStatusCheckState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(ESenseState);
    return KErrNone;
    }


/**

*/
TAllowMediumRemovalState::TAllowMediumRemovalState()
:   TMassStorageState(EAllowRemovalState)
    {
    }


TMassStorage::TEvent TAllowMediumRemovalState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.PreventAllowMediumRemovalL(EFalse);
    }


TInt TAllowMediumRemovalState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
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
    aFsm.SetState(TMassStorageState::EInquiryState);
    return KErrCompletion;
    }


/**

*/
TStopUnitState::TStopUnitState()
:   TMassStorageState(EStopUnitState)
    {
    }


TMassStorage::TEvent TStopUnitState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.StartStopUnitL(EFalse);
    }


TInt TStopUnitState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(ENotReadyState);
    return KErrCompletion;
    };


TInt TStopUnitState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(TMassStorageState::EInquiryState);
    return KErrCompletion;
    }


/**

*/
TSenseState::TSenseState()
:   TMassStorageState(EConnectedState)
    {
    }


TMassStorage::TEvent TSenseState::EntryL(CMassStorageFsm& aFsm)
    {
    return aFsm.SenseL();
    };


TInt TSenseState::ScsiCommandPassed(CMassStorageFsm& aFsm)
    {
    aFsm.SetState(ENotReadyState);
    return KErrCompletion;
    };


TInt TSenseState::ScsiCommandFailed(CMassStorageFsm& aFsm)
    {
    // This event should not happen
    aFsm.SetState(EInquiryState);
    return KErrCompletion;
    }

