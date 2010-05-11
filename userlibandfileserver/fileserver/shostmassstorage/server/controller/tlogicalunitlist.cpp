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

#include "msdebug.h"
#include "debug.h"

TLogicalUnitList::~TLogicalUnitList()
    {
    __MSFNLOG
    iLu.ResetAndDestroy();
    }

void TLogicalUnitList::AddLuL(CUsbHostMsLogicalUnit* aLu)
    {
    __MSFNLOG
    __HOSTPRINT1(_L("Adding LU 0x%x"), aLu);
    TInt r= FindLu(aLu->Lun());

    if (r < 0)
        {
        if (r != KErrNotFound)
            {
            User::Leave(r);
            }
        }
    else	// If it is able to find the lun already
        {
        User::Leave(KErrAlreadyExists);
        }

	User::LeaveIfError(iLu.Append(aLu));
    }


void TLogicalUnitList::RemoveLuL(TLun aLun)
    {
    __MSFNLOG
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
    __MSFNLOG
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
    __MSFNSLOG
	return iLu.Count();
	}

TInt TLogicalUnitList::FindLu(TLun aLun) const
    {
    __MSFNSLOG

    TInt index;
    for (index = 0; index < iLu.Count(); index++)
        {
        __HOSTPRINT2(_L("search LUN=%d : interface id = %d"), aLun, iLu[index]->Lun());
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
    __MSFNSLOG
    TInt index = FindLu(aLun);
    User::LeaveIfError(index);
    return *iLu[index];
    }


CUsbHostMsLogicalUnit& TLogicalUnitList::GetLu(TInt aIndex) const
    {
    __MSFNSLOG
    return *iLu[aIndex];
    }
