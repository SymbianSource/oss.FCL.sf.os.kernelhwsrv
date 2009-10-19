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

#ifndef USBDI_HUB_H
#define USBDI_HUB_H

class DUsbHubDriver;
class DUsbDevice;


const TUint32 KFunctionDriverFrameworkSid = 0x10282B48;

/**
  Logical Device (factory class) for USB Hub Driver
*/
class DUsbHubDriverFactory : public DLogicalDevice
	{
public:
	DUsbHubDriverFactory();
	~DUsbHubDriverFactory();
private:
	//	Inherited from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

/**
*/
class DUsbHubDriverChannel : public DLogicalChannel
	{
public:
	DUsbHubDriverChannel();
	virtual ~DUsbHubDriverChannel();
	virtual TInt SendMsg(TMessageBase* aMsg);

private:
	// Panic reasons
	enum TPanic
		{
		ERequestAlreadyPending = 1
		};
	
	//	Inherited from DObject
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);

	// Inherited from DLogicalChannel
	virtual void HandleMsg(TMessageBase* aMsg);

	// Implementation for the differnt kinds of messages sent through RBusLogicalChannel
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	void DoCancel(TUint aMask);
	TInt SendControl(TMessageBase* aMsg);
	TInt SendRequest(TMessageBase* aMsg);

	DUsbDevice* GetDeviceFromHandle(TAny* aSrc);
		
private:
	DUsbHubDriver*	iHubDriver;
	DThread*		iClient;
	};

#endif // USBDI_HUB_H

