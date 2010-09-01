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
//

#ifndef T_RMDEBUG_APP_H
#define T_RMDEBUG_APP_H

_LIT(KRMDebugTestApplication, "z:\\sys\\bin\\t_rmdebug_app.exe");
_LIT(KUserPanic, "UserPanic");
const TInt KUserPanicCode = 0x1234ABCD;
enum TMyPropertyKeys {EMyPropertyInteger};


// enumeration of functions to call in test debug application
enum TDebugFunctionType
	{
	EDefaultDebugFunction,
	EPrefetchAbortFunction,
	EUserPanicFunction,
	EStackOverflowFunction,
	EDataAbortFunction,
	EUndefInstructionFunction,
	EDataReadErrorFunction,
	EDataWriteErrorFunction,
	EUserExceptionFunction,
	EWaitFiveSecondsThenExit,
	ESpinForever,
	ESpinForeverWithBreakPoint,
	ENormalExit
	};

#endif //T_RMDEBUG_APP_H

