// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\smonitor.cpp
// Kernel crash debugger - machine independent portion common between the crash
// debugger and crash logger.
// 
//

#include <kernel/monitor.h>
#include "msgqueue.h"

#define iMState		iWaitLink.iSpare1

GLDEF_D Monitor* TheMonitorPtr = 0;
LOCAL_D Monitor* TheAltMonitorPtrs[MONITOR_MAXCOUNT] = { 0 };

_LIT8(KLitThread, "THREAD");
_LIT8(KLitProcess, "PROCESS");
_LIT8(KLitChunk, "CHUNK");
_LIT8(KLitLibrary, "LIBRARY");
_LIT8(KLitSemaphore, "SEMAPHORE");
_LIT8(KLitMutex, "MUTEX");
_LIT8(KLitTimer, "TIMER");
_LIT8(KLitServer, "SERVER");
_LIT8(KLitSession, "SESSION");
_LIT8(KLitLogicalDevice, "LOGICAL DEVICE");
_LIT8(KLitPhysicalDevice, "PHYSICAL DEVICE");
_LIT8(KLitLogicalChannel, "LOGICAL CHANNEL");
_LIT8(KLitChangeNotifier, "CHANGE NOTIFIER");
_LIT8(KLitUndertaker, "UNDERTAKER");
_LIT8(KLitUnknown, "UNKNOWN");
_LIT8(KLitLibraries, "LIBRARIES");
_LIT8(KLitS, "S");
_LIT8(KLitES, "ES");
_LIT8(KLitMsgQueue, "MESSAGE QUEUE");
_LIT8(KLitPropertyRef, "PROPERTY REF");
_LIT8(KLitCondVar, "CONDITION VARIABLE");

const TDesC8* const ContainerName[ENumObjectTypes]=
	{
	&KLitThread,
	&KLitProcess,
	&KLitChunk,
	&KLitLibrary,
	&KLitSemaphore,
	&KLitMutex,
	&KLitTimer,
	&KLitServer,
	&KLitSession,
	&KLitLogicalDevice,
	&KLitPhysicalDevice,
	&KLitLogicalChannel,
	&KLitChangeNotifier,
	&KLitUndertaker,
	&KLitMsgQueue,
	&KLitPropertyRef,
	&KLitCondVar
	};

void mstrcat(TDes8& aDes, const char* s)
	{
	aDes+=TPtrC8((const TUint8*)s);
	}

void Monitor::GetObjectTypeName(TDes8& aDes, TInt aType, TBool aPlural)
	{
	if (aType<0 || aType>=ENumObjectTypes)
		aDes=KLitUnknown;
	else
		{
		aDes=*ContainerName[aType];
		if (aPlural)
			{
			switch (aType)
				{
				case EProcess:
				case EMutex:
					aDes+=KLitES;
					break;
				case ELibrary:
					aDes=KLitLibraries;
					break;
				default:
					aDes+=KLitS;
					break;
				}
			}
		}
	}

EXPORT_C void Monitor::PrintLine(const TDesC8& aDes)
	{
	Print(aDes);
	NewLine();
	}

EXPORT_C void Monitor::Printf(const char* aFmt, ...)
	{
	TBuf8<256> printBuf;
	VA_LIST list;
	VA_START(list,aFmt);
	Kern::AppendFormat(printBuf,aFmt,list);
	Print(printBuf);
	}

EXPORT_C void Monitor::Print(const char* s)
	{
	Print(TPtrC8((const TUint8*)s));
	}

EXPORT_C void Monitor::PrintLine(const char* s)
	{
	Print(s);
	NewLine();
	}

EXPORT_C void Monitor::NewLine()
	{
	TBuf<2> buf(0);
	buf.Append(0x0d);
	buf.Append(0x0a);
	Print(buf);
	}

DMonObject* DMonObject::Owner()
	{
	if (iOwner)
		return iOwner;
	return NULL;
	}

TInt DMonObject::Type()
	{
	return iContainerID-1;  // KErrNotFound (-1) returned for objects not in a container
	}

void DMonObject::AppendName(TDes8& aDes)
	{
	((DObject*)this)->TraceAppendName(aDes,EFalse);
	}

void DMonObject::FullName(TDes8& aDes)
	{
	((DObject*)this)->TraceAppendFullName(aDes,EFalse);
	}

EXPORT_C void Monitor::ObjectDump(TUint aAddr)
	{
	DMonObject* pO=(DMonObject*)aAddr;
	pO->DumpData();
	}

