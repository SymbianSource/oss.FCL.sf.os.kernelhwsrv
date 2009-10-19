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
// e32\e32test\demandpaging\d_pagingexample_2_pre.cpp
// Demand paging migration example device driver d_pagingexample_2_pre: a
// DLogicalChannelBase-dervied driver, pre-migration
// 
//

#include "d_pagingexample.h"
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>

const TInt KBufferSize = KMaxTransferSize;

//
// Logical channel
//

class DExampleChannel : public DLogicalChannelBase
	{
public:
	typedef RPagingExample::TConfigData TConfigData;
	typedef RPagingExample::TValueStruct TValueStruct;
public:
	DExampleChannel();
	~DExampleChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoCancel(TUint aMask);
private:
 	TDfcQue* DfcQ();
	void Shutdown();
	void GetConfig(TConfigData&);
	void SetConfig(const TConfigData&);
	TInt StartNotify(TRequestStatus* aStatus);
	TInt StartAsyncGetValue(TInt* aValue, TRequestStatus* aStatus);
	TInt StartAsyncGetValue2(TInt* aValue1, TInt* aValue2, TRequestStatus* aStatus);
	TInt StartRead(TAny* aBuffer, TInt aLength, TRequestStatus* aStatus);
	TInt StartReadDes(TDes8* aDesOut, TRequestStatus* aStatus);
	TInt StartWrite(TAny* aBuffer, TInt aLength, TRequestStatus* aStatus);
	TInt StartWriteDes(const TDesC8* aDesIn, TRequestStatus* aStatus);
	void ReceiveToReadBuffer();
	void SendFromWriteBuffer();
	static void CancelDfcFunc(TAny* aPtr);
	static void AsyncGetValueCompleteDfcFunc(TAny* aPtr);
	static void AsyncGetValue2CompleteDfcFunc(TAny* aPtr);
	static void ReceiveCompleteDfcFunc(TAny* aPtr);
	static void SendCompleteDfcFunc(TAny* aPtr);
	void CompleteNotify();
	void CompleteAsyncGetValue();
	void CompleteAsyncGetValue2();
	void CompleteRead();
	void CompleteWrite();
	void Cancel();
private:
	NFastMutex iLock;
	TConfigData iConfig;
	DThread* iClient;
	TDfc iCancelDfc;
	
	// Notify
	TRequestStatus* iNotifyStatus;
	
	// Async get value
	TRequestStatus* iAsyncGetValueStatus;
	TInt* iAsyncGetValueDest;
	NTimer iAsyncGetValueTimer;
	TDfc iAsyncGetValueDfc;

	// Async get value 2
	TRequestStatus* iAsyncGetValue2Status;
	TInt* iAsyncGetValue2Dest1;
	TInt* iAsyncGetValue2Dest2;
	NTimer iAsyncGetValue2Timer;
	TDfc iAsyncGetValue2Dfc;
	
	// Read
	TRequestStatus* iReadStatus;
	NTimer iReadTimer;
	TAny* iReadDest;
	TInt iReadLength;
	TDes8* iReadDes;
	TDfc iCompleteReadDfc;
	
	// Write
	TRequestStatus* iWriteStatus;
	NTimer iWriteTimer;
	TAny* iWriteSrc;
	TInt iWriteLength;
	const TDesC8* iWriteDes;
	TDfc iCompleteWriteDfc;
	TUint8 iBuffer[KBufferSize];
	};

DExampleChannel::DExampleChannel() :
	iCancelDfc(CancelDfcFunc, this, DfcQ(), 0),
	iAsyncGetValueTimer(NULL, this),
	iAsyncGetValueDfc(AsyncGetValueCompleteDfcFunc, this, DfcQ(), 0),
	iAsyncGetValue2Timer(NULL, this),
	iAsyncGetValue2Dfc(AsyncGetValue2CompleteDfcFunc, this, DfcQ(), 0),
	iReadTimer(NULL, this),
	iCompleteReadDfc(ReceiveCompleteDfcFunc, this, DfcQ(), 0),
	iWriteTimer(NULL, this),
	iCompleteWriteDfc(SendCompleteDfcFunc, this, DfcQ(), 0)
	{
	iClient = &Kern::CurrentThread();
	iClient->Open();
	}

