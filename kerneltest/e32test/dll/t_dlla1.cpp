// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_dlla1.cpp
// 
//

#include <e32svr.h>

const TInt KAmountOfBss=0x1004;
TInt someBss[KAmountOfBss];

TInt somedata=0;
TFullName ProcessName(RProcess().FullName());
TFullName ThreadName(RThread().FullName());


TInt AnyOldFunction()
	{
	return 0x56789012;
	}
TInt (*function)()=&AnyOldFunction;

EXPORT_C TInt AFunction()
	{
	if ((*function)()!=0x56789012)
		return KErrGeneral;
	TInt i;
	for (i=0; i<KAmountOfBss; i++)
		{
		if (someBss[i]!=0)
			{
//			RDebug::Print(_L("i=%x addr %08x  data %08x"),i,&someBss[i], someBss[i]);
			return KErrArgument;
			}
		}
	return KErrNone;
	}

EXPORT_C TInt Inc()
	{ return ++somedata; }

EXPORT_C TFullName& GetProcessName()
	{
	return ProcessName;
	}

EXPORT_C TFullName& GetThreadName()
	{
	return ThreadName;
	}

