// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\damandpaging\testdefs.h
// 
//

#ifndef __TESTDEFS_H__
#define __TESTDEFS_H__


const TInt KNandCollectGarbage	= 0x00000001;
const TInt KNandGetDeferStats	= 0x00000002;

struct SDeferStats
	{
	TInt iPageGarbage;
	TInt iPageOther;
	TInt iNormalGarbage;
	TInt iNormalOther;
	TUint16 iNormalFragmenting;
	TUint8 iClashFragmenting;
	TUint8 iSpare;
	
	// If ETrue, media driver writes behave synchronously, i.e. without 
	// suspending the media driver thread and waiting for an interrupt.
	TBool iSynchronousMediaDriver;
	};


#endif
