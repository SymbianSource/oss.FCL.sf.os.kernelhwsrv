;
; Copyright (c) 2007-2008 Nokia Corporation and/or its subsidiary(-ies).
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
; Description: General subroutines for bootstrap
;

LOCALS
.486p

INCLUDE	bootcpu.inc

;
;*******************************************************************************
;

cseg segment use32 para public 'code'
assume	cs:cseg, ds:cseg, ss:cseg, es:cseg, fs:cseg, gs:cseg


;*******************************************************************************
; Fill memory with a word value
;
; Enter with :
;       [esp+0] = address (must be word aligned)
;       [esp+4] = number of bytes (must be multiple of 4)
;       [esp+8] = value to fill
;
; Leave with :
;		All registers unmodified
;		Parameters popped from stack
;*******************************************************************************

WordFill	PROC NEAR

		pushfd					; save original flags
		xchg	edi, [esp+8]	; edi = dest addr, save orig edi
		xchg	ecx, [esp+12]	; ecx = size, save orig ecx
		xchg	eax, [esp+16]	; eax = fill value, save orig eax
		shr		ecx, 2			; bytes to dwords
		jz		short fill0		; if size zero, don't bother
		cld						; go forwards
		rep stosd				; fill
fill0:
		popfd					; restore flags
		pop		eax				; return address into eax
		pop		edi				; restore edi
		pop		ecx				; restore ecx
		xchg	eax, [esp]		; restore eax, return address to stack
		ret

		ENDP

;*******************************************************************************
; Move memory from one address to another
; Works with overlapping address ranges
;
; Enter with :
;		[esp+0] = destination address (must be word aligned)
;		[esp+4] = source address (must be word aligned)
;		[esp+8] = number of bytes (must be multiple of 4)
;       r0 = source address (must be word aligned)
;       r1 = destination address (must be word aligned)
;       r2 = number of bytes (must be multiple of 4)
;
; Leave with :
;		All registers unmodified
;		Parameters popped from stack
;*******************************************************************************

WordMove	PROC NEAR

		pushfd					; save original flags
		xchg	edi, [esp+8]	; edi = dest addr, save orig edi
		xchg	esi, [esp+12]	; esi = source addr, save orig esi
		xchg	ecx, [esp+16]	; ecx = size, save orig ecx
		shr		ecx, 2			; bytes to dwords
		jz		short move0		; if size zero, no-op
		cld						; go forwards
		cmp		edi, esi		;
		je		short move0		; if dest = source, no-op
		jb		short move_fwd	; if dest < source, go forwards
		lea		edi, [edi+ecx*4-4]	; start at inclusive end address for backwards copy
		lea		esi, [esi+ecx*4-4]
		std						; go backwards
move_fwd:
		rep movsd				; do copy
move0:
		popfd					; restore flags
		pop		ecx				; return address into ecx
		pop		edi				; restore edi
		pop		esi				; restore esi
		xchg	ecx, [esp]		; restore ecx, return address to stack
		ret

		ENDP

;*******************************************************************************
; Look up a parameter in a list of entries { int param no; int value; }
; Terminated by -ve parameter no.
;
; Enter with :
;		Parameter number following call
;		EAX = Address of list
;		EDI = address of super page
;		ESI = address of TRomHeader
;
; Return with:
;		If parameter present, CF=0 and EAX = parameter value
;		If parameter absent, CF=1, EAX undefined
;		Flags modified
;		No other registers modified
;		
;*******************************************************************************

FindParameter	PROC NEAR

		push	edx
		mov		edx, [esp+4]			; edx = return address
		add		dword ptr [esp+4], 4	; step return address past parameter number
		mov		edx, [edx]				; edx = parameter number
find_param_loop:
		cmp		dword ptr [eax], edx	; found?
		je		short found_param		; yes (note CF=0 if equal)
		cmp		dword ptr [eax], 0		; end of table?
		lea		eax, [eax+8]			; step to next entry
		jns		short find_param_loop	; loop if not end of table
		sub		eax, 12
		stc								; CF=1 to indicate not found
found_param:
		mov		eax, [eax+4]			; eax = parameter value if found
		pop		edx						; restore edx
		ret

		ENDP

;*******************************************************************************
; Call a function via the boot table
;
; Enter with :
;		EDI = address of super page
;		Word following call contains function number
;
;*******************************************************************************

DoBootCall	PROC NEAR

		push	eax						; save eax
		pushfd							; save flags
		mov		eax, [esp+8]			; return address
		add		dword ptr [esp+8], 4	; step return address past function number
		mov		eax, [eax]				; eax = function number
		shl		eax, 2					; byte offset
		add		eax, [edi + SSuperPageBase_iBootTable]
		mov		eax, [eax]				; offset of function from iCodeBase
		add		eax, [edi + SSuperPageBase_iCodeBase]
		popfd							; restore flags
		xchg	eax, [esp]				; function address to stack, restore eax
		ret								; jump to function, return to address after function number

		ENDP




;*******************************************************************************
; SUBROUTINE	ExciseRamArea
; Remove a block of RAM used from the free RAM list.
; Optionally make corresponding ROM table entries if this is 'ROM' code
; running in RAM.
; Enter with:
;	[esp+0]		Physical base address of area to excise
;	[esp+4]		Size of area to excise
;	[esp+8]		Linear base address of code block
;					(only required for ROM entry creation)
;		EDI = address of super page
;		ESI = address of TRomHeader
;		EBX = pointer to first SRamBank with some free RAM
;		EBP = pointer to next SRomBank to be filled in if needed
;			= 0 if filling in ROM blocks is not required
;
; Exit with:
;	EBX = pointer to first nonempty SRamBank following excised region
;		 may point to SRamBank list terminator
;	EBP = incremented past any newly created SRomBank entries
;	Flags modified
;	All other registers unmodified
;
;*******************************************************************************
ExciseRamArea	PROC NEAR

; Offsets from ESP
wssz = 64
R15 = wssz + 12
R0 = wssz + 12 + 4
R1 = wssz + 12 + 8
R2 = wssz + 12 + 12
R3 = 12
R4 = 16
R5 = 20
R6 = 24
R7 = 28
R8 = 32
R9 = 36
R11 = 44
R14 = 56

		push	eax
		push	ecx
		push	edx
		sub		esp, wssz					; workspace
		mov		[esp+R11], ebp
		PRINT4	[esp+R0], "ExciseRamArea Base";
		PRINT4	[esp+R1], "ExciseRamArea Size";
		PRINT4	[esp+R2], "ExciseRamArea LinB";
xra1:
		mov		edx, [ebx]					; edx = bank base
		mov		ecx, [ebx+4]				; ecx = bank size
		add		ebx, 8
		PRINT	edx, "ERA1 base"
		PRINT	ecx, "ERA1 size"
		cmp		ecx, 0
		FAULT	z							; fault if reached end of RAM list
		mov		eax, [esp+R0]
		sub		eax, edx					; eax = area base - RAM bank base
		FAULT	b							; if area base < RAM bank base, fault
		cmp		eax, ecx					; check if area begins in this bank
		jae		xra1						; if not, try next bank
		sub		ebx, 8						; rewind EBX to this bank

; at this point EBX -> RAM bank
; EDX = RAM bank base, ECX = RAM bank size
; EAX = offset into RAM bank of first byte to be excised
; [esp+R1] = size of remaining area to be excised
xra9:
		mov		ebp, ecx
		sub		ebp, eax					; ebp = remaining size of RAM bank
		cmp		ebp, [esp+R1]
		jbe		xra9a
		mov		ebp, [esp+R1]				; ebp = amount of this bank to take
xra9a:
		PRINT	eax, "ERA BankOffset"
		PRINT	ebp, "ERA Take"

		cmp		dword ptr [esp+R11], 0
		jz		xra8						; skip if SRomBank creation not required
		xchg	ebx, [esp+R11]
		mov		[esp+R4], eax
		mov		eax, [esp+R0]
		mov		[ebx+SRomBank_iBase], eax
		mov		[ebx+SRomBank_iSize], ebp
		mov		dword ptr [ebx+SRomBank_iWidth], ((ERomTypeRamAsRom SHL 8)+5)	; 32 bit wide, RAM as ROM
		mov		eax, [esp+R2]
		mov		[ebx+SRomBank_iLinBase], eax
		add		[esp+R2], ebp
		PRINT4	[ebx+SRomBank_iBase], "RomBank base"
		PRINT4	[ebx+SRomBank_iSize], "RomBank size"
		PRINT4	[ebx+SRomBank_iLinBase], "RomBank linbase"
		add		ebx, SRomBank_sz
		mov		eax, [esp+R4]
		xchg	ebx, [esp+R11]
xra8:
;	adjust RAM banks
		cmp		eax, 0
		je		xra2						; skip if we started at beginning of bank
		mov		[ebx+SRamBank_iSize], eax	; else adjust bank size
		sub		ecx, eax
		sub		ecx, ebp					; ecx = amount of bank remaining
		jnz		xra8a
		add		ebx, SRamBank_sz			; if nothing left, step to next bank
		jmp		xra3						; and branch out
xra8a:
		PRINT	ecx, "ECB2 remain"

; need to add a new RAM bank
		push	ebp
		lea		ebp, [ebx+4]				; ebp -> iSize of current bank
xra4:
		add		ebp, 8						; next bank
		cmp		dword ptr [ebp], 0			; test for terminator
		jne		xra4						; loop until ebp = address of terminator + 4
		sub		ebp, 4						; ebp = address of terminator
xra5:
		mov		eax, [ebp]					; move bank up
		mov		[ebp+8], eax
		mov		eax, [ebp+4]
		mov		[ebp+12], eax
		sub		ebp, 8						; previous bank
		cmp		ebp, ebx					; reached original bank?
		ja		short xra5					; if not, repeat
		add		ebx, 8						; where to put new bank
		pop		ebp
