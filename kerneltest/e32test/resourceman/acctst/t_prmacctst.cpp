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
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include "d_prmacctst.h"

_LIT8(KTestToc, "TOC");
_LIT8(KTestTac1, "TAC1");
_LIT8(KTestTac2, "TAC2");
_LIT8(KTestTac3, "TAC3");

// The number of test values for multi-level resources
static const TInt KTestMultiLevelValues = 10;

// Bit 16 in the resource ID indicates that the resource has dependencies
static const TUint32 KTestIdHasDependencies = 0x00010000;
static const TInt    KTestMaxDependencies = 256;

static const TInt    KTestResourceNotInUse = -1;

// structure for holding ResId and ResPrty
struct SResDepInfo
	{
	TUint iResourceId;
	TUint8 iDependencyPriority;
	};
class CTestPrmAccTst : public CBase
	{
public:
	CTestPrmAccTst();
	~CTestPrmAccTst();
	TInt DoTestPreamble();
	void DoTest();
	void DoTestPostamble();
protected:
	TBool ResourceInterdependency(TInt aResourceIdOrig, TInt aCurrentResourceId, TInt aResourceIdTarget);
	void TestEnumerateResources();
	void TestSingleUserResources(TInt aResourceNo);
	void TestSharedResources(TInt aResourceNo);
	void TestBinaryResources(TInt aResourceNo);
	void TestMultilevelResources(TInt aResourceNo);
	void TestLatency(TInt aResourceNo);
	void TestSense(TInt aResourceNo);
	void TestCustomSense(TInt aResourceNo);
	void TestDefaultPowerResumption();
	void TestDependenciesAreDeclared(TInt aResourceNo);
private:
	enum TBinary
		{
		EBinaryOff = 0,
		EBinaryOn = 1
		};
protected:
	RTest test;
private:
	// The test PRM clients
	RPrmIf iToc;   // Test Observer Client (never owns any resource)
	RPrmIf iTac1;  // Active clients...
	RPrmIf iTac2;
	RPrmIf iTac3;
	RPrmIf iKextc;	// The Kernel Extension Client (owns Single User Resources)
	//
	TBool iIsPrmSupported;
	//
	RBuf8 iResources;
	TUint iNoResources;
	//
	static CTestPrmAccTst* iSingletonInstance;
	};

CTestPrmAccTst::CTestPrmAccTst()
	:test(_L("T_PRMACCTST")),
		iIsPrmSupported(EFalse),
		iNoResources(0)
	{
	}

CTestPrmAccTst::~CTestPrmAccTst()
	{
	iResources.Close();
	TInt r = User::FreeLogicalDevice(KPrmIfLddName);
	test(r==KErrNone);
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
	r = User::FreePhysicalDevice(_L("resourcecontrollerextended.pdd"));
	test(r==KErrNone);
#endif
	User::After(100000);
	}

/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-PRMACCTST-ENUMRES-0559
@SYMTestCaseDesc	Ensure current resource state of PRM is correctly listed
@SYMREQ				REQ7751
@SYMPREQ			PREQ1398
@SYMTestPriority	Critical
@SYMTestActions
	1.	Display all resources and their properties
	2.	Ensure the resource state range is coherent
@SYMTestExpectedResults
	1.	All resources are shown with the correct properties
	2.	Min <= Default <= Max and Min < Max if Negative Sense
		Min >= Default >= Max and Max > Min if Positive Sense
		Binary resources states are '0' or '1'
*/