EXPORT_C void Monitor::ObjectFullDump(TUint aAddr)
	{
	DMonObject* pO=(DMonObject*)aAddr;
	TInt index=pO->Type();
	DumpObjectData(pO,index);
	}

EXPORT_C void Monitor::DoMemoryDumpL(TUint aStart, TInt aLength)
	{
	TUint readSize=0x1;
	DoMemoryDumpL(aStart,aLength,readSize);		
	}

 
 EXPORT_C void Monitor::DoMemoryDumpL(TUint aStart, TInt aLength, TUint aReadSize)
 	{
    TBuf8<80> out;
	TBuf8<20> ascii;
	TUint a=aStart;
	do
		{
		out.Zero();
		ascii.Zero();
		out.AppendNumFixedWidth(a,EHex,8);
		out.Append(_L(": "));
		TUint b;
		if(aReadSize == 0x4)
	   		{
			for (b=0; b<16; b=b+4)
				{
				TUint c=*(TUint*)(a+b);
				out.AppendNumFixedWidth(c,EHex,8);
				out.Append(' ');
				if (c<0x20 || c>=0x7f)
					{
					c=0x2e;
					}
				ascii.Append(TChar(c));
				}
			}	
		else if(aReadSize == 0x2)
			{
			for (b=0; b<16; b+=2)
				{
				TUint16 c=*(TUint16*)(a+b);
				out.AppendNumFixedWidth(c,EHex,4);
				out.Append(' ');
				if (c<0x20 || c>=0x7f)
					{
					c=0x2e;	
					}
				ascii.Append(TChar(c));
				}	
			}
		else if(aReadSize == 0x1)
			{
			for (b=0; b<16; b++)
				{
				TUint8 c=*(TUint8*)(a+b);
				out.AppendNumFixedWidth(c,EHex,2);
				out.Append(' ');
				if (c<0x20 || c>=0x7f)
					{
					c=0x2e;	
					}
				ascii.Append(TChar(c));
				}
			}
		out.Append(ascii);
		PrintLine(out);
		a+=16;
		aLength-=16;
		}while(aLength>0); 
 	}

LOCAL_C TUint32 Read32L(TUint32 anAddr)
	{
	return *((TUint32*)anAddr);
	}

EXPORT_C void Monitor::DoDiscontiguousMemoryDumpL(TUint aStart, TInt aLength)
	{
	TUint readSize=0x1;
	DoDiscontiguousMemoryDumpL(aStart, aLength, readSize);
	}


EXPORT_C void Monitor::DoDiscontiguousMemoryDumpL(TUint aStart, TInt aLength, TUint aReadSize)
	{
	const TUint KPageMask = iPageSize - 1;

	TUint start = aStart & ~0xf; // start and finish on 16-byte boundary
	TUint limit = (aStart + aLength + 0xf) & ~0xf;
	while(start!=limit)
		{
		MTRAPD(r,DoMemoryDumpL(start,limit-start,aReadSize));
		if (r==KErrNone)
			return;
		if (r!=KErrAbort)
			Leave(r);

		// Got page fault so keep attempting to read from the next page until a valid 
		// page is found or the limit is reached.
		start = iExceptionInfo[1];	// faulted address
		TUint exceptionStart = start;
		do
			{
			ProcessError(r); // Output the fault information.
			start = (start + iPageSize) & ~KPageMask; // move on to next page
			if (TUint(start-limit) <= TUint(iPageSize))
				{ // reached limit
				start = limit;
				break;
				}
			MTRAP(r,Read32L(start));
			}
		while (r!=KErrNone);

		Printf("Skipped %x bytes due to exception(s)\r\n", start - exceptionStart);
		NewLine();
		}
	}

void DMonObject::DumpData()
	{
	TBuf8<32> f;
	Monitor::GetObjectTypeName(f,Type(),EFalse);
	TheMonitorPtr->Printf("%S at %08x VPTR=%08x AccessCount=%d Owner=%08x\r\n",&f,this,iVptr,iAccessCount,iOwner);
	TBuf8<KMaxKernelName> object_name;
	FullName(object_name);
	TheMonitorPtr->Printf("Full name %S\r\n",&object_name);	
	}

EXPORT_C void Monitor::DisplayFaultInfo()
	{
	TSuperPage& sp=Kern::SuperPage();
	TExcInfo& x=sp.iKernelExcInfo;
	Printf("Fault Category: %S  Fault Reason: %08x\r\n",&iFaultCategory,iFaultReason);
	Printf("ExcId %08x CodeAddr %08x DataAddr %08x Extra %08x\r\n",sp.iKernelExcId,x.iCodeAddress,x.iDataAddress,x.iExtraData);
	DisplayCpuFaultInfo();
	}

