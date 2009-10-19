// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

// The test domain hierarchy id 

static const TDmHierarchyId	KDmHierarchyIdTest = 99;

/*
Domains defined in this hiearchy
The hierarchy desribed here looks like this:
		Root
		A				B				C
		AA	AB			BA				CA
			ABA	ABB						CAA

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
System-wide start-up states

Some of these states may be ommitted depending on the start-up mode.
E.g. The system-starter might choose to omit EStartupNonCritical in "safe" mode
*/
enum TStartupState
	{
	/** In this state, all ROM based (static) components or resources that 
	are critical to the operation of the phone are started */
	EStartupCriticalStatic,

	/** In this state, all non-ROM based (dynamic) components or resources that 
	are critical to the operation of the phone are started */
	EStartupCriticalDynamic,

	/** In this state, all ROM based (static) or non-ROM based (dynamic) 
	components or resources that are not critical to the operation of the phone 
	are started */
	EStartupNonCritical,

	/** An integer that is strictly greater thean any legal start-up state value */
	EStartupLimit
	};

#endif

