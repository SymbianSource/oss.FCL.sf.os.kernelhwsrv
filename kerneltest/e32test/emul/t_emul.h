// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\emul\t_emul_dll.h
// 
//

#ifndef __T_EMUL_H__
#define __T_EMUL_H__

enum TSlaveAction
	{
	ESlaveDoNothing,
	ESlaveTrapExceptionInExe,
	ESlaveTrapExceptionInLinkedDll,
	ESlaveTrapExceptionInLoadedDll,
	};

IMPORT_C void TrapExceptionInDll();

const TInt KTrapExceptionInDllOrdinal = 1;

typedef void (*TTrapExceptionInDllFunc)();

_LIT(KTEmulSlaveName, "t_emul_slave");
_LIT(KTEmulDll2Name, "t_emul_dll2");

#endif
