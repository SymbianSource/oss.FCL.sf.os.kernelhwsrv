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
//

#include <f32file.h>
#include <d32usbdi_hubdriver.h>
#include <d32usbdi.h>
#include <d32otgdi.h>
#include <d32usbdescriptors.h>
#include <d32usbtransfers.h>

#include <shared.h>

#include "rusbhostmsdevice.h"

#include "rextfilesystem.h"
#include "usbtypes.h"
#include "cusbmsmountmanager.h"
#include "tmslog.h"
#include "debug.h"


CDevice* CDevice::NewL()
    {
    __MSFNSLOG
	CDevice* r = new (ELeave) CDevice();
	CleanupStack::PushL(r);

	r->ConstructL();
	CleanupStack::Pop();
	return r;
    }


void CDevice::ConstructL()
    {
    __MSFNLOG
    }


CDevice::CDevice()
    {
    __MSFNLOG
    }


CDevice::~CDevice()
    {
    __MSFNLOG
    __USBHOSTPRINT1(_L("~CDevice Token=%d"), iDeviceToken);
    }


TToken CDevice::OpenDeviceL(TUint aDeviceHandle, RUsbHubDriver& aHub)
    {
    __MSFNLOG
    __USBHOSTPRINT1(_L("CDevice::OpenDeviceL Handle=%d"), aDeviceHandle);

    TInt err = iUsbDevice.Open(aHub, aDeviceHandle);
    __USBHOSTPRINT1(_L(" - returned %d\n"), err);
    User::LeaveIfError(err);

    /* Retrieve the device descriptor */
    TUsbDeviceDescriptor devDescriptor;
    User::LeaveIfError(iUsbDevice.GetDeviceDescriptor(devDescriptor));
    iUsbPrint.PrintDescriptor(devDescriptor, 0, &iUsbDevice);

    iUsbPrint.PrintTree(devDescriptor);

    /* Retrieve the configuration descriptor */
    TUsbConfigurationDescriptor configDescriptor;
    User::LeaveIfError(iUsbDevice.GetConfigurationDescriptor(configDescriptor));
    iUsbPrint.PrintDescriptor(configDescriptor, 0, &iUsbDevice);

    /* Get the token for interface 0 */
    TUint32 token;
    err = iUsbDevice.GetTokenForInterface(0, token);
    __USBHOSTPRINT2(_L("RUsbDevice::GetTokenForInterface returned error %d, token %08x"), err, token);
    User::LeaveIfError(err);

    /* open the interface */
    RUsbInterface interface_ep0;
    err = interface_ep0.Open(token);
    __USBHOSTPRINT1(_L("RUsbInterface::Open returned error %d"), err);
    User::LeaveIfError(err);

    /* Retrieve the interface and device descriptors */
    TUsbInterfaceDescriptor ifDescriptor;
    User::LeaveIfError(interface_ep0.GetInterfaceDescriptor(ifDescriptor));
    iUsbPrint.PrintDescriptor(ifDescriptor);

    if (!IsDeviceMassStorage(ifDescriptor, devDescriptor))
        {
        RDebug::Print(_L("ATTACHED DEVICE IS NOT A MASS STORAGE DEVICE!\n"));
        User::Leave(KErrGeneral);
        }

	TUint8 iProtocolId = ifDescriptor.InterfaceSubClass();
	TUint8 iTransportId = ifDescriptor.InterfaceProtocol();

    interface_ep0.Close();

    THostMassStorageConfig msConfig;
    msConfig.iInterfaceToken = token;
	msConfig.iProtocolId =iProtocolId;
	msConfig.iTransportId = iTransportId;
	msConfig.iStatusPollingInterval = 10; // 10 secs

    TUint32 numLun;

    TRequestStatus status;
    iUsbHostMsDevice.Add(msConfig, status);
	User::WaitForRequest(status);
	if (status.Int() != KErrNone)
        {
		__USBHOSTPRINT(_L("Add device failed"));
		User::Leave(status.Int());
        }
	TInt r = iUsbHostMsDevice.GetNumLun(numLun);
	if (r != KErrNone)
        {
		__USBHOSTPRINT(_L("GetNumLun failed"));
		User::Leave(r);
        }

    if (numLun > KMaxLun)
        {
        __USBHOSTPRINT1(_L("Device MaxLun = %d. Error MaxLun > MAXLUN !"), numLun);
        User::Leave(KErrGeneral);
        }

    __USBHOSTPRINT1(_L("MSC registered with %d Luns"), numLun);

    iDeviceToken = token;
    iNumLuns = numLun;
    iDeviceHandle = aDeviceHandle;
    return token;
    }


