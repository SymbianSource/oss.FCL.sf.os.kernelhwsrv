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
*/

#ifndef CMASSSTORAGEFSM_H
#define CMASSSTORAGEFSM_H

class CScsiProtocol;

/**
Wrapper for Mass Storage operations. Each action converts the response into a
TEvent value for use by the CMassStorageStateMachine.
*/
class TMassStorage: public CBase
    {
public:
    TMassStorage(CScsiProtocol& aScsiProtocol);

    /** FSM Events */
    enum TEvent
        {
        EEvCommandPassed,
        EEvCommandFailed,   // transport status error
        EEvCommandError     // stall or protocol error
        };

    // SCSI Commands
    TEvent InquiryL();
    TEvent SenseL();
    TEvent TestUnitReadyL();
    TEvent StartStopUnitL(TBool aStart);
    TEvent PreventAllowMediumRemovalL(TBool aPrevent);
    TEvent ReadCapacityL();
    TEvent ModeSense10L();
    TEvent ModeSense6L();

    // SCSI state
    TBool IsSbcSet() const;
    TBool IsRemovableMedia() const;
    const TSenseInfo& MsSenseInfo() const;

private:
    TEvent ErrToEvent(TInt err) const;

private:
    /** Reference to the actual interface */
    CScsiProtocol& iScsiProtocol;
    };


inline TMassStorage::TEvent TMassStorage::ErrToEvent(TInt err) const
    {
    if (err == KErrNone)
        return EEvCommandPassed;
    else if (err == KErrCommandFailed)
        return EEvCommandFailed;
    else // (KErrGeneral, KErrCommandStalled)
        return EEvCommandError;
    }


class CMassStorageFsm;


/**
Abstract class to represent state object. Each state object defines action to do
on entry and the next state to move to.
*/
class TMassStorageState
    {
public:
    friend class CMassStorageFsm;

    /** State ID */
    enum TStateId
        {
        // INQUIRY
        EInquiryState = 0,
        EInquirySenseState = 1,

        // TEST UNIT READY
        ENotReadyState = 2,
        ENotReadySenseState = 3,

        // START STOP UNIT
        EStartUnitState = 4,
        EStartUnitSenseState = 5,

        // PREVENT ALLOW MEDIUM REMOVAL
        EPreventRemovalState = 6,
        EPreventRemovalSenseState = 7,

        // READ CAPACITY 10
        EReadCapacityState = 8,

        // MODE SENSE 10
        EModeSense10State = 9,
        EModeSense10SenseState = 10,

        // MODE SENSE 6
        EModeSense6State = 11,
        EModeSense6SenseState = 12,

        // CONNECTED
        EConnectedState = 13,

        // STATUS CHECK
        EStatusCheckState = 14,

        // PREVENT ALLOW MEDIUM REMOVAL
        EAllowRemovalState = 15,

        // STOP UNIT
        EStopUnitState = 16,

        // REQUEST SENSE (common sense error handler)
        ESenseState = 17,

        //
        ENumberOfStates
        };

public:
    TMassStorageState(TStateId aStateId);

    /** State Entry */
    virtual TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm) = 0;

    /** Maps PASSED Event to next state */
    virtual TInt ScsiCommandPassed(CMassStorageFsm& aFsm) = 0;
    /** Maps FAILED event to next state */
    virtual TInt ScsiCommandFailed(CMassStorageFsm& aFsm) = 0;
    /** Maps ERROR event to next state */
    virtual TInt ScsiCommandError(CMassStorageFsm& aFsm);

protected:
    /** General event handler for a sense error event */
    TInt SenseError(CMassStorageFsm& aFsm);

private:
    /** State ID for state implentation */
    const TStateId iStateId;
    };



