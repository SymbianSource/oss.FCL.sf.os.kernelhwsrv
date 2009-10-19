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

#ifndef __CSTATEMACHINE_H__
#define __CSTATEMACHINE_H__

#include <e32base.h>
#include <e32cmn.h>

// Forward declaration
class TState;

/**
 Implements a generic state machine
 */
class CStateMachine : public CBase
{
public:
    static CStateMachine* NewL(); 
    ~CStateMachine();
    void MoveTo(int aState);
    const TState* AddState(int aState);
    const TState* FindState(int aStateId) const;
    void SetInitState(int aState);
    TInt CurrentStateId() const; 
    TInt FromStateId() const;
    void SetFromStateId(TInt aStateId);
    
private:
    CStateMachine();

    // Declared but not defined to disable copy and assignment
    CStateMachine(const CStateMachine&);
    CStateMachine& operator = (const CStateMachine&);
    
    const TState* iCurrentState;
    TInt iFromStateId;
    RPointerArray<const TState> allStates;
};

#endif // __CSTATEMACHINE_H__