xra6:
		add		ebp, [esp+R0]				; base of new bank
		mov		[ebx], ebp
		mov		[ebx+4], ecx				; size of new bank
		PRINT4	[ebx], "ERA2 new base"
		PRINT4	[ebx+4], "ERA2 new size"
xra0:										; finished
		mov		ebp, [esp+R11]				; update EBP
		add		esp, wssz
		pop		edx
		pop		ecx
		pop		eax
		ret		12

xra2:
;	excision area begins at start of bank		
		sub		ecx, ebp					; ecx = amount of bank remaining
		jnz		xra6						; if nonzero, adjust bank base and finish

; entire bank is to be excised - remove it from the list
		push	ebp
		mov		ebp, ebx
xra7:
		mov		eax, [ebp+8]				; move next bank back
		mov		[ebp], eax
		mov		eax, [ebp+12]
		mov		[ebp+4], eax
		add		ebp, 8						; step to next bank, if any
		cmp		eax, 0						; did we just move terminator?
		jnz		xra7						; loop if not
		pop		ebp

; nothing remains of this bank
xra3:
		sub		dword ptr [esp+R1], ebp		; adjust remaining excision size
		jz		xra0						; if all done, finish
		mov		edx, [ebx]					; edx = bank base
		mov		[esp+R0], edx				; adjust physical base of remaining excision area
		mov		ecx, [ebx+4]				; ecx = bank size
		xor		eax, eax					; excision area starts at beginning of this bank
		jmp		xra9

		ENDP


IFDEF	CFG_MMDirect

;*******************************************************************************
; Subroutine RamPhysicalToLinear
; Converts a physical RAM address to a linear RAM address using the list of
; physical RAM banks, assuming the direct memory model mapping scheme
;
; Enter with:
;		EAX = physical address to be translated
;		EDI points to super page
;		ESI points to ROM header
;
; Return with:
;		EAX = translated linear address
;		Flags modified
;		No other registers modified
;*******************************************************************************
RamPhysicalToLinear PROC NEAR

		PRINT	eax, "RamPhysicalToLinear in"
		push	ebx
		push	ecx
		push	edx
		push	ebp
IF 0
		mov		ebp, [edi+SSuperPageBase_iRamBase]		; linear base
		mov		ebx, [edi+SSuperPageBase_iRamBootData]
		xor		ecx, ecx
		xor		edx, edx
rptl1:
		add		eax, edx
		add		ebp, ecx				; step linear address on
		mov		edx, [ebx]				; bank base
		mov		ecx, [ebx+4]			; bank size
		add		ebx, SRamBank_sz
		cmp		ecx, 0					; reached end of list?
		FAULT	e
		sub		eax, edx				; addr - bank base
		jb		short rptl1				; if -ve loop
		cmp		eax, ecx				; compare offset to bank size
		jae		short rptl1				; if offset >= size, next bank

; address is in this bank
		add		eax, ebp				; eax = addr - bank phys base + bank linear base
ENDIF
		PRINT	eax, "RamPhysicalToLinear out"
		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		ret

		ENDP


;*******************************************************************************
; Subroutine RamLinearToPhysical
; Converts a linear RAM address to a physical RAM address using the list of
; physical RAM banks, assuming the direct memory model mapping scheme
;
; Enter with:
;		EAX = linear address to be translated
;		EDI points to super page
;		ESI points to ROM header
;
; Return with:
;		EAX = translated physical address
;		Flags modified
;		No other registers modified
;*******************************************************************************
RamLinearToPhysical	PROC NEAR

		PRINT	eax, "RamLinearToPhysical in"
		push	ebx
		push	ecx
		push	ebp
IF 0
		mov		ebp, [edi+SSuperPageBase_iRamBase]		; linear base
		mov		ebx, [edi+SSuperPageBase_iRamBootData]
		xor		ecx, ecx
rltp1:
		add		ebp, ecx				; step linear address on
		mov		ecx, [ebx+4]			; bank size
		add		ebx, SRamBank_sz
		cmp		ecx, 0					; reached end of list?
		FAULT	e
		cmp		eax, ebp				; addr >= linear base?
		jb		short rltp1				; no - loop
		sub		eax, ebp				; eax = offset of given address from linear bank base
		cmp		eax, ecx				; offset < bank size ?
		jb		short rltp2				; yes - it's this bank
		add		eax, ebp				; no - restore eax
		jmp		short rltp1				; and loop
rltp2:
; address is in this bank
		add		eax, [ebx - SRamBank_sz]	; add bank physical base to offset
ENDIF
		PRINT	eax, "RamLinearToPhysical out"
		pop		ebp
		pop		ecx
		pop		ebx
		ret

		ENDP

	ENDIF

;*******************************************************************************
; Subroutine RomPhysicalToLinear
; Converts a physical ROM address to a linear ROM address using the list of
; physical ROM banks.
;
; Enter with:
;		EAX = physical address to be translated
;		EDI points to super page
;		ESI points to ROM header
;		SSuperPageBase::iRomBootData points to valid ROM bank table.
;
; Return with:
;		EAX = translated linear address
;		Flags modified
;		No other registers modified
;*******************************************************************************
RomPhysicalToLinear PROC NEAR

		PRINT	eax, "RomPhysicalToLinear in"
		push	ebx
		push	ecx
		push	edx
		mov		ebx, [edi+SSuperPageBase_iRomBootData]
		sub		ebx, SRomBank_sz
		xor		edx, edx
romptl1:
		add		eax, edx
		add		ebx, SRomBank_sz
		mov		edx, [ebx+SRomBank_iBase]		; bank base
		mov		ecx, [ebx+SRomBank_iSize]		; bank size
		cmp		ecx, 0							; reached end of list?
		FAULT	e
		sub		eax, edx						; addr - bank phys base
		jb		short romptl1					; if -ve loop
		cmp		eax, ecx						; compare offset to bank size
		jae		short romptl1					; if offset >= size, next bank

; address is in this bank
		add		eax, [ebx+SRomBank_iLinBase]	; eax = addr - bank phys base + bank lin base
		PRINT	eax, "RomPhysicalToLinear out"
		pop		edx
		pop		ecx
		pop		ebx
		ret

		ENDP


;*******************************************************************************
; Subroutine RomLinearToPhysical
; Converts a linear ROM address to a physical ROM address using the list of
; physical ROM banks.
;
; Enter with:
;		EAX = linear address to be translated
;		EDI points to super page
;		ESI points to ROM header
;		SSuperPageBase::iRomBootData points to valid ROM bank table.
;
; Return with:
;		EAX = translated physical address
;		Flags modified
;		No other registers modified
;*******************************************************************************
RomLinearToPhysical	PROC NEAR

		PRINT	eax, "RomLinearToPhysical in"
		push	ebx
		push	ecx
		push	edx
		mov		ebx, [edi+SSuperPageBase_iRomBootData]
		sub		ebx, SRomBank_sz
		xor		edx, edx
romltp1:
		add		eax, edx
		add		ebx, SRomBank_sz
		mov		edx, [ebx+SRomBank_iLinBase]	; bank linear base
		mov		ecx, [ebx+SRomBank_iSize]		; bank size
		cmp		ecx, 0							; reached end of list?
		FAULT	e
		sub		eax, edx						; addr - bank linear base
		jc		short romltp1					; if -ve, loop
		cmp		eax, ecx						; compare offset to bank size
		jae		short romltp1					; if offset >= size, next bank

; address is in this bank
		add		eax, [ebx+SRomBank_iBase]		; eax = addr - bank lin base + bank phys base
		PRINT	eax, "RomLinearToPhysical out"
		pop		edx
		pop		ecx
		pop		ebx
		ret

		ENDP


;*******************************************************************************
; Subroutine FindPrimary
; Sets up EBP to point to TRomImageHeader for the primary file
;
; Enter with:
;		EDI points to super page
;		ESI points to ROM header
;		SSuperPageBase::iActiveVariant set up
;		SSuperPageBase::iRomBootData points to valid ROM bank table.
;
; Exit with:
;		EBP pointing to TRomImageHeader for primary file
;		SSuperPageBase::iPrimaryEntry set up with linear address of TRomEntry
;		for primary file.
;		Flags modified
;		No other registers modified
;*******************************************************************************
FindPrimary PROC NEAR

		push	eax
		push	edx
		mov		ebp, [esi+TRomHeader_iPrimaryFile]	; ebp = linear address of TRomEntry for 1st primary
		PRINT	ebp, "iPrimaryFile"
		mov		edx, [edi+SSuperPageBase_iActiveVariant]	; edx = active HWVD
		PRINT	edx, "Active HWVD"
		rol		edx, 16					; active CPU into DL, active ASSP into DH
findprm1:
		mov		[edi+SSuperPageBase_iPrimaryEntry], ebp		; save linear address of TRomEntry
		mov		eax, ebp
		call	RomLinearToPhysical					; get physical address of TRomEntry
		mov		eax, [eax+TRomEntry_iAddressLin]	; linear address of TRomImageHeader
		call	RomLinearToPhysical					; get physical address of TRomImageHeader
		mov		ebp, eax							; into EBP
		mov		eax, [ebp+TRomImageHeader_iHardwareVariant]	; get hardware variant descriptor
		PRINT	eax, "File HWVD"
		cmp		eax, 04000000h
		jb		short FoundPrimary		; if layer <= 3 this one will do
		rol		eax, 16					; fileHWVD.parent into AL, fileHWVD.layer into AH
		cmp		al, 3					; parent>3 ?
		ja		short findprm2			; skip if so
		cmp		ah, dl					; CPU match?
		je		short FoundPrimary		; if so, this one will do
