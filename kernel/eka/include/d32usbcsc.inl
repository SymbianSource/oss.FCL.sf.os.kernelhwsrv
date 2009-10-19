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
// e32\include\d32usbcsc.inl
// User side class definitions for USB Device support (inline header).
// 
//

/** @file d32usbcsc.inl
	@publishedPartner
	@released
*/

#ifndef __D32USBCSC_INL__
#define __D32USBCSC_INL__


/** @internalTechnology
*/
struct TUsbcScIfcInfo
	{
	TUsbcScInterfaceInfoBuf* iInterfaceData;
	TPtr8* iString;
	TUint32 iBandwidthPriority;
	};


inline TUsbcScHdrEndpointRecord::TUsbcScHdrEndpointRecord(TInt aBufferNo, TUint8 aType)
:	iBufferNo((TUint8)aBufferNo),
 	iType(aType)
 	{
	};


inline TUint TUsbcScHdrEndpointRecord::Type() const
	{
		return (TUint) (iType>>2);
	};

inline TUint TUsbcScHdrEndpointRecord::Direction() const
	{
		return (TUint) (iType&3);
	};


inline void TUsbcScBufferRecord::Set(TUint aOffset, TUint aEndOffset)
	{
	iOffset = aOffset;
	iSize = aEndOffset-aOffset;
	};

inline TUint TUsbcScBufferRecord::Offset() const
	{
	return iOffset;
	};

inline TUint TUsbcScBufferRecord::Size() const
	{
	return iSize;
	};


inline TEndpointPairInfo::TEndpointPairInfo(TUint8 aType, TUint16 aPair, TUint8 aSpare)
	: iType(aType), iSpare(aSpare), iPair(aPair)
	{}


inline TUsbcScEndpointInfo::TUsbcScEndpointInfo(TUint aType, TUint aDir, TInt aInterval, TInt aExtra,
												TUint aBufferSize, TUint aReadSize
)
	: TUsbcEndpointInfo(aType, aDir, 0, aInterval, aExtra),
 	  iBufferSize(aBufferSize),  iReadSize(aReadSize), iPairing(), iAlignment(0), iFlags(0)
	{}




inline TUsbcScInterfaceInfo::TUsbcScInterfaceInfo(TInt aClass, TInt aSubClass,
											  TInt aProtocol, TDesC16* aString,
											  TUint aTotalEndpoints)
	: iClass(aClass, aSubClass, aProtocol), iString(aString),
	  iTotalEndpointsUsed(aTotalEndpoints), iFeatureWord(0)
	{}


inline TUsbcScBufferRecord* TUsbcScChunkBuffersHeader::Ep0Out() const
	{
	return (TUsbcScBufferRecord*) &iBufferOffset[0];
	};
inline TUsbcScBufferRecord* TUsbcScChunkBuffersHeader::Ep0In() const
	{
	return (TUsbcScBufferRecord*) &iBufferOffset[iRecordSize];
	};
inline TUsbcScBufferRecord* TUsbcScChunkBuffersHeader::Buffers(TInt aBuffer) const
	{
	return (TUsbcScBufferRecord*) &iBufferOffset[(aBuffer+2)*iRecordSize];
	};

inline TInt TUsbcScChunkBuffersHeader::NumberOfBuffers() const
	{
	return iNumOfBufs;
	};


#ifndef __KERNEL_MODE__



/** @capability CommDD
*/
inline TInt RDevUsbcScClient::Open(TInt aUnit)
	{
	_LIT(KUsbDevName, "usbcsc");
	return (DoCreate(KUsbDevName, VersionRequired(), aUnit, NULL, NULL, EOwnerThread));
	}


inline TVersion RDevUsbcScClient::VersionRequired() const
	{
	return (TVersion(EMajorVersionNumber, EMinorVersionNumber, EBuildVersionNumber));
	}


inline TInt RDevUsbcScClient::EndpointZeroRequestError()
	{
	return DoControl(EControlEndpointZeroRequestError);
	}


inline TInt RDevUsbcScClient::EndpointCaps(TDes8& aCapsBuf)
	{
	return DoControl(EControlEndpointCaps, &aCapsBuf);
	}


