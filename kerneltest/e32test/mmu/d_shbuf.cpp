// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/mmu/d_shbuf.cpp
//

#include "d_shbuf.h"
#include <kernel/kernel.h>
#include <kernel/cache.h>
#include "plat_priv.h"
#include <kernel/sshbuf.h>


#define TEST_EXP(a)					CheckPoint(a, __LINE__)
#define TEST_KERRNONE(a)			CheckPointError(a, __LINE__)

#ifdef TEST_CLIENT_THREAD
#define TEST_ENTERCS()	NKern::ThreadEnterCS()
#define TEST_LEAVECS()	NKern::ThreadLeaveCS()
#else
#define TEST_ENTERCS()
#define TEST_LEAVECS()
#endif // TEST_CLIENT_THREAD

const TInt KMaxPhysicalMemoryBlockSize = 512 << 10; // 512KB;

// ----------------------------------------------------------------------------

class DShBufTestDrvFactory : public DLogicalDevice
	{
public:
	DShBufTestDrvFactory();
	~DShBufTestDrvFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
public:
#ifndef TEST_CLIENT_THREAD
	TDynamicDfcQue* iDfcQ;
#endif
	};

// ----------------------------------------------------------------------------

#ifdef TEST_CLIENT_THREAD
class DShBufTestDrvChannel : public DLogicalChannelBase
#else
class DShBufTestDrvChannel : public DLogicalChannel
#endif
	{
public:
	DShBufTestDrvChannel();
	~DShBufTestDrvChannel();
	// Inherited from DLogicalChannel
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
#ifdef TEST_CLIENT_THREAD
	// Inherited from DLogicalChannelBase: process all DoControl in the user's context
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
#else
	TInt DoControl(TInt aReqNo, TAny* a1, TAny* a2);
	virtual void HandleMsg(TMessageBase* aMsg);
    virtual TInt SendMsg(TMessageBase* aMsg);
#endif
public:
	TShPoolCreateInfo* iCreateinfo;
	TShPoolInfo iUserpoolinfo;
	TShPool* iPools[2];
	TShBuf* iAdopted;
	TUint8  iDriverTxBuffer[8192];
	TUint8  iDriverRxBuffer[8192];
#ifndef TEST_CLIENT_THREAD
	DThread* iClient;
	TVirtualPinObject* iPin;
#endif
	};

// ----------------------------------------------------------------------------

void CheckPoint(TInt aCondition, TInt aLine)
	{
	if (!aCondition)
		{
		Kern::Printf("Device driver test failed (line %d)", aLine);
		}
	}

void CheckPointError(TInt aErrorCode, TInt aLine)
	{
	if (aErrorCode != KErrNone)
		{
		Kern::Printf("Device driver error %d (line %d)", aErrorCode, aLine);
		}
	}

TInt Log2(TInt aNum)
	{
	TInt res = -1;
	while(aNum)
		{
		res++;
		aNum >>= 1;
		}
	return res;
	}

TInt RoundUp(TInt aNum, TInt aAlignmentLog2)
	{
	if (aNum % (1 << aAlignmentLog2) == 0)
		{
		return aNum;
		}
	return (aNum & ~((1 << aAlignmentLog2) - 1)) + (1 << aAlignmentLog2);
	}

#ifdef __WINS__
#define SHBUF_NOT_WINS(x)
#else
#define SHBUF_NOT_WINS(x)	x
#endif
TBool IsBufferContiguous(TShBuf* SHBUF_NOT_WINS(aBuf))
	{
	TInt pagesize;
	TInt r = Kern::HalFunction(EHalGroupKernel, EKernelHalPageSizeInBytes, &pagesize, 0);
	TEST_KERRNONE(r);

#ifdef __WINS__
	return ETrue;
#else
	TUint8* ptr = Kern::ShBufPtr(aBuf);
	TUint size = Kern::ShBufSize(aBuf);

	TBool iscontiguous = ETrue;

	TPhysAddr startphys = Epoc::LinearToPhysical((TLinAddr) ptr);
	TUint i;

	for (i = 0; i < size; i += pagesize)
		{
		TPhysAddr current = Epoc::LinearToPhysical((TLinAddr) ptr + i);
		if (current != startphys + i)
			{
			Kern::Printf("Page %d: 0x%08x (started@0x%08x expected 0x%08x)", i, current, startphys, startphys + i);
			iscontiguous = EFalse;
			break;
			}
		}

	return iscontiguous;
#endif // __WINS__
	}

DECLARE_STANDARD_LDD()
	{
	return new DShBufTestDrvFactory;
	}

DShBufTestDrvFactory::DShBufTestDrvFactory()
	{
	iParseMask=0; //no units, no info, no pdd
	iUnitsMask=0;
	iVersion=TVersion(1,0,KE32BuildVersionNumber);
	}

DShBufTestDrvFactory::~DShBufTestDrvFactory()
	{
#ifndef TEST_CLIENT_THREAD
	if (iDfcQ)
		iDfcQ->Destroy();
#endif
	}

#ifndef TEST_CLIENT_THREAD
const TInt KShBufTestThreadPriority = 1;
_LIT(KShBufTestThread,"ShBufTestThread");
#endif

TInt DShBufTestDrvFactory::Install()
	{
#ifndef TEST_CLIENT_THREAD
	TInt r = Kern::DynamicDfcQCreate(iDfcQ, KShBufTestThreadPriority, KShBufTestThread);

	if (r != KErrNone)
		return r;
	return(SetName(&KTestShBufOwn));
#else
	return(SetName(&KTestShBufClient));
#endif
	}


void DShBufTestDrvFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Get capabilities - overriding pure virtual
	}

TInt DShBufTestDrvFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DShBufTestDrvChannel;
	return aChannel?KErrNone:KErrNoMemory;
	}

// ----------------------------------------------------------------------------

DShBufTestDrvChannel::DShBufTestDrvChannel()
	{
#ifndef TEST_CLIENT_THREAD
	iClient=&Kern::CurrentThread();
	iClient->Open();
#endif

	TPtr8 bufp(iDriverRxBuffer,0,sizeof(iDriverRxBuffer));

	for(TInt pos = 0;  pos < bufp.Length();  pos++)
		{
		bufp[pos] = (TUint8)(pos & 31);
		}
	}

DShBufTestDrvChannel::~DShBufTestDrvChannel()
	{
	NKern::ThreadEnterCS();
#ifndef TEST_CLIENT_THREAD
	Kern::SafeClose((DObject*&)iClient, NULL);
	if(iPin)
		{
		Kern::DestroyVirtualPinObject(iPin);
		}
#endif
	delete iCreateinfo;
	NKern::ThreadLeaveCS();
	}

TInt DShBufTestDrvChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
#ifndef TEST_CLIENT_THREAD
	SetDfcQ(((DShBufTestDrvFactory*)iDevice)->iDfcQ);
	iMsgQ.Receive();
#endif
	
	return KErrNone;
	}

