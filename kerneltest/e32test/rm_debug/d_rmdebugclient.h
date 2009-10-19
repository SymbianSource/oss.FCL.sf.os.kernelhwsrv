// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// t_rmdebugclient.h
// Definitions for the run mode debug agent client side sessions.
// 
//

#ifndef RMDEBUGCLIENT_H
#define RMDEBUGCLIENT_H

class TProcessInfo
	{
	public:
		TUint           iProcessID;
		TInt            iAttributes;
		TUint32         iPriority;
		TBuf<KMaxName> 	iName;    
	};

class TThreadInfo
	{
	public:
		TUint           iThreadID;
		TUint8          iPriority;
		TUint8          iThreadType;
		TBuf<KMaxName>  iName;
		TUint           iOwningProcessID;

	// Probably would want a state element here, under debug control, stopped etc
	// such that additional information could be provided which was only valid for
	// stopped threads.

	};

class TExecReq
	{
	public:
		TUint32 iRequest;       // Step, Step into, step threads
		TUint32 iStartAddress;
		TUint32 iStopAddress;

	};

class TMemoryInfo
	{
	public:
		TUint32 iAddress;
		TUint32	iSize;
		TPtr8*	iDataPtr;
	};


// Client Side session
class RDebugServSession : public RSessionBase
	{
	public:
		RDebugServSession();
		TVersion Version() const;

		TInt Open();
		TInt Close();

		TInt ReadMemory(const TUint32 aThreadId, TMemoryInfo* aInfo);
		TInt WriteMemory(const TUint32 aThreadId, TMemoryInfo* aInfo);

//		TInt ReadThreadInfo(const TInt aIndex, const TInt aOwningProc, TThreadInfo* aInfo);
//		TInt ReadProcessInfo(const TInt aIndex, TProcessInfo* aInfo);
		TInt SuspendThread(const TInt ThreadID);
		TInt ResumeThread(const TInt ThreadID);

	private:
		RThread iServerThread;       
	};

// Function codes (opcodes) used in message passing between client and server
enum TDebugServRqst
	{
	EDebugServOpen = 1,
	EDebugServClose,

	EDebugServSuspendThread,
	EDebugServResumeThread,
//	EDebugServReadProcessInfo,
//	EDebugServReadThreadInfo,
	EDebugServReadMemory,
	EDebugServWriteMemory,
	};

#endif // RMDEBUGCLIENT_H
