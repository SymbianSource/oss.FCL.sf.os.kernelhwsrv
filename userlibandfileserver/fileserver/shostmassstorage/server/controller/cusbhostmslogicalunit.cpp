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

#include "shared.h"
#include "msgservice.h"
#include "msctypes.h"

#include "mprotocol.h"
#include "mblocktransferprotocol.h"
#include "tspcclientinterface.h"
#include "cscsiprotocol.h"

#include "cusbhostmslogicalunit.h"
#include "usbmshostpanic.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cusbhostmslogicalunitTraces.h"
#endif

CUsbHostMsLogicalUnit* CUsbHostMsLogicalUnit::NewL(TLun aLun)
    {
    CUsbHostMsLogicalUnit* r = new (ELeave) CUsbHostMsLogicalUnit(aLun);
    CleanupStack::PushL(r);
    r->ConstructL();
    CleanupStack::Pop();
    return r;
    }


void CUsbHostMsLogicalUnit::ConstructL()
    {
    const TInt KInitialDataBufSize = 0x200;
    iDataBuf.CreateL(KInitialDataBufSize);
    }


CUsbHostMsLogicalUnit::CUsbHostMsLogicalUnit(TLun aLun)
:   iProtocol(NULL),
    iLun(aLun),
    iSuspendRequest(EFalse)
    {
    }

CUsbHostMsLogicalUnit::~CUsbHostMsLogicalUnit()
    {
    iDataBuf.Close();
    delete iProtocol;
    }


void CUsbHostMsLogicalUnit::InitL()
    {
    iProtocol->InitialiseUnitL();
    }

void CUsbHostMsLogicalUnit::UnInitL()
    {
    iProtocol->UninitialiseUnitL();
    }

void CUsbHostMsLogicalUnit::ReadL(const RMessage2& aMessage)
    {
    TReadWrite p;
    TPtr8 pReadWrite((TUint8*)&p,sizeof(TReadWrite));
    aMessage.ReadL(0, pReadWrite);
    OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSLOGICALUNIT_10,
                 "pos = 0x%x %x len = %08x", I64HIGH(p.iPos), I64LOW(p.iPos), p.iLen);

    User::LeaveIfError(CheckPosition(p));

    // check if buffer can hold requested data and resize if not
    if (iDataBuf.MaxLength() < p.iLen)
        iDataBuf.ReAllocL(p.iLen);

    iProtocol->ReadL(p.iPos, iDataBuf, p.iLen);
    aMessage.WriteL(1, iDataBuf);
    }


void CUsbHostMsLogicalUnit::WriteL(const RMessage2& aMessage)
    {
    TReadWrite p;
    TPtr8 pReadWrite((TUint8*)&p,sizeof(TReadWrite));
    aMessage.ReadL(1, pReadWrite);
    OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSLOGICALUNIT_11,
                 "pos = 0x%x %x len = %08x", I64HIGH(p.iPos), I64LOW(p.iPos), p.iLen);

    User::LeaveIfError(CheckPosition(p));

    // check if buffer can hold requested data and resize if not
    if (iDataBuf.MaxLength() < p.iLen)
        iDataBuf.ReAllocL(p.iLen);

    aMessage.ReadL(0, iDataBuf);
    iProtocol->WriteL(p.iPos, iDataBuf, p.iLen);
    }


void CUsbHostMsLogicalUnit::EraseL(const RMessage2& aMessage)
    {
    TReadWrite p;
    TPtr8 pReadWrite((TUint8*)&p,sizeof(TReadWrite));
    aMessage.ReadL(0, pReadWrite);
    OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSLOGICALUNIT_12,
                 "pos = 0x%x %x len = %08x", I64HIGH(p.iPos), I64LOW(p.iPos), p.iLen);

    User::LeaveIfError(CheckPosition(p));

    // check if buffer can hold requested data and resize if not
    if (iDataBuf.MaxLength() < p.iLen)
        iDataBuf.ReAllocL(p.iLen);

    iDataBuf.FillZ(p.iLen);
    iProtocol->WriteL(p.iPos, iDataBuf, p.iLen);
    }


void CUsbHostMsLogicalUnit::CapsL(const RMessage2& aMessage)
    {
    TCapsInfo capsInfo;
    TPckg<TCapsInfo> pckg(capsInfo);
    iProtocol->GetCapacityL(capsInfo);
    iSize = static_cast<TInt64>(capsInfo.iNumberOfBlocks) * capsInfo.iBlockLength;
    aMessage.WriteL(0, pckg);
    }

void CUsbHostMsLogicalUnit::NotifyChange(const RMessage2& aMessage)
    {
    iProtocol->NotifyChange(aMessage);
    }


void CUsbHostMsLogicalUnit::ForceCompleteNotifyChangeL()
    {
    iProtocol->ForceCompleteNotifyChangeL();
    }

void CUsbHostMsLogicalUnit::CancelChangeNotifierL()
    {
    iProtocol->CancelChangeNotifierL();
    }

TBool CUsbHostMsLogicalUnit::IsConnected()
    {
    return iProtocol->IsConnected() ? ETrue : EFalse;
    }

TBool CUsbHostMsLogicalUnit::IsReadyToSuspend()
    {
    return iSuspendRequest ? ETrue : EFalse;
    }

void CUsbHostMsLogicalUnit::ReadyToSuspend()
    {
    iSuspendRequest = ETrue;
    }

void CUsbHostMsLogicalUnit::CancelReadyToSuspend()
    {
    iSuspendRequest = EFalse;
    }

void CUsbHostMsLogicalUnit::SuspendL()
    {
    iProtocol->SuspendL();
    }

void CUsbHostMsLogicalUnit::ResumeL()
    {
    // We do not cancel iSuspendRequest here
    iProtocol->ResumeL();
    }

void CUsbHostMsLogicalUnit::DoLunReadyCheckL()
    {
    iProtocol->DoScsiReadyCheckEventL();
    }

TInt CUsbHostMsLogicalUnit::InitialiseProtocolL(TLun aLun,
                                                THostMassStorageConfig& aConfig,
                                                MTransport& aTransport)
    {
    __ASSERT_DEBUG(iProtocol == NULL, User::Panic(KUsbMsHostPanicCat, EProtocolNotFree));
    switch(aConfig.iProtocolId)
        {
    case ScsiProtocol:
        iProtocol = CScsiProtocol::NewL(aLun, aTransport);
        break;
    default:
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSLOGICALUNIT_13,
                  "Unsupported Transport class requested");
        iProtocol = NULL;
        return KErrNotSupported;
        }
    return KErrNone;
    }

TInt CUsbHostMsLogicalUnit::CheckPosition(const TReadWrite& aReadWrite)
    {
    if (aReadWrite.iLen == 0)
        {
        if (aReadWrite.iPos == iSize)
            return KErrEof;
        else
            return KErrArgument;
        }

    // detect drive not present
    if (iSize == 0)
        return KErrNotReady;

    if (aReadWrite.iPos >= iSize)
        return KErrArgument;
    return KErrNone;
    }