findprm_next:
		mov		ebp, [ebp+TRomImageHeader_iNextExtension]	; no good, step to next
		cmp		ebp, 0					; end of list?
		jne		short findprm1			; if not, try next
		FAULT							; else can't find primary :-(
findprm2:
		cmp		ax, dx					; check CPU + ASSP
		jne		short findprm_next		; no match - try next
		and		eax, edx				; match - check variant mask
		cmp		eax, 10000h
		jb		short findprm_next		; no good - try next image

FoundPrimary:
		pop		edx
		pop		eax
		PRINT	ebp, "Found Primary"
		ret

		ENDP


;*******************************************************************************
; Subroutine CheckForExtensionRom
; Tests if a new-style extension ROM is present at a given address.
;
; Enter with:
;		EAX = address of suspected extension ROM
;		EDI points to super page
;		ESI points to kernel ROM header
;
; Return with:
;		CF=1 if extension ROM present
;		CF=0 if extension ROM not present
;		Flags modified
;		No other registers modified
;*******************************************************************************
CheckForExtensionRom	PROC NEAR

		PRINT	eax, "?ExtROM"
		push	ecx
		push	edx
		mov		edx, [eax+TExtensionRomHeader_iKernelVersion]
		cmp		edx, [esi+TRomHeader_iVersion]
		jne		short chkext0
		mov		edx, [eax+TExtensionRomHeader_iKernelCheckSum]
		cmp		edx, [esi+TRomHeader_iCheckSum]
		jne		short chkext0
		mov		edx, [esi+TRomHeader_iTime+4]
		mov		ecx, [esi+TRomHeader_iTime]				; edx:ecx = kernel ROM time
		cmp		edx, [eax+TExtensionRomHeader_iKernelTime+4]
		jne		short chkext0
		cmp		ecx, [eax+TExtensionRomHeader_iKernelTime]
		jne		short chkext0
		stc
		sbb		ecx, [eax+TExtensionRomHeader_iTime]
		sbb		edx, [eax+TExtensionRomHeader_iTime+4]	; edx:ecx = kernel ROM time - extension ROM time - 1
		jge		short chkext0							; if kernel ROM time >= (extension ROM time + 1), no good
		PRTLN	"ExtROM OK"
		stc
		pop		edx
		pop		ecx
		ret

chkext0:
		PRTLN	"Not ExtROM"
		xor		edx, edx
		pop		edx
		pop		ecx
		ret

		ENDP


;*******************************************************************************
; Subroutine SetupSuperPageRunningFromRAM
; Calculates the address of the super page if we are running from RAM and we
; want the super page to come after the ROM image. Takes account of extension
; ROMs.
;
; Enter with:
;		ESI points to kernel ROM header, which is also the physical base 
;		address of the ROM in RAM
;
; Return with:
;		EDI contains calculated address of super page
;		No other registers modified
;*******************************************************************************
SetupSuperPageRunningFromRAM	PROC NEAR

		push	ecx
		push	edx

IFNDEF CFG_SupportEmulatedRomPaging
		mov		edi, [esi+TRomHeader_iPageableRomStart]		; size of unpaged part of ROM
		cmp		edi, 0
		jz		SetupSuperPageRunningFromRAM_UnPaged
		add		edi, esi									; adjust address to base of ROM in 
															; RAM plus size of unpaged ROM

		mov		cl, 32-CFG_RomSizeAlign
		mov		edx, 0ffffffffh
		shr		edx, cl
		add		edi, edx
		or		edi, edx									; round up to 64K (or CFG_RomSizeAlign)
		xor		edi, edx

		jmp		short SetupSuperPageRunningFromRAM_CheckExtensionRomHeader
ENDIF

SetupSuperPageRunningFromRAM_UnPaged:
		mov		edi, [esi+TRomHeader_iUncompressedSize]	; EDI = base ROM size
		add		edi, esi							; address after base ROM

		mov		cl, 32-CFG_RomSizeAlign
		mov		edx, 0ffffffffh
		shr		edx, cl
		add		edi, edx
		or		edi, edx									; round up to 64K (or CFG_RomSizeAlign)
		xor		edi, edx

SetupSuperPageRunningFromRAM_CheckExtensionRomHeader:

; edi = Physical base address of Extension ROM header which is equivalent to 
; 		the end of 'normal' ROM in RAM

		mov		edx, [edi+TExtensionRomHeader_iKernelVersion]
		cmp		edx, [esi+TRomHeader_iVersion]
		jne		short ssup0
		mov		edx, [edi+TExtensionRomHeader_iKernelCheckSum]
		cmp		edx, [esi+TRomHeader_iCheckSum]
		jne		short ssup0
		mov		edx, [esi+TRomHeader_iTime+4]
		mov		ecx, [esi+TRomHeader_iTime]				; edx:ecx = kernel ROM time
		cmp		edx, [edi+TExtensionRomHeader_iKernelTime+4]
		jne		short ssup0
		cmp		ecx, [edi+TExtensionRomHeader_iKernelTime]
		jne		short ssup0
		stc
		sbb		ecx, [edi+TExtensionRomHeader_iTime]
		sbb		edx, [edi+TExtensionRomHeader_iTime+4]	; edx:ecx = kernel ROM time - extension ROM time - 1
		jge		short ssup0								; if kernel ROM time >= (extension ROM time + 1), no good
		add		edi, [edi+TExtensionRomHeader_iUncompressedSize]	; step past extension ROM

		mov		cl, 32-CFG_RomSizeAlign
		mov		edx, 0ffffffffh
		shr		edx, cl
		add		edi, edx
		or		edi, edx								; round up to 64K (or CFG_RomSizeAlign)
		xor		edi, edx

ssup0:
		pop		edx
		pop		ecx
		ret

		ENDP


;*******************************************************************************
; Subroutine RelocateSuperPage
; Relocate the super page and CPU page
;
; Enter with:
;		EAX	= Final address of super page
;		EDI	= Current address of super page
;
; Return with:
;		EDI	= Initial EAX
;		ESP updated if necessary
;*******************************************************************************
RelocateSuperPage	PROC NEAR
		
		PRINT	edi, "RelocateSuperPage Initial"
		PRINT	eax, "RelocateSuperPage Destination"
		PRINT	esp, "RelocateSuperPage Initial SP"
		cmp		eax, edi
		jne		short reloc_sp_ne
		ret
reloc_sp_ne:
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	ebp
		lea		ebp, [edi+CpuBootStackTop]	; upper limit for bounds check

; Zero destination to begin with
		push	0
		push	KPageSize
		push	eax
		call	WordFill

; Copy SSuperPageBase valid fields
		ADR		ebx, RelocSPOffsetTable
		sub		ebx, 4
reloc_sp1:
		add		ebx, 4
		mov		ecx, [ebx]					; offset + flags
		test	ecx, ecx
		js		short reloc_sp2				; branch out if reached end
		test	ecx, 10000000h
		movzx	ecx, cx						; clear flags
		mov		edx, [edi+ecx]				; eax = old super page value
		jz		short reloc_sp3				; skip if straight copy need
		cmp		edx, edi					; value >= initial base?
		jb		short reloc_sp3				; no - just copy
		cmp		edx, ebp					; value < upper bound?
		jae		short reloc_sp3				; no - just copy
		sub		edx, edi
		add		edx, eax					; new value = old value - old base + new base
reloc_sp3:
		mov		[eax+ecx], edx				; store new super page value
		jmp		short reloc_sp1
reloc_sp2:
; copy the entire CPU page
		push	KPageSize
		push	edi
		add		dword ptr [esp], CpuPageOffset		
		push	eax
		add		dword ptr [esp], CpuPageOffset
		call	WordMove

; relocate the stack pointer if necessary
		lea		edx, [esp+24]				; value of ESP before calling function
		cmp		edx, edi
		jb		short reloc_sp4
		cmp		edx, ebp
		jae		short reloc_sp4
		sub		esp, edi
		lea		esp, [esp+eax+24]			; new esp = old esp - old base + new base + 24
		mov		ebx, [edx-4]				; return address
		push	ebx							; move to new stack
		mov		ebx, [edx-8]				; saved EAX
		push	ebx							; move to new stack
		mov		ebx, [edx-12]				; saved EBX
		push	ebx							; move to new stack
		mov		ebx, [edx-16]				; saved ECX
		push	ebx							; move to new stack
		mov		ebx, [edx-20]				; saved EDX
		push	ebx							; move to new stack
		mov		ebx, [edx-24]				; saved EBP
		push	ebx							; move to new stack
reloc_sp4:
; Change to new super page location
		xchg	eax, edi					; EDI = final super page address, EAX = initial
		sub		eax, edi
		neg		eax							; EAX = final - initial
		PRINT	edi, "RelocateSuperPage Final"
		lea		ebx, [esp+20]
		PRINT	ebx, "RelocateSuperPage Final SP"
		mov		cl, BMA_Reloc
		BOOTCALL BTF_Alloc					; fix up allocator
		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
		ret

RelocSPOffsetTable:
		DD		SSuperPageBase_iTotalRomSize
		DD		SSuperPageBase_iTotalRamSize
		DD		SSuperPageBase_iRamBootData + 10000000h
		DD		SSuperPageBase_iRomBootData + 10000000h
		DD		SSuperPageBase_iBootTable + 10000000h
		DD		SSuperPageBase_iCodeBase
		DD		SSuperPageBase_iBootFlags
		DD		SSuperPageBase_iBootAlloc + 10000000h
		DD		SSuperPageBase_iMachineData + 10000000h
		DD		SSuperPageBase_iActiveVariant
		DD		SSuperPageBase_iPrimaryEntry
		DD		SSuperPageBase_iDebugPort
		DD		SSuperPageBase_iCpuId
		DD		SSuperPageBase_iRootDirList
		DD		SSuperPageBase_iKernelLimit
		DD		SSuperPageBase_iRamBase
		DD		-1

		ENDP




