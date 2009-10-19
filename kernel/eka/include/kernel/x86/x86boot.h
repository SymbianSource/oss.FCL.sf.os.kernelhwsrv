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
// e32\include\kernel\x86\x86boot.h
// Definitions shared with bootstrap 
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __X86_BOOT_H__
#define __X86_BOOT_H__

#ifdef __STANDALONE_NANOKERNEL__
#include <e32cmn.h>
#include <nk_priv.h>
#include <nk_plat.h>
#else
#include <e32const.h>
#include <plat_priv.h>
#include <assp.h>
#endif

#ifdef __SMP__
const TInt K_Max_Cpus = 8;

__ASSERT_COMPILE(KMaxCpus == K_Max_Cpus);
#endif

typedef TUint32 TX86Reg;

#ifndef __KPAGESIZE_DEFINED__
const TInt KPageShift=12;
const TInt KPageSize=1<<KPageShift;
const TInt KPageMask=KPageSize-1;
#define __KPAGESIZE_DEFINED__
#endif

enum TX86Vectors
	{
	EX86VectorDivBy0=0,
	EX86VectorDebug=1,
	EX86VectorNMI=2,
	EX86VectorBreakpoint=3,
	EX86VectorINTO=4,
	EX86VectorBOUND=5,
	EX86VectorInvalidOpcode=6,
	EX86VectorDeviceNotAvailable=7,
	EX86VectorDoubleFault=8,
	EX86VectorReserved09=9,
	EX86VectorInvalidTSS=10,
	EX86VectorSegmentNotPresent=11,
	EX86VectorStackFault=12,
	EX86VectorGPF=13,
	EX86VectorPageFault=14,
	EX86VectorReserved0F=15,
	EX86VectorFPError=16,
	EX86VectorAlignment=17,
	EX86VectorReserved12=18,
	EX86VectorReserved13=19,
	EX86VectorReserved14=20,
	EX86VectorReserved15=21,
	EX86VectorReserved16=22,
	EX86VectorReserved17=23,
	EX86VectorReserved18=24,
	EX86VectorReserved19=25,
	EX86VectorReserved1A=26,
	EX86VectorReserved1B=27,
	EX86VectorReserved1C=28,
	EX86VectorReserved1D=29,
	EX86VectorReserved1E=30,
	EX86VectorReserved1F=31,
	// 32-255 are vectored hardware interrupts
	};

enum TX86Flags
	{
	EX86FlagCF=0x00000001,		// carry
	//00000002 read 1
	EX86FlagPF=0x00000004,		// parity even
	//00000008 read 0
	EX86FlagAF=0x00000010,		// auxiliary carry
	//00000020 read 0
	EX86FlagZF=0x00000040,		// zero
	EX86FlagSF=0x00000080,		// sign
	EX86FlagTF=0x00000100,		// trace
	EX86FlagIF=0x00000200,		// interrupt enable
	EX86FlagDF=0x00000400,		// direction (0=up, 1=down)
	EX86FlagOF=0x00000800,		// overflow
	EX86FlagIOPL=0x00003000,	// IO privilege level
	EX86FlagNF=0x00004000,		// nested
	//00008000 read 0
	EX86FlagRF=0x00010000,		// resume
	EX86FlagVM=0x00020000,		// virtual 8086 mode
	EX86FlagAC=0x00040000,		// alignment check
	EX86FlagVI=0x00080000,		// virtual interrupt (Pentium and above)
	EX86FlagVIP=0x00100000,		// virtual interrupt pending (Pentium and above)
	EX86FlagID=0x00200000,		// identification (Pentium and above)
	};

//
// MMU Registers
//
// CR0
const TUint32 KX86CR0_PE=0x00000001;	// protection enable
const TUint32 KX86CR0_MP=0x00000002;	// math present (should always be set to 1)
const TUint32 KX86CR0_EM=0x00000004;	// emulation (FPU)
const TUint32 KX86CR0_TS=0x00000008;	// task switched
const TUint32 KX86CR0_ET=0x00000010;	// extension type (always 1)
const TUint32 KX86CR0_NE=0x00000020;	// numeric error action (set to 1)
const TUint32 KX86CR0_WP=0x00010000;	// write protect (1=RORO 0=RWRO)
const TUint32 KX86CR0_AM=0x00040000;	// alignment check enable
const TUint32 KX86CR0_NW=0x20000000;	// not write through
const TUint32 KX86CR0_CD=0x40000000;	// cache disable
const TUint32 KX86CR0_PG=0x80000000;	// paging enable

// CR1 reserved

// CR2 = fault address on page fault

// CR3
const TUint32 KX86CR3_PWT=0x00000008;	// page write through
const TUint32 KX86CR3_PCD=0x00000010;	// page cache disable
const TUint32 KX86CR3_PDBR=0xfffff000;	// page directory address

// CR4 (Pentium and above only)
const TUint32 KX86CR4_VME=0x00000001;	// Enable virtual 8086 mode extensions
const TUint32 KX86CR4_PVI=0x00000002;	// Enable virtual 8086 interrupts
const TUint32 KX86CR4_TSD=0x00000004;	// Restrict timestamp to CPL=0
const TUint32 KX86CR4_DE =0x00000008;	// Enable debug extensions
const TUint32 KX86CR4_PSE=0x00000010;	// Enable page size extension (4Mb pages)
const TUint32 KX86CR4_PAE=0x00000020;	// Enable physical address extension (36-bit physical addresses) (P6)
const TUint32 KX86CR4_MCE=0x00000040;	// Enable machine check exception
const TUint32 KX86CR4_PGE=0x00000080;	// Enable global page translations (P6)
const TUint32 KX86CR4_PCE=0x00000100;	// Enable performance monitoring counter at any CPL (P6)
const TUint32 KX86CR4_OSFSXR=0x00000200;		// Pentium II
const TUint32 KX86CR4_OSMMEXCPT=0x00000400;		// Pentium III



/*
 * Segment descriptors
 *
 *  11111111111111110000000000000000
 *  FEDCBA9876543210FEDCBA9876543210
 * basetop8 GD0AxxxxPLLStttt base23-16		HIGH WORD
 *  base 0-15       limit 0-15				LOW WORD
 *
 * xxxx = limit 19:16
 * G = granularity (1=4K, 0=1byte)
 * D = default operation size (code only) 1=32bit 0=16 bit
 * A = unused by processor
 * P = present
 * LL = descriptor privilege level (DPL)
 * S = 0 for system segment, 1 for application segment
 * tttt = type
 *
 * APPLICATION SEGMENTS:
 * Low bit of type is ACCESSED bit (set on any access to segment)
 * Others are as follows:
 * 0 - data RO
 * 1 - data RW
 * 2 - data RO expand-down
 * 3 - data RW expand-down
 * 4 - code execute only
 * 5 - code execute/read
 * 6 - code execute only conforming
 * 7 - code execute/read conforming
 *
 * SYSTEM SEGMENTS:
 * 0 - reserved
 * 1 - Not busy 286 TSS
 * 2 - LDT
 * 3 - Busy 286 TSS
 * 4 - Call gate
 * 5 - Task gate
 * 6 - 286 Interrupt gate
 * 7 - 286 Trap gate
 * 8 - reserved
 * 9 - Not busy 486 TSS
 * A - reserved
 * B - Busy 486 TSS
 * C - 486 Call gate
 * D - reserved
 * E - 486 Interrupt gate
 * F - 486 Trap gate
 *
 * For task gates, the lower 16 bits of the base give the TSS selector
 * For interrupt/trap gates, bits 16-31 of HIGH WORD give the upper 16 bits of EIP,
 *		bits 0-15 of LOW WORD give lower 16 bits of EIP,
 *		bits 16-31 of LOW WORD give code segment selector.
 */

// Segment selectors
typedef TUint16 TX86Selector;

