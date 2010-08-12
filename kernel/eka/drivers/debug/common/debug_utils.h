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
// Purpose: Static functions for use by debug driver classes
//

/**
 * @file
 * @internalComponent
 * @released
 */

#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <kernel/kern_priv.h>

class DebugUtils
	{
	public:
		static DThread* OpenThreadHandle(const TUint64 aThreadId);
		static DProcess* OpenProcessHandle(const TUint64 aProcessId);
	};

#endif //DEBUG_UTILS_H

