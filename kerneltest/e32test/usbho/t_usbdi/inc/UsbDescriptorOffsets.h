#ifndef __USB_DESCRIPTOR_OFFSETS_H
#define __USB_DESCRIPTOR_OFFSETS_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* @file UsbDescriptorOffsets.h
* @internalComponent
* 
*
*/




#include <e32base.h>

namespace NUnitTesting_USBDI
	{

// USB Device Descriptor

extern const TInt KDevDescOffset_bcdUSB;
extern const TInt KDevDescOffset_bDeviceClass;
extern const TInt KDevDescOffset_bDeviceSubClass;
extern const TInt KDevDescOffset_bDeviceProtocol;
extern const TInt KDevDescOffset_bMaxPacketSize; // For Ep0
extern const TInt KDevDescOffset_idVendor;
extern const TInt KDevDescOffset_idProduct;
extern const TInt KDevDescOffset_bcdDevice;
extern const TInt KDevDescOffset_iManufacturer;
extern const TInt KDevDescOffset_iProduct;
extern const TInt KDevDescOffset_iSerialNumber;
extern const TInt KDevDescOffset_bNumConfigurations;

// USB Configuration Descriptor

extern const TInt KConfDescOffset_bNumInterfaces;
extern const TInt KConfDescOffset_bConfigurationValue;
extern const TInt KConfDescOffset_iConfiguration;
extern const TInt KConfDescOffset_bMaxPower;
	
// USB Interface Descriptor

extern const TInt KIntDescOffset_bInterfaceNumber;
extern const TInt KIntDescOffset_bAlternateSetting;
extern const TInt KIntDescOffset_bNumEndpoints;
extern const TInt KIntDescOffset_iInterface;

// USB String Descriptor 0
	
extern const TInt KStrDescZeroOffset_bLength;
extern const TInt KStrDescZeroOffset_bDescriptorType;
extern const TInt KStrDescZeroOffset_wLANGID0;
	
// USB String Descriptor

extern const TInt KStrDescOffset_bLength;
extern const TInt KStrDescOffset_bDescriptorType;
extern const TInt KStrDescOffset_bString;
	
	}


#endif