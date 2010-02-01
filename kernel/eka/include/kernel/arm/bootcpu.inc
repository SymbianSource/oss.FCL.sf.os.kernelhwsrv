; Copyright (c) 2003-2008 Nokia Corporation and/or its subsidiary(-ies).
; All rights reserved.
; This component and the accompanying materials are made available
; under the terms of the License "Eclipse Public License v1.0"
; which accompanies this distribution, and is available
; at the URL "http://www.eclipse.org/legal/epl-v10.html".
;
; Initial Contributors:
; Nokia Corporation - initial contribution.
;
; Contributors:
;
; Description:
; e32\include\kernel\arm\bootcpu.inc
;
;
;*******************************************************************************
;
; CPU/MMU definitions for bootstrap
;

	IF	:LNOT: :DEF: __BOOTCPU_INC__
	GBLL	__BOOTCPU_INC__

	MACRO
	INIT_LOGICAL_SYMBOL	$sym, $count
	IF :LNOT: :DEF: $sym
		GBLL	$sym
$sym	SETL	{FALSE}
	ELSE
$sym	SETL	{TRUE}
		IF :LEN: "$count" > 0
$count		SETA	$count + 1
		ENDIF
	ENDIF
	MEND

	MACRO
	INIT_NUMERIC_SYMBOL	$sym, $default
	IF :LNOT: :DEF: $sym
		GBLA	$sym
$sym	SETA	$default
	ENDIF
	MEND

	MACRO
	INIT_NUMERIC_CONSTANT	$sym, $default
	IF :LNOT: :DEF: $sym
$sym	EQU		$default
	ENDIF
	MEND

