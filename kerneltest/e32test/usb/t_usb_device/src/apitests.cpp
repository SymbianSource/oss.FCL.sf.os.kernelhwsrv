// Copyright (c) 2000-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/usb/t_usb_device/src/apitests.cpp
// USB Test Program T_USB_DEVICE, functional part.
// Device-side part, to work against T_USB_HOST running on the host.
//
//

#include "general.h"									// CActiveControl, CActiveRW
#include "config.h"
#include "usblib.h"										// Helpers
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "apitestsTraces.h"
#endif


extern RTest test;
extern TBool gVerbose;
extern TBool gSkip;
extern TBool gTempTest;

_LIT16(KString_one, "Arbitrary String Descriptor Test String 1");
_LIT16(KString_two, "Another Arbitrary String Descriptor Test String");

void SetupDescriptors(LDDConfigPtr aLddPtr,RDEVCLIENT* aPort, TUint16 aPid = 0)
	{
	// === Device Descriptor
	test.Start(_L("Set up descriptors"));

	test.Next(_L("GetDeviceDescriptorSize"));
	TInt deviceDescriptorSize = 0;
	aPort->GetDeviceDescriptorSize(deviceDescriptorSize);
	test_Equal(KUsbDescSize_Device,static_cast<TUint>(deviceDescriptorSize));

	test.Next(_L("GetDeviceDescriptor"));
	TBuf8<KUsbDescSize_Device> deviceDescriptor;
	TInt r = aPort->GetDeviceDescriptor(deviceDescriptor);
	test_KErrNone(r);

	test.Next(_L("SetDeviceDescriptor"));
	const TInt KUsbSpecOffset = 2;
	const TInt KUsbDevClass = 4;
	const TInt KUsbDevSubClass = 5;
	const TInt KUsbDevProtocol = 6;
	const TInt KUsbVendorIdOffset = 8;
	const TInt KUsbProductIdOffset = 10;
	const TInt KUsbDevReleaseOffset = 12;
	// Change the USB spec number
	deviceDescriptor[KUsbSpecOffset]   = LoByte(aLddPtr->iSpec);
	deviceDescriptor[KUsbSpecOffset+1] = HiByte(aLddPtr->iSpec);
	// Change the Device Class, SubClass and Protocol to zero so that they are not device specific
	// and diferent clases can used on different interfaces
	deviceDescriptor[KUsbDevClass] = 0;
	deviceDescriptor[KUsbDevSubClass] = 0;
	deviceDescriptor[KUsbDevProtocol] = 0;
	// Change the device vendor ID (VID)
	deviceDescriptor[KUsbVendorIdOffset]   = LoByte(aLddPtr->iVid);			// little endian!
	deviceDescriptor[KUsbVendorIdOffset+1] = HiByte(aLddPtr->iVid);
	// Change the device product ID (PID)
	if (aPid != 0)
		{
		deviceDescriptor[KUsbProductIdOffset]	= LoByte(aPid);		// little endian!
		deviceDescriptor[KUsbProductIdOffset+1] = HiByte(aPid);
		}
	else
		{
		deviceDescriptor[KUsbProductIdOffset]	= LoByte(aLddPtr->iPid);		// little endian!
		deviceDescriptor[KUsbProductIdOffset+1] = HiByte(aLddPtr->iPid);
		}
	// Change the device release number
	deviceDescriptor[KUsbDevReleaseOffset]	 = LoByte(aLddPtr->iRelease);	// little endian!
	deviceDescriptor[KUsbDevReleaseOffset+1] = HiByte(aLddPtr->iRelease);
	r = aPort->SetDeviceDescriptor(deviceDescriptor);
	test_KErrNone(r);

	if (!gSkip)
		{
		test.Next(_L("GetDeviceDescriptor()"));
		TBuf8<KUsbDescSize_Device> descriptor2;
		r = aPort->GetDeviceDescriptor(descriptor2);
		test_KErrNone(r);

		test.Next(_L("Compare device descriptor with value set"));
		r = descriptor2.Compare(deviceDescriptor);
		test_KErrNone(r);
		}

	// === Configuration Descriptor

	test.Next(_L("GetConfigurationDescriptorSize"));
	TInt configDescriptorSize = 0;
	aPort->GetConfigurationDescriptorSize(configDescriptorSize);
	test_Equal(KUsbDescSize_Config,static_cast<TUint>(configDescriptorSize));

	test.Next(_L("GetConfigurationDescriptor"));
	TBuf8<KUsbDescSize_Config> configDescriptor;
	r = aPort->GetConfigurationDescriptor(configDescriptor);
	test_KErrNone(r);

	test.Next(_L("SetConfigurationDescriptor"));
	// Change Self Power and Remote Wakeup
	const TInt KUsbAttributesOffset = 7;
	const TUint8 KUsbAttributeDefault = 0x80;
	const TUint8 KUsbAttributeSelfPower = 0x40;
	const TUint8 KUsbAttributeRemoteWakeup = 0x20;
	configDescriptor[KUsbAttributesOffset] = KUsbAttributeDefault | (aLddPtr->iSelfPower ? KUsbAttributeSelfPower : 0)
													| (aLddPtr->iRemoteWakeup ? KUsbAttributeRemoteWakeup : 0);
	// Change the reported max power
	// 100mA (= 2 * 0x32) is the highest value allowed for a bus-powered device.
	const TInt KUsbMaxPowerOffset = 8;
	configDescriptor[KUsbMaxPowerOffset] = aLddPtr->iMaxPower;
	r = aPort->SetConfigurationDescriptor(configDescriptor);
	test_KErrNone(r);

	if (!gSkip)
		{
		test.Next(_L("GetConfigurationDescriptor()"));
		TBuf8<KUsbDescSize_Config> descriptor2;
		r = aPort->GetConfigurationDescriptor(descriptor2);
		test_KErrNone(r);

		test.Next(_L("Compare configuration desc with value set"));
		r = descriptor2.Compare(configDescriptor);
		test_KErrNone(r);
		}

	// === String Descriptors

	test.Next(_L("SetStringDescriptor"));

	// Set up any standard string descriptors that were defined in the xml config
	if (aLddPtr->iManufacturer)
		{
		r = aPort->SetManufacturerStringDescriptor(* aLddPtr->iManufacturer);
		test_KErrNone(r);
		}

	if (aLddPtr->iProduct)
		{
		r = aPort->SetProductStringDescriptor(* aLddPtr->iProduct);
		test_KErrNone(r);
		}

	if (aLddPtr->iSerialNumber)
		{
		r = aPort->SetSerialNumberStringDescriptor(* aLddPtr->iSerialNumber);
		test_KErrNone(r);
		}

	// Set up two arbitrary string descriptors, which can be queried
	// manually from the host side for testing purposes

	TBuf16<KUsbStringDescStringMaxSize / 2> wr_str(KString_one);
	r = aPort->SetStringDescriptor(stridx1, wr_str);
	test_KErrNone(r);

	wr_str.FillZ(wr_str.MaxLength());
	wr_str = KString_two;
	r = aPort->SetStringDescriptor(stridx2, wr_str);
	test_KErrNone(r);

	test.End();

	}

