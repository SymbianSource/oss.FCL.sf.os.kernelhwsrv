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
// e32test\dll\t_dlla3.cpp
// 
//

#include <e32svr.h>

TInt initialisedData=0x12345678;
TInt moreInitialisedData=0x9abcdef0;

EXPORT_C TInt Function3()
	{
	return 3;
	}

EXPORT_C TInt Function4()
	{
	return 4;
	}

EXPORT_C TInt GetDll3Data()
	{
	return initialisedData;
	}

EXPORT_C void SetDll3Data(TInt aVal)
	{
	initialisedData=aVal;
	}

