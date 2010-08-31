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
// e32\kernel\sutils.cpp
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"
#include <e32panic.h>
_LIT(KLitDfcThread,"DfcThread");

extern const SNThreadHandlers EpocThreadHandlers;



/**
Adds a HAL entry handling function for the specified group of HAL entries.

@param	aId   The HAL group attribute that this function handles, as defined by
              one of the THalFunctionGroup enumerators.
@param	aFunc Pointer to the handler function
@param	aPtr  Pointer which is passed to the handler function when it is
              called. This is usually a pointer to an object which handles
              the HAL attribute.

@return KErrNone, if successful; KErrArgument if aId is EHalGroupKernel, EHalGroupVariant or EHalGroupPower,
or aId is greater than or equal to KMaxHalGroups; KErrInUse, if a handler is already registered.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Suitable for use in a device driver.

@see THalFunctionGroup
@see KMaxHalGroups
*/
EXPORT_C TInt Kern::AddHalEntry(TInt aId, THalFunc aFunc, TAny* aPtr)
	{
	return Kern::AddHalEntry(aId, aFunc, aPtr, 0);
	}

/**
Adds a HAL entry handling function for the specified group of HAL entries.

@param	aId   The HAL group attribute that this function handles, as defined by
              one of the THalFunctionGroup enumerators.
@param	aFunc Pointer to the handler function
@param	aPtr  Pointer which is passed to the handler function when it is
              called. This is usually a pointer to an object which handles
              the HAL attribute.
@param  aDeviceNumber
			  The device number (eg. screen number).

@return KErrNone, if successful; KErrArgument if aId is EHalGroupKernel, EHalGroupVariant or EHalGroupPower,
or aId is greater than or equal to KMaxHalGroups; KErrInUse, if a handler is already registered.

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Suitable for use in a device driver.

@see THalFunctionGroup
@see KMaxHalGroups
*/
EXPORT_C TInt Kern::AddHalEntry(TInt aId, THalFunc aFunc, TAny* aPtr, TInt aDeviceNumber)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::AddHalEntry(TInt aId, THalFunc aFunc, TAny* aPtr, TInt aDeviceNumber)");			
	__KTRACE_OPT(KEXTENSION,Kern::Printf("Kern::AddHalEntry %d %08x %08x",aId,aFunc,aPtr));
	if (aId==(TInt)EHalGroupKernel || aId==(TInt)EHalGroupVariant || aId==(TInt)EHalGroupPower || aId>=KMaxHalGroups || (TUint)aDeviceNumber>=(TUint)KMaxHalEntries)
		return KErrArgument;
	TInt r=KErrInUse;
	if (aDeviceNumber>0)
		{
		TBool delete_entry = EFalse;
		NKern::LockSystem();
		SHalEntry2* p = &K::HalEntryArray[aId];
		SHalEntry* extended_entry = p->iExtendedEntries;
		if(!extended_entry)
			{
			NKern::UnlockSystem();
			extended_entry = (SHalEntry*)Kern::AllocZ((KMaxHalEntries-1)*sizeof(SHalEntry));
			if(!extended_entry)
				return KErrNoMemory;
			NKern::LockSystem();
			if(!p->iExtendedEntries)
				p->iExtendedEntries = extended_entry;
			else
				delete_entry = ETrue;
			}
		if(!extended_entry[aDeviceNumber-1].iFunction)
			{
			extended_entry[aDeviceNumber-1].iFunction = aFunc;
			extended_entry[aDeviceNumber-1].iPtr = aPtr;
			r = KErrNone;
			}
		NKern::UnlockSystem();
		if(delete_entry)
			Kern::Free(extended_entry);
		}
	else
		{
		NKern::LockSystem();
		SHalEntry2& e=K::HalEntryArray[aId];
		if (!e.iFunction)
			{
			e.iFunction=aFunc;
			e.iPtr=aPtr;
			r=KErrNone;
			}
		NKern::UnlockSystem();
		}
	__KTRACE_OPT(KEXTENSION,Kern::Printf("Kern::AddHalEntry returns %d",r));
	return r;
	}



/**
Removes a HAL entry handling function for the specified group of HAL entries.

@param	aId The HAL group attribute, as defined by one of the THalFunctionGroup
            enumerators, for which the handler function is to be removed.

@return KErrNone, if successful; KErrArgument if aId is EHalGroupKernel,
                  EHalGroupVariant or EHalGroupMedia, or aId is greater than
                  or equal KMaxHalGroups.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see THalFunctionGroup
@see KMaxHalGroups
*/
EXPORT_C TInt Kern::RemoveHalEntry(TInt aId)
	{
	return Kern::RemoveHalEntry(aId,0);
	}

/**
Removes a HAL entry handling function for the specified group of HAL entries.

@param	aId The HAL group attribute, as defined by one of the THalFunctionGroup
            enumerators, for which the handler function is to be removed.
@param  aDeviceNumber The device number (eg. screen number)

@return KErrNone, if successful; KErrArgument if aId is EHalGroupKernel,
                  EHalGroupVariant or EHalGroupMedia, or aId is greater than
                  or equal KMaxHalGroups.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see THalFunctionGroup
@see KMaxHalGroups
*/
EXPORT_C TInt Kern::RemoveHalEntry(TInt aId, TInt aDeviceNumber)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::RemoveHalEntry(TInt aId, TInt aDeviceNumber)");			
	__KTRACE_OPT(KEXTENSION,Kern::Printf("Kern::RemoveHalEntry %d %d",aId,aDeviceNumber));
	if (aId<(TInt)EHalGroupPower || aId>=KMaxHalGroups || (TUint)aDeviceNumber>=(TUint)KMaxHalEntries)
		return KErrArgument;
	NKern::LockSystem();
	SHalEntry2* pE=&K::HalEntryArray[aId];
	if(aDeviceNumber>0)
		{
		SHalEntry* pBase=pE->iExtendedEntries;
		if(pBase)
			{
			pBase[aDeviceNumber-1].iFunction=NULL;
			pBase[aDeviceNumber-1].iPtr=NULL;
			}
		}
	else
		{		
		pE->iFunction=NULL;
		pE->iPtr=NULL;
		}
	NKern::UnlockSystem();
	return KErrNone;
	}

/**
Gets the HAL entry handling function for the specified group of HAL entries.

@param	aId The HAL group attribute, as defined by one of the THalFunctionGroup
            enumerators, for which the handler function is required.

@return A pointer to handler information containing the handler function; NULL
        if aId is negative or is greater than or equal to KMaxHalGroups, or no
        handler function can be found.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see THalFunctionGroup
@see KMaxHalGroups
*/
EXPORT_C SHalEntry* Kern::FindHalEntry(TInt aId)
	{
	return Kern::FindHalEntry(aId,0);
	}


/**
Gets the HAL entry handling function for the specified group of HAL entries.

@param	aId The HAL group attribute, as defined by one of the THalFunctionGroup
            enumerators, for which the handler function is required.
@param  aDeviceNumber The device number (eg. screen number)

@return A pointer to handler information containing the handler function; NULL
        if aId is negative or is greater than or equal to KMaxHalGroups, or no
        handler function can be found.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see THalFunctionGroup
@see KMaxHalGroups
*/
EXPORT_C SHalEntry* Kern::FindHalEntry(TInt aId, TInt aDeviceNumber)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::FindHalEntry(TInt aId, TInt aDeviceNumber)");			
	__KTRACE_OPT(KEXTENSION,Kern::Printf("Kern::FindHalEntry %d %d",aId,aDeviceNumber));
	if (aId<0 || aId>=KMaxHalGroups || TUint(aDeviceNumber)>=TUint(KMaxHalEntries))
		return NULL;
	SHalEntry2* p=&K::HalEntryArray[0]+aId;
	SHalEntry* pBase=(SHalEntry*)p;
	if(aDeviceNumber>0)
		{
		if(p->iExtendedEntries)
			pBase=p->iExtendedEntries + (aDeviceNumber-1);
		}
	if(!pBase->iFunction)
		return NULL;
	return pBase;
	}




/**
Returns the active debug mask obtained by logically ANDing the global debug mask
in the super page with the per-thread debug mask in the current DThread object.

If the current thread is not a symbian OS thread the global debug mask is used.

Only supports the first 32 global debug trace bits.

@return The debug mask.
*/
EXPORT_C TInt KDebugMask()
	{
	TInt m=TheSuperPage().iDebugMask[0];
	NThread* nt = NCurrentThread();
	if (nt && nt->iHandlers==&EpocThreadHandlers)
		m &= TheCurrentThread->iDebugMask;
	return m;
	}



/**
Returns the state (ETrue or EFalse) of given bit in the active debug mask
which is obtained by logically ANDing the global debug mask in the super page 
with the per-thread debug mask in the current DThread object.

If the current thread is not a symbian OS thread the global debug mask is used.

@return The state of the debug mask bit number.
*/

EXPORT_C TBool KDebugNum(TInt aBitNum)
	{
	TInt m = 0;

	// special case for KALWAYS
	if (aBitNum == KALWAYS)
		{
		m = TheSuperPage().iDebugMask[0] ||
			TheSuperPage().iDebugMask[1] ||
			TheSuperPage().iDebugMask[2] ||
			TheSuperPage().iDebugMask[3] ||
			TheSuperPage().iDebugMask[4] ||
			TheSuperPage().iDebugMask[5] ||
			TheSuperPage().iDebugMask[6] ||
		    TheSuperPage().iDebugMask[7];
		}
	else if  ( (aBitNum > KMAXTRACE) || (aBitNum < 0) )
		m = 0;
	else
		{
		TInt index = aBitNum >> 5;
		m = TheSuperPage().iDebugMask[index];
		m &= 1 << (aBitNum & 31);
		if (!index)
			{
			// if index is zero then AND in the per thread debug mask
			NThread* nt = K::Initialising ? 0 : NCurrentThread();
			if (nt && nt->iHandlers==&EpocThreadHandlers)
				m &= TheCurrentThread->iDebugMask;
			}
		}

	return (m != 0);
	}


/**
Prints a formatted string on the debug port.

The function uses Kern::AppendFormat() to do the formatting.

Although it is safe to call this function from an ISR, it polls the output
serial port and may take a long time to complete, invalidating any
real-time guarantee.

If called from an ISR, it is possible for output text to be intermingled
with other output text if one set of output interrupts or preempts another.

Some of the formatting options may not work inside an ISR.

Be careful not to use a string that is too long to fit onto the stack.

@param aFmt The format string. This must not be longer than 256 characters.
@param ...	A variable number of arguments to be converted to text as dictated
            by the format string.

@pre Calling thread can either be in a critical section or not.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked
@pre Call in any context.
@pre Suitable for use in a device driver

@see Kern::AppendFormat()
*/

EXPORT_C void Kern::Printf(const char* aFmt, ...)
	{
	TBuf8<256> printBuf;
	VA_LIST list;
	VA_START(list,aFmt);
	Kern::AppendFormat(printBuf,aFmt,list);
	K::TextTrace(printBuf,EKernelTrace);
	}

void AppendNumBuf(TDes8& aDes, const TDesC8& aNum, TInt width, char fill)
	{
	TInt l=aNum.Length();
	for (; l<width; ++l)
		aDes.Append(TChar(fill));
	aDes.Append(aNum);
	}




/**
Formats and appends text to the specified narrow descriptor without making any
executive calls.

The function takes a format string and a variable number of arguments. The
format specifiers in the format string are used to interpret and the arguments.

Format directives have the following syntax:
@code
<format-directive> ::= 
	"%" [<padding-character>] [<field-width>] [<long-flag>] <conversion-specifier>
@endcode

If a field width is specified and the width of the formatted field is less
than this width, then the field is padded with the padding character.
The only supported padding characters are ' ' (default) and '0'.

The long flag specifier ('l') modifies the semantic of the conversion
specifier as explained below.

The possible values for the conversion specifiers, the long flag and the way in
which arguments are interpreted, are as follows:
@code
d	Interpret the argument as a TInt decimal representation
ld	NOT SUPPORTED - use lx instead
u	Interpret the argument as a TUint decimal representation
lu	NOT SUPPORTED - use lx instead
x	Interpret the argument as a TUint hexadecimal representation
X	As above
lx	Interpret the argument as a Uint64 hexadecimal representation
lX	As above
c	Interpret the argument as a character
s	Interpret the argument as a pointer to narrow C string
ls	Interpret the argument as a pointer to narrow C string
S 	Interpret the argument as a pointer to narrow descriptor or NULL
lS	NOT SUPPORTED - use S instead
O	Interpret the argument as a pointer to DObject or NULL 
	Generates the object full name or 'NULL'
o	Interpret the argument as a pointer to DObject or NULL
	Generates the object name or 'NULL'
M	Interpret the argument as a pointer to a fast mutex or NULL
	Generates the name, if this is a well-known fast mutex, address otherwise
m	Interpret the argument as a pointer to a fast semaphore or NULL
	Generates the owning thread name, if this is a well-known fast semaphore, address otherwise
T	Interpret the argument as a pointer to a nanothread or NULL 
	Generates the full name, if this is a Symbian OS thread, address otherwise
C	Interpret the argument as a pointer to a DCodeSeg or NULL
	Generates the filename and module version number
G	Interpret the argument as a pointer to a nanothread group or NULL 
	Generates the full name if this corresponds to a Symbian OS process, address otherwise
@endcode

The function can be called from the interrupt context, but extreme caution is advised as it
may require a lot of stack space and interrupt stacks are very small.

@param aDes 	Narrow descriptor that must be big-enough to hold result
@param aFmt 	The format string
@param aList 	A variable number of arguments to be converted to text as dictated by the format string

@pre Calling thread can be either in a critical section or not.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked
@pre Call in any context.
@pre Suitable for use in a device driver

@panic The set of panics that can be raised when appending data to descriptors.

@see   TDes8
*/
EXPORT_C void Kern::AppendFormat(TDes8& aDes, const char* aFmt, VA_LIST aList)
	{
	
#define NEXT_FMT(c,p) if (((c)=*(p)++)==0) return
	_LIT8(NullDescriptor,"(null)");
	_LIT8(KLitNULL,"NULL");
	_LIT8(KLitSysLock,"SysLock");
	_LIT8(KLitObjLock,"ObjLock");
	_LIT8(KLitMsgLock,"MsgLock");
	_LIT8(KLitLogonLock,"LogonLock");
	_LIT8(KLitMiscNtfMgrLock,"MiscNtfMgrLock");

	TBuf8<24> NumBuf;
	FOREVER
		{
		char c;
		NEXT_FMT(c,aFmt);
		if (c=='%')
			{
			char fill=' ';
			TInt width=0;
			TBool long_arg=EFalse;
			TBool ok=ETrue;
			NEXT_FMT(c,aFmt);
			if (c=='0')
				{
				fill='0';
				NEXT_FMT(c,aFmt);
				}
			while(c>='0' && c<='9')
				{
				width=width*10+c-'0';
				NEXT_FMT(c,aFmt);
				}
			if (c=='l')
				{
				long_arg=ETrue;
				NEXT_FMT(c,aFmt);
				}
			switch(c)
				{
				case 'd':
					{
					if (long_arg)
						ok=EFalse;
					else
						{
						TInt val=VA_ARG(aList,TInt);
						NumBuf.Num(val);
						AppendNumBuf(aDes,NumBuf,width,fill);
						}
					break;
					}
				case 'u':
					{
					if (long_arg)
						ok=EFalse;
					else
						{
						TUint val=VA_ARG(aList,TUint);
						NumBuf.Num(val,EDecimal);
						AppendNumBuf(aDes,NumBuf,width,fill);
						}
					break;
					}
				case 'x':
				case 'X':
					{
					if (long_arg)
						{
						Uint64 val=VA_ARG(aList,Uint64);
						TUint vl=(TUint)val;
						TUint vh=(TUint)(val>>32);
						if (vh)
							{
							NumBuf.Num(vh,EHex);
							NumBuf.AppendNumFixedWidth(vl,EHex,8);
							}
						else
							{
							NumBuf.Num(vl,EHex);
							}
						}
					else
						{
						TUint val=VA_ARG(aList,TUint);
						NumBuf.Num(val,EHex);
						}
					AppendNumBuf(aDes,NumBuf,width,fill);
					break;
					}
				case 'S':
				case 's':
					{
					TPtrC8 ptrc8;
					const TDesC *pS=VA_ARG(aList,const TDesC*);
					if (c=='s')
						{
							ptrc8.Set((const TUint8*)pS), pS=(const TDesC*)&ptrc8;
						}
					if (pS)
						{
							AppendNumBuf(aDes,*(const TDesC8*)pS,width,fill);
						}
					else
						aDes.Append(NullDescriptor);
					break;
					}
				case 'O':
					{
					DObject* pO=VA_ARG(aList,DObject*);
					if (pO)
						pO->TraceAppendFullName(aDes,ETrue);
					else
						aDes.Append(KLitNULL);
					break;
					}
				case 'o':
					{
					DObject* pO=VA_ARG(aList,DObject*);
					if (pO)
						pO->TraceAppendName(aDes,ETrue);
					else
						aDes.Append(KLitNULL);
					break;
					}
				case 'M':		// fast mutex
					{
					NFastMutex* pM=VA_ARG(aList,NFastMutex*);
					if (!pM)
						aDes.Append(KLitNULL);
					else if (pM==&TheScheduler.iLock)
						aDes.Append(KLitSysLock);
					else if (pM==&DObject::Lock)
						aDes.Append(KLitObjLock);
					else if (pM==&TMessageQue::MsgLock)
						aDes.Append(KLitMsgLock);
					else if (pM==&TLogon::LogonLock)
						aDes.Append(KLitLogonLock);
					else if (pM==&K::TheMiscNotifierMgr.iLock)
						aDes.Append(KLitMiscNtfMgrLock);
					else
						aDes.AppendNumFixedWidth((TUint)pM,EHex,8);
					break;
					}
				case 'm':		// fast semaphore
					{
					NFastSemaphore* pS=VA_ARG(aList,NFastSemaphore*);
					if (!pS)
						aDes.Append(KLitNULL);
					else
						{
						// following commented out because pointers may end up referencing non-existent memory...
/*
						DThread* pT1=_LOFF(pS,DThread,iNThread.iRequestSemaphore);
						DThread* pT2=_LOFF(pS,DThread,iKernMsg.iSem);
						if (pT1->iNThread.iHandlers==&EpocThreadHandlers)
							pT1->TraceAppendFullName(aDes,ETrue);
						else if (pT2->iNThread.iHandlers==&EpocThreadHandlers)
							pT2->TraceAppendFullName(aDes,ETrue);
						else
*/							aDes.AppendNumFixedWidth((TUint)pS,EHex,8);
						}
					break;
					}
				case 'T':		// NKERN thread
					{
					NThread* pN=VA_ARG(aList,NThread*);
					if (!pN)
						aDes.Append(KLitNULL);
					else if (pN->iHandlers==&EpocThreadHandlers)
						{
						DThread* pT=_LOFF(pN,DThread,iNThread);
						pT->TraceAppendFullName(aDes,ETrue);
						}
					else
						aDes.AppendNumFixedWidth((TUint)pN,EHex,8);
					break;
					}
				case 'C':
					{
					DCodeSeg* pO=VA_ARG(aList,DCodeSeg*);
					if (pO)
						pO->TraceAppendFullName(aDes);
					else
						aDes.Append(KLitNULL);
					break;
					}
#ifdef __SMP__
				case 'G':		// NKERN thread group
					{
					NThreadGroup* pG=VA_ARG(aList,NThreadGroup*);
					if (!pG)
						aDes.Append(KLitNULL);
//					else if (pN->iHandlers==&EpocThreadHandlers)
//						{
//						DThread* pT=_LOFF(pN,DThread,iNThread);
//						pT->TraceAppendFullName(aDes,ETrue);
//						}
					else
						aDes.AppendNumFixedWidth((TUint)pG,EHex,8);
					break;
					}
#endif
				case 'c':
					c=(char)VA_ARG(aList,TUint);
					// fall through
				default:
					ok=EFalse;
					break;
				}
				if (ok)
					continue;
			}
		aDes.Append(TChar(c));
		}
	}

