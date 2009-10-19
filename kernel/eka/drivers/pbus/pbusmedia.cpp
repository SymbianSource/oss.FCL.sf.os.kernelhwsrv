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
// e32\drivers\pbus\pbusmedia.cpp
// 
//

#include <drivers/pbusmedia.h>

void mediaCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2)
	{
	DPBusPrimaryMedia* pM=(DPBusPrimaryMedia*)aPtr;
	__KTRACE_OPT(KLOCDRV,Kern::Printf("mediaCallBack media %d, reason %d, a1=0x%x, a2=0x%x",pM->iMediaId,aReason,a1,a2));
	switch (aReason)
		{
		case TPBusCallBack::EPBusStateChange:
			pM->PBusStateChange((TInt)a1,(TInt)a2);
			break;
		}
	}

/**
  Constructor for DPBusPrimaryMedia. Initializes the iSocket with aSocket.
  @param aSocket	Pointer to DPBusSocket object
  @see DPBusPrimaryMedia::iSocket
  */

DPBusPrimaryMedia::DPBusPrimaryMedia(DPBusSocket* aSocket)
	:	iSocket(aSocket)
	{
	}
/**
  This function install a media call back for a removable media device.
  @param aDevice	Local media ID.
  @param aMediaId	Media Id (unique for a media subsystem)
  @param aLastMediaId	This indicates number of used media ids+ number of DMedia objects to be associated with the media driver
  @return KErrNone if successful,
          otherwise one of the other system wide error codes.
  @see DPrimaryMediaBase::Create()
  */
TInt DPBusPrimaryMedia::Create(TMediaDevice aDevice, TInt aMediaId, TInt aLastMediaId)
	{
	// Permanently install a media call back if for a removable media device
	TInt r=KErrArgument;
	iPBusState=EPBusCardAbsent;
	if (__IS_REMOVABLE(aDevice))
		{
		iBusCallBack.iFunction=mediaCallBack;
		iBusCallBack.iPtr=this;
		iBusCallBack.SetSocket(iSocket->iSocketNumber);
		iDfcQ=&iSocket->iDfcQ;
		r=DPrimaryMediaBase::Create(aDevice,aMediaId,aLastMediaId);
		if (r==KErrNone)
			{
			iBusCallBack.Add();
			iPBusState=iSocket->State();
			iMsgQ.Receive();
			}
		}
	return r;
	}

/**
  Checks the PBUS state.
  @return KErrNone if successful,
          KErrNotReady if card is absent.
  @see TPBusState
  */
TInt DPBusPrimaryMedia::QuickCheckStatus()
	{
	TInt r=KErrNone;
	if (iSocket && iSocket->State()==EPBusCardAbsent)
		r=KErrNotReady;
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPBusPrimaryMedia::QuickCheckStatus media %d returns %d",iMediaId,r));
	return r;
	}

/**
  This function is called by the local media device driver to force a remount of the media device.
  @param  aFlags	Corresponds to force media change.
  @return KErrNone if successful, 
		  otherwise one of the other system wide error codes. 
  @see TForceMediaChangeFlags
  */
TInt DPBusPrimaryMedia::ForceMediaChange(TInt aFlags)
	{
	if ((aFlags != KMediaRemountForceMediaChange) || (iPBusState == EPBusCardAbsent))
		{
		TInt pbusState = iPBusState;
		
		// This should ensure NotifyMediaChange() is called for ALL primary media attached to this socket
		iSocket->ChangeState(EPBusCardAbsent, KErrNotReady);

		// If a request was cancelled it's possible that the socket controller has been left in an 
		// unusable state which might cause the next request to fail, so power down the socket to be safe
		iSocket->ResetSocket(EFalse);

		iSocket->ChangeState(pbusState == EPBusCardAbsent ? EPBusCardAbsent : EPBusOff, KErrNotReady);

		return KErrCompletion;
		}
	
	iSocket->ForceMediaChange();
	return KErrNone;
	}

