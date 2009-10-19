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



/**
 @file
 @internalTechnology
*/

#ifndef MSERVERPROTOCOL_H
#define MSERVERPROTOCOL_H

class MDeviceTransport;
class TMassStorageConfig;


class MServerProtocol
	{
public:
	virtual void RegisterTransport(MDeviceTransport* aTransport) = 0;
	virtual TBool DecodePacket(TPtrC8& aData, TUint8 aLun) = 0;
	virtual TInt MediaWritePacket(TUint& aBytesWritten) = 0;
    virtual void MediaWriteAbort() = 0;
	virtual TInt Cancel() = 0;
	virtual void ReportHighSpeedDevice() {};
    virtual void SetParameters(const TMassStorageConfig& aConfig) = 0;
	virtual ~MServerProtocol() {};
	};


#endif // MSERVERPROTOCOL_H
