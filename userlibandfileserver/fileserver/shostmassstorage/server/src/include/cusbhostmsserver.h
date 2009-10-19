// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CUSBMASSSTORAGESERVER_H
#define CUSBMASSSTORAGESERVER_H

//
// Forward declarations
//
//class CUsbHostMassStorageController;

/**
 The CUsbHostMsServer class
 Implements a Symbian OS server that exposes the RUsbMassStorage API
 */
 class CUsbHostMsServer : public CPolicyServer
	{
public:
	static CUsbHostMsServer* NewLC();
	virtual ~CUsbHostMsServer();

	virtual CSession2* NewSessionL(const TVersion &aVersion, const RMessage2& aMessage) const;
	void Error(TInt aError);

	void IncrementSessionCount();
	void DecrementSessionCount();
	inline TInt SessionCount() const;

protected:
	CUsbHostMsServer();
	void ConstructL();

private:
	TInt iSessionCount;


	};

#include "cusbhostmsserver.inl"

#endif // CUSBMASSSTORAGESERVER_H