#if 0
void DumpMemoryLine(TLinAddr a)
	{
	const TUint8* p = (const TUint8*)a;
	TUint8 c[16];
	TInt i;
	for (i=0; i<16; ++i)
		{
		TUint8 x = p[i];
		if (x<0x21 || x>0x7e)
			x = 0x2e;
		c[i] = (TUint8)x;
		}
	Kern::Printf("%08x: %02x %02x %02x %02x %02x %02x %02x %02x "
					   "%02x %02x %02x %02x %02x %02x %02x %02x "
					   "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
		a,	p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
			p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15],
			c[ 0], c[ 1], c[ 2], c[ 3], c[ 4], c[ 5], c[ 6], c[ 7],
			c[ 8], c[ 9], c[10], c[11], c[12], c[13], c[14], c[15]
		);
	}

void DumpMemory(const char* aTitle, TLinAddr aStart, TLinAddr aSize)
	{
	Kern::Printf(aTitle);
	while (aSize)
		{
		DumpMemoryLine(aStart);
		aStart += 16;
		if (aSize>=16)
			aSize -= 16;
		else
			aSize = 0;
		}
	}
#endif

extern "C" {
/**
Faults the system, noting file name and line number.

Used from nanokernel code and in various __ASSERT macros.

@param	file	The file name as a C string (__FILE__).
@param	line	The line number (__LINE__).

@see Kern::Fault()
*/
EXPORT_C void NKFault(const char* file, TInt line)
	{
	Kern::Fault(file,line);
	}
}




/** 
Faults the system.

This will start the Crash Debugger if it is present,
otherwise the system is rebooted by calling Kern::Restart(0).

@param aCat   A pointer to a zero terminated string containing the category
              of the fault.
@param aFault The fault number.

@pre Call in any context.
@pre Kernel can be locked or unlocked.
@pre Interrupts can either be enabled or disabled.
@pre Any kind of lock can be held.

@see Kern::Restart()
*/
EXPORT_C void Kern::Fault(const char* aCat, TInt aFault)
	{
	TPtrC8 cat((const TUint8*)aCat);
	Kern::Printf("FAULT: %S 0x%08x (%d)                    ",&cat,aFault,aFault);

	// Disables interrupts
	// Doesn't return
	NKern::NotifyCrash(&cat, aFault);
	}


void K::DoFault(const TAny* aCat, TInt aFault)
	{
	BTrace::Control(BTrace::ECtrlSystemCrashed);
	A::StartCrashDebugger(aCat, aFault);
	TheSuperPage().iKernelFault=aFault;

	// bodge the first 8 bytes of the name into the code and data
	if (aFault!=K::ESystemException)
		{
		const TDesC8* cat = (const TDesC8*)aCat;
		TInt csz = cat->Size();
		TExcInfo& xinf=TheSuperPage().iKernelExcInfo;
		xinf.iCodeAddress=0;
		xinf.iDataAddress=0;
		memcpy((TUint8*)&xinf.iCodeAddress,cat->Ptr(),Min(csz,8));
		}

	Kern::Restart(0);
	}




/**
Gets the address of the low priority DFC queue.

Please note that this function is now deprecated.  It is a leftover from the days before Symbian OS v9.3
when sharing a general purpose DFC queue was acceptable.  With the coming of code and ROM paging this
practice became undesirable, and with the coming of writeable data paging it has become downright dangerous
due to the unbounded performance times introduced by paging, which can cause drivers that share this queue
to block one another.  Therefore, this is to be removed and all drivers must use their own private queue.

Instead of using this function, create your own DFC queue using code something like the following:

TDynamicDfcQue *iDfcQue; // In class definition in header file

// In your driver or extension's constructor:

const TInt KDfcHelperThreadPriority = 27;        // Same as DFC thread 0
_LIT(KDfcHelperThreadName, "MyDfcHelperThread"); // Name as appropriate for your code

if (Kern::DynamicDfcQCreate(iDfcQue, KDfcHelperThreadPriority, KDfcHelperThreadName) == KErrNone)
	{
	// Now you can use iDfcQue wherever you would have called Kern::DfcQue0() and all DFC callbacks
	// will happen on your private DFC queue
	}

@return A pointer to the low priority DFC queue.

@pre Call in any context.
@deprecated
*/
EXPORT_C TDfcQue* Kern::DfcQue0()
	{
	return K::DfcQ0;
	}




/**
Gets the address of the high priority DFC queue.

This is the one used for the nanokernel timer DFC. In the absence of
a personality layer this will usually be the highest priority thread
in the system.

Please note that this function is now deprecated.  It is a leftover from the days before Symbian OS v9.3
when sharing a general purpose DFC queue was acceptable.  With the coming of code and ROM paging this
practice became undesirable, and with the coming of writeable data paging it has become downright dangerous
(DfcQue1 particularly so) due to the unbounded performance times introduced by paging, which can cause
drivers that share this queue to block one another.  Therefore, this is to be removed and all drivers must
use their own private queue.

@see Kern::DfcQue0() for an example of creating a private DFC queue.

@return A pointer to the high priority DFC queue.

@pre Call in any context.
@deprecated
*/
EXPORT_C TDfcQue* Kern::DfcQue1()
	{
	return K::DfcQ1;
	}




/**
Gets the address of the supervisor thread DFC queue.

@return A pointer to the supervisor thread DFC queue.

@pre Call in any context.
*/
EXPORT_C TDfcQue* Kern::SvMsgQue()
	{
	return K::SvMsgQ;
	}




/** 
Creates a new DFC queue.

The function allocates a TDfcQue object on the heap and initialises it with
the provided parameters.

The thread created for the queue will have its real time state enabled.  If 
this is not the desired behaviour then TDynamicDfcQue::SetRealtimeState() can
be used to disable the real time state of the thread.

@param aDfcQ     A reference to a pointer which, on successful return, is set
                 to point to the new DFC queue. On failure, the pointer is set
                 to NULL.

@param aPriority The thread priority for the queue.

@param aName     A pointer to a name for the queue thread. If NULL,
                 a unique name of the form 'DfcThreadNNN' is generated for the
                 queue.

@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::DfcQInit()
@see TDynamicDfcQue::SetRealtimeState() 
*/
EXPORT_C TInt Kern::DfcQCreate(TDfcQue*& aDfcQ, TInt aPriority, const TDesC* aName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::DfcQCreate");			
	TInt r=KErrNoMemory;
	TDfcQue* pQ=new TDfcQue;
	aDfcQ=pQ;
	if (pQ)
		{
		r=Kern::DfcQInit(pQ,aPriority,aName);
		if (r!=KErrNone)
			{
			delete pQ;
			aDfcQ=NULL;
			}
		}
	return r;
	}




/** 
Creates a new dynamic DFC queue.

The function allocates a TDynamicDfcQue object on the heap and initialises it
with the provided parameters.

The thread created for the queue will have its real time state enabled.  If 
this is not the desired behaviour then TDynamicDfcQue::SetRealtimeState() can
be used to disable the real time state of the thread.

@param aDfcQ     A reference to a pointer which, on successful return, is set
                 to point to the new DFC queue. On failure, the pointer is set
                 to NULL.

@param aPriority The thread priority for the queue.

@param aBaseName The base name for the queue thread.  A 9 character string will
                 be appended to this name to create a unique thread name,
                 therefore the base name must not exceed 71 characters.

@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::DfcQInit()
@see TDynamicDfcQue::SetRealtimeState()
*/
EXPORT_C TInt Kern::DynamicDfcQCreate(TDynamicDfcQue*& aDfcQ, TInt aPriority, const TDesC& aBaseName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::DynamicDfcQCreate");			
	aDfcQ = NULL;
	TDynamicDfcQue* pQ=new TDynamicDfcQue;
	if (!pQ)
		return KErrNoMemory;

	TInt r;
	do
		{
		// Generate successive IDs using linear congruential random number generator
		TUint32 original_qid;
		TUint32 qid;
		do	{
			original_qid = K::DynamicDfcQId;
			qid = original_qid * 69069 + 1;
			} while (!__e32_atomic_cas_rlx32(&K::DynamicDfcQId, &original_qid, qid));
		TKName name(aBaseName);
		name.Append('-');
		name.AppendNum(qid, EHex);
		r = Kern::DfcQInit(pQ,aPriority,&name);
		}
	while (r == KErrAlreadyExists);
	
	if (r!=KErrNone)
		delete pQ;
	else
		aDfcQ = pQ;

	return r;
	}




void DynamicDfcQKillFunction(TAny* aDfcQ)
	{
	Kern::SetThreadPriority(KDefaultExitPriority);
	delete (TDfcQue*)aDfcQ; 
 	Kern::Exit(0); 
	}




TDynamicDfcQue::TDynamicDfcQue()
	: iKillDfc(DynamicDfcQKillFunction, this, this, 0)
	{
	}



/** 
Destroys the DFC queue.

The function destroys the DFC queue, killing the DFC thread and deleting the TDynamicDfcQue
object itself.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::DfcQCreate()
@see Kern::DfcQInit()
*/
EXPORT_C void TDynamicDfcQue::Destroy()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TDynamicDfcQue::Destroy");
	iKillDfc.Enque();
	}



/**
Sets the realtime state for the thread that runs the DFC queue.

@param aNewState The new realtime state for the thread.

@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre Interrupts enabled
@pre Can be used in a device driver.
*/
EXPORT_C void TDynamicDfcQue::SetRealtimeState(TThreadRealtimeState aNewState)
	{
	_LOFF(iThread,DThread,iNThread)->SetRealtimeState(aNewState);
	}




_LIT(KLitKernCommon, "KERN-COMMON");
void Panic(TCdtPanic aPanic)
	{
	Kern::PanicCurrentThread(KLitKernCommon, aPanic);
	}

void K::Fault(K::TFault aFault)
	{
	Kern::Fault("KERN",aFault);
	}




/**
Waits for a request to complete.

@param aStatus The status of the request to wait for.
*/
EXPORT_C void Kern::WaitForRequest(TRequestStatus& aStatus)
	{
	TInt i=-1;
	do
		{
		++i;
		NKern::WaitForAnyRequest();
		} while (aStatus==KRequestPending);
	if (i)
		ExecHandler::RequestSignal(i);
	}


/**
Allocates a block of the specified size on the kernel heap and zero-fills it.

@param aSize The size of the buffer to be allocated, in bytes. This must be
             positive and must be less than the value of
             @code
             KMaxTInt/2
             @endcode
             otherwise the allocation request fails.

@return A pointer to the allocated buffer, if successful; NULL if the
        allocation request fails.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TAny* Kern::Alloc(TInt aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::Alloc");			
	if ((TUint)aSize < KMaxTInt/2)
		return K::Allocator->Alloc(aSize);
	return NULL;
	}




/**
Allocates a block of the specified size on the kernel heap and zero-fills it.

@deprecated

Calling this function has the same effect as calling Kern::Alloc().

@param aSize The size of the buffer to be allocated, in bytes. This must be
             positive and must be less than the value of
             @code
             KMaxTInt/2
             @endcode
             otherwise the allocation request fails.

@return A pointer to the allocated buffer, if successful; NULL if the
        allocation request fails.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::Alloc()
*/
EXPORT_C TAny* Kern::AllocZ(TInt aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::AllocZ");			
	return Kern::Alloc(aSize);
	}




/**
Frees a block of memory back to the kernel heap.

The pointer passed must point to a valid allocated kernel heap cell, which
will be the case if it was previously allocated using Kern::Alloc() or
Kern::AllocZ().
 
@param aPtr A pointer to the buffer to be freed.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::Alloc()
@see Kern::AllocZ()
*/
EXPORT_C void Kern::Free(TAny* aPtr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::Free");			
	K::Allocator->Free(aPtr);
	}




/**	
Reallocates a buffer.

The buffer is assumed to have been previously allocated using Kern::Alloc() or
Kern::AllocZ().

If the new requested size is bigger than the current size, then the function
tries to grow the currently allocated buffer, and if that fails, allocates a new
buffer by calling Kern::Alloc(), copies the content of the old buffer into the
new buffer, and frees the old buffer.  Any newly committed memory is
zero-filled.  If the allocation mode is ENeverMove, the currently allocated
buffer cannot be grown, and the function returns NULL instead.

If the new requested size is less than the current size, then the function
shrinks the allocated buffer, and, if the remainder is large enough, creates a
new free cell.

If the pointer passed to this function is NULL, then it behaves like
Kern::Alloc(). However, if the allocation mode is ENeverMove, then it just 
returns NULL.

@param aPtr  A pointer to the existing buffer that is to be reallocated.

@param aSize The new requested size of the buffer, in bytes.

@param aMode The allocation mode. It specifies how the buffer should be
             reallocated. It can take one of the values ENeverMove and 
             EAllowMoveOnShrink.

@return Pointer to the reallocated buffer or NULL if the re-allocation request
        fails.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@post Calling thread is in a critical section.

@see Kern::Alloc()
@see ENeverMove
@see EAllowMoveOnShrink
*/
EXPORT_C TAny* Kern::ReAlloc(TAny* aPtr, TInt aSize, TInt aMode)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ReAlloc");			
	return K::Allocator->ReAlloc(aPtr, aSize, aMode);
	}




/**
Safely reallocates a buffer.

The buffer is assumed to have been previously allocated using Kern::Alloc() or
Kern::AllocZ().

If the new requested size is zero, the function frees the pointer and sets it
to NULL.

If the new requested size is bigger than the old size, then the function tries
to grow the currently allocated buffer using Kern::ReAlloc() specifiying the
ENeverMove allocation mode. If this fails, it does the following sequence of
operations: it calls Kern::Alloc() to allocate a new larger size buffer, copies
the content of the old buffer into the new buffer (zero filling the extra space
in the new buffer), acquires the system lock, sets aPtr to point to the new
buffer, releases the system lock and finally frees the original buffer.

If the new requested size is less than the old size, the function shrinks the
buffer but does not move it.

This function is intended to allow the implementation of a dynamically growing
array which can be indexed and read very efficiently by holding only the 
system lock, while modification of the array is protected by a heavyweight mutex.

@param aPtr     A reference to a pointer to the buffer to be reallocated.
@param aOldSize The size of the currently allocated buffer.
@param aNewSize The new requested size of the buffer.

@return KErrNone, if successful; KErrNoMemory, if there is insufficient memory.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@post Calling thread is in a critical section.

@see Kern::ReAlloc()
@see Kern::Alloc()
*/
EXPORT_C TInt Kern::SafeReAlloc(TAny*& aPtr, TInt aOldSize, TInt aNewSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::SafeReAlloc");			
	if (aNewSize > aOldSize)
		{
#ifdef _DEBUG
		// we can't rely of simulated OOM in the kernel allocator because if
		// ReAlloc fails (and swallows simulated OOM) then the following Alloc will succeed...
		if(K::CheckForSimulatedAllocFail())
			return KErrNoMemory;
#endif
		TAny* p = ReAlloc(aPtr, aNewSize, RAllocator::ENeverMove);
		if (p)
			return KErrNone;			// grow in place succeeded, no need to move
		TAny* pNew = Alloc(aNewSize);	// otherwise allocate bigger block
		if (!pNew)
			return KErrNoMemory;
		TAny* pOld = aPtr;
		memcpy(pNew, pOld, aOldSize);	// copy current contents
#ifdef _DEBUG
		if (pOld)
			K::Allocator->DebugFunction(RAllocator::ECopyDebugInfo, pOld, pNew);
#endif
		NKern::LockSystem();
		aPtr = pNew;
		NKern::UnlockSystem();
		Free(pOld);					// free old block
		}
	else if (aNewSize < aOldSize)
		{
		if (aNewSize > 0)
			aPtr = ReAlloc(aPtr, aNewSize, 0);	// can't fail
		else
			{
			NKern::LockSystem();
			TAny* pOld = aPtr;
			aPtr = NULL;
			NKern::UnlockSystem();
			Free(pOld);
			}
		}
	return KErrNone;
	}




/**
Walks the kernel heap to validate its consistency. If the heap is inconsistent,
the kernel will panic with an appropriate panic code.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C void Kern::ValidateHeap()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ValidateHeap");			
	K::Allocator->Check();
	}




/** 
Atomically swaps the pointer to the kernel-side reference counted object with a
NULL value, and then closes the object.

@param aObj A reference to a pointer to a kernel-side reference counted object
            that is to be closed; it is safe to pass a NULL value.
@param aPtr	A pointer that is passed as a parameter to DObject::Close().

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@post  aObj is NULL.

@see DObject::Close()
*/
EXPORT_C void Kern::SafeClose(DObject*& aObj, TAny* aPtr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::SafeClose");
	DObject* pO = (DObject*)__e32_atomic_swp_ord_ptr(&aObj, 0);
	if (pO)
		pO->Close(aPtr);
	}


