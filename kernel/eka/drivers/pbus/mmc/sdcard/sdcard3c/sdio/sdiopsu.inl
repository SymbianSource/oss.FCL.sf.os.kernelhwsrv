/*
* Copyright (c) 2003 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


#ifndef __SDIOPSU_INL__
#define __SDIOPSU_INL__

// ======== DSDIOPsu ========

inline void DSDIOPsu::Lock()
/**
Locks the PSU, preventing it from powering down
*/
	{ (void)__e32_atomic_store_ord32(&iIsLocked, 1); }

inline void DSDIOPsu::Unlock()
/**
Unlocks the PSU, allowing it to be powered down
*/
	{ (void)__e32_atomic_store_ord32(&iIsLocked, 0); }


#endif	// #ifndef __SDIOPSU_INL__

