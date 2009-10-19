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
// f32\inc\runtests.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __RUNTESTS_H__
#define __RUNTESTS_H__

/**
The SID of RUNTESTS.EXE and the category of it PubSub properties
*/
const TInt32 KRuntestsCategoryValue = 0x10210F4F;
const TUid KRuntestsCategory = {KRuntestsCategoryValue};

/**
The key identifier for the RUNTESTS publish and subscribe property which contains
the name of the currently executing test.
*/
const TUint KRuntestsCurrentTestKey = 0;

/**
The key identifier of the publish and subscribe property which a process should
create in order to signal RUNTESTS that it persists intentionally.
*/
const TUint KRuntestsIntentionalPersistenceKey = 0x9E3779B9u;

/**
The key value of the publish and subscribe property which a process should
create in order to signal RUNTESTS that it persists intentionally.
*/
const TUint KRuntestsIntentionalPersistenceValue = 0xCF1BBCDDu;


#endif