TInt K::MakeHandle(TOwnerType aType, DObject* anObject)
	{
	TInt h;
	TInt r=TheCurrentThread->MakeHandle(aType,anObject,h);
	if(r==KErrNone)
		return h;
	else
		return r;
	}

TInt K::MakeHandle(TOwnerType aType, DObject* anObject, TUint aAttr)
	{
	TInt h;
	TInt r=TheCurrentThread->MakeHandle(aType,anObject,h,aAttr);
	if(r==KErrNone)
		return h;
	else
		return r;
	}

TInt K::MakeHandleAndOpen(TOwnerType aType, DObject* anObject, TInt& aHandle)
	{
	return TheCurrentThread->MakeHandleAndOpen(aType,anObject,aHandle);
	}

TInt K::MakeHandleAndOpen(TOwnerType aType, DObject* anObject, TInt& aHandle, TUint aAttr)
	{
	return TheCurrentThread->MakeHandleAndOpen(aType,anObject,aHandle, aAttr);
	}

TInt K::HandleClose(TInt aHandle)
	{
	return TheCurrentThread->HandleClose(aHandle);
	}

TInt DThread::MakeHandle(TOwnerType aType, DObject* aObj, TInt& aHandle)
	{
	TInt r=MakeHandleAndOpen(aType, aObj, aHandle);
	if (r==KErrNone)
		aObj->Close(NULL);	// NULL to balance access count but leave attached to process
	return r;
	}

TInt DThread::MakeHandle(TOwnerType aType, DObject* aObj, TInt& aHandle, TUint aAttr)
	{
	TInt r=MakeHandleAndOpen(aType, aObj, aHandle, aAttr);
	if (r==KErrNone)
		aObj->Close(NULL);	// NULL to balance access count but leave attached to process
	return r;
	}

TInt DThread::MakeHandleAndOpen(TOwnerType aType, DObject* aObj, TInt& aHandle)
	{
	return MakeHandleAndOpen(aType, aObj, aHandle, 0);
	}

TInt DThread::MakeHandleAndOpen(TOwnerType aType, DObject* aObj, TInt& aHandle, TUint aAttr)
	{
	TInt r = aObj->Open();
	if (r==KErrNone)
		{
		r = aObj->RequestUserHandle(this, aType, aAttr);
		if (r==KErrNone)
			{
			if (aType==EOwnerThread)
				{
				__KTRACE_OPT(KEXEC,Kern::Printf("Making handle from thread %O to object %O", this, aObj));

				r = iHandles.Add(aObj, aAttr);
				if (r >= 0)
					{
					aHandle = r | KHandleFlagLocal;
					r = KErrNone;
					}
				}
			else
				{
				__KTRACE_OPT(KEXEC,Kern::Printf("Making handle from process %O to object %O", iOwningProcess, aObj));

				r = iOwningProcess->iHandles.Add(aObj, aAttr);
				if (r >= 0)
					{
					aHandle = r;
					r = KErrNone;
					}
				}
			}
		if (r==KErrNone)
			{
			// It is assumed that:
			// 1.	AddToProcess() can only fail the first time the object is added to the process
			// 2.	Close(iOwningProcess) is equivalent to Close(NULL) if the object has not been
			//		added to the process.
			r=aObj->AddToProcess(iOwningProcess, aAttr);
			if (r!=KErrNone)
				{
				// Add to process failed - try to remove handle
				// If thread/process is exiting this might fail, but the handle will be closed 
				// by the exit handler. In either case this balances the Open() above.
				HandleClose(aHandle);
				aHandle=0;
				}
			}
		else
			aObj->Close(NULL);	// NULL since we did not add to process
		}
	return r;
	}

/**
Makes a thread-owned handle to a kernel object and increments the access count
on the object.

@param aThread  The thread to own the handle.
                If this is NULL, the current thread is used.

@param aObject  The object to which the handle will refer.

@return The created handle (a value >=0), if successful;
        otherwise one of the other system wide error codes, (a value <0).

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/

EXPORT_C TInt Kern::MakeHandleAndOpen(DThread* aThread, DObject* aObject)
	{
	return MakeHandleAndOpen(aThread, aObject, EOwnerThread);
	}

/**
Makes a handle to a kernel object and increments the access count on the object.

The handle can be owned by either a thread or a process.

@param aThread  The thread to own the handle, if the handle is to be owned by a
                thread.
                A thread owned by the process to own the handle, if the handle
                is to be owned by a process.
                If this is NULL, the current thread is used.

@param aObject  The object to which the handle will refer.

@param aType    An enumeration whose enumerators define the ownership of this
                handle.

@return The created handle (a value >=0), if successful;
        otherwise one of the other system wide error codes, (a value <0).

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/

EXPORT_C TInt Kern::MakeHandleAndOpen(DThread* aThread, DObject* aObject, TOwnerType aType)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::MakeHandleAndOpen");
	if (!aThread)
		aThread = TheCurrentThread;
	TInt h;
	TInt r = aThread->MakeHandleAndOpen(aType, aObject, h);
	return (r == KErrNone) ? h : r;
	}


TInt DThread::HandleClose(TInt aHandle)
	{
	// Ignore attempts to close special or null handles
	// or handles with the 'no close' bit set.
	if (aHandle<=0 || (aHandle & KHandleNoClose))
		return KErrNone;
	TInt r=KErrNone;
	DObject* pO=NULL;
	if (aHandle&KHandleFlagLocal)
		{
		TUint32	attr; // Receives the attributes of the removed handle...
		aHandle&=~KHandleFlagLocal;
		r=iHandles.Remove(aHandle,pO,attr);
		}
	else
		{
		TUint32	attr; // Receives the attributes of the removed handle...
		r=iOwningProcess->iHandles.Remove(aHandle,pO,attr);
		}
	if (r==KErrNone)
		r=pO->Close(iOwningProcess)&DObject::EObjectUnmapped;
	return r;
	}

/**
Discard a handle to a kernel object and decrements the access count on the object.

@param aThread The thread which owns the handle. If this is NULL, the current thread is used.
@param aObject The handle to close.

@return KErrNone, if successful; otherwise one of the other system wide error codes.

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.

*/
EXPORT_C TInt Kern::CloseHandle(DThread* aThread, TInt aHandle)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::CloseHandle");			
	if (!aThread)
		aThread = TheCurrentThread;
	return aThread->HandleClose(aHandle);
	}


TInt DThread::OpenFindHandle(TOwnerType aType, const TFindHandle& aFindHandle, TInt& aHandle)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("DThread::OpenFindHandle"));
	TInt r=KErrNone;
	DObjectCon* pC=K::ContainerFromFindHandle(aFindHandle);
	if (!pC)
		return KErrBadHandle;
	pC->Wait();
	DObject* pO=pC->At(aFindHandle);
	if (pO)
		r=pO->Open();
	pC->Signal();
	if (!pO)
		return KErrNotFound;
	if (r!=KErrNone)
		return KErrBadHandle;
	__KTRACE_OPT(KEXEC,Kern::Printf("Object %O found",pO));
	if ((pO->Protection()!=DObject::EGlobal) && (TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecProcessIsolation))
		{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
		r = PlatSec::ProcessIsolationFail(__PLATSEC_DIAGNOSTIC_STRING("Checked by RHandleBase::Open(const TFindHandleBase)"));
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
		r = PlatSec::EmitDiagnostic();
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
		}
	if (r==KErrNone)
		r=MakeHandle(aType,pO,aHandle);
	if (r!=KErrNone)
		pO->Close(NULL);
	return r;
	}

TInt DThread::OpenObject(TOwnerType aType, const TDesC& aName, TInt& aHandle, DObject*& anObj, TInt aObjType)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("DThread::OpenObject %S",&aName));
	anObj=NULL;
	TInt r=Kern::ValidateFullName(aName);
	if (r!=KErrNone)
		return r;
	DObject* pO=NULL;
	r=K::Containers[aObjType]->OpenByFullName(pO,aName);
	if (r!=KErrNone)
		return r;
	__KTRACE_OPT(KEXEC,Kern::Printf("Object %O found", pO));
	anObj=pO;
	r=MakeHandle(aType,pO,aHandle);
	if (r!=KErrNone)
		pO->Close(NULL);	// NULL because chunk not added to process
	return r;
	}

#ifndef __HANDLES_MACHINE_CODED__
/** Translate a user handle relative to a specific thread.

	The handle may refer to type of kernel object.

	@param	aHandle	The handle to translate.

	@return	A pointer to the kernel object to which the handle refers;
	        NULL if the handle is invalid.

	@pre System lock must be held.
 */
EXPORT_C DObject* DThread::ObjectFromHandle(TInt aHandle)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"DThread::ObjectFromHandle(TInt aHandle)");			
	if (aHandle<0)
		{
		aHandle &= ~KHandleNoClose;
		if (aHandle==(KCurrentThreadHandle&~KHandleNoClose))
			return TheCurrentThread;
		if (aHandle==(KCurrentProcessHandle&~KHandleNoClose))
			return TheCurrentThread->iOwningProcess;
#ifdef	__OBSOLETE_V1_IPC_SUPPORT__
		TUint32 h = aHandle;
		if (h < 0x88000000u)
			{
			h = (h & 0x00007FFFu) | ((h & 0x07FF0000u) >> 1);
			h = TUint32(K::MsgInfo.iBase) + (h << 2);
			RMessageK* m = RMessageK::MessageK(h, this);
			if (!m || m->iFunction == RMessage2::EDisConnect)
				return NULL;
			return m->iClient;
			}
#endif
		return NULL;
		}
	DObject* pO=NULL;
	if (aHandle&KHandleFlagLocal)
		{
		pO=iHandles.At(aHandle&~KHandleFlagLocal);
		}
	else
		{
		pO=iOwningProcess->iHandles.At(aHandle);
		}
	return pO;
	}

/**
Translates a user handle relative to a specific thread.

The handle must refer to a specific type of kernel object.

@param	aHandle	The handle to translate.
@param	aType	The type of kernel object to which the handle must refer.
				This should be a member of the TObjectType enumeration.
				
@return	A pointer to the kernel object to which the handle refers.
        NULL if the handle is invalid or refers to the wrong type of object.

@pre System lock must be held.
*/
EXPORT_C DObject* DThread::ObjectFromHandle(TInt aHandle, TInt aType)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"DThread::ObjectFromHandle(TInt aHandle, TInt aType)");
	TUint attr = 0;
	return ObjectFromHandle(aHandle, aType, attr);
	}

EXPORT_C DObject* DThread::ObjectFromHandle(TInt aHandle, TInt aType, TUint& aAttr)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"DThread::ObjectFromHandle(TInt aHandle, TInt aType)");			
	if (aHandle<0)
		{
		aHandle &= ~KHandleNoClose;
		if (aHandle==(KCurrentThreadHandle&~KHandleNoClose) && aType==EThread)
			return TheCurrentThread;
		if (aHandle==(KCurrentProcessHandle&~KHandleNoClose) && aType==EProcess)
			return TheCurrentThread->iOwningProcess;
#ifdef	__OBSOLETE_V1_IPC_SUPPORT__
		TUint32 h = aHandle;
		if (aType==EThread && h < 0x88000000u)
			{
			h = (h & 0x00007FFFu) | ((h & 0x07FF0000u) >> 1);
			h = TUint32(K::MsgInfo.iBase) + (h << 2);
			RMessageK* m = RMessageK::MessageK(h, this);
			if (!m || m->iFunction == RMessage2::EDisConnect)
				return NULL;
			return m->iClient;
			}
#endif
		return NULL;
		}
	DObject* pO=NULL;

	if (aHandle&KHandleFlagLocal)
		{
		pO=iHandles.At(aHandle&~KHandleFlagLocal,aType+1, (TUint32*)&aAttr);
		}
	else
		{
	    pO=iOwningProcess->iHandles.At(aHandle,aType+1, (TUint32*)&aAttr);
		}
	return pO;
	}

DObject* K::ObjectFromHandle(TInt aHandle)
//
// Look up an object in the current thread/process handles array
// Panic on bad handle
// Enter and leave with system lock held
//
	{
	DObject* pO=TheCurrentThread->ObjectFromHandle(aHandle);
	if (!pO)
		K::PanicCurrentThread(EBadHandle);
	return pO;
	}

DObject* K::ObjectFromHandle(TInt aHandle, TInt aType)
//
// Look up an object of specific type in the current thread/process handles array
// Panic on bad handle
// Enter and leave with system lock held
//
	{
	DObject* pO=TheCurrentThread->ObjectFromHandle(aHandle,aType);
	if (!pO)
		K::PanicCurrentThread(EBadHandle);
	return pO;
	}

DObject* K::ObjectFromHandle(TInt aHandle, TInt aType, TUint& aAttr)
//
// Look up an object of specific type in the current thread/process handles array
// Panic on bad handle
// Enter and leave with system lock held
//
	{
	DObject* pO=TheCurrentThread->ObjectFromHandle(aHandle,aType,aAttr);
	if (!pO)
		K::PanicCurrentThread(EBadHandle);
	return pO;
	}



/**
Returns the kernel object that the given handle refers.

The handle passed is looked up in the thread's handles collection if the handle is local or
in the thread's owner process' collection otherwise. If aHandle is negative or not found in
the thread's or process' collection then NULL is returned.
Two special handle values KCurrentThreadHandle and KCurrentProcessHandle can be used to get
a pointer to the current thread and the current process.

aType is used to ensure that the object referred by the handle is of desired type.
If the type of the object referred by aHandle is different from aType then NULL is returned.
If aType is negative, the type of the object is ignored and no type checking is done.
If aType is positive and greater than the maximum number of object types (ENumObjectTypes)
the kernel will fault.

@param aThread The thread that owns the handle passed.
@param aHandle Handle to the object to be returned.
@param aType TObjectType parameter specifying the type of the object referred by the handle.  

@return Pointer to the DObject referred by the handle or NULL if the handle is not
		found in the thread's handles collection.

@pre System must be locked
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre Can be used in a device driver.

@see TObjectType
@see DThread::ObjectFromHandle()
*/
EXPORT_C DObject* Kern::ObjectFromHandle(DThread* aThread, TInt aHandle, TInt aType)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED,
		"Kern::ObjectFromHandle(DThread* aThread, TInt aHandle, TInt aType)");
	if (aType>=0)
		{
		if (aType<ENumObjectTypes)
			return aThread->ObjectFromHandle(aHandle,aType);
		K::Fault(K::EBadObjectType);
		}
	return aThread->ObjectFromHandle(aHandle);
	}

/**
Returns the kernel object that the given handle refers.

The handle passed is looked up in the thread's handles collection if the handle is local or
in the thread's owner process' collection otherwise. If aHandle is negative or not found in
the thread's or process' collection then NULL is returned.
Two special handle values KCurrentThreadHandle and KCurrentProcessHandle can be used to get
a pointer to the current thread and the current process.

aType is used to ensure that the object referred by the handle is of desired type.
If the type of the object referred by aHandle is different from aType then NULL is returned.
If aType is negative, the type of the object is ignored and no type checking is done.
If aType is positive and greater than the maximum number of object types (ENumObjectTypes)
the kernel will fault.

@param aThread The thread that owns the handle passed.
@param aHandle Handle to the object to be returned.
@param aType TObjectType parameter specifying the type of the object referred by the handle.  
@param aAttr Returns the attributes for this object.  

@return Pointer to the DObject referred by the handle or NULL if the handle is not
		found in the thread's handles collection.

@pre System must be locked
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre Can be used in a device driver.

@see TObjectType
@see DThread::ObjectFromHandle()
*/
EXPORT_C DObject* Kern::ObjectFromHandle(DThread* aThread, TInt aHandle, TInt aType, TUint& aAttr)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED|MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED,
		"Kern::ObjectFromHandle(DThread* aThread, TInt aHandle, TInt aType)");
	if (aType>=0)
		{
		if (aType<ENumObjectTypes)
		    return aThread->ObjectFromHandle(aHandle,aType, aAttr);
		K::Fault(K::EBadObjectType);
		}
	return aThread->ObjectFromHandle(aHandle, 0, aAttr);
	}
#endif

TInt K::OpenObjectFromHandle(TInt aHandle, DObject*& anObject)
//
// Look up a handle and open the object.
// Enter and return with no fast mutexes held.
// If successful, calling thread is placed into critical section.
// Return KErrBadHandle if handle bad, KErrNone if OK
//
	{
	DThread& t=*TheCurrentThread;
	TInt r=KErrBadHandle;
	NKern::ThreadEnterCS();
	NKern::LockSystem();
	DObject* pO=t.ObjectFromHandle(aHandle);
	if (pO)
		r=pO->Open();
	NKern::UnlockSystem();
	if (r!=KErrNone)
		{
		anObject=NULL;
		NKern::ThreadLeaveCS();
		}
	else
		anObject=pO;
	return r;
	}




/** 
Gets a pointer to the thread corresponding to the specified thread Id value.

The caller must ensure that the returned DThread instance is not closed
asynchronously by another thread. 

@param aId 	The thread id.

@return	A pointer to the thread, or NULL if not found.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre Thread container mutex must be held.
@pre Call in a thread context.
@pre No fast mutex must be held
@pre Can be used in a device driver.

@post Thread container mutex is held.
@post Calling thread is in a critical section.
*/
EXPORT_C DThread* Kern::ThreadFromId(TUint aId)
	{
	DObjectCon& threads=*K::Containers[EThread];
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ThreadFromId");				
	__ASSERT_WITH_MESSAGE_MUTEX(threads.Lock(),"Thread container mutex must be held","Kern::ThreadFromId");		
	TInt c=threads.Count();
	TInt i;
	for (i=0; i<c; i++)
		{
		DThread* pT=(DThread*)threads[i];
		if (pT->iId==aId)
			return pT;
		}
	return NULL;
	}




