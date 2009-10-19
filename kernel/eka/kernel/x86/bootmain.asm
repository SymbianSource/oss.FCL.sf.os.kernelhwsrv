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
; Description:
;

;
;*******************************************************************************
;
; Bootstrap main program

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

org	0

TheRomHeader:							; this label represents the base of the ROM header

; Bootstrap entry point
; This doesn't run directly from the reset vector
; We need to be in 32 bit protected mode here
	jmp	ResetEntry

org TRomHeader_iRestartVector - 5
restart0:
	jmp	RestartEntry
	jmp short restart0

; Skip to end of ROM header

org TRomHeader_sz


;***************************************************************************************
; MAIN ENTRY POINT
;***************************************************************************************
ResetEntry:
		cli
		cld

;
; The following call should, at a minimum
;	1.	Determine the hardware configuration
;	2.	Determine the reset reason. If it is wakeup from a low power mode, perform
;		whatever reentry sequence is required and jump back to the kernel.
;	3.	Set up the memory controller so that at least some RAM is available
;	4.	Set EDI to point to the super page or to a temporary version of the super page
;		with at least the following fields valid:
;			iBootTable, iCodeBase, iActiveVariant, iCpuId
;	5.	Set ESP to point to the boot stack at the end of the CPU page
;	6.	In debug builds initialise the debug serial port
;
;	All registers may be modified by this call
;
;	Note that we JMP to this function instead of CALLing, since we don't have a stack
;	The function should return to InitialiseHardwareReturn
;
		jmp		InitialiseHardware
InitialiseHardwareReturn:

		; Set ESI to point to ROM header
		ADR		esi, TheRomHeader

		; Set up stack in CPU page
		lea		esp, [edi+CpuBootStackTop]

		mov		eax, [esi + TRomHeader_iDebugPort]
		mov		[edi+SSuperPageBase_iDebugPort], eax

IFDEF CFG_BootLoader OR CFG_CopyRomToAddress
; Copy the code to RAM
		PRTLN	"Copy code to RAM"

IFDEF CFG_SupportEmulatedRomPaging
		mov		eax, [esi+TRomHeader_iUncompressedSize]		; size of ROM
ELSE
		mov		eax, [esi+TRomHeader_iPageableRomStart]		; size of unpaged part of ROM
		cmp		eax, 0
		jnz		short copycode1
		mov		eax, [esi+TRomHeader_iUncompressedSize]		; size if not a pageable ROM
copycode1:
ENDIF
		add		eax, 3
		and		al, 0fch								; round up to 4 bytes
		push	eax

		push	dword ptr [edi+SSuperPageBase_iCodeBase]	; source address

		; Two methods of obtaining the destination physical address: either the
		; config.inc can define CFG_CopyRomToAddress as a numeric constant or it
		; will use the romlinearbase for bootloaders.
IFDEF CFG_CopyRomToAddress
		push	dword ptr CFG_CopyRomToAddress			; destination defined in config.inc (must be physical address)
ELSE
		push	dword ptr [esi+TRomHeader_iRomBase]		; destination defined in header.iby (must be physical address)
ENDIF
		PRINT4	[esp+4], "Source"
		mov		ebx, [esp]								; destination
		PRINT	ebx, "Dest"
		sub		ebx, [esp+4]							; destination - source
		PRINT4	[esp+8], "Size"
		PRINT	ebx, "Offset"
		ADR		eax, ReturnHereAfterCopy
		add		eax, ebp								; return address in copied code
		push	eax
		jmp		WordMove								; copy code and return to destination

ReturnHereAfterCopy:
		add		esi, ebx								; Fix up ROM header address
		add		[edi+SSuperPageBase_iCodeBase], ebx		; Fix up iCodeBase
		add		[edi+SSuperPageBase_iBootTable], ebx	; Fix up iBootTable

		PRINT	esi, "ESI"
		PRINT4	[edi+SSuperPageBase_iCodeBase], "New iCodeBase"
		PRINT4	[edi+SSuperPageBase_iBootTable], "New iBootTable"
		GETEIP	eax
		PRINT	eax, "EIP"
