// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// eka\include\memmodel\epoc\direct\x86\mmboot.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __MMBOOT_H__
#define __MMBOOT_H__
#include <memmodel.h>


#ifndef __KPAGESIZE_DEFINED__
const TInt KPageShift=12;
const TInt KPageSize=1<<KPageShift;
const TInt KPageMask=KPageSize-1;
#define __KPAGESIZE_DEFINED__
#endif

#endif	// __MMBOOT_H__
