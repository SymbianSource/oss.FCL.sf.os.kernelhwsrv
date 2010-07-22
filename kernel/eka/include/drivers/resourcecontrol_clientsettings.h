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
// e32\include\drivers\resourcecontrol_clientsettings.h
//
//

#ifndef __RESOURCECONTROL_CLIENTSETTINGS_H__
#define __RESOURCECONTROL_CLIENTSETTINGS_H__

#include <drivers/hcr.h>

const TUint KBitPerClientSettings = 5; // 0 - 31, Max of 32 elements defined for use per client
const TUint KClientElementIdBase = 0x10;	// Max of 16 elements defined for use by resman
const TUint8 KStaticResourceTableSize = 5;

inline TUint ElementId_ClientSettingBase(TUint aClientToken)
	{
	return	((aClientToken << KBitPerClientSettings) + KClientElementIdBase);
	}

inline HCR::TElementId ElementId_ClientName(TUint aClientToken)
	{
	return ((HCR::TElementId) ElementId_ClientSettingBase(aClientToken));
	}

inline HCR::TElementId ElementId_ClientPropertyFlag(TUint aClientToken)
	{
	return ((HCR::TElementId) (ElementId_ClientSettingBase(aClientToken) + 1));
	}

inline HCR::TElementId ElementId_ClientPreallocation(TUint aClientToken)
	{
	return ((HCR::TElementId) (ElementId_ClientSettingBase(aClientToken) + 2));
	}

inline HCR::TElementId ElementId_ClientStaticResource(TUint aClientToken, TUint aResource)
	{
	return ((HCR::TElementId) (ElementId_ClientSettingBase(aClientToken) + 3 + aResource));
	}

inline HCR::TElementId ElementId_ClientDynamicResource(TUint aClientToken, TUint aResource)
	{
	return ((HCR::TElementId) (ElementId_ClientSettingBase(aClientToken) + 3 + KStaticResourceTableSize + aResource));
	}


// Dynamic Resource Settings

const TUint KDynamicResourceElementIdBase = 0x20000;
const TUint KBitPerDynamicResourceSettings = 5; // 0 - 31, Max of 32 elements defined for use per dynamic resource

inline TUint ElementId_DynamicResourceBase(TUint aDynamicResource)
	{
	return ((aDynamicResource << KBitPerDynamicResourceSettings) + KDynamicResourceElementIdBase);
	}

inline HCR::TElementId ElementId_DynamicResourceName(TUint aDynamicResource)
	{
	return ((HCR::TElementId) (ElementId_DynamicResourceBase(aDynamicResource)));
	}

inline HCR::TElementId ElementId_DynamicResourcePropertyFlag(TUint aDynamicResource)
	{
	return ((HCR::TElementId) (ElementId_DynamicResourceBase(aDynamicResource) + 1));
	}

inline HCR::TElementId ElementId_DynamicResourceMaxLevel(TUint aDynamicResource)
	{
	return ((HCR::TElementId) (ElementId_DynamicResourceBase(aDynamicResource) + 2));
	}

inline HCR::TElementId ElementId_DynamicResourceMinLevel(TUint aDynamicResource)
	{
	return ((HCR::TElementId) (ElementId_DynamicResourceBase(aDynamicResource) + 3));
	}

inline HCR::TElementId ElementId_DynamicResourceDefaultLevel(TUint aDynamicResource)
	{
	return ((HCR::TElementId) (ElementId_DynamicResourceBase(aDynamicResource) + 4));
	}

inline HCR::TElementId ElementId_DynamicResourceDependencyMask1(TUint aDynamicResource)
	{
	return ((HCR::TElementId) (ElementId_DynamicResourceBase(aDynamicResource) + 5));
	}

inline HCR::TElementId ElementId_DynamicResourceDependencyMask2(TUint aDynamicResource)
	{
	return ((HCR::TElementId) (ElementId_DynamicResourceBase(aDynamicResource) + 6));
	}

inline HCR::TElementId ElementId_DynamicResourceDependencyMask3(TUint aDynamicResource)
	{
	return ((HCR::TElementId) (ElementId_DynamicResourceBase(aDynamicResource) + 7));
	}

#endif

