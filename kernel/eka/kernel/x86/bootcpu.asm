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
; Description: Bootstrap CPU specific routines
;

LOCALS
.486p

INCLUDE	bootcpu.inc

;
;*******************************************************************************
;

cseg segment use32 para public 'code'
assume	cs:cseg, ds:cseg, ss:cseg, es:cseg, fs:cseg, gs:cseg

;
;*******************************************************************************
;

;*******************************************************************************
; Initialise any CPU/MMU registers before switching on MMU
;
; Enter with :
;		EDI = address of super page
;		Valid stack
;
; Leave with :
;*******************************************************************************
	GLOBAL DoWriteC : PROC
InitCpu	PROC NEAR

		xor		eax, eax
		mov		[edi+SSuperPageBase_iMachineData], eax
		pushfd
		mov		edx, [esp]
		or		edx, EFLAGS_ID
		push	edx
		popfd
		pushfd
		pop		ecx
		test	ecx, EFLAGS_ID
		jz		short no_cpu_id
		xor		ecx, EFLAGS_ID
		push	ecx
		popfd
		pushfd
		pop		edx
		test	edx, EFLAGS_ID
		jnz		short no_cpu_id

		; CPUID instruction is valid
		cpuid
		cmp		eax, 0
		jz		only_0_supported
		mov		eax, 1
		cpuid

		; EAX = CPU identifier
		mov		[edi+SSuperPageBase_iMachineData], eax

		; EDX = feature flags
		mov		eax, edx

		; Enable 4M pages if supported
		test	eax, EX86Feat_PSE
		jz		short initcpu1
		getcr	REG_ECX, 4
		or		ecx, CR4_PSE
		setcr	4, REG_ECX
initcpu1:

only_0_supported:
no_cpu_id:
		popfd
		mov		[edi+SSuperPageBase_iCpuId], eax

		call	SetupGDT

		call	SetupIDT

		ret

		ENDP


;*******************************************************************************
; Update page tables
;
; Enter with :
;		EDI = address of super page
;		EAX = address of PTE/PDE modified
;		EDX = virtual address of mapping
;		CH	= 12 for single PTE updated
;			= 22 for single PDE updated
;			= 12 + 64 if whole page table changed
;				In this case, EDX is not set, EAX=access address of page table
; Leave with :
;		Nothing modified except flags
;*******************************************************************************
PageTableUpdate	PROC NEAR

		PRINTB	ch, "PTU: CH"
		PRINT	eax, "PTU: EAX"
		PRINT	edx, "PTU: EDX"
		push	ebx
		test	dword ptr [edi+SSuperPageBase_iCpuId], EX86Feat_PGE
		jz		short ptu_no_pge
		push	ecx
		getcr	REG_ECX, 4
		mov		ebx, NOT CR4_PGE
		and		ebx, ecx
		setcr	4, REG_EBX
		mov		ebx, cr3
		mov		cr3, ebx
		setcr	4, REG_ECX
		pop		ecx
		pop		ebx
		ret
ptu_no_pge:
		mov		ebx, cr3
		mov		cr3, ebx
		pop		ebx
		ret

		ENDP


;*******************************************************************************
; Get PDE value
;
; Enter with :
;		CH	= 12 for 4K page
;			= 22 for 4M page
;		EBP = index into boot table of required permission description
;			 or permission descriptor itself in standard form (BTP_ENTRY macro).
;		EDI = address of super page
;
; Boot table entry is as follows:
;	Bits 0-3	= permissions
;	Bits 4-7	= cache attributes
;	Bit	8		= 1 if global (if PGE supported)
;
; Leave with :
;		EAX = PDE value
;		No other registers modified apart from flags
;*******************************************************************************
GetPdeValue	PROC NEAR

		push	ebx
		push	ecx
		push	edx
		push	ebp
		test	ebp, BTP_FLAG				; index or permission descriptor?
		jnz		short getpde1				; skip if permission descriptor
		PRINT	ebp, "GetPDE index"
		mov		edx, [edi+SSuperPageBase_iBootTable]
		mov		ebp, [edx+ebp*4]			; get boot table entry
