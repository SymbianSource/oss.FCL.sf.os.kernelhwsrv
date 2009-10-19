// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// \f32test\loader\t_import_ldd.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include "d_export.h"
#include "d_import.h"

static RTest test(_L("t_import_ldd"));


TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing ldd importing"));
	
	test.Next(_L("Load the exporting ldd"));
	TInt r=User::LoadLogicalDevice(KExportTestLddName);
	test_Equal(KErrNone, r);	// Don't allow KErrAlreadyExists as don't want ldd to be XIP.

	RExportLdd exportLdd;
	test_KErrNone(exportLdd.Open());
	// The exported function multiplies the 2 arguments.
	test_Equal(12, exportLdd.RunExport(3, 4));
	
	test.Next(_L("Load the importing ldd"));
	r=User::LoadLogicalDevice(KImportTestLddName);
	test_Equal(KErrNone, r);	// Don't allow KErrAlreadyExists as don't want ldd to be XIP.


	RImportLdd importLdd;
	test_KErrNone(importLdd.Open());
	// The imported function multiplies the 2 arguments.
	test_Equal(12, importLdd.RunImport(3, 4));
	
	exportLdd.Close();
	importLdd.Close();

	test_KErrNone(User::FreeLogicalDevice(KExportTestLddName));
	test_KErrNone(User::FreeLogicalDevice(KImportTestLddName));

	test.End();
	return KErrNone;
    }