; CP15 control register bits
; Define these here so they can be used in config.inc to override MMU settings
MMUCR_M		EQU		0x00000001			; MMU enable
MMUCR_A		EQU		0x00000002			; Alignment check
MMUCR_C		EQU		0x00000004			; DCache enable
MMUCR_W		EQU		0x00000008			; Write buffer enable
MMUCR_SBO	EQU		0x00000070			; Always 1
MMUCR_B		EQU		0x00000080			; Big endian
MMUCR_S		EQU		0x00000100			; System permission modifier
MMUCR_R		EQU		0x00000200			; ROM permission modifier
MMUCR_F		EQU		0x00000400			;
MMUCR_Z		EQU		0x00000800			; Flow prediction enable
MMUCR_I		EQU		0x00001000			; ICache enable
MMUCR_V		EQU		0x00002000			; High vectors
MMUCR_RR	EQU		0x00004000			; Round Robin cache replacement
MMUCR_L4	EQU		0x00008000			; LDR PC v4 behaviour (don't set T bit)
MMUCR_DT	EQU		0x00010000			; DTCM enable

MMUCR_IT	EQU		0x00040000			; ITCM enable


MMUCR_FI	EQU		0x00200000			; Fast Interrupt configuration (low latency)
MMUCR_U		EQU		0x00400000			; Unaligned data access enable (mixed endian)
MMUCR_XP	EQU		0x00800000			; ARMv6 extended page table format
MMUCR_VE	EQU		0x01000000			; Vectored interrupt enable
MMUCR_EE	EQU		0x02000000			; CPSR E bit following exception (mixed endian)
MMUCR_L2XSCALE	EQU	0x04000000			; L2 Cache enable on XScale platform
MMUCR_TRE	EQU		0x10000000			; TEX remapping
MMUCR_FA	EQU		0x20000000			; AP[0] is used as Access Bit. Four (instead of eight) access permissions are available.

	INCLUDE	config.inc

	INIT_LOGICAL_SYMBOL	CFG_DebugBootRom
	INIT_LOGICAL_SYMBOL	CFG_CustomVectors
	INIT_LOGICAL_SYMBOL	CFG_UseBootstrapVectors
	INIT_LOGICAL_SYMBOL	CFG_AutoDetectROM
	INIT_LOGICAL_SYMBOL	CFG_IncludeRAMAllocator
	INIT_LOGICAL_SYMBOL	CFG_BootLoader
	INIT_LOGICAL_SYMBOL	SMP

	IF	SMP
	GBLL	CFG_USE_SHARED_MEMORY
CFG_USE_SHARED_MEMORY		SETL	{TRUE}
	ENDIF

	INCLUDE	bootmacro.inc
	INCLUDE	bootdefs.inc
	INCLUDE kernboot.inc
	INCLUDE e32rom.inc

; Check memory model definition
	GBLA	NMM
NMM	SETA	0

	INIT_LOGICAL_SYMBOL	CFG_MMDirect, NMM
	INIT_LOGICAL_SYMBOL	CFG_MMMoving, NMM
	INIT_LOGICAL_SYMBOL	CFG_MMMultiple, NMM
	INIT_LOGICAL_SYMBOL	CFG_MMFlexible, NMM

	IF	NMM=0
		! 1, "No memory model specified"
	ENDIF
	IF	NMM>1
		! 2, "More than one memory model specified"
	ENDIF

	IF :LNOT: CFG_MMDirect
		INCLUDE	mmboot.inc
	ENDIF

	INIT_NUMERIC_CONSTANT	CFG_RomSizeAlign, 16	; 64K by default

; First declare variables
	GBLA	NCPU
NCPU	SETA	0
	IF	:DEF: CPUOK
NCPU	SETA	1
	ENDIF

	GBLL	CFG_ARMV7
CFG_ARMV7					SETL	{FALSE}
	GBLL	CFG_ARMV6
CFG_ARMV6					SETL	{FALSE}
	GBLL	CFG_MMUPresent
CFG_MMUPresent				SETL	{FALSE}
	GBLL	CFG_CachePresent
CFG_CachePresent			SETL	{FALSE}
	GBLL	CFG_WriteBufferPresent
CFG_WriteBufferPresent		SETL	{FALSE}
	GBLL	CFG_SplitCache
CFG_SplitCache				SETL	{FALSE}
	GBLL	CFG_SplitTLB
CFG_SplitTLB				SETL	{FALSE}
	GBLL	CFG_AltDCachePresent
CFG_AltDCachePresent		SETL	{FALSE}
	GBLL	CFG_WriteBackCache
CFG_WriteBackCache			SETL	{FALSE}
	GBLL	CFG_CacheWriteAllocate
CFG_CacheWriteAllocate		SETL	{FALSE}
	GBLL	CFG_CachePhysicalTag
CFG_CachePhysicalTag		SETL	{FALSE}
	GBLL	CFG_CacheFlushByDataRead
CFG_CacheFlushByDataRead	SETL	{FALSE}
	GBLL	CFG_CacheFlushByWaySetIndex
CFG_CacheFlushByWaySetIndex	SETL	{FALSE}
	GBLL	CFG_CacheFlushByLineAlloc
CFG_CacheFlushByLineAlloc	SETL	{FALSE}
	GBLL	CFG_CachePolicyInPTE
CFG_CachePolicyInPTE		SETL	{FALSE}
	GBLL	CFG_TEX
CFG_TEX						SETL	{FALSE}
	GBLL	CFG_SingleEntryDCacheFlush
CFG_SingleEntryDCacheFlush	SETL	{FALSE}
	GBLL	CFG_SingleEntryICacheFlush
CFG_SingleEntryICacheFlush	SETL	{FALSE}
	GBLL	CFG_SingleEntryITLBFlush
CFG_SingleEntryITLBFlush	SETL	{FALSE}
	GBLL	CFG_SingleEntryTLBFlush
CFG_SingleEntryTLBFlush		SETL	{FALSE}
	GBLL	CFG_CacheTypeReg
CFG_CacheTypeReg			SETL	{FALSE}
	GBLL	CFG_BTBPresent
CFG_BTBPresent				SETL	{FALSE}
	GBLL	CFG_AuxCRPresent
CFG_AuxCRPresent			SETL	{FALSE}
	GBLL	CFG_CARPresent
CFG_CARPresent				SETL	{FALSE}
	GBLL	CFG_PrefetchBuffer
CFG_PrefetchBuffer			SETL	{FALSE}
	GBLL	CFG_FCSE_Present
CFG_FCSE_Present			SETL	{FALSE}
	GBLL	CFG_ASID_Present
CFG_ASID_Present			SETL	{FALSE}
	GBLL	CFG_Cpu_Has_CLZ
CFG_Cpu_Has_CLZ				SETL	{FALSE}
	GBLL	CFG_Cpu_Has_TrustZone
CFG_Cpu_Has_TrustZone		SETL	{FALSE}
;TrustZone platforms with Symbian OS Kernel running in Non-Secure Mode
	GBLL	CFG_TrustZone_NonSecure
CFG_TrustZone_NonSecure		SETL	{FALSE}
; Cache attributes are re-mapped into TEX[0]:C:B only. TEX[2:1] are freed up for S/W purpose.
; Also, access permissions are mapped to APX:AP[1] only. AP[0] is freed up for S/W purpose.
	GBLL	CFG_MemoryTypeRemapping
CFG_MemoryTypeRemapping		SETL	{FALSE}
; Page tables are Write-Back cached. Mandatory if CFG_MemoryTypeRemapping is TRUE 
	GBLL	CFG_WriteThroughDisabled
CFG_WriteThroughDisabled	SETL	{FALSE}
	GBLL	CFG_Cpu_Has_WFE_SEV
CFG_Cpu_Has_WFE_SEV			SETL	{FALSE}
	GBLL	CFG_Cpu_Has_WFI
CFG_Cpu_Has_WFI				SETL	{FALSE}



;*******************************************************************************
	IF	:DEF: CFG_CPU_GENERIC_ARM4

NCPU	SETA	NCPU+1

	ENDIF

;*******************************************************************************
	IF	:DEF: CFG_CPU_ARM710T :LOR: :DEF: CFG_CPU_ARM720T

NCPU	SETA	NCPU+1

CFG_MMUPresent				SETL	{TRUE}
CFG_CachePresent			SETL	{TRUE}
CFG_WriteBufferPresent		SETL	{TRUE}
CFG_SingleEntryTLBFlush		SETL	{TRUE}

	IF	:DEF: CFG_CPU_ARM720T
CFG_FCSE_Present			SETL	{TRUE}
	ENDIF

	INIT_NUMERIC_CONSTANT	InitialMMUCR,	MMUCR_A+MMUCR_SBO+MMUCR_R
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_W

PDE_EXTRA					EQU		0x00000010

	ENDIF

;*******************************************************************************
	IF	:DEF: CFG_CPU_SA1

NCPU	SETA	NCPU+1

CFG_MMUPresent				SETL	{TRUE}
CFG_CachePresent			SETL	{TRUE}
CFG_WriteBufferPresent		SETL	{TRUE}
CFG_SplitCache				SETL	{TRUE}
CFG_SplitTLB				SETL	{TRUE}
CFG_AltDCachePresent		SETL	{TRUE}
CFG_WriteBackCache			SETL	{TRUE}
CFG_CacheFlushByDataRead	SETL	{TRUE}
CFG_SingleEntryDCacheFlush	SETL	{TRUE}
CFG_FCSE_Present			SETL	{TRUE}

	INIT_NUMERIC_CONSTANT	InitialMMUCR,	MMUCR_A+MMUCR_SBO+MMUCR_R+MMUCR_I
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_W+MMUCR_V

PDE_EXTRA					EQU		0x00000000

	ENDIF

;*******************************************************************************
	IF	:DEF: CFG_CPU_ARM920T	:LOR:	:DEF: CFG_CPU_ARM925T	:LOR:	:DEF: CFG_CPU_ARM926J

NCPU	SETA	NCPU+1

CFG_MMUPresent				SETL	{TRUE}
CFG_CachePresent			SETL	{TRUE}
CFG_WriteBufferPresent		SETL	{TRUE}
CFG_SplitCache				SETL	{TRUE}
CFG_SplitTLB				SETL	{TRUE}
CFG_WriteBackCache			SETL	{TRUE}
CFG_CacheFlushByWaySetIndex	SETL	{TRUE}
CFG_CachePolicyInPTE		SETL	{TRUE}
CFG_CacheTypeReg			SETL	{TRUE}
CFG_SingleEntryDCacheFlush	SETL	{TRUE}
CFG_SingleEntryICacheFlush	SETL	{TRUE}
CFG_SingleEntryITLBFlush	SETL	{TRUE}
CFG_FCSE_Present			SETL	{TRUE}

	IF	:DEF: CFG_CPU_ARM926J
CFG_Cpu_Has_CLZ				SETL	{TRUE}
	ENDIF

	IF	:DEF: CFG_CPU_ARM926J
	INIT_NUMERIC_CONSTANT	InitialMMUCR,	MMUCR_A+MMUCR_W+MMUCR_SBO+MMUCR_R+MMUCR_I+0x00050000
	ELSE
	INIT_NUMERIC_CONSTANT	InitialMMUCR,	MMUCR_A+MMUCR_W+MMUCR_SBO+MMUCR_R+MMUCR_I
	ENDIF

	IF	:DEF: CFG_CPU_ARM920T
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_V+0xC0000000	; FastBus + AsyncClock
	ELSE
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_V
	ENDIF

PDE_EXTRA					EQU		0x00000010

	ENDIF

;*******************************************************************************
	IF	:DEF: CFG_CPU_XSCALE

NCPU	SETA	NCPU+1

CFG_Cpu_Has_CLZ				SETL	{TRUE}
CFG_MMUPresent				SETL	{TRUE}
CFG_CachePresent			SETL	{TRUE}
CFG_WriteBufferPresent		SETL	{TRUE}
CFG_SplitCache				SETL	{TRUE}
CFG_SplitTLB				SETL	{TRUE}
	IF	:LNOT: :DEF: CFG_CPU_MANZANO
CFG_AltDCachePresent		SETL	{TRUE}
CFG_CacheFlushByLineAlloc	SETL	{TRUE}
	ENDIF
CFG_CacheWriteAllocate		SETL	{TRUE}
CFG_WriteBackCache			SETL	{TRUE}
CFG_SingleEntryDCacheFlush	SETL	{TRUE}
CFG_SingleEntryICacheFlush	SETL	{TRUE}
CFG_SingleEntryITLBFlush	SETL	{TRUE}
CFG_CachePolicyInPTE		SETL	{TRUE}
CFG_CacheTypeReg			SETL	{TRUE}
CFG_BTBPresent				SETL	{TRUE}
CFG_AuxCRPresent			SETL	{TRUE}
CFG_CARPresent				SETL	{TRUE}
CFG_FCSE_Present			SETL	{TRUE}
CFG_TEX						SETL	{TRUE}

	INIT_NUMERIC_CONSTANT	InitialMMUCR,	MMUCR_A+MMUCR_W+MMUCR_SBO+MMUCR_R+MMUCR_I
	IF	:DEF: CFG_HasXScaleL2Cache
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_V+MMUCR_Z+MMUCR_L2XSCALE
	ELSE
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_V+MMUCR_Z
	ENDIF

	INIT_NUMERIC_CONSTANT	DefaultAuxCRClear,	0xFFFFFFFF
	INIT_NUMERIC_CONSTANT	DefaultAuxCRSet,	0x00000020	; enable WB coalescing, minicache WTRA

PDE_EXTRA					EQU		0x00000000

	ENDIF

;*******************************************************************************
	IF	:DEF: CFG_CPU_ARM1136

NCPU	SETA	NCPU+1

CFG_ARMV6					SETL	{TRUE}
CFG_MMUPresent				SETL	{TRUE}
CFG_CachePresent			SETL	{TRUE}
CFG_CachePhysicalTag		SETL	{TRUE}
CFG_WriteBufferPresent		SETL	{TRUE}
CFG_SplitCache				SETL	{TRUE}
CFG_CacheWriteAllocate		SETL	{TRUE}
CFG_WriteBackCache			SETL	{TRUE}
CFG_CacheFlushByWaySetIndex	SETL	{TRUE}
CFG_SingleEntryDCacheFlush	SETL	{TRUE}
CFG_SingleEntryICacheFlush	SETL	{TRUE}
CFG_SingleEntryTLBFlush		SETL	{TRUE}
CFG_CachePolicyInPTE		SETL	{TRUE}
CFG_CacheTypeReg			SETL	{TRUE}
CFG_BTBPresent				SETL	{TRUE}
CFG_AuxCRPresent			SETL	{TRUE}
CFG_CARPresent				SETL	{TRUE}
CFG_PrefetchBuffer			SETL	{TRUE}
CFG_FCSE_Present			SETL	{TRUE}
CFG_ASID_Present			SETL	{TRUE}
CFG_Cpu_Has_CLZ				SETL	{TRUE}
CFG_TEX						SETL	{TRUE}
	IF	CFG_MMFlexible
; flexible memory model doesn't use Write Through memory for internal mappings. 
CFG_WriteThroughDisabled	SETL	{TRUE}
	ELSE
; multiple memory model uses Write Through memory for internal mappings unless erratum 399234 prevents us to do so 
	IF  (:LNOT: :DEF: CFG_CPU_ARM1136_ERRATUM_399234_FIXED)
CFG_WriteThroughDisabled	SETL	{TRUE}
	ENDIF
	ENDIF
	
	INIT_NUMERIC_CONSTANT	InitialMMUCR,	MMUCR_A+MMUCR_W+MMUCR_SBO+MMUCR_I+MMUCR_IT+MMUCR_DT
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_V+MMUCR_Z+MMUCR_XP+MMUCR_U

	INIT_NUMERIC_CONSTANT	DefaultAuxCRClear,	0x0000003F

	IF  (:LNOT: :DEF: CFG_CPU_ARM1136_ERRATUM_415662_FIXED :LOR: :LNOT: :DEF: CFG_CPU_ARM1136_ERRATUM_411920_FIXED)
	; base port should also ensure that BPR_AuxCRSet parameter (if used) has RV bit set
	INIT_NUMERIC_CONSTANT	DefaultAuxCRSet,	0x00000027	; enable static/dynamic/return prediction and
															; disable "Block Transfer Cache Operations" (RV bit)
	ELSE
	INIT_NUMERIC_CONSTANT	DefaultAuxCRSet,	0x00000007	; enable static/dynamic/return prediction
	ENDIF

PDE_EXTRA					EQU		0x00000000

	ENDIF

;*******************************************************************************
	IF	:DEF: CFG_CPU_ARM1176

NCPU	SETA	NCPU+1

CFG_ARMV6					SETL	{TRUE}
CFG_MMUPresent				SETL	{TRUE}
CFG_CachePresent			SETL	{TRUE}
CFG_CachePhysicalTag		SETL	{TRUE}
CFG_WriteBufferPresent		SETL	{TRUE}
CFG_SplitCache				SETL	{TRUE}
CFG_CacheWriteAllocate		SETL	{TRUE}
CFG_WriteBackCache			SETL	{TRUE}
CFG_CacheFlushByWaySetIndex	SETL	{TRUE}
CFG_SingleEntryDCacheFlush	SETL	{TRUE}
CFG_SingleEntryICacheFlush	SETL	{TRUE}
CFG_SingleEntryTLBFlush		SETL	{TRUE}
CFG_CachePolicyInPTE		SETL	{TRUE}
CFG_CacheTypeReg			SETL	{TRUE}
CFG_BTBPresent				SETL	{TRUE}
;CFG_AuxCRPresent			SETL	{TRUE} ;set to false because AuxCR is not write accessible in non-secure mode
CFG_CARPresent				SETL	{TRUE}
CFG_PrefetchBuffer			SETL	{TRUE}
CFG_FCSE_Present			SETL	{TRUE}
CFG_ASID_Present			SETL	{TRUE}
CFG_Cpu_Has_CLZ				SETL	{TRUE}
CFG_TEX						SETL	{TRUE}
CFG_MemoryTypeRemapping		SETL	{TRUE}
CFG_WriteThroughDisabled	SETL	{TRUE}
CFG_Cpu_Has_TrustZone		SETL	{TRUE}
	IF :LNOT: :DEF: CFG_TrustZone_Secure
CFG_TrustZone_NonSecure		SETL	{TRUE}
	ENDIF

	INIT_NUMERIC_CONSTANT	InitialMMUCR,	MMUCR_A+MMUCR_W+MMUCR_SBO+MMUCR_I+MMUCR_IT+MMUCR_DT+MMUCR_TRE+MMUCR_FA
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_V+MMUCR_Z+MMUCR_XP+MMUCR_U

	INIT_NUMERIC_CONSTANT	DefaultAuxCRClear,	0x0000003F
	IF  (:LNOT: :DEF: CFG_CPU_ARM1136_ERRATUM_411920_FIXED)
	; ARM1136 errata suggests this fix for 1176 as well
	; base port should also ensure that BPR_AuxCRSet parameter (if used) has RV bit set
	INIT_NUMERIC_CONSTANT	DefaultAuxCRSet,	0x00000027	; enable static/dynamic/return prediction and
															; disable "Block Transfer Cache Operations" (RV bit)
	ELSE
	INIT_NUMERIC_CONSTANT	DefaultAuxCRSet,	0x00000007	; enable static/dynamic/return prediction
	ENDIF

PDE_EXTRA					EQU		0x00000000

	ENDIF

;*******************************************************************************
	IF	:DEF: CFG_CPU_ARM11MP

NCPU	SETA	NCPU+1

CFG_ARMV6					SETL	{TRUE}
CFG_MMUPresent				SETL	{TRUE}
CFG_CachePresent			SETL	{TRUE}
CFG_CachePhysicalTag		SETL	{TRUE}
CFG_WriteBufferPresent		SETL	{TRUE}
CFG_SplitCache				SETL	{TRUE}
CFG_CacheWriteAllocate		SETL	{TRUE}
CFG_WriteBackCache			SETL	{TRUE}
CFG_CacheFlushByWaySetIndex	SETL	{TRUE}
CFG_SingleEntryDCacheFlush	SETL	{TRUE}
CFG_SingleEntryICacheFlush	SETL	{TRUE}
CFG_SingleEntryTLBFlush		SETL	{TRUE}
CFG_CachePolicyInPTE		SETL	{TRUE}
CFG_CacheTypeReg			SETL	{TRUE}
CFG_BTBPresent				SETL	{TRUE}
CFG_AuxCRPresent			SETL	{TRUE}
CFG_CARPresent				SETL	{TRUE}
CFG_PrefetchBuffer			SETL	{TRUE}
CFG_FCSE_Present			SETL	{TRUE}
CFG_ASID_Present			SETL	{TRUE}
CFG_TEX						SETL	{TRUE}
CFG_MemoryTypeRemapping		SETL	{TRUE}
CFG_Cpu_Has_CLZ				SETL	{TRUE}
CFG_WriteThroughDisabled	SETL	{TRUE}
CFG_Cpu_Has_WFE_SEV			SETL	{TRUE}
CFG_Cpu_Has_WFI				SETL	{TRUE}

	INIT_NUMERIC_CONSTANT	InitialMMUCR,	MMUCR_A+MMUCR_W+MMUCR_SBO+MMUCR_I+MMUCR_IT+MMUCR_DT+MMUCR_TRE+MMUCR_FA
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_V+MMUCR_Z+MMUCR_XP+MMUCR_U

	INIT_NUMERIC_CONSTANT	DefaultAuxCRClear,	0x0000003F

	IF :DEF: CFG_USE_SHARED_MEMORY
	INIT_NUMERIC_CONSTANT	SET_AUXSMP,	0x20
	ELSE
	INIT_NUMERIC_CONSTANT	SET_AUXSMP,	0x0
	ENDIF

	INIT_NUMERIC_CONSTANT	DefaultAuxCRSet,	0x00000007 | SET_AUXSMP	; enable static/dynamic/return prediction

PDE_EXTRA					EQU		0x00000000

	ENDIF

;*******************************************************************************
	IF	:DEF: CFG_CPU_CORTEX_A8N
	IF	:LNOT: :DEF: CFG_CPU_CORTEX_A8
	GBLL	CFG_CPU_CORTEX_A8
	ENDIF
	ENDIF

	IF	:DEF: CFG_CPU_CORTEX_A8

NCPU	SETA	NCPU+1

CFG_ARMV7					SETL	{TRUE}
CFG_MMUPresent				SETL	{TRUE}
CFG_CachePresent			SETL	{TRUE}
CFG_CachePhysicalTag		SETL	{TRUE}
CFG_WriteBufferPresent		SETL	{TRUE}
CFG_SplitCache				SETL	{TRUE}
CFG_AltDCachePresent		SETL	{TRUE}
CFG_CacheWriteAllocate		SETL	{TRUE}
CFG_WriteBackCache			SETL	{TRUE}
CFG_CacheFlushByWaySetIndex	SETL	{TRUE}
CFG_SingleEntryDCacheFlush	SETL	{TRUE}
CFG_SingleEntryICacheFlush	SETL	{TRUE}
CFG_SingleEntryTLBFlush		SETL	{TRUE}
CFG_CachePolicyInPTE		SETL	{TRUE}
CFG_CacheTypeReg			SETL	{TRUE}
CFG_BTBPresent				SETL	{TRUE}
CFG_CARPresent				SETL	{TRUE}
CFG_PrefetchBuffer			SETL	{TRUE}
CFG_FCSE_Present			SETL	{TRUE}
CFG_ASID_Present			SETL	{TRUE}
CFG_TEX						SETL	{TRUE}
CFG_Cpu_Has_CLZ				SETL	{TRUE}
CFG_Cpu_Has_TrustZone		SETL	{TRUE}
	IF :LNOT: :DEF: CFG_TrustZone_Secure
CFG_TrustZone_NonSecure		SETL	{TRUE}
	ELSE
CFG_AuxCRPresent			SETL	{TRUE} ; AuxCR is not write accessible in non-secure mode
	ENDIF
CFG_MemoryTypeRemapping		SETL	{TRUE}
CFG_WriteThroughDisabled	SETL	{TRUE}
CFG_Cpu_Has_WFE_SEV			SETL	{TRUE}
CFG_Cpu_Has_WFI				SETL	{TRUE}
	INIT_NUMERIC_CONSTANT	InitialMMUCR,	MMUCR_A+MMUCR_W+MMUCR_SBO+MMUCR_I+MMUCR_DT+MMUCR_IT+MMUCR_U+MMUCR_XP+MMUCR_TRE+MMUCR_FA
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_V+MMUCR_Z

	INIT_NUMERIC_CONSTANT	DefaultAuxCRClear,	0x00000000
	INIT_NUMERIC_CONSTANT	DefaultAuxCRSet,	0x00000002 ;Enable L2 Cache

PDE_EXTRA					EQU		0x00000000

	ENDIF

;*******************************************************************************
	IF	:DEF: CFG_CPU_CORTEX_A9

NCPU	SETA	NCPU+1

CFG_ARMV7					SETL	{TRUE}
CFG_MMUPresent				SETL	{TRUE}
CFG_CachePresent			SETL	{TRUE}
CFG_CachePhysicalTag		SETL	{TRUE}
CFG_WriteBufferPresent		SETL	{TRUE}
CFG_SplitCache				SETL	{TRUE}
CFG_AltDCachePresent		SETL	{TRUE}
CFG_CacheWriteAllocate		SETL	{TRUE}
CFG_WriteBackCache			SETL	{TRUE}
CFG_CacheFlushByWaySetIndex	SETL	{TRUE}
CFG_SingleEntryDCacheFlush	SETL	{TRUE}
CFG_SingleEntryICacheFlush	SETL	{TRUE}
CFG_SingleEntryTLBFlush		SETL	{TRUE}
CFG_CachePolicyInPTE		SETL	{TRUE}
CFG_CacheTypeReg			SETL	{TRUE}
CFG_BTBPresent				SETL	{TRUE}
CFG_CARPresent				SETL	{TRUE}
CFG_PrefetchBuffer			SETL	{TRUE}
CFG_FCSE_Present			SETL	{TRUE}
CFG_ASID_Present			SETL	{TRUE}
CFG_TEX						SETL	{TRUE}
CFG_Cpu_Has_CLZ				SETL	{TRUE}
CFG_Cpu_Has_TrustZone		SETL	{TRUE}
	IF :LNOT: :DEF: CFG_TrustZone_Secure
CFG_TrustZone_NonSecure		SETL	{TRUE}
	ELSE
CFG_AuxCRPresent			SETL	{TRUE} ; AuxCR is not write accessible in non-secure mode
	ENDIF
CFG_MemoryTypeRemapping		SETL	{TRUE}
CFG_WriteThroughDisabled	SETL	{TRUE}
CFG_Cpu_Has_WFE_SEV			SETL	{TRUE}
CFG_Cpu_Has_WFI				SETL	{TRUE}
	INIT_NUMERIC_CONSTANT	InitialMMUCR,	MMUCR_A+MMUCR_W+MMUCR_SBO+MMUCR_I+MMUCR_DT+MMUCR_IT+MMUCR_U+MMUCR_XP+MMUCR_TRE+MMUCR_FA
	INIT_NUMERIC_CONSTANT	ExtraMMUCR,		MMUCR_M+MMUCR_C+MMUCR_V+MMUCR_Z

	INIT_NUMERIC_CONSTANT	DefaultAuxCRClear,	0x000003FF

A9_ACTLR_CACHE_TLB_BRDCST	EQU		0x00000001	; ACTLR bit 0 set => Cache/TLB maintenance operations are broadcast to other cores
A9_ACTLR_DSIDE_PREFETCH		EQU		0x00000004	; D-side prefetch enable
A9_ACTLR_FOZ				EQU		0x00000008	; Enable "Full Of Zero" mode (whatever that is)
A9_ACTLR_SMP				EQU		0x00000040	; Enable coherency
A9_ACTLR_CACHE_EXCLUSIVE	EQU		0x00000080	; Enable exclusive caching
A9_ACTLR_PARITY				EQU		0x00000200	; Enable parity checking

	IF :DEF: CFG_USE_SHARED_MEMORY
	INIT_NUMERIC_CONSTANT	SET_AUXSMP,	A9_ACTLR_SMP | A9_ACTLR_CACHE_TLB_BRDCST
	ELSE
	INIT_NUMERIC_CONSTANT	SET_AUXSMP,	0
	ENDIF

	INIT_NUMERIC_CONSTANT	DefaultAuxCRSet,	0x00000000 | SET_AUXSMP

PDE_EXTRA					EQU		0x00000000

	ENDIF

;*******************************************************************************

	IF CFG_MemoryTypeRemapping
; Default values for Primary Region Remap Registers & Normal Memory Remap Registers. Memory types shall be:
; TEX[0]:C:B	Memory Type
;---------------------------
;	000			Strongly ordered
;	001			Device
;	010			Normal, inner & outer uncached
;	011			Normal, inner & outer write back, write/read allocate
;	1XX			Strongly ordered (not used by Kernel)
; Memory types for TEX[0]:C:B = 101, 110 & 111 may be overwritten (for the purpose of use in device drivers)
; by BPR_Platform_Specific_Mappings parameter in Baseport.
; Shared attribute is re-mapped as: 0->0 & 1->1 for both Device and Normal memory
	INIT_NUMERIC_CONSTANT	DefaultPRRR,	0x000a00a4
	INIT_NUMERIC_CONSTANT	DefaultNMRR,	0x00400040
	ENDIF

; MMU page table descriptors
; Level 1 (ARMv4)
PDE_PT		EQU		0x00000001			; page table
PDE_PT_MSK	EQU		0xfffffc00			; page table address mask
PDE_SECTION	EQU		0x00000002			; section
PDE_SEC_MSK	EQU		0xfff00000			; section address mask
PDE_B		EQU		0x00000004			; B bit for section
PDE_C		EQU		0x00000008			; C bit for section
PDE_DOM_SH	EQU		5					; shift for domain in PDE
PDE_DOM_MSK	EQU		0x000001e0			; mask for domain in PDE
PDE_AP_SH	EQU		10					; shift for access permissions (section)
PDE_AP_MSK	EQU		0x00000c00			; mask for access permissions (section)

	IF	CFG_TEX
; Level 1 (ARMv5)
PDE_P		EQU		0x00000200			; PDE P bit (ECC enable)
PDE_TEX_SH	EQU		12					; shift for TEX (section)
PDE_TEX_MSK	EQU		0x00007000			; mask for TEX (section)
	ENDIF

	IF	CFG_ARMV6 :LOR: CFG_ARMV7
; Level 1 (ARMv6)
PDE_XN		EQU		0x00000010			; XN bit for section (ARMv6)
PDE_APX		EQU		0x00008000			; APX for section (ARMv6)
PDE_S		EQU		0x00010000			; S bit for section (ARMv6)
PDE_NG		EQU		0x00020000			; NG bit for section (ARMv6)
	ENDIF

; Level 2 (ARMv4)
PTE_LP		EQU		0x00000001			; large page
PTE_LP_MSK	EQU		0xffff0000			; large page address mask
PTE_SP		EQU		0x00000002			; small page
PTE_SP_MSK	EQU		0xfffff000			; small page address mask
PTE_B		EQU		0x00000004			; PTE B bit
PTE_C		EQU		0x00000008			; PTE C bit
PTE_AP0_SH	EQU		4					; AP0 shift
PTE_AP0_MSK	EQU		0x00000030			; AP0 mask

	IF	CFG_TEX
; Level 2 (ARMv5)
PTE_ESP		EQU		0x00000003			; extended small page
PTE_LP_TEX_SH	EQU	12					; large page TEX shift
PTE_LP_TEX_MSK	EQU	0x00007000			; large page TEX mask
PTE_AP_SH	EQU		4					; AP shift
PTE_AP_MSK	EQU		0x00000030			; AP mask
PTE_ESP_TEX_SH	EQU	6					; extended small page TEX shift
PTE_ESP_TEX_MSK	EQU	0x000000c0			; extended small page TEX mask
	ENDIF

	IF	CFG_ARMV6  :LOR: CFG_ARMV7
; Level 2 (ARMv6)
PTE_ESP_TEX1	EQU	0x00000080			; extended small page TEX1 bit
PTE_APX		EQU		0x00000200			; APX for extended small or large page
PTE_S		EQU		0x00000400			; S for extended small or large page
PTE_NG		EQU		0x00000800			; NG for extended small or large page
PTE_LP_XN	EQU		0x00008000			; XN for large page
PTE_ESP_XN	EQU		0x00000001			; XN for extended small page
	ENDIF

; MMU Permissions (APX:AP)
	IF	CFG_ARMV6 :LOR: CFG_ARMV7

PERM_RWNO		EQU	1
PERM_RWRW		EQU	3
PERM_RONO		EQU	5
	IF CFG_MemoryTypeRemapping
PERM_RORO		EQU	7
	ELSE
PERM_NONO		EQU	0
PERM_RWRO		EQU	2
PERM_RORO		EQU	6
	ENDIF

	ELSE

PERM_RORO		EQU	0
PERM_RWNO		EQU	1
PERM_RWRO		EQU	2
PERM_RWRW		EQU	3

	ENDIF

; Cache attributes (TEX:CB)

	IF	CFG_CachePolicyInPTE
		IF	CFG_ARMV6 :LOR: CFG_ARMV7
			IF CFG_MemoryTypeRemapping
;arm1176, arm11mpcore, armv7
MEMORY_STRONGLY_ORDERED EQU	0x00; strongly ordered
MEMORY_DEVICE			EQU	0x01; device
MEMORY_UNCACHED			EQU	0x02; outer and inner noncacheable, buffered, coalescing
MEMORY_FULLY_CACHED		EQU	0x03; outer and inner write back, read/write allocate
;types 4 & 5 are reserved for Kernel's internal usage (and not in use so far)
MEMORY_SPECIFIC6		EQU	0x06; Baseport specific memory type, as specified by BPR_PRRR_NRRR_Extra
MEMORY_SPECIFIC7		EQU	0x07; Baseport specific memory type, as specified by BPR_PRRR_NRRR_Extra
	
			ELSE
;arm1136
CACHE_SO		EQU	0x00	; strongly ordered
CACHE_SD		EQU	0x01	; shared device
CACHE_WBRA		EQU	0x03	; outer and inner WBRA
CACHE_BUFC		EQU	0x04	; outer and inner noncacheable, buffered, coalescing
CACHE_NSD		EQU	0x08	; nonshared device
CACHE_WBWA		EQU	0x15	; outer and inner WBWA

				IF :DEF: CFG_CPU_ARM1136_ERRATUM_399234_FIXED
CACHE_WTRA		EQU 0x02		; outer and inner WTRA
CACHE_WTRA_WBRA	EQU	0x1e		; outer WBRA inner WTRA (page tables)
CACHE_WTRA_WBWA	EQU	0x16		; outer WBWA inner WTRA (page tables)
				ELSE
; Downgrade Write-Through cache mode to outer and inner noncacheable, buffered, coalescing
CACHE_WTRA		EQU CACHE_BUFC
CACHE_WTRA_WBRA	EQU	CACHE_BUFC
CACHE_WTRA_WBWA	EQU	CACHE_BUFC
				ENDIF

			ENDIF
		ELSE
			IF CFG_TEX
				IF :DEF: CFG_CPU_MANZANO
; Manzano
CACHE_NCNB		EQU	0	; not cached or buffered
CACHE_BUFC		EQU	4	; buffered, coalescing, not cached
CACHE_WTRA		EQU 0x12; write through read allocate
CACHE_WBRA		EQU	0x13; write back read allocate
CACHE_BFNC		EQU	5	; buffered, non-coalescing, not cached
CACHE_OFF_WBWA	EQU	0x14; L1 not cached/buffered			L2 write back write allocate
CACHE_WTRA_WBWA	EQU	0x16; L1 write throught read allocate	L2 write back write allocate
CACHE_WBRA_WBWA	EQU	0x17; L1 write back read allocate		L2 write back write allocate
				ELSE
; Basic XScale
CACHE_NCNB		EQU	0	; not cached or buffered
CACHE_BUFC		EQU	1	; buffered, coalescing, not cached
CACHE_WTRA		EQU 2	; write through read allocate
CACHE_WBRA		EQU	3	; write back read allocate
CACHE_BFNC		EQU	5	; buffered, non-coalescing, not cached
CACHE_MINI		EQU	6	; minicache
CACHE_WBWA		EQU	7	; write back write allocate
				ENDIF
			ELSE
; ARM9/10
CACHE_NCNB		EQU	0	; not cached or buffered
CACHE_BUF		EQU	1	; buffered not cached
CACHE_WT		EQU 2	; write through
CACHE_WB		EQU	3	; write back
			ENDIF
		ENDIF

	ELSE

		IF	CFG_WriteBackCache
; SA1
CACHE_NCNB		EQU	0	; not cached or buffered
CACHE_BUF		EQU	1	; buffered not cached
CACHE_MINI		EQU 2	; minicache, writeback
CACHE_WB		EQU	3	; cached write back
		ELSE
; old write through
CACHE_NCNB		EQU	0	; not cached or buffered
CACHE_BUF		EQU	1	; buffered not cached
CACHE_WT_NB		EQU 2	; cached not buffered (?)
CACHE_WT		EQU	3	; cached write through
		ENDIF

	ENDIF

; Macro to define a boot table permission entry
; Entry is defined by:
;	Permissions (3), Cache Attributes (5), Domain (4)
;	P bit (ARMv5, 1)
;	XN (ARMv6, 1)
;	NG (ARMv6, 1)
;	S (ARMv6, 1)
;
BTP_FLAG			EQU	0x80000000

	IF	CFG_ARMV6 :LOR: CFG_ARMV7

BTPERM_EXECUTE		EQU	1
BTPERM_NO_EXECUTE	EQU	0
BTPERM_GLOBAL		EQU	1
BTPERM_LOCAL		EQU	0
BTPERM_SHARED		EQU	1
BTPERM_NON_SHARED	EQU	0
BTPERM_ECC			EQU	1
BTPERM_NON_ECC		EQU	0

	MACRO
	BTP_ENTRY	$domain, $perm, $cache, $execute, $global, $P, $S
	DCD		$domain + ($perm :SHL: 4) + ($cache :SHL: 7) + ($execute :SHL: 12) + ($global :SHL: 13) + ($P :SHL: 14) + ($S :SHL: 15) + BTP_FLAG
	MEND

	ELSE
		IF CFG_TEX

BTPERM_ECC			EQU	1
BTPERM_NON_ECC		EQU	0

	MACRO
	BTP_ENTRY	$domain, $perm, $cache, $P
	DCD		$domain + ($perm :SHL: 4) + ($cache :SHL: 7) + ($P :SHL: 14) + BTP_FLAG
	MEND

		ELSE

	MACRO
	BTP_ENTRY	$domain, $perm, $cache
	DCD		$domain + ($perm :SHL: 4) + ($cache :SHL: 7) + BTP_FLAG
	MEND

		ENDIF
	ENDIF

; Default domain + permissions for uncached mapping
	IF	CFG_MMDirect
CLIENT_DOMAIN	EQU	0					; direct model uses domain 0 for everything
UNC_PERM		EQU	PERM_RWRW			; uncached memory mapping is user-accessible
	ENDIF

	IF	CFG_MMMoving
CLIENT_DOMAIN	EQU	1					; moving model uses domain 1 for general fixed mappings
UNC_PERM		EQU	PERM_RWNO			; and uncached mapping is dummy and not user accessible
	ENDIF

	IF	CFG_MMMultiple
CLIENT_DOMAIN	EQU	0					; multiple model uses domain 0 for general fixed mappings
UNC_PERM		EQU	PERM_RWNO			; and uncached mapping is dummy and not user accessible
	ENDIF

	IF	CFG_MMFlexible
CLIENT_DOMAIN	EQU	0					; flexible model uses domain 0 for general fixed mappings
UNC_PERM		EQU	PERM_RWNO			; and uncached mapping is dummy and not user accessible
	ENDIF

;*******************************************************************************
; Macros for hardware mappings

HW_MULT_4K			EQU	0x000			; size is number of 4K pages
HW_MULT_64K			EQU	0x400			; size is number of 64K pages
HW_MULT_1M			EQU	0x800			; size is number of 1M pages
HW_MULT_MASK		EQU	0xC00
HW_MAP_EXT			EQU	0x200			; extended mapping description
HW_MAP_EXT2			EQU	0x100			; extended mapping description
HW_SIZE_MASK		EQU	0x0FF			; bottom 8 bits give size

; Declare a hardware mapping with standard permissions
	MACRO
	HW_MAPPING	$phys, $size, $mult
	IF	($mult=HW_MULT_4K) :LAND: ($phys :AND: 0x0FFF)<>0
		! 1, "HW physical address not 4K aligned"
	ENDIF
	IF	($mult=HW_MULT_64K) :LAND: ($phys :AND: 0x0FFFF)<>0
		! 1, "HW physical address not 64K aligned"
	ENDIF
	IF	($mult=HW_MULT_1M) :LAND: ($phys :AND: 0x0FFFFF)<>0
		! 1, "HW physical address not 1M aligned"
	ENDIF
	IF	($size>255)
		! 1, "HW mapping maximum 255 pages"
	ENDIF
		DCD	$phys + $size + $mult
	MEND

; Declare a hardware mapping with nonstandard permissions
; Follow this with a BTP_ENTRY macro specifying the required permissions
	MACRO
	HW_MAPPING_EXT	$phys, $size, $mult
	IF	($mult=HW_MULT_4K) :LAND: ($phys :AND: 0x0FFF)<>0
		! 1, "HW physical address not 4K aligned"
	ENDIF
	IF	($mult=HW_MULT_64K) :LAND: ($phys :AND: 0x0FFFF)<>0
		! 1, "HW physical address not 64K aligned"
	ENDIF
	IF	($mult=HW_MULT_1M) :LAND: ($phys :AND: 0x0FFFFF)<>0
		! 1, "HW physical address not 1M aligned"
	ENDIF
	IF	($size>255)
		! 1, "HW mapping maximum 255 pages"
	ENDIF
		DCD	$phys + $size + $mult + HW_MAP_EXT
	MEND

; Declare a hardware mapping with nonstandard linear address
	MACRO
	HW_MAPPING_EXT2	$phys, $size, $mult, $lin
	IF	($mult=HW_MULT_4K) :LAND: ($phys :AND: 0x0FFF)<>0
		! 1, "HW physical address not 4K aligned"
	ENDIF
	IF	($mult=HW_MULT_64K) :LAND: ($phys :AND: 0x0FFFF)<>0
		! 1, "HW physical address not 64K aligned"
	ENDIF
	IF	($mult=HW_MULT_1M) :LAND: ($phys :AND: 0x0FFFFF)<>0
		! 1, "HW physical address not 1M aligned"
	ENDIF
	IF	($mult=HW_MULT_4K) :LAND: ($lin :AND: 0x0FFF)<>0
		! 1, "HW linear address not 4K aligned"
	ENDIF
	IF	($mult=HW_MULT_64K) :LAND: ($lin :AND: 0x0FFFF)<>0
		! 1, "HW linear address not 64K aligned"
	ENDIF
	IF	($mult=HW_MULT_1M) :LAND: ($lin :AND: 0x0FFFFF)<>0
		! 1, "HW linear address not 1M aligned"
	ENDIF
	IF	($size>255)
		! 1, "HW mapping maximum 255 pages"
	ENDIF
		DCD	$phys + $size + $mult + HW_MAP_EXT2
		DCD	$lin
	MEND

; Declare a hardware mapping with nonstandard linear address and permissions
; Follow this with a BTP_ENTRY macro specifying the required permissions
	MACRO
	HW_MAPPING_EXT3	$phys, $size, $mult, $lin
	IF	($mult=HW_MULT_4K) :LAND: ($phys :AND: 0x0FFF)<>0
		! 1, "HW physical address not 4K aligned"
	ENDIF
	IF	($mult=HW_MULT_64K) :LAND: ($phys :AND: 0x0FFFF)<>0
		! 1, "HW physical address not 64K aligned"
	ENDIF
	IF	($mult=HW_MULT_1M) :LAND: ($phys :AND: 0x0FFFFF)<>0
		! 1, "HW physical address not 1M aligned"
	ENDIF
	IF	($mult=HW_MULT_4K) :LAND: ($lin :AND: 0x0FFF)<>0
		! 1, "HW linear address not 4K aligned"
	ENDIF
	IF	($mult=HW_MULT_64K) :LAND: ($lin :AND: 0x0FFFF)<>0
		! 1, "HW linear address not 64K aligned"
	ENDIF
	IF	($mult=HW_MULT_1M) :LAND: ($lin :AND: 0x0FFFFF)<>0
		! 1, "HW linear address not 1M aligned"
	ENDIF
	IF	($size>255)
		! 1, "HW mapping maximum 255 pages"
	ENDIF
		DCD	$phys + $size + $mult + HW_MAP_EXT2 + HW_MAP_EXT
		DCD	$lin
	MEND

;*******************************************************************************
; Macros for ROM

ROM_WIDTH_8			EQU	3				; 8 bit wide
ROM_WIDTH_16		EQU	4				; 16 bit wide
ROM_WIDTH_32		EQU	5				; 32 bit wide

	MACRO
	ROM_PARAMS	$width, $type, $rand_speed, $seq_speed
	DCD	$width + ($type<<8) + ($rand_speed<<16) + ($seq_speed<<24)
	MEND

	MACRO
	ROM_BANK	$physbase, $maxsize, $lin_override, $width, $type, $rand_speed, $seq_speed
	DCD	$physbase
	DCD	$maxsize
	ROM_PARAMS $width, $type, $rand_speed, $seq_speed
	DCD $lin_override
	MEND

;*******************************************************************************
; Macros for RAM

RAM_VERBATIM		EQU	1				; OR base with this to skip testing of RAM

;*******************************************************************************

	IF	NCPU=0
		! 1, "No CPU specified"
	ENDIF
	IF	NCPU>1
		! 2, "More than one CPU specified"
	ENDIF

	IF	CFG_BootLoader :LAND: :LNOT: CFG_MMDirect
		! 3, "Bootloader must use direct model"
	ENDIF
	IF	:LNOT: CFG_MMDirect
		IF	:LNOT:	CFG_MMUPresent
			! 3, "Only direct model permitted without MMU"
		ENDIF
	ENDIF
	IF	CFG_ARMV6 :LOR: CFG_ARMV7
		IF	CFG_MMMoving
			! 4, "Moving model not supported on ARMv6/7"
		ENDIF
		INIT_NUMERIC_CONSTANT	CFG_ARMV6_LARGE_CONFIG_THRESHOLD, 0x02000000
	ELSE
		IF	CFG_MMMultiple
			! 5, "Multiple model not supported on ARMv4/5"
		ENDIF
		IF	CFG_MMFlexible
			! 5, "Flexible model not supported on ARMv4/5"
		ENDIF
	ENDIF
	IF	CFG_MMUPresent
CFG_IncludeRAMAllocator	SETL	{TRUE}
	ENDIF

;*******************************************************************************

; Write buffer
	MACRO
	DRAIN_WB	$reg, $cc
	IF	CFG_WriteBufferPresent
		IF	:DEF: CFG_CPU_XSCALE
			MCR$cc	p15, 0, $reg, c7, c10, 4
			MCR$cc	p15, 0, $reg, c7, c5, 0
		ELSE
			MCR$cc	p15, 0, $reg, c7, c10, 4
		ENDIF
	ENDIF
	MEND

; CP15 barrier
	MACRO
	CPWAIT	$reg, $cc
	IF	:DEF: CFG_CPU_XSCALE
		MRC$cc	p15, 0, $reg, c2, c0, 0
		MOV$cc	$reg, $reg
		SUB$cc	PC, PC, #4
	ENDIF
	MEND

; Cache (entire)

	IF  ((:DEF: CFG_CPU_ARM1136 :LOR: :DEF: CFG_CPU_ARM1176) :LAND: :LNOT: :DEF: CFG_CPU_ARM1136_ERRATUM_411920_FIXED)

	MACRO
	PURGE_ICACHE $reg1, $reg2, $cc
  
		GETCPSR	$reg2
		orr		$reg1, $reg2, #0xC0
		SETCPSR	$reg1				; disable interrupts

		MOV$cc $reg1, #0
		MCR$cc p15, 0, $reg1, c7, c5, 0 ; Invalidate Entire Instruction Cache
		MCR$cc p15, 0, $reg1, c7, c5, 0 ; Invalidate Entire Instruction Cache
		MCR$cc p15, 0, $reg1, c7, c5, 0 ; Invalidate Entire Instruction Cache
		MCR$cc p15, 0, $reg1, c7, c5, 0 ; Invalidate Entire Instruction Cache

		MSR    cpsr_c, $reg2		; reenable interrupts
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
	MEND

	ELSE

	MACRO
	PURGE_ICACHE $reg, $cc
	IF	CFG_CachePresent
		IF	CFG_SplitCache
			IF  :DEF: CFG_CPU_CORTEX_A9 :LAND: (:LNOT: :DEF: CFG_CPU_ARM_A9_ERRATUM_571618_FIXED)
				; ARM Cortex-A9 MPCore erratum 571618 workaround
				; Execute memory barrier before interruptible CP15 operations
				ARM_DMB
			ENDIF
			MCR$cc	p15, 0, $reg, c7, c5, 0
		ELSE
			MCR$cc	p15, 0, $reg, c7, c7, 0
		ENDIF
	ENDIF
	MEND

	ENDIF

	MACRO
	PURGE_DCACHE $reg, $cc
	IF	CFG_CachePresent
		IF	CFG_SplitCache
			MCR$cc	p15, 0, $reg, c7, c6, 0
		ELSE
			MCR$cc	p15, 0, $reg, c7, c7, 0
		ENDIF
	ENDIF
	MEND

	IF :LNOT: CFG_TrustZone_NonSecure
	;Not available in non-secure mode of TrustZone
	MACRO
	PURGE_IDCACHE $reg, $cc
	IF	CFG_CachePresent
		IF	CFG_ARMV7
			! 1, "Instruction removed on ARMv7"
		ELSE
			MCR$cc	p15, 0, $reg, c7, c7, 0
		ENDIF
	ENDIF
	MEND
	ENDIF

	MACRO
	FLUSH_DCACHE $reg, $cc
	IF	CFG_CachePresent
		IF	CFG_WriteBackCache
			BL$cc FlushWriteBackDCache
		ELSE
			PURGE_DCACHE $reg, $cc
		ENDIF
	ENDIF
	MEND

	; 1136 & 1176 bootroms don't use this macro. If they do, 411920 ARM1136 erratum should be fixed here.
	MACRO
	FLUSH_IDCACHE $reg, $cc
	IF	CFG_CachePresent
		IF	CFG_WriteBackCache
			MOV$cc PC, PC
			ADD	PC, PC, #4
			BL FlushWriteBackDCache
			IF  :DEF: CFG_CPU_CORTEX_A9 :LAND: (:LNOT: :DEF: CFG_CPU_ARM_A9_ERRATUM_571618_FIXED)
				; ARM Cortex-A9 MPCore erratum 571618 workaround
				; Execute memory barrier before interruptible CP15 operations
				ARM_DMB
			ENDIF
			MCR p15, 0, $reg, c7, c5, 0
		ELSE
			PURGE_IDCACHE $reg, $cc
		ENDIF
	ENDIF
	MEND


; Cache (line by address)
	IF CFG_ARMV6 :LOR: CFG_ARMV7

	MACRO
	CLEAN_DCACHE_LINE $reg, $cc
			MCR$cc  p15, 0, $reg, c7, c10, 1 
	MEND

	MACRO
	CACHE_LINE_SIZE $reg1, $reg2
		IF CFG_ARMV6
			MOV $reg1, #32 		; This is the same on all ARMv6 platforms
		ELSE
			MRC  p15, 0, $reg2, c0, c0, 1 ; get Cache Type reg.
			;reg2 - bits[19:16] = log2(MinDCacheLine_in_words)		
			AND	$reg2, #0xf0000 ;
			MOV $reg2, $reg2, lsr #16		;
			MOV $reg1, #4
			MOV $reg1, $reg1,lsl $reg2	; reg1 = MinDCacheLine_in_bytes
		ENDIF
	MEND

	ENDIF


; Cache (line by index)

; TLB (entire)
	MACRO
	FLUSH_ITLB $reg, $cc
	IF	CFG_MMUPresent
		IF	CFG_SplitTLB
			MCR$cc	p15, 0, $reg, c8, c5, 0
		ELSE
			MCR$cc	p15, 0, $reg, c8, c7, 0
		ENDIF
	ENDIF
	MEND

	MACRO
	FLUSH_DTLB $reg, $cc
	IF	CFG_MMUPresent
		IF	CFG_SplitTLB
			MCR$cc	p15, 0, $reg, c8, c6, 0
		ELSE
			MCR$cc	p15, 0, $reg, c8, c7, 0
		ENDIF
	ENDIF
	MEND

	MACRO
	FLUSH_IDTLB $reg, $cc
	IF	CFG_MMUPresent
		MCR$cc	p15, 0, $reg, c8, c7, 0
	ENDIF
	MEND

; TLB (entry)

	MACRO
	FLUSH_ITLB_ENTRY $reg, $cc
	IF	CFG_MMUPresent
		IF	CFG_SplitTLB
			IF	CFG_SingleEntryITLBFlush
				MCR$cc p15, 0, $reg, c8, c5, 1
			ELSE
				MCR$cc p15, 0, $reg, c8, c5, 0
			ENDIF
		ELSE
			IF	CFG_SingleEntryTLBFlush
				IF	:DEF: CFG_CPU_ARM11MP
				MCR$cc p15, 0, $reg, c8, c7, 3
				ELSE
				MCR$cc p15, 0, $reg, c8, c7, 1
				ENDIF
			ELSE
				MCR$cc p15, 0, $reg, c8, c7, 0
			ENDIF
		ENDIF
	ENDIF
	MEND

	MACRO
	FLUSH_DTLB_ENTRY $reg, $cc
	IF	CFG_MMUPresent
		IF	CFG_SplitTLB
			MCR$cc p15, 0, $reg, c8, c6, 1
		ELSE
			IF	CFG_SingleEntryTLBFlush
				IF	:DEF: CFG_CPU_ARM11MP
				MCR$cc p15, 0, $reg, c8, c7, 3
				ELSE
				MCR$cc p15, 0, $reg, c8, c7, 1
				ENDIF
			ELSE
				MCR$cc p15, 0, $reg, c8, c7, 0
			ENDIF
		ENDIF
	ENDIF
	MEND

	MACRO
	FLUSH_IDTLB_ENTRY $reg, $cc
	IF	CFG_MMUPresent
		IF	CFG_SplitTLB
			IF	CFG_SingleEntryTLBFlush
				MCR$cc p15, 0, $reg, c8, c7, 1
			ELSE IF	CFG_SingleEntryITLBFlush
				MCR$cc p15, 0, $reg, c8, c6, 1
				MCR$cc p15, 0, $reg, c8, c5, 1
			ELSE
				MCR$cc p15, 0, $reg, c8, c7, 0
			ENDIF
		ELSE
			IF	CFG_SingleEntryTLBFlush
				IF	:DEF: CFG_CPU_ARM11MP
				MCR$cc p15, 0, $reg, c8, c7, 3
				ELSE
				MCR$cc p15, 0, $reg, c8, c7, 1
				ENDIF	
			ELSE
				MCR$cc p15, 0, $reg, c8, c7, 0
			ENDIF
		ENDIF
	ENDIF
	MEND

; BTB
	MACRO
	FLUSH_BTB	$reg, $cc
	IF	CFG_BTBPresent
		MCR$cc p15, 0, $reg, c7, c5, 6
	ENDIF
	MEND

; CAR
	IF	CFG_CARPresent
	MACRO
	GET_CAR	$reg, $cc
	IF	CFG_ARMV6 :LOR: CFG_ARMV7
		MRC$cc	p15, 0, $reg, c1, c0, 2
	ENDIF
	IF	:DEF: CFG_CPU_XSCALE
		MRC$cc	p15, 0, $reg, c15, c1, 0
	ENDIF
	MEND

	MACRO
	SET_CAR	$reg, $cc
	IF	CFG_ARMV6 :LOR: CFG_ARMV7
		MCR$cc	p15, 0, $reg, c1, c0, 2
		ARM_ISB
	ENDIF
	IF	:DEF: CFG_CPU_XSCALE
		MCR$cc	p15, 0, $reg, c15, c1, 0
	ENDIF
	MEND
	ENDIF

; DACR
	IF	CFG_MMUPresent
	MACRO
	GET_DACR	$reg, $cc
	MRC$cc	p15, 0, $reg, c3, c0, 0
	MEND

	MACRO
	SET_DACR	$reg, $cc
	MCR$cc	p15, 0, $reg, c3, c0, 0
	MEND
	ENDIF

; FCSE
	IF	CFG_FCSE_Present
	MACRO
	GET_FCSE $reg, $cc
	MRC$cc	p15, 0, $reg, c13, c0, 0
	MEND

	MACRO
	SET_FCSE $reg, $cc
	MCR$cc	p15, 0, $reg, c13, c0, 0
	MEND
	ENDIF

; ASID
	IF	CFG_ASID_Present
	MACRO
	GET_ASID $reg, $cc
	MRC$cc	p15, 0, $reg, c13, c0, 1
	MEND

	MACRO
	SET_ASID $reg, $cc
	ARM_DSB
	MCR$cc	p15, 0, $reg, c13, c0, 1
	ARM_ISB
	MEND
	ENDIF

; MMUID
	IF	CFG_MMUPresent
	MACRO
	GET_MMUID $reg, $cc
	MRC$cc	p15, 0, $reg, c0, c0, 0
	MEND
	ENDIF

; Cache type
	IF	CFG_CacheTypeReg
	MACRO
	GET_CTR $reg, $cc
	MRC$cc	p15, 0, $reg, c0, c0, 1
	MEND
	ENDIF

; TCM type/TLB type
	IF	CFG_ARMV6 :LOR: CFG_ARMV7
	MACRO
	GET_TCM_TYPE $reg, $cc
	MRC$cc	p15, 0, $reg, c0, c0, 2
	MEND

	MACRO
	GET_TLB_TYPE $reg, $cc
	MRC$cc	p15, 0, $reg, c0, c0, 3
	MEND
	ENDIF

; Address selection
	MACRO
	GET_ADDRESS	$reg, $phys, $lin
	IF	CFG_MMDirect
		LDR		$reg, =$phys
	ELSE
		MRC		p15, 0, $reg, c1, c0, 0
		TST		$reg, #MMUCR_M
		LDREQ	$reg, =$phys
		LDRNE	$reg, =$lin
	ENDIF
	MEND

;CLZ instruction macro
	IF CFG_Cpu_Has_CLZ
	MACRO
	CLZ_M $Rd, $Rm
		DCD	0xe16f0f10 + ($Rd :SHL: 12) + $Rm
	MEND
	ENDIF

; WFI instruction macro
	IF	CFG_Cpu_Has_WFI
	MACRO
	ARM_WFI
	ARM_DSB
	DCD		0xE320F003
	MEND
	ENDIF

; WFE/SEV instruction macros
	IF	CFG_Cpu_Has_WFE_SEV
	MACRO
	ARM_WFE
	ARM_DSB
	DCD		0xE320F002
	MEND

	MACRO
	ARM_SEV
	DCD		0xE320F004
	MEND
	ENDIF

; DMB/DSB/ISB instruction macros
	IF	CFG_ARMV7

	MACRO
	ARM_DSB
	DCD		0xF57FF04F					; DSBSY (full system DSB)
	MEND

	MACRO
	ARM_DMB
	DCD		0xF57FF05F					; DMBSY (full system DMB)
	MEND

	MACRO
	ARM_ISB
	DCD		0xF57FF06F					; ISBSY (full system ISB)
	MEND

	ELSE

	MACRO
	ARM_DSB
	DRAIN_WB	r0						; DSB=drain write buffer CP15 instruction
	MEND

	MACRO
	ARM_DMB								; Use DSB instead of DMB since performance
	ARM_DSB								; not a problem in bootstrap
	MEND

	MACRO
	ARM_ISB
	IF	CFG_ARMV6
	MCR		p15, 0, r0, c7, c5, 4
	ENDIF
	MEND

	ENDIF


;*******************************************************************************
; handy bits and pieces
;*******************************************************************************

	IF	SMP

	IF	CFG_MMDirect
SuperCpuSize	EQU	0x14000				; Space reserved for super page + CPU page(s)
DefaultPTAlloc	EQU	0x10000				; Space reserved for page tables
	ELSE
SuperCpuSize	EQU	0x2000				; Space reserved for super page + CPU page(s)
	ENDIF

	ELSE

	IF	CFG_MMDirect
SuperCpuSize	EQU	0x5000				; Space reserved for super page + CPU page(s)
DefaultPTAlloc	EQU	0x8000				; Space reserved for page tables
	ELSE
SuperCpuSize	EQU	0x2000				; Space reserved for super page + CPU page(s)
	ENDIF

	ENDIF


;*******************************************************************************
; extern declarations
;*******************************************************************************

	IF	:LNOT: :DEF: __BOOTCPU_S__
	IMPORT	InitCpu
	IF	CFG_MMUPresent
		IMPORT	PageTableUpdate
		IMPORT	GetPdeValue
		IMPORT	GetPteValue
		IMPORT	EnableMmu
	ENDIF
	ENDIF

	IF	:LNOT: :DEF: __BOOTUTILS_S__
	IMPORT	WordFill
	IMPORT	WordMove
	IMPORT	BootCall
	IMPORT	GetParameter
	IMPORT	GetMandatoryParameter
	IMPORT	FindParameter
	IMPORT	FindRamBankWidth
	IMPORT	FindRamBankConfig
	IMPORT	ExciseRamArea
	IMPORT	RomPhysicalToLinear
	IMPORT	RomLinearToPhysical
	IMPORT	FindPrimary
	IMPORT	CheckForExtensionRom
	IMPORT	SetupSuperPageRunningFromRAM
	IMPORT	RelocateSuperPage
	IF CFG_MMDirect
		IMPORT	RamPhysicalToLinear
		IMPORT	RamLinearToPhysical
	ENDIF

	IF	CFG_MMUPresent
	IF :LNOT: CFG_MMDirect
		IMPORT	AllocAndMap
	ENDIF
		IMPORT	MapContiguous
		IMPORT	InitMemMapSystem
		IMPORT	SwitchToVirtual
	ENDIF

	IF	CFG_DebugBootRom
		IMPORT	WriteS
		IMPORT	WriteNL
		IMPORT	WriteW
		IMPORT	WriteH
		IMPORT	WriteB
		IMPORT	MemDump
		IMPORT	InitStacks
	ENDIF

	IF	CFG_IncludeRAMAllocator
		IMPORT	HandleAllocRequest
	ELSE
		IMPORT	AllocatorStub
	ENDIF

	IF	CFG_AutoDetectROM
		IMPORT	FindRomBankSize
		IMPORT	CheckROMPresent
	ENDIF

	IMPORT	BasicFaultHandler

	ENDIF

	IF :DEF: __BOOTUTILS_S__ :LOR: :DEF: __BOOTCPU_S__ :LOR: :DEF: __BOOTMAIN_S__
	IMPORT	Fault
	ENDIF

	IF	SMP
	IF	:LNOT: :DEF: __BOOTCPU_S__
	IMPORT	GetAPBootCodePhysAddr
	IMPORT	APBootCodeEntry
	IMPORT	APResetEntry
	IMPORT	HandshakeAP
	ENDIF
	ENDIF

	ENDIF	;	__BOOTCPU_INC__
	END