/**
Gets a pointer to the process corresponding to the specified process Id value.

The caller must ensure that the returned DProcess instance is not deleted
asynchronously by another thread.  

@param aId 	The process id.
@return	A pointer to the process, or NULL if not found.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre Process container mutex must be held.
@pre Call in a thread context.
@pre No fast mutex must be held
@pre Can be used in a device driver.

@post Process container mutex is held.
@post Calling thread is in a critical section.
*/
EXPORT_C DProcess* Kern::ProcessFromId(TUint aId)
	{
	DObjectCon& processes=*K::Containers[EProcess];
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ProcessFromId");
	__ASSERT_WITH_MESSAGE_MUTEX(processes.Lock(),"Process container mutex must be held","Kern::ProcessFromId");
	//end of preconditions check
	TInt c=processes.Count();
	TInt i;
	for (i=0; i<c; i++)
		{
		DProcess* pP=(DProcess*)processes[i];
		if (pP->iId==aId)
			return pP;
		}
	return NULL;
	}

TBool K::IsInKernelHeap(const TAny* aPtr, TInt aSize)
//
// Check if an address range lies within the kernel heap chunk
//
	{
	TLinAddr a=(TLinAddr)aPtr;
	TLinAddr base=(TLinAddr)K::HeapInfo.iBase;
	TInt max=K::HeapInfo.iMaxSize;
	return (a>=base && TInt(a-base+aSize)<=max);
	}

GLDEF_C TInt CalcKernelHeapUsed()
	{
	return ((RHeapK*)K::Allocator)->TotalAllocSize();
	}




/**
Copies data from a source descriptor in kernel memory, to a target descriptor 
in user memory, in a way that enables forward and backward compatibility.

If the length of the source data is longer that the maximum length of the
target descriptor then the number of bytes copied is limited to the maximum
length of the target descriptor.

If the length of the source data is smaller that the maximum length of the
target descriptor then the target descriptor is padded with zeros.

If the current thread is a user thread (i.e. if the mode in spsr_svc
is 'User'), then data is written using user mode privileges.

@param aDestU The target descriptor in user memory.
@param aSrcK  The source descriptor in kernel memory.

@panic KERN-EXEC 33, if aDestU is not a writable descriptor type.

@pre Do not call from User thread if in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@post The length of aDestU is equal to the number of bytes copied, excluding
      any padding.
@post If aDestU is a TPtr type then its maximum length is equal its new length.
*/
EXPORT_C void Kern::InfoCopy(TDes8& aDestU, const TDesC8& aSrcK)
	{
	CHECK_PRECONDITIONS(MASK_NO_CRITICAL_IF_USER|MASK_THREAD_STANDARD,"Kern::InfoCopy(TDes8& aDestU, const TDesC8& aSrcK)");				
	Kern::InfoCopy(aDestU,aSrcK.Ptr(),aSrcK.Length());
	}




/**
Copies data from kernel memory to a target descriptor in user memory, 
in a way that enables forward and backward compatibility.

If the length of the source data is longer that the maximum length of the
target descriptor then the number of bytes copied is limited to the maximum
length of the target descriptor.

If the length of the source data is smaller that the maximum length of the
target descriptor then the target descriptor is padded with zeros.

If the current thread is a user thread (i.e. if the mode in spsr_svc
is 'User'), then data is written using user mode privileges.

@param aDestU The target descriptor in user memory.
@param aPtrK Address of the first byte of data to be copied in kernel memory.
@param aLengthK Length of data to be copied.

@panic KERN-EXEC 33, the target descriptor is not writable.

@pre Do not call from User thread if in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@post The length of aDestU is equal to the number of bytes copied, excluding
      any padding.
@post If aDestU is a TPtr type then its maximum length is equal its new length.
*/
EXPORT_C void Kern::InfoCopy(TDes8& aDestU, const TUint8* aPtrK, TInt aLengthK)
    {
	CHECK_PRECONDITIONS(MASK_NO_CRITICAL_IF_USER|MASK_THREAD_STANDARD,"Kern::InfoCopy(TDes8& aDestU, const TUint8* aPtrK, TInt aLengthK)");				
	TInt userLen;
	TInt userMax;
	TUint8* userPtr=(TUint8*)Kern::KUDesInfo(aDestU,userLen,userMax);
	if (userMax<0)
		K::PanicKernExec(EKUDesInfoInvalidType);
    TInt copyLength=Min(aLengthK,userMax);
	if (aLengthK<userMax)
		kumemset(userPtr+aLengthK,0,userMax-aLengthK);
	TPtrC8 kptr(aPtrK,copyLength);
	Kern::KUDesPut(aDestU,kptr);
    }




/**
Gets the power model.

@return A pointer to the power model object.

@pre Call in any context.
*/
EXPORT_C DPowerModel* Kern::PowerModel()
	{
	return K::PowerModel;
	}




/**
Gets the status of the power supply.

@return The status of the power supply. EGood, if there is no power model.

@pre Calling thread can be either in a critical section or not.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TSupplyStatus Kern::MachinePowerStatus()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::MachinePowerStatus");				
	if(K::PowerModel)
		return K::PowerModel->MachinePowerStatus();
	// If no power model...
	return EGood;
	}




/**
Changes the priority of the specified thread or the current thread.

@param aPriority The new priority to be set.
@param aThread   The thread that is to have its priority set. If NULL, the
                 thread is the current thread.

@return KErrNone, if successful; KErrArgument, if the priority value is
        negative or greater than or equal to KNumPriorities.

@pre Calling thread can be either in a critical section or not.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see DThread::SetThreadPriority()
@see KNumPriorities
*/
EXPORT_C TInt Kern::SetThreadPriority(TInt aPriority, DThread* aThread)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::SetThreadPriority");				
	if (!aThread)
		aThread=TheCurrentThread;
	__KTRACE_OPT(KEXEC,Kern::Printf("Kern::SetThreadPriority %d %O",aPriority,aThread));
	if (aPriority<0 || aPriority>=KNumPriorities)
		return KErrArgument;
	NKern::LockSystem();
	aThread->SetThreadPriority(aPriority);
	NKern::UnlockSystem();
	return KErrNone;
	}




/**
Gets the device's superpage.

@return A reference to the device's superpage.

@pre Call in any context.
*/
EXPORT_C TSuperPage& Kern::SuperPage()
	{
	return *(TSuperPage*)SuperPageAddress;
	}




/**
Gets the device's configuration information.

@return A reference to the device configuration information.

@pre Call in any context.
*/
EXPORT_C TMachineConfig& Kern::MachineConfig()
	{
	return *K::MachineConfig;
	}




/**
Suspends execution of the specified thread.

If the thread is running a critical section, suspension will be deferred until
it leaves the critical section.

@param aThread The thread to be suspended.
@param aCount  Specifies how many times this thread should be suspended. It
               will require the same number of calls to ThreadResume() to undo
               the result of this call to ThreadSuspend().

@pre Calling thread can be either in a critical section or not.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
               
@see DThread::Suspend()
*/
EXPORT_C void Kern::ThreadSuspend(DThread& aThread, TInt aCount)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadSuspend");				
	NKern::LockSystem();
	aThread.Suspend(aCount);
	NKern::UnlockSystem();
	}




/**
Resumes execution of the specified thread.

Calling Resume() does not mean that the thread becomes runnable. Instead it
increments the thread's suspend count. When the count reaches 0, the thread
is made runnable (in case it's not blocked).
	
@param aThread The thread to be resumed.

@pre Calling thread can be either in a critical section or not.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see DThread::Resume()
*/
EXPORT_C void Kern::ThreadResume(DThread& aThread)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadResume");				
	NKern::LockSystem();
	aThread.Resume();
	NKern::UnlockSystem();
	}
	
	
	
	
/**
Waits on the specified mutex.

If the calling thread is a user thread, it must be in a critical section while
it holds the mutex to prevent deadlocks (thread suspended while holding mutex), inconsistent
states (thread killed while data protected by mutex in inconsistent state)
and resource leaks (thread killed before taking ownership of some
resource).

@param aMutex Mutex to wait on.

@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Kern::MutexWait(DMutex& aMutex)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::MutexWait");				
	NKern::LockSystem();
	TInt r=aMutex.Wait();
	NKern::UnlockSystem();
	return r;
	}




/**
Signals the specified mutex.

If the calling thread is a user thread, it must be in a critical section.

@param aMutex Mutex to signal

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C void Kern::MutexSignal(DMutex& aMutex)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::MutexSignal");				
	NKern::LockSystem();
	aMutex.Signal();
	}




/**
Creates a kernel mutex object with the specified name.

On return, the kernel mutex object is not visible and has no owner.

@param aMutex A reference to a DMutex pointer.
              On successful return from this function, the pointer is set
              to the address of the created DMutex object.
@param aName  The name of the mutex.
@param aOrder A value representing the order of the mutex with respect to deadlock prevention.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
 
@post  On successful return, aMutex contains a pointer to the newly created
       DMutex object.
 
@return KErrNone, if successful, otherwise one of the other system-wide
        error codes.
*/
EXPORT_C TInt Kern::MutexCreate(DMutex*& aMutex, const TDesC& aName, TUint aOrder)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::MutexCreate");				
	return K::MutexCreate(aMutex, aName, NULL, EFalse, aOrder);
	}


/**
Waits on the specified semaphore.

@param aSem		Semaphore to wait on
@param aNTicks	Maximum number of nanokernel ticks to wait before timing out
				the operation. Zero means wait forever. If this parameter is
				not specified it defaults to 0.

@return KErrNone, if successful;
        KErrTimedOut, if the maximum wait time was exceeded before the
		semaphore was signalled;
        KErrGeneral, if the semaphore was deleted.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TInt Kern::SemaphoreWait(DSemaphore& aSem, TInt aNTicks)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::SemaphoreWait");				
	NKern::LockSystem();
	return aSem.Wait(aNTicks);
	}




/**
Signals the specified semaphore.

@param aSem Semaphore to signal.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C void Kern::SemaphoreSignal(DSemaphore& aSem)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::SemaphoreSignal");				
	NKern::LockSystem();
	aSem.Signal();
	}



/**
Creates a semaphore with the specified name.

Note that, on return, the semaphore is not visible, and has no owner.

@param aSem A reference to a pointer to a semaphore.
@param aName  The name of the semaphore.
@param aInitialCount The count with which the semaphore should start.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@post  On successful return, aSem contains a pointer to the newly created
       semaphore.

@return KErrNone, if successful, otherwise one of the other system-wide
        error codes.
*/
EXPORT_C TInt Kern::SemaphoreCreate(DSemaphore*& aSem, const TDesC& aName, TInt aInitialCount)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::SemaphoreCreate");				
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Kern::SemaphoreCreate %S init %d", &aName, aInitialCount));
	TInt r = KErrNoMemory;
	DSemaphore* pS = new DSemaphore;
	if (pS)
		{
		r = pS->Create(NULL, &aName, aInitialCount, EFalse);
		if (r==KErrNone)
			aSem = pS;
		else
			pS->Close(NULL);
		}
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Kern::SemaphoreCreate returns %d", r));
	return r;
	}



TUint K::CheckFreeMemoryLevel(TInt aInitial, TInt aFinal, TBool aFailed)
	{
	NKern::LockSystem();
	TInt low=K::MemoryLowThreshold;
	TInt good=K::MemoryGoodThreshold;
	NKern::UnlockSystem();
	TUint changes=0;
	if (aFinal<low && aInitial>=low)
		changes |= (EChangesFreeMemory | EChangesLowMemory);
	if (aFinal>=good && aInitial<good)
		changes |= EChangesFreeMemory;
	if (aFailed)
		changes |= EChangesOutOfMemory;
	if (changes)
		{
		// asynchronously notify changes
		Kern::AsyncNotifyChanges(changes);
		}
	return changes;
	}


TBool K::CheckForSimulatedAllocFail()
	{
#ifdef _DEBUG
	if(K::Allocator)
		return ((RHeapK*)K::Allocator)->CheckForSimulatedAllocFail();
#endif
	return EFalse;
	}


/**
Gets the current Symbian OS thread.

Note that if this function is called from an ISR or an IDFC, then it returns
a reference to the interrupted thread.
Note also that this function assumes that the current thread is a Symbian OS
thread. The result will not be sensible if it is a raw nanokernel thread.

@return A reference to the current thread.

@pre Call in a thread context.
*/
EXPORT_C DThread& Kern::CurrentThread()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_NOT_IDFC,"Kern::CurrentThread()");				
	return *TheCurrentThread;
	}




/**
Gets the current process.

The current process is that to which the current thread belongs.

Note that if this function is called from an ISR or an IDFC, then the
associated thread is the interrupted thread.
Note also that this function assumes that the current thread is a Symbian OS
thread. The result will not be sensible if it is a raw nanokernel thread.

@return A reference to the current process.

@pre Call in a thread context.

@see Kern::CurrentThread()
*/
EXPORT_C DProcess& Kern::CurrentProcess()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_NOT_IDFC,"Kern::CurrentProcess()");				
	return *TheCurrentThread->iOwningProcess;
	}


DThread* K::ThreadEnterCS()
	{
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	return TheCurrentThread;
	}

DThread* K::ThreadLeaveCS()
	{
	NKern::LockSystem();
	NKern::ThreadLeaveCS();
	return TheCurrentThread;
	}

DObject* K::ThreadEnterCS(TInt aHandle, TInt aType)
//
// Enter a thread critical section, translate a handle and open the object
// Return a pointer to the object
// Enter with system locked, leave with system unlocked
//
	{
	DObject* pO=NULL;
	if (aType>=0)
		pO=TheCurrentThread->ObjectFromHandle(aHandle,aType);
	else
		pO=TheCurrentThread->ObjectFromHandle(aHandle);
	if (!pO || pO->Open())
		K::PanicCurrentThread(EBadHandle);
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	return pO;
	}

TUint32 K::KernelConfigFlags()
	{
	TUint32 flags = TheSuperPage().KernelConfigFlags();
	if(TEST_DEBUG_MASK_BIT(KTESTLATENCY))
		flags &= ~EKernelConfigPlatSecDiagnostics;
	
	TBool codePagingSupported = K::MemModelAttributes & EMemModelAttrCodePaging;
	if (!codePagingSupported)
		flags = (flags & ~EKernelConfigCodePagingPolicyMask) | EKernelConfigCodePagingPolicyNoPaging;

	TBool dataPagingSupported = K::MemModelAttributes & EMemModelAttrDataPaging;
	if (!dataPagingSupported)
		flags = (flags & ~EKernelConfigDataPagingPolicyMask) | EKernelConfigDataPagingPolicyNoPaging;
	
	return flags;
	}

void signal_sem(TAny* aPtr)
	{
	NKern::FSSignal((NFastSemaphore*)aPtr);
	}

TInt WaitForIdle(TInt aTimeoutMilliseconds)
	{
	NFastSemaphore s(0);
	TDfc idler(&signal_sem, &s, Kern::SvMsgQue(), 0);	// supervisor thread, priority 0, so will run after destroyed DFC
	NTimer timer(&signal_sem, &s);
	idler.QueueOnIdle();
	timer.OneShot(NKern::TimerTicks(aTimeoutMilliseconds), ETrue);	// runs in DFCThread1
	NKern::FSWait(&s);	// wait for either idle DFC or timer
	TBool timeout = idler.Cancel();	// cancel idler, return TRUE if it hadn't run
	TBool tmc = timer.Cancel();	// cancel timer, return TRUE if it hadn't expired
	if (!timeout && !tmc)
		NKern::FSWait(&s);	// both the DFC and the timer went off - wait for the second one
	if (timeout)
		return KErrTimedOut;
	return KErrNone;
	}

TInt K::KernelHal(TInt aFunction, TAny* a1, TAny* a2)
	{
	(void)a2;
	TInt r=KErrNone;
	switch (aFunction)
		{
		case EKernelHalMemoryInfo:
			{
			TMemoryInfoV1Buf infoBuf;
			TMemoryInfoV1& info=infoBuf();
			info.iTotalRamInBytes=TheSuperPage().iTotalRamSize;
			info.iTotalRomInBytes=TheSuperPage().iTotalRomSize;
			info.iMaxFreeRamInBytes=K::MaxFreeRam;
			NKern::LockSystem();
			info.iFreeRamInBytes=Kern::FreeRamInBytes();
			info.iInternalDiskRamInBytes=TheSuperPage().iRamDriveSize;
			NKern::UnlockSystem();
			info.iRomIsReprogrammable=ETrue;
			Kern::InfoCopy(*(TDes8*)a1,infoBuf);
			break;
			}
/* Deprecated in 6.0 ??
		case EKernelHalRomInfo:
			{
			TRomInfoV1Buf infoBuf;
			TRomInfoV1& info=infoBuf();
			memcpy(&info,&TheSuperPage().iRomConfig[0],sizeof(TRomInfoV1));
			Kern::InfoCopy(*(TDes8*)a1,infoBuf);
			break;
			}
*/
		case EKernelHalStartupReason:
			kumemput32(a1,&TheSuperPage().iStartupReason,sizeof(TMachineStartupType));
			break;
		case EKernelHalFaultReason:
			kumemput32(a1,&TheSuperPage().iKernelFault,sizeof(TInt));
			break;
		case EKernelHalExceptionId:
			kumemput32(a1,&TheSuperPage().iKernelExcId,sizeof(TInt));
			break;
		case EKernelHalExceptionInfo:
			kumemput32(a1,&TheSuperPage().iKernelExcInfo,sizeof(TExcInfo));
			break;
		case EKernelHalCpuInfo:
			r=KErrNotSupported;
			break;
		case EKernelHalPageSizeInBytes:
			{
			TInt pageSize=M::PageSizeInBytes();
			kumemput32(a1,&pageSize,sizeof(TInt));
			break;
			}
		case EKernelHalTickPeriod:
			{
			kumemput32(a1,&K::TickQ->iTickPeriod,sizeof(TInt));
			break;
			}
		case EKernelHalNTickPeriod:
			{
			TInt period=NTickPeriod();
			kumemput32(a1,&period,sizeof(TInt));
			break;
			}
		case EKernelHalFastCounterFrequency:
			{
			TInt freq=NKern::FastCounterFrequency();
			kumemput32(a1,&freq,sizeof(TInt));
			break;
			}
		case EKernelHalMemModelInfo:
			r=(TInt)K::MemModelAttributes;
			break;
		case EKernelHalHardwareFloatingPoint:
			TUint32 types;
			r=K::FloatingPointTypes(types);
			kumemput32(a1,&types,sizeof(TUint32));
			break;

		case EKernelHalGetNonsecureClockOffset:
			kumemput32(a1, &K::NonSecureOffsetSeconds, sizeof(K::NonSecureOffsetSeconds));
			break;
		case EKernelHalSetNonsecureClockOffset:
			if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by KernelHal function")))
				r=KErrPermissionDenied;
			else
				{
				// Only allow the nonsecure offset to be set *once* (i.e. by halsettings.exe during startup). 
				// Subsequent updates to this value are of course done through setting the 
				// nonsecure system time.
				if (K::SecureClockStatus & ESecureClockOffsetPresent) 
					r = KErrGeneral; 
				else 
					{
					// Update the nonsecure offset not by writing it directly, but by using the 
					// time-setting API. This will also cause the software clock to be updated
					// with the offset, while leaving the hardware clock untouched.
					TTimeK t = Kern::SystemTime();
					K::SecureClockStatus |= ESecureClockOffsetPresent;
					TInt64 offset = (TInt)a1;
					offset *= 1000000;
					t += offset;
					NKern::ThreadEnterCS();
					Kern::SetSystemTime(t, 0);
					NKern::ThreadLeaveCS();
					}
				}
			break;
#ifdef __SMP__
		case EKernelHalSmpSupported:
			r = KErrNone;
			break;
#endif
		case EKernelHalNumLogicalCpus:
#ifdef __SMP__
			r = NKern::NumberOfCpus();
#else
			r = 1;
#endif
			break;
		case EKernelHalSupervisorBarrier:
			{
			NKern::ThreadEnterCS();
			r = KErrNone;
			TInt timeout = (TInt)a1;
			if (timeout>0)
				{
				r = WaitForIdle(timeout);
				}
			if (r==KErrNone)
				{
				TMessageBase& m=Kern::Message();
				m.SendReceive(&K::SvBarrierQ);
				}
			NKern::ThreadLeaveCS();
			break;
			}
		case EKernelHalFloatingPointSystemId:
			TUint32 sysid;
			r=K::FloatingPointSystemId(sysid);
			kumemput32(a1,&sysid,sizeof(TUint32));
			break;

		case EKernelHalLockThreadToCpu:
			{
#ifdef __SMP__
			r = KErrArgument;
			TUint32 cpuId = (TUint32)a1;
			TUint32 ncpus = NKern::NumberOfCpus();
			if (cpuId < ncpus)
				{
				NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), cpuId);
				r = KErrNone;
				}
			else if (cpuId & NTHREADBASE_CPU_AFFINITY_MASK)
				{
				TUint32 mask = cpuId & ~NTHREADBASE_CPU_AFFINITY_MASK;
				TUint32 amask = ~((~0u)<<ncpus);
				if (cpuId==KCpuAffinityAny || ((mask & amask) && (mask &~ amask)==0))
					{
					NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), cpuId);
					r = KErrNone;
					}
				}
