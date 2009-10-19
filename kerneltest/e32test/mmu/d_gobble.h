// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\d_gobble.h
// 
//

#if !defined(__D_GOBBLE_H__)
#define __D_GOBBLE_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KGobblerLddName,"Gobbler");
_LIT(KGobblerLddFileName,"d_gobble");

class TCapsGobblerV01
	{
public:
	TVersion	iVersion;
	};

class RGobbler : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlGobbleRAM,
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KGobblerLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }
	inline TUint32 GobbleRAM(TUint32 aLeave)
		{ return (TUint32)DoControl(EControlGobbleRAM, (TAny*)aLeave, 0);}
#endif
	};

#endif
