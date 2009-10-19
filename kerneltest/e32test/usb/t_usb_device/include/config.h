// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/usb/t_usb_device/include/t_usbdev_config.h
// 
//

#ifndef __CONFIG_H__
#define __CONFIG_H__

enum XMLState
	{
	EEmpty,
	EStartKey,
	EEndKey,
	EAttribute,
	EValue
	};

struct IFConfig
	{
	struct IFConfig * iPtrNext;
	#ifdef USB_SC
	TUsbcScInterfaceInfo* iInfoPtr;
	#else
	TUsbcInterfaceInfo* iInfoPtr;
	#endif
	TBool iAlternateSetting;
	TBool iEpDMA[KMaxEndpointsPerClient];
	TBool iEpDoubleBuff[KMaxEndpointsPerClient];
	TUint8 iNumber;
	TUint32 iBandwidthIn;
	TUint32 iBandwidthOut;
	TUint8 iPortNumber;
	IFConfig (TUint8 aNumber);
	};
typedef struct IFConfig IFConfigType;
typedef IFConfigType* IFConfigPtr;

	
struct LDDConfig
	{
	struct LDDConfig * iPtrNext;
	IFConfigPtr iIFPtr;
	TBuf<50> iName;
	TBool iSoftConnect;
	TBool iSelfPower;
	TBool iRemoteWakeup;
	TBool iHighSpeed;
	TBool iEPStall;
	TUint8 iNumEndpoints;
	TUint8 iNumChannels;
	TUint8 iFeatures;
	TUint8 iMaxPower;
	TUint16 iSpec;
	TUint16 iVid;
	TUint16 iPid;
	TUint16 iRelease;
	TBool iOtg_A;
	TBool iOtg_altA;
	TBool iOtg_altB;
	TDesC16 * iManufacturer;
	TDesC16 * iProduct;
	TDesC16 * iSerialNumber;
	LDDConfig (TPtrC aName);
	};
typedef struct LDDConfig LDDConfigType;
typedef LDDConfigType* LDDConfigPtr;

struct ConfigPtrs
	{
	LDDConfigPtr* iNextLDDPtrPtr;
	LDDConfigPtr iThisLDDPtr;
	IFConfigPtr iThisIFPtr;
	IFConfigPtr* iNextIFPtrPtr;
	ConfigPtrs (LDDConfigPtr* LDDPtrPtr);
	};
typedef struct ConfigPtrs ConfigPtrsType;
typedef ConfigPtrsType* ConfigPtrsPtr;
	
bool ProcessConfigFile (RFile aConfigFile,CConsoleBase* iConsole, LDDConfigPtr * LDDPtrPtr);
TBool CheckAttribute (CConsoleBase* iConsole, ConfigPtrsPtr cpPtr,TInt aKeyIndex, TPtrC aDes);
TBool CheckValue (CConsoleBase* iConsole, ConfigPtrsPtr cpPtr,TInt aKeyIndex, TPtrC aDes);
TInt CheckXmlKey (TPtrC aKey, TInt aLevel);
TBool TDesToTUint (TPtrC aDes, TUint * aValue);
TBool TDesToBool (TPtrC aDes, TBool * aValue);

#endif	// __CONFIG_H__
