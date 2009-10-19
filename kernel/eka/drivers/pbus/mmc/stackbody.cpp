/*
* Copyright (c) 2006-2008 Nokia Corporation and/or its subsidiary(-ies).
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

#include <drivers/mmc.h>
#include "stackbody.h"

DMMCStack::DBody::DBody(DMMCStack& aStack) :
	iStack(aStack),
	iCurrentSelectedBusWidth( (TUint32)EBusWidthInvalid ) 
	{
	}

// Temporarily increase the idle timeout
void DMMCStack::DBody::SetInactivityTimeout(TInt aInactivityTimeout)
	{
	iStack.iSocket->ResetInactivity(0);
	
	// save existing value - if not already saved (!)
	if (iInactivityLock++ == 0)
		{
		iInactivityTimeout = iStack.iSocket->iVcc->iInactivityTimeout;
		if (iStack.iSocket->iVccCore)
			iVccCoreSleepTimeout = iStack.iSocket->iVccCore->iInactivityTimeout;
		}

	// set the new inactivity timeout to the maximum of the existing value and the specified value
	TInt inactivityTimeout = Max(aInactivityTimeout, iInactivityTimeout);
	iStack.iSocket->iVcc->iInactivityTimeout = inactivityTimeout;
	if (iStack.iSocket->iVccCore)
		// Sleep timeout should always be less than Vcc Inactivity timer
		iStack.iSocket->iVccCore->iInactivityTimeout = inactivityTimeout;
	}

void DMMCStack::DBody::RestoreInactivityTimeout()
	{
	iStack.iSocket->ResetInactivity(0);

	if (--iInactivityLock == 0)
		{
		iStack.iSocket->iVcc->iInactivityTimeout = iInactivityTimeout;
		if (iStack.iSocket->iVccCore)
			iStack.iSocket->iVccCore->iInactivityTimeout = iVccCoreSleepTimeout;
		}
	}