static void TestDeviceQualifierDescriptor(RDEVCLIENT* aPort)
	{
	test.Start(_L("Device_Qualifier Descriptor Manipulation"));

	test.Next(_L("GetDeviceQualifierDescriptor()"));
	TBuf8<KUsbDescSize_DeviceQualifier> descriptor;
	TInt r = aPort->GetDeviceQualifierDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("SetDeviceQualifierDescriptor()"));
	// Change the USB spec number to 3.00
	descriptor[KDevDesc_SpecOffset]   = 0x00;
	descriptor[KDevDesc_SpecOffset+1] = 0x03;
	// Change the device class, subclass and protocol codes
	descriptor[KDevDesc_DevClassOffset]    = 0xA1;
	descriptor[KDevDesc_DevSubClassOffset] = 0xB2;
	descriptor[KDevDesc_DevProtocolOffset] = 0xC3;
	r = aPort->SetDeviceQualifierDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("GetDeviceQualifierDescriptor()"));
	TBuf8<KUsbDescSize_DeviceQualifier> descriptor2;
	r = aPort->GetDeviceQualifierDescriptor(descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare Device_Qualifier desc with value set"));
	r = descriptor2.Compare(descriptor);
	test_Equal(0,r);

	test.End();
	}



static void	TestOtherSpeedConfigurationDescriptor(RDEVCLIENT* aPort)
	{
	test.Start(_L("Other_Speed_Configuration Desc Manipulation"));

	test.Next(_L("GetOtherSpeedConfigurationDescriptor()"));
	TBuf8<KUsbDescSize_OtherSpeedConfig> descriptor;
	TInt r = aPort->GetOtherSpeedConfigurationDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("SetOtherSpeedConfigurationDescriptor()"));
	// Invert Remote-Wakup support
	descriptor[KConfDesc_AttribOffset] = (descriptor[KConfDesc_AttribOffset] ^ KUsbDevAttr_RemoteWakeup);
	// Change the reported max power to 330mA (2 * 0xA5)
	descriptor[KConfDesc_MaxPowerOffset] = 0xA5;
	r = aPort->SetOtherSpeedConfigurationDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("GetOtherSpeedConfigurationDescriptor()"));
	TBuf8<KUsbDescSize_OtherSpeedConfig> descriptor2;
	r = aPort->GetOtherSpeedConfigurationDescriptor(descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare O_S_Config desc with value set"));
	r = descriptor2.Compare(descriptor);
	test_KErrNone(r);

	test.End();
	}


