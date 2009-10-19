// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\pbusmedia.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __PBUSMEDIA_H__
#define __PBUSMEDIA_H__
#include <drivers/pbus.h>
#include <drivers/locmedia.h>

const TInt KMediaRemountForceMediaChange = 0x00000001;

/**
  The purpose of this class is to abstract the interface between the local media sub-system and peripheral bus controller.
  An instance of this class provides the notification call back of media change and power down events.
  This class is used by Media driver.
  @see TMMCardControllerInterface
  */
NONSHARABLE_CLASS(DPBusPrimaryMedia) : public DPrimaryMediaBase
	{
public:
	DPBusPrimaryMedia(DPBusSocket* aSocket);
public:
	virtual TInt Create(TMediaDevice aDevice, TInt aMediaId, TInt aLastMediaId);
	virtual TInt QuickCheckStatus();
	virtual TInt ForceMediaChange(TInt aFlags);
	virtual TInt InitiatePowerUp();
	virtual TInt DoInCritical();
	virtual void DoEndInCritical();
	virtual void DeltaCurrentConsumption(TInt aCurrent);
	virtual void DefaultDriveCaps(TLocalDriveCapsV2& aCaps);
	virtual TBool IsRemovableDevice(TInt& aSocketNum);
public:
	void PBusStateChange(TInt aState, TInt anError);
public:
	/**
	 Peripheral bus notification callback.
     @see TPBusCallBack
	 */
	TPBusCallBack iBusCallBack;

	/**
	 Pointer to peripheral bus socket object.
     @see DPBusSocket
	 */
	DPBusSocket* iSocket;

	/**
     Holds one of the state corresponding to PBUS.
	 @see TPBusState
	 */
	TInt iPBusState;
	/** 
	Updated with greater than zero, if pbus supports more than one card.
	For each slot new instance of DPBusPrimaryMedia is created and updated in iSlotNumber.
	*/
	TInt iSlotNumber;	
	};

#endif