inline TInt RDevUsbcScClient::DeviceCaps(TUsbDeviceCaps& aCapsBuf)
	{
	return DoControl(EControlDeviceCaps, &aCapsBuf);
	}


inline TInt RDevUsbcScClient::GetAlternateSetting(TInt &aInterfaceNumber)
	{
	return DoControl(EControlGetAlternateSetting, &aInterfaceNumber);
	}


inline TInt RDevUsbcScClient::DeviceStatus(TUsbcDeviceState &aDeviceStatus)
	{
	return DoControl(EControlDeviceStatus, &aDeviceStatus);
	}


inline TInt RDevUsbcScClient::EndpointStatus(TInt aEndpoint,TEndpointState &aEndpointStatus)
	{
	return DoControl(EControlEndpointStatus,(TAny*) aEndpoint, &aEndpointStatus);
	}

/*
inline TInt RDevUsbcScClient::QueryReceiveBuffer(TInt aEndpoint,TInt& aNumberOfBytes)
	{
	return DoControl(EControlQueryReceiveBuffer, (TAny*) aEndpoint, &aNumberOfBytes);
	}

*/
inline TInt RDevUsbcScClient::SendEp0StatusPacket()
	{
	return DoControl(EControlSendEp0StatusPacket);
	}


inline TInt RDevUsbcScClient::HaltEndpoint(TInt aEndpoint)
	{
	return DoControl(EControlHaltEndpoint, (TAny*) aEndpoint);
	}


inline TInt RDevUsbcScClient::ClearHaltEndpoint(TInt aEndpoint)
	{
	return DoControl(EControlClearHaltEndpoint, (TAny*) aEndpoint);
	}


inline TUint RDevUsbcScClient::EndpointZeroMaxPacketSizes()
	{
	return DoControl(EControlEndpointZeroMaxPacketSizes);
	}


inline TInt RDevUsbcScClient::SetEndpointZeroMaxPacketSize(TInt aMaxPacketSize)
	{
	return DoControl(EControlSetEndpointZeroMaxPacketSize, (TAny*) aMaxPacketSize);
	}


inline TInt RDevUsbcScClient::GetEndpointZeroMaxPacketSize()
	{
	return DoControl(EControlGetEndpointZeroMaxPacketSize);
	}


inline TInt RDevUsbcScClient::GetDeviceDescriptor(TDes8& aDeviceDescriptor)
	{
	return DoControl(EControlGetDeviceDescriptor, &aDeviceDescriptor);
	}


inline TInt RDevUsbcScClient::SetDeviceDescriptor(const TDesC8& aDeviceDescriptor)
	{
	return DoControl(EControlSetDeviceDescriptor, const_cast<TDesC8*>(&aDeviceDescriptor));
	}


