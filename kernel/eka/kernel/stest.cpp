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
// e32\kernel\stest.cpp
// 
//

#include <kernel/kern_priv.h>
#include "kern_test.h"
#include "securerng.h"

#ifdef _DEBUG
class TTestCallback: public TUserModeCallback
	{
public:
	enum TCallbackType { ESleep, ESpin };

	static void Callback(TAny* aThisPtr, TUserModeCallbackReason aReasonCode);
	TTestCallback(TCallbackType aType);

private:
	TCallbackType iType;
	};

TTestCallback::TTestCallback(TCallbackType aType)
	:	TUserModeCallback(Callback), iType(aType)
	{
	}

void TTestCallback::Callback(TAny* aThisPtr, TUserModeCallbackReason aReasonCode)
	{
	TTestCallback* tc = (TTestCallback*)aThisPtr;
	TCallbackType type = tc->iType;
	Kern::AsyncFree(tc);
	// we leave the CS here so we can be killed
	// we won't ever return normally, so it's ok that it's unbalanced
	NKern::ThreadLeaveCS();
	if (aReasonCode == EUserModeCallbackRun)
		{
		if (type == ESleep)
			NKern::Sleep(KMaxTInt);
		else if (type == ESpin)
			for (;;) {}
		}
	}
#endif


EXPORT_C TInt KernTest::Test(TTestFunction aFunc, TAny* a1, TAny* a2, TAny* a3)
	{
	TInt r = KErrNotSupported;
	(void)aFunc; (void)a1; (void)a2; (void)a3;

	switch(aFunc)
		{
#ifdef _DEBUG
	case EUserModeCallbackSleep:
			{
			// a1 is a DThread*. We add a user mode callback to that thread
			// which sleeps for a long time, effectively blocking the thread
			// indefinately.
			DThread* t = (DThread*)a1;
			NKern::ThreadEnterCS();
			r = NKern::QueueUserModeCallback(&t->iNThread, new TTestCallback(TTestCallback::ESleep));
			NKern::ThreadLeaveCS();
			break;
			}
	case EUserModeCallbackSpin:
			{
			// a1 is a DThread*. We add a user mode callback to that thread
			// which spins in an infinite loop, ensuring that it will be
			// preempted rather than finishing or blocking.
			DThread* t = (DThread*)a1;
			NKern::ThreadEnterCS();
			r = NKern::QueueUserModeCallback(&t->iNThread, new TTestCallback(TTestCallback::ESpin));
			NKern::ThreadLeaveCS();
			break;
			}
#endif
	case ERNGReseedHook:
			{
			// a1 is a function which wants to be called with arg a2 when the RNG is reseeded.
			// Used to test if reseeds are sufficiently frequent.
			SecureRNG->SetReseedHook((void(*)(TAny*))a1, a2);
			break;
			}
	default:
		// To stop compiler warnings about unhandled enum cases in release builds
		break;
		}

	return r;
	}	