;*******************************************************************************
; MMU code
;*******************************************************************************

IFNDEF	CFG_MMDirect

;*******************************************************************************
; Allocate memory and map it to consecutive linear addresses
; Enter with
;	[esp+0]		Base Virtual Address
;	[esp+4]		Permissions (offset in boot table)
;	[esp+8]		Total size of mapping required
;	[esp+12]	log2(maximum page size to consider)
;	EDI points to super page
;
; Return with
;	Flags modified
;	All registers unmodified
;	Parameters popped
;*******************************************************************************
AllocAndMap	PROC NEAR

		PRINT4	[esp+4], "AllocAndMap virt"
		PRINT4	[esp+8], "AllocAndMap perm"
		PRINT4	[esp+12], "AllocAndMap size"
		PRINT4	[esp+16], "AllocAndMap page"
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	ebp
		mov		edx, [esp+24]			; linear address
		mov		ebx, [esp+32]			; size
		mov		eax, 0fffh
		and		eax, edx
		add		ebx, eax
		and		edx, 0fffff000h			; round address down to page
		add		ebx, 0fffh
		and		ebx, 0fffff000h			; round size up to page
allocandmap1:
		mov		ch, 12					; start with 4K
		test	dword ptr [edi+SSuperPageBase_iCpuId], EX86Feat_PSE
		jz		short allocandmap2		; CPU doesn't support 4M pages, use 4K
		cmp		ebx, 400000h			; size >= 4M?
		jb		short allocandmap2		; if not, use 4K
		test	edx, 0ffc00000h			; VA multiple of 4M?
		jnz		short allocandmap2		; if not, use 4K
		mov		ch, [esp+36]			; CH = log2(max page size)
allocandmap2:
		mov		cl, BMA_Kernel			; allocate general kernel RAM
		BOOTCALL BTF_Alloc				; allocate RAM, physical address -> EAX
										; CH reduced if necessary to reflect size allocated
		mov		ebp, [esp+28]			; permissions
		call	MapSingleEntry			; map next page
		cmp		ebx, 0
		jnz		short allocandmap1		; loop if not finished

		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
		ret		16

		ENDP

ENDIF ; CFG_MMDirect

;*******************************************************************************
; Map a contiguous region
; Enter with
;	[esp+0]		Base Virtual Address
;	[esp+4]		Base Physical Address
;	[esp+8]		Permissions (offset in boot table)
;	[esp+12]	Total size of mapping required
;	[esp+16]	log2(maximum page size to consider)
;	EDI points to super page
;
; Return with
;	Flags modified
;	All registers unmodified
;	Parameters popped
;*******************************************************************************
MapContiguous PROC NEAR

		PRINT4	[esp+4], "MapCont virt"
		PRINT4	[esp+8], "MapCont phys"
		PRINT4	[esp+12], "MapCont perm"
		PRINT4	[esp+16], "MapCont size"
		PRINT4	[esp+20], "MapCont page"
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	ebp
		mov		edx, [esp+24]			; linear address
		mov		eax, [esp+28]			; physical address
		mov		ebp, [esp+32]			; permissions
		mov		ebx, [esp+36]			; size
		mov		ch, 12					; start with 4K
		test	dword ptr [edi+SSuperPageBase_iCpuId], EX86Feat_PSE
		jz		short mapcont1			; CPU doesn't support 4M pages, use 4K
		mov		ch, [esp+40]			; CH = log2(max page size)
mapcont1:
		call	MapSingleEntry			; map next page
		cmp		ebx, 0
		jnz		short mapcont1			; loop if not finished

		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
		ret		20

		ENDP

;*******************************************************************************
; Make a single MMU mapping of either 4K or 4M
; Enter with
;	EAX = physical address
;	EDX = virtual address
;	EBP = permissions (offset in boot table)
;	EBX = total size of mapping required
;	CH = log2(maximum page size to consider)
;	EDI points to super page
;	ESI points to ROM header
;
; Return with
;	EAX, EDX incremented by size of entry made
;	EBX decremented by size of entry made
;	other registers unmodified
;
;*******************************************************************************
MapSingleEntry	PROC NEAR
		PRINT	edx, "MapSglE virt"
		PRINT	eax, "MapSglE phys"
		PRINT	ebp, "MapSglE perm"
		PRINT	ebx, "MapSglE size"
		PRINTB	ch, "MapSglE page"
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	ebp
		test	eax, 3fffffh				; PA mult of 4M ?
		jnz		MapSinglePage				; if not, use page
		test	edx, 3fffffh				; VA mult of 4M ?
		jnz		MapSinglePage				; if not, use page
		cmp		ebx, 400000h				; size >= 4M ?
		jb		MapSinglePage				; if not, use page
		cmp		ch, 22						; section wanted?
		jb		MapSinglePage				; if not, use page

; use section
MapSingleSection:
		mov		ch, 22
		BOOTCALL BTF_GetPdePerm				; PDE permissions into EAX
		or		eax, [esp+16]				; PDE for mapping
		shr		edx, 20						; offset into page directory
		add		edx, [edi+SSuperPageBase_iPageDir]
		PRINT	edx, "MSS: PDA"
		PRINT	eax, "MSS: PDE"
		cmp		dword ptr [edx], 0
		FAULT	nz							; fail if there is an existing PDE
		mov		[edx], eax					; write new PDE
		mov		eax, edx					; page table address updated
		mov		edx, [esp+4]				; VA for mapping, CH=size
		BOOTCALL BTF_PTUpdate
		pop		ebp							; recover initial register contents
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
		add		eax, 400000h				; step PA by 4M
		add		edx, 400000h				; step VA by 4M
		sub		ebx, 400000h				; reduce size by 4M
		ret

; use page
MapSinglePage:
		mov		ch, 12
		BOOTCALL BTF_GetPdePerm				; PDE permissions into EAX
		call	LookupPageTable				; find page table
											; if exists, return access address in EAX and CF=1
											; else return CF=0, EAX unchanged
		jc		short mapsinglepage1
		call	NewPageTable				; allocate a new page table and map it, return access address in EAX
mapsinglepage1:
		push	eax							; save page table access address
		mov		ch, 12
		BOOTCALL BTF_GetPtePerm				; get PTE permissions into EAX
		or		eax, [esp+20]				; PTE for new mapping
		pop		edx							; recover page table access address
		mov		ebx, [esp+4]				; VA for mapping
		shr		ebx, 10						;
		and		ebx, 0ffch					; offset into PTE
		add		edx, ebx					; address of PTE
		PRINT	edx, "MSP: PTA"
		PRINT	eax, "MSP: PTE"
		cmp		dword ptr [edx], 0
		FAULT	nz							; fail if there is an existing PTE
		mov		[edx], eax					; write new PTE
		mov		eax, edx					; page table address updated
		mov		edx, [esp+4]				; VA for mapping, CH=size
		BOOTCALL BTF_PTUpdate
		pop		ebp							; recover initial register contents
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
		add		eax, 1000h					; step PA by 4K
		add		edx, 1000h					; step VA by 4K
		sub		ebx, 1000h					; reduce size by 4K
		ret

		ENDP


;*******************************************************************************
; Check if a virtual address is mapped by a page table
; Enter with
;	EDX = virtual address
;	EAX = PDE permissions for page table (for comparison)
;	EDI points to super page
;
; Return with
;	EAX = access address of page table, CF=1 if page table exists
;	EAX unchanged, CF=0 if page table does not exist
;	other registers unmodified
;
;*******************************************************************************
LookupPageTable	PROC NEAR

		PRINT	edx, "LookupPT virt"
		PRINT	eax, "LookupPT perm"
		push	ebx
		push	ecx
		push	edx
		push	ebp
		mov		ebp, [edi+SSuperPageBase_iPageDir]
		shr		edx, 22
		mov		ebx, [ebp+edx*4]
		test	bl, PDE_P					; sets CF=0
		jz		lookuppt0
		xor		ebx, eax
		test	ebx, PTE_SOFT				; sets CF=0
		jnz		lookuppt0
		and		ebx, 0fffff000h				; EBX = page table phys addr
		PRINT	ebx, "PTPhys"

; Page table is present, EBX contains physical address
; Find access address
		mov		eax, cr0
		test	eax, CR0_PG
		mov		eax, ebx
		jz		lookuppt1					; if MMU off, return phys addr and CF=1
		call	FindPageTableVA				; else find virtual address of page table
lookuppt1:
		stc
lookuppt0:
		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		ret

		ENDP


;*******************************************************************************
; Find the virtual address of a page table from its physical address
; Enter with
;	EAX = Page table physical address
;	EDI points to super page
;
; Return with
;	EAX = virtual address of page table
;	other registers unmodified
;
;*******************************************************************************
FindPageTableVA	PROC NEAR
		PRINT	eax, "FindPageTableVA phys"
IFDEF CFG_MMDirect
; For direct model RAM mapping is known
		call	RamPhysicalToLinear
ELSE
		push	ebp
		push	ebx
		push	ecx
		push	edx
		xor		ecx, ecx					; ECX = page table index
		mov		ebp, KPageTableBase			; EBP = PT0 linear
		mov		ebx, cr0
		test	ebx, CR0_PG					; MMU on?
		jnz		short fptva1				; skip if on
		shr		ebp, 20						; EBP = PDE index for PT0
		and		ebp, 0ffch					;
		add		ebp, [edi+SSuperPageBase_iPageDir]
		mov		ebp, [ebp]					; EBP = PDE for PT0
		and		ebp, 0fffff000h				; EBP = PT0 physical