static void TestInterfaceDescriptor(RDEVCLIENT* aPort, TInt aNumSettings)
	{
	test.Start(_L("Interface Descriptor Manipulation"));

	// For all settings

	TInt desc_size = 0;
	TInt r = 0;
	TBuf8<KUsbDescSize_Interface> descriptor;
	TBuf8<KUsbDescSize_Interface> descriptor2;
	for (TInt i =0; i < aNumSettings; i++)
		{

		test.Next(_L("GetInterfaceDescriptorSize()"));
		r = aPort->GetInterfaceDescriptorSize(i, desc_size);
		if (r != KErrNone)
			{
			OstTraceExt2(TRACE_NORMAL, TESTINTERFACEDESCRIPTOR_TESTINTERFACEDESCRIPTOR, "Error %d in GetInterfaceDescriptorSize %d\n",r,i);
			}
		test_KErrNone(r);
		test_Equal(KUsbDescSize_Interface,static_cast<TUint>(desc_size));

		test.Next(_L("GetInterfaceDescriptor()"));
		r = aPort->GetInterfaceDescriptor(i, descriptor);
		test_KErrNone(r);

		test.Next(_L("SetInterfaceDescriptor()"));
		// Change the interface protocol to 0x78(+)
		TUint8 prot = 0x78;
		if (descriptor[KIfcDesc_ProtocolOffset] == prot)
			prot++;
		descriptor[KIfcDesc_ProtocolOffset] = prot;
		r = aPort->SetInterfaceDescriptor(i, descriptor);
		test_KErrNone(r);

		test.Next(_L("GetInterfaceDescriptor()"));
		r = aPort->GetInterfaceDescriptor(i, descriptor2);
		test_KErrNone(r);

		test.Next(_L("Compare interface descriptor with value set"));
		r = descriptor2.Compare(descriptor);
		test_KErrNone(r);
		}

	test.Next(_L("GetInterfaceDescriptor()"));
	r = aPort->GetInterfaceDescriptor(aNumSettings, descriptor);
	test_Equal(KErrNotFound,r);

	test.Next(_L("SetInterfaceDescriptor()"));
	r = aPort->SetInterfaceDescriptor(aNumSettings, descriptor);
	test_Equal(KErrNotFound,r);

	test.End();
	}


