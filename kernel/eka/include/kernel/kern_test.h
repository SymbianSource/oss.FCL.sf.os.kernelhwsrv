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
// e32\include\kernel\kern_test.h
// 
//

/**
 @file
 @internalComponent
*/

#ifndef __KERN_TEST_H__
#define __KERN_TEST_H__

#include <kernel/kernel.h>

// The methods in this class are for test use only and should have specific, single purposes.
// They are only functional in debug builds; release builds will return KErrNotSupported.
// Do not use for any purpose other than implementing e32tests.
class KernTest
	{
public:
	enum TTestFunction
		{
		EUserModeCallbackSleep,
		EUserModeCallbackSpin,		
		ERNGReseedHook,
		};

	IMPORT_C static TInt Test(TTestFunction aFunc, TAny* a1 = NULL, TAny* a2 = NULL, TAny* a3 = NULL);
	};

#endif //__KERN_TEST_H__