/**
State Machine to perform Mass STorage Connection and Disconnection
*/
class CMassStorageFsm: public TMassStorage
    {
public:
	static CMassStorageFsm* NewL(CScsiProtocol& aScsiProtocol);
	~CMassStorageFsm();

private:
    void ConstructL();
	CMassStorageFsm(CScsiProtocol& aScsiProtocol);

public:
    TInt ConnectLogicalUnitL();
    TInt DisconnectLogicalUnitL();

    TBool IsConnected() const;
	void  ClearStatusCheck();
	TBool IsStatusCheck() const;
	void  SetStatusCheck();

    void SetState(TMassStorageState::TStateId aStateId);

    void SetStartStopUnitRequired(TBool aRequired);
    TBool StartStopUnitRequired() const;

private:
    TInt ProcessStateL();


    // Event handlers
    TInt ScsiCommandPassed();
    TInt ScsiCommandFailed();
    TInt ScsiCommandError();

private:
    TMassStorage::TEvent EntryL();

private:
    TMassStorageState* iState;

    TFixedArray<TMassStorageState*, TMassStorageState::ENumberOfStates> iStateTable;

    TBool iStartStopUnitRequired;
	TBool iStatusCheck;
    };

/**
State that executes INQUIRY
*/
class TInquiryState: public TMassStorageState
    {
public:
    TInquiryState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);

    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };

/**
State that execute REQUEST SENSE after TInquiryState
*/
class TInquirySenseState: public TMassStorageState
    {
public:
    TInquirySenseState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };

/**
State that executes TEST UNIT READY
*/
class TNotReadyState: public TMassStorageState
    {
public:
    TNotReadyState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };


/**
State that executes REQUEST SENSE after TNotReadyState
*/
class TNotReadySenseState: public TMassStorageState
    {
public:
    TNotReadySenseState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };


/**
State that executes START STOP UNIT (start)
*/
class TStartUnitState: public TMassStorageState
    {
public:
    TStartUnitState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };

/**
State that executes REQUEST SENSE after TStartUnitState
*/
class TStartUnitSenseState: public TMassStorageState
    {
public:
    TStartUnitSenseState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };


/**
State that executes PREVENT MEDIA REMOVAL
*/
class TPreventMediumRemovalState: public TMassStorageState
    {
public:
    TPreventMediumRemovalState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };


/**
State that executes REQUEST SENSE from TPreventAllowMediumRemovalState
*/
class TPreventMediumRemovalSenseState: public TMassStorageState
    {
public:
    TPreventMediumRemovalSenseState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };


/**
State that execute READ CAPACITY (10) state
*/
class TReadCapacity10State: public TMassStorageState
    {
public:
    TReadCapacity10State();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    TInt ScsiCommandError(CMassStorageFsm& aFsm);
    };


/**
State that executes MODE SENSE (10) state
*/
class TModeSense10State: public TMassStorageState
    {
public:
    TModeSense10State();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    TInt ScsiCommandError(CMassStorageFsm& aFsm);
    };

/**
State that executes REQUEST SENSE from TModeSense10State
*/
class TModeSense10SenseState: public TMassStorageState
    {
public:
    TModeSense10SenseState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };

/**
State that executes MODE SENSE (6) state
*/
class TModeSense6State: public TMassStorageState
    {
public:
    TModeSense6State();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    TInt ScsiCommandError(CMassStorageFsm& aFsm);
    };


/**
State that executes REQUEST SENSE from TModeSense6State
*/
class TModeSense6SenseState: public TMassStorageState
    {
public:
    TModeSense6SenseState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };


/**
State that begins move from connected stae back to disconnected state
*/
class TConnectedState: public TMassStorageState
    {
public:
    TConnectedState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };

/**
State that executes TEST UNIT READY
*/
class TStatusCheckState: public TMassStorageState
    {
public:
    TStatusCheckState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };

/**
State that executes PREVENT ALLOW MEDIA REMOVAL
*/
class TAllowMediumRemovalState: public TMassStorageState
    {
public:
    TAllowMediumRemovalState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };


/**
State that executes START STOP UNIT
*/
class TStopUnitState: public TMassStorageState
    {
public:
    TStopUnitState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };

/**
State that executes REQUEST SENSE
*/
class TSenseState: public TMassStorageState
    {
public:
    TSenseState();
    TMassStorage::TEvent EntryL(CMassStorageFsm& aFsm);
    TInt ScsiCommandPassed(CMassStorageFsm& aFsm);
    TInt ScsiCommandFailed(CMassStorageFsm& aFsm);
    };


#include "cmassstoragefsm.inl"

#endif // CMASSSTORAGEFSM_H
