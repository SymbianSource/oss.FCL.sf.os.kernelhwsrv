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
#include <e32svr.h>
#include <u32std.h>

#include "t_asid.h"
#include "d_asid.h"

#define TEST_R(r, e) if (!(e)) {RDebug::Printf("Failure at line %d, r = %d", __LINE__, r); return r; }

_LIT(KDummyProcessName,"T_ASID_DUMMY");

RTest test(_L("T_ASID"));

const TUint KMaxAsids = 256; // On arm and current version of FMM has maximum of 256 active processes at once.
const TUint KLeakedProcesses = KMaxAsids<<1;

void TestMaxAsids()
	{
	test.Printf(_L("Attempt to load > %d zombie processes\n"), KMaxAsids);
	RProcess* processes = new RProcess[KLeakedProcesses];
	TUint i = 0;
	for (; i < KLeakedProcesses; i++)
		{
		test_KErrNone(processes[i].Create(KDummyProcessName, KNullDesC));
		TRequestStatus s;
		processes[i].Logon(s);
		test_Equal(KRequestPending, s.Int());
		processes[i].Resume();
		User::WaitForRequest(s);
		if (processes[i].ExitType()!=EExitKill || processes[i].ExitReason()!=KErrNone)
			{
			TExitCategoryName aExitCategory = processes[i].ExitCategory();
			test.Printf(_L("Exit      %d,%d,%S\n"),processes[i].ExitType(),processes[i].ExitReason(),&aExitCategory);
			test(0);
			}
		}
	// Clean up the process handles here.
	for (i = 0; i < KLeakedProcesses; i++)
		processes[i].Close();
	delete[] processes;
	}


TInt TestIpcThread(TAny*)
	{
	// Start a server for zombie to connect to.
	RServer2 ipcServer;
	TInt r = ipcServer.CreateGlobal(KAsidIpcServerName);
	TEST_R(r, r == KErrNone);
	RMessage2 ipcMessage;
	for (TUint i = 0; i < 300; i++)
		{
		// Start the process.
		RProcess zombie;
		TInt r = zombie.Create(KDummyProcessName, KAsidIpcServerName);
		TEST_R(r, r == KErrNone);
		TRequestStatus s;
		zombie.Logon(s);
		TEST_R(s.Int(), s.Int() == KRequestPending);
		zombie.Resume();

		// Wait for the connect and ipc message from the zombie
		ipcServer.Receive(ipcMessage);
		TEST_R(ipcMessage.Function(), ipcMessage.Function() == EConnect);
		ipcMessage.Complete(KErrNone);
		ipcServer.Receive(ipcMessage);
		TEST_R(ipcMessage.Function(), ipcMessage.Function() == EIpcData);

		TUint8 array1[KAsidDesLen];
		memset(array1, KAsidValue, KAsidDesLen);
		TPtr8 buf1(array1, KAsidDesLen, KAsidDesLen);

		TUint8 array[KAsidDesLen];
		TPtr8 buf(array, KAsidDesLen);
		test_Equal(KErrNone, ipcMessage.Read(0, buf));
		r = ipcMessage.Read(0, buf);
		TEST_R(r, r == KErrNone);
		r = buf.Compare(buf1);
		TEST_R(r, r == 0);
		ipcMessage.Complete(KErrNone);	

		// Try to read from the client while it is exiting.
		ipcServer.Receive(ipcMessage);
		TEST_R(ipcMessage.Function(), ipcMessage.Function() ==  EIpcData);
		User::After(5*i);

		r = ipcMessage.Read(0, buf);
		//RDebug::Printf("%d", r);

		// Wait for client to exit.
		User::WaitForRequest(s);
		TEST_R(s.Int(), s.Int() == KErrNone);
		ipcServer.Receive(ipcMessage);	// Clear out the disconnect message.
		TEST_R(ipcMessage.Function(), ipcMessage.Function() == EDisConnect);
		ipcMessage.Complete(KErrNone);
		zombie.Close();
		}
	ipcServer.Close();
	return KErrNone;
	}


