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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "../../include/drivers/locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "pbusmediaTraces.h"
#endif

void mediaCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2)
	{
	OstTraceFunctionEntry0( MEDIACALLBACK_ENTRY );
	DPBusPrimaryMedia* pM=(DPBusPrimaryMedia*)aPtr;
	__KTRACE_OPT(KLOCDRV,Kern::Printf("mediaCallBack media %d, reason %d, a1=0x%x, a2=0x%x",pM->iMediaId,aReason,a1,a2));
	OstTraceExt4( TRACE_INTERNALS, MEDIACALLBACK, "aPtr=0x%x; aReason=%d; a1=0x%x; a2=0x%x", ( TUint )( aPtr ), aReason, ( TUint )( a1 ), ( TUint )( a2 ) );
	
	switch (aReason)
		{
		case TPBusCallBack::EPBusStateChange:
			pM->PBusStateChange((TInt)a1,(TInt)a2);
			break;
		}
	OstTraceFunctionExit0( MEDIACALLBACK_EXIT );
	}

/**
  Constructor for DPBusPrimaryMedia. Initializes the iSocket with aSocket.
  @param aSocket	Pointer to DPBusSocket object
  @see DPBusPrimaryMedia::iSocket
  */

DPBusPrimaryMedia::DPBusPrimaryMedia(DPBusSocket* aSocket)
	:	iSocket(aSocket)
	{
	OstTraceFunctionEntryExt( DPBUSPRIMARYMEDIA_DPBUSPRIMARYMEDIA_ENTRY, this );
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
	OstTraceExt4(TRACE_FLOW, DPBUSPRIMARYMEDIA_CREATE_ENTRY ,"DPBusPrimaryMedia::Create;aDevice=%d;aMediaId=%d;aLastMediaId=%d;this=%x", (TInt) aDevice, aMediaId, aLastMediaId, (TUint) this);
	
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
	OstTraceFunctionExitExt( DPBUSPRIMARYMEDIA_CREATE_EXIT, this, r );
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
	OstTraceFunctionEntry1( DPBUSPRIMARYMEDIA_QUICKCHECKSTATUS_ENTRY, this );
	TInt r=KErrNone;
	if (iSocket && iSocket->State()==EPBusCardAbsent)
		r=KErrNotReady;
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPBusPrimaryMedia::QuickCheckStatus media %d returns %d",iMediaId,r));
	OstTraceExt2(TRACE_INTERNALS, DPBUSPRIMARYMEDIA_QUICKCHECKSTATUS, "iMediaId=%d; retval=%d",iMediaId,r);
	OstTraceFunctionExit1( DPBUSPRIMARYMEDIA_QUICKCHECKSTATUS_EXIT, this );
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
	OstTraceFunctionEntryExt( DPBUSPRIMARYMEDIA_FORCEMEDIACHANGE_ENTRY, this );
	if ((aFlags != KMediaRemountForceMediaChange) || (iPBusState == EPBusCardAbsent))
		{
		TInt pbusState = iPBusState;
		
		// This should ensure NotifyMediaChange() is called for ALL primary media attached to this socket
		iSocket->ChangeState(EPBusCardAbsent, KErrNotReady);

		// If a request was cancelled it's possible that the socket controller has been left in an 
		// unusable state which might cause the next request to fail, so power down the socket to be safe
		iSocket->ResetSocket(EFalse);

		iSocket->ChangeState(pbusState == EPBusCardAbsent ? EPBusCardAbsent : EPBusOff, KErrNotReady);

		OstTraceFunctionExitExt( DPBUSPRIMARYMEDIA_FORCEMEDIACHANGE_EXIT1, this, KErrCompletion );
		return KErrCompletion;
		}
	
	iSocket->ForceMediaChange();
	OstTraceFunctionExitExt( DPBUSPRIMARYMEDIA_FORCEMEDIACHANGE_EXIT2, this, KErrNone );
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
	OstTraceFunctionEntry1( DPBUSPRIMARYMEDIA_INITIATEPOWERUP_ENTRY, this );
	TInt r = iSocket->PowerUp();  
	OstTraceFunctionExitExt( DPBUSPRIMARYMEDIA_INITIATEPOWERUP_EXIT, this, r );
	return r;
	}

