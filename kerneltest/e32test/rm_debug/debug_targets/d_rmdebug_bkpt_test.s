; Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

        AREA |d-rmdebug-bkpt$$Code|, CODE, READONLY, ALIGN=6

	CODE32

;
; Breakpoints in loop test
; 
; This function initialises some variables and then performs some basic operations 
; within the for loop. This allows us to set multiple breakpoints within the loop 
; to test and see whether they are being hit. 
;

	EXPORT RMDebug_Bkpt_Test_Entry
	EXPORT RMDebug_Bkpt_Test_Loop_Break_1
	EXPORT RMDebug_Bkpt_Test_Loop_Break_2
	
RMDebug_Bkpt_Test_Entry
    mov r2,#10
    mov r0,#20    
    mov r3,#0
    mov r1,#1
    b COMPARE
LOOP      
    add r3,r2,r0   
RMDebug_Bkpt_Test_Loop_Break_1    
    mov r2,r0
RMDebug_Bkpt_Test_Loop_Break_2  
    mov r0,r3 
    add r1,r1,#1
COMPARE
    cmp r1,#30
    ble LOOP 
    bx lr
 
	END

; End of file - d_rmdebug_bkpt_test.s