inline TInt RDevUsbcScClient::GetDeviceDescriptorSize(TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TInt r = DoControl(EControlGetDeviceDescriptorSize, &p);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline TInt RDevUsbcScClient::GetConfigurationDescriptor(TDes8& aConfigurationDescriptor)
	{
	return DoControl(EControlGetConfigurationDescriptor, &aConfigurationDescriptor);
	}


inline TInt RDevUsbcScClient::SetConfigurationDescriptor(const TDesC8& aConfigurationDescriptor)
	{
	return DoControl(EControlSetConfigurationDescriptor, const_cast<TDesC8*> (&aConfigurationDescriptor));
	}


inline TInt RDevUsbcScClient::GetConfigurationDescriptorSize(TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TInt r=DoControl(EControlGetConfigurationDescriptorSize, &p);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline TInt RDevUsbcScClient::GetInterfaceDescriptor(TInt aSettingNumber, TDes8& aInterfaceDescriptor)
	{
	return DoControl(EControlGetInterfaceDescriptor,(TAny*) aSettingNumber, &aInterfaceDescriptor);
	}


inline TInt RDevUsbcScClient::SetInterfaceDescriptor(TInt aSettingNumber, const TDesC8& aInterfaceDescriptor)
	{
	return DoControl(EControlSetInterfaceDescriptor,(TAny*) aSettingNumber,
					 const_cast<TDesC8*>(&aInterfaceDescriptor));
	}


inline TInt RDevUsbcScClient::GetInterfaceDescriptorSize(TInt aSettingNumber, TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TInt r = DoControl(EControlGetInterfaceDescriptorSize,(TAny*) aSettingNumber, &p);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline TInt RDevUsbcScClient::GetEndpointDescriptor(TInt aSettingNumber, TInt aEndpointNumber,
												  TDes8& aEndpointDescriptor)
	{
	TEndpointDescriptorInfo info = {aSettingNumber, aEndpointNumber, &aEndpointDescriptor};
	return DoControl(EControlGetEndpointDescriptor, &info, NULL);
	}


inline TInt RDevUsbcScClient::SetEndpointDescriptor(TInt aSettingNumber, TInt aEndpointNumber,
												  const TDesC8& aEndpointDescriptor)
	{
	TEndpointDescriptorInfo info = {aSettingNumber, aEndpointNumber, const_cast<TDesC8*>(&aEndpointDescriptor)};
	return DoControl(EControlSetEndpointDescriptor, &info, NULL);
	}


inline TInt RDevUsbcScClient::GetEndpointDescriptorSize(TInt aSettingNumber, TInt aEndpointNumber, TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TEndpointDescriptorInfo info = {aSettingNumber, aEndpointNumber, &p};
	TInt r = DoControl(EControlGetEndpointDescriptorSize, &info, NULL);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline void RDevUsbcScClient::GetOtgDescriptorSize(TInt& aSize)
	{
	aSize = KUsbDescSize_Otg;
	}


inline TInt RDevUsbcScClient::GetOtgDescriptor(TDes8& aOtgDesc)
	{
	return DoControl(EControlGetOtgDescriptor, (TAny*)&aOtgDesc);
	}


inline TInt RDevUsbcScClient::SetOtgDescriptor(const TDesC8& aOtgDesc)
	{
	return DoControl(EControlSetOtgDescriptor, (TAny*)&aOtgDesc);
	}


inline TInt RDevUsbcScClient::GetDeviceQualifierDescriptor(TDes8& aDescriptor)
	{
	return DoControl(EControlGetDeviceQualifierDescriptor, &aDescriptor);
	}


inline TInt RDevUsbcScClient::SetDeviceQualifierDescriptor(const TDesC8& aDescriptor)
	{
	return DoControl(EControlSetDeviceQualifierDescriptor, const_cast<TDesC8*>(&aDescriptor));
	}


inline TInt RDevUsbcScClient::GetOtherSpeedConfigurationDescriptor(TDes8& aDescriptor)
	{
	return DoControl(EControlGetOtherSpeedConfigurationDescriptor, &aDescriptor);
	}


inline TInt RDevUsbcScClient::SetOtherSpeedConfigurationDescriptor(const TDesC8& aDescriptor)
	{
	return DoControl(EControlSetOtherSpeedConfigurationDescriptor, const_cast<TDesC8*> (&aDescriptor));
	}


inline TInt RDevUsbcScClient::GetCSInterfaceDescriptorBlock(TInt aSettingNumber, TDes8& aInterfaceDescriptor)
	{
	return DoControl(EControlGetCSInterfaceDescriptor,(TAny*) aSettingNumber, &aInterfaceDescriptor);
	}


inline TInt RDevUsbcScClient::GetCSInterfaceDescriptorBlockSize(TInt aSettingNumber, TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TInt r = DoControl(EControlGetCSInterfaceDescriptorSize,(TAny*) aSettingNumber, &p);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline TInt RDevUsbcScClient::GetCSEndpointDescriptorBlock(TInt aSettingNumber, TInt aEndpointNumber,
														 TDes8& aEndpointDescriptor)
	{
	TEndpointDescriptorInfo info={aSettingNumber, aEndpointNumber, &aEndpointDescriptor};
	return DoControl(EControlGetCSEndpointDescriptor,&info,NULL);
	}


inline TInt RDevUsbcScClient::GetCSEndpointDescriptorBlockSize(TInt aSettingNumber, TInt aEndpointNumber,
															 TInt& aSize)
	{
	TPckgBuf<TInt> p;
	TEndpointDescriptorInfo info = {aSettingNumber, aEndpointNumber, &p};
	TInt r = DoControl(EControlGetCSEndpointDescriptorSize, &info, NULL);
	if (r == KErrNone)
		aSize = p();
	return r;
	}


inline TInt RDevUsbcScClient::SignalRemoteWakeup()
	{
	return DoControl(EControlSignalRemoteWakeup);
	}


inline TInt RDevUsbcScClient::DeviceDisconnectFromHost()
	{
	return DoControl(EControlDeviceDisconnectFromHost);
	}


inline TInt RDevUsbcScClient::DeviceConnectToHost()
	{
	return DoControl(EControlDeviceConnectToHost);
	}


inline TInt RDevUsbcScClient::PowerUpUdc()
	{
	return DoControl(EControlDevicePowerUpUdc);
	}


inline TBool RDevUsbcScClient::CurrentlyUsingHighSpeed()
	{
	return DoControl(EControlCurrentlyUsingHighSpeed);
	}


inline TInt RDevUsbcScClient::SetInterface(TInt aInterfaceNumber, TUsbcScInterfaceInfoBuf& aInterfaceData)
	{
	TPtr8 name_8(NULL,0);
	TUsbcScIfcInfo ifcinfo;
	ifcinfo.iInterfaceData = const_cast<TUsbcScInterfaceInfoBuf*>(&aInterfaceData);
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
	return DoControl(EControlSetInterface, (TAny*)aInterfaceNumber, &ifcinfo);
	}

inline TInt RDevUsbcScClient::RealizeInterface(RChunk& aChunk)
	{
	return aChunk.SetReturnedHandle(DoControl(EControlRealizeInterface));
	}


inline TInt RDevUsbcScClient::ReleaseInterface(TInt aInterfaceNumber)
	{
	return DoControl(EControlReleaseInterface, (TAny*)aInterfaceNumber);
	}


inline TInt RDevUsbcScClient::SetCSInterfaceDescriptorBlock(TInt aSettingNumber, const TDesC8& aInterfaceDescriptor)
	{
	TCSDescriptorInfo info = {aSettingNumber, 0, const_cast<TDesC8*>(&aInterfaceDescriptor),
							  aInterfaceDescriptor.Size()};
	return DoControl(EControlSetCSInterfaceDescriptor, &info, NULL);
	}


inline TInt RDevUsbcScClient::SetCSEndpointDescriptorBlock(TInt aSettingNumber, TInt aEndpointNumber,
														 const TDesC8& aEndpointDescriptor)
	{
	TCSDescriptorInfo info = {aSettingNumber, aEndpointNumber, const_cast<TDesC8*>(&aEndpointDescriptor),
							  aEndpointDescriptor.Size()};
	return DoControl(EControlSetCSEndpointDescriptor, &info, NULL);
	}


inline TInt RDevUsbcScClient::SetDeviceControl()
	{
	return DoControl(EControlSetDeviceControl);
	}


inline TInt RDevUsbcScClient::ReleaseDeviceControl()
	{
	return DoControl(EControlReleaseDeviceControl);
	}


inline TInt RDevUsbcScClient::GetStringDescriptorLangId(TUint16& aLangId)
	{
	TPckgBuf<TUint16> p;
	const TInt r = DoControl(EControlGetStringDescriptorLangId, &p);
	if (r == KErrNone)
		aLangId = p();
	return r;
	}


inline TInt RDevUsbcScClient::SetStringDescriptorLangId(TUint16 aLangId)
	{
	return DoControl(EControlSetStringDescriptorLangId, (TAny*)(TUint)aLangId);
	}


inline TInt RDevUsbcScClient::GetManufacturerStringDescriptor(TDes16& aString)
	{
	TPtr8 name_8(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aString.Ptr())), aString.MaxSize());
	const TInt r = DoControl(EControlGetManufacturerStringDescriptor, &name_8);
	aString.SetLength(name_8.Size()/2);
	return r;
	}


inline TInt RDevUsbcScClient::SetManufacturerStringDescriptor(const TDesC16& aString)
	{
	TPtrC8 name_8(reinterpret_cast<const TUint8*>(aString.Ptr()), aString.Size());
	return DoControl(EControlSetManufacturerStringDescriptor, &name_8);
	}


inline TInt RDevUsbcScClient::RemoveManufacturerStringDescriptor()
	{
	return DoControl(EControlRemoveManufacturerStringDescriptor);
	}


inline TInt RDevUsbcScClient::GetProductStringDescriptor(TDes16& aString)
	{
	TPtr8 name_8(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aString.Ptr())), aString.MaxSize());
	const TInt r = DoControl(EControlGetProductStringDescriptor, &name_8);
	aString.SetLength(name_8.Size()/2);
	return r;
	}


inline TInt RDevUsbcScClient::SetProductStringDescriptor(const TDesC16& aString)
	{
	TPtrC8 name_8(reinterpret_cast<const TUint8*>(aString.Ptr()), aString.Size());
	return DoControl(EControlSetProductStringDescriptor, &name_8);
	}


inline TInt RDevUsbcScClient::RemoveProductStringDescriptor()
	{
	return DoControl(EControlRemoveProductStringDescriptor);
	}


inline TInt RDevUsbcScClient::GetSerialNumberStringDescriptor(TDes16& aString)
	{
	TPtr8 name_8(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aString.Ptr())), aString.MaxSize());
	const TInt r = DoControl(EControlGetSerialNumberStringDescriptor, &name_8);
	aString.SetLength(name_8.Size()/2);
	return r;
	}


