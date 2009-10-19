// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\direct\x86\xutils.cpp
// 
//

//#define __DBG_MON_FAULT__
//#define __RAM_LOADED_CODE__
//#define __EARLY_DEBUG__

#include <x86_mem.h>

EXPORT_C TPhysAddr Epoc::LinearToPhysical(TLinAddr aLinAddr)
	{
	return aLinAddr;
	}

