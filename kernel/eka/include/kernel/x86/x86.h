// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\x86\x86.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __X86_KERN_H__
#define __X86_KERN_H__

#ifdef __STANDALONE_NANOKERNEL__
#include <e32cmn.h>
#include <nk_priv.h>
#include <nk_plat.h>
#else
#include <e32const.h>
#include <plat_priv.h>
#include <assp.h>
#endif

#include <x86boot.h>

//
// information passed to the exception handler
//
class TX86ExcInfo
	{
public:
#ifdef __SMP__
	TX86Reg	iEsp;			// read only
	TX86Reg	iSs;			// read only
	TX86Reg iFaultAddress;
	TUint32	iEcx;
	TUint32	iEdx;
	TUint32	iEbx;
	TUint32	iEsi;
	TUint32	iEdi;
	TUint32	iEbp;
	TUint32	iEax;
	TUint32	iDs;
	TUint32	iEs;
	TUint32	iFs;
	TUint32	iGs;
	TInt	iExcId;			// vector number
	TInt	iExcErrorCode;	// error code from exception
	TX86Reg	iEip;
	TX86Reg	iCs;
	TX86Reg	iEflags;
	TX86Reg iEsp3;			// user ESP if exception in user mode
	TX86Reg iSs3;			// user SS if exception in user mode
#else
	TX86Reg	iEsp;			// read only
	TX86Reg	iSs;			// read only
	TX86Reg iFaultAddress;
	TX86Reg	iEax;
	TX86Reg	iEdx;
	TX86Reg	iEcx;
	TX86Reg	iEbx;
	TX86Reg	iEsi;
	TX86Reg	iEdi;
	TX86Reg	iEbp;
	TX86Reg	iGs;
	TX86Reg	iFs;
	TX86Reg	iEs;
	TX86Reg	iDs;
	TInt	iExcId;			// vector number
	TInt	iExcErrorCode;	// error code from exception
	TX86Reg	iEip;
	TX86Reg	iCs;
	TX86Reg	iEflags;
	TX86Reg iEsp3;			// user ESP if exception in user mode
	TX86Reg iSs3;			// user SS if exception in user mode
#endif
	};


#ifndef __SMP__
extern "C" {
GLREF_D SFullX86RegSet X86_Regs;
GLREF_D TX86Tss* X86_TSS_Ptr;
}
#endif

extern "C" {
extern TLinAddr SuperPageAddress;
}

GLREF_D TInt X86_NanoWaitCal;

class X86
	{
public:
	static void Init1Interrupts();
#ifdef __SMP__
	inline static TCpuPages& CpuPage()
		{return *(TCpuPages*)(SuperPageAddress+KPageSize);}
#else
	inline static TCpuPage& CpuPage()
		{return *(TCpuPage*)(SuperPageAddress+KPageSize);}
#endif
	static TUint32 GetCR0();
	static void SetCR0(TUint32);
	static TUint32 ModifyCR0(TUint32 aClearMask, TUint32 aSetMask);
	static TUint32 GetCpuID();
	static TBool IsPentium();
	static TBool IsP6();
public:
	IMPORT_C static void SetIrqHandler(TLinAddr aHandler);
	IMPORT_C static TLinAddr IrqStackTop(TInt aCpu);
	IMPORT_C static TLinAddr IrqReturnAddress();
	IMPORT_C static TUint64 Timestamp();
public:
#ifndef __STANDALONE_NANOKERNEL__
	static Asic* TheAsic;
#endif
	static TUint32 DefaultCR0;
#ifdef __SMP__
	static SCpuBootData CpuBootData[KMaxCpus];
#endif
	};

