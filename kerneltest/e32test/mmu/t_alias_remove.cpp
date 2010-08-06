// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_alias_remove.cpp
// Overview:
// Test interactions when free memory being aliases.
// Details:
// Create 3 mappings to one chunk one that owns the chunk, one to map it again another process 
// and another alias mapping.
// Then while the alias mapping is accessing the chunk close the second mapping.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32hal.h>
#include <e32svr.h>
#include "..\defrag\d_pagemove.h"

const TPtrC KAliasProcessName = _L("T_ALIAS_REMOVE");
const TPtrC KAliasChunkName = _L("AliasChunk");
const TPtrC KAliasServerName = _L("AliasServer");
const TPtrC KMasterServerName = _L("MasterServer");

RBuf ChunkName;
RBuf MasterServerName;
RBuf AliasServerName;

RTest test(KAliasProcessName);

//#define ENABLE_PRINTFS
#ifndef __MSVC6__	// VC6 can't cope with variable arguments in macros.

#define T_PRINTF(x...) test.Printf(x)
#define D_PRINTF(x...) RDebug::Printf(x)
#ifdef ENABLE_PRINTFS
#define PRINTF(x) x
#else
#define PRINTF(x)
#endif // ENABLE_PRINTFS

#else
#define PRINTF(x)
#endif	// __MSCV6__

enum TSlaveMsgType
	{
	ESlaveConnect = -1,
	ESlaveDisconnect = -2,
	ESlaveReadChunk = 0,
	ESlaveClosedChunk = 1,
	};


struct SThreadData 
	{
	TUint8 iFillValue;
	TUint iChunkSize;
	RThread* iMasterThread;
	TUint iProcessId;
	};


class RAliasSession : public RSessionBase
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


TInt SlaveProcess(TUint aProcessId, TUint aFillValue)
	{		
	// Connect to the master server to indicate that we're ready to receive ipc messages.
	RAliasSession masterSession;
	test_KErrNone(masterSession.CreateSession(MasterServerName, 1));

	PRINTF(T_PRINTF(_L("Process ID %d Slave open chunk\n"), aProcessId));
	// Open the global chunk.
	RChunk chunk;
	TInt r = chunk.OpenGlobal(ChunkName, ETrue);
	test_KErrNone(r);

	// Connect to alias server.
	PRINTF(T_PRINTF(_L("Process ID %d Slave connect to alias server\n"), aProcessId));
	RAliasSession aliasSession;
	test_KErrNone(aliasSession.CreateSession(AliasServerName, 1));

	PRINTF(T_PRINTF(_L("Process ID %d Slave send data to alias server\n"), aProcessId));
	TPtr8 arg0(chunk.Base(), chunk.Size(), chunk.Size());
	r = aliasSession.PublicSend(ESlaveReadChunk, TIpcArgs(&arg0));
	test_KErrNone(r);
	
	// Close the chunk removing its mapping before the server has read it.
	chunk.Close();
	PRINTF(T_PRINTF(_L("Process ID %d Slave closed chunk\n"), aProcessId));

	r = masterSession.PublicSendReceive(ESlaveClosedChunk, TIpcArgs());
	test_KErrNone(r);
	aliasSession.Close();
	masterSession.Close();
	return KErrNone;
	}


