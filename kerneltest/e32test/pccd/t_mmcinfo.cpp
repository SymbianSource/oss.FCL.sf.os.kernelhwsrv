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
// e32test\pccd\t_mmcinfo.cpp
// Display MMC Card register contents
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include "d_mmctest.h"

LOCAL_D	RTest test(_L("MMC INFO TEST"));

RMmcCntrlIf iDriver;
TInt iStack;
TInt iCard;

GLDEF_C TInt E32Main()
/**
 * Test Entry Point for T_MMCINFO.
 * 
 * This test uses the associated driver (D_mmctest) 
 * to gain access to the mmc stack configuration data
 */
    {	
	test.Title();

	test.Start(_L("Load D_MMCTEST Driver"));
	TInt r;	
	r=User::LoadLogicalDevice(_L("D_MMCTEST"));
	if(r==KErrNotFound)
		{
		test.Printf(_L("Test Driver not present on this platform \n"));
		test.End();
		return(0);
		}
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Connect to the test Driver"));
	// Connect to the driver
	iDriver.Close();
    r=iDriver.Open(iStack,iDriver.VersionRequired());
    if(r==KErrNotFound)
        {
        test.Printf(_L("Test not supported on this platform\n"));
        test.End();
        return(0);
        }
    test_KErrNone(r);
    
    // Power the stack down & up to make sure the CardInfo is up to date
    test.Next(_L("Powering Stack"));
    iDriver.Reset();
    iDriver.PwrDownStack();
    TRequestStatus status = KRequestPending;
    iDriver.PwrUpAndInitStack(status);
    User::WaitForRequest(status);    
    r = status.Int();    
    if (r==KErrNotReady)
        {
        test.Printf(_L("Card not found on this platform \n"));
        test.End();
        return(0);
        }        
    test_KErrNone(r);

    test.Next(_L("Retrieve Stack Info"));
    TUint cardsPresentMask;
    r=iDriver.StackInfo(cardsPresentMask);
    test_KErrNone(r);
    
    test.Next(_L("Select Card - 0"));
    iDriver.SelectCard(iCard);
    TMmcCardInfo ci;
    r=iDriver.CardInfo(ci);
    test_KErrNone(r);
            
    test.Next(_L("Print Card CSD & extended CSD registers"));
    status = KRequestPending;
    iDriver.PrintCardRegisters(status);    
    User::WaitForRequest(status);
    r = status.Int();
    test_KErrNone(r);

	test.End();
	return(0);
	}
  
