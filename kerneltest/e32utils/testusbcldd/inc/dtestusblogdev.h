// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\testusbcldd\inc\dtestusblogdev.h
// Test USB logical channel classes.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __DTESTUSBCLOGDEVICE_H_
#define __DTESTUSBCLOGDEVICE_H_

#include "kerndefs.h"
#include "usbcdesc.h"

const TInt KTestUsbcMajorVersion = 0;
const TInt KTestUsbcMinorVersion = 1;
const TInt KTestUsbcBuildVersion = 1;
const TInt KTestUsbcMaxRequests = 10;

/** The USB version we are compliant with (BCD) */
const TUint16 KUsbcUsbVersion = 0x0110;

/** The Ep0 tx buffer area */
const TInt KUsbcBufSz_Ep0Tx = 1024;

/** Size of buffer for transferring data from client and host */
const TInt KTransferBufSize = 1024;

/** Must correspond to the max enum of TRequest + 1;
	currently this is ERequestEndpointStatusNotify = 9 */
const TInt KUsbcMaxRequests = 10;

/** Maximum number of endpoints an Interface may have */
const TInt KUsbcMaxEpNumber = 5;

/** Size of endpoint buffers */
const TInt KEndpointBufferSize = 16 * 1024;

class DTestUsbcEndpoint;
class DTestUsbcLogDevice : public DLogicalDevice
	{
public:
	DTestUsbcLogDevice();
	~DTestUsbcLogDevice();
	TInt Install();
	void GetCaps(TDes8& aDes) const;
	TInt Create(DLogicalChannelBase*& aChannel);
	DLogicalChannel* CreateL();
	
private:
	RPointerArray<DTestUsbcEndpoint> iEndpoints;
	};

class TUsbcInterfaceSet;
class TUsbcLogicalEndpoint;

/** This is one 'Alternate Setting' of an interface. */
class TUsbcInterface
	{
public:
	TUsbcInterface(TUsbcInterfaceSet* aIfcSet, TUint8 aSetting);
	~TUsbcInterface();
public:
	/** Array of endpoints making up (belonging to) this setting. */
	RPointerArray<TUsbcLogicalEndpoint> iEndpoints;
	/** 'Back' pointer. */
	TUsbcInterfaceSet* const iInterfaceSet;
	/** bAlternateSetting (zero-based). */
	const TUint8 iSettingCode;
	};

/** This is an 'Interface' (owning 1 or more alternate settings TUsbcInterface). */
class TUsbcInterfaceSet
	{
public:
	TUsbcInterfaceSet(const DBase* aClientId, TUint8 aIfcNum);
	~TUsbcInterfaceSet();
	const TUsbcInterface* CurrentInterface() const;
	TUsbcInterface* CurrentInterface();
public:
	/** Array of alternate settings provided by (belonging to) this interface. */
	RPointerArray<TUsbcInterface> iInterfaces;
	/** Pointer to the LDD which created and owns this interface. */
	const DBase* const iClientId;
	/** bInterfaceNumber (zero-based). */
	TUint8 iInterfaceNumber;
	/** bAlternateSetting (zero-based). */
	TUint8 iCurrentInterface;
	};
	
/** This is a logical 'Endpoint', as used by our device configuration model. */
class TUsbcLogicalEndpoint
	{
public:
	TUsbcLogicalEndpoint(TUint aEndpointNum, const TUsbcEndpointInfo& aInfo,
						 TUsbcInterface* aInterface);
	~TUsbcLogicalEndpoint();
	public:
	/** The virtual (logical) endpoint number. */
	const TInt iLEndpointNum;
	/** This endpoint's info structure. */
	TUsbcEndpointInfo iInfo;
	/** 'Back' pointer. */
	const TUsbcInterface* iInterface;
	};
	