getpde1:
		and		ebp, NOT BTP_FLAG			; remove flag
		PRINTB	ch, "GetPDE size"
		PRINT	ebp, "GetPDE entry"
		test	dword ptr [edi+SSuperPageBase_iCpuId], EX86Feat_PGE
		jnz		short getpde2				; skip if CPU supports PGE
		and		ebp, NOT 100h				; if not, ignore global flag
getpde2:
		mov		eax, ebp
		and		eax, 0ch					; leave PDE permission bits
		shr		eax, 1						;
		or		al, 1						; U,W in right place, set P
		cmp		ch, 12
		je		short getpde3				; skip if 4K page
		test	dword ptr [edi+SSuperPageBase_iCpuId], EX86Feat_PSE
		FAULT	z							; die if processor doesn't support PSE
		lea		eax, [ebp*2+1]				; for 4M pages use PTE permissions
		and		eax, 7
		mov		edx, ebp
		shr		edx, 4
		and		edx, 3
		shl		edx, 3						; PCD, PWT
		or		al, dl						; put them in with U, W, P
		or		al, PDE_PS					; set PS flag for 4M page
		and		ebp, 100h
		or		eax, ebp					; set PDE_G if global mapping requested
getpde3:
		PRINT	eax, "GetPDE value"
		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		ret

		ENDP


;*******************************************************************************
; Get PTE value
;
; Enter with :
;		CH	= 12 for 4K page
;		EBP = index into boot table of required permission description
;			 or permission descriptor itself in standard form (BTP_ENTRY macro).
;		EDI = address of super page
;
; Boot table entry is as follows:
;	Bits 0-3	= permissions
;	Bits 4-7	= cache attributes
;	Bit	8		= 1 if global (if PGE supported)
;
; Leave with :
;		EAX = PTE value
;		No other registers modified apart from flags
;*******************************************************************************
GetPteValue	PROC NEAR

		push	ebx
		push	ecx
		push	edx
		push	ebp
		test	ebp, BTP_FLAG				; index or permission descriptor?
		jnz		short getpte1				; skip if permission descriptor
		PRINT	ebp, "GetPTE index"
		mov		edx, [edi+SSuperPageBase_iBootTable]
		mov		ebp, [edx+ebp*4]			; get boot table entry
getpte1:
		and		ebp, NOT BTP_FLAG			; remove flag
		PRINTB	ch, "GetPTE size"
		PRINT	ebp, "GetPTE entry"
		test	dword ptr [edi+SSuperPageBase_iCpuId], EX86Feat_PGE
		jnz		short getpte2				; skip if CPU supports PGE
		and		ebp, NOT 100h				; if not, ignore global flag
getpte2:
		lea		eax, [ebp*2+1]				; U,W in right place, set P
		and		eax, 7						; leave only U, W, P
		mov		edx, ebp
		shr		edx, 4
		and		edx, 3
		shl		edx, 3						; PCD, PWT
		or		al, dl						; put them in with U, W, P
		cmp		ch, 12
		FAULT	ne							; only 4K pages supported here
		and		ebp, 100h
		or		eax, ebp					; set PTE_G if global mapping requested
		PRINT	eax, "GetPTE value"
		pop		ebp
		pop		edx
		pop		ecx
		pop		ebx
		ret

		ENDP


;*******************************************************************************
; Enable the MMU and remove the temporary identity mapping
;
; Enter with :
;		EDI = physical address of super page, containing SSwitch block
;
; Leave with :
;		EDI = linear address of super page
;		ESP changed to linear address
;		Return to linear addresses
;*******************************************************************************
EnableMmu	PROC NEAR

		lea		ebx, [edi+CpuBootStackOffset]		; ebx points to SSwitch block
		mov		eax, [ebx+SSwitch_iPageDirPhys]
		mov		cr3, eax							; initialise page directory base

		; Put in identity mapping if necessary
		cmp		dword ptr [ebx+SSwitch_iTempPdeOffset], 0
		js		enable_mmu_1
		mov		edx, [ebx+SSwitch_iTempPtePtr]
		mov		eax, [ebx+SSwitch_iTempPte]
		lea		ecx, enable_mmu_1					; offset from ROM header (since we link at 0)
		shr		ecx, 12								; pages
		lea		edx, [edx+ecx*4]					; step PTE ptr
		shl		ecx, 12
		add		eax, ecx							; step PTE
		mov		[edx], eax							; install identity PTE
		PRINT	edx, "ID1: PTA"
		PRINT	eax, "ID1: PTE"
		add		eax, 1000h
		mov		[edx+4], eax						; install second identity PTE
		PRINT	eax, "ID2: PTE"