void Monitor::DumpThreadData(DThread* pT)
	{
	TBuf8<80> buf=_L8("Thread MState ");
	switch(pT->iMState)
		{
		case DThread::ECreated: buf+=_L8("CREATED"); break;
		case DThread::EDead: buf+=_L8("DEAD"); break;
		case DThread::EReady: buf+=_L8("READY"); break;
		case DThread::EWaitSemaphore: buf+=_L8("WAITSEM "); buf.AppendNumFixedWidth((TUint)pT->iWaitObj,EHex,8); break;
		case DThread::EWaitSemaphoreSuspended: buf+=_L8("WAITSEMS "); buf.AppendNumFixedWidth((TUint)pT->iWaitObj,EHex,8); break;
		case DThread::EWaitMutex: buf+=_L8("WAITMUTEX "); buf.AppendNumFixedWidth((TUint)pT->iWaitObj,EHex,8); break;
		case DThread::EWaitMutexSuspended: buf+=_L8("WAITMUTXS "); buf.AppendNumFixedWidth((TUint)pT->iWaitObj,EHex,8); break;
		case DThread::EHoldMutexPending: buf+=_L8("HOLDMUTXP "); buf.AppendNumFixedWidth((TUint)pT->iWaitObj,EHex,8); break;
		case DThread::EWaitCondVar: buf+=_L8("WAITCONDVAR "); buf.AppendNumFixedWidth((TUint)pT->iWaitObj,EHex,8); break;
		case DThread::EWaitCondVarSuspended: buf+=_L8("WAITCONDVRS "); buf.AppendNumFixedWidth((TUint)pT->iWaitObj,EHex,8); break;
		default: buf+=_L8("??"); buf.AppendNumFixedWidth((TUint)pT->iMState,EHex,8); break;
		}
	PrintLine(buf);
	Printf("Default priority %d WaitLink Priority %d\r\n",pT->iDefaultPriority,pT->iWaitLink.iPriority);
	Printf("ExitInfo %d,%d,%S\r\n",pT->iExitType,pT->iExitReason,&pT->iExitCategory);
	Printf("Flags %08x, Handles %08x\r\n",pT->iFlags,&pT->iHandles);
	Printf("Supervisor stack base %08x size %x\r\n", pT->iSupervisorStack, pT->iSupervisorStackSize);
	Printf("User stack base %08x size %x\r\n", pT->iUserStackRunAddress, pT->iUserStackSize);
	Printf("Id=%d, Alctr=%08x, Created alctr=%08x, Frame=%08x\r\n",pT->iId,pT->iAllocator,pT->iCreatedAllocator,pT->iFrame);
	Printf("Trap handler=%08x, ActiveScheduler=%08x, Exception handler=%08x\r\n",
								pT->iTrapHandler, pT->iScheduler, pT->iExceptionHandler);
	Printf("TempObj=%08x TempAlloc=%08x IpcCount=%08x\r\n",pT->iTempObj,pT->iTempAlloc,pT->iIpcCount);
	DisplayNThreadInfo(&pT->iNThread);
	DumpCpuThreadData(pT);
	}


void Monitor::DoStackDumpL(TUint aStart, TUint aEnd)
	{
	const TUint KPageMask = iPageSize - 1;

 	// Skip unused area of stack to minimise size of dump
	aStart &= 0xfffffff0;
	TUint* ptr = (TUint*)aStart;
  	TUint* limit = (TUint*)aEnd;
  	TUint pattern = 0;
	TUint* abortAddr = 0;
	while (ptr < limit)
		{
		MTRAPD(r, pattern = *ptr);
		if(r == KErrNone)
			break;
		// Current page not found so move onto the next one.
		ptr = (TUint*)(((TUint)ptr + iPageSize) & ~KPageMask);
		abortAddr = ptr;
		ProcessError(r);
		}

  	if (pattern == 0x29292929 ||	// User stack
		pattern == 0xeeeeeeee ||	// Supervisor stack
		pattern == 0xffffffff || 
		pattern == 0xaaaaaaaa ||	// IRQ stack
		pattern == 0xbbbbbbbb ||	// FIQ stack
		pattern == 0xdddddddd ||	// ABT and UND stack
		pattern == 0x03030303)		// Uninitialised memory
  		{
  		while ((ptr + 3) < limit)
			{
  			MTRAPD(r, if( ptr[0] == pattern && ptr[1] == pattern && ptr[2] == pattern && ptr[3] == pattern) ptr+=4; else break;);
			if (r != KErrNone)
				{// Current page not found so move onto the next one.
				ptr = (TUint*)(((TUint)ptr + iPageSize) & ~KPageMask);
				abortAddr = ptr;	// Due to alignment ptr[0]-ptr[3] will all be within the same page.
				ProcessError(r);
				}
			}
  		}
  	if (ptr != (TUint*)aStart)
		{
		if (abortAddr)
			{
			Printf("Skipped %x bytes due to exception(s)\r\n", (TUint)abortAddr - aStart);
			if (ptr != abortAddr)
				Printf("Skipped %x unused bytes of %02x\r\n", (TUint)ptr - (TUint)abortAddr, pattern & 0xff);
			}
		else
			{
			Printf("Skipped %x unused bytes of %02x\r\n", (TUint)ptr - aStart, pattern & 0xff);
			}
		}

	if (ptr < limit)
		DoDiscontiguousMemoryDumpL((TUint)ptr, (limit - ptr) << 2, (TBool)ETrue);
  	}

