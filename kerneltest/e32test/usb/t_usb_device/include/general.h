// Copyright (c) 1997-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/usb/t_usb_device/include/general.h
// 
//

#ifndef __GENERAL_H__
#define __GENERAL_H__

#define __E32TEST_EXTENSION__

#include <e32hal.h>
#include <e32uid.h>
#include <hal.h>

#include <e32cons.h>
#include <e32svr.h>
#include <e32std.h>
#include <e32test.h>
#include <f32file.h>

#ifdef USB_SC
#include <d32usbcsc.h>
#define RDEVCLIENT RDevUsbcScClient
#define TENDPOINTNUMBER TInt
#else
#include <d32usbc.h>
#define RDEVCLIENT RDevUsbcClient
#define TENDPOINTNUMBER TEndpointNumber
#endif

static const TInt KHostLogFileSize = 64 * 1024;			// 64kB

static const TInt KMaxInterfaces = 10;
static const TInt KMaxConcurrentTests = 10;
static const TInt KMaxInterfaceSettings = 10;

static const TInt KWaitSettingSize = 8;

static const TUint16 KFirstSettingThreadMask = 0x8000;
static const TUint16 KLastSettingThreadMask = 0x4000;

static const TUint16 KHostErrorIndex = 0xFFF;

static const TUint8 stridx1 = 0xCC;
static const TUint8 stridx2 = 0xDD;
static const TUint8 stridx3 = 0xEE;
static const TUint8 stridx4 = 0xFF;

#ifdef USB_SC
_LIT(KActivePanic, "T_USB_SCDEVICE");
#else
_LIT(KActivePanic, "T_USB_DEVICE");
#endif

// Descriptor Offset Constants

static const TInt KUsbDesc_SizeOffset = 0;
static const TInt KUsbDesc_TypeOffset = 1;
static const TInt KDevDesc_SpecOffset = 2;
static const TInt KDevDesc_DevClassOffset = 4;
static const TInt KDevDesc_DevSubClassOffset = 5;
static const TInt KDevDesc_DevProtocolOffset = 6;
static const TInt KDevDesc_Ep0SizeOffset = 7;
static const TInt KDevDesc_VendorIdOffset = 8;
static const TInt KDevDesc_ProductIdOffset = 10;
static const TInt KDevDesc_DevReleaseOffset = 12;

static const TInt KConfDesc_AttribOffset = 7;
static const TInt KConfDesc_MaxPowerOffset = 8;

static const TInt KIfcDesc_LengthOffset = 0;
static const TInt KIfcDesc_TypeOffset = 1;
static const TInt KIfcDesc_NumberOffset = 2;
static const TInt KIfcDesc_SettingOffset = 3;
static const TInt KIfcDesc_NumEndpointsOffset = 4;
static const TInt KIfcDesc_ClassOffset = 5;
static const TInt KIfcDesc_SubClassOffset = 6;
static const TInt KIfcDesc_ProtocolOffset = 7;
static const TInt KIfcDesc_InterfaceOffset = 8;

static const TInt KEpDesc_LengthOffset = 0;
static const TInt KEpDesc_TypeOffset = 1;
static const TInt KEpDesc_AddressOffset = 2;
static const TInt KEpDesc_AttributesOffset = 3;
static const TInt KEpDesc_PacketSizeOffset = 4;
static const TInt KEpDesc_IntervalOffset = 6;
static const TInt KEpDesc_SynchAddressOffset = 8;

enum TXferMode
	{
	ENone,
	ELoop,
	ELoopComp,
	EReceiveOnly,
	ETransmitOnly
	};

enum TXferType
	{
	ETxferNone,
	EWaitSetting,
	ESuspend,
	EAltSetting,
	EReadXfer,
	EWriteXfer
	};

struct TestParam
	{
	TUint32 minSize;
	TUint32 maxSize;
	TUint32 packetNumber;
	TUint8 interfaceNumber;
	TUint8 alternateSetting;
	TUint8 outPipe;
	TUint8 inPipe;
	TUint16 repeat;
	TUint16 settingRepeat;
	TUint8 beforeIndex;
	TUint8 afterIndex;
	};
	
