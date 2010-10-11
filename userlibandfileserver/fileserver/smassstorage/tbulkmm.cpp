/*
* Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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
 @internalTechnology
*/

#include <e32base.h>
#include "tbulkmm.h"

#ifdef MSDC_MULTITHREADED    
TBulkMm::TBulkMm()
:	iSwap(ETrue)
    {
    }


void TBulkMm::GetNextTransferBuffer(TUint aLength, TPtr8& aPtr)
	{
	if (iSwap)
		{
		iDataBuf1.SetLength(aLength);
		aPtr.Set(iDataBuf1.LeftTPtr(iDataBuf1.Length()));
		iSwap = EFalse;
		}
	else
		{
		iDataBuf2.SetLength(aLength);
		aPtr.Set(iDataBuf2.LeftTPtr(iDataBuf2.Length()));
		iSwap = ETrue;
		}
	}
#endif

