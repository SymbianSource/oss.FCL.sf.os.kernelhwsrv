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
// e32\e32test\demandpaging\d_pagingexample_2_post.cpp
// Demand paging migration example device driver d_pagingexample_2_post: a
// DLogicalChannelBase-dervied driver, post-migration
// 
//

#include "d_pagingexample.h"
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>

const TInt KDfcQThreadPriority = 25;
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
	TDynamicDfcQue* iDynamicDfcQ;
	TConfigData iConfig;
	DThread* iClient;
	TDfc iCancelDfc;
	
	// Notify
	TClientRequest* iNotifyRequest;
	
	// Async get value
	TClientDataRequest<TValueStruct>* iAsyncGetValueRequest;
	NTimer iAsyncGetValueTimer;
	TDfc iAsyncGetValueDfc;

	// Async get value
	TClientDataRequest2<TInt,TInt>* iAsyncGetValue2Request;
	NTimer iAsyncGetValue2Timer;
	TDfc iAsyncGetValue2Dfc;

	// Read
	TClientBufferRequest* iReadRequest;
	NTimer iReadTimer;
	TClientBuffer* iClientReadBuffer;
	TDfc iCompleteReadDfc;
	
	// Write
	TClientBufferRequest* iWriteRequest;
	NTimer iWriteTimer;
	TClientBuffer* iClientWriteBuffer;
	TDfc iCompleteWriteDfc;
	TUint8 iBuffer[KBufferSize];
	};

DExampleChannel::DExampleChannel() :
	iCancelDfc(CancelDfcFunc, this, 0),
	iAsyncGetValueTimer(NULL, this),
	iAsyncGetValueDfc(AsyncGetValueCompleteDfcFunc, this, 0),
	iAsyncGetValue2Timer(NULL, this),
	iAsyncGetValue2Dfc(AsyncGetValue2CompleteDfcFunc, this, 0),
	iReadTimer(NULL, this),
	iCompleteReadDfc(ReceiveCompleteDfcFunc, this, 0),
	iWriteTimer(NULL, this),
	iCompleteWriteDfc(SendCompleteDfcFunc, this, 0)
	{
	iClient = &Kern::CurrentThread();
	iClient->Open();
	}

DExampleChannel::~DExampleChannel()
	{
	Kern::SafeClose((DObject*&)iClient, NULL);
	if (iDynamicDfcQ)
		iDynamicDfcQ->Destroy();
	Kern::DestroyClientRequest(iNotifyRequest);
	Kern::DestroyClientRequest(iAsyncGetValueRequest);
	Kern::DestroyClientRequest(iAsyncGetValue2Request);
	Kern::DestroyClientBufferRequest(iReadRequest);
	Kern::DestroyClientBufferRequest(iWriteRequest);
	}

TDfcQue* DExampleChannel::DfcQ()
	{
	return iDynamicDfcQ;
	}