enable_mmu_1:
		mov		ebp, [ebx+SSwitch_iSuperPageLin]
		sub		ebp, edi							; super page offset
		mov		esi, [ebx+SSwitch_iRomHdrLin]
		sub		esi, [ebx+SSwitch_iRomHdrPhys]		; ROM header offset

		; fix up GDT pointer
		sub		esp, 8
		sgdt	fword ptr [esp]
		add		[esp+2], ebp
		lgdt	fword ptr [esp]
		add		esp, 8

		; linear jump address
		ADR		ecx, enable_mmu_2
		add		ecx, esi
		mov		edi, [ebx+SSwitch_iSuperPageLin]	; EDI = linear super page address

		; enable MMU
		wbinvd
		mov		eax, cr0
		or		eax, CR0_PG + CR0_WP
		mov		cr0, eax

		; jump to flush prefetch queue and switch to linear EIP
		jmp		ecx

enable_mmu_2:
		; reload segment regs
		mov		eax, KRing0DS
		mov		ss, ax
		mov		ds, ax
		mov		es, ax
		xor		eax, eax
		mov		fs, ax
		mov		gs, ax

		test	dword ptr [edi+SSuperPageBase_iCpuId], EX86Feat_PGE
		jz		short enable_mmu_3					; skip if no global pages
		getcr	REG_EAX, 4
		or		eax, CR4_PGE
		setcr	4, REG_EAX							; enable global pages
		mov		eax, cr3
		mov		cr3, eax							; remove any non-global TLB entries

enable_mmu_3:
		lea		ebx, [edi+CpuBootStackOffset]		; ebx points to SSwitch block (now linear)
		sub		esp, [ebx+SSwitch_iStackPhys]
		add		esp, [ebx+SSwitch_iStackLin]		; fix stack pointer

		pop		eax									; return address
		add		eax, esi							; fix it up
		pushfd										; push flags
		push	KRing0CS							; push required CS
		push	eax									; push linear return address

		; remove identity mapping if necessary
		mov		eax, [ebx+SSwitch_iTempPdeOffset]
		cmp		eax, 0
		js		short enable_mmu_4
		shl		eax, 2
		add		eax, [ebx+SSwitch_iPageDirLin]
		mov		ecx, [ebx+SSwitch_iOrigPde]
		mov		[eax], ecx							; replace original PDE
		mov		eax, cr3
		mov		cr3, eax							; remove any non-global TLB entries (temp entries are nonglobal)

enable_mmu_4:

		iretd										; return to linear addresses

		ENDP


;*******************************************************************************
; Debug exception handlers
;*******************************************************************************

	VEC_NE	MACRO	vecnum
	push	0
	push	vecnum
	jmp		VectorCommon
	ENDM

	VEC_E	MACRO	vecnum
	push	vecnum
	jmp		VectorCommon
	ENDM

Vec00		proc near
	VEC_NE	00h
	endp
Vec01		proc near
	VEC_NE	01h
	endp
Vec02		proc near
	VEC_NE	02h
	endp
Vec03		proc near
	VEC_NE	03h
	endp
Vec04		proc near
	VEC_NE	04h
	endp
Vec05		proc near
	VEC_NE	05h
	endp
Vec06		proc near
	VEC_NE	06h
	endp
Vec07		proc near
	VEC_NE	07h
	endp
Vec08		proc near
	VEC_E	08h
	endp
Vec09		proc near
	VEC_NE	09h
	endp
Vec0A		proc near
	VEC_E	0Ah
	endp
Vec0B		proc near
	VEC_E	0Bh
	endp
Vec0C		proc near
	VEC_E	0Ch
	endp
