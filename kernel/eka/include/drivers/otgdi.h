// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @internalComponent
*/

#ifndef OTGDI_H
#define OTGDI_H

class DUsbOtgDriver;

/**
  OTGDI is designed to be called solely from USBMAN
*/
const TUint32 KUsbmanSvrUid = {0x101FE1DB};

/**
  Logical Device (Factory Class) for USB OTG Driver
*/
class DUsbOtgDriverFactory : public DLogicalDevice
	{
public:

	DUsbOtgDriverFactory();
	~DUsbOtgDriverFactory();

	// Inherited from DLogicalDevice

	virtual TInt Install();
	virtual void GetCaps( TDes8& aDes ) const;
	virtual TInt Create( DLogicalChannelBase*& aChannel );
	};

/**
  Communication Channel between User and Kernel for USB OTG Driver
*/
class DUsbOtgDriverChannel : public DLogicalChannel
	{
public:

	DUsbOtgDriverChannel();
	virtual ~DUsbOtgDriverChannel();

	// Inherited from DObject

	virtual TInt DoCreate( TInt aUnit, const TDesC8* anInfo, const TVersion& aVer );

	// Inherited from DLogicalChannel

	virtual void HandleMsg( TMessageBase* aMsg );
	virtual TInt SendMsg(TMessageBase* aMsg);

private:

	// Implementation for the different kinds of messages sent through RBusLogicalChannel

	TInt DoControl( TInt aFunction, TAny* a1, TAny* a2 );
	TInt DoRequest( TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2 );
	void DoCancel( TUint aMask );
	TInt SendRequest(TMessageBase* aMsg);

private:

	// Stack lock/unlock protected methods

	TInt ProtectedControl( TInt aFunction );

private:

	DUsbOtgDriver*	iOtgDriver;
	DThread*		iClient;
	};

#endif // OTGDI_H
