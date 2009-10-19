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
#include "d_export.h"

//
// Class definitions
//

class DExportFactory : public DLogicalDevice
	{
public:
	~DExportFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DExportChannel : public DLogicalChannelBase
	{
public:
	DExportChannel();
	~DExportChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
public:
	DExportFactory*	iFactory;
	};

//
// DExportFactory
//

TInt DExportFactory::Install()
	{
	return SetName(&KExportTestLddName);
	}

DExportFactory::~DExportFactory()
	{
	}

void DExportFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DExportFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = NULL;
	DExportChannel* channel=new DExportChannel;
	if(!channel)
		return KErrNoMemory;
	channel->iFactory = this;
	aChannel = channel;
	return KErrNone;
	}

DECLARE_STANDARD_LDD()
	{
	return new DExportFactory;
	}

//
// DExportChannel
//

TInt DExportChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

DExportChannel::DExportChannel()
	{
	}

DExportChannel::~DExportChannel()
	{
	}


TInt DExportChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNotSupported;

	switch(aFunction)
		{
		case RExportLdd::ERunExport:
			r = ExportMultiplyFunction((TUint)a1, (TUint)a2);
			break;
		}
	return r;
	}

EXPORT_C TInt ExportMultiplyFunction(TUint aNum0, TUint aNum1)
	{
	return aNum0 * aNum1;
	}