ENDIF

		mov		eax, [esi+TRomHeader_iRomRootDirectoryList]
		mov		[edi+SSuperPageBase_iRootDirList], eax
		mov		byte ptr [edi+SSuperPageBase_iBootFlags+3], 0	; clear top 8 bits of boot flags

; Determine the machine RAM configuration

		PRTLN	"Determine RAM config"
		lea		ebx, [edi+CpuRamTableOffset]
		mov		[edi+SSuperPageBase_iRamBootData], ebx
		lea		ecx, [edi+CpuRomTableOffset]
		mov		[edi+SSuperPageBase_iRomBootData], ecx
		push	0
		push	CpuBootStackOffset - CpuRomTableOffset
		push	ecx
		call	WordFill					; zero ROM and RAM data

		mov		eax, ebx
		BOOTCALL BTF_RamBanks				; Populate RAM bank table

		GETEIP	eax
testrambank1:
		mov		edx, [ebx]					; bank base
		mov		ecx, [ebx+4]				; bank size
		add		ebx, 8
		PRINT	edx, "TRB Base"
		PRINT	ecx, "TRB Size"
		cmp		ecx, 0						; end of list?
		jz		testrambank_end				; branch if so
		cmp		eax, edx					; PC >= base ?
		jb		testrambank1				; no - next bank
		mov		ebp, eax
		sub		ebp, edx					; PC - base
		cmp		ebp, ecx					; < size?
		jae		testrambank1				; no - next bank

		; Running from RAM - set boot flag
		or		dword ptr [edi+SSuperPageBase_iBootFlags], KBootFlagRunFromRAM

		jmp		testrambank1

testrambank_end:
		mov		[ebx], ecx
		mov		[ebx+4], ecx				; zero terminate reserved bank list

; Determine the machine ROM configuration
		test	dword ptr [edi+SSuperPageBase_iBootFlags], KBootFlagRunFromRAM
		jz		TestRomBanks				; branch if running from ROM
		
; ROM is actually in RAM

		PRTLN	"Running from RAM"
IFDEF CFG_SupportEmulatedRomPaging
		mov		ebp, [esi+TRomHeader_iUncompressedSize]		; size of ROM
ELSE
		mov		ebp, [esi+TRomHeader_iPageableRomStart]		; size of unpaged part of ROM
		cmp		ebp, 0
		jnz		short pgrom1
		mov		ebp, [esi+TRomHeader_iUncompressedSize]		; size if not a pageable ROM
pgrom1:
ENDIF
		mov		cl, 32-CFG_RomSizeAlign
		mov		edx, 0ffffffffh
		shr		edx, cl
		add		ebp, edx
		or		ebp, edx
		xor		ebp, edx									; round up to 64K (or CFG_RomSizeAlign)
		mov		[edi+SSuperPageBase_iTotalRomSize], ebp		; save it

		push	dword ptr [esi+TRomHeader_iRomBase]			; linear base of area to excise
		push	ebp											; size of area to excise
		push	dword ptr [edi+SSuperPageBase_iCodeBase]	; physical base of area to excise
		mov		ebx, [edi+SSuperPageBase_iRamBootData]
		mov		ebp, [edi+SSuperPageBase_iRomBootData]
		call	ExciseRamArea								; remove RAM which holds image from free list
		cmp		dword ptr [ebx+4], 0						; reached end of RAM list?
		je		rom_in_ram_1								; if so, don't look for extension ROM
		mov		eax, [ebx]									; first RAM after kernel ROM image
		call	CheckForExtensionRom						; check for extension ROM
		jnc		rom_in_ram_1								; skip if not
		mov		ecx, [eax+TExtensionRomHeader_iUncompressedSize] ; else ecx=extension ROM size
		add		ecx, edx
		or		ecx, edx
		xor		ecx, edx									; round up
		add		[edi+SSuperPageBase_iTotalRomSize], ecx		; accumulate total ROM size
		mov		edx, [eax+TExtensionRomHeader_iRomRootDirectoryList]
		mov		[edi+SSuperPageBase_iRootDirList], edx		; update root dir pointer
		push	dword ptr [eax+TExtensionRomHeader_iRomBase]	; linear base of area to excise
		push	ecx											; size of area to excise
		push	eax											; physical base of area to excise
		call	ExciseRamArea								; remove RAM which holds extension ROM image from free list