void CTestPrmAccTst::TestEnumerateResources()
	{
	test.Printf(_L("Id       Resource name                    C LG LS T  U  S DefltLvl MinLevel MaxLevel\n"));
	test.Printf(_L("--------+--------------------------------+-+--+--+--+--+-+--------+--------+--------\n"));
	TInt i;
	for (i = 0; i < (TInt) iNoResources; i++)
		{
		TResInfo& res = *((TResInfo*) iResources.Ptr() + i);

		// Display properties
		test.Printf(_L("%08x "), res.iResourceId);
		test_Compare(res.iResourceName->Length(), <=, KNameMaxLength);
		TBuf<KNameMaxLength> tmp;
		tmp.Copy(*res.iResourceName);
		test.Printf(_L("%-.32S "), &tmp);
		//
		if (res.iClass == TResInfo::EPhysical)
			{
			test.Printf(_L("P "));
			}
		else if (res.iClass == TResInfo::ELogical)
			{
			test.Printf(_L("L "));
			}
		else
			{
			test(EFalse);
			}
		//
		if (res.iLatencyGet == TResInfo::EInstantaneous)
			{
			test.Printf(_L("In "));
			}
		else if (res.iLatencyGet == TResInfo::ELongLatency)
			{
			test.Printf(_L("Lo "));
			}
		else
			{
			test(EFalse);
			}
		//
		if (res.iLatencySet == TResInfo::EInstantaneous)
			{
			test.Printf(_L("In "));
			}
		else if (res.iLatencySet == TResInfo::ELongLatency)
			{
			test.Printf(_L("Lo "));
			}
		else
			{
			test(EFalse);
			}
		//
		if (res.iType == TResInfo::EBinary)
			{
			test.Printf(_L("B  "));
			}
		else if (res.iType == TResInfo::EMultiLevel)
			{
			test.Printf(_L("ML "));
			}
		else if (res.iType == TResInfo::EMultiProperty)
			{
			test.Printf(_L("MP "));
			}
		else
			{
			test(EFalse);
			}
		//
		if (res.iUsage == TResInfo::ESingleUse)
			{
			test.Printf(_L("SU "));
			}
		else if (res.iUsage == TResInfo::EShared)
			{
			test.Printf(_L("Sh "));
			}
		else
			{
			test(EFalse);
			}
		//
		if (res.iSense == TResInfo::EPositive)
			{
			test.Printf(_L("+ "));
			}
		else if (res.iSense == TResInfo::ENegative)
			{
			test.Printf(_L("- "));
			}
		else if (res.iSense == TResInfo::ECustom)
			{
			test.Printf(_L("C "));
			}
		else
			{
			test(EFalse);
			}
		//
		test.Printf(_L("%08x "), res.iDefaultLevel);
		test.Printf(_L("%08x "), res.iMinLevel);
		test.Printf(_L("%08x\n"), res.iMaxLevel);
		
		// Retrieve resource dependencies
		if (res.iResourceId & KTestIdHasDependencies)
			{
			RBuf8 deplist;
			deplist.Create(sizeof(SResDepInfo) * KTestMaxDependencies);
			iToc.GetResourceDependencies(res.iResourceId, deplist);
			TInt j;
			test.Printf(_L("  Direct Dependencies:"));
			SResDepInfo* ptr = (SResDepInfo*)deplist.Ptr();
			for (j = 0; j < (TInt) (deplist.Length() / sizeof(SResDepInfo)); j++, ptr++)
				{
				test.Printf(_L("ResourceId: %08x"), ptr->iResourceId);
				test.Printf(_L("Resource Priority: %08x"), ptr->iDependencyPriority);
				}
			test.Printf(_L("\n"));
			deplist.Close();
			}
		test.Printf(_L("C:Class,P:Physical,L:Logical"));
		test.Printf(_L("LG/LS:Latency Get/Set,In:Instantaneous,Lo:Long Latency"));
		test.Printf(_L("T:Type,MP:Multiproperty,ML:Multilevel,B:Binary"));
		test.Printf(_L("U:Usage,SU:Single-User,Sh:Shared"));
		test.Printf(_L("S:Sense,+:Positive,-:Negative,C:Custom"));

		// Ensure the state range of the resource does not contradict its properties
		if (res.iType == TResInfo::EBinary)
			{
			if (res.iSense == TResInfo::EPositive)
				{
				test(res.iMinLevel == EBinaryOff);
				test(res.iMaxLevel == EBinaryOn);
				}
			else if (res.iSense == TResInfo::ENegative)
				{
				test(res.iMinLevel == EBinaryOn);
				test(res.iMaxLevel == EBinaryOff);
				}
			else if (res.iSense == TResInfo::ECustom)
				{
				test(res.iMinLevel == EBinaryOff || res.iMinLevel == EBinaryOn);
				test(res.iMaxLevel == EBinaryOff || res.iMaxLevel == EBinaryOn);
				test_Compare(res.iMinLevel, !=, res.iMaxLevel);
				}
			test((res.iDefaultLevel == EBinaryOff) || (res.iDefaultLevel == EBinaryOn));
			}
		// Level range must respect resource sense
		if (res.iSense == TResInfo::EPositive)
			{
			test_Compare(res.iMinLevel, <=, res.iMaxLevel);
			test_Compare(res.iMinLevel, <=, res.iDefaultLevel);
			test_Compare(res.iDefaultLevel, <=, res.iMaxLevel);
			test_Compare(res.iMinLevel, <, res.iMaxLevel);
			}
		else if (res.iSense == TResInfo::ENegative)
			{
			test_Compare(res.iMinLevel, >=, res.iMaxLevel);
			test_Compare(res.iMinLevel, >=, res.iDefaultLevel);
			test_Compare(res.iDefaultLevel, >=, res.iMaxLevel);
			test_Compare(res.iMinLevel, >, res.iMaxLevel);
			}
		}
	}

/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-PRMACCTST-SINGLEUSER-0560
@SYMTestCaseDesc	Ensure Single User resources can only be used by one client
					at a time
@SYMREQ				REQ7751
@SYMPREQ			PREQ1398
@SYMTestPriority	High
@SYMTestActions
Pre-condition:
	The resource is not in use
For each Single User resource:
	1.	Register TAC1 & TAC2
	2.	TAC1 changes resource state
	3.	TAC2 changes resource state
	4.	De-register TAC1
	---
	5.	Register TAC1
	6.	TAC2 changes resource state
	7.	TAC1 changes resource state
	8.	De-register TAC1 & TAC2
@SYMTestExpectedResults
	1.	Clients registered
	2.	Resource state changed
	3.	KErrAccessDenied as there is already one registered client
	4.	Client de-registered
	---
	5.	Client registered
	6.	Resource state changed
	7.	KErrAccessDenied as there is already one registered client
	8.	Both clients de-registered
*/