void CDevice::CloseDeviceL()
    {
    __MSFNLOG

    THostMassStorageConfig msConfig;
    msConfig.iInterfaceToken = iDeviceToken;

    iUsbHostMsDevice.Remove();
    iUsbDevice.Close();
    }


void CDevice::MountLogicalUnitsL()
    {
    __MSFNLOG
    iExt.OpenL();

    for (TInt lun = 0; lun < iNumLuns; lun++)
        {
        TDriveNumber driveNumber = iExt.GetDriveL();
        __PRINT2(_L("Mounting drive=%d lun=%d..."), driveNumber, lun);
        RDebug::Print(_L("Mounting drive=%d lun=%d..."), driveNumber, lun);
        TRAPD(err, iExt.MountL(iUsbHostMsDevice, driveNumber, iDeviceToken, lun));
        if (err == KErrNone || err == KErrNotReady || err == KErrCorrupt)
            {
            iLuList.Append(driveNumber);
            }
        __PRINT1(_L("%d"), err);
        RDebug::Print(_L("err=%d"), err);
        }
    }


void CDevice::DismountLogicalUnitsL()
    {
    __MSFNLOG
    for (TInt lun = 0; lun < iLuList.Count(); lun++)
        {
        TDriveNumber driveNumber = iLuList[lun];
        iExt.DismountL(iUsbHostMsDevice, driveNumber);
        }
    iLuList.Reset();

    iExt.CloseL();
    }


TInt CDevice::GetEndpointAddress(RUsbInterface& aUsbInterface,
                                 TInt aInterfaceSetting,
                                 TUint8 aTransferType,
                                 TUint8 aDirection,
                                 TInt& aEndpointAddress) const
	{
    __MSFNSLOG

	// Get the interface descriptor
	RDebug::Print(_L("GetEndpointAddress : Getting the interface descriptor for this alternate setting"));

	TUsbInterfaceDescriptor alternateInterfaceDescriptor;
	TInt err = aUsbInterface.GetAlternateInterfaceDescriptor(aInterfaceSetting, alternateInterfaceDescriptor);

	if (err)
		{
		RDebug::Print(_L("GetEndpointAddress : <Error %d> Unable to get alternate interface (%d) descriptor"),err,aInterfaceSetting);
		return err;
		}

	// Parse the descriptor tree from the interface
	RDebug::Print(_L("Search the child descriptors for matching endpoint attributes"));

	TUsbGenericDescriptor* descriptor = alternateInterfaceDescriptor.iFirstChild;

	while (descriptor)
		{
		RDebug::Print(_L("GetEndpointAddress : Check descriptor type for endpoint"));

		// Cast the descriptor to an endpoint descriptor
		TUsbEndpointDescriptor* endpoint = TUsbEndpointDescriptor::Cast(descriptor);

		if (endpoint)
			{
			RDebug::Print(_L("GetEndpointAddress : Match attributes for transfer type"));

			if ( (endpoint->Attributes() & aTransferType) == aTransferType)
				{
				RDebug::Print(_L("GetEndpointAddress : Match attributes for endpoint direction"));

				if ( (endpoint->EndpointAddress() & 0x80) == aDirection)
					{
					aEndpointAddress = endpoint->EndpointAddress();
					RDebug::Print(_L("GetEndpointAddress : Endpoint address found"));
					return KErrNone;
					}
				}
			}

		descriptor = descriptor->iNextPeer;
		}

	// Unable to find the endpoint address
	RDebug::Print(_L("GetEndpointAddress : Unable to find endpoint address matching the specified attributes"));

	return KErrNotFound;
	}


