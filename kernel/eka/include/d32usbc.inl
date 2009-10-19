// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/include/d32usbc.inl
// User side class definitions for USB Device support (inline header).
// 
//

/**
 @file d32usbc.inl
 @publishedPartner
 @released
*/

#ifndef __D32USBC_INL__
#define __D32USBC_INL__


/** @internalTechnology
*/
struct TUsbcIfcInfo
	{
	TUsbcInterfaceInfoBuf* iInterfaceData;
	TPtr8* iString;
	TUint32 iBandwidthPriority;
	};


/** @internalTechnology
*/
struct TEndpointTransferInfo
	{
	TDesC8* iDes;
	TTransferType iTransferType;
	TInt iTransferSize;
	TBool iZlpReqd;
	};

inline TUsbcInterfaceInfo::TUsbcInterfaceInfo(TInt aClass, TInt aSubClass,
											  TInt aProtocol, TDesC16* aString,
											  TUint aTotalEndpoints)
	: iClass(aClass, aSubClass, aProtocol), iString(aString),
	  iTotalEndpointsUsed(aTotalEndpoints), iEndpointData(), iFeatureWord(0)
	{}


#ifndef __KERNEL_MODE__

/** @capability CommDD
*/
inline TInt RDevUsbcClient::Open(TInt aUnit)
	{
	_LIT(KUsbDevName, "Usbc");
	return (DoCreate(KUsbDevName, VersionRequired(), aUnit, NULL, NULL, EOwnerThread));
	}


inline TVersion RDevUsbcClient::VersionRequired() const
	{
	return (TVersion(EMajorVersionNumber, EMinorVersionNumber, EBuildVersionNumber));
	}


inline TInt RDevUsbcClient::EndpointZeroRequestError()
	{
	return DoControl(EControlEndpointZeroRequestError);
	}


inline TInt RDevUsbcClient::EndpointCaps(TDes8& aCapsBuf)
	{
	return DoControl(EControlEndpointCaps, &aCapsBuf);
	}


inline TInt RDevUsbcClient::DeviceCaps(TUsbDeviceCaps& aCapsBuf)
	{
	return DoControl(EControlDeviceCaps, &aCapsBuf);
	}


inline TInt RDevUsbcClient::GetAlternateSetting(TInt &aInterfaceNumber)
	{
	return DoControl(EControlGetAlternateSetting, &aInterfaceNumber);
	}


inline TInt RDevUsbcClient::DeviceStatus(TUsbcDeviceState &aDeviceStatus)
	{
	return DoControl(EControlDeviceStatus, &aDeviceStatus);
	}


inline TInt RDevUsbcClient::EndpointStatus(TEndpointNumber aEndpoint,TEndpointState &aEndpointStatus)
	{
	return DoControl(EControlEndpointStatus,(TAny*) aEndpoint, &aEndpointStatus);
	}


inline TInt RDevUsbcClient::QueryReceiveBuffer(TEndpointNumber aEndpoint,TInt& aNumberOfBytes)
	{
	return DoControl(EControlQueryReceiveBuffer, (TAny*) aEndpoint, &aNumberOfBytes);
	}


inline TInt RDevUsbcClient::SendEp0StatusPacket()
	{
	return DoControl(EControlSendEp0StatusPacket);
	}


inline TInt RDevUsbcClient::HaltEndpoint(TEndpointNumber aEndpoint)
	{
	return DoControl(EControlHaltEndpoint, (TAny*) aEndpoint);
	}


inline TInt RDevUsbcClient::ClearHaltEndpoint(TEndpointNumber aEndpoint)
	{
	return DoControl(EControlClearHaltEndpoint, (TAny*) aEndpoint);
	}


inline TUint RDevUsbcClient::EndpointZeroMaxPacketSizes()
	{
	return DoControl(EControlEndpointZeroMaxPacketSizes);
	}


inline TInt RDevUsbcClient::SetEndpointZeroMaxPacketSize(TInt aMaxPacketSize)
	{
	return DoControl(EControlSetEndpointZeroMaxPacketSize, (TAny*) aMaxPacketSize);
	}


