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

#ifndef LOGICALUNITLIST_H
#define LOGICALUNITLIST_H

class TLogicalUnitList : public CBase
    {
public:
    ~TLogicalUnitList();
    void AddLuL(CUsbHostMsLogicalUnit* aLu);
    void RemoveLuL(TLun aLun);
    void RemoveAllLuL();
    CUsbHostMsLogicalUnit& GetLuL(TLun aLun) const;
    CUsbHostMsLogicalUnit& GetLu(TInt aIndex) const;
	TInt Count() const;
private:
    TInt FindLu(TLun aLun) const;
    RPointerArray<CUsbHostMsLogicalUnit> iLu;
    };

#endif // LOGICALUNITLIST_H