#else
			r = KErrNone;
#endif
			break;	
       		}    

		case EKernelHalConfigFlags:
			// return bottom 31 bits of config flags so as not to signal an error
			r=K::KernelConfigFlags() & 0x7fffffff;
			break;

#ifdef __SMP__
		case EKernelHalCpuStates:
			{
			SCpuStates states;
			memclr(&states, sizeof(states));

			TScheduler& s = TheScheduler;
			TInt irq = s.iGenIPILock.LockIrqSave();
			states.iTA = s.iThreadAcceptCpus;
			states.iIA = s.iIpiAcceptCpus;
			states.iCU = s.iCpusComingUp;
			states.iGD = s.iCpusGoingDown;
			states.iDC = s.iCCDeferCount;
			states.iSC = s.iCCSyncCpus;
			states.iRC = s.iCCReactivateCpus;
			states.iCCS = s.iCCState;
			states.iPO = TUint8(s.iPoweringOff ? (s.iPoweringOff->iCpuNum|0x80) : 0);
			states.iPODC = s.iDetachCount;
			TInt i;
			TInt nc = NKern::NumberOfCpus();
			for (i=0; i<nc; ++i)
				{
				TSubScheduler& ss = TheSubSchedulers[i];
				states.iDS[i] = ss.iDeferShutdown;
#ifdef __CPU_ARM
				volatile TUint32* p = (volatile TUint32*)ss.iUncached;
				states.iUDC[i] = p[0];
				states.iUAC[i] = p[1];
#endif
				}
			s.iGenIPILock.UnlockIrqRestore(irq);

			kumemput32(a1, &states, sizeof(states));
			r = KErrNone;
			break;
			}

		case EKernelHalSetNumberOfCpus:
			{
			TInt n = (TInt)a1;
			if (n<=0 || n>NKern::NumberOfCpus())
				r = KErrArgument;
			else
				{
				NKern::SetNumberOfActiveCpus(n);
				r = KErrNone;
				}
			break;
			}
#endif
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

void K::CheckKernelUnlocked()
	{
	if (NKern::KernelLocked() || NKern::HeldFastMutex())
		K::Fault(K::EPanicWhileKernelLocked);
	}

void K::CheckFileServerAccess()
	{
	DProcess* pP=&Kern::CurrentProcess();
	if (pP!=K::TheKernelProcess && pP!=K::TheFileServerProcess)
		K::PanicKernExec(EAccessDenied);
	}

void K::SetMachineConfiguration(const TDesC8& aConfig)
//
// Set the platform dependant machine configuration.
// NOTE: We assume the machine configuration is small enough
// that it can be copied with the kernel locked without adversely
// affecting real-time performance. On EIGER this means about 2K.
// LATER: This 2K has been reduced to 512 bytes, which could be getting a bit tight here.
//
	{
	TPtr8 c(A::MachineConfiguration());
	NKern::LockSystem();
	c=aConfig;
	NKern::UnlockSystem();
	}




/**
Initialises a new DFC queue.

The function creates and starts a kernel thread to process the supplied DFC
queue. On successful completion, the queue is ready to start processing DFCs.

The thread created for the queue will have its real time state enabled.  If 
this is not the desired behaviour then TDynamicDfcQue::SetRealtimeState() can
be used to disable the real time state of the thread.

@param aDfcQ     A pointer to the DFC queue to be initialised.
@param aPriority The thread priority for the queue.
@param aName     A pointer to a descriptor containing the name for the queue
                 thread. If NULL (the default), a uniqiue name of the form
                 'DfcThreadNNN' is generated for the queue, where NNN
                 represents three numeric characters.
                 
@return KErrNone, if successful, otherwise one of the other system-wide
        error codes.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::DfcQCreate()
@see TDynamicDfcQue::SetRealtimeState()
*/
EXPORT_C TInt Kern::DfcQInit(TDfcQue* aDfcQ, TInt aPriority, const TDesC* aName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::DfcQInit");				
	__KTRACE_OPT(KDFC,Kern::Printf("Kern::DfcQInit %d at %08x",aPriority,aDfcQ));
	SThreadCreateInfo info;
	info.iType=EThreadSupervisor;
	info.iFunction=(TThreadFunction)TDfcQue::ThreadFunction;
	info.iPtr=aDfcQ;
	info.iSupervisorStack=NULL;
	info.iSupervisorStackSize=0;	// zero means use default value
	info.iInitialThreadPriority=aPriority;
	if (aName)
		info.iName.Set(*aName);
	else
		{
		TBuf<16> n(KLitDfcThread());
		n.AppendNum((TInt)__e32_atomic_add_ord32(&K::DfcQId, 1));
		info.iName.Set(n);
		}
	info.iTotalSize = sizeof(info);
	TInt r=Kern::ThreadCreate(info);
	if (r==KErrNone)
		{
		DThread* pT=(DThread*)info.iHandle;
		__KTRACE_OPT(KDFC,Kern::Printf("TDfcQue thread %O at %08x",pT,pT));
		aDfcQ->iThread=&pT->iNThread;
#ifndef __DFC_THREADS_NOT_REALTIME
		// Dfc threads are real time by default when data paging is enabled.
		TUint dataPolicy = TheSuperPage().KernelConfigFlags() & EKernelConfigDataPagingPolicyMask;
		if (dataPolicy != EKernelConfigDataPagingPolicyNoPaging)
			pT->SetRealtimeState(ERealtimeStateOn);
#endif
		Kern::ThreadResume(*pT);
		}
	return r;
	}




/**
Performs a polling operation at specified regular intervals, for a specified
maximum number of attempts.

The polling operation is performed by the specified function. The function is
called repeatedly at each interval until it either returns true, or the maximum
number of attempts has been reached.

@param aFunction     The function implementing the polling operation.
@param aPtr          An argument passed to the polling function.
@param aPollPeriodMs The interval between successive attempts at calling the
                     polling function, in milliseconds. Note that the the time
                     period is converted into ticks, and may be rounded up to
                     give an integral number of ticks.
@param aMaxPoll      The maximum number of attempts at calling the polling
                     function before timing out.

@return KErrNone,     if the polling function returns true;
        KErrBadPower, if the device's power status is no longer good;
        KErrTimedOut, if the maximum number of attempts has been reached.
        
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.        
*/
EXPORT_C TInt Kern::PollingWait(TPollFunction aFunction, TAny* aPtr, TInt aPollPeriodMs, TInt aMaxPoll)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::PollingWait");				
	TInt ticks=NKern::TimerTicks(aPollPeriodMs);
	FOREVER
		{
		if ((*aFunction)(aPtr))
			return KErrNone;
		if (!Kern::PowerGood())
			return KErrBadPower;
		if (--aMaxPoll==0)
			return KErrTimedOut;
		NKern::Sleep(ticks);
		}
	}

TUint32 K::CompressKHeapPtr(const TAny* aPtr)
	{
	TUint32 r=(TUint32(aPtr)-TUint32(K::HeapInfo.iBase))>>2;
	__ASSERT_DEBUG(r<(1u<<26),K::Fault(K::EInvalidKernHeapCPtr));
	return r;
	}

const TAny* K::RestoreKHeapPtr(TUint32 aCPtr)
	{
	__ASSERT_DEBUG(aCPtr<(1u<<26),K::Fault(K::EInvalidKernHeapCPtr));
	return (const TAny*)(TUint32(K::HeapInfo.iBase)+(aCPtr<<2));
	}

TUint K::NewId()
	{
	TUint id = __e32_atomic_add_ord32(&K::NextId, 1);
	if(id==~0u)
		K::Fault(K::EOutOfIds);
	return id;
	}

/**	
@pre    No fast mutex can be held.
@pre	Call in a thread context.
@pre	Kernel must be unlocked
@pre	interrupts enabled
*/
EXPORT_C void Kern::CodeSegGetMemoryInfo(DCodeSeg& aCodeSeg, TModuleMemoryInfo& aInfo, DProcess* aProcess)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::CodeSegGetMemoryInfo");
	aCodeSeg.GetMemoryInfo(aInfo, aProcess);
	}

/**
Discovers the DThread associated with an NThread.

@param aNThread	The NThread who's counterpart DThread is to be found.

@return A DThread or NULL if there is no counterpart DThread.
*/
EXPORT_C DThread* Kern::NThreadToDThread(NThread* aNThread)
	{
	if (aNThread && aNThread->iHandlers==&EpocThreadHandlers)
		return _LOFF(aNThread,DThread, iNThread);
	else
		return NULL;
	}

EXPORT_C TKernelHookFn Kern::SetHook(TKernelHookType aType, TKernelHookFn aFunction, TBool aOveride /*=EFalse*/)
	{
	if((TUint)aType>=ENumKernelHooks)
		K::Fault(K::EBadKernelHookType);
	TKernelHookFn oldFn = (TKernelHookFn)__e32_atomic_swp_ord_ptr(&K::KernelHooks[aType], aFunction);
	if(oldFn && !aOveride)
		K::Fault(K::EKernelHookAlreadySet);
	return oldFn;
	}

/**
Wait for a length of time specified in nanoseconds.

This function is typically implemented using a busy-wait, so should only be
called to wait for short periods.

@param aInterval The length of time to wait in nanoseconds.
*/
EXPORT_C void Kern::NanoWait(TUint32 aInterval)
	{
	K::NanoWaitHandler()(aInterval);
	}

extern "C" void nanowait(TUint32 aInterval)
	{
	Kern::NanoWait(aInterval);
	}


/**
Checks of kernel preconditions.
If some precondition is not met and the appropriate macro is defined, this function will print information about broken precondition 
to debug output and fault the system

@param aConditionMask 32-bit bitmask specifying which particular preconditions should be checked
@param aFunction Title of the calling function
*/
#ifdef _DEBUG
#if (defined (__KERNEL_APIS_CONTEXT_CHECKS_WARNING__)||defined (__KERNEL_APIS_CONTEXT_CHECKS_FAULT__))
extern "C" TInt CheckPreconditions(TUint32 aConditionMask, const char* aFunction, TLinAddr aAddr)
	{
	if (K::Initialising || NKern::Crashed())
		return KErrNone;
	
	TUint32 m = aConditionMask;
	NThread* nt = 0;
	DThread* t = 0;
	NKern::TContext ctx = (NKern::TContext)NKern::CurrentContext();
	if (ctx == NKern::EThread)
		{
		nt = NKern::CurrentThread();
		t = Kern::NThreadToDThread(nt);
		}
	if (m & MASK_NO_FAST_MUTEX)
		{
		if (!nt || !NKern::HeldFastMutex())
			m &= ~MASK_NO_FAST_MUTEX;
		}
	if (m & MASK_NO_CRITICAL)
		{
		if (t && t->iThreadType==EThreadUser && nt->iCsCount==0)
			m &= ~MASK_NO_CRITICAL;
		else if (!nt || nt->iCsCount==0)
			m &= ~MASK_NO_CRITICAL;
		}
	if (m & (MASK_CRITICAL|MASK_NO_KILL_OR_SUSPEND))
		{
		if (t && (t->iThreadType!=EThreadUser || nt->iCsCount>0))
			m &= ~(MASK_CRITICAL|MASK_NO_KILL_OR_SUSPEND);
		else if (!nt || nt->iCsCount>0)
			m &= ~(MASK_CRITICAL|MASK_NO_KILL_OR_SUSPEND);
		}
	if (m & MASK_NO_KILL_OR_SUSPEND)
		{
		if (!nt || NKern::KernelLocked() || NKern::HeldFastMutex())
			m &= ~MASK_NO_KILL_OR_SUSPEND;
		}
	if (m & MASK_KERNEL_LOCKED)
		{
		if (NKern::KernelLocked())
			m &= ~MASK_KERNEL_LOCKED;
		}
	if (m & MASK_KERNEL_UNLOCKED)
		{
		if (!NKern::KernelLocked())
			m &= ~MASK_KERNEL_UNLOCKED;
		}
	if (m & MASK_KERNEL_LOCKED_ONCE)
		{
		if (NKern::KernelLocked(1))
			m &= ~MASK_KERNEL_LOCKED_ONCE;
		}
	if (m & MASK_INTERRUPTS_ENABLED)
		{
		if (InterruptsStatus(ETrue))
			m &= ~MASK_INTERRUPTS_ENABLED;
		}
	if (m & MASK_INTERRUPTS_DISABLED)
		{
		if (InterruptsStatus(EFalse))
			m &= ~MASK_INTERRUPTS_DISABLED;
		}
	if (m & MASK_SYSTEM_LOCKED)
		{
		if (TheScheduler.iLock.HeldByCurrentThread())
			m &= ~MASK_SYSTEM_LOCKED;
		}
	if (m & MASK_NOT_THREAD)
		{
		if (ctx!=NKern::EThread)
			m &= ~MASK_NOT_THREAD;
		}
	if (m & MASK_NOT_ISR)
		{
		if (ctx!=NKern::EInterrupt)
			m &= ~MASK_NOT_ISR;
		}
	if (m & MASK_NOT_IDFC)
		{
		if (ctx!=NKern::EIDFC)
			m &= ~MASK_NOT_IDFC;
		}
	if (m & MASK_NO_CRITICAL_IF_USER)
		{
		if (t && (t->iThreadType!=EThreadUser || nt->iCsCount==0))
			m &= ~MASK_NO_CRITICAL_IF_USER;
		else if (!nt || nt->iCsCount==0)
			m &= ~MASK_NO_CRITICAL_IF_USER;
		}
	if (m & MASK_NO_RESCHED)
		{
		if (!nt || NKern::KernelLocked())
			m &= ~MASK_NO_RESCHED;
		}
	if (!m)
		return KErrNone;
	if (aFunction && aAddr)
		{
		Kern::Printf("In function %s called from %08x :-", aFunction, aAddr);
		}
	else 
		{
		if (aFunction)
			Kern::Printf("In function %s :-", aFunction);
		else
			Kern::Printf("At address %08x :-", aAddr);
		}
	if (m & MASK_NO_FAST_MUTEX)
		Kern::Printf("Assertion failed: No fast mutex must be held");
	if (m & MASK_NO_CRITICAL)
		Kern::Printf("Assertion failed: Calling thread must not be in critical section");
	if (m & MASK_CRITICAL)
		Kern::Printf("Assertion failed: Calling thread must be in critical section");
	if (m & MASK_KERNEL_LOCKED)
		Kern::Printf("Assertion failed: Kernel must be locked");
	if (m & MASK_KERNEL_UNLOCKED)
		Kern::Printf("Assertion failed: Kernel must not be locked");
	if (m & MASK_KERNEL_LOCKED_ONCE)
		Kern::Printf("Assertion failed: Kernel must be locked exactly once");
	if (m & MASK_INTERRUPTS_ENABLED)
		Kern::Printf("Assertion failed: Interrupts must be enabled");
	if (m & MASK_INTERRUPTS_DISABLED)
		Kern::Printf("Assertion failed: Interrupts must be disabled");
	if (m & MASK_SYSTEM_LOCKED)
		Kern::Printf("Assertion failed: System lock must be held");
	if (m & MASK_NOT_THREAD)
		Kern::Printf("Assertion failed: Don't call in thread context");
	if (m & MASK_NOT_ISR)
		Kern::Printf("Assertion failed: Don't call in ISR context");
	if (m & MASK_NOT_IDFC)
		Kern::Printf("Assertion failed: Don't call in IDFC context");
	if (m & MASK_NO_CRITICAL_IF_USER)
		Kern::Printf("Assertion failed: Don't call from user thread in critical section");
	if (m & MASK_ALWAYS_FAIL)
		Kern::Printf("Assertion failed");
	if (m & MASK_NO_RESCHED)
		Kern::Printf("Assertion failed: Don't call from thread with kernel unlocked");
	if (m & MASK_NO_KILL_OR_SUSPEND)
		Kern::Printf("Assertion failed: Must not be suspended or killed here");

#ifdef __KERNEL_APIS_CONTEXT_CHECKS_FAULT__
	if (aFunction)
		Kern::Fault(aFunction, 0);
	return KErrGeneral;
#else
	return KErrNone;
#endif//__KERNEL_APIS_CONTEXT_CHECKS_FAULT__
	}	