#ifdef __GCC32__
#define MOV_CR4_EDX asm("mov cr4, edx")
#define MOV_CR4_ECX asm("mov cr4, ecx")
#define MOV_ECX_CR4 asm("mov ecx, cr4")
#define MOV_EAX_CR4 asm("mov eax, cr4")
#else
#define SETCR(creg,reg)	_asm _emit 0fh _asm _emit 22h _asm _emit (0c0h+8*(creg)+(reg))
#define GETCR(reg,creg)	_asm _emit 0fh _asm _emit 20h _asm _emit (0c0h+8*(creg)+(reg))
#define MOV_CR4_EDX SETCR(4, REG_EDX)
#define MOV_CR4_ECX SETCR(4, REG_ECX)
#define MOV_ECX_CR4 GETCR(REG_ECX,4)
#define MOV_EAX_CR4 GETCR(REG_EAX,4)
#endif


// DEBUG-mode definitions
#ifdef _DEBUG
#ifdef __GCC32__
#define ASM_DEBUG4(fn,a0,a1,a2,a3)	asm("pushfd \n push eax \n push ecx \n push edx");		\
									asm("push "#a3 "\n push "#a2 "\n push "#a1 "\n push "#a0);	\
									asm("call %a0": : "i"(&__DebugMsg##fn)); \
									asm("add esp, 16");	\
									asm("pop edx \n pop ecx \n pop eax \n popfd");

#define ASM_DEBUG3(fn,a0,a1,a2)		asm("pushfd \n push eax \n push ecx \n push edx");		\
									asm("push "#a2 "\n push "#a1 "\n push "#a0);			\
									asm("call %a0": : "i"(&__DebugMsg##fn)); \
									asm("add esp, 12");	\
									asm("pop edx \n pop ecx \n pop eax \n popfd");

#define ASM_DEBUG2(fn,a0,a1)		asm("pushfd \n push eax \n push ecx \n push edx");		\
									asm("push " #a1 "\n push " #a0);						\
									asm("call %a0": : "i"(&__DebugMsg##fn)); \
									asm("add esp, 8");	\
									asm("pop edx \n pop ecx \n pop eax \n popfd");

#define ASM_DEBUG1(fn,a0)			asm("pushfd \n push eax \n push ecx \n push edx");		\
									asm("push "#a0); \
									asm("call %a0": : "i"(&__DebugMsg##fn)); \
									asm("add esp, 4"); \
									asm("pop edx \n pop ecx \n pop eax \n popfd");

#define ASM_DEBUG0(fn)				asm("pushfd \n push eax \n push ecx \n push edx");		\
									asm("call %a0": : "i"(&__DebugMsg##fn)); \
									asm("pop edx \n pop ecx \n pop eax \n popfd");

#define ASM_CDEBUG4(cc,fn,a0,a1,a2,a3)														\
									asm("j"#cc " LABEL");									\
									ASM_DEBUG4(fn,a0,a1,a2,a3)								\
									asm("LABEL:");

#define ASM_CDEBUG3(cc,fn,a0,a1,a2)	asm("j"#cc " LABEL");									\
									ASM_DEBUG3(fn,a0,a1,a2)									\
									asm("LABEL:");

#define ASM_CDEBUG2(cc,fn,a0,a1)	asm("j"#cc " LABEL");									\
									ASM_DEBUG2(fn,a0,a1)									\
									asm("LABEL:");

#define ASM_CDEBUG1(cc,fn,a0,a1)	asm("j"#cc " LABEL");									\
									ASM_DEBUG1(fn,a0)										\
									asm("LABEL:");

#define ASM_CDEBUG0(cc,fn,a0,a1)	asm("j"#cc " LABEL");									\
									ASM_DEBUG0(fn)											\
									asm("LABEL:");

#else
#define ASM_DEBUG4(fn,a0,a1,a2,a3)	_asm pushfd _asm push eax _asm push ecx _asm push edx	\
									_asm push a3 _asm push a2 _asm push a1 _asm push a0		\
									_asm call __DebugMsg##fn _asm add esp, 16				\
									_asm pop edx _asm pop ecx _asm pop eax _asm popfd

#define ASM_DEBUG3(fn,a0,a1,a2)		_asm pushfd _asm push eax _asm push ecx _asm push edx	\
									_asm push a2 _asm push a1 _asm push a0					\
									_asm call __DebugMsg##fn _asm add esp, 12				\
									_asm pop edx _asm pop ecx _asm pop eax _asm popfd

#define ASM_DEBUG2(fn,a0,a1)		_asm pushfd _asm push eax _asm push ecx _asm push edx	\
									_asm push a1 _asm push a0 _asm call __DebugMsg##fn _asm add esp, 8	\
									_asm pop edx _asm pop ecx _asm pop eax _asm popfd

#define ASM_DEBUG1(fn,a0)			_asm pushfd _asm push eax _asm push ecx _asm push edx	\
									_asm push a0 _asm call __DebugMsg##fn _asm add esp, 4	\
									_asm pop edx _asm pop ecx _asm pop eax _asm popfd

#define ASM_DEBUG0(fn)				_asm pushfd _asm push eax _asm push ecx _asm push edx	\
									_asm call __DebugMsg##fn								\
									_asm pop edx _asm pop ecx _asm pop eax _asm popfd

#define ASM_CDEBUG4(cc,fn,a0,a1,a2,a3)														\
									_asm j##cc LABEL										\
									_asm pushfd _asm push eax _asm push ecx _asm push edx	\
									_asm push a3 _asm push a2 _asm push a1 _asm push a0		\
									_asm call __DebugMsg##fn _asm add esp, 16				\
									_asm pop edx _asm pop ecx _asm pop eax _asm popfd		\
									_asm LABEL:

#define ASM_CDEBUG3(cc,fn,a0,a1,a2)	_asm j##cc LABEL										\
									_asm pushfd _asm push eax _asm push ecx _asm push edx	\
									_asm push a2 _asm push a1 _asm push a0					\
									_asm call __DebugMsg##fn _asm add esp, 12				\
									_asm pop edx _asm pop ecx _asm pop eax _asm popfd		\
									_asm LABEL:

#define ASM_CDEBUG2(cc,fn,a0,a1)	_asm j##cc LABEL										\
									_asm pushfd _asm push eax _asm push ecx _asm push edx	\
									_asm push a1 _asm push a0 _asm call __DebugMsg##fn _asm add esp, 8	\
									_asm pop edx _asm pop ecx _asm pop eax _asm popfd		\
									_asm LABEL:

#define ASM_CDEBUG1(cc,fn,a0)		_asm j##cc LABEL										\
									_asm pushfd _asm push eax _asm push ecx _asm push edx	\
									_asm push a0 _asm call __DebugMsg##fn _asm add esp, 4	\
									_asm pop edx _asm pop ecx _asm pop eax _asm popfd		\
									_asm LABEL:

#define ASM_CDEBUG0(cc,fn)			_asm j##cc LABEL										\
									_asm pushfd _asm push eax _asm push ecx _asm push edx	\
									_asm call __DebugMsg##fn								\
									_asm pop edx _asm pop ecx _asm pop eax _asm popfd		\
									_asm LABEL:

#endif
#define _CONCAT(a,b)	a##b
#define LABEL			_CONCAT(l,__LINE__)
#else
#define ASM_DEBUG4(fn,a0,a1,a2,a3)
#define ASM_DEBUG3(fn,a0,a1,a2)
#define ASM_DEBUG2(fn,a0,a1)
#define ASM_DEBUG1(fn,a0)
#define ASM_DEBUG0(fn)
#define ASM_CDEBUG4(cc,fn,a0,a1,a2,a3)
#define ASM_CDEBUG3(cc,fn,a0,a1,a2)
#define ASM_CDEBUG2(cc,fn,a0,a1)
#define ASM_CDEBUG1(cc,fn,a0)
#define ASM_CDEBUG0(cc,fn)
#endif

#endif
