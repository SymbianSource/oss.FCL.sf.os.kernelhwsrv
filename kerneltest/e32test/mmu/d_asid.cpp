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
// e32test\mmu\d_asid.cpp
// 
//

#include <kernel/kern_priv.h>
#include "d_asid.h"

//
// Class definitions
//

class DAsidFactory : public DLogicalDevice
	{
public:
	~DAsidFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DAsidChannel : public DLogicalChannelBase
	{
public:
	DAsidChannel();
	~DAsidChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
public:
	DAsidFactory*	iFactory;
private:
	DThread* iThread;
	};

//
// DAsidFactory
//

TInt DAsidFactory::Install()
	{
	return SetName(&KMemoryTestLddName);
	}

DAsidFactory::~DAsidFactory()
	{
	}

void DAsidFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DAsidFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = NULL;
	DAsidChannel* channel=new DAsidChannel;
	if(!channel)
		return KErrNoMemory;
	channel->iFactory = this;
	aChannel = channel;
	return KErrNone;
	}

DECLARE_STANDARD_LDD()
	{
	return new DAsidFactory;
	}

//
// DAsidChannel
//

TInt DAsidChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

DAsidChannel::DAsidChannel() : iThread(NULL)
	{
	}

DAsidChannel::~DAsidChannel()
	{
	if (iThread)
		iThread->Close(NULL);
	}


TInt DAsidChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNotSupported;

	switch(aFunction)
		{
		case RAsidLdd::EGetCurrentThread:
			{
			DThread* thread = &Kern::CurrentThread();
			kumemput32(a1, (TAny*)&thread, sizeof(DThread*));
			r = KErrNone;
			}
			break;

		case RAsidLdd::EOpenThread:
			{
			if (iThread)
				{
				r = KErrInUse;
				break;
				}
			iThread = (DThread*) a1;
			iThread->Open();
			r = KErrNone;
			}
			break;

		case RAsidLdd::ECloseThread:
			{
			if (!iThread)
				{
				r = KErrNotFound;
				break;
				}
			NKern::ThreadEnterCS();
			iThread->Close(NULL);
			iThread = NULL;
			r = KErrNone;
			NKern::ThreadLeaveCS();
			}
			break;
			
		case RAsidLdd::EReadDesHeader:
			{
			SDesHeader hdr;
			TUint8* ptr;
			kumemget32((TAny*)&hdr, a2, sizeof(SDesHeader));
			r = Kern::ThreadGetDesInfo((DThread*)a1, hdr.iDes, hdr.iLength, hdr.iMaxLength, ptr, ETrue);
			if (r == KErrNone)
				{// copy the data back to user side struct.
				kumemput32(a2, &hdr, sizeof(SDesHeader));
				}
			}
			break;
		}
	return r;
	}
