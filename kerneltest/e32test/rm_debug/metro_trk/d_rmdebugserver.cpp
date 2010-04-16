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
// Provides the debug agent server implementation.
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <trkkerneldriver.h>
#include "d_rmdebugserver.h"
#include "d_rmdebugclient.h"
#include "t_rmdebug.h"


CDebugServServer::CDebugServServer(CActive::TPriority aActiveObjectPriority)
   : CServer2(aActiveObjectPriority)
//
// Server constructor
//
	{
	}

CSession2* CDebugServServer::NewSessionL(const TVersion& /*aVersion*/, const RMessage2& /*aMessage*/) const
//
// Session constructor
//
	{
	// make sure the kernel side device driver is not already loaded
   TInt err;
	err = User::LoadLogicalDevice(KDebugDriverFileName);
	if ((KErrNone == err) || (KErrAlreadyExists == err))
       {
       return new(ELeave) CDebugServSession();
       }
   else
       {
       return (NULL);
       }
	}   

CDebugServSession::CDebugServSession()
// Session implementation
	{
   TInt err;
	TMetroTrkDriverInfo info;
   info.iUserLibraryEnd = 0;
   err = iKernelDriver.Open(info);
   if (KErrNone != err)
		{
 		User::Leave(err);
		}          
	}

CDebugServSession::~CDebugServSession()
//
// Session destructor
//
	{
	// stop the kernel side driver
	iKernelDriver.Close();

	User::FreeLogicalDevice(KDebugDriverName);
	}


void CDebugServSession::ServiceL(const RMessage2& aMessage)
//
// Session service handler
//
	{
	TInt res = KErrNone;

	switch(aMessage.Function())
		{
		case EDebugServResumeThread:
			res = ResumeThread(aMessage);
			break;

		case EDebugServSuspendThread:
			res = SuspendThread(aMessage);
			break;          

//		case EDebugServReadProcessInfo:
//			res = ReadProcessInfo(aMessage);
//			break;        
//
//		case EDebugServReadThreadInfo:
//			res = ReadThreadInfo(aMessage);
//			break;

		case EDebugServReadMemory:
			res = ReadMemory(aMessage);
			break;        

		case EDebugServWriteMemory:
			res = WriteMemory(aMessage);
			break;        

		default:
			User::Leave(KErrNotSupported);
			break;
		}

	aMessage.Complete(res);
	}



TInt CDebugServSession::SuspendThread(const RMessage2& aMessage)
//
// Session suspend thread
//
	{
	TInt err;

	err = iKernelDriver.SuspendThread(aMessage.Int0());

	return err;
	}

TInt CDebugServSession::ResumeThread(const RMessage2& aMessage)
//
// Server resume thread
//
	{
	TInt err;

	err = iKernelDriver.ResumeThread(aMessage.Int0());

	return err;
	}

//TInt CDebugServSession::ReadProcessInfo(const RMessage2& aMessage)
////
//// Server read process information
////
//	{
//	TInt err;
//	TProcessInfo procinfo;
//	TMetroTrkTaskInfo processInfo(0);
//
//	err = iKernelDriver.GetProcessInfo(aMessage.Int0(), processInfo);
//
//	if (KErrNone == err)
//		{
//		procinfo.iProcessID = processInfo.iId;
//		procinfo.iPriority = processInfo.iPriority;
//		procinfo.iName.Copy(processInfo.iName);
//
//		TPckgBuf<TProcessInfo> p(procinfo);
//		aMessage.WriteL(1,p);        
//		}
//
//	return err;
//	}
//
//TInt CDebugServSession::ReadThreadInfo(const RMessage2& aMessage)
////
//// Server read thread information
////
//	{
//	TInt err;
//	TThreadInfo thrdinfo;
//	TMetroTrkTaskInfo threadInfo(aMessage.Int1()); // Sets OtherID to the second input parameter in aMessage
//
//	// aMessage.Int0 is the index into the thread list for the process
//	err = iKernelDriver.GetThreadInfo(aMessage.Int0(), threadInfo);	   
//
//	if (KErrNone == err)
//		{
//		thrdinfo.iThreadID = threadInfo.iId;
//		thrdinfo.iPriority = threadInfo.iPriority;
//		thrdinfo.iName.Copy(threadInfo.iName);
//		thrdinfo.iOwningProcessID = threadInfo.iOtherId;
//
//		TPckgBuf<TThreadInfo> p(thrdinfo);
//
//		// Write out the results to the third argument passed in (pointer to the threadinfo structure)
//		aMessage.WriteL(2,p);           
//		}
//
//	return err;
//	}

TInt CDebugServSession::ReadMemory(const RMessage2& aMessage)
//
// Server read process memory
//
	{   
	TInt err;
	TUint32 threadId = aMessage.Int0();
	TPckgBuf<TMemoryInfo> pckg = *(TPckgBuf<TMemoryInfo> *)(aMessage.Ptr1());
	TMemoryInfo* InputMemoryInfo = &pckg();

	TPtr8 *ptrtst = InputMemoryInfo->iDataPtr;

	err = iKernelDriver.ReadMemory(threadId, InputMemoryInfo->iAddress, InputMemoryInfo->iSize, *ptrtst);

	return err;
	}