void CTestPrmAccTst::TestSingleUserResources(TInt aResourceNo)
	{
	test.Printf(_L("---Single-User "));
	TResInfo& res = *((TResInfo*) iResources.Ptr() + aResourceNo);
	// Test pre-condition: resource not in use
	TInt levelowner;
	TInt r = iToc.GetLevelOwner(res.iResourceId, levelowner);
	test_KErrNone(r);
	if (levelowner != KTestResourceNotInUse)
		{
		test.Printf(_L("Not tested (Single-User resource already in use)\n"));
		return;
		}
	// Define two test values
	TInt state;
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	//
	TInt tstval1;
	TInt tstval2;
	if (state == res.iMinLevel)
		{
		tstval1 = res.iMaxLevel;
		tstval2 = res.iMinLevel;
		}
	else
		{
		tstval1 = res.iMinLevel;
		tstval2 = res.iMaxLevel;
		}
	// Test starts here
	r = iTac1.Open();
	test_KErrNone(r);
	r = iTac2.Open();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);
	r = iTac2.RegisterClient(KTestTac2);
	test_KErrNone(r);
	//
	r = iTac1.ChangeResourceState(res.iResourceId, tstval1);
	test_KErrNone(r);
	r = iTac1.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval1, state);
	r = iTac2.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval1, state);
	r = iTac2.ChangeResourceState(res.iResourceId, tstval2);
	test_Equal(KErrAccessDenied, r); // TAC2 cannot change the resource state
	//
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);
	r = iTac2.ChangeResourceState(res.iResourceId, tstval2);
	test_KErrNone(r);
	r = iTac1.GetResourceState(res.iResourceId, state);
	test_KErrNone(r); // TAC1 can still access the resource state...
	test_Equal(tstval2, state);
	r = iTac1.ChangeResourceState(res.iResourceId, tstval1);
	test_Equal(KErrAccessDenied, r); // ... but cannot change it
	r = iTac2.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval2, state); // The resource state remains unchanged indeed
	//
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	r = iTac2.DeRegisterClient();
	test_KErrNone(r);
	iTac1.Close();
	iTac2.Close();
	test.Printf(_L("\n"));
	}

/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-PRMACCTST-SHARED-0561
@SYMTestCaseDesc	Ensure a Shared Resources can be changed by several
					clients
@SYMREQ				REQ7751
@SYMPREQ			PREQ1398
@SYMTestPriority	High
@SYMTestActions
Pre-conditions: 
	If in use, resource state is not equal to max level
	Resource is not Custom Sense
For each Shared resource:
	1.	Register TOC, TAC1 & TAC2
	2.	TAC1 changes resource state to level1
	3.	TAC2 changes resource state to level2
	4.	TAC2 changes resource state to level1
	5.	TAC1 changes resource state to level2
	6.	De-register TOC, TAC1 & TAC2
@SYMTestExpectedResults
	1.	Clients registered
	2.	Resource state changed
	3.	Resource state changed
	4.	Resource state changed
	5.	Resource state changed
	6.	Clients de-registered
*/

void CTestPrmAccTst::TestSharedResources(TInt aResourceNo)
	{
	test.Printf(_L("---Shared "));
	TResInfo& res = *((TResInfo*) iResources.Ptr() + aResourceNo);
	// Test pre-conditions
	if (res.iSense == TResInfo::ECustom)
		{
		test.Printf(_L("Not tested (Custom sense resource)\n"));
		return;
		}
	//
	TInt state;
	TInt r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	TInt levelowner;
	r = iToc.GetLevelOwner(res.iResourceId, levelowner);
	test_KErrNone(r);
	//
	if (levelowner != KTestResourceNotInUse && state == res.iMaxLevel)
		{
		test.Printf(_L("Not tested: Resource is already at maximum level\n"));
		return;
		}
	// Define two test values
	TInt tstval1;
	TInt tstval2;
	if (levelowner != KTestResourceNotInUse)
		{
		// Resource already in use
		tstval1 = state;
		tstval2 = res.iMaxLevel;
		}
	else
		{
		// Resource not in use
		tstval1 = res.iMinLevel;
		tstval2 = res.iMaxLevel;
		}
	// Test starts here
	r = iTac1.Open();
	test_KErrNone(r);
	r = iTac2.Open();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);
	r = iTac2.RegisterClient(KTestTac2);
	test_KErrNone(r);
	//
	r = iTac1.ChangeResourceState(res.iResourceId, tstval1);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval1, state);
	//
	r = iTac2.ChangeResourceState(res.iResourceId, tstval2);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval2, state);
	//
	r = iTac2.ChangeResourceState(res.iResourceId, tstval1);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval1, state);
	//
	r = iTac1.ChangeResourceState(res.iResourceId, tstval2);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval2, state);
	//
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	r = iTac2.DeRegisterClient();
	test_KErrNone(r);
	//
	iTac1.Close();
	iTac2.Close();
	test.Printf(_L("\n"));
	}

/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-PRMACCTST-BINARY-0562
@SYMTestCaseDesc	Ensure Binary Resources function as expected
@SYMREQ				REQ7751
@SYMPREQ			PREQ1398
@SYMTestPriority	High
@SYMTestActions
Pre-conditions:
Resource not in use, or...
	If Shared/Positive Sense, current resource must be off
	If Shared/Negative Sense, current resource must be on
Resource is not Custom Sense
For each Binary resource:
	1.	Register TAC1
	2.	Turn resource off and toggle resource state several times
	3.	De-register TAC1
@SYMTestExpectedResults
	1.	Client registered
	2.	Resource state changes as expected
	3.	De-register TAC1
*/

void CTestPrmAccTst::TestBinaryResources(TInt aResourceNo)
	{
	test.Printf(_L("---Binary "));
	TResInfo& res = *((TResInfo*) iResources.Ptr() + aResourceNo);
	// Test pre-conditions
	if (res.iSense == TResInfo::ECustom)
		{
		test.Printf(_L("Not tested (Custom sense resource)\n"));
		return;
		}
	TInt state;
	TInt r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	TInt levelowner;
	r = iToc.GetLevelOwner(res.iResourceId, levelowner);
	test_KErrNone(r);
	//
	if (levelowner != KTestResourceNotInUse)
		{
		if (res.iUsage == TResInfo::ESingleUse)
			{
			test.Printf(_L("Not tested (Single-User resource already in use)\n"));
			return;
			}
		if (res.iSense == TResInfo::EPositive && state == EBinaryOn)
			{
			test.Printf(_L("Not tested (Positive sense resource is already on)\n"));
			return;
			}
		if (res.iSense == TResInfo::ENegative && state == EBinaryOff)
			{
			test.Printf(_L("Not tested (Negative sense resource is already off)\n"));
			return;
			}
		}
	// Test starts here
	r = iTac1.Open();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);

	// Turn Resource off
	r = iTac1.ChangeResourceState(res.iResourceId, EBinaryOff);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(EBinaryOff, state);

	// Turn it on
	r = iTac1.ChangeResourceState(res.iResourceId, EBinaryOn);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(EBinaryOn, state);

	// Turn it off
	r = iTac1.ChangeResourceState(res.iResourceId, EBinaryOff);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(EBinaryOff, state);
	//
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	iTac1.Close();
	test.Printf(_L("\n"));
	}

