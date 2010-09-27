// Copyright (c) 1998-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\win32\vectors.cpp
//
//

#include "nk_priv.h"

inline TInt Invoke(TLinAddr aHandler, const TInt* aArgs)
	{
	return (TExecHandler(aHandler))(aArgs[0], aArgs[1], aArgs[2], aArgs[3]);
	}

/**	Executive dispatcher.
	This is hooked by EUSER to handle executive dispatch into the kernel.
	aArgs can be treated as an array of 1 to 4 arguments (depending on the exec call).

	@internalTechnology
 */
EXPORT_C TInt __fastcall Dispatch(TInt aFunction, TInt* aArgs)
	{
	NThread& me = *static_cast<NThread*>(TheScheduler.iCurrentThread);
	__NK_ASSERT_ALWAYS(!me.iDiverting);
	
	EnterKernel();

	if (aFunction & 0x800000)
		{
		// fast exec
		const SFastExecTable* table = me.iFastExecTable;

		aFunction &= 0x7fffff;
		if (aFunction == 0)
			{
			// special case fast exec call
			NKern::WaitForAnyRequest();
			LeaveKernel();
			return 0;
			}

		if (TUint(aFunction) < TUint(table->iFastExecCount))
			{
			NKern::Lock();
			TInt r = Invoke(table->iFunction[aFunction-1], aArgs);
			NKern::Unlock();
			LeaveKernel();
			return r;
			}

		// invalid exec number passed, so ensure we invoke the invalid exec
		// handler by setting an illegal slow exec number
		aFunction = -1;
		}

	// slow exec
	const SSlowExecTable* table = (const SSlowExecTable*)((const TUint8*)me.iSlowExecTable - _FOFF(SSlowExecTable, iEntries));

	if (TUint(aFunction) >= TUint(table->iSlowExecCount))
		return Invoke(table->iInvalidExecHandler, aArgs);

	const SSlowExecEntry& e = table->iEntries[aFunction];

	if (e.iFlags & KExecFlagClaim)
		NKern::LockSystem();

	if (e.iFlags & KExecFlagPreprocess)
		{
		// replace the first argument with the result of preprocessing
		TPreprocessHandler preprocesser = (TPreprocessHandler)table->iPreprocessHandler;
		preprocesser(aArgs, e.iFlags);
		}

	TInt r = Invoke(e.iFunction, aArgs);

	if (e.iFlags & KExecFlagRelease)
		NKern::UnlockSystem();

	LeaveKernel();
	return r;
	}
