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
// f32test\demandpaging\t_ramstress.h
// 
//

#ifndef _T_RAMSTRESS_H_
#define _T_RAMSTRESS_H_

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KRamStressTestLddName,"D_RAMSTRESS");

class RRamStressTestLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EDoMovePagesInZone,
		EDoSetDebugFlag,
		EDoSetEndFlag,
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{ return DoCreate(KRamStressTestLddName,TVersion(),KNullUnit,NULL,NULL,EOwnerProcess,ETrue); }
	inline TInt DoSetDebugFlag(TInt aState)
		{ return DoControl(EDoSetDebugFlag,(TAny*)aState, (TAny*)NULL); }
	inline TInt DoSetEndFlag(TInt aState)
		{ return DoControl(EDoSetEndFlag,(TAny*)aState, (TAny*)NULL); }
	inline TInt DoMovePagesInZone(TUint aZoneIndex)
		{ return DoControl(EDoMovePagesInZone,(TAny*)aZoneIndex, (TAny*)NULL); }
#endif
	};


#endif //_T_RAMSTRESS_H_

