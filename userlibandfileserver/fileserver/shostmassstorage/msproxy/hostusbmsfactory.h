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

#ifndef __USBHOSTMS_PROXY_DRIVE_FACTORY_H__
#define __USBHOSTMS_PROXY_DRIVE_FACTORY_H__

class CUsbHostMsProxyDriveFactory : public CExtProxyDriveFactory
	{
public:
	CUsbHostMsProxyDriveFactory();
	~CUsbHostMsProxyDriveFactory();
	virtual TInt Install();
	virtual TInt CreateProxyDrive(CProxyDrive*& aMountProxyDrive,CMountCB* aMount);
	virtual void AsyncEnumerate();
	};

#endif
