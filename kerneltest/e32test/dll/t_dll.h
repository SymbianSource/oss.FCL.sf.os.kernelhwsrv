// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_dll.h
// 
//

#include <e32std.h>
#include <e32base.h>
#include <e32test.h>
#include <e32svr.h>
#include <e32ver.h>

class TestDll
	{
public:
	IMPORT_C static TInt Data();
	IMPORT_C static void SetData(TInt aValue);
	IMPORT_C static TUint Test1();
	IMPORT_C static TUint Test2();
	IMPORT_C static TUint Server();
	IMPORT_C static RSemaphore Sem();
	};

class TestDll1
	{
public:
	IMPORT_C static TInt Data();
	IMPORT_C static void SetData(TInt aValue);
	IMPORT_C static TUint Test1();
	IMPORT_C static TUint Test2();
	IMPORT_C static TUint Server();
	IMPORT_C static RSemaphore Sem();
	IMPORT_C static TInt Attach(TBool aAttach);
	IMPORT_C static TInt GlobalAlloc(TInt aSize);
	IMPORT_C static TBool GlobalAllocated();
	IMPORT_C static TInt GlobalRead(TInt aPos,TDes8 &aDes);
	IMPORT_C static TInt GlobalWrite(TInt aPos,const TDesC8 &aDes);
	};

class TestDll2
	{
public:
	IMPORT_C static TInt Data();
	IMPORT_C static void SetData(TInt aValue);
	IMPORT_C static TUint Test1();
	IMPORT_C static TUint Test2();
	IMPORT_C static TUint Server();
	IMPORT_C static RSemaphore Sem();
	IMPORT_C static TInt Attach(TBool aAttach);
	IMPORT_C static TInt GlobalAlloc(TInt aSize);
	IMPORT_C static TBool GlobalAllocated();
	IMPORT_C static TInt GlobalRead(TInt aPos,TDes8 &aDes);
	IMPORT_C static TInt GlobalWrite(TInt aPos,const TDesC8 &aDes);
	};

