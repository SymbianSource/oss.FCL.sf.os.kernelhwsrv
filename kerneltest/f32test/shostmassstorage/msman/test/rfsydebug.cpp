// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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


#include <e32def.h>
#include <e32property.h>

#define __E32TEST_EXTENSION__
#include <e32test.h>


#include "rfsydebug.h"


static const TUid KFsyPropertyCat={0x10210EB3};

extern RTest test;


RFsyDebug::RFsyDebug(TInt aDriveNo)
:   iDriveNo(aDriveNo)
    {
    //-- define a propery which will control mount process in the fsy.
    //-- The property key is a drive number being tested

    _LIT_SECURITY_POLICY_PASS(KTestPropPolicy);
    TInt res = RProperty::Define(KFsyPropertyCat,
                                 iDriveNo,
                                 RProperty::EInt,
                                 KTestPropPolicy,
                                 KTestPropPolicy);

    test(res == KErrNone || res == KErrAlreadyExists);
    }

RFsyDebug::~RFsyDebug()
    {
    TInt res = RProperty::Delete(KFsyPropertyCat, iDriveNo);
    test_KErrNone(res);
    }

/** set a debug property value, which then will be read by FAT fsy */
void RFsyDebug::Set(TUint32 aFlags)
{
    TInt res = RProperty::Set(KFsyPropertyCat, iDriveNo, aFlags);
    test_KErrNone(res);
}


void RFsyDebug::DisableAll()
{
    TInt res = RProperty::Set(KFsyPropertyCat, iDriveNo, static_cast<TInt>(KMntProp_DisableALL));
    test_KErrNone(res);
}


