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
// e32\e32test\demandpaging\d_pagingexample_1_post.cpp
// Demand paging migration example device driver d_pagingexample_1_post: a DLogicalChannel-dervied
// driver, post-migration
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

class DExampleChannel : public DLogicalChannel
	{
public:
	typedef RPagingExample::TConfigData TConfigData;
	typedef RPagingExample::TValueStruct TValueStruct;
public:
	DExampleChannel();
	~DExampleChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt SendMsg(TMessageBase* aMsg);
	TInt SendControl(TMessageBase* aMsg);
	TInt SendRequest(TMessageBase* aMsg);
	virtual void HandleMsg(TMessageBase* aMsg);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	void DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoCancel(TUint aMask);
private:
	TDfcQue* DfcQ();
	void Shutdown();
	void SetConfig(const TConfigData&);
	TInt PreNotify(TRequestStatus* aStatus);
	void StartNotify();
	TInt PreAsyncGetValue(TInt* aValue, TRequestStatus* aStatus);
	void StartAsyncGetValue();
	TInt PreAsyncGetValue2(TInt* aValue1, TInt* aValue2, TRequestStatus* aStatus);
	void StartAsyncGetValue2();
	TInt PreRead(TAny* aBuffer, TInt aLength, TRequestStatus* aStatus);
	void StartRead();
	TInt PreReadDes(TDes8* aDesOut, TRequestStatus* aStatus);
	void StartReadDes();
	TInt PreWrite(TAny* aBuffer, TInt aLength, TRequestStatus* aStatus);
	void StartWrite();
	TInt PreWriteDes(const TDesC8* aDesIn, TRequestStatus* aStatus);
	void StartWriteDes();
	void ReceiveToReadBuffer();
	void SendFromWriteBuffer();
	static void AsyncGetValueCompleteDfcFunc(TAny* aPtr);
	static void AsyncGetValue2CompleteDfcFunc(TAny* aPtr);
	static void ReceiveCompleteDfcFunc(TAny* aPtr);
	static void SendCompleteDfcFunc(TAny* aPtr);
	void CompleteNotify();
	void CompleteAsyncGetValue();
	void CompleteAsyncGetValue2();
	void CompleteRead();
	void CompleteWrite();
private:
	TDynamicDfcQue* iDynamicDfcQ;
	TConfigData iConfig;
	DThread* iClient;
	
	// Notify
	TClientRequest* iNotifyRequest;
	
	// Async get value
	TClientDataRequest<TValueStruct>* iAsyncGetValueRequest;
	NTimer iAsyncGetValueTimer;
	TDfc iAsyncGetValueDfc;
	
	// Async get value 2
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
	r = Kern::CreateClientBufferRequest(iReadRequest, 1, TClientBufferRequest::EPinVirtual);
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
	
	SetDfcQ(DfcQ());
	iAsyncGetValueDfc.SetDfcQ(DfcQ());
	iAsyncGetValue2Dfc.SetDfcQ(DfcQ());
	iCompleteReadDfc.SetDfcQ(DfcQ());
	iCompleteWriteDfc.SetDfcQ(DfcQ());
	iMsgQ.Receive();
	return KErrNone;
	}

// override SendMsg method to allow pinning data in the context of the client thread
TInt DExampleChannel::SendMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id = m.iValue;

	// we only support one client
	if (id != (TInt)ECloseMsg && m.Client() != iClient)
		return KErrAccessDenied;
	
	TInt r = KErrNone;
	if (id != (TInt)ECloseMsg && id != KMaxTInt)
		{
		if (id<0)
			{
			TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
			r = SendRequest(aMsg);
			if (r != KErrNone)
				Kern::RequestComplete(pS,r);
			}
		else
			r = SendControl(aMsg);
		}
	else
		r = DLogicalChannel::SendMsg(aMsg);
	
	return r;
	}

void DExampleChannel::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;
	
	if (id==(TInt)ECloseMsg)
		{
		Shutdown();
		m.Complete(KErrNone,EFalse);
		return;
		}
    else if (id==KMaxTInt)
		{
		// DoCancel
		DoCancel(m.Int0());
		m.Complete(KErrNone,ETrue);
		return;
		}
    else if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		DoRequest(~id,pS,m.Ptr1(),m.Ptr2());
		m.Complete(KErrNone,ETrue);
		}
    else
		{
		// DoControl
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}
	