Vec0D		proc near
	VEC_E	0Dh
	endp
Vec0E		proc near
	VEC_E	0Eh
	endp
Vec0F		proc near
	VEC_NE	0Fh
	endp
Vec10		proc near
	VEC_NE	10h
	endp
Vec11		proc near
	VEC_E	11h
	endp
Vec12		proc near
	VEC_NE	12h
	endp
Vec13		proc near
	VEC_NE	13h
	endp
Vec14		proc near
	VEC_NE	14h
	endp
Vec15		proc near
	VEC_NE	15h
	endp
Vec16		proc near
	VEC_NE	16h
	endp
Vec17		proc near
	VEC_NE	17h
	endp
Vec18		proc near
	VEC_NE	18h
	endp
Vec19		proc near
	VEC_NE	19h
	endp
Vec1A		proc near
	VEC_NE	1Ah
	endp
Vec1B		proc near
	VEC_NE	1Bh
	endp
Vec1C		proc near
	VEC_NE	1Ch
	endp
Vec1D		proc near
	VEC_NE	1Dh
	endp
Vec1E		proc near
	VEC_NE	1Eh
	endp
Vec1F		proc near
	VEC_NE	1Fh
	endp

VecTable	label dword
	dd		Vec00
	dd		Vec01
	dd		Vec02
	dd		Vec03
	dd		Vec04
	dd		Vec05
	dd		Vec06
	dd		Vec07
	dd		Vec08
	dd		Vec09
	dd		Vec0A
	dd		Vec0B
	dd		Vec0C
	dd		Vec0D
	dd		Vec0E
	dd		Vec0F
	dd		Vec10
	dd		Vec11
	dd		Vec12
	dd		Vec13
	dd		Vec14
	dd		Vec15
	dd		Vec16
	dd		Vec17
	dd		Vec18
	dd		Vec19
	dd		Vec1A
	dd		Vec1B
	dd		Vec1C
	dd		Vec1D
	dd		Vec1E
	dd		Vec1F
VecTableEnd	label dword


;*******************************************************************************
; Install an IDT entry
;
; Enter with :
;		[esp+0] = vector number
;		[esp+4] = address of handler
;		EDI = address of super page
;		ESI = address of TRomHeader
;
; Return with:
;		Flags modified
;		No other registers modified
;		Arguments popped
;		
;*******************************************************************************
InstallIDTEntry	PROC NEAR

		push	eax
		push	ecx
		push	edx
		mov		eax, [esp+16]			; vector number
		mov		ecx, [esp+20]			; handler address
		PRINT	eax, "InstallIdtEntry vector  "
		PRINT	ecx, "InstallIdtEntry handler "
		sub		esp, 8
		sidt	fword ptr [esp]
		shl		eax, 3
		add		eax, [esp+2]			; eax = address of IDT entry
		mov		dx, cs					; segment selector
		shl		edx, 16
		mov		dx, cx					; edx = SEG : OFF0-15
		mov		[eax], edx				; write first word of IDT entry
		mov		cx, 8e00h				; 32 bit interrupt gate, DPL = 0
		mov		[eax+4], ecx			; write second word of IDT entry
		add		esp, 8
		pop		edx
		pop		ecx
		pop		eax
		ret		8

		ENDP


;*******************************************************************************
; Set up the bootstrap IDT
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
SetupIDT	PROC NEAR

		push	eax
		push	ecx
		push	edx
		push	0
		push	8*256
		lea		eax, [edi + IdtOffsetFromSuperPage]
		push	eax
		call	WordFill				; fill with 0 to begin with
		sub		esp, 8
		mov		ecx, 8*256-1
		mov		[esp], cx
		mov		[esp+2], eax
		lidt	fword ptr [esp]			; install IDT

IFDEF CFG_DebugBootRom
		sub		esp, 8
		sidt	fword ptr [esp]
		xor		eax, eax
		mov		ax, [esp]
		mov		ecx, [esp+2]
		add		esp, 8
		PRINT	eax, "IDT limit"		; some debug prints
		PRINT	ecx, "IDT base"