#ifndef TEST_CLIENT_THREAD
void DShBufTestDrvChannel::HandleMsg(TMessageBase* aMsg)
	{
    TInt r=KErrNone;
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;
	if (id==(TInt)ECloseMsg)
		{
		m.Complete(KErrNone,EFalse);
		return;
		}
	else
		{
		r=DoControl(id,m.Ptr0(),m.Ptr1());
		}
	m.Complete(r,ETrue);
	}

TInt DShBufTestDrvChannel::SendMsg(TMessageBase* aMsg)
	{
	// We can only handle one request at a time.
	TEST_EXP(!iCreateinfo && !iPin);
	if(iCreateinfo || iPin)
		{
		return KErrInUse;
		}

	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TAny* a1 = m.Ptr0();
	TAny* a2 = m.Ptr1();
	TInt r = KErrNone;

	// Make a copy of the parameters in the asynchronous read case so that we don't
	// risk a page fault by reading user-mode memory from the msg DFC.
	//
	// Manage writes using a TClientBufferRequest.
	switch(aMsg->iValue)
		{
		// Reads
		case RShBufTestChannel::ETestOpenUserPool:
			kumemget(&iUserpoolinfo, a2, sizeof(iUserpoolinfo));
		break;
		case RShBufTestChannel::ETestCreatePoolContiguousPool:
			NKern::ThreadEnterCS();
			iCreateinfo = new TShPoolCreateInfo;
			NKern::ThreadLeaveCS();
			TEST_EXP(iCreateinfo != NULL);
			if(!iCreateinfo)
				{
				r = KErrNoMemory;
				break;
				}

			kumemget(iCreateinfo, a1, sizeof(TShPoolInfo));
		break;
		case RShBufTestChannel::EFromTPtr8ProcessAndRelease:
			{
			TPtr8 dest(iDriverTxBuffer, sizeof(iDriverTxBuffer));
			Kern::ThreadDesRead(iClient, a1, dest, 0, KChunkShiftBy0);
			}
		break;

		// Writes
		case RShBufTestChannel::ETestOpenKernelPool:
			NKern::ThreadEnterCS();
			iCreateinfo = new TShPoolCreateInfo;
			NKern::ThreadLeaveCS();
			TEST_EXP(iCreateinfo != NULL);
			if(!iCreateinfo)
				{
				r = KErrNoMemory;
				break;
				}

			kumemget(iCreateinfo, a1, sizeof(TShPoolInfo));

			// Fallthrough...
		case RShBufTestChannel::ETestAllocateMax:
		case RShBufTestChannel::ETestAllocateKernelBuffer:
			{
			NKern::ThreadEnterCS();
			r = Kern::CreateAndPinVirtualMemory(iPin, (TLinAddr)a2, sizeof(TInt));
			NKern::ThreadLeaveCS();
			}
		break;

		// Descriptor writes
		case RShBufTestChannel::EFromTPtr8ProcessAndReturn:
			{
			TPtr8 tempPtr(0, 0, 0);
			kumemget(&tempPtr, a1, sizeof(tempPtr));

			TUint size = tempPtr.Size();
			
			if(size <= sizeof(iDriverRxBuffer))
				{
				NKern::ThreadEnterCS();
				r = Kern::CreateAndPinVirtualMemory(iPin, (TLinAddr)tempPtr.Ptr(), size);
				NKern::ThreadLeaveCS();
				}
			else
				{
				r = KErrNoMemory;
				}
			}
		break;
		}

	if(r == KErrNone)
		{
		r = DLogicalChannel::SendMsg(aMsg);
		}

	return r;
	}
#endif

