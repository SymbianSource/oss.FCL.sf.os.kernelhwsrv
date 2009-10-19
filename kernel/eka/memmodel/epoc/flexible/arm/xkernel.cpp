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
//

#include "arm_mem.h"
#include "../mmu/mm.h"

TInt DArmPlatThread::SetupContext(SThreadCreateInfo& aInfo)
	{
	switch(iThreadType)
		{
		case EThreadSupervisor:
		case EThreadMinimalSupervisor:
		case EThreadInitial:
		case EThreadAPInitial:
			break;
		case EThreadUser:
#ifndef __SMP__
			iNThread.iSpare3 /*iUserContextType*/ = NThread::EContextUndefined;
#endif
			break;
		}
	iNThread.SetAddressSpace(iOwningProcess);
	iNThread.SetAttributes(KThreadAttAddressSpace);
#ifdef __SMP__
	iCpuRestoreCookie = -1;
#else
	// OK to get this thread's owning process os asid as the process can't free
	// it's asid while this thread is being created because the current thread
	// has the same owning process.
	MM::IpcAliasPde(iAliasPdePtr,((DMemModelProcess*)iOwningProcess)->OsAsid());
#endif
	return KErrNone;
	}

TIpcExcTrap::TExcLocation TIpcExcTrap::ExcLocation(DThread* aThread, TAny* aContext)
	{
	TArmExcInfo& info=*(TArmExcInfo*)aContext;
	if (info.iExcCode==EArmExceptionDataAbort)
		{
		TLinAddr va=(TLinAddr)info.iFaultAddress;

		TLinAddr aliasAddr = ((DMemModelThread*)aThread)->iAliasLinAddr;
		TBool remoteError;
		if(aliasAddr)
			remoteError = TUint(va^aliasAddr)<TUint(KPageSize);
		else
			// The second clause in the statement below was "va < iRemoteBase + iSize".
			// iRemoteBase + iSize might conceivably wrap round.
			// The usual fix for this is to change
			//			va >= base && va < base + size
			// to		va >= base && (va - base) < size
			// but this requires the first clause (va >= base) so that va-base doesn't wrap negative.
			// Since the first clause in this expression is va >= (iRemoteBase & ~3)
			// we have to re-write the expression as follows:
			// Let base' = iRemoteBase & ~3
			// so  base  = base' + (base & 3)
			// then we have va >= base' && va < base' + (base & 3) + iSize
			// (effectively the & ~3 on the first clause extends the range downwards by base & 3)
			remoteError = va>=(iRemoteBase&~3) &&
							(va - (iRemoteBase & ~3)) < iSize + (iRemoteBase & 3);
		if (remoteError)
			return EExcRemote;

		// Third clause was va < iLocalBase + iSize, fixed as in the "remoteError =" line above
		if (iLocalBase && va>=(iLocalBase&~3) &&
			(va - (iLocalBase & ~3)) < iSize + (iLocalBase & 3))
			return EExcLocal;
		}
	return EExcUnknown;
	}
