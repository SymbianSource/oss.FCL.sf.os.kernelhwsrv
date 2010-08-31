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
// e32test\resourceman\t_rescontrolclisync.cpp
// TestCase Description:
// This test harness is to test the unsafe setup of power request call back object
// inside resourcecontrol.cpp. The memeber variable of TPowerRequestCb should not 
// be corrupted when a second ChangeResourceState is execute during the call back
// function is running.
// 
//

#include <e32test.h>
#include <e32hal.h>
#include <e32math.h>
#include <e32def.h>
#include <e32def_private.h>
#include "d_rescontrolclisync.h"



LOCAL_D RTest test(_L("T_RESCONTROLCLISYNC"));
RTestResMan lddChan;
RTestResMan lddChan2;

#define TESTANDCLEAN(x) TestAndClean(x,__LINE__)

void TestAndClean(TBool aTestValid, TInt aLine)
    {
    if(!aTestValid)
        {
        lddChan.DeRegisterClient();
        lddChan2.DeRegisterClient();
        lddChan.Close();
        lddChan2.Close();
        User::FreeLogicalDevice(KLddFileName);
        User::FreePhysicalDevice(KPddFileName);
        RDebug::Printf("Test fail at line %d", aLine);
        test.HandleError(EFalse, aLine, (TText *)__FILE__);
        }
    }

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing Resource Manager...\n"));

	test.Next(_L("Load Physical device"));
	TInt r = User::LoadPhysicalDevice(KPddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);
	test.Next(_L("Load Logical Device"));
	r=User::LoadLogicalDevice(KLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);
    test.Next(_L("Open Logical Channel 1"));	
	r = lddChan.Open();
    test(r==KErrNone);	
    test.Next(_L("Open Logical Channel 2"));        
	r = lddChan2.Open();
	test(r==KErrNone);

    test.Next(_L("Register client 1"));    
	r = lddChan.RegisterClient();
    RDebug::Printf("Register client 1 return r = %d", r);
    TESTANDCLEAN(r==KErrNone);

	test.Next(_L("Register client 2"));    
	r = lddChan2.RegisterClient();
    RDebug::Printf("Register client 2 return r = %d", r);
    TESTANDCLEAN(r==KErrNone);
    
    test.Next(_L("Print resource info"));    
    r = lddChan.PrintResourceInfo();
    RDebug::Printf("Print resource info return r = %d", r);
    TESTANDCLEAN(r==KErrNone);

    TRequestStatus RequestStatus;
    TRequestStatus RequestStatus2;
    
    test.Next(_L("WaitAndChangeResource"));    
    lddChan.WaitAndChangeResource(RequestStatus);

    test.Next(_L("ChangeResourceAndSignal"));    
    lddChan2.ChangeResourceAndSignal(RequestStatus2);

    User::WaitForRequest(RequestStatus);
    TESTANDCLEAN(RequestStatus.Int()==KErrNone);
    User::WaitForRequest(RequestStatus2);
    TESTANDCLEAN(RequestStatus2.Int()==KErrNone);

    test.Next(_L("De-register client 1")); 
	r = lddChan.DeRegisterClient();
    test(r==KErrNone);
    
    test.Next(_L("De-register client 2")); 	
    r = lddChan2.DeRegisterClient();
    test(r==KErrNone);
    
	test.Printf(_L("Closing the channel\n"));
	lddChan.Close();
	lddChan2.Close();
	test.Printf(_L("Freeing logical device\n"));
	r = User::FreeLogicalDevice(KLddFileName);
	test(r==KErrNone);
	r = User::FreePhysicalDevice(KPddFileName);
	test(r==KErrNone);
	test.End();
	test.Close();
	return KErrNone;
	}