rom_in_ram_1:
		jmp		DoneRomBanks							; finished with ROM

; ROM is actually in ROM or FLASH
TestRomBanks:
		PRTLN	"Running from ROM/FLASH"
		FAULT	; don't support this

DoneRomBanks:

; Support for areas?
; Put it here if needed.

; point EAX to preallocated block list
		mov		eax, [edi+SSuperPageBase_iRamBootData]

find_prealloc_list:
		add		eax, 8
		cmp		dword ptr [eax-4], 0
		jnz		short find_prealloc_list

; Reserve any platform-dependent extra physical memory here.
; Two methods are available:
;	1.	ExciseRamArea - the kernel will then not treat the region as RAM
;	2.	Add preallocation regions to the RAM bank list. R11 has been set
;		to point to the first of these. List is terminated by a zero size.
;		The kernel will treat these regions as RAM.
;
		BOOTCALL	BTF_Reserve

IFDEF	CFG_DebugBootRom
; in debug, dump ROM and RAM bank config
		PRTLN	"ROM and RAM config:"
		mov		eax, [edi+SSuperPageBase_iRomBootData]
		mov		ebx, CpuBootStackOffset
		sub		ebx, CpuRomTableOffset
		MEMDUMP	eax, ebx
ENDIF

; calculate total RAM size
		mov		ebx, [edi+SSuperPageBase_iRamBootData]
		xor		eax, eax
		sub		ebx, 8

accumulate_ram_size:
		add		ebx, 8
		mov		ecx, [ebx+4]
		add		eax, ecx
		cmp		ecx, 0
		jnz		short accumulate_ram_size

		PRINT	eax, "TotalRamSize"
		mov		[edi+SSuperPageBase_iTotalRamSize], eax

; find the kernel image and point EBP to it
		PRTLN	"Find primary"
		call	FindPrimary

IFDEF CFG_MMDirect
		; direct model - work out the end of the kernel memory area
		mov		ecx, [esi+TRomHeader_iKernelLimit]	; end of kernel heap, rounded up to 4K
		GETPARAM	BPR_PageTableSpace, 10000h		; get reserved space for page tables, default to 64K
		add		ecx, eax							; add space for page tables
		mov		[edi+SSuperPageBase_iKernelLimit], ecx
		PRINT	ecx, "Kernel Limit"

		GETPARAM	BPR_KernDataOffset, SuperCpuSize	; get kernel data offset into R0, default to 8K/24K
		mov		ecx, [esi+TRomHeader_iKernDataAddress]	;
		sub		ecx, eax								; ECX = super page linear address
		PRINT	ecx, "SuperPageLin"

; for non-bootloader, iRamBase = TRomHeader::iKernDataAddress - KernDataOffset
SuperPageAtBeginning:
		mov		[edi+SSuperPageBase_iRamBase], ecx
		PRINT	ecx, "iRamBase"

ENDIF

; initialise the RAM allocator
		PRTLN	"Init RAM allocator"
		mov		cl, BMA_Init
		BOOTCALL	BTF_Alloc

; get the final super page physical address
		mov		cl, BMA_SuperCPU
		BOOTCALL	BTF_Alloc
		PRINT	eax, "Final SuperPage Phys"

; if super page has moved, relocate it
		cmp		eax, edi
		jne		short super_page_unmoved
		call	RelocateSuperPage
super_page_unmoved:

; initialise the memory mapping system
		PRTLN	"InitMemMapSystem"
		call	InitMemMapSystem

; map the ROM
		PRTLN	"Map ROM"
		mov		ebx, [edi+SSuperPageBase_iRomBootData]