ENDIF
		add		esp, 8
		ADR		edx, VecTable
		ADR		ecx, VecTableEnd
		sub		ecx, edx
		add		edx, ecx				; edx = table end
		shr		ecx, 2
		dec		ecx						; last vector number
setupidt1:
		sub		edx, 4
		mov		eax, [edx]				; handler offset from ROM header
		add		eax, esi
		push	eax						; push handler address
		push	ecx						; push vector number
		call	InstallIDTEntry
		dec		ecx
		jns		short setupidt1
		pop		edx
		pop		ecx
		pop		eax
		ret

		ENDP


;*******************************************************************************
; Set up the GDT
;
; Enter with :
;		EDI = address of super page
;
; Return with:
;		Flags modified
;		No other registers modified
;		
;*******************************************************************************
SetupGDT	PROC NEAR

		ADR		eax, setup_gdt_1
		pushfd
		push	KRing0CS
		push	eax

		lea		ebx, [edi+CpuPageOffset]	; GDT at beginning of CPU page
		push	0
		push	GdtSizeBytes
		push	ebx
		call	WordFill					; zero initially

		; segment 1 - ring 0 code
		lea		edx, [ebx+08h]
		mov		dword ptr [edx+0], 0000ffffh	; BASE15:08 | BASE07:00 | LIMIT15:08 | LIMIT07:00
		mov		dword ptr [edx+4], 00cf9b00h	; BASE31:24 | GD0xllll	| Prr11CRA	 | BASE23:16

		; segment 2 - ring 0 data
		lea		edx, [ebx+10h]
		mov		dword ptr [edx+0], 0000ffffh	; BASE15:08 | BASE07:00 | LIMIT15:08 | LIMIT07:00
		mov		dword ptr [edx+4], 00cf9300h	; BASE31:24 | GD0xllll	| Prr10EWA	 | BASE23:16

		; segment 3 - ring 3 code
		lea		edx, [ebx+18h]
IFNDEF CFG_MMFlexible
		mov		dword ptr [edx+0], 0000ffffh	; BASE15:08 | BASE07:00 | LIMIT15:08 | LIMIT07:00
		mov		dword ptr [edx+4], 00cbfb00h	; BASE31:24 | GD0xllll	| Prr11CRA	 | BASE23:16
ELSE ; flexible mm..
		mov		dword ptr [edx+0], ((KUserMemoryLimit SHR 12)-1) AND 0000ffffh					; BASE15:08 | BASE07:00 | LIMIT15:08 | LIMIT07:00
		mov		dword ptr [edx+4], (((KUserMemoryLimit SHR 12)-1) AND 000f0000h) OR 00c0fb00h	; BASE31:24 | GD0xllll	| Prr11CRA	 | BASE23:16
ENDIF

		; segment 4 - ring 3 data
		lea		edx, [ebx+20h]
IFNDEF CFG_MMFlexible
		mov		dword ptr [edx+0], 0000ffffh	; BASE15:08 | BASE07:00 | LIMIT15:08 | LIMIT07:00
		mov		dword ptr [edx+4], 00cbf300h	; BASE31:24 | GD0xllll	| Prr10EWA	 | BASE23:16
ELSE ; flexible mm..
		mov		dword ptr [edx+0], ((KUserMemoryLimit SHR 12)-1) AND 0000ffffh					; BASE15:08 | BASE07:00 | LIMIT15:08 | LIMIT07:00
		mov		dword ptr [edx+4], (((KUserMemoryLimit SHR 12)-1) AND 000f0000h) OR 00c0f300h	; BASE31:24 | GD0xllll	| Prr10EWA	 | BASE23:16
ENDIF

		; other segments are used for TSS's (one per CPU)
		; these are set up by the kernel

		sub		esp, 8
		mov		word ptr [esp], GdtSizeBytes-1
		mov		[esp+2], ebx
		lgdt	fword ptr [esp]
		add		esp, 8
		iretd

setup_gdt_1:
		mov		eax, KRing0DS
		mov		ss, ax
		mov		ds, ax
		mov		es, ax
		xor		eax, eax
		mov		fs, ax
		mov		gs, ax
		lldt	ax
		ret

		ENDP


;******************************************************************************

cseg	ends
end

;******************************************************************************

