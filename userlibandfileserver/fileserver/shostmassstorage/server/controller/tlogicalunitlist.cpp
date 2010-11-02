// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
*/


#include <e32base.h>

#include "msctypes.h"
#include "cusbhostmslogicalunit.h"
#include "tlogicalunitlist.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "tlogicalunitlistTraces.h"
#endif

TLogicalUnitList::~TLogicalUnitList()
    {
    iLu.ResetAndDestroy();
    }

void TLogicalUnitList::AddLuL(CUsbHostMsLogicalUnit* aLu)
    {
    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, TLOGICALUNITLIST_10,
              "Adding LU 0x%x", aLu);
    TInt r= FindLu(aLu->Lun());

    if (r < 0)
        {
        if (r != KErrNotFound)
            {
            User::Leave(r);
            }
        }
    else    // If it is able to find the lun already
        {
        User::Leave(KErrAlreadyExists);
        }

    User::LeaveIfError(iLu.Append(aLu));
    }


void TLogicalUnitList::RemoveLuL(TLun aLun)
    {
    TInt index = FindLu(aLun);
    User::LeaveIfError(index);

    CUsbHostMsLogicalUnit* lu = iLu[index];
    TRAPD(err, lu->UnInitL());
    delete lu;
    iLu.Remove(index);
    if(err != KErrNone)
        User::Leave(err);
    }


void TLogicalUnitList::RemoveAllLuL()
    {
    TInt ret = KErrNone;
    for (TInt index = 0; index < iLu.Count(); index++)
        {
        CUsbHostMsLogicalUnit* lu = iLu[index];
        TRAPD(err, lu->UnInitL());
        delete lu;
        // set return flag for first error condition
        if (ret == KErrNone && err != KErrNone)
            ret = err;
        }

    iLu.Reset();
    User::LeaveIfError(ret);
    }

TInt TLogicalUnitList::Count() const
    {
    return iLu.Count();
    }

TInt TLogicalUnitList::FindLu(TLun aLun) const
    {
    TInt index;
    for (index = 0; index < iLu.Count(); index++)
        {
        OstTraceExt2(TRACE_SHOSTMASSSTORAGE_HOST, TLOGICALUNITLIST_11,
                     "search LUN=%d : interface id = %d",
                     (TInt)aLun, (TInt)iLu[index]->Lun());
        if (iLu[index]->Lun() == aLun)
            {
            break;
            }
        }

    if (index == iLu.Count())
        {
        return KErrNotFound;
        }

    return index; // Instead of KErrNone, we pass the index >= 0 of the aLun in the list
    }

CUsbHostMsLogicalUnit& TLogicalUnitList::GetLuL(TLun aLun) const
    {
    TInt index = FindLu(aLun);
    User::LeaveIfError(index);
    return *iLu[index];
    }


CUsbHostMsLogicalUnit& TLogicalUnitList::GetLu(TInt aIndex) const
    {
    return *iLu[aIndex];
    }