inline TInt RDevUsbcScClient::SetSerialNumberStringDescriptor(const TDesC16& aString)
	{
	TPtrC8 name_8(reinterpret_cast<const TUint8*>(aString.Ptr()), aString.Size());
	return DoControl(EControlSetSerialNumberStringDescriptor, &name_8);
	}


inline TInt RDevUsbcScClient::RemoveSerialNumberStringDescriptor()
	{
	return DoControl(EControlRemoveSerialNumberStringDescriptor);
	}


inline TInt RDevUsbcScClient::GetConfigurationStringDescriptor(TDes16& aString)
	{
	TPtr8 name_8(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aString.Ptr())), aString.MaxSize());
	const TInt r = DoControl(EControlGetConfigurationStringDescriptor, &name_8);
	aString.SetLength(name_8.Size() / 2);
	return r;
	}


inline TInt RDevUsbcScClient::SetConfigurationStringDescriptor(const TDesC16& aString)
	{
	TPtrC8 name_8(reinterpret_cast<const TUint8*>(aString.Ptr()), aString.Size());
	return DoControl(EControlSetConfigurationStringDescriptor, &name_8);
	}


inline TInt RDevUsbcScClient::RemoveConfigurationStringDescriptor()
	{
	return DoControl(EControlRemoveConfigurationStringDescriptor);
	}


