// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#ifdef __CPU_NEEDS_BTAC_FLUSH_AFTER_ASID_CHANGE
#define __ASM_SYNC_AFTER_ASID_CHANGE(rTemp) asm("mcr p15, 0, "#rTemp", c7, c5, 6 "); // BPIALL (Branch predictor invalidate all)
#else
#define __ASM_SYNC_AFTER_ASID_CHANGE(rTemp)
#endif

#if defined(__CPU_ARM1136__) && defined(__HAS_EXTERNAL_CACHE__) && !defined(__CPU_ARM1136_ERRATUM_317041_FIXED)
#define __ASM_SET_ADDRESS_SPACE(rASID,rTTBR0,rTemp)   \
        asm("mrc p15, 0, "#rTemp", c2, c0, 1 ");    /* copy TTBR1... */                                     \
        asm("orr "#rTemp", "#rTemp", #0x18"); 		/* ERRATUM 1136_317041	*/								\
        asm("mcr p15, 0, "#rTemp", c2, c0, 0 ");    /* ... to TTBR0 (so we have global-only mappings) */    \
        __INST_SYNC_BARRIER_Z__(rTemp);                                                                     \
        __DATA_SYNC_BARRIER__(rTemp);               /* needed before change to ContextID */                 \
        asm("mcr p15, 0, "#rASID", c13, c0, 1 ");   /* set ContextID (ASID + debugging thread ID) */        \
        __INST_SYNC_BARRIER__(rTemp);                                                                       \
        asm("orr "#rTTBR0", "#rTTBR0", #0x18"); 	/* ERRATUM 1136_317041*/								\
        asm("mcr p15, 0, "#rTTBR0", c2, c0, 0 ");   /* set TTBR0*/                                          \
        __INST_SYNC_BARRIER__(rTemp);                                                                       \
        __ASM_SYNC_AFTER_ASID_CHANGE(rTemp)

#else
#define __ASM_SET_ADDRESS_SPACE(rASID,rTTBR0,rTemp)   \
        asm("mrc p15, 0, "#rTemp", c2, c0, 1 ");    /* copy TTBR1... */                                     \
        asm("mcr p15, 0, "#rTemp", c2, c0, 0 ");    /* ... to TTBR0 (so we have global-only mappings) */    \
        __INST_SYNC_BARRIER_Z__(rTemp);                                                                     \
        __DATA_SYNC_BARRIER__(rTemp);               /* needed before change to ContextID */                 \
        asm("mcr p15, 0, "#rASID", c13, c0, 1 ");   /* set ContextID (ASID + debugging thread ID) */                    \
        __INST_SYNC_BARRIER__(rTemp);                                                                       \
        asm("mcr p15, 0, "#rTTBR0", c2, c0, 0 ");   /* set TTBR0*/                                          \
        __INST_SYNC_BARRIER__(rTemp);                                                                       \
        __ASM_SYNC_AFTER_ASID_CHANGE(rTemp)
#endif

#ifndef __ASM_CLI

#ifdef __FIQ_IS_UNCONTROLLED__
#define	__ASM_CLI()							CPSIDI			/* Disable all interrupts */
#define	__ASM_STI()							CPSIEI			/* Enable all interrupts */
#else
#define	__ASM_CLI()							CPSIDIF			/* Disable all interrupts */
#define	__ASM_STI()							CPSIEIF			/* Enable all interrupts */
#endif

#endif