class DLddTestUsbcChannel : public DLogicalChannel
	{
public:
	DLddTestUsbcChannel(RPointerArray<DTestUsbcEndpoint>& aEndpoints);	
	void HandleMsg(TMessageBase* aMsg);
	TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	~DLddTestUsbcChannel();

private:
	TInt DoCancel(TInt aReqNo);
	void DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TBool ValidateEndpoint(TUsbcEndpointInfo* aEndpointInfo);
	TInt SetInterface(TInt aInterfaceNumber, TUsbcInterfaceInfoBuf *aUserInterfaceInfoBuf, TPtr8* aInterfaceString);
	TInt SetInterface(TInt aInterfaceNumber, TUsbcClassInfo& aClass, TDesC8* aString,
					  TInt aTotalEndpointsUsed, const TUsbcEndpointInfo aEndpointData[]);
	TUsbcInterface* CreateInterface(TInt aIfc);
	void DeleteInterface(TInt aIfc);
	void DeleteInterfaceSet();
	TInt CreateEndpoints(TUsbcInterface* aIfc, TInt aEndpointsUsed, const TUsbcEndpointInfo aEndpointData[]);
	TInt SetupIfcDescriptor(TUsbcInterface* aIfc, TUsbcClassInfo& aClass,
							TDesC8* aString, const TUsbcEndpointInfo aEndpointData[]);
	TInt ReleaseInterface(TInt aInterfaceNumber);
	TInt HostEndpointStatusNotify(TInt aEndpointNumber, TRequestStatus* aStatus);
	TInt EndpointStatusNotify(TUint* aEndpointMask, TRequestStatus* aStatus);
	void EndpointStatusNotifyCallback();
	TInt ClearEndpoint(TInt aEndpointNumber);
	TInt DoTransferAsyncReq(TInt aEndpointNumber, TAny* a1, TAny* a2, TRequestStatus& aStatus);
	TBool ValidEndpoint(TInt aEndpointNumber);
	TInt FindRealEndpoint(TInt aEndpointNumber);
	TInt HaltClearEndpoint(TBool aHalt, TInt aEndpointNumber);
	void AlternateDeviceStatusNotify();
	TInt SetAlternateDeviceStatusNotify(TRequestStatus* aStatus, TUint* aValue);
	void CancelAlternateDeviceStatusNotify();
	TInt ReEnumerate(TRequestStatus* aStatus);
	void SetDeviceState(TUsbcDeviceState aState);
private:
	DThread* iClient;
	TUsbcDescriptorPool iDescriptors;
	TUsbcInterfaceSet iIfcSet;
	RPointerArray<DTestUsbcEndpoint>& iEndpoints;
	TUint* iEndpointStatusMask;
	TRequestStatus* iEndpointStatusNotifyRequest;
	TRequestStatus* iAlternateDeviceStatusNotifyRequest;
	TUint* iAlternateDeviceStatusNotifyValue;
	TUsbcDeviceState iDeviceState;
	
public:
	static const TUsbcEndpointData iEndpointData[];
		
	friend class DTestUsbcEndpoint;
	};
			
class DTestUsbcEndpoint : public DBase
	{
public:
	DTestUsbcEndpoint();
	~DTestUsbcEndpoint();
	TInt Create(const TUsbcEndpointCaps& aCaps);
	TInt TryToComplete();
	TInt CopyData(TInt aSrcOffset, DThread* aDestClient, TDesC8* aDest,
				  TInt aDestOffset, TInt aLen);
	TBool SupportsDir(TUint aDir);
	TBool EndpointSuitable(const TUsbcEndpointInfo& aInfo);
	void DoCancel();
	TInt Halt();
	TInt Clear();
	TBool IsHalted();
	void SetClearCallback(DLddTestUsbcChannel* aCallback);
	TInt HostStatusNotify(DThread* aHost, TRequestStatus* aStatus);
	TInt NewRequest(DThread* aClient, TRequestStatus* aStatus, TEndpointTransferInfo& aInfo, TTransferType aType);
	TInt NewHostRequest(DThread* aHost, TRequestStatus* aStatus, TEndpointTransferInfo& aInfo, TTransferType aType);
private:
	TRequestStatus* iClientStatus;
	TRequestStatus* iHostStatus;
	TRequestStatus* iHostNotifyStatus;
	DThread* iClient;
	DThread* iHost;
	DThread* iNotifyHost;
	TUsbcEndpointCaps iCaps;
	TBool iRequestPending;
	TBool iHostRequestPending;
	TEndpointTransferInfo iClientTransferInfo;
	TEndpointTransferInfo iHostTransferInfo;
	TInt iDataTransferred;
	TInt iHostDataTransferred;
	TTransferType iRequestType;
	TTransferType iHostRequestType;
	HBuf8Plat* iBuffer;
	TBool iHalted;
	DLddTestUsbcChannel* iClearCallback;
	
public:
	TBool iReserve;
	};

#endif // __DTESTUSBCLOGDEVICE_H_
