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
// e32\memmodel\epoc\direct\arm\xkernel.cpp
// 
//

#include "arm_mem.h"

/********************************************
 * Thread
 ********************************************/

TInt DArmPlatThread::SetupContext(SThreadCreateInfo& anInfo)
	{
#ifndef __SMP__
	if(iThreadType==EThreadUser)
		iNThread.iSpare3 /*iUserContextType*/ = NThread::EContextUndefined;
#endif
	return KErrNone;
	}

DArmPlatProcess::DArmPlatProcess()
	{}

DArmPlatProcess::~DArmPlatProcess()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DArmPlatProcess destruct"));
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
	TArmExcInfo& info=*(TArmExcInfo*)aContext;
	if (info.iExcCode==EArmExceptionDataAbort)
		{
		TUint32 opcode = *(TUint32*)info.iR15;		// faulting instruction
		if (opcode>=0xf0000000)
			return;									// not a load/store so fault the kernel
		TUint32 opc1 = (opcode>>25)&7;
		TBool load;
		if ( (opc1==2) || (opc1==4) || (opc1==3 && !(opcode&0x10)) )
			load=opcode & 0x00100000;				// bit 20=1 for load, 0 for store
		else if (opc1==0)
			{
			TUint32 opc2 = (opcode>>4)&0xf;
			if (opc2==0x0b)
				load=opcode & 0x00100000;			// bit 20=1 for load, 0 for store
			else if ((opc2&0x0d)==0x0d)
				{
				if (opcode&0x00100000)
					load=ETrue;						// LDRSB/LDRSH
				else
					load=(opcode&0x20)^0x20;		// LDRD/STRD, bit5=1 for STRD, 0 for LDRD
				}
			else
				return;								// not a load/store so fault the kernel
			}
		else
			return;									// not a load/store so fault the kernel

		if ((load && !xt.iDir) || (!load && xt.iDir))
			xt.Exception(KErrBadDescriptor);	// problem accessing remote address - 'leave' so an error code will be returned
		NKern::UnlockSystem();			// else assume problem accessing local address - return and panic current thread as usual
		}
	// otherwise return and fault kernel
	}