struct SX86Des
	{
	TUint32 iLow;
	TUint32 iHigh;
	};

#define RING0_CS		0x08
#define RING0_DS		0x10
#define RING3_CS		0x1B
#define RING3_DS		0x23

const TX86Selector KRing0CS=RING0_CS;
const TX86Selector KRing0DS=RING0_DS;
const TX86Selector KRing3CS=RING3_CS;
const TX86Selector KRing3DS=RING3_DS;

#ifdef __SMP__
#define	TSS_SELECTOR(cpu)	(0x28+((cpu)<<3))
const TInt KSmpGdtSize = 5 + K_Max_Cpus;
#else
#define TSS_SELECTOR	0x28
const TX86Selector KSelectorTSS = TSS_SELECTOR;
const TInt KGdtSize = 6;
const TInt KSmpGdtSize = 13;
#endif

const TInt KIdtSize=256;

// TSS
struct TX86Tss
	{
	TX86Selector iLink;
	TUint16 iFiller1;
	TLinAddr iEsp0;
	TX86Selector iSs0;
	TUint16 iFiller2;
	TLinAddr iEsp1;
	TX86Selector iSs1;
	TUint16 iFiller3;
	TLinAddr iEsp2;
	TX86Selector iSs2;
	TUint16 iFiller4;
	TX86Reg iCR3;
	TX86Reg iEip;
	TX86Reg iEFlags;
	TX86Reg iEax;
	TX86Reg iEcx;
	TX86Reg iEdx;
	TX86Reg iEbx;
	TX86Reg iEsp;
	TX86Reg iEbp;
	TX86Reg iEsi;
	TX86Reg iEdi;
	TX86Selector iEs;
	TUint16 iFiller6;
	TX86Selector iCs;
	TUint16 iFiller7;
	TX86Selector iSs;
	TUint16 iFiller8;
	TX86Selector iDs;
	TUint16 iFiller9;
	TX86Selector iFs;
	TUint16 iFiller10;
	TX86Selector iGs;
	TUint16 iFiller11;
	TX86Selector iLdt;
	TUint16 iFiller12;
	TUint16 iTbit;
	TUint16 iIOMapOffset;
	};


class TX86RegSet
	{
public:
	TX86Reg	iEax;
	TX86Reg	iEbx;
	TX86Reg	iEcx;
	TX86Reg	iEdx;
	TX86Reg	iEsp;
	TX86Reg	iEbp;
	TX86Reg	iEsi;
	TX86Reg	iEdi;
	TX86Reg	iCs;
	TX86Reg	iDs;
	TX86Reg	iEs;
	TX86Reg	iFs;
	TX86Reg	iGs;
	TX86Reg	iSs;
	TX86Reg	iEflags;
	TX86Reg	iEip;
	};


struct SFullX86RegSet
	{
	TX86Reg	iEax;
	TX86Reg	iEbx;
	TX86Reg	iEcx;
	TX86Reg	iEdx;
	TX86Reg	iEsp;
	TX86Reg	iEbp;
	TX86Reg	iEsi;
	TX86Reg	iEdi;
	TX86Reg	iCs;
	TX86Reg	iDs;
	TX86Reg	iEs;
	TX86Reg	iFs;
	TX86Reg	iGs;
	TX86Reg	iSs;
	TX86Reg	iEflags;
	TX86Reg	iEip;
	TAny*	iFaultCategory;
	TInt	iFaultReason;
	TInt	iIrqNestCount;
	};

const TLinAddr	KApBootPage	= 0xC000;			// 4 low memory pages used to boot APs
												// also used for BIOS calls