MapRomBlock:
		mov		ecx, [ebx+SRomBank_iSize]		; bank size
		cmp		ecx, 0
		jz		short MapRomDone				; branch out if reached end of list
		push	22								; max page size for mapping
		push	ecx								; size of mapping
		push	BTP_Rom							; permissions for mapping
		push	dword ptr [ebx+SRomBank_iBase]	; physical base of mapping
		push	dword ptr [ebx+SRomBank_iLinBase]	; linear base of mapping
		call	MapContiguous					; make mapping
		add		ebx, SRomBank_sz
		jmp		short MapRomBlock
MapRomDone:

IFDEF	CFG_MMDirect

; direct model - identity map all RAM
; permissions are kernel up to the end of the kernel heap, user after
		PRTLN	"Direct Map RAM"
		mov		ebx, [edi+SSuperPageBase_iRamBootData]
		push	ebp

MapRamBlock:
		mov		edx, [ebx]						; bank phys base
		mov		ecx, [ebx+4]					; bank size
		add		ebx, 8
		cmp		ecx, 0
		jz		MapRamBlockEnd					; end of list
		mov		eax, [edi+SSuperPageBase_iKernelLimit]
		cmp		eax, edx						; kernel limit in this bank?
		jbe		MapUserRamBlock					; kernel limit <= bank base -> user
		mov		ebp, eax
		sub		ebp, edx						; kernel limit - bank base
		cmp		ebp, ecx						; compare with bank size
		jbe		short mrb1
		mov		ebp, ecx						; ebp = min(kernel limit-bank base, bank size)
mrb1:
		push	22								; max page size for mapping
		push	ebp								; size
		push	BTP_Kernel						; permissions
		push	edx								; physical base
		push	edx								; linear base = physical
		call	MapContiguous					; make mapping
		cmp		ecx, ebp						; used all of bank?
		je		MapRamBlock						; yes - next bank
		add		edx, ebp						; kernel->user transition address
		sub		ecx, ebp						; size left for user mapping

MapUserRamBlock:
		push	22								; max page size for mapping
		push	ecx								; size
		push	BTP_User						; permissions
		push	edx								; physical base
		push	edx								; linear base = physical
		call	MapContiguous					; make mapping
		jmp		MapRamBlock

MapRamBlockEnd:
		pop		ebp
ELSE

; multiple model

; map super page + CPU page
		PRTLN	"Map super/CPU pages"
		push	12								; page size
		push	SuperCpuSize					; size
		push	BTP_SuperCPU					; permissions
		push	edi								; physical
		push	KSuperPageLinAddr				; linear
		call	MapContiguous

; allocate one page for page table info and map it
		PRTLN	"Map PTINFO"
		push	12								; page size
		push	1000h							; size
		push	BTP_PtInfo						; permissions
		push	KPageTableInfoBase				; linear
		call	AllocAndMap

; on multiple model, map ASID info area
IFDEF	CFG_MMMultiple
		PRTLN	"Map ASID info"
		push	12								; page size
		push	1000h							; size
		push	BTP_PtInfo						; permissions
		push	KAsidInfoBase					; linear
		call	AllocAndMap
ENDIF

; on flexible model, map initial page array area
IFDEF	CFG_MMFlexible
		PRTLN	"Map PageArrayGroup"
		push	12								; page size
		push	1000h							; size
		push	BTP_Kernel						; permissions
		push	KPageArraySegmentBase				; linear
		call	AllocAndMap
ENDIF

ENDIF	; CFG_MMDirect

; map hardware
		PRTLN	"Map HW"
		BOOTCALL	BTF_HwBanks					; get pointer to list of HW banks into EAX
		mov		ebx, eax						; into EBX
		push	ebp

IFNDEF CFG_MMDirect
		mov		ebp, KPrimaryIOBase				; linear address for HW
ENDIF

MapHwBank:
		mov		edx, [ebx]						; phys
		mov		ecx, [ebx+4]					; size | mult
		PRINT	edx, "HwBank Phys"
		PRINT	ecx, "HwBank SizM"
		test	ecx, 0fffffffh					; size = 0?
		jz		MapHwBank_End					; if so, end of list
		add		ebx, 8
		mov		eax, ecx
		shr		eax, 31							; eax = 1 for 4M, 0 for 4K
		add		eax, eax
		lea		eax, [eax+eax*4+12]				; eax = 22 for 4M, 12 for 4K
		push	eax								; page size
		mov		eax, ecx
		shl		eax, 12							; size in bytes
		push	eax
		push	BTP_Hw							; default permissions
		push	edx								; physical