fptva1:
		mov		edx, [ebp+ecx*4]			; EDX = PT0[index]
		xor		edx, eax					; XOR with supplied physical address
		shr		edx, 12						; zero if physical address matches
		jz		short fptva2				; branch out if match found
		inc		ecx							; else increment index
		cmp		ch, 4						; reached end?
		jb		short fptva1				; no - loop
		FAULT								; can't find VA so give up
fptva2:
		shl		ecx, 12						; ECX = match index * 4K
		lea		eax, [ecx+KPageTableBase]	; EAX = Page table VA
		pop		edx
		pop		ecx
		pop		ebx
		pop		ebp
ENDIF
		PRINT	eax, "FindPageTableVA virt"
		ret

		ENDP


;*******************************************************************************
; Allocate a new page table and map a virtual address with it
; Enter with
;	EDX = virtual address
;	EAX = PDE permissions for page table
;	EDI points to super page
;
; Return with
;	EAX = access address of page table
;	other registers unmodified
;
;*******************************************************************************
NewPageTable	PROC NEAR

		PRINT	edx, "NewPageTable virtual"
		PRINT	eax, "NewPageTable PDEperm"
		push	ebx
		push	ecx
		push	edx
		push	ebp
		push	eax							; save PDE permissions
		mov		cl, BMA_PageTable
		BOOTCALL BTF_Alloc					; allocate page table, phys addr into EAX
		push	eax							; save ptphys
		or		eax, [esp+4]				; PDE for mapping
		shr		edx, 20						; offset into page directory
		and		dl, 0fch
		add		edx, [edi+SSuperPageBase_iPageDir]
		PRINT	edx, "NPT: PDA"
		PRINT	eax, "NPT: PDE"
		PRINT4	[edx], "NPT: OldPDE"
		cmp		dword ptr [edx], 0
		FAULT	nz							; fail if there is an existing PDE
		mov		[edx], eax					; write new PDE
		mov		eax, edx					; page table address updated
		mov		ch, 22
		mov		edx, [esp+12]				; VA for mapping, CH=size
		BOOTCALL BTF_PTUpdate
		mov		eax, [esp]					; EAX = ptphys

IFDEF CFG_MMDirect

		add		esp, 4
		mov		ebx, cr0
		test	ebx, CR0_PG					; MMU on?
		jz		short npt0					; if not, access address = physical address
		call	RamPhysicalToLinear			; use known mapping to get virtual address
npt0:

ELSE	; CFG_MMDirect

; find next free VA for page table
		xor		ecx, ecx					; ECX = page table index
		mov		ebp, KPageTableBase			; EBP = PT0 linear
		mov		ebx, cr0
		test	ebx, CR0_PG					; MMU on?
		jnz		short npt1					; skip if on
		shr		ebp, 20						; EBP = PDE index for PT0
		and		ebp, 0ffch					;
		add		ebp, [edi+SSuperPageBase_iPageDir]
;		PRINT	ebp, "PD+PTBx"
		mov		ebp, [ebp]					; EBP = PDE for PT0
;		PRINT	ebp, "PT0PDE"
		and		ebp, 0fffff000h				; EBP = PT0 physical
npt1:
		cmp		dword ptr [ebp+ecx*4], 0	; test PT0[index]
		jz		short npt2					; if zero, we have a free VA
		inc		ecx							; else increment index
		cmp		ch, 4						; reached end?
		jb		short npt1					; no - loop
		FAULT								; can't find VA so give up
npt2:
;		PRINT	ecx, "ix"
		lea		edx, [ebp+ecx*4]			; EDX = address of PT0 PTE
;		PRINT	edx, "&PT0PTE"
		shl		ecx, 12						; ECX = VA offset of page table from PT0
		add		ecx, KPageTableBase			; VA of page table
		push	ecx							; save it
		mov		ch, 12						; 4K page
		mov		ebp, BTP_PageTable
		BOOTCALL BTF_GetPtePerm				; PTE permissions for page table into EAX
		or		eax, [esp+4]				; EAX = PT0 PTE
		PRINT	edx, "NPT: PTA"
		PRINT	eax, "NPT: PTE"
		mov		[edx], eax					; write new PTE
		mov		eax, edx					; page table address updated
		mov		ch, 12
		mov		edx, [esp]					; VA for mapping, CH=size
		BOOTCALL BTF_PTUpdate
		pop		eax							; EAX = VA of page table
		pop		ecx							; ECX = PA of page table
		mov		ebx, cr0
		test	ebx, CR0_PG					; MMU on?
		jnz		short npt3					; skip if on
		mov		eax, ecx					; else access address = PA
npt3:
		push	0
		push	1000h
		push	eax
		call	WordFill					; clear page table
		mov		ch, 12 + 64					; size
		BOOTCALL BTF_PTUpdate
		PRINT	eax, "NewPageTable access"

ENDIF	; CFG_MMDirect

		pop		ebp
		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		ret

		ENDP


;*******************************************************************************
; Initialise the memory mapping system
; Allocate the page directory and the first page table
; Clear them both (apart from RAM drive PDEs)
; Map the page table
; Map the page directory
; Direct model just allocates and clears the page directory.
;
; Enter with
;	EDI points to super page
;	ESI points to the ROM header
;
; Return with
;	No registers modified
;
;*******************************************************************************
InitMemMapSystem PROC NEAR

		push	eax
		push	ebx
		push	ecx
		push	edx
		push	ebp

; Allocate page directory
		mov		cl, BMA_PageDirectory
		BOOTCALL BTF_Alloc				; EAX = PDphys
		PRINT	eax, "PageDirPhys"
		mov		[edi+SSuperPageBase_iPageDir], eax
		mov		ebp, eax

IFDEF CFG_MMDirect

		push	0
		push	1000h
		push	eax
		call	WordFill				; clear page directory

ELSE

IFDEF CFG_MMMultiple
		push	0
		push	((KRamDriveStartAddress SHR 22) SHL 2)
		push	eax
		call	WordFill				; clear page directory below RAM drive
		mov		eax, ((KRamDriveEndAddress SHR 22) SHL 2)
		push	0
		push	1000h
		sub		[esp], eax
		add		eax, ebp
		push	eax
		call	WordFill				; clear page directory above RAM drive
ELSE ; flexible
		push	0
		push	1000h
		push	eax
		call	WordFill				; clear page directory
ENDIF
		mov		cl, BMA_PageTable
		BOOTCALL BTF_Alloc				; EAX = PT0phys
		PRINT	eax, "PT0"
		push	0
		push	1000h
		push	eax
		call	WordFill				; clear PT0
		push	eax						; save PT0phys
		mov		ch, 12					; 4K page
		push	ebp
		mov		ebp, BTP_PageTable
		BOOTCALL BTF_GetPdePerm			; PDE permissions for page table into EAX
		pop		ebp
		or		eax, [esp]				; PDE for PT0 mapping
		lea		edx, [ebp + ((KPageTableBase SHR 22) SHL 2)]
		PRINT	edx, "PT0 PDE addr"
		PRINT	eax, "PT0 PDE"
		mov		[edx], eax				; install PT0 PDE
		mov		ch, 12					; 4K page
		push	ebp
		mov		ebp, BTP_PageTable
		BOOTCALL BTF_GetPtePerm			; PTE permissions for page table into EAX
		pop		ebp
		pop		edx						; EDX = PT0 physical address
		or		eax, edx				; PTE for PT0 mapping
		PRINT	edx, "PT0 PTE addr"
		PRINT	eax, "PT0 PTE"
		mov		[edx], eax				; install PT0 PTE

		push	12						; page size for mapping
		push	1000h					; total size of mapping
		push	BTP_PageTable			; permissions for mapping
		push	ebp						; physical address
		push	KPageDirectoryBase		; virtual address
		call	MapContiguous			; map page directory

ENDIF	; CFG_MMDirect

		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
		ret

		ENDP


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
;	EDI = super page physical address
;	EBP = kernel TRomImageHeader physical address
;	ESI = ROM header physical address
;	ESP = stack physical address
;
; Return with
;	EDI, EBP, ESI, ESP converted to linear addresses
;	EAX, EBX, ECX, EDX, Flags modified
;*******************************************************************************
SwitchToVirtual	PROC NEAR

		lea		ebx, [edi+CpuBootStackOffset]	; ebx points to SSwitch block
		mov		[ebx+SSwitch_iRomHdrPhys], esi
		PRINT	esi, "RomHdrPhys"
		mov		[ebx+SSwitch_iSuperPagePhys], edi
		PRINT	edi, "SuperPagePhys"
		mov		[ebx+SSwitch_iStackPhys], esp
		PRINT	esp, "StackPhys"
		mov		[ebx+SSwitch_iImgHdrPhys], ebp
		PRINT	ebp, "ImgHdrPhys"
		mov		eax, [edi+SSuperPageBase_iPageDir]
		mov		[ebx+SSwitch_iPageDirPhys], eax
		PRINT	eax, "PageDirPhys"

		mov		eax, esi
		call	RomPhysicalToLinear		; Calculate linear ROM header address
		mov		[ebx+SSwitch_iRomHdrLin], eax
		PRINT	eax, "RomHdrLin"
		mov		eax, ebp
		call	RomPhysicalToLinear		; Calculate linear primary image header address
		mov		[ebx+SSwitch_iImgHdrLin], eax
		PRINT	eax, "ImgHdrLin"
IFDEF CFG_MMDirect
		mov		eax, edi
		call	RamPhysicalToLinear
ELSE
		mov		eax, KSuperPageLinAddr
ENDIF
		mov		[ebx+SSwitch_iSuperPageLin], eax
		PRINT	eax, "SuperPageLin"
IFDEF CFG_MMDirect
		mov		eax, esp
		call	RamPhysicalToLinear
ELSE
		mov		eax, esp
		sub		eax, edi
		add		eax, KSuperPageLinAddr