/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-PRMACCTST-MULTILEVEL-0563
@SYMTestCaseDesc	Ensure Multi-Level Resources function as expected
@SYMREQ				REQ7751
@SYMPREQ			PREQ1398
@SYMTestPriority	High
@SYMTestActions
Pre-condition:
	If in use, resource state is not equal to max level
	If in use, resource is not Single-User
	Resource is not Custom Sense
For each Multi-Level resource:
	1.	Register TOC and TAC1
	2.	Define a number of gradually increasing (or decreasing if negative
		sense resource) test values across the range of valid states. Attempt
		to change the resource state to each of these values.
	3.	De-register TOC and TAC1
@SYMTestExpectedResults
	1.	Clients registered
	2.	Resource state should be changed to the test value if this value is
		accepted by the PSL. If not, the resource state should be changed to
		the minimum valid value that satisfies this requirement.
	3.	Clients de-registered
*/

void CTestPrmAccTst::TestMultilevelResources(TInt aResourceNo)
	{
	test.Printf(_L("---Multi-level "));
	TResInfo& res = *((TResInfo*) iResources.Ptr() + aResourceNo);
	// Test pre-conditions
	if (res.iSense == TResInfo::ECustom)
		{
		test.Printf(_L("Not tested (Custom sense resource)\n"));
		return;
		}
	TInt state;
	TInt r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	TInt levelowner;
	r = iToc.GetLevelOwner(res.iResourceId, levelowner);
	test_KErrNone(r);
	//
	if (levelowner != KTestResourceNotInUse)
		{
		if (state == res.iMaxLevel)
			{
			test.Printf(_L("Not tested (Resource is already at maximum level)\n"));
			return;
			}
		if (res.iUsage == TResInfo::ESingleUse)
			{
			test.Printf(_L("Not tested (Single-User resource already in use)\n"));
			return;
			}
		}
	// Define test values
	TInt tstval[KTestMultiLevelValues];
	TInt i;
	for (i = 0; i < KTestMultiLevelValues; i++)
		{
		if (levelowner == KTestResourceNotInUse)
			{
			// If resource is not in use, we can use the entire state range
			tstval[i] = res.iMinLevel + i * (res.iMaxLevel - res.iMinLevel) / (KTestMultiLevelValues - 1);
			}
		else
			{
			// Or else we are limited to the Current State-MaxLevel range
			tstval[i] = state + i * (res.iMaxLevel - state) / (KTestMultiLevelValues - 1);
			}
		}
	test_Compare(tstval[0], !=, tstval[KTestMultiLevelValues - 1]);
	// Test starts here
	r = iTac1.Open();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);
	//
	for (i = 0; i < KTestMultiLevelValues; i++)
		{
		r = iTac1.ChangeResourceState(res.iResourceId, tstval[i]);
		test_KErrNone(r);
		r = iToc.GetResourceState(res.iResourceId, state);
		test_KErrNone(r);
		// Resource state should be equal to the test value
		// or to the first valid value that satisfies the request
		if (res.iSense == TResInfo::EPositive)
			{
			test_Compare(state, >=, tstval[i]);
			}
		else
			{
			test_Compare(state, <=, tstval[i]);
			}
		test.Printf(_L("."));
		}
	//
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	iTac1.Close();
	test.Printf(_L("\n"));
	}

/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-PRMACCTST-LATENCY-0564
@SYMTestCaseDesc	Ensure instantaneous resource change state instantaneously
@SYMREQ				REQ7751
@SYMPREQ			PREQ1398
@SYMTestPriority	High
@SYMTestActions
Pre-condition:
	If in use, resource is not Single-User
	If in use, resource state is not equal to max level
	If in use, resource is not Custom Sense
For each Multi-Level resource:
	1.	Register TAC1
	2.	TAC1 changes resource state to max value (async)
	3.	Immediately afterwards, TAC1 gets resource state (sync)
	4.	Wait for callback function
	5.	TAC1 changes resource state to min value (async)
	6.	Immediately afterwards, TAC1 gets resource state (sync)
	7.	Wait for callback function
	8.	De-register TAC1
@SYMTestExpectedResults
	1.	Client registered
	2.	No error reported.
	3.	If resource is Instantaneous Set, then the resource state is equal to
		the new state. If Long Latency Set, then the state is either the old or
		the new state.
	4.	Callback function is called.
	5.	No error reported.
	6.	If resource is Instantaneous Set, then the resource state is equal to
		the new state. If Long Latency Set, then the state is either the old or
		the new state.
	7.	Callback function is called.
	8.	Client de-registered.
*/

