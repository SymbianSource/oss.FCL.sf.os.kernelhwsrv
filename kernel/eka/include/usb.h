// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\usb.h
// Definitions of USB-specific constants and macros.
// 
//

/**
 @file usb.h
 @publishedPartner
 @released
*/

#ifndef __USB_H__
#define __USB_H__

//
// 'Chapter 9' Request Types (bmRequestType)
//
const TUint8 KUsbRequestType_DirShift   = 7;
const TUint8 KUsbRequestType_DirMask    = (1 << KUsbRequestType_DirShift);

const TUint8 KUsbRequestType_DirToDev   = (0 << KUsbRequestType_DirShift);
const TUint8 KUsbRequestType_DirToHost  = (1 << KUsbRequestType_DirShift);

const TUint8 KUsbRequestType_TypeShift  = 5;
const TUint8 KUsbRequestType_TypeMask   = (3 << KUsbRequestType_TypeShift);
const TUint8 KUsbRequestType_TypeStd    = (0 << KUsbRequestType_TypeShift);
const TUint8 KUsbRequestType_TypeClass  = (1 << KUsbRequestType_TypeShift);
const TUint8 KUsbRequestType_TypeVendor = (2 << KUsbRequestType_TypeShift);

const TUint8 KUsbRequestType_DestShift  = 0;
const TUint8 KUsbRequestType_DestMask   = (0x1f << KUsbRequestType_DestShift);
const TUint8 KUsbRequestType_DestDevice = (0x00 << KUsbRequestType_DestShift);
const TUint8 KUsbRequestType_DestIfc    = (0x01 << KUsbRequestType_DestShift);
const TUint8 KUsbRequestType_DestEp     = (0x02 << KUsbRequestType_DestShift);
const TUint8 KUsbRequestType_DestOther  = (0x03 << KUsbRequestType_DestShift);

//
// 'Chapter 9' Endpoint Zero Requests (bRequest)
//
const TUint8 KUsbRequest_GetStatus     = 0;
const TUint8 KUsbRequest_ClearFeature  = 1;
const TUint8 KUsbRequest_SetFeature    = 3;
const TUint8 KUsbRequest_SetAddress    = 5;
const TUint8 KUsbRequest_GetDescriptor = 6;
const TUint8 KUsbRequest_SetDescriptor = 7;
const TUint8 KUsbRequest_GetConfig     = 8;
const TUint8 KUsbRequest_SetConfig     = 9;
const TUint8 KUsbRequest_GetInterface  = 10;
const TUint8 KUsbRequest_SetInterface  = 11;
const TUint8 KUsbRequest_SynchFrame    = 12;

//
// Descriptor Types
//
const TUint8 KUsbDescType_Device               = 1;
const TUint8 KUsbDescType_Config               = 2;
const TUint8 KUsbDescType_String               = 3;
const TUint8 KUsbDescType_Interface            = 4;
const TUint8 KUsbDescType_Endpoint             = 5;
const TUint8 KUsbDescType_DeviceQualifier      = 6;
const TUint8 KUsbDescType_OtherSpeedConfig     = 7;
const TUint8 KUsbDescType_InterfacePower       = 8;
const TUint8 KUsbDescType_Otg                  = 9;
const TUint8 KUsbDescType_Debug                = 10;
const TUint8 KUsbDescType_InterfaceAssociation = 11;

//
// Descriptor Sizes
//
const TUint KUsbDescSize_Device               = 18;
const TUint KUsbDescSize_Config               = 9;
const TUint KUsbDescSize_Interface            = 9;
const TUint KUsbDescSize_Endpoint             = 7;
const TUint KUsbDescSize_Otg                  = 5;
const TUint KUsbDescSize_DeviceQualifier      = 10;
const TUint KUsbDescSize_OtherSpeedConfig     = 9;
const TUint KUsbDescSize_InterfaceAssociation = 8;
const TUint KUsbDescMaxSize_String            = 255;
const TUint KUsbStringDescStringMaxSize       = 252;		// it's actually 253, but that's awkward

//
// Configuration Characteristics (Configuration Descriptor)
//
const TUint8 KUsbDevAttr_SelfPowered  = (0x01 << 6);
const TUint8 KUsbDevAttr_RemoteWakeup = (0x01 << 5);

//
// Descriptor Indices for String Descriptors
//
const TUint KUsbDescStringIndex_Manufact = 14;
const TUint KUsbDescStringIndex_Product  = 15;
const TUint KUsbDescStringIndex_Serial   = 16;
const TUint KUsbDescStringIndex_Config   = 6;

//
// Endpoint Attributes
//
const TUint8 KUsbEpAttr_TransferTypeShift       = 0;
const TUint8 KUsbEpAttr_TransferTypeMask        = (0x03 << KUsbEpAttr_TransferTypeShift);
const TUint8 KUsbEpAttr_TransferTypeControl     = (0x00 << KUsbEpAttr_TransferTypeShift);
const TUint8 KUsbEpAttr_TransferTypeIsochronous = (0x01 << KUsbEpAttr_TransferTypeShift);
const TUint8 KUsbEpAttr_TransferTypeBulk        = (0x02 << KUsbEpAttr_TransferTypeShift);
const TUint8 KUsbEpAttr_TransferTypeInterrupt   = (0x03 << KUsbEpAttr_TransferTypeShift);

