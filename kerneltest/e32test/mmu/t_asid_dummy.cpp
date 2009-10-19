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
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include "t_asid.h"
#include "d_asid.h"

RTest test(_L("T_ASID_DUMMY"));

class RAsidSession : RSessionBase
	{
public:
	TInt CreateSession(const TDesC& aServerName, TInt aMsgSlots) 
		{ 
		return RSessionBase::CreateSession(aServerName,User::Version(),aMsgSlots);
		}
	TInt PublicSendReceive(TInt aFunction, const TIpcArgs &aPtr)
		{
		return (SendReceive(aFunction, aPtr));
		}
	TInt PublicSend(TInt aFunction, const TIpcArgs &aPtr)
		{
		return (Send(aFunction, aPtr));
		}
	};

GLDEF_C TInt E32Main()
	{

	TUint len = User::CommandLineLength();
	if (len > 0)
		{
		HBufC* buf = HBufC::New(len);
		test(buf != NULL);
		TPtr ptr = buf->Des();
		User::CommandLine(ptr);

		if (ptr == KAsidIpcServerName)
			{
			TUint8 array[KAsidDesLen];
			memset(array, KAsidValue, KAsidDesLen);
			TPtr8 buf(array, KAsidDesLen, KAsidDesLen);

			RAsidSession session;
			test_KErrNone(session.CreateSession(KAsidIpcServerName, 1));

			test_KErrNone(session.PublicSendReceive(EIpcData, TIpcArgs(&buf)));
			test_KErrNone(session.PublicSend(EIpcData, TIpcArgs(&buf)));
			// Just exit badly, i.e. without disconnecting...
			}
		else if (ptr == KAsidDesServerName)
			{
			TUint8 array[KAsidDesLen];
			memset(array, KAsidValue, KAsidDesLen);
			TPtr8 buf(array, KAsidDesLen, KAsidDesLen);

			// Get the current thread pointer to pass to server.		
			TAny* threadPtr;
			RAsidLdd asidLdd;
			test_KErrNone(asidLdd.Open());
			test_KErrNone(asidLdd.GetCurrentThread(threadPtr));

			RAsidSession session;
			test_KErrNone(session.CreateSession(KAsidDesServerName, 1));

			test_KErrNone(session.PublicSendReceive(EDesData, TIpcArgs((TAny*)&buf, threadPtr)));
			test_KErrNone(session.PublicSend(EDesData, TIpcArgs((TAny*)&buf, threadPtr)));
			// Just exit badly, i.e. without disconnecting...
			}
			
		}

	return KErrNone;
	}
