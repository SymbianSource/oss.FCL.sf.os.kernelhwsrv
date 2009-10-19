// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\loader\security\fuzzv1.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <f32file.h>

TUint SomeData = 123;
TUint* APointer = &SomeData;

#ifdef BE_HUGE
const TInt hugetext[32768] = {8};
TInt hugedata[32768] = {5, hugetext[0]};
#endif

extern "C" IMPORT_C TInt SomeExport();
EXPORT_C TInt SomeExport()
	{
	return 7;
	}

extern "C" IMPORT_C TInt AnotherExport();
EXPORT_C TInt AnotherExport()
	{
	return 4;
	}

TInt E32Main()
	{
	User::Version();
	RFs::GetSystemDrive();
	return KErrNone;
	}