TInt DExampleChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	TInt r;
	
	r = Kern::CreateClientRequest(iNotifyRequest);
	if (r != KErrNone)
		return r;
	r = Kern::CreateClientDataRequest(iAsyncGetValueRequest);
	if (r != KErrNone)
		return r;
	r = Kern::CreateClientDataRequest2(iAsyncGetValue2Request);
	if (r != KErrNone)
		return r;
	r = Kern::CreateClientBufferRequest(iWriteRequest, 1, TClientBufferRequest::EPinVirtual);
	if (r != KErrNone)
		return r;
	
	// create a dynamic DFC queue, which is used for handling client messages and for our own DFCs
	r = Kern::DynamicDfcQCreate(iDynamicDfcQ, KDfcQThreadPriority, KPagingExample1PostLdd);
	if (r != KErrNone)
		return r;

	// todo: this will be the default anyway
	iDynamicDfcQ->SetRealtimeState(ERealtimeStateOn);
	
	iCancelDfc.SetDfcQ(DfcQ());
	iAsyncGetValueDfc.SetDfcQ(DfcQ());
	iAsyncGetValue2Dfc.SetDfcQ(DfcQ());
	iCompleteReadDfc.SetDfcQ(DfcQ());
	iCompleteWriteDfc.SetDfcQ(DfcQ());
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
	if (iAsyncGetValueRequest->IsReady())
		{
		iAsyncGetValueTimer.Cancel();
		iAsyncGetValueDfc.Cancel();
		Kern::QueueRequestComplete(iClient, iAsyncGetValueRequest, KErrCancel);
		}
	
	if (iAsyncGetValue2Request->IsReady())
		{
		iAsyncGetValue2Timer.Cancel();
		iAsyncGetValue2Dfc.Cancel();
		Kern::QueueRequestComplete(iClient, iAsyncGetValue2Request, KErrCancel);
		}

	if (iReadRequest && iReadRequest->IsReady())
		{
		iReadTimer.Cancel();
		iCompleteReadDfc.Cancel();
		Kern::QueueBufferRequestComplete(iClient, iReadRequest, KErrCancel);
		Kern::DestroyClientBufferRequest(iReadRequest);
		}

	if (iWriteRequest->IsReady())
		{
		iWriteTimer.Cancel();
		iCompleteWriteDfc.Cancel();
		Kern::QueueBufferRequestComplete(iClient, iWriteRequest, KErrCancel);
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
	// example implementation completes the request immediately
	TInt r = iNotifyRequest->SetStatus(aStatus);
	if (r != KErrNone)
		return r;
	CompleteNotify(); // example implementation completes the request immediately
	return KErrNone;
	}

void DExampleChannel::CompleteNotify()
	{
	Kern::QueueRequestComplete(iClient, iNotifyRequest, KErrNone);
	}

TInt DExampleChannel::StartAsyncGetValue(TInt* aValue, TRequestStatus* aStatus)
	{
	// use of TClientDataRequest API protected by fast mutex
	NKern::FMWait(&iLock);
	TInt r = iAsyncGetValueRequest->SetStatus(aStatus);
	if (r == KErrNone)
		iAsyncGetValueRequest->SetDestPtr(aValue);
	NKern::FMSignal(&iLock);
	if (r != KErrNone)
		return r;

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
	// use of TClientDataRequest API protected by fast mutex
	NKern::FMWait(&iLock);
	if (iAsyncGetValueRequest->IsReady())
		{
		iAsyncGetValueRequest->Data().iValue1 = 1;
		iAsyncGetValueRequest->Data().iValue2 = _L8("shrt");
		Kern::QueueRequestComplete(iClient, iAsyncGetValueRequest, KErrNone);
		}
	NKern::FMSignal(&iLock);
	}

TInt DExampleChannel::StartAsyncGetValue2(TInt* aValue1, TInt* aValue2, TRequestStatus* aStatus)
	{
	// use of TClientDataRequest API protected by fast mutex
	NKern::FMWait(&iLock);
	TInt r = iAsyncGetValue2Request->SetStatus(aStatus);
	if (r == KErrNone)
		iAsyncGetValue2Request->SetDestPtr1(aValue1);
	if (r == KErrNone)
		iAsyncGetValue2Request->SetDestPtr2(aValue2);
	NKern::FMSignal(&iLock);
	if (r != KErrNone)
		return r;

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
	// use of TClientDataRequest API protected by fast mutex
	NKern::FMWait(&iLock);
	if (iAsyncGetValue2Request->IsReady())
		{
		iAsyncGetValue2Request->Data1() = 1;
		iAsyncGetValue2Request->Data2() = 2;
		Kern::QueueRequestComplete(iClient, iAsyncGetValue2Request, KErrNone);
		}
	NKern::FMSignal(&iLock);
	}

