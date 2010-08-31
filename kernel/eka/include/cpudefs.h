// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\cpudefs.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __CPUDEFS_H__
#define __CPUDEFS_H__

#ifdef __ARMCC__
#define	__ARM_ASSEMBLER_ISA__	4	// "Instruction not supported on targeted CPU :("
#else
#define	__ARM_ASSEMBLER_ISA__	4
#endif

// Should really have been __CPU_CORTEX_A8__ instead of __CPU_CORTEX_A8N__
#ifdef __CPU_CORTEX_A8N__
#undef __CPU_CORTEX_A8__
#define __CPU_CORTEX_A8__
#endif

//
// Supported CPUs
//

#ifdef __MARM__

#undef __CPU_SPECIFIED
#if defined(__CPU_ARM710T__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_ARM720T__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_SA1__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_ARM920T__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_ARM925T__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_XSCALE__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_ARM926J__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_ARM1136__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_ARM1176__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_ARM11MP__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_CORTEX_A8__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_CORTEX_A9__)
	#define __CPU_SPECIFIED
#elif defined(__CPU_GENERIC_ARM4__)
	#define __CPU_SPECIFIED
#endif

#if defined(__SMP__)
	#if defined(__CPU_SPECIFIED)
		#if !defined(__CPU_ARM11MP__) && !defined(__CPU_CORTEX_A9__)
			#error Specified CPU does not support SMP
		#endif
	#else
	// If no CPU specified, assume lowest common denominator SMP
	#define	__CPU_ARM11MP__
	#endif
#endif

#if defined(__CPU_ARM710T__)
	#define __CPU_ARMV4T

#elif defined(__CPU_ARM720T__)
	#define __CPU_ARMV4T

#elif defined(__CPU_SA1__)
	#define __CPU_ARMV4

#elif defined(__CPU_ARM920T__)
	#define __CPU_ARMV4T

#elif defined(__CPU_ARM925T__)
	#define __CPU_ARMV4T

#elif defined(__CPU_XSCALE__)
	#define __CPU_ARMV5T
	#define __ENHANCED_DSP_INSTRUCTIONS

#elif defined(__CPU_ARM926J__)
	#define __CPU_ARMV5T
	#define __ENHANCED_DSP_INSTRUCTIONS
	#define __CPU_HAS_JAZELLE

#elif defined(__CPU_ARM1136__)
	#define __CPU_ARMV6

#elif defined(__CPU_ARM1176__)
	#define __CPU_ARMV6

#elif defined(__CPU_ARM11MP__)
	#define __CPU_ARMV6
	#define	__CPU_ARM_HAS_WFI
	#define	__CPU_ARM_HAS_WFE_SEV

#elif defined(__CPU_CORTEX_A8__)
	#define __CPU_ARMV7

#elif defined(__CPU_CORTEX_A9__)
	#define __CPU_ARMV7

#elif defined(__CPU_GENERIC_ARM4__)
	#define __CPU_ARMV4

#else
	// #error Unsupported CPU
	#define __CPU_UNKNOWN
#endif

#endif  // __MARM__



// Macros for emitting single bytes of machine code
#ifdef __CW32__
# define BYTE(x)	_asm byte x
#elif __GCC32__
# define BYTE(x)	asm(".byte "#x);
#else
# define BYTE(x)	_asm _emit x
#endif


