// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\d_lddturnaoundtimertest.cpp
// LDD for getting the timer count & ticks for testing turnaround timer implementation.
// 
//

#include <kernel/kernel.h>
#include "d_lddturnaroundtimertest.h"

class DTest1;
class DTestFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	DTestFactory();
	virtual TInt Install(); 					//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

class DTest1 : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DTest1();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	};



DECLARE_STANDARD_LDD()
	{
	return new DTestFactory;
	}

//
// Constructor
//
DTestFactory::DTestFactory()
	{

	}

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
//
// Create new channel
//
	aChannel=new DTest1;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
	{
	return SetName(&KLddName);
	}

void DTestFactory::GetCaps(TDes8& /*aDes*/) const
//
// Get capabilities - overriding pure virtual
//
	{
	}

TInt DTest1::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create channel
//
	{
	return KErrNone;
	}

DTest1::~DTest1()
//
// Destructor
//
	{
	}

TInt DTest1::Request(TInt aReqNo, TAny* a1, TAny* /*a2*/)
	{
//
// Get the timer tick count & ticks
//
    TUint temp = 0;
	switch(aReqNo)
		{
		case (RLddTest1::EGET_TIMERTICKS):
			{
			kumemget32(&temp, a1, sizeof(temp));
			temp = NKern::TimerTicks(temp);
			kumemput(a1, &temp, sizeof(temp));
			return KErrNone;
			}
		case (RLddTest1::EGET_TIMERTICKCOUNT):
			{
			temp = NKern::TickCount();
       		kumemput(a1, &temp, sizeof(temp));
			return KErrNone;
			}
		}
	return KErrNone;	
}
