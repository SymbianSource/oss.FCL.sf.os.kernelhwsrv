// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_dlla2.cpp
// 
//

#include <e32svr.h>

IMPORT_C void ExportedFunction();

class TDlla2
	{
public:
	IMPORT_C TDlla2();
	~TDlla2();
public:
	TUint32 iId;
	};

TInt Data2=0xd478acbb;
TDlla2 DllA2;

EXPORT_C TDlla2::TDlla2()
	{
	RDebug::Print(_L("T_DLLA2 Process Attach %08x"),Data2);
	iId=RThread().Id();
	RDebug::Print(_L("T_DLLA2 RThread().Id()=%08x"),iId);
	ExportedFunction();
	}

TDlla2::~TDlla2()
	{
	RDebug::Print(_L("T_DLLA2 Process Detach"));
	ExportedFunction();
	}
