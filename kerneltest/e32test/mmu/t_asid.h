// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_asid.cpp
// 
//
#include <e32std.h>

_LIT(KAsidIpcServerName, "CAsidIpcServer");
_LIT(KAsidDesServerName, "CAsidDesServer");

const TUint KAsidValue = 123;
const TInt KAsidDesLen = 100;

enum TAsidMsgType
	{
	EConnect = -1,
	EDisConnect = -2,
	EIpcData = 0,		// 1 argument message, 1st arg = descriptor to be read.
	EDesData = 1,		// 2 argument, 1st arg = pointer to the descriptor to be read, 2nd arg = DThread* that owns descriptor.
	EMsgCount = 3
	};
