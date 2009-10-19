; Copyright (c) 2007 Nokia Corporation and/or its subsidiary(-ies).
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
; e32\include\kernel\x86\bootmacro.inc
;
;
;*******************************************************************************
;
; General macros for bootstrap
;

IFNDEF __BOOTMACRO_INC__
__BOOTMACRO_INC__	equ 0

;*******************************************************************************
; Macros for instructions not supported by TASM
; i.e. those only available on Pentium and above
;*******************************************************************************
	cpuid	MACRO
	db		0fh,0a2h
	ENDM

REG_EAX		equ	0
REG_ECX		equ	1
REG_EDX		equ	2
REG_EBX		equ	3
REG_ESP		equ	4
REG_EBP		equ	5
REG_ESI		equ	6
REG_EDI		equ	7

	setcr	MACRO	creg, reg
	db		0fh, 22h, (0c0h+8*(creg)+(reg))
	ENDM

	getcr	MACRO	reg, creg
	db		0fh, 20h, (0c0h+8*(creg)+(reg))
	ENDM


;*******************************************************************************
; Get the address of a label using PC-relative addressing (stack required)
;*******************************************************************************
	ADR		MACRO	reg, sym
	LOCAL	there
	call	there
there:
	pop		&reg
	add		&reg, (&sym - there)
	ENDM
	

;*******************************************************************************
; Get the address of a label using PC-relative addressing and stack it
;*******************************************************************************
	PUSHADR	MACRO	sym
	LOCAL	there
	call	there
there:
	add		dword ptr [esp], (&sym - there)
	ENDM
	

;*******************************************************************************
; Get the value of EIP (stack required)
;*******************************************************************************
	GETEIP	MACRO	reg
	LOCAL	here
here:
	ADR		reg, here
	ENDM
	

;*******************************************************************************
; Busy wait
;*******************************************************************************
	SPIN	MACRO	times
	LOCAL	spinloop
	push	eax
	mov		eax, times
spinloop:
	sub		eax, 1
	jnz		spinloop
	pop		eax
	ENDM

	SPINFOREVER	MACRO
	LOCAL	spinloop
spinloop:
	jmp		short spinloop
	ENDM


;*******************************************************************************
; Call a function via the boot table
;*******************************************************************************
	BOOTCALL	MACRO	offset
	call	DoBootCall
	dd		offset
	ENDM


;*******************************************************************************
; Output a string. Stack required.
;*******************************************************************************
	PUTS	MACRO	text
IFDEF	CFG_DebugBootRom
	call	WriteS
	db		text,0
ENDIF
	ENDM


;*******************************************************************************
; Output a string. Stack required.
;*******************************************************************************
	PRTLN	MACRO	text
IFDEF	CFG_DebugBootRom
	call	WriteS
	db		text,13,10,0
ENDIF
	ENDM


;*******************************************************************************
; Output a word, optionally preceded by some text, to the serial port.
;*******************************************************************************
	PRINT	MACRO	arg, text
IFDEF	CFG_DebugBootRom
	push	arg
IFNB <text>
	call	WriteS
	db		text,32,0
ENDIF
	call	WriteW
	call	WriteNL
ENDIF
	ENDM


	PRINT4	MACRO	arg, text
IFDEF	CFG_DebugBootRom
	push	dword ptr arg
IFNB <text>
	call	WriteS
	db		text,32,0
ENDIF
	call	WriteW
	call	WriteNL
ENDIF
	ENDM


;*******************************************************************************
; Output a byte, optionally preceded by some text, to the serial port.
;*******************************************************************************
	PRINTB	MACRO	arg, text
IFDEF	CFG_DebugBootRom
	push	0
	mov		[esp], &arg
IFNB <text>
	call	WriteS
	db		text,32,0
ENDIF
	call	WriteB
	call	WriteNL
ENDIF
	ENDM


;*******************************************************************************
; Output a 16 bit word, optionally preceded by some text, to the serial port.
;*******************************************************************************
	PRINTW	MACRO	arg, text
IFDEF	CFG_DebugBootRom
	push	0
	mov		[esp], &arg
IFNB <text>
	call	WriteS
	db		text,32,0
ENDIF
	call	WriteH
	call	WriteNL
ENDIF
	ENDM


;*******************************************************************************
; Dump memory. Stack required.
;*******************************************************************************
	MEMDUMP	MACRO	base, size
IFDEF	CFG_DebugBootRom
	push	size
	push	base
	call	DoMemDump
ENDIF
	ENDM


;*******************************************************************************
; Fault the system
;*******************************************************************************
FAULT	MACRO	cc
	LOCAL	skip
	LOCAL	fail
IFB	<cc>
	call	DoFault
ELSE
	j&cc	short fail
	jmp		short skip
fail:
	call	DoFault
skip:
ENDIF
	ENDM


;*******************************************************************************
; Get a platform-dependent parameter
; Default if not supplied
;*******************************************************************************
	GETPARAM	MACRO	pnum, default
	LOCAL	skip
	BOOTCALL	BTF_Params
	dd		pnum
	jnc		short skip
	mov		eax, default
skip:
	ENDM


;*******************************************************************************
; Get a mandatory platform-dependent parameter
; Fault if not supplied
;*******************************************************************************
	GETMPARAM	MACRO	pnum
	BOOTCALL	BTF_Params
	dd		pnum
	FAULT	c
	ENDM



ENDIF	;	__BOOTMACRO_INC__