// This structure must be placed in an identity mapped page in base memory (ie below 000A0000)
// The code at the beginning of this page is entered by a warm reset of the AP.
struct SApInitInfo
	{
	TUint64	iCode[256];		// iCode[254,255] = mov cr0, eax + intersegment jump
	SX86Des iTempGdt[16];	// temporary identity mapped copy of GDT
	TUint64 iTempGdtr;		// Temporary GDTR referencing temporary GDT
	TUint64	iGdtr;			// initial GDTR
	TUint64 iIdtr;			// initial IDTR
	TUint32 iCr0;			// initial CR0
	TUint32 iCr3;			// initial CR3
	TUint32 iCr4;			// initial CR4
	TUint32 iBootFlag;		// flag to indicate AP has booted
	TUint32 iBootFlag2;		// flag used while synchronizing timestamps
	TUint32 iLinAddr;		// flat linear address of this structure
	TUint32 iStackBase;
	TUint32 iStackSize;
	TAny*	iExtra;
	TAny*	iExtra2;
	TUint64 iBPTimestamp;
	TUint64	iAPTimestamp;
	TX86RegSet iRgs;
	};


#ifdef __SMP__
struct SCpuData
	{
	TX86Tss iTss;
	TUint32	iTssPad[64-sizeof(TX86Tss)/4];
	SFullX86RegSet iRegs;
	TUint32 iRegPad[64-sizeof(SFullX86RegSet)/4];
	TUint64 iIrqStack[256-64];
	};

class TCpuPages
	{
public:
	SX86Des iGdt[KSmpGdtSize];
	TUint32	iGdtPad[64 - sizeof(SX86Des)/4*KSmpGdtSize];
	SX86Des iIdt[KIdtSize];
	TUint64 iCpuPgPad[512-256-32];
	SCpuData iCpuData[K_Max_Cpus];
	};

const TUint32	KBootFlagMagic = 0xDDB3D743;
const TLinAddr	KMpInfoAddr = 0x91000;			// address where MP info table is copied by bootloader

extern TLinAddr ApTrampolinePage;

struct SCpuBootData
	{
	TUint8	iPresent;
	TUint8	iAPICID;
	TUint8	iAPICVersion;
	TUint8	iCpuFlags;
	TUint32	iSignature;
	TUint32	iFeatureFlags;
	TUint32	iRsvd2;
	TUint32	iRsvd3;
	};
#else
class TCpuPage
	{
public:
	SX86Des iGdt[KGdtSize];		// 030h bytes @ 000h
	TX86Tss	iTss;				// 068h bytes @ 030h
	TUint8 iOffset_Idt[0x68];	// 068h bytes @ 098h
	SX86Des iIdt[KIdtSize];		// 800h bytes @ 100h
	};
#endif

// CPU feature flags stored in SSuperPageBase::iCpuId
enum TX86CpuFeatures
	{
	EX86Feat_DE		=0x00000004,			// CPU supports debug extensions
	EX86Feat_PSE	=0x00000008,			// CPU supports 4M pages
	EX86Feat_TSC	=0x00000010,			// CPU has timestamp counter
	EX86Feat_MSR	=0x00000020,			// CPU has RDMSR, WRMSR
	EX86Feat_PAE	=0x00000040,			// CPU supports Physical Address Extension
	EX86Feat_CX8	=0x00000100,			// CPU supports CMPXCHG8B
	EX86Feat_APIC	=0x00000200,			// APIC on chip
	EX86Feat_PGE	=0x00002000,			// CPU supports global mappings
	EX86Feat_MMX	=0x00800000,			// MMX technology
	EX86Feat_FXSR	=0x01000000,			// FXSAVE/FXRSTOR
	EX86Feat_SSE	=0x02000000,			// SSE extensions
	EX86Feat_SSE2	=0x04000000,			// SSE2 extensions
	EX86Feat_HTT	=0x10000000,			// Hyperthreading

	EX86Feat_CR4	=0x01002048,			// CPU supports CR4 if any of these are set
	};


// Macros for instructions not recognised by VC6
#define	REG_EAX		0
#define	REG_ECX		1
#define	REG_EDX		2
#define	REG_EBX		3
#define	REG_ESP		4
#define	REG_EBP		5
#define	REG_ESI		6
#define	REG_EDI		7

#endif