ENDIF
		mov		[ebx+SSwitch_iStackLin], eax
		PRINT	eax, "StackLin"
IFDEF CFG_MMDirect
		mov		eax, [edi+SSuperPageBase_iPageDir]
		call	RamPhysicalToLinear
ELSE
		mov		eax, KPageDirectoryBase
ENDIF
		mov		[ebx+SSwitch_iPageDirLin], eax
		PRINT	eax, "PageDirLin"
		mov		dword ptr [ebx+SSwitch_iTempPdeOffset], 0ffffffffh
		cmp		esi, [ebx+SSwitch_iRomHdrLin]
		je		no_temp_pt				; If bootstrap is already identity mapped, no temporary mapping needed

		mov		cl, BMA_PageTable
		BOOTCALL BTF_Alloc				; Allocate temporary page table. Physical address into EAX.
		PRINT	eax, "TempPtPhys"
		mov		[ebx+SSwitch_iTempPtPhys], eax
		mov		edx, esi
		shr		edx, 22					; Temp PDE offset
		mov		[ebx+SSwitch_iTempPdeOffset], edx
		PRINT	edx, "TempPdeOffset"
		shl		edx, 2
		add		edx, [edi+SSuperPageBase_iPageDir]
		mov		ecx, [edx]				; old PDE
		mov		[ebx+SSwitch_iOrigPde], ecx
		PRINT	ecx, "OrigPde"
		test	cl, PDE_P				; check if old PDE is empty
		jnz		short old_pde_present
		push	0
		push	1000h
		push	eax
		call	WordFill				; if empty, fill temp page table with 0
		jmp		short install_temp_pde
old_pde_present:
		push	1000h
		push	ecx
		push	eax
		and		dword ptr [esp+4], 0fffff000h
		call	WordMove				; else copy original page table into temp
install_temp_pde:
		mov		ch, 12
		mov		ebp, BTP_Temp
		BOOTCALL BTF_GetPdePerm			; get PDE permissions for temp mapping into EAX
		or		eax, [ebx+SSwitch_iTempPtPhys]
		mov		[edx], eax				; install temp PDE
		PRINT	edx, "TMP: PDA"
		PRINT	eax, "TMP: PDE"
		mov		eax, esi
		shr		eax, 10
		and		eax, 0ffch				; PTE index * 4
		add		eax, [ebx+SSwitch_iTempPtPhys]
		mov		[ebx+SSwitch_iTempPtePtr], eax	; pointer to entry controlling identity mapped ROM header
		PRINT	eax, "TempPtePtr"
		mov		ch, 12
		mov		ebp, BTP_Temp
		BOOTCALL BTF_GetPtePerm			; get PTE permissions for temp mapping into EAX
		or		eax, esi				; PTE value which would map ROM header
		mov		[ebx+SSwitch_iTempPte], eax
		PRINT	eax, "TempPte"

no_temp_pt:
		; The following call enables the MMU
		; On return EIP, ESP and EDI have been changed to linear addresses
		; The temporary PDE has been replaced with the original PDE
		BOOTCALL	BTF_EnableMMU		; ENABLE THE MMU - RETURN TO LINEAR ADDRESSES

		lea		ebx, [edi+CpuBootStackOffset]		; ebx points to SSwitch block
		mov		eax, [ebx+SSwitch_iPageDirLin]
		mov		[edi+SSuperPageBase_iPageDir], eax	; switch to linear page directory
		mov		esi, [ebx+SSwitch_iRomHdrLin]		; reload ESI with linear ROM header pointer
		mov		ebp, [ebx+SSwitch_iImgHdrLin]		; reload EBP with linear image header pointer

		; Fix up the super page
		; NOTE - Can't do debug prints until after this section
		mov		edx, edi
		sub		edx, [ebx+SSwitch_iSuperPagePhys]	; edx = offset from phys to lin super page
		mov		ecx, esi
		sub		ecx, [ebx+SSwitch_iRomHdrPhys]		; ecx = offset from phys to lin ROM header
		add		[edi+SSuperPageBase_iRamBootData], edx
		add		[edi+SSuperPageBase_iRomBootData], edx
		add		[edi+SSuperPageBase_iMachineData], edx
		add		[edi+SSuperPageBase_iBootAlloc], edx
		add		[edi+SSuperPageBase_iBootTable], ecx
		add		[edi+SSuperPageBase_iCodeBase], ecx

		; Fix up the IDT
		call	SetupIDT

		; Fix return address
		add		[esp], ecx

		; Fix up allocator
		mov		eax, edx
		mov		cl, BMA_Reloc
		BOOTCALL BTF_Alloc

		PRINT	edi, "Return EDI"
		PRINT	esi, "Return ESI"
		PRINT	ebp, "Return EBP"
		PRINT	esp, "Return ESP"
		PRTLN	"Super page/CPU page:"
		MEMDUMP	edi, SuperCpuSize
		ret

		ENDP


IFNDEF	CFG_MMDirect

;*******************************************************************************
; Memory allocator for virtual memory models
;*******************************************************************************

;*******************************************************************************
; Determine if a region under test overlaps a preallocated block
; 
; Enter with
;	[esp+0]	= base of region under test
;	[esp+4]	= size of region under test - 1
;	EDI = pointer to SSuperPageBase
;	EBP = pointer to SAllocData
;
; Return with
;	If there is an overlap, EAX = address after the preallocated block and CF=1
;	If there is no overlap, EAX = original base and CF=0
;	No registers modified other than EAX, EFLAGS
;	Stack parameters popped
;*******************************************************************************
CheckPreAlloc	PROC NEAR

;		PRINT4	[esp+4], "ChkPre base"
;		PRINT4	[esp+8], "ChkPre size"
		push	ebx
		push	ecx
		push	edx
		push	ebp
		mov		edx, [ebp+SAllocData_iExtraSkip]	; edx = extra skip base
		mov		ecx, [ebp+SAllocData_iExtraSkip+4]	; ecx = extra skip size
		mov		ebp, [ebp+SAllocData_iSkip]			; ebp points to skip ranges
chkpre1:
		; edx = skip range base, ecx = skip range size
;		PRINT	edx, "Skip base"
;		PRINT	ecx, "Skip size"
		lea		ecx, [edx+ecx-1]					; ecx = inclusive end of skip range
		mov		eax, [esp+20]
		add		eax, [esp+24]						; eax = inclusive end of region under test
		cmp		edx, [esp+20]
		jae		short chkpre2
		mov		edx, [esp+20]						; edx = max(region base, skip base) = intersection base
chkpre2:
		cmp		eax, ecx
		jbe		short chkpre3
		mov		eax, ecx							; eax = intersection inclusive end
chkpre3:
;		PRINT	edx, "Int base"
;		PRINT	eax, "Int end"
		cmp		edx, eax							; edx > eax iff intersection empty
		jbe		short chkpre4						; branch if intersection nonempty
		mov		edx, [ebp]							; base of next skip range
		mov		ecx, [ebp+4]						; size of next skip range
		add		ebp, 8
		cmp		ecx, 0								; reached end of range?
		jne		chkpre1								; loop if not
		jmp		chkpre0								; return with CF=0
chkpre4:
		PRINT4	[esp+20], "OVERLAP RUT  BASE"
		PRINT4	[esp+24], "OVERLAP RUT  SIZE-1"
		PRINT	edx, "OVERLAP SKIP BASE"
		PRINT	ecx, "OVERLAP SKIP END"
		inc		ecx
		mov		[esp+20], ecx						; r0 = inclusive end of skip
		stc											; return with CF=1
chkpre0:
		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		mov		eax, [esp+4]
		ret		8

		ENDP


;*******************************************************************************
; Advance an SPos structure past the next valid block
; 
; Enter with
;	EBX	= pointer to SPos structure
;	EDI = pointer to SSuperPageBase
;	EBP = pointer to SAllocData
;
; Return with
;	Address of next valid block in EAX
;	SRamBank pointer in R1, bank base in EDX, bank size in ECX
;	EBX, EBP, ESI, EDI unmodified
;*******************************************************************************
FindValidBlock	PROC NEAR

		push	ebp
		mov		eax, [ebx+0]					; eax = p->iNext
		mov		ebp, [ebx+4]					; ebp = p->iBank
		PRINT	eax, "FVB next"
		PRINT	ebp, "FVB bank"
		PRINT4	[ebx+8], "FVB mask"
		mov		edx, [ebp+0]					; edx = bank base
		mov		ecx, [ebp+4]					; ecx = bank size
fvb1:
		add		eax, [ebx+8]					; eax += p->iMask
		jc		fvb0							; if address wraps, OOM
		or		eax, [ebx+8]
		xor		eax, [ebx+8]					; round to required boundary
		PRINT	eax, "FVB try"
		PRINT	edx, "FVB bank base"
		PRINT	ecx, "FVB bank size"
		push	esi
		mov		esi, eax
		sub		esi, edx						; esi = offset into bank
		FAULT	b								; shouldn't be negative
		add		esi, [ebx+8]					; add size of region - 1
		cmp		esi, ecx						; compare with bank size
		jae		fvb2							; if >= bank size we have stepped outside bank
		push	ebp
		mov		ebp, [esp+8]					; retrieve original EBP
		push	dword ptr [ebx+8]
		push	eax
		call	CheckPreAlloc					; check we don't overlap a skip range
		pop		ebp
		pop		esi
		jc		fvb1							; if we do, advance past skip and try again
		mov		[ebx], eax						; otherwise we have a valid block
		or		byte ptr [ebx], 1				; set p->iNext = addr + 1
		PRINT	eax, "FVB ret"
		pop		ebp
		ret
