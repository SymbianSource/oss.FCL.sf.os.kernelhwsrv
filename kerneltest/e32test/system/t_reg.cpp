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
// e32test\system\t_reg.cpp
// Overview:
// Test the machine configuration functions
// API Information:
// User::MachineConfiguration, User::SetMachineConfiguration
// Details:
// - Save machine configuration to different size descriptors and verify
// the results are as expected.
// - Set the machine configuration back to previous configurations, verify
// the results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32hal.h>

LOCAL_D RTest test(_L("T_REG"));


LOCAL_C void testMachineConfiguration()
//
// Test the machine configuration functions
//
	{

	test.Start(_L("Create test environment"));
//	
	test.Next(_L("Save machine configuration to tiny descriptor"));
	TInt x=0;
    TPtr8 mc11((TUint8*)&x,sizeof(x));
	test(User::MachineConfiguration(mc11,x)==KErrArgument);
	test(x!=0);
	test(x<10000);
//
	test.Next(_L("Save machine configuration to just too small descriptor"));
	TUint8* pMC1=(TUint8*)User::Alloc(x);
	test(pMC1!=NULL);
	TInt y;
    TPtr8 mc12(pMC1,x-1);
	test(User::MachineConfiguration(mc12,y)==KErrArgument);
	test(y==x);
//
	test.Next(_L("Save machine configuration properly"));
    TPtr8 mc13(pMC1,x);
	test(User::MachineConfiguration(mc13,y)==KErrNone);
	test(y==x);
//
	test.Next(_L("Save machine configuration to tiny descriptor"));
	TInt z=0;
    TPtr8 mc21((TUint8*)&z,sizeof(z));
	test(User::MachineConfiguration(mc21,z)==KErrArgument);
	test(z!=0);
	test(z<10000);
//
	test.Next(_L("Save machine configuration to just too small descriptor"));
	TUint8* pMC2=(TUint8*)User::Alloc(z);
	test(pMC2!=NULL);
    TPtr8 mc22(pMC2,z-1);
	test(User::MachineConfiguration(mc22,y)==KErrArgument);
	test(y==z);
//
	test.Next(_L("Save machine configuration properly"));
    TPtr8 mc23(pMC2,z);
	test(User::MachineConfiguration(mc23,y)==KErrNone);
	test(y==z);
//
//	__KHEAP_MARK;	// GLD 3/2/97 I don't know why the KHEAP checking is no longer working
	test.Next(_L("Restore first configuration"));
	test(User::SetMachineConfiguration(TPtrC8(pMC1,x))==KErrNone);
//	__KHEAP_CHECK(0);
//
	test.Next(_L("Restore second configuration"));
	test(User::SetMachineConfiguration(TPtrC8(pMC2,z))==KErrNone);
//	__KHEAP_MARKEND;
//
	test.Next(_L("Tidy up"));
	delete pMC1;
	delete pMC2;
	test.End();
	}


GLDEF_C TInt E32Main()
//
// Test the registry functions.
//
    {

	test.Title();
//
	test.Start(_L("Test Machine Configuration"));
	testMachineConfiguration();
//
	test.End();
	return(KErrNone);
    }