DExampleChannel::~DExampleChannel()
	{
	Kern::SafeClose((DObject*&)iClient, NULL);
	}

TDfcQue* DExampleChannel::DfcQ()
	{
	return Kern::DfcQue0();
	}

TInt DExampleChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

TInt DExampleChannel::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
	TInt r = KErrNone;	
	if (&Kern::CurrentThread() != iClient)
		r = KErrAccessDenied;  // we only support one client
	else if (aReqNo==KMaxTInt)
		{
		// DoCancel
		r = DoCancel((TInt)a1);
		}
    else if (aReqNo<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)a1;
		TAny* array[2] = { NULL, NULL };
		kumemget32(array, a2, 2 * sizeof(TAny*));
		r = DoRequest(~aReqNo, pS, array[0], array[1]);
		if(r != KErrNone)
			Kern::RequestComplete(pS, r);
		r = KErrNone;
		}
    else
		{
		// DoControl
		r = DoControl(aReqNo, a1, a2);
		}
	return r;
	}

TInt DExampleChannel::DoControl(TInt aFunction,TAny* a1,TAny* /*a2*/)
	{
	TInt r = KErrNone;
	TConfigData configBuffer;
	switch (aFunction)
		{
		case RPagingExample::EGetConfig:
			GetConfig(configBuffer);
			umemput32(a1, (TAny*)&configBuffer, sizeof(TConfigData));
			break;

		case RPagingExample::ESetConfig:
			umemget32(&configBuffer, a1, sizeof(TConfigData));
			SetConfig(configBuffer);
			break;

		default:
			r = KErrNotSupported;
		}
	return r;
	}

TInt DExampleChannel::DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	TInt r = KErrNotSupported;
	switch (aFunction)
		{
		case RPagingExample::ERequestNotify:
			r = StartNotify(aStatus);
			break;
			
		case RPagingExample::ERequestAsyncGetValue:
			r = StartAsyncGetValue((TInt*)a1, aStatus);
			break;
			
		case RPagingExample::ERequestAsyncGetValue2:
			r = StartAsyncGetValue2((TInt*)a1, (TInt*)a2, aStatus);
			break;
			
		case RPagingExample::ERequestRead:
			r = StartRead(a1, (TInt)a2, aStatus);
			break;
			
		case RPagingExample::ERequestReadDes:
			r = StartReadDes((TDes8*)a1, aStatus);
			break;

		case RPagingExample::ERequestWrite:
			r = StartWrite(a1, (TInt)a2, aStatus);
			break;
			
		case RPagingExample::ERequestWriteDes:
			r = StartWriteDes((TDes8*)a1, aStatus);
			break;
		}
	return r;
	}

TInt DExampleChannel::DoCancel(TUint /*aMask*/)
	{
	iCancelDfc.Enque();
	return KErrNone;
	}

void DExampleChannel::CancelDfcFunc(TAny* aPtr)
	{
	DExampleChannel* self = (DExampleChannel*)aPtr;
	self->Cancel();
	}

void DExampleChannel::Cancel()
	{
	if (iAsyncGetValueStatus)
		{
		iAsyncGetValueTimer.Cancel();
		iAsyncGetValueDfc.Cancel();
		Kern::RequestComplete(iClient, iAsyncGetValueStatus, KErrCancel);
		}

	if (iAsyncGetValue2Status)
		{
		iAsyncGetValue2Timer.Cancel();
		iAsyncGetValue2Dfc.Cancel();
		Kern::RequestComplete(iClient, iAsyncGetValue2Status, KErrCancel);
		}

	if (iReadStatus)
		{
		iReadTimer.Cancel();
		iCompleteReadDfc.Cancel();
		Kern::RequestComplete(iClient, iReadStatus, KErrCancel);
		}

	if (iWriteStatus)
		{
		iWriteTimer.Cancel();
		iCompleteWriteDfc.Cancel();
		Kern::RequestComplete(iClient, iWriteStatus, KErrCancel);
		}
	}

void DExampleChannel::Shutdown()
	{
	}

void DExampleChannel::GetConfig(TConfigData& aConfigOut)
	{
	NKern::FMWait(&iLock);
	aConfigOut = iConfig;
	NKern::FMSignal(&iLock);
	}

