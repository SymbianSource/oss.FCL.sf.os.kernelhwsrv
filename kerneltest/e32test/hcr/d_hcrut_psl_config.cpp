// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file is part of the NE1_TB Variant Base Port
// Hardware Configuration Respoitory Platform Specific Layer (PSL) 
//


/** 
@file hcr_psl_config.h
File provides dummy test configuration for HCR prototype developement. 
Definitions will be replaced by real NE1 configuration later.

@internalTechnology
*/

// -- INCLUDES ----------------------------------------------------------------


#include "hcr_hai.h"
#include "hcr_uids.h"

using namespace HCR;


// -- GLOBALS -----------------------------------------------------------------

#define STRING_TTEXT8  "Hardware Configuration Repository"


const TUint8   gETypeData[] = {0x53, 0x79, 0x6d, 0x62, 0x69, 0x61, 0x6e, 0x20, 0x46, 0x6f, 0x75, 0x6e, 0x64, 0x61, 0x74, 0x69, 0x6f, 0x6e};
const TText8*  gETypeText8 = reinterpret_cast<const TText8*>(STRING_TTEXT8);
const TInt64   gETypeInt64 = I64LIT(-0x200000020);
const TUint64  gETypeUInt64 = UI64LIT(0x200000000);
const TInt32   geTypeInt32A[] = { -2147483647, 0, 1, 2147483647 };
const TUint32  geTypeUInt32A[] = { 101010, 0xffffffff, 0, 128, 0xfffffffe, 76 };
const TInt32   geTypeInt32Az[] = { 0 };

SSettingC gSettingsList[] = 
    {
    { { { KHCRUID_TestCategory1, 1}, ETypeInt32,   0x0000, 0 }, { { 2345123 }}},
    { { { KHCRUID_TestCategory1, 2}, ETypeInt16,   0x0000, 0 }, { { 32001 }}},
    { { { KHCRUID_TestCategory1, 3}, ETypeInt8,    0x0000, 0 }, { { -126 }}},
    { { { KHCRUID_TestCategory1, 4}, ETypeBool,    0x0000, 0 }, { { ETrue }}},
    { { { KHCRUID_TestCategory1, 5}, ETypeUInt32,  0x0000, 0 }, { { 0xaabbccdd }}},
    { { { KHCRUID_TestCategory1, 6}, ETypeUInt16,  0x0000, 0 }, { { 0x1122 }}},
    { { { KHCRUID_TestCategory1, 7}, ETypeUInt8,   0x0000, 0 }, { { 0x33 }}},
    { { { KHCRUID_TestCategory1, 9}, ETypeLinAddr, 0x0000, 0 }, { { 0x01008000 }}},
    
    { { { KHCRUID_TestCategory1, 10}, ETypeBinData,0x0000, sizeof(gETypeData) },   { { reinterpret_cast<TInt32>(gETypeData) }}},
    { { { KHCRUID_TestCategory1, 11}, ETypeText8,  0x0000, sizeof(STRING_TTEXT8) },  { { reinterpret_cast<TInt32>(gETypeText8) }}},
    { { { KHCRUID_TestCategory1, 13}, ETypeInt64,  0x0000, sizeof(gETypeInt64) },  { { reinterpret_cast<TInt32>(&gETypeInt64) }}},
    { { { KHCRUID_TestCategory1, 14}, ETypeUInt64, 0x0000, sizeof(gETypeUInt64) }, { { reinterpret_cast<TInt32>(&gETypeUInt64) }}},

    { { { KHCRUID_TestCategory1, 15}, ETypeArrayInt32,  0x0000, sizeof(geTypeInt32A) },  { { reinterpret_cast<TInt32>(&geTypeInt32A) }}},
    { { { KHCRUID_TestCategory1, 16}, ETypeArrayUInt32, 0x0000, sizeof(geTypeUInt32A) }, { { reinterpret_cast<TInt32>(&geTypeUInt32A) }}},
    { { { KHCRUID_TestCategory1, 17}, ETypeArrayInt32,  0x0000, sizeof(geTypeInt32Az) },  { { reinterpret_cast<TInt32>(&geTypeInt32Az) }}}

    };

SRepositoryBase gHeader = 
    {
    HCR_FINGER_PRINT, 
    EReposCompiled, 
    KRepositoryFirstVersion,
    EReposReadOnly,
    (sizeof(gSettingsList)/sizeof(SSettingC))
    };

GLDEF_C SRepositoryCompiled gRepository = 
    { 
    &gHeader, 
    gSettingsList 
    };


