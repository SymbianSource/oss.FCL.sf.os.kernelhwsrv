// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Shared client/server definitions 
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __HOSTUSBMSSHARED_H__
#define __HOSTUSBMSSHARED_H__

const TUint32 KMaxUsbSerialNumLength=256;

/**
This class represents the device configuration.
*/
class THostMassStorageConfig
	{
public:
    /** Device token */
	TUint32 iInterfaceToken;
    /** USB Vendor ID */
	TUint16 iVendorId;
    /** USB Product ID */
	TUint16 iProductId;
    /** USB Device Release Number in BCD */
	TUint16 iBcdDevice;
    /** USB Number of possible configurations supported by the device */
	TUint8  iConfigurationNumber;
    /** USB Interface number */
	TUint8  iInterfaceNumber;
    /** Device Serial Number */
	TBuf8<KMaxUsbSerialNumLength> iSerialNumber;
	/** Protocol to be used by the MSC */
	TUint8	iProtocolId;
	/** Transport to be used by the MSC */
	TUint8	iTransportId;
	/** Time internval to check media status and finalisation */
	TUint8	iStatusPollingInterval;
	/** Device's capabilitiy for RemoteWakeup */
	TUint8	iRemoteWakeup;
	/** OTG capability of the device */
	TUint8	iIsOtgClient;	// NOT USED
	/** Time interval to delay suspending the interface after finalisation */
	TUint8	iOtgSuspendTime;	// NOT USED

private:
	TUint32 iSpare1;
	TUint32 iSpare2;
	TUint32 iSpare3;
	TUint32 iSpare4;
	};


struct TCapsInfo
    {
    /** Size of Media in Blocks */
    TUint32 iNumberOfBlocks;
    /** Block Length */
    TUint32 iBlockLength;
    /** Media write protect */
    TBool iWriteProtect;
    /** Block device or CD-ROM */
    TMediaType iMediaType;
    };



#endif //__HOSTUSBMSSHARED_H__
