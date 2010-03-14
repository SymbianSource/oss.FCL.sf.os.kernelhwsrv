; Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
; e32\kernel\arm\bootmain.s
; Bootstrap main program
;

		GBLL	__BOOTMAIN_S__

		INCLUDE	bootcpu.inc

;
;*******************************************************************************
;

        AREA |Boot$$Code|, CODE, READONLY, ALIGN=6

;
;*******************************************************************************
;

	EXPORT	TheRomHeader
TheRomHeader							; this label represents the base of the ROM header

	IF	CFG_CustomVectors
		INCLUDE	custom_vectors.inc
	ELSE

; Reset vector
		B	ResetEntry

	IF	CFG_MMDirect :LAND: (:LNOT: CFG_MMUPresent :LOR: CFG_UseBootstrapVectors)
		B	. + KernelCodeOffset		; undef
		B	. + KernelCodeOffset		; swi
		B	. + KernelCodeOffset		; prefetch
		B	. + KernelCodeOffset		; data
		B	. + KernelCodeOffset		;
		B	. + KernelCodeOffset		; irq
		B	. + KernelCodeOffset		; fiq

	ELSE
		IF	CFG_DebugBootRom
	IMPORT	HandleUndef
	IMPORT	HandleSwi
	IMPORT	HandleAbtPrefetch
	IMPORT	HandleAbtData
	IMPORT	HandleRsvdVector
	IMPORT	HandleIrq
	IMPORT	HandleFiq

		B	HandleUndef
		B	HandleSwi
		B	HandleAbtPrefetch
		B	HandleAbtData
		B	HandleRsvdVector
		B	HandleIrq
		B	HandleFiq

		ELSE

		FAULT
		FAULT
		FAULT
		FAULT
		FAULT
		FAULT
		FAULT

		ENDIF
	ENDIF
	ENDIF

RestartPadBegin
		%	124 - (RestartPadBegin - TheRomHeader)

; Restart vector - called to reboot under software control
	IMPORT	RestartEntry
		B	RestartEntry

; Leave additional space for the rest of the ROM header

		%	TRomHeader_sz - 128

;***************************************************************************************
; MAIN ENTRY POINT
;	Called on hardware reset
;***************************************************************************************
	EXPORT	ResetEntry
ResetEntry

		GETCPSR r3						; cpsr (r3) & r2 MUST be preserved for
										; entry into InitialiseHardware() to 
										; allow pre-OS loaders to pass info to 
										; bootstrap PSL

	IF :DEF: CFG_BootedInSecureState
	IMPORT	SwitchToNonSecureState
		BL		SwitchToNonSecureState	; corrupts r0,r1
	ENDIF

		MOV		r0, r3
		BIC		r0, r0, #0x1f
		ORR		r0, r0, #0xd3			; 32-bit SVC mode, no interrupts

	IF	CFG_ARMV6 :LOR: CFG_ARMV7
		ORR		r0, r0, #0x100			; on V6 disable imprecise aborts
	ENDIF

		SETCPSR	r0

		ADR		r12, TheRomHeader