TBool CDevice::IsDeviceMassStorage(const TUsbInterfaceDescriptor& aInterfaceDesc,
                                   const TUsbDeviceDescriptor& aDeviceDesc) const
    {
    __MSFNSLOG
	/* check the interface descriptor */
	if(aInterfaceDesc.InterfaceClass() == 0x08 &&
		aInterfaceDesc.InterfaceSubClass() == 0x06 &&
		aInterfaceDesc.InterfaceProtocol() == 0x50)
        {
		if(aDeviceDesc.DeviceClass() == 0x00 &&
			aDeviceDesc.DeviceSubClass() == 0x00 &&
			aDeviceDesc.DeviceProtocol() == 0x00)
			return ETrue;
        }

	return EFalse;

    }


TLun CDevice::DriveMap(TDriveMap& aDriveMap) const
    {
    __MSFNSLOG
    TDriveNumber driveNumber;
    RDebug::Printf("LuList.Count=%d", iLuList.Count());
    for (TInt i = 0; i < iLuList.Count(); i++)
        {
        driveNumber = iLuList[i];
        aDriveMap[driveNumber] = iDeviceToken;
        RDebug::Printf("Device %d token=%d driveNumber=%d", i, iDeviceToken, driveNumber);
        }

    return iNumLuns;
    }


TLun CDevice::DeviceMap(TDeviceMap& aDeviceMap) const
    {
    __MSFNSLOG
    TDriveNumber driveNumber;
    RDebug::Printf("LuList.Count=%d", iLuList.Count());
    for (TInt i = 0; i < iLuList.Count(); i++)
        {
        driveNumber = iLuList[i];
        aDeviceMap[i] = driveNumber;
        RDebug::Printf("CDevice LUN=%d driveNumber=%d", i, driveNumber);
        }

    return iNumLuns;
    }


CUsbMsMountManager* CUsbMsMountManager::NewL()
    {
    __MSFNSLOG
	CUsbMsMountManager* r = new (ELeave) CUsbMsMountManager();
	CleanupStack::PushL(r);

	r->ConstructL();
	CleanupStack::Pop();
	return r;
    }


void CUsbMsMountManager::ConstructL()
    {
    __MSFNLOG
    }


CUsbMsMountManager::CUsbMsMountManager()
    {
    __MSFNLOG
    }


CUsbMsMountManager::~CUsbMsMountManager()
    {
    __MSFNLOG
    iDeviceList.ResetAndDestroy();
    }


// adds new entry for this device
void CUsbMsMountManager::AddDeviceL(CDevice* aDevice)
    {
    __MSFNLOG
    iDeviceList.Append(aDevice);
    }


CDevice* CUsbMsMountManager::RemoveDeviceL(TUint aDeviceHandle)
    {
    __MSFNLOG
	TInt index = GetHandleIndexL(aDeviceHandle);
	CDevice* device = iDeviceList[index];
    iDeviceList.Remove(index);
    return device;
    }

void CUsbMsMountManager::CloseAllDevicesL()
    {
    __MSFNLOG
    for (TInt i = 0; i < iDeviceList.Count(); i++)
        {
        iDeviceList[i]->CloseDeviceL();
        }
    }


TInt CUsbMsMountManager::GetDeviceIndexL(TToken aDeviceToken) const
    {
    __MSFNSLOG
    TInt index;
    for (index = 0; index < iDeviceList.Count(); index++)
        {
        if (aDeviceToken == iDeviceList[index]->DeviceToken())
            {
            break;
            }
        }

    if (index == iDeviceList.Count())
        {
        User::Leave(KErrNotFound);
        }

    return index;
    }


