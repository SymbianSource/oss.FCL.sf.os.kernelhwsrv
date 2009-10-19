/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

#include <e32test.h>
#include <e32svr.h>
#include <e32msgqueue.h>

LOCAL_D RTest test(_L("t_mqueueecho"));


_LIT(KQueueA, "A");
_LIT(KQueueB, "B");

LOCAL_C void RunTests(void)
	{
	test.Start(_L("Testing"));

	RMsgQueueBase in;
	RMsgQueueBase out;

	TInt ret = in.OpenGlobal(KQueueA);
	test (KErrNone == ret);
	ret = out.OpenGlobal(KQueueB);
	test (KErrNone == ret);

	TInt size = in.MessageSize();
	test (size == out.MessageSize());
 
	TUint8* pBuffer = (TUint8*)User::Alloc(size);
	test (NULL != pBuffer);

	TInt count = 0;
	do
		{
		in.ReceiveBlocking(pBuffer, size);
		out.SendBlocking(pBuffer, size);
		test.Printf(_L("Echo iteration %i received %i\r\n"), ++count, *(TInt*)pBuffer);
		}
	while (*(TInt*)pBuffer != 0);


	in.Close();
	out.Close();
	test.Next(_L("Ending test.\n"));
	test.End();
	
	test.Close();
	}

GLDEF_C TInt E32Main()
//
//
    {

	test.Title();

	RunTests();

	return KErrNone;
    }

