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
// e32test\misc\t_uid.cpp
// Overview:
// Test handling of UIDs (Unique Identifiers). 
// API Information:
// TUid, TUidType, TCheckedUid.
// Details:
// - Assign some globally unique 32-bit numbers with specified values, 
// get standard text form of the UID and check it is as expected.
// - Set the specified Uid type to be packaged and verify
// - validity of UID type. 
// - Uid type contained is as expected.
// - component UIDs are same as specified UID.
// - the most derived UID is as expected.
// - Check the process' Uids are as expected.
// - Load the specified DLL, get Uid of DLL, name of this DLL's file,
// compare the name with a specified text and check it is as expected,
// verify the Uid is as expected.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_UID"));

const TUid g1 = {0x10000001};
const TUid g11 = {0x10000001};
const TUid g2 = {0x10000002};
const TUid g22 = {0x10000002};
const TUid g3 = {0x10000003};

LOCAL_C void testUid()
//
// Test UIDs
//
	{

	test.Start(_L("All functions"));
	test(g1==g11);
	test(g2==g22);
	test(g1!=g2);
	TName a1Name = g1.Name();
	TName a11Name = g11.Name();
	TName a2Name = g2.Name();
	TName a22Name = g22.Name();
	test.Printf(_L("%S %S\n"),&a1Name,&a11Name);
	test.Printf(_L("%S %S\n"),&a2Name,&a22Name);
	test.End();
	}

LOCAL_C void testCheckedUid()
//
// Test checked UIDs
//
	{

	test.Start(_L("All functions"));
	TCheckedUid check1;
	check1.Set(TUidType(g1));
	test(check1.UidType().IsValid()==TRUE);
	test(check1.UidType()[0]==g1);
	test(check1.UidType()[1]==KNullUid);
	test(check1.UidType()[2]==KNullUid);
	test(check1.UidType().MostDerived()==g1);
	test(check1.UidType().IsPresent(g1)==TRUE);
	test(check1.UidType().IsPresent(g2)==FALSE);
	test(check1.UidType().IsPresent(g3)==FALSE);
	TCheckedUid check2;
	check2.Set(TUidType(g1,g2));
	test(check2.UidType().IsValid()==TRUE);
	test(check2.UidType()[0]==g1);
	test(check2.UidType()[1]==g2);
	test(check2.UidType()[2]==KNullUid);
	test(check2.UidType().MostDerived()==g2);
	test(check2.UidType().IsPresent(g1)==TRUE);
	test(check2.UidType().IsPresent(g2)==TRUE);
	test(check2.UidType().IsPresent(g3)==FALSE);
	TCheckedUid check3;
	check3.Set(TUidType(g1,g2,g3));
	test(check3.UidType().IsValid()==TRUE);
	test(check3.UidType()[0]==g1);
	test(check3.UidType()[1]==g2);
	test(check3.UidType()[2]==g3);
	test(check3.UidType().MostDerived()==g3);
	test(check3.UidType().IsPresent(g1)==TRUE);
	test(check3.UidType().IsPresent(g2)==TRUE);
	test(check3.UidType().IsPresent(g3)==TRUE);
	HBufC8* pH=check3.Des().Alloc();
    TUidType t1(g3,check3.UidType()[1],check3.UidType()[2]);
    check3=t1;
	test(check3.UidType().IsValid()==TRUE);
    TUidType t2(g3,g1,check3.UidType()[2]);
    check3=t2;
	test(check3.UidType().IsValid()==TRUE);
    TUidType t3(g3,g1,g2);
    check3=t3;
	test(check3.UidType().IsValid()==TRUE);
	test(check3.UidType()[0]==g3);
	test(check3.UidType()[1]==g1);
	test(check3.UidType()[2]==g2);
	test(check3.UidType().IsPresent(g1)==TRUE);
	test(check3.UidType().IsPresent(g2)==TRUE);
	test(check3.UidType().IsPresent(g3)==TRUE);
	check3.Set(*pH);
	test(check3.UidType().IsValid()==TRUE);
	test(check3.UidType()[0]==g1);
	test(check3.UidType()[1]==g2);
	test(check3.UidType()[2]==g3);
	test(check3.UidType().IsPresent(g1)==TRUE);
	test(check3.UidType().IsPresent(g2)==TRUE);
	test(check3.UidType().IsPresent(g3)==TRUE);
	TCheckedUid check4(*pH);
	delete pH;
	test(check4.UidType().IsValid()==TRUE);
	test(check4.UidType()[0]==g1);
	test(check4.UidType()[1]==g2);
	test(check4.UidType()[2]==g3);
//
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test Uid handling.
//
    {

	test.Title();
//
	test.Start(_L("Uid tests"));
	testUid();
//
	test.Next(_L("Checked Uid tests"));
	testCheckedUid();
//
	test.Next(_L("Check this process's Uids"));
	test(RProcess().Type()[1]==TUid::Uid(0x22222222));
	test(RProcess().Type()[2]==TUid::Uid(0x33333333));

	test.Next(_L("Load Uid DLL"));
	RLibrary lib;
	TInt r=lib.Load(_L("T_DUID.DLL"));
	test(r==KErrNone);
	test.Next(_L("Test FileName"));
	test.Printf(lib.FileName());
	test.Printf(_L("\n"));

#if defined(__WINS__)
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		test(lib.FileName().CompareF(_L("Z:\\Sys\\Bin\\T_DUID.DLL"))==0);
	else
		test(lib.FileName().CompareF(_L("Z:\\System\\Bin\\T_DUID.DLL"))==0);
#else
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		test(lib.FileName().MatchF(_L("?:\\Sys\\Bin\\T_DUID.DLL"))!=KErrNotFound);
	else
		test(lib.FileName().MatchF(_L("?:\\System\\Bin\\T_DUID.DLL"))!=KErrNotFound);
#endif
	test.Next(_L("Check DLL Uid"));
	test(lib.Type()[1]==TUid::Uid(0x12345678));
	test(lib.Type()[2]==TUid::Uid(0x87654321));
	lib.Close();
	test.End();
	return(KErrNone);
	}



