// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Overview:
// Test the RProperty and RPropertyRef classes
// API Information:
// RProperty, RPropertyRef
// Details:
// - Test and verify the RProperty methods: Delete, Define, Attach and 
// Subscribe work as expected.
// - Test the RPropery::Define() method. 
// Define the attributes and access control for a property. Verify results 
// are as expected. Verify property redefinition does not change security 
// settings. Perform various additional tests associated with the Define 
// method. Verify results are as expected.
// - Test the RPropery::Delete() method. 
// Verify that if the property has not been defined, Delete fails with 
// KErrNotFound. Verify pending subscriptions will be completed with 
// KErrNotFound if the property is deleted. Verify new requests will not 
// complete until the property is defined and published again.
// - Test the RPropery::Get() and Set() methods. 
// Verify failure if the property has not been defined, can set property
// to zero length, failure if the property is larger than KMaxPropertySize.
// Verify operation mismatch with property type fails as expected. Verify
// various get/set operations work as expected.
// - Test the RPropery::Subscribe() and Cancel() methods. 
// Verify failure if the property has not been defined, request will not 
// complete. Cancel outstanding subscription request, verify results are as
// expected.
// - Test the platform security settings, verify results are as expected. 
// Verify that RProperty::Delete() can only succeed when called by the 
// property owner. Verify Define, Subscribe, Get caller must have read 
// capabilities or error as expected. Verify Set caller must have write 
// capabilities or error as expected.
// - Perform multiple Set, Get, Subscribe and Cancel operations between 
// multiple threads, verify results are as expected.
// - Create multiple slave threads, verify Set, Get, Subscribe and Cancel 
// operations work as expected.
// - Using a loadable device driver, test the basic functionality of an 
// RPropertyRef object. Perform many of the same tests as above except 
// on a RPropertyRef object. Verify results are as expected.
// - Perform all the above multithreaded tests in parallel. Verify results
// are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include "t_property.h"

static const TInt32 KUidPropTestCategoryValue = 0x101f75b7;
static const TUid KPropTestCategory = { KUidPropTestCategoryValue };

GLDEF_C TInt E32Main()
	{
	TInt iter = 10000;

	TInt len = User::CommandLineLength();
	if (len)
		{
		//
		// Copy the command line in a buffer
		//
		HBufC* hb = HBufC::NewMax(len);
		__ASSERT_ALWAYS(hb, User::Panic(_L("t_property: no memory"), 0));
		TPtr cmd((TUint16*) hb->Ptr(), len);
		User::CommandLine(cmd);

		TLex l(cmd);
		TInt r = l.Val(iter);
		__ASSERT_ALWAYS((r == KErrNone), User::Panic(_L("Usage: t_property <iteration number>\n"), 0));
		delete hb;
		}
	
	CTestProgram::Start();

	CTestProgram* panic = new CPropPanic(KPropTestCategory, 01);
	panic->Launch(1);
	delete panic;

		{
		// Tests to be executed in order
		CTestProgram* progs[] = 
			{
			new CPropDefine(KPropTestCategory, 11, RProperty::EInt),
			new CPropDefine(KPropTestCategory, 22, RProperty::EByteArray),
			new CPropDefine(KPropTestCategory, 28, RProperty::ELargeByteArray),
			new CPropDelete(KPropTestCategory, 33, RProperty::EInt),
			new CPropDelete(KPropTestCategory, 44, RProperty::EByteArray),
			new CPropDelete(KPropTestCategory, 50, RProperty::ELargeByteArray),
			new CPropSetGet(KPropTestCategory, 55, RProperty::EInt),
			new CPropSetGet(KPropTestCategory, 66, RProperty::EByteArray),
			new CPropSetGet(KPropTestCategory, 78, RProperty::ELargeByteArray),
			new CPropSubsCancel(KPropTestCategory, 73, RProperty::ELargeByteArray),
			new CPropSubsCancel(KPropTestCategory, 77, RProperty::EByteArray),
			new CPropSubsCancel(KPropTestCategory, 88, RProperty::EInt),
			new CPropSecurity(KPropTestCategory, 99, RProperty::EInt, 1000),
			new CPropSetGetRace(KPropTestCategory, 111),
			new CPropCancelRace(KPropTestCategory, 122),
			new CPropBroadcast(KPropTestCategory, 133, 2000, 8, 0),
			new CPropBroadcast(KPropTestCategory, 144, 3000, 8, 4),
			new CPropBroadcast(KPropTestCategory, 155, 4000, 8, 8),
			new CPropLddClient(KPropTestCategory, 166, RProperty::EInt),
			new CPropLddClient(KPropTestCategory, 177, RProperty::EByteArray),
			new CPropLddClient(KPropTestCategory, 188, RProperty::ELargeByteArray),

			NULL 
		};

		TInt i;
		TInt n = (sizeof(progs)/sizeof(*progs)) - 1;

		for (i = 0; i < n; ++i)
			{
			__ASSERT_ALWAYS(progs[i], User::Panic(_L("t_property: no memory"), 0));
			}

		// 2 - just to be sure that we can execute the program twice
		CTestProgram::LaunchGroup(progs, 2);

		for (i = 0; i < n; ++i)
			{
			delete progs[i];
			}
		}

		{
		// Tests to be executed in parallel
		CTestProgram* progs[] = 
			{
			new CPropSetGetRace(KPropTestCategory, 111),
			new CPropCancelRace(KPropTestCategory, 122),
			new CPropBroadcast(KPropTestCategory, 133, 2000, 8, 0),
			new CPropBroadcast(KPropTestCategory, 144, 3000, 8, 4),
			new CPropBroadcast(KPropTestCategory, 155, 4000, 8, 8),

			NULL 
			};

		TInt i;
		TInt n = (sizeof(progs)/sizeof(*progs)) - 1;
		for (i = 0; i < n; ++i)
			{
			__ASSERT_ALWAYS(progs[i], User::Panic(_L("t_property: no memory"), 0));
			}
		
		CTestProgram::SpawnGroup(progs, iter);

		for (i = 0; i < n; ++i)
			{
			delete progs[i];
			}
		}

	CTestProgram::End();

	return KErrNone;
	}
