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
// e32\memmodel\epoc\direct\x86\xkernel.cpp
// 
//

#include <x86_mem.h>

/********************************************
 * Thread
 ********************************************/

TInt DX86PlatThread::SetupContext(SThreadCreateInfo& anInfo)
	{
	return KErrNone;
	}

DX86PlatProcess::DX86PlatProcess()
	{}

DX86PlatProcess::~DX86PlatProcess()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DX86PlatProcess destruct"));
	DMemModelProcess::Destruct();
	}

TBool Exc::IsMagic(TLinAddr /*anAddress*/)
//
// Return TRUE if anAddress is a 'magic' exception handling instruction
//
	{
	return EFalse;
	}

void DThread::IpcExcHandler(TExcTrap* aTrap, DThread* aThread, TAny* aContext)
	{
	aThread->iIpcClient = 0;
	TIpcExcTrap& xt=*(TIpcExcTrap*)aTrap;
	TX86ExcInfo& info=*(TX86ExcInfo*)aContext;
	if (info.iExcId==EX86VectorPageFault)
		{
		TLinAddr va=(TLinAddr)info.iFaultAddress;
		if (va>=xt.iRemoteBase && (va-xt.iRemoteBase)<xt.iSize)
			xt.Exception(KErrBadDescriptor);	// problem accessing remote address - 'leave' so an error code will be returned
		if (xt.iLocalBase && va>=xt.iLocalBase && (va-xt.iLocalBase)<xt.iSize)
			NKern::UnlockSystem();		// problem accessing local address - return and panic current thread as usual
		}
	// otherwise return and fault kernel
	}