TInt DExampleChannel::SendControl(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;

	// thread-local copy of configuration data
	TConfigData kernelConfigBuffer;
	TAny* userConfigPtr = m.Ptr0();
	
	switch (id)
		{
		case RPagingExample::ESetConfig:
			// copy config from client to local buffer in context of client thread
			umemget32(&kernelConfigBuffer, userConfigPtr, sizeof(TConfigData));
			// update message to point to kernel-side buffer
			m.iArg[0] = &kernelConfigBuffer;
			break;
			
		case RPagingExample::EGetConfig:
			// update message to point to kernel-side buffer
			m.iArg[0] = &kernelConfigBuffer;
			break;
		}

	TInt r = DLogicalChannel::SendMsg(aMsg);
	if (r != KErrNone)
		return r;

	switch (id)
		{
		case RPagingExample::EGetConfig:
			// copy config from local bufferto client in context of client thread
			umemput32(userConfigPtr, &kernelConfigBuffer, sizeof(TConfigData));
			break;
		}

	return r;
	}

TInt DExampleChannel::DoControl(TInt aFunction,TAny* a1,TAny* /*a2*/)
	{
	TInt r = KErrNone;
	TConfigData* configBuffer = (TConfigData*)a1;
	switch (aFunction)
		{
		case RPagingExample::EGetConfig:
			// copy current config into local buffer in context of DFC thread to avoid potential race conditions
			*configBuffer = iConfig;
			break;

		case RPagingExample::ESetConfig:
			// set config from copy in local buffer in context of DFC thread to avoid potential race conditions
			SetConfig(*configBuffer);
			break;

		default:
			r = KErrNotSupported;
		}
	return r;
	}

TInt DExampleChannel::SendRequest(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt function = ~m.iValue;
	TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
	TAny* a1 = m.Ptr1();
	TAny* a2 = m.Ptr2();
		
	TInt r = KErrNotSupported;
	switch (function)
		{
		case RPagingExample::ERequestNotify:
			r = PreNotify(pS);
			break;

		case RPagingExample::ERequestAsyncGetValue:
			r = PreAsyncGetValue((TInt*)a1, pS);
			break;

		case RPagingExample::ERequestAsyncGetValue2:
			r = PreAsyncGetValue2((TInt*)a1, (TInt*)a2, pS);
			break;
			
		case RPagingExample::ERequestRead:
			r = PreRead(a1, (TInt)a2, pS);
			break;
			
		case RPagingExample::ERequestReadDes:
			r = PreReadDes((TDes8*)a1, pS);
			break;

		case RPagingExample::ERequestWrite:
			r = PreWrite(a1, (TInt)a2, pS);
			break;
			
		case RPagingExample::ERequestWriteDes:
			r = PreWriteDes((TDes8*)a1, pS);
			break;
		}

	if (r == KErrNone)
		r = DLogicalChannel::SendMsg(aMsg);
	return r;
	}

void DExampleChannel::DoRequest(TInt aFunction, TRequestStatus* /*aStatus*/, TAny* /*a1*/, TAny* /*a2*/)
	{
	switch (aFunction)
		{
		case RPagingExample::ERequestNotify:
			StartNotify();
			break;
			
		case RPagingExample::ERequestAsyncGetValue:
			StartAsyncGetValue();
			break;
			
		case RPagingExample::ERequestAsyncGetValue2:
			StartAsyncGetValue2();
			break;
			
		case RPagingExample::ERequestRead:
			StartRead();
			break;
			
		case RPagingExample::ERequestReadDes:
			StartReadDes();
			break;

		case RPagingExample::ERequestWrite:
			StartWrite();
			break;
			
		case RPagingExample::ERequestWriteDes:
			StartWriteDes();
			break;

		default:
			__NK_ASSERT_ALWAYS(EFalse);  // we already validated the request number
		}
	}

TInt DExampleChannel::DoCancel(TUint /*aMask*/)
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
	
	if (iReadRequest->IsReady())
		{
		iReadTimer.Cancel();
		iCompleteReadDfc.Cancel();
		Kern::QueueBufferRequestComplete(iClient, iReadRequest, KErrCancel);
		}
	
	if (iWriteRequest->IsReady())
		{
		iWriteTimer.Cancel();
		iCompleteWriteDfc.Cancel();
		Kern::QueueBufferRequestComplete(iClient, iWriteRequest, KErrCancel);
		}
	
	return KErrNone;
	}

void DExampleChannel::Shutdown()
	{
	}

void DExampleChannel::SetConfig(const TConfigData& aNewConfig)
	{
	iConfig = aNewConfig;
	}

TInt DExampleChannel::PreNotify(TRequestStatus* aStatus)
	{
	return iNotifyRequest->SetStatus(aStatus);
	}

void DExampleChannel::StartNotify()
	{
	CompleteNotify(); // example implementation completes the request immediately
	}

void DExampleChannel::CompleteNotify()
	{
	Kern::QueueRequestComplete(iClient, iNotifyRequest, KErrNone);
	}

