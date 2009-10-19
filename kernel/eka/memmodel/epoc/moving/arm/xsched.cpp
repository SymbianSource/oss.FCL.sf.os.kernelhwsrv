// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\moving\arm\xsched.cpp
// 
//

#include "arm_mem.h"

#define iMState		iWaitLink.iSpare1

//#define __DEBUG_BAD_ADDR

#define iCurrentVMProcess				iExtras[0]
#define	iCurrentDataSectionProcess		iExtras[1]
#define	iCompleteDataSectionProcess		iExtras[2]

#ifdef __SCHEDULER_MACHINE_CODED__

#if defined(_DEBUG)
extern "C" void __DebugMsgFixedAddress()
	{
	__KTRACE_OPT(KSCHED2,Kern::Printf("Fixed Address process"));
	}

extern "C" void __DebugMsgMoving()
	{
	__KTRACE_OPT(KSCHED2,Kern::Printf("Moving Chunk process"));
	}

extern "C" void __DebugMsgProtectChunks(int aProc)
	{
	DProcess *pP=(DProcess*)aProc;
	__KTRACE_OPT(KSCHED2,Kern::Printf("%O->Protect Chunks",pP));
	}

extern "C" void __DebugMsgUnprotectChunks(int aProc)
	{
	DProcess *pP=(DProcess*)aProc;
	__KTRACE_OPT(KSCHED2,Kern::Printf("%O->Unprotect Chunks",pP));
	}

extern "C" void __DebugMsgMoveChunksToHome(int aProc)
	{
	DProcess *pP=(DProcess*)aProc;
	__KTRACE_OPT(KSCHED2,Kern::Printf("%O->MoveChunksToHomeSection",pP));
	}

extern "C" void __DebugMsgMoveChunksToData(int aProc)
	{
	DProcess *pP=(DProcess*)aProc;
	__KTRACE_OPT(KSCHED2,Kern::Printf("%O->MoveChunksToDataSection",pP));
	}

extern "C" void __DebugMsgMoveChunkToHomeAddress(int aChunk)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
	__KTRACE_OPT(KSCHED2,Kern::Printf("MoveToHomeSection, initaddr %08X size %08X final addr %08X",pC->Base(),pC->Size(),pC->iHomeRegionBase));
	}

extern "C" void __DebugMsgProtectChunk(int aChunk)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
	__KTRACE_OPT(KSCHED2,Kern::Printf("ProtectChunk, base %08X size %08X",pC->Base(),pC->Size()));
	}

extern "C" void __DebugMsgMoveChunkToRunAddress(int aChunk, int aRunAddr)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
	__KTRACE_OPT(KSCHED2,Kern::Printf("MoveToRunAddress, base %08X size %08X run address %08X",pC->Base(),pC->Size(),aRunAddr));
	}

extern "C" void __DebugMsgUnprotectChunk(int aChunk)
	{
	DMemModelChunk* pC=(DMemModelChunk*)aChunk;
	__KTRACE_OPT(KSCHED2,Kern::Printf("UnprotectChunk, base %08X size %08X",pC->Base(),pC->Size()));
	}

/*************************/

extern "C" void __DebugMsgDThreadRequestComplete(TInt a0, TInt a1)
	{
	DThread* pT=(DThread*)a0;
	__KTRACE_OPT(KDATAPAGEWARN,Kern::Printf("Data paging: Use of deprecated DThread::RequestComplete API by %O at %08x", pT, a1));
	}

extern "C" void __DebugMsgRequestComplete(TInt a0, TInt a1, TInt a2)
	{
	DThread* pT=(DThread*)a0;
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O RequestComplete %08x %d",pT,a1,a2));
	__KTRACE_OPT(KDATAPAGEWARN,Kern::Printf("Data paging: Use of deprecated Kern::RequestComplete API by %O", TheCurrentThread));
	}

extern "C" void __DebugMsgReqCompleteWrite(TInt a0, TInt a1, TInt a2)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Writing %d to %08x",a2,a0+a1));
	}
#endif
#endif

