// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/drivers/usbcc/misc.cpp
// Platform independent layer (PIL) of the USB Device controller driver:
// Implementations of misc. classes defined in usbc.h.
// 
//

/**
 @file misc.cpp
 @internalTechnology
*/

#include <drivers/usbc.h>


/** Helper function for logical endpoints and endpoint descriptors:
	Split single Ep size into separate FS/HS sizes.
	This function modifies its arguments.
 */
TInt TUsbcEndpointInfo::AdjustEpSizes(TInt& aEpSize_Fs, TInt& aEpSize_Hs) const
	{
	if (iType == KUsbEpTypeBulk)
		{
		// FS: [8|16|32|64] HS: 512
		if (iSize < 64)
			{
			aEpSize_Fs = iSize;
			}
		else
			{
			aEpSize_Fs = 64;
			}
		aEpSize_Hs = 512;
		}
	else if (iType == KUsbEpTypeInterrupt)
		{
		// FS: [0..64] HS: [0..1024]
		if (iSize < 64)
			{
			aEpSize_Fs = iSize;
			}
		else
			{
			aEpSize_Fs = 64;
			}
		aEpSize_Hs = iSize;
		}
	else if (iType == KUsbEpTypeIsochronous)
		{
		// FS: [0..1023] HS: [0..1024]
		if (iSize < 1023)
			{
			aEpSize_Fs = iSize;
			}
		else
			{
			aEpSize_Fs = 1023;
			}
		aEpSize_Hs = iSize;
		}
	else if (iType == KUsbEpTypeControl)
		{
		// FS: [8|16|32|64] HS: 64
		if (iSize < 64)
			{
			aEpSize_Fs = iSize;
			}
		else
			{
			aEpSize_Fs = 64;
			}
		aEpSize_Hs = 64;
		}
	else
		{
		aEpSize_Fs = aEpSize_Hs = 0;
		return KErrGeneral;
		}

	// For the reason of the following checks see Table 9-14. "Allowed wMaxPacketSize
	// Values for Different Numbers of Transactions per Microframe".
	if ((iType == KUsbEpTypeInterrupt) || (iType == KUsbEpTypeIsochronous))
		{
		if (iTransactions == 1)
			{
			if (aEpSize_Hs < 513)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Warning: Ep size too small: %d < 513. Correcting...",
												  aEpSize_Hs));
				aEpSize_Hs = 513;
				}
			}
		else if (iTransactions == 2)
			{
			if (aEpSize_Hs < 683)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Warning: Ep size too small: %d < 683. Correcting...",
												  aEpSize_Hs));
				aEpSize_Hs = 683;
				}
			}
		}
	return KErrNone;
	}


/** Helper function for logical endpoints and endpoint descriptors:
	If not set, assign a valid and meaningful value to iInterval_Hs, deriving from iInterval.
	This function modifies the objects's data member(s).
 */
TInt TUsbcEndpointInfo::AdjustPollInterval()
	{
	if (iInterval_Hs != -1)
		{
		// Already done.
		return KErrNone;
		}
	if ((iType == KUsbEpTypeBulk) || (iType == KUsbEpTypeControl))
		{
		// Valid range: 0..255 (maximum NAK rate).
		// (The host controller will probably ignore this value though -
		//  see the last sentence of section 9.6.6 for details.)
		iInterval_Hs = 255;
		}
	else if (iType == KUsbEpTypeInterrupt)
		{
		// HS interval = 2^(iInterval_Hs-1) with a valid iInterval_Hs range of 1..16.
		// The following table shows the mapping of HS values to actual intervals (and
		// thus FS values) for the range of possible FS values (1..255).
		// There is not always a 1:1 mapping possible, but we want at least to make sure
		// that the HS polling interval is never longer than the FS one (except for 255).
		//
		// 1 = 1
		// 2 = 2
		// 3 = 4
		// 4 = 8
		// 5 = 16
		// 6 = 32
		// 7 = 64
		// 8 = 128
		// 9 = 256
		if (iInterval == 255)
			iInterval_Hs = 9;
		else if (iInterval >= 128)
			iInterval_Hs = 8;
		else if (iInterval >= 64)
			iInterval_Hs = 7;
		else if (iInterval >= 32)
			iInterval_Hs = 6;
		else if (iInterval >= 16)
			iInterval_Hs = 5;
		else if (iInterval >= 8)
			iInterval_Hs = 4;
		else if (iInterval >= 4)
			iInterval_Hs = 3;
		else if (iInterval >= 2)
			iInterval_Hs = 2;
		else if (iInterval == 1)
			iInterval_Hs = 1;
		else
			{
			// iInterval wasn't set properly by the user
			iInterval_Hs = 1;
			return KErrGeneral;
			}
		}
	else if (iType == KUsbEpTypeIsochronous)
		{
		// Interpretation is the same for FS and HS.
		iInterval_Hs = iInterval;
		}
	else
		{
		// '1' is a valid value for all endpoint types...
		iInterval_Hs = 1;
		return KErrGeneral;
		}
	return KErrNone;
	}


