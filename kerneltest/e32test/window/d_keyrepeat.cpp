// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\window\d_keyrepeat.cpp
// LDD for testing class TRawEvent kernel side keyrepeat entries
// 
//

#include <kernel/kernel.h>

#include "d_keyrepeat.h"

class DKLDDFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	DKLDDFactory();
	virtual TInt Install(); 								//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;				//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

class DKLDDChannel : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DKLDDChannel();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
public:
	RKeyEvent iStoredKeyEvent;
	TRawEvent iStoredEvent;
	};


DECLARE_STANDARD_LDD()
	{
	return new DKLDDFactory;
	}

//
// Constructor
//
DKLDDFactory::DKLDDFactory()
	{
	}

TInt DKLDDFactory::Create(DLogicalChannelBase*& aChannel)
	{
//
// Create new channel
//  
	aChannel=new DKLDDChannel;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DKLDDFactory::Install()
//
// Install the LDD - overriding pure virtual
	{
	return SetName(&KLddName);
	}

void DKLDDFactory::GetCaps(TDes8& /*aDes*/) const
//
// Get capabilities - overriding pure virtual
//
	{
	}

TInt DKLDDChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create channel
//
	{
	return KErrNone;
	}

DKLDDChannel::~DKLDDChannel()
//
// Destructor
//
	{
	}

RKeyEvent::RKeyEvent()
//
// default constructor
//
	{
	}

TInt DKLDDChannel::Request(TInt aReqNo, TAny* a1, TAny* /*a2*/)
	{
	TInt r=KErrNone;
	TInt repeats=0;

	switch(aReqNo)
		{
		case RTestKeyRepeatLdd::ESetRepeat:
			kumemget(&iStoredKeyEvent,a1,sizeof(RKeyEvent));
			iStoredEvent.SetRepeat(TRawEvent::EKeyRepeat, iStoredKeyEvent.iKey, iStoredKeyEvent.iRepeatCount);
			NKern::ThreadEnterCS();
			r=Kern::AddEvent(iStoredEvent);
			NKern::ThreadLeaveCS();
			break;
		case RTestKeyRepeatLdd::ERepeats:
			repeats = iStoredEvent.Repeats();
			if (repeats!=iStoredKeyEvent.iRepeatCount)
				{
				r=KErrGeneral;
				}
			break;

		default:
			r=KErrNotSupported;
			break;
		} 
	return r;
	}



