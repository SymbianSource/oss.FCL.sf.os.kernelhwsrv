// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/include/d32usbcshared.inl
// User side class definitions for USB Device support.
// 
//

/**
 @file d32usbcshared.inl
 @publishedPartner
 @released
*/

#ifndef __D32USBCSHARED_INL__
#define __D32USBCSHARED_INL__

inline TInt TUsbcEndpointCaps::MaxPacketSize() const
	{
	return (iSizes & KUsbEpSize1024) ? 1024 :
		((iSizes & KUsbEpSize1023) ? 1023 :
		 ((iSizes & KUsbEpSize512) ? 512 :
		  ((iSizes & KUsbEpSize256) ? 256 :
		   ((iSizes & KUsbEpSize128) ? 128 :
			((iSizes & KUsbEpSize64) ? 64 :
			 ((iSizes & KUsbEpSize32) ? 32 :
			  ((iSizes & KUsbEpSize16) ? 16 :
			   ((iSizes & KUsbEpSize8) ? 8 : 0))))))));
	}


inline TInt TUsbcEndpointCaps::MinPacketSize() const
	{
	return (iSizes & KUsbEpSize8) ? 8 :
		((iSizes & KUsbEpSize16) ? 16 :
		 ((iSizes & KUsbEpSize32) ? 32 :
		  ((iSizes & KUsbEpSize64) ? 64 :
		   ((iSizes & KUsbEpSize128) ? 128 :
			((iSizes & KUsbEpSize256) ? 256 :
			 ((iSizes & KUsbEpSize512) ? 512 :
			  ((iSizes & KUsbEpSize1023) ? 1023 :
			   ((iSizes & KUsbEpSize1024) ? 1024 : 0))))))));
	}


static inline TUint PacketSize2Mask(TInt aSize)
	{
	return (aSize == 8) ? KUsbEpSize8 :
		((aSize == 16) ? KUsbEpSize16 :
		 ((aSize == 32) ? KUsbEpSize32 :
		  ((aSize == 64) ? KUsbEpSize64 :
		   ((aSize == 128) ? KUsbEpSize128 :
			((aSize == 256) ? KUsbEpSize256 :
			 ((aSize == 512) ? KUsbEpSize512 :
			  ((aSize == 1023) ? KUsbEpSize1023 :
			   ((aSize == 1024) ? KUsbEpSize1024 : 0))))))));
	}


static inline TUint EpTypeMask2Value(TInt aType)
	{
	return (aType & KUsbEpTypeControl) ? KUsbEpAttr_TransferTypeControl :
		((aType & KUsbEpTypeIsochronous) ? KUsbEpAttr_TransferTypeIsochronous :
		 ((aType & KUsbEpTypeBulk) ? KUsbEpAttr_TransferTypeBulk :
		  ((aType & KUsbEpTypeInterrupt) ? KUsbEpAttr_TransferTypeInterrupt : -1)));
	}


/** @internalTechnology
*/
struct TEndpointDescriptorInfo
	{
	TInt iSetting;											// alternate setting
	TInt iEndpoint;											// excludes ep0
	TAny* iArg;												// address of data
	};


/** @internalTechnology
*/
struct TCSDescriptorInfo
	{
	TInt iSetting;											// alternate setting
	TInt iEndpoint;											// excludes ep0, not used for CS ifc desc
	TAny* iArg;												// address of data
	TInt iSize;												// size of data (descriptor block)
	};

inline TUsbcEndpointInfo::TUsbcEndpointInfo(TUint aType, TUint aDir, TInt aSize,
											TInt aInterval, TInt aExtra)
	: iType(aType), iDir(aDir), iSize(aSize), iInterval(aInterval),
	  iInterval_Hs(-1), iTransactions(0), iExtra(aExtra), iFeatureWord1(0),
	  iReserved(0)
	{}

inline TUsbcClassInfo::TUsbcClassInfo(TInt aClass, TInt aSubClass, TInt aProtocol)
	: iClassNum(aClass), iSubClassNum(aSubClass), iProtocolNum(aProtocol), iReserved(0)
	{}


#endif


