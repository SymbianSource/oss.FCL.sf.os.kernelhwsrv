/*
* Copyright (c) 2006 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


/**
@file
@internalComponent

Definition of a private extension classes owned by the MMC stack
*/

NONSHARABLE_CLASS(DMMCStack::DBody) : public DBase
	{
public:
	DBody(DMMCStack& aStack);

	// Methods to temporarily increase the idle timeout
	void SetInactivityTimeout(TInt aInactivityTimeout);
	void RestoreInactivityTimeout();

private:
	DMMCStack& iStack;

	TInt iInactivityTimeout;		// Copy of DPBusPsuBase::iInactivityTimeout. Used by SetInactivityTimeout()
	TInt iVccCoreSleepTimeout;         // Copy of DPBusPsuBase::iInactivityTimeout. for iVccCore Psu Sleep timer
	TInt iInactivityLock;			// incremented whenever SetInactivityTimeout() is called, decremented by RestoreInactivityTimeout()
    TUint32 iCurrentSelectedBusWidth;	// The currently set bus width
    TUint32 iCurrentSelectedClock;		// The currently set clock rate in KiloHertz

friend class DMMCStack;
	};