void CTestPrmAccTst::TestLatency(TInt aResourceNo)
	{
	test.Printf(_L("---Latency "));
	TResInfo& res = *((TResInfo*) iResources.Ptr() + aResourceNo);
	// Test pre-conditions
	if (res.iSense == TResInfo::ECustom)
		{
		test.Printf(_L("Not tested (Custom sense resource)\n"));
		return;
		}
	TInt state;
	TInt r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	TInt levelowner;
	r = iToc.GetLevelOwner(res.iResourceId, levelowner);
	test_KErrNone(r);
	//
	if (levelowner != KTestResourceNotInUse)
		{
		if (state == res.iMaxLevel)
			{
			test.Printf(_L("Not tested (Resource is already at maximum level)\n"));
			return;
			}
		if (res.iUsage == TResInfo::ESingleUse)
			{
			test.Printf(_L("Not tested (Single-User resource already in use)\n"));
			return;
			}
		}
	// Define the two test values
	TInt tstval1;
	TInt tstval2;
	if (levelowner != KTestResourceNotInUse)
		{
		// Resource in use
		tstval1 = res.iMaxLevel;
		tstval2 = state;
		}
	else
		{
		tstval1 = res.iMaxLevel;
		tstval2 = res.iMinLevel;
		}
	// Test starts here
	r = iTac1.Open();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);
	//
	TTestResourceStateBuf buf;
	buf().iResourceId = res.iResourceId;
	buf().iNewState = tstval1;
	TRequestStatus rs;
	TInt tmpstate;
	iTac1.ChangeResourceStateAndGetState(rs, buf, tmpstate);
	User::WaitForRequest(rs);
	test_KErrNone(rs.Int());
	if (res.iLatencySet == TResInfo::EInstantaneous)
		{
		test_Equal(tstval1, tmpstate); // Temp state is equal to the new state
		}
	else if (tmpstate != state) // Temp state is not necessarily equal to the new state
		{
		test_Equal(tstval1, tmpstate);
		}
	TInt newstate;
	r = iToc.GetResourceState(res.iResourceId, newstate);
	test_KErrNone(r);
	test_Equal(tstval1, newstate);
	//
	buf().iNewState = tstval2;
	iTac1.ChangeResourceStateAndGetState(rs, buf, tmpstate);
	User::WaitForRequest(rs);
	test_KErrNone(rs.Int());
	if (res.iLatencySet == TResInfo::EInstantaneous)
		{
		test_Equal(tstval2, tmpstate); // Temp state is equal to the new state
		}
	else if (tmpstate != tstval1) // Temp state is not necessarily equal to the new state
		{
		test_Equal(tstval2, tmpstate);
		}
	r = iToc.GetResourceState(res.iResourceId, newstate);
	test_KErrNone(r);
	test_Equal(tstval2, newstate);
	//
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	iTac1.Close();
	test.Printf(_L("\n"));
	}

/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-PRMACCTST-SENSE-0565
@SYMTestCaseDesc	Ensure Negative and Positive Sense resources behave as
					expected
@SYMREQ				REQ7751
@SYMPREQ			PREQ1398
@SYMTestPriority	High
@SYMTestActions
Pre-condition:
	Resource is Shared
	If in use, resource state is not equal to max level
For each Positive or Negative Sense resource:
	1.	Register TOC, TAC1, TAC2 and TAC3
	2.	Define three distincts test values, s1, s2 and s3, that correspond to
		actual resource states in the PSL. s1 and s3 are at opposite ends of
		the range of states the resource can be set to. s2 is an intermediate
		state. It might not always be possible to find a distinct value for s2
		(if the resource is binary or if its current use is limiting the number
		of states the resource can be set to). In this case, s2 = s1.
	3.	TAC1 sets resource state to s1
	4.	TAC2 sets resource state to s2
	5.	TAC3 sets resource state to s3
	6.	De-register TAC2
	7.	De-register TAC3
	8.	De-register TAC1 and register TAC1, TAC2 and TAC3 again.
	9.	TAC1 sets resource state to s1
	10.	TAC3 sets resource state to s3
	11.	TAC2 sets resource state to s2
	12.	De-register TAC3.
	13.	TAC3 sets resource state to s3
	14.	De-register TOC, TAC1, TAC2 and TAC3
@SYMTestExpectedResults
	1.	Clients registered
	2.	s1 <= s2 < s3
	3.	Resource state set to s1
	4.	Resource state set to s2
	5.	Resource state set to s3
	6.	Resource state remains unchanged (TAC3 owns level)
	7.	Resource state returns to s1 (TAC1 now owns level)
	8.	Clients registered
	9.	Resource state set to s1
	10.	Resource state set to s3
	11.	Resource state remains unchanged (TAC3 owns level)
	12.	Resource state changes to s2 (TAC2 now owns level)
	13.	Resource state set to s3
	14. Clients de-registered
*/

