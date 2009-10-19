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
// e32test\earlyextension\d_testearlyextension.h
// 
//

#ifndef __D_TESTEARLYEXTENSION_H__
#define __D_TESTEARLYEXTENSION_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName, "D_TESTEARLYEXTENSION.LDD");

class RLddEarlyExtensionTest : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EGET_SYSTEM_TIME_STAMPS = 0,
		};

public:
	inline TInt Open();
	inline TInt Test_getSystemTimeStamps(Int64& earlyExtTimeStamp, Int64& extTimeStamp);
	};

#ifndef __KERNEL_MODE__
inline TInt RLddEarlyExtensionTest::Open()
	{ return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL, NULL, EOwnerThread); }
inline TInt RLddEarlyExtensionTest::Test_getSystemTimeStamps(Int64& earlyExtTimeStamp, Int64& extTimeStamp)
	{ return DoControl(EGET_SYSTEM_TIME_STAMPS, (TAny *)&earlyExtTimeStamp, (TAny*)&extTimeStamp); }
#endif

#endif //__D_TESTEARLYEXTENSION_H__


