// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\testusbcldd\inc\dtestusblogdev.h
// Macro's, typedefs and definitions to account for changes between EKA1 and EKA2.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __KERNDEFS_H_
#define __KERNDEFS_H_

#include <kernel/kern_priv.h>

/** 
 *	OS Version Specifics - Macro'd here to facilitate sharing of code between EKA1 and EKA2
 */
typedef HBuf8							HBuf8Plat;
#define __NEWPLATBUF(a,b)				a = HBuf8Plat::New(b); if(a) a->SetMax();
#define __KSTRING(x)					x
#define __MEMCPY(a, b, c)				memcpy((a), (b), (c));
#define __THREADWRITE(a, b, c)			Kern::ThreadDesWrite((a), (b), (c), 0);
#define __THREADWRITEOFFSET(a, b, c, d)	Kern::ThreadDesWrite((a), (b), (c), (d), (a));
#define __THREADRAWWRITE(a, b, c, d)	Kern::ThreadRawWrite((a), (b), (c), (d), (a));
#define __THREADREAD(a, b, c)			Kern::ThreadDesRead((a), (b), (c), 0);
#define __THREADRAWREAD(a, b, c, d)		Kern::ThreadRawRead((a), (b), (c), (d));
#define __THREADDESLEN(a, b)			Kern::ThreadGetDesLength((a), (b));
#define __THREADPANIC(a, b)				Kern::ThreadKill((a), EExitPanic, (b), KLitKernExec);
#define __THREADREADPLATBUF(a, b, c, d)	(d) = Kern::ThreadDesRead((a), (b), *(c), 0);

#endif // __KERNDEFS_H