inline TInt RDevUsbcClient::GetEndpointZeroMaxPacketSize()
	{
	return DoControl(EControlGetEndpointZeroMaxPacketSize);
	}


inline TInt RDevUsbcClient::GetDeviceDescriptor(TDes8& aDeviceDescriptor)
	{
	return DoControl(EControlGetDeviceDescriptor, &aDeviceDescriptor);
	}


inline TInt RDevUsbcClient::SetDeviceDescriptor(const TDesC8& aDeviceDescriptor)
	{
	return DoControl(EControlSetDeviceDescriptor, const_cast<TDesC8*>(&aDeviceDescriptor));
	}


inline TInt RDevUsbcClient::GetDeviceDescriptorSize(TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TInt r = DoControl(EControlGetDeviceDescriptorSize, &p);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline TInt RDevUsbcClient::GetConfigurationDescriptor(TDes8& aConfigurationDescriptor)
	{
	return DoControl(EControlGetConfigurationDescriptor, &aConfigurationDescriptor);
	}


inline TInt RDevUsbcClient::SetConfigurationDescriptor(const TDesC8& aConfigurationDescriptor)
	{
	return DoControl(EControlSetConfigurationDescriptor, const_cast<TDesC8*> (&aConfigurationDescriptor));
	}


inline TInt RDevUsbcClient::GetConfigurationDescriptorSize(TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TInt r=DoControl(EControlGetConfigurationDescriptorSize, &p);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline TInt RDevUsbcClient::GetInterfaceDescriptor(TInt aSettingNumber, TDes8& aInterfaceDescriptor)
	{
	return DoControl(EControlGetInterfaceDescriptor,(TAny*) aSettingNumber, &aInterfaceDescriptor);
	}


inline TInt RDevUsbcClient::SetInterfaceDescriptor(TInt aSettingNumber, const TDesC8& aInterfaceDescriptor)
	{
	return DoControl(EControlSetInterfaceDescriptor,(TAny*) aSettingNumber,
					 const_cast<TDesC8*>(&aInterfaceDescriptor));
	}


inline TInt RDevUsbcClient::GetInterfaceDescriptorSize(TInt aSettingNumber, TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TInt r = DoControl(EControlGetInterfaceDescriptorSize,(TAny*) aSettingNumber, &p);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline TInt RDevUsbcClient::GetEndpointDescriptor(TInt aSettingNumber, TInt aEndpointNumber,
												  TDes8& aEndpointDescriptor)
	{
	TEndpointDescriptorInfo info = {aSettingNumber, aEndpointNumber, &aEndpointDescriptor};
	return DoControl(EControlGetEndpointDescriptor, &info, NULL);
	}


inline TInt RDevUsbcClient::SetEndpointDescriptor(TInt aSettingNumber, TInt aEndpointNumber,
												  const TDesC8& aEndpointDescriptor)
	{
	TEndpointDescriptorInfo info = {aSettingNumber, aEndpointNumber, const_cast<TDesC8*>(&aEndpointDescriptor)};
	return DoControl(EControlSetEndpointDescriptor, &info, NULL);
	}


inline TInt RDevUsbcClient::GetEndpointDescriptorSize(TInt aSettingNumber, TInt aEndpointNumber, TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TEndpointDescriptorInfo info = {aSettingNumber, aEndpointNumber, &p};
	TInt r = DoControl(EControlGetEndpointDescriptorSize, &info, NULL);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline void RDevUsbcClient::GetOtgDescriptorSize(TInt& aSize)
	{
	aSize = KUsbDescSize_Otg;
	}


inline TInt RDevUsbcClient::GetOtgDescriptor(TDes8& aOtgDesc)
	{
	return DoControl(EControlGetOtgDescriptor, (TAny*)&aOtgDesc);
	}


inline TInt RDevUsbcClient::SetOtgDescriptor(const TDesC8& aOtgDesc)
	{
	return DoControl(EControlSetOtgDescriptor, (TAny*)&aOtgDesc);
	}


inline TInt RDevUsbcClient::GetDeviceQualifierDescriptor(TDes8& aDescriptor)
	{
	return DoControl(EControlGetDeviceQualifierDescriptor, &aDescriptor);
	}


inline TInt RDevUsbcClient::SetDeviceQualifierDescriptor(const TDesC8& aDescriptor)
	{
	return DoControl(EControlSetDeviceQualifierDescriptor, const_cast<TDesC8*>(&aDescriptor));
	}


inline TInt RDevUsbcClient::GetOtherSpeedConfigurationDescriptor(TDes8& aDescriptor)
	{
	return DoControl(EControlGetOtherSpeedConfigurationDescriptor, &aDescriptor);
	}


inline TInt RDevUsbcClient::SetOtherSpeedConfigurationDescriptor(const TDesC8& aDescriptor)
	{
	return DoControl(EControlSetOtherSpeedConfigurationDescriptor, const_cast<TDesC8*> (&aDescriptor));
	}


inline TInt RDevUsbcClient::GetCSInterfaceDescriptorBlock(TInt aSettingNumber, TDes8& aInterfaceDescriptor)
	{
	return DoControl(EControlGetCSInterfaceDescriptor,(TAny*) aSettingNumber, &aInterfaceDescriptor);
	}


inline TInt RDevUsbcClient::GetCSInterfaceDescriptorBlockSize(TInt aSettingNumber, TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TInt r = DoControl(EControlGetCSInterfaceDescriptorSize,(TAny*) aSettingNumber, &p);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline TInt RDevUsbcClient::GetCSEndpointDescriptorBlock(TInt aSettingNumber, TInt aEndpointNumber,
														 TDes8& aEndpointDescriptor)
	{
	TEndpointDescriptorInfo info={aSettingNumber, aEndpointNumber, &aEndpointDescriptor};
	return DoControl(EControlGetCSEndpointDescriptor,&info,NULL);
	}


inline TInt RDevUsbcClient::GetCSEndpointDescriptorBlockSize(TInt aSettingNumber, TInt aEndpointNumber,
															 TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TEndpointDescriptorInfo info = {aSettingNumber, aEndpointNumber, &p};
	TInt r = DoControl(EControlGetCSEndpointDescriptorSize, &info, NULL);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline TInt RDevUsbcClient::SignalRemoteWakeup()
	{
	return DoControl(EControlSignalRemoteWakeup);
	}


inline TInt RDevUsbcClient::DeviceDisconnectFromHost()
	{
	return DoControl(EControlDeviceDisconnectFromHost);
	}


inline TInt RDevUsbcClient::DeviceConnectToHost()
	{
	return DoControl(EControlDeviceConnectToHost);
	}


inline TInt RDevUsbcClient::PowerUpUdc()
	{
	return DoControl(EControlDevicePowerUpUdc);
	}


inline TBool RDevUsbcClient::CurrentlyUsingHighSpeed()
	{
	return DoControl(EControlCurrentlyUsingHighSpeed);
	}


inline TInt RDevUsbcClient::SetInterface(TInt aInterfaceNumber, TUsbcInterfaceInfoBuf& aInterfaceData,
										 TUint32 aBandwidthPriority)
	{
	TPtr8 name_8(NULL,0);
	TUsbcIfcInfo ifcinfo;
	ifcinfo.iInterfaceData = const_cast<TUsbcInterfaceInfoBuf*>(&aInterfaceData);
	if (!aInterfaceData().iString)
		{
		ifcinfo.iString = NULL;
		}
	else
		{
		name_8.Set(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aInterfaceData().iString->Ptr())),
				   aInterfaceData().iString->Size(), aInterfaceData().iString->Size());
		ifcinfo.iString = &name_8;
		}
	ifcinfo.iBandwidthPriority = aBandwidthPriority;
	return DoControl(EControlSetInterface, (TAny*)aInterfaceNumber, &ifcinfo);
	}


inline TInt RDevUsbcClient::ReleaseInterface(TInt aInterfaceNumber)
	{
	return DoControl(EControlReleaseInterface, (TAny*)aInterfaceNumber);
	}


inline TInt RDevUsbcClient::SetCSInterfaceDescriptorBlock(TInt aSettingNumber, const TDesC8& aInterfaceDescriptor)
	{
	TCSDescriptorInfo info = {aSettingNumber, 0, const_cast<TDesC8*>(&aInterfaceDescriptor),
							  aInterfaceDescriptor.Size()};
	return DoControl(EControlSetCSInterfaceDescriptor, &info, NULL);
	}


inline TInt RDevUsbcClient::SetCSEndpointDescriptorBlock(TInt aSettingNumber, TInt aEndpointNumber,
														 const TDesC8& aEndpointDescriptor)
	{
	TCSDescriptorInfo info = {aSettingNumber, aEndpointNumber, const_cast<TDesC8*>(&aEndpointDescriptor),
							  aEndpointDescriptor.Size()};
	return DoControl(EControlSetCSEndpointDescriptor, &info, NULL);
	}


inline TInt RDevUsbcClient::SetDeviceControl()
	{
	return DoControl(EControlSetDeviceControl);
	}


inline TInt RDevUsbcClient::ReleaseDeviceControl()
	{
	return DoControl(EControlReleaseDeviceControl);
	}


inline TInt RDevUsbcClient::GetStringDescriptorLangId(TUint16& aLangId)
	{
	TPckgBuf<TUint16> p;
	const TInt r = DoControl(EControlGetStringDescriptorLangId, &p);
	if (r == KErrNone)
		aLangId = p();
	return r;
	}


inline TInt RDevUsbcClient::SetStringDescriptorLangId(TUint16 aLangId)
	{
	return DoControl(EControlSetStringDescriptorLangId, (TAny*)(TUint)aLangId);
	}


inline TInt RDevUsbcClient::GetManufacturerStringDescriptor(TDes16& aString)
	{
	TPtr8 name_8(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aString.Ptr())), aString.MaxSize());
	const TInt r = DoControl(EControlGetManufacturerStringDescriptor, &name_8);
	aString.SetLength(name_8.Size()/2);
	return r;
	}


inline TInt RDevUsbcClient::SetManufacturerStringDescriptor(const TDesC16& aString)
	{
	TPtrC8 name_8(reinterpret_cast<const TUint8*>(aString.Ptr()), aString.Size());
	return DoControl(EControlSetManufacturerStringDescriptor, &name_8);
	}


inline TInt RDevUsbcClient::RemoveManufacturerStringDescriptor()
	{
	return DoControl(EControlRemoveManufacturerStringDescriptor);
	}


inline TInt RDevUsbcClient::GetProductStringDescriptor(TDes16& aString)
	{
	TPtr8 name_8(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aString.Ptr())), aString.MaxSize());
	const TInt r = DoControl(EControlGetProductStringDescriptor, &name_8);
	aString.SetLength(name_8.Size()/2);
	return r;
	}