#endif//__KERNEL_APIS_CONTEXT_CHECKS_WARNING__||__KERNEL_APIS_CONTEXT_CHECKS_FAULT__
#endif


/**
Set the behaviour of text tracing. (Kern::Printf, RDebug::Print etc.)

For example, to disable text trace output to serial port, use:
@code
	Kern::SetTextTraceMode(Kern::ESerialOutNever,Kern::ESerialOutMask);
@endcode

To query the current behaviour:
@code
	TUint textTraceMode = Kern::SetTextTraceMode(0,0);
@endcode

@param aMode Values formed from enum TTextTraceMode.
@param aMask Bitmask indicating which flags are to be modified.
@return The text trace mode in operation before this function was called.

@publishedPartner
*/
EXPORT_C TUint Kern::SetTextTraceMode(TUint aMode, TUint aMask)
	{
	return __e32_atomic_axo_ord32(&K::TextTraceMode, ~aMask, aMode&aMask);
	}


void K::TextTrace(const TDesC8& aText, TTraceSource aTraceSource, TBool aNewLine)
	{
	TBool crashed = NKern::Crashed();
	const TUint8* ptr = aText.Ptr();
	TInt size = aText.Size();

	// Handle BTrace first...
	TUint category;
	switch(aTraceSource)
		{
	case EUserTrace:
		category = BTrace::ERDebugPrintf;
		break;
	case EKernelTrace:
		category = BTrace::EKernPrintf;
		break;
	case EPlatSecTrace:
		category = BTrace::EPlatsecPrintf;
		break;
	default:
		category = ~0u;
		break;
		}
	TInt result = 0;
	if(category!=~0u)
		{
		TUint threadId = KNullThreadId;
		if(!K::Initialising && NKern::CurrentContext()==NKern::EThread)
			{
			NThread* n = NKern::CurrentThread();
			if(n)
				{
				DThread* t = Kern::NThreadToDThread(n);
				if(t)
					threadId = t->iId;
				}
			}
		result = BTraceContextBig(category,0,threadId,ptr,size);
		}

	NThread* csThread = 0;
	if (!K::Initialising && NKern::CurrentContext() == NKern::EThread && !NKern::KernelLocked() && !crashed && InterruptsStatus(ETrue))
		{
		csThread = NCurrentThread();
		NKern::_ThreadEnterCS();
		}

	if(!result)
		if(K::TraceHandler())
			result = K::TraceHandler()(aText, aTraceSource);

	TUint mode = K::TextTraceMode;
	if(mode!=Kern::ESerialOutNever)
		if(mode==Kern::ESerialOutAlways || !result)
			A::DebugPrint(ptr,size,aNewLine);

	if (csThread)
		NKern::_ThreadLeaveCS();
	}

#if defined(_DEBUG) && !defined(__SMP__)
TInt KCrazySchedulerEnabled()
	{
	return TheSuperPage().KernelConfigFlags() & EKernelConfigCrazyScheduling;
	}
#endif

/*
TClientRequest states and synchronization

TClientRequest objects are synchronized based on atomic updates to the iStatus
member using __e32_atomic_xxx_yyy_ptr() operations.

The contents of the iStatus member are made up of a TRequestStatus pointer in
bit 2-31 and two flag bits in bits 0 and 1.

The object can be in the following states indicated by the value in iStatus:

  State:   Pointer:  Bit 1:  Bit 0:
  ---------------------------------
  FREE	   zero		 0		 0
  READY	   non-zero  0       0 
  INUSE	   non-zero  1       0
  CLOSING  non-zero  1       1
  DEAD	   any		 0       1

The following state transitions are possible:

  Start state:	Operation:	End state:
  ------------------------------------
  FREE			Reset		FREE
                Close       DEAD
                SetStatus	READY
  
  READY			Reset       FREE
                Close       DEAD
                Queue       INUSE
  
  INUSE         Callback    FREE
                Close       CLOSING
  
  CLOSING		Callback    DEAD

When the object enters the DEAD state, it is deleted.
*/

inline void IgnorePrintf(...) { }

#define CLIENT_REQUEST_DEBUG IgnorePrintf
//#define CLIENT_REQUEST_DEBUG Kern::Printf

/**
Create a TClientRequest object.

The object is initially in the EFree state.

@param aRequestPtr	A reference to the TClientRequest pointer which is to be set
					to the newly created object.

@return KErrNone, if successful, otherwise one of the other system-wide error codes.

@see TClientRequest::State()

@publishedPartner
@released
*/
EXPORT_C TInt Kern::CreateClientRequest(TClientRequest*& aRequestPtr)
	{
	TClientRequest* self = (TClientRequest*)Kern::Alloc(sizeof(TClientRequest));
	if (!self)
		return KErrNoMemory;
	new (self) TClientRequest;
	T_UintPtr zero = 0;
	if (!__e32_atomic_cas_ord_ptr(&aRequestPtr, &zero, self))
		{
		self->Close();
		return KErrInUse;
		}
	return KErrNone;
	}

/**
@prototype
@internalTechnology
*/
EXPORT_C TInt Kern::CreateClientDataRequestBase(TClientDataRequestBase*& aRequestPtr, TInt aSize)
	{
	TClientDataRequestBase* self = (TClientDataRequestBase*)Kern::Alloc(sizeof(TClientDataRequestBase) + aSize);
	if (!self)
		return KErrNoMemory;
	new (self) TClientDataRequestBase(aSize);
	T_UintPtr zero = 0;
	if (!__e32_atomic_cas_ord_ptr(&aRequestPtr, &zero, self))
		{
		self->Close();
		return KErrInUse;
		}
	return KErrNone;
	}

/**
@prototype
@internalTechnology
*/
EXPORT_C TInt Kern::CreateClientDataRequestBase2(TClientDataRequestBase2*& aRequestPtr, TInt aSize1, TInt aSize2)
	{
	TInt size = _ALIGN_UP(sizeof(TClientDataRequestBase2), 8) + _ALIGN_UP(aSize1, 8) + aSize2;
	TClientDataRequestBase2* self = (TClientDataRequestBase2*)Kern::Alloc(size);
	if (!self)
		return KErrNoMemory;
	new (self) TClientDataRequestBase2(aSize1, aSize2);
	T_UintPtr zero = 0;
	if (!__e32_atomic_cas_ord_ptr(&aRequestPtr, &zero, self))
		{
		self->Close();
		return KErrInUse;
		}
	return KErrNone;
	}

/**
Destroy a TClientRequest object.

The pointer to the object is set to NULL.

@param aRequestPtr A reference to the TClientRequest pointer to free.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@publishedPartner
@released
*/
EXPORT_C void Kern::DestroyClientRequest(TClientRequest*& aRequestPtr)
	{
	TClientRequest* request = (TClientRequest*)__e32_atomic_swp_rel_ptr(&aRequestPtr, 0);
	if (request)
		request->Close();
	}

TClientRequest::TClientRequest(TUserModeCallbackFunc aCallback)
	:	TUserModeCallback(aCallback),
		iStatus(0),
		iResult(KRequestPending)
	{
	}

void TClientRequest::Close()
	{
	CLIENT_REQUEST_DEBUG("%08x TClientRequest::Close", this);
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TClientRequest::Close");
	T_UintPtr status = (T_UintPtr)__e32_atomic_ior_ord_ptr(&iStatus, KClientRequestFlagClosing);
	CLIENT_REQUEST_DEBUG("  state == %d", GetState(status));
	__ASSERT_DEBUG(GetState(status) <= EInUse, K::Fault(K::EClientRequestCloseInWrongState));
	if (!(status & KClientRequestFlagInUse))
		Kern::AsyncFree(this);  // must call async version since current thread may be exiting here
	}

/**
Indicates whether the request is ready to be queued, in other words whether SetState() has been called on it.

Note that this method is not synchronised.  If multiple threads are accessing this object (except by
calling Kern::QueueRequestComplete), then some form of external synchronisation is required.

@publishedPartner
@released
*/
EXPORT_C TBool TClientRequest::IsReady()
	{
	T_UintPtr status = iStatus;	// sample volatile value
	return status && !(status & KClientRequestFlagMask);
	}

TClientRequest::~TClientRequest()
	{
	// This should never be called because we use Kern::Free to free the object after calling
	// Close().  If this is called it means someone deleted a derived object without calling
	// Close().
	CLIENT_REQUEST_DEBUG("%08x TClientRequest::~TClientRequest", this);
	K::Fault(K::EClientRequestDeletedNotClosed);
	}

/**
Get the current state of this object.

A TClientRequest object can be in one of three states, described by the TClientRequest::TState
enumeration.  These are:
 - EFree:    The initial state
 - EReady:   The object has been set up with the TRequestStatus pointer of a client request, and is
             ready to be queued for completion.
 - EInUse:   The object has been queued for completion, but this has not yet occurred.
 - EClosing: The object has been queued for completion and then had Close() called on it, but
             completion has not yet occured.
 
@return The state of the object.
*/
TClientRequest::TState TClientRequest::State()
	{
	return GetState(iStatus);
	}

TClientRequest::TState TClientRequest::GetState(T_UintPtr aStatus)
	{
	if (aStatus == 0)
		return EFree;
	switch (aStatus & KClientRequestFlagMask)
		{
		case 0:
			return EReady;
		case KClientRequestFlagInUse:
			return EInUse;
		case KClientRequestFlagInUse | KClientRequestFlagClosing:
			return EClosing;
		}
	return EBad;
	}

/**
Set the client's TRequestStatus pointer.

This method should be called when the client initiates an asynchronous request.
If the object was initially in the EFree state this method puts it into the
EReady state, otherwise it does nothing.

@return KErrNone if the object state has been transitioned from EFree to EReady
		KErrInUse if the object was not initially in the EFree state

@publishedPartner
@released
*/
EXPORT_C TInt TClientRequest::SetStatus(TRequestStatus* aStatus)
	{
	CLIENT_REQUEST_DEBUG("%08x TClientRequest::SetStatus", this);
	// Return an error if the status pointer is bad.  Don't fault the kernel as this would allow a
	// user thread to crash the system.
	if (((T_UintPtr)aStatus & KClientRequestFlagMask) != 0 || (T_UintPtr)aStatus == KClientRequestNullStatus)
		return KErrArgument;
	T_UintPtr newStatus = aStatus ? (T_UintPtr)aStatus : KClientRequestNullStatus;
	T_UintPtr zero = 0;
	return __e32_atomic_cas_ord_ptr(&iStatus, &zero, newStatus) ? KErrNone : KErrInUse;	// acq?
	}

/**
Get the client's TRequestStatus pointer.

@return The client's TRequestStatus pointer.

@publishedPartner
@released
*/
EXPORT_C TRequestStatus* TClientRequest::StatusPtr()
	{
	return (TRequestStatus*)(iStatus & ~KClientRequestFlagMask);
	}

/**
Queue the request for completion.

If the object is not in the EReady state, this method does nothing.  Otherwise the client thread is
signalled immediately, and the object left in the EInUse state.  When the client thread next runs,
the reason code is written back to it and the object is left in the EFree state.

This method is only synchronised with respect to itself.  Multiple threads can call this method
concurrently and only one will complete the request.

@param aThread  The client thread to which to write the reason code.
@param aRequest The client request object.
@param aReason	The reason code with which to complete the request.

@pre	Call in a thread context.
@pre	Kernel must be unlocked
@pre	Interrupts enabled

@publishedPartner
@released
*/
EXPORT_C void Kern::QueueRequestComplete(DThread* aThread, TClientRequest* aRequest, TInt aReason)
	{
	CLIENT_REQUEST_DEBUG("%08x Kern::QueueRequestComplete %T %d", aRequest, aThread, aReason);
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED | MASK_INTERRUPTS_ENABLED | MASK_NOT_ISR | MASK_NOT_IDFC, "Kern::QueueRequestComplete");
	if (aRequest->StartComplete(aThread, aReason))
		aRequest->EndComplete(aThread);
	}

TBool TClientRequest::StartComplete(DThread* aThread, TInt aReason)
	{
	NKern::ThreadEnterCS();
	T_UintPtr status = iStatus;
	do	{
		if (!status || (status & KClientRequestFlagMask))
			{
			CLIENT_REQUEST_DEBUG("status %08x request not ready", status);
			NKern::ThreadLeaveCS();
			return EFalse;
			}
		} while (!__e32_atomic_cas_ord_ptr(&iStatus, &status, status | KClientRequestFlagInUse));
	iResult = aReason;
	(void)aThread;
#ifdef BTRACE_REQUESTS
	BTraceContext12(BTrace::ERequests,BTrace::ERequestComplete,&aThread->iNThread,iStatus,aReason);
#endif
	return ETrue;
	}

void TClientRequest::EndComplete(DThread* aThread)
	{
	// NB: if the callback is successfully queued, the target thread may run and
	// process it before we get back from the call to QueueUserModeCallback().
	// In that case, 'iStatus' will be changed; and in general it is not safe to
	// look at any element of 'this' once the callback is queued, as it may change
	// asynchronously.  Therefore, we must cache the value of 'iStatus' beforehand
	// and later use this saved copy to decide whether to signal the target thread.
	T_UintPtr status = iStatus;
	TInt r = NKern::QueueUserModeCallback(&aThread->iNThread, this);

	if (r == KErrNone)
		{
		__NK_ASSERT_DEBUG(status & KClientRequestFlagInUse);
		if ((status & ~KClientRequestFlagInUse) != KClientRequestNullStatus)
			NKern::ThreadRequestSignal(&aThread->iNThread);
		}
	else
		{
		__NK_ASSERT_DEBUG(r == KErrDied);
		// Thread was exiting, queue it for cleanup by attaching it to
		// the supervisor thread and queueing a DFC to deal with it
		CLIENT_REQUEST_DEBUG("  queue callback failed, queueing for cleanup");
		NKern::QueueUserModeCallback(K::SvMsgQ->iThread, this);
		DeadClientCleanupDfc.Enque();
		}
	NKern::ThreadLeaveCS();
	}

void TClientRequest::DoDeadClientCleanup(TAny*)
	{
	NKern::CancelUserModeCallbacks();
	}

/**
Reset this object to its initial state so that it can be re-used.

The request pointer is cleared and the state of the object is set to EFree.

This method may only be called when the object is in the EFree or EReady states.

Note that this method is not synchronized.  If multiple threads are accessing
this object (except by calling Kern::QueueRequestComplete), then some form of
external synchronisation is required.

@publishedPartner
@released
*/
EXPORT_C void TClientRequest::Reset()
	{
	CLIENT_REQUEST_DEBUG("%08x TClientRequest::Reset", this);
	T_UintPtr oldStatus = (T_UintPtr)__e32_atomic_swp_ord_ptr(&iStatus, 0);
	CLIENT_REQUEST_DEBUG("oldStatus == %08x", oldStatus);
	__ASSERT_DEBUG(GetState(oldStatus) <= EReady, K::Fault(K::EClientRequestResetInWrongState));
	}

#ifndef __CLIENT_REQUEST_MACHINE_CODED__

void TClientRequest::CallbackFunc(TAny* aData, TUserModeCallbackReason aReason)
	{
	TClientRequest* req = (TClientRequest*)aData;
	CLIENT_REQUEST_DEBUG("%08x TClientRequest::CallbackFunc", req);
	TInt result = req->iResult;
	
	// Ensure request object can be reused before write to user-space takes place
	T_UintPtr statusPtr = req->MakeFree() & ~KClientRequestFlagMask;
	
	if (aReason == EUserModeCallbackRun && statusPtr != KClientRequestNullStatus)
		K::USafeWrite((TAny*)statusPtr, &result, sizeof(result));
	}

#endif

T_UintPtr TClientRequest::MakeFree()
	{
	// Move callback to the free state, deleting it if necessary
	CHECK_PRECONDITIONS(MASK_CRITICAL,"TClientRequest::MakeFree"); // needed for Kern::AsyncFree
	iResult = KRequestPending;
	T_UintPtr oldStatus = (T_UintPtr)__e32_atomic_and_ord_ptr(&iStatus, KClientRequestFlagClosing);
	CLIENT_REQUEST_DEBUG("MakeFree %08x oldStatus %08x", this, oldStatus);
	__ASSERT_DEBUG(GetState(oldStatus)==EInUse || GetState(oldStatus)==EClosing, K::Fault(K::EClientRequestCallbackInWrongState));
	if (oldStatus & KClientRequestFlagClosing)
		Kern::AsyncFree(this);  // must call async version since current thread may be exiting here
	return oldStatus;
	}

TClientDataRequestBase::TClientDataRequestBase(TInt aBufferSize) :
	TClientRequest(CallbackFunc),
	iSize(aBufferSize)
	{
	}

void TClientDataRequestBase::CallbackFunc(TAny* aData, TUserModeCallbackReason aReason)
	{
	TClientDataRequestBase* req = (TClientDataRequestBase*)aData;
	
#ifdef _DEBUG
	TState state = GetState(req->iStatus);
	__ASSERT_DEBUG(state == EInUse || state == EClosing, K::Fault(K::EClientRequestCallbackInWrongState));
#endif

	if (aReason == EUserModeCallbackRun)
		K::USafeWrite(req->iDestPtr, req->Buffer(), req->iSize);

	TClientRequest::CallbackFunc(aData, aReason);
	}

TClientDataRequestBase2::TClientDataRequestBase2(TInt aBufferSize1, TInt aBufferSize2) :
	TClientRequest(CallbackFunc),
	iSize1(aBufferSize1),
	iSize2(aBufferSize2)
	{
	}

void TClientDataRequestBase2::CallbackFunc(TAny* aData, TUserModeCallbackReason aReason)
	{
	TClientDataRequestBase2* req = (TClientDataRequestBase2*)aData;
	
#ifdef _DEBUG
	TState state = GetState(req->iStatus);
	__ASSERT_DEBUG(state == EInUse || state == EClosing, K::Fault(K::EClientRequestCallbackInWrongState));
#endif

	if (aReason == EUserModeCallbackRun)
		{
		K::USafeWrite(req->iDestPtr1, req->Buffer1(), req->iSize1);
		K::USafeWrite(req->iDestPtr2, req->Buffer2(), req->iSize2);
		}
	
	TClientRequest::CallbackFunc(aData, aReason);
	}