#ifdef TEST_CLIENT_THREAD
TInt DShBufTestDrvChannel::Request(TInt aReqNo, TAny* a1, TAny* a2)
#else
TInt DShBufTestDrvChannel::DoControl(TInt aReqNo, TAny* a1, TAny* a2)
#endif
	{
	TInt r=KErrNotSupported;

	switch (aReqNo)
		{
// ----------------------------------------------------------------------------
// TInt RShBufTestChannel::OpenUserPool(TInt aHandle, TShPoolInfo& aPoolInfo)
		case RShBufTestChannel::ETestOpenUserPool:
			{
			DThread* tP=NULL;
			
#ifdef TEST_CLIENT_THREAD
			kumemget(&iUserpoolinfo, a2, sizeof(iUserpoolinfo));
			tP=&Kern::CurrentThread();
#else
			tP=iClient;
#endif

			TEST_EXP(!iPools[0]);
			if(iPools[0])
				{
				r = KErrAlreadyExists;
				break;
				}

			NKern::ThreadEnterCS();
			r = Kern::ShPoolOpen(iPools[0], tP, (TInt) a1, ETrue, KDefaultPoolHandleFlags);
			NKern::ThreadLeaveCS();

			TEST_KERRNONE(r);
			if (r)
				{
				break;
				}

			TInt n;
			n = reinterpret_cast<DShPool*>(iPools[0])->AccessCount();
			TEST_EXP(n == 2);
			if (n != 2)
				{
				r = KErrUnknown;
				break;
				}
			
			TShPoolInfo poolinfo;
			Kern::ShPoolGetInfo(iPools[0], poolinfo);
			if (!((poolinfo.iBufSize == iUserpoolinfo.iBufSize) &&
				((TUint)Kern::ShPoolBufSize(iPools[0]) == iUserpoolinfo.iBufSize) &&
				(poolinfo.iInitialBufs == iUserpoolinfo.iInitialBufs) &&
				(poolinfo.iMaxBufs == iUserpoolinfo.iMaxBufs) &&
				(poolinfo.iGrowTriggerRatio == iUserpoolinfo.iGrowTriggerRatio) &&
				(poolinfo.iGrowByRatio == iUserpoolinfo.iGrowByRatio) &&
				(poolinfo.iShrinkHysteresisRatio == iUserpoolinfo.iShrinkHysteresisRatio) &&
				(poolinfo.iAlignment == iUserpoolinfo.iAlignment) &&
				((poolinfo.iFlags & EShPoolNonPageAlignedBuffer) == (iUserpoolinfo.iFlags & EShPoolNonPageAlignedBuffer)) &&
				((poolinfo.iFlags & EShPoolPageAlignedBuffer) == (iUserpoolinfo.iFlags & EShPoolPageAlignedBuffer))))
				{
				TEST_EXP(EFalse);
				Kern::Printf("poolinfo.iBufSize == %d (expected %d)", poolinfo.iBufSize, iUserpoolinfo.iBufSize);
				Kern::Printf("BufSize() == %d", Kern::ShPoolBufSize(iPools[0]));
				Kern::Printf("poolinfo.iInitialBufs == %d (expected %d)", poolinfo.iInitialBufs, iUserpoolinfo.iInitialBufs);
				Kern::Printf("poolinfo.iMaxBufs == %d (expected %d)", poolinfo.iMaxBufs, iUserpoolinfo.iMaxBufs);
				Kern::Printf("poolinfo.iGrowTriggerRatio == %d (expected %d)", poolinfo.iGrowTriggerRatio, iUserpoolinfo.iGrowTriggerRatio);
				Kern::Printf("poolinfo.iGrowByRatio == %d (expected %d)", poolinfo.iGrowByRatio, iUserpoolinfo.iGrowByRatio);
				Kern::Printf("poolinfo.iShrinkHysteresisRatio == %d (expected %d)", poolinfo.iShrinkHysteresisRatio, iUserpoolinfo.iShrinkHysteresisRatio);
				Kern::Printf("poolinfo.iAlignment == %d (expected %d)", poolinfo.iAlignment, iUserpoolinfo.iAlignment);
				Kern::Printf("poolinfo.iFlags == 0x%08x (user=0x%08x)", poolinfo.iFlags, iUserpoolinfo.iFlags);

				r = KErrUnknown;
				break;
				}

			if(poolinfo.iFlags & EShPoolPageAlignedBuffer)
				{
				NKern::ThreadEnterCS();
				r = Kern::ShPoolSetBufferWindow(iPools[0],-1);
				NKern::ThreadLeaveCS();
				TEST_KERRNONE(r);
				if(r!=KErrNone)
					break;
				}

			r = KErrNone;
			}
		break;
// ----------------------------------------------------------------------------
// TInt RShBufTestChannel::OpenKernelPool(TShPoolCreateInfo& aInfo, TInt& aHandle)
		case RShBufTestChannel::ETestOpenKernelPool:
			{
			TInt handle;
#ifdef TEST_CLIENT_THREAD
			// We can only handle one request at a time.
			TEST_EXP(!iCreateinfo);
			if(iCreateinfo)
				{
				r = KErrInUse;
				break;
				}

			NKern::ThreadEnterCS();
			iCreateinfo = new TShPoolCreateInfo;
			NKern::ThreadLeaveCS();
			TEST_EXP(iCreateinfo != NULL);
			if(!iCreateinfo)
				{
				r = KErrNoMemory;
				break;
				}

			kumemget(iCreateinfo, a1, sizeof(TShPoolInfo));
#endif

			TEST_EXP(!iPools[1]);
			if(iPools[1])
				{
				r = KErrAlreadyExists;
				break;
				}

			NKern::ThreadEnterCS();
			r = Kern::ShPoolCreate(iPools[1], *iCreateinfo, ETrue, KDefaultPoolHandleFlags);
			delete iCreateinfo;
			iCreateinfo = NULL;
			NKern::ThreadLeaveCS();

			TEST_KERRNONE(r);
			if (r)
				{
#ifndef TEST_CLIENT_THREAD
					NKern::ThreadEnterCS();
					Kern::DestroyVirtualPinObject(iPin);
					NKern::ThreadLeaveCS();
#endif
				break;
				}

			TInt n;
			n = reinterpret_cast<DShPool*>(iPools[1])->AccessCount();
			TEST_EXP(n == 1);
			if (n != 1)
				{
#ifndef TEST_CLIENT_THREAD
					NKern::ThreadEnterCS();
					Kern::DestroyVirtualPinObject(iPin);
					NKern::ThreadLeaveCS();
#endif

				r = KErrUnknown;
				break;
				}

			TShPoolInfo poolinfo;
			Kern::ShPoolGetInfo(iPools[1], poolinfo);
			if(poolinfo.iFlags & EShPoolPageAlignedBuffer)
				{
				NKern::ThreadEnterCS();
				r = Kern::ShPoolSetBufferWindow(iPools[1],-1);
				NKern::ThreadLeaveCS();
				TEST_KERRNONE(r);
				if(r!=KErrNone)
					{
#ifndef TEST_CLIENT_THREAD
						NKern::ThreadEnterCS();
						Kern::DestroyVirtualPinObject(iPin);
						NKern::ThreadLeaveCS();
#endif

					break;
					}
				}

#ifdef TEST_CLIENT_THREAD
			// Now create a handle for the client
			NKern::ThreadEnterCS();
			handle = Kern::ShPoolMakeHandleAndOpen(iPools[1], NULL, KDefaultPoolHandleFlags);
			NKern::ThreadLeaveCS();
#else
			handle = Kern::ShPoolMakeHandleAndOpen(iPools[1], iClient, KDefaultPoolHandleFlags);
#endif
			TEST_EXP(handle > 0);
			if (handle < 0)
				{
#ifndef TEST_CLIENT_THREAD
					NKern::ThreadEnterCS();
					Kern::DestroyVirtualPinObject(iPin);
					NKern::ThreadLeaveCS();
#endif

				r = handle;
				break;
				}

			n = reinterpret_cast<DShPool*>(iPools[1])->AccessCount();

			TEST_EXP(n == 2);
			if (n != 2)
				{
#ifndef TEST_CLIENT_THREAD
					NKern::ThreadEnterCS();
					Kern::DestroyVirtualPinObject(iPin);
					NKern::ThreadLeaveCS();
#endif

				r = KErrUnknown;
				break;
				}

#ifdef TEST_CLIENT_THREAD
			kumemput(a2, &handle, sizeof(handle));
#else
			Kern::ThreadRawWrite(iClient, a2, &handle, sizeof(handle), iClient);
			
			NKern::ThreadEnterCS();
			Kern::DestroyVirtualPinObject(iPin);
			NKern::ThreadLeaveCS();
#endif
			}
		break;
// ----------------------------------------------------------------------------
// TInt RShBufTestChannel::CloseUserPool()
		case RShBufTestChannel::ETestCloseUserPool:
			{
			TInt n;
			n = reinterpret_cast<DShPool*>(iPools[0])->AccessCount();

			TEST_EXP(n == 1);
			if (n != 1)
				{
				r = KErrUnknown;
				break;
				}
			TEST_ENTERCS();
			r = Kern::ShPoolClose(iPools[0]);
			iPools[0] = 0;
			TEST_LEAVECS();
			if (r>0)
				{
				r = KErrNone;
				}

			TEST_KERRNONE(r);
			}
		break;
// ----------------------------------------------------------------------------
// TInt RShBufTestChannel::CloseKernelPool()
		case RShBufTestChannel::ETestCloseKernelPool:
			{
#if 0
			TInt n;
			n = reinterpret_cast<DShPool*>(iPools[1])->AccessCount();
			TEST_EXP(n == 2);
			if (n != 2)
				{
				r = KErrUnknown;
				break;
				}
#endif
			TEST_ENTERCS();
			r = Kern::ShPoolClose(iPools[1]);
			iPools[1] = 0;
			TEST_LEAVECS();
			if (r>0)
				{
				r = KErrNone;
				}

			TEST_KERRNONE(r);
			}
		break;
// ----------------------------------------------------------------------------
// TInt RShBufTestChannel::ManipulateUserBuffer(TInt aHandle)
		case RShBufTestChannel::ETestManipulateUserBuffer:
			{
			TShBuf* ubuf = NULL;
			DThread* tP;

#ifdef TEST_CLIENT_THREAD
			tP=&Kern::CurrentThread();
#else
			tP=iClient;
#endif
			NKern::ThreadEnterCS();

			r = Kern::ShBufOpen(ubuf, tP, (TInt) a1);

			TEST_KERRNONE(r);
			if (r!=KErrNone)
				{
				NKern::ThreadLeaveCS();
				break;
				}

			TInt n;
			n = reinterpret_cast<DShBuf*>(ubuf)->AccessCount();

			TEST_EXP(n == 2);
			if (n != 2)
				{
				r = KErrUnknown;
				Kern::ShBufClose(ubuf);
				NKern::ThreadLeaveCS();
				break;
				}

			TInt i;

			TInt blocks = Kern::ShBufSize(ubuf) / KTestData1().Length();

			for (i = 0; i < blocks; i++)
				{

				TPtr8 ptr(Kern::ShBufPtr(ubuf) + (i * KTestData1().Length()), KTestData1().Length(), KTestData1().Length());
				r = KTestData1().Compare(ptr);

				if (r)
					{
					break;
					}
				ptr.Fill(i);
				}

			TEST_EXP(r == KErrNone);
			if (r)
				{
				r = KErrUnknown;
				}
			Kern::ShBufClose(ubuf);
			NKern::ThreadLeaveCS();
			}
		break;
// ----------------------------------------------------------------------------
// TInt RShBufTestChannel::AllocateKernelBuffer(TInt aPoolIndex, TInt& aHandle)
		case RShBufTestChannel::ETestAllocateKernelBuffer:
			{
			TInt poolindex = (TInt) a1;
			if ((poolindex != 0) && (poolindex != 1))
				{
				r = KErrArgument;
				break;
				}

			NKern::ThreadEnterCS();

			// Allocate kernel-side buffer
			TShBuf* kbuf;
			r = Kern::ShPoolAlloc(iPools[poolindex], kbuf, 0);

			TEST_KERRNONE(r);
			if (r)
				{
				NKern::ThreadLeaveCS();
				break;
				}

			// Fill it with test data
			TUint i;
			for (i = 0; i < Kern::ShPoolBufSize(iPools[poolindex]) / KTestData2().Length(); i++)
				{
				TPtr8 ptr(Kern::ShBufPtr(kbuf) + (i * KTestData2().Length()), KTestData2().Length(), KTestData2().Length());
				ptr.Copy(KTestData2());
				}

			// Now create a handle for the client
			TInt handle;
#ifdef TEST_CLIENT_THREAD
			handle = Kern::ShBufMakeHandleAndOpen(kbuf, NULL);
#else
			handle = Kern::ShBufMakeHandleAndOpen(kbuf, iClient);
#endif

			TEST_EXP(handle > 0);
			if (handle < 0)
				{
				r = handle;
				Kern::ShBufClose(kbuf);
				NKern::ThreadLeaveCS();

				break;
				}
			TInt n;
			n = reinterpret_cast<DShBuf*>(kbuf)->AccessCount();

			TEST_EXP(n == 2);
			if (n != 2)
				{
				r = KErrUnknown;
				Kern::ShBufClose(kbuf);
				NKern::ThreadLeaveCS();

				break;
				}
#ifdef TEST_CLIENT_THREAD
			NKern::ThreadLeaveCS();

			kumemput(a2, &handle, sizeof(handle));

			NKern::ThreadEnterCS();
			Kern::ShBufClose(kbuf);
			NKern::ThreadLeaveCS();
#else
			NKern::ThreadLeaveCS();

			Kern::ThreadRawWrite(iClient, a2, &handle, sizeof(handle), iClient);
			
			NKern::ThreadEnterCS();
			Kern::DestroyVirtualPinObject(iPin);

			// Close buffer - but it is still referenced by client handle
			Kern::ShBufClose(kbuf);
			NKern::ThreadLeaveCS();
#endif
			}
		break;
// ----------------------------------------------------------------------------
// TInt ContiguousPoolKernel(TShPoolCreateInfo& aInfo)
		case RShBufTestChannel::ETestCreatePoolContiguousPool:
			{
#ifdef TEST_CLIENT_THREAD
			NKern::ThreadEnterCS();
			iCreateinfo = new TShPoolCreateInfo;
			NKern::ThreadLeaveCS();
			TEST_EXP(iCreateinfo != NULL);
			if(!iCreateinfo)
				{
				r = KErrNoMemory;
				break;
				}

			kumemget(iCreateinfo, a1, sizeof(TShPoolInfo));
#endif

			TShPool* mainpool;
			TShPool* otherpool;

			NKern::ThreadEnterCS();

			r = Kern::ShPoolCreate(otherpool, *iCreateinfo, ETrue, KDefaultPoolHandleFlags);
			TEST_KERRNONE(r);

			r = Kern::ShPoolSetBufferWindow(otherpool,-1);
			TEST_KERRNONE(r);

			iCreateinfo->SetContiguous();
			TEST_KERRNONE(r);

			r = Kern::ShPoolCreate(mainpool, *iCreateinfo, ETrue, KDefaultPoolHandleFlags);
			NKern::ThreadEnterCS();
			delete iCreateinfo;
			iCreateinfo = NULL;
			NKern::ThreadLeaveCS();
			TEST_KERRNONE(r);

			r = Kern::ShPoolSetBufferWindow(mainpool,-1);
			TEST_KERRNONE(r);

			TInt i;
			TShBuf* mainbuf[KTestPoolSizeInBufs];
			TShBuf* otherbuf[KTestPoolSizeInBufs];
			for (i = 0; i < KTestPoolSizeInBufs; i++)
				{
				r = Kern::ShPoolAlloc(mainpool, mainbuf[i], 0);
				if (r)
					{
					Kern::Printf("i=%d r=%d\n", i, r);
					TEST_KERRNONE(r);
					}
				r = Kern::ShPoolAlloc(otherpool, otherbuf[i], 0);
				if (r)
					{
					Kern::Printf("i=%d r=%d\n", i, r);
					TEST_KERRNONE(r);
					}
				TBool iscontiguous;
				iscontiguous = IsBufferContiguous(mainbuf[i]);
				if (!iscontiguous)
					{
					Kern::Printf("i=%d\n", i, r);
					TEST_EXP(iscontiguous);
					}
				// delay?
				}

			// Free every other buffer
			for (i = 0; i < KTestPoolSizeInBufs; i += 2)
				{
				Kern::ShBufClose(mainbuf[i]);
				Kern::ShBufClose(otherbuf[i]);
				}

			// Re-allocate buffers
			for (i = 0; i < KTestPoolSizeInBufs; i += 2)
				{
				r = Kern::ShPoolAlloc(otherpool, otherbuf[i], 0);
				if (r)
					{
					Kern::Printf("i=%d r=%d\n", i, r);
					TEST_KERRNONE(r);
					}
				r = Kern::ShPoolAlloc(mainpool, mainbuf[i], 0);
				if (r)
					{
					Kern::Printf("i=%d r=%d\n", i, r);
					TEST_KERRNONE(r);
					}
				TBool iscontiguous;
				iscontiguous = IsBufferContiguous(mainbuf[i]);
				if (!iscontiguous)
					{
					Kern::Printf("i=%d\n", i, r);
					TEST_EXP(iscontiguous);
					// bang
					}
				}
			for (i = 0; i < KTestPoolSizeInBufs; i++)
				{
				Kern::ShBufClose(mainbuf[i]);
				Kern::ShBufClose(otherbuf[i]);
				}

			Kern::ShPoolClose(mainpool);
			Kern::ShPoolClose(otherpool);
			NKern::ThreadLeaveCS();
			}
		break;
// ----------------------------------------------------------------------------
// TInt CreatePoolPhysAddrCont(TInt aBufSize)
// TInt CreatePoolPhysAddrNonCont(TInt aBufSize)
		case RShBufTestChannel::ETestCreatePoolPhysAddrCont:
		case RShBufTestChannel::ETestCreatePoolPhysAddrNonCont:
			{
			r = KErrNone;
#ifndef __WINS__
			TInt bufsize = (TInt) a1;
			TInt minimumAlignmentLog2 = __e32_find_ms1_32(Cache::DmaBufferAlignment());
			if (minimumAlignmentLog2 < 5)
				minimumAlignmentLog2 = 5;
			TInt pagesize;
			r = Kern::HalFunction(EHalGroupKernel, EKernelHalPageSizeInBytes, &pagesize, 0);
			TEST_KERRNONE(r);
			if (r)
				{
				break;
				}

			if (bufsize > KMaxPhysicalMemoryBlockSize)
				{
				// Buffer too large
				return KErrNone;
				}
			TInt physicalblocksize = RoundUp(128 * RoundUp(bufsize, Log2(minimumAlignmentLog2)), Log2(pagesize) + 1);
			if (physicalblocksize > KMaxPhysicalMemoryBlockSize)
				{
				physicalblocksize = KMaxPhysicalMemoryBlockSize;
				}
			if (physicalblocksize < pagesize * 4)
				{
				physicalblocksize = pagesize * 4;
				}

			NKern::ThreadEnterCS();

			// Allocate an array of physical addresses
			TPhysAddr* addrtable = NULL;

			// Allocate physical memory
			TPhysAddr physaddr;
			if (aReqNo == RShBufTestChannel::ETestCreatePoolPhysAddrCont)
				{
				r = Epoc::AllocPhysicalRam(physicalblocksize, physaddr, 0);
				}
			else
				{
				addrtable = (TPhysAddr*) Kern::Alloc((physicalblocksize / pagesize) * sizeof(TPhysAddr));
				TEST_EXP(addrtable != NULL);
				if (addrtable == NULL)
					{
					r = KErrNoMemory;
					NKern::ThreadLeaveCS();
					break;
					}

				TPhysAddr* addrtabletmp;
				addrtabletmp = (TPhysAddr*) Kern::Alloc((physicalblocksize / pagesize / 2) * sizeof(TPhysAddr));
				TEST_EXP(addrtabletmp != NULL);
				if (addrtabletmp == NULL)
					{
					r = KErrNoMemory;
					}
				else
					{
					// Allocate discontiguous memory
					r = Epoc::AllocPhysicalRam(1, addrtable);
					TEST_KERRNONE(r);
					if (r == KErrNone)
						{
						r = Epoc::AllocPhysicalRam(1, addrtabletmp); // 1 page gap
						TEST_KERRNONE(r);
						if (r == KErrNone)
							{
							r = Epoc::AllocPhysicalRam(physicalblocksize / pagesize / 2 - 1, addrtable + 1);
							TEST_KERRNONE(r);
							if (r == KErrNone)
								{
								r = Epoc::AllocPhysicalRam(physicalblocksize / pagesize / 2 - 1, addrtabletmp + 1); // big gap
								TEST_KERRNONE(r);
							if (r == KErrNone)
									{
									r = Epoc::AllocPhysicalRam(physicalblocksize / pagesize / 2, addrtable + physicalblocksize / pagesize / 2);
									TEST_KERRNONE(r);
									r = Epoc::FreePhysicalRam(physicalblocksize / pagesize / 2 - 1, addrtabletmp + 1);
									TEST_KERRNONE(r);
									}
								}
							r = Epoc::FreePhysicalRam(1, addrtabletmp);
							TEST_KERRNONE(r);
							}
						}
					Kern::Free(addrtabletmp);
					}
				}
			
			if (r)
				{
				Kern::Free(addrtable);
				NKern::ThreadLeaveCS();
				break;
				}

			// Create pool
			TInt poolsizeinbufs;
			poolsizeinbufs = physicalblocksize / RoundUp(bufsize, minimumAlignmentLog2);

			TShPool* pool = NULL;
			if (aReqNo == RShBufTestChannel::ETestCreatePoolPhysAddrCont)
				{
				TShPoolCreateInfo inf(TShPoolCreateInfo::EDevice, bufsize,
									  poolsizeinbufs, 0, physicalblocksize / pagesize, physaddr);
				r = Kern::ShPoolCreate(pool, inf, ETrue, KDefaultPoolHandleFlags);
				}
			else
				{
				TShPoolCreateInfo inf(TShPoolCreateInfo::EDevice, bufsize,
									  poolsizeinbufs, 0, physicalblocksize / pagesize, addrtable);
				r = Kern::ShPoolCreate(pool, inf, ETrue, KDefaultPoolHandleFlags);
				}
			TEST_KERRNONE(r);
			if (r == KErrNone)
				{
				// Do some buffer allocation with the pool
				TInt freecount1 = Kern::ShPoolFreeCount(pool);
				RPointerArray<TShBuf> bufarray;
				TInt allocated = 0;
				do
					{
					TShBuf* buf;
					r = Kern::ShPoolAlloc(pool, buf, 0);
					if (r == KErrNone)
						{
						TPtr8 ptr(Kern::ShBufPtr(buf), Kern::ShBufSize(buf), Kern::ShBufSize(buf));
						ptr.Fill('$');
						bufarray.Append(buf);
						allocated++;
						}
					}
				while (r == KErrNone);
				TInt freecount2 = Kern::ShPoolFreeCount(pool);

				if (r != KErrNoMemory)
					{
					TEST_KERRNONE(r);
					}
				while (bufarray.Count())
					{
					if (bufarray[0])
						{
						Kern::ShBufClose(bufarray[0]);
						}
					bufarray.Remove(0);
					}
				TInt freecount3 = Kern::ShPoolFreeCount(pool);
				bufarray.Close();
				//
				r = Kern::ShPoolClose(pool);
				if (r>0)
					{
					r = KErrNone;
					}

				TEST_KERRNONE(r);

				if ((freecount1 != freecount3) || (freecount1 != allocated) || (freecount1 != poolsizeinbufs) || (freecount2))
					{
					r = KErrUnknown;
					Kern::Printf("fc1=%d fc2=%d fc3=%d alloc=%d", freecount1, freecount2, freecount3, allocated);
					TEST_EXP(EFalse);
					}
				}
			NKern::Sleep(5000);
			TInt r2;
			if (aReqNo == RShBufTestChannel::ETestCreatePoolPhysAddrCont)
				{
				r2 = Epoc::FreePhysicalRam(physaddr, physicalblocksize);
				}
			else
				{
				r2 = Epoc::FreePhysicalRam(physicalblocksize / pagesize, addrtable);
				Kern::Free(addrtable);
				}
			TEST_KERRNONE(r2);
			if (!r && r2)
				{
				r = r2; // if an error occurred whilst freeing physical memory, report it
				}
			NKern::ThreadLeaveCS();
#endif // __WINS__
			}
		break;
// ----------------------------------------------------------------------------
// TInt AllocateMax(TInt aPoolIndex, TInt& aAllocated)
		case RShBufTestChannel::ETestAllocateMax:
			{
			TInt r2;
			TInt poolindex = (TInt) a1;
			if ((poolindex != 0) && (poolindex != 1))
				{
				r2 = KErrArgument;
				break;
				}
			TShPoolInfo poolinfo;
			Kern::ShPoolGetInfo(iPools[poolindex], poolinfo);

			NKern::ThreadEnterCS();

			RPointerArray<TShBuf> bufarray;
			do
				{
				TShBuf* buf;
				r2 = Kern::ShPoolAlloc(iPools[poolindex], buf, 0);
				if(r2==KErrNoMemory && (TUint)bufarray.Count()<poolinfo.iMaxBufs)
					{
					NKern::Sleep(1000);
					r2 = Kern::ShPoolAlloc(iPools[poolindex], buf, 0);
					}
				if (r2 == KErrNone)
					{
					r2 = bufarray.Append(buf);
					TEST_KERRNONE(r2);
					if (r2!=KErrNone)
						{
						Kern::ShBufClose(buf);
						r2 = KErrGeneral;
						}
					}
				}
			while (r2 == KErrNone);

			// close all buffers...
			TInt n = bufarray.Count();
			while (n)
				Kern::ShBufClose(bufarray[--n]);

			if (r2 != KErrNoMemory)
				{
				TEST_KERRNONE(r2);
				}
			else
				{
				// Do it once more
				n = 0;
				while (n<bufarray.Count())
					{
					r2 = Kern::ShPoolAlloc(iPools[poolindex], bufarray[n], 0);
					if(r2==KErrNoMemory)
						{
						NKern::Sleep(1000);
						r2 = Kern::ShPoolAlloc(iPools[poolindex], bufarray[n], 0);
						}
					if (r2)
						{
						Kern::Printf("Line %d: n=%d r2=%d", __LINE__, n, r2);
						break;
						}
					++n;
					}

				if (r2 == KErrNone)
					{
					TShBuf* extrabuf;
					r2 = Kern::ShPoolAlloc(iPools[poolindex], extrabuf, 0);

					TEST_EXP(r2 == KErrNoMemory);
					}

				while (n)
					Kern::ShBufClose(bufarray[--n]);
				}
			
			TInt allocated = bufarray.Count();

			bufarray.Close();
			if (r2 == KErrNoMemory)
				{
				r = KErrNone;
				}
			else
				{
				r = r2;
				}

#ifdef TEST_CLIENT_THREAD
			NKern::ThreadLeaveCS();
			kumemput(a2, &allocated, sizeof(allocated));
#else
			NKern::ThreadLeaveCS();

			Kern::ThreadRawWrite(iClient, a2, &allocated, sizeof(allocated), iClient);
			
			NKern::ThreadEnterCS();
			Kern::DestroyVirtualPinObject(iPin);
			NKern::ThreadLeaveCS();
#endif

			}
		break;
// ----------------------------------------------------------------------------
// TInt BufferAlignmentKernel(TInt aBufSize)
		case RShBufTestChannel::ETestBufferAlignmentKernel:
			{
			TInt bufsize = (TInt) a1;
			TInt alignment = (TInt) a2;

			TInt pagesize;
			r = Kern::HalFunction(EHalGroupKernel, EKernelHalPageSizeInBytes, &pagesize, 0);
			TEST_KERRNONE(r);
			if (r)
				{
				break;
				}

			NKern::ThreadEnterCS();

			const TInt KNumBuffers = 20;

			{
			TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, bufsize, KNumBuffers, alignment); // TODO: Change minbufs back to 8 when the pool growing code works
			TShPool* pool;
			r = Kern::ShPoolCreate(pool, inf, ETrue, KDefaultPoolHandleFlags);
			TEST_KERRNONE(r);
			if (r)
				{
				NKern::ThreadLeaveCS();
				break;
				}

			TInt j;
			TShBuf* buf[KNumBuffers];
			memclr(buf,sizeof(buf));
			for (j = 0; j < KNumBuffers; j++)
				{
				r = Kern::ShPoolAlloc(pool, buf[j], 0);
				TEST_KERRNONE(r);
				if (r)
					{
					Kern::Printf("i=%d j=%d", alignment, j);
					break;
					}
				}
			if (r == KErrNone)
				{
				if (alignment < KTestMinimumAlignmentLog2)
					{
					alignment = KTestMinimumAlignmentLog2;
					}
				for (j = 0; j < KNumBuffers; j++)
					{
					if (((TUint32) Kern::ShBufPtr(buf[j]) & ((1 << alignment) - 1)))
						{
						Kern::Printf("Pool%d buf[%d]->Base() == 0x%08x", alignment, j, Kern::ShBufPtr(buf[j]));
						r = KErrUnknown;
						break;
						}
					}
				}
			for (j = 0; j < KNumBuffers; j++)
				{
				if (buf[j])
					{
					Kern::ShBufClose(buf[j]);
					}
				}
			TInt r2;
			r2 = Kern::ShPoolClose(pool);

			if (r2>0)
				{
				r2 = KErrNone;
				}

			TEST_KERRNONE(r2);
			if (r == KErrNone)
				{
				r = r2;
				}
			if (r)
				{
				NKern::ThreadLeaveCS();
				break;
				}
			}
			// Page aligned buffers
			TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, bufsize, KNumBuffers); // TODO: Change minbufs back to 8 when the pool growing code works
			TShPool* pool;
			r = Kern::ShPoolCreate(pool, inf, ETrue, KDefaultPoolHandleFlags);
			TEST_KERRNONE(r);
			if (r)
				{
				NKern::ThreadLeaveCS();
				break;
				}

			r = Kern::ShPoolSetBufferWindow(pool,-1);
			TEST_KERRNONE(r);
			if (r)
				{
				Kern::ShPoolClose(pool);
				NKern::ThreadLeaveCS();
				break;
				}

			TInt j;
			TShBuf* buf[KNumBuffers];
			memclr(buf,sizeof(buf));
			for (j = 0; j < KNumBuffers; j++)
				{
				r = Kern::ShPoolAlloc(pool, buf[j], 0);
				TEST_KERRNONE(r);
				if (r)
					{
					Kern::Printf("j=%d", j);
					break;
					}
				}
			if (r == KErrNone)
				{
				for (j = 0; j < KNumBuffers; j++)
					{
					if ((TUint32) Kern::ShBufPtr(buf[j]) & (pagesize - 1))
						{
						Kern::Printf("buf[%d]->Base() == 0x%08x", j, Kern::ShBufPtr(buf[j]));
						r = KErrUnknown;
						break;
						}
					}
				}
			for (j = 0; j < KNumBuffers; j++)
				{
				if (buf[j])
					{
					Kern::ShBufClose(buf[j]);
					}
				}
			TInt r2;
			r2 = Kern::ShPoolClose(pool);
			if (r2>0)
				{
				r2 = KErrNone;
				}

			TEST_KERRNONE(r2);
			if (!r)
				{
				r = r2;
				}

			NKern::ThreadLeaveCS();
			}
		break;
