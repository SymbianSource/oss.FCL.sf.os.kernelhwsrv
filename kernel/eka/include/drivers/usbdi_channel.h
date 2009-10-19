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

#ifndef USBDI_CHANNEL_H
#define USBDI_CHANNEL_H

class DUsbInterface;
class DUsbPageList;

/**
  Logical Device (factory class) for USBDI
*/
NONSHARABLE_CLASS(DUsbdiFactory) : public DLogicalDevice
	{
public:
	DUsbdiFactory();
	~DUsbdiFactory();

private:
	//	Inherited from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};
	

NONSHARABLE_CLASS(DUsbdiChannel) : public DLogicalChannelBase
	{
public:
	DUsbdiChannel();

	TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
	TInt Request(TInt aReqNo, TAny* a1, TAny* a2);

	virtual void CloseChannel();

private:
	~DUsbdiChannel();
	TInt Close();

	TInt AllocChunk(TInt aRequestSize, TInt* aChunkHandle, TInt* aOffset);
	void CloseChunk();

	TInt HandleControl(TInt aReqNo, TAny* a1, TAny* a2);
	TInt HandleRequest(TInt aReqNo, TRequestStatus* aReq, TAny* a1, TAny* a2);
	TInt HandleCancel(TUint aCancelMask);
	
private:
	DUsbInterface* iInterface;
	DUsbPageList* iPageList; // Owns the shared chunk
	};

#endif