TInt DExampleChannel::PreAsyncGetValue(TInt* aValue, TRequestStatus* aStatus)
	{
	TInt r = iAsyncGetValueRequest->SetStatus(aStatus);
	if (r != KErrNone)
		return r;
	iAsyncGetValueRequest->SetDestPtr(aValue);
	return KErrNone;
	}

void DExampleChannel::StartAsyncGetValue()
	{
	// queue a timer to simulate an asynchronous operation
	iAsyncGetValueTimer.OneShot(KAsyncDelay, iAsyncGetValueDfc);
	}

void DExampleChannel::AsyncGetValueCompleteDfcFunc(TAny* aPtr)
	{
	DExampleChannel* self = (DExampleChannel*)aPtr;
	self->CompleteAsyncGetValue();
	}

void DExampleChannel::CompleteAsyncGetValue()
	{
	iAsyncGetValueRequest->Data().iValue1 = 1;
	iAsyncGetValueRequest->Data().iValue2 = _L8("shrt");
	Kern::QueueRequestComplete(iClient, iAsyncGetValueRequest, KErrNone);
	}

TInt DExampleChannel::PreAsyncGetValue2(TInt* aValue1, TInt* aValue2, TRequestStatus* aStatus)
	{
	TInt r = iAsyncGetValue2Request->SetStatus(aStatus);
	if (r != KErrNone)
		return r;
	iAsyncGetValue2Request->SetDestPtr1(aValue1);
	iAsyncGetValue2Request->SetDestPtr2(aValue2);
	return KErrNone;
	}

void DExampleChannel::StartAsyncGetValue2()
	{
	// queue a timer to simulate an asynchronous operation
	iAsyncGetValue2Timer.OneShot(KAsyncDelay, iAsyncGetValue2Dfc);
	}

void DExampleChannel::AsyncGetValue2CompleteDfcFunc(TAny* aPtr)
	{
	DExampleChannel* self = (DExampleChannel*)aPtr;
	self->CompleteAsyncGetValue2();
	}

void DExampleChannel::CompleteAsyncGetValue2()
	{
	iAsyncGetValue2Request->Data1() = 1;
	iAsyncGetValue2Request->Data2() = 2;
	Kern::QueueRequestComplete(iClient, iAsyncGetValue2Request, KErrNone);
	}

TInt DExampleChannel::PreRead(TAny* aBuffer, TInt aLength, TRequestStatus* aStatus)
	{
	// check length argument first
	if (aLength < 1 || aLength > KBufferSize)
		return KErrArgument;

	// start request setup
	TInt r = iReadRequest->StartSetup(aStatus);
	if (r != KErrNone)
		return r;
	
	// add client buffer, this does pinning in context of client thread
	r = iReadRequest->AddBuffer(iClientReadBuffer, (TLinAddr)aBuffer, aLength, ETrue);
	if (r != KErrNone)
		return KErrNoMemory;

	iReadRequest->EndSetup();
	return KErrNone;
	}

void DExampleChannel::StartRead()
	{	
	ReceiveToReadBuffer(); // called in DFC thread as may require serialised access to hardware
	}

TInt DExampleChannel::PreReadDes(TDes8* aDesOut, TRequestStatus* aStatus)
	{
	// start request setup
	TInt r = iReadRequest->StartSetup(aStatus);
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
	return KErrNone;
	}

void DExampleChannel::StartReadDes()
	{
	ReceiveToReadBuffer(); // called in DFC thread as may require serialised access to hardware
	}

TInt DExampleChannel::PreWrite(TAny* aBuffer, TInt aLength, TRequestStatus* aStatus)
	{
	// check length argument first
	if (aLength < 1 || aLength > KBufferSize)
		return KErrArgument;

	// demonstrate use the single-buffer version of Setup
	return iWriteRequest->Setup(iClientWriteBuffer, aStatus, (TLinAddr)aBuffer, aLength);
	}

void DExampleChannel::StartWrite()
	{
	SendFromWriteBuffer(); // called in DFC thread as may require serialised access to hardware
	}

TInt DExampleChannel::PreWriteDes(const TDesC8* aDesIn, TRequestStatus* aStatus)
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
	
	return KErrNone;
	}

void DExampleChannel::StartWriteDes()
	{
	SendFromWriteBuffer(); // called in DFC thread as may require serialised access to hardware
	}

void DExampleChannel::ReceiveToReadBuffer()
	{
	// just queue a timer to simulate an asynchronous receive operation
	// actually will return the previous contents of the buffer
	iReadTimer.OneShot(KAsyncDelay, iCompleteReadDfc);
	}

void DExampleChannel::SendFromWriteBuffer()
	{
	// just queue a timer to simulate an asynchronous send operation
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
	return SetName(&KPagingExample1PostLdd);
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
