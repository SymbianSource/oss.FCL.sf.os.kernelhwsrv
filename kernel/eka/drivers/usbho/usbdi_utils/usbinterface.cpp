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

#include <d32usbdi.h>

#include <d32usbtransfers.h>
#include "usbtransferstrategy.h"
#include "zerocopytransferstrategy.h"


/**
Opens an interface identified by a token.  When the hub driver loads a driver (via function driver
framework), this token is generated to allow the driver to open the interface.

This also causes the interface's descriptors to be parsed for future reference.

@param[in] aToken The token for the interface to open.
@return System-wide error code.
*/
EXPORT_C TInt RUsbInterface::Open(TUint32 aToken, TOwnerType aType)
	{
	TPckgC<TUint32> token(aToken);
	TInt err = DoCreate(Name(), VersionRequired(), KNullUnit, NULL, &token, aType);
	if(err == KErrNone)
		{
		// Create a transfer strategy
		iTransferStrategy = new RUsbZeroCopyTransferStrategy;
		if(!iTransferStrategy)
			{
			Close();
			return KErrNoMemory;
			}

		// Get descriptor size
		TInt interfaceDescSize = 0;
		err = DoControl(EGetInterfaceDescriptorSize, &interfaceDescSize);
		if(err != KErrNone)
			{
			Close();
			return err;
			}
		iInterfaceDescriptorData = HBufC8::New(interfaceDescSize);

		if(!iInterfaceDescriptorData)
			{
			Close();
			return KErrNoMemory;
			}

		// Get descriptor data
		TPtr8 interfaceDesc = iInterfaceDescriptorData->Des();
		err = DoControl(EGetInterfaceDescriptor, &interfaceDesc);
		if(err != KErrNone)
			{
			Close();
			return err;
			}

		// Parse descriptor
		TUsbGenericDescriptor* parsed = NULL;
		err = UsbDescriptorParser::Parse(*iInterfaceDescriptorData, parsed);
		if(err != KErrNone)
			{
			if(parsed)
				{
				parsed->DestroyTree(); //or however much has been completed
				delete parsed;
				}
			Close();
			return err;
			}

		iHeadInterfaceDescriptor = TUsbInterfaceDescriptor::Cast(parsed);
		if(!iHeadInterfaceDescriptor)
			{
			if(parsed)
				{
				parsed->DestroyTree();
				delete parsed;
				}
			Close();
			return KErrCorrupt;
			}
		}

	return err;
	}
	
/**
Close handle to interface.

Closes any pipe handles still open.
*/
EXPORT_C void RUsbInterface::Close()
	{
	iAlternateSetting = 0;
	if(iHeadInterfaceDescriptor)
		{
		iHeadInterfaceDescriptor->DestroyTree();
		delete iHeadInterfaceDescriptor;
		iHeadInterfaceDescriptor = NULL;
		}
	if(iInterfaceDescriptorData)
		{
		delete iInterfaceDescriptorData;
		iInterfaceDescriptorData = NULL;
		}
	if(iTransferStrategy)
		{
		iTransferStrategy->Close();
		delete iTransferStrategy;
		iTransferStrategy = NULL;
		}
	RBusLogicalChannel::Close();
	}


EXPORT_C TInt RUsbInterface::RegisterTransferDescriptor(RUsbTransferDescriptor& aTransfer)
	{
	TTransferMemoryDetails details;
	details.iType		= aTransfer.iType;
	details.iSize		= aTransfer.iMaxSize;
	details.iMaxPackets	= aTransfer.iMaxNumPackets;
	TInt err = DoControl(EGetSizeAndAlignment, &details);
	if(err != KErrNone)
		{
		return err;
		}
	return iTransferStrategy->RegisterTransferDescriptor(aTransfer, details.iSize, details.iAlignment, details.iMaxPackets);
	}
	
EXPORT_C void RUsbInterface::ResetTransferDescriptors()
	{
	iTransferStrategy->ResetTransferDescriptors();
	}

EXPORT_C TInt RUsbInterface::InitialiseTransferDescriptors()
	{
	return iTransferStrategy->InitialiseTransferDescriptors(*this);
	}


