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
// e32test\power\d_lddpowerseqtest.cpp
// LDD for testing the power up and down sequence
// 
//

#include <kernel/kernel.h>
#include <kernel/kpower.h>
#include "d_lddpowerseqtest.h"


_LIT(KLitPower1,"PowerSeqTest1");
_LIT(KLitPower2,"PowerSeqTest2");

//
// Variables to store asynchronous status request and time count user variables address.
//
TRequestStatus *aStatus_up1;
TRequestStatus *aStatus_up2;
TRequestStatus *aStatus_down1;
TRequestStatus *aStatus_down2;
TUint *time_power1down;
TUint *time_power2down;
TUint *time_power1up;
TUint *time_power2up;
TUint sleepTime;


class DTest1;
//
// Power handler1
//
class DTest1PowerHandler : public DPowerHandler
	{
public:
	DTest1PowerHandler();

	void PowerUp();
	void PowerDown(TPowerState);
	};

//
// Power handler2
//
class DTest2PowerHandler : public DPowerHandler
	{
public:
	DTest2PowerHandler();
	void PowerUp();
	void PowerDown(TPowerState);
	void ActDead();
private:
	TBool iActDead;
	};

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
	DTest1PowerHandler power1;
	DTest2PowerHandler power2;
private:
	TUint iPslShutdownTimeoutMsBackup;
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
	//try to remove a handler that hasn't been added yet - should not cause any problems
	power1.Remove();
	//Add power handlers
	power2.Add();
	power1.Add();
	return KErrNone;
	}

DTest1::~DTest1()
//
// Destructor
//
	{
	power1.Remove();
	power2.Remove();
	//try to remove a handler twice - should not cause any problems
	power2.Remove();
	((DTestPowerManager*)(Kern::PowerModel()))->iPslShutdownTimeoutMs = iPslShutdownTimeoutMsBackup;
	}

TInt DTest1::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
//
// Store status requests and time stamp variable for each power up and power down
//

	if(aReqNo<0)
		{
		// 'Request' functions...
		TRequestStatus* s = (TRequestStatus*)a1;
		TAny* args[2];
		kumemget32(args,a2,sizeof(args)); // get user side arguments

		switch(~aReqNo)
			{
			case RLddTest1::EPOWERDOWN_POWER1:
				aStatus_down1 = s;
				time_power1down = (TUint*)args[0];
				break;

			case RLddTest1::EPOWERDOWN_POWER2:
				aStatus_down2 = s;
				time_power2down = (TUint*)args[0];
				break;

			case RLddTest1::EPOWERUP_POWER1:
				aStatus_up1 = s;
				time_power1up = (TUint*)args[0];
				break;

			case RLddTest1::EPOWERUP_POWER2:
				aStatus_up2 = s;
				time_power2up = (TUint*)args[0];
				break;
			}
		}
	else
		{
		// 'Control' functions...
		switch(aReqNo)
			{
				
			// DoControl
			case RLddTest1::ESET_SLEEPTIME:
				sleepTime = (TUint)a1;
				break;
			case RLddTest1::EPOWER_ACTDEAD_POWER2:
				power2.ActDead();
				break;	
			case RLddTest1::EPOWER_ESETPOWERDOWNTIMEOUT:
				iPslShutdownTimeoutMsBackup = ((DTestPowerManager*)(Kern::PowerModel()))->iPslShutdownTimeoutMs;
				((DTestPowerManager*)(Kern::PowerModel()))->iPslShutdownTimeoutMs = (TUint)a1;
				break;				
			}
		}

	return KErrNone;
	}

DTest1PowerHandler::DTest1PowerHandler():DPowerHandler(KLitPower1)
	{
//
// Power handler1 constructor
//
	}

DTest2PowerHandler::DTest2PowerHandler():DPowerHandler(KLitPower2), iActDead(EFalse)
	{
//
// Power handler2 constructor
//
	}


void DTest1PowerHandler::PowerUp()
	{
//
// Sleep for sometime to get different tick counts for comparision.
// Copy the tick count to user variable and complete the request
//
	NKern::Sleep(sleepTime);

	TUint temp = NKern::TickCount();
	kumemput(time_power1up, (const TUint *)&temp, sizeof(temp));

	Kern::RequestComplete(aStatus_up1, KErrNone);

	PowerUpDone();

	}

void DTest2PowerHandler::PowerUp()
	{
//
// Copy the tick count to user variable and complete the request
//

	TUint temp = NKern::TickCount();
	kumemput(time_power2up, (const TUint *)&temp, sizeof(temp));

	Kern::RequestComplete(aStatus_up2, KErrNone);

	PowerUpDone();
	}

void DTest1PowerHandler::PowerDown(TPowerState /*aState*/)
	{
//
// Copy the tick count to user variable and complete the request
//

	TUint temp = NKern::TickCount();
	kumemput(time_power1down, (const TUint *)&temp, sizeof(temp));

	Kern::RequestComplete(aStatus_down1, KErrNone);

	PowerDownDone();
	}

void DTest2PowerHandler::PowerDown(TPowerState /*aState*/)
	{
//
// Sleep for sometime to get different tick counts for comparision.
// Copy the tick count to user variable and complete the request
//

	NKern::Sleep(sleepTime);

	TUint temp = NKern::TickCount();
	kumemput(time_power2down, (const TUint *)&temp, sizeof(temp));

	Kern::RequestComplete(aStatus_down2, KErrNone);

	if(!iActDead)
		{
		PowerDownDone();
		}
	}

void DTest2PowerHandler::ActDead()
	{
	iActDead = ETrue;
	}

