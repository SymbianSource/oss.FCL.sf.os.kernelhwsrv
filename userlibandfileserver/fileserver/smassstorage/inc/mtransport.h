// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef MTRANSPORT_H
#define MTRANSPORT_H


class MProtocolBase;

class MTransportBase
    {
    public:
    virtual void SetupWriteData(TPtrC8& aData) = 0;
    virtual TInt Start() = 0;
    virtual TInt Stop() = 0;
    virtual void RegisterProtocol(MProtocolBase& aProtocol) = 0;
    virtual TInt BytesAvailable() = 0;
    virtual ~MTransportBase() {};
    virtual TInt InitialiseTransportL(TInt aTransportLddFlag) = 0;
    virtual void SetupReadData(TUint aLength) = 0;
    virtual void GetCommandBufPtr(TPtr8& aDes, TUint aLength) = 0;
    virtual void GetReadDataBufPtr(TPtr8& aDes) = 0;
    virtual void GetWriteDataBufPtr(TPtrC8& aDes) = 0;
#ifdef MSDC_MULTITHREADED
    virtual void ProcessReadData(TAny* aAddress) = 0;
#endif
    };


#endif // MTRANSPORT_H

