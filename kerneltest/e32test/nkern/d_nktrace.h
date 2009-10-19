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
// e32test\nkern\d_nktrace.h
// 
//

#if !defined(__D_NKTRACE_H__)
#define __D_NKTRACE_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"NKTraceTest");

class TCapsNKTraceTestV01
	{
public:
	TVersion	iVersion;
	};

class RNKTraceTest : public RBusLogicalChannel
	{
public:
	enum TControl
		{
   	    EControlKTrace,
   	    EControlKDebug,
		};

	enum TTest
		{
		ETestKTrace,
		ETestKDebug
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }
	inline TInt KTrace(TInt aTestNum)
		{ return DoControl(EControlKTrace,(TAny*)aTestNum); }
	inline TInt KDebug(TInt aTestNum)
		{ return DoControl(EControlKDebug,(TAny*)aTestNum); }
#endif
	};

#endif
