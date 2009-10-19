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
; template/bootstrap/template.s
; Template for platform specific boot code
;

		GBLL	__VARIANT_S__		; indicates that this is platform-specific code
		GBLL	__TEMPLATE_S__		; indicates which source file this is

		INCLUDE	bootcpu.inc

;
;*******************************************************************************
;
; Platform specific constant definitions

RamBank0Base		EQU		0x10000000
RamBank0MaxSize		EQU		0x00800000
RamBank1Base		EQU		0x20000000
RamBank1MaxSize		EQU		0x00000000

PrimaryRomBase		EQU		0x00000000
PrimaryRomSize		EQU		0x00800000
ExtensionRomBase	EQU		0x08000000
ExtensionRomSize	EQU		0x00000000

Serial0PhysBase		EQU		0x80000000
Serial1PhysBase		EQU		0x80000100

;
;*******************************************************************************
;

        AREA |Boot$$Code|, CODE, READONLY, ALIGN=6

;
;*******************************************************************************
;




;*******************************************************************************
; Initialise Hardware
;	Initialise CPU registers
;	Determine the hardware configuration
;	Determine the reset reason. If it is wakeup from a low power mode, perform
;		whatever reentry sequence is required and jump back to the kernel.
;	Set up the memory controller so that at least some RAM is available
;	Set R10 to point to the super page or to a temporary version of the super page
;		with at least the following fields valid:
;		iBootTable, iCodeBase, iActiveVariant, iCpuId
;	In debug builds initialise the debug serial port
;
; Enter with:
;	R12 points to TRomHeader
;	NO STACK
;	R14 = return address (as usual)
;
; All registers may be modified by this call
;*******************************************************************************
	IF	CFG_BootLoader
	; For bootloader we only get here on a full reset
	; Other resets will simply jump back into the previously-loaded image
	EXPORT	DoInitialiseHardware
DoInitialiseHardware	ROUT
	ELSE
	EXPORT	InitialiseHardware
InitialiseHardware	ROUT
	ENDIF
		MOV		r13, lr										; save return address
		ADRL	r1, ParameterTable							; pass address of parameter table
		BL		InitCpu										; initialise CPU/MMU registers

		; Put your hardware initialising code here

	IF	CFG_DebugBootRom
		BL		InitDebugPort
	ENDIF

