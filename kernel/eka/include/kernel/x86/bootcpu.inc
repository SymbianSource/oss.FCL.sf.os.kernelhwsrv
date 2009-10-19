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
;*******************************************************************************; e32\include\kernel\x86\bootcpu.inc
;
; CPU/MMU definitions for bootstrap
;

IFNDEF __BOOTCPU_INC__
__BOOTCPU_INC_	equ 0

INIT_NUMERIC_SYMBOL		MACRO	sym, default
IFNDEF &sym
sym	= default
ENDIF
	ENDM

INIT_NUMERIC_CONSTANT	MACRO	sym, default
IFNDEF &sym
sym	equ	default
ENDIF
	ENDM

; EFLAGS bits
EFLAGS_CF	equ	00000001h	; Carry
EFLAGS_PF	equ	00000004h	; Parity Even
EFLAGS_AF	equ	00000010h	; Auxiliary Carry
EFLAGS_ZF	equ	00000040h	; Zero
EFLAGS_SF	equ	00000080h	; Sign
EFLAGS_TF	equ	00000100h	; Trace
EFLAGS_IF	equ	00000200h	; Interrupt Enable
EFLAGS_DF	equ	00000400h	; Direction
EFLAGS_OF	equ	00000800h	; Overflow
EFLAGS_IOPL	equ	00003000h	; IOPL
EFLAGS_NT	equ	00004000h	; Nested Task
EFLAGS_RF	equ	00010000h	; Resume Flag
EFLAGS_VM	equ	00020000h	; Virtual-8086 mode
EFLAGS_AC	equ	00040000h	; Alignment Check
EFLAGS_VIF	equ	00080000h	; Virtual Interrupt Enable Flag
EFLAGS_VIP	equ	00100000h	; Virtual Interrupt Pending
EFLAGS_ID	equ	00200000h	; CPUID detection


; CR0 control register bits
CR0_PE		equ	00000001h	; Protection Enable
CR0_MP		equ	00000002h	; Monitor Coprocessor
CR0_EM		equ	00000004h	; Emulation
CR0_TS		equ	00000008h	; Task Switched
CR0_ET		equ	00000010h	; Extension Type
CR0_NE		equ	00000020h	; Numeric Error
CR0_WP		equ	00010000h	; Write Protect (when set U=1 W=0 gives RORO rather than RWRO)
CR0_AM		equ	00040000h	; Alignment Mask (set to enable alignment check)
CR0_NW		equ	20000000h	; Cache Write mode
CR0_CD		equ	40000000h	; Cache Disable
CR0_PG		equ	80000000h	; Paging

; CR3 control register bits
CR3_PCD		equ 00000010h	; Page Cache Disable
CR3_PWT		equ	00000008h	; Page Write Through

; CR4 control register bits (Pentium and above only)
CR4_VME		equ 00000001h	; Virtual 8086 Mode Enable
CR4_PVI		equ	00000002h	; Protected-mode Virtual Interrupt
CR4_TSD		equ	00000004h	; Time Stamp Disable
CR4_DE		equ	00000008h	; Debugging Extension
CR4_PSE		equ	00000010h	; Page Size Extension (setting enables use of 4M pages)
CR4_PAE		equ	00000020h	; Physical Address Extension
CR4_MCE		equ	00000040h	; Machine Check Enable
CR4_PGE		equ	00000080h	; Page Global Enable (enables use of global pages)
CR4_PCE		equ	00000100h	; Performance Counter Enable
CR4_OSFXSR	equ	00000200h	; OS Support for FXSAVE and FXRSTOR
CR4_OSXMMEXCPT	equ	00000400h	; OS Support for Unmasked SIMD Floating Point Exceptions


INCLUDE	config.inc

INCLUDE	bootmacr.inc
INCLUDE	bootdefs.inc
INCLUDE kernboot.inc
INCLUDE e32rom.inc
INCLUDE	x86boot.inc

; Check memory model definition
NMM = 0
IFDEF CFG_MMDirect
NMM = NMM + 1
ENDIF
IFDEF CFG_MMMoving
ERR "Moving memory model not supported on x86"
ENDIF
IFDEF CFG_MMMultiple
NMM = NMM + 1
ENDIF
IFDEF CFG_MMFlexible
NMM = NMM + 1
ENDIF

IF	NMM EQ 0
ERR "No memory model specified"
ENDIF
IF	NMM GT 1
ERR "More than one memory model specified"
ENDIF

IFNDEF CFG_MMDirect
INCLUDE	mmboot.inc
ENDIF

INIT_NUMERIC_CONSTANT	CFG_RomSizeAlign, 16	; 64K by default

; Segment definitions
SEL_IDX		equ		0fff8h
SEL_TI		equ		0004h
SEL_RPL		equ		0003h





