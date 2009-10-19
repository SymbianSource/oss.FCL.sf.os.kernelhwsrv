// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_env.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <e32panic.h>
#include <e32msgqueue.h>
#include <f32file.h>

LOCAL_D RTest test(_L("T_ENV_CHILD"));



GLDEF_C TInt E32Main()
    {

	test.Title();
	test.Start(_L("Environment"));
	
	//parameter slot 1 contains a message queue of TInts which contain control messages
	//parameter slot 2 contains a message queue of TInts
	//parameter slot 3 is a mutex
	//parameter slot 4 is a semaphore
	//parameter slot 5 is a chunk

	RMsgQueue<TInt> controlQueue;
	TInt err = controlQueue.Open(1, EOwnerProcess);
	test(err==KErrNone);

	TInt t = 0;
	controlQueue.ReceiveBlocking(t);
	switch (t)
		{
		case 0:
			{
			// out of range test
			User::SetJustInTime(EFalse);
			RMsgQueue<TInt> intQueue;
			intQueue.Open(-1);			//should panic
			break;
			}

		case 1:
			{
			// out of range test
			User::SetJustInTime(EFalse);
			RMsgQueue<TInt> intQueue;
			intQueue.Open(4545);			//should panic
			break;
			}
				
		case 2:
			{
			//attempt to read slot which is empty
			RMsgQueue<TInt> intQueue;
			TInt ret = intQueue.Open(15);			//15 is empty
			test(ret == KErrNotFound);
			break;
			}
		
		case 3:
			//attempt to open incorrect handle type
			{
			RMutex mutex;
			TInt ret = mutex.Open(2);			//2 is a TInt queue
			test(ret == KErrArgument);
			break;
			}
		case 4:

			{
			//test passing a mutex (slot 3)
			RMutex mutex;
			mutex.Open(3, EOwnerThread);
			TFullName name = mutex.FullName();
			TInt ret = name.CompareF(_L("testmutex"));
			test (ret == KErrNone);
			mutex.Close();
			break;
			}

		case 5:
			{
			//test passing a semaphore (slot 4)
			RSemaphore sem;
			sem.Open(4, EOwnerThread);
			TFullName name = sem.FullName();
			TInt ret = name.CompareF(_L("testsemaphore"));
			test (ret == KErrNone);
			sem.Close();
			break;
			}

		case 6:
			{
			//test file handle (slots 7=session 8=file)
			_LIT8(KTestData,"test data");
			RFs session;
			session.Open(7);
			RFile file;
			TInt handle;
			TInt len = User::ParameterLength(8);
			test (len == 4);
			TInt ret = User::GetTIntParameter(8, handle);
			test(ret == KErrNone);
			file.Adopt(session, handle);
			TBuf8<100> rbuf;
			ret = file.Read(0, rbuf);
			test(ret == KErrNone);
			file.Close();
			ret = rbuf.CompareF(KTestData);
			test(ret == KErrNone);
			
			session.Close();
			break;
			}
	
		case 7:
			{
			//test a chunk in slot 5
			RChunk chunk;
			TInt ret = chunk.Open(5, EOwnerThread);
			test (ret == KErrNone);
			TFullName name = chunk.FullName();
			ret = name.CompareF(_L("testchunk"));
			test (ret == KErrNone);
			chunk.Close();

			break;
			}
		case 8:
			{
			//test passing a 16 bit descriptor // slot 15
			//_L("16 bit text"
			TBuf16<40> buf;
			TInt len = User::ParameterLength(15);
			TInt ret = User::GetDesParameter(15, buf);
			test (ret == KErrNone);
			test(buf.Length() == len/2);
			ret = buf.CompareF(_L("16 bit text"));
			test (ret == KErrNone);

			break;
			}
		case 9:
			{
			//test passing a 8 bit descriptor // slot 15
			TBuf8<40> buf;
			TInt len = User::ParameterLength(15);
			TInt ret = User::GetDesParameter(15, buf);
			test (ret == KErrNone);
			test (len == buf.Length());
			ret = buf.CompareF(_L8("8 bit text"));
			test (ret == KErrNone);
			break;
			}

		case 10:
			{
			User::SetJustInTime(EFalse);
			TPtr8 bad((TUint8*)0xfeed, 20);
			User::GetDesParameter(15, bad);
			break;
			}

		case 11:
			{
			//test passing zero length data
			TBuf8<40> buf;
			TInt len = User::ParameterLength(15);
			TInt ret = User::GetDesParameter(15, buf);
			test (ret == KErrNone);
			test (len == buf.Length());
			test (len == 0);
			break;
			}

		case 12:
			{
			//test getting command line, will be zero at the moment as just a reserved slot
			TBuf8<40> buf;
			TInt len = User::ParameterLength(0);
			TInt ret = User::GetDesParameter(0, buf);
			test (ret == KErrNone);
			test (len == buf.Length());
			test (len == 0);
			break;
			}
		
		default:
			test(0);
			break;
		}
		
	controlQueue.Close();
	test.End();
	return 0;
    }
