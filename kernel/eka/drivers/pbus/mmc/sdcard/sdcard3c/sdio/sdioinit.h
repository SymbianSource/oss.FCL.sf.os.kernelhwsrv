// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Factory class for creating platform-specific SDIO Drivers
// 
//

/**
  @file sdioinit.h
  @internalTechnology
*/

#ifndef __SDIOINIT_H__
#define __SDIOINIT_H__

#include <drivers/mmccd_ifc.h>

class TSDIOCardControllerInterface : public TMMCardControllerInterface
/**
 * Factory class for creating platform-specific SDIO Drivers
 */
	{
public:
	/**
	 @publishedPartner
	 @released

	 Provided by the Variant DLL to confirm whether an SDIO stack is supported 
	 on the specified socket and if it is, the Media Info. for that socket.
	 @param aSocket the socket ID
	 @param aMediaDeviceInfo the Media Info. for that socket
	 @return ETrue if MMC is supported on the socket
	 */
	virtual TBool IsSDIOSocket(TInt aSocket,SMediaDeviceInfo& aMediaDeviceInfo)=0;
	
protected:
	/**
	 Performs registration of the CSA media devices on the current socket
	 */
	IMPORT_C virtual TInt RegisterMediaDevices(TInt aSocket);
    };

#endif


