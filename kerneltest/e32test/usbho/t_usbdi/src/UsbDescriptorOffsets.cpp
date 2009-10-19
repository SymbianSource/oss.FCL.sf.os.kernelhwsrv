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
// @file usbdescriptoroffsets.cpp
// @internalComponent
// 
//

#include "UsbDescriptorOffsets.h"


namespace NUnitTesting_USBDI
	{

// USB Device Descriptor

const TInt KDevDescOffset_bcdUSB(2);
const TInt KDevDescOffset_bDeviceClass(4);
const TInt KDevDescOffset_bDeviceSubClass(5);
const TInt KDevDescOffset_bDeviceProtocol(6);
const TInt KDevDescOffset_bMaxPacketSize(7); // For Ep0
const TInt KDevDescOffset_idVendor(8);
const TInt KDevDescOffset_idProduct(10);
const TInt KDevDescOffset_bcdDevice(12);
const TInt KDevDescOffset_iManufacturer(14);
const TInt KDevDescOffset_iProduct(15);
const TInt KDevDescOffset_iSerialNumber(16);
const TInt KDevDescOffset_bNumConfigurations(17);

// USB Configuration Descriptor

const TInt KConfDescOffset_bNumInterfaces(4);
const TInt KConfDescOffset_bConfigurationValue(5);
const TInt KConfDescOffset_iConfiguration(6);
const TInt KConfDescOffset_bMaxPower(8);

// USB Interface Descriptor

const TInt KIntDescOffset_bInterfaceNumber(2);
const TInt KIntDescOffset_bAlternateSetting(3);
const TInt KIntDescOffset_bNumEndpoints(4);	
const TInt KIntDescOffset_iInterface(8);

// USB String Descriptor 0

const TInt KStrDescZeroOffset_bLength(0);
const TInt KStrDescZeroOffset_bDescriptorType(1);
const TInt KStrDescZeroOffset_wLANGID0(2);

// USB String Descriptor

const TInt KStrDescOffset_bLength(0);
const TInt KStrDescOffset_bDescriptorType(1);
const TInt KStrDescOffset_bString(2);

	}