void CTestPrmAccTst::TestSense(TInt aResourceNo)
	{
	test.Printf(_L("---Sense "));
	TResInfo& res = *((TResInfo*) iResources.Ptr() + aResourceNo);
	// Test pre-conditions
	if (res.iUsage != TResInfo::EShared)
		{
		test.Printf(_L("Not tested: Resource is Single-User\n"));
		return;
		}
	//
	TInt state;
	TInt r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	TInt levelowner;
	r = iToc.GetLevelOwner(res.iResourceId, levelowner);
	test_KErrNone(r);
	//
	if (levelowner != KTestResourceNotInUse && state == res.iMaxLevel)
		{
		test.Printf(_L("Not tested  (Resource is already at maximum level)\n"));
		return;
		}
	// Define three test values
	r = iTac1.Open();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);
	TInt tstval1;
	TInt tstval2;
	TInt tstval3;
	if (levelowner == KTestResourceNotInUse)
		{
		tstval1 = res.iMinLevel;
		}
	else
		{
		tstval1 = state;
		}
	tstval3 = res.iMaxLevel;
	// Attempt to find a distinct intermediate value by dichotomy
	tstval2 = tstval3;
	while ((tstval2 - tstval1 < -1) || (tstval2 - tstval1 > 1))
		{
		tstval2 -= (tstval2 - tstval1 + (res.iSense == TResInfo::EPositive ? 1 : 0)) / 2;
		r = iTac1.ChangeResourceState(res.iResourceId, tstval2);
		test_KErrNone(r);
		r = iTac1.GetResourceState(res.iResourceId, state);
		test_KErrNone(r);
		}
	if (state == tstval1 && res.iType == TResInfo::EMultiLevel)
		{
		test.Printf(_L("(Could not find three distincts test values)"));
		}
	tstval2 = state;
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	iTac1.Close();
	
	// Test starts here
	r = iTac1.Open();
	test_KErrNone(r);
	r = iTac2.Open();
	test_KErrNone(r);
	r = iTac3.Open();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);
	r = iTac2.RegisterClient(KTestTac2);
	test_KErrNone(r);
	r = iTac3.RegisterClient(KTestTac3);
	test_KErrNone(r);

	// Set resource state to the first test value
	r = iTac1.ChangeResourceState(res.iResourceId, tstval1);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval1, state);

	// Set resource state to middle test value
	r = iTac2.ChangeResourceState(res.iResourceId, tstval2);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval2, state);

	// Set resource to the third test value
	r = iTac3.ChangeResourceState(res.iResourceId, tstval3);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval3, state);

	// De-register second client
	r = iTac2.DeRegisterClient();
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval3, state); // state remains  unchanged

	// De-register third client
	r = iTac3.DeRegisterClient();
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval1, state); // state changes

	// De-register and register all clients again
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);
	r = iTac2.RegisterClient(KTestTac2);
	test_KErrNone(r);
	r = iTac3.RegisterClient(KTestTac3);
	test_KErrNone(r);

	// Set resource state to the first test value
	r = iTac1.ChangeResourceState(res.iResourceId, tstval1);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval1, state);

	// Set resource to the third test value
	r = iTac3.ChangeResourceState(res.iResourceId, tstval3);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval3, state);

	// Set resource state to middle test value
	r = iTac2.ChangeResourceState(res.iResourceId, tstval2);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval3, state); // state does not change

	// De-register client holding resource
	r = iTac3.DeRegisterClient();
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval2, state);

	// Register client again
	r = iTac3.RegisterClient(KTestTac3);
	test_KErrNone(r);
	r = iTac3.ChangeResourceState(res.iResourceId, tstval3);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval3, state);

	//
	r = iTac3.DeRegisterClient();
	test_KErrNone(r);
	r = iTac2.DeRegisterClient();
	test_KErrNone(r);
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	iTac3.Close();
	iTac2.Close();
	iTac1.Close();
	test.Printf(_L("\n"));
	}

/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-PRMACCTST-CUSTOMSENSE-0566
@SYMTestCaseDesc	Ensure the Custom Sense function is implemented
@SYMREQ				REQ7751
@SYMPREQ			PREQ1398
@SYMTestPriority	High
@SYMTestActions
For each Custom Sense resource:
	1.	Register TAC1
	2.	Attempt to set the state of the resource to its current state. The
		purpose of this test action is to ensure that the PSL's custom function
		for this resource has been implemented. If the custom function is not
		present, the PIL will generate a panic.
	3.	De-register TAC1
@SYMTestExpectedResults
	1.	Client registered
	2.	The resource change request does not cause a panic.
	3.	Client de-registered
*/

void CTestPrmAccTst::TestCustomSense(TInt aResourceNo)
	{
	test.Printf(_L("---Custom Sense"));
	TResInfo& res = *((TResInfo*) iResources.Ptr() + aResourceNo);
	TInt r = iTac1.Open();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);
	//
	TInt state;
	r = iTac1.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	r = iTac1.ChangeResourceState(res.iResourceId, state);
	test_KErrNone(r);
	//
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	iTac1.Close();
	test.Printf(_L("\n"));
	}

/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-PRMACCTST-POWERRESUMPTION-0567
@SYMTestCaseDesc	Ensure resources go back to their default state when not in
					use
@SYMREQ				REQ7751
@SYMPREQ			PREQ1398
@SYMTestPriority	High
@SYMTestActions
Pre-condition:
	Resource not in use
	Not a Custom Sense resource
For each resource:
	1.	Register TAC1
	2.	Set resource state to something that is not the default level
	3.	De-register TAC1
	4.	Register TOC
	5.	Get resource state
	6.	De-register TOC
@SYMTestExpectedResults
	1.	Client registered
	2.	Resource state changed
	3.	Client de-registered
	4.	Client registered
	5.	Resource state is at the default level
	6.	Client de-registered
*/