// thiscall is different on GCC
#ifdef __GCC32__
#define THISCALL_PROLOG0() asm("mov ecx,[esp+4]");
#define THISCALL_PROLOG1() asm("mov ecx,[esp+4] \n mov eax,[esp+8] \n mov [esp+4],eax");
#define THISCALL_PROLOG2() asm("mov ecx,[esp+4] \n mov eax,[esp+8] \n mov [esp+4],eax \n mov eax,[esp+12] \n mov [esp+8],eax");
#define THISCALL_PROLOG3() asm("mov ecx,[esp+4] \n mov eax,[esp+8] \n mov [esp+4],eax \n mov eax,[esp+12] \n mov [esp+8],eax \n mov eax,[esp+16] \n mov [esp+12],eax");
#define THISCALL_PROLOG0_BIGRETVAL() asm("mov ecx,[esp+8]");
#define THISCALL_PROLOG1_BIGRETVAL() asm("mov ecx,[esp+8] \n mov eax,[esp+12] \n mov [esp+8],eax");
#define THISCALL_EPILOG0() asm("ret");
#define THISCALL_EPILOG1() asm("ret");
#define THISCALL_EPILOG2() asm("ret");
#define THISCALL_EPILOG3() asm("ret");
#define THISCALL_EPILOG0_BIGRETVAL() asm("ret 4");
#define THISCALL_EPILOG1_BIGRETVAL() asm("ret 4");
#else
#define THISCALL_PROLOG0()
#define THISCALL_PROLOG1()
#define THISCALL_PROLOG2()
#define THISCALL_PROLOG3()
#define THISCALL_PROLOG0_BIGRETVAL() 
#define THISCALL_PROLOG1_BIGRETVAL() 
#define THISCALL_EPILOG0() __asm ret
#define THISCALL_EPILOG1() __asm ret 4
#define THISCALL_EPILOG2() __asm ret 8
#define THISCALL_EPILOG3() __asm ret 12
#define THISCALL_EPILOG0_BIGRETVAL() __asm ret 4
#define THISCALL_EPILOG1_BIGRETVAL() __asm ret 8
#endif


// Workaround for MSVC++ 5.0 bug; MSVC incorrectly fixes up conditional jumps
// when the destination is a C++ function.
#if defined(__VC32__) && (_MSC_VER==1100)	// untested on MSVC++ > 5.0
# define _ASM_j(cond,dest) _asm jn##cond short $+11 _asm jmp dest
# define _ASM_jn(cond,dest) _asm j##cond short $+11 _asm jmp dest
#else
# if defined __GCC32__
#  define _ASM_j(cond,dest) asm("j"#cond " %a0": : "i"(dest));
#  define _ASM_jn(cond,dest) asm("jn"#cond " %a0": :"i"(dest));
# else
#  define _ASM_j(cond,dest) _asm j##cond dest
#  define _ASM_jn(cond,dest) _asm jn##cond dest
# endif
#endif



//#define __MINIMUM_MACHINE_CODE__

#if defined(__WINS__)
#define __NAKED__ __declspec( naked )
#if !defined(__MINIMUM_MACHINE_CODE__) && defined(__KERNEL_MODE__)
// Assembly language memmove() and memcpy() are used for WINS but only in the kernel, not euser
#define __MEMMOVE_MACHINE_CODED__
#endif
#define __CPU_X86
#endif

#if defined(__X86__)
# ifdef __GCC32__
#  define __NAKED__	// GCC does not support naked functions on X86
# else
#  define __NAKED__ __declspec( naked )
# endif
# ifndef __MINIMUM_MACHINE_CODE__
#  define __MEM_MACHINE_CODED__
# endif
# define __CPU_X86
#endif


#if defined(__MARM__)
#ifndef __NAKED__ // should be defined in prefix file
	#ifndef __GCCXML__
        #define __NAKED__ __declspec( naked )
    #else
        #define __NAKED__
    #endif
#endif
#ifndef __CIA__
#undef __NAKED__
#define __NAKED__ ____ONLY_USE_NAKED_IN_CIA____
#endif
	#define __CPU_ARM

#if defined(__MARM_ARMV5__) && !defined(__CPU_ARMV5T)
#define __CPU_ARMV5T
#endif

#ifndef __MINIMUM_MACHINE_CODE__
#if !defined(__BIG_ENDIAN__)
	#define __MEM_MACHINE_CODED__
	#define __DES_MACHINE_CODED__
	#define __REGIONS_MACHINE_CODED__
	#define __DES8_MACHINE_CODED__
	#define __DES16_MACHINE_CODED__
	#define __HEAP_MACHINE_CODED__
	#define __REALS_MACHINE_CODED__
	#define __COBJECT_MACHINE_CODED__
	#define __CACTIVESCHEDULER_MACHINE_CODED__
	#define __CSERVER_MACHINE_CODED__
	#define __ARRAY_MACHINE_CODED__
	#define __HUFFMAN_MACHINE_CODED__
#if defined(__MARM_ARM4__) || defined(__MARM_ARMI__) || defined(__MARM_THUMB__) || defined(__MARM_ARMV4__) || defined(__MARM_ARMV5__)
	#define __DES16_MACHINE_CODED_HWORD__
#endif
#endif
#endif
#endif

