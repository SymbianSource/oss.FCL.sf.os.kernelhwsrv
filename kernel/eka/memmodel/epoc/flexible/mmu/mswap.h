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
 @internalComponent
*/

#ifndef MSWAP_H
#define MSWAP_H

extern void GetSwapInfo(SVMSwapInfo& aInfoOut);
extern TInt SetSwapThresholds(const SVMSwapThresholds& aThresholds);
extern TBool GetPhysicalAccessSupported();
extern TBool GetUsePhysicalAccess();
extern void SetUsePhysicalAccess(TBool aUsePhysicalAccess);
extern TUint GetPreferredDataWriteSize();
extern TInt SetDataWriteSize(TUint aWriteShift);

#endif