void CTestPrmAccTst::TestDefaultPowerResumption()
	{
	TInt i;
	for (i = 0; i < (TInt) iNoResources; i++)
		{
		TResInfo& res = *((TResInfo*) iResources.Ptr() + i);
		test.Printf(_L("Resource %08x "), res.iResourceId);
		TInt levelowner;
		TInt r = iToc.GetLevelOwner(res.iResourceId, levelowner);
		test_KErrNone(r);

		// Test pre-condition: resource not in use
		if (levelowner != KTestResourceNotInUse)
			{
			test.Printf(_L("Not tested (already in use)\n"));
			continue;
			}
		if (res.iSense == TResInfo::ECustom)
			{
			test.Printf(_L("Not tested (custom sense)\n"));
			continue;
			}
		
		// Define a test value
		TInt tstval;
		if (res.iDefaultLevel == res.iMaxLevel)
			{
			tstval = res.iMinLevel;
			}
		else
			{
			tstval = res.iMaxLevel;
			}
		
		// Test starts here
		r = iTac1.Open();
		test_KErrNone(r);
		r = iTac1.RegisterClient(KTestTac1);
		test_KErrNone(r);
		// Change resource state
		TInt state;
		r = iToc.GetResourceState(res.iResourceId, state);
		test_KErrNone(r);
		test_Equal(res.iDefaultLevel, state);
		r = iTac1.ChangeResourceState(res.iResourceId, tstval);
		test_KErrNone(r);
		r = iToc.GetResourceState(res.iResourceId, state);
		test_KErrNone(r);
		test_Equal(tstval, state);
		// De-register active client so resource is freed
		r = iTac1.DeRegisterClient();
		test_KErrNone(r);
		iTac1.Close();
		// Read both cached and actual values
		TInt cached;
		r = iToc.GetResourceStateCached(res.iResourceId, cached);
		test_KErrNone(r);
		r = iToc.GetResourceState(res.iResourceId, state);
		test_KErrNone(r);
		test_Equal(cached, state);
		test_Equal(res.iDefaultLevel, state);
		test.Printf(_L("\n"));
		}
	//
	test.Printf(_L("\n"));
	}

/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-PRMACCTST-DEPENDENCIESDECLARED-0568
@SYMTestCaseDesc	Ensure all actual resources dependencies have been declared
@SYMREQ				REQ7751
@SYMPREQ			PREQ1398
@SYMTestPriority	High
@SYMTestActions
Pre-condition:
	Resource not in use
	Not a Custom Sense resource
For each resource:
	1.	Register TOC & TAC1
	2.	Get state of all resources
	3.	Set the state of the resource under test state to something that is not
		the default level
	4.	Get state of all resource
	5.	De-register TAC1 & TOC
@SYMTestExpectedResults
	1.	Clients registered
	2.	None
	3.	Resource state changed
	4.	Only resources with a direct or indirect dependency on the resource
		under test have changed.
	5.	Client de-registered
*/

TBool CTestPrmAccTst::ResourceInterdependency(TInt aResourceIdOrig, TInt aCurrentResourceId, TInt aResourceIdTarget)
	{
	// Get Reference on current resource node
	TBool r = EFalse;
	if (aCurrentResourceId & KTestIdHasDependencies)
		{
		// Get dependencies of current resources
		RBuf8 deplist;
		deplist.Create(sizeof(SResDepInfo) * KTestMaxDependencies);
		r = iToc.GetResourceDependencies(aCurrentResourceId, deplist);
		if (r)
			{
			test.Printf(_L("ERROR: aCurrentResourceId==%08x"), aCurrentResourceId);
			test_KErrNone(r);
			}
		TInt i;
		SResDepInfo *ptr = (SResDepInfo*)deplist.Ptr();
		for (i = 0; i < (TInt) (deplist.Length() / sizeof(SResDepInfo)); i++, ptr++)
			{
			TInt depid = ptr->iResourceId;
			if (depid == aResourceIdTarget)
				{
				// We've got a match
				r = ETrue;
				}
			else if (depid == aResourceIdOrig)
				{
				// No going back to parent node
				continue;
				}
			else
				{
				// Recurse down the tree
				r = ResourceInterdependency(aCurrentResourceId, depid, aResourceIdTarget);
				}
			if (r)
				{
				break;
				}
			}
		deplist.Close();
		}
	return r;
	}

void CTestPrmAccTst::TestDependenciesAreDeclared(TInt aResourceNo)
	{
	test.Printf(_L("---Test all dependencies are declared\n"));
	TResInfo& res = *((TResInfo*) iResources.Ptr() + aResourceNo);
	//
	TInt levelowner;
	TInt r = iToc.GetLevelOwner(res.iResourceId, levelowner);
	test_KErrNone(r);
	// Test pre-condition: resource not in use
	if (levelowner != KTestResourceNotInUse)
		{
		test.Printf(_L("Not tested (already in use)\n"));
		return;
		}
	if (res.iSense == TResInfo::ECustom)
		{
		test.Printf(_L("Not tested (custom sense)\n"));
		return;
		}
	// Define a test value
	TInt tstval;
	if (res.iDefaultLevel == res.iMaxLevel)
		{
		tstval = res.iMinLevel;
		}
	else
		{
		tstval = res.iMaxLevel;
		}

	// Test starts here
	// Save current state of all resources;
	RArray<TInt> oldstate;
	TInt i;
	TInt state;
	for (i = 0; i < (TInt) iNoResources; i++)
		{
		TResInfo& tmpres = *((TResInfo*) iResources.Ptr() + i);
		r = iToc.GetResourceState(tmpres.iResourceId, state);
		test_KErrNone(r);
		r = oldstate.Append(state);
		test_KErrNone(r);
		}
	//
	r = iTac1.Open();
	test_KErrNone(r);
	r = iTac1.RegisterClient(KTestTac1);
	test_KErrNone(r);

	// Change resource state
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(res.iDefaultLevel, state);
	r = iTac1.ChangeResourceState(res.iResourceId, tstval);
	test_KErrNone(r);
	r = iToc.GetResourceState(res.iResourceId, state);
	test_KErrNone(r);
	test_Equal(tstval, state);

	// Now we check that resources with no dependency on resource under test
	// have not changed
	for (i = 0; i < (TInt) iNoResources; i++)
		{
		test.Printf(_L("."));
		if (i == aResourceNo)
			{
			continue;
			}
		TResInfo& tmpres = *((TResInfo*) iResources.Ptr() + i);
		r = iToc.GetResourceState(tmpres.iResourceId, state);
		test_KErrNone(r);
		if (!ResourceInterdependency(0, res.iResourceId, tmpres.iResourceId) &&
			(state != oldstate[i]))
			{
			test.Printf(_L("Resource %08x has changed!\n"), tmpres.iResourceId);
			test_Equal(state, oldstate[i]);
			}	
		}
	test.Printf(_L("\n"));

	// De-register active client
	r = iTac1.DeRegisterClient();
	test_KErrNone(r);
	iTac1.Close();
	//
	oldstate.Close();
	}