TInt CUsbMsMountManager::GetHandleIndexL(TUint aDeviceHandle) const
    {
    __MSFNSLOG
    TInt index;
    for (index = 0; index < iDeviceList.Count(); index++)
        {
        if (aDeviceHandle == iDeviceList[index]->DeviceHandle())
            {
            break;
            }
        }

    if (index == iDeviceList.Count())
        {
        User::Leave(KErrNotFound);
        }

    return index;
    }



// mounts all LUNs for the device
void CUsbMsMountManager::MountDeviceL(TUint aDeviceHandle)
    {
    __MSFNLOG
    TInt index = GetHandleIndexL(aDeviceHandle);
    iDeviceList[index]->MountLogicalUnitsL();
    }



// dismount all LUNs for this device
void CUsbMsMountManager::DismountDeviceL(TUint aDeviceHandle)
    {
    __MSFNLOG
    TInt index = GetHandleIndexL(aDeviceHandle);
    iDeviceList[index]->DismountLogicalUnitsL();
    }


// dismount all LUNs
void CUsbMsMountManager::DismountL()
    {
    __MSFNLOG
    for (TInt i = 0; i < iDeviceList.Count(); i++)
        {
        iDeviceList[i]->DismountLogicalUnitsL();
        }
    }


void CUsbMsMountManager::DriveMap(TDriveMap& aDriveMap) const
    {
    __MSFNSLOG
    TInt maxLun = 0;
    RDebug::Printf("DeviceList.Count=%d", iDeviceList.Count());
    for (TInt i = 0; i < iDeviceList.Count(); i++)
        {
        maxLun = iDeviceList[i]->DriveMap(aDriveMap);
        RDebug::Printf("%d %d", i, maxLun);
        }
    }


void CUsbMsMountManager::DeviceMap(TInt aDeviceIndex, TDeviceMap& aDeviceMap) const
    {
    __MSFNSLOG
    RDebug::Printf("Device=%d", aDeviceIndex);

    __ASSERT_DEBUG(aDeviceIndex < iDeviceList.Count(), User::Invariant());
    iDeviceList[aDeviceIndex]->DeviceMap(aDeviceMap);
    }



TUsbPrint::TUsbPrint()
:   iDebug(EFalse)
    {
    }

void TUsbPrint::PrintTree(const TUsbGenericDescriptor& aDesc, TInt aDepth)
	{
    if (!iDebug)
        {
        return;
        }

	TBuf<20> buf;
	for(TInt depth=aDepth;depth>=0;--depth)
		{
		buf.Append(_L("  "));
		}
	if(aDesc.iRecognisedAndParsed == TUsbGenericDescriptor::ERecognised)
		{
		RDebug::Print(_L("%S+0x%08x - %d 0x%02x"), &buf, &aDesc, aDesc.ibLength, aDesc.ibDescriptorType);
		}
	else
		{
		RDebug::Print(_L("%S-0x%08x - %d 0x%02x"), &buf, &aDesc, aDesc.ibLength, aDesc.ibDescriptorType);
		}
	HBufC* blob = HBufC::New(5*aDesc.iBlob.Length()); // 5* for " 0x" + 2*bytes for hex representation
	if(blob)
		{
		for(TInt i=0;i<aDesc.iBlob.Length();++i)
			{
			blob->Des().AppendFormat(_L("0x%02x "), aDesc.iBlob[i]);
			}
		RDebug::Print(_L("%S >%S"), &buf, blob);
		delete blob;
		}
	if(aDesc.iFirstChild)
		{
		RDebug::Print(_L("%S \\ "), &buf);
		PrintTree(*(aDesc.iFirstChild), aDepth+1);
		RDebug::Print(_L("%S / "), &buf);
		}
	if(aDesc.iNextPeer)
		{
		PrintTree(*(aDesc.iNextPeer), aDepth);
		}
	}


static TUint16 gLangId = 0x0000;