TInt ChunkReadThread(TAny* aThreadData)
	{
	SThreadData* threadData =  (SThreadData*)aThreadData;
	RServer2 aliasServer;
	TInt r = aliasServer.CreateGlobal(AliasServerName);
	if (r != KErrNone)
		{
		RDebug::Printf("Process ID %d Error creating alias server r=%d", threadData->iProcessId, r);
		return r;
		}
	// Connect to the master server to indicate that we're ready to receive ipc messages.
	RAliasSession masterSession;
	test_KErrNone(masterSession.CreateSession(MasterServerName, 1));

	PRINTF(D_PRINTF("Process ID %d Alias wait for slave connection", threadData->iProcessId));
	RMessage2 aliasMessage;
	// Read and complete the connect message from the slave.
	aliasServer.Receive(aliasMessage);
	test_Equal(ESlaveConnect, aliasMessage.Function());
	aliasMessage.Complete(KErrNone);

	// Read the data of the remote chunk.
	PRINTF(D_PRINTF("Process ID %d Alias read chunk data", threadData->iProcessId));
	HBufC8* argTmp = HBufC8::New(threadData->iChunkSize);
	test_NotNull(argTmp);
	RBuf8 argBuf(argTmp);
	aliasServer.Receive(aliasMessage);
	test_Equal(ESlaveReadChunk, aliasMessage.Function());
	r = aliasMessage.Read(0, argBuf);
	if (r == KErrNone)
		{// Successfully read the chunk so verify it.
		aliasMessage.Complete(KErrNone);

		PRINTF(D_PRINTF("Process ID %d Alias verify chunk data", threadData->iProcessId));
		const TUint8* bufPtr = argBuf.Ptr();
		const TUint8* bufEnd = bufPtr + threadData->iChunkSize;
		for (; bufPtr < bufEnd; bufPtr++)
			{
			if (*bufPtr != threadData->iFillValue)
				{
				RDebug::Printf("Process ID %d Read incorrect data exp 0x%x got 0x%x", 
								threadData->iProcessId, threadData->iFillValue, *bufPtr);
				r = *bufPtr;
				break;
				}
			}
		}
	else
		{
		PRINTF(D_PRINTF("Process ID %d Error reading chunk remotely %d", threadData->iProcessId, r));
		}
	argBuf.Close();
	masterSession.Close();
	return r;
	}


TInt MasterProcess(TInt aProcessId)
	{
	TInt pageSize;
	UserHal::PageSizeInBytes(pageSize);
	// Need a large chunk so that alias that reads it is held for a long
	// enough period for there to be conflicts with the chunk closure in 
	// the slave process.
	const TUint KChunkSize = pageSize * 1024;

	PRINTF(T_PRINTF(_L("Process ID %d Create chunk\n"), aProcessId));
	RChunk chunk;
	TInt r = chunk.CreateGlobal(ChunkName, KChunkSize, KChunkSize);
	test_KErrNone(r);


	for (TUint8 fillValue = 1; fillValue < 255; fillValue++)
		{
		// Output a character every 16 iterations so test machines 
		// don't time out.
		if ((fillValue & 0xf) == 1)
			test.Printf(_L("."));

		test.Printf(_L("Process ID %d start slave fill value %d\n"), aProcessId, fillValue);
		RServer2 masterServer;
		r = masterServer.CreateGlobal(MasterServerName);
		test_KErrNone(r);
		RMessage2 masterMessage;

		// Update the chunk to new fill value.
		memset(chunk.Base(), fillValue, KChunkSize);

		PRINTF(T_PRINTF(_L("Process ID %d Start the slave process\n"), aProcessId));
		RProcess slaveProcess;
		test_KErrNone(slaveProcess.Create(KAliasProcessName, KNullDesC));
		test_KErrNone(slaveProcess.SetParameter(1, aProcessId));
		test_KErrNone(slaveProcess.SetParameter(2, fillValue));
		TRequestStatus slaveStatus;
		slaveProcess.Logon(slaveStatus);
		test_Equal(KRequestPending, slaveStatus.Int());
		slaveProcess.Resume();

		// Wait for the connect message from the slave process.
		masterServer.Receive(masterMessage);
		test_Equal(ESlaveConnect, masterMessage.Function());

		SThreadData threadData;
		threadData.iFillValue = fillValue;
		threadData.iChunkSize = KChunkSize;
		threadData.iProcessId = aProcessId;
		RThread readThread;
		r = readThread.Create(KNullDesC, ChunkReadThread, 10 * pageSize, KChunkSize, KChunkSize * 2, &threadData);
		test_KErrNone(r);
		TRequestStatus threadStatus;
		readThread.Logon(threadStatus);
		test_Equal(KRequestPending, threadStatus.Int());
		readThread.Resume();

		PRINTF(T_PRINTF(_L("Process ID %d Wait for alias thread to start server\n"), aProcessId));
		RMessage2 aliasMessage;
		masterServer.Receive(aliasMessage);
		test_Equal(ESlaveConnect, aliasMessage.Function());
		aliasMessage.Complete(KErrNone);

		// Signal to the slave process to send chunk to alias thread.
		PRINTF(T_PRINTF(_L("Process ID %d Signal to slave to send chunk to alias\n"), aProcessId));
		masterMessage.Complete(KErrNone);

		// Wait for slave to close the chunk and fill it with new value.
		for (;;)
			{
			masterServer.Receive(masterMessage);
			TInt func = masterMessage.Function();
			PRINTF(T_PRINTF(_L("Process ID %d rxd %d\n"), aProcessId, func));
			if (func == ESlaveClosedChunk)
				{// Slave closed the chunk.
				memset(chunk.Base(), ++fillValue, KChunkSize);
				break;
				}
			else
				{// Alias has read the chunk and completed.
				test_Equal(ESlaveDisconnect, func);
				}
			}
		
		test.Printf(_L("Process ID %d Wait for alias to complete\n"), aProcessId);
		masterMessage.Complete(KErrNone);
		User::WaitForRequest(threadStatus);
		TInt statusInt = threadStatus.Int();
		test_Value(	statusInt, 
					statusInt == KErrNone || 
					statusInt == KErrBadDescriptor ||
					statusInt == KErrDied);
		test_Equal(EExitKill, readThread.ExitType());
		readThread.Close();

		test.Printf(_L("Process ID %d Wait for slave to complete\n"), aProcessId);
		User::WaitForRequest(slaveStatus);
		test_Equal(EExitKill, slaveProcess.ExitType());
		test_Equal(KErrNone, slaveProcess.ExitReason());
		slaveProcess.Close();
		masterServer.Close();
		}

	chunk.Close();

	return 0;
	}


