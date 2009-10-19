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
// e32test\defrag\t_defrag_ref.cpp
// Overview:
// Test application that uses the reference defragmantation driver to invoke
// the top-level defragmentation operations.
// API Information:
// RBusLogicalChannel
// Details:
// - Load and open the logical device driver ("D_DEFRAG_REF.LDD"). Verify
// results.
// - Request that the driver attempt to trigger various defragmentation
// operations. Verify that they complete successfully.
// Platforms/Drives/Compatibility:
// Hardware only. No defrag support on emulator.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include "d_defrag_ref.h"
#include "..\..\mmu\mmudetect.h"

LOCAL_D RTest test(_L("T_DEFRAG_REF"));

const TPtrC KLddFileName=_L("d_defrag_ref.ldd");

GLDEF_C TInt E32Main()
    {
	test.Title();
	if (!HaveMMU())
		{
		test.Printf(_L("This test requires an MMU\n"));
		return KErrNone;
		}

	test.Start(_L("Load test LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	RDefragChannel defrag;
	TRequestStatus req;
	test.Next(_L("Open test LDD"));
	r = defrag.Open();
	if (r == KErrNotSupported)
		{// This system has no defrag support so can't continue
		goto exit;
		}
	test_KErrNone(r);		

	test.Next(_L("Test calls to GeneralDefragRam DFC"));
	test_KErrNone(defrag.GeneralDefragDfc(&req));

	test.Next(_L("Only one defrag operation can be active per channel"));
	test_Equal(KErrInUse, defrag.GeneralDefrag());

	test.Next(_L("Wait for defrag request to complete"));
	User::WaitForRequest(req);
	test_KErrNone(defrag.GeneralDefragDfcComplete());
	
	test.Next(_L("Test calls to GeneralDefragRam"));
	test_KErrNone(defrag.GeneralDefrag());

	test.Next(_L("Test calls to GeneralDefragRamSem"));
	test_KErrNone(defrag.GeneralDefragSem());

	test.Next(_L("Test allocating into the least preferable zone"));
	r = defrag.AllocLowestZone();
	test(r == KErrNone || r == KErrNoMemory);
	if (r == KErrNone)
		{
		test.Next(_L("Test closing the chunk mapped to the least preferable zone"));
		test_KErrNone(defrag.CloseChunk());
		}

	test.Next(_L("Test claiming the least preferable zone"));
	r = defrag.ClaimLowestZone();
	test(r == KErrNone || r == KErrNoMemory);
	if (r == KErrNone)
		{
		test.Next(_L("Test closing the chunk mapped to the least preferable zone"));
		test_KErrNone(defrag.CloseChunk());
		}

exit:
	test.Next(_L("Close test LDD"));
	defrag.Close();
	User::FreeLogicalDevice(KLddFileName);

	test.End();
	return(KErrNone);
    }
