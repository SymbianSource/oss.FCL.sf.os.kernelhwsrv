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

        
		AREA |d-rmdebug-step$$Code|, CODE, READONLY, ALIGN=6

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
; ARM -> Thumb -> ARM interworking test
;
; Note: We always start and finish this test
; in ARM mode.
	EXPORT RMDebug_StepTest_Interwork
	EXPORT RMDebug_StepTest_Interwork_1
	EXPORT RMDebug_StepTest_Interwork_2
	EXPORT RMDebug_StepTest_Interwork_3	
RMDebug_StepTest_Interwork
	mov		r0, lr	; preserve lr
RMDebug_StepTest_Interwork_1
	blx		RMDebug_StepTest_Interwork_2

	CODE16
RMDebug_StepTest_Interwork_2
	blx		RMDebug_StepTest_Interwork_3

	CODE32

RMDebug_StepTest_Interwork_3
	bx		r0

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

; Thumb tests

; Thumb non-pc modifying
;
;
RMDebug_StepTest_Thumb_Non_PC_Modifying
	mov		r0, lr	; preserve lr
	blx		RMDebug_StepTest_Thumb_Non_PC_Modifying_1
	bx		r0

;
; Thumb Branch
;
RMDebug_StepTest_Thumb_Branch
	mov		r0, lr	; preserve lr
	blx		RMDebug_StepTest_Thumb_Branch_1
	bx		r0		

;
; Thumb Branch and link
;
RMDebug_StepTest_Thumb_Branch_And_Link
	mov		r0, lr	; preserve lr
	blx		RMDebug_StepTest_Thumb_Branch_And_Link_1
	bx		r0 

;
; Thumb Back Branch and link
;
RMDebug_StepTest_Thumb_Back_Branch_And_Link
	mov		r0, lr	; preserve lr
	blx		RMDebug_StepTest_Thumb_Back_Branch_And_Link_1
	bx		r0 

;
; Thumb ADD PC,PC, #0
;
RMDebug_StepTest_Thumb_AddPC
	mov		r0, lr	; preserve lr
	blx		RMDebug_StepTest_Thumb_AddPC_1
	bx		r0 

	CODE16

	; Thumb tests
	EXPORT RMDebug_StepTest_Thumb_Non_PC_Modifying
	EXPORT RMDebug_StepTest_Thumb_Non_PC_Modifying_1
	EXPORT RMDebug_StepTest_Thumb_Non_PC_Modifying_2

	EXPORT RMDebug_StepTest_Thumb_Branch
	EXPORT RMDebug_StepTest_Thumb_Branch_1
	EXPORT RMDebug_StepTest_Thumb_Branch_2

	EXPORT RMDebug_StepTest_Thumb_Branch_And_Link
	EXPORT RMDebug_StepTest_Thumb_Branch_And_Link_1
	EXPORT RMDebug_StepTest_Thumb_Branch_And_Link_2
	EXPORT RMDebug_StepTest_Thumb_Branch_And_Link_3

	EXPORT RMDebug_StepTest_Thumb_Back_Branch_And_Link
	EXPORT RMDebug_StepTest_Thumb_Back_Branch_And_Link_1
	EXPORT RMDebug_StepTest_Thumb_Back_Branch_And_Link_2
	EXPORT RMDebug_StepTest_Thumb_Back_Branch_And_Link_3

RMDebug_StepTest_Thumb_Non_PC_Modifying_1
	mov		r0, r0	; nop
RMDebug_StepTest_Thumb_Non_PC_Modifying_2
	bx		lr	

RMDebug_StepTest_Thumb_Branch_1
	b		RMDebug_StepTest_Thumb_Branch_2
	mov		r0, r0
RMDebug_StepTest_Thumb_Branch_2
	bx		lr

RMDebug_StepTest_Thumb_Branch_And_Link_1
	mov		r1, lr
RMDebug_StepTest_Thumb_Branch_And_Link_2
	bl		RMDebug_StepTest_Thumb_Branch_And_Link_3
	mov		r0, r0
RMDebug_StepTest_Thumb_Branch_And_Link_3
	bx		r1

RMDebug_StepTest_Thumb_Back_Branch_And_Link_3
	bx		r1

RMDebug_StepTest_Thumb_Back_Branch_And_Link_1
	mov		r1, lr
RMDebug_StepTest_Thumb_Back_Branch_And_Link_2
	bl		RMDebug_StepTest_Thumb_Back_Branch_And_Link_3
	bx		r1

;
; ADD PC
;
	EXPORT RMDebug_StepTest_Thumb_AddPC
	EXPORT RMDebug_StepTest_Thumb_AddPC_1
	EXPORT RMDebug_StepTest_Thumb_AddPC_2
	EXPORT RMDebug_StepTest_Thumb_AddPC_3

RMDebug_StepTest_Thumb_AddPC_1
	mov		r1, lr
	mov		r2, #4
RMDebug_StepTest_Thumb_AddPC_2
	add		pc, pc, r2	; should arrive at RMDebug_StepTest_Thumb_AddPC_3
	mov		r0, r0
	mov		r0, r0
	mov		r0, r0
RMDebug_StepTest_Thumb_AddPC_3
	bx		r1

	ALIGN 4

	CODE32

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

; End of file - d_rmdebug_step_test.s