inline TInt RDevUsbcScClient::GetStringDescriptor(TUint8 aIndex, TDes16& aString)
	{
	TPtr8 name_8(const_cast<TUint8*>(reinterpret_cast<const TUint8*>(aString.Ptr())), aString.MaxSize());
	const TInt r = DoControl(EControlGetStringDescriptor, (TAny*)(TUint)aIndex, &name_8);
	aString.SetLength(name_8.Size() / 2);
	return r;
	}


inline TInt RDevUsbcScClient::SetStringDescriptor(TUint8 aIndex, const TDesC16& aString)
	{
	TPtrC8 name_8(reinterpret_cast<const TUint8*>(aString.Ptr()), aString.Size());
	return DoControl(EControlSetStringDescriptor, (TAny*)(TUint)aIndex, &name_8);
	}


inline TInt RDevUsbcScClient::RemoveStringDescriptor(TUint8 aIndex)
	{
	return DoControl(EControlRemoveStringDescriptor, (TAny*)(TUint)aIndex);
	}


inline TInt RDevUsbcScClient::AllocateEndpointResource(TInt aEndpoint, TUsbcEndpointResource aResource)
	{
	return DoControl(EControlAllocateEndpointResource, (TAny*)aEndpoint, (TAny*)aResource);
	}


inline TInt RDevUsbcScClient::DeAllocateEndpointResource(TInt aEndpoint, TUsbcEndpointResource aResource)
	{
	return DoControl(EControlDeAllocateEndpointResource, (TAny*)aEndpoint, (TAny*)aResource);
	}


