// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\include\nkernsmp\x86\apic.h
// 
//

#define	X86_LOCAL_APIC_BASE			0xFEE00000

#define	X86_LOCAL_APIC_OFFSET_ID	0x20
#define	X86_LOCAL_APIC_OFFSET_VER	0x30
#define	X86_LOCAL_APIC_OFFSET_TPR	0x80
#define	X86_LOCAL_APIC_OFFSET_APR	0x90
#define	X86_LOCAL_APIC_OFFSET_PPR	0xA0
#define	X86_LOCAL_APIC_OFFSET_EOI	0xB0
#define	X86_LOCAL_APIC_OFFSET_LDR	0xD0
#define	X86_LOCAL_APIC_OFFSET_DFR	0xE0
#define	X86_LOCAL_APIC_OFFSET_SIVR	0xF0
#define	X86_LOCAL_APIC_OFFSET_ISR	0x100
#define	X86_LOCAL_APIC_OFFSET_TMR	0x180
#define	X86_LOCAL_APIC_OFFSET_IRR	0x200
#define	X86_LOCAL_APIC_OFFSET_ESR	0x280
#define	X86_LOCAL_APIC_OFFSET_ICRL	0x300
#define	X86_LOCAL_APIC_OFFSET_ICRH	0x310
#define	X86_LOCAL_APIC_OFFSET_LVTTMR	0x320
#define	X86_LOCAL_APIC_OFFSET_LVTTSR	0x330
#define	X86_LOCAL_APIC_OFFSET_LVTPMCR	0x340
#define	X86_LOCAL_APIC_OFFSET_LVTLINT0	0x350
#define	X86_LOCAL_APIC_OFFSET_LVTLINT1	0x360
#define	X86_LOCAL_APIC_OFFSET_LVTERR	0x370
#define	X86_LOCAL_APIC_OFFSET_INITCNT	0x380
#define	X86_LOCAL_APIC_OFFSET_CURRCNT	0x390
#define	X86_LOCAL_APIC_OFFSET_DIVCNF	0x3E0



#define	apic_reg(x)				X86_LOCAL_APIC_OFFSET_##x
#define read_apic_reg(x)		*((volatile TUint32*)(X86_LOCAL_APIC_BASE + apic_reg(x)))
#define write_apic_reg(x,y)		*((volatile TUint32*)(X86_LOCAL_APIC_BASE + apic_reg(x))) = (y)

#define __USE_LOGICAL_DEST_MODE__