static void TestClassSpecificDescriptors(RDEVCLIENT* aPort)
	{
	test.Start(_L("Class-specific Descriptor Manipulation"));

	// First a class-specific Interface descriptor

	test.Next(_L("SetCSInterfaceDescriptorBlock()"));
	// choose arbitrary new descriptor size
	const TInt KUsbDescSize_CS_Interface = KUsbDescSize_Interface + 10;
	TBuf8<KUsbDescSize_CS_Interface> cs_ifc_descriptor;
	cs_ifc_descriptor.FillZ(cs_ifc_descriptor.MaxLength());
	cs_ifc_descriptor[KUsbDesc_SizeOffset] = KUsbDescSize_CS_Interface;
	cs_ifc_descriptor[KUsbDesc_TypeOffset] = KUsbDescType_CS_Interface;
	TInt r = aPort->SetCSInterfaceDescriptorBlock(0, cs_ifc_descriptor);
	test_KErrNone(r);

	test.Next(_L("GetCSInterfaceDescriptorBlockSize()"));
	TInt desc_size = 0;
	r = aPort->GetCSInterfaceDescriptorBlockSize(0, desc_size);
	test_KErrNone(r);
	test_Equal(KUsbDescSize_CS_Interface,desc_size);

	test.Next(_L("GetCSInterfaceDescriptorBlock()"));
	TBuf8<KUsbDescSize_CS_Interface> descriptor;
	r = aPort->GetCSInterfaceDescriptorBlock(0, descriptor);
	test_KErrNone(r);

	test.Next(_L("Compare CS ifc descriptor with value set"));
	r = descriptor.Compare(cs_ifc_descriptor);
	test_KErrNone(r);

	// Next a class-specific Endpoint descriptor

	test.Next(_L("SetCSEndpointDescriptorBlock()"));
	// choose arbitrary new descriptor size
	const TInt KUsbDescSize_CS_Endpoint = KUsbDescSize_Endpoint + 5;
	TBuf8<KUsbDescSize_CS_Endpoint> cs_ep_descriptor;
	cs_ep_descriptor.FillZ(cs_ep_descriptor.MaxLength());
	cs_ep_descriptor[KUsbDesc_SizeOffset] = KUsbDescSize_CS_Endpoint;
	cs_ep_descriptor[KUsbDesc_TypeOffset] = KUsbDescType_CS_Endpoint;
	r = aPort->SetCSEndpointDescriptorBlock(0, 2, cs_ep_descriptor);
	test_KErrNone(r);

	test.Next(_L("GetCSEndpointDescriptorBlockSize()"));
	r = aPort->GetCSEndpointDescriptorBlockSize(0, 2, desc_size);
	test_KErrNone(r);
	test_Equal(KUsbDescSize_CS_Endpoint,desc_size);

	test.Next(_L("GetCSEndpointDescriptorBlock()"));
	TBuf8<KUsbDescSize_CS_Endpoint> descriptor2;
	r = aPort->GetCSEndpointDescriptorBlock(0, 2, descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare CS ep descriptor with value set"));
	r = descriptor2.Compare(cs_ep_descriptor);
	test_KErrNone(r);

	test.End();
	}


void TestEndpointDescriptor(RDEVCLIENT* aPort,TInt aIfSetting, TInt aEpNumber,TUsbcEndpointInfo aEpInfo)
	{
	test.Start(_L("Endpoint Descriptor Manipulation"));

	TBuf8<KUsbDescSize_AudioEndpoint> epDescriptor;
	TInt desc_size;
	TInt r = aPort->GetEndpointDescriptorSize(aIfSetting, aEpNumber, desc_size);
	test_KErrNone(r);
	test_Equal(KUsbDescSize_Endpoint + aEpInfo.iExtra,static_cast<TUint>(desc_size));

	r = aPort->GetEndpointDescriptor(aIfSetting, aEpNumber, epDescriptor);
	test_KErrNone(r);

	test(((aEpInfo.iDir & KUsbEpDirIn) && (epDescriptor[KEpDesc_AddressOffset] & 0x80) ||
		!(aEpInfo.iDir & KUsbEpDirIn) && !(epDescriptor[KEpDesc_AddressOffset] & 0x80)) &&
			EpTypeMask2Value(aEpInfo.iType) == (TUint)(epDescriptor[KEpDesc_AttributesOffset] & 0x03) &&
			aEpInfo.iInterval == epDescriptor[KEpDesc_IntervalOffset]);

	// Change the endpoint poll interval
	TUint8 ival = 0x66;
	if (epDescriptor[KEpDesc_IntervalOffset] == ival)
		ival++;


	TUint8 saveAddr = 0;										// save the address
	if (aEpInfo.iExtra > 0)
		{
		saveAddr = epDescriptor[KEpDesc_SynchAddressOffset];
		TUint8 addr = 0x85;										// bogus address
		if (epDescriptor[KEpDesc_SynchAddressOffset] == addr)
			addr++;
		epDescriptor[KEpDesc_SynchAddressOffset] = addr;
		}

	epDescriptor[KEpDesc_IntervalOffset] = ival;
	r = aPort->SetEndpointDescriptor(aIfSetting, aEpNumber, epDescriptor);
	test_KErrNone(r);

	TBuf8<KUsbDescSize_AudioEndpoint> descriptor2;
	r = aPort->GetEndpointDescriptor(aIfSetting, aEpNumber, descriptor2);
	test_KErrNone(r);

	r = descriptor2.Compare(epDescriptor);
	test_KErrNone(r);

	if (aEpInfo.iExtra > 0)
		{
		// Restore the endpoint synch address
		epDescriptor[KEpDesc_SynchAddressOffset] = saveAddr;
		}

	// Restore the endpoint poll interval
	epDescriptor[KEpDesc_IntervalOffset] = aEpInfo.iInterval;
	r = aPort->SetEndpointDescriptor(aIfSetting, aEpNumber, epDescriptor);
	test_KErrNone(r);

	test.End();
	}

static void TestStandardStringDescriptors(RDEVCLIENT* aPort)
	{
	test.Start(_L("String Descriptor Manipulation"));

	//
	// --- LANGID code
	//

	test.Next(_L("GetStringDescriptorLangId()"));
	TUint16 rd_langid_orig;
	TInt r = aPort->GetStringDescriptorLangId(rd_langid_orig);
	test_KErrNone(r);
	test.Printf(_L("Original LANGID code: 0x%04X\n"), rd_langid_orig);
	OstTrace1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS, "Original LANGID code: 0x%04X\n", rd_langid_orig);

	test.Next(_L("SetStringDescriptorLangId()"));
	TUint16 wr_langid = 0x0809;								// English (UK) Language ID
	if (wr_langid == rd_langid_orig)
		wr_langid = 0x0444;									// Tatar Language ID
	r = aPort->SetStringDescriptorLangId(wr_langid);
	test_KErrNone(r);

	test.Next(_L("GetStringDescriptorLangId()"));
	TUint16 rd_langid;
	r = aPort->GetStringDescriptorLangId(rd_langid);
	test_KErrNone(r);
	test.Printf(_L("New LANGID code: 0x%04X\n"), rd_langid);
	OstTrace1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP01, "New LANGID code: 0x%04X\n", rd_langid);

	test.Next(_L("Compare LANGID codes"));
	test_Equal(wr_langid,rd_langid);

	test.Next(_L("Restore original LANGID code"));
	r = aPort->SetStringDescriptorLangId(rd_langid_orig);
	test_KErrNone(r);
	r = aPort->GetStringDescriptorLangId(rd_langid);
	test_KErrNone(r);
	test_Equal(rd_langid_orig,rd_langid);

	//
	// --- Manufacturer string
	//

	test.Next(_L("GetManufacturerStringDescriptor()"));
	TBuf16<KUsbStringDescStringMaxSize / 2> rd_str_orig;
	r = aPort->GetManufacturerStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	TBool restore_string;
	if (r == KErrNone)
		{
		test.Printf(_L("Original Manufacturer string: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP02, "Original Manufacturer string: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		{
		test.Printf(_L("No Manufacturer string set\n"));
		OstTrace0(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP03, "No Manufacturer string set\n");
		restore_string = EFalse;
		}

	test.Next(_L("SetManufacturerStringDescriptor()"));
	_LIT16(manufacturer, "Manufacturer Which Manufactures Devices");
	TBuf16<KUsbStringDescStringMaxSize / 2> wr_str(manufacturer);
	r = aPort->SetManufacturerStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetManufacturerStringDescriptor()"));
	TBuf16<KUsbStringDescStringMaxSize / 2> rd_str;
	r = aPort->GetManufacturerStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Manufacturer string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP04, "New Manufacturer string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Manufacturer strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("SetManufacturerStringDescriptor()"));
	_LIT16(manufacturer2, "Different Manufacturer Which Manufactures Different Devices");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = manufacturer2;
	r = aPort->SetManufacturerStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetManufacturerStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = aPort->GetManufacturerStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Manufacturer string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP05, "New Manufacturer string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Manufacturer strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("RemoveManufacturerStringDescriptor()"));
	r = aPort->RemoveManufacturerStringDescriptor();
	test_KErrNone(r);
	r = aPort->GetManufacturerStringDescriptor(rd_str);
	test_Equal(KErrNotFound,r);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = aPort->SetManufacturerStringDescriptor(rd_str_orig);
		test_KErrNone(r);
		r = aPort->GetManufacturerStringDescriptor(rd_str);
		test_KErrNone(r);
		r = rd_str.Compare(rd_str_orig);
		test_KErrNone(r);
		}

	//
	// --- Product string
	//

	test.Next(_L("GetProductStringDescriptor()"));
	rd_str_orig.FillZ(rd_str.MaxLength());
	r = aPort->GetProductStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	if (r == KErrNone)
		{
		test.Printf(_L("Old Product string: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP06, "Old Product string: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		restore_string = EFalse;

	test.Next(_L("SetProductStringDescriptor()"));
	_LIT16(product, "Product That Was Produced By A Manufacturer");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = product;
	r = aPort->SetProductStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetProductStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = aPort->GetProductStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Product string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP07, "New Product string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Product strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("SetProductStringDescriptor()"));
	_LIT16(product2, "Different Product That Was Produced By A Different Manufacturer");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = product2;
	r = aPort->SetProductStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetProductStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = aPort->GetProductStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Product string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP08, "New Product string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Product strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("RemoveProductStringDescriptor()"));
	r = aPort->RemoveProductStringDescriptor();
	test_KErrNone(r);
	r = aPort->GetProductStringDescriptor(rd_str);
	test_Equal(KErrNotFound,r);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = aPort->SetProductStringDescriptor(rd_str_orig);
		test_KErrNone(r);
		r = aPort->GetProductStringDescriptor(rd_str);
		test_KErrNone(r);
		r = rd_str.Compare(rd_str_orig);
		test_KErrNone(r);
		}

	//
	// --- Serial Number string
	//

	test.Next(_L("GetSerialNumberStringDescriptor()"));
	rd_str_orig.FillZ(rd_str.MaxLength());
	r = aPort->GetSerialNumberStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	if (r == KErrNone)
		{
		test.Printf(_L("Old Serial Number: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP09, "Old Serial Number: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		restore_string = EFalse;

	test.Next(_L("SetSerialNumberStringDescriptor()"));
	_LIT16(serial, "000666000XYZ");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = serial;
	r = aPort->SetSerialNumberStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetSerialNumberStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = aPort->GetSerialNumberStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Serial Number: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP10, "New Serial Number: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Serial Number strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("SetSerialNumberStringDescriptor()"));
	_LIT16(serial2, "Y11611193111711111Y");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = serial2;
	r = aPort->SetSerialNumberStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetSerialNumberStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = aPort->GetSerialNumberStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Serial Number: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP11, "New Serial Number: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Serial Number strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("RemoveSerialNumberStringDescriptor()"));
	r = aPort->RemoveSerialNumberStringDescriptor();
	test_KErrNone(r);
	r = aPort->GetSerialNumberStringDescriptor(rd_str);
	test_Equal(KErrNotFound,r);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = aPort->SetSerialNumberStringDescriptor(rd_str_orig);
		test_KErrNone(r);
		r = aPort->GetSerialNumberStringDescriptor(rd_str);
		test_KErrNone(r);
		r = rd_str.Compare(rd_str_orig);
		test_KErrNone(r);
		}

	//
	// --- Configuration string
	//

	test.Next(_L("GetConfigurationStringDescriptor()"));
	rd_str_orig.FillZ(rd_str.MaxLength());
	r = aPort->GetConfigurationStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	if (r == KErrNone)
		{
		test.Printf(_L("Old Configuration string: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP12, "Old Configuration string: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		restore_string = EFalse;

	test.Next(_L("SetConfigurationStringDescriptor()"));
	_LIT16(config, "Relatively Simple Configuration That Is Still Useful");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = config;
	r = aPort->SetConfigurationStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetConfigurationStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = aPort->GetConfigurationStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Configuration string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP13, "New Configuration string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Configuration strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("SetConfigurationStringDescriptor()"));
	_LIT16(config2, "Convenient Configuration That Can Be Very Confusing");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = config2;
	r = aPort->SetConfigurationStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetConfigurationStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = aPort->GetConfigurationStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Configuration string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP14, "New Configuration string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Configuration strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("RemoveConfigurationStringDescriptor()"));
	r = aPort->RemoveConfigurationStringDescriptor();
	test_KErrNone(r);
	r = aPort->GetConfigurationStringDescriptor(rd_str);
	test_Equal(KErrNotFound,r);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = aPort->SetConfigurationStringDescriptor(rd_str_orig);
		test_KErrNone(r);
		r = aPort->GetConfigurationStringDescriptor(rd_str);
		test_KErrNone(r);
		r = rd_str.Compare(rd_str_orig);
		test_KErrNone(r);
		}

	test.End();
	}


static void TestArbitraryStringDescriptors(RDEVCLIENT* aPort,TInt aNumSettings)
	{
	test.Start(_L("Arbitrary String Descriptor Manipulation"));

	// First test string

	test.Next(_L("GetStringDescriptor() 1"));
	TBuf16<KUsbStringDescStringMaxSize / 2> rd_str;
	TInt r = aPort->GetStringDescriptor(stridx1, rd_str);
	test_KErrNone(r);

	TBuf16<KUsbStringDescStringMaxSize / 2> wr_str(KString_one);
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	// Second test string

	test.Next(_L("GetStringDescriptor() 2"));
	rd_str.FillZ(rd_str.MaxLength());
	r = aPort->GetStringDescriptor(stridx2, rd_str);
	test_KErrNone(r);

	wr_str = KString_two;
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	// Third test string

	test.Next(_L("GetStringDescriptor() 3"));
	rd_str.FillZ(rd_str.MaxLength());
	r = aPort->GetStringDescriptor(stridx3, rd_str);
	test_Equal(KErrNotFound,r);

	test.Next(_L("SetStringDescriptor() 3"));
	_LIT16(string_three, "Arbitrary String Descriptor Test String 3");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = string_three;
	r = aPort->SetStringDescriptor(stridx3, wr_str);
	test_KErrNone(r);

	// In between we create another interface setting to see what happens
	// to the existing string descriptor indices.
	// (We don't have to test this on every platform -
	// besides, those that don't support alt settings
	// are by now very rare.)
	if (SupportsAlternateInterfaces())
		{
		#ifdef USB_SC
		TUsbcScInterfaceInfoBuf ifc;
		#else
		TUsbcInterfaceInfoBuf ifc;
		#endif
		_LIT16(string, "T_USB_DEVICE Bogus Test Interface (Next Setting)");
		ifc().iString = const_cast<TDesC16*>(&string);
		ifc().iTotalEndpointsUsed = 0;
		TInt r = aPort->SetInterface(aNumSettings, ifc);
		test_KErrNone(r);
		}

	test.Next(_L("GetStringDescriptor() 3"));
	r = aPort->GetStringDescriptor(stridx3, rd_str);
	test_KErrNone(r);
	test.Printf(_L("New test string @ idx %d: \"%lS\"\n"), stridx3, &rd_str);
	OstTraceExt2(TRACE_NORMAL, TESTARBITRARYSTRINGDESCRIPTORS_TESTARBITRARYSTRINGDESCRIPTORS, "New test string @ idx %d: \"%lS\"\n", stridx3, rd_str);

	test.Next(_L("Compare test strings 3"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	// Remove string descriptors 3 and 4

	test.Next(_L("RemoveStringDescriptor() 4"));
	r = aPort->RemoveStringDescriptor(stridx4);
	test_Equal(KErrNotFound,r);

	test.Next(_L("RemoveStringDescriptor() 3"));
	r = aPort->RemoveStringDescriptor(stridx3);
	test_KErrNone(r);

	r = aPort->GetStringDescriptor(stridx3, rd_str);
	test_Equal(KErrNotFound,r);

	if (SupportsAlternateInterfaces())
		{
		TInt r = aPort->ReleaseInterface(aNumSettings);
		test_KErrNone(r);
		}

	test.End();
	}

void TestInvalidSetInterface (RDEVCLIENT* aPort,TInt aNumSettings)
	{
	#ifdef USB_SC
	TUsbcScInterfaceInfoBuf ifc;
	#else
	TUsbcInterfaceInfoBuf ifc;
	#endif
	_LIT16(string, "T_USB_DEVICE Invalid Interface");
	ifc().iString = const_cast<TDesC16*>(&string);
	ifc().iTotalEndpointsUsed = 0;

	test.Start(_L("Test Invalid Interface Setting"));

	if (SupportsAlternateInterfaces())
		{
		TInt r = aPort->SetInterface(aNumSettings+1, ifc);
		test_Compare(r,!=,KErrNone);
		}

	if (aNumSettings > 1)
		{
		TInt r = aPort->SetInterface(aNumSettings-1, ifc);
		test_Compare(r,!=,KErrNone);
		}

	TInt r = aPort->SetInterface(0, ifc);
	test_Compare(r,!=,KErrNone);

	test.End();
	}

void TestInvalidReleaseInterface (RDEVCLIENT* aPort,TInt aNumSettings)
	{
	test.Start(_L("Test Invalid Interface Release"));

	if (aNumSettings > 2)
		{
		TInt r = aPort->ReleaseInterface(aNumSettings-3);
		test_Compare(r,!=,KErrNone);
		}

	if (aNumSettings > 1)
		{
		TInt r = aPort->ReleaseInterface(aNumSettings-2);
		test_Compare(r,!=,KErrNone);
		}

	test.End();
	}

void TestDescriptorManipulation(TBool aHighSpeed, RDEVCLIENT* aPort, TInt aNumSettings)
	{
	test.Start(_L("Test USB Descriptor Manipulation"));

	if (aHighSpeed)
		{
		TestDeviceQualifierDescriptor(aPort);

		TestOtherSpeedConfigurationDescriptor(aPort);
		}

	TestInterfaceDescriptor(aPort,aNumSettings);

	TestClassSpecificDescriptors(aPort);

	TestStandardStringDescriptors(aPort);

	TestArbitraryStringDescriptors(aPort,aNumSettings);
	test.Next(_L("Test USB Descriptor Manipulation1"));

	test.End();
	}


void TestOtgExtensions(RDEVCLIENT* aPort)
	{
	test.Start(_L("Test Some OTG API Extensions"));

	// Test OTG descriptor manipulation
	test.Next(_L("Get OTG Descriptor Size"));
	TInt size;
	aPort->GetOtgDescriptorSize(size);
	test_Equal(KUsbDescSize_Otg,static_cast<TUint>(size));

	test.Next(_L("Get OTG Descriptor"));
	TBuf8<KUsbDescSize_Otg> otgDesc;
	TInt r = aPort->GetOtgDescriptor(otgDesc);
	test(r == KErrNotSupported || r == KErrNone);

	test.Next(_L("Set OTG Descriptor"));
	TBool supportOtg = EFalse;
	if (r == KErrNotSupported)
		{
		r = aPort->SetOtgDescriptor(otgDesc);
		test_Equal(KErrNotSupported,r);
		}
	else
		{
		supportOtg = ETrue;
		otgDesc[0] = KUsbDescSize_Otg;
		otgDesc[1] = KUsbDescType_Otg;
		otgDesc[2] = KUsbOtgAttr_SrpSupp;
		r = aPort->SetOtgDescriptor(otgDesc);
		test_KErrNone(r);
		TBuf8<KUsbDescSize_Otg> desc;
		r = aPort->GetOtgDescriptor(desc);
		test_KErrNone(r);
		test_Equal(0,desc.Compare(otgDesc));
		}

	// Test get/set OTG feature
	test.Next(_L("Get OTG Features"));
	TUint8 features;
	r = aPort->GetOtgFeatures(features);
	if (supportOtg)
		{
		test_KErrNone(r);
		TBool b_HnpEnable = (features & KUsbOtgAttr_B_HnpEnable) ? ETrue : EFalse;
		TBool a_HnpSupport = (features & KUsbOtgAttr_A_HnpSupport) ? ETrue : EFalse;
		TBool a_AltHnpSupport = (features & KUsbOtgAttr_A_AltHnpSupport) ? ETrue : EFalse;
		test.Printf(_L("### OTG Features:\nB_HnpEnable(%d)\nA_HnpSupport(%d)\nA_Alt_HnpSupport(%d)\n"),
					b_HnpEnable, a_HnpSupport, a_AltHnpSupport);
		OstTraceExt3(TRACE_NORMAL, TESTOTGEXTENSIONS_TESTOTGEXTENSIONS, "### OTG Features:\nB_HnpEnable(%d)\nA_HnpSupport(%d)\nA_Alt_HnpSupport(%d)\n",
					b_HnpEnable, a_HnpSupport, a_AltHnpSupport);
		}
	else
		{
		test_Equal(KErrNotSupported,r);
		test.Printf(_L("GetOtgFeatures() not supported\n"));
		OstTrace0(TRACE_NORMAL, TESTOTGEXTENSIONS_TESTOTGEXTENSIONS_DUP01, "GetOtgFeatures(not supported\n");
		}

	test.End();
}


void TestEndpoint0MaxPacketSizes(RDEVCLIENT* aPort)
	{
	test.Start(_L("Test Endpoint0 MaxPacketSizes"));

	TUint32 sizes = aPort->EndpointZeroMaxPacketSizes();
	TInt r = KErrNone;
	TBool good;
	TInt mpsize = 0;
	for (TInt i = 0; i < 32; i++)
		{
		TUint bit = sizes & (1 << i);
		if (bit != 0)
			{
			switch (bit)
				{
			case KUsbEpSizeCont:
				good = EFalse;
				break;
			case KUsbEpSize8:
				mpsize = 8;
				good = ETrue;
				break;
			case KUsbEpSize16:
				mpsize = 16;
				good = ETrue;
				break;
			case KUsbEpSize32:
				mpsize = 32;
				good = ETrue;
				break;
			case KUsbEpSize64:
				mpsize = 64;
				good = ETrue;
				break;
			case KUsbEpSize128:
			case KUsbEpSize256:
			case KUsbEpSize512:
			case KUsbEpSize1023:
			default:
				good = EFalse;
				break;
				}
			if (good)
				{
				test.Printf(_L("Ep0 supports %d bytes MaxPacketSize\n"), mpsize);
				OstTrace1(TRACE_NORMAL, TESTENDPOINT0MAXPACKETSIZES_TESTENDPOINT0MAXPACKETSIZES, "Ep0 supports %d bytes MaxPacketSize\n", mpsize);
				}
			else
				{
				test.Printf(_L("Bad Ep0 size: 0x%08x, failure will occur\n"), bit);
				OstTrace1(TRACE_NORMAL, TESTENDPOINT0MAXPACKETSIZES_TESTENDPOINT0MAXPACKETSIZES_DUP01, "Bad Ep0 size: 0x%08x, failure will occur\n", bit);
				r = KErrGeneral;
				}
			}
		}
	test_KErrNone(r);

    test.End();
	}