fvb2:
		pop		esi
		add		ebp, SRamBank_sz				; step to next RAM bank
		mov		edx, [ebp+0]					; edx = bank base
		mov		ecx, [ebp+4]					; ecx = bank size
		mov		[ebx+4], ebp					; update p->iBank
		PRINT	ebp, "FVB update bank"
		mov		eax, edx						; start again from base of bank
		cmp		ecx, 0							; end of list?
		jne		fvb1							; no - keep going
fvb0:
		xor		eax, eax						; else alloc failed
		dec		eax								; return EAX=0xFFFFFFFF
		mov		[ebx], eax						; update p->iNext
		PRINT	eax, "FVB OOM"
		pop		ebp
		ret

		ENDP


;*******************************************************************************
; Find the SPos structure for a given allocation size
; 
; Enter with
;	CH	= log2(alloc size)
;	EDI = pointer to SSuperPageBase
;	EBP = pointer to SAllocData
;
; Return with
;	Address of SPos in EBX
;	EAX = 2^CH-1, CL modified
;	Registers other than EAX, EBX, CL, EFLAGS unmodified
;*******************************************************************************
FindSizeData	PROC NEAR

		mov		eax, 1
		mov		cl, ch
		shl		eax, cl
		dec		eax								; eax = 2^CH-1
		lea		ebx, [ebp-SAllocData_SPos_sz]
findsizedata1:
		add		ebx, SAllocData_SPos_sz
		cmp		eax, [ebx+8]					; check EAX with mask for current SPos
		je		short findsizedata0				; branch if we've found it
		cmp		dword ptr [ebx+12], 0			; final?
		je		short findsizedata1				; loop if not
		FAULT									; bad size - die
findsizedata0:
		ret

		ENDP

;*******************************************************************************
; Allocate a block of a given size, which must be a power of 2
; 
; Enter with
;	CH	= log2(alloc size)
;	EDI = pointer to SSuperPageBase
;	EBP = pointer to SAllocData
;
; Return with
;	EAX = address of allocated block, or 0xFFFFFFFF if none could be found
;	EBX points to SPos for requested size
;	ECX, EDX, EFLAGS modified
;	EBP, ESI, EDI unmodified
;*******************************************************************************
DoAllocBlock	PROC NEAR

		push	ebp
		PRINTB	ch, "DoAllocBlock"
		call	FindSizeData					; set up EBX to correct SPos
		push	ebx								; save it
doallocblock1:
		push	ebp
		mov		ebp, [esp+8]					; retrieve original EBP
		call	FindValidBlock					; EAX = candidate block address
		pop		ebp
		cmp		eax, 0ffffffffh					; OOM?
		je		doallocblock0					; branch if so
doallocblock2:
		cmp		dword ptr [ebx+SAllocData_SPos_iFinal], 0
		jnz		doallocblock0					; if final, candidate is OK
		add		ebx, SAllocData_SPos_sz			; else step to next SPos
		test	eax, [ebx+SAllocData_SPos_iMask]
		jnz		doallocblock0					; if not correctly aligned for larger size, candidate is OK

		; eax=candidate block address, ebx=SPos pointer, ecx=SRamBank size, edx=SRamBank base
		mov		ebp, eax
		sub		ebp, edx						; ebp = offset from SRamBank base
		FAULT	b								; shouldn't be negative
		add		ebp, [ebx+SAllocData_SPos_iMask]	; add size-1 for larger block
		cmp		ebp, ecx						; compare to SRamBank size
		jae		doallocblock0					; if >=, not valid for larger size - finished
		push	eax
		push	ebp
		mov		ebp, [esp+12]					; retrieve original EBP
		push	dword ptr [ebx+SAllocData_SPos_iMask]
		push	eax
		call	CheckPreAlloc					; check if larger size would overlap skip regions
		pop		ebp
		pop		eax								; restore EAX
		jc		doallocblock0					; if overlap, not valid for larger size - finished
		cmp		eax, [ebx]						; compare candidate address to larger bank->iNext
		jae		doallocblock2					; if no clash, try next size
		PRINT	eax, "CLASH base"
		mov		ecx, [ebx+SAllocData_SPos_iMask]
		PRINT	ecx, "CLASH mask"
		mov		ebx, [esp]						; recover SPos pointer
		mov		eax, [ebx]						; EAX = SPos->iNext
		add		eax, ecx						; step past clashing block
		mov		[ebx], eax						;
		jmp		doallocblock1					; and continue search
doallocblock0:
		; have a good block in EAX or EAX=0xFFFFFFFF if OOM
		PRINT	eax, "<DoAllocBlock"
		mov		ebx, [esp]						; recover SPos pointer
		mov		ecx, [ebx]						; ecx = SPos->iNext
		mov		edx, [ebx+4]					; edx = SPos->iBank
doallocblock3:
		cmp		dword ptr [ebx+SAllocData_SPos_iFinal], 0
		jnz		short doallocblock4				; if final, finished
		add		ebx, SAllocData_SPos_sz			; else step to next SPos
		cmp		ecx, [ebx]						; if original SPos->iNext > larger SPos->iNext ...
		jbe		short doallocblock5
		mov		[ebx], ecx						; ... larger SPos->iNext = original SPos->iNext and ...
		mov		[ebx+4], edx					; ... larger SPos->iBank = original SPos->iBank
doallocblock5:
		jmp		short doallocblock3
doallocblock4:
		pop		ebx
		pop		ebp
		ret

		ENDP


;*******************************************************************************
; Initialise the allocator data
; 
; Enter with
;	EDI = pointer to SSuperPageBase
;
; Return with
;	EBP = pointer to SAllocData
;	EAX, EDX, EFLAGS modified
;	EBX, ECX, ESI, EDI unmodified
;*******************************************************************************
InitAllocator	PROC NEAR

		lea		ebp, [edi+CpuAllocDataOffset]			; ebp -> SAllocData
		mov		[edi+SSuperPageBase_iBootAlloc], ebp	; save pointer
		mov		eax, [edi+SSuperPageBase_iRamBootData]	; SRamBank list
		mov		edx, [eax]								; base address of first bank
		mov		[ebp+0], edx					; pos[0]->iNext
		mov		[ebp+4], eax					; pos[0]->iBank
		mov		dword ptr [ebp+8], 0fffh		; pos[0]->iMask		= 4K-1
		mov		dword ptr [ebp+12], 0			; pos[0]->iFinal
		mov		[ebp+16], edx					; pos[1]->iNext
		mov		[ebp+20], eax					; pos[1]->iBank
		mov		dword ptr [ebp+24], 3fffffh		; pos[1]->iMask		= 4M-1
		mov		dword ptr [ebp+28], 1			; pos[1]->iFinal
		mov		dword ptr [ebp+36], 1			; iExtraSkip base
		mov		dword ptr [ebp+40], 0			; iExtraSkip size
initalloc1:
		add		eax, 8							; step to next bank
		cmp		dword ptr [eax-8], 0			; end of list?
		jne		short initalloc1				; loop if not
		mov		[ebp+32], eax					; iSkip points to preallocated block list
		PRTLN	"SAllocData:"
		MEMDUMP	ebp, SAllocData_sz
		ret

		ENDP


;*******************************************************************************
; Physical RAM Allocator for MMU configurations
; Accepts all allocator calls
; 
; Enter with
;	CL	= type of allocation
;	CH	= log2(size) for BMA_Kernel allocations
;	EDI = pointer to SSuperPageBase
;	ESI = pointer to TRomHeader
;
; Return with
;	EAX	= pointer to allocated memory
;	Faults on OOM
;	CH may be reduced for BMA_Kernel allocations if the original request size
;	could not be satisfied.
;	No other registers modified
;*******************************************************************************
HandleAllocRequest	PROC NEAR

		push	eax
		push	ebx
		push	ecx
		push	edx
		push	ebp
		PRINTB	cl, "HandleAllocRequest type"
		mov		ebp, [edi+SSuperPageBase_iBootAlloc]	; set up EBP->SAllocData
		cmp		cl, BMA_NUM_FUNCTIONS
		FAULT	ae								; bad request number
		movzx	eax, cl
		ADR		edx, HA0						; base address of table
		add		edx, [edx+eax*4]				; add table[cl] to base address of table
		push	edx
		mov		eax, [esp+20]					; restore EAX
		mov		edx, [esp+8]					; restore EDX
		ret										; jump to handler for request
HA0:
		DD		HandleAllocInit-HA0				; init
		DD		HandleAllocSuper-HA0			; super/CPU
		DD		HandleAllocPageDir-HA0			; page directory
		DD		HandleAllocPageTable-HA0		; page table
		DD		HandleAllocKernel-HA0			; kernel
		DD		HandleAllocReloc-HA0			; relocate data

HandleAllocInit:
		call	InitAllocator
		jmp		HAexit

HandleAllocSuper:
		mov		eax, edi							; don't move it

		; Install skip region for super/CPU pages
		mov		[ebp+36], eax						; iExtraSkip base
		mov		dword ptr [ebp+40], SuperCpuSize	; iExtraSkip size
		PRTLN	"SAllocData:"
		MEMDUMP	ebp, SAllocData_sz
		jmp		HAexit2

HandleAllocPageDir:
HandleAllocPageTable:
		mov		ch, 12
		call	DoAllocBlock					; allocate a 4K page for PageDir or PageTable
HAexit2:
		cmp		eax, 0ffffffffh
		FAULT	e								; fault if alloc failed
		PRINT	eax, "<HandleAllocRequest ret"
		mov		[esp+16], eax
HAexit:
		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
		ret

HandleAllocKernel:
		PRINTB	ch, "HandleAllocRequest size"
		cmp		ch, 22
		jb		short HA1
		mov		byte ptr [esp+9], 22			; update return CH
		call	DoAllocBlock					; try to get 4M block
		cmp		eax, 0ffffffffh
		jne		short HA2						; branch if succeeded
