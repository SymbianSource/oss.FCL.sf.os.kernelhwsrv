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
// e32test\dll\t_nestedload.cpp
// Overview:
// Test that nested loading of DLLs does not cause incorrect constructor calls
// API Information:
// n/a
// Details:
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

/*

This EXE links to and dynamically loads seven DLLs, t_nestdll1-7.dll.
The diagram below shows the structure of the linkage. Regular arrows
are normal static linkage, arrows marked with DY are dynamic loads with
RLibrary::Load.

 +-----+      +---+      +---+
 | EXE |----->| 1 |--DY->| 3 |
 +-----+      +---+      +---+\
    | \                        \  EXE and 3 both link to 2
    D  \                        \    +---+
    Y   +------------------------+-->| 2 |
    |                                +---+
    v
  +---+
  | 5 |
  +---+\
    |   \
    D    \
    Y     \
    |      \
    v       \
  +---+      \
  | 6 |       \
  +---+        \
    |           \
    D            \
    Y             \
    |              \
    v               \   5 and 7 both link to 4
  +---+              \    +---+
  | 7 |---------------+-->| 4 |
  +---+                   +---+

The result should be that the constructors are called in order from 1 to 7,
with no constructor being called more than once or omitted.

*/

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32svr.h>

IMPORT_C TInt DataForDLL1();
IMPORT_C TInt DataForDLL2();

TInt E32Main()
	{
	RTest test(_L("T_NESTEDLOAD"));
	test.Title();

	test.Start(_L("Test nested loading"));

	test.Next(_L("Dummy calls to static dependencies"));
	DataForDLL1();
	DataForDLL2();

	test.Next(_L("Load library triggering nested load"));
	RLibrary lib;
	test_KErrNone(lib.Load(_L("t_nestdll5.dll")));

	test.Next(_L("Retrieve load order array"));
	TInt* loadOrder = (TInt*)UserSvr::DllTls(0);
	test_NotNull(loadOrder);

	test.Next(_L("Load order:"));
	TInt i;
	for (i = 0; i < 100; ++i)
		{
		if (loadOrder[i] == 0)
			break;
		test.Printf(_L("%d "), loadOrder[i]);
		}
	test.Printf(_L("\n"));

	// We expect DLLs 1-7 to be loaded in numerical order, with no repeats.
	for (i = 0; i < 7; ++i)
		test_Equal(i+1, loadOrder[i]);
	test_Equal(0, loadOrder[7]);

	test.End();
	return(KErrNone);
	}