void TUsbPrint::SetLanguageToPrintL(RUsbDevice& aDevice)
	{
    if (!iDebug) return;

	// Try to set language to US Eng, otherwise take the first one listed.
	if(gLangId == 0x0000) // Only make the request if not been made before.
		{
		// Get string descriptor 0.
		TBuf8<256> stringBuf;
		TUsbStringDescriptor* stringDesc = NULL;
		User::LeaveIfError(aDevice.GetStringDescriptor(stringDesc, stringBuf, 0));
		CleanupStack::PushL(*stringDesc);

		// Search for US English
		TBool usEngLang = EFalse;
		TInt langId = 0;
		TInt index = 0;
		const TUint16 KLangIdUsEng = 0x0409;
		while(!usEngLang && langId != KErrNotFound)
			{
			langId = stringDesc->GetLangId(index);
			usEngLang = (langId == KLangIdUsEng);
			index++;
			}

		// Set the language appropriately
		if(usEngLang)
			{
        	gLangId = KLangIdUsEng;
			}
		else
			{
			gLangId = stringDesc->GetLangId(0);
			}

		CleanupStack::PopAndDestroy(); // stringDesc
		}
	}


void TUsbPrint::PrintStringFromIndex(const TDesC& aFormatString,
                                     TInt aIndex,
                                     RUsbDevice* aDevice)
	{
    if (!iDebug) return;

	// If we have no device handle, we cannot go and get any strings.
	// If we have index 0, this indicates we don't have a string for this entry.
	if(aDevice && aIndex != 0)
		{
		TRAPD(err, SetLanguageToPrintL(*aDevice));
		if(err == KErrNone)
			{
			TBuf8<255> stringBuf;
			TUsbStringDescriptor* stringDesc = NULL;
			err = aDevice->GetStringDescriptor(stringDesc, stringBuf, aIndex, gLangId);
			if(err == KErrNone)
				{
				TBuf<128> buf;
				stringDesc->StringData(buf);
				RDebug::Print(aFormatString, &buf);
				stringDesc->DestroyTree();
				}
			delete stringDesc;
			}
		else
			{
			RDebug::Print(_L("Error while Selecting Langauge %d\n"), err);
			}
		}
	}


void TUsbPrint::PrintDescriptor(const TUsbDeviceDescriptor& aDeviceDesc,
                                TInt /*aVariant*/,
                                RUsbDevice* aDevice)
	{
    if (!iDebug) return;

	RDebug::Print(_L("USBBcd = 0x%04x\n"), aDeviceDesc.USBBcd());
	RDebug::Print(_L("DeviceClass = 0x%02x\n"), aDeviceDesc.DeviceClass());
	RDebug::Print(_L("DeviceSubClass = 0x%02x\n"), aDeviceDesc.DeviceSubClass());
	RDebug::Print(_L("DeviceProtocol = 0x%02x\n"), aDeviceDesc.DeviceProtocol());
	RDebug::Print(_L("MaxPacketSize0 = 0x%02x\n"), aDeviceDesc.MaxPacketSize0());
	RDebug::Print(_L("VendorId = 0x%04x\n"), aDeviceDesc.VendorId());
	RDebug::Print(_L("ProductId = 0x%04x\n"), aDeviceDesc.ProductId());
	RDebug::Print(_L("DeviceBcd = 0x%04x\n"), aDeviceDesc.DeviceBcd());
	RDebug::Print(_L("ManufacturerIndex = 0x%02x\n"), aDeviceDesc.ManufacturerIndex());
	PrintStringFromIndex(_L("ManufacturerString = %S\n"), aDeviceDesc.ManufacturerIndex(), aDevice);
	RDebug::Print(_L("ProductIndex = 0x%02x\n"), aDeviceDesc.ProductIndex());
	PrintStringFromIndex(_L("ProductString = %S\n"), aDeviceDesc.ProductIndex(), aDevice);
	RDebug::Print(_L("SerialNumberIndex = 0x%02x\n"), aDeviceDesc.SerialNumberIndex());
	PrintStringFromIndex(_L("SerialNumberString = %S\n"), aDeviceDesc.SerialNumberIndex(), aDevice);
	RDebug::Print(_L("NumConfigurations = 0x%02x\n"), aDeviceDesc.NumConfigurations());
	}