IFDEF CFG_MMDirect
		push	edx								; direct model, linear = physical
ELSE
		push	ebp								; linear
ENDIF
		test	ecx, HW_MAP_EXT2				; linear override?
		jz		short maphw2
		mov		eax, [ebx]
		add		ebx, 4
		mov		[esp], eax						; yes - take specified linear address
		jmp		short maphw3
maphw2:
IFNDEF CFG_MMDirect
		test	ecx, HW_MULT_4M					; 4M mapping requested?
		jz		short maphw4					; skip if not
		test	dword ptr [edi+SSuperPageBase_iCpuId], EX86Feat_PSE
		jz		short maphw4					; skip if 4M not supported
		add		ebp, 003fffffh
		and		ebp, 0ffc00000h					; round base up to 4M
		mov		[esp], ebp
maphw4:
		add		ebp, [esp+12]					; step linear address on
ENDIF
maphw3:
		test	ecx, HW_MAP_EXT					; permission override?
		jz		short maphw5
		mov		eax, [ebx]
		add		ebx, 4
		mov		[esp+8], eax					; yes - take specified permissions
maphw5:
		call	MapContiguous					; make mapping
		jmp		MapHwBank

MapHwBank_End:
		pop		ebp

		PRTLN	"Switch to virtual"
		call	SwitchToVirtual					; SWITCH TO VIRTUAL ADDRESSES

; get initial thread stack size

		mov		ebx, [ebp+TRomImageHeader_iStackSize]		; kernel stack size
		add		ebx, 0fffh
		and		ebx, 0fffff000h								; round up to 4K
		PRINT	ebx, "InitStackSize"
				
IFNDEF CFG_MMDirect

; calculate initial size of kernel heap, needs to be just enough to allow the device to
; boot past the point where the kernel heap is mutated to be growable.
		mov		eax, [edi+SSuperPageBase_iTotalRamSize]		; total RAM size
IFDEF CFG_MMFlexible
; Flexible memory model requires approx. 2 bytes per RAM page.
		shr		eax, 11										; total RAM size in half pages
ELSE
; Other memory models require approx. 1 byte per RAM page.
		shr		eax, 12										; total RAM size in pages
ENDIF

IFDEF CFG_KernelHeapMultiplier
		mov		ecx, CFG_KernelHeapMultiplier				; multiplier * 16
		mul		ecx
		shrd	eax, edx, 4
ELSE
		; default multiplier = 1 byte per page
ENDIF
IFDEF CFG_KernelHeapBaseSize
		add		eax, CFG_KernelHeapBaseSize					; add base size
ELSE
		add		eax, 24*1024								; default 24K
ENDIF
		PRINT	eax, "CalcInitHeap"
		mov		ecx, [ebp+TRomImageHeader_iHeapSizeMin]
		PRINT	ecx, "SpecInitHeap"
		cmp		eax, ecx
		jae		short calckheap1
		mov		eax, ecx									; if size specified in ROMBUILD > calc size, take specified size
calckheap1:
		add		eax, 0fffh
		and		eax, 0fffff000h								; round up to 4K
		PRINT	eax, "Preliminary InitHeap"

; map kernel .data/.bss, initial thread stack and kernel heap
		mov		ecx, [esi+TRomHeader_iTotalSvDataSize]		; total size of kernel .data / .bss
		add		ecx, 0fffh
		and		ecx, 0fffff000h								; round up to 4K
		PRINT	ecx, "Rounded SvData"

		push	12											; page size
		push	ecx											; size = SvData size + stack size + heap size
		add		[esp], ebx
		add		[esp], eax
		PRINT4	[esp], "Total SvHeap"
		push	BTP_Kernel
		push	dword ptr [esi+TRomHeader_iKernDataAddress]	; linear address
		PRTLN	"Map stack/heap"
		call	AllocAndMap

		mov		[edi+SSuperPageBase_iInitialHeapSize], eax	; initial heap size

