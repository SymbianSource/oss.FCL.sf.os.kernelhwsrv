// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\resmanus\d_resmanusbtrace.h
// 
//

#if !defined(__D_RESMANUSBTRACE_H__)
#define __D_RESMANUSBTRACE_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"D_RESMANUSBTRACE.LDD");

class RLddTest1 : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ECONTROL_SENDLOG
		};

public:
#ifndef __KERNEL_MODE__

	inline TInt Open()
		{ return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL,NULL); }
	inline TInt SendLog(TLogInfo* aInfo)
		{ return DoControl(ECONTROL_SENDLOG, aInfo); }

#endif 
	};
#endif   //__D_RESMANUSBTRACE_H__
