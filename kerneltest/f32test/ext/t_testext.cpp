// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\ext\t_testext.cpp
// 
//

#include <e32math.h>
#include "t_testext.h"

CTestProxyDrive::CTestProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount)
    : CBaseExtProxyDrive(aProxyDrive,aMount), iMount(aMount)
    {
    __PRINT(_L("CTestProxyDrive::CTestProxyDrive"));
    }

CTestProxyDrive::~CTestProxyDrive()
    {
    __PRINT(_L("CTestProxyDrive::~CTestProxyDrive"));
    delete[] iMap;
    }

void CTestProxyDrive::InitL()
    {
    __PRINT(_L("CTestProxyDrive::Init"));

    DoInitL();

    // Allocate bad sector bit map
    TInt size = (iTotalSectors+7) >> 3; // round up by byte
    iMap = new(ELeave) TUint8[size];
    Mem::FillZ(iMap, size);

    iMountInitialised = ETrue;
    }

TInt CTestProxyDrive::ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2)
    {
    __PRINT(_L("CTestProxyDrive::ControlIO"));

    TInt r = KErrNone;

    if (!iMountInitialised)
        return KErrNotReady;

    switch(aCommand+EExtCustom)
        {
        case ESetEvent:
            r = SetEvent((TInt)aParam1, (TInt)aParam2);
            break;
        
        case EMark:
            //-- mark given sector as bad
            __PRINT1(_L("CTestProxyDrive: marking sector:%d as bad"), (TInt)aParam1);
            r = Mark((TInt)aParam1);
            break;
        
        case EUnmark:
            //-- mark given sector as good
            __PRINT1(_L("CTestProxyDrive: Unmarking sector:%d"), (TInt)aParam1);
            r = Unmark((TInt)aParam1);
            break;
        
        case EUnmarkAll:
            r = UnmarkAll();
            break;
        
        case EIsMarked:
            if (IsMarked((TInt)aParam1))
                r = aMessage.Write(3, TPckgBuf<TBool>(ETrue));
            else
                r = aMessage.Write(3, TPckgBuf<TBool>(EFalse));
            break;
        case EDiskSize:
            r = aMessage.Write(2, TPckgBuf<TInt64>(iMount->Size()));
            break;
        default:
            r = DoControlIO(aMessage,aCommand,aParam1,aParam2);
            __PRINT2(_L("Get unknown command %d error %d"), aCommand, r);
        }
    return r;
    }

TInt CTestProxyDrive::GetLastErrorInfo(TDes8 &aErrorInfo)
    {
    __PRINT(_L("CTestProxyDrive::GetLastErrorInfo"));
    TPckgBuf<TErrorInfo> errInfoBuf;
    errInfoBuf().iReasonCode = iLastErrorReason;
    errInfoBuf().iErrorPos = iSuccessBytes;
    aErrorInfo = errInfoBuf;

    return KErrNone;
    }

TInt CTestProxyDrive::SetEvent(TInt aType, TInt aValue)
    {
    __PRINT(_L("CTestProxyDrive::SetEvent"));

    switch (aType)
        {
        case ENone:
            break;
        case ENext:
            iEventType = aType;
            break;
        case ERandom:
            Mark((TInt)(Math::Random() / (TReal64)KMaxTUint) * iTotalSectors);
            break;
        case EDeterministic:
            iEventType = aType;
            if (aValue <= 0)
                return KErrArgument;
            iCount = aValue;
            break;
        default:
            return KErrArgument;
        }
    return KErrNone;
    }


/**
    mark aSector as a bad sector by adding it to the bad sectors map
    @param  aSector sector number to mark as bad
*/
TInt CTestProxyDrive::Mark(TInt aSector)
    {
    __PRINT1(_L("CTestProxyDrive::Mark %d"), aSector);

    if (aSector >= iTotalSectors)
        return KErrNone;
    if (aSector < 0)
        return KErrArgument;
    iMap[aSector>>3] = (TUint8)(iMap[aSector>>3] | (1<<(aSector & 7)));
    return KErrNone;
    }

TInt CTestProxyDrive::Unmark(TInt aSector)
//
// Remove bad cluster aValue from list.
//
    {
    __PRINT1(_L("CTestProxyDrive::Unmark %d"), aSector);

    if (aSector>=iTotalSectors || aSector<0)
        return KErrArgument;
    iMap[aSector>>3] = (TUint8)(iMap[aSector>>3] & ~(1<<(aSector & 7)));
    return KErrNone;
    }

TInt CTestProxyDrive::UnmarkAll()
//
// Clear all bad cluster marks.
//
    {
    __PRINT(_L("CTestProxyDrive::UnmarkAll"));

    Mem::FillZ(iMap, (iTotalSectors+7)>>3);
    return KErrNone;
    }

TBool CTestProxyDrive::IsMarked(TInt aSector) const
//
// Check if cluster number is in bad cluster list.
//
    {
    

    if (aSector>=iTotalSectors || aSector<0)
        return EFalse;
    
    const TBool bMarked = iMap[aSector>>3] & (1<<(aSector & 7));
    if(bMarked)
        {
        __PRINT1(_L("CTestProxyDrive::IsMarked %d"), aSector);
        }

    return bMarked ;
    }

TBool CTestProxyDrive::CheckEvent(TInt64 aPos, TInt aLength)
    {
    //__PRINT(_L("CTestProxyDrive::CheckEvent"));

    if (!iMountInitialised)
        return EFalse;

    return DoCheckEvent(aPos, aLength);
    }