// TClientBuffer implementation

#ifndef __MARM__

/**
Read the header of a user-side descriptor in the current process, parse it, and populate a
TDesHeader with the result.

@param aDesPtr The descriptor for which information is to be fetched.
@param aOut    On return, set to the parsed contents of the descriptor header.

@return KErrNone if successful, or one of the system-wide error codes.

@pre  Interrupts must be enabled.
@pre  Kernel must be unlocked.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Can be used in a device driver.
*/
TInt K::USafeReadAndParseDesHeader(TAny* aDesPtr, TDesHeader& aOut)
	{
	CHECK_PAGING_SAFE;
	static const TUint8 LengthLookup[16]={4,8,12,8,12,0,0,0,0,0,0,0,0,0,0,0};
	TRawDesHeader header;
	const TUint32* pS=(const TUint32*)aDesPtr;
	if (!pS || (TInt(pS)&3)!=0)
		return KErrBadDescriptor;
	if (K::USafeRead(pS,&header[0],sizeof(TUint32)))
		return KErrBadDescriptor;
	TInt type=header[0]>>KShiftDesType8;
	TInt l=LengthLookup[type];
	if (l==0)
		return KErrBadDescriptor;
	if (l>(TInt)sizeof(TUint32) && K::USafeRead(pS+1,&header[1],l-sizeof(TUint32)))
		return KErrBadDescriptor;
	return K::ParseDesHeader(aDesPtr, header, aOut);
	}

#endif

// Parse a descriptor header, return KErrBadDescriptor if there's an error
// Note that this can parse a header in-place (i.e. when &aIn == &aOut)
TInt K::ParseDesHeader(const TAny* aDes, const TRawDesHeader& aIn, TDesHeader& aOut)
	{
	TUint type = aIn[0] >> KShiftDesType;
	TUint len = aIn[0] & KMaskDesLength;
	TUint max = (TUint)TDesHeader::KConstMaxLength;
	TLinAddr p;
	switch (type)
		{
		case EBufC:		p=(TLinAddr)aDes+sizeof(TDesC); break;
		case EPtrC:		p=(TLinAddr)aIn[1]; break;
		case EPtr:		p=(TLinAddr)aIn[2]; max=(TInt)aIn[1]; break;
		case EBuf:		p=(TLinAddr)aDes+sizeof(TDes); max=(TInt)aIn[1]; break;
		case EBufCPtr:	p=(TLinAddr)aIn[2]+sizeof(TDesC); max=(TInt)aIn[1]; break;
		default:
			return KErrBadDescriptor;
		}
	if (len>max || (type == EBufCPtr && ((TUint)p & 3) != 0))
		return KErrBadDescriptor;
	aOut.Set(aIn[0], p, max);
	return KErrNone;
	}

/**
Create a TClientBuffer object.

The object is not initially populated with information about a buffer, and the IsSet() method will
return false.
*/
EXPORT_C TClientBuffer::TClientBuffer() :
	iPtr(0)
	{
	}

/**
Indicates whether this object has been set by calling either SetFromDescriptor() or SetFromBuffer().

@return Whether the object has been set.
*/
EXPORT_C TBool TClientBuffer::IsSet() const
	{
	return iPtr != 0;
	}

/**
Reset this object to its initial state.

Calling IsSet() will subsequently return false.

@publishedPartner
@released
*/
EXPORT_C void TClientBuffer::Reset()
	{
	iPtr = 0;
	}

/**
Set this object to refer to a client descriptor.

@param aDesPtr       A pointer to the client's descriptor (in user memory).
@param aClientThread This should normally be NULL to indicate the current thread, although a
                     different thread can be specified.

The descriptor (including the header) is expected to reside in user memory.  The header is read in the process of populating this object.

Calling IsSet() will subsequently return true.

@publishedPartner
@released
*/ 
EXPORT_C TInt TClientBuffer::SetFromDescriptor(TAny* aDesPtr, DThread* aClientThread)
	{
	iPtr = (TUint32)aDesPtr;
	__NK_ASSERT_ALWAYS((iPtr & 3) == 0);
	TInt r;
	if (aClientThread)
		{
#ifndef __MEMMODEL_FLEXIBLE__
		NKern::LockSystem();
#endif
		r = aClientThread->ReadAndParseDesHeader(aDesPtr, iHeader);
#ifndef __MEMMODEL_FLEXIBLE__
		NKern::UnlockSystem();
#endif
		}
	else
		r = K::USafeReadAndParseDesHeader(aDesPtr, iHeader);
	return r;
	}

/**
Set this object to refer to a client buffer specified by start address and length.

@param aStartAddr    The start address of the buffer (in user memory)
@param aLength       The length of the buffer in bytes.
@param aWriteable    Whether the buffer should be written to by kernel-side code.

The buffer is expected to reside in user memory.

Calling IsSet() will subsequently return true.

@publishedPartner
@released
*/ 
EXPORT_C void TClientBuffer::SetFromBuffer(TLinAddr aStartAddr, TInt aLength, TBool aWriteable)
	{
	iPtr = EIsBuffer;
	if (aWriteable)
		iHeader.Set(EPtr << KShiftDesType8, aStartAddr, aLength);
	else
		iHeader.Set((EPtrC << KShiftDesType8) | aLength, aStartAddr);
	}

/**
Indicates whether the client descriptor is writeable, as opposed to constant.

@return Whether the client descriptor is writeable.

@publishedPartner
@released
*/
EXPORT_C TBool TClientBuffer::IsWriteable() const
	{
	return iHeader.IsWriteable();
	}

/**
Get the length of the client's descriptor.

@return The length of the descriptor

@publishedPartner
@released
*/
EXPORT_C TInt TClientBuffer::Length() const
	{
	return iHeader.Length();
	}

/**
Get the maximum length of the client's writeable descriptor.

@return The length of the descriptor on sucess, otherwise one of the system-wide error codes.

@publishedPartner
@released
*/
EXPORT_C TInt TClientBuffer::MaxLength() const
	{
	return iHeader.MaxLength();
	}

TAny* TClientBuffer::DesPtr() const
	{
	return (TAny*)(iPtr & ~3);
	}

TAny* TClientBuffer::DataPtr() const
	{
	return (TAny*)iHeader.DataPtr();
	}

/**
Update the client's descriptor header to reflect the length of data written to the buffer.

@param aClientThread This should normally be NULL to indicate the current thread, although a
                     different thread can be specified.

This method should be called (usually in the context of the client thread) after the buffer has been
written to using Kern::ThreadBufWrite().

If this object was not set by calling SetFromDescriptor(), this method does nothing.

@return KErrNone if successful, or KErrBadDescriptor if there was an exception while updating the length.

@publishedPartner
@released
*/
EXPORT_C TInt TClientBuffer::UpdateDescriptorLength(DThread* aClientThread)
	{
	TInt r = KErrNone;

	if ((iPtr & EIsBuffer) == 0 && IsWriteable())
		{
		if (aClientThread)
			r = Kern::ThreadRawWrite(aClientThread, (TAny*)iPtr, &iHeader.TypeAndLength(), sizeof(TUint32));
		else
			{
			TAny* excAddr = K::USafeWrite((TAny*)iPtr, &iHeader.TypeAndLength(), sizeof(TUint32));
			if (excAddr != NULL)
				r = KErrBadDescriptor;
			}
		if (r == KErrNone && iHeader.Type() == EBufCPtr)
			{
			TInt len = iHeader.Length();
			TUint8* pL = (TUint8*)(iHeader.DataPtr() - sizeof(TDesC));
			if (aClientThread)
				r = Kern::ThreadRawWrite(aClientThread, (TAny*)pL, &len, sizeof(TUint32));
			else
				{
				TAny* excAddr = K::USafeWrite((TAny*)pL, &len, sizeof(TUint32));
				if (excAddr != NULL)
					r = KErrBadDescriptor;
				}
			}
		}
	return r;
	}

// Implementation of TClientBufferRequest

NFastMutex TClientBufferRequest::Lock;

TClientBufferRequest::TClientBufferRequest(TUint aFlags) :
	TClientRequest(TClientBufferRequest::CallbackFunc),
	iFlags(aFlags)
	{
	}

TInt TClientBufferRequest::AllocateBufferData()
	{
	// allocate data for one buffer and add it to the end of the list
	SBufferData* item = new SBufferData;
	if (item == NULL)
		return KErrNoMemory;
	if (iFlags & EPinVirtual)
		{
		TInt r = Kern::CreateVirtualPinObject(item->iPinObject);
		if (r != KErrNone)
			{
			delete item;
			return r;
			}
		}
	iBufferList.Add(item);
	return KErrNone;
	}

TInt TClientBufferRequest::Construct(TInt aInitialBuffers)
	{
	TInt r = KErrNone;
	for (TInt i = 0 ; r == KErrNone && i < aInitialBuffers ; ++i)
		r = AllocateBufferData();
	return r;
	}

/**
Create a TClientBufferRequest object.

@param aInitialBuffers The number of buffer slots to allocate initially.  
@param aFlags          Indicates whether buffers should have their virtual memory pinned.

@publishedPartner
@released
*/
EXPORT_C TInt Kern::CreateClientBufferRequest(TClientBufferRequest*& aRequestPtr, TUint aInitialBuffers, TUint aFlags)
	{
	TClientBufferRequest* self = (TClientBufferRequest*)Kern::Alloc(sizeof(TClientBufferRequest));
	if (!self)
		return KErrNoMemory;
	new (self) TClientBufferRequest(aFlags);
	TInt r = self->Construct(aInitialBuffers);
	T_UintPtr zero = 0;
	if (r == KErrNone && !__e32_atomic_cas_ord_ptr(&aRequestPtr, &zero, self))
		r = KErrInUse;
	if (r != KErrNone)
		self->Close();
	return r;
	}

void TClientBufferRequest::Close()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TClientBufferRequest::Close");
	T_UintPtr status = (T_UintPtr)__e32_atomic_ior_ord_ptr(&iStatus, KClientRequestFlagClosing);
	__ASSERT_DEBUG(GetState(status) <= EInUse, K::Fault(K::EClientRequestCloseInWrongState));
	if (!(status & KClientRequestFlagInUse))
		{
		SBufferData* item;
		while(item = (SBufferData*)iBufferList.GetFirst(), item != NULL)
			{
			Kern::DestroyVirtualPinObject(item->iPinObject);  // todo
			Kern::AsyncFree(item);
			}
		Kern::AsyncFree(this);  // must call async version since current thread may be exiting here
		}
	}

/**
Destroy a TClientBufferRequest object.

@publishedPartner
@released
*/
EXPORT_C void Kern::DestroyClientBufferRequest(TClientBufferRequest*& aRequestPtr)
	{
	TClientBufferRequest* request = (TClientBufferRequest*)__e32_atomic_swp_rel_ptr(&aRequestPtr, 0);
	if (request)
		request->Close();
	}

#define iMState     iWaitLink.iSpare1

/**
Start the setup process and set the client's TRequestStatus pointer.

This method should be called first when the client initiates an asynchronous request, in the context
of the client thread.

After calling this, the driver can call AddBuffer the appropriate number of times.

@return KErrNone if successful, or KErrInUse if the object has already been setup.

@publishedPartner
@released
*/
EXPORT_C TInt TClientBufferRequest::StartSetup(TRequestStatus* aStatus)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TClientBufferRequest::StartSetup");
	NKern::FMWait(&Lock);
	TInt r = TClientRequest::SetStatus(aStatus);
	if (r == KErrNone)
		{
		__NK_ASSERT_DEBUG(iSetupThread == NULL || iSetupThread->iMState == DThread::EDead);
		if (iSetupThread)
 			iSetupThread->Close(NULL);
		iSetupThread = TheCurrentThread;
		iSetupThread->Open();
		}
	NKern::FMSignal(&Lock);
	return r;
	}

TClientBufferRequest::SBufferData* TClientBufferRequest::StartAddBuffer()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TClientBufferRequest::AddBuffer");
	if (iSetupThread != TheCurrentThread)
		K::Fault(K::EBufferRequestAddInWrongState);
	TInt r = KErrNone;
	if (((SBufferData*)iBufferList.Last())->iBuffer.IsSet())
		{
		r = AllocateBufferData();	
		if (r != KErrNone)
			{
			Reset();
			return NULL;
			}
		}
	SBufferData* data = (SBufferData*)iBufferList.Last();
	__NK_ASSERT_DEBUG(!data->iBuffer.IsSet());
	return data;
	}

TInt TClientBufferRequest::EndAddBuffer(TClientBuffer*& aBufOut, SBufferData* aData)
	{
	if (iFlags & EPinVirtual)
		{
		TInt r = Kern::PinVirtualMemory(aData->iPinObject, aData->iBuffer);
		if (r != KErrNone)
			{
			Reset();
			aData->iBuffer.Reset();
			aBufOut = 0;
			return r;
			}
		}
	iBufferList.Rotate();
	aBufOut = &aData->iBuffer;
	return KErrNone;
	}

/**
Associate a user-side descriptor with this request, and optionally pin it.

This method should be called after StartSetup when the client initiates an asynchronous request, in
the context of the client thread.  If StartSetup has not been called, this method panics.

This method can be called multiple times.

The descriptor header is read into the kernel from the current process' address space, and if
requested the memory is pinned.

@return On success, a pointer to a TClientBuffer, which should be used to write to the descriptor.
        NULL if there was not enough memory to complete the operation.

@publishedPartner
@released
*/
EXPORT_C TInt TClientBufferRequest::AddBuffer(TClientBuffer*& aBufOut, TAny* aDesPtr)
	{
	SBufferData* data = StartAddBuffer();
	if (data == NULL)
		return KErrNoMemory;
	data->iBuffer.SetFromDescriptor(aDesPtr);
	return EndAddBuffer(aBufOut, data);
	}

/**
Associate a user-side memory buffer with this request, and optionally pin it.

This method should be called after StartSetup when the client initiates an asynchronous request, in
the context of the client thread.  If StartSetup has not been called, this method faults the kernel.

This method can be called multiple times.

If requested, the memory is pinned.

@return On success, a pointer to a TClientBuffer, which can be used to write to the buffer.
        NULL if there was not enough memory to complete the operation.

@publishedPartner
@released
*/
EXPORT_C TInt TClientBufferRequest::AddBuffer(TClientBuffer*& aBufOut, TLinAddr aStartAddr, TInt aLength, TBool aWriteable)
	{
	SBufferData* data = StartAddBuffer();
	if (data == NULL)
		return KErrNoMemory;
	data->iBuffer.SetFromBuffer(aStartAddr, aLength, aWriteable);
	return EndAddBuffer(aBufOut, data);
	}

/**
Complete the setup process.

This method should always be called if the setup process has completed successfully, after any calls
to AddBuffer.  It is not necessary to call this if StartSetup or AddBuffer return an error.

This should always be called in the context of the client thread.

@publishedPartner
@released
*/
EXPORT_C void TClientBufferRequest::EndSetup()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TClientBufferRequest::EndSetup");
	NKern::FMWait(&Lock);
	if (iSetupThread != TheCurrentThread)
		K::Fault(K::EBufferRequestEndSetupInWrongState);
	DThread* thread = iSetupThread;
	iSetupThread = NULL;
	NKern::ThreadEnterCS();
	NKern::FMSignal(&Lock);
	thread->Close(NULL);
	NKern::ThreadLeaveCS();
	}

/**
Reset this object to allow it be reused, without completing the client request.

This may be called at any time.  It must be called in the context of the client thread.

@publishedPartner
@released
*/
EXPORT_C void TClientBufferRequest::Reset()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TClientBufferRequest::Reset");
	NKern::FMWait(&Lock);
	TBool inSetup = iSetupThread != NULL;
	if (inSetup && iSetupThread != TheCurrentThread)
		K::Fault(K::EBufferRequestResetInWrongState);
	if (!inSetup)
		{
		TClientRequest::Reset();
		NKern::FMSignal(&Lock);
		return;
		}
	NKern::FMSignal(&Lock);	
	SDblQueLink* link = iBufferList.First();
	while (link != &iBufferList.iA)
		{
		SBufferData* data = (SBufferData*)link;
		data->iBuffer.Reset();
		if (iFlags & TClientBufferRequest::EPinVirtual)
			Kern::UnpinVirtualMemory(data->iPinObject);
		link = data->iNext;
		}
	NKern::FMWait(&Lock);
	TClientRequest::Reset();
	DThread* thread = iSetupThread;
	iSetupThread = NULL;
	NKern::ThreadEnterCS();
	NKern::FMSignal(&Lock);
	thread->Close(NULL);
	NKern::ThreadLeaveCS();
	}

/**
Queue the request for completion.

If the object has not been setup by calling StartSetup/AddBuffer/EndSetup, this method does nothing.
Otherwise, if unpins any memory that was pinned by calling AddBuffer, and causes the client's
TRequestStatus and any writeable descriptor lengths to be written back to the client thread when it
next runs.

This method is not synchronised, and therefore should only ever be called from the context of a
single thread (for example a DFC queue thread).  Alternatively, an external synchonisation mechanism
such as a mutex can be used.

@prototype
@internalTechnology
*/
EXPORT_C void Kern::QueueBufferRequestComplete(DThread* aThread, TClientBufferRequest* aRequest, TInt aReason)
	{
	aRequest->QueueComplete(aThread, aReason);
	}

void TClientBufferRequest::QueueComplete(DThread* aThread, TInt aReason)
	{
	NKern::FMWait(&Lock);
	TBool ready = iSetupThread == NULL && TClientRequest::StartComplete(aThread, aReason);
	NKern::FMSignal(&Lock);
	if (!ready)
		return;
	if (iFlags & TClientBufferRequest::EPinVirtual)
		{
		SDblQueLink* link = iBufferList.First();
		while (link != &iBufferList.iA)
			{
			TClientBufferRequest::SBufferData* data = (TClientBufferRequest::SBufferData*)link;
			Kern::UnpinVirtualMemory(data->iPinObject);
			link = data->iNext;
			}
		}
	EndComplete(aThread);
	}

