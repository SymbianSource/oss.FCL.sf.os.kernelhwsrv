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
// e32/include/nkern/arm/irq_exit.h
// 
//

#ifndef __IRQ_EXIT_H__
#define __IRQ_EXIT_H__

#include <e32def.h>

/**
	@file
	@publishedPartner
	@released
*/

/**
	IRQ Postamble which will not reschedule (can be returned to by co-resident OS).
	This routine is called after the IRQ has been dispatched.
	It may be returned to instead of __ArmVectorIrq() if a reschedule must be avoided.
*/
extern "C" IMPORT_C void __ArmVectorIrqPostambleNoResched();

/**
	FIQ Postamble which will not reschedule (can be returned to by co-resident OS).
	This routine is called after the FIQ has been dispatched.
	It may be returned to instead of __ArmVectorFiq() if a reschedule must be avoided.
*/
extern "C" IMPORT_C void __ArmVectorFiqPostambleNoResched();

#endif	// __IRQ_EXIT_H__