TUsbcPhysicalEndpoint::TUsbcPhysicalEndpoint()
	: iEndpointAddr(0), iIfcNumber(NULL), iLEndpoint(NULL), iSettingReserve(EFalse), iHalt(EFalse)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcPhysicalEndpoint::TUsbcPhysicalEndpoint"));
	}


TInt TUsbcPhysicalEndpoint::TypeAvailable(TUint aType) const
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcPhysicalEndpoint::TypeAvailable"));
	switch (aType)
		{
	case KUsbEpTypeControl:
		return (iCaps.iTypesAndDir & KUsbEpTypeControl);
	case KUsbEpTypeIsochronous:
		return (iCaps.iTypesAndDir & KUsbEpTypeIsochronous);
	case KUsbEpTypeBulk:
		return (iCaps.iTypesAndDir & KUsbEpTypeBulk);
	case KUsbEpTypeInterrupt:
		return (iCaps.iTypesAndDir & KUsbEpTypeInterrupt);
	default:
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: invalid EP type: %d", aType));
		return 0;
		}
	}


TInt TUsbcPhysicalEndpoint::DirAvailable(TUint aDir) const
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcPhysicalEndpoint::DirAvailable"));
	switch (aDir)
		{
	case KUsbEpDirIn:
		return (iCaps.iTypesAndDir & KUsbEpDirIn);
	case KUsbEpDirOut:
		return (iCaps.iTypesAndDir & KUsbEpDirOut);
	default:
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: invalid EP direction: %d", aDir));
		return 0;
		}
	}


TInt TUsbcPhysicalEndpoint::EndpointSuitable(const TUsbcEndpointInfo* aEpInfo, TInt aIfcNumber) const
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcPhysicalEndpoint::EndpointSuitable"));
	__KTRACE_OPT(KUSB, Kern::Printf("  looking for EP: type=0x%x dir=0x%x size=%d (ifc_num=%d)",
									aEpInfo->iType, aEpInfo->iDir, aEpInfo->iSize, aIfcNumber));
	if (iSettingReserve)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  -> setting conflict"));
		return 0;
		}
	// (aIfcNumber == -1) means the ep is for a new default interface setting
	else if (iIfcNumber && (*iIfcNumber != aIfcNumber))
		{
		// If this endpoint has already been claimed (iIfcNumber != NULL),
		// but by a different interface(-set) than the currently looking one
		// (*iIfcNumber != aIfcNumber), then it's not available.
		// This works because we can assign the same physical endpoint
		// to different alternate settings of the *same* interface, and
		// because we check for available endpoints for every alternate setting
		// as a whole.
		__KTRACE_OPT(KUSB, Kern::Printf("  -> ifc conflict"));
		return 0;
		}
	else if (!TypeAvailable(aEpInfo->iType))
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  -> type conflict"));
		return 0;
		}
	else if (!DirAvailable(aEpInfo->iDir))
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  -> direction conflict"));
		return 0;
		}
	else if (!(iCaps.iSizes & PacketSize2Mask(aEpInfo->iSize)) && !(iCaps.iSizes & KUsbEpSizeCont))
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  -> size conflict"));
		return 0;
		}
	else
		return 1;
	}


TUsbcPhysicalEndpoint::~TUsbcPhysicalEndpoint()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcPhysicalEndpoint::~TUsbcPhysicalEndpoint()"));
	iLEndpoint = NULL;
	}


