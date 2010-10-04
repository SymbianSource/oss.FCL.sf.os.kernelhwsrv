// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __DOMAIN_POLICY_TEST_H__
#define __DOMAIN_POLICY_TEST_H__

#include <domainpolicy.h>

// Policy Domain Timeout
const TInt KDomainTimeout = 2000000; /* 2000ms */

// The original test domain hierarchy id
static const TDmHierarchyId	KDmHierarchyIdTest = 99;

// The test domain hierarchy Id for V2 policy with V2 features enabled
static const TDmHierarchyId	KDmHierarchyIdTestV2 = 98;

// The original test domain hierarchy id but as a V2 policy with V1 functionality
static const TDmHierarchyId	KDmHierarchyIdTestV2_97 = 97;

// The original test domain hierarchy id but as a V2 policy with V1 functionality, null state spec
static const TDmHierarchyId	KDmHierarchyIdTestV2_96 = 96;

//  The test domain hierarchy Id for V2 policy with V2 features enabled but botched
static const TDmHierarchyId	KDmHierarchyIdTestV2_95 = 95;

//  The test domain hierarchy Id for V2 policy with V2 features enabled but botched
static const TDmHierarchyId	KDmHierarchyIdTestV2_94 = 94;


/*
Domains defined in this hierarchy.
The hierarchy desribed here looks like this:
		Root
		A				B				C
		AA	AB			BA				CA
			ABA	ABB						CAA
			
This hierarchy is used in V1 and V2 policies. 
*/
static const TDmDomainId	KDmIdTestA		= 0x02;
static const TDmDomainId	KDmIdTestB		= 0x03;
static const TDmDomainId	KDmIdTestC		= 0x04;

static const TDmDomainId	KDmIdTestAA		= 0x05;
static const TDmDomainId	KDmIdTestAB		= 0x06;
static const TDmDomainId	KDmIdTestBA		= 0x07;
static const TDmDomainId	KDmIdTestCA		= 0x08;

static const TDmDomainId	KDmIdTestABA	= 0x09;
static const TDmDomainId	KDmIdTestABB	= 0x0A;
static const TDmDomainId	KDmIdTestCAA	= 0x0B;

static const TInt KTestDomains = 0x0B;	// number of domains including root


/*
Simulated system-wide states for test purposes.

The typical state transitions expected in this model:
	EStartupCriticalStatic 	-> EStartupCriticalDynamic
	EStartupCriticalDynamic -> EStartupNonCritical
	EStartupNonCritical 	-> ENormalRunning
	ENormalRunning 			-> EBackupMode
	EBackupMode				-> ENormalRunning
	ENormalRunning			-> EShutdownCritical
	EShutdownCritical 		-> EShutdownNonCritical

However, that does not stop tests from transition from/to any state as required
for test cases. Further states can be added through numbering e.g. EBackupMode1
if required.
*/
enum TSystemState
	{
	/** Device starting up initialising ROM based components/resources */
	EStartupCriticalStatic,

	/** Device continuing start up initisliaing non-ROM based components/resoruces */
	EStartupCriticalDynamic,

	/** Device continues start up initialising non-critical components/resoruces */
	EStartupNonCritical,
	
	/** Device running normally */
	ENormalRunning,
		
	/** Device about to start system wide backup operation */
	EBackupMode,

	/** Device about to start system wide restore operation */
	ERestoreMode,

	/** Device starting shutdown perform critical shutdown actions */
	EShutdownCritical,

	/** Device performing non-critical shutdown actions */
	EShutdownNonCritical,


	/** An integer that is strictly greater than any legal system state value */
	ESystemStateLimit
	};


// Externs for test policy data

extern const TDmDomainSpec DomainHierarchy[];
extern const TDmHierarchyPolicy HierarchyPolicy;

extern const SDmStateSpecV1 StateSpecification[];
extern const TUint StateSpecificationSize;
extern const TInt StateSpecificationVersion;

#endif


