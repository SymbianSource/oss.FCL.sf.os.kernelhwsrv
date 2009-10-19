// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\arm\cscmdatasave.cpp
// SCM - Arm portion
// 
//

#define __INCLUDE_REG_OFFSETS__  // for SP_R13U in nk_plat.h


#include "arm_mem.h"
#include "nk_plat.h"
#include <scmdatatypes.h>
#include <scmdatasave.h>


/**
 * Reads the CPU registers at the time of the crash
 * @param aRegs struct to store the resulting CPU registers
 */
void SCMDataSave::ReadCPURegisters(SFullArmRegSet& aRegs)
	{
	aRegs =*(SFullArmRegSet*)iMonitor->iRegs;
	}

/**
 * Reads the user registers for a given thread - may not be the current thread
 * @param aThread thread whose registers we want to read
 * @param aRegs registers will be written here if available
 * @return KErrArgument if aThread is the current thread or any of the system wide error codes
 */
TInt SCMDataSave::ReadUserRegisters(DThread* aThread, TArmRegSet& aRegs, TUint32& aAvailableRegs)
	{
	TFileName filename;
	aThread->TraceAppendFullName(filename, EFalse);	
	
	//we retrieve the registers differently for the current thread
	if(aThread == &Kern::CurrentThread())
		{
		return KErrArgument;
		}
	
	TUint32* stackPointer = (TUint32*)aThread->iNThread.iSavedSP; //Still need to check pointer somehow
	TUint32* stackTop = (TUint32*)((TUint32)aThread->iNThread.iStackBase +(TUint32)aThread->iNThread.iStackSize);
	TArmReg* out = (TArmReg*)(&aRegs);		
	
	//Get a pointer to this threads context table
	NThread::TUserContextType type = aThread->iNThread.UserContextType();
	const TArmContextElement* table = aThread->iNThread.UserContextTables()[type];
	
	aAvailableRegs = 0;
	for(TInt i = 0; i<KArmRegisterCount; ++i)
		{
		TInt value = table[i].iValue;
		TInt type = table[i].iType;
		
		if(type == TArmContextElement::EOffsetFromSp)
			{
			value = stackPointer[value];	
			aAvailableRegs |= (1<<i);
			}
		else if(type == TArmContextElement::EOffsetFromStackTop)
			{
			value = stackTop[-value];
			aAvailableRegs |= (1<<i);
			}
		else if(type == TArmContextElement::ESpPlusOffset)
			{
			value = (TInt)(stackPointer + value);
			aAvailableRegs |= (1<<i);
			}
		
		out[i] = value;
		}
	
	return KErrNone;
	}

/**
 * Reads the system registers for a given thread
 * Can not be used on the current thread
 * @param aThread
 * @param aRegs
 * @param aAvailableRegs
 * @return KErrArgument if aThread is the current thread or any of the system wide error codes
 */
TInt SCMDataSave::ReadSystemRegisters(DThread* aThread, TArmRegSet& aRegs, TUint32& aAvailableRegs)
	{	
	if(aThread == &Kern::CurrentThread())
		{
		return KErrArgument;
		}
	
	TFileName filename;
	aThread->TraceAppendFullName(filename, EFalse);
	
	TUint32* stackPointer = (TUint32*)aThread->iNThread.iSavedSP;
	TUint32* stackTop = (TUint32*)((TUint32)aThread->iNThread.iStackBase +(TUint32)aThread->iNThread.iStackSize);
	TArmReg* out = (TArmReg*)(&aRegs);		
	
	//Get a pointer to this threads context table
	const TArmContextElement* table = aThread->iNThread.UserContextTables()[NThread::EContextKernel];
	
	aAvailableRegs = 0;
	for(TInt i = 0; i<KArmRegisterCount; ++i)
		{
		TInt value = table[i].iValue;
		TInt type = table[i].iType;
		
		if(type == TArmContextElement::EOffsetFromSp)
			{
			//ensure we are still on the stack
			if(stackPointer + value >= stackTop)
				continue;
				
			value = stackPointer[value];	
			aAvailableRegs |= (1<<i);
			}
		else if(type == TArmContextElement::EOffsetFromStackTop)
			{
			//ensure we are still on the stack
			if(stackTop - value < (TUint32*)aThread->iNThread.iStackBase)
				continue;
			
			value = stackTop[-value];
			aAvailableRegs |= (1<<i);
			}
		else if(type == TArmContextElement::ESpPlusOffset)
			{
			value = (TInt)(stackPointer + value);
			aAvailableRegs |= (1<<i);
			}
		
		out[i] = value;
		}	
	
	return KErrNone;
	}

//EOF