inline TInt RDevUsbcClient::SetProductStringDescriptor(const TDesC16& aString)
	{
	TPtrC8 name_8(reinterpret_cast<const TUint8*>(aString.Ptr()), aString.Size());
	return DoControl(EControlSetProductStringDescriptor, &name_8);
	}


inline TInt RDevUsbcClient::RemoveProductStringDescriptor()
	{
	return DoControl(EControlRemoveProductStringDescriptor);
	}


inline TInt RDevUsbcClient::GetSerialNumberStringDescriptor(TDes16& aString)
	{
	TPtr8 name_8(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aString.Ptr())), aString.MaxSize());
	const TInt r = DoControl(EControlGetSerialNumberStringDescriptor, &name_8);
	aString.SetLength(name_8.Size()/2);
	return r;
	}


inline TInt RDevUsbcClient::SetSerialNumberStringDescriptor(const TDesC16& aString)
	{
	TPtrC8 name_8(reinterpret_cast<const TUint8*>(aString.Ptr()), aString.Size());
	return DoControl(EControlSetSerialNumberStringDescriptor, &name_8);
	}


inline TInt RDevUsbcClient::RemoveSerialNumberStringDescriptor()
	{
	return DoControl(EControlRemoveSerialNumberStringDescriptor);
	}


inline TInt RDevUsbcClient::GetConfigurationStringDescriptor(TDes16& aString)
	{
	TPtr8 name_8(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aString.Ptr())), aString.MaxSize());
	const TInt r = DoControl(EControlGetConfigurationStringDescriptor, &name_8);
	aString.SetLength(name_8.Size() / 2);
	return r;
	}


