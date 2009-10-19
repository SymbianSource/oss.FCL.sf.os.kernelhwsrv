// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_start2.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>

_LIT(KPanicCat,"Start2");
#define INIT_VAL	0xddb3d743

#define test(x)	((void)((x)||(User::Panic(KPanicCat,__LINE__),0)))

class TStartTime
	{
public:
	TStartTime();
public:
	TUint32 iStartTime;
	};

TStartTime StartTime;
TUint32 InitData=INIT_VAL;

TStartTime::TStartTime()
	{
	test(InitData==INIT_VAL);
	test(iStartTime==0);
		
	iStartTime=User::TickCount();
	User::After(30000);
	}

EXPORT_C TUint32 GetStartTime2()
	{
	return StartTime.iStartTime;
	}
