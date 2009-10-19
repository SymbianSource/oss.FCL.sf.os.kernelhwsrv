/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
// D_FTRACE.CPP
//
//
//
#include "plat_priv.h"
#include <kernel/kernel.h>

#include "f32trace.h"

#define __DLOGICAL_CHANNEL_BASE__

#ifdef __DLOGICAL_CHANNEL_BASE__
DMutex* TheTraceMutex = NULL;
_LIT(KLitTraceMutexName, "FTRACE_MUTEX");
#else
TDynamicDfcQue* gDfcQ;
const TInt KDFTraceThreadPriority = 27;
_LIT(KDFTraceThread,"DFTraceThread");
#endif

const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=0;


class DLddFactoryFTrace : public DLogicalDevice
	{
public:
	DLddFactoryFTrace();
	virtual ~DLddFactoryFTrace();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

#ifdef __DLOGICAL_CHANNEL_BASE__
class DLddFTrace : public DLogicalChannelBase
#else
class DLddFTrace : public DLogicalChannel
#endif
	{
public:
	DLddFTrace();
	~DLddFTrace();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);

#ifdef __DLOGICAL_CHANNEL_BASE__
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
#else
	virtual void HandleMsg(class TMessageBase *);
#endif

private:
	void DoCancel(TInt aReqNo);
	TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);

private:
	DThread* iClient;
    };

DECLARE_STANDARD_LDD()
	{
#ifdef __DLOGICAL_CHANNEL_BASE__
	TInt r = Kern::MutexCreate(TheTraceMutex,  KLitTraceMutexName, KMutexOrdNone);
	if (r != KErrNone)
		return NULL;
#endif

	return new DLddFactoryFTrace;
	}

DLddFactoryFTrace::DLddFactoryFTrace()
	{

    iParseMask=KDeviceAllowUnit;  // Pass stack number as unit
	iUnitsMask=0xffffffff;
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

TInt DLddFactoryFTrace::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DLddFTrace;
	return aChannel ? KErrNone : KErrNoMemory;
	}

TInt DLddFactoryFTrace::Install()
	{
#ifndef __DLOGICAL_CHANNEL_BASE__
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(gDfcQ, KDFTraceThreadPriority, KDFTraceThread);
	if (r != KErrNone)
		return r; 	
#endif

    TPtrC name=_L("FTrace");
	return(SetName(&name));
	}

void DLddFactoryFTrace::GetCaps(TDes8& /*aDes*/) const
	{
	}

DLddFactoryFTrace::~DLddFactoryFTrace()
	{
#ifndef __DLOGICAL_CHANNEL_BASE__
	if (gDfcQ)
		gDfcQ->Destroy();
#endif
	}

DLddFTrace::DLddFTrace()
	{
	iClient=&Kern::CurrentThread();
	((DObject*)iClient)->Open();	// can't fail since thread is running
    }

DLddFTrace::~DLddFTrace()
	{
  	Kern::SafeClose((DObject*&)iClient,NULL);
    }

TInt DLddFTrace::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{

	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return(KErrNotSupported);

#ifndef __DLOGICAL_CHANNEL_BASE__
	SetDfcQ(gDfcQ);
	iMsgQ.Receive();
#endif

    return(KErrNone);
	}

void DLddFTrace::DoCancel(TInt /*aReqNo*/)
	{
	}

#ifdef __DLOGICAL_CHANNEL_BASE__
TInt DLddFTrace::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
	NKern::ThreadEnterCS();
	Kern::MutexWait(*TheTraceMutex);
	TInt r = DoControl(aReqNo, a1, a2);
	Kern::MutexSignal(*TheTraceMutex);
	NKern::ThreadLeaveCS();

	return r;
	}

#else