void Monitor::DoDumpThreadStack(DThread *aThread)
	{
	DMonObject* pO=(DMonObject*)aThread;
	if (!pO || pO->Type()!=EThread)
		{
		return;
		}
	pO->DumpData();

	TUint supSp, usrSp;
	GetStackPointers(&aThread->iNThread, supSp, usrSp);
	
	if (aThread->iUserStackRunAddress && aThread->iUserStackSize)
		{
		TInt usrSize = aThread->iUserStackSize;
		TUint usrRunStart = (TUint)aThread->iUserStackRunAddress;
		TUint usrRunEnd = usrRunStart+usrSize;
		
		Printf("User stack base at %08x, size == %x\r\n", usrRunStart, usrSize);
		Printf("Stack pointer == %08x\r\n", usrSp);

		TUint usrStart = MapAndLocateUserStack(aThread);
		if (usrStart)
			{
			Printf("Stack mapped at %08x\r\n", usrStart);

			TUint usrEnd = usrStart + usrSize;
			if (usrSp >= usrRunStart && usrSp < usrRunEnd)
				usrStart += usrSp - usrRunStart;
			
			DoStackDumpL(usrStart, usrEnd);
			NewLine();
			}
		else
			Printf("Couldn't locate user stack\r\n");
		}
	else
		Printf("No user-mode stack\r\n");

	TInt  supSize = aThread->iSupervisorStackSize;
	TUint supStart = (TUint)aThread->iSupervisorStack;
	TUint supEnd = supStart+supSize;
	
	Printf("Supervisor stack base at %08x, size == %x\r\n", supStart, supSize);
	Printf("Stack pointer == %08x\r\n", supSp);	
	
	if (supSp >= supStart && supSp < supEnd)
		supStart = supSp;
 
	DoStackDumpL(supStart, supEnd);
	}

EXPORT_C void Monitor::DumpThreadStack(DThread *aThread)
	{
	MTRAPD(r,DoDumpThreadStack(aThread));
	if (r!=KErrNone)
		ProcessError(r);
	}

EXPORT_C void Monitor::DumpThreadStacks(TBool aIncludeCurrent)
	{
	DObjectCon* pC=Container(EThread);
	for (TInt i=0 ; i<pC->Count() ; ++i)
		{
		DThread* pT=(DThread*)(*pC)[i];
		if (aIncludeCurrent || pT!=&Kern::CurrentThread())
			{
			DumpThreadStack(pT);
			NewLine();
			}
		}
	}
	

EXPORT_C void Monitor::DumpExceptionStacks()
	{
 	#if defined(__EPOC32__) && !defined(__CPU_X86)
 
 	TStackInfo& stackInfo = Kern::SuperPage().iStackInfo;
 
	Printf("IRQ stack base at %08x, size == %x\r\n", stackInfo.iIrqStackBase, stackInfo.iIrqStackSize);
 	MTRAPD(r1,DoStackDumpL((TLinAddr)stackInfo.iIrqStackBase,
						   (TLinAddr)stackInfo.iIrqStackBase + stackInfo.iIrqStackSize));
 	if(r1!=KErrNone)
 		ProcessError(r1);
 		
	Printf("\r\nFIQ stack base at %08x, size == %x\r\n", stackInfo.iFiqStackBase, stackInfo.iFiqStackSize);
 	MTRAPD(r2,DoStackDumpL((TLinAddr)stackInfo.iFiqStackBase,
						   (TLinAddr)stackInfo.iFiqStackBase + stackInfo.iFiqStackSize));
 	if(r2!=KErrNone)
 		ProcessError(r2);	
 
 	#else
	PrintLine("Not Supported");
 	#endif
 	}
 	
 