/**
  Flags the media driver as entering a critical part of its processing.
  @return KErrNone if successful,
  		  otherwise one of the other system wide error codes.
  @see DPBusSocket::InCritical()
  */
TInt DPBusPrimaryMedia::DoInCritical()
	{
	OstTraceFunctionEntry1( DPBUSPRIMARYMEDIA_DOINCRITICAL_ENTRY, this );
	TInt r = iSocket->InCritical(); 
	OstTraceFunctionExitExt( DPBUSPRIMARYMEDIA_DOINCRITICAL_EXIT, this, r );
	return r;
	}

/**
  Flags the media driver as leaving a critical part of its processing.
  @return KErrNone if successful, 
  		  otherwise one of the other system wide error codes.
  @see DPBusSocket::EndInCritical()
  */
void DPBusPrimaryMedia::DoEndInCritical()
	{
	OstTraceFunctionEntry1( DPBUSPRIMARYMEDIA_DOENDINCRITICAL_ENTRY, this );
	iSocket->EndInCritical();
	OstTraceFunctionExit1( DPBUSPRIMARYMEDIA_DOENDINCRITICAL_EXIT, this );
	}

/**
  Sets the incremental value of current consumption to aCurrent.
  @param aCurrent Delta Current in Milliamps.
  @see DPBusSocket::DeltaCurrentConsumption()
  */
void DPBusPrimaryMedia::DeltaCurrentConsumption(TInt aCurrent)
	{
	OstTraceFunctionEntryExt( DPBUSPRIMARYMEDIA_DELTACURRENTCONSUMPTION_ENTRY, this );
	iSocket->DeltaCurrentConsumption(aCurrent);
	OstTraceFunctionExit1( DPBUSPRIMARYMEDIA_DELTACURRENTCONSUMPTION_EXIT, this );
	}

/**
  Gets the default drive capability/attributes.
  @param aCaps	A reference to a client-supplied TLocalDriveCapsV2 class to be filled by this function.
  @see TLocalDriveCapsV2
  @see TMediaType
  */
void DPBusPrimaryMedia::DefaultDriveCaps(TLocalDriveCapsV2& aCaps)
	{
	OstTraceFunctionEntry1( DPBUSPRIMARYMEDIA_DEFAULTDRIVECAPS_ENTRY, this );
	// aCaps is zeroed beforehand
	aCaps.iType = EMediaNotPresent;
	aCaps.iDriveAtt = KDriveAttLocal|KDriveAttRemovable;
	OstTraceFunctionExit1( DPBUSPRIMARYMEDIA_DEFAULTDRIVECAPS_EXIT, this );
	}

/**
  Checks whether it is a removable media device or not.
  @param aSocketNum	This will be updated with socket number
  @return ETrue if Removable Device, EFalse if the device is Non-Removable.
  */
TBool DPBusPrimaryMedia::IsRemovableDevice(TInt& aSocketNum)
	{
	OstTraceFunctionEntryExt( DPBUSPRIMARYMEDIA_ISREMOVABLEDEVICE_ENTRY, this );
	aSocketNum=iSocket->iSocketNumber;
	OstTraceFunctionExit1( DPBUSPRIMARYMEDIA_ISREMOVABLEDEVICE_EXIT, this );
	return(ETrue);
	}
	
void DPBusPrimaryMedia::PBusStateChange(TInt aState, TInt anError)
	{
	OstTraceFunctionEntryExt( DPBUSPRIMARYMEDIA_PBUSSTATECHANGE_ENTRY, this );
	// receive power down and media change notifications
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPBusPrimaryMedia(%d)::PBusStateChange state %d, err %d",iMediaId,aState,anError));
	OstTraceExt3(TRACE_INTERNALS, DPBUSPRIMARYMEDIA_PBUSSTATECHANGE, "iMediaId=%d; aState=%d; anError=%d", iMediaId,aState,anError);
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
	OstTraceFunctionExit1( DPBUSPRIMARYMEDIA_PBUSSTATECHANGE_EXIT, this );
	}

