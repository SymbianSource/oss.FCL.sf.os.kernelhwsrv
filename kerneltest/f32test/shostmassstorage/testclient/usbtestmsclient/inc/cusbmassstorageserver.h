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
// Implements a Symbian OS server that exposes the RUsbMassStorage API
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef __CUSBMASSSTORAGESERVER_H__
#define __CUSBMASSSTORAGESERVER_H__




//
// Forward declarations
//
class CUsbMassStorageController;

/**
 The CUsbMassStorageServer class
 Implements a Symbian OS server that exposes the RUsbMassStorage API
 */
 class CUsbMassStorageServer : public CPolicyServer
	{
public:
	static CUsbMassStorageServer* NewLC(CUsbMassStorageController& aController);
	virtual ~CUsbMassStorageServer();

	virtual CSession2* NewSessionL(const TVersion &aVersion, const RMessage2& aMessage) const;
	void Error(TInt aError);

	inline CUsbMassStorageController& Controller() const;

	void IncrementSessionCount();
	void DecrementSessionCount();
	inline TInt SessionCount() const;

protected:
	CUsbMassStorageServer(CUsbMassStorageController& aController);
	void ConstructL();

private:
	CUsbMassStorageController& iController;
	TInt iSessionCount;
	};

#include "cusbmassstorageserver.inl"

#endif // __CUSBMASSSTORAGESERVER_H__