inline TInt RDevUsbcClient::SetConfigurationStringDescriptor(const TDesC16& aString)
	{
	TPtrC8 name_8(reinterpret_cast<const TUint8*>(aString.Ptr()), aString.Size());
	return DoControl(EControlSetConfigurationStringDescriptor, &name_8);
	}


inline TInt RDevUsbcClient::RemoveConfigurationStringDescriptor()
	{
	return DoControl(EControlRemoveConfigurationStringDescriptor);
	}


inline TInt RDevUsbcClient::GetStringDescriptor(TUint8 aIndex, TDes16& aString)
	{
	TPtr8 name_8(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aString.Ptr())), aString.MaxSize());
	const TInt r = DoControl(EControlGetStringDescriptor, (TAny*)(TUint)aIndex, &name_8);
	aString.SetLength(name_8.Size() / 2);
	return r;
	}


inline TInt RDevUsbcClient::SetStringDescriptor(TUint8 aIndex, const TDesC16& aString)
	{
	TPtrC8 name_8(reinterpret_cast<const TUint8*>(aString.Ptr()), aString.Size());
	return DoControl(EControlSetStringDescriptor, (TAny*)(TUint)aIndex, &name_8);
	}


inline TInt RDevUsbcClient::RemoveStringDescriptor(TUint8 aIndex)
	{
	return DoControl(EControlRemoveStringDescriptor, (TAny*)(TUint)aIndex);
	}


