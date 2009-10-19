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
// e32test\heap\d_kheap.h
// 
//

#ifndef __D_KHEAP_H__
#define __D_KHEAP_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KHeapTestDriverName,"d_kheap");
const TUint KHeapFailCycles = 10;

class RKHeapDevice : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ESetThreadPriorityHigh,
		ECreateSharedChunk,
		ECreatHwChunk,
		ETestBurstFailNext,
		ETestBurstDeterministic,
		};

public:
#ifndef __KERNEL_MODE__
	TInt Open()
		{return DoCreate(KHeapTestDriverName,TVersion(1,0,0),KNullUnit,NULL,NULL);}
	TInt SetThreadPriorityHigh()
		{return DoControl(ESetThreadPriorityHigh);}
	TInt CreateSharedChunk()
		{return DoControl(ECreateSharedChunk);}
	#ifdef __EPOC32__
	TInt CreateHwChunk()
		{return DoControl(ECreatHwChunk);}
	#endif //__EPOC32__

	TInt TestBurstFailNext(TUint aControl, TUint aBurst)
		{return DoControl(ETestBurstFailNext, (TAny*)aControl, (TAny*)aBurst);}

	TInt TestBurstDeterministic(TUint aRate, TUint aBurst)
		{return DoControl(ETestBurstDeterministic, (TAny*)aRate, (TAny*)aBurst);}
#endif //__KERNEL_MODE__
	};
#endif //__D_KHEAP_H__
