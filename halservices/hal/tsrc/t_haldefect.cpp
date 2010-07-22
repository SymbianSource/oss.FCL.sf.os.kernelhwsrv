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
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <hal.h>

LOCAL_D RTest test(_L("T_HalDefect Testcases"));


	
void DoTestRC_361803()
	{
	__UHEAP_MARK;
	test.Start(_L("DoTestRC_361803 tests"));

	TInt origValue = 0;
	TInt r = HAL::Get(HALData::EDisplayMemoryHandle, origValue);
	// Standard base emulator PSL without "-1 check", HAL::Get, r==KErrArgument
	// Standard base emulator PSL, no mods, HAL::GetAll, r==KErrNone, origValue==0
	// NaviEngine PSL, no mods, r==KErrNotSupported
	// Platforms that support it: r==KErrNone, origValue==+ve 
	if ((r == KErrNotSupported) || (r == KErrArgument) || 
	    (origValue == 0))						 // Skip test if not supported
		{
		test.Printf(_L("Platform doesn't support EDigitiserOrientation, skipping, (%d, %d)\n"), r, origValue);
		test.End();
		__UHEAP_MARKEND;
		return;
		}
	// Attribute supported
	test_KErrNone(r);
	RHandleBase handle;
	handle.SetHandle(origValue);
	handle.Close();
	test.Printf(_L("Platform DOES support EDigitiserOrientation, handle closed\n"));
			
	HAL::SEntry* pE = 0;
	TInt pC = 0;
	r = HAL::GetAll(pC, pE);
	test_KErrNone(r);
	
	const HAL::SEntry* pS=pE;
	const HAL::SEntry* pEnd=pS + pC;
	TBool displayMemHandleFound = EFalse;
		
    test.Printf(_L("ENumHalAttributes == %d, nEntries == %d\n"), HALData::ENumHalAttributes, pC);
	for (TInt s = 0; pS<pEnd; ++pS, ++s)
		{
		// Following line only needed for development and debug of test case.
		// test.Printf(_L("Attr: %d; Prop: %x; Value: %x\n"), s, pS->iProperties, pS->iValue );
 
		if ((pS->iProperties & HAL::EEntryValid ) &&
		    ((s%HAL::ENumHalAttributes) == HALData::EDisplayMemoryHandle))
		    {		    
		    // Note, GetAll on Emulator PSL will set r==KErrNone and value to 0
		    // So check value to ensure a handle has been allocated.
		    if( pS->iValue >= 0 )
		        {
		        displayMemHandleFound++;
		        RHandleBase handle;
		        handle.SetHandle(pS->iValue);
		        handle.Close();
				break;
		        }
		    }	
		}
	
	test.Printf(_L("HAL::GetAll() DisplayMemHandle should not have been found (0), result == (%d)\n"), displayMemHandleFound );
	test_Equal(displayMemHandleFound, 0);
		
	test.Printf(_L("DoTestRC_361803 - complete\n"));
	test.End();
	__UHEAP_MARKEND;
	}


GLDEF_C TInt E32Main()
    {
	__UHEAP_MARK;
	
 	test.Title();
	test.Start(_L("User-side HAL Defect Test Cases"));
	
	DoTestRC_361803();	// ox1cimx1#361803
	
	test.Printf(_L("\n"));
	test.End();
	test.Close();

	__UHEAP_MARKEND;
    return KErrNone;
    }