GLDEF_C TInt E32Main()
	{
	TInt processId;
	if(User::GetTIntParameter(1, processId)==KErrNone)
		{
		test_KErrNone(ChunkName.Create(KAliasChunkName.Length() + 3));
		ChunkName.Copy(KAliasChunkName);
		ChunkName.AppendNum(processId);

		test_KErrNone(MasterServerName.Create(KMasterServerName.Length() + 3));
		MasterServerName.Copy(KMasterServerName);
		MasterServerName.AppendNum(processId);

		test_KErrNone(AliasServerName.Create(KAliasServerName.Length() + 3));
		AliasServerName.Copy(KAliasServerName);
		AliasServerName.AppendNum(processId);

		TInt fillValue;
		if(User::GetTIntParameter(2, fillValue)==KErrNone)
			{
			return SlaveProcess(processId, fillValue);
			}
		return MasterProcess(processId);
		}

	// Get the number of cpus and use it to determine how many processes to execute.
	RPageMove pagemove;
	test_KErrNone(pagemove.Open());
	TUint masterProcesses = pagemove.NumberOfCpus() + 1;
	pagemove.Close();

	TInt cmdLineLen = User::CommandLineLength();
	if(cmdLineLen)
		{
		RBuf cmdLine;
		test_KErrNone(cmdLine.Create(cmdLineLen));
		User::CommandLine(cmdLine);
		test_KErrNone(TLex(cmdLine).Val(masterProcesses));
		}

	test.Title();
	test.Start(_L(""));
	test.Printf(_L("Create %d processes for accessing aliases being removed\n"), masterProcesses); 

	TUint32 debugMask = UserSvr::DebugMask();
	User::SetDebugMask(0);

	// Start master processes to alias memory between each other.
	RProcess* masters = new RProcess[masterProcesses];
	TRequestStatus* masterStatus = new TRequestStatus[masterProcesses];
	TUint i = 0;
	for (; i < masterProcesses; i++)
		{
		test_KErrNone(masters[i].Create(KAliasProcessName, KNullDesC));
		test_KErrNone(masters[i].SetParameter(1, i));
		masters[i].Logon(masterStatus[i]);
		test_Equal(KRequestPending, masterStatus[i].Int());
		}
	test.Next(_L("Resume the processes")); 
	for (i = 0; i < masterProcesses; i++)
		{
		masters[i].Resume();
		}

	test.Next(_L("Wait for processes to exit")); 
	for (i = 0; i < masterProcesses; i++)
		{
		User::WaitForRequest(masterStatus[i]);
		test_Equal(EExitKill, masters[i].ExitType());
		test_Equal(KErrNone, masters[i].ExitReason());
		}
	User::SetDebugMask(debugMask);
	delete masterStatus;
	delete masters;
	test.Printf(_L("\n"));
	test.End();
	return KErrNone;
	}