TInt CTestPrmAccTst::DoTestPreamble()
	{
	test.Title();
	test.Start(_L("Preamble"));

	// Load Device Driver
	TInt r;
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
	test.Printf(_L("Load Simulated PSL PDD\n"));
	r = User::LoadPhysicalDevice(_L("RESOURCECONTROLLEREXTENDED.PDD"));
	if (r == KErrNotFound)
		{
		test.Printf(_L("RESOURCECONTROLLEREXTENDED.PDD not found\nTest skipped\n"));
		return r;
		}
	if (r)
		{
		test_Equal(KErrAlreadyExists, r);
		test.Printf(_L("Already loaded\n"));
		}
	test.Printf(_L("Load test LDD\n"));
	r = User::LoadLogicalDevice(_L("D_PRMACCTSTSIM.LDD"));
	if (r)
		{
		test_Equal(KErrAlreadyExists, r);
		test.Printf(_L("Already loaded\n"));
		}
#else
	test.Printf(_L("Load test LDD\n"));
	r = User::LoadLogicalDevice(_L("D_PRMACCTST.LDD"));
	if (r)
		{
		test_Equal(KErrAlreadyExists, r);
		test.Printf(_L("Already loaded\n"));
		}
#endif // RESOURCE_MANAGER_SIMULATED_PSL

	// Check if Device Driver was loaded as a Kernel Extension
	iKextc.Open();
	TUint clientId;
	r = iKextc.GetKernelExtClientId(clientId);
	test_KErrNone(r);
	if (!clientId)
		{
		test.Printf(_L("No Kernel extension PRM client\n"));
		iKextc.Close();
		}
	else
		{
		// De-register client so Single User resources can be tested
		r = iKextc.OpenClient(clientId);
		test_KErrNone(r);
		r = iKextc.DeRegisterClient();
		test_KErrNone(r);
		iKextc.Close();
		}

	// Register TOC (always on - de-registered in test postamble)
	r = iToc.Open();
	test_KErrNone(r);
	r = iToc.RegisterClient(KTestToc);
	test_KErrNone(r);

	// Get resources info
	r = iToc.GetTotalNumberOfResources(iNoResources);
	test_KErrNone(r);
	test.Printf(_L("Resources found: %d\n"), iNoResources);
	if (iNoResources > 0)
		{
		r = iResources.Create(iNoResources * sizeof(TResInfo));
		test_KErrNone(r);
		test.Printf(_L("Retrieving information on resources\n"));
		r = iToc.GetInfoOnResourcesInUseByClient(0, iResources);
		test_KErrNone(r);
		iIsPrmSupported = ETrue;
		}
	else
		{
		test.Printf(_L("No resource found.\n"));
		}
	return KErrNone;
	}

void CTestPrmAccTst::DoTest()
	{
	if (!iIsPrmSupported)
		{
		return;
		}
	//
	test.Next(_L("Enumerate Resources"));
	TestEnumerateResources();
	//
	TInt i;
	test.Next(_L("Resource Properties"));
	for (i = 0; i < (TInt) iNoResources; i++)
		{
		TResInfo& res = *((TResInfo*) iResources.Ptr() + i);
		test.Printf(_L("+++Resource %08x\n"), res.iResourceId);
		//
		if (res.iUsage == TResInfo::ESingleUse)
			{
			TestSingleUserResources(i);
			}
		else if (res.iUsage == TResInfo::EShared)
			{
			TestSharedResources(i);
			}
		//
		if (res.iType == TResInfo::EBinary)
			{
			TestBinaryResources(i);
			}
		else if (res.iType == TResInfo::EMultiLevel)
			{
			TestMultilevelResources(i);
			}
		//
		TestLatency(i);
		//
		if ((res.iSense == TResInfo::EPositive) || (res.iSense == TResInfo::ENegative))
			{
			TestSense(i);
			}
		else if (res.iSense == TResInfo::ECustom)
			{
			TestCustomSense(i);
			}
		}
	//
	test.Next(_L("Default Power Resumption"));
	TestDefaultPowerResumption();
	//
	test.Next(_L("PRM Extension - Dependencies"));
	for (i = 0; i < (TInt) iNoResources; i++)
		{
		TResInfo& res = *((TResInfo*) iResources.Ptr() + i);
		if (res.iResourceId & KTestIdHasDependencies)
			{
			test.Printf(_L("+++Resource %08x\n"), res.iResourceId);
			TestDependenciesAreDeclared(i);
			}
		}
	}

void CTestPrmAccTst::DoTestPostamble()
	{
	test.Next(_L("Postamble"));
	if (iToc.Handle())
		{
		TInt r = iToc.DeRegisterClient();
		test_KErrNone(r);
		iToc.Close();
		}
	test.End();
	test.Close();
	}

GLDEF_C TInt MainL()
	{
	CTestPrmAccTst* testapp;
	testapp = new (ELeave) CTestPrmAccTst();
	CleanupStack::PushL(testapp);
	if (testapp->DoTestPreamble() == KErrNone)
		{
		testapp->DoTest();
		}
	testapp->DoTestPostamble();
	CleanupStack::PopAndDestroy(testapp);
	return KErrNone;
	}

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TRAP_IGNORE(MainL());
	delete cleanup;
	__UHEAP_MARKEND;
	return KErrNone;
	}