; MMU page table descriptors
PDE_P		equ		00000001h		; Present
PTE_P		equ		00000001h		; Present
PDE_W		equ		00000002h		; Writeable
PTE_W		equ		00000002h		; Writeable
PDE_U		equ		00000004h		; User accessible
PTE_U		equ		00000004h		; User accessible
PDE_PWT		equ		00000008h		; Write Through
PTE_PWT		equ		00000008h		; Write Through
PDE_PCD		equ		00000010h		; Cache Disable
PTE_PCD		equ		00000010h		; Cache Disable
PDE_A		equ		00000020h		; Accessed
PTE_A		equ		00000020h		; Accessed
PTE_D		equ		00000040h		; Dirty (PTEs only)
PDE_PS		equ		00000080h		; Page Size (PDE only, 1 indicates 4M page)
PDE_G		equ		00000100h		; Global (4M pages only)
PTE_G		equ		00000100h		; Global (4K pages)
PTE_PAT		equ		00000080h		; Page Attribute Table (4K pages)
PDE_AVAIL	equ		00000E00h		; Available to OS
PTE_AVAIL	equ		00000E00h		; Available to OS
PDE_PAT		equ		00001000h		; Page Attribute Table (4M pages)

; bits used by OS and not modified by HW
PTE_SOFT	equ		PTE_P + PTE_W + PTE_U + PTE_PWT + PTE_PCD + PTE_PAT + PTE_G


; Raw MMU permissions
PAGE_RONO	equ		0
PAGE_RWNO	equ		PTE_W
PAGE_RORO	equ		PTE_U
PAGE_RWRW	equ		(PTE_U OR PTE_W)

; Raw Cache Attributes
PAGE_NC		equ		PTE_PCD
PAGE_WT		equ		PTE_PWT
PAGE_WB		equ		0

; Permission types
PERM_RONO	equ		0
PERM_RWNO	equ		1
PERM_RORO	equ		2
PERM_RWRW	equ		3

; Cache attribute types (CD:WT)
CACHE_NC	equ		2
CACHE_WT	equ		1
CACHE_WB	equ		0


; Macro to define a boot table permission entry
; Entry is defined by:
;	Permissions (2), Cache Attributes (2), Global (1)
;
BTP_FLAG			equ	80000000h
BTPERM_GLOBAL		equ	1
BTPERM_LOCAL		equ	0

BTP_ENTRY	MACRO	perm, cache, global
	DD		perm OR (perm SHL 2) OR (cache SHL 4) OR (global SHL 8) OR BTP_FLAG
	ENDM

BTP_ENTRY2	MACRO	pdeperm, pteperm, cache, global
	DD		pteperm OR (pdeperm SHL 2) OR (cache SHL 4) OR (global SHL 8) OR BTP_FLAG
	ENDM

;*******************************************************************************
; Macros for hardware mappings

HW_MULT_4K			equ	0				; only use 4K pages
HW_MULT_4M			equ	80000000h		; use 4M pages if possible
HW_MULT_MASK		equ	0C0000000h
HW_MAP_EXT			equ	20000000h		; extended mapping description
HW_MAP_EXT2			equ	10000000h		; extended mapping description
HW_SIZE_MASK		equ	0fffffffh		; bottom 28 bits give size

; Declare a hardware mapping with standard permissions
HW_MAPPING	MACRO	phys, size, mult
	IF (phys AND 0fffh)
		ERR "HW physical address not 4K aligned"
	ENDIF
	IF (size GT 1048576)
		ERR "HW mapping maximum 1048576 pages"
	ENDIF
	DD	phys
	DD	size OR mult
	ENDM

; Declare a hardware mapping with nonstandard permissions
; Follow this with a BTP_ENTRY macro specifying the required permissions
HW_MAPPING_EXT	MACRO	phys, size, mult
	IF (phys AND 0fffh)
		ERR "HW physical address not 4K aligned"
	ENDIF
	IF (size GT 1048576)
		ERR "HW mapping maximum 1048576 pages"
	ENDIF
	DD	phys
	DD	size OR mult OR HW_MAP_EXT
	ENDM

; Declare a hardware mapping with nonstandard linear address
HW_MAPPING_EXT2	MACRO	phys, size, mult, lin
	IF (phys AND 0fffh)
		ERR "HW physical address not 4K aligned"
	ENDIF
	IF (lin AND 0fffh)
		ERR "HW linear address not 4K aligned"
	ENDIF
	IF (size GT 1048576)
		ERR "HW mapping maximum 1048576 pages"
	ENDIF
	DD	phys
	DD	size OR mult OR HW_MAP_EXT2
	DD	lin
	ENDM

; Declare a hardware mapping with nonstandard linear address and permissions
; Follow this with a BTP_ENTRY macro specifying the required permissions
HW_MAPPING_EXT3	MACRO	phys, size, mult, lin
	IF (phys AND 0fffh)
		ERR "HW physical address not 4K aligned"
	ENDIF
	IF (lin AND 0fffh)
		ERR "HW linear address not 4K aligned"
	ENDIF
	IF (size GT 1048576)
		ERR "HW mapping maximum 1048576 pages"
	ENDIF
	DD	phys
	DD	size OR mult OR HW_MAP_EXT OR HW_MAP_EXT2
	DD	lin
	ENDM


