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
; e32\kernel\arm\bootcpu.s
; Bootstrap CPU specific routines
;

		GBLL	__BOOTCPU_S__

		INCLUDE	bootcpu.inc

	IF	SMP
		INCLUDE	arm_types.inc
		INCLUDE	arm_gic.inc
	ENDIF

;
;*******************************************************************************
;

	IF	SMP
        AREA |Boot$$Code|, CODE, READONLY, ALIGN=10		; so APBootCode won't cross a page boundary
	ELSE
        AREA |Boot$$Code|, CODE, READONLY, ALIGN=6
	ENDIF

;
;*******************************************************************************
;
	IF	SMP
;*******************************************************************************
; Auxiliary Processor Boot Code
;
;*******************************************************************************
	EXPORT	GetAPBootCodePhysAddr
GetAPBootCodePhysAddr	ROUT
		ADR		r0, APBootCodeEntry
		MOV		pc, lr

;*******************************************************************************
; Auxiliary Processor Boot Code
;
; Enter with R0 pointing to SFullArmRegSet
; SVC mode with interrupts disabled
;
; R0->iN.iR1 contains physical address of APBootPageDir
; R0->iN.iR2 contains offset from physical to virtual addresses
; R0->iN.iR3 contains virtual address of SFullArmRegSet
;
; *** THIS CODE MUST NOT STRADDLE A PAGE BOUNDARY ***
;
;*******************************************************************************
	EXPORT	APBootCodeEntry
