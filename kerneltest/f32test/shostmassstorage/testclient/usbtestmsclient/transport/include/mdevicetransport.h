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

#ifndef MDEVICETRANSPORT_H
#define MDEVICETRANSPORT_H


class MServerProtocol;

class MDeviceTransport
	{
public:
	virtual void SetupDataOut(TPtr8& aData) = 0;
	virtual void SetupDataIn(TPtrC8& aData) = 0;
	virtual TInt Start() = 0;
	virtual TInt Stop() = 0;
	virtual void RegisterProtocol(MServerProtocol& aProtocol) = 0;
	virtual TInt BytesAvailable() = 0;
	virtual ~MDeviceTransport() {};
	};

#endif // MDEVICETRANSPORT_H