void DLddFTrace::HandleMsg(TMessageBase* aMsg)
    {
    TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;
    
	if (id==(TInt)ECloseMsg)
		{
		m.Complete(KErrNone, EFalse);
		return;
		}
    else if (id==KMaxTInt)
		{
		// DoCancel
		m.Complete(KErrNone,ETrue);
		return;
		}

    if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		
		// WDP FIXME change this to use the Kern::RequestComplete() API which doesn't take a thread pointer
		// when this becomes available
    	Kern::RequestComplete(iClient, pS, KErrNotSupported);
		m.Complete(KErrNotSupported, ETrue);
		}
    else
		{
		// DoControl
		TInt r=DoControl(id, m.Ptr0(), m.Ptr1());
		m.Complete(r,ETrue);
		}
	}
#endif	// __DLOGICAL_CHANNEL_BASE__

const TUint KTraceBufferSize = 4096;
TUint8 gTraceBuffer[KTraceBufferSize];



#define MIN(a,b)			((a) < (b) ? (a) : (b))

TInt DLddFTrace::DoControl(TInt aFunction, TAny* a1, TAny* a2)
//
// Mostly requests (but some kernel server async ones)
//
	{
	TInt r=KErrNotSupported;
	switch (aFunction)
		{
        case RFTrace::ETraceMultiple:
            {
			typedef struct {
				TClassification iCategory;
				TUint8 iPadding1[sizeof(TUint) - sizeof(TClassification)];

				TFormatId iFormatId;
				TUint8 iPadding2[sizeof(TUint) - sizeof(TFormatId)];

				TUint32 iUid;
				TInt iDescriptorCount;
				} TraceArgs;

			TraceArgs args={0};

#ifdef __DLOGICAL_CHANNEL_BASE__
			XTRAP(r, XT_DEFAULT, kumemget32(&args, a1, sizeof(args)));
			if (r != KErrNone)
				return r;
#else
			r = Kern::ThreadRawRead(iClient, a1, &args, sizeof(args));
			if (r != KErrNone)
				return r;
#endif


			// current descriptor - MUST be either a TPtr8 or a TBuf8<4>
			TUint32 desc[2] = {0, 0};	
			TUint32& desLength = desc[0];

			TUint offset = 0;

			*((TUint*) (gTraceBuffer+offset)) = args.iFormatId;
			offset+= sizeof(TUint);

			TDesC8* des = (TDesC8*) ((TUint8*) a2);
			const TInt desSize = sizeof(TPtrC8);
			for (TInt n=0; n< args.iDescriptorCount; n++, des = (TDesC8*) (((TUint8*) des) + desSize) )
				{

#ifdef __DLOGICAL_CHANNEL_BASE__
				XTRAP(r, XT_DEFAULT, kumemget32(desc, des, sizeof(desc)));
#else
				r = Kern::ThreadRawRead(iClient, des, desc, sizeof(desc));
				if (r != KErrNone)
					return r;
#endif
				TUint32 desType = desLength >> KShiftDesType;
				desLength &= (TUint) (KMaskDesLength);
				if (desType == EPtrC)
					{
					*((TUint*) (gTraceBuffer+offset)) = desLength;
					desLength = (desLength+3)&~3;
					offset+= sizeof(TUint);
					}
				else if (desType == EBufC)
					{
					*((TUint*) (gTraceBuffer+offset)) = desc[1];
					offset+= sizeof(TUint);
					if (desLength > 4)
						return KErrArgument;
					desLength = 0;
					continue;
					}
				else
					return KErrArgument;

				TUint len = MIN(KTraceBufferSize - offset, desLength);
#ifdef __DLOGICAL_CHANNEL_BASE__
				XTRAP(r, XT_DEFAULT, kumemget(gTraceBuffer+offset, (const TUint8*) desc[1], len));
#else
				TPtr8 dest(gTraceBuffer+offset, len, len);
				r = Kern::ThreadDesRead(iClient, des, dest, 0, KChunkShiftBy0);
				if (r != KErrNone)
					return r;
#endif
				offset+= len;
				
				}

			BTrace::OutFilteredBig 
				(BTRACE_HEADER_C(8,args.iCategory, 0), args.iUid, gTraceBuffer, offset);

			r=KErrNone;
			break;
            }
		}
	return(r);
	}