HA1:
		mov		ch, 12							; else try to get 4K block
		mov		[esp+9], ch						; update return CH
		call	DoAllocBlock					; try to get 4M block
HA2:
		mov		ch, [esp+9]
		PRINTB	ch, "<HandleAllocRequest size"
		jmp		HAexit2

HandleAllocReloc:
		add		[ebp+4], eax					; fix pos[0]->iBank
		add		[ebp+20], eax					; fix pos[1]->iBank
		add		[ebp+32], eax					; fix iSkip
		jmp		HAexit

		ENDP


ELSE	; CFG_MMDirect


;*******************************************************************************
; Simple memory allocator for direct model
;*******************************************************************************
; Accepts allocator calls other than BMA_Kernel
; 
; Enter with
;	CL	= type of allocation
;	EDI = pointer to SSuperPageBase
;	ESI = pointer to TRomHeader
;
; Return with
;	EAX	= pointer to allocated memory
;	Faults on OOM
;	No other registers modified
;*******************************************************************************
HandleAllocRequest	PROC NEAR

		push	eax
		push	ebx
		push	ecx
		push	edx
		push	ebp
		PRINTB	cl, "HandleAllocRequest type"
		mov		ebp, [edi+SSuperPageBase_iBootAlloc]	; set up EBP->SAllocData
		cmp		cl, BMA_NUM_FUNCTIONS
		FAULT	ae								; bad request number
		movzx	eax, cl
		ADR		edx, HA0						; base address of table
		add		edx, [edx+eax*4]				; add table[cl] to base address of table
		jmp		edx								; jump to handler for request
HA0:
		DD		HandleAllocInit-HA0				; init
		DD		HandleAllocSuper-HA0			; super/CPU
		DD		HandleAllocPageDir-HA0			; page directory
		DD		HandleAllocPageTable-HA0		; page table
		DD		HandleAllocKernel-HA0			; kernel
		DD		HandleAllocReloc-HA0			; relocate data

HandleAllocInit:
		lea		ebp, [edi+CpuAllocDataOffset]			; ebp -> SDirectAlloc
		mov		[edi+SSuperPageBase_iBootAlloc], ebp	; save pointer
		mov		eax, [edi+SSuperPageBase_iKernelLimit]	; linear upper limit
		mov		ebx, eax
		dec		eax										; make inclusive
		call	RamLinearToPhysical						; convert to physical
		inc		eax										; make exclusive
		mov		[ebp+SDirectAlloc_iNextPageTable], eax	; store physical
		PRINT	eax, "Init iNextPageTable"
		mov		ecx, ebx
		sub		ecx, eax								; linear - physical
		GETPARAM	BPR_PageTableSpace, 10000h			; get reserved space for page tables, default to 64K
		xchg	eax, ebx
		sub		eax, ebx							; linear lower limit
		mov		ebx, eax
		call	RamLinearToPhysical					; convert to physical
		mov		[ebp+SDirectAlloc_iLimit], eax		; store physical
		PRINT	eax, "Init iLimit"
		sub		ebx, eax							; linear - physical
		cmp		ebx, ecx							; make sure upper + lower limits in same bank
		FAULT	ne
		jmp		HAexit

HandleAllocSuper:
		GETPARAM BPR_KernDataOffset, SuperCpuSize	; get kernel data offset into EAX, default to 8K or 24K
		neg		eax
		add		eax, [esi+TRomHeader_iKernDataAddress]
		PRINT	eax, "Super lin"
		call	RamLinearToPhysical
		PRINT	eax, "Super phys"
		jmp		HAexit2

HandleAllocPageDir:
HandleAllocPageTable:
		mov		eax, [ebp+SDirectAlloc_iNextPageTable]
		sub		eax, 1000h
		cmp		eax, [ebp+SDirectAlloc_iLimit]
		FAULT	b								; insufficient RAM reserved
		mov		[ebp+SDirectAlloc_iNextPageTable], eax
HAexit2:
		cmp		eax, 0ffffffffh
		FAULT	e								; fault if alloc failed
		PRINT	eax, "<HandleAllocRequest ret"
		mov		[esp+16], eax
HandleAllocReloc:	; not required on direct memory model
HAexit:
		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
		ret

HandleAllocKernel:
		FAULT		; not supported on direct memory model

		ENDP


ENDIF	; CFG_MMDirect


;*******************************************************************************
; Debug only code
;*******************************************************************************

IFDEF		CFG_DebugBootRom

;*******************************************************************************
; Print a null-terminated string located after the call
; No registers modified (including flags)
;*******************************************************************************
WriteS	PROC NEAR

		xchg	edx, [esp]			; save edx, return address to edx
		push	eax
		pushfd
writeS_loop:
		mov		al, [edx]
		inc		edx
		cmp		al, 0
		je		short writeS_end
		BOOTCALL BTF_WriteC
		jmp		short writeS_loop
writeS_end:
		popfd						; restore flags
		pop		eax					; restore eax
		xchg	edx, [esp]			; real return address to stack, restore edx
		ret

		ENDP

;*******************************************************************************
; Print a newline
; No registers modified (including flags)
;*******************************************************************************
WriteNL	PROC NEAR

		push	eax
		mov		al, 13
		BOOTCALL BTF_WriteC
		mov		al, 10
		BOOTCALL BTF_WriteC
		pop		eax
		ret

		ENDP

;*******************************************************************************
; Print word on top of stack in hexadecimal
; No registers modified (including flags)
; Argument is popped
;*******************************************************************************
WriteW	PROC NEAR
		pushfd
		push	eax
		push	ecx
		push	edx
		mov		edx, [esp+20]
		mov		cl, 32
		call	WriteHex
		pop		edx
		pop		ecx
		pop		eax
		popfd
		ret		4
		ENDP

WriteH	PROC NEAR
		pushfd
		push	eax
		push	ecx
		push	edx
		mov		edx, [esp+20]
		mov		cl, 16
		call	WriteHex
		pop		edx
		pop		ecx
		pop		eax
		popfd
		ret		4
		ENDP

WriteB	PROC NEAR
		pushfd
		push	eax
		push	ecx
		push	edx
		mov		edx, [esp+20]
		mov		cl, 8
		call	WriteHex
		pop		edx
		pop		ecx
		pop		eax
		popfd
		ret		4
		ENDP

; Internal routine to print EDX as hex
; CL = 4*number of digits to print
; So CL=8 prints the lower byte of EDX as 2 hex digits
; EAX, CL, EDX, flags modified
WriteHex	PROC NEAR

		ror		edx, cl
writehex1:
		rol		edx, 4
		mov		al, dl
		and		al, 0fh
		cmp		al, 9
		jbe		short ascz
		add		al, 7
ascz:
		add		al, 48
		BOOTCALL BTF_WriteC
		sub		cl, 4
		jne		short writehex1
		ret

		ENDP

;*******************************************************************************
; Dump memory in hexadecimal
;	Base address on stack below return address
;	Size on stack below base address
; No registers modified (including flags)
; Arguments are popped
;*******************************************************************************
DoMemDump	PROC NEAR

		pushfd
		pushad
		mov		ebx, [esp+40]		; base address
		mov		ebp, [esp+44]		; size

memdump1:
		mov		edx, ebx
		mov		cl, 32
		call	WriteHex
		mov		al, 58
		BOOTCALL BTF_WriteC
		mov		ch, 16

memdump2:
		mov		al, 32
		BOOTCALL BTF_WriteC
		mov		dl, [ebx]
		inc		ebx
		mov		cl, 8
		call	WriteHex
		dec		ch
		jnz		short memdump2

		call	WriteNL
		sub		ebp, 16
		ja		memdump1

		popad
		popfd
		ret		8

		ENDP


ENDIF	; CFG_DebugBootRom

;*******************************************************************************
; Print a stack frame containing exception info
;
; Enter with :
;		EDI = address of super page
;		ESI = address of TRomHeader
;
; Return with:
;		Flags modified
;		No other registers modified
;		
;*******************************************************************************
; This prints a stack frame containing the exception info
;
; ESP+00	EDI		ESI		EBP		ESP
;	  10	EBX		EDX		ECX		EAX
;	  20	ES		DS		SS		CS
;	  30	CR0		CR2		CR3		VEC#
;	  40	ERR		EIP		CS		EFLAGS

VectorCommon	proc near

IFDEF CFG_DebugBootRom
		sub		esp, 12
		push	cs
		push	ss
		push	ds
		push	es
		pushad
		mov		eax, cr0
		mov		[esp+48], eax
		mov		eax, cr2
		mov		[esp+52], eax
		mov		eax, cr3
		mov		[esp+56], eax
;		SPIN	1000000h
		PRTLN	"***EXCEPTION***"
		mov		eax, esp
		MEMDUMP	eax, 50h
		PRTLN	"Halting"
ENDIF

VectorSpin:
		jmp		short VectorSpin

		ENDP


;*******************************************************************************
; Fault handler
;*******************************************************************************

BasicFaultHandler	PROC NEAR
	IFDEF	CFG_DebugBootRom

		xchg	eax, [esp]
		PRINT	eax, "***FAULT***"
		pushfd
		push	cs
		push	eax
		push	0
		push	0
		mov		eax, cr3
		push	eax
		mov		eax, cr2
		push	eax
		mov		eax, cr0
		push	eax
		push	cs
		push	ss
		push	ds
		push	es
		mov		eax, [esp+48]
		pushad
;		SPIN	1000000h
		mov		eax, esp
		MEMDUMP	eax, 50h
		PRTLN	"Halting"
	ENDIF	; CFG_DebugBootRom

FaultSpin:
		jmp		short FaultSpin

		ENDP

;******************************************************************************

cseg	ends
end

;******************************************************************************