; Set up the required super page values
		LDR		r10, =0xC0000000							; initial super page
		LDR		r0, =0x05040001								; variant code
		STR		r0, [r10, #SSuperPageBase_iActiveVariant]
		STR		r0, [r10, #SSuperPageBase_iHwStartupReason]	; reset reason (from hardware)
		ADD		r1, r10, #CpuPageOffset
		STR		r1, [r10, #SSuperPageBase_iMachineData]
		ADRL	r0, BootTable
		STR		r0, [r10, #SSuperPageBase_iBootTable]		; Set the boot function table
		STR		r12, [r10, #SSuperPageBase_iCodeBase]		; Set the base address of bootstrap code
		MRC		p15, 0, r0, c0, c0, 0						; read CPU ID from CP15 (remove if no CP15)
		STR		r0, [r10, #SSuperPageBase_iCpuId]

		MOV		pc, r13										; return





;*******************************************************************************
; Notify an unrecoverable error during the boot process
;
; Enter with:
;	R14 = address at which fault detected
;
; Don't return
;*******************************************************************************
	EXPORT	Fault
Fault	ROUT
		B		BasicFaultHandler	; generic handler dumps registers via debug
									; serial port





;*******************************************************************************
; Reboot the system
;
; Enter with:
;		R0 = reboot reason code
;
; Don't return (of course)
;*******************************************************************************
	ALIGN	32, 0
	EXPORT	RestartEntry
RestartEntry	ROUT
		; save R0 parameter in HW dependent register which is preserved over reset
		; put HW specific code here to reset system
		SUB		pc, pc, #8





;*******************************************************************************
; Get a pointer to the list of RAM banks
;
; The pointer returned should point to a list of {BASE; MAXSIZE;} pairs, where
; BASE is the physical base address of the bank and MAXSIZE is the maximum
; amount of RAM which may be present in that bank. MAXSIZE should be a power of
; 2 and BASE should be a multiple of MAXSIZE. The generic code will examine the
; specified range of addresses and determine the actual amount of RAM if any
; present in the bank. The list is terminated by an entry with zero size.
;
; The pointer returned will usually be to constant data, but could equally well
; point to RAM if dynamic determination of the list is required.
;
; Enter with :
;		R10 points to super page
;		R12 points to ROM header
;		R13 points to valid stack
;
; Leave with :
;		R0 = pointer
;		Nothing else modified
;*******************************************************************************
GetRamBanks	ROUT
		ADR		r0, %FT1
		MOV		pc, lr
1
		DCD		RamBank0Base, RamBank0MaxSize
		DCD		RamBank1Base, RamBank1MaxSize
		DCD		0,0				; terminator





;*******************************************************************************
; Get a pointer to the list of ROM banks
;
; The pointer returned should point to a list of entries of SRomBank structures,
; usually declared with the ROM_BANK macro.
; The list is terminated by a zero size entry (four zero words)
;
; ROM_BANK	PB, SIZE, LB, W, T, RS, SS
; PB = physical base address of bank
; SIZE = size of bank
; LB = linear base if override required - usually set this to 0
; W = bus width (ROM_WIDTH_8, ROM_WIDTH_16, ROM_WIDTH_32)
; T = type (see TRomType enum in kernboot.h)
; RS = random speed
; SS = sequential speed
;
; Only PB, SIZE, LB are used by the rest of the bootstrap.
; The information given here can be modified by the SetupRomBank call, if
; dynamic detection and sizing of ROMs is required.
;
; Enter with :
;		R10 points to super page
;		R12 points to ROM header
;		R13 points to valid stack
;
; Leave with :
;		R0 = pointer
;		Nothing else modified
;*******************************************************************************
GetRomBanks	ROUT
		ADR		r0, %FT1
		MOV		pc, lr
1
		ROM_BANK	PrimaryRomBase,		PrimaryRomSize,		0, ROM_WIDTH_32, ERomTypeXIPFlash, 0, 0
		ROM_BANK	ExtensionRomBase,	ExtensionRomSize,	0, ROM_WIDTH_32, ERomTypeXIPFlash, 0, 0
		DCD		0,0,0,0			; terminator





;*******************************************************************************
; Get a pointer to the list of hardware banks
;
; The pointer returned should point to a list of hardware banks declared with
; the HW_MAPPING and/or HW_MAPPING_EXT macros. A zero word terminates the list.
; For the direct memory model, all hardware on the system should be mapped here
; and the mapping will set linear address = physical address.
; For the moving or multiple model, only the hardware required to boot the kernel
; and do debug tracing needs to be mapped here. The linear addresses used will
; start at KPrimaryIOBase and step up as required with the order of banks in
; the list being maintained in the linear addresses used.
;
; HW_MAPPING PB, SIZE, MULT
;	This declares a block of I/O with physical base PB and address range SIZE
;	blocks each of which has a size determined by MULT. The page size used for
;	the mapping is determined by MULT. The linear address base of the mapping
;	will be the next free linear address rounded up to the size specified by
;	MULT.
;	The permissions used for the mapping are the standard I/O permissions (BTP_Hw).
;
; HW_MAPPING_EXT PB, SIZE, MULT
;	This declares a block of I/O with physical base PB and address range SIZE
;	blocks each of which has a size determined by MULT. The page size used for
;	the mapping is determined by MULT. The linear address base of the mapping
;	will be the next free linear address rounded up to the size specified by
;	MULT.
;	The permissions used for the mapping are determined by a BTP_ENTRY macro
;	immediately following this macro in the HW bank list or by a DCD directive
;	specifying a different standard permission type.
;
; HW_MAPPING_EXT2 PB, SIZE, MULT, LIN
;	This declares a block of I/O with physical base PB and address range SIZE
;	blocks each of which has a size determined by MULT. The page size used for
;	the mapping is determined by MULT. The linear address base of the mapping
;	is specified by the LIN parameter.
;	The permissions used for the mapping are the standard I/O permissions (BTP_Hw).
;
; HW_MAPPING_EXT3 PB, SIZE, MULT, LIN
;	This declares a block of I/O with physical base PB and address range SIZE
;	blocks each of which has a size determined by MULT. The page size used for
;	the mapping is determined by MULT. The linear address base of the mapping
;	is specified by the LIN parameter.
;	The permissions used for the mapping are determined by a BTP_ENTRY macro
;	immediately following this macro in the HW bank list or by a DCD directive
;	specifying a different standard permission type.
;
; Configurations without an MMU need not implement this function.
;
; Enter with :
;		R10 points to super page
;		R12 points to ROM header
;		R13 points to valid stack
;
; Leave with :
;		R0 = pointer
;		Nothing else modified
;*******************************************************************************
GetHwBanks	ROUT
		ADR		r0, %FT1
		MOV		pc, lr
1
	IF	CFG_MMDirect
		; for direct model we must map all peripherals here
		; use section mappings to reduce number of page tables required
		HW_MAPPING		0x00100000,	   31,	HW_MULT_1M		; 0x00100000 - 0x01FFFFFF
		HW_MAPPING		0x08000000,	   32,	HW_MULT_1M		; 0x08000000 - 0x09FFFFFF
		HW_MAPPING		0x80000000,		1,	HW_MULT_1M		; 0x80000000 - 0x800FFFFF
		HW_MAPPING		0x90000000,		1,	HW_MULT_1M		; 0x90000000 - 0x900FFFFF
		HW_MAPPING		0xA0000000,		1,	HW_MULT_1M		; 0xA0000000 - 0xA00FFFFF
		HW_MAPPING		0xB0000000,		1,	HW_MULT_1M		; 0xB0000000 - 0xB00FFFFF
		HW_MAPPING		0xB0100000,		1,	HW_MULT_1M		; 0xB0100000 - 0xB01FFFFF
	ELSE
		HW_MAPPING		0x80000000,		1,	HW_MULT_4K		; 0x80000000 - 0x80000FFF mapped at KPrimaryIOBase + 0
		HW_MAPPING		0x80010000,		1,	HW_MULT_4K		; 0x80010000 - 0x80010FFF mapped at KPrimaryIOBase + 0x1000
		HW_MAPPING		0x80020000,		1,	HW_MULT_64K		; 0x80020000 - 0x8002FFFF mapped at KPrimaryIOBase + 0x10000
		HW_MAPPING_EXT	0x90000000,		1,	HW_MULT_4K		; 0x90000000 - 0x90000FFF mapped at KPrimaryIOBase + 0x20000 ...
		DCD				BTP_Rom								; ... with same permissions as ROM
	ENDIF
		DCD			0											; terminator





;*******************************************************************************
; Set up RAM bank
;
; Do any additional RAM controller initialisation for each RAM bank which wasn't
; done by InitialiseHardware.
; Called twice for each RAM bank :-
;	First with R3 = 0xFFFFFFFF before bank has been probed
;	Then, if RAM is present, with R3 indicating validity of each byte lane, ie
;	R3 bit 0=1 if D0-7 are valid, bit1=1 if D8-15 are valid etc.
; For each call R1 specifies the bank physical base address.
;
; Enter with :
;		R10 points to super page
;		R12 points to ROM header
;		R13 points to stack
;		R1 = physical base address of bank
;		R3 = width (bottom 4 bits indicate validity of byte lanes)
;			 0xffffffff = preliminary initialise
;
; Leave with :
;		No registers modified
;*******************************************************************************
SetupRamBank	ROUT
		MOV		pc, lr





;*******************************************************************************
; Set up ROM bank
;
; Do any required autodetection and autosizing of ROMs and any additional memory
; controller initialisation for each ROM bank which wasn't done by
; InitialiseHardware.
;
; The first time this function is called R11=0 and R0 points to the list of
; ROM banks returned by the BTF_RomBanks call. This allows any preliminary setup
; before autodetection begins.
;
; This function is subsequently called once for each ROM bank with R11 pointing
; to the current information held about that ROM bank (SRomBank structure).
; The structure pointed to by R11 should be updated with the size and width
; determined. The size should be set to zero if there is no ROM present in the
; bank.
;
; Enter with :
;		R10 points to super page
;		R12 points to ROM header
;		R13 points to stack
;		R11 points to SRomBank info for this bank
;		R11 = 0 for preliminary initialise (all banks)
;
; Leave with :
;		Update SRomBank info with detected size/width
;		Set the size field to 0 if the ROM bank is absent
;		Can modify R0-R4 but not other registers
;
;*******************************************************************************
SetupRomBank	ROUT
		MOV		pc, lr





;*******************************************************************************
; Reserve physical memory
;
; Reserve any physical RAM needed for platform-specific purposes before the
; bootstrap begins allocating RAM for page tables/kernel data etc.
;
; There are two methods for this:
;	1.	The function ExciseRamArea may be used. This will remove a contiguous
;		region of physical RAM from the RAM bank list. That region will never
;		again be identified as RAM.
;	2.	A list of excluded physical address ranges may be written at [R11].
;		This should be a list of (base,size) pairs terminated by a (0,0) entry.
;		This RAM will still be identified as RAM by the kernel but will not
;		be allocated by the bootstrap and will subsequently be marked as
;		allocated by the kernel immediately after boot.
;
; Enter with :
;		R10 points to super page
;		R11 indicates where preallocated RAM list should be written.
;		R12 points to ROM header
;		R13 points to stack
;
; Leave with :
;		R0-R3 may be modified. Other registers should be preserved.
;*******************************************************************************
ReservePhysicalMemory	ROUT
		MOV		pc, lr





;*******************************************************************************
; Return parameter specified by R0 (see TBootParam enum)
;
; Enter with :
;		R0 = parameter number
;
; Leave with :
;		If parameter value is supplied, R0 = value and N flag clear
;		If parameter value is not supplied, N flag set. In this case the
;		parameter may be defaulted or the system may fault.
;		R0,R1,R2 modified. No other registers modified.
;
;*******************************************************************************
GetParameters ROUT
		ADR		r1, ParameterTable
		B		FindParameter
ParameterTable
		; Include any parameters specified in TBootParam enum here
		; if you want to override them.
		DCD		BPR_UncachedLin,	0			; parameter number, parameter value
	IF  :DEF: CFG_CPU_ARM1136 :LAND: (:LNOT: :DEF: CFG_CPU_ARM1136_ERRATUM_364296_FIXED)
        DCD     BPR_FinalMMUCRSet,      ExtraMMUCR + MMUCR_FI
        DCD     BPR_AuxCRSet,           DefaultAuxCRSet + 0x80000000
	ENDIF		
		DCD		-1								; terminator





;*******************************************************************************
; Do final platform-specific initialisation before booting the kernel
;
; Typical uses for this call would be:
;	1.	Mapping cache flushing areas
;	2.	Setting up pointers to routines in the bootstrap which are used by
;		the variant or drivers (eg idle code).
;
; Enter with :
;		R10 points to super page
;		R11 points to TRomImageHeader for the kernel
;		R12 points to ROM header
;		R13 points to stack
;
; Leave with :
;		R0-R9 may be modified. Other registers should be preserved.
;
;*******************************************************************************
FinalInitialise ROUT
		STMFD	sp!, {lr}

	IF	CFG_Template

		; set up main cache flush area
		MOV		r1, #0xE0000000			; physical address
	IF	CFG_MMDirect
		MOV		r0, r1					; direct, linear = physical
	ELSE
		LDR		r0, =KDCacheFlushArea	; linear
	ENDIF
		STR		r0, [r10, #SSuperPageBase_iDCacheFlushArea]
		MOV		r2, #BTP_MainCache		; permissions
		MOV		r3, #0x100000			; size
		MOV		r4, #20					; use section
		BL		MapContiguous

		; set up mini cache flush area
		ADD		r1, r1, r3				; physical address
		ADD		r0, r0, r3				; linear
		STR		r0, [r10, #SSuperPageBase_iAltDCacheFlushArea]
		MOV		r2, #BTP_MiniCache		; permissions
		BL		MapContiguous

		MOV		r3, #0x80000			; wrap for cache flush
		STR		r3, [r10, #SSuperPageBase_iDCacheFlushWrap]
		STR		r3, [r10, #SSuperPageBase_iAltDCacheFlushWrap]

		; set up idle code address
		ADR		r0, IdleCode
		ADD		r5, r10, #CpuPageOffset
		STR		r0, [r5, #CPUPage_Idle]

	ENDIF

		LDMFD	sp!, {pc}





;*******************************************************************************
; Output a character to the debug port
;
; Enter with :
;		R0 = character to output
;		R13 points to valid stack
;
; Leave with :
;		nothing modified
;*******************************************************************************
DoWriteC	ROUT
	IF	CFG_DebugBootRom
		STMFD	sp!, {r1,lr}
		BL		GetDebugPortBase

		; wait for debug port to be ready for data
		; output character to debug port

		LDMFD	sp!, {r1,pc}
	ELSE
		MOV		pc, lr
	ENDIF

	IF	CFG_DebugBootRom

;*******************************************************************************
; Initialise the debug port
;
; Enter with :
;		R12 points to ROM header
;		There is no valid stack
;
; Leave with :
;		R0-R2 modified
;		Other registers unmodified
;*******************************************************************************
InitDebugPort	ROUT
		MOV     r0, lr
		BL		GetDebugPortBase			; r1 = base address of debug port

		; set up debug port

		MOV		pc, r0

;*******************************************************************************
; Get the base address of the debug UART
;
; Enter with :
;		R12 points to ROM header
;		There may be no stack
;
; Leave with :
;		R1 = base address of port
;		No other registers modified
;*******************************************************************************
GetDebugPortBase	ROUT
		LDR		r1, [r12, #TRomHeader_iDebugPort]
		CMP		r1, #1
		BNE		%FA1							; skip if not port 1
		GET_ADDRESS	r1, Serial1PhysBase, Serial1LinBase
		MOV		pc, lr
1
		GET_ADDRESS	r1, Serial0PhysBase, Serial0LinBase
		MOV		pc, lr

	ENDIF	; CFG_DebugBootRom





;*******************************************************************************
; BOOT FUNCTION TABLE
;*******************************************************************************

BootTable
		DCD	DoWriteC				; output a debug character
		DCD	GetRamBanks				; get list of RAM banks
		DCD	SetupRamBank				; set up a RAM bank
		DCD	GetRomBanks				; get list of ROM banks
		DCD	SetupRomBank				; set up a ROM bank
		DCD	GetHwBanks				; get list of HW banks
		DCD	ReservePhysicalMemory			; reserve physical RAM if required
		DCD	GetParameters				; get platform dependent parameters
		DCD	FinalInitialise				; Final initialisation before booting the kernel
	IF :LNOT: CFG_MMUPresent				; no mmu, so use stub version ...
		DCD AllocatorStub				; allocate memory
	ELSE
		DCD HandleAllocRequest				; allocate memory		
		DCD	GetPdeValue				; usually in generic code
		DCD	GetPteValue				; usually in generic code
		DCD	PageTableUpdate				; usually in generic code
		DCD	EnableMmu				; Enable the MMU (usually in generic code)
	ENDIF

; These entries specify the standard MMU permissions for various areas
; They can be omitted if MMU is absent
	IF	CFG_MMUPresent
    BTP_ENTRY   CLIENT_DOMAIN, PERM_RORO, MEMORY_FULLY_CACHED,       	1,  1,  0,  0   ; ROM
    BTP_ENTRY   CLIENT_DOMAIN, PERM_RWNO, MEMORY_FULLY_CACHED,       	0,  1,  0,  0   ; kernel data/stack/heap
    BTP_ENTRY   CLIENT_DOMAIN, PERM_RWNO, MEMORY_FULLY_CACHED,       	0,  1,  0,  0   ; super page/CPU page
    BTP_ENTRY   CLIENT_DOMAIN, PERM_RWNO, MEMORY_FULLY_CACHED,  	0,  1,  0,  0   ; page directory/tables
    BTP_ENTRY   CLIENT_DOMAIN, PERM_RONO, MEMORY_FULLY_CACHED,       	1,  1,  0,  0   ; exception vectors
    BTP_ENTRY   CLIENT_DOMAIN, PERM_RWNO, MEMORY_STRONGLY_ORDERED,      0,  1,  0,  0   ; hardware registers
    DCD         0                                                           ; unused (minicache flush)
    DCD         0                                                           ; unused (maincache flush)
    BTP_ENTRY   CLIENT_DOMAIN, PERM_RWNO, MEMORY_FULLY_CACHED,       	0,  1,  0,  0   ; page table info
    BTP_ENTRY   CLIENT_DOMAIN, PERM_RWRW, MEMORY_FULLY_CACHED,       	1,  1,  0,  0   ; user RAM
    BTP_ENTRY   CLIENT_DOMAIN, PERM_RONO, MEMORY_STRONGLY_ORDERED,      1,  1,  0,  0   ; temporary identity mapping
    BTP_ENTRY   CLIENT_DOMAIN, UNC_PERM,  MEMORY_STRONGLY_ORDERED,      0,  1,  0,  0   ; uncached
	ENDIF


		END