void TUsbPrint::PrintDescriptor(const TUsbConfigurationDescriptor& aConfigDesc,
                                TInt /*aVariant*/,
                                RUsbDevice* aDevice)
	{
    if (!iDebug) return;
	RDebug::Print(_L("TotalLength = 0x%04x\n"), aConfigDesc.TotalLength());
	RDebug::Print(_L("NumInterfaces = 0x%02x\n"), aConfigDesc.NumInterfaces());
	RDebug::Print(_L("ConfigurationValue = 0x%02x\n"), aConfigDesc.ConfigurationValue());
	RDebug::Print(_L("ConfigurationIndex = 0x%02x\n"), aConfigDesc.ConfigurationIndex());
	PrintStringFromIndex(_L("ConfigurationString = %S\n"), aConfigDesc.ConfigurationIndex(), aDevice);
	RDebug::Print(_L("Attributes = 0x%02x\n"), aConfigDesc.Attributes());
	RDebug::Print(_L("MaxPower = 0x%02x\n"), aConfigDesc.MaxPower());
	}


void TUsbPrint::PrintDescriptor(const TUsbEndpointDescriptor& aEndpointDesc,
                                TInt /*aVariant*/,
                                RUsbDevice* /*aDevice*/)
	{
    if (!iDebug) return;
	RDebug::Print(_L("EndpointAddress = 0x%02x\n"), aEndpointDesc.EndpointAddress());
	RDebug::Print(_L("Attributes = 0x%02x\n"), aEndpointDesc.Attributes());
	RDebug::Print(_L("MaxPacketSize = 0x%04x\n"), aEndpointDesc.MaxPacketSize());
	RDebug::Print(_L("Interval = 0x%02x\n"), aEndpointDesc.Interval());
	}


void TUsbPrint::PrintDescriptor(const TUsbInterfaceDescriptor& aInterfaceDesc,
                                TInt /*aVariant*/,
                                RUsbDevice* /*aDevice*/)
	{
    if (!iDebug) return;
	RDebug::Print(_L("InterfaceNumber = 0x%02x\n"), aInterfaceDesc.InterfaceNumber());
	RDebug::Print(_L("AlternateSetting = 0x%02x\n"), aInterfaceDesc.AlternateSetting());
	RDebug::Print(_L("NumEndpoints = 0x%02x\n"), aInterfaceDesc.NumEndpoints());
	RDebug::Print(_L("InterfaceClass = 0x%02x\n"), aInterfaceDesc.InterfaceClass());
	RDebug::Print(_L("InterfaceSubClass = 0x%02x\n"), aInterfaceDesc.InterfaceSubClass());
	RDebug::Print(_L("InterfaceProtocol = 0x%02x\n"), aInterfaceDesc.InterfaceProtocol());
	RDebug::Print(_L("Interface = 0x%02x\n"), aInterfaceDesc.Interface());
	}


void TUsbPrint::PrintDescriptor(const TUsbStringDescriptor& aStringDesc,
                                TInt aVariant,
                                RUsbDevice* /*aDevice*/)
	{
    if (!iDebug) return;
	if(aVariant == 0)
		{
		RDebug::Print(_L("String Descriptor Zero\n"));
		TInt index = 0;
		TInt langId = 0;
		while((langId = aStringDesc.GetLangId(index)) != KErrNotFound)
			{
			RDebug::Print(_L("  >0x%04x\n"), langId);
			++index;
			}
		}
	else
		{
		RDebug::Print(_L("Generic String Descriptor\n"));
		HBufC16* string = HBufC16::New(128);
		if(string)
			{
			TPtr16 stringPtr = string->Des();
			aStringDesc.StringData(stringPtr);
			RDebug::Print(_L("  >%S\n"), string);
			}
		delete string;
		}
	}