void DExampleChannel::SetConfig(const TConfigData& aNewConfig)
	{
	NKern::FMWait(&iLock);
	iConfig = aNewConfig;
	NKern::FMSignal(&iLock);
	}

TInt DExampleChannel::StartNotify(TRequestStatus* aStatus)
	{
	iNotifyStatus = aStatus;	
	CompleteNotify(); // example implementation completes the request immediately
	return KErrNone;
	}

void DExampleChannel::CompleteNotify()
	{
	Kern::RequestComplete(iClient, iNotifyStatus, KErrNone);
	}

TInt DExampleChannel::StartAsyncGetValue(TInt* aValue, TRequestStatus* aStatus)
	{
	iAsyncGetValueDest = aValue;
	iAsyncGetValueStatus = aStatus;

	// queue a timer to simulate an asynchronous operation
	iAsyncGetValueTimer.OneShot(KAsyncDelay, iAsyncGetValueDfc);
	return KErrNone;
	}

void DExampleChannel::AsyncGetValueCompleteDfcFunc(TAny* aPtr)
	{
	DExampleChannel* self = (DExampleChannel*)aPtr;
	self->CompleteAsyncGetValue();
	}

void DExampleChannel::CompleteAsyncGetValue()
	{
	TValueStruct value;
	value.iValue1 = 1;
	value.iValue2 = _L8("shrt");
	TInt r = Kern::ThreadRawWrite(iClient, iAsyncGetValueDest, (TAny*)&value, sizeof(TValueStruct), iClient);
	Kern::RequestComplete(iClient, iAsyncGetValueStatus, r);
	}

TInt DExampleChannel::StartAsyncGetValue2(TInt* aValue1, TInt* aValue2, TRequestStatus* aStatus)
	{
	iAsyncGetValue2Dest1 = aValue1;
	iAsyncGetValue2Dest2 = aValue2;
	iAsyncGetValue2Status = aStatus;

	// queue a timer to simulate an asynchronous operation
	iAsyncGetValue2Timer.OneShot(KAsyncDelay, iAsyncGetValue2Dfc);
	return KErrNone;
	}

void DExampleChannel::AsyncGetValue2CompleteDfcFunc(TAny* aPtr)
	{
	DExampleChannel* self = (DExampleChannel*)aPtr;
	self->CompleteAsyncGetValue2();
	}

void DExampleChannel::CompleteAsyncGetValue2()
	{
	TInt value1 = 1;
	TInt value2 = 2;
	TInt r = Kern::ThreadRawWrite(iClient, iAsyncGetValue2Dest1, (TAny*)&value1, sizeof(TInt), iClient);
	if (r == KErrNone)
		r = Kern::ThreadRawWrite(iClient, iAsyncGetValue2Dest2, (TAny*)&value2, sizeof(TInt), iClient);
	Kern::RequestComplete(iClient, iAsyncGetValue2Status, r);
	}

TInt DExampleChannel::StartRead(TAny* aBuffer, TInt aLength, TRequestStatus* aStatus)
	{
	if (!NKern::CompareAndSwap(*(TAny**)&iReadStatus, NULL, aStatus))
		return KErrInUse;
	if (aLength < 1 || aLength > KBufferSize)
		{
		iReadStatus = NULL;
		return KErrArgument;
		}
	iReadDest = aBuffer;
	iReadLength = aLength;
	ReceiveToReadBuffer();
	return KErrNone;
	}

TInt DExampleChannel::StartReadDes(TDes8* aDesOut, TRequestStatus* aStatus)
	{
	if (!NKern::CompareAndSwap(*(TAny**)&iReadStatus, NULL, aStatus))
		return KErrInUse;
	TInt r = Kern::ThreadGetDesMaxLength(iClient, aDesOut);
	if (r < 0)
		{
		iReadStatus = NULL;
		return r;
		}
	TInt length = r;
	if (length < 1 || length > KBufferSize)
		{
		iReadStatus = NULL;
		return KErrArgument;
		}
	iReadDes = aDesOut;
	iReadLength = length;
	ReceiveToReadBuffer();
	return KErrNone;
	}

