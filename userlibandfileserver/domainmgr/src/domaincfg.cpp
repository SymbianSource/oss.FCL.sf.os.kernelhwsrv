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
// Code for managing the Domain manager's configuration settings
//
//

#include "domaincfg.h"
#include "domainsrv.h"

TTransitionConfig::TTransitionConfig(TDmDomainState aState)
	{
	iState = aState;
	iTimeoutMs = -1;
	iDeferralLimit = -1;
	iFailurePolicy = -1;
	}

TTransitionConfig::TTransitionConfig(const SDmStateSpecV1& aSpec)
	{
	iState = aSpec.iState;
	iTimeoutMs  = aSpec.iTimeoutMs;
	iDeferralLimit = aSpec.iDeferralLimit;
	iFailurePolicy = aSpec.iFailurePolicy;
	}

TInt TTransitionConfig::CheckValues() const
	{
	// Do some basic value sanity checking
	if ((iTimeoutMs < 0) ||
		(iDeferralLimit < 0) ||
		(iFailurePolicy < 0))
		{
		__DS_TRACE((_L("DM: Error: Negative value in struct TTransitionConfig")));
		return KDmErrBadDomainSpec;
		}
	return KErrNone;
	}


CHierarchySettings::CHierarchySettings()
	{
	__DS_TRACE((_L("DM: CHierarchySettings::CHierarchySettings @0x%08x"), this));

	iConfigs.SetKeyOffset(_FOFF(TTransitionConfig, iState));
	}

CHierarchySettings::~CHierarchySettings()
	{
	__DS_TRACE((_L("DM: CHierarchySettings::~CHierarchySettings @0x%08x"), this));

	iConfigs.Close();
	}


void CHierarchySettings::StoreConfigL(const TTransitionConfig& aConfig)
	{
	__DS_TRACE((_L("DM: CHierarchySettings::StoreConfigL(): @0x%08x: T %d ms, Def %d, Fail %d, State 0x%x (%d)"),
		this, aConfig.iTimeoutMs, aConfig.iDeferralLimit, aConfig.iFailurePolicy, aConfig.iState, aConfig.iState));
	iConfigs.InsertInUnsignedKeyOrderL(aConfig);
	}

void CHierarchySettings::SetCurrentTargetTransition(TDmDomainState aState)
	{
	__DS_TRACE((_L("DM: CHierarchySettings::SetCurrentTargetTransition @0x%08x: 0x%x (%d)"),
		this, aState, aState));

	iCurrentState = aState;
	}

TBool CHierarchySettings::GetDomainTimeout(TTimeIntervalMicroSeconds32& aTimeout) const
	{
	const TTransitionConfig* cfg = LookupConfig(iCurrentState);

	TBool valueOverriden = EFalse;
	TInt16 unsetValue = 0;
	if(cfg && cfg->iTimeoutMs != unsetValue)
		{
		valueOverriden = ETrue;
		}

	if(valueOverriden)
		{
		// convert ms to microseconds
		aTimeout = 1000 * cfg->iTimeoutMs;
		}
	else
		{
		aTimeout = 1000 * unsetValue;
		}

	__DS_TRACE((_L("DM: CHierarchySettings::GetDomainTimeout @0x%08x: state 0x%x (%d), override = %d, aTimeout = %d us"),
		this, iCurrentState, iCurrentState, valueOverriden, aTimeout.Int()));

	return valueOverriden;
	}

TBool CHierarchySettings::GetDeferralBudget(TInt& aDeferralCount) const
	{
	const TTransitionConfig* cfg = LookupConfig(iCurrentState);

	TBool valueOverriden = EFalse;
	TInt16 unsetValue = 0;
	if(cfg && cfg->iDeferralLimit != unsetValue)
		{
		valueOverriden = ETrue;
		}

	if(valueOverriden)
		{
		aDeferralCount = cfg->iDeferralLimit;
		}
	else
		{
		aDeferralCount = unsetValue;
		}

	__DS_TRACE((_L("DM: CHierarchySettings::GetDeferralBudget @0x%08x: state 0x%x (%d), override = %d, aDeferralCount = %d"),
		this, iCurrentState, iCurrentState, valueOverriden, aDeferralCount));

	return valueOverriden;
	}

TBool CHierarchySettings::GetFailurePolicy(TDmTransitionFailurePolicy& aFailurePolicy) const
	{
	const TTransitionConfig* cfg = LookupConfig(iCurrentState);

	TBool valueOverriden = EFalse;
	TDmTransitionFailurePolicy unsetValue = ETransitionFailureUsePolicyFromOrdinal3;
	if(cfg && (TDmTransitionFailurePolicy)cfg->iFailurePolicy != unsetValue)
		{
		valueOverriden = ETrue;
		}

	if(valueOverriden)
		{
		aFailurePolicy = (TDmTransitionFailurePolicy)cfg->iFailurePolicy;
		}
	else
		{
		aFailurePolicy = unsetValue;
		}

	__DS_TRACE((_L("DM: CHierarchySettings::GetFailurePolicy @0x%08x: state 0x%x (%d), override = %d, aFailurePolicy = %d"),
		this, iCurrentState, iCurrentState, valueOverriden, aFailurePolicy));

	return valueOverriden;
	}

const TTransitionConfig* CHierarchySettings::LookupConfig(TDmDomainState aState) const
	{
	const TTransitionConfig key(aState);
	TInt index = -1;

	TRAPD(r, iConfigs.FindInUnsignedKeyOrderL(key, index));

	if(r == KErrNotFound)
		return NULL;

	return &(iConfigs[index]);
	}



