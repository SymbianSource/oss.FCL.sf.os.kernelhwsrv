// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file Kernel side interfaces to example Logical and Physical Device Drivers
 @publishedPartner
 @released
*/

#ifndef __DRIVER1_DEV_H__
#define __DRIVER1_DEV_H__

/**
  Physical Device (factory class) for 'Driver1'
*/
class DDevice1PddFactory : public DPhysicalDevice
	{
public:
	DDevice1PddFactory();
	~DDevice1PddFactory();
	// Inherited from DPhysicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
public:
	TInt iHardwareInUse;
private:
	enum TMinimumLDDVersion
		{
		EMinimumLddMajorVersion=1,
		EMinimumLddMinorVersion=0,
		EMinimumLddBuild=0 //Not used
		};
public:
	TDynamicDfcQue* iDfcQ;
	};

/**
  Logical Device (factory class) for 'Driver1'
*/
class DDriver1Factory : public DLogicalDevice
	{
public:
	DDriver1Factory();
	~DDriver1Factory();
	//	Inherited from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DDriver1;


/**
  Logical Channel class for 'Driver1'
*/
class DDriver1Channel : public DLogicalChannel
	{
public:
	DDriver1Channel();
	virtual ~DDriver1Channel();
	//	Inherited from DObject
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	// Inherited from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	// Inherited from DLogicalChannel
	virtual void HandleMsg(TMessageBase* aMsg);
private:
	// Panic reasons
	enum TPanic
		{
		ERequestAlreadyPending = 1
		};
	// Implementation for the differnt kinds of messages sent through RBusLogicalChannel
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	void DoCancel(TUint aMask);
	// Accessor for the PDD
	inline DDriver1* Pdd();
	// Methods for configuration
	TInt GetConfig(TDes8* aConfigBuf);
	TInt SetConfig(const TDesC8* aConfigBuf);
	void CurrentConfig(RDriver1::TConfig& aConfig);
	// Methods for processing a SendData request
	TInt SendData(TRequestStatus* aStatus,const TDesC8* aData);
	void SendDataCancel();
	void DoSendDataComplete();
	static void SendDataDfc(TAny* aPtr);
	// Methods for processing a ReceiveData request
	TInt ReceiveData(TRequestStatus* aStatus,TDes8* aBuffer);
	void ReceiveDataCancel();
	void DoReceiveDataComplete();
	static void ReceiveDataDfc(TAny* aPtr);
public:
	// Interface methods for use by PDD
	virtual void SendDataComplete(TInt aResult);
	virtual void ReceiveDataComplete(TInt aResult);
private:
	DThread* iClient;
	// Members used for processing a SendData request
	TRequestStatus* iSendDataStatus;
	TDfc iSendDataDfc;
	TInt iSendDataResult;
	TBuf8<256> iSendDataBuffer;
	// Members used for processing a ReceiveData request
	TDes8* iReceiveDataDescriptor;
	TRequestStatus* iReceiveDataStatus;
	TDfc iReceiveDataDfc;
	TInt iReceiveDataResult;
	TBuf8<256> iReceiveDataBuffer;
	};

inline DDriver1* DDriver1Channel::Pdd()
	{ return (DDriver1*)iPdd; }

/**
  Interface to 'Driver1' physical device
*/
class DDriver1 : public DBase
	{
public:
	/**
	Structure for holding PDD capabilities information
	*/
	class TCaps
		{
	public:
		TVersion iVersion;
		};
public:
	virtual TInt BufferSize() const =0;
	virtual TInt Speed() const =0;
	virtual TInt SetSpeed(TInt aSpeed) =0;
	virtual TInt SendData(const TDesC8& aData) =0;
	virtual void SendDataCancel() =0;
	virtual TInt ReceiveData(TDes8& aBuffer) =0;
	virtual void ReceiveDataCancel() =0;
public:
	DDriver1Channel* iLdd;
	};

#endif