;
; The following call should, at a minimum
;	1.	Determine the hardware configuration
;	2.	Determine the reset reason. If it is wakeup from a low power mode, perform
;		whatever reentry sequence is required and jump back to the kernel.
;	3.	Set up the memory controller so that at least some RAM is available
;	4.	Set R10 to point to the super page or to a temporary version of the super page
;		with at least the following fields valid:
;			iBootTable, iCodeBase, iActiveVariant, iCpuId
;		and optionally:
;			iSmrData
;	5.	In debug builds initialise the debug serial port
;
; Enter with:
;		R2	= value as at entry to ResetEntry, preserved unmodified
;		R3	= value of cpsr on entry to ResetEntry
;		R12 = points to TRomHeader
;		NO STACK
;		R14 = return address (as usual)
;
; Leave with :
;		R10 = physical address of super page
;
;	All registers may be modified by this call
;
	IMPORT	InitialiseHardware

		BL		InitialiseHardware						; r0..r14 modified
		ADR		r12, TheRomHeader
		
		ADD		sp, r10, #CpuBootStackTop				; set up a boot stack
		LDR		r0, [r12, #TRomHeader_iDebugPort]
		STR		r0, [r10, #SSuperPageBase_iDebugPort]	; set the debug port in the super page
		DEBUG_INIT_STACKS								; r0..r2 modified (if)

		PRTLN	"InitialiseHardware use of optional SuperPage fields:"
	IF	:DEF: CFG_ENABLE_SMR_SUPPORT					; When defined the bootstrap PSL in InitialiseHardware()
		LDR		r0, [r10, #SSuperPageBase_iSmrData] 	; must set this field to a valid address of the SMRIB or 
														; KSuperPageAddressFieldUndefined if no SMRIB found.
		DWORD	r0, "  SMR_SUPPORT Enabled - iSmrData"	
	ELSE
		MVN		r0, #0									; Set iSmrData field to KSuperPageAddressFieldUndefined (-1)
		STR		r0, [r10, #SSuperPageBase_iSmrData] 	; when SMR feature not supported in ROM image. This is    
		DWORD	r0, "  SMR_SUPPORT Disabled - iSmrData" ; done to ensure kernel-side code can tell if SMRIB exists.	       
	ENDIF

	IF	CFG_BootLoader :LOR: :DEF: CFG_CopyRomToAddress
; Copy the code to RAM
		PRTLN	"Copy code to RAM"
		LDR		r0, [r10, #SSuperPageBase_iCodeBase]	; source

	; Two methods of obtaining the destination physical address: either the
	; config.inc can define CFG_CopyRomToAddress as a numeric constant or it
	; will use the romlinearbase for bootloaders.
	IF	:DEF: CFG_CopyRomToAddress
		LDR		r1, =CFG_CopyRomToAddress				; destination defined in config.inc (must be physical address)
	ELSE
		LDR		r1, [r12, #TRomHeader_iRomBase]			; destination defined in header.iby (must be physical address)
	ENDIF
	IF	:DEF: CFG_SupportEmulatedRomPaging
		LDR		r2, [r12, #TRomHeader_iUncompressedSize]; size of rom
	ELSE
		LDR		r2, [r12, #TRomHeader_iPageableRomStart]; size of unpaged part of ROM
		CMP		r2, #0
		LDREQ	r2, [r12, #TRomHeader_iUncompressedSize]; size if not a pageable rom
	ENDIF
		ADD		r2, r2, #0x3
		BIC		r2, r2, #0x3							; make 4byte aligned
		
		SUB		r4, r1, r0								; save offset destination - source
		DWORD	r0, "Source"
		DWORD	r1, "Dest"
		DWORD	r2, "Size"
		DWORD	r4, "Offset"
		ADR		lr, ReturnHereAfterCopy
		ADD		lr, lr, r4								; lr = return address in RAM
		B		WordMove								; copy the code, return to RAM copy
		LTORG		; Required when CFG_DebugBootRom is enabled
		; WordMove returns to here in RAM
ReturnHereAfterCopy
		LDR		r0, [r10, #SSuperPageBase_iCodeBase]	;
		ADD		r0, r0, r4								;
		STR		r0, [r10, #SSuperPageBase_iCodeBase]	; fix up iCodeBase
		LDR		r1, [r10, #SSuperPageBase_iBootTable]	;
		ADD		r1, r1, r4								;
		STR		r1, [r10, #SSuperPageBase_iBootTable]	; fix up iBootTable
		ADD		r12, r12, r4							; fix up R12 (points to TRomHeader)
		DWORD	r0, "New iCodeBase"
		DWORD	r1, "New iBootTable"
		DWORD	r12, "R12"
		DWORD	pc, "PC"
	ENDIF

		LDR		r0, [r12, #TRomHeader_iRomRootDirectoryList]
		STR		r0, [r10, #SSuperPageBase_iRootDirList]
		LDR		r7, [r10, #SSuperPageBase_iBootFlags]	; get boot flags
		BIC		r7, r7, #0xff000000
		STR		r7, [r10, #SSuperPageBase_iBootFlags]	; clear top 8 bits
		STR		r12, [r10, #SSuperPageBase_iRomHeaderPhys]

; Live registers here are R10, R12, R13
; Determine the machine RAM configuration

		PRTLN	"Determine RAM config"
		ADD		r11, r10, #CpuRamTableOffset
		STR		r11, [r10, #SSuperPageBase_iRamBootData]
		ADD		r0, r10, #CpuRomTableOffset
		STR		r0, [r10, #SSuperPageBase_iRomBootData]
		
		MOV		r1, #CpuRamTableTop-CpuRomTableOffset
		MOV		r2, #0
		BL		WordFill					; zero-fill RAM boot data
		BOOTCALL	BTF_RamBanks			; r0 points to list of possible RAM banks
											; in increasing physical address order
		MOV		r9, r0						; into r9

TestRamBank1
		LDR		r1, [r9], #4				; r1 = physical address of bank
		LDR		r8, [r9], #4				; r8 = size of bank
		DWORD	r1, "TRB Base"
		DWORD	r8, "TRB Size"
		CMP		r8, #0						; reached end?
		BEQ		TestRamBank_End				; branch if end
		TST		r1, #RAM_VERBATIM			; if flag set, accept bank verbatim
		BIC		r2, r1, #RAM_VERBATIM		; clear flag
		MOVNE	r0, r11
		STRNE	r2, [r0], #4				; store base
		STRNE	r8, [r0], #4				; store size
		BNE		TestRamBank3				; do PC check and then next bank
		MVN		r3, #0
		BOOTCALL	BTF_SetupRamBank		; set width to 32
		BL		FindRamBankWidth			; test for presence of RAM
		DWORD	r3, "TRB ByteLane"
		CMP		r3, #0						;
		BEQ		TestRamBank1				; if no RAM, try next
		BOOTCALL	BTF_SetupRamBank		; set width
TestRamBank3
		MOV		r0, pc						; check if PC is in RAM
		SUBS	r0, r0, r1					; PC - base
		CMPHS	r8, r0						; if PC>=base, check size>(PC-base)
		LDRHI	r7, [r10, #SSuperPageBase_iBootFlags]		; 
		ORRHI	r7, r7, #KBootFlagRunFromRAM				; if so, we are running from RAM
		STRHI	r7, [r10, #SSuperPageBase_iBootFlags]		; so set boot flag
		TST		r1, #RAM_VERBATIM			; if flag set, accept bank verbatim
		ADDNE	r0, r11, #8
		BNE		TestRamBank4
		MOV		r2, r8						; r2 = max bank size
		MOV		r0, r11						; r0 = allocator data ptr
		BL		FindRamBankConfig			; find blocks
TestRamBank4
	IF	CFG_DebugBootRom
TestRamBank2
		LDR		r1, [r11], #4
		DWORD	r1, "Block base"
		LDR		r1, [r11], #4
		DWORD	r1, "Block size"
		CMP		r11, r0
		BLO		TestRamBank2
	ENDIF
		MOV		r11, r0						; save allocator data ptr
		B		TestRamBank1				; next bank

TestRamBank_End


; Determine the machine ROM configuration

		LDR		r11, [r10, #SSuperPageBase_iRomBootData]
		LDR		r0, [r10, #SSuperPageBase_iBootFlags]
		TST		r0, #KBootFlagRunFromRAM
		BEQ		TestRomBanks				; branch if running from ROM

; ROM is actually in RAM

		PRTLN	"Running from RAM"
	IF	:DEF: CFG_SupportEmulatedRomPaging
		LDR		r8, [r12, #TRomHeader_iUncompressedSize]; r8=size of ROM
	ELSE
		LDR		r8, [r12, #TRomHeader_iPageableRomStart]; r8=size of unpaged part of ROM
		CMP		r8, #0
		LDREQ	r8, [r12, #TRomHeader_iUncompressedSize]; r8=size of ROM if not a pageable rom
	ENDIF
		MVN		r2, #0									; round this up to 64k
		ADD		r8, r8, r2, lsr #(32 - CFG_RomSizeAlign)
		BIC		r8, r8, r2, lsr #(32 - CFG_RomSizeAlign)
		LDR		r9, [r10, #SSuperPageBase_iRamBootData]
		LDR		r0, [r10, #SSuperPageBase_iCodeBase]	; running from RAM - ROM base = code base
		LDR		r2, [r12, #TRomHeader_iRomBase]			; r2=linear base of ROM block
		MOV		r1, r8									; size of area to excise
		BL		ExciseRamArea							; remove RAM which holds image from free list
		LDR		r0, [r9, #SRamBank_iSize]
		CMP		r0, #0
		BEQ		RomInRam1								; reached end of RAM list - no extension ROM
		LDR		r0, [r9, #SRamBank_iBase]				; r0 = next RAM address after kernel ROM image
		BL		CheckForExtensionRom					; check for extension ROM
		BEQ		RomInRam1								; skip if not present
		LDR		r1, [r0, #TExtensionRomHeader_iUncompressedSize] ; else r1=extension ROM size
		MVN		r2, #0									; round this up to 64k
		ADD		r1, r1, r2, lsr #(32 - CFG_RomSizeAlign)
		BIC		r1, r1, r2, lsr #(32 - CFG_RomSizeAlign)
		ADD		r8, r8, r1								; accumulate ROM size
		LDR		r2, [r0, #TExtensionRomHeader_iRomRootDirectoryList]
		STR		r2, [r10, #SSuperPageBase_iRootDirList]		; and update root dir ptr
		LDR		r2, [r0, #TExtensionRomHeader_iRomBase]		; r2=linear base of extension ROM block
		BL		ExciseRamArea							; remove RAM which holds extension ROM image from free list
RomInRam1
		B		DoneRomBanks				; finished with ROM

; ROM is actually in ROM or FLASH
TestRomBanks
		PRTLN	"Running from ROM/FLASH"
		BOOTCALL	BTF_RomBanks			; r0 points to list of possible ROM banks
											; in increasing physical address order
		MOV		r9, r0						; R9 -> bank list
		MOV		r11, #0
		BOOTCALL	BTF_SetupRomBank		; do any setup prior to detection
		LDR		r11, [r10, #SSuperPageBase_iRomBootData]

; Walk through list of possible banks
; Call a setup function for each one
; This should set up width/wait states if not set by earlier initialisation.
; It may also do detection of optional banks if required.
TestRomBank1
		LDMIA	r9!, {r1-r4}
		STMIA	r11, {r1-r4}
		DWORD	r1, "TROMB Base"
		DWORD	r2, "TROMB Size"
		DWORD	r4, "TROMB LinB"
		CMP		r2, #0						; end of list?
		BEQ		CalcRomLinBases				; branch if so
		BOOTCALL	BTF_SetupRomBank		; do detection if needed (for optional ROMs)

		LDMIA	r11, {r1-r4}
	IF	CFG_DebugBootRom
		DWORD	r1, "TROMB PostDet Base"
		DWORD	r2, "TROMB PostDet Size"
		DWORD	r4, "TROMB PostDet LinB"
	ENDIF

		CMP		r2, #0
		ADDNE	r11, r11, #SRomBank_sz		; if bank present, increment r11
		B		TestRomBank1

CalcRomLinBases
		LDR		r11, [r10, #SSuperPageBase_iRomBootData]
		MOV		r8, #0						; total ROM size
CalcRomLinBases1
		LDMIA	r11!, {r0,r2-r4}			; r0=phys base, r2=size, r4=lin base
		DWORD	r0, "CRLB Base"
		DWORD	r2, "CRLB Size"
		DWORD	r4, "CRLB LinB"
		CMP		r2, #0						; reached end?
		BEQ		CalcRomLinBases_End
		ADD		r8, r8, r2					; accumulate size
	IF	CFG_MMUPresent
		CMP		r4, #0						; lin addr override?
		MOVNE	r9, r4						; if so, take it
	ELSE
		MOV		r9, r0						; if no MMU, lin = phys
	ENDIF
		SUBS	r1, r12, r0					; check if this is boot block - R1=rom header phys-base
		CMPHS	r2, r1						; if rom hdr phys>=base, compare size with rom header phys-base
	IF	CFG_MMUPresent
		LDRHI	r9, [r12, #TRomHeader_iRomBase]	; linear base of boot ROM (= lin addr of rom header)
		SUBHI	r9, r9, r1					; calculate linear address corresponding to base of ROM block
	ENDIF
		BHI		CalcRomLinBases2			; skip if boot block
		BL		CheckForExtensionRom		; else check if this is extension ROM
		BEQ		CalcRomLinBases2			; skip if not
	IF	CFG_MMUPresent
		LDR		r9, [r0, #TExtensionRomHeader_iRomBase]	; if it is, take linear address override
	ENDIF
		LDR		r1, [r0, #TExtensionRomHeader_iRomRootDirectoryList]
		STR		r1, [r10, #SSuperPageBase_iRootDirList]		; and update root dir ptr
CalcRomLinBases2
		DWORD	r9, "CRLB Final Linear Base"
		STR		r9, [r11, #-4]				; write final linear base
	IF	CFG_MMUPresent
		ADD		r9, r9, r2					; and step on by size
	ENDIF
		B		CalcRomLinBases1			; do next bank
CalcRomLinBases_End

DoneRomBanks
		STR		r8, [r10, #SSuperPageBase_iTotalRomSize]	; save total ROM size

; Support for areas?
; Put it here if needed.

; RAM reservation/pre-allocation section
;
; SSuperPageBase_iRamBootData member points to two arrays/lists that are
; used to initialise the kernel RAM Allocator. On entry to this section the
; list contains the free RAM regions and no pre-allocations:
;	<free ram region><...><null entry><null entry>
;
; On exit from this section pre-allocated entries may have been added:
; 	<free ram region><><null entry><pre-allocated ram region><><null entry>
;
; - Setup R11 to point to free entry after first null entry in iRamBootData list
; - Reserve any SMRs in the SP SMRIB by adding them to the pre-allocation list
; - Call the bootstrap PSL to allow further platform-specific RAM reservation
;

; Point R11 to preallocated block list
		LDR		r11, [r10, #SSuperPageBase_iRamBootData]
3
		LDMIA	r11!, {r0,r1}
		CMP		r1, #0
		BNE		%BT3

; At this point in bootstrap:
; r10 = pointer to super page (SP)
; r11 = address of first pre-alloc SRamBank after first terminator entry
; r12 = pointer to ROM header
; r13 = stack pointer

; Reserve SMR memory blocks, if SMRIB valid in super page

		MOV		r6, #0						; Setup r6 to count SMRIB entries for trace
		LDR		r5, [r10, #SSuperPageBase_iSmrData]
		CMN	 	r5, #1						; Compare to KSuperPageAddressFieldUndefined	
		BCS	 	NoSMRIB						; jump over if no SMRIB
		PRTLN	"Processing (any) SMRIB..."
4
		LDMIA	r5, {r0, r1}				; load iBase and iSize members		
		ADD	 	r5, r5, #SSmrBank_sz		; Skip the two other members, inc R5 to next entry
		CMP	 	r1, #0
		BEQ	 	DoneSMRReservation			; Jump out of loop if no more SMRs
		
		MVN		r3, #0
		ANDS	r2, r1, r3, LSR #20
		FAULT	NE							; Fault if SMR Size is not multiple of 4Kb
 
		STMIA	r11!, {r0, r1}				; Add SMR to pre-alloc list
		ADD		r6, r6, #1
		B		%BT4
											; No need to add null entry as the 
DoneSMRReservation							; SRamBank area has been zero filled   

		CMP		r6, #0
		BNE		%FT5
		PRTLN	"PSL created SMRIB of zero size!"
		B		NoSMRIB		
5
		DWORD	r6, "PSL created SMRIB of X entries"
	IF	CFG_DebugBootRom
		PRTLN	"SMRIB Memory Dump:"
		LDR		r5, [r10, #SSuperPageBase_iSmrData]
		MOV		r6, r6, LSL #4 
		MEMDUMP	r5, r6
	ENDIF
	
NoSMRIB
	
; Here r11 points to the null terminator of the preallocated block list

; Reserve any platform-dependent extra physical memory here.
; Two methods are available:
;	1.	ExciseRamArea - the kernel will then not treat the region as RAM
;	2.	Add preallocation regions to the RAM bank list. R11 has been set
;		to point to the first of these. List is terminated by a zero size.
;		The kernel will treat these regions as RAM.
;
		BOOTCALL	BTF_Reserve

	IF	CFG_DebugBootRom
; In debug, dump ROM and RAM bank config
		PRTLN	"ROM and RAM config:"
		LDR		r11, [r10, #SSuperPageBase_iRomBootData]
		MOV		r1, #CpuRamTableTop-CpuRomTableOffset
		MEMDUMP	r11, r1
	ENDIF

; Calculate total RAM size
		LDR		r11, [r10, #SSuperPageBase_iRamBootData]
		MOV		r9, #0
AccumulateRamSize
		LDMIA	r11!, {r0,r1}
		ADD		r9, r9, r1
		CMP		r1, #0
		BNE		AccumulateRamSize
		DWORD	r9, "TotalRamSize"
		STR		r9, [r10, #SSuperPageBase_iTotalRamSize]

; RAM reservation/pre-allocation section end
;

; find the kernel image and point R11 to it
		PRTLN	"Find primary"
		BL		FindPrimary
		STR		r11, [sp, #-4]!

	IF	CFG_MMDirect
		; direct model - work out the end of the kernel memory area
		LDR		r8, [r12, #TRomHeader_iKernelLimit]	; end of kernel heap, rounded up to 4K
	IF	CFG_MMUPresent
		GETPARAM	BPR_PageTableSpace, DefaultPTAlloc	; get reserved space for page tables
		ADD		r8, r8, r0						; add space for page tables
	IF	:LNOT: :DEF: CFG_MinimiseKernelHeap
		MVN		r1, #0
		ADD		r8, r8, r1, LSR #12
		BIC		r8, r8, r1, LSR #12				; round up to 1Mb to give limit
	ENDIF
	ENDIF
		STR		r8, [r10, #SSuperPageBase_iKernelLimit]
		DWORD	r8, "Kernel Limit"

		GETPARAM	BPR_KernDataOffset, SuperCpuSize	; get kernel data offset into R0 with default
		LDR		r2, [r12, #TRomHeader_iKernDataAddress]	;
		SUB		r0, r2, r0								; r0 = super page linear address
		DWORD	r0, "SuperPageLin"

	IF	CFG_BootLoader

; for bootloader calculate linear RAM base as:
;	1. TRomHeader::iKernDataAddress - KernDataOffset if BootLdr_ImgAddr > physical RAM base
;	2. TRomHeader::iKernDataAddress - KernDataOffset - X if BootLdr_ImgAddr = physical RAM base
; where X = amount of physical RAM preceding the super page in physical address order
; physical address of super page is chosen to allow BootLdr_ExtraRAM (usually 1Mb) of 'user' RAM

		MOV		r2, r0									; r2 = super page lin
		GETMPARAM	BPR_BootLdrImgAddr					; R0 = image physical address
		LDR		r8, [r10, #SSuperPageBase_iRamBootData]
		LDR		r3, [r8, #0]							; r3 = physical RAM base
		CMP		r0, r3
		MOV		r0, r2									;
		BHI		SuperPageAtBeginning					; if boot image not at RAM base, put super page there
		LDR		r1, [r12, #TRomHeader_iUserDataAddress]	; 'user' linear address
		MOV		r2, r0									; r2 = super page lin
		GETPARAM	BPR_BootLdrExtraRAM, 0x100000		; get extra RAM size into R0, default to 1M
		ADD		r1, r1, r0								; linear top of RAM
		SUB		r1, r1, r2								; r1 = total amount of RAM reserved at top
		DWORD	r1, "ReserveAtTop"
		LDR		r0, [r10, #SSuperPageBase_iTotalRamSize]	; r0 = total physical RAM size
		SUB		r0, r0, r1								; r2 = physical RAM below super page
		DWORD	r0, "RamBelowSuperPage"
		SUB		r0, r2, r0								; r0 = linear RAM base

	ENDIF	; CFG_BootLoader
; for non-bootloader, iRamBase = TRomHeader::iKernDataAddress - KernDataOffset
SuperPageAtBeginning
		STR		r0, [r10, #SSuperPageBase_iRamBase]
		DWORD	r0, "iRamBase"

	ENDIF

; initialise the RAM allocator
		PRTLN	"Init RAM allocator"
		MOV		r2, #BMA_Init
		BOOTCALL	BTF_Alloc

; get the final super page physical address
		MOV		r2, #BMA_SuperCPU
		BOOTCALL	BTF_Alloc
		DWORD	r0, "Final SuperPage Phys"

; if super page has moved, relocate it
		CMP		r0, r10
		BLNE	RelocateSuperPage
		LDR		r11, [sp], #4					; recover kernel image pointer

	IF	CFG_MMUPresent

; initialise the memory mapping system
		PRTLN	"InitMemMapSystem"
		BL		InitMemMapSystem

; map the ROM
		PRTLN	"Map ROM"
		LDR		r9, [r10, #SSuperPageBase_iRomBootData]

MapRomBlock
		LDR		r0, [r9, #SRomBank_iLinBase]	; r0 = linear base
		LDR		r1, [r9, #SRomBank_iBase]		; r1 = physical base
		LDR		r3, [r9, #SRomBank_iSize]		; r3 = size
		MOV		r2, #BTP_Rom					; r2 = access permissions
		MOV		r4, #20							; r4 = log2(max page size)
		CMP		r3, #0
		BEQ		MapRomDone						; branch out if reached end of list
		BL		MapContiguous
		ADD		r9, r9, #SRomBank_sz
		B		MapRomBlock
MapRomDone


	IF	CFG_MMDirect

; direct model - map all RAM in one block, starting at SSuperPageBase::iRamBase
; permissions are kernel up to the end of the kernel heap, user after
; also map non-kernel RAM at its physical address, uncached
		PRTLN	"Direct Map RAM"
		LDR		r8, [r10, #SSuperPageBase_iKernelLimit]	; top of kernel area (linear)
		LDR		r9, [r10, #SSuperPageBase_iRamBootData]
		MOV		r2, #BTP_Kernel					; start with kernel access permissions
		MOV		r4, #20
		LDR		r0, [r10, #SSuperPageBase_iRamBase]		; linear address
		DWORD	r0, "Direct Map RAM Base"
MapRamBlock
		LDMIA	r9!, {r1,r5}					; r1 = bank physical base, r5 = bank size
		CMP		r5, #0							; reached end of list?
		BEQ		MapRamBlock_End					; jump if end
		DWORD	r1, "Bank base"
		DWORD	r5, "Bank size"
		SUBS	r3, r8, r0						; r3 = kernel limit - base
		MOVLO	r3, r5							; if base>=kernel limit, size to map = bank size
		BLO		MapUserRamBlock					; if base>=kernel limit, map user
		CMP		r3, r5							; kernel area >= size?
		MOVHS	r3, r5							; if so, size to map = bank size
		BHS		MapKernelRamBlock				; and map kernel block
		BL		MapContiguous					; else map size r3 as kernel
		ADD		r0, r0, r3						; increment linear address
		ADD		r1, r1, r3						; increment physical address
		SUB		r3, r5, r3						; remaining size of bank
MapUserRamBlock
		MOV		r2, #BTP_User					; change to user permissions
		BL		MapContiguous					; Map bank
		ADD		r0, r0, r3						; increment linear address
		STMFD	sp!, {r0}						; save linear address
		MOV		r0, r1							; linear = physical
		MOV		r2, #BTP_Uncached				; map as uncached user
		BL		MapContiguous
		LDMFD	sp!, {r0}						; recover linear address
		B		MapRamBlock
MapKernelRamBlock
		BL		MapContiguous					; Map bank
		ADD		r0, r0, r3						; increment linear address
		B		MapRamBlock
MapRamBlock_End

	ELSE

; moving, multiple or flexible model

; map super page + CPU page
		PRTLN	"Map super/CPU pages"
		MOV		r0, #KSuperPageLinAddr			; linear
		MOV		r1, r10							; physical
		MOV		r2, #BTP_SuperCPU				; permissions
		MOV		r3, #SuperCpuSize				; size
		MOV		r4, #12							; map size
		BL		MapContiguous

; allocate one page for page table info and map it
		PRTLN	"Map PTINFO"
		LDR		r0, =KPageTableInfoBase			; linear
		MOV		r2, #BTP_PtInfo					; permissions
		MOV		r3, #0x1000						; size
		MOV		r4, #12							; map size
		BL		AllocAndMap

	ENDIF	; CFG_MMDirect

	IF	CFG_MMMultiple

; on multiple model, map ASID info area
		PRTLN	"Map ASID info"
		LDR		r0, =KAsidInfoBase				; linear
		MOV		r2, #BTP_PtInfo					; permissions
		MOV		r3, #0x1000						; size
		MOV		r4, #12							; map size
		BL		AllocAndMap

	ENDIF

	IF	CFG_MMFlexible

; on flexible model, map page array segment area
		PRTLN	"Map PageArrayGroup"
		LDR		r0, =KPageArraySegmentBase		; linear
		MOV		r2, #BTP_Kernel					; permissions
		MOV		r3, #0x1000						; size
		MOV		r4, #12							; map size
		BL		AllocAndMap

	ENDIF

; map hardware
		PRTLN	"Map HW"
		BOOTCALL	BTF_HwBanks					; get pointer to list of HW banks into r0
		MOV		r9, r0							; into r9
	IF	:LNOT: CFG_MMDirect
		MOV		r0, #KPrimaryIOBase				; linear address for HW
	ENDIF

	IF :DEF: CFG_HasL210Cache :LOR: :DEF: CFG_HasL220Cache :LOR: :DEF: CFG_HasPL310Cache
		LDR		r5, [r10, #SSuperPageBase_iArmL2CacheBase] ; r5 = PhysAddr of External Cache Controller.
		MOV		r7, #1	; R7 != 0 => LinAddr of ExtCacheCtrl is not found yet. Set R7 to 0 when found. 
	ENDIF
MapHwBank
		LDR		r1, [r9], #4					; get phys addr / size
		DWORD	r1, "HwBank Entry"
		MVN		r14, #0
		ANDS	r3, r1, #HW_SIZE_MASK			; r3 = bottom 8 bits = #pages
		BEQ		MapHwBank_End					; jump out if end of list (number of pages = 0)
		MOV		r3, r3, LSL #12					; pages -> bytes
		AND		r4, r1, #HW_MULT_MASK
		CMP		r4, #HW_MULT_64K
		MOVEQ	r3, r3, LSL #4					; if MULT_64K, multiply page count by 16
		MOVEQ	r4, #16							; and use 64K pages
		MOVHI	r3, r3, LSL #8					; if MULT_1M, multiply page count by 256
		MOVHI	r4, #20							; and use 1M pages
		MOVLO	r4, #12							; otherwise use 4K pages
		TST		r1, #HW_MAP_EXT2				; linear address specified?
		LDRNE	r6, [r9], #4					; if so get it from next descriptor word
		TST		r1, #HW_MAP_EXT					; extended mapping?
		LDRNE	r2, [r9], #4					; if so, get permissions from next descriptor word
		MOVEQ	r2, #BTP_Hw						; else use standard HW permissions
		TST		r1, #HW_MAP_EXT2				; linear address specified?
		AND		r1, r1, r14, LSL #12			; r1 = top 20 bits = physical address
		BNE		MapHwBank2						; branch if linear address specified
	IF	CFG_MMDirect
		MOV		r0, r1							; linear = physical
	ELSE
		MVN		r6, r14, LSL r4					; r6 = 2^r4-1
		ADD		r0, r0, r6
		BIC		r0, r0, r6						; round up linear address
	ENDIF
		BL		MapContiguous					; make mapping
	IF :DEF: CFG_HasL210Cache :LOR: :DEF: CFG_HasL220Cache :LOR: :DEF: CFG_HasPL310Cache
		MOV		r4, r0 ; r4 = LinAddr of the current HwBank
	ENDIF
		ADD		r0, r0, r3						; increment linear address
	IF :DEF: CFG_HasL210Cache :LOR: :DEF: CFG_HasL220Cache :LOR: :DEF: CFG_HasPL310Cache
		B		MapHwBank3						; test whether the current HwBank contains ExtCacheCtrl 
	ELSE
		B		MapHwBank						; next bank
	ENDIF
MapHwBank2
		STR		r0, [sp, #-4]!					; save default linear address
		MOV		r0, r6							; r0 = specified linear address
		BL		MapContiguous					; make mapping
	IF :DEF: CFG_HasL210Cache :LOR: :DEF: CFG_HasL220Cache :LOR: :DEF: CFG_HasPL310Cache
		MOV		r4, r0 ; r4 = LinAddr of the current HwBank
	ENDIF
		LDR		r0, [sp], #4					; restore default linear address

	IF :DEF: CFG_HasL210Cache :LOR: :DEF: CFG_HasL220Cache :LOR: :DEF: CFG_HasPL310Cache
MapHwBank3
		; Check if the current HW bank contains External Cache Controller.
		; If so, write down its virtual address into SSuperPageBase::iArmL2CacheBase.
		; r5 = phys. address of the external cache controller
		; r1 = physical address of the current HW bank
		; r4 = virtual address of the current HW bank
		; r3 = the size of the current bank
		; r7 = 0 if we have already found cache controller in of the previous HW banks
		CMP		r7, #0					; Still in search for linear address of external cache controller?
		BEQ		MapHwBank				; If no, go to the next HwBank

		SUBS	r7, r5, r1				; r7 = PhysAddr of ExtCacheCtrl - PhysAddr of current HwBank
										; i.e. offset of cache controller with respect to the current bank
		BLO		MapHwBank				; ofsset(in r7) is <0 so not in this bank (and r7 != 0)

		CMP		r7, r3					; If 0 <= r7 < r3 then it's in this bank
		BHS		MapHwBank				; Not in this bank (and r7 != 0)

		; The current HwBank holds External Cache Controller
		ADD		r5, r7, r4				; r5 = LinAddr of ExtCacheCtrl
		STR		r5, [r10, #SSuperPageBase_iArmL2CacheBase]	; Set Linear Address of ExtCacheCtrl in super page

		MOV		r7, #0					; Mark that Linear Address of ExtCacheCtrl is found
	ENDIF	; IF :DEF: CFG_HasL210Cache :LOR: :DEF: CFG_HasL220Cache :LOR: :DEF: CFG_HasPL310Cache

		B		MapHwBank						; next bank

MapHwBank_End

; dummy uncached mapping
		PRTLN	"Setup dummy uncached"
	IF	CFG_MMDirect
		GETMPARAM	BPR_UncachedLin				; linear address into R0
		MOV		r3, #0x1000						; 4K
		MOV		r4, #12							; small page
		ADD		r1, r10, #0x2000				; physical
	ELSE
		LDR		r0, =KDummyUncachedAddr			; linear address into R0
		MOV		r3, #0x1000						; 4K
		MOV		r4, #12							; small page
		MOV		r1, r10							; physical = super page physical
	ENDIF
		MOV		r2, #BTP_Uncached				; permissions
		STR		r0, [r10, #SSuperPageBase_iUncachedAddress]
	IF	CFG_MMUPresent :LAND: :LNOT: CFG_MMDirect
		BL		AllocAndMap
	ELSE
		BL		MapContiguous
	ENDIF	

	IF	CFG_MMDirect
	IF	SMP

; AP Boot Page
		GETMPARAM	BPR_APBootLin				; linear address into R0
		DWORD	r0, "APBootPageLin"
		STR		r0, [r10, #SSuperPageBase_iAPBootPageLin]
		MOV		r3, #0x1000						; 4K
		MOV		r4, #12							; small page
		ADD		r1, r10, #0x3000				; physical
		DWORD	r1, "APBootPagePhys"
		STR		r1, [r10, #SSuperPageBase_iAPBootPagePhys]
		MOV		r2, #BTP_Uncached				; permissions
		BL		MapContiguous
		MOV		r0, r1
		MOV		r1, #0x1000
		MOV		r2, #0
		BL		WordFill						; clear AP Boot Page
	ENDIF
	ENDIF

	IF	CFG_MMDirect :LAND: CFG_UseBootstrapVectors
		STR		r12, [sp, #-4]!					; exc phys = ROM header phys
	ELSE
		LDR		r0, [r11, #TRomImageHeader_iCodeAddress]	; r0 = kernel code base linear
		DWORD	r0, "KernCodeLin"
		BL		RomLinearToPhysical				; r0 = kernel code base physical
		DWORD	r0, "KernCodePhys"
		STR		r0, [sp, #-4]!					; save it
	ENDIF

		PRTLN	"Switch to virtual"
		BL		SwitchToVirtual					; SWITCH TO VIRTUAL ADDRESSES

; map the exception vectors
		PRTLN	"Map vectors"
		MRC		p15, 0, r0, c1, c0, 0			; R0 = MMUCR
		TST		r0, #MMUCR_V					; HIVECS?
		MOV		r0, #0							; if not, linear = 0
		SUBNE	r0, r0, #0x10000				; else linear = 0xFFFF0000
		LDR		r1, [sp], #4					; physical
	IF	CFG_MMDirect
		CMP		r1, r0							; in direct model, if lin = phys mapping is already there
		BEQ		SkipMapExcVectors
	ENDIF
		MOV		r2, #BTP_Vector					; permissions
		MOV		r3, #0x1000						; map size
		MOV		r4, #12							; page size
		BL		MapContiguous
SkipMapExcVectors

	ELSE	; CFG_MMUPresent

		GETMPARAM	BPR_UncachedLin				; address into R0
		STR		r0, [r10, #SSuperPageBase_iUncachedAddress]

	ENDIF	; CFG_MMUPresent

; get initial thread stack size
		MVN		r9, #0
		LDR		r8, [r11, #TRomImageHeader_iStackSize]		; kernel stack size
		ADD		r8, r8, r9, LSR #20
		BIC		r8, r8, r9, LSR #20							; round up to 4K
		DWORD	r8, "InitStackSize"

	IF	:LNOT:	CFG_MMDirect

; calculate initial size of kernel heap, needs to be just enough to allow the device to
; boot past the point where the kernel heap is mutated to be growable.
		LDR		r3, [r10, #SSuperPageBase_iTotalRamSize]	; total RAM size
		LDR		r2, [r11, #TRomImageHeader_iHeapSizeMin]	; kernel heap size min
	IF :DEF: CFG_MMFlexible
; Flexible memory model requires approx. 2 bytes per RAM page.
		MOV		r3, r3, LSR #11								; Total RAM size in half pages
	ELSE
; Other memory models require approx. 1 byte per RAM page.
		MOV		r3, r3, LSR #12								; Total RAM size in pages
	ENDIF
	IF	:DEF: CFG_KernelHeapMultiplier
		LDR		r0, =CFG_KernelHeapMultiplier				; multiplier * 16
		MUL		r3, r0, r3
		MOV		r3, r3, LSR #4
	ENDIF
	IF	:DEF: CFG_KernelHeapBaseSize
		LDR		r0, =CFG_KernelHeapBaseSize
		ADD		r3, r3, r0									; add base size specified in config.inc
	ELSE
		ADD		r3, r3, #48*1024							; add default base size of 48K 
															; was 24K on 15/07/09, increased after H2 test ROM exceed this
	ENDIF
		DWORD	r3, "CalcInitHeap"
		DWORD	r2, "SpecInitHeap"
		CMP		r3, r2
		MOVLS	r3, r2										; if ROMBUILD specified figure higher, use it
		ADD		r3, r3, r9, LSR #20
		BIC		r3, r3, r9, LSR #20							; round up to 4K
		DWORD	r3, "preliminary InitHeap"

; map kernel .data/.bss, initial thread stack and kernel heap
		LDR		r7, [r12, #TRomHeader_iTotalSvDataSize]		; total size of kernel .data / .bss
		ADD		r7, r7, r9, LSR #20
		BIC		r7, r7, r9, LSR #20							; round up to 4K
		DWORD	r7, "Rounded SvData"
		LDR		r0, [r12, #TRomHeader_iKernDataAddress]		; linear address
		MOV		r2, #BTP_Kernel								; permissions
		ADD		r3, r3, r8									; size = heap size + stack size + data/bss size
		ADD		r3, r3, r7
	IF	:LNOT: :DEF: CFG_MinimiseKernelHeap
		ADD		r3, r3, r9, LSR #16
		BIC		r3, r3, r9, LSR #16							; round total up to 64K
	ENDIF
		DWORD	r3, "Total SvHeap"
		SUB		r4, r3, r8									; subtract stack size
		SUB		r4, r4, r7									; subtract data/bss size
		STR		r4, [r10, #SSuperPageBase_iInitialHeapSize]	; save in super page
		DWORD	r4, "InitHeap"
		MOV		r4, #16										; use 64k pages, 1mb fails when converted to DChunk
		PRTLN	"Map stack/heap"
		BL		AllocAndMap

	ELSE	; CFG_MMDirect

		LDR		r7, [r12, #TRomHeader_iTotalSvDataSize]		; total size of kernel .data / .bss
		ADD		r7, r7, r9, LSR #20
		BIC		r7, r7, r9, LSR #20							; round up to 4K
		DWORD	r7, "Rounded SvData"
		LDR		r3, [r11, #TRomImageHeader_iHeapSizeMax]	; kernel heap max
		STR		r3, [r10, #SSuperPageBase_iInitialHeapSize]	; save in super page
		DWORD	r3, "InitHeap"

	ENDIF	; CFG_MMDirect

; fill initial thread stack with 0xff

		LDR		r0, [r12, #TRomHeader_iKernDataAddress]		; linear address
		ADD		r0, r0, r7									; = data address + size of data/bss
		DWORD	r0, "Initial stack base"
		MOV		r1, r8										; size
		MVN		r2, #0										; fill value
		BL		WordFill

; switch to initial thread stack

		DWORD	r0, "Initial SP"
		MOV		sp, r0

; initialise kernel .data and .bss

		PRTLN	"Init .data/.bss"
		LDR		r0, [r11, #TRomImageHeader_iDataAddress]	; source address
		DWORD	r0, "Kernel .data source"
		LDR		r1, [r12, #TRomHeader_iKernDataAddress]		; destination address
		DWORD	r1, "Kernel .data dest"
		LDR		r2, [r11, #TRomImageHeader_iDataSize]		; size of .data
		DWORD	r2, "Kernel .data size"
		ADD		r2, r2, #3
		BIC		r2, r2, #3									; round to multiple of 4
		ADD		r4, r1, r2									; base address of .bss
		DWORD	r4, "Kernel .bss dest"
		DWORD	r7, "TotalSvDataSize"						; total data/bss size rounded up to 4K
		SUB		r5, r7, r2									; size of .bss
		DWORD	r5, "Kernel .bss size"
		BL		WordMove									; initialise .data
		MOV		r0, r4
		MOV		r1, r5
		MOV		r2, #0
		BL		WordFill									; initialise .bss

	IF	:LNOT:	CFG_MMDirect

; allocate SPageInfo array

		PRTLN	"Map SPageInfo array start"

		LDR		r9, =KPageInfoMap
		MOV		r0, r9							; linear
		MOV		r2, #BTP_Kernel					; permissions
		MOV		r3, #0x1000						; size
		MOV		r4, #12							; map size
		MOV		r5, r3
		BL		AllocAndMap
		MOV		r0, r9
		MOV		r1, r5	
		MOV		r2, #0
		BL		WordFill						; zero memory

		LDR		r4, [r10, #SSuperPageBase_iRamBootData]
_page_info_map_make_outer
		LDMIA	r4!, {r5,r6}					; r5 = bank physical base, r6 = bank size
		CMP		r6, #0							; reached end of list?
		BEQ		_page_info_map_make_end			; jump if end
		ADD		r6, r6, r5
		SUB		r6, r6, #1
		MOV		r5, r5, LSR #24-KPageInfoShift
		MOV		r6, r6, LSR #24-KPageInfoShift
		CMP		r5, r6
		BHI		_page_info_map_make_outer
		MOV		r7, #1
_page_info_map_make_inner
		DWORD	r5, "SPageInfo Page"
		AND		r1, r5, #7
		MOV		r1, r7, LSL r1
		LDRB	r0, [r9, r5, LSR #3]
		ORR		r0, r0, r1
		STRB	r0, [r9, r5, LSR #3]
		CMP		r5, r6
		ADD		r5, r5, #1
		BNE		_page_info_map_make_inner
		B		_page_info_map_make_outer
_page_info_map_make_end

		MOV		r5, #0
_page_info_alloc_find_start
		AND		r1, r5, #7
		MOV		r1, r7, LSL r1
		LDRB	r0, [r9, r5, LSR #3]
		TST		r0, r1
		BNE		_page_info_alloc_start_found
		ADD		r5, r5, #1
		MOVS	r0, r5, LSL #24-KPageInfoShift
		BNE		_page_info_alloc_find_start
		B		_page_info_alloc_end
_page_info_alloc_start_found
		MOV		r6, r5							; r6 = page offset for start of region
_page_info_alloc_find_end
		AND		r1, r5, #7
		MOV		r1, r7, LSL r1
		LDRB	r0, [r9, r5, LSR #3]
		TST		r0, r1
		BEQ		_page_info_alloc_end_found
		ADD		r5, r5, #1
		MOVS	r0, r5, LSL #24-KPageInfoShift
		BNE		_page_info_alloc_find_end
_page_info_alloc_end_found
		; r6 = start page offset
		; r5 = end page offset
		SUB		r0, r5, r6						; r0 = number of pages to map
		MOV		r3, r0, LSL #12					; r3 = size to map
		LDR		r0, =KPageInfoLinearBase
		ADD		r0, r0, r6, LSL #12				; r0 = address to map at
		MOV		r2, #BTP_Kernel					; permissions
		MOV		r4, #20							; map size
		STMDB	sp!, {r0,r3}
		BL		AllocAndMap
		LDMIA	sp!, {r0,r1}
		MOV		r2, #0
		BL		WordFill						; zero memory

		MOVS	r0, r5, LSL #24-KPageInfoShift
		BNE		_page_info_alloc_find_start
_page_info_alloc_end
		PRTLN	"Map SPageInfo array end"

	ENDIF	; CFG_MMDirect


; allocate memory for IRQ, FIQ, UND & ABT stacks

		PRTLN	"Allocate IRQ, FIQ, UND & ABT stacks"

	IF	CFG_MMDirect
		ADD		r5, r10, #SSuperPageBase_iStackInfo ; r5 <= offset SSuperPageBase_iStackInfo
		ADD		r0, r10, #0x3000					; r0 = stack base
	IF	SMP
		ADD		r0, r0,	#0x1000						; r0 = stack base
	ENDIF
		MOV		r3, #0x1000							; stack size
		DWORD	r0, "IRQ stack base"
		STR		r0, [r5, #TStackInfo_iIrqStackBase]	; save stack base in super page
		DWORD	r3, "IRQ stack size"
		STR		r3, [r5, #TStackInfo_iIrqStackSize]	; save stack size in super page
		ADD		r0, r0, r3
		MOV		r3, #0x400							; stack size
		DWORD	r0, "FIQ stack base"
		STR		r0, [r5, #TStackInfo_iFiqStackBase]	; save in super page
		DWORD	r3, "FIQ stack size"
		STR		r3, [r5, #TStackInfo_iFiqStackSize]	; save in super page
		ADD		r0, r0, r3
		DWORD	r0, "UND stack base"
		STR		r0, [r5, #TStackInfo_iUndStackBase]	; save in super page
		DWORD	r3, "UND stack size"
		STR		r3, [r5, #TStackInfo_iUndStackSize]	; save in super page
		ADD		r0, r0, r3
		DWORD	r0, "ABT stack base"
		STR		r0, [r5, #TStackInfo_iAbtStackBase]	; save in super page
		DWORD	r3, "ABT stack size"
		STR		r3, [r5, #TStackInfo_iAbtStackSize]	; save in super page
		ADD		r0, r0, r3
		DWORD	r0, "i_Regs"						; address of SFullArmRegSet

	ELSE
		MOV		r2, #BTP_Kernel						; permissions
		MOV		r4, #KPageShift						; page size  
		ADD		r5, r10, #SSuperPageBase_iStackInfo ; r5 <= offset SSuperPageBase_iStackInfo
		MVN		r1, #0								; fill value

; IRQ stack

		LDR		r0, =KExcptStacksLinearBase
		ADD		r0, r0, #KPageSize					; leave space for guard page
		DWORD	r0, "IRQ stack base"
		STR		r0, [r5, #TStackInfo_iIrqStackBase]	; save stack base in super page
		LDR		r3, =KIrqStackSize					; stack size
		DWORD	r3, "IRQ stack size"
		STR		r3, [r5, #TStackInfo_iIrqStackSize]	; save stack size in super page
		BL		AllocAndMap

; FIQ stack
	
		ADD		r0, r0, r3							; ro += stack_size 
		ADD		r0, r0, r1, LSR #32-KPageShift
		BIC		r0, r0, r1, LSR #32-KPageShift		; round up to PageSize
		ADD		r0, r0, #KPageSize					; ro += KPageSize
		DWORD	r0, "FIQ stack base"
		STR		r0, [r5, #TStackInfo_iFiqStackBase]	; save in super page
		LDR		r3, =KFiqStackSize					; stack size
		DWORD	r3, "FIQ stack size"
		STR		r3, [r5, #TStackInfo_iFiqStackSize]	; save in super page
		BL		AllocAndMap

; UND stack
	
		ADD		r0, r0, r3							; ro += stack_size 
		ADD		r0, r0, r1, LSR #32-KPageShift
		BIC		r0, r0, r1, LSR #32-KPageShift		; round up to PageSize
		ADD		r0, r0, #KPageSize					; ro += KPageSize
		DWORD	r0, "UND stack base"
		STR		r0, [r5, #TStackInfo_iUndStackBase]	; save in super page
		LDR		r3, =KUndStackSize					; stack size
		DWORD	r3, "UND stack size"
		STR		r3, [r5, #TStackInfo_iUndStackSize]	; save in super page
		BL		AllocAndMap

; ABT stack
	
		ADD		r0, r0, r3							; ro += stack_size 
		ADD		r0, r0, r1, LSR #32-KPageShift
		BIC		r0, r0, r1, LSR #32-KPageShift		; round up to PageSize
		ADD		r0, r0, #KPageSize					; ro += KPageSize
		DWORD	r0, "ABT stack base"
		STR		r0, [r5, #TStackInfo_iAbtStackBase]	; save in super page
		LDR		r3, =KAbtStackSize					; stack size
		SUB		r3, r3, #0x400						; reserve 1KB for SFullArmRegSet
		DWORD	r3, "ABT stack size"
		STR		r3, [r5, #TStackInfo_iAbtStackSize]	; save in super page
		BL		AllocAndMap

	ENDIF

; fill IRQ, FIQ, UND, ABT stacks and set up banked stack pointers
		GETCPSR	r4
		LDR		r0, [r5, #TStackInfo_iIrqStackBase]	;
		LDR		r1, [r5, #TStackInfo_iIrqStackSize]	;
		MOV		r2, #0xAA
		ADD		r2, r2, r2, lsl #8
		ADD		r2, r2, r2, lsl #16
		BL		WordFill
		BIC		r3, r4, #0x1f
		ORR		r3, r3, #0xd2						; mode_irq
		SETCPSR	r3
		MOV		r13, r0								; set up R13_irq
		SETCPSR	r4
		LDR		r0, [r5, #TStackInfo_iFiqStackBase]	;
		LDR		r1, [r5, #TStackInfo_iFiqStackSize]	;
		MOV		r2, #0xBB
		ADD		r2, r2, r2, lsl #8
		ADD		r2, r2, r2, lsl #16
		BL		WordFill
		BIC		r3, r4, #0x1f
		ORR		r3, r3, #0xd1						; mode_fiq
		SETCPSR	r3
		MOV		r13, r0								; set up R13_fiq
		SETCPSR	r4
		LDR		r0, [r5, #TStackInfo_iUndStackBase]	;
		LDR		r1, [r5, #TStackInfo_iUndStackSize]	;
		MOV		r2, #0xDD
		ADD		r2, r2, r2, lsl #8
		ADD		r2, r2, r2, lsl #16
		BL		WordFill
		BIC		r3, r4, #0x1f
		ORR		r3, r3, #0xdb						; mode_und
		SETCPSR	r3
		MOV		r13, r0								; set up R13_und
		SETCPSR	r4
		LDR		r0, [r5, #TStackInfo_iAbtStackBase]	;
		LDR		r1, [r5, #TStackInfo_iAbtStackSize]	;
		MOV		r2, #0xDD
		ADD		r2, r2, r2, lsl #8
		ADD		r2, r2, r2, lsl #16
		BL		WordFill
		BIC		r3, r4, #0x1f
		ORR		r3, r3, #0xd7						; mode_abt
		SETCPSR	r3
		MOV		r13, r0								; set up R13_abt
		SETCPSR	r4
		MOV		r1, #0x400
		MOV		r2, #0
		BL		WordFill							; zero fill SFullArmRegSet space
	IF	CFG_MMDirect
		LDR		r1, [r5, #TStackInfo_iIrqStackBase]	;
		SETCPSR	r3
		SUB		r14, r0, r1							; total size of exception mode stacks + SFullArmRegSet
		SETCPSR	r4
	ENDIF

	IF	SMP
	IF	:LNOT: CFG_MMDirect
		; Allocate and map uncached AP Boot Page
		MOV		r2, #BMA_Kernel						; type
		MOV		r4, #12								; size = 1 page
		BOOTCALL	BTF_Alloc						; allocate page, physical address into R0
		DWORD	r0, "APBootPagePhys"
		STR		r0, [r10, #SSuperPageBase_iAPBootPagePhys]
		MOV		r1, r0								; physical
		LDR		r0, =KAPBootPageLin					; virtual
		DWORD	r0, "APBootPageLin"
		STR		r0, [r10, #SSuperPageBase_iAPBootPageLin]
		MOV		r2, #BTP_Uncached					; permissions
		MOV		r3, #KPageSize						; size
		MOV		r4, #12
		BL		MapContiguous						; map the page
		MOV		r1, #0x1000
		MOV		r2, #0
		BL		WordFill							; clear AP Boot Page
	ENDIF
	ENDIF

; do final hardware-dependent initialisation

		PRTLN	"Final platform dependent initialisation"
		BOOTCALL	BTF_Final

; for bootloader work out address where image should go
	IF	CFG_BootLoader
		GETMPARAM	BPR_BootLdrImgAddr						; R0 = image physical address
		BL		RamPhysicalToLinear
		MOV		r9, r0										; save linear in R9
		DWORD	r9, "ImgAddrLin"
	ENDIF

; final diagnostics
	IF	CFG_DebugBootRom

		PRTLN	"Super page and CPU page:"
		MOV		r1, #SuperCpuSize
		MEMDUMP r10, r1										; dump super page + CPU page

	IF	CFG_MMUPresent
		PRTLN	"Page directory:"
	IF	CFG_MMDirect
		GETPARAM	BPR_PageTableSpace, DefaultPTAlloc		; get reserved space for page tables
		LDR		r2, [r10, #SSuperPageBase_iPageDir]			;
		MOV		r1, r0										; size
		ADD		r2, r2, #0x4000								; end of page dir
	IF	SMP
		ADD		r2, r2, #0x4000								; end of APBoot page dir
	ENDIF
		SUB		r0, r2, r0									; size includes page directory and tables
	ELSE
		LDR		r0, [r10, #SSuperPageBase_iPageDir]			; page directory address
		MOV		r1, #0x4000									; page directory size
	ENDIF
		MEMDUMP r0, r1										; dump page directory

	IF	:LNOT: CFG_MMDirect
		LDR		r0, =KPageTableBase							; page table linear base
		SUB		r2, r0, #4
_CountPt
		LDR		r1, [r2, #4]!								; get next PT0 entry
		CMP		r1, #0										; empty?
		BNE		_CountPt									; if not, next
		SUB		r1, r2, r0									; 4*number of page tables
		MOV		r1, r1, LSL #10								; 1K per page table
		PRTLN	"Page tables:"
		MEMDUMP r0, r1										; dump page tables
	IF	SMP
		LDR		r0, [r10, #SSuperPageBase_iAPBootPageDirPhys]
		DWORD	r0, "APBootPageDirPhys"
		PRTLN	"APBoot PageDir/Tables:"
		LDR		r0, =KAPBootPageDirLin						; r0 = page directory linear
		MOV		r1, #0x5000									; 16K page directory + page tables
		MEMDUMP r0, r1										; dump page tables
		PRTLN	"APBootPage"
		LDR		r0, [r10, #SSuperPageBase_iAPBootPageLin]
		MOV		r1, #0x1000
		MEMDUMP r0, r1										; dump AP Boot Page
	ENDIF
	ENDIF

	ENDIF	; CFG_MMUPresent
	ENDIF	; CFG_DebugBootRom

	IF :LNOT: CFG_MemoryTypeRemapping
		; Pass BPR_Platform_Specific_Mappings to Kernel on platforms
		; with no memory type remapping feature. Kernel will emulate them
		; in order to support TMemoryType values 4-7
		MOV		r0, #BPR_Platform_Specific_Mappings
		BOOTCALL	BTF_Params				; r0 = BPR_Platform_Specific_Mappings
		MOVMI	r0, #0						; r0 = 0, if parameter not defined in table.
		STR		r0, [r10, #SSuperPageBase_iPlatformSpecificMappings]
	ENDIF
	
	IF :DEF: CFG_HasL210Cache
		;Enable L2 cache. Enabling L220 & PL310 is baseport specific due to security extension (TrustZone).
		LDR     r0, [r10, #SSuperPageBase_iArmL2CacheBase]
		MOV		r1, #1
		STR     r1, [r0, #0x100]
		PRTLN	"L2CACHE: Enabled"
	ENDIF

; boot the kernel

        LDR     r14, [r11, #TRomImageHeader_iEntryPoint]	; kernel entry point
        DWORD   r14, "Jumping to OS at location"
		MOV		r0, r12										; pass address of ROM header
		MOV		r1, r10										; pass address of super page
		DWORD	r0, "R0"
		DWORD	r1, "R1"
	IF	CFG_BootLoader
		STR		r9, [r10, #SSuperPageBase_iCodeBase]		; for bootloader pass image address
	ENDIF
	IF	CFG_DebugBootRom
		; pause to let tracing finish
		MOV		r12, #0x00100000
		SUBS	r12, r12, #1
		SUBNE	pc, pc, #12
	ENDIF
        MOV     pc, r14										; jump to kernel entry point




		END
