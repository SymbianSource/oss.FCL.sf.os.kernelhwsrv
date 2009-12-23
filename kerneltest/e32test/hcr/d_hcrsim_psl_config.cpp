/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#include "hcr_hai.h"
#include "hcr_uids.h"
using namespace HCR;
#include "d_hcrsim_testdata.h"

// Test Repository
SRepositoryBase RepositoryHeader =
	{
	HCR_FINGER_PRINT, 
	EReposCompiled, 
	KRepositoryFirstVersion,
	EReposReadOnly,
	(sizeof(SettingsList) / sizeof(SSettingC))
	};

SRepositoryCompiled CompiledRepository =
	{ 
	&RepositoryHeader, 
	SettingsList 
	};

// Empty Repository
SSettingC SettingsListEmpty[] = {
	{{{ 0, 0 }, ETypeUndefined, 0x0000, 0 }, {{ 0 }}}
	};

SRepositoryBase EmptyRepositoryHeader =
	{
	HCR_FINGER_PRINT, 
	EReposCompiled, 
	KRepositoryFirstVersion,
	EReposReadOnly,
	0
	};

SRepositoryCompiled CompiledEmptyRepository =
	{ 
	&EmptyRepositoryHeader,
	SettingsListEmpty
	};

// Corrupt Repository 1
SRepositoryBase RepositoryHeaderCorrupt1 =
	{
	HCR_FINGER_PRINT, 
	EReposCompiled, 
	KRepositoryFirstVersion,
	EReposReadOnly,
	(sizeof(SettingsListCorrupt1) / sizeof(SSettingC))
	};

SRepositoryCompiled CompiledRepositoryCorrupt1 =
	{ 
	&RepositoryHeaderCorrupt1,
	SettingsListCorrupt1
	};

// Corrupt Repository 2
SRepositoryBase RepositoryHeaderCorrupt2 =
	{
	HCR_FINGER_PRINT, 
	EReposCompiled, 
	KRepositoryFirstVersion,
	EReposReadOnly,
	(sizeof(SettingsListCorrupt2) / sizeof(SSettingC))
	};

SRepositoryCompiled CompiledRepositoryCorrupt2 =
	{ 
	&RepositoryHeaderCorrupt2,
	SettingsListCorrupt2
	};

// Bad repository: iOrderedSettingList points to NULL
SRepositoryCompiled CompiledRepositoryNullOrderedList =
	{ 
	&RepositoryHeader,
	NULL
	};