APBootCodeEntry	ROUT
		MRC		p15, 0, r1, c1, c0, 0
		BIC		r1, r1, #MMUCR_I			; disable ICache
		MCR		p15, 0, r1, c1, c0, 0
		ARM_DSB
		ARM_ISB

	IF	CFG_ARMV7
		MOV		r12, r0						; save context pointer
		BL		PurgeDCache					; Invalidate data cache the long way (TZ friendly)
		MOV		r0, r12						; retrieve context pointer
		MOV		r1, #0
		MCR		p15, 0, r1, c7, c5, 0		; Invalidate ICache and BTAC (this core only)
		ARM_DSB
		ARM_ISB
	ELSE
		MCR		p15, 0, r1, c7, c7, 0		; Invalidate ICache, DCache and BTAC
		ARM_DSB
		ARM_ISB
	ENDIF

		LDR		r1, [r0, #SFullArmRegSet_iB+SBankedRegs_iDACR]
		MCR		p15, 0, r1, c3, c0, 0
		LDR		r1, [r0, #SFullArmRegSet_iB+SBankedRegs_iPRRR]
		MCR		p15, 0, r1, c10, c2, 0
		LDR		r1, [r0, #SFullArmRegSet_iB+SBankedRegs_iNMRR]
		MCR		p15, 0, r1, c10, c2, 1
		MOV		r1, #0
		MCR		p15, 0, r1, c13, c0, 0		; FCSE not used
		LDR		r2, [r0, #SFullArmRegSet_iB+SBankedRegs_iCTXIDR]
		ARM_DSB								; CONTEXTIDR write requires DSB immediately before
		MCR		p15, 0, r2, c13, c0, 1
		ARM_ISB								; CONTEXTIDR write requires ISB immmediately after
		MCR		p15, 0, r1, c2, c0, 2		; TTBCR = 0 initially
		LDR		r1, [r0, #SFullArmRegSet_iN+SNormalRegs_iR1]
		MCR		p15, 0, r1, c2, c0, 0		; TTBR0 = APBootPageDirPhys
		LDR		r1, [r0, #SFullArmRegSet_iB+SBankedRegs_iACTLR]
	IF	CFG_AuxCRPresent
		MCR		p15, 0, r1, c1, c0, 1		; AuxCR (includes coherency bit)
	ENDIF
		ARM_ISB
		LDR		r7, [r0, #SFullArmRegSet_iB+SBankedRegs_iSCTLR]	; desired final SCTLR
		LDR		r2, [r0, #SFullArmRegSet_iN+SNormalRegs_iR2]	; offset to add to PC
		LDR		r3, [r0, #SFullArmRegSet_iB+SBankedRegs_iTTBR0]
		LDR		r4, [r0, #SFullArmRegSet_iB+SBankedRegs_iTTBR1]
		LDR		r5, [r0, #SFullArmRegSet_iB+SBankedRegs_iTTBCR]
		LDR		r0, [r0, #SFullArmRegSet_iN+SNormalRegs_iR3]	; virtual address of SFullArmRegSet
		BIC		r1, r7, #MMUCR_I			; start off with no caching
		BIC		r1, r1, #MMUCR_C			; start off with no caching
		BIC		r1, r1, #MMUCR_Z			; start off with no caching
		MCR		p15, 0, r1, c8, c7, 0		; flush TLBs
		MCR		p15, 0, r1, c1, c0, 0		; enable MMU - APBootPageDir now in use
		ARM_ISB
		ADD		pc, pc, r2					; jump to virtual address mapping of this code
		NOP									; jump skips this
		MCR		p15, 0, r4, c2, c0, 1		; Final TTBR1
		ARM_ISB
		MCR		p15, 0, r3, c2, c0, 0		; Final TTBR0
		ARM_ISB
		MCR		p15, 0, r5, c2, c0, 2		; Final TTBCR
		MCR		p15, 0, r1, c8, c7, 0		; flush TLBs
		ARM_DSB
		ARM_ISB
		MCR		p15, 0, r7, c1, c0, 0		; enable caching now we are using the correct page tables
		ARM_ISB

		; restore other registers
		LDR		r1, [r0, #SFullArmRegSet_iB+SBankedRegs_iRWRWTID]
		MCR		p15, 0, r1, c13, c0, 2
		LDR		r1, [r0, #SFullArmRegSet_iB+SBankedRegs_iRWROTID]
		MCR		p15, 0, r1, c13, c0, 3
		LDR		r1, [r0, #SFullArmRegSet_iB+SBankedRegs_iRWNOTID]
		MCR		p15, 0, r1, c13, c0, 4

		IF		CFG_ARMV7
		LDR		r1, [r0, #SFullArmRegSet_iA+SAuxiliaryRegs_iTEEHBR]
		MCR		p14, 6, r1, c1, c0, 0
		ENDIF

		LDR		r1, [r0, #SFullArmRegSet_iA+SAuxiliaryRegs_iCPACR]
		MCR		p15, 0, r1, c1, c0, 2
		ARM_ISB								; CPACR write requires ISB immediately afterwards

		MOV		r1, #0xd1
		SETCPSR	r1
		ADD		r1, r0, #SFullArmRegSet_iN+SNormalRegs_iR8Fiq
		LDMIA	r1!, {r8-r14}
		LDR		r1, [r1]
		SETSPSR	r1
		MOV		r1, #0xd2
		SETCPSR	r1
		ADD		r1, r0, #SFullArmRegSet_iN+SNormalRegs_iR13Irq
		LDMIA	r1!, {r13-r14}
		LDR		r1, [r1]
		SETSPSR	r1
		MOV		r1, #0xdb
		SETCPSR	r1
		ADD		r1, r0, #SFullArmRegSet_iN+SNormalRegs_iR13Und
		LDMIA	r1!, {r13-r14}
		LDR		r1, [r1]
		SETSPSR	r1
		MOV		r1, #0xd7
		SETCPSR	r1
		ADD		r1, r0, #SFullArmRegSet_iN+SNormalRegs_iR13Abt
		LDMIA	r1!, {r13-r14}
		LDR		r1, [r1]
		SETSPSR	r1
		MOV		r1, #0xd3
		SETCPSR	r1
		ADD		r1, r0, #SFullArmRegSet_iN+SNormalRegs_iR13Svc
		LDMIA	r1!, {r13-r14}
		LDR		r1, [r0, #SFullArmRegSet_iN+SNormalRegs_iFlags]
		SETSPSR	r1				; spsr_svc = required flags
		LDMIA	r0, {r0-r14}^	; restore R0-R14_usr - R3 now points to SFullArmRegSet
		ADD		r3, r3, #SFullArmRegSet_iN+SNormalRegs_iR15
		LDMIA	r3, {pc}^		; return and restore flags


;*******************************************************************************
; Poll for an IPI
;
; Enter with:
;		R1 = how long to wait (loops), 0=forever
;		R8 = address of ARM GIC Distributor
;		R9 = address of ARM GIC CPU Interface
;		R11 = address of snoop control unit
;
; Return with:
;		R0 = interrupt ID received or -1 if timeout
;		Z flag set if OK, clear if timeout
;		R1 modified
;
;*******************************************************************************
PollForIPI		ROUT
4
		LDR		r0, [r9, #GicCpuIfc_iAck]		; Get next pending interrupt ID
		ARM_DSB
		TST		r0, #0x3F0						; IPI?
		BEQ		%FA2
		CMP		r0, #0x3FC						; valid interrupt ID?
		BHS		%FA3							; skip if not
		STR		r0, [r9, #GicCpuIfc_iEoi]		; Acknowledge interrupt
		ARM_DSB
3
		SUBS	r1, r1, #1
		BLO		%FA1							; waiting forever
		BNE		%BA4
		MVNS	r0, #0							; return -1 if timeout
		MOV		pc, lr
1
		ARM_WFI									; else wait for another interrupt
		MOV		r1, #0
		B		%BA4							;
2
		STR		r0, [r9, #GicCpuIfc_iEoi]		; Acknowledge IPI
		ARM_DSB
		MOV		pc, lr


;*******************************************************************************
; Echo an IPI
;
; Enter with:
;		R0 = ID of IPI (bits 0-9 = interrupt number, 10-12 = CPU number)
;		R8 = address of ARM GIC Distributor
;		R9 = address of ARM GIC CPU Interface
;		R11 = address of snoop control unit
;
; Return with:
;		R0 unmodified
;		R2 = R0
;
;*******************************************************************************
EchoIPI			ROUT
		MOV		r2, r0, LSR #10					; r2 = CPU number
		AND		r2, r2, #7
		LDRB	r2, [pc, r2]					; r2 = 1<<CPU number
		B		%FA1
		DCB		0x01,0x02,0x04,0x08
		DCB		0x10,0x20,0x40,0x80
1
		MOV		r2, r2, LSL #16					; r2 = 0x10000<<CPU number
		MOV		r0, r0, ROR #4					; IPI number into R0 bits 28-31
		ORR		r2, r2, r0, LSR #28				; r2 = (0x10000<<CPU number) | IPI number
		MOV		r0, r0, ROR #28					; restore R0
		STR		r2, [r8, #GicDistributor_iSoftIrq]	; echo IPI
		ARM_DSB									; make sure it's been sent before proceeding
		MOV		r2, r0
		MOV		pc, lr
	


;*******************************************************************************
; Receive a 32 bit value via IPIs
;
; Enter with:
;		R1 = how long to wait (loops) for RX to start, 0=forever
;		R8 = address of ARM GIC Distributor
;		R9 = address of ARM GIC CPU Interface
;		R11 = address of snoop control unit
;
; Return with:
;		If valid data received, N=0, R1=source CPU number, R0=data
;		If valid data not received, N=1, R1=-1, R0=0
;		R2-R5 modified
;
;*******************************************************************************
	IF	CFG_DebugBootRom
DefaultTimeout	EQU		0x01000000
	ELSE
DefaultTimeout	EQU		0x00001000
	ENDIF

RxViaIPI		ROUT
		MOV		r5, lr
		MVN		r3, #0							; will remember source CPU
1
		MOV		r4, #3							; will accumulate RX data
		ORR		r5, r5, #3
		BL		PollForIPI
		BNE		%FA0							; timeout
		AND		r2, r0, #0x0F					; r2 = IPI number
		CMP		r2, #4							; BOOT IPI ?
		BNE		%BA1							; no - wait for next
		CMP		r3, #0							; source CPU known?
		BICMI	r3, r0, #0x0F					; if not, remember this one
		EOR		r2, r0, r3						; BOOT IPI from expected CPU?
		CMP		r2, #4
		BNE		%BA1							; if not, wait for next
		BL		EchoIPI							; yes - echo BOOT IPI
2
		CMP		r1, #0
		CMPHI	r1, #DefaultTimeout
		MOVLO	r1, #DefaultTimeout				; ensure reasonable timeout
		BL		PollForIPI						; wait for another IPI
		BNE		%BA1							; timeout
		EOR		r2, r0, r3						; from source CPU?
		CMP		r2, #4
		BHS		%BA1							; if not, or BOOT IPI or invalid IPI, resync
		MOV		r2, r5, LSL #30					; previous checksum into r2[30:31]
		EOR		r0, r0, r2, LSR #30				; accumulated checksum into r0[0:1]
		BL		EchoIPI							; valid data - echo accumulated checksum
		EOR		r0, r0, r5
		AND		r0, r0, #3						; data bits back into R0[0:1]
		EOR		r5, r5, r0						; update checksum for next iteration
		MOVS	r0, r0, LSL #31					; first data bit into C, other into bit 31
		ADCS	r4, r4, r4						; shift into R4
		ADDS	r0, r0, r0						; second data bit
		ADCS	r4, r4, r4						; shift into R4, carry set if we have all 32 bits
		BCC		%BA2							; don't have 32 bits yet
		CMP		r1, #0
		CMPHI	r1, #DefaultTimeout
		MOVLO	r1, #DefaultTimeout				; ensure reasonable timeout
		BL		PollForIPI						; wait for another IPI
		BNE		%BA1							; timeout
		EOR		r2, r0, r3						; BOOT IPI from source CPU?
		BNE		%BA1							; no - resync
		BL		EchoIPI							; yes - echo it
		MOV		r0, r4							; and return data in R0
		MOVS	r1, r3, ASR #10					; and source CPU number in R1
		BIC		r5, r5, #3
		MOV		pc, r5
0
		MOV		r0, #0
		MVNS	r1, #0							; error - return r0=0, r1=-1
		BIC		r5, r5, #3
		MOV		pc, r5



;*******************************************************************************
; Send a 32 bit value via IPIs
;
; Enter with:
;		R0 = value to send
;		R1 = Destination CPU number
;		R8 = address of ARM GIC Distributor
;		R9 = address of ARM GIC CPU Interface
;		R11 = address of snoop control unit
;
; Return with:
;		R0-R5 modified
;
;*******************************************************************************

; Debug macro to print a register only if called by CPU 0
; NOTE: THIS TRASHES LR AND FLAGS
		MACRO
		DWORD0	$reg, $text
	[	CFG_DebugBootRom
		MRC p15, 0, lr, c0, c0, 5
		TST lr, #15
		BNE %FT8
		DWORD	$reg, $text
8
	]
		MEND


TxViaIPI		ROUT
		MOV		r5, lr
		MOV		r4, r0							; r4 = data to send
		MOV		r3, r1							; r3 = destination CPU
1
		MOV		r0, r3, LSL #10					; destination CPU into r0[10:12]
		ORR		r0, r0, #4						; with BOOT IPI number
		BL		EchoIPI							; send BOOT IPI to destination
		DWORD0	r0, "TxB>"
		MOV		r1, #DefaultTimeout*2
		BL		PollForIPI
		BNE		%BA1							; timed out - try again
		DWORD0	r0, "TxB<"
		CMP		r0, r2
		BNE		%BA1							; wrong response - try again
		BIC		r3, r3, #0xFF000000
		BIC		r3, r3, #0x00FF0000
		ORR		r3, r3, #0x0F000000				; loop counter n = 15 (bits 24-31)
		ORR		r5, r5, #3						; checksum
2
		MOV		r1, r3, LSR #23					; r1 = 2n
		MOV		r0, #3
		AND		r0, r0, r4, LSR r1				; r0 = bits 2n+1,2n of data shifted into bits 1,0
		EOR		r5, r5, r0						; update checksum
		ORR		r0, r0, r3, LSL #10				; add destination CPU number
		DWORD0	r3, "N"
		DWORD0	r0, "Tx>"
		BL		EchoIPI							; send data bits
		MOV		r1, #DefaultTimeout*2
		BL		PollForIPI						; wait for accumulated checksum to be echoed back
		DWORD0	r0, "Tx<"
		TST		r0, #0x80000000
		BNE		%BA1							; timeout - start again from scratch
		EOR		r2, r2, r5
		BIC		r2, r2, #3
		EOR		r2, r2, r5						; replace r2[1:0] with r5[1:0] (=checksum), leave r2[31:2] alone
		CMP		r2, r0							; check for correct echo
		BNE		%BA1							; wrong response
		SUBS	r3, r3, #0x01000000				; decrement n
		BCS		%BA2							; if n>=0 do next 2 bits
		MOV		r0, r3, LSL #10
		ORR		r0, r0, #4						; BOOT IPI, destination CPU number in bits 10-12
		DWORD0	r0, "TxE>"
		BL		EchoIPI							; send BOOT IPI to destination to terminate TX
		MOV		r1, #DefaultTimeout*32
		BL		PollForIPI
		DWORD0	r0, "TxE<"
		TST		r0, #0x80000000
		BNE		%BA1							; timeout - start again from scratch
		CMP		r0, r2
		BNE		%BA1							; wrong response - start again from scratch
		BIC		r5, r5, #3
		MOV		pc, r5							; done



;*******************************************************************************
; Set up GIC interface for this CPU and local stuff in the GIC Distributor
;
; Enter with:
;		R8 = address of ARM GIC Distributor
;		R9 = address of ARM GIC CPU Interface
;		R11 = address of snoop control unit
;
; Return with:
;		R0-R3 modified
;
;*******************************************************************************
LocalSetupGIC	ROUT
		GETCPSR	r0
		ORR		r0, r0, #0x1C0
		SETCPSR	r0											; disable all interrupts

		; set up GIC CPU interface
		MOV		r0, #0
		STR		r0, [r9, #GicCpuIfc_iCtrl]					; interface off
		ARM_DSB
		MOV		r0, #0xFF
		STR		r0, [r9, #GicCpuIfc_iPriMask]				; set priority mask = 0xFF
		ARM_DSB
		LDR		r0, [r9, #GicCpuIfc_iPriMask]				; read back to see which bits implemented
		RSB		r0, r0, #0x100								; r0 = lowest value of mask which allows some interrupts
		STR		r0, [r9, #GicCpuIfc_iPriMask]				; set priority mask to that
		ARM_DSB
		CLZ_M	0, 0
		STR		r0, [r9, #GicCpuIfc_iBinaryPoint]			; set binary point value
		ARM_DSB

		; set up GIC Distributor stuff which is local to this CPU
	IF	CFG_Cpu_Has_TrustZone
		MOV		r0, #0
		STR		r0, [r8, #GicDistributor_iIntSec]			; STIs and PPIs secure
		ARM_DSB
	ENDIF
		MVN		r0, #0
		STR		r0, [r8, #GicDistributor_iEnableClear]		; Disable all PPIs
		ARM_DSB
		STR		r0, [r8, #GicDistributor_iPendingClear]		; Clear any pending PPIs
		ARM_DSB
		MOV		r0, #0
		STR		r0, [r8, #GicDistributor_iPriority]			; Interrupts 0-3 highest priority
		MVN		r0, #0
		MOV		r0, r0, LSL #8
		STR		r0, [r8, #GicDistributor_iPriority+4]		; Interrupt 4 highest priority, 5-7 lowest
		MVN		r0, #0
		STR		r0, [r8, #GicDistributor_iPriority+8]		; Interrupts 8-11 lowest priority
		STR		r0, [r8, #GicDistributor_iPriority+12]		; Interrupts 12-15 lowest priority
		STR		r0, [r8, #GicDistributor_iPriority+16]		; Interrupts 16-19 lowest priority
		STR		r0, [r8, #GicDistributor_iPriority+20]		; Interrupts 20-23 lowest priority
		STR		r0, [r8, #GicDistributor_iPriority+24]		; Interrupts 24-27 lowest priority
		STR		r0, [r8, #GicDistributor_iPriority+28]		; Interrupts 28-31 lowest priority
		ARM_DSB

		; Is this CPU 0 ?
		MRC		p15, 0, r0, c0, c0, 5
		TST		r0, #0x0F
		BNE		%FA1										; skip if not

		; set up rest of GIC Distributor
		MOV		r0, #0
		STR		r0, [r8, #GicDistributor_iCtrl]
		LDR		r1, [r8, #GicDistributor_iType]
		DWORD	r1, "GIC Type"
		AND		r1, r1, #0x1F
		MOV		r1, r1, LSL #5
		DWORD	r1, "GIC Num SPIs"

		; disable all SPIs
		MVN		r0, #0
		ADD		r2, r8, #GicDistributor_iEnableClear
		ADD		r2, r2, #4
		ADD		r3, r2, r1, LSR #3
		DWORD	r2, "EnbClrBase"
		DWORD	r3, "EnbClrEnd"
2
		CMP		r2, r3
		STRLO	r0, [r2], #4
		BLO		%BA2
		ARM_DSB

		; clear all pending SPIs
		ADD		r2, r8, #GicDistributor_iPendingClear
		ADD		r2, r2, #4
		ADD		r3, r2, r1, LSR #3
		DWORD	r2, "PendClrBase"
		DWORD	r3, "PendClrEnd"
3
		CMP		r2, r3
		STRLO	r0, [r2], #4
		BLO		%BA3
		ARM_DSB

		; set all SPIs to lowest priority
		ADD		r2, r8, #GicDistributor_iPriority
		ADD		r2, r2, #32
		ADD		r3, r2, r1
		DWORD	r2, "PriBase"
		DWORD	r3, "PriEnd"
4
		CMP		r2, r3
		STRLO	r0, [r2], #4
		BLO		%BA4
		ARM_DSB

		; set all SPIs to target = none
		ADD		r2, r8, #GicDistributor_iTarget
		ADD		r2, r2, #32
		ADD		r3, r2, r1
		DWORD	r2, "TrgBase"
		DWORD	r3, "TrgEnd"
5
		CMP		r2, r3
		STRLO	r0, [r2], #4
		BLO		%BA5
		ARM_DSB

		MOV		r0, #1
		STR		r0, [r8, #GicDistributor_iCtrl]				; distributor on
1
		;
		MOV		r0, #1
		STR		r0, [r9, #GicCpuIfc_iCtrl]					; interface on
		MOV		pc, lr


;*******************************************************************************
; Auxiliary Processor Reset Entry Code
;
; Enter with:
;		R8 = address of ARM GIC Distributor
;		R9 = address of ARM GIC CPU Interface
;		R11 = address of snoop control unit
; Doesn't return
;
;*******************************************************************************
	EXPORT	APResetEntry
APResetEntry	ROUT
		BL		LocalSetupGIC

; Receive a 32 bit word via IPIs
1
		MOV		r1, #0
		BL		RxViaIPI
		CMP		r1, #0					; CPU number
		BNE		%BA1					; if not 0, try again
		MOV		r6, r0					; received data

; update SCU
		LDR		r1, [r11, #8]			; get SCU CPU status
		MRC		p15, 0, r4, c0, c0, 5
		AND		r4, r4, #0x0f			; r4 = CPU number 0-3
		MOV		r2, #3
		MOV		r2, r2, LSL r4
		MOV		r2, r2, LSL r4			; r2 = 3<<(2*cpu number)
		BIC		r1, r1, r2				; clear 2 bit field in SCU CPU status for this CPU
		STR		r1, [r11, #8]
		ARM_DSB

		MOV		r0, r6					; r0 = physical address of AP Boot Page
		ADD		r0, r0, #0x400
		ADD		r0, r0, r4, LSL #2		; r0 points to iAPBootPtr for this CPU
		MVN		r2, #0
		STR		r2, [r0]				; set it to 0xFFFFFFFF
		ARM_DSB
		ARM_SEV
		ARM_DSB
5
		LDR		r1, [r0]				; read iAPBootPtr
		ARM_DSB
		CMP		r1, r2					; valid?
		BICNE	r0, r1, #3				; if yes, r0 = address of register block ...
		BNE		APBootCodeEntry			; ... and jump to special doubly-mapped boot code
		ARM_WFE							; else wait
		B		%BA5					; and try again



;*******************************************************************************
; Auxiliary Processor Handshake Code
;
; Called by boot processor from BTF_Final function, so MMU on by now
;
; Enter with:
;		R6 = CPU number
;		R7 = virtual address of snoop control unit
;		R8 = virtual address of ARM GIC Distributor
;		R9 = virtual address of ARM GIC CPU Interface
;		R10 = virtual address of super page
;		R13 = virtual address of stack
;
; Return with:
;		No registers modified
;
;*******************************************************************************
	EXPORT	HandshakeAP
HandshakeAP	ROUT
		STMFD	sp!, {r0-r9,r11,lr}
		DWORD	r6, "HandshakeAP CPU"
		DWORD	r7, "HandshakeAP SCU"
		DWORD	r8, "HandshakeAP GIC D"
		DWORD	r9, "HandshakeAP GIC I"
		MOV		r11, r7

		CMP		r6, #1
		BLEQ	LocalSetupGIC
	ADD r11, r11, #16

		LDR		r1, [r7, #4]					; SCU configuration register
		DWORD	r1, "SCU Config"

		LDR		r0, [r10, #SSuperPageBase_iAPBootPagePhys]
		MOV		r1, r6
		DWORD	r0, "TxD"
		DWORD	r1, "TxC"
		BL		TxViaIPI						; send AP Boot Page Phys

		LDR		r0, [r10, #SSuperPageBase_iAPBootPageLin]
		ADD		r0, r0, #0x400
		ADD		r0, r0, r6, LSL #2
		DWORD	r0, "APBootPtr address"
1
		LDR		r1, [r0]
		ARM_DSB
		DWORD	r1, "APBootPtr value"
		CMN		r1, #1
		BEQ		%FA2
		ARM_WFE
		B		%BA1
2
		LDR		r1, [r7, #4]					; SCU configuration register
		DWORD	r1, "SCU Config"
		LDMFD	sp!, {r0-r9,r11,pc}


	ENDIF



;*******************************************************************************
; Initialise any CPU/MMU registers before switching on MMU
;
; Enter with :
;		R1 points to platform dependent boot parameter table
;		No valid stack
;
; Leave with :
;		Only R0-R7 modified
;*******************************************************************************
	EXPORT InitCpu
InitCpu	ROUT

	IF CFG_TrustZone_NonSecure :LOR: CFG_ARMV7
		MOV		r3, #0

	IF  ((:DEF: CFG_CPU_ARM1136 :LOR: :DEF: CFG_CPU_ARM1176) :LAND: :LNOT: :DEF: CFG_CPU_ARM1136_ERRATUM_411920_FIXED)
		PURGE_ICACHE r3, r0					; atomic ICache purge
	ELSE
		PURGE_ICACHE r3						; atomic ICache purge
	ENDIF


	; PurgeDCache needs more registers then InitCpu is supposed to use (r0-r7).
	; That is why we'll switch here to FIQ mode.(Stack is still not valid at this point.)

		;Switch to FIQ mode
		GETCPSR r5
		BIC r4, r5, #0xdf
		ORR r4, r4, #0xd1
		SETCPSR r4

		;Running in FIQ mode...
		MOV		r12, r1						; save parameter table pointer
		BL PurgeDCache						; purge DCache
		MOV		r1, r12						; set r1 back to table pointer
		MOV		r6, r12						; r6 = table pointer

		;Switch back to SVC mode
		GETCPSR r5
		BIC r4, r5, #0xdf
		ORR r4, r4, #0xd3
		SETCPSR r4

		MOV		r7, lr						; r7 = return address
		MOV		r3, #0
	ELSE	
		MOV		r7, lr						; save return address
		MOV		r6, r1						; save parameter table pointer
		MOV		r3, #0
		
		PURGE_IDCACHE r3
	ENDIF

	IF CFG_MemoryTypeRemapping
	; Configure PRRR & NMRR registers in CP15. They are defined by:
	; (1)DefaultPRRR, (2)DefaultNMRR and (3)BPR_Platform_Specific_Mappings Boot Parameter
		MOV		r0, #BPR_Platform_Specific_Mappings
		BL		FindParameter				; r0 = BPR_Platform_Specific_Mappings
		MOVMI	r0, #0						; r0 = 0, if parameter not defined in table.

		MOV		r1, r0						; r1 = BPR_Platform_Specific_Mappings
		AND		r0, #0x3f					; r0 = PRRR bits from BPR_Platform_Specific_Mappings (mappings 5-7)
		LDR		r2, =DefaultPRRR			; r2 = default PRRR (mappings 0-4)
		ORR		r2, r2, r0, lsl#10
		MCR		p15, 0, r2, c10, c2, 0		; set PRRR

		MOV		r0, #0xfc000000
		ORR		r0, #0xfc00					; r0 = mask for NMRR mappins 5-7
		AND		r1, r1, r0					; r1 = NMRR bits from BPR_Platform_Specific_Mappings
		LDR		r2, =DefaultNMRR			; r2 = default NMRR (mappings 0-4)
		ORR		r2, r2, r1
		MCR		p15, 0, r2, c10, c2, 1		; set NMRR

		MOV		r1, r6						; set r1 back to table pointer
	ENDIF

	IF	CFG_MMUPresent
		FLUSH_IDTLB r3
		MOV		r0, #BPR_InitialMMUCRClear
		BL		FindParameter
		MOVPL	r5, r0						; r5 = clear mask
		MVNMI	r5, #0						; default value = 0xFFFFFFFF
		MOV		r1, r6
		MOV		r0, #BPR_InitialMMUCRSet
		BL		FindParameter				; r0 = set mask
		LDRMI	r0, =InitialMMUCR			; default value - enables ICache if present
		MRC		p15, 0, r1, c1, c0, 0		; get MMUCR
		BIC		r1, r1, r5					; clear bits
		ORR		r1, r1, r0					; set bits
		MCR		p15, 0, r1, c1, c0, 0		; set MMUCR

	IF	CFG_MMDirect
		MOV		r1, #1						; direct model uses domain 0 only
	ELSE
		LDR		r1, =KSupervisorInitialDomainAccess
	ENDIF
		SET_DACR r1

	IF	CFG_FCSE_Present
		SET_FCSE r3							; clear FCSE
	ENDIF

	IF	CFG_ASID_Present
		SET_ASID r3							; use ASID 0 initially
	ENDIF

	IF	CFG_AuxCRPresent
		MOV		r0, #BPR_AuxCRClear
		MOV		r1, r6
		BL		FindParameter
		MOVPL	r5, r0						; r5 = clear mask
		LDRMI	r5, =DefaultAuxCRClear		; default value
		MOV		r1, r6
		MOV		r0, #BPR_AuxCRSet
		BL		FindParameter				; r0 = set mask
		LDRMI	r0, =DefaultAuxCRSet		; default value
		MRC		p15, 0, r1, c1, c0, 1		; get AuxCR
		BIC		r1, r1, r5					; clear bits
		ORR		r1, r1, r0					; set bits
		MCR		p15, 0, r1, c1, c0, 1		; set AuxCR
	ENDIF

	IF	CFG_CARPresent
		MOV		r0, #BPR_CAR
		MOV		r1, r6
		BL		FindParameter				; r0 = initial CAR value
		MOVMI	r0, #0						; default to no coprocessors enabled
		SET_CAR	r0
	ENDIF
		CPWAIT	r3

	ENDIF	; CFG_MMUPresent
		MOV		pc, r7
		LTORG

	IF	CFG_MMUPresent


	IF CFG_WriteThroughDisabled
;*******************************************************************************
; Cleans a region in DCache. Doesn't do write buffer nor tlb flush.
; Enter with :
;			r0 The starting address of the memory to be cleaned
;			r1 The size of the memory to clean. Treated as unsigned.
; 			   For ff.. value, this will go in loop for a long time.	
; Leave with :
;		Nothing modified
;*******************************************************************************
;	EXPORT	CleanDCacheRegion
CleanDCacheRegion	ROUT

	STMFD	sp!, {r0-r3,lr}

	CACHE_LINE_SIZE r2,r3		; r2 = cache line size
	SUB		r3, r2, #1 			; r3 = line size bit mask
	AND		r3, r3, r0			; r3 = offset of start address within cache-line
	BIC     r0, r0, r3			; round start address down to line-size ...
	ADD 	r1, r1, r3			; ... and add it to the size
1
	CLEAN_DCACHE_LINE r0,AL		; clean it
	ADD 	r0, r0, r2			; step address on for line length
	SUBS 	r1, r1, r2			; decrement size by line length
	BHI %BT1					; loop if more to clean (if r1>0)

	LDMFD	sp!, {r0-r3,pc}

	ENDIF	; CFG_WriteThroughDisabled

;*******************************************************************************
; Update page tables
;
; Enter with :
;			r0 The starting address of the memory that is updated
;			r1 The size of the memory that is updated.
; Leave with :
;		Nothing modified
;*******************************************************************************
	EXPORT	PageTableUpdate
PageTableUpdate	ROUT
		STMFD	sp!, {r0,lr}

	IF CFG_WriteThroughDisabled
		;As there is no Write Through cache mode, page tables must be in WB memory
		BL CleanDCacheRegion	
	ENDIF
		MOV		r0, #0
		DRAIN_WB r0
		FLUSH_IDTLB r0
		LDMFD	sp!, {r0,pc}

;*******************************************************************************
; Get PDE value
;
; Enter with :
;		R2 = index into boot table of required permission description
;			 or permission descriptor itself in standard form (BTP_ENTRY macro).
;		R4 = log2(page size of interest) = 12, 16 or 20
;		R10 points to super page
;		R13 points to valid stack
;
; Boot table entry is as follows:
;	Bits 0-3	= domain
;	Bits 4-6	= permissions
;	Bits 7-11	= cache attributes
;	Bit 12		= 1 if executable (ARMv6 only)
;	Bit	13		= 1 if global (ARMv6 only)
;	Bit	14		= 1 if ECC required (ARMv5,6)
;	Bit	15		= 1 if shared (ARMv6 only)
;
; Leave with :
;		R2 = PDE value
;		No other registers modified
;*******************************************************************************
	EXPORT	GetPdeValue
GetPdeValue	ROUT
		STMFD	sp!, {r0,lr}
		TST		r2, #BTP_FLAG				; index or permission descriptor?
		LDRPL	r0, [r10, #SSuperPageBase_iBootTable]
		LDRPL	r0, [r0, r2, lsl #2]		; r0 = boot table entry
		BICMI	r0, r2, #BTP_FLAG			; if permission, remove flag
		DWORD	r2, "GetPDE index"
		DWORD	r4, "GetPDE size"
		DWORD	r0, "GetPDE entry"
		AND		r2, r0, #0x0f				; r2 = domain
		MOV		r2, r2, LSL #PDE_DOM_SH		; shift into place
		ORR		r2, r2, #PDE_EXTRA			; add in extra bits
	IF	CFG_TEX
		TST		r0, #0x4000					; ECC?
		ORRNE	r2, r2, #PDE_P
	ENDIF
		CMP		r4, #20						; section required?
		ORRLO	r2, r2, #PDE_PT				; if page table, put in type bit
		BLO		%FA1						; skip if page table
		ORR		r2, r2, #PDE_SECTION		; put in section type bit
		AND		lr, r0, #0x30				; bottom 2 bits of permissions
		ORR		r2, r2, lr, LSL #6			; into PDE
		AND		lr, r0, #0x180				; bottom 2 bits of TEXCB
		ORR		r2, r2, lr, LSR #5			; into PDE
	IF	CFG_TEX
	IF CFG_MemoryTypeRemapping
		AND		lr, r0, #0x200				; With remapping, top 2 bits of TEX field are not used
	ELSE
		AND		lr, r0, #0xe00				; TEX field
	ENDIF
		ORR		r2, r2, lr, LSL #3			; into PDE
	ENDIF
	IF	CFG_ARMV6 :LOR: CFG_ARMV7
		TST		r0, #0x40					; APX
		ORRNE	r2, r2, #PDE_APX			; into PDE
		TST		r0, #0x1000					; executable?
		ORREQ	r2, r2, #PDE_XN				; if not, set XN
		TST		r0, #0x2000					; global?
		ORREQ	r2, r2, #PDE_NG				; if not, set NG
		TST		r0, #0x8000					; shared?
		ORRNE	r2, r2, #PDE_S				; if so, set S
	ENDIF
1
		DWORD	r2, "GetPDE value"
		LDMFD	sp!, {r0,pc}

;*******************************************************************************
; Get PTE value
;
; Enter with :
;		R2 = index into boot table of required permission description
;			 or permission descriptor itself in standard form (BTP_ENTRY macro).
;		R4 = log2(page size of interest) = 12 or 16
;		R10 points to super page
;		R13 points to valid stack
;
; Boot table entry is as follows:
;	Bits 0-3	= domain
;	Bits 4-6	= permissions
;	Bits 7-11	= cache attributes
;	Bit 12		= 1 if executable (ARMv6 only)
;	Bit	13		= 1 if global (ARMv6 only)
;	Bit	14		= 1 if ECC required (ARMv5,6)
;	Bit	15		= 1 if shared (ARMv6 only)
;
; Leave with :
;		R2 = PTE value
;		No other registers modified
;*******************************************************************************
	EXPORT	GetPteValue
GetPteValue	ROUT
		STMFD	sp!, {r0,lr}
		TST		r2, #BTP_FLAG				; index or permission descriptor?
		LDRPL	r0, [r10, #SSuperPageBase_iBootTable]
		LDRPL	r0, [r0, r2, lsl #2]		; r0 = boot table entry
		BICMI	r0, r2, #BTP_FLAG			; if permission, remove flag
		DWORD	r2, "GetPTE index"
		DWORD	r4, "GetPTE size"
		DWORD	r0, "GetPTE entry"
		AND		r2, r0, #0x30				; bottom 2 bits of permissions
		CMP		r4, #16						; check large or small page
	IF	:LNOT: (CFG_ARMV6 :LOR: CFG_ARMV7)
	IF	CFG_TEX
		ORRHS	r2, r2, r2, LSL #2			; for large page only, replicate into AP1, AP2, AP3
		ORRHS	r2, r2, r2, LSL #4
		ORRLO	r2, r2, #PTE_ESP			; for small page, always use extended small page
	ELSE
		ORR		r2, r2, r2, LSL #2			; replicate into AP1, AP2, AP3
		ORR		r2, r2, r2, LSL #4
	ENDIF
	ENDIF
		ORRHS	r2, r2, #PTE_LP				; for large page, add large page type bit
		ORRLO	r2, r2, #PTE_SP				; else add small page type bit
	IF	CFG_TEX
	IF CFG_MemoryTypeRemapping
		AND		lr, r0, #0x200				; With remapping, top 2 bits of TEX field are not used
	ELSE
		AND		lr, r0, #0xe00				; TEX field
	ENDIF
		ORRHS	r2, r2, lr, LSL #3			; for large page, TEX at bit 12
		ORRLO	r2, r2, lr, LSR #3			; for small page, TEX at bit 6
	ENDIF
		AND		lr, r0, #0x180				; CB bits
		ORR		r2, r2, lr, LSR #5			; into PTE
	IF	CFG_ARMV6 :LOR: CFG_ARMV7
		MOVHS	lr, #PTE_LP_XN				; XN bit for large page
		MOVLO	lr, #PTE_ESP_XN				; XN bit for small page
	IF	CFG_MMFlexible :LAND: CFG_MemoryTypeRemapping
		ORRLO	lr, #PTE_ESP_TEX1			; TEX1 is a copy of the XN bit for FlexibleMM with MemoryTypeRemapping
	ENDIF
		TST		r0, #0x40					; APX
		ORRNE	r2, r2, #PTE_APX			; into PDE
		TST		r0, #0x2000					; global?
		ORREQ	r2, r2, #PTE_NG				; if not, set NG
		TST		r0, #0x8000					; shared?
		ORRNE	r2, r2, #PTE_S				; if so, set S
		TST		r0, #0x1000					; executable?
		ORREQ	r2, r2, lr					; if not, set XN
	ENDIF
		DWORD	r2, "GetPTE value"
		LDMFD	sp!, {r0,pc}

;*******************************************************************************
; Enable the MMU and remove the temporary identity mapping
;
; Enter with :
;		R5	= page directory linear address
;		R6	= original PDE where identity mapping is now located
;		R7	= PDE index of temporary identity mapping
;		R9	= delta ROM header pointer (linear - physical)
;		R10 = physical super page pointer
;		R11 = physical kernel image pointer
;		R12 = physical ROM header pointer
;		R13 = physical stack pointer
;		R14 = physical return address
;
; Leave with :
;		R0 - R3 modified
;*******************************************************************************
	EXPORT	EnableMmu
EnableMmu	ROUT
		LDR		r0, [r10, #SSuperPageBase_iPageDir]
	IF	CFG_ARMV6 :LOR: CFG_ARMV7
		MOV		r1, r0
		GETPARAM BPR_TTBRExtraBits, 0x18		; r0 = bits 0-6 of TTBR values (cache/sharing attributes)
		ORR		r0, r0, r1						; for ARMv6 include extra caching bits
	ENDIF	; CFG_ARMV6 :LOR: CFG_ARMV7
	IF	:DEF: CFG_HasXScaleL2Cache
		MOV		r1, r0
		GETPARAM BPR_TTBRExtraBits, 0x18		;
		ORR		r0, r0, r1						; page walk L2 cache attributes for Manzano with L2 cache
	ENDIF	; CFG_HasXScaleL2Cache
		DWORD	r0, "TTBR0"
		MCR		p15, 0, r0, c2, c0, 0			; set TTBR

	IF	CFG_MMFlexible
		MCR		p15, 0, r0, c2, c0, 1			; set TTBR1 for Flexible Memory Model
		DWORD	r0, "TTBR1"
		MOV		r0, #1
		DWORD	r0, "TTBCR"
		MCR		p15, 0, r0, c2, c0, 2			; set TTCR for Flexible Memory Model (2GB local PD)
	ELSE
		IF	CFG_ARMV6 :LOR: CFG_ARMV7
			MCR		p15, 0, r0, c2, c0, 1			; set TTBR1 for ARMv6
			DWORD	r0, "TTBR1"
			LDR		r0, [r10, #SSuperPageBase_iTotalRamSize]
			LDR		r1, =CFG_ARMV6_LARGE_CONFIG_THRESHOLD
			CMP		r0, r1
			MOVHI	r0, #1							; if RAM size > threshold set N=1
			MOVLS	r0, #2							; if RAM size <= threshold set N=2
			MCR		p15, 0, r0, c2, c0, 2			; set TTCR for ARMv6
			DWORD	r0, "TTBCR"
		ENDIF	; CFG_ARMV6 :LOR: CFG_ARMV7
	ENDIF
		MRC		p15, 0, r2, c1, c0, 0			; r2 = MMU control register
		BIC		r1, r2, #MMUCR_I				; clear ICache enable
		MCR		p15, 0, r1, c1, c0, 0			; disable ICache
		MOV		r1, #0
	IF CFG_TrustZone_NonSecure :LOR: CFG_ARMV7
		IF  ((:DEF: CFG_CPU_ARM1136 :LOR: :DEF: CFG_CPU_ARM1176) :LAND: :LNOT: :DEF: CFG_CPU_ARM1136_ERRATUM_411920_FIXED)
		PURGE_ICACHE r1, r3						; atomic ICache purge
		ELSE
		PURGE_ICACHE r1							; atomic ICache purge
		ENDIF

		STMFD	sp!, {r0-r11,lr}
		BL PurgeDCache							; purge DCache
		LDMFD	sp!, {r0-r11,lr}
	ELSE
		PURGE_IDCACHE r1						; purge all caches
	ENDIF
		DRAIN_WB r1
		CPWAIT	r1

		MOV		r3, lr							; save lr
		GETPARAM BPR_FinalMMUCRClear, 0			; r0 = bits to clear in MMUCR
		BIC		r2, r2, r0						; clear them
		GETPARAM BPR_FinalMMUCRSet, ExtraMMUCR	; r0 = bits to set in MMUCR
		ORR		r0, r0, r2						; set them

		BIC		r1, r0, #MMUCR_I
		BIC		r1, r1, #MMUCR_C				; MMUCR without C and I
		PRTLN	"********************************************************************************"
		DWORD	r0, "EnableMMU"

		MOV	    r2, #0
		ARM_DSB
		MCR		p15, 0, r1, c1, c0, 0			; enable MMU, no caching yet
		ARM_ISB
		CPWAIT	r1
		ADD		pc, pc, r9						; jump to linear addresses
		NOP										; skips this
		STR		r6, [r5, r7, LSL #2]			; restore original PDE, overwriting temporary mapping
		MOV		r1, #0
		FLUSH_IDTLB	r1							; flush TLBs
		CPWAIT	r1
		ARM_ISB
		MCR		p15, 0, r0, c1, c0, 0			; enable caching
		ARM_DSB
		ARM_ISB
		ADD		pc, r3, r9						; return to linear address
		LTORG

	ENDIF	; CFG_MMUPresent

	IF CFG_ARMV7

;*******************************************************************************
; Performs Data(Unified) Cache initialisation (purge) on ARMv7.
; Doesn't use stack. Doesn't follow the standard calling convension and
; therefore, may not be called by C++ procedures.
; Leave with :
;		R0-R11 modified
;*******************************************************************************
	EXPORT PurgeDCache
PurgeDCache	ROUT

		MRC		p15, 1, r0, c0, c0, 1 		; Read CLIDR
		ANDS	r3, r0, #&7000000
		MOV 	r3, r3, LSR #23 			; Cache level value (naturally aligned)
		BEQ 	%FT5
		MOV 	r10, #0						; r10 is CurrentCacheLevel<<1
1
		ADD 	r2, r10, r10, LSR #1 		; Work out 3xcachelevel
		MOV 	r1, r0, LSR r2 				; bottom 3 bits are the Ctype for this level
		AND 	r1, r1, #7 					; get those 3 bits alone
		CMP 	r1, #2
		BLT		%FT4 						; no cache or only instruction cache at this level
		MCR 	p15, 2, r10, c0, c0, 0		; Cache Size Selection Register = CurrentCacheLevel<<1;
		MOV 	r1, #0
		ARM_ISB								; PrefetchFlush to sync the change to the CacheSizeID reg
		MRC 	p15, 1, r1, c0, c0, 0 		; reads current Cache Size ID register
		AND 	r2, r1, #&7 				; extract the line length field
		ADD 	r2, r2, #4 					; add 4 for the line length offset (log2 16 bytes)
		LDR 	r4, =0x3FF
		ANDS	r4, r4, r1, LSR #3 			; R4 is the max number on the way size (right aligned)
		CLZ_M 	5, 4 						; R5 is the bit position of the way size increment
		LDR 	r7, =0x00007FFF
		ANDS	r7, r7, r1, LSR #13 		; R7 is the max number of the index size (right aligned)
2
		MOV 	r9, r4 						; R9 working copy of the max way size (right aligned)
3
		ORR 	r11, r10, r9, LSL r5 		; factor in the way number and cache number into R11
		ORR 	r11, r11, r7, LSL r2 		; factor in the index number
		MCR 	p15, 0, r11, c7, c6, 2 		; invalidate (aka purge) by set/way
		SUBS	r9, r9, #1 					; decrement the way number
		BGE 	%BT3
		SUBS	r7, r7, #1 					; decrement the index
		BGE 	%BT2
4
		ADD 	r10, r10, #2 				; increment the cache number
		CMP 	r3, r10
		BGT 	%BT1
5
		MOV		pc, lr

	ENDIF ;	CFG_ARMV7

	IF :DEF: CFG_CPU_ARM1176

;*******************************************************************************
; Performs Data(Unified) Cache initialisation (purge) on 1176.
; Doesn't use stack. Doesn't follow the standard calling convension and
; therefore, may not be called by C++ procedures.
; Leave with :
;		R0-R5 modified
; Note: 1176 DCache is always 4-way cache.
;*******************************************************************************
	EXPORT PurgeDCache
PurgeDCache	ROUT

	MRC p15,0,r0,c0,c0,1 	; Read cache type reg
	AND r0,r0,#0x1C0000 	; Extract D cache size 
	MOV r0,r0, LSR #18 		; Move to bottom bits (values 3,4,5,6 or 7 for 4, 8, 16,32 or 64KB CacheSize)
	ADD r0,r0,#7 			; r0 = log2(CacheSize/4)
	
	MOV r5,#3				; Set counter: r5=3
	MOV r3,#1
	MOV r3,r3, LSL r0 		; r3 = Index loop max 
1
	MOV r2,#0 				; Index counter: r2=0
	MOV r1,r5,LSL#30 		; r1 = Set counter in upper 2 bits
2
	ORR r4,r2,r1 			; Set and Index format
	MCR p15,0,r4,c7,c6,2 	; Invalidate D cache line
	ADD r2,r2,#1:SHL:5 		; Increment Index counter
	CMP r2,r3 				; Done all index values?
	BNE %BT2 				; Loop until done

	SUBS r5,r5,#1 			; Decrease Set counter
	BPL %BT1				; Loop if zero or positive

	MOV		pc, lr

	ENDIF ;	CFG_CPU_ARM1176


	IF :DEF: CFG_BootedInSecureState
;*******************************************************************************
; - Sets AuxCR register
; - Switches CPU to Non-Secure Mode.
;
; ARM's TrustedZone is not used in Kernel.Therefore, this should be the first to execute
; if bootrom starts in Secure Mode. Must not be called if already in Non-Secure Mode.
; Runs with no valid stack.
;
; Routine MUST NEVER corrupt R2 or R3, see bootmain.s ResetEntry().
;
; Leave with :
;		R0-R1 modified
;		R2-R3 unmodified, preserved
;*******************************************************************************
	EXPORT SwitchToNonSecureState
SwitchToNonSecureState	ROUT

		MRC		p15, 0, r0, c1, c0, 1	; get AuxCR
		LDR		r1, =DefaultAuxCRClear	; default value
		BIC		r0, r0, r1				; clear bits
		LDR		r1, =DefaultAuxCRSet	; default value
		ORR		r0, r0, r1				; set bits
		MCR		p15, 0, r0, c1, c0, 1	; set AuxCR

		LDR 	r0, =0x73FFF
		MCR		p15, 0, r0, c1, c1, 2 	; Set full control from non-secure mode in NSAC register
		LDR 	r0, =0x31
		MCR		p15, 0, r0, c1, c1, 0 	; Switch to non-secure mode in SCR register
		ARM_ISB							; SCR write requires ISB immediately afterwards
		MOV		pc, lr
	ENDIF


	END