;*******************************************************************************
; Macros for ROM

ROM_WIDTH_8			EQU	3				; 8 bit wide
ROM_WIDTH_16		EQU	4				; 16 bit wide
ROM_WIDTH_32		EQU	5				; 32 bit wide

ROM_PARAMS	MACRO	width, type, rand_speed, seq_speed
	DD			width OR (type SHL 8) OR (rand_speed SHL 16) OR (seq_speed SHL 24)
	ENDM

ROM_BANK	MACRO	physbase, maxsize, lin_override, width, type, rand_speed, seq_speed
	DD			physbase
	DD			maxsize
	ROM_PARAMS	width, type, rand_speed, seq_speed
	DD			lin_override
	ENDM

;*******************************************************************************
; Macros for RAM

RAM_VERBATIM		EQU	1				; OR base with this to skip testing of RAM

;*******************************************************************************

	IFDEF	CFG_BootLoader AND NOT CFG_MMDirect
		ERR "Bootloader must use direct model"
	ENDIF

CFG_IncludeRAMAllocator		equ	1

;*******************************************************************************


; Address selection

GET_ADDRESS	MACRO	reg, phys, lin
	LOCAL	mmu_off
	IFDEF	CFG_MMDirect
	mov		&reg, &phys
	ELSE
	mov		&reg, cr0
	test	&reg, CR0_PG
	mov		&reg, &phys
	jz		mmu_off
	mov		&reg, &lin
	ENDIF
mmu_off:
	ENDM


;*******************************************************************************
; handy bits and pieces
;*******************************************************************************

IFDEF	SMP

IdtOffsetFromSuperPage	equ	TCpuPages_iIdt + KPageSize
SuperCpuSize			equ 6*KPageSize
GdtSizeBytes			equ	KSmpGdtSize*8

ELSE

IdtOffsetFromSuperPage	equ	TCpuPage_iIdt + KPageSize
SuperCpuSize			equ 2*KPageSize
GdtSizeBytes			equ	KGdtSize*8

ENDIF

;*******************************************************************************
; extern declarations
;*******************************************************************************



	GLOBAL	InitCpu			: PROC
	GLOBAL	PageTableUpdate	: PROC
	GLOBAL	GetPdeValue		: PROC
	GLOBAL	GetPteValue		: PROC
	GLOBAL	EnableMmu		: PROC
	GLOBAL	SetupIDT		: PROC
	GLOBAL	SetupGDT		: PROC

	GLOBAL	WordFill		: PROC
	GLOBAL	WordMove		: PROC
	GLOBAL	DoBootCall		: PROC
	GLOBAL	FindParameter			: PROC
	GLOBAL	FindRamBankWidth		: PROC
	GLOBAL	FindRamBankConfig		: PROC
	GLOBAL	ExciseRamArea			: PROC
	GLOBAL	RomPhysicalToLinear		: PROC
	GLOBAL	RomLinearToPhysical		: PROC
	GLOBAL	FindPrimary				: PROC
	GLOBAL	CheckForExtensionRom	: PROC
	GLOBAL	SetupSuperPageRunningFromRAM	: PROC
	GLOBAL	RelocateSuperPage		: PROC

	IFDEF CFG_MMDirect
		GLOBAL	RamPhysicalToLinear	: PROC
		GLOBAL	RamLinearToPhysical	: PROC
	ENDIF

	IFNDEF CFG_MMDirect
		GLOBAL	AllocAndMap			: PROC
	ENDIF
		GLOBAL	MapContiguous		: PROC
		GLOBAL	InitMemMapSystem	: PROC
		GLOBAL	SwitchToVirtual		: PROC

	IFDEF CFG_DebugBootRom
		GLOBAL	WriteS		: PROC
		GLOBAL	WriteNL		: PROC
		GLOBAL	WriteW		: PROC
		GLOBAL	WriteH		: PROC
		GLOBAL	WriteB		: PROC
		GLOBAL	DoMemDump	: PROC
	ENDIF

	GLOBAL	HandleAllocRequest		: PROC

;	IFDEF CFG_AutoDetectROM
;		GLOBAL	FindRomBankSize		: PROC
;		GLOBAL	CheckROMPresent		: PROC
;	ENDIF

	GLOBAL	BasicFaultHandler		: PROC
	GLOBAL	DoFault	: PROC

	GLOBAL	TheRomHeader			: PROC
	GLOBAL	RestartEntry			: PROC
	GLOBAL	ResetEntry				: PROC
	GLOBAL	InitialiseHardware		: PROC
	GLOBAL	InitialiseHardwareReturn	: PROC
	GLOBAL	VectorCommon			: PROC
	GLOBAL	GetRomHeaderAddress		: PROC

ENDIF	;	__BOOTCPU_INC__