ELSE	; CFG_MMDirect

		mov		ecx, [esi+TRomHeader_iTotalSvDataSize]		; total size of kernel .data / .bss
		add		ecx, 0fffh
		and		ecx, 0fffff000h								; round up to 4K
		PRINT	ecx, "Rounded SvData"
		mov		eax, [ebp+TRomImageHeader_iHeapSizeMax]		; kernel heap max
		mov		[edi+SSuperPageBase_iInitialHeapSize], eax	; save in super page
		PRINT	eax, "InitHeap"

ENDIF	; CFG_MMDirect

; fill initial thread stack with 0xff

		push	0ffffffffh									; fill value
		push	ebx											; size
		mov		edx, [esi+TRomHeader_iKernDataAddress]		;
		add		edx, ecx									; linear address = data address + size of data/bss
		push	edx
		PRINT	edx, "Initial stack base"
		call	WordFill

; switch to initial thread stack

		add		edx, ebx
		PRINT	edx, "Initial SP"
		mov		esp, edx

; initialise kernel .data and .bss

		PRTLN	"Init .data/.bss"
		mov		edx, [ebp+TRomImageHeader_iDataSize]		; size of .data
		push	edx
		push	dword ptr [ebp+TRomImageHeader_iDataAddress]	; source address
		push	dword ptr [esi+TRomHeader_iKernDataAddress]	; destination address
		PRINT4	[esp+4], "Kernel .data source"
		PRINT4	[esp+0], "Kernel .data dest"
		PRINT	edx, "Kernel .data size"
		call	WordMove									; initialise .data

		add		edx, 3
		and		dl, 0fch
		push	0
		sub		ecx, edx									; size of .bss
		push	ecx
		add		edx, [esi+TRomHeader_iKernDataAddress]		; base address of .bss
		push	edx
		PRINT	edx, "Kernel .bss dest"
		PRINT	ecx, "Kernel .bss size"
		call	WordFill									; clear .bss

IFNDEF CFG_MMDirect

; allocate SPageInfo array

		push	ebp
		PRTLN	"Map SPageInfo array start"

; PageInfoMap has 1 bit for each page of SPageInfo structures
; There are M SPageInfo structures in each page (M=4K/32 = 128)
; Bit n of PageInfoMap (counting from LSB of lowest addressed byte) is set if
; there is at least one page of RAM in the block of size M*4K starting at n*M*4K
; log2(M*4K) = 12 + log2(M) = 24 - KPageInfoShift
		mov		ebp, KPageInfoMap
		push	12								; page size
		push	1000h							; size
		push	BTP_Kernel						; permissions
		push	ebp								; linear
		call	AllocAndMap
		push	0
		push	1000h
		push	ebp
		call	WordFill						; zero memory

		mov		ebx, [edi+SSuperPageBase_iRamBootData]	; Pointer to list of RAM banks

PageInfoMapOuter:
		mov		edx, [ebx]						; bank physical base
		mov		ecx, [ebx+4]					; bank size
		cmp		ecx, 0
		jz		PageInfoMapEnd					; size=0 -> end of list
		lea		ecx, [edx+ecx-1]				; ecx = bank physical end (inclusive)
		shr		edx, (24 - KPageInfoShift)		; first KPageInfoMap bit number
		shr		ecx, (24 - KPageInfoShift)		; last KPageInfoMap bit number
		PRINT	edx, "PageInfoMap first bit"
		PRINT	ecx, "PageInfoMap last  bit"
		dec		edx
PageInfoMapInner:
		inc		edx
		bts		[ebp], edx						; set bit number EDX starting from [EBP] bit 0
		cmp		edx, ecx
		jne		short PageInfoMapInner			; loop until bit ECX has been set
		add		ebx, 8
		jmp		PageInfoMapOuter

PageInfoMapEnd:
		PRTLN	"PageInfoMap:"
		MEMDUMP	ebp, %(32 SHL KPageInfoShift)