// ----------------------------------------------------------------------------
// TInt NegativeTestsKernel()
		case RShBufTestChannel::ETestNegativeTestsKernel:
			{
			TInt pagesize;
			r = Kern::HalFunction(EHalGroupKernel, EKernelHalPageSizeInBytes, &pagesize, 0);
			TEST_KERRNONE(r);
			if (r)
				{
				break;
				}
			
            #define TEST_POOLCREATE_FAIL(i, p, e, r) \
                {                               \
                TInt r2;                        \
                TEST_ENTERCS();                 \
                r2 = Kern::ShPoolCreate(p, i, ETrue, KDefaultPoolHandleFlags);  \
                TEST_LEAVECS();                 \
                if (r2 != e)                    \
                    {                           \
                    Kern::Printf("Device drive (line %d) r=%d", __LINE__, r2); \
                    TEST_EXP(EFalse);               \
                    r = KErrUnknown;            \
                    break;                      \
                    }                           \
                }

			TShPool* pool;
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 0, 0); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 100, 0); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 0, 100); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, KMaxTUint, 10); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 10, KMaxTUint); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, KMaxTUint, KMaxTUint); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 65537, 65536); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 10, 1 + (1 << (32 - Log2(pagesize)))); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
#ifndef __WINS__
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 128 * pagesize, (Kern::FreeRamInBytes() / (128 * pagesize)) + 1); TEST_POOLCREATE_FAIL(inf, pool, KErrNoMemory, r); }
#endif
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 0, 0, 0); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 100, 0, 0); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 0, 100, 0); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, KMaxTUint, 10, 0); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, KMaxTUint, 0); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, KMaxTUint, KMaxTUint, 0); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 65537, 65536, 0); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, 10, KMaxTUint); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, 10, 33); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, 300, 24); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, 65537, 16); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 10, 10, Log2(pagesize) + 1); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			{ TShPoolCreateInfo inf(TShPoolCreateInfo::ENonPageAlignedBuffer, 128, 10, 0); inf.SetGuardPages(); TEST_POOLCREATE_FAIL(inf, pool, KErrArgument, r); }
			}
		break;
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// TInt RShBufTestChannel::PinBuffer(TInt aPoolHandle, TInt aBufferHandle)
#ifndef __WINS__
		case RShBufTestChannel::ETestPinBuffer:
			{
			TInt rignore;
			TInt pagesize;
			r = Kern::HalFunction(EHalGroupKernel, EKernelHalPageSizeInBytes, &pagesize, 0);
			TEST_KERRNONE(r);
			if (r)
				{
				break;
				}
			TShPool* upool = NULL;
			TShBuf* ubufu = NULL; // User buffer unmapped
			TShBuf* ubufm = NULL; // User buffer mapped
			DThread* tP;
#ifdef TEST_CLIENT_THREAD
			tP=&Kern::CurrentThread();
#else
			tP=iClient;
#endif

			// Create pin object
			TPhysicalPinObject* pinobj;
			TEST_ENTERCS();
			r = Kern::CreatePhysicalPinObject(pinobj);
			TEST_LEAVECS();
			TEST_KERRNONE(r);
			if (r)
				{
				break;
				}

			// Open user pool
			TEST_ENTERCS();
			r = Kern::ShPoolOpen(upool, tP, (TInt) a1, ETrue, KDefaultPoolHandleFlags);
			TEST_LEAVECS();
			TEST_KERRNONE(r);
			if (r)
				{
				TEST_ENTERCS();
				rignore = Kern::DestroyPhysicalPinObject(pinobj);
				TEST_LEAVECS();
				TEST_KERRNONE(rignore);
				break;
				}
			TShPoolInfo poolinfo;
			Kern::ShPoolGetInfo(upool, poolinfo);

			// Open user buffer but do not map it
			TEST_ENTERCS();
			r = Kern::ShBufOpen(ubufu, tP, (TInt) a2);
			TEST_LEAVECS();
			TEST_KERRNONE(r);
			if (r)
				{
				TEST_ENTERCS();
				rignore = Kern::DestroyPhysicalPinObject(pinobj);
				TEST_LEAVECS();
				TEST_KERRNONE(rignore);

				TEST_ENTERCS();
				rignore = Kern::ShPoolClose(upool);
				TEST_LEAVECS();
				TEST_KERRNONE(rignore);

				break;
				}

			// Allocate an array of physical addresses
			TPhysAddr* addrtable;
			TUint size = Kern::ShBufSize(ubufu);
			TEST_ENTERCS();
			addrtable = (TPhysAddr*) Kern::Alloc((RoundUp(size, Log2(pagesize)) / pagesize) * sizeof(TPhysAddr));
			TEST_LEAVECS();
			TEST_EXP(addrtable != NULL);
			if (!addrtable)
				{
				TEST_ENTERCS();
				rignore = Kern::DestroyPhysicalPinObject(pinobj);
				TEST_LEAVECS();
				TEST_KERRNONE(rignore);
				TEST_ENTERCS();
				rignore = Kern::ShBufClose(ubufu);
				TEST_LEAVECS();

				TEST_KERRNONE(rignore);
				TEST_ENTERCS();
				rignore = Kern::ShPoolClose(upool);
				TEST_LEAVECS();
				TEST_KERRNONE(rignore);
				r = KErrNoMemory;

				break;
				}

			// Pin buffer
			TPhysAddr addr;
			TUint32 mapattr;
			TUint color;
			NKern::ThreadEnterCS();
			r = Kern::ShBufPin(ubufu, pinobj, ETrue, addr, addrtable, mapattr, color);
			NKern::ThreadLeaveCS();
			TEST_KERRNONE(r);
			if (addr != addrtable[0])
				{
				TEST_EXP(addr == KPhysAddrInvalid);
				if (poolinfo.iFlags & EShPoolContiguous)
					{
					TEST_EXP(EFalse); // Shouldn't happen with contiguous pools
					Kern::Printf("addr=0x%08x addrtable[0]=0x%08x", addr, addrtable[0]);
					r = KErrUnknown;
					}
				else
					{
					if (addr != KPhysAddrInvalid)
						{
						TEST_EXP(EFalse); // if buffer is not contiguous addr must be KPhysAddrInvalid
						Kern::Printf("addr=0x%08x addrtable[0]=0x%08x", addr, addrtable[0]);
						r = KErrUnknown;
						}
					}
				}
			// Leave later if this fails

			// Destroy pin object
			TEST_ENTERCS();
			TInt r2 = Kern::DestroyPhysicalPinObject(pinobj);
			TEST_LEAVECS();
			TEST_KERRNONE(r2);

			// Close unmapped buffer
			TEST_ENTERCS();
			rignore = Kern::ShBufClose(ubufu);
			TEST_LEAVECS();

			TEST_KERRNONE(rignore);

			// Leave test now if previous call to Kern::ShBufPin failed
			if (r)
				{
				TEST_ENTERCS();
				Kern::Free(addrtable);
				rignore = Kern::ShPoolClose(upool);
				TEST_LEAVECS();

				TEST_KERRNONE(rignore);
				
				break;
				}

			// Open window if pool is buffer-aligned
			if (poolinfo.iFlags & EShPoolPageAlignedBuffer)
				{
				NKern::ThreadEnterCS();
				r = Kern::ShPoolSetBufferWindow(upool, -1);
				NKern::ThreadLeaveCS();
				TEST_KERRNONE(r);
				if (r)
					{
					TEST_ENTERCS();
					Kern::Free(addrtable);
					rignore = Kern::ShPoolClose(upool);

					TEST_LEAVECS();
					TEST_KERRNONE(rignore);
					
					break;
					}
				}

			// Open user buffer and map it this time
			TEST_ENTERCS();
			r = Kern::ShBufOpen(ubufm, tP, (TInt) a2);
			TEST_LEAVECS();
			TEST_KERRNONE(r);
			if (r)
				{
				TEST_ENTERCS();
				Kern::Free(addrtable);
				rignore = Kern::ShPoolClose(upool);
				TEST_LEAVECS();

				TEST_KERRNONE(rignore);

				break;
				}

			// Ensure that physical addresses match
			TUint8* ptr = Kern::ShBufPtr(ubufm);
			TEST_EXP(ptr != NULL);
			TBool isok = ETrue;
			TInt i;
			for (i = 0; i < RoundUp(size, Log2(pagesize)) / pagesize; i++)
				{
				TPhysAddr current = Epoc::LinearToPhysical((TLinAddr) ptr + i * pagesize);
				if (current != addrtable[i])
					{
					Kern::Printf("Page %d: Current=0x%08x addrtable=0x%08x (linaddr=0x%08x)", i, current, addrtable[i], ptr + i * pagesize);
					isok = EFalse;
					break;
					}
				}
			if (!isok)
				{
				r = KErrUnknown;
				}
			TEST_KERRNONE(r);

			// Close mapped buffer
			TEST_ENTERCS();
			rignore = Kern::ShBufClose(ubufm);
			TEST_LEAVECS();

			TEST_KERRNONE(rignore);

			// Close pool
			TEST_ENTERCS();
			rignore = Kern::ShPoolClose(upool);
			TEST_LEAVECS();

			TEST_KERRNONE(rignore);

			// Free address table
			TEST_ENTERCS();
			Kern::Free(addrtable);
			TEST_LEAVECS();

			if (!r && r2)
				{
				r = r2;
				}
			}
		break;