TInt DExampleChannel::StartWrite(TAny* aBuffer, TInt aLength, TRequestStatus* aStatus)
	{
	if (!NKern::CompareAndSwap(*(TAny**)&iWriteStatus, NULL, aStatus))
		return KErrInUse;
	if (aLength < 1 || aLength > KBufferSize)
		{
		iWriteStatus = NULL;
		return KErrArgument;
		}
	iWriteSrc = aBuffer;
	iWriteLength = aLength;
	SendFromWriteBuffer();
	return KErrNone;
	}

TInt DExampleChannel::StartWriteDes(const TDesC8* aDesIn, TRequestStatus* aStatus)
	{
	if (!NKern::CompareAndSwap(*(TAny**)&iWriteStatus, NULL, aStatus))
		return KErrInUse;
	TInt r = Kern::ThreadGetDesLength(iClient, aDesIn);
	if (r < 0)
		return r;
	TInt length = r;
	if (length < 1 || length > KBufferSize)
		{
		iWriteStatus = NULL;
		return KErrArgument;
		}
	iWriteLength = length;
	iWriteDes = aDesIn;
	SendFromWriteBuffer();
	return KErrNone;
	}

void DExampleChannel::ReceiveToReadBuffer()
	{
	// just queue a timer to simulate an asynchronous receive operation
	// actually will return the previous contents of the buffer
	NKern::FMWait(&iLock);
	// The synchonisation is for illustrative purposes only - in a real driver we might make use of
	// the configution here, so this would need to be protected from concurrent modification.
	NKern::FMSignal(&iLock);
	iReadTimer.OneShot(KAsyncDelay, iCompleteReadDfc);
	}

void DExampleChannel::SendFromWriteBuffer()
	{
	// just queue a timer to simulate an asynchronous send operation
	NKern::FMWait(&iLock);
	// The synchonisation is for illustrative purposes only - in a real driver we might make use of
	// the configution here, so this would need to be protected from concurrent modification.
	NKern::FMSignal(&iLock);
	iWriteTimer.OneShot(KAsyncDelay, iCompleteWriteDfc);
	}

void DExampleChannel::ReceiveCompleteDfcFunc(TAny* aPtr)
	{
	DExampleChannel* self = (DExampleChannel*)aPtr;
	self->CompleteRead();
	}

void DExampleChannel::SendCompleteDfcFunc(TAny* aPtr)
	{
	DExampleChannel* self = (DExampleChannel*)aPtr;
	self->CompleteWrite();
	}

void DExampleChannel::CompleteRead()
	{
	TInt r;
	if (iReadDest)
		{
		r = Kern::ThreadRawWrite(iClient, iReadDest, iBuffer, iReadLength, iClient);
		iReadDest = NULL;
		}
	else
		{
		TPtrC8 des(iBuffer, iReadLength);
		r = Kern::ThreadDesWrite(iClient, iReadDes, des, 0, KChunkShiftBy0, iClient);
		iReadDes = NULL;
		}
	
	iReadLength = 0;
	Kern::RequestComplete(iClient, iReadStatus, r);
	}

void DExampleChannel::CompleteWrite()
	{
	TInt r;
	if (iWriteSrc)
		{
		r = Kern::ThreadRawRead(iClient, iWriteSrc, iBuffer, iWriteLength);
		iWriteSrc = NULL;
		}
	else
		{
		TPtr8 des(iBuffer, iWriteLength);
		r = Kern::ThreadDesRead(iClient, iWriteDes, des, 0);
		iWriteDes = NULL;
		}
	
	iWriteLength = 0;
	Kern::RequestComplete(iClient, iWriteStatus, r);
	}

//
// Logical device
//

class DExampleFactory : public DLogicalDevice
	{
public:
	DExampleFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

DExampleFactory::DExampleFactory()
	{
	iParseMask = 0;
	iVersion = TVersion(1, 0, 0);
	}

TInt DExampleFactory::Install()
	{
	return SetName(&KPagingExample2PreLdd);
	}

void DExampleFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DExampleFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = new DExampleChannel;
	return aChannel ? KErrNone : KErrNoMemory;
	}

DECLARE_STANDARD_LDD()
	{
	return new DExampleFactory;
	}
