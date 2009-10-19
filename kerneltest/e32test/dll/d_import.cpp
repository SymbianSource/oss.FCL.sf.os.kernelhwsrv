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
// e32test\mmu\d_export.cpp
// 
//

#include <kernel/kern_priv.h>
#include "d_import.h"

//
// Class definitions
//

class DImportFactory : public DLogicalDevice
	{
public:
	~DImportFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DImportChannel : public DLogicalChannelBase
	{
public:
	DImportChannel();
	~DImportChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
public:
	DImportFactory*	iFactory;
	};

//
// DImportFactory
//

TInt DImportFactory::Install()
	{
	return SetName(&KImportTestLddName);
	}

DImportFactory::~DImportFactory()
	{
	}

void DImportFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DImportFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = NULL;
	DImportChannel* channel=new DImportChannel;
	if(!channel)
		return KErrNoMemory;
	channel->iFactory = this;
	aChannel = channel;
	return KErrNone;
	}

DECLARE_STANDARD_LDD()
	{
	return new DImportFactory;
	}

//
// DImportChannel
//

TInt DImportChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

DImportChannel::DImportChannel()
	{
	}

DImportChannel::~DImportChannel()
	{
	}


TInt DImportChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNotSupported;

	switch(aFunction)
		{
		case RImportLdd::ERunImport:
			r = ExportMultiplyFunction((TUint)a1, (TUint)a2);
			break;
		}
	return r;
	}
