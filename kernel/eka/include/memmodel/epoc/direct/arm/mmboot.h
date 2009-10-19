// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// eka\include\memmodel\epoc\direct\arm\mmboot.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __MMBOOT_H__
#define __MMBOOT_H__
#include <arm.h>
#include <memmodel.h>
#include <kernel/cache.h>


// Constants for ARM MMU
const TInt KPageShift=12;
const TInt KPageSize=1<<KPageShift;
const TInt KPageMask=KPageSize-1;

#endif	// __MMBOOT_H__
