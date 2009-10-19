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
; e32\kernel\arm\bootutils.s
; General subroutines for bootstrap
;

		GBLL	__BOOTUTILS_S__

		INCLUDE	bootcpu.inc

;
;*******************************************************************************
;

        AREA |Boot$$Code|, CODE, READONLY, ALIGN=6


;*******************************************************************************
; Fill memory with a word value
;
; Enter with :
;       r0 = address (must be word aligned)
;       r1 = number of bytes (must be multiple of 4)
;       r2 = value to fill
;
; Leave with :
;       r0 = address following last filled word
;       r1 = 0
;		other registers unmodified
;*******************************************************************************

	EXPORT WordFill
WordFill	ROUT
		SUBS	r1, r1, #4
		STRHS	r2, [r0], #4
		BHI		WordFill
        MOV		pc, lr

;*******************************************************************************
; Move memory from one address to another
; Works with overlapping address ranges
;
; Enter with :
;       r0 = source address (must be word aligned)
;       r1 = destination address (must be word aligned)
;       r2 = number of bytes (must be multiple of 4)
;
; Leave with :
;       r2 = 0
;		r0, r1, r3 modified
;		other registers unmodified
;*******************************************************************************

	EXPORT WordMove
WordMove	ROUT
		CMP		r1, r0
		MOVEQ	pc, lr
		BHI		%FT2
1		SUBS    r2, r2, #4
		LDRHS	r3, [r0], #4
		STRHS	r3, [r1], #4
		BHI		%BT1
		MOV		pc, lr
2		SUBS    r2, r2, #4
		LDRHS	r3, [r0, r2]
		STRHS	r3, [r1, r2]
		BHI		%BT2
		MOV		pc, lr

;*******************************************************************************
; Look up a parameter in a list of entries { int param no; int value; }
; Terminated by -ve parameter no.
;
; Enter with :
;		R0 = parameter number to find (see enum TBootParam)
;		R1 = address of list
;		R10 = address of super page
;		R12 = address of TRomHeader
;		Stack not required
;
; Return with:
;		R0 = parameter value and N flag clear if parameter provided
;		N flag set if parameter not provided
;		R1,R2 modified. No other registers modified
;		
;*******************************************************************************

	EXPORT	FindParameter