#ifdef __CPU_ARMV4
	#define __CPU_64BIT_MULTIPLY
#endif
#ifdef __CPU_ARMV4T
	#define __CPU_THUMB
	#define __CPU_ARM_SUPPORTS_BX
	#define __CPU_64BIT_MULTIPLY
#endif
#ifdef __CPU_ARMV5T
	#define __CPU_THUMB
	#define __CPU_ARM_SUPPORTS_BX
	#define __CPU_ARM_SUPPORTS_BLX
	#define __CPU_64BIT_MULTIPLY
	#define __CPU_ARM_LDR_PC_SETS_TBIT
	#define __CPU_ARM_HAS_CLZ
	#define __CPU_ARM_HAS_PLD
#endif
#ifdef __ENHANCED_DSP_INSTRUCTIONS
	#define __CPU_ARM_HAS_MCRR
	#define __CPU_ARM_HAS_LDRD_STRD
#endif
#if defined(__CPU_ARMV6) || defined(__CPU_ARMV7)
	#define __CPU_THUMB
	#define __CPU_ARM_SUPPORTS_BX
	#define __CPU_ARM_SUPPORTS_BLX
	#define __CPU_64BIT_MULTIPLY
	#define __CPU_ARM_LDR_PC_SETS_TBIT
	#define __CPU_ARM_HAS_CLZ
	#define __CPU_ARM_HAS_MCRR
	#define __CPU_ARM_HAS_LDREX_STREX
	#define __CPU_ARM_HAS_LDRD_STRD
	#define __CPU_ARM_HAS_PLD
	#define __CPU_ARM_HAS_CPS
	#define __CPU_ARM_HAS_SPLIT_FSR
#if !defined(__CPU_ARM1136__) && !defined(__CPU_ARM11MP__)
	#define __CPU_ARM_HAS_CP15_IFAR
#endif
	#define	__CPU_ARM_SUPPORTS_USER_MODE_BARRIERS
#endif
#if defined(__CPU_ARMV7) || (defined(__CPU_ARM1136__) && defined(__CPU_ARM1136_IS_R1__)) || defined(__CPU_ARM1176__) || defined(__CPU_ARM11MP__)
	#define __CPU_ARM_HAS_LDREX_STREX_V6K
	#define __CPU_HAS_CP15_THREAD_ID_REG
#endif
#if defined(__MARM_ARM4T__) || defined(__MARM_INTERWORK__)
	#define __SUPPORT_THUMB_INTERWORKING
#endif
#if defined(__CPU_ARMV7)
#define	__CPU_ARM_HAS_WFI
#define	__CPU_ARM_HAS_WFE_SEV
#define __CPU_THUMB2
#define __CPU_SUPPORT_THUMB2EE
#endif


// ARM CPU macros to allow Thumb/Non-thumb builds
#ifdef __CPU_ARM

#define	EXC_TRAP_CTX_SZ		10		// Nonvolatile registers + sp + pc

#ifdef __SUPPORT_THUMB_INTERWORKING
#define __JUMP(cc,r) asm("bx"#cc " "#r )
#ifdef __CPU_ARM_LDR_PC_SETS_TBIT
#define __POPRET(rlist) asm("ldmfd sp!, {"rlist"pc} ")
#define __CPOPRET(cc,rlist) asm("ldm"#cc "fd sp!, {"rlist"pc} ")
#else
#define __POPRET(rlist) asm("ldmfd sp!, {"rlist"lr} ");\
						asm("bx lr ")