TInt DExampleChannel::StartRead(TAny* aBuffer, TInt aLength, TRequestStatus* aStatus)
	{
	// check length argument first
	if (aLength < 1 || aLength > KBufferSize)
		return KErrArgument;

	// normally drivers would pre-create a TClientBufferRequest where possible, but here we create
	// one on demand to test this possibilty
	NKern::ThreadEnterCS();
	TInt r = Kern::CreateClientBufferRequest(iReadRequest, 1, TClientBufferRequest::EPinVirtual);
	NKern::ThreadLeaveCS();  // iReadRequest is deleted by the destructor
	if (r != KErrNone)
		return r;

	// start request setup
	r = iReadRequest->StartSetup(aStatus);
	if (r != KErrNone)
		return r;
	
	// add client buffer, this does pinning in context of client thread
	r = iReadRequest->AddBuffer(iClientReadBuffer, (TLinAddr)aBuffer, aLength, ETrue);
	if (r != KErrNone)
		return KErrNoMemory;

	iReadRequest->EndSetup();
	
	ReceiveToReadBuffer();
	return KErrNone;
	}

TInt DExampleChannel::StartReadDes(TDes8* aDesOut, TRequestStatus* aStatus)
	{
	// normally drivers would pre-create a TClientBufferRequest where possible, but here we create
	// one on demand to test this possibilty
	NKern::ThreadEnterCS();
	TInt r = Kern::CreateClientBufferRequest(iReadRequest, 1, TClientBufferRequest::EPinVirtual);
	NKern::ThreadLeaveCS();  // iReadRequest is deleted by the destructor
	if (r != KErrNone)
		return r;
	
	// start request setup
	r = iReadRequest->StartSetup(aStatus);
	if (r != KErrNone)
		return r;

	// add client descriptor, this does pinning in context of client thread
	r = iReadRequest->AddBuffer(iClientReadBuffer, aDesOut);
	if (r != KErrNone)
		return r;

	// can check length argument here
	TInt length = iClientReadBuffer->MaxLength();
	if (length < 1 || length > KBufferSize)
		{
		// need to reset object so it can be reused
		iReadRequest->Reset();
		return KErrArgument;
		}

	iReadRequest->EndSetup();	

	ReceiveToReadBuffer();
	return KErrNone;
	}

TInt DExampleChannel::StartWrite(TAny* aBuffer, TInt aLength, TRequestStatus* aStatus)
	{
	// check length argument first
	if (aLength < 1 || aLength > KBufferSize)
		return KErrArgument;

	// demonstrate use the single-buffer version of Setup
	TInt r = iWriteRequest->Setup(iClientWriteBuffer, aStatus, (TLinAddr)aBuffer, aLength);
	if (r != KErrNone)
		return r;

	SendFromWriteBuffer();
	return KErrNone;
	}

TInt DExampleChannel::StartWriteDes(const TDesC8* aDesIn, TRequestStatus* aStatus)
	{
	// demonstrate use the single-buffer version of Setup
	TInt r = iWriteRequest->Setup(iClientWriteBuffer, aStatus, (TAny*)aDesIn);
	if (r != KErrNone)
		return r;

	// can check length argument here
	TInt length = iClientWriteBuffer->Length();
	if (length < 1 || length > KBufferSize)
		{
		// need to reset object so it can be reused
		iWriteRequest->Reset();
		return KErrArgument;
		}
	
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
	TPtrC8 des(iBuffer, iClientReadBuffer->MaxLength());
	TInt r = Kern::ThreadBufWrite(iClient, iClientReadBuffer, des, 0, KChunkShiftBy0, iClient);
	Kern::QueueBufferRequestComplete(iClient, iReadRequest, r);
	Kern::DestroyClientBufferRequest(iReadRequest);
	}

void DExampleChannel::CompleteWrite()
	{
	TPtr8 des(iBuffer, iClientWriteBuffer->Length());
	TInt r = Kern::ThreadBufRead(iClient, iClientWriteBuffer, des, 0, KChunkShiftBy0);
	Kern::QueueBufferRequestComplete(iClient, iWriteRequest, r);
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
	return SetName(&KPagingExample2PostLdd);
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
