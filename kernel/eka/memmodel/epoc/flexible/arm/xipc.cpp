// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifdef _DEBUG

#include "arm_mem.h"
#include "mpager.h"

TInt DoThreadReadAndParseDesHeader(DThread* aThread, const TAny* aSrc, TDesHeader& aDest);

TInt DThread::ReadAndParseDesHeader(const TAny* aSrc, TDesHeader& aDest)
	{
	if (KDebugNum(KFORCEKUPAGEFAULTS))
		{
		TInt r = ThePager.FlushRegion((DMemModelProcess*)iOwningProcess,
									  (TLinAddr)aSrc, sizeof(TDesHeader));
		(void)r; // ignore errors
		}
	return DoThreadReadAndParseDesHeader(this, aSrc, aDest);
	}

#endif