FindParameter	ROUT
1		LDR		r2, [r1], #8
		CMP		r2, #0
		MOVMI	pc, lr				; if reached end, return with N flag set
		CMP		r2, r0				; required number found?
		BNE		%BT1
		LDR		r0, [r1, #-4]		; yes, get value
		MOV		pc, lr

;*******************************************************************************
; Get a platform-dependent mandatory parameter
;
; Enter with :
;		R0 = parameter number (see enum TBootParam)
;		R10 = address of super page
;		R12 = address of TRomHeader
;		R13 points to valid stack
;
; Return with:
;		R0 = parameter value
;		No other registers modified
;		
;*******************************************************************************

	EXPORT	GetMandatoryParameter
GetMandatoryParameter	ROUT
		STMFD	sp!, {r1,r2,lr}
		BOOTCALL	BTF_Params
		FAULT	MI
		LDMFD	sp!, {r1,r2,pc}

;*******************************************************************************
; Get a platform-dependent parameter
;
; Enter with :
;		R0 = parameter number (see enum TBootParam)
;		R10 = address of super page
;		R12 = address of TRomHeader
;		R13 points to valid stack
;
; Return with:
;		R0 = parameter value and N flag clear if parameter provided
;		N flag set if parameter not provided
;		No other registers modified
;		
;*******************************************************************************

	EXPORT	GetParameter
GetParameter	ROUT
		STMFD	sp!, {r1,r2,lr}
		BOOTCALL	BTF_Params
		LDMFD	sp!, {r1,r2,pc}

;*******************************************************************************
; Call a function via the boot table
;
; Enter with :
;		r10 = address of super page
;		[r14] = bottom 8 bits contain function number
;		r14 = return address
;*******************************************************************************

	EXPORT	BootCall
BootCall	ROUT
		STMFD	sp!, {r0,r1,lr}			; save r0, r1, reserve space for address
		LDRB	r0, [lr]				; get function number
		LDR		r1, [r10, #SSuperPageBase_iBootTable]
		LDR		r0, [r1, r0, lsl #2]	; r0 = code offset of function
		LDR		r1, [r10, #SSuperPageBase_iCodeBase]
		ADD		r0, r0, r1				; r0 = address of function
		STR		r0, [sp, #8]			; onto stack
		LDMFD	sp!, {r0,r1,pc}			; restore registers and jump to function





;*******************************************************************************
; Test for the presence and width of RAM
;
; Enter with :
;		RAM controller set to 32-bit bus width
;		R1=address to test (will use [R1] and [R1+4] )
;
; Leave with :
;		R3=0 if no RAM present
;		R3 bit 0 = 1 if D 0- 7 are connected to RAM
;		R3 bit 1 = 1 if D 8-15 are connected to RAM
;		R3 bit 2 = 1 if D16-23 are connected to RAM
;		R3 bit 3 = 1 if D24-31 are connected to RAM
;		R2 modified
;		R8_fiq-R12_fiq,R14_fiq modified
;*******************************************************************************
	EXPORT FindRamBankWidth
FindRamBankWidth	ROUT
		GETCPSR	r2
		BIC		r3, r2, #0x1f
		ORR		r3, r3, #0x11
		SETCPSR	r3						; go into mode_fiq
		LDMIA	r1, {r11,r12}			; save original values of [r1] and [r1+4]
		MOV		r3, #15
		LDR		r8, =0x55555555
		BL		FindRamBankWidth1		; test with 55555555
		ADD		r8, r8, r8
		BL		FindRamBankWidth1		; test with AAAAAAAA
		MVN		r8, #0
		BL		FindRamBankWidth1		; test with FFFFFFFF
		MOV		r8, #0
		BL		FindRamBankWidth1		; test with 00000000
		STMIA	r1, {r11,r12}			; restore original RAM values
		SETCPSR	r2						; back to original mode
		MOV		pc, lr
	
FindRamBankWidth1
; r1=address to test
; r3=mask value - reset bits if RAM fails test
; r8=data value to test with
		MVN		r9, r8
		STR		r9, [r1]				; store NOT(test value) at [r1]
		STR		r8, [r1, #4]			; store test value at [r1+4]
		LDR		r10, [r1]				; read location [r1]
		LDR		r10, [r1, #4]			; read location [r1+4]
		LDR		r10, [r1]				; read location [r1]
		EOR		r10, r10, r9			; EOR with value written
		LDR		r9, [r1, #4]			; read location [r1+4]
		EOR		r9, r9, r8				; EOR with value written
		ORRS	r9, r9, r10				; get overall difference mask
		MOVEQ	pc, lr					; exit if all 32 bits respond correctly
		TST		r9, #0xff000000			; bits 24-31 wrong?
		BICNE	r3, r3, #8				; if so clear bit 3
		TST		r9, #0x00ff0000			; bits 16-23 wrong?
		BICNE	r3, r3, #4				; if so clear bit 2
		TST		r9, #0x0000ff00			; bits 8-15 wrong?
		BICNE	r3, r3, #2				; if so clear bit 1
		TST		r9, #0x000000ff			; bits 0-7 wrong?
		BICNE	r3, r3, #1				; if so clear bit 0
		MOV		pc, lr



;*******************************************************************************
; Find the address configuration of a bank of RAM
;
; Enter with :
;		RAM controller set up for correct data bus width for RAM devices
;		R0 pointing to next location in RAM allocator data, or 0
;		R1=base address of bank to test
;		R2=maximum possible size of this bank (must be power of 2)
;
; Leave with :
;		If R0!=0, allocator data written for this bank, R0 stepped on, R1 modified
;		If R0=0, R0 and R1 unmodified
;		R2 = number of disjoint physical address blocks
;		R3 = size of each physical address block
;		R4 = disconnect mask for bank
;		R5-R12 unmodified
;		R8_fiq to R14_fiq modified
;*******************************************************************************
	EXPORT FindRamBankConfig
FindRamBankConfig	ROUT
		GETCPSR	r3
		BIC		r3, r3, #0x1f
		ORR		r3, r3, #0x11			; mode_fiq
		SETCPSR	r3
		LDR		r10, [r1]				; save original value of [r1]
		MOV		r3, #0
		BL		FindRamBankAddrMap		; test with data value 0
		MOV		r4, r11					; r4=disconnect mask obtained
		MVN		r3, #0
		BL		FindRamBankAddrMap		; test with data value ffffffff
		AND		r4, r4, r11				; r4 has 1's in disconnected address line positions
		STR		r10, [r1]				; restore original value of [r1]
		MOV		r3, #KMinRamBankSize	; start with minimum allowed bank size

		; Contiguous block size is determined by least significant disconnected
		; address line. Search for this here.
FindRamBankCfg1
		TST		r4, r3					; test if address line disconnected
		BNE		FindRamBankCfg2			; if so, r3=block size
		MOV		r3, r3, LSL #1			; else step to next address line
		CMP		r3, r2					; done all address lines in bank?
		BLO		FindRamBankCfg1			; if not, do next

		; Number of disjoint blocks is determined by number of connected address
		; lines which are more significant than the least significant disconnected
		; line. Determine this here.
FindRamBankCfg2							; when we get here, r3=block size
		MOV		r8, #1					; r8 will count number of blocks
		MOV		r9, r3					; start with r9=l.s. disconnected bit

FindRamBankCfg3
		MOV		r9, r9, LSL #1			; step to next address bit
		CMP		r9, r2					; end of bank?
		BHS		FindRamBankCfg6			; branch if end
		TST		r4, r9					; connected bit?
		MOVEQ	r8, r8, LSL #1			; if so, double number of blocks
		B		FindRamBankCfg3			; check next address bit

		; Now we compute the base address of each contiguous block.
FindRamBankCfg6							; when we get here r8=number of blocks
		MOV		r2, r8					; into r2
		CMP		r0, #0
		BEQ		FindRamBankCfg0			; if r0=0, stop here

FindRamBankCfg4
		STR		r1, [r0], #4			; store base address of block
		STR		r3, [r0], #4			; store size of block
		SUBS	r8, r8, #1				; decrement number of blocks
		BEQ		FindRamBankCfg0			; branch out if all blocks done
		MOV		r9, r3					; first step=block size
FindRamBankCfg5
		ADD		r1, r1, r9				; step base address on
		ANDS	r9, r1, r4				; have we set a disconnected bit? if so bit set in r9
		BNE		FindRamBankCfg5			; and step address on again
		B		FindRamBankCfg4			; if not disconnected bit, r1=next block base addr

FindRamBankCfg0
		GETCPSR	r9
		BIC		r9, r9, #0x1f
		ORR		r9, r9, #0x13			; mode_svc
		SETCPSR	r9
		MOV		pc, lr					; return


FindRamBankAddrMap	ROUT
; R0 pointing to next location in RAM allocator data
; R1=base address of bank to test
; R2=bank size
; R3=test data value
; return with R11=disconnected address mask
; R8, R9 modified, R0-R3 unmodified
		STR		r3, [r1]				; write test value
		MOV		r11, #0					; initialise disconnected mask to 0
		MOV		r8, #KMinRamBankSize	; start with minimum allowed bank size

1		LDR		r9, [r1, r8]			; read from [r1+2^n]
		CMP		r9, r3					; test if we read same value as [r1]
		ORREQ	r11, r11, r8			; if so, set bit n in mask
		MOV		r8, r8, LSL #1			; step to next address line
		CMP		r8, r2					; done all address lines in bank?
		BLO		%BT1					; if not, loop and do next one

		MOV		pc, lr					; finished, disconnect mask in r11



;*******************************************************************************
; SUBROUTINE	ExciseRamArea
; Remove a block of RAM used from the free RAM list.
; Optionally make corresponding ROM table entries if this is 'ROM' code
; running in RAM.
; Enter with:
;	R0 = physical base address of area to excise
;	R1 = size of area to excise
;	R2 = linear base address of code block
;		 (only required for ROM entry creation)
;	R9 = pointer to first SRamBank with some free RAM
;	R10 = pointer to super page
;	R11 = pointer to next SRomBank to be filled in if needed
;		= 0 if filling in ROM blocks is not required
;	R13 = stack pointer
;
; Exit with:
;	R9 = pointer to first nonempty SRamBank following excised region
;		 may point to SRamBank list terminator
;	R11 = incremented past any newly created SRomBank entries
;	All other registers unmodified
;*******************************************************************************
	EXPORT	ExciseRamArea
ExciseRamArea	ROUT
		STMFD	sp!, {r0-r8,lr}
		DWORD	r0, "ExciseRamArea Base"
		DWORD	r1, "ExciseRamArea Size"
		DWORD	r2, "ExciseRamArea LinB"
		MOV		lr, r2
1		LDMIA	r9!, {r2,r3}				; r2=base, r3=size
		DWORD	r2, "ERA1 base"
		DWORD	r3, "ERA1 size"
		CMP		r3, #0
		FAULT	EQ							; fault if reached end of RAM list
		SUBS	r4, r0, r2					; r4 = area base - RAM base
		FAULT	LO							; if RAM base > area base, fault
		CMP		r4, r3						; check if area begins in this bank
		BHS		%BT1						; if not, try next RAM bank
		SUB		r9, r9, #8					; point r9 to bank

; at this point r9 -> RAM bank
; r2 = RAM bank base, r3 = RAM bank size
; r4 = offset into RAM bank of first byte to be excised
; r1 = size of remaining area to be excised
ExciseRamArea9
		SUB		r5, r3, r4					; r5 = remaining size of RAM bank
		CMP		r5, r1
		MOVHI	r5, r1						; r5 = amount of this bank to take
		DWORD	r4, "ERA BankOffset"
		DWORD	r5, "ERA Take"

		CMP		r11, #0
		BEQ		%FT8						; skip if SRomBank creation not required
; make ROM table entry
		STR		r0, [r11, #SRomBank_iBase]
		STR		r5, [r11, #SRomBank_iSize]
		DWORD	r0, "RomBank base"
		DWORD	r5, "RomBank size"
		DWORD	lr, "RomBank linB"
		LDR		r6, =((ERomTypeRamAsRom<<8)+5)	; 32 bit wide, RAM as ROM
		STR		r6, [r11, #SRomBank_iWidth]
		STR		lr, [r11, #SRomBank_iLinBase]
		ADD		lr, lr, r5
		ADD		r11, r11, #SRomBank_sz
8

; adjust RAM banks
		CMP		r4, #0
		BEQ		%FT2						; skip if we start at beginning of bank
		STR		r4, [r9, #SRamBank_iSize]	; else adjust bank size
		SUB		r3, r3, r4
		SUBS	r3, r3, r5					; r3 = amount of bank remaining
		ADDEQ	r9, r9, #SRamBank_sz		; if nothing left, step to next RAM bank
		BEQ		%FT3						; and branch out
		DWORD	r3, "ECB2 remain"

; need to add a new RAM bank
		ADD		r8, r9, #4
4		LDR		r7, [r8, #8]!
		CMP		r7, #0
		BNE		%BT4						; loop until r8 points to terminator + 4
		SUB		r8, r8, #4
5		LDMIA	r8!, {r6,r7}
		STMIA	r8, {r6,r7}					; move bank
		SUB		r8, r8, #16
		CMP		r8, r9						; reached original bank?
		BHI		%BT5						; if not, do next
		ADD		r9, r9, #8					; where to put new bank

6		ADD		r0, r0, r5					; new bank base
		DWORD	r0, "ERA2 new base"
		DWORD	r3, "ERA2 new size"
		STMIA	r9, {r0,r3}					; store new bank info, r9 updated
		LDMFD	sp!, {r0-r8,pc}

; excision area begins at start of bank
2		SUBS	r3, r3, r5					; r3 = amount of bank remaining
		BNE		%BT6						; if nonzero branch to adjust bank base

; entire bank is to be excised - remove it from the list
		MOV		r8, r9
7		LDR		r6, [r8, #8]
		LDR		r7, [r8, #12]
		STMIA	r8!, {r6,r7}
		CMP		r7, #0
		BNE		%BT7

; nothing remains of this bank
3		SUBS	r1, r1, r5					; adjust remaining excision size
		LDMEQFD	sp!, {r0-r8,pc}				; if all done, return
		LDR		r0, [r9, #SRamBank_iBase]	; adjust physical base
		LDMIA	r9, {r2,r3}
		MOV		r4, #0						; excision area continues into beginning of bank
		B		ExciseRamArea9


	IF	CFG_MMDirect

;*******************************************************************************
; Subroutine RamPhysicalToLinear
; Converts a physical RAM address to a linear RAM address using the list of
; physical RAM banks, assuming the direct memory model mapping scheme
;
; Enter with:
;		R0 = physical address to be translated
;		R10 points to super page
;		R12 points to ROM header
;		R13 points to stack
;
; Return with:
;		R0 = translated linear address
;		No other registers modified
;*******************************************************************************
	EXPORT	RamPhysicalToLinear
RamPhysicalToLinear ROUT
		DWORD	r0, "RamPhysicalToLinear in"
	IF	CFG_MMUPresent
		STMFD	sp!, {r1-r3,lr}
		LDR		lr, [r10, #SSuperPageBase_iRamBase]		; lr = linear base
		LDR		r2, [r10, #SSuperPageBase_iRamBootData]
		SUB		r2, r2, #SRamBank_sz	; step back to start
		MOV		r3, #0

1		ADD		r2, r2, #SRamBank_sz	; step to next block
		ADD		lr, lr, r3				; step linear address on
		LDR		r1, [r2, #SRamBank_iBase]	; r1 = physical address of this RAM bank
		LDR		r3, [r2, #SRamBank_iSize]	; r3 = size of this RAM bank
		CMP		r3, #0					; reached end of list?
		FAULT	EQ						; yes, fault
		CMP		r0, r1					; check if address >= physical base
		BLO		%BT1					; if not, next bank
		SUB		r0, r0, r1				; else r0 = offset from bank base
		CMP		r0, r3					; compare with size
		ADDHS	r0, r0, r1				; if >=size, restore r0 ...
		BHS		%BT1					; ... and do next bank
										; else it is this bank, lr = linear base
		ADD		r0, r0, lr				; r0 = phys addr - phys base + lin base
		DWORD	r0, "RamPhysicalToLinear out"
		LDMFD	sp!, {r1-r3,pc}
	ELSE
		MOV		pc, lr
	ENDIF

;*******************************************************************************
; Subroutine RamLinearToPhysical
; Converts a linear RAM address to a physical RAM address using the list of
; physical RAM banks, assuming the direct memory model mapping scheme
;
; Enter with:
;		R0 = linear address to be translated
;		R10 points to super page
;		R12 points to ROM header
;		R13 points to stack
;
; Return with:
;		R0 = translated physical address
;		No other registers modified
;*******************************************************************************
	EXPORT	RamLinearToPhysical
RamLinearToPhysical	ROUT
		DWORD	r0, "RamLinearToPhysical in"
	IF	CFG_MMUPresent
		STMFD	sp!, {r1-r3,lr}
		LDR		lr, [r10, #SSuperPageBase_iRamBase]		; lr = linear base
		LDR		r2, [r10, #SSuperPageBase_iRamBootData]
		SUB		r2, r2, #SRamBank_sz	; step back to start
		MOV		r3, #0

1		ADD		r2, r2, #SRamBank_sz	; step to next block
		ADD		lr, lr, r3				; step linear address on
		LDR		r3, [r2, #SRamBank_iSize]	; r3 = size of this RAM bank
		CMP		r3, #0					; reached end of list?
		FAULT	EQ						; yes, fault
		CMP		r0, lr					; check if address >= linear base
		BLO		%BT1					; if not, next bank
		SUB		r0, r0, lr				; else r0 = offset from bank linear base
		CMP		r0, r3					; compare with size
		ADDHS	r0, r0, lr				; if >=size, restore r0 ...
		BHS		%BT1					; ... and do next bank
		LDR		r1, [r2, #SRamBank_iBase]	; else it is this bank, r1 = physical base
		ADD		r0, r0, r1				; r0 = lin addr - lin base + phys base
		DWORD	r0, "RamLinearToPhysical out"
		LDMFD	sp!, {r1-r3,pc}
	ELSE
		MOV		pc, lr
	ENDIF

	ENDIF

;*******************************************************************************
; Subroutine RomPhysicalToLinear
; Converts a physical ROM address to a linear ROM address using the list of
; physical ROM banks.
;
; Enter with:
;		R0 = physical address to be translated
;		R10 points to super page
;		R12 points to ROM header
;		SSuperPageBase::iRomBootData points to valid ROM bank table.
;
; Return with:
;		R0 = translated linear address
;		R1,R2,R3 modified
;*******************************************************************************
	EXPORT	RomPhysicalToLinear
RomPhysicalToLinear ROUT
		DWORD	r0, "RomPhysicalToLinear in"
	IF	CFG_MMUPresent
		LDR		r2, [r10, #SSuperPageBase_iRomBootData]
		SUB		r2, r2, #SRomBank_sz	; step back to start

1		ADD		r2, r2, #SRomBank_sz	; step to next block
		LDR		r1, [r2, #SRomBank_iBase]	; r1 = physical address of this ROM bank
		LDR		r3, [r2, #SRomBank_iSize]		; r3 = size of this ROM bank
		CMP		r3, #0					; reached end of list?
		FAULT	EQ						; yes, fault
		CMP		r0, r1					; check if address >= physical base
		BLO		%BT1					; if not, next bank
		SUB		r0, r0, r1				; else r0 = offset from bank base
		CMP		r0, r3					; compare with size
		ADDHS	r0, r0, r1				; if >=size, restore r0 ...
		BHS		%BT1					; ... and do next bank
		LDR		r1, [r2, #SRomBank_iLinBase]	; else it is this bank, r1 = linear base
		ADD		r0, r0, r1				; r0 = phys addr - phys base + lin base
		DWORD	r0, "RomPhysicalToLinear out"
	ENDIF
		MOV		pc, lr


;*******************************************************************************
; Subroutine RomLinearToPhysical
; Converts a linear ROM address to a physical ROM address using the list of
; physical ROM banks.
;
; Enter with:
;		R0 = linear address to be translated
;		R10 points to super page
;		R12 points to ROM header
;		SSuperPageBase::iRomBootData points to valid ROM bank table.
;
; Return with:
;		R0 = translated physical address
;		R1,R2,R3 modified
;*******************************************************************************
	EXPORT	RomLinearToPhysical
RomLinearToPhysical	ROUT
		DWORD	r0, "RomLinearToPhysical in"
	IF	CFG_MMUPresent
		LDR		r2, [r10, #SSuperPageBase_iRomBootData]
		SUB		r2, r2, #SRomBank_sz	; step back to start

1		ADD		r2, r2, #SRomBank_sz	; step to next block
		LDR		r1, [r2, #SRomBank_iLinBase]	; r1 = linear address of this ROM bank
		LDR		r3, [r2, #SRomBank_iSize]		; r3 = size of this ROM bank
		CMP		r3, #0					; reached end of list?
		FAULT	EQ						; yes, fault
		CMP		r0, r1					; check if address >= linear base
		BLO		%BT1					; if not, next bank
		SUB		r0, r0, r1				; else r0 = offset from bank base
		CMP		r0, r3					; compare with size
		ADDHS	r0, r0, r1				; if >=size, restore r0 ...
		BHS		%BT1					; ... and do next bank
		LDR		r1, [r2, #SRomBank_iBase]	; else it is this bank, r1 = physical base
		ADD		r0, r0, r1				; r0 = lin addr - lin base + phys base
		DWORD	r0, "RomLinearToPhysical out"
	ENDIF
		MOV		pc, lr


;*******************************************************************************
; Subroutine FindPrimary
; Sets up r11 to point to TRomImageHeader for the primary file
;
; Enter with:
;		R10 points to super page
;		R12 points to ROM header
;		SSuperPageBase::iActiveVariant set up
;		SSuperPageBase::iRomBootData points to valid ROM bank table.
;
; Exit with:
;		R11 pointing to TRomImageHeader for primary file
;		SSuperPageBase::iPrimaryEntry set up with linear address of TRomEntry
;		for primary file.
;		R0-R4 modified
;*******************************************************************************
	EXPORT	FindPrimary
FindPrimary		ROUT
		MOV		r4, r14					; save return address in r4
		LDR		r0, [r12, #TRomHeader_iPrimaryFile]	; r0=linear address of TRomEntry
													; for first primary file
		DWORD	r0, "iPrimaryFile"
1		STR		r0, [r10, #SSuperPageBase_iPrimaryEntry]	; save linear address of TRomEntry
		BL		RomLinearToPhysical		; r0=physical address of TRomEntry
		LDR		r0, [r0, #TRomEntry_iAddressLin]	; r0=linear address of TRomImageHeader
		BL		RomLinearToPhysical		; r0=physical address of TRomImageHeader
		MOV		r11, r0					; into r11
		LDR		r3, [r10, #SSuperPageBase_iActiveVariant]		; r3 = active HWVD
		DWORD	r3, "Active HWVD"
		LDR		r0, [r11, #TRomImageHeader_iHardwareVariant]	; get hardware variant descriptor
		DWORD	r0, "File   HWVD"
		MOV		r1, r0, LSR #24			; r1 = layer
		CMP		r1, #3
		BLS		FoundPrimary			; if <=3, this one will do
		AND		r2, r0, #0x00ff0000		; r2 = parent << 16
		CMP		r2, #0x00030000
		BHI		%FT2					; skip if parent > 3
		AND		r2, r3, #0xff000000		; r2 = active CPU << 24
		CMP		r2, r1, LSL #24			; does it match this primary?
		BEQ		FoundPrimary			; if so, found it
3		LDR		r0, [r11, #TRomImageHeader_iNextExtension]	; no good, step to next
		CMP		r0, #0					; end of list?
		BNE		%BT1					; if not, try next primary
		FAULT							; else can't find primary :-(
2		EOR		r1, r0, r3				; check CPU + ASSP
		CMP		r1, #0x10000
		BHS		%BT3					; they don't match - try next image
		AND		r1, r0, r3				; they match - check variant mask
		MOVS	r1, r1, LSL #16
		BEQ		%BT3					; no good - try next image

FoundPrimary
		MOV		pc, r4					; found primary OK


		LTORG

;*******************************************************************************
; Subroutine CheckForExtensionRom
; Tests if a new-style extension ROM is present at a given address.
;
; Enter with:
;		R0 = address of suspected extension ROM
;		R10 points to super page
;		R12 points to kernel ROM header
;		R13 points to valid stack
;
; Return with:
;		R1 = 0, Z=1 if no extension ROM present
;		R1 = 1, Z=0 if extension ROM present
;		No other registers modified
;*******************************************************************************
	EXPORT	CheckForExtensionRom
CheckForExtensionRom	ROUT
		STMFD	sp!, {r2,r3,lr}
		DWORD	r0, "?ExtROM"
		LDR		r1, [r0,  #TExtensionRomHeader_iKernelVersion]
		LDR		r2,	[r12, #TRomHeader_iVersion]
		CMP		r1, r2
		BNE		CheckForExtensionRom_Bad
		LDR		r1, [r0,  #TExtensionRomHeader_iKernelCheckSum]
		LDR		r2,	[r12, #TRomHeader_iCheckSum]
		CMP		r1, r2
		BNE		CheckForExtensionRom_Bad
		LDR		r2, [r0,  #TExtensionRomHeader_iKernelTime]
		LDR		r1,	[r12, #TRomHeader_iTime]
		CMP		r1, r2
		BNE		CheckForExtensionRom_Bad
		LDR		r3, [r0,  #TExtensionRomHeader_iKernelTime+4]
		LDR		r1,	[r12, #TRomHeader_iTime+4]
		CMP		r1, r3
		BNE		CheckForExtensionRom_Bad
		LDR		r1, [r0,  #TExtensionRomHeader_iTime]
		SUBS	r1, r1, r2
		LDR		r2, [r0,  #TExtensionRomHeader_iTime+4]
		SBCS	r2, r2, r3									; r3:r2 = time - kernel time
		BLT		CheckForExtensionRom_Bad					; if time<kernel time, no good
		DWORD	r0, "ExtROM"
		MOVS	r1, #1										; if time>=kernel time, OK
		LDMFD	sp!, {r2,r3,pc}
CheckForExtensionRom_Bad
		DWORD	r0, "Not ExtROM"
		MOVS	r1, #0
		LDMFD	sp!, {r2,r3,pc}

;*******************************************************************************
; Subroutine SetupSuperPageRunningFromRAM
; Calculates the address of the super page if we are running from RAM and we
; want the super page to come after the ROM image. Takes account of extension
; ROMs.
;
; Enter with:
;		R12 points to kernel ROM header, which is also the physical base 
;		address of the ROM in RAM
;		Stack not required
;
; Return with:
;		R10 contains calculated address of super page
;		R1-R3 modified
;		No other registers modified
;*******************************************************************************
	EXPORT	SetupSuperPageRunningFromRAM
SetupSuperPageRunningFromRAM	ROUT

	IF	:LNOT: :DEF: CFG_SupportEmulatedRomPaging
		LDR		r1, [r12, #TRomHeader_iPageableRomStart]; size of unpaged part of ROM
		CMP		r1, #0
		BEQ		SetupSuperPageRunningFromRAM_UnPagedRom
													; Paged ROM so ensure superpage
		ADD		r10, r12, r1						; at the end of unpaged ROM
		MVN		r1, #0								; rounded up to ROM alignment
		ADD		r10, r10, r1, lsr #(32 - CFG_RomSizeAlign)
		BIC		r10, r10, r1, lsr #(32 - CFG_RomSizeAlign)
		B		SetupSuperPageRunningFromRAM_CheckExtensionRomHeader 
	ENDIF

SetupSuperPageRunningFromRAM_UnPagedRom
		LDR		r10, [r12, #TRomHeader_iUncompressedSize]; r10 = base ROM size
		ADD		r10, r10, r12							; r10 = address after base ROM
		MVN		r1, #0								; rounded up to ROM alignment
		ADD		r10, r10, r1, lsr #(32 - CFG_RomSizeAlign)
		BIC		r10, r10, r1, lsr #(32 - CFG_RomSizeAlign)

SetupSuperPageRunningFromRAM_CheckExtensionRomHeader	; don't use CheckForExtensionRom as it uses stack

; r10 = Physical base address of Extension ROM header which is equivalent to 
; 		the end of 'normal' ROM in RAM

		LDR		r1, [r10,  #TExtensionRomHeader_iKernelVersion]
		LDR		r2,	[r12, #TRomHeader_iVersion]
		CMP		r1, r2
		BNE		%FT10
		LDR		r1, [r10,  #TExtensionRomHeader_iKernelCheckSum]
		LDR		r2,	[r12, #TRomHeader_iCheckSum]
		CMP		r1, r2
		BNE		%FT10
		LDR		r2, [r10,  #TExtensionRomHeader_iKernelTime]
		LDR		r1,	[r12, #TRomHeader_iTime]
		CMP		r1, r2
		BNE		%FT10
		LDR		r3, [r10,  #TExtensionRomHeader_iKernelTime+4]
		LDR		r1,	[r12, #TRomHeader_iTime+4]
		CMP		r1, r3
		BNE		%FT10
		LDR		r1, [r10,  #TExtensionRomHeader_iTime]
		SUBS	r1, r1, r2
		LDR		r2, [r10,  #TExtensionRomHeader_iTime+4]
		SBCS	r2, r2, r3									; r3:r2 = time - kernel time
		BLT		%FT10										; if time<kernel time, no good
		LDR		r1, [r10, #TExtensionRomHeader_iUncompressedSize]	; r1 = extension ROM size
		ADD		r10, r10, r1								; step super page past extension ROM
		MVN		r1, #0										; rounded up to ROM alignment
		ADD		r10, r10, r1, lsr #(32 - CFG_RomSizeAlign)
		BIC		r10, r10, r1, lsr #(32 - CFG_RomSizeAlign)	; r10 = RAM phys addr end of unpaged ROM
10
		MOV		pc, lr

;*******************************************************************************
; Subroutine RelocateSuperPage
; Relocate the super page and CPU page
;
; Enter with:
;		R0	= Final address of super page
;		R10	= Current address of super page
;
; Return with:
;		R10	= Initial R0
;		R13 updated if necessary
;*******************************************************************************
	EXPORT	RelocateSuperPage
RelocateSuperPage	ROUT
		DWORD	r10, "RelocateSuperPage Initial"
		DWORD	r0, "RelocateSuperPage Destination"
		DWORD	r13, "RelocateSuperPage Initial SP"
		MOV		r7, lr
		MOV		r9, r0						; save final address
		MOV		r2, #CpuBootStackTop		; size
		ADD		r11, r10, r2				; upper limit for bounds check
		SUB		r8, r9, r10					; r8 = final - initial address

; Copy SSuperPageBase valid fields
; Leave rest of super page since it should be preserved on warm reset
		ADRL	r4, %FT4					; pointer to offset table
1		LDR		r3, [r4], #4				; offset + flags
		MSR		cpsr_f, r3					; flags into NZCV
		BIC		r3, r3, #0xf0000000			; clear flags
		BMI		%FT2						; end of list
		LDR		r0, [r10, r3]				; get old super page value
		BVC		%FT3						; skip if copy required
		CMP		r0, r10						; >=initial super page base?
		CMPHS	r11, r0						; if so, is upper bound>pointer?
		ADDHI	r0, r0, r8					; if so, add offset from initial to final address
3		STR		r0, [r9, r3]				; store new super page value
		B		%BT1						; next

; copy the entire CPU page
2		ADD		r0, r10, #CpuPageOffset		; source
		ADD		r1, r9, #CpuPageOffset		; destination
		MOV		r2, #CpuBootStackTop-CpuPageOffset	; size
		BL		WordMove					; copy data

; relocate the stack pointer if necessary
		CMP		sp, r10						; SP>=initial super page base?
		CMPHS	r11, sp						; if so, is upper bound>SP?
		ADDHI	sp, sp, r8					; if so, add offset from initial to final address

; Change to new super page location
		MOV		r10, r9						;
		DWORD	r10, "RelocateSuperPage Final"
		DWORD	r13, "RelocateSuperPage Final SP"
		MOV		r0, r8						; super page offset
		MOV		r2, #BMA_Reloc
		BOOTCALL	BTF_Alloc				; fix up allocator
		MOV		pc, r7

4
		DCD		SSuperPageBase_iTotalRomSize
		DCD		SSuperPageBase_iTotalRamSize
		DCD		SSuperPageBase_iRamBootData + 0x10000000
		DCD		SSuperPageBase_iRomBootData + 0x10000000
		DCD		SSuperPageBase_iBootTable + 0x10000000
		DCD		SSuperPageBase_iCodeBase
		DCD		SSuperPageBase_iBootFlags
		DCD		SSuperPageBase_iBootAlloc + 0x10000000
		DCD		SSuperPageBase_iMachineData + 0x10000000
		DCD		SSuperPageBase_iActiveVariant
		DCD		SSuperPageBase_iPrimaryEntry
		DCD		SSuperPageBase_iDebugPort
		DCD		SSuperPageBase_iCpuId
		DCD		SSuperPageBase_iRootDirList
		DCD		SSuperPageBase_iHwStartupReason
		DCD		SSuperPageBase_iKernelLimit
		DCD		SSuperPageBase_iRamBase
		DCD		SSuperPageBase_iRomHeaderPhys
		DCD		SSuperPageBase_iAPBootPagePhys
		DCD		SSuperPageBase_iAPBootPageLin
		DCD		SSuperPageBase_iAPBootPageDirPhys
		DCD		SSuperPageBase_iSmrData + 0x10000000
		DCD		-1


;*******************************************************************************
; ROM autodetection code
;*******************************************************************************

		IF		CFG_AutoDetectROM

;*******************************************************************************
; Test for ROM on lower 16 bits of [r8]
; r0-r7, r12 modified.
; MOV instructions selected to differ considerably in lower 16 bits
; Must be run with instruction caching disabled
;*******************************************************************************
RomTestLower16	ROUT
		LDR		r7, [r8]
		MOV		r12, #0x000AA000
		MOV		r12, #0x000AA000
		MOV		r12, #0x000AA000
		LDR		r1, [r8]
		MOV		r5, #0x15400000
		MOV		r5, #0x15400000
		MOV		r5, #0x15400000
		LDR		r2, [r8]
		MOV		r12, #0x0000DB00
		MOV		r12, #0x0000DB00
		MOV		r12, #0x0000DB00
		LDR		r4, [r8]
		MOV		r3, #0xCC000000
		MOV		r3, #0xCC000000
		MOV		r3, #0xCC000000
		LDR		r3, [r8]
		MOV		r0, #0x0000000F
		MOV		r0, #0x0000000F
		MOV		r0, #0x0000000F
		LDR		r5, [r8]
		MOV		r12, #0x000000F0
		MOV		r12, #0x000000F0
		MOV		r12, #0x000000F0
		LDR		r6, [r8]
		MOV		r0, #0x00000000
		MOV		r0, #0x00000000
		MOV		r0, #0x00000000
		LDR		r0, [r8]
		MOV		r12, #0x000003FC
		MOV		r12, #0x000003FC
		MOV		r12, #0x000003FC
		MOV		r12, #0xFF
		ORR		r12, r12, #0xFF00
		AND		r0, r0, r12
		AND		r1, r1, r12
		AND		r2, r2, r12
		AND		r3, r3, r12
		AND		r4, r4, r12
		AND		r5, r5, r12
		AND		r6, r6, r12
		AND		r7, r7, r12
		B		RomCompareValues

;*******************************************************************************
; Test for ROM on upper 16 bits of [r8]
; r0-r7, r12 modified
; strange instructions selected to differ considerably in upper 16 bits
; Must be run with instruction caching disabled
;*******************************************************************************
RomTestUpper16	ROUT
		LDR		r0, [r8]
		ADCGE	r12, r10, #0
		ADCGE	r12, r10, #0
		ADCGE	r12, r10, #0
		LDR		r1, [r8]
		CMPPL	R5, R0
		CMPPL	R5, R0
		CMPPL	R5, R0
		LDR		r2, [r8]
		TEQCCS	R3, #0
		TEQCCS	R3, #0
		TEQCCS	R3, #0
		LDR		r3, [r8]
		SBCGT	r12, r12, r12
		SBCGT	r12, r12, r12
		SBCGT	r12, r12, r12
		LDR		r4, [r8]
		RSCS	r12, r0, r12
		RSCS	r12, r0, r12
		RSCS	r12, r0, r12
		LDR		r5, [r8]
		TSTEQ	r14, #0
		TSTEQ	r14, #0
		TSTEQ	r14, #0
		LDR		r6, [r8]
		ANDEQ	r12, r0, r0
		ANDEQ	r12, r0, r0
		ANDEQ	r12, r0, r0
		LDR		r7, [r8]
		MVNS	r12, #0
		MVNS	r12, #0
		MVNS	r12, #0
		MOV		r0, r0, LSR #16
		MOV		r1, r1, LSR #16
		MOV		r2, r2, LSR #16
		MOV		r3, r3, LSR #16
		MOV		r4, r4, LSR #16
		MOV		r5, r5, LSR #16
		MOV		r6, r6, LSR #16
		MOV		r7, r7, LSR #16

;*******************************************************************************
; Compare r0-r7.
; If all r0-r7 are equal, return r12 = 0xC0000000, sign flag set, zero flag clear
; If all r0-r7 are equal in the lower 8 bits but unequal in the other bits,
;		return r12 = 0x40000000, sign flag clear, zero flag clear
; If r0-r7 differ in the lower 8 bits, return r12=0, sign flag clear, zero flag set
; r0-r7, r12 modified.
;*******************************************************************************
RomCompareValues	ROUT
		EOR		r1, r0, r1
		EOR		r2, r0, r2
		EOR		r3, r0, r3
		EOR		r4, r0, r4
		EOR		r5, r0, r5
		EOR		r6, r0, r6
		EOR		r7, r0, r7
		ORR		r0, r1, r2
		ORR		r0, r0, r3
		ORR		r0, r0, r4
		ORR		r0, r0, r5
		ORR		r0, r0, r6
		ORRS	r0, r0, r7
		MOVEQ	r12, #0xc0000000
		MOVNE	r12, #0x40000000
		TST		r0, #0xff
		MOVNE	r12, #0
		MOVS	r12, r12
		MOV		pc, lr

;*******************************************************************************
; Calculate the size of a ROM bank by checking for ghost images.
; Assume size is a power of 2 and is >=1Mb
;
; Enter with
;		R1 = base address of ROM bank
;		R2 = maximum possible size of ROM bank (must be a power of 2)
;		Wait states sufficient and bus width correct for ROM
;
; Return with
;		R0 = size of ROM in bytes
;		R3-R6 modified
;*******************************************************************************
	EXPORT	FindRomBankSize
FindRomBankSize	ROUT
		MOV		r0, r2, LSR #1				; assume maximum size first - check for ghost at half that
1		ADD		r3, r1, r0					; r3 = address of ghost image to be tested for
		LDR		r4, =RomGhostOffset+RomGhostSize-4
2		LDR		r5, [r1, r4]				; fetch word from ROM
		LDR		r6, [r3, r4]				; fetch word from possible ghost image
		CMP		r5, r6						; compare them
		ADDNE	r0, r0, r0					; if different, not a ghost
		MOVNE	pc, lr						; so return 2*offset tested
		SUB		r4, r4, #4					; next word
		CMP		r4, #RomGhostOffset			;
		BHS		%BT2						; loop until all checked
		CMP		r0, #0x100000				; ghost found - reached minimum size?
		MOVHI	r0, r0, LSR #1				; if not, halve offset to check
		BHI		%BT1						; and recheck for ghost
		MOV		pc, lr						; else return min size

;*******************************************************************************
;
; Determine the presence or absence of ROM at a given location, and its bus width.
; This is done by doing several reads of the same ROM location with different
; instruction prefetches preceding them. Data bus lines which are not driven
; during a read cycle will tend to retain the last value which was driven onto
; the bus, due to the capacitive loading of the data bus. Hence the same location
; is unlikely to return the same value for all reads if there is no ROM present.
; Several locations are read to increase the probability of detecting the absence
; of ROM. The lower 8 bits, lower 16 bits and upper 16 bits of the returned values
; are analysed separately to determine which data bus lines are being driven
; and hence the width of the ROM.
;
; Enter with
;		R0 = base address of ROM bank
;		R13 points to valid stack
;		Wait states sufficient for ROM and bus width set to 32 bits
;		Instruction caching disabled
;		
; Return with
;		R0=-1 if no ROM present
;		R0=0 if 32-bit ROM present
;		R0=1 if 16-bit ROM present
;		R0=2 if 8-bit ROM present
;		No other registers modified
;*******************************************************************************
	EXPORT	CheckROMPresent
CheckROMPresent	ROUT
		STMFD	sp!, {r1-r12,r14}
		MOV		r8, r0					; ROM address into R8
		MOV		r11, #0					; Initialise return value
		MOV		r10, #64				; check 64 locations in the ROM

1		BL		RomTestLower16			; check if ROM present on lower 16 or lower 8 bits
		MVNEQ	r11, #0					; if no ROM on lower 8 bits, set r11 = -1 ...
		BEQ		%FT2					; ... and exit
		MOVPL	r11, #2					; if ROM on bits 0-7 but not 8-15, 8 bit wide
		BPL		%FT3					; if no ROM on bits 8-15, don't bother testing 16-31
		TST		r11, #2					; test 8 bit flag - if set, can't be more than 8 bits
		BNE		%FT3					; wide, so don't bother testing bits 16-31
		BL		RomTestUpper16			; ROM is present on bits 0-15, so test bits 16-31
		ORRPL	r11, r11, #1			; if no ROM present on bits 16-31, 16 bit wide
3		ADD		r8, r8, #60				; increment address to next word
		SUBS	r10, r10, #1			;
		BNE		%BT1					;
2		MOV		r0, r11					; result into R0
		LDMFD	sp!, {r1-r12,pc}


		LTORG

		ENDIF	; CFG_AutoDetectROM

;*******************************************************************************
; MMU code
;*******************************************************************************

		IF		CFG_MMUPresent
		IF		:LNOT: CFG_MMDirect

;*******************************************************************************
; Allocate memory and map it to consecutive linear addresses
; Enter with
;	R0 = base virtual address
;	R2 = permissions (offset in boot table)
;	R3 = total size of mapping required
;	R4 = log2(maximum page size to consider)
;	R10 points to super page
;	R13 points to a valid stack
;
; Return with
;	all registers unmodified
;
;*******************************************************************************
	EXPORT	AllocAndMap
AllocAndMap	ROUT
		DWORD	r0, "AllocAndMap virt"
		DWORD	r2, "AllocAndMap perm"
		DWORD	r3, "AllocAndMap size"
		DWORD	r4, "AllocAndMap page"
		STMFD	sp!, {r0-r5,lr}
		MVN		r2, #0
		BIC		r5, r0, r2, LSR #20		; next linear address
		ADD		r3, r3, r2, LSR #20		; r3 = size rounded up to 4K
		BIC		r3, r3, r2, LSR #20		; = remaining size
1		MOV		r4, #12					; start with 4K
	IF	:LNOT: CFG_MMFlexible	; don't use large pages for flexible memory model
		CMP		r3, #0x00010000			; size >= 64K?
		BLO		%FA2					; if <64K, use 4K pages
		TST		r5, #0x0000f000			; VA multiple of 64K?
		BNE		%FA2					; if not, use 4K pages
		MOV		r4, #16					; go to 64K pages
	ENDIF
		CMP		r3, #0x00100000			; size >= 1Mb ?
		BLO		%FA2					; if <1Mb, use 64K pages
		TST		r5, #0x000ff000			; VA multiple of 1Mb?
		MOVEQ	r4, #20					; if it is, go to 1Mb pages
2		LDR		r2, [sp, #16]			; log2(page size)
		CMP		r4, r2
		MOVHI	r4, r2					; limit to specified page size
		MOV		r2, #BMA_Kernel			; allocate general kernel RAM
		BOOTCALL	BTF_Alloc			; allocate RAM, physical address -> R0
										; reduces R4 if necessary
		MOV		r1, r0					; into R1
		MOV		r0, r5					; linear address
		LDR		r2, [sp, #8]			; permissions back into R2
		BL		MapSingleEntry			; map
		MOV		r5, r0					; save incremented linear address
		CMP		r3, #0					; done?
		BGT		%BA1					; if not, loop
		LDMFD	sp!, {r0-r5,pc}

		ENDIF ; CFG_MMDirect

;*******************************************************************************
; Map a contiguous region
; Enter with
;	R0 = virtual address
;	R1 = physical address
;	R2 = permissions (offset in boot table)
;	R3 = total size of mapping required
;	R4 = log2(maximum page size to consider)
;	R10 points to super page
;	R13 points to a valid stack
;
; Return with
;	all registers unmodified
;
;*******************************************************************************
	EXPORT	MapContiguous
MapContiguous ROUT
		DWORD	r0, "MapCont virt"
		DWORD	r1, "MapCont phys"
		DWORD	r2, "MapCont perm"
		DWORD	r3, "MapCont size"
		DWORD	r4, "MapCont page"
		STMFD	sp!, {r0,r1,r3,lr}
1		BL		MapSingleEntry
		CMP		r3, #0
		BGT		%BT1
		LDMFD	sp!, {r0,r1,r3,pc}


;*******************************************************************************
; Make a single MMU mapping of either 4K, 64K or 1Mb
; Enter with
;	R0 = virtual address
;	R1 = physical address
;	R2 = permissions (offset in boot table)
;	R3 = total size of mapping required
;	R4 = log2(maximum page size to consider)
;	R10 points to super page
;	R13 points to a valid stack
;
; Return with
;	R0, R1 incremented by size of entry made
;	R3 decremented by size of entry made
;	other registers unmodified
;
;*******************************************************************************
MapSingleEntry	ROUT
		DWORD	r0, "MapSglE virt"
		DWORD	r1, "MapSglE phys"
		DWORD	r2, "MapSglE perm"
		DWORD	r3, "MapSglE size"
		DWORD	r4, "MapSglE page"
		STMFD	sp!, {r2,r4-r9,r11,r12,lr}
		ORR		r5, r0, r1					; r5 = linear OR physical
		MOVS	r5, r5, LSL #12				; if linear or physical not multiple of 1Mb ...
		BNE		MapSinglePage				; ... use pages
		CMP		r3, #0x00100000
		CMPHS	r4, #20
		BLO		MapSinglePage				; if size<1Mb or don't want section, use page

		; Use section
MapSingleSection
		MOV		r4, #20
		BOOTCALL	BTF_GetPdePerm			; PDE permissions into R2
		ORR		r2, r2, r1					; PDE for mapping
		LDR		r8, [r10, #SSuperPageBase_iPageDir]
		ADD		r8, r8, r0, LSR #18			; address in page dir
		DWORD	r8, "MSS: PDA"
		DWORD	r2, "MSS: PDE"
		LDR		r5, [r8]
		CMP		r5, #0
		FAULT	NE							; don't overwrite existing PDE
		
		STR		r2, [r8]					;write-down a new page directory entry
		
		; Update main memory after page table change
		MOV		r5, r0	; save r0
		MOV		r6, r1	; save r1
		MOV		r0, r8	; Address
		MOV 	r1, #4	; Size
		BOOTCALL	BTF_PTUpdate
		
		;update entry values
		ADD		r0, r5, #0x00100000
		ADD		r1, r6, #0x00100000
		SUB		r3, r3, #0x00100000
		LDMFD	sp!, {r2,r4-r9,r11,r12,pc}

MapSinglePage
		MOV		r4, #12
		BOOTCALL	BTF_GetPdePerm			; PDE permissions for page table into R2
		BL		LookupPageTable				; find page table
		CMN		r7, #1
		BLEQ	NewPageTable				; if no page table, allocate a new one and map it

		; Now have r8 = page table physical address
		; r7 = page table access address
		LDMIA	sp, {r2,r4}					; recover stored R2 (permissions) and R4 (max page size)
	IF	CFG_MMFlexible
		B		MapSingleSmallPage			; don't use large pages for flexible memory model
	ENDIF
		CMP		r3, #0x00010000
		CMPHS	r4, #16
		BLO		MapSingleSmallPage			; map size < 64k or small page specified so use small page
		MOVS	r5, r5, LSL #4				; zero if 64k mapping possible
		BNE		MapSingleSmallPage			; else use small page
		MOV		r4, #16
		BOOTCALL	BTF_GetPtePerm			; r2 = PTE permissions for 64k page
		AND		r5, r0, #0xf0000			; page table offset << 12
		ADD		r7, r7, r5, LSR #10
		ORR		r2, r2, r1					; PTE

		DWORD	r7, "MLP: PTA"
		DWORD	r2, "MLP: PTE"
		MOV		r8, r7	; r8 = the address of the first entry
MapLargePage
		LDR		r5, [r7]
		CMP		r5, #0
		FAULT	NE							; don't overwrite existing PTE
		STR		r2, [r7], #4				; write-down a page table entry
		SUBS	r4, r4, #1
		BNE		MapLargePage
		
		; Update main memory after page table change
		MOV		r5, r0	; save r0	
		MOV		r6, r1	; save r1
		MOV		r0, r8	; r0 = the address of the first entry
		MOV 	r1, #64	; Size. Large page table requires 16 entries - which makes 64 bytes
		BOOTCALL	BTF_PTUpdate

		ADD		r0, r5, #0x00010000
		ADD		r1, r6, #0x00010000
		SUB		r3, r3, #0x00010000
		LDMFD	sp!, {r2,r4-r9,r11,r12,pc}

MapSingleSmallPage
		MOV		r4, #12
		BOOTCALL	BTF_GetPtePerm			; r2 = PTE permissions for 4k page
		AND		r5, r0, #0xff000			; page table offset << 12
		ADD		r7, r7, r5, LSR #10
		ORR		r2, r2, r1					; PTE
		DWORD	r7, "MSP: PTA"
		DWORD	r2, "MSP: PTE"
		LDR		r5, [r7]
		CMP		r5, #0
		FAULT	NE							; don't overwrite existing PTE

		STR		r2, [r7]					;write-down a new page table entry

		; Update main memory after page table change
		MOV		r5, r0	; save r0	
		MOV		r6, r1	; save r1
		MOV		r0, r7	; Address
		MOV 	r1, #4	; Size
		BOOTCALL	BTF_PTUpdate
		
		;update entry values
		ADD		r0, r5, #0x00001000
		ADD		r1, r6, #0x00001000
		SUB		r3, r3, #0x00001000
		LDMFD	sp!, {r2,r4-r9,r11,r12,pc}



;*******************************************************************************
; Check if a virtual address is mapped by a page table
; Enter with
;	R0 = virtual address
;	R2 = PDE permissions for page table (for comparison)
;	R10 points to super page
;	R13 points to a valid stack
;
; Return with
;	R8	= physical address of page table
;		= -1 if none
;	R7	= R8 if MMU currently disabled
;		= virtual address of page table if MMU currently enabled
;		= -1 if no page table
;	other registers unmodified
;
;*******************************************************************************
LookupPageTable	ROUT
		DWORD	r0, "LookupPT virt"
		DWORD	r2, "LookupPT perm"
		LDR		r8, [r10, #SSuperPageBase_iPageDir]
		MOV		r7, r0, LSR #20
		ADD		r8, r8, r7, LSL #2
		LDR		r8, [r8]
		MVN		r7, #0
		EOR		r8, r8, r2
		TST		r8, r7, LSR #22				; 0 if permissions correct
		MVNNE	r8, #0						; if not (or no page table present)
		MOVNE	pc, lr						; return with r7=r8=-1
		AND		r8, r8, r7, LSL #10			; r8 = pt phys addr
		MRC		p15, 0, r7, c1, c0, 0		; r7 = MMUCR
		TST		r7, #1						; MMU enabled?
		BNE		FindPageTableVA
		DWORD	r8, "LookupPT phys=access"
		MOV		r7, r8						; if not, r7=r8
		MOV		pc, lr						; and return

		; need to find virtual address of page table
		; assume first page table (PT0) maps page tables
FindPageTableVA	ROUT
		DWORD	r8, "FindPageTableVA phys"
		STMFD	sp!, {r0,lr}
	IF	CFG_MMDirect
		MOV		r0, r8
		BL		RamPhysicalToLinear			; for direct model, RAM mapping is known
		MOV		r7, r0
	ELSE
		LDR		r7, =KPageTableBase			; r7 = PT0 linear address
		MRC		p15, 0, r0, c1, c0, 0
		TST		r0, #MMUCR_M							; MMU enabled?
		LDREQ	r0, [r10, #SSuperPageBase_iPageDir]		; if not, r0 = page directory phys addr ...
		MOVEQ	r7, r7, LSR #20							; ... r7 = PDE index for PT0 ...
		LDREQ	r7, [r0, r7, LSL #2]					; ... r7 = PDE for PT0 ...
		MOVEQ	r7, r7, LSR #10
		MOVEQ	r7, r7, LSL #10				; ... r7 = PT0 physical address
		MOV		r0, #0						; r0 = page table index * 4
1		LDR		lr, [r7, r0]				; lr = PT0[index]
		EOR		lr, lr, r8					; XOR with desired physical address
		MOVS	lr, lr, LSR #12				; zero if page frame address matches
		ADDNE	r0, r0, #4					; if not, next entry
		CMPNE	r0, #1024					; only check 256 entries
		BNE		%BT1
		CMP		lr, #0						; did physical address match ?
		FAULT	NE							; if not, not found
		LDR		r7, =KPageTableBase			; r7 = page table linear base
		ADD		r7, r7, r0, LSL #10			; r7 = virtual address of page table page
		AND		r0, r8, #0xc00				; r0 = bits 10, 11 of page table physical address
		ADD		r7, r7, r0					; r7 = virtual address of page table
	ENDIF
		DWORD	r7, "FindPageTableVA virt"
		LDMFD	sp!, {r0,pc}


;*******************************************************************************
; Allocate a new page table and map a virtual address with it
; Enter with
;	R0 = virtual address
;	R2 = PDE permissions for page table
;	R10 points to super page
;	R13 points to a valid stack
;
; Return with
;	R8	= physical address of page table
;	R7	= R8 if MMU currently disabled
;		= virtual address of page table if MMU currently enabled
;	other registers unmodified
;
;*******************************************************************************
NewPageTable	ROUT
		DWORD	r0, "NewPageTable virtual"
		DWORD	r2, "NewPageTable PDEperm"
		STMFD	sp!, {r0-r5,lr}
		MOV		r2, #BMA_PageTable
		BOOTCALL	BTF_Alloc				; get physical address of page table into R0
		MOV		r8, r0						; into r8
		LDR		r2, [sp, #8]				; recover saved r2
		LDR		r0, [sp, #0]				; recover saved r0
		ORR		r2, r2, r8					; PDE for mapping
		LDR		r1, [r10, #SSuperPageBase_iPageDir]
		MOV		r0, r0, LSR #20
		ADD		r1, r1, r0, LSL #2
		LDR		r7, [r1]
		DWORD	r7, "NPT: OldPDE"
		DWORD	r1, "NPT: PDA"
		DWORD	r2, "NPT: PDE"
		CMP		r7, #0
		FAULT	NE							; don't overwrite existing PDE

		STR		r2, [r1]					; put in PDE

		; Update main memory after page table change
		MOV		r0, r1 ; address
		MOV		r1, #4 ; the size of a single entry
		BOOTCALL	BTF_PTUpdate

		MRC		p15, 0, r3, c1, c0, 0		; r3 = MMUCR
		TST		r8, #0xc00					; first page table in a page?
		BEQ		%FA3						; skip if it is
		TST		r3, #MMUCR_M				; MMU on?
		LDMFD	sp!, {r0-r5,lr}				; if it is, ...
		BNE		FindPageTableVA				; ... find VA and exit
		DWORD	r8, "NewPageTable phys=access"
		MOV		r7, r8
		MOV		pc, lr						; else return with r7=r8

	IF	CFG_MMDirect
3		TST		r3, #MMUCR_M				; MMU on?
		MOVEQ	r7, r8						; if not, r7=r8
		BLNE	FindPageTableVA				; if on, use known mapping to find VA
		DWORD	r8, "NewPageTable phys"
		DWORD	r7, "NewPageTable virt"
		MOV		r0, r7
		MOV		r1, #0x1000
		MOV		r2, #0
		BL		WordFill					; clear entire page
		BOOTCALL	BTF_PTUpdate
		LDMFD	sp!, {r0-r5,pc}

	ELSE
		; find next free VA for page table
3		LDR		r7, =KPageTableBase						; r7 = page table linear base = PT0 linear
		TST		r3, #MMUCR_M							; MMU on ?
		LDREQ	r1, [r10, #SSuperPageBase_iPageDir]		; if not, r1 = page directory phys ...
		MOVEQ	r7, r7, LSR #20							; ... r7 = PDE index for page tables ...
		LDREQ	r7, [r1, r7, LSL #2]					; ... r7 = PDE for page tables ...
		MOVEQ	r7, r7, LSR #10							;
		MOVEQ	r7, r7, LSL #10							; ... r7 = PT0 physical address

		MOV		r0, #0						; r0 = page table index * 4
1		LDR		r1, [r7, r0]				; r1 = PT0[index]
		CMP		r1, #0						; empty?
		ADDNE	r0, r0, #4					; if not, step to next entry
		CMPNE	r0, #1024					; reached end?
		BNE		%BT1						; if not, try next entry
		CMP		r1, #0						; was entry empty?
		FAULT	NE							; if not, out of space for page tables

		MOV		r4, #12						; 4K page
		MOV		r2, #BTP_PageTable
		BOOTCALL	BTF_GetPtePerm			; r2 = PTE permissions for page table
		ORR		r2, r2, r8					; OR in physical address to give PTE
		ADD		r1, r7, r0					; r1 = address of PT0 entry for this page table
		LDR		r7, [r1]
		DWORD	r7, "NPT: OldPTE"
		DWORD	r1, "NPT: PTA"
		DWORD	r2, "NPT: PTE"
		CMP		r7, #0
		FAULT	NE							; don't overwrite existing PTE

		STR		r2, [r1]					; put in PT0 entry

		; Update main memory after page table change
		MOV		r5, r0 ; save r0
		MOV		r0, r1 ; Address
		MOV		r1, #4 ; Size of a single entry
		BOOTCALL	BTF_PTUpdate

		TST		r3, #MMUCR_M				; MMU on?
		MOVEQ	r7, r8						; if not, r7=r8
		LDRNE	r7, =KPageTableBase			; MMU on - r7 = page table linear base
		ADDNE	r7, r7, r5, LSL #10			; r7 = virtual address of page table
		DWORD	r8, "NewPageTable phys"
		DWORD	r7, "NewPageTable virt"
		MOV		r0, r7
		MOV		r1, #0x1000
		MOV		r2, #0
		BL		WordFill					; clear entire page

		; Update main memory after page table change
		MOV		r0, r7 		; Address
		MOV		r1, #0x1000	; Size of a page table
		BOOTCALL	BTF_PTUpdate
		LDMFD	sp!, {r0-r5,pc}
	ENDIF


;*******************************************************************************
; Initialise the memory mapping system
; Allocate the page directory and the first page table
; Clear them both (apart from RAM drive PDEs)
; Map the page table
; Map the page directory
; Direct model just allocates and clears the page directory.
; This is called before MMU and DCache is switched ON => no need to ensure
; coherency between cache & main memory.
; Enter with
;	R10 points to super page
;	R12 points to the ROM header
;	R13 points to a valid stack
;
; Return with
;	No registers modified
;
;*******************************************************************************
	EXPORT	InitMemMapSystem
InitMemMapSystem	ROUT
		STMFD	sp!, {r0-r9,r11,lr}

	IF	SMP
		; Allocate an extra page directory for use by APs during boot
		MOV		r2, #BMA_PageDirectory
		BOOTCALL	BTF_Alloc ;On return, r0=physical address of page directory
		DWORD	r0, "APBootPageDirPhys"
		STR		r0, [r10, #SSuperPageBase_iAPBootPageDirPhys]
		MOV		r1, #0x4000				; size
		MOV		r2, #0					; fill value
		BL		WordFill				; clear entire page directory
	ENDIF

		; Allocate main page directory
		MOV		r2, #BMA_PageDirectory
		BOOTCALL	BTF_Alloc ;On return, r0=physical address of page directory

		DWORD	r0, "PageDirPhys"
		STR		r0, [r10, #SSuperPageBase_iPageDir]
		MOV		r8, r0
		MOV		r2, #0					; fill value

	IF	CFG_MMDirect

		MOV		r1, #0x4000				; size
		BL		WordFill				; clear entire page directory
	IF	SMP
		MOV		r2, #BMA_PageTable
		BOOTCALL	BTF_Alloc
1
		MOV		r9, r0					; page table
		MOV		r2, #BMA_PageTable
		BOOTCALL	BTF_Alloc			; allocate another page table
		SUB		r0, r0, r9
		CMP		r0, #0x400
		BNE		%BA1					; need two consecutive page tables
		DWORD	r9, "APBootPageTablePhys"
		MOV		r0, r9
		MOV		r1, #0x800
		TST		r0, #0xC00				; first page table in page?
		MOVEQ	r1, #0x1000				; if so, clear whole page
		MOV		r2, #0
		BL		WordFill				; clear page tables
	ENDIF

	ELSE ; CFG_MMDirect
		MOV		r11, #KPageDirectoryBase	; r11 = page dir linear base
		IF	CFG_MMFlexible
			MOV		r1, #0x4000				; size
			BL		WordFill				; clear entire page directory
		ELSE
			MOV		r1, #KRamDriveStartAddress
			MOV		r1, r1, LSR #20
			MOV		r1, r1, LSL #2
			BL		WordFill				; clear page dir below ram drive
			
			MOV		r0, #KRamDriveEndAddress
			RSB		r1, r0, #0
			MOV		r0, r0, LSR #20
			ADD		r0, r8, r0, LSL #2
			MOV		r1, r1, LSR #20
			MOV		r1, r1, LSL #2
			BL		WordFill				; clear page dir above ram drive
		ENDIF
		
		MOV		r2, #BMA_PageTable
		BOOTCALL	BTF_Alloc
		DWORD	r0, "PT0"
		MOV		r7, r0					; physical address of PT0
		MOV		r2, #0					; fill value
		MOV		r1, #0x1000				; size
		BL		WordFill				; clear PT0-3
		MOV		r2, #BTP_PageTable
		MOV		r4, #12					; 4K page
		BOOTCALL	BTF_GetPdePerm		; PDE permissions for page table into R2
		ORR		r0, r7, r2				; PDE for PT0
		LDR		r1, =KPageTableBase
		MOV		r1, r1, LSR #20			; r1 = PDE index for PT0
		ADD		r1, r8, r1, LSL #2		; r1 = PDE address
		DWORD	r0, "PT0 PDE"
		DWORD	r1, "PT0 PDE addr"
		STR		r0, [r1]				; install PDE to map PT0
		MOV		r2, #BTP_PageTable
		MOV		r4, #12					; 4K page
		BOOTCALL	BTF_GetPtePerm		; PTE permissions for page table into R2
		ORR		r0, r7, r2				; PTE for PT0
		DWORD	r0, "PT0 PTE"
		DWORD	r7, "PT0 PTE addr"
		STR		r0, [r7]				; install PTE to map PT0

		MOV		r0, r11					; r0 = page directory linear
		MOV		r1, r8					; r8 = page directory physical
		MOV		r2, #BTP_PageTable		; r2 = access permissions (boot table offset)
		MOV		r3, #0x4000				; r3 = size of mapping
		MOV		r4, #12					; r4 = log2(max page size)
		BL		MapContiguous			; map the page directory

	IF	SMP
		; Map APBootPageDir
		PRTLN	"Map APBootPageDir"
		LDR		r0, =KAPBootPageDirLin	; r0 = page directory linear
		LDR		r1, [r10, #SSuperPageBase_iAPBootPageDirPhys]	; physical
		MOV		r2, #BTP_PageTable		; r2 = access permissions (boot table offset)
		MOV		r3, #0x4000				; r3 = size of mapping
		MOV		r4, #12					; r4 = log2(max page size)
		BL		MapContiguous			; map the page directory

		; Allocate a page for the two APBoot page tables
		MOV		r2, #BMA_Kernel
		BOOTCALL	BTF_Alloc
		MOV		r9, r0					; page table
		DWORD	r9, "APBootPageTablePhys"
		MOV		r1, #0x1000
		MOV		r2, #0
		BL		WordFill				; zero page
		
		; Map it after the APBootPageDir
		PRTLN	"Map APBootPageTables"
		LDR		r0, =KAPBootPageDirLin	; r0 = page directory linear
		ADD		r0, r0, #0x4000			; APBootPageTables linear
		MOV		r1, r9					; physical
		MOV		r2, #BTP_PageTable		; permissions
		MOV		r3, #0x1000				; size
		MOV		r4, #12					; map size
		BL		MapContiguous			; map the APBootPageTables

	ENDIF	; SMP

	ENDIF	; CFG_MMDirect

	IF	SMP
		; Install mappings for the APBoot code
		MOV		r2, #BTP_Vector			; APBoot code permissions same as exception vector permissions
		MOV		r4, #12					; 4K page
		BOOTCALL	BTF_GetPtePerm		; PTE permissions for page table into R2
		MOV		r5, r2					; into R5
		MOV		r2, #BTP_Vector			; APBoot code permissions same as exception vector permissions
		MOV		r4, #12					; 4K page
		BOOTCALL	BTF_GetPdePerm		; PDE permissions into R2
		MOV		r4, r2					; into R4
		BL		GetAPBootCodePhysAddr
		MOV		r6, r0					; Physical address of AP boot code into R6
		BL		RomPhysicalToLinear
		MOV		r7, r0					; Virtual address of AP boot code into R7
		LDR		r8, [r10, #SSuperPageBase_iAPBootPageDirPhys]	; APBoot page directory physical address into R8
										; APBoot page table physical address already in R9
		BL		SetupAPBootMappings		; Install mappings

	ENDIF	; SMP

		LDMFD	sp!, {r0-r9,r11,pc}

	IF	SMP
;*******************************************************************************
; Set up AP Boot mappings
; Using already allocated APBoot page directory and page tables, map the AP
; boot code at both its physical and virtual addresses.
; 
; Enter with
;	MMU off
;	R4 = PDE permissions for mappings
;	R5 = PTE permissions for mappings
;	R6 = Physical address of AP boot code
;	R7 = Virtual address of AP boot code
;	R8 = APBoot page directory physical address
;	R9 = APBoot page table physical address
;	R10 = super page physical address
;	R12 = ROM header physical address
;	R13 = stack physical address
;
; Return with
;	Registers unmodified
;
;*******************************************************************************
SetupAPBootMappings	ROUT
		STMFD	sp!, {r0-r9,r11,lr}

		DWORD	r4, "APBootCode PDE Perm"
		DWORD	r5, "APBootCode PTE Perm"
		DWORD	r6, "APBootCode Phys"
		DWORD	r7, "APBootCode Virt"
		DWORD	r8, "APBootCode PD Phys"
		DWORD	r9, "APBootCode PT Phys"

		; Install identity mapping for the APBoot code
		ORR		r0, r9, r4				; PDE
		MOV		r1, r6, LSR #20			; PDE index = (physical address >> 20)
		ADD		r1, r8, r1, LSL #2		; PDE address
		DWORD	r0, "APBoot Identity PDE"
		DWORD	r1, "APBoot Identity PDA"
		STR		r0, [r1]				; write PDE
		MOV		r0, r6, LSR #12			; physical address of boot code >> 12
		ORR		r0, r5, r0, LSL #12		; PTE
		MOV		r1, r6, LSR #12
		AND		r1, r1, #0xFF			; PTE index = (physical address >> 12) & 0xff
		ADD		r1, r9, r1, LSL #2		; PTE address
		DWORD	r0, "APBoot Identity PTE"
		DWORD	r1, "APBoot Identity PTA"
		STR		r0, [r1]				; write PTE

		; Install mapping for the APBoot code at its virtual address
		EORS	r0, r6, r7				; Physical and virtual identical?
		BEQ		%FA1					; If so, don't need second mapping
		MOVS	r0, r0, LSR #20			; Physical and virtual in same PDE range?
		BEQ		%FA2					; If so, don't need second PDE, just second PTE
		ADD		r9, r9, #0x400			; Use second page table in page
		ORR		r0, r9, r4				; PDE
		MOV		r1, r7, LSR #20			; PDE index = (virtual address >> 20)
		ADD		r1, r8, r1, LSL #2		; PDE address
		DWORD	r0, "APBoot Virtual PDE"
		DWORD	r1, "APBoot Virtual PDA"
		STR		r0, [r1]				; write PDE
2
		MOV		r0, r6, LSR #12			; physical address of boot code >> 12
		ORR		r0, r5, r0, LSL #12		; PTE
		MOV		r1, r7, LSR #12
		AND		r1, r1, #0xFF			; PTE index = (virtual address >> 12) & 0xff
		ADD		r1, r9, r1, LSL #2		; PTE address
		DWORD	r0, "APBoot Virtual PTE"
		DWORD	r1, "APBoot Virtual PTA"
		STR		r0, [r1]				; write PTE
1
		LDMFD	sp!, {r0-r9,r11,pc}

	ENDIF

;*******************************************************************************
; Switch to virtual mappings
; Make temporary identity mapping
; Set up MMU registers
; Enable MMU
; Remove temporary identity mapping
; Adjust pointers
; 
; 
; Enter with
;	R10 = super page physical address
;	R11 = kernel TRomImageHeader physical address
;	R12 = ROM header physical address
;	R13 = stack physical address
;
; Return with
;	R10-R13 converted to linear addresses
;
;*******************************************************************************
	EXPORT	SwitchToVirtual
SwitchToVirtual	ROUT
		STMFD	sp!, {lr}
		DWORD	r10, "SuperPagePhys"
		DWORD	sp, "StackPhys"
		
		PRTLN	"Super page base BEFORE fixup:"
		MOV		r1, #SSuperPageBase_sz
		MEMDUMP	r10, r1

		
		MOV		r0, r12					; ROM header physical address into R0
		BL		RomPhysicalToLinear		; convert to linear
		DWORD	r0, "RomHeaderLinear"
		SUB		r9, r0, r12				; r9 = ROM linear - ROM physical (for bootstrap at least)
		DWORD	r9, "RomDelta"
		MOV		r0, r11					; ROM image header physical address into R0
		BL		RomPhysicalToLinear		; convert to linear
		MOV		r11, r0
		MOV		r7, pc, LSR #20			; r7 = PDE index for PC
		DWORD	r7, "PC PDE index"
		LDR		r8, [r10, #SSuperPageBase_iPageDir]	; r8 = page dir phys
		LDR		r6, [r8, r7, LSL #2]	; r6 = PDE for current PC
		DWORD	r6, "Saved PDE"
		MOV		r2, #BTP_Temp
		MOV		r4, #20
		BOOTCALL	BTF_GetPdePerm		; get uncached section PDE permissions into r2
		ORR		r2, r2, r7, LSL #20		; make PDE for identity mapping
		ADD		r0, r8, r7, LSL #2
		DWORD	r0, "TempPDE addr"
		DWORD	r2, "TempPDE"
		STR		r2, [r0]				; install temporary identity mapping

		; MMU and DCache are still switched OFF => no need to ensure
		; coherency between cache & main memory.
		; BOOTCALL	BTF_PTUpdate
		
	IF	CFG_MMDirect
		MOV		r0, r8
		BL		RamPhysicalToLinear
		MOV		r5, r0					; r5 = page directory linear
		MOV		r0, r10
		BL		RamPhysicalToLinear
		MOV		r4, r0					; R4 = super page linear
	ELSE
		MOV		r5, #KPageDirectoryBase	; r5 = page directory linear
	ENDIF
		DWORD	r8, "PD phys"
		DWORD	r5, "PD lin"
		CMP		r7, r5, LSR #20			; Is PC in same section as page directory linear?
		FAULT	EQ						; If so, bad news :-(

		BOOTCALL	BTF_EnableMMU		; ENABLE THE MMU - RETURN TO LINEAR ADDRESSES

	IF	CFG_MMDirect
		MOV		r6, r10					; r6 = super page phys
		MOV		r10, r4					; r10 = super page lin
	ELSE
		MOV		r6, r10					; r6 = super page phys
		MOV		r10, #KSuperPageLinAddr	; r10 = super page lin
	ENDIF
		ADD		r12, r12, r9			; R12 points to TRomHeader
		SUB		r6, r10, r6				; R6 = delta super page
		ADD		sp, sp, r6				; Switch to linear stack
		STR		r5, [r10, #SSuperPageBase_iPageDir]	; save page directory linear

		; Fix up the super page
		ADRL	r4, %FA3
		
1		LDR		r5, [r4], #4			; get index
		MOV		r1, r5					; save index flags for later
		MSR		cpsr_f, r5				; flags into NZCV
		BIC		r5, r5, #0xf0000000		; remove flags
		BMI		%FT2					; end of list
		LDR		r0, [r10, r5]			; get super page value
		CMN 	r0, #1					
		BCS		%BT1					; skip field if value==KSuperPageAddressFieldUndefined, 
										; in casees where the field is optional
		MSR		cpsr_f, r1				; based on flags fix up address in r0
		ADDCC	r0, r0, r6				; if bit 29 = 0, relocate relative to super page
		ADDEQ	r0, r0, r9				; if bit 30 = 1, relocate relative to bootstrap ROM
		BLHI	RomPhysicalToLinear		; if bits 30:29 = 01, relocate relative to full ROM
		STR		r0, [r10, r5]
		B		%BT1
2
		MOV		r2, #BMA_Reloc
		MOV		r0, r6					; super page offset
		BOOTCALL	BTF_Alloc			; fix up allocator

		DWORD	r10, "SuperPageLin"
		DWORD	r11, "PrimaryImgLin"
		DWORD	r12, "RomHdrLin"
		DWORD	sp, "StackLin"
		PRTLN	"Super page base after fixup:"
		MOV		r1, #SSuperPageBase_sz
		MEMDUMP	r10, r1
		LDMIA	sp!, {lr}				; Return address (physical)
		ADD		pc, lr, r9				; Return to linear address

3
		DCD		SSuperPageBase_iRamBootData						; super page relative
		DCD		SSuperPageBase_iRomBootData						; super page relative
		DCD		SSuperPageBase_iMachineData						; super page relative
		DCD		SSuperPageBase_iBootAlloc						; super page relative
		DCD		SSuperPageBase_iSmrData							; super page relative, optional SP field, hence value maybe -1
		DCD		SSuperPageBase_iBootTable		+ 0x60000000	; bootstrap relative
		DCD		SSuperPageBase_iCodeBase		+ 0x60000000	; bootstrap relative
		DCD		-1
		
		ENDIF	; CFG_MMUPresent

		IF	CFG_IncludeRAMAllocator
		IF	:LNOT: CFG_MMDirect

;*******************************************************************************
; Memory allocator for virtual memory models
;*******************************************************************************

;*******************************************************************************
; Determine if a region under test overlaps a preallocated block
; 
; Enter with
;	R0	= base of region under test
;	R2	= size of region under test - 1
;	R10 = pointer to SSuperPageBase
;	R11 = pointer to SAllocData
;	R13 = stack pointer
;
; Return with
;	If there is an overlap, R0 is stepped past the preallocated block and Z=0
;	If there is no overlap, R0 is unmodified and Z=1
;	No registers modified other than R0
;*******************************************************************************
CheckPreAlloc	ROUT
		STMFD	sp!, {r3-r5,lr}
		ADD		r3, r11, #SAllocData_iExtraSkip
		LDMIA	r3, {r4,r5}						; start with extra skip range
		LDR		r3, [r11, #SAllocData_iSkip]	; skip ranges

1		; r4=base of skip range, r5=size of skip range
		ADD		lr, r0, r2						; lr = inclusive end of region under test
		ADD		r5, r4, r5
		SUB		r5, r5, #1						; r5 = inclusive end of skip
		CMP		r4, r0							;
		MOVLS	r4, r0							; r4 = max(region base, skip base) = intersection base
		CMP		lr, r5							;
		MOVHI	lr, r5							; lr = intersection inclusive end
		CMP		r4, lr							; r4>lr iff intersection empty
		BHI		%FT2							;
		DWORD	r0, "OVERLAP RUT  BASE"
		DWORD	r2, "OVERLAP RUT  SIZE-1"
		DWORD	r4, "OVERLAP SKIP BASE"
		DWORD	r5, "OVERLAP SKIP END"
		MOVS	r0, r5							; r0=inclusive end of skip (can't be zero)
		ADD		r0, r0, #1						; step r0 over skip range
		LDMFD	sp!, {r3-r5,pc}					; return with Z flag clear
2
		LDMIA	r3!, {r4,r5}					; get next skip range
		CMP		r5, #0							; reached end?
		BNE		%BT1							; 
		LDMFD	sp!, {r3-r5,pc}					; yes - return with Z set and R0 unmodified

;*******************************************************************************
; Advance an SPos structure past the next valid block
; 
; Enter with
;	R9	= pointer to SPos structure
;	R10 = pointer to SSuperPageBase
;	R11 = pointer to SAllocData
;	R13 = stack pointer
;
; Return with
;	Address of next valid block in R0
;	SRamBank pointer in R1, bank base in R4, bank size in R5
;	R2 = iMask value of SPos
;*******************************************************************************
FindValidBlock	ROUT
		STMFD	sp!, {lr}
		LDMIA	r9, {r0,r1,r2}					; r0 = p->iNext, r1 = p->iBank, r2 = p->iMask
		DWORD	r0, "FVB next"
		DWORD	r1, "FVB bank"
		DWORD	r2, "FVB mask"
		LDMIA	r1, {r4,r5}						; r4=SRamBank base, r5=SRamBank size
1		ADDS	r0, r0, r2
		BIC		r0, r0, r2						; first round up to the required boundary
		BCS		%FA0							; if address wraps round, OOM
		DWORD	r0, "FVB try"
		DWORD	r4, "FVB bank base"
		DWORD	r5, "FVB bank size"
		SUBS	lr, r0, r4						; offset into bank
		FAULT	CC								; shouldn't be -ve
		ADD		lr, lr, r2						; add size of region - 1
		CMP		lr, r5							; compare with bank size
		BHS		%FA2							; if >=, we have stepped outside the bank
		BL		CheckPreAlloc					; check we don't overlap a skip range
		BNE		%BA1							; if we do, advance R0 past the skip and try again
		ORR		lr, r0, #1						; if not, OK - we have a valid block
		STR		lr, [r9]						; set iNext = addr + 1
		DWORD	r0, "FVB ret"
		LDMFD	sp!, {pc}						;
2		ADD		r1, r1, #SRamBank_sz			; step to next SRamBank
		LDMIA	r1, {r4,r5}						; r4=SRamBank base, r5=SRamBank size
		STR		r1, [r9, #4]					; update iBank
		DWORD	r1, "FVB update bank"
		MOV		r0, r4							; start looking again from base of bank
		CMP		r5, #0							; end of list?
		BNE		%BA1							; no - keep going
0		MVN		r0, #0							; else alloc failed - return with r0=0xffffffff
		STR		r0, [r9]						; update iNext
		DWORD	r0, "FVB OOM"
		LDMFD	sp!, {pc}						;

;*******************************************************************************
; Find the SPos structure for a given allocation size
; 
; Enter with
;	R4	= log2(alloc size)
;	R10 = pointer to SSuperPageBase
;	R11 = pointer to SAllocData
;	R13 = stack pointer
;
; Return with
;	Address of SPos in R9
;	R0 = R2 = 2^R4-1, R1 also modified
;*******************************************************************************
FindSizeData	ROUT
		MVN		r2, #0
		MVN		r2, r2, LSL r4					; r2 = 2^R4-1
		MOV		r9, r11							; start with first SPos
1		LDR		r0, [r9, #8]					; r0 = mask for current SPos
		CMP		r2, r0
		MOVEQ	pc, lr							; found it
		LDR		r1, [r9, #12]					; r1 = iFinal
		ADD		r9, r9, #SAllocData_SPos_sz		; step to next SPos
		CMP		r1, #0							; final?
		BEQ		%BT1							; loop if not
		FAULT									; bad size - die

;*******************************************************************************
; Allocate a block of a given size, which must be a power of 2
; 
; Enter with
;	R4	= log2(alloc size)
;	R10 = pointer to SSuperPageBase
;	R11 = pointer to SAllocData
;	R13 = stack pointer
;
; Return with
;	R0	= address of allocated block, or 0xFFFFFFFF if none could be found
;	R9	= pointer to SPos for requested size
;	R1-R5,R8,R9 also modified
;*******************************************************************************
	EXPORT	DoAllocBlock
DoAllocBlock	ROUT
		DWORD	r4, ">DoAllocBlock"
		STMFD	sp!, {lr}
		BL		FindSizeData					; set up R9 to correct SPos
1		BL		FindValidBlock					; R0 = candidate block address
		CMN		r0, #1							; OOM?
		BEQ		%FA0							; branch if so
		MOV		r8, r9							; r8 = SPos for bigger blocks to examine
2		LDR		r1, [r8, #SAllocData_SPos_iFinal]	; r1 = r8->iFinal
		CMP		r1, #0
		BNE		%FA0							; if final, candidate is OK
		ADD		r8, r8, #SAllocData_SPos_sz		; else step r8 to next SPos
		LDR		r2, [r8, #SAllocData_SPos_iMask]	; r2 = r8->iMask
		TST		r0, r2
		BNE		%FA0							; not correctly aligned for larger size - finished
		SUBS	r1, r0, r4						; r1 = offset from bank base
		FAULT	CC								; shouldn't be negative
		ADD		r1, r1, r2						; add size-1 for larger block
		CMP		r1, r5							; compare to bank size
		BHS		%FA0							; if >=, not valid for larger size - finished
		MOV		r1, r0
		BL		CheckPreAlloc					; check for overlap with skip regions
		MOV		r0, r1							; don't want to modify R0
		BNE		%FA0							; if overlap, not valid for larger size - finished
		LDR		r1, [r8]						; r1 = r8->iNext
		CMP		r1, r0							; if r1>r0, there is a clash
		BLS		%BA2							; if no clash try next size
		DWORD	r0, "CLASH base"
		DWORD	r2, "CLASH mask"
		LDR		r0, [r9]						; r0 = r9->iNext
		ADD		r0, r0, r2						;
		STR		r0, [r9]						; r9->iNext += r8->iMask (step past clashing block)
		B		%BA1							; and continue search
0		; have a good block in R0 or R0=0xFFFFFFFF if OOM
		DWORD	r0, "<DoAllocBlock"
		MOV		r8, r9							; r8 = SPos for bigger blocks to examine
		LDMIA	r9, {r3,r4}						; r3 = r9->iNext, r4=r9->iBank
3		LDR		r1, [r8, #SAllocData_SPos_iFinal]	; r1 = r8->iFinal
		CMP		r1, #0
		LDMNEFD	sp!, {pc}						; if final, finished
		ADD		r8, r8, #SAllocData_SPos_sz		; else step r8 to next SPos
		LDR		r1, [r8]						; r1 = r8->iNext
		CMP		r3, r1							; if r9->iNext > r8->iNext ...
		STMHIIA	r8, {r3,r4}						; ... r8->iNext=r9->iNext, r8->iBank=r9->iBank
		B		%BA3							;


;*******************************************************************************
; Initialise the allocator data
; 
; Enter with
;	R10 = pointer to SSuperPageBase
;	R13 = stack pointer
;
; Return with
;	R0-R4,R11 modified
;*******************************************************************************
	EXPORT	InitAllocator
InitAllocator	ROUT
		STMFD	sp!, {lr}
		LDR		r11, =CpuAllocDataOffset
		ADD		r11, r11, r10					; r11 -> SAllocData
		STR		r11, [r10, #SSuperPageBase_iBootAlloc]		; save pointer
		LDR		r1, [r10, #SSuperPageBase_iRamBootData]		; r4->SRamBank list
		LDR		r0, [r1]						; base address of first bank
		LDR		r2, =0xfff						; 4K - 1
		MOV		r3, #0							; final = 0
		STMIA	r11!, {r0-r3}					; pos[0]
		ORR		r2, r2, r2, LSL #2				; 16K - 1
		STMIA	r11!, {r0-r3}					; pos[1]
		ORR		r2, r2, r2, LSL #2				; 64K - 1
		STMIA	r11!, {r0-r3}					; pos[1]
		ORR		r2, r2, r2, LSL #4				; 1M - 1
		MOV		r4, #1							; final = 1
		STMIA	r11!, {r0-r2,r4}				; pos[1]
		STR		r4, [r11, #4]					; iExtraSkip base
		STR		r3, [r11, #8]					; iExtraSkip size
		STR		r3, [r11, #12]					; iNextPageTable
1		LDMIA	r1!, {r0,r2}					; SRamBank base, size
		CMP		r2, #0
		BNE		%BT1							; loop until size = 0
		STR		r1, [r11], #-64					; iSkip points to preallocated block list
		PRTLN	"SAllocData:"
		MEMDUMP	r11, #SAllocData_sz
		LDMFD	sp!, {pc}

;*******************************************************************************
; Physical RAM Allocator for MMU configurations
; Accepts all allocator calls
; 
; Enter with
;	R2	= type of allocation
;	R4	= log2(size) for BMA_Kernel allocations
;	R10 = pointer to SSuperPageBase
;	R12 = pointer to TRomHeader
;	R13 = stack pointer
;
; Return with
;	R0	= pointer to allocated memory
;	Faults on OOM
;	R4 may be reduced for BMA_Kernel allocations if the original request size
;	could not be satisfied.
;	No other registers modified
;*******************************************************************************
	EXPORT	HandleAllocRequest
HandleAllocRequest	ROUT
		DWORD	r2, ">HandleAllocRequest type"
		STMFD	sp!, {r1-r9,r11,lr}
		LDR		r11, [r10, #SSuperPageBase_iBootAlloc]	; set up R11->SAllocData
		CMP		r2, #BMA_NUM_FUNCTIONS
		FAULT	HS								; bad request number
		LDR		r1, [pc, r2, LSL #2]
HA0		ADD		pc, pc, r1
		DCD		HandleAllocInit-HA0-8			; init
		DCD		HandleAllocSuper-HA0-8			; super/CPU
		DCD		HandleAllocPageDir-HA0-8		; page directory
		DCD		HandleAllocPageTable-HA0-8		; page table
		DCD		HandleAllocKernel-HA0-8			; kernel
		DCD		HandleAllocReloc-HA0-8			; relocate data
HandleAllocInit
		BL		InitAllocator
		LDMFD	sp!, {r1-r9,r11,pc}
HandleAllocSuper
		MOV		r0, r10							; don't move it
		ADD		r2, r11, #SAllocData_iExtraSkip
		MOV		r1, #SuperCpuSize				; skip size
		STMIA	r2, {r0,r1}						; install skip region for super/CPU pages
HandleAllocExit
		CMN		r0, #1
		FAULT	EQ								; fault if alloc fails
		DWORD	r0, "<HandleAllocRequest ret"
		LDMFD	sp!, {r1-r9,r11,pc}
HandleAllocPageDir
		MOV		r4, #14							; page dir is always 16K
		BL		DoAllocBlock					; get block
		B		HandleAllocExit
HandleAllocPageTable
		LDR		r0, [r11, #SAllocData_iNextPageTable]
		TST		r0, #0xc00						; free subpage?
		ADDNE	r1, r0, #0x400
		STRNE	r1, [r11, #SAllocData_iNextPageTable]	; if so, step iNextPageTable on
		BNE		HandleAllocExit					; and return
		MOV		r4, #12							; else allocate new page
		BL		DoAllocBlock					; get block
		ADD		r1, r0, #0x400
		STR		r1, [r11, #SAllocData_iNextPageTable]	; set up iNextPageTable
		B		HandleAllocExit					; and return
HandleAllocKernel
		DWORD	r4, ">HandleAllocRequest size"
2
		BL		DoAllocBlock					; try to get block
		CMN		r0, #1
		LDR		r4, [sp, #12]					; recover R4
		BNE		%FA1							; if success, finished
		CMP		r4, #12
		FAULT	EQ								; if 4K, fault
		SUB		r4, r4, #4						; reduce size 1M -> 64K -> 4K
		STR		r4, [sp, #12]					; update saved R4
		B		%BA2
1
		DWORD	r4, "<HandleAllocRequest size"
		DWORD	r0, "<HandleAllocRequest ret"
		LDMFD	sp!, {r1-r9,r11,pc}

HandleAllocReloc
		LDR		r1, [r11, #4]!					; bank 0
		ADD		r1, r1, r0
		STR		r1, [r11]						; fix
		LDR		r1, [r11, #16]!					; bank 1
		ADD		r1, r1, r0
		STR		r1, [r11]						; fix
		LDR		r1, [r11, #16]!					; bank 2
		ADD		r1, r1, r0
		STR		r1, [r11]						; fix
		LDR		r1, [r11, #16]!					; bank 3
		ADD		r1, r1, r0
		STR		r1, [r11]						; fix
		LDR		r1, [r11, #12]!					; iSkip
		ADD		r1, r1, r0
		STR		r1, [r11]						; fix
		LDMFD	sp!, {r1-r9,r11,pc}

		LTORG

		ELSE	; CFG_MMDirect

;*******************************************************************************
; Simple memory allocator for direct model
;*******************************************************************************
; Accepts allocator calls other than BMA_Kernel
; 
; Enter with
;	R2	= type of allocation
;	R10 = pointer to SSuperPageBase
;	R12 = pointer to TRomHeader
;	R13 = stack pointer
;
; Return with
;	R0	= pointer to allocated memory
;	Faults on OOM
;	No other registers modified
;*******************************************************************************
	EXPORT	HandleAllocRequest
HandleAllocRequest	ROUT
		DWORD	r2, ">HandleAllocRequest type"
		STMFD	sp!, {r1-r9,r11,lr}
		LDR		r11, [r10, #SSuperPageBase_iBootAlloc]	; set up R11->SDirectAlloc
		CMP		r2, #BMA_NUM_FUNCTIONS
		FAULT	HS								; bad request number
		LDR		r1, [pc, r2, LSL #2]
HA0		ADD		pc, pc, r1
		DCD		HandleAllocInit-HA0-8			; init
		DCD		HandleAllocSuper-HA0-8			; super/CPU
		DCD		HandleAllocPageDir-HA0-8		; page directory
		DCD		HandleAllocPageTable-HA0-8		; page table
		DCD		HandleAllocKernel-HA0-8			; kernel
		DCD		HandleAllocReloc-HA0-8			; relocate data
HandleAllocInit
		LDR		r11, =CpuAllocDataOffset
		ADD		r11, r11, r10							; r11 -> SDirectAlloc
		STR		r11, [r10, #SSuperPageBase_iBootAlloc]	; save pointer
		LDR		r0, [r10, #SSuperPageBase_iKernelLimit]	; linear upper limit
		MOV		r1, r0
		SUB		r0, r0, #1								; make inclusive
		BL		RamLinearToPhysical						; convert to physical
		ADD		r0, r0, #1								; make exclusive
		STR		r0, [r11, #SDirectAlloc_iNextPageTable]	; store physical
		DWORD	r0, "Init iNextPageTable"
		SUB		r2, r1, r0								; save linear - physical
		GETPARAM	BPR_PageTableSpace, DefaultPTAlloc	; get reserved space for page tables
		SUB		r0, r1, r0								; linear lower limit
		MOV		r3, r0
		BL		RamLinearToPhysical						; convert to physical
		STR		r0, [r11, #SDirectAlloc_iLimit]			; store physical
		DWORD	r0, "Init iLimit"
		SUB		r4, r3, r0								; save linear - physical
		CMP		r2, r4									; make sure upper + lower limits in same bank
		FAULT	NE
		LDMFD	sp!, {r1-r9,r11,pc}
HandleAllocSuper
		GETPARAM	BPR_KernDataOffset, SuperCpuSize	; get kernel data offset into R0 with default
		LDR		r1, [r12, #TRomHeader_iKernDataAddress]
		SUB		r0, r1, r0								; linear
		DWORD	r0, "Super lin"
		BL		RamLinearToPhysical
		DWORD	r0, "Super phys"
		LDMFD	sp!, {r1-r9,r11,pc}
HandleAllocPageDir
		LDR		r0, [r11, #SDirectAlloc_iNextPageTable]	; next address
		LDR		r1, [r11, #SDirectAlloc_iLimit]			; lower limit on address
		SUB		r0, r0, #0x4000							; subtract page dir size
		BIC		r0, r0, #0xff
		BIC		r0, r0, #0x3f00							; round down to 16K
		STR		r0, [r11, #SDirectAlloc_iNextPageTable]	; store next address
		DWORD	r0, "Page dir phys"
		CMP		r0, r1
		FAULT	LO										; insufficient RAM reserved
		LDMFD	sp!, {r1-r9,r11,pc}
HandleAllocPageTable
		LDR		r0, [r11, #SDirectAlloc_iNextPageTable]	; next address
		LDR		r1, [r11, #SDirectAlloc_iLimit]			; lower limit on address
		TST		r0, #0xC00								; new page required?
		SUBEQ	r0, r0, #0x1000							; if so, subtract 4K
		ADD		r2, r0, #0x400							; step on by page table size
		TST		r2, #0xC00								; new page required next time?
		SUBEQ	r2, r2, #0x1000							; if so, backtrack to start of current page
		DWORD	r2, "NextPageTable"
		STR		r2, [r11, #SDirectAlloc_iNextPageTable]	; store next address
		DWORD	r0, "PageTable"
		CMP		r0, r1
		FAULT	LO										; insufficient RAM reserved
		LDMFD	sp!, {r1-r9,r11,pc}
HandleAllocKernel
		FAULT									; not supported on direct model
HandleAllocReloc
		LDMFD	sp!, {r1-r9,r11,pc}				; not required on direct model

		ENDIF

		ELSE	; CFG_IncludeRAMAllocator

;*******************************************************************************
; Dummy allocator for non-MMU configurations
; Only accepts BMA_Init and BMA_SuperCPU calls
; 
; Enter with
;	R2	= type of allocation
;	R10 = pointer to SSuperPageBase
;	R13 = stack pointer
;
; Return with
;	R0	= allocated value
;	No other registers modified
;*******************************************************************************
	EXPORT	AllocatorStub
AllocatorStub	ROUT
		CMP		r2, #BMA_SuperCPU
		MOVEQ	r0, r10							; leave super page where it is
		MOVLS	pc, lr							; ignore Init call
		FAULT									; don't accept anything else

		ENDIF	; CFG_IncludeRAMAllocator

;*******************************************************************************
; Debug only code
;*******************************************************************************

		IF		CFG_DebugBootRom

;*******************************************************************************
; Print a null-terminated string located after the instruction after the call
; No registers modified (except flags)
;*******************************************************************************
	EXPORT	WriteS
WriteS	ROUT
		STMFD	sp!, {r0, r1, lr}
		ADD		r1, lr, #4						; skip the following branch
1		LDRB	r0, [r1], #1
		CMP		r0, #0
		LDMEQFD	sp!, {r0, r1, pc}
		BOOTCALL	BTF_WriteC
		B		%BT1

;*******************************************************************************
; Print a newline
; No registers modified (except flags)
;*******************************************************************************
	EXPORT	WriteNL
WriteNL	ROUT
		STMFD	sp!, {r0,lr}
		MOV		r0, #13
		BOOTCALL	BTF_WriteC
		MOV		r0, #10
		BOOTCALL	BTF_WriteC
		LDMFD	sp!, {r0,pc}


;*******************************************************************************
; Print R0 in hexadecimal
; No registers modified (except flags)
;*******************************************************************************
	EXPORT	WriteW
WriteW	ROUT
		STMFD	sp!, {r0-r2,lr}
		MOV		r2, r0
		MOV		r1, #8
		B	WriteHex

	EXPORT	WriteH
WriteH	ROUT
		STMFD	sp!, {r0-r2,lr}
		MOV		r2, r0, lsl #16
		MOV		r1, #4
		B	WriteHex

	EXPORT	WriteB
WriteB	ROUT
		STMFD	sp!, {r0-r2,lr}
		MOV		r2, r0, lsl #24
		MOV		r1, #2
		B	WriteHex

WriteHex	ROUT
		MOV		r0, r2, lsr #28
		MOV		r2, r2, lsl #4
		CMP		r0, #9
		ADDHI	r0, r0, #7
		ADD		r0, r0, #48
		BOOTCALL	BTF_WriteC
		SUBS	r1, r1, #1
		BNE		WriteHex
		LDMFD	sp!, {r0-r2,pc}

;*******************************************************************************
; Dump memory from R0 to R0+R1-1 inclusive in hexadecimal
; No registers modified (except flags)
;*******************************************************************************
	EXPORT	MemDump
MemDump	ROUT
		STMFD	sp!, {r0-r3,lr}
		MOV		r2, r0
1		MOV		r0, r2
		BL		WriteW
		MOV		r0, #58
		BOOTCALL	BTF_WriteC
		MOV		r3, #16
2
		MOV		r0, #32
		BOOTCALL	BTF_WriteC
		LDRB	r0, [r2], #1
		BL		WriteB
		SUBS	r3, r3, #1
		BNE		%BT2
		BL		WriteNL
		SUBS	r1, r1, #16
		BHI		%BT1
		LDMFD	sp!, {r0-r3,pc}

;*******************************************************************************
; Initialise stacks for modes other than SVC to R10 + 0x1000
; R0, R1, R2 modified
;*******************************************************************************
	EXPORT	InitStacks
InitStacks	ROUT
		GETCPSR	r1
		MOV		r2, r10
		ORR		r1, r1, #0x1f			; system/user
		SETCPSR	r1
		ADD		sp, r2, #0x1000
		BIC		r0, r1, #0x0e			; fiq
		SETCPSR	r0
		ADD		sp, r2, #0x1000
		BIC		r0, r1, #0x0d			; irq
		SETCPSR	r0
		ADD		sp, r2, #0x1000
		BIC		r0, r1, #0x08			; abt
		SETCPSR	r0
		ADD		sp, r2, #0x1000
		BIC		r0, r1, #0x04			; undef
		SETCPSR	r0
		ADD		sp, r2, #0x1000
		BIC		r0, r1, #0x0c			; back to svc
		SETCPSR	r0
		MOV		pc, lr

;*******************************************************************************
; Debug exception handlers
;*******************************************************************************

	EXPORT	HandleUndef
HandleUndef	ROUT
		SUB		sp, sp, #160
		STMIA	sp, {r0-r14}^
		MOV		r0, #2
		B		HandleException

	EXPORT	HandleSwi
HandleSwi
		SUB		sp, sp, #160
		STMIA	sp, {r0-r14}^
		MOV		r0, #3
		B		HandleException

	EXPORT	HandleAbtPrefetch
HandleAbtPrefetch
		SUB		sp, sp, #160
		STMIA	sp, {r0-r14}^
		MOV		r0, #0
		B		HandleException

	EXPORT	HandleAbtData
HandleAbtData
		SUB		sp, sp, #160
		STMIA	sp, {r0-r14}^
		MOV		r0, #1
		SUB		lr, lr, #4
		B		HandleException

	EXPORT	HandleRsvdVector
HandleRsvdVector
		SUB		sp, sp, #160
		STMIA	sp, {r0-r14}^
		MOV		r0, #4
		B		HandleException

	EXPORT	HandleIrq
HandleIrq
		SUB		sp, sp, #160
		STMIA	sp, {r0-r14}^
		MOV		r0, #5
		B		HandleException

	EXPORT	HandleFiq
HandleFiq
		SUB		sp, sp, #160
		STMIA	sp, {r0-r14}^
		MOV		r0, #6

HandleException
		SUB		lr, lr, #4
HandleException2
		STR		lr, [sp, #60]			; now have R0-R15 user
		STR		r0, [sp, #64]			; exception type

	IF	CFG_MMUPresent
		MRC		p15, 0, r0, c5, c0, 0	; FSR
		MRC		p15, 0, r1, c6, c0, 0	; FAR
	ELSE
		MOV		r0, #0
		MOV		r1, #1
	ENDIF
		GETSPSR	r2						; saved CPSR
		STR		r0, [sp, #68]
		STR		r1, [sp, #72]
		STR		r2, [sp, #76]
		ADD		r0, sp, #80

		GETCPSR	r3
		BIC		r2, r3, #0x1f
		ORR		r2, r2, #0x13			; mode_svc
		SETCPSR	r2
		STMIA	r0!, {r13-r14}			; r13_svc, r14_svc
		ORR		r2, r2, #0x12			; mode_irq
		SETCPSR	r2
		STMIA	r0!, {r13-r14}			; r13_irq, r14_irq
		ORR		r2, r2, #0x17			; mode_abt
		SETCPSR	r2
		STMIA	r0!, {r13-r14}			; r13_abt, r14_abt
		ORR		r2, r2, #0x1b			; mode_und
		SETCPSR	r2
		STMIA	r0!, {r13-r14}			; r13_und, r14_und
		ORR		r2, r2, #0x11			; mode_fiq
		SETCPSR	r2
		STMIA	r0!, {r8-r14}			; r8_fiq - r14_fiq

		SETCPSR	r3						; back to original mode
		MOV		r0, sp
		MOV		r1, #160
		BL		MemDump					; dump saved registers
		SUB		pc, pc, #8				; halt

	ENDIF	; CFG_DebugBootRom

;*******************************************************************************
; Fault handler
;*******************************************************************************

	EXPORT	BasicFaultHandler
BasicFaultHandler	ROUT
	IF	CFG_DebugBootRom
		DWORD	lr, "***FAULT***"
		SUB		sp, sp, #160
		STMIA	sp, {r0-r14}^
		MOV		r0, #7
		B		HandleException2
	ENDIF	; CFG_DebugBootRom
		SUB		pc, pc, #8				; halt

		END