void Monitor::DumpProcessData(DProcess* pP)
	{
	Printf("ExitInfo %d,%d,%S\r\n",pP->iExitType,pP->iExitReason,&pP->iExitCategory);
	Printf("Flags %08x, Handles %08x, Attributes %08x\r\n",pP->iFlags,&pP->iHandles,pP->iAttributes);
	Printf("DataBssChunk %08x, CodeSeg %08x\r\n",pP->iDataBssStackChunk,pP->iCodeSeg);
	Printf("DllLock %08x, Process Lock %08x SID %08x\r\n",pP->iDllLock,pP->iProcessLock,pP->iS.iSecureId);
	Printf("TempCodeSeg %08x CodeSeg %08x Capability %08x %08x\r\n",pP->iTempCodeSeg,pP->iCodeSeg,pP->iS.iCaps[1],pP->iS.iCaps[0]);
	TInt c=pP->iDynamicCode.Count();
	Printf("CodeSegs: Count=%d\r\n",c);
	if (c>0)
		{
		SCodeSegEntry* e = &pP->iDynamicCode[0];
		TInt i;
		for(i=0; i<c; ++i, ++e)
			Printf("%2d: seg=%08x lib=%08x\r\n", i, e->iSeg, e->iLib);
		}
	DumpMemModelProcessData(pP);
	DumpCpuProcessData(pP);
	}

void Monitor::DumpMessageQueueData(DMsgQueue* aQueue)
	{
	Printf("StartOfPool %08x, EndOfPool %08x\r\n", aQueue->iMsgPool, aQueue->iEndOfPool);
	Printf("FirstFullSlot %08x, FirstFreeSlot %08x\r\n", aQueue->iFirstFullSlot, aQueue->iFirstFreeSlot);
	Printf("MaxMsgLength %d\r\n", aQueue->iMaxMsgLength);
	TBuf8<80> buf=_L8("MessageQueue state ");
	switch (aQueue->iState)
		{
		case DMsgQueue::EEmpty: buf+=_L8("Empty"); break;
		case DMsgQueue::EPartial: buf+=_L8("Partial"); break;
		case DMsgQueue::EFull: buf+=_L8("Full"); break;
		default: buf+=_L8("????"); break;
		}
	PrintLine(buf);
	Printf("ThreadWaitingForData %08x, DataAvailStatPtr %08x\r\n", 
		   aQueue->iThreadWaitingOnDataAvail, aQueue->iDataAvailRequest->StatusPtr());
	Printf("ThreadWaitingForSpace %08x, SpaceAvailStatPtr %08x\r\n", 
		   aQueue->iThreadWaitingOnSpaceAvail, aQueue->iSpaceAvailRequest->StatusPtr());
	}


void Monitor::DumpObjectData(DMonObject* pO, TInt type)
	{
	if (!pO)
		return;
	pO->DumpData();
	switch(type)
		{
		case EThread: DumpThreadData((DThread*)pO); break;
		case EProcess: DumpProcessData((DProcess*)pO); break;
		case EChunk: DumpChunkData((DChunk*)pO); break;
		case ELibrary: break;
		case ESemaphore: break;
		case EMutex: break;
		case ETimer: break;
		case EServer: break;
		case ESession: break;
		case ELogicalDevice: break;
		case EPhysicalDevice: break;
		case ELogicalChannel: break;
		case EChangeNotifier: break;
		case EUndertaker: break;
		case EMsgQueue: DumpMessageQueueData((DMsgQueue*)pO); break;
		case EPropertyRef: break;
		case ECondVar: break;
		default: break;
		}
	}


DObjectCon* Monitor::Container(TInt anIndex)
	{
	return Kern::Containers()[anIndex];
	}

EXPORT_C void Monitor::DumpObjectContainer(TUint aIndex, TBool aPause)
	{
	if(aIndex>=ENumObjectTypes)
		{
		PrintLine(_L8("Invalid object type"));
		return;
		}
	DObjectCon* pC=Container(aIndex);
	TInt c=pC->Count();
	TBuf8<32> type;
	GetObjectTypeName(type, aIndex, c!=1);
	Printf("Container %d at %08x contains %d %S:\r\n",aIndex,pC,c,&type);
	TInt j;
	for (j=0; j<c; j++)
		{
		DObject* pO=(*pC)[j];
		Monitor::DumpObjectData((DMonObject*)pO,aIndex);
		Pause(aPause);
		}
	NewLine();
	}
		
