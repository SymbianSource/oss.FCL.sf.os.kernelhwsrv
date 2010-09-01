; Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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

        AREA |d-rmdebug-bkpt$$Code|, CODE, READONLY, ALIGN=6

	CODE32

	; ARM tests
	
; 
; Non-PC modifying
;
	EXPORT RMDebug_StepTest_Non_PC_Modifying
	EXPORT RMDebug_StepTest_Non_PC_Modifying_OK
 
RMDebug_StepTest_Non_PC_Modifying
	mov		r0,r0		; nop
RMDebug_StepTest_Non_PC_Modifying_OK
	bx		lr			; should return to normal execution of the test thread

;
; Branch
;
	EXPORT RMDebug_StepTest_Branch
	EXPORT RMDebug_StepTest_Branch_1

RMDebug_StepTest_Branch
	b		RMDebug_StepTest_Branch_1		
	mov		r0, #2		; if the pc ends up here, we know its gone wrong
RMDebug_StepTest_Branch_1
	bx		lr			; return

;
; Branch and Link
;
	EXPORT RMDebug_StepTest_Branch_And_Link
	EXPORT RMDebug_StepTest_Branch_And_Link_1
	EXPORT RMDebug_StepTest_Branch_And_Link_2

RMDebug_StepTest_Branch_And_Link		
	mov		r0, lr		; preserve lr for the moment
RMDebug_StepTest_Branch_And_Link_1
	bl		RMDebug_StepTest_Branch_And_Link_2
	mov		r1, #1		; insert a gap in the instruction stream so we know we branched.
RMDebug_StepTest_Branch_And_Link_2
	mov		lr, r0		; restore lr			
	bx		lr			; should return to normal execution of the test thread

;
; MOV PC
;
	EXPORT RMDebug_StepTest_MOV_PC
	EXPORT RMDebug_StepTest_MOV_PC_1
	EXPORT RMDebug_StepTest_MOV_PC_2

RMDebug_StepTest_MOV_PC
	mov		r0, #4
RMDebug_StepTest_MOV_PC_1
	add		pc, pc, r0	; should be a jump (bear in mind reading pc = current inst + 8bytes for arm)
	mov		r0, #1		; Simple instructions which allow us to test where the PC really is
	mov		r0, #2		; just by reading r0.
RMDebug_StepTest_MOV_PC_2
	mov		r0, #3		; 
	mov		r0, #4		; 
	bx		lr			; should return to normal execution of the test thread

; 
; LDR PC
;
	EXPORT RMDebug_StepTest_LDR_PC
	EXPORT RMDebug_StepTest_LDR_PC_1

RMDebug_StepTest_LDR_PC
	ldr		pc, =RMDebug_StepTest_LDR_PC_1
	mov		r0, #1		;  separate the branch target so we can prove it works
RMDebug_StepTest_LDR_PC_1
	bx		lr			; should return to normal execution of the test thread
	
;
; Stepping performance tests
;
; This counts down from 100000 to 0
; This means that for all practical purposes
; we can single-step as much as we like
; in less than one second and have some likelyhood
; that we will not step too far from our loop

	EXPORT RMDebug_StepTest_Count
	EXPORT RMDebug_StepTest_Count_1
	EXPORT RMDebug_StepTest_Count_2

RMDebug_StepTest_Count
	ldr		r2, =100000
RMDebug_StepTest_Count_1
	subs	r2, r2, #1
RMDebug_StepTest_Count_2
	bne		RMDebug_StepTest_Count_1
	bx		lr

;
; ARM multiple-step ( 5 steps )
;
	EXPORT RMDebug_StepTest_ARM_Step_Multiple
	EXPORT RMDebug_StepTest_ARM_Step_Multiple_1

RMDebug_StepTest_ARM_Step_Multiple
	mov		r0,r0		; nop
	mov		r0,r0		; nop
	mov		r0,r0		; nop
	mov		r0,r0		; nop
	mov		r0,r0		; nop
RMDebug_StepTest_ARM_Step_Multiple_1
	bx		lr

	END

; End of file - d_rmdebug_step_test_armv4.s