#define __CPOPRET(cc,rlist)	asm("ldm"#cc "fd sp!, {"rlist"lr} ");\
							asm("bx"#cc " lr ")
#endif
#else
#define __JUMP(cc,r) asm("mov"#cc " pc, "#r )
#define __POPRET(rlist) asm("ldmfd sp!, {"rlist"pc} ")
#define __CPOPRET(cc,rlist) asm("ldm"#cc "fd sp!, {"rlist"pc} ")
#endif

#ifdef __CPU_ARM_SUPPORTS_BLX
#if __ARM_ASSEMBLER_ISA__ >= 5
#define BLX(Rm)							asm("blx r" #Rm)
#else
#define BLX(Rm)							asm(".word %a0" : : "i" ((TInt)( 0xe12fff30 | (Rm) )))
#endif
#define __JUMPL(Rm) BLX(Rm)
#else
#ifdef __SUPPORT_THUMB_INTERWORKING
#define __JUMPL(Rm) asm("mov lr, pc "); \
                    asm("bx r"#Rm )
#else
#define __JUMPL(Rm) asm("mov lr, pc "); \
                    asm("mov pc, r"#Rm )
#endif
#endif

#ifdef __MARM_THUMB__
#ifndef __ARMCC__
#define __SWITCH_TO_ARM		asm("push {r0} ");\
							asm("add r0, pc, #4 ");\
							asm("bx r0 ");\
							asm("nop ");\
							asm(".align 2 ");\
							asm(".code 32 ");\
							asm("ldr r0, [sp], #4 ")
#define __END_ARM			asm(".code 16 ")
#else
#define __SWITCH_TO_ARM        asm(".code 32 ");
#define __END_ARM
#endif
#else
#define __SWITCH_TO_ARM
#define __END_ARM
#endif

#define CC_EQ	0U
#define	CC_NE	1U
#define CC_CS	2U
#define CC_CC	3U
#define CC_MI	4U
#define CC_PL	5U
#define CC_VS	6U
#define CC_VC	7U
#define CC_HI	8U
#define CC_LS	9U
#define CC_GE	10U
#define CC_LT	11U
#define CC_GT	12U
#define CC_LE	13U
#define	CC_AL	14U

#ifdef __CPU_ARM_HAS_CLZ
#if __ARM_ASSEMBLER_ISA__ >= 5
#define CLZ(Rd,Rm)		asm("clz r" #Rd ", r" #Rm)
#else
#define CLZ(Rd,Rm)		asm(".word %a0" : : "i" ((TInt)0xe16f0f10|((Rd)<<12)|(Rm)));
#endif
#define CLZcc(cc,Rd,Rm)	asm(".word %a0" : : "i" ((TInt)0x016f0f10|((cc)<<28)|((Rd)<<12)|(Rm)));
#endif
#ifdef __CPU_ARM_HAS_MCRR
#define MCRR(cop,opc,Rd,Rn,CRm)			asm(".word %a0" : : "i" ((TInt)0xec400000|((Rn)<<16)|((Rd)<<12)|((cop)<<8)|((opc)<<4)|(CRm)));
#define MCRRcc(cc,cop,opc,Rd,Rn,CRm)	asm(".word %a0" : : "i" ((TInt)0x0c400000|((cc)<<28)|((Rn)<<16)|((Rd)<<12)|((cop)<<8)|((opc)<<4)|(CRm)));
#define MRRC(cop,opc,Rd,Rn,CRm)			asm(".word %a0" : : "i" ((TInt)0xec500000|((Rn)<<16)|((Rd)<<12)|((cop)<<8)|((opc)<<4)|(CRm)));
#define MRRCcc(cc,cop,opc,Rd,Rn,CRm)	asm(".word %a0" : : "i" ((TInt)0x0c500000|((cc)<<28)|((Rn)<<16)|((Rd)<<12)|((cop)<<8)|((opc)<<4)|(CRm)));
#endif
#ifdef __CPU_ARM_HAS_LDREX_STREX
// LDREX Rd, [Rn] 		- load from [Rn] into Rd exclusive
// STREX Rd, Rm, [Rn] 	- store Rm into [Rn] with exclusive access; success/fail indicator into Rd
#define LDREXcc(cc,Rd,Rn)				asm(".word %a0" : : "i" ((TInt)(0x01900f9f|((cc)<<28)|((Rd)<<12)|((Rn)<<16))));
#define STREXcc(cc,Rd,Rm,Rn)			asm(".word %a0" : : "i" ((TInt)(0x01800f90|((cc)<<28)|((Rd)<<12)|(Rm)|((Rn)<<16))));
#if __ARM_ASSEMBLER_ISA__ >= 6
#define LDREX(Rd,Rn)					asm("ldrex r" #Rd ", [r" #Rn "] ")
#define STREX(Rd,Rm,Rn)					asm("strex r" #Rd ", r" #Rm ", [r" #Rn "] ")
#else
#define LDREX(Rd,Rn)					asm(".word %a0" : : "i" ((TInt)(0x01900f9f|((CC_AL)<<28)|((Rd)<<12)|((Rn)<<16))));
#define STREX(Rd,Rm,Rn)					asm(".word %a0" : : "i" ((TInt)(0x01800f90|((CC_AL)<<28)|((Rd)<<12)|(Rm)|((Rn)<<16))));
#endif
#endif
#ifdef __CPU_ARM_HAS_LDREX_STREX_V6K
// Byte, halfword, doubleword STREX/LDREX & unconditional CLREX
#if __ARM_ASSEMBLER_ISA__ >= 6
#define LDREXB(Rd,Rn)					asm("ldrexb r" #Rd ", [r" #Rn "] ")
#define STREXB(Rd,Rm,Rn)				asm("strexb r" #Rd ", r" #Rm ", [r" #Rn "] ")
#define LDREXH(Rd,Rn)					asm("ldrexh r" #Rd ", [r" #Rn "] ")
#define STREXH(Rd,Rm,Rn)				asm("strexh r" #Rd ", r" #Rm ", [r" #Rn "] ")
#define LDREXD(Rd,Rn)					asm("ldrexd r" #Rd ", [r" #Rn "] ")
#define STREXD(Rd,Rm,Rn)				asm("strexd r" #Rd ", r" #Rm ", [r" #Rn "] ")
#else
#define LDREXB(Rd,Rn)					asm(".word %a0" : : "i" ((TInt)(0x01D00f9f|((CC_AL)<<28)|((Rd)<<12)|((Rn)<<16))));
#define STREXB(Rd,Rm,Rn)				asm(".word %a0" : : "i" ((TInt)(0x01C00f90|((CC_AL)<<28)|((Rd)<<12)|(Rm)|((Rn)<<16))));
#define LDREXH(Rd,Rn)					asm(".word %a0" : : "i" ((TInt)(0x01f00f9f|((CC_AL)<<28)|((Rd)<<12)|((Rn)<<16))));
#define STREXH(Rd,Rm,Rn)				asm(".word %a0" : : "i" ((TInt)(0x01e00f90|((CC_AL)<<28)|((Rd)<<12)|(Rm)|((Rn)<<16))));
#define LDREXD(Rd,Rn)					asm(".word %a0" : : "i" ((TInt)(0x01b00f9f|((CC_AL)<<28)|((Rd)<<12)|((Rn)<<16))));
#define STREXD(Rd,Rm,Rn)				asm(".word %a0" : : "i" ((TInt)(0x01a00f90|((CC_AL)<<28)|((Rd)<<12)|(Rm)|((Rn)<<16))));
#endif
#if !defined(__CPU_ARM1136__) || defined(__CPU_ARM1136_ERRATUM_406973_FIXED)
#define __CPU_ARM_HAS_WORKING_CLREX
#if __ARM_ASSEMBLER_ISA__ >= 6
#define CLREX							asm("clrex ")
#else
#define CLREX							asm(".word %a0" : : "i" ((TInt)(0xf57ff01f)));
#endif
#endif
#endif 
#ifdef __CPU_ARM_HAS_LDRD_STRD
#if __ARM_ASSEMBLER_ISA__ >= 5
#define LDRD(Rd,Rn)						asm("ldrd r" #Rd ", [r" #Rn "] ")
#define STRD(Rd,Rn)						asm("strd r" #Rd ", [r" #Rn "] ")
#else
#define LDRD(Rd,Rn)						asm(".word %a0" : : "i" ((TInt)( 0xe1c000d0 | ((Rn)<<16) | ((Rd)<<12) )))
#define STRD(Rd,Rn)						asm(".word %a0" : : "i" ((TInt)( 0xe1c000f0 | ((Rn)<<16) | ((Rd)<<12) )))
#endif
#define LDRD_ioff(Rd,Rn,off)			asm(".word %a0" : : "i" ((TInt)( 0xe1c000d0 | ((Rn)<<16) | ((Rd)<<12) | (((off)&0xf0)<<4) | ((off)&0x0f) )))
#define STRD_ioff(Rd,Rn,off)			asm(".word %a0" : : "i" ((TInt)( 0xe1c000f0 | ((Rn)<<16) | ((Rd)<<12) | (((off)&0xf0)<<4) | ((off)&0x0f) )))
#endif
#if defined(__CPU_ARM_HAS_PLD) && !defined(__CPU_ARM926J__) && !defined(__CPU_UNKNOWN)		// PLD is a no-op on ARM926
#if __ARM_ASSEMBLER_ISA__ >= 5
#define PLD(Rn)							asm("pld [r" #Rn "] ")
#else
#define PLD(Rn)							asm(".word %a0" : : "i" ((TInt)( 0xf5d0f000 | ((Rn)<<16) )))
#endif
#define PLD_ioff(Rn, off)				asm(".word %a0" : : "i" ((TInt)( 0xf5d0f000 | ((Rn)<<16) | (off) )))	// preload with immediate offset
#define PLD_noff(Rn, off)				asm(".word %a0" : : "i" ((TInt)( 0xf550f000 | ((Rn)<<16) | (off) )))	// preload with negative offset
#else
#define PLD(Rn)
#define PLD_ioff(Rn, off)
#define PLD_noff(Rn, off)
#endif
#ifdef __CPU_HAS_CP15_THREAD_ID_REG
#define GET_RWRW_TID(cc,r)				asm("mrc"#cc" p15, 0, "#r", c13, c0, 2 ");
#define GET_RWRO_TID(cc,r)				asm("mrc"#cc" p15, 0, "#r", c13, c0, 3 ");
#define GET_RWNO_TID(cc,r)				asm("mrc"#cc" p15, 0, "#r", c13, c0, 4 ");
#define SET_RWRW_TID(cc,r)				asm("mcr"#cc" p15, 0, "#r", c13, c0, 2 ");
#define SET_RWRO_TID(cc,r)				asm("mcr"#cc" p15, 0, "#r", c13, c0, 3 ");
#define SET_RWNO_TID(cc,r)				asm("mcr"#cc" p15, 0, "#r", c13, c0, 4 ");
#endif

#ifdef __CPU_SUPPORT_THUMB2EE
#define GET_THUMB2EE_HNDLR_BASE(cc,r)	asm("mrc"#cc" p14, 6, "#r", c1, c0, 0 ")
#define SET_THUMB2EE_HNDLR_BASE(cc,r)	asm("mcr"#cc" p14, 6, "#r", c1, c0, 0 ")
#endif

#if defined(__CPU_ARMV7)
#define	ARM_DMB_gen(opt)				asm(".word %a0" : : "i" ((TInt)(0xf57ff050 | (opt) )) )
#define	ARM_DSB_gen(opt)				asm(".word %a0" : : "i" ((TInt)(0xf57ff040 | (opt) )) )
#define	ARM_ISB_gen(opt)				asm(".word %a0" : : "i" ((TInt)(0xf57ff060 | (opt) )) )

#define	ARM_DMBSY	ARM_DMB_gen(0xf)	// full system DMB
#define	ARM_DSBSY	ARM_DSB_gen(0xf)	// full system DSB
#define	ARM_DMBST	ARM_DMB_gen(0xe)	// full system DMB, orders writes only
#define	ARM_DSBST	ARM_DSB_gen(0xe)	// full system DSB, orders writes only
#define	ARM_DMBSH	ARM_DMB_gen(0xb)	// DMB encompassing inner-shareable domain
#define	ARM_DSBSH	ARM_DSB_gen(0xb)	// DMB encompassing inner-shareable domain
#define	ARM_DMBSHST	ARM_DMB_gen(0xa)	// DMB encompassing inner-shareable domain, orders writes only
#define	ARM_DSBSHST	ARM_DSB_gen(0xa)	// DMB encompassing inner-shareable domain, orders writes only

#define	ARM_ISBSY	ARM_ISB_gen(0xf)	// full system ISB

#define	ARM_NOP							asm(".word 0xe320f000 ")
#define	ARM_YIELD						asm(".word 0xe320f001 ")

#define	__DATA_MEMORY_BARRIER__(reg)	ARM_DMBSH
#define	__DATA_MEMORY_BARRIER_Z__(reg)	asm("mov "#reg", #0"); ARM_DMBSH
#define	__DATA_SYNC_BARRIER__(reg)		ARM_DSBSH
#define	__DATA_SYNC_BARRIER_Z__(reg)	asm("mov "#reg", #0"); ARM_DSBSH
#define	__INST_SYNC_BARRIER__(reg)		ARM_ISBSY
#define	__INST_SYNC_BARRIER_Z__(reg)	asm("mov "#reg", #0"); ARM_ISBSY

#elif defined(__CPU_ARM11MP__)

#define	ARM_DMB(reg)					asm("mcr p15, 0, "#reg", c7, c10, 5 ")
#define	ARM_DSB(reg)					asm("mcr p15, 0, "#reg", c7, c10, 4 ")
#define	ARM_ISB(reg)					asm("mcr p15, 0, "#reg", c7, c5, 4 ")

#define	ARM_NOP							asm(".word 0xe320f000 ")
#define	ARM_YIELD						asm(".word 0xe320f001 ")

#define	__DATA_MEMORY_BARRIER__(reg)	ARM_DMB(reg)
#define	__DATA_MEMORY_BARRIER_Z__(reg)	asm("mov "#reg", #0"); ARM_DMB(reg)
#define	__DATA_SYNC_BARRIER__(reg)		ARM_DSB(reg)
#define	__DATA_SYNC_BARRIER_Z__(reg)	asm("mov "#reg", #0"); ARM_DSB(reg)
#define	__INST_SYNC_BARRIER__(reg)		ARM_ISB(reg)
#define	__INST_SYNC_BARRIER_Z__(reg)	asm("mov "#reg", #0"); ARM_ISB(reg)

#elif defined(__CPU_ARMV6__)

#define	ARM_DMB(reg)					asm("mcr p15, 0, "#reg", c7, c10, 5 ")
#define	ARM_DSB(reg)					asm("mcr p15, 0, "#reg", c7, c10, 4 ")
#define	ARM_ISB(reg)					asm("mcr p15, 0, "#reg", c7, c5, 4 ")

#define	__DATA_MEMORY_BARRIER__(reg)	ARM_DMB(reg)
#define	__DATA_MEMORY_BARRIER_Z__(reg)	asm("mov "#reg", #0"); ARM_DMB(reg)
#define	__DATA_SYNC_BARRIER__(reg)		ARM_DSB(reg)
#define	__DATA_SYNC_BARRIER_Z__(reg)	asm("mov "#reg", #0"); ARM_DSB(reg)
#define	__INST_SYNC_BARRIER__(reg)		ARM_ISB(reg)
#define	__INST_SYNC_BARRIER_Z__(reg)	asm("mov "#reg", #0"); ARM_ISB(reg)

#else

#define	__DATA_MEMORY_BARRIER__(reg)
#define	__DATA_MEMORY_BARRIER_Z__(reg)	asm("mov "#reg", #0")
#define	__DATA_SYNC_BARRIER__(reg)		asm("mcr p15, 0, "#reg", c7, c10, 4 ")
#define	__DATA_SYNC_BARRIER_Z__(reg)	asm("mov "#reg", #0"); asm("mcr p15, 0, "#reg", c7, c10, 4 ")
#define	__INST_SYNC_BARRIER__(reg)
#define	__INST_SYNC_BARRIER_Z__(reg)	asm("mov "#reg", #0")

#endif

#ifdef __SMP__
#define	__SMP_DATA_MEMORY_BARRIER__(reg)	__DATA_MEMORY_BARRIER__(reg)
#define	__SMP_DATA_MEMORY_BARRIER_Z__(reg)	__DATA_MEMORY_BARRIER_Z__(reg)
#define	__SMP_DATA_SYNC_BARRIER__(reg)		__DATA_SYNC_BARRIER__(reg)
#define	__SMP_DATA_SYNC_BARRIER_Z__(reg)	__DATA_SYNC_BARRIER_Z__(reg)
#define	__SMP_INST_SYNC_BARRIER__(reg)		__INST_SYNC_BARRIER__(reg)
#define	__SMP_INST_SYNC_BARRIER_Z__(reg)	__INST_SYNC_BARRIER_Z__(reg)
#else
#define	__SMP_DATA_MEMORY_BARRIER__(reg)
#define	__SMP_DATA_MEMORY_BARRIER_Z__(reg)	asm("mov "#reg", #0")
#define	__SMP_DATA_SYNC_BARRIER__(reg)
#define	__SMP_DATA_SYNC_BARRIER_Z__(reg)	asm("mov "#reg", #0")
#define	__SMP_INST_SYNC_BARRIER__(reg)
#define	__SMP_INST_SYNC_BARRIER_Z__(reg)	asm("mov "#reg", #0")
#endif

#ifdef	__CPU_ARM_HAS_WFI
#define	ARM_WFIcc(cc)					__DATA_SYNC_BARRIER__(r0); \
										asm(".word %a0" : : "i" ((TInt)(0x0320f003 | ((cc)<<28) )) )				    
#define	ARM_WFI		ARM_WFIcc(CC_AL)
#endif

#ifdef	__CPU_ARM_HAS_WFE_SEV
#define	ARM_WFEcc(cc)					__DATA_SYNC_BARRIER__(r0); \
										asm(".word %a0" : : "i" ((TInt)(0x0320f002 | ((cc)<<28) )) )
#if __ARM_ASSEMBLER_ISA__ >= 6
#define	ARM_WFE		__DATA_SYNC_BARRIER__(r0); \
					asm("wfe ")
#else
#define	ARM_WFE		ARM_WFEcc(CC_AL)
#endif
#define	ARM_SEVcc(cc)					asm(".word %a0" : : "i" ((TInt)(0x0320f004 | ((cc)<<28) )) )
#if __ARM_ASSEMBLER_ISA__ >= 6
#define	ARM_SEV		asm("sev ")
#else
#define	ARM_SEV		ARM_SEVcc(CC_AL)
#endif
#endif

#ifndef	ARM_NOP
#define	ARM_NOP							asm("nop ")
#define	ARM_YIELD						asm("nop ")
#endif

// Support for throwing exceptions through ARM embedded assembler
// Should only be needed user side
#ifndef __EH_FRAME_ADDRESS
#define	__EH_FRAME_ADDRESS(reg,offset)
#define __EH_FRAME_PUSH2(reg1,reg2) 
#define __EH_FRAME_SAVE1(reg,offset)
#endif

// StrongARM msr bug workaround: 
// (conditional msr might cause,that the next instruction is executed twice by these processors)  
#ifdef __CPU_SA1__
#define __MSR_CPSR_C(cc,r)   \
				asm("msr"#cc" cpsr_c," #r);  \
				ARM_NOP; 		
#else // !__CPU_SA1__
#define __MSR_CPSR_C(cc,r) asm("msr"#cc" cpsr_c,"#r);
#endif

// Causes undefined instruction exception on both ARM and THUMB
#define __ASM_CRASH()					asm(".word 0xe7ffdeff ")
#if defined(__GNUC__)  
#define	__crash()						asm(".word 0xe7ffdeff " : : : "memory")
#elif defined(__GCCXML__)
#define __crash()						(*((TInt *) 0x0) = 0xd1e)
#elif defined(__ARMCC__)
// RVCT doesn't let us inline an undefined instruction
// use a CDP to CP15 instead - doesn't work on THUMB but never mind
#if __ARMCC_VERSION < 310000
#define	__crash()						asm("cdp p15, 0, c0, c0, c0, 0 ")
#else
// Inline assembler is deprecated in RVCT 3.1 so we use an intrinsic.
#define __crash()						__cdp(15, 0x00, 0x000)
#endif
#endif

// Macro used to get the caller of the function containing a CHECK_PRECONDITIONS()
#if defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 200000
#define PRECOND_FUNCTION_CALLER		__return_address()
#endif

#if !defined(__CPU_ARM_HAS_LDREX_STREX_V6K)
#if defined(__CPU_ARM_HAS_LDREX_STREX)
#define	__ATOMIC64_USE_SLOW_EXEC__
#else
#define	__ATOMIC64_USE_FAST_EXEC__
#define	__ATOMIC_USE_FAST_EXEC__
#endif
#endif

#endif	// __CPU_ARM

#ifdef	__CPU_X86
#define	EXC_TRAP_CTX_SZ		10		// ebx, esp, ebp, esi, edi, ds, es, fs, gs, eip

// Causes exception
#if defined(__VC32__)
#define	__crash()						do { _asm int 255 } while(0)
#elif defined(__CW32__)
#define	__crash()						do { *(volatile TInt*)0 = 0; } while(0)
#else
#define	__crash()						asm("int 0xff " : : : "memory")
#endif

#ifdef __VC32__
// Not available in the version of MSVC normally used
// #define PRECOND_FUNCTION_CALLER		((TLinAddr)_ReturnAddress())
#endif

#endif	// __CPU_X86

#ifdef __GCC32__
#define PRECOND_FUNCTION_CALLER		((TLinAddr)__builtin_return_address(0))
#endif

#endif