void Monitor::DisplayDebugMaskInfo()
	{
    for(TInt index=0; index<KNumTraceMaskWords; index++)
    	{
    	Printf("DebugMask[%d] = %08x\r\n", index, Kern::SuperPage().iDebugMask[index]);
    	}
     }

EXPORT_C void Monitor::Pause(TBool aPause)
	{
	}

EXPORT_C void Monitor::ProcessInfoCommand(const TDesC8& aDes, TInt& i)
	{
	DisplaySchedulerInfo();
	DisplayDebugMaskInfo();
	NThread* currentThread = NKern::CurrentThread();
	DMonObject* pO=(DMonObject*)Kern::NThreadToDThread(currentThread);
	if (pO)
		{
		Printf("TheCurrentThread=%08x\r\n",pO);
		Monitor::DumpObjectData(pO,EThread);
		pO=(DMonObject*)&Kern::CurrentProcess();
		Printf("TheCurrentProcess=%08x\r\n",pO);
		Monitor::DumpObjectData(pO,EProcess);
		MMProcessInfoCommand();
		}
	else
		Printf("NCurrentThread=%08x\r\nNThread has no DProcess\r\n",currentThread);

	}

EXPORT_C void Monitor::ProcessError(TInt anError)
	{
	NewLine();
	if (anError==KErrAbort)
		{
		Printf("Exception: Type %d Code %08x Data %08x Extra %08x\r\n",
						iExceptionInfo[3],iExceptionInfo[0],iExceptionInfo[1],iExceptionInfo[2]);
		}
	else if (anError==KErrCancel)
		Print("Escape\r\n");
	}

EXPORT_C Monitor::Monitor()
	{
	}

void Monitor::Init(TAny* aCategory, TInt aReason)
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Calling Monitor 1: %x",TheMonitorPtr));
	TInt restartType = TheMonitorPtr->Init2(aCategory,aReason);
	
	for (TInt i = 0; i < MONITOR_MAXCOUNT && TheAltMonitorPtrs[i]; ++i)
		{
		//Initialise data members of the Alternative Monitor (see Monitor::Entry)
		TheAltMonitorPtrs[i]->iRegs = TheMonitorPtr->iRegs;
		TheAltMonitorPtrs[i]->iPageSize = TheMonitorPtr->iPageSize;

		TheMonitorPtr = TheAltMonitorPtrs[i];
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Calling Monitor %d: %x",i+2,TheMonitorPtr));
		restartType |= TheMonitorPtr->Init2(aCategory,aReason);
		}

	__KTRACE_OPT(KALWAYS,Kern::Printf("All monitors have completed.  Restarting..."));
	//Ensure all characters make it to the serial port...
	__KTRACE_OPT(KALWAYS,Kern::Printf("                                           "));
	if(restartType&ESoftRestart == ESoftRestart)
		Kern::Restart(0);
	else //EHardRestart
		Kern::Restart(0x80000000);
	}

EXPORT_C void Monitor::RegisterMonitorImpl(Monitor* aImpl)
	{
	if(TheMonitorPtr == 0)
		{
		/* TheMonitorPtr should point to the first monitor registered.  Which
		 * will be the first one loaded, which is the first one specified in the
		 * oby file. */
		TheMonitorPtr = aImpl;
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("TheMonitorPtr set to: %x", aImpl));
		/* Once we have a valid TheMonitorPtr we set the Monitor Entry point to
		 * say that there is some sort of crash handling.*/
		Epoc::SetMonitorEntryPoint(Monitor::Entry);
		}
	else
		{
		TInt i;
		for (i = 0; i < MONITOR_MAXCOUNT; ++i)
			{
			if(TheAltMonitorPtrs[i] == 0)
				{
				TheAltMonitorPtrs[i] = aImpl;
				__KTRACE_OPT(KDEBUGGER,Kern::Printf("TheAltMonitorPtrs[%d] set to: %x", i, TheAltMonitorPtrs[i]));
				break;
				}
			}
		if (i == MONITOR_MAXCOUNT)
			{
			Kern::Fault("Too many crash monitors attempted registration", 0);
			}
		}
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Registered Monitor: %x", aImpl));
	}
	
#define GETSEGADD(aSeg,aMember)	aSeg, (aSeg==NULL)?0:_LOFF(aSeg,DCodeSeg,aMember)