TInt CDebugServSession::WriteMemory(const RMessage2& aMessage)
//
// Server write process memory
//
	{
	TInt err;
	TUint32 threadId = aMessage.Int0();
	TPckgBuf<TMemoryInfo> pckg = *(TPckgBuf<TMemoryInfo> *)(aMessage.Ptr1());
	TMemoryInfo* InputMemoryInfo = &pckg();

	TPtr8 *ptrtst = InputMemoryInfo->iDataPtr;

	err = iKernelDriver.WriteMemory(threadId, InputMemoryInfo->iAddress, InputMemoryInfo->iSize, *ptrtst);

	return err;
	}


GLDEF_C TInt CDebugServServer::ThreadFunction(TAny*)
//
// Server thread function, continues until active scheduler stops
//
	{
	CTrapCleanup* cleanup=CTrapCleanup::New();
	if (cleanup == NULL)
		{
		User::Leave(KErrNoMemory);
		}

	CActiveScheduler *pA=new CActiveScheduler;
	CDebugServServer *pS=new CDebugServServer(EPriorityStandard);

	CActiveScheduler::Install(pA);

	TInt err = pS->Start(KDebugServerName);
	if (err != KErrNone)
		{
		User::Leave(KErrNone);
		}

	RThread::Rendezvous(KErrNone);

	CActiveScheduler::Start();

	delete pS;
	delete pA;
	delete cleanup;

	return (KErrNone);
	}



EXPORT_C TInt StartThread(RThread& aServerThread)
//
// Start the server thread
//
	{
	TInt res=KErrNone;

	TFindServer finddebugserver(KDebugServerName);
	TFullName name;

	if (finddebugserver.Next(name) != KErrNone)
		{
		res = aServerThread.Create( KDebugServerName,
									CDebugServServer::ThreadFunction,
									KDefaultStackSize,
									KDefaultHeapSize,
									KDefaultHeapSize,
									NULL
									);

		if (res == KErrNone)
			{
			TRequestStatus rendezvousStatus;

			aServerThread.SetPriority(EPriorityNormal);
			aServerThread.Rendezvous(rendezvousStatus);
			aServerThread.Resume();
			User::WaitForRequest(rendezvousStatus);
			}                                 
		else
			{
			aServerThread.Close();
			}
		}

	return res;
	}



RDebugServSession::RDebugServSession()
//
// Server session constructor
//
	{
	}

TInt RDebugServSession::Open()
//
// Session open
//
	{
	TInt r = StartThread(iServerThread);
	if (r == KErrNone)
		{
		r=CreateSession(KDebugServerName, Version(), KDefaultMessageSlots);
		}

	return r;
	}


TVersion RDebugServSession::Version(void) const
//
// Session version
//
	{
	return (TVersion(KDebugServMajorVersionNumber, KDebugServMinorVersionNumber, KDebugServBuildVersionNumber));
	}

TInt RDebugServSession::SuspendThread(const TInt aThreadID)
//
// Session suspend thread request
//
	{
	TIpcArgs args(aThreadID);
	TInt res;
	res = SendReceive(EDebugServSuspendThread, args);

	return res;
	}

TInt RDebugServSession::ResumeThread(const TInt aThreadID)
//
// Session resume thread request
//
	{
	TIpcArgs args(aThreadID);
	TInt res;
	res = SendReceive(EDebugServResumeThread, args);

	return res;
	}


//TInt RDebugServSession::ReadProcessInfo(const TInt aIndex, TProcessInfo* aInfo)
////
//// Session read process information request
////
//	{
//	TPckgBuf<TProcessInfo> pckg;
//	pckg = *aInfo;
//
//	TIpcArgs args(aIndex, &pckg);
//
//	TInt res;
//
//	res = SendReceive(EDebugServReadProcessInfo, args);
//
//	*aInfo = pckg();
//
//	return res;
//
//	}
//
//TInt RDebugServSession::ReadThreadInfo(const TInt aIndex, const TInt aProc, TThreadInfo* aInfo)
////
//// Session read thread information request
////
//	{
//	TPckgBuf<TThreadInfo> pckg;
//	pckg = *aInfo;
//
//	TIpcArgs args(aIndex, aProc, &pckg);
//
//	TInt res;
//
//	res = SendReceive(EDebugServReadThreadInfo, args);
//
//	*aInfo = pckg();
//
//	return res;
//
//	}


TInt RDebugServSession::ReadMemory(const TUint32 aThreadID, TMemoryInfo* aInfo)
//
// Session read thread memory request
//
	{
	TPckgBuf<TMemoryInfo> pckg;
	pckg = *aInfo;

	TIpcArgs args(aThreadID, &pckg);

	TInt res;

	res = SendReceive(EDebugServReadMemory, args);

	*aInfo = pckg();

	return res;

	}


TInt RDebugServSession::WriteMemory(const TUint32 aThreadID, TMemoryInfo* aInfo)
//
// Session write thread memory request
//
	{
	TPckgBuf<TMemoryInfo> pckg;
	pckg = *aInfo;

	TIpcArgs args(aThreadID, &pckg);

	TInt res;

	res = SendReceive(EDebugServWriteMemory, args);

	return res;
	}



TInt RDebugServSession::Close()
//
// Session close the session and thread
//
	{
	RSessionBase::Close();
	iServerThread.Close();

	return KErrNone;
	}

