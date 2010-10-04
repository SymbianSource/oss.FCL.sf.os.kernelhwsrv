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
// Provides classes for holding domain policy data.
//
//

#ifndef __DOMAIN_CFG_H__
#define __DOMAIN_CFG_H__

#include <e32base.h>

#include <domainpolicy.h>


/**
The server's representation of per-transition config settings

It is intended to act as a buffer between the server and any
future SDmStateSpecV* structs.

*/
struct TTransitionConfig : public SDmStateSpecV1
	{
	TTransitionConfig(const SDmStateSpecV1& aSpec);
	// This version of the constructor is used as a key when looking up a
	// specific actual config.
	TTransitionConfig(TDmDomainState);
	TInt CheckValues() const;
	};

/**
Repository for a hierachy's settings
*/
class CHierarchySettings : public CBase
	{
public:
	CHierarchySettings();
	~CHierarchySettings();

	void StoreConfigL(const TTransitionConfig& aConfig);

	// Called by CDmHierarchy
	void SetCurrentTargetTransition(TDmDomainState);

	// Called by various objects to retrieve settings
	TBool GetDomainTimeout(TTimeIntervalMicroSeconds32&) const;
	TBool GetDeferralBudget(TInt&) const;
	TBool GetFailurePolicy(TDmTransitionFailurePolicy&) const;

private:
	const TTransitionConfig* LookupConfig(TDmDomainState) const;

	TDmDomainState iCurrentState;

	RArray<const TTransitionConfig> iConfigs;
	};

/**
This class wraps up the fetching of a data type for which
an overridden value may be available from CHierarchySettings

Template parameters:
T - Type of the value which may be over-ridden.
F - Pointer to member function of CHierarchySettings.
	This will be used to query and fetch an over-ride value.
*/
template< typename T, TBool (CHierarchySettings::*F)(T&) const>
class TOverrideableSetting
	{
public:
	/**
	@param aDefault Default value
	@param aSettings The object from which overrides are fetched
	*/
	TOverrideableSetting(T aDefault, const CHierarchySettings* aSettings = NULL)
		: iSettings(aSettings), iDefault(aDefault)
		{}

	void SetSettings(const CHierarchySettings* aSettings)
		{
		iSettings = aSettings;
		}

	/**
	Used to access the wrapped value. It will supply an
	over-ridden value if available, otherwise, the internal
	default vaule.
	*/
	T operator ()() const
		{
		T value;
		const TBool overridden = (iSettings->*F)(value);

		if(!overridden)
			{
			value = iDefault;
			}
		return value;
		}

private:
	const CHierarchySettings* iSettings;
	const T iDefault;
	};
#endif