EXPORT_C void Monitor::DisplayCodeSeg(DCodeSeg* aSeg,TBool aFull)
	{
	TInt i;
	
	Printf("\r\nCodeSeg at %08x:\r\n",aSeg);
	MTRAP(i,Printf("   FileName: %S\r\n",aSeg->iFileName));
	if (i!=KErrNone)
		ProcessError(i);

	Printf("   RunAddress: %08x\r\n",aSeg->iRunAddress);
	if (aFull)
		{
		Printf("\r\n   iLink:     Prev %08x (%08x) Next %08x (%08x)\r\n",GETSEGADD(aSeg->iLink.iPrev,iLink),GETSEGADD(aSeg->iLink.iNext,iLink));
		Printf("   iTempLink: Prev %08x (%08x) Next %08x (%08x)\r\n",GETSEGADD(aSeg->iTempLink.iPrev,iTempLink),GETSEGADD(aSeg->iTempLink.iNext,iTempLink));
		Printf("   iGbgLink:  Prev %08x (%08x) Next %08x (%08x)\r\n", GETSEGADD(aSeg->iGbgLink.iPrev,iGbgLink),GETSEGADD(aSeg->iGbgLink.iNext,iGbgLink));
		Printf("   iAccessCount: %x\r\n",aSeg->iAccessCount);
		Printf("   iEntryPtVeneer: %08x\r\n",aSeg->iEntryPtVeneer);
		Printf("   iFileEntryPoint: %08x\r\n",aSeg->iFileEntryPoint);
		Printf("   iExtOffset: %x\r\n",aSeg->iExtOffset);
		Printf("   iUids:");
		for (i=0; i<KMaxCheckedUid; i++)
			Printf(" %08x",aSeg->iUids.iUid[i]);
		Printf("\r\n   iDeps: %08x (",aSeg->iDeps);
		if (aSeg->iDepCount<1000) 
			for (i=0; i<aSeg->iDepCount; i++)
				Printf(" %08x",aSeg->iDeps[i]);
			else
				Printf("List not shown due to large number");
		Printf(" )\r\n   iDepCount: %x\r\n",aSeg->iDepCount);
		Printf("   iNextDep: %x\r\n",aSeg->iNextDep);
		Printf("   iMark: %x\r\n",aSeg->iMark);
		Printf("   iAttr: %x\r\n",aSeg->iAttr);
		Printf("   iExeCodeSeg: %08x\r\n",aSeg->iExeCodeSeg);
		Printf("   iAttachProcess: %08x\r\n",aSeg->iAttachProcess);
		Printf("   iModuleVersion: %x\r\n",aSeg->iModuleVersion);
		Printf("   iS:\r\n");
		Printf("      SecureId: %08x, VendorId: %08x\r\n",aSeg->iS.iSecureId,aSeg->iS.iVendorId);
		Printf("      Caps:");
		for (i=0; i<SCapabilitySet::ENCapW; i++)
			Printf(" %08x",aSeg->iS.iCaps[i]);
		Printf("\r\n   iSize: %x\r\n\r\n",aSeg->iSize);
		
		Printf("   iXIP: %x\r\n",((DEpocCodeSeg*)aSeg)->iXIP);
		Printf("   iInfo: %08x ",((DEpocCodeSeg*)aSeg)->iInfo);
		
		if (((DEpocCodeSeg*)aSeg)->iXIP==1)
			{
			Printf("(TRomImageHeader*)\r\n");
				MTRAP(i,DisplayTRomImageHeader((TRomImageHeader*) (((DEpocCodeSeg*)aSeg)->iInfo)))			
			}
		else if (((DEpocCodeSeg*)aSeg)->iXIP==0)
			{
			Printf("(SRamCodeInfo*)\r\n");
			MTRAP(i,DisplaySRamCodeInfo((SRamCodeInfo*) (((DEpocCodeSeg*)aSeg)->iInfo)))			
			}
		else
			{
			Printf("(TAny*)\r\n\r\n");
			i=KErrNone;
			}
			
		if (i!=KErrNone)
			ProcessError(i);

		MDisplayCodeSeg(aSeg);
		}
	}
	