inline TBool RDevUsbcScClient::QueryEndpointResourceUse(TInt aEndpoint, TUsbcEndpointResource aResource)
	{
	return DoControl(EControlQueryEndpointResourceUse, (TAny*)aEndpoint, (TAny*)aResource);
	}


inline TInt RDevUsbcScClient::ReadDataNotify(TInt aBufferNumber, TRequestStatus& aStatus, TInt aLength)
	{
	TAny *a[2];
	a[0]=(TAny*) aBufferNumber;
	a[1]=(TAny*) aLength;
	aStatus=KRequestPending;
	TInt r = DoControl(~ERequestReadDataNotify, &aStatus, &a[0]);
	if (r)
		aStatus=r;
	return r;
	}



inline void RDevUsbcScClient::WriteData(TInt aBufferNumber, TUint aStart, TUint aLength, TUint aFlags, TRequestStatus& aStatus)
	{
	DoRequest( ERequestWriteData | ((aBufferNumber&KFieldBuffMask) << KFieldBuffPos) | ((aFlags&KFieldFlagsMask) << KFieldFlagsPos),
			   aStatus, (TAny*) aStart, (TAny*) aLength);
	}



inline void RDevUsbcScClient::AlternateDeviceStatusNotify(TRequestStatus& aStatus, TUint& aValue)
	{
	DoRequest(ERequestAlternateDeviceStatusNotify, aStatus, &aValue);
	}


inline void RDevUsbcScClient::ReEnumerate(TRequestStatus& aStatus)
	{
	DoRequest(ERequestReEnumerate, aStatus);
	}


inline void RDevUsbcScClient::EndpointStatusNotify(TRequestStatus& aStatus, TUint& aEndpointMask)
	{
	DoRequest(ERequestEndpointStatusNotify, aStatus, &aEndpointMask);
	}


inline void RDevUsbcScClient::ReadCancel(TInt aBuffer)
	{
	DoControl(ERequestReadDataNotifyCancel, (TAny*) aBuffer);
	}


inline void RDevUsbcScClient::WriteCancel(TInt aBuffer)
	{
	DoControl(ERequestWriteDataCancel, (TAny*) aBuffer);
	}


inline void RDevUsbcScClient::EndpointTransferCancel(TUint aBufferMask)
	{
	DoControl(ERequestCancel, (TAny*) aBufferMask);
	}


inline void RDevUsbcScClient::AlternateDeviceStatusNotifyCancel()
	{
	DoControl(ERequestAlternateDeviceStatusNotifyCancel);
	}


inline void RDevUsbcScClient::ReEnumerateCancel()
	{
	DoControl(ERequestReEnumerateCancel);
	}


inline void RDevUsbcScClient::EndpointStatusNotifyCancel()
	{
	DoControl(ERequestEndpointStatusNotifyCancel);
	}

inline TInt RDevUsbcScClient::GetOtgFeatures(TUint8& aFeatures)
	{
	TPckgBuf<TUint8> p;
	TInt r = DoControl(EControlGetOtgFeatures, &p);
	if (r == KErrNone)
		aFeatures = p();
	return r;
	}
	

inline void RDevUsbcScClient::OtgFeaturesNotify(TRequestStatus& aStatus, TUint8& aValue)
	{
	DoRequest(ERequestOtgFeaturesNotify, aStatus, &aValue);
	}

inline void RDevUsbcScClient::OtgFeaturesNotifyCancel()
	{
	DoControl(ERequestOtgFeaturesNotifyCancel);
	}

inline TInt RDevUsbcScClient::StartNextInAlternateSetting()
	{
	return DoControl(EControlStartNextInAlternateSetting);
	}

//Buffer Interface Layer (BIL) inline functions


inline TInt TEndpointBuffer::GetBuffer(TUint& aOffset,TUint& aSize,TBool& aZLP,TRequestStatus& aStatus,TUint aLength)
	{
	TInt r = GetBuffer(aOffset,aSize,aZLP,aStatus,aLength);
	aOffset -= iBaseAddr;
	return r;
	};


inline TInt TEndpointBuffer::GetEndpointNumber()
	{
	return iEndpointNumber;
	}

#endif // #ifndef __KERNEL_MODE__

#endif // #ifndef __D32USBCSC_INL__