void TClientBufferRequest::CallbackFunc(TAny* aData, TUserModeCallbackReason aReason)
	{
	TClientBufferRequest* self = (TClientBufferRequest*)aData;
	
	TState state = GetState(self->iStatus);
	__ASSERT_DEBUG(state == EInUse || state == EClosing, K::Fault(K::EClientRequestCallbackInWrongState));

	if (aReason == EUserModeCallbackRun)
		{
		SDblQueLink* link = self->iBufferList.First();
		while (link != &self->iBufferList.iA)
			{
			SBufferData* data = (SBufferData*)link;
			if (data->iBuffer.IsSet())
				{
				if (self->iFlags & TClientBufferRequest::EPinVirtual)
					data->iBuffer.UpdateDescriptorLength();  // ignore error here
				data->iBuffer.Reset();
				}
			link = data->iNext;
			}
		}

	if (state == EClosing)
		{
		SBufferData* item;
		while(item = (SBufferData*)(self->iBufferList.GetFirst()), item != NULL)
			{
			Kern::DestroyVirtualPinObject(item->iPinObject);
			Kern::AsyncFree(item);
			}
		}

	TClientRequest::CallbackFunc(aData, aReason);
	}

// Implementation of kernel pin APIs

/**
Create an object which can be used to pin virtual memory.

@param aPinObject A reference to a pointer which is set to the newly-created object on success.

@return KErrNone, if successful, otherwise one of the other system-wide error codes.

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Suitable for use in a device driver.

@see Kern::DestroyVirtualPinObject()

@prototype
*/
EXPORT_C TInt Kern::CreateVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::CreateVirtualPinObject");			
	return M::CreateVirtualPinObject(aPinObject);
	}

/**
Pin an area of virtual memory.

The act of pinning virtual memory means that the memory in the specified virtual address range is
guaranteed to remain in system RAM while it is pinned, unless it is decommited.  The actual physical
RAM used is not guaranteed to stay the same however, as it could be replaced in the process of RAM
defragmentation.

This operation is provided to enable device drivers to pin client memory in the context of the
client thread, so that when it is accessed from a different thread later on (for example from a DFC
thread) there is no possibility of taking page faults.

Note that this operation may fail with KErrNoMemory.

@param aPinObject  A virtual pin object previously created by calling Kern::CreateVirtualPinObject().
@param aStart      The start address of the memory to pin.
@param aSize       The size of the memory to pin in bytes.
@param aThread	   The thread that owns the memory to pin, or NULL to use the current thread.

@return KErrNone, if successful, otherwise one of the other system-wide error codes.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::UnpinVirtualMemory()

@prototype
*/
EXPORT_C TInt Kern::PinVirtualMemory(TVirtualPinObject* aPinObject, TLinAddr aStart, TUint aSize, DThread* aThread)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::PinVirtualMemory");
	if (aThread == NULL)
		aThread = TheCurrentThread;
	if (aSize == 0)
		return KErrNone;
	NKern::ThreadEnterCS();
	TInt r = M::PinVirtualMemory(aPinObject, aStart, aSize, aThread);
	NKern::ThreadLeaveCS();
	return r;
	}

/**
Pin an area of virtual memory.

The act of pinning virtual memory means that the memory in the specified virtual address range is
guaranteed to remain in system RAM while it is pinned, unless it is decommited.  The actual phyiscal
RAM used is not guaranteed to stay the same however, as it could be replaced in the process of RAM
defragmentation.

This operation is provided to enable device drivers to pin client memory in the context of the
client thread, so that when it is accessed from a different thread later on (for example from a DFC
thread) there is no possibility of taking page faults.

Note that this operation may fail with KErrNoMemory.

Note - Instances of TVirtualPinObject are not protected from concurrent operations being performed 
with them by separate threads i.e. it is the responsibility of the creator of a TVirtualPinObject 
instance to ensure that it will not be pinned, unpinned or destroyed by more than one thread at a time.

@param aPinObject  A virtual pin object previously created by calling Kern::CreateVirtualPinObject().
@param aDes	       A TClientBuffer object representing a client descriptor to pin.
@param aThread	   The thread that owns the memory to pin, or NULL to use the current thread.

@return KErrNone, if successful, otherwse one of the other system-wide error codes.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::UnpinVirtualMemory()

@prototype
*/
EXPORT_C TInt Kern::PinVirtualMemory(TVirtualPinObject* aPinObject, const TClientBuffer& aDes, DThread* aThread)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::PinVirtualMemory");
	if (aThread == NULL)
		aThread = TheCurrentThread;
	TInt length = aDes.IsWriteable() ? aDes.MaxLength() : aDes.Length();
	if (length < 0)
		return length;
	if (length == 0)
		return KErrNone;
	NKern::ThreadEnterCS();
	TInt r = M::PinVirtualMemory(aPinObject, (TLinAddr)aDes.DataPtr(), length, aThread);
	NKern::ThreadLeaveCS();
	return r;
	}
/**
Create a pin object and then pin an area of virtual memory in the current address space.  If 
an error occurs then no pin object will exist

The act of pinning virtual memory means that the memory in the specified virtual address range is
guaranteed to remain in system RAM while it is pinned, unless it is decommited.  The actual physical
RAM used is not guaranteed to stay the same however, as it could be replaced in the process of RAM
defragmentation.

This operation is provided to enable device drivers to pin client memory in the context of the
client thread, so that when it is accessed from a different thread later on (for example from a DFC
thread) there is no possibility of taking page faults.

Note that this operation may fail with KErrNoMemory.

Note - Instances of TVirtualPinObject are not protected from concurrent operations being performed 
with them by separate threads i.e. it is the responsibility of the creator of a TVirtualPinObject 
instance to ensure that it will not be pinned, unpinned or destroyed by more than one thread at a time.

@param aPinObject	A reference to a pointer which is set to the newly-created object on success.
@param aStart		The start address of the memory to pin.
@param aSize		The size of the memory to pin in bytes.

@return KErrNone, if successful, otherwise one of the other system-wide error codes.

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::UnpinVirtualMemory()
@see Kern::DestroyVirtualPinObject()

@prototype
*/
EXPORT_C TInt Kern::CreateAndPinVirtualMemory(TVirtualPinObject*& aPinObject, TLinAddr aStart, TUint aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::CreateAndPinVirtualMemory");
	return M::CreateAndPinVirtualMemory(aPinObject, aStart, aSize);
	}


/**
Unpin an area of memory previously pinned by calling Kern::PinVirtualMemory().

@param aPinObject  The virtual pin object used to pin the memory.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::PinVirtualMemory()

@prototype
*/
EXPORT_C void Kern::UnpinVirtualMemory(TVirtualPinObject* aPinObject)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::UnpinVirtualMemory");
	NKern::ThreadEnterCS();
	M::UnpinVirtualMemory(aPinObject);
	NKern::ThreadLeaveCS();
	}

/**
Dispose of a virtual pin object which is no longer required.

Any memory pinned by the object is unpinned first.

@param	aPinObject	A reference to a pointer to the pin object to destroy.
					This pointer will be set to NULL on return.

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Suitable for use in a device driver.

@see Kern::CreateVirtualPinObject()

@prototype
*/
EXPORT_C void Kern::DestroyVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::DestroyVirtualPinObject");			
	M::DestroyVirtualPinObject(aPinObject);
	}

/**
Creates an object which is used to pin physical memory. Supported by Kernel running flexible memory model.

Note - Instances of TPhysicalPinObject are not protected from concurrent operations being performed 
with them by separate threads i.e. it is the responsibility of the creator of a TPhysicalPinObject 
instance to ensure that it will not be pinned, unpinned or destroyed by more than one thread at a time.

@param aPinObject A reference to a pointer which is set to the newly-created object on success.

@return KErrNotSupported on memory models other then flexible.
		KErrNone, if successful, otherwise one of the other system-wide error codes.

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Suitable for use in a device driver.

@see Kern::DestroyPhysicalPinObject()

@prototype
*/
EXPORT_C TInt Kern::CreatePhysicalPinObject(TPhysicalPinObject*& aPinObject)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::CreatePhysicalPinObject");
	return M::CreatePhysicalPinObject(aPinObject);
	}

/**
Pins an area of physical memory. Supported by Kernel running flexible memory model.

The physical memory to pin is defined by its existing virtual mapping (by aStart, aSize & aThread parameters).
On return, aAddress will hold physical address (if memory is mapped contigiously) and aPages
area will be populated with the list of physical pages of the mapping. aColour will hold the mapping colour
of the first physical page in the mapping.

This operation is provided to enable device drivers to operate DMA transfers on memory which is not mapped to
Kernel address space (but to user client's, instead).

The act of pinning physical memory means that it is guaranteed to be excluded from RAM defragmentation.
However, it can still be the subject of demand paging. Physically pinned memory is also guaranteed not to be
reused for some other purpose - even if the process owning the memory decommits it or terminates.

Note that this operation may fail with KErrNoMemory.

@param aPinObject		A physical pin object previously created by calling Kern::CreatePhysicalPinObject().
@param aStart			Virtual address of memory to pin.
@param aSize			The length (in bytes) of memory to pin.
@param aThread	   		The thread that owns the memory to pin, or NULL to use the current thread.
@param aReadOnlyMemory  Set to ETrue if the content of physical memory is not going to change while being
						pinned, e.g. if it is DMA copied into H/W. Set to EFalse otherwise.
						Setting this argument to ETrue will improve the performance when/if memory is paged out.
@param aAddress 		On success, this value is set to one of two values:
						- If the specified region is physically contiguous, the value is the
						  physical address of the first byte in the region.
						- If the region is discontiguous, the value is set to KPhysAddrInvalid.
@param aPages			Points to area of TPhysAddr which will on exit hold the addresses of the physical pages contained
						in the specified region. The array must be large enough to hold the whole list of pages in the region.
						If aPageList is zero , then the function will fail with KErrNotFound if the specified region
						is not physically contiguous.
@param aMapAttr         On success, this is set to the mmu mapping attributes used for the memory. This
						is a value constructed from the bit masks in the enumeration TMappingAttributes. The typical
                        use for this value is to use it as an argument to to Kernel's Sync Physical Memory interface.
						
@param aColour			On exit, holds the mapping colour of the first physical page in the mapping. Device drivers
   						have no use of this value but to pass to Kernel's Sync Physical Memory interface.

@return 				KErrNotSupported on memory models other then flexible.
						KErrNone, if successful, otherwise one of the other system-wide error codes.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::UnpinPhysicalMemory()
@see Cache::SyncPhysicalMemoryBeforeDmaWrite()
@see Cache::SyncPhysicalMemoryBeforeDmaRead()
@see Cache::SyncPhysicalMemoryAfterDmaRead()
@prototype
 */
EXPORT_C TInt Kern::PinPhysicalMemory(TPhysicalPinObject* aPinObject, TLinAddr aStart, TUint aSize, TBool aReadOnlyMemory,
				 				TPhysAddr& aAddress, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour, DThread* aThread)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::PinPhysicalMemory");
	if (aThread == NULL)
		aThread = TheCurrentThread;
	if (aSize == 0)
		return KErrNone;
	NKern::ThreadEnterCS();
	TInt r = M::PinPhysicalMemory(aPinObject, aStart, aSize, aReadOnlyMemory, aAddress, aPages, aMapAttr, aColour, aThread);
	NKern::ThreadLeaveCS();
	return r;
	}

/**
Unpins an area of physical memory previously pinned by calling Kern::PinPhysicalMemory().

@param aPinObject  The physical pin object used to pin the memory.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@return 				KErrNotSupported on memory models other then flexible.
						KErrNone, on flexible memory model.

@see Kern::PinPhysicalMemory()

@prototype
*/
EXPORT_C TInt Kern::UnpinPhysicalMemory(TPhysicalPinObject* aPinObject)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::UnpinPhysicalMemory");
	NKern::ThreadEnterCS();
	M::UnpinPhysicalMemory(aPinObject);
	NKern::ThreadLeaveCS();
	return KErrNone;
	}

/**
Dispose of a physical pin object which is no longer required.

Any memory pinned by the object is unpinned first.

@param	aPinObject	A reference to a pointer to the pin object to destroy.
					This pointer will be set to NULL on return.

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Suitable for use in a device driver.

@return 				KErrNotSupported on memory models other then flexible.
						KErrNone, on flexible memory model.

@see Kern::CreatePhysicalPinObject()

@prototype
*/
EXPORT_C TInt Kern::DestroyPhysicalPinObject(TPhysicalPinObject*& aPinObject)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::DestroyPhysicalPinObject");
	M::DestroyPhysicalPinObject(aPinObject);
	return KErrNone;
	}


/**
Creates an object which is used to map user side memory in the kernel address space 
and also physically pin the memory. Supported by Kernel running the flexible memory model.

When the map object will only be used to map non-demand paged memory aMaxReserveSize can be used 
to pre-reserve the resources that would be required for Kern::MapAndPinMemory().  This will 
prevent Kern::MapAndPinMemory() failing due to low memory and will reduce the time to execute
Kern::MapAndPinMemory() but only when the mapping object is used to map non-demand paged memory.

Note - Instances of TKernelMapObject are not protected from concurrent operations being performed 
with them by separate threads i.e. it is the responsibility of the creator of a TKernelMapObject 
instance to ensure that it will not be mapped, unmapped or destroyed by more than one thread at a time.

@param aMapObject 		A reference to a pointer which is set to the newly created object on success.
@param aMaxReserveSize	When set to zero (the default) the resources required to map memory will be
						allocated when Kern::MapAndPinMemory() is invoked with the newly created map 
						object.	 When not set to zero all resources required for the mapping object 
						to map up to aMaxReserveSize bytes will be reserved when creating the mapping 
						object.  When the mapping object is used to map non-demand paged memory, 
						this will prevent Kern::MapAndPinMemory() from returning KErrNoMemory 
						when invoked with the mapping object.  However, it will limit the mapping 
						object to being able to map a maximum of aMaxReserveSize bytes and may also
						waste resources if it is used to map less than aMaxReserveSize bytes.

@return KErrNotSupported on memory models other then flexible.
		KErrNone, if successful, otherwise one of the other system-wide error codes.

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Suitable for use in a device driver.

@see Kern::DestroyKernelMapObject()
@see Kern::MapAndPinMemory()

@prototype
*/
EXPORT_C TInt Kern::CreateKernelMapObject(TKernelMapObject*& aMapObject, TUint aMaxReserveSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::CreateKernelMapObject");			
	return M::CreateKernelMapObject(aMapObject, aMaxReserveSize);
	}


/**
Maps an area of memory into the kernel address space and physically pins it. Supported by Kernel running 
the flexible memory model.

The memory to map and pin is defined by its existing virtual mapping (via the aStart, aSize & aThread parameters).
On return, if aPages is not NULL, aPages will be populated with the list of physical pages mapped.

This operation is provided to enable device drivers to not only operate DMA transfers on memory which was not 
originally mapped to the kernel address space (but to user client's, instead) but also read or modify the data 
from kernel context, e.g. a DFC.

The act of mapping the memory means that it is guaranteed to be excluded from RAM defragmentation.  The new mapping 
will also be protected from the effects of demand paging.  Once mapped the memory is also guaranteed not to be reused for 
some other purpose - even if the process owning the memory decommits it or terminates.

Note that this operation may fail with KErrNoMemory if the kernel mapping object was created with aMaxReserveSize==0
or the memory to be mapped is demand paged.

@param aMapObject		A kernel map object previously created by calling Kern::CreateKernelMapObject().
@param aThread	   		The thread that owns the memory to map, or NULL to use the current thread.
@param aStart			Virtual address of memory to map.
@param aSize			The length (in bytes) of memory to map.
@param aMapAttributes	A bit mask of TKernelMapAttributes attributes for the mapping.
@param aKernelAddr		Set to the base linear address of the new kernel mapping.  This is the address to use when accessing
						the memory from kernel side and to pass to cache maintence operations before and after performing
						DMA operations.
@param aPages			Points to area of TPhysAddr which will on exit hold the addresses of the physical pages contained
						in the specified region. The array must be large enough to hold the whole list of pages in the region.
						Set aPages to NULL if the physical addresses of the pages being mapped are not required.

@return 				KErrNone, if successful,
						KErrInUse if aMapObject is already mapping some memory,
						KErrArgument if aSize is larger than the argument aMaxReserveSize of Kern::CreateKernelMapObject()
						when creating aMapObject or aMapAttributes has invalid bits set.
						KErrNotSupported on memory models other then flexible.
						Otherwise one of the other system-wide error codes.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::UnmapAndUnpinMemory()
@see Cache::SyncMemoryBeforeDmaWrite()
@see Cache::SyncMemoryBeforeDmaRead()
@see Cache::SyncMemoryAfterDmaRead()
@prototype
 */
EXPORT_C TInt Kern::MapAndPinMemory(TKernelMapObject* aMapObject, DThread* aThread, TLinAddr aStart, TUint aSize, 
									TUint aMapAttributes, TLinAddr& aKernelAddr, TPhysAddr* aPages)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::MapAndPinMemory");
	if (~EKernelMap_ValidMask & aMapAttributes)
		{// Invalid mapping attribute flags set.
		return KErrArgument;
		}
	if (aThread == NULL)
		aThread = TheCurrentThread;
	if (aSize == 0)
		return KErrNone;
	NKern::ThreadEnterCS();
	TInt r = M::MapAndPinMemory(aMapObject, aThread, aStart, aSize, aMapAttributes, aKernelAddr, aPages);
	NKern::ThreadLeaveCS();
	return r;
	}


/**
Unmaps and unpins an area of memory previously mapped by calling Kern::MapAndPinMemory().

@param aMapObject  The kernel mapping object used to map and pin the memory.

@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@see Kern::MapAndPinMemory()

@prototype
*/
EXPORT_C void Kern::UnmapAndUnpinMemory(TKernelMapObject* aMapObject)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::UnmapAndUnpinMemory");
	NKern::ThreadEnterCS();
	M::UnmapAndUnpinMemory(aMapObject);
	NKern::ThreadLeaveCS();
	}


/**
Dispose of a kernel mapping object which is no longer required.

Any memory mapped and pinned by the object is unmapped and unpinned first.

@param	aMapObject	A reference to a pointer to the mapping object to destroy.
					This pointer will be set to NULL on return.

@pre Calling thread must be in a critical section
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Suitable for use in a device driver.

@see Kern::CreateKernelMapObject()

@prototype
*/
EXPORT_C void Kern::DestroyKernelMapObject(TKernelMapObject*& aMapObject)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::DestroyKernelMapObject");
	M::DestroyKernelMapObject(aMapObject);
	}