#endif // __WINS__
// ----------------------------------------------------------------------------
		case RShBufTestChannel::EFromRShBufProcessAndReturn:
			{
			// inline TInt FromRShBufProcessAndReturn(TInt aHandle);
			TInt bufsize = (TInt) a1;

			TEST_ENTERCS();
			// Allocate kernel-side buffer
			TShBuf* kbuf;
			r = Kern::ShPoolAlloc(iPools[0], kbuf, 0);

			if (r)
				{
				TEST_LEAVECS();
				break;
				}

			TUint8*  ptr = Kern::ShBufPtr(kbuf);
			TInt* lengthPtr = (TInt*)ptr;
			*lengthPtr = bufsize - 2;

#if 0 // do not cache
			for(TInt pos = 4;  pos < bufsize;  pos++)
				{
				ptr[pos] = (TUint8)(pos & 31);
				}
#endif

			// Now create a handle for the client
			TInt handle;
#ifdef TEST_CLIENT_THREAD
			handle = Kern::ShBufMakeHandleAndOpen(kbuf, NULL);
#else
			handle = Kern::ShBufMakeHandleAndOpen(kbuf, iClient);
#endif

			if (handle < 0)
				{
				r = handle;
				Kern::ShBufClose(kbuf);
				TEST_LEAVECS();
				break;
				}

			// Close buffer - but it is still referenced by client handle
			Kern::ShBufClose(kbuf);
			TEST_LEAVECS();

			r=handle;
			}
		break;
		case RShBufTestChannel::EFromRShBufProcessAndRelease:
			{
			// inline TInt FromRShBufProcessAndRelease(TInt aHandle);
			TShBuf* ubuf = NULL;
			DThread* tP;

#ifdef TEST_CLIENT_THREAD
			tP=&Kern::CurrentThread();
#else
			tP=iClient;
#endif

			TEST_ENTERCS();
			r = Kern::ShBufOpen(ubuf, tP, (TInt) a1);
			// close handle on behalf of user side application
			Kern::CloseHandle(tP, (TInt) a1);
			TEST_LEAVECS();

			if(r!=KErrNone)
				{
				Kern::Printf("Buf not found");
				break;
				}

#ifdef _DEBUG
			TUint8*  dataPtr = Kern::ShBufPtr(ubuf);

			TInt*  lengthPtr = (TInt*)(&dataPtr[0]);
			
			for(TInt pos = 4;  pos < *lengthPtr;  pos++)
				{
				if (dataPtr[pos] != (TUint8)(pos & 31))
					{
					r=KErrCorrupt;
					Kern::Printf("Buf corrupt");
					break;
					}
				}
#endif

			TEST_ENTERCS();
			Kern::ShBufClose(ubuf);
			TEST_LEAVECS();

			r=KErrNone;
			}
		break;
		case RShBufTestChannel::EFromTPtr8ProcessAndReturn:
			{
			TInt bufsize = (TInt) a2;
			TPtr8 rxBuf(iDriverRxBuffer,sizeof(iDriverRxBuffer),sizeof(iDriverRxBuffer));

#if 0
			for(TInt pos = 0;  pos < bufsize;  pos++)
				{
				rxBuf[pos] = (TUint8)(pos & 31);
				}
#endif
			rxBuf.SetLength(bufsize-2);

#ifdef TEST_CLIENT_THREAD
			Kern::KUDesPut(*(TDes8*)a1, rxBuf);		// put content from test app
			r=KErrNone;
#else
			r = Kern::ThreadDesWrite(iClient, a1, rxBuf, 0, iClient);
			
			NKern::ThreadEnterCS();
			Kern::DestroyVirtualPinObject(iPin);
			NKern::ThreadLeaveCS();
#endif
			}
		break;
		case RShBufTestChannel::EFromTPtr8ProcessAndRelease:
			{
			// inline TInt FromTPtr8ProcessAndRelease(TDes8& aBuf);
#if defined _DEBUG  || defined TEST_CLIENT_THREAD
			TPtr8 bufp(iDriverTxBuffer, sizeof(iDriverTxBuffer), sizeof(iDriverTxBuffer));
#endif
#ifdef TEST_CLIENT_THREAD
			Kern::KUDesGet(bufp,*(const TDesC8*)a1);		// get content from test app
#endif
			
#ifdef _DEBUG
			TUint8* bufptr = const_cast<TUint8*>(bufp.Ptr());
			for(TInt pos = 0; pos < bufp.Length();  pos++)
				{
				if (bufptr[pos] != (TUint8)(pos & 31))
					{
					r=KErrCorrupt;
					Kern::Printf("Buf corrupt");
					break;
					}
				}
#endif

			// Nothing to release here!
			
			r=KErrNone;
			}
		break;
		}
	
	return r;
	}
