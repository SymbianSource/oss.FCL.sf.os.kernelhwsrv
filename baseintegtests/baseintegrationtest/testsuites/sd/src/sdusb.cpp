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
// Enables USB mass storage mode. Ends when disconnected by user.
// 
//

#include <massstorage.h>
#include <d32usbc.h>
#include "sdusb.h"

/*
Class constructor

@param None
@return None
*/
CBaseTestSDUsb::CBaseTestSDUsb()
	{
	SetTestStepName(KTestStepUsb);
	}

/*
Test Step Preamble
 - Initialise attribute iDrive
 - Connect to the File Server

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDUsb::doTestStepPreambleL()
	{
	SetTestStepResult(EFail);
	
	if (!InitDriveLetter())
		return TestStepResult();
	if (!InitFileServer())
		return TestStepResult();

	SetTestStepResult(EPass);
	return TestStepResult();
	}

/*
Test step

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDUsb::doTestStepL()
	{
	TInt r;
	_LIT(KMsFsy, "MSFS.FSY");
	_LIT(KMsFs, "MassStorageFileSystem");
	
	// Add MS file system	
	r = iFs.AddFileSystem(KMsFsy);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		ERR_PRINTF2(_L("AddFileSystem failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	RDevUsbcClient usb;

	// Load the logical device
	_LIT(KDriverFileName,"EUSBC.LDD");
	r = User::LoadLogicalDevice(KDriverFileName);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		ERR_PRINTF2(_L("LoadLogicalDevice failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	r = usb.Open(0);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RDevUsbcClient::Open failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	RUsbMassStorage UsbMs;
	TBuf<8>  t_vendorId(_L("vendor"));
	TBuf<16> t_productId(_L("product"));
	TBuf<4>  t_productRev(_L("1.00"));

	TMassStorageConfig msConfig;
	msConfig.iVendorId.Copy(t_vendorId);
	msConfig.iProductId.Copy(t_productId);
	msConfig.iProductRev.Copy(t_productRev);

	// Connect to Mass Storage
	r = UsbMs.Connect();
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RUsbMassStorage::Connect failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	// Start Mass Storage
	r = UsbMs.Start(msConfig);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RUsbMassStorage::Start failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	TBuf8<KUsbDescSize_Device> deviceDescriptor;
	r = usb.GetDeviceDescriptor(deviceDescriptor);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RDevUsbcClient::GetDeviceDescriptor failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	const TInt KUsbSpecOffset = 2;
	const TInt KUsbDeviceClassOffset = 4;
	const TInt KUsbVendorIdOffset = 8;
	const TInt KUsbProductIdOffset = 10;
	const TInt KUsbDevReleaseOffset = 12;
	//Change the USB spec number to 2.00
	deviceDescriptor[KUsbSpecOffset]   = 0x00;
	deviceDescriptor[KUsbSpecOffset+1] = 0x02;
	//Change the Device Class, Device SubClass and Device Protocol 
	deviceDescriptor[KUsbDeviceClassOffset] = 0x00;
	deviceDescriptor[KUsbDeviceClassOffset+1] = 0x00;
	deviceDescriptor[KUsbDeviceClassOffset+2] = 0x00;
	//Change the device vendor ID (VID) to 0x0E22 (Symbian)
	deviceDescriptor[KUsbVendorIdOffset]   = 0x22;   // little endian
	deviceDescriptor[KUsbVendorIdOffset+1] = 0x0E;
	//Change the device product ID (PID) to 0x1111
	deviceDescriptor[KUsbProductIdOffset]   = 0x12;
	deviceDescriptor[KUsbProductIdOffset+1] = 0x11;
	//Change the device release number to 3.05
	deviceDescriptor[KUsbDevReleaseOffset]   = 0x05;
	deviceDescriptor[KUsbDevReleaseOffset+1] = 0x03;
	r = usb.SetDeviceDescriptor(deviceDescriptor);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RDevUsbcClient::SetDeviceDescriptor failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}


	_LIT16(productID_L, "Symbian USB Mass Storage Device");
	TBuf16<KUsbStringDescStringMaxSize / 2> productID(productID_L);
	// Set product string descriptor
	r = usb.SetProductStringDescriptor(productID);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RDevUsbcClient::SetProductStringDescriptor failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	TRequestStatus enum_status;
	INFO_PRINTF1(_L("Re-enumerating..."));
	usb.ReEnumerate(enum_status);
	User::WaitForRequest(enum_status);
   	INFO_PRINTF1(_L("Re-enumerating done"));
   	
   	// Mount Mass Storage FS
    r = iFs.DismountFileSystem(_L("fat"), iDrive);
    if (r != KErrNone)
   		{
		ERR_PRINTF2(_L("RFs::DismountFileSystem (FAT) failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	r = iFs.MountFileSystem(KMsFs, iDrive);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RFs::MountFileSystem (MSFS) failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	// Wait until the USB cable is removed or device is suspended
	TUsbcDeviceState initialStatus;
	r = usb.DeviceStatus(initialStatus);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RDevUsbcClient::DeviceStatus failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	if (initialStatus == EUsbcDeviceStateUndefined)
		{
		ERR_PRINTF1(_L("USB device status is undefined"));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	TUint deviceState = initialStatus;
	do {
		TRequestStatus rs;
		usb.AlternateDeviceStatusNotify(rs, deviceState);
		User::WaitForRequest(rs);
		} while (deviceState != EUsbcDeviceStateUndefined && deviceState != EUsbcDeviceStateSuspended);

	// Dismount Mass Storage FS
    r = iFs.DismountFileSystem(KMsFs, iDrive);
    if (r != KErrNone)
   		{
		ERR_PRINTF2(_L("RFs::DismountFileSystem (MSFS) failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	r = iFs.MountFileSystem(_L("fat"), iDrive);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RFs::MountFileSystem (FAT) failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	// Stop USB Mass Storage
	r = UsbMs.Stop();
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RUsbMassStorage::Stop failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	UsbMs.Close();
	usb.Close();
	r = iFs.RemoveFileSystem(KMsFs);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RFs::RemoveFileSystem failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	r = User::FreeLogicalDevice(_L("USBC"));
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("FreeLogicalDevice failed: %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	SetTestStepResult(EPass);
	return TestStepResult();
	}
