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
// e32test\demandpaging\d_pagestress.h
// 
//

#ifndef __D_PAGESTRESS_H__
#define __D_PAGESTRESS_H__


#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KPageStressTestLddName,"D_PAGESTRESS");

class RPageStressTestLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EDoConsumeRamSetup,
		EDoConsumeRamFinish,
		EDoConsumeSomeRam,
		EDoReleaseSomeRam,
		EDoSetDebugFlag,
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{ return DoCreate(KPageStressTestLddName,TVersion(),KNullUnit,NULL,NULL,EOwnerProcess,ETrue); }
	inline TInt DoConsumeRamSetup(TInt aNumPagesLeft, TInt aPagesInBlock)
		{ return DoControl(EDoConsumeRamSetup,(TAny*)aNumPagesLeft, (TAny*)aPagesInBlock); }
	inline TInt DoConsumeRamFinish(void)
		{ return DoControl(EDoConsumeRamFinish,(TAny*)NULL, (TAny*)NULL); }
	inline TInt DoConsumeSomeRam(TInt aBlocks)
		{ return DoControl(EDoConsumeSomeRam,(TAny*)aBlocks, (TAny*)NULL); }
	inline TInt DoReleaseSomeRam(TInt aBlocks)
		{ return DoControl(EDoReleaseSomeRam,(TAny*)aBlocks, (TAny*)NULL); }
	inline TInt DoSetDebugFlag(TInt aState)
		{ return DoControl(EDoSetDebugFlag,(TAny*)aState, (TAny*)NULL); }
#endif
	};


#endif // __D_PAGESTRESS_H__