/**
  Called by clients to power up the PBUS.
  @return KErrNone if successful,
  		  otherwise one of the other system wide error codes.
  @see  DPBusSocket::PowerUp()
  */
TInt DPBusPrimaryMedia::InitiatePowerUp()
	{
	return iSocket->PowerUp();
	}

/**
  Flags the media driver as entering a critical part of its processing.
  @return KErrNone if successful,
  		  otherwise one of the other system wide error codes.
  @see DPBusSocket::InCritical()
  */
TInt DPBusPrimaryMedia::DoInCritical()
	{
	return iSocket->InCritical();
	}

/**
  Flags the media driver as leaving a critical part of its processing.
  @return KErrNone if successful, 
  		  otherwise one of the other system wide error codes.
  @see DPBusSocket::EndInCritical()
  */
void DPBusPrimaryMedia::DoEndInCritical()
	{
	iSocket->EndInCritical();
	}

/**
  Sets the incremental value of current consumption to aCurrent.
  @param aCurrent Delta Current in Milliamps.
  @see DPBusSocket::DeltaCurrentConsumption()
  */
void DPBusPrimaryMedia::DeltaCurrentConsumption(TInt aCurrent)
	{
	iSocket->DeltaCurrentConsumption(aCurrent);
	}

/**
  Gets the default drive capability/attributes.
  @param aCaps	A reference to a client-supplied TLocalDriveCapsV2 class to be filled by this function.
  @see TLocalDriveCapsV2
  @see TMediaType
  */
void DPBusPrimaryMedia::DefaultDriveCaps(TLocalDriveCapsV2& aCaps)
	{
	// aCaps is zeroed beforehand
	aCaps.iType = EMediaNotPresent;
	aCaps.iDriveAtt = KDriveAttLocal|KDriveAttRemovable;
	}

/**
  Checks whether it is a removable media device or not.
  @param aSocketNum	This will be updated with socket number
  @return ETrue if Removable Device, EFalse if the device is Non-Removable.
  */
TBool DPBusPrimaryMedia::IsRemovableDevice(TInt& aSocketNum)
	{
	aSocketNum=iSocket->iSocketNumber;
	return(ETrue);
	}
	
void DPBusPrimaryMedia::PBusStateChange(TInt aState, TInt anError)
	{
	// receive power down and media change notifications
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPBusPrimaryMedia(%d)::PBusStateChange state %d, err %d",iMediaId,aState,anError));
	if (aState!=iPBusState)
		{
		TInt oldState = iPBusState;
		iPBusState=aState;
		switch (aState)
			{
			case EPBusCardAbsent:
				NotifyMediaChange();
				break;
			case EPBusOff:
				switch (anError)
					{
					case KErrNone:
						// machine power down
						NotifyPowerDown();
						break;
					case KErrTimedOut:					
						// machine power down
						NotifyPowerDown();
						if(oldState == EPBusCardAbsent)
							{
							// powering down after power up with no card present.
							// ...to prevent the bus powering up again, maintain
							//    the card state as absent.  A media change will
							//    update the status to allow the bus to power up.
							iPBusState = EPBusCardAbsent;
							}
						break;
					case KErrNotReady:
						// card detected following door close
						NotifyMediaPresent();
						break;
					case KErrAbort:
						NotifyEmergencyPowerDown();
						break;
					default:
						if (iState==EPoweringUp1 || iState==EPoweringUp2)
							PowerUpComplete(anError);
						break;
					}
			case EPBusPoweringUp:
				// no action required
				break;
			case EPBusOn:
				// bus is now powered up
				if (iState==EPoweringUp1 || iState==EPoweringUp2)
					PowerUpComplete(anError);
				break;
			case EPBusPsuFault:
				NotifyPsuFault(anError);
				break;
			case EPBusPowerUpPending:
				// no action required
				break;
			default:
				break;
			}
		}
	}