; For each bit set in KPageInfoMap, allocate a page for SPageInfo structures and zero it
; If bit n in KPageInfoMap is set, we allocate a page at KPageInfoLinearBase + n*4K
		xor		ecx, ecx

PageInfoAllocFind:
		bt		[ebp], ecx						; test bit ECX in map
		jc		short PageInfoAllocFound		; if set, branch to find end of range
		inc		ecx
		cmp		ecx, (256 SHL KPageInfoShift)	; reached end?
		jne		short PageInfoAllocFind
		jmp		PageInfoAllocDone				; finished
PageInfoAllocFound:
		mov		edx, ecx						; edx = first pos
PageInfoAllocFindEnd:
		bt		[ebp], ecx						; test bit ECX in map
		jnc		short PageInfoAlloc				; if clear, branch to allocate pages
		inc		ecx
		cmp		ecx, (256 SHL KPageInfoShift)	; reached end?
		jne		short PageInfoAllocFindEnd
PageInfoAlloc:
		mov		eax, ecx
		sub		eax, edx						; EAX = number of pages
		shl		eax, 12							; size of region
		shl		edx, 12							; offset of region base
		add		edx, KPageInfoLinearBase		; EDX = base address to map
		push	22								; page size
		push	eax								; size
		push	BTP_Kernel						; permissions
		push	edx								; linear
		call	AllocAndMap						; make mapping
		push	0
		push	eax
		push	edx
		call	WordFill						; zero the mapped pages
		cmp		ecx, (256 SHL KPageInfoShift)	; reached end?
		jne		PageInfoAllocFind				; no - look for next range

PageInfoAllocDone:
		PRTLN	"Map SPageInfo array end"
		pop		ebp

ENDIF	; CFG_MMDirect

; do final hardware-dependent initialisation

		PRTLN	"Final platform dependent initialisation"
		BOOTCALL	BTF_Final

; final diagnostics

IFDEF CFG_DebugBootRom

		PRTLN	"Super page and CPU page:"
		MEMDUMP edi, SuperCpuSize							; dump super page + CPU page

		PRTLN	"Page directory:"
IFDEF CFG_MMDirect
		GETPARAM	BPR_PageTableSpace, 10000h				; get reserved space for page tables, default to 64K
		mov		edx, [edi+SSuperPageBase_iPageDir]
		add		edx, 1000h									; end of page dir
		sub		edx, eax									; base of page dir/page table area
ELSE
		mov		edx, [edi+SSuperPageBase_iPageDir]			; page directory address
		mov		eax, 1000h									; page directory size
ENDIF
		MEMDUMP edx, eax										; dump page directory

IFNDEF CFG_MMDirect
		mov		edx, KPageTableBase							; page table linear base
		lea		eax, [edx-4]
CountPt:
		add		eax, 4
		cmp		dword ptr [eax], 0							; Check if next PT0 entry is empty
		jnz		short CountPt								; if not, next
		sub		eax, edx									; number of page tables * 4
		shl		eax, 10										; 4K per page table
		PRTLN	"Page Tables:"
		MEMDUMP	edx, eax									; dump page tables
ENDIF

		PRTLN	"SvData:"
		mov		edx, [esi+TRomHeader_iKernDataAddress]
		mov		eax, [esi+TRomHeader_iTotalSvDataSize]
		MEMDUMP	edx, eax

ENDIF	; CFG_DebugBootRom


; boot the kernel
		push	edi											; pass address of super page
		push	esi											; pass address of ROM header
		mov		eax, [ebp+TRomImageHeader_iEntryPoint]		; kernel entry point
		PRINT   eax, "Jumping to OS at location"
IFDEF	CFG_DebugBootRom
		; pause to let tracing finish
		SPIN	10000000
ENDIF
		call	eax											; jump to kernel entry point

		; shouldn't return here
stop:
		cli
		hlt
		jmp		short stop


GetRomHeaderAddress PROC NEAR
		ADR		esi, TheRomHeader
		ret
		ENDP


;******************************************************************************

cseg	ends
end

;******************************************************************************