void Monitor::DisplaySRamCodeInfo(SRamCodeInfo* aS)
	{
	Printf("      iCodeSize: %08x\r\n", aS->iCodeSize);
	Printf("      iTextSize: %08x\r\n",aS->iTextSize);
	Printf("      iCodeRunAddr: %08x\r\n",aS->iCodeRunAddr);
	Printf("      iCodeLoadAddr: %08x\r\n",aS->iCodeLoadAddr);
	Printf("      iDataSize: %08x\r\n",aS->iDataSize);
	Printf("      iBssSize: %08x\r\n",aS->iBssSize);
	Printf("      iDataRunAddr: %08x\r\n",aS->iDataRunAddr);
	Printf("      iDataLoadAddr: %08x\r\n",aS->iDataLoadAddr);
	Printf("      iConstOffset: %08x\r\n",aS->iConstOffset);
	Printf("      iExportDir: %08x\r\n",aS->iExportDir);
	Printf("      iExportDirCount: %08x\r\n",aS->iExportDirCount);
	Printf("      iExceptionDescriptor: %08x\r\n\r\n",aS->iExceptionDescriptor);
	}

void Monitor::DisplayTRomImageHeader(TRomImageHeader* aS)
	{
	TInt i;
	Printf("      iUid1: %08x, iUid2: %08x, iUid3: %08x\r\n", aS->iUid1, aS->iUid2, aS->iUid3);		
	Printf("      iUidChecksum: %08x \r\n",aS->iUidChecksum);
	Printf("      iEntryPoint:  %08x \r\n",aS->iEntryPoint);	
	Printf("      iCodeAddress: %08x, iCodeSize: %08x\r\n",aS->iCodeAddress,aS->iCodeSize);
	Printf("      iDataAddress: %08x, iDataSize: %08x\r\n",aS->iDataAddress,aS->iDataSize);
	Printf("      iTextSize:    %08x, iBssSize:  %08x\r\n",aS->iTextSize,aS->iBssSize);	 
	Printf("      iHeapSizeMin: %08x, iHeapSizeMax: %08x, iStackSize: %08x\r\n",aS->iHeapSizeMin,aS->iHeapSizeMax,aS->iStackSize);
	Printf("      iDllRefTable: %08x\r\n",aS->iDllRefTable);
	if (aS->iDllRefTable)
		{
		Printf("          Flags: %04x, NumberOfEntries: %04x\r\n",aS->iDllRefTable->iFlags,aS->iDllRefTable->iNumberOfEntries);
		Printf("          Entry:");
		for (i=0; i<aS->iDllRefTable->iNumberOfEntries; i++)
			Printf(" %08x", aS->iDllRefTable->iEntry[i]);
		Printf("\r\n");
	}
	Printf("      iExportDirCount: %x, iExportDir: %08x\r\n",aS->iExportDirCount, aS->iExportDir);
	Printf("      iS:\r\n");
	Printf("         SecureId: %08x, VendorId: %08x\r\n",aS->iS.iSecureId,aS->iS.iVendorId);
	Printf("         Caps:");
	for (i=0; i<SCapabilitySet::ENCapW; i++)
		Printf(" %08x",aS->iS.iCaps[i]);
	Printf("\r\n      iToolsVersion: Major %02x Minor %02x Build %04x\r\n",aS->iToolsVersion.iMajor, aS->iToolsVersion.iMinor, aS->iToolsVersion.iBuild);
	Printf("      iFlags: %08x\r\n",aS->iFlags);
	Printf("      iPriority: %x\r\n",aS->iPriority);
	Printf("      iDataBssLinearBase: %08x\r\n",aS->iDataBssLinearBase);
	Printf("      iNextExtension: %08x\r\n",aS->iNextExtension);
	Printf("      iHardwareVariant: %08x\r\n",aS->iHardwareVariant);
	Printf("      iTotalDataSize: %08x\r\n",aS->iTotalDataSize);
	Printf("      iModuleVersion: %08x\r\n",aS->iModuleVersion);
	Printf("      iExceptionDescriptor: %08x\r\n\r\n",aS->iExceptionDescriptor);
	}


EXPORT_C void Monitor::DisplayCodeSeg(TBool aFull)
	{
	SDblQue* codeSegList = Kern::CodeSegList();
	Printf("Full CodeSeg List:\r\n\r\n");
		for (SDblQueLink* codeseg= codeSegList->First(); codeseg!=(SDblQueLink*) codeSegList; codeseg=codeseg->iNext)
			{
				DisplayCodeSeg(_LOFF(codeseg,DCodeSeg, iLink),aFull);
			}	
	}
GLDEF_C TInt KernelModuleEntry(TInt aReason)
	{
	return KErrNone;
	}


/**
Time of system crash.

@return Time of crash in seconds since year 0 (nominal Gregorian), or zero if crash time un-available.
*/
EXPORT_C TInt64 CrashTime()
	{
	TInt64 secsSince0AD = 0;
	MTRAPD(res, secsSince0AD = (Kern::SystemTime() + 999999) / 1000000);
	return secsSince0AD;
	}


