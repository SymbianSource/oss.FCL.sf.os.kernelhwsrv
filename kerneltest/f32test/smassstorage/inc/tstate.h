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
//

#ifndef __TSTATE_H__
#define __TSTATE_H__

#include "usbmsshared.h"
#include "cpropertywatch.h"

class TState
{
public:
    TState(int id) : stateId(id) {}
    virtual ~TState() {}
    
    virtual void MoveTo(int aStateId) const = 0;
    TInt GetStateId() const {return stateId;}

private:
    const TInt stateId;
};

//////////////////////////////////////////////////////////////

class TDisconnected : public TState
{
public:
    TDisconnected() : TState(EUsbMsDriveState_Disconnected){}
    ~TDisconnected(){}

    void MoveTo(TInt aStateId) const;

private:
    void MoveToConnecting() const;
    void MoveToConnected() const;
};

//////////////////////////////////////////////////////////////

class TConnecting : public TState
{
public:
    TConnecting() : TState(EUsbMsDriveState_Connecting){}
    ~TConnecting(){}

    void MoveTo(TInt aStateId) const;

private:
    void MoveToWritten() const;
};

//////////////////////////////////////////////////////////////

class TConnected : public TState
{
public:
    TConnected() : TState(EUsbMsDriveState_Connected){}
    ~TConnected(){}

    void MoveTo(TInt aStateId) const;

private:
    void MoveToActive() const;
};

//////////////////////////////////////////////////////////////

class TDisconnecting : public TState
{
public:
    TDisconnecting() : TState(EUsbMsDriveState_Disconnecting){}
    ~TDisconnecting(){}

    void MoveTo(TInt aStateId) const;

private:
    void MoveToDisconnected() const;
};

//////////////////////////////////////////////////////////////

class TActive : public TState
{
public:
    TActive() : TState(EUsbMsDriveState_Active){}
    ~TActive(){}

    void MoveTo(TInt aStateId) const;
    
private:
    void MoveToLocked() const;
    void MoveToDisconnecting() const;
};

//////////////////////////////////////////////////////////////

class TLocked : public TState
{
public:
    TLocked() : TState(EUsbMsDriveState_Locked){}
    ~TLocked(){}

    void MoveTo(TInt aStateId) const;

private:
    void MoveToDisconnecting() const;
};

//////////////////////////////////////////////////////////////

class TWritten : public TState
{
public:
    TWritten() : TState(EUsbMsState_Written){}
    ~TWritten(){}

    void MoveTo(TInt aStateId) const;

private:
    void MoveToRead() const;
};

//////////////////////////////////////////////////////////////

class TRead : public TState
{
public:
    TRead() : TState(EUsbMsState_Read){}
    ~TRead(){}

    void MoveTo(TInt aStateId) const;

private:
    void MoveToDisconnected() const;
};

#endif // __TSTATE_H__    
   