inline TInt RDevUsbcClient::AllocateEndpointResource(TInt aEndpoint, TUsbcEndpointResource aResource)
	{
	return DoControl(EControlAllocateEndpointResource, (TAny*)aEndpoint, (TAny*)aResource);
	}


inline TInt RDevUsbcClient::DeAllocateEndpointResource(TInt aEndpoint, TUsbcEndpointResource aResource)
	{
	return DoControl(EControlDeAllocateEndpointResource, (TAny*)aEndpoint, (TAny*)aResource);
	}


inline TBool RDevUsbcClient::QueryEndpointResourceUse(TInt aEndpoint, TUsbcEndpointResource aResource)
	{
	return DoControl(EControlQueryEndpointResourceUse, (TAny*)aEndpoint, (TAny*)aResource);
	}


inline void RDevUsbcClient::ReadUntilShort(TRequestStatus &aStatus, TEndpointNumber aEndpoint, TDes8 &aDes)
	{
	TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
	TEndpointTransferInfo info = {&aDes, ETransferTypeReadUntilShort, aDes.MaxLength()};
	DoRequest(ep, aStatus, &info, NULL);
	}


inline void RDevUsbcClient::ReadUntilShort(TRequestStatus &aStatus, TEndpointNumber aEndpoint, TDes8 &aDes,
										   TInt aLen)
	{
	TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
	TEndpointTransferInfo info = {&aDes, ETransferTypeReadUntilShort, aLen};
	DoRequest(ep, aStatus, &info, NULL);
	}


inline void RDevUsbcClient::ReadOneOrMore(TRequestStatus &aStatus, TEndpointNumber aEndpoint, TDes8 &aDes)
	{
	TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
	TEndpointTransferInfo info = {&aDes, ETransferTypeReadOneOrMore, aDes.MaxLength()};
	DoRequest(ep, aStatus, &info, NULL);
	}


inline void RDevUsbcClient::ReadOneOrMore(TRequestStatus &aStatus, TEndpointNumber aEndpoint, TDes8 &aDes,
										  TInt aLen)
	{
	TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
	TEndpointTransferInfo info = {&aDes, ETransferTypeReadOneOrMore, aLen};
	DoRequest(ep, aStatus, &info, NULL);
	}