TUsbcLogicalEndpoint::TUsbcLogicalEndpoint(DUsbClientController* aController, TUint aEndpointNum,
										   const TUsbcEndpointInfo& aEpInfo, TUsbcInterface* aInterface,
										   TUsbcPhysicalEndpoint* aPEndpoint)
	: iController(aController), iLEndpointNum(aEndpointNum), iInfo(aEpInfo), iInterface(aInterface),
	  iPEndpoint(aPEndpoint)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcLogicalEndpoint::TUsbcLogicalEndpoint()"));
	//  Adjust FS/HS endpoint sizes
	if (iInfo.AdjustEpSizes(iEpSize_Fs, iEpSize_Hs) != KErrNone)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Unknown endpoint type: %d", iInfo.iType));
		}
	__KTRACE_OPT(KUSB, Kern::Printf("  Now set: iEpSize_Fs=%d iEpSize_Hs=%d (iInfo.iSize=%d)",
									iEpSize_Fs, iEpSize_Hs, iInfo.iSize));
	//  Adjust HS polling interval
	if (iInfo.AdjustPollInterval() != KErrNone)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Unknown ep type (%d) or invalid interval value (%d)",
										  iInfo.iType, iInfo.iInterval));
		}
	__KTRACE_OPT(KUSB, Kern::Printf("  Now set: iInfo.iInterval=%d iInfo.iInterval_Hs=%d",
									iInfo.iInterval, iInfo.iInterval_Hs));
	// Additional transactions requested on a non High Bandwidth ep?
	if ((iInfo.iTransactions > 0) && !aPEndpoint->iCaps.iHighBandwidth)
		{
		__KTRACE_OPT(KPANIC,
					 Kern::Printf("  Warning: Additional transactions requested but not a High Bandwidth ep"));
		}
	}


TUsbcLogicalEndpoint::~TUsbcLogicalEndpoint()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcLogicalEndpoint::~TUsbcLogicalEndpoint: #%d", iLEndpointNum));
	// If the real endpoint this endpoint points to is also used by
	// any other logical endpoint in any other setting of this interface
	// then we leave the real endpoint marked as used. Otherwise we mark
	// it as available (set its ifc number pointer to NULL).
	const TInt n = iInterface->iInterfaceSet->iInterfaces.Count();
	for (TInt i = 0; i < n; ++i)
		{
		const TUsbcInterface* const ifc = iInterface->iInterfaceSet->iInterfaces[i];
		const TInt m = ifc->iEndpoints.Count();
		for (TInt j = 0; j < m; ++j)
			{
			const TUsbcLogicalEndpoint* const ep = ifc->iEndpoints[j];
			if ((ep->iPEndpoint == iPEndpoint) && (ep != this))
				{
				__KTRACE_OPT(KUSB, Kern::Printf("  Physical endpoint still in use -> we leave it as is"));
				return;
				}
			}
		}
	__KTRACE_OPT(KUSB, Kern::Printf("  Closing DMA channel"));
	const TInt idx = iController->EpAddr2Idx(iPEndpoint->iEndpointAddr);
	// If the endpoint doesn't support DMA (now or ever) the next operation will be a no-op.
	iController->CloseDmaChannel(idx);
	__KTRACE_OPT(KUSB, Kern::Printf("  Setting physical ep 0x%02x ifc number to NULL (was %d)",
									iPEndpoint->iEndpointAddr, *iPEndpoint->iIfcNumber));
	iPEndpoint->iIfcNumber = NULL;
	}


TUsbcInterface::TUsbcInterface(TUsbcInterfaceSet* aIfcSet, TUint8 aSetting, TBool aNoEp0Requests)
	: iEndpoints(2), iInterfaceSet(aIfcSet), iSettingCode(aSetting), iNoEp0Requests(aNoEp0Requests)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcInterface::TUsbcInterface()"));
	}


TUsbcInterface::~TUsbcInterface()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcInterface::~TUsbcInterface()"));
	iEndpoints.ResetAndDestroy();
	}


TUsbcInterfaceSet::TUsbcInterfaceSet(const DBase* aClientId, TUint8 aIfcNum)
	: iInterfaces(2), iClientId(aClientId), iInterfaceNumber(aIfcNum), iCurrentInterface(0)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcInterfaceSet::TUsbcInterfaceSet()"));
	}


TUsbcInterfaceSet::~TUsbcInterfaceSet()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcInterfaceSet::~TUsbcInterfaceSet()"));
	iInterfaces.ResetAndDestroy();
	}


TUsbcConfiguration::TUsbcConfiguration(TUint8 aConfigVal)
	: iInterfaceSets(1), iConfigValue(aConfigVal)			// iInterfaceSets(1): granularity
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcConfiguration::TUsbcConfiguration()"));
	}


TUsbcConfiguration::~TUsbcConfiguration()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcConfiguration::~TUsbcConfiguration()"));
	iInterfaceSets.ResetAndDestroy();
	}


_LIT(KDriverName, "Usbcc");

DUsbcPowerHandler::DUsbcPowerHandler(DUsbClientController* aController)
	: DPowerHandler(KDriverName), iController(aController)
	{}


void DUsbcPowerHandler::PowerUp()
	{
	if (iController)
		iController->iPowerUpDfc.Enque();
	}


void DUsbcPowerHandler::PowerDown(TPowerState)
	{
	if (iController)
		iController->iPowerDownDfc.Enque();
	}


// -eof-
