; e32\include\kernel\arm\bootmacro.inc
;
; Copyright (c) 2003-2003 Symbian Ltd. All rights reserved.
;
;*******************************************************************************
;
; General macros for bootstrap
;

	IF	:LNOT: :DEF: __BOOTMACRO_INC__
	GBLL	__BOOTMACRO_INC__

	INIT_LOGICAL_SYMBOL	USE_CXSF

		MACRO
		GETCPSR	$reg
		MRS		$reg, cpsr
		MEND

		MACRO
		CGETCPSR	$cc, $reg
		MRS$cc		$reg, cpsr
		MEND

		MACRO
		GETSPSR	$reg
		MRS		$reg, spsr
		MEND

		MACRO
		CGETSPSR	$cc, $reg
		MRS$cc		$reg, spsr
		MEND

		MACRO
		SETCPSR	$reg
	[ USE_CXSF
		MSR		CPSR_cxsf, $reg
	|
		MSR		CPSR, $reg
	]
		MEND

		MACRO
		CSETCPSR	$cc, $reg
	[ USE_CXSF
		MSR$cc		CPSR_cxsf, $reg
	|
		MSR$cc		CPSR, $reg
	]
		MEND

		MACRO
		SETSPSR	$reg
	[ USE_CXSF
		MSR		SPSR_cxsf, $reg
	|
		MSR		SPSR, $reg
	]
		MEND

		MACRO
		CSETSPSR	$cc, $reg
	[ USE_CXSF
		MSR$cc		SPSR_cxsf, $reg
	|
		MSR$cc		SPSR, $reg
	]
		MEND

; Call a function via the boot table
		MACRO
		BOOTCALL	$offset
		BL			BootCall
		DCD			0xE3A0E000 :OR: $offset		; MOV LR, #$offset
		MEND

; Get a platform-dependent parameter
; Default if not supplied
		MACRO
		GETPARAM	$pnum, $default
		STR			lr, [sp, #-4]!
		LDR			r0, =$pnum
		BL			GetParameter
		LDRMI		r0, =$default
		LDR			lr, [sp], #4
		MEND

; Get a mandatory platform-dependent parameter
; Fault if not supplied
		MACRO
		GETMPARAM	$pnum, $default
		STR			lr, [sp, #-4]!
		LDR			r0, =$pnum
		BL			GetMandatoryParameter
		LDR			lr, [sp], #4
		MEND

; Output a string. Stack required.
		MACRO
		PRINT	$text
	[	CFG_DebugBootRom
		STR		lr, [sp, #-4]!
		BL		WriteS
		B		%FT9
		=		"$text", 0
		ALIGN
		LTORG
9
		LDR		lr, [sp], #4
	]
		MEND


; Output a string. Stack required.
		MACRO
		PRTLN	$text
	[	CFG_DebugBootRom
		STR		lr, [sp, #-4]!
		BL		WriteS
		B		%FT1
		=		"$text", 13, 10, 0
		ALIGN
		LTORG
1
		LDR		lr, [sp], #4
	]
		MEND


; Output a word, optionally preceded by some text, to the serial port if
; DebugBootRom==True. Stack required.
		MACRO
		DWORD	$reg, $text
	[	CFG_DebugBootRom
		PRINT	"$text:\t"
		STMFD	sp!, {r0,lr}
		MOV		r0, $reg
		BL		WriteW
		BL		WriteNL
		LDMFD	sp!, {r0,lr}
	]
		MEND

; Dump memory. Stack required.
		MACRO
		MEMDUMP	$base, $len
	[	CFG_DebugBootRom
		STMFD	sp!, {r0,r1,lr}
		MOV		r0, $base
		MOV		r1, $len
		BL		MemDump
		LDMFD	sp!, {r0,r1,lr}
	]
		MEND

; Init stacks for debug
		MACRO
		DEBUG_INIT_STACKS
	[	CFG_DebugBootRom
		BL		InitStacks
	]
		MEND

; Fault the system
		MACRO
		FAULT	$cc
		BL$cc	Fault
		MEND

	ENDIF	;	__BOOTMACRO_INC__

		END