typedef struct TestParam TestParamType;
typedef TestParamType* TestParamPtr;

//
// Helpful Defines
//
#define HiByte(a) (TUint8)((a)>>8) 	
#define LoByte(a) (TUint8)((a)&0xFF)

#define TUSB_PRINT(string) \
		do { \
			if (iConsole) \
			{			\
		    iConsole->Printf(_L(string)); \
		    iConsole->Printf(_L("\n")); \
			} \
		} while (0)

#define TUSB_PRINT1(string, a) \
		do { \
		if (iConsole) \
			{			\
			iConsole->Printf(_L(string), (a)); \
			iConsole->Printf(_L("\n")); \
			}	\
		} while (0)

#define TUSB_PRINT2(string, a, b) \
		do { \
			if (iConsole) \
			{			\
			iConsole->Printf(_L(string), (a), (b)); \
			iConsole->Printf(_L("\n")); \
			}	\
		} while (0)

#define TUSB_PRINT3(string, a, b, c) \
		do { \
			if (iConsole) \
			{			\
			iConsole->Printf(_L(string), (a), (b), (c)); \
			iConsole->Printf(_L("\n")); \
			}	\
		} while (0)

#define TUSB_PRINT4(string, a, b, c, d) \
		do { \
			if (iConsole) \
			{			\
			iConsole->Printf(_L(string), (a), (b), (c), (d)); \
			iConsole->Printf(_L("\n")); \
			}	\
		} while (0)

#define TUSB_PRINT5(string, a, b, c, d, e) \
		do { \
			if (iConsole) \
			{			\
			iConsole->Printf(_L(string), (a), (b), (c), (d), (e)); \
			iConsole->Printf(_L("\n")); \
			}	\
		} while (0)

#define TUSB_PRINT6(string, a, b, c, d, e, f) \
		do { \
			if (iConsole) \
			{			\
			iConsole->Printf(_L(string), (a), (b), (c), (d), (e), (f)); \
			iConsole->Printf(_L("\n")); \
			}	\
		} while (0)

#define TUSB_VERBOSE_PRINT(string) \
		do { \
		if (gVerbose) \
			{ \
			TUSB_PRINT(string); \
			} \
		} while (0)

#define TUSB_VERBOSE_PRINT1(string, a) \
		do { \
		if (gVerbose) \
			{ \
			TUSB_PRINT1(string, a); \
			} \
		} while (0)

#define TUSB_VERBOSE_PRINT2(string, a, b) \
		do { \
		if (gVerbose) \
			{ \
			TUSB_PRINT2(string, a, b); \
			} \
		} while (0)

#define TUSB_VERBOSE_PRINT3(string, a, b, c) \
		do { \
		if (gVerbose) \
			{ \
			TUSB_PRINT3(string, a, b, c); \
			} \
		} while (0)

#define TUSB_VERBOSE_PRINT4(string, a, b, c, d) \
		do { \
		if (gVerbose) \
			{ \
			TUSB_PRINT4(string, a, b, c, d); \
			} \
		} while (0)

#define TUSB_VERBOSE_PRINT5(string, a, b, c, d, e) \
		do { \
		if (gVerbose) \
			{ \
			TUSB_PRINT5(string, a, b, c, d, e); \
			} \
		} while (0)

#define TUSB_VERBOSE_APRINT(string) \
		do { \
		if (gVerbose) \
			{ \
				if (iConsole) \
				{			\
				aConsole->Printf(_L(string)); \
				aConsole->Printf(_L("\n")); \
				}	\
			} \
		} while (0)

#define TUSB_HEAPSIZE(string) \
		do { \
			TInt totalSize = 0; \
			TInt numCells = User::AllocSize (totalSize); \
		} while (0)
	
#endif	// __GENERAL_H__
