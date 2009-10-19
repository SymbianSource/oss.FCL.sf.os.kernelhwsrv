// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\heap\TestRHeapShrink.h
// 
//

#ifdef __EABI__
	IMPORT_D extern const TInt KHeapShrinkHysRatio;
#else
 // This value must be updated every time the value in heap.cpp is updated
	const TInt KHeapShrinkHysRatio = RHeap::EShrinkRatioDflt;
#endif