TInt TestDesThread(TAny*)
	{
	RAsidLdd asidLdd;
	TInt r = asidLdd.Open();
	TEST_R(r, r == KErrNone);
	RMessage2 desMessage;
	RServer2 desServer;
	// Start a server for zombie to connect to.
	r = desServer.CreateGlobal(KAsidDesServerName);
	TEST_R(r, r == KErrNone);

	for (TUint i = 0; i < 300; i++)
		{
		// Start the process.
		RProcess zombie;
		r = zombie.Create(KDummyProcessName, KAsidDesServerName);
		TEST_R(r, r == KErrNone);
		TRequestStatus s;
		zombie.Logon(s);
		TEST_R(s.Int(), s.Int() == KRequestPending);
		zombie.Resume();

		// Wait for the connect and des message from the zombie
		desServer.Receive(desMessage);
		TEST_R(desMessage.Function(), desMessage.Function() == EConnect);
		desMessage.Complete(KErrNone);
		desServer.Receive(desMessage);
		TEST_R(desMessage.Function(), desMessage.Function() == EDesData);

		TAny* desPtr = (TAny*)desMessage.Ptr0();
		TAny* threadPtr = (TAny*)desMessage.Ptr1();
		// Open a handle on the client's thread to stop it being deleted.
		r = asidLdd.OpenThread(threadPtr);
		TEST_R(r, r == KErrNone);
		SDesHeader desHdr;
		desHdr.iDes = desPtr;
		r = asidLdd.ReadDesHeader(threadPtr, desHdr);
		TEST_R(r, r == KErrNone);
		TEST_R(desHdr.iLength, desHdr.iLength == KAsidDesLen);
		TEST_R(desHdr.iMaxLength, desHdr.iMaxLength == KAsidDesLen);
		desMessage.Complete(KErrNone);

		// Wait for further des message from client.
		desServer.Receive(desMessage);
		TEST_R(desMessage.Function(), desMessage.Function() == EDesData);
		desPtr = (TAny*)desMessage.Ptr0();
		TEST_R((TInt)desMessage.Ptr1(), (TInt)desMessage.Ptr1() == (TInt)threadPtr);

		// Try to read descriptor header from the client while it is exiting.
		TInt r = asidLdd.ReadDesHeader(threadPtr, desHdr);
		User::After(5*i);

		//RDebug::Printf("%d", r);
		if (r == KErrNone)
			{
			TEST_R(desHdr.iLength, desHdr.iLength == KAsidDesLen);
			TEST_R(desHdr.iMaxLength, desHdr.iMaxLength == KAsidDesLen);
			}

		// Wait for client to exit.
		User::WaitForRequest(s);
		TEST_R(s.Int(), s.Int() == KErrNone);
		desServer.Receive(desMessage);	// Clear out the disconnect message.
		TEST_R(desMessage.Function(), desMessage.Function() == EDisConnect);
		desMessage.Complete(KErrNone);
		// Close handles in client process and thread.
		r = asidLdd.CloseThread();
		TEST_R(r, r == KErrNone);
		zombie.Close();
		}
	desServer.Close();
	asidLdd.Close();
	return KErrNone;
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Test zombie processes release their ASIDs"));

	// Get memory model.
	if ((UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL)&EMemModelTypeMask) < EMemModelTypeFlexible)
		{
		test.Printf(_L("SKIPPING TEST - Older memory models don't release ASIDs from zombie processes\n"));
		test.End();
		return KErrNone;
		}

	TestMaxAsids();

	test.Next(_L("Test ipc message copy to a zombie process"));
	
	test_KErrNone(TestIpcThread(NULL));
	TestMaxAsids();

	test.Next(_L("Test reading descriptor header from a zombie process"));
	test_KErrNone(TestDesThread(NULL));
	TestMaxAsids();
	
	test.Next(_L("Test ipc and des header to zombie processes concurrently"));
	RThread ipcThread;
	RThread desThread;
	ipcThread.Create(KNullDesC, TestIpcThread, 0x2000, NULL, NULL);
	desThread.Create(KNullDesC, TestDesThread, 0x2000, NULL, NULL);
	TRequestStatus ipcStatus;
	TRequestStatus desStatus;
	ipcThread.Logon(ipcStatus);
	desThread.Logon(desStatus);
	ipcThread.Resume();
	desThread.Resume();

	// Wait for threads to complete.
	User::WaitForRequest(ipcStatus);
	test_KErrNone(ipcStatus.Int());
	User::WaitForRequest(desStatus);
	test_KErrNone(desStatus.Int());
	TestMaxAsids();

	test.End();
	return KErrNone;
	}