const TUint8 KUsbEpAttr_SyncTypeShift           = 2;
const TUint8 KUsbEpAttr_SyncTypeMask            = (0x03 << KUsbEpAttr_SyncTypeShift);
const TUint8 KUsbEpAttr_SyncTypeNoSync          = (0x00 << KUsbEpAttr_SyncTypeShift);
const TUint8 KUsbEpAttr_SyncTypeAsync           = (0x01 << KUsbEpAttr_SyncTypeShift);
const TUint8 KUsbEpAttr_SyncTypeAdaptive        = (0x02 << KUsbEpAttr_SyncTypeShift);
const TUint8 KUsbEpAttr_SyncTypeSync            = (0x03 << KUsbEpAttr_SyncTypeShift);

const TUint8 KUsbEpAttr_UsageTypeShift          = 4;
const TUint8 KUsbEpAttr_UsageTypeMask           = (0x03 << KUsbEpAttr_UsageTypeShift);
const TUint8 KUsbEpAttr_UsageTypeDataEp         = (0x00 << KUsbEpAttr_UsageTypeShift);
const TUint8 KUsbEpAttr_UsageTypeFeedbackEp     = (0x01 << KUsbEpAttr_UsageTypeShift);
const TUint8 KUsbEpAttr_UsageTypeImplFbDataEp   = (0x02 << KUsbEpAttr_UsageTypeShift);
const TUint8 KUsbEpAttr_UsageTypeReserved       = (0x03 << KUsbEpAttr_UsageTypeShift);

//
// OTG Feature Indicators
//
const TUint8 KUsbOtgAttr_SrpSupp         = 0x01;
const TUint8 KUsbOtgAttr_HnpSupp         = 0x02;
const TUint8 KUsbOtgAttr_B_HnpEnable     = 0x04;
const TUint8 KUsbOtgAttr_A_HnpSupport    = 0x08;
const TUint8 KUsbOtgAttr_A_AltHnpSupport = 0x10;
const TUint16 KUsbOtgDesc_bcdOTG         = 0x0200;

//
// Feature Settings
//
const TUint KUsbFeature_EndpointHalt    = 0;
const TUint KUsbFeature_RemoteWakeup    = 1;
const TUint KUsbFeature_TestMode        = 2;
const TUint KUsbFeature_B_HnpEnable     = 3;
const TUint KUsbFeature_A_HnpSupport    = 4;
const TUint KUsbFeature_A_AltHnpSupport = 5;

//
// Test Mode Selectors (Set/ClearFeature)
//
const TUint KUsbTestSelector_Test_J            = 0x01;
const TUint KUsbTestSelector_Test_K            = 0x02;
const TUint KUsbTestSelector_Test_SE0_NAK      = 0x03;
const TUint KUsbTestSelector_Test_Packet       = 0x04;
const TUint KUsbTestSelector_Test_Force_Enable = 0x05;

//
// Address Masks
//
const TUint8 KUsbEpAddress_In       = 0x80;
const TUint8 KUsbEpAddress_Portmask = 0x0f;

//
// Device Status Values (GET_STATUS)
//
const TUint16 KUsbDevStat_SelfPowered  = (1 << 0);
const TUint16 KUsbDevStat_RemoteWakeup = (1 << 1);

//
// Endpoint Status Values (GET_STATUS)
//
const TUint16 KUsbEpStat_Halt = (1 << 0);


//
// USB Descriptor Handling
//
/*------------------------------------------------
  USB transfers data in little-endian fashion.
  The following macros swap the byte order in
  words (16 bit) and longwords (32 bit), such that
  they are in little-endian order afterwards.
  ------------------------------------------------*/
#if defined(__BIG_ENDIAN__)		  // Hitachi SuperH, Motorola 68k
#define SWAP_BYTES_16(x) \
  ((((x) >> 8) & 0x00ff) | \
   (((x) << 8) & 0xff00))
#define SWAP_BYTES_32(x) \
  ((((x) >> 24) & 0x000000ff) | \
   (((x) >> 8)  & 0x0000ff00) | \
   (((x) << 24) & 0xff000000) | \
   (((x) << 8)  & 0x00ff0000))
#else							  // ARM, Intel
#define SWAP_BYTES_16(x)  (x)
#define SWAP_BYTES_32(x)  (x)
#endif // defined(__BIG_ENDIAN__)


static inline TUint8 LowByte(TUint16 aWord)
	{
	return static_cast<TUint8>(aWord & 0x00ff);
	}

static inline TUint8 HighByte(TUint16 aWord)
	{
	return static_cast<TUint8>((aWord >> 8) & 0x00ff);
	}


//
// Class-specific Values
//

// These are from the CDC (valid also for Audio Class)
const TUint8 KUsbDescType_CS_Interface = 0x24;
const TUint8 KUsbDescType_CS_Endpoint  = 0x25;

// Audio Device Class
const TUint KUsbDescSize_AudioEndpoint = KUsbDescSize_Endpoint + 2;
const TUint KUsbAudioInterfaceClassCode                          = 0x01;
const TUint KUsbAudioInterfaceSubclassCode_Subclass_Undefined    = 0x00;
const TUint KUsbAudioInterfaceSubclassCode_Audiocontrol          = 0x01;
const TUint KUsbAudioInterfaceSubclassCode_Audiostreaming        = 0x02;
const TUint KUsbAudioInterfaceSubclassCode_Midistreaming         = 0x03;
const TUint KUsbAudioInterfaceProtocolCode_Pr_Protocol_Undefined = 0x00;


//
// These are defined just for convenience:
//
const TUint8 KEp0_Out = 0;
const TUint8 KEp0_In  = 1;
const TUint8 KEp0_Rx  = KEp0_Out;
const TUint8 KEp0_Tx  = KEp0_In;


//
// USB Implementers Forum, Inc (USB-IF) assigned Vendor IDs:
//
const TUint16 KUsbVendorId_Symbian = 0x0E22;				// Symbian Ltd. (dec. 3618)


#endif // __USB_H__
