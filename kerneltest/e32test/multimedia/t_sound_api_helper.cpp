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
// e32test\multimedia\t_sound2_helper.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include "t_soundutils.h"

TInt E32Main()
	{
	RTest test(_L("t_sound_api_helper"));
#ifdef __WINS__
	// don't use JIT if running on emulator because don't want to halt
	// the test run
	User::SetJustInTime(EFalse);
#endif
	TInt unit;
	TInt r= User::GetTIntParameter(KSlot, unit);
	test(r==KErrNone);

	test.Start(_L("t_sound_api_helper"));
	RSoundSc soundSc;
	r = soundSc.Open(unit);
	test.Printf(_L("RSoundSc::Open --> r=%d\n"), r);
	test(r==KErrNone || r==KErrPermissionDenied);
	if(r==KErrNone)
		soundSc.Close();
	test.End();
	test.Close();
	// Panic with the return code from Open so that t_sound_api
	// can check for the right error code.
	User::Panic(KSoundAPICaps, r);

	// unused return value - present to prevent compiler warning
	return KErrNone;
	}
