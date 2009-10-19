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
// e32test\bench\d_kernasmbm.h
// Header defining interface to d_kernasmbm ldd
// 
//

#ifndef __D_KERNASMBM_H__
#define __D_KERNASMBM_H__

#include "t_asmbm.h"

_LIT(KKernAsmBmLddName,"d_kernasmbm");

const TInt KKernAsmBmBufferSize = 64*1024 + 32;

class RKernAsmBmLdd : public RBusLogicalChannel, public MBenchmarkList
	{
public:
	enum TControl
		{
		ECount,
		EInfo,
		ERun
		};
public:
	inline TInt Open();
	inline void Close();
	inline virtual TInt Count();
	inline virtual TInt Info(TInt aIndex, TBmInfo& aInfoOut);
	inline virtual TInt Run(TInt aIndex, const TBmParams& aParams, TInt& aDeltaOut);
private:
	TAny* iBuf;  // buffer to test kern <-> user transfers
	};

#ifndef __KERNEL_MODE__

#include <e32std.h>

inline TInt RKernAsmBmLdd::Open()
	{
	iBuf = User::Alloc(KKernAsmBmBufferSize + 32);
	if (!iBuf)
		return KErrNoMemory;
	return DoCreate(KKernAsmBmLddName,TVersion(0,1,1),KNullUnit,NULL,NULL);
	}

inline void RKernAsmBmLdd::Close()
	{
	User::Free(iBuf);
	iBuf = NULL;
	RBusLogicalChannel::Close();
	}

inline TInt RKernAsmBmLdd::Count()
	{
	return DoControl(ECount);
	}

inline TInt RKernAsmBmLdd::Info(TInt aIndex, TBmInfo& aInfoOut)
	{
	TPckg<TBmInfo> infoPckg(aInfoOut);
	return DoControl(EInfo | (aIndex << 8), &infoPckg);
	}

inline TInt RKernAsmBmLdd::Run(TInt aIndex, const TBmParams& aParams, TInt& aDeltaOut)
	{
	TPckgC<TBmParams> paramsPckg(aParams);
	TInt r = DoControl(ERun | (aIndex << 8), &paramsPckg, iBuf);
	if (r == KErrNone)
		aDeltaOut = *(TInt*)iBuf;
	return r;
	}

#endif

#endif