inline void RDevUsbcClient::Read(TRequestStatus &aStatus, TEndpointNumber aEndpoint, TDes8 &aDes)
	{
	TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
	TEndpointTransferInfo info = {&aDes, ETransferTypeReadData, aDes.MaxLength()};
	DoRequest(ep, aStatus, &info, NULL);
	}


inline void RDevUsbcClient::Read(TRequestStatus &aStatus, TEndpointNumber aEndpoint, TDes8 &aDes, TInt aLen)
	{
	TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
	TEndpointTransferInfo info = {&aDes, ETransferTypeReadData, aLen};
	DoRequest(ep, aStatus, &info, NULL);
	}


inline void RDevUsbcClient::ReadPacket(TRequestStatus &aStatus, TEndpointNumber aEndpoint, TDes8 &aDes,
									   TInt aMaxLen)
	{
	TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
	TEndpointTransferInfo info = {&aDes, ETransferTypeReadPacket, aMaxLen};
	DoRequest(ep, aStatus, &info, NULL);
	}


inline void RDevUsbcClient::Write(TRequestStatus &aStatus, TEndpointNumber aEndpoint, const TDesC8& aDes,
								  TInt aLen, TBool aZlpRequired)
	{
	TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
	TEndpointTransferInfo info = {const_cast<TDesC8*>(&aDes), ETransferTypeWrite, aLen, aZlpRequired};
	DoRequest(ep, aStatus, &info, NULL);
	}


inline void RDevUsbcClient::AlternateDeviceStatusNotify(TRequestStatus& aStatus, TUint& aValue)
	{
	DoRequest(ERequestAlternateDeviceStatusNotify, aStatus, &aValue);
	}


inline void RDevUsbcClient::ReEnumerate(TRequestStatus& aStatus)
	{
	DoRequest(ERequestReEnumerate, aStatus);
	}


inline void RDevUsbcClient::EndpointStatusNotify(TRequestStatus& aStatus, TUint& aEndpointMask)
	{
	DoRequest(ERequestEndpointStatusNotify, aStatus, &aEndpointMask);
	}


inline void RDevUsbcClient::ReadCancel(TEndpointNumber aEndpoint)
	{
	if (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient)
		return;
	DoCancel(1 << aEndpoint);
	}


inline void RDevUsbcClient::WriteCancel(TEndpointNumber aEndpoint)
	{
	ReadCancel(aEndpoint);
	}


inline void RDevUsbcClient::EndpointTransferCancel(TUint aEndpointMask)
	{
	/* Mask off non-endpoint cancels */
	DoCancel(aEndpointMask & ERequestAllCancel);
	}


inline void RDevUsbcClient::AlternateDeviceStatusNotifyCancel()
	{
	DoCancel(ERequestAlternateDeviceStatusNotifyCancel);
	}


inline void RDevUsbcClient::ReEnumerateCancel()
	{
	DoCancel(ERequestReEnumerateCancel);
	}


inline void RDevUsbcClient::EndpointStatusNotifyCancel()
	{
	DoCancel(ERequestEndpointStatusNotifyCancel);
	}

inline TInt RDevUsbcClient::GetOtgFeatures(TUint8& aFeatures)
	{
	TPckgBuf<TUint8> p;
	TInt r = DoControl(EControlGetOtgFeatures, &p);
	if (r == KErrNone)
		aFeatures = p();
	return r;
	}


inline void RDevUsbcClient::OtgFeaturesNotify(TRequestStatus& aStatus, TUint8& aValue)
	{
	DoRequest(ERequestOtgFeaturesNotify, aStatus, &aValue);
	}


inline void RDevUsbcClient::OtgFeaturesNotifyCancel()
	{
	DoCancel(ERequestOtgFeaturesNotifyCancel);
	}


#endif // #ifndef __KERNEL_MODE__

#endif // #ifndef __D32USBC_INL__
