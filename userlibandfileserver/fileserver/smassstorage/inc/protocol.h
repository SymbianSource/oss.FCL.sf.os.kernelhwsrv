// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <e32base.h>			// C Class Definitions, Cleanup Stack
#include <e32def.h>				// T Type  Definitions
#include <e32std.h>    			// ELeave definition
#include "usbmsshared.h"


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


class MProtocolBase
	{
	public:
	virtual void RegisterTransport(MTransportBase* aTransport) = 0;
	virtual TBool DecodePacket(TPtrC8& aData, TUint aLun) = 0;
	virtual TInt ReadComplete(TInt aError) = 0;
	virtual TInt Cancel() = 0;
	virtual void ReportHighSpeedDevice() {};
	virtual ~MProtocolBase() {};
#ifdef MSDC_MULTITHREADED
	virtual void InitializeBufferPointers(TPtr8& aDes1, TPtr8& aDes2) = 0;
#endif
	};


#endif // __PROTOCOL_H__

