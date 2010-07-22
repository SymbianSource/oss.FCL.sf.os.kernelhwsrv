// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\x86\ncutilf.cpp
// 
//

#include <nk_priv.h>


/******************************************************************************
 * Spin lock
 ******************************************************************************/
/** Create a spin lock

	@publishedPartner
	@released
*/
EXPORT_C TSpinLock::TSpinLock(TUint aOrder)
	{
	(void)aOrder;
	__NK_ASSERT_DEBUG( (aOrder==EOrderNone) || ((aOrder&0x7f)<0x20) );
	if (aOrder>=0x80 && aOrder!=EOrderNone)
		aOrder -= 0x60;
	aOrder |= 0xFF00u;
	iLock = TUint64(aOrder)<<48;	// byte 6 = 00-1F for interrupt, 20-3F for preemption
									// byte 7 = FF if not held
	}


/******************************************************************************
 * Read/Write Spin lock
 ******************************************************************************/
/** Create a spin lock

	@publishedPartner
	@released
*/
EXPORT_C TRWSpinLock::TRWSpinLock(TUint aOrder)
	{
	(void)aOrder;
	__NK_ASSERT_DEBUG( (aOrder==TSpinLock::EOrderNone) || ((aOrder&0x7f)<0x20) );
	if (aOrder>=0x80 && aOrder!=TSpinLock::EOrderNone)
		aOrder -= 0x60;
	aOrder |= 0xFF00u;
	iLock = TUint64(aOrder)<<48;	// byte 6 = 00-1F for interrupt, 20-3F for preemption
									// byte 7 = FF if not held
	}


/** Get the frequency of counter queried by NKern::Timestamp().
*/
EXPORT_C TUint32 NKern::TimestampFrequency()
	{
	return TheScheduler.iVIB->iTimestampFreq;
	}

