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

DMutex* TheTraceMutex = NULL;
_LIT(KLitTraceMutexName, "FTRACE_MUTEX");

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

class DLddFTrace : public DLogicalChannelBase
	{
public:
	DLddFTrace();
	~DLddFTrace();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);

	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);

private:
	void DoCancel(TInt aReqNo);
	TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);

private:
    };

DECLARE_STANDARD_LDD()
	{
	TInt r = Kern::MutexCreate(TheTraceMutex,  KLitTraceMutexName, KMutexOrdNone);
	if (r != KErrNone)
		return NULL;

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
    TPtrC name=_L("FTrace");
	return(SetName(&name));
	}

void DLddFactoryFTrace::GetCaps(TDes8& /*aDes*/) const
	{
	}

DLddFactoryFTrace::~DLddFactoryFTrace()
	{
	}

DLddFTrace::DLddFTrace()
	{
    }

DLddFTrace::~DLddFTrace()
	{
    }

TInt DLddFTrace::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{

	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return(KErrNotSupported);

    return(KErrNone);
	}

void DLddFTrace::DoCancel(TInt /*aReqNo*/)
	{
	}

TInt DLddFTrace::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
	NKern::ThreadEnterCS();
	Kern::MutexWait(*TheTraceMutex);
	TInt r = DoControl(aReqNo, a1, a2);
	Kern::MutexSignal(*TheTraceMutex);
	NKern::ThreadLeaveCS();

	return r;
	}


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

			XTRAP(r, XT_DEFAULT, kumemget32(&args, a1, sizeof(args)));
			if (r != KErrNone)
				return r;

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

				XTRAP(r, XT_DEFAULT, kumemget32(desc, des, sizeof(desc)));
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
				XTRAP(r, XT_DEFAULT, kumemget(gTraceBuffer+offset, (const TUint8*) desc[1], len));
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

