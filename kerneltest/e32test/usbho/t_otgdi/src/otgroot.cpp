// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// @internalComponent
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32Test.h>	// RTest header
#include <e32def.h>
#include <e32def_private.h>
#include <d32otgdi.h>		// OTGDI header
#include <d32usbc.h>		// USBCC header
#include "otgroot.h"
#include "testcaseroot.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "otgrootTraces.h"
#endif

RUsbOtgDriver  oUsbOtgDriver;
RDevUsbcClient oUsbcClient;


//=====================================================================================
	
/*	this class wraps all OTGDI calls as well to simplify 
	 calling and normalising object access. - it has no base-class hence we can use multiple-inheritance
	 */	
	COtgRoot::COtgRoot()
	{
	iOptActive = EFalse;
	SetLoaded(EFalse);	
	}
	
	
/** otgLoadLdd
*/
TInt COtgRoot::otgLoadLdd()
	{
	
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGLOADLDD);
	    }
	LOG_VERBOSE2(_L("Load driver: %S\n"), &KOTGDeviceInterfaceDriverName);
	if(gVerboseOutput)
	    {
	    OstTraceExt1(TRACE_VERBOSE, COTGROOT_OTGLOADLDD_DUP01, "Load driver: %S\n", KOTGDeviceInterfaceDriverName);
	    }

	if (!LddLoaded())
		{

	 	// load ldd device drivers (load otg only, it will load the needed stuff for us)
		TInt err(User::LoadLogicalDevice(KOTGDeviceInterfaceDriverName));
		if ( (err != KErrNone) && (err != KErrAlreadyExists) )
			{
			test.Printf(_L("<Error %d> Unable to load driver: %S\n"), err, &KOTGDeviceInterfaceDriverName);
			OstTraceExt2(TRACE_NORMAL, COTGROOT_OTGLOADLDD_DUP02, "<Error %d> Unable to load driver: %S\n", err, KOTGDeviceInterfaceDriverName);
			SetLoaded(EFalse);
			return(err);
			}
		else
			{
			LOG_VERBOSE2(_L("Loaded driver: '%S' OK\n"), &KOTGDeviceInterfaceDriverName);
			if(gVerboseOutput)
			    {
			    OstTraceExt1(TRACE_VERBOSE, COTGROOT_OTGLOADLDD_DUP03, "Loaded driver: '%S' OK\n", KOTGDeviceInterfaceDriverName);
			    }
			SetLoaded(ETrue);
			}
		
		}
		return(KErrNone);
	}


/** otgOpen
*/
TInt COtgRoot::otgOpen()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGOPEN);
	    }
	
	LOG_VERBOSE2(_L("Opening session... loaded = %d\n"), LddLoaded());
	if(gVerboseOutput)
	    {
	    OstTrace1(TRACE_VERBOSE, COTGROOT_OTGOPEN_DUP01, "Opening session... loaded = %d\n", LddLoaded());
	    }

	TInt err(oUsbOtgDriver.Open());
	if (err != KErrNone)
		{
		test.Printf(_L("<Error %d> Unable to open a channel to USB OTG driver\n"),err);
		OstTrace1(TRACE_NORMAL, COTGROOT_OTGOPEN_DUP02, "<Error %d> Unable to open a channel to USB OTG driver\n",err);
		return(err);
		}
	else
		{
		LOG_VERBOSE1(_L("Open channel OK\n"));
		if(gVerboseOutput)
		    {
		    OstTrace0(TRACE_VERBOSE, COTGROOT_OTGOPEN_DUP03, "Open channel OK\n");
		    }
		}
		
	return(KErrNone);	
	}


/** otgClose
*/
void COtgRoot::otgClose()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGCLOSE);
	    }
	
	test.Printf(_L("Closing session... loaded = %d\n"), LddLoaded());
	OstTrace1(TRACE_NORMAL, COTGROOT_OTGCLOSE_DUP01, "Closing session... loaded = %d\n", LddLoaded());
	oUsbOtgDriver.Close();
	}


/* otgActivateOptTestMode
 */ 
TInt COtgRoot::otgActivateOptTestMode()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGACTIVATEOPTTESTMODE);
	    }

	TInt err = oUsbOtgDriver.ActivateOptTestMode();

	return(err);
	}


/** otgStartStacks
*/
TInt COtgRoot::otgStartStacks()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGSTARTSTACKS);
	    }

	TInt err(oUsbOtgDriver.StartStacks());
	if (err != KErrNone)
		{

		OstTrace1(TRACE_WARNING, COTGROOT_OTGSTARTSTACKS_DUP01, "[WARNING failed %d]", err);

		}
	return(err);
	
	}


/** otgStopStacks
*/ 
void COtgRoot::otgStopStacks()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGSTOPSTACKS);
	    }
	
	oUsbOtgDriver.StopStacks();
	}


/** otgUnloadLdd
*/
void COtgRoot::otgUnloadLdd()
	{

	// code to unload the OTG ldd driver
	TInt err (User::FreeLogicalDevice(KOTGDeviceInterfaceDriverName));
	if (err != KErrNone)
		{
		OstTrace1(TRACE_WARNING, COTGROOT_OTGUNLOADLDD, "[WARNING failed %d]", err);
		}

	SetLoaded(EFalse);
	}


/** otgQueueOtgEventRequest
*/
void COtgRoot::otgQueueOtgEventRequest(RUsbOtgDriver::TOtgEvent& aEvent, TRequestStatus &aStatus)
	{
	LOG_VERBOSE2(_L("Queue an Event Request %08X.\n"), (TInt)(&aStatus));
	if(gVerboseOutput)
	    {
	    OstTrace1(TRACE_VERBOSE, COTGROOT_OTGQUEUEOTGEVENTREQUEST_DUP01, "Queue an Event Request %08X.\n", (TInt)(&aStatus));
	    }

	oUsbOtgDriver.QueueOtgEventRequest(aEvent, aStatus);
	
	}
	

/** otgCancelOtgEventRequest
*/	
void COtgRoot::otgCancelOtgEventRequest()
	{
	LOG_VERBOSE1(_L("Cancel Event Request.\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_OTGCANCELOTGEVENTREQUEST, "Cancel Event Request.\n");
	    }
	oUsbOtgDriver.CancelOtgEventRequest();
	}
    
    
/** otgQueueOtgMessageRequest
*/
void COtgRoot::otgQueueOtgMessageRequest(RUsbOtgDriver::TOtgMessage& aMessage, TRequestStatus &aStatus)
	{

	LOG_VERBOSE2(_L("Queue a Message Request %08X.\n"), (TInt)(&aStatus));
	if(gVerboseOutput)
	    {
	    OstTrace1(TRACE_VERBOSE, COTGROOT_OTGQUEUEOTGMESSAGEREQUEST_DUP01, "Queue a Message Request %08X.\n", (TInt)(&aStatus));
	    }

	oUsbOtgDriver.QueueOtgMessageRequest(aMessage, aStatus);
		
	}
	

/** otgCancelOtgMessageRequest
*/	
void COtgRoot::otgCancelOtgMessageRequest()
	{
	LOG_VERBOSE1(_L("Cancel Message Request.\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_OTGCANCELOTGMESSAGEREQUEST, "Cancel Message Request.\n");
	    }
	oUsbOtgDriver.CancelOtgMessageRequest();
	}    
    
void COtgRoot::otgQueuePeripheralStateRequest(TUint& aPeripheralState, TRequestStatus& aStatus)
	{
	LOG_VERBOSE1(_L("Queue Peripheral State Request.\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_OTGQUEUEPERIPHERALSTATEREQUEST, "Queue Peripheral State Request.\n");
	    }
	oUsbcClient.AlternateDeviceStatusNotify(aStatus, aPeripheralState);
	}

void COtgRoot::otgCancelPeripheralStateRequest()
	{
	LOG_VERBOSE1(_L("Cancel Peripheral State Request.\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_OTGCANCELPERIPHERALSTATEREQUEST, "Cancel Peripheral State Request.\n");
	    }
	oUsbcClient.AlternateDeviceStatusNotifyCancel();	
	}

void COtgRoot::otgQueueAConnectionIdleRequest(RUsbOtgDriver::TOtgConnection& aAConnectionIdle, TRequestStatus& aStatus)
	{
	LOG_VERBOSE1(_L("Queue A Connection Idle Request.\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_OTGQUEUEACONNECTIONIDLEREQUEST, "Queue A Connection Idle Request.\n");
	    }
	oUsbOtgDriver.QueueOtgConnectionNotification(aAConnectionIdle, aStatus);
	}

void COtgRoot::otgCancelAConnectionIdleRequest()
	{
	LOG_VERBOSE1(_L("Cancel A Connection Idle Request.\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_OTGCANCELACONNECTIONIDLEREQUEST, "Cancel A Connection Idle Request.\n");
	    }
	oUsbOtgDriver.CancelOtgConnectionNotification();
	}


/** otgQueueOtgStateRequest
*/
void COtgRoot::otgQueueOtgStateRequest(RUsbOtgDriver::TOtgState& aState, TRequestStatus &aStatus)
	{
	LOG_VERBOSE2(_L("Queue a State Request %08X.\n"), (TInt)(&aStatus));
	if(gVerboseOutput)
	    {
	    OstTrace1(TRACE_VERBOSE, COTGROOT_OTGQUEUEOTGSTATEREQUEST_DUP01, "Queue a State Request %08X.\n", (TInt)(&aStatus));
	    }

	oUsbOtgDriver.QueueOtgStateRequest(aState, aStatus);
	
	}
	

/** otgCancelOtgStateRequest
*/	
void COtgRoot::otgCancelOtgStateRequest()
	{
	LOG_VERBOSE1(_L("Cancel State Request.\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_OTGCANCELOTGSTATEREQUEST, "Cancel State Request.\n");
	    }
	oUsbOtgDriver.CancelOtgStateRequest();
	}

    
/** otgBusRequest
raise VBus (in reality this must only happen when a 'A' is present... and when not present it starts HNP)
*/
TInt COtgRoot::otgBusRequest()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGBUSREQUEST);
	    }
	
	TInt err(0);
	err = oUsbOtgDriver.BusRequest();
	if (err != KErrNone)
		{
		OstTrace1(TRACE_WARNING, COTGROOT_OTGBUSREQUEST_DUP01, "[WARNING failed %d]", err);
		}
	return(err);
	}
	

/* call when SRP has been recieved, based on our HNP Enable setting, this will allow HNP. The 
 * A-device may choose to call BusRequest directly, which will result in a 'short-circuit' role-swap.
 */
TInt COtgRoot::otgBusRespondSRP()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGBUSRESPONDSRP);
	    }
	TInt err(0);
		err = oUsbOtgDriver.BusRespondSrp();
		if (err != KErrNone)
			{
			OstTrace1(TRACE_WARNING, COTGROOT_OTGBUSRESPONDSRP_DUP01, "[WARNING failed %d]", err);
			}
		return(err);
	}

	
/** Drop VBus (A-host)
*/	
TInt COtgRoot::otgBusDrop()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGBUSDROP);
	    }
	TInt err(0);
	err = oUsbOtgDriver.BusDrop();
	if (err != KErrNone)
		{
		OstTrace1(TRACE_WARNING, COTGROOT_OTGBUSDROP_DUP01, "[WARNING failed %d]", err);
		}
	return(err);
	}

/** otgBusClearError
*/
TInt COtgRoot::otgBusClearError()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGBUSCLEARERROR);
	    }
	
	TInt err(0);
	err = oUsbOtgDriver.BusClearError();
	if (err != KErrNone)
		{
		OstTrace1(TRACE_WARNING, COTGROOT_OTGBUSCLEARERROR_DUP01, "[WARNING failed %d]", err);
		}
	return(err);
	}
	

	
void COtgRoot::otgQueueOtgIdPinNotification(RUsbOtgDriver::TOtgIdPin& aPin, TRequestStatus& aStatus)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGQUEUEOTGIDPINNOTIFICATION);
	    }
	oUsbOtgDriver.QueueOtgIdPinNotification(aPin, aStatus);	// the kernel driver populates aPin...
	}

	
void COtgRoot::otgCancelOtgIdPinNotification()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGCANCELOTGIDPINNOTIFICATION);
	    }
	oUsbOtgDriver.CancelOtgIdPinNotification();
	}


void COtgRoot::otgQueueOtgVbusNotification(RUsbOtgDriver::TOtgVbus& aVbus, 
                                                TRequestStatus& aStatus
                                               )
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGQUEUEOTGVBUSNOTIFICATION);
	    }
	oUsbOtgDriver.QueueOtgVbusNotification(aVbus, aStatus);
	}
	
	
void COtgRoot::otgCancelOtgVbusNotification()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGCANCELOTGVBUSNOTIFICATION);
	    }
	oUsbOtgDriver.CancelOtgVbusNotification();
	}


TBool COtgRoot::otgIdPinPresent()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGIDPINPRESENT);
	    }
	TRequestStatus aStatus;
	RUsbOtgDriver::TOtgIdPin aPin;
	oUsbOtgDriver.QueueOtgIdPinNotification(aPin, aStatus);	// the kernel driver populates aPin...
	LOG_VERBOSE2(_L("(sync) ID_PIN=%d\n"), iOTGIdPin);
	if(gVerboseOutput)
	    {
	    OstTrace1(TRACE_VERBOSE, COTGROOT_OTGIDPINPRESENT_DUP01, "(sync) ID_PIN=%d\n", iOTGIdPin);
	    }
	
	oUsbOtgDriver.CancelOtgIdPinNotification();
	// swallow the event
	User::WaitForRequest(aStatus);
	
	if (RUsbOtgDriver::EIdPinAPlug == aPin)			// at this stage, the aPin value is known
		{
		return(ETrue);
		}
	return(EFalse);
	}
	
	
TBool COtgRoot::otgVbusPresent()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGROOT_OTGVBUSPRESENT);
	    }
	TRequestStatus aStatus;
 	RUsbOtgDriver::TOtgVbus aVBus;
	oUsbOtgDriver.QueueOtgVbusNotification(aVBus, aStatus);	// the kernel driver populates aPin in a kernel thread...
	oUsbOtgDriver.CancelOtgVbusNotification();
	// swallow the event
	User::WaitForRequest(aStatus);

	if (RUsbOtgDriver::EVbusHigh == aVBus)			// by this stage, the aVBus value is known
		{
		return(ETrue);
		}
	return(EFalse);
	}


TBool COtgRoot::iLoadedLdd = EFalse;
TBool COtgRoot::iFdfActorActive = EFalse;
RProcess COtgRoot::iFdfActorProcess;

/** static */
TBool& COtgRoot::LddLoaded()
	{ 
	return(iLoadedLdd);
	}


/** static */
TBool COtgRoot::SetLoaded(TBool aState) 
	{ 
	iLoadedLdd = aState; 
	return(LddLoaded());
	}

/** static */
void COtgRoot::OtgEventString( RUsbOtgDriver::TOtgEvent aEvent, TBuf<MAX_DSTRLEN> &aDescription)
	{

	switch( aEvent )
		{
		case RUsbOtgDriver::EEventAPlugInserted:		aDescription= _L("A Plug Inserted");break;
		case RUsbOtgDriver::EEventAPlugRemoved:			aDescription= _L("A Plug Removed");	break;
		case RUsbOtgDriver::EEventVbusRaised:			aDescription= _L("VBUS Raised");	break;
		case RUsbOtgDriver::EEventVbusDropped:			aDescription= _L("VBUS Dropped");	break;
		case RUsbOtgDriver::EEventSrpReceived:			aDescription= _L("SRP Received");	break;
		case RUsbOtgDriver::EEventSrpInitiated:			aDescription= _L("SRP Initiated");	break;
		case RUsbOtgDriver::EEventHnpEnabled:			aDescription= _L("HNP Enabled");	break;
		case RUsbOtgDriver::EEventHnpDisabled:			aDescription= _L("HNP Disabled");	break;
		case RUsbOtgDriver::EEventRoleChangedToHost:	aDescription= _L("Role->Host");		break;
		case RUsbOtgDriver::EEventRoleChangedToDevice:	aDescription= _L("Role->Device");	break;
		case RUsbOtgDriver::EEventRoleChangedToIdle:	aDescription= _L("Role->Idle");		break;
		
		case RUsbOtgDriver::EEventHnpSupported : 		aDescription= _L("HNP Supported");		break;
		case RUsbOtgDriver::EEventHnpAltSupported:		aDescription= _L("Alt-HNP Supp.");		break;
		case RUsbOtgDriver::EEventBusConnectionBusy:	aDescription= _L("Connection Busy");	break;
		case RUsbOtgDriver::EEventBusConnectionIdle:	aDescription= _L("Connection Idle");	break;
		default:										aDescription= _L("Unknown");		break;
		}
	}

/** static */
void COtgRoot::OtgStateString( RUsbOtgDriver::TOtgState aState, TBuf<MAX_DSTRLEN> &aDescription)
	{

	switch( aState )
		{
		case RUsbOtgDriver::EStateReset:		aDescription= _L("Reset");			break;
		case RUsbOtgDriver::EStateAIdle:		aDescription= _L("A-Idle");			break;
		case RUsbOtgDriver::EStateAHost:		aDescription= _L("A-Host");			break;
		case RUsbOtgDriver::EStateAPeripheral:	aDescription= _L("A-Peripheral");	break;
		case RUsbOtgDriver::EStateAVbusError:   aDescription= _L("A-VBus Error");	break;
		case RUsbOtgDriver::EStateBIdle:		aDescription= _L("B-Idle");			break;
		case RUsbOtgDriver::EStateBPeripheral:	aDescription= _L("B-Peripheral");	break;
		case RUsbOtgDriver::EStateBHost:		aDescription= _L("B-Host");			break;
		default:								aDescription= _L("Unknown");		break;
		}
	}

/** static */
void COtgRoot::OtgMessageString( RUsbOtgDriver::TOtgMessage aMessage, TBuf<MAX_DSTRLEN> &aDescription)
	{

	switch( aMessage )
		{
		case RUsbOtgDriver::EEventQueueOverflow:			aDescription = _L("Event Queue Overflow");		break;
		case RUsbOtgDriver::EStateQueueOverflow:			aDescription = _L("State Queue Overflow");		break;
		case RUsbOtgDriver::EMessageQueueOverflow:			aDescription = _L("Message Queue Overflow");	break;
		case RUsbOtgDriver::EMessageBadState:				aDescription = _L("Bad State");					break;
		case RUsbOtgDriver::EMessageStackNotStarted:		aDescription = _L("Stack Not Started");			break;
		case RUsbOtgDriver::EMessageVbusAlreadyRaised:		aDescription = _L("Vbus Already Raised");		break;
		case RUsbOtgDriver::EMessageSrpForbidden:			aDescription = _L("SRP Forbidden");				break;
		case RUsbOtgDriver::EMessageBusControlProblem:		aDescription = _L("Bus Control Problem");		break;
		case RUsbOtgDriver::EMessageVbusError:				aDescription = _L("Vbus Error");				break;
		case RUsbOtgDriver::EMessageSrpTimeout:				aDescription = _L("SRP Timeout");				break;
		case RUsbOtgDriver::EMessageSrpActive:				aDescription = _L("SRP In Use");				break;
		// PREQ 1305 messages
		case RUsbOtgDriver::EMessageSrpNotPermitted:		aDescription = _L("Srp Not Permitted"); break;
		case RUsbOtgDriver::EMessageHnpNotPermitted:		aDescription = _L("Hnp Not Permitted"); break;
		case RUsbOtgDriver::EMessageHnpNotEnabled:			aDescription = _L("Hnp Not Enabled"); break;
		case RUsbOtgDriver::EMessageHnpNotSuspended: 			aDescription = _L("HNP not possible, not suspended"); break;
		case RUsbOtgDriver::EMessageVbusPowerUpNotPermitted:	aDescription = _L("Vbus PowerUp Not Permitted"); break;
		case RUsbOtgDriver::EMessageVbusPowerUpError:			aDescription = _L("Vbus PowerUp Error"); break;
		case RUsbOtgDriver::EMessageVbusPowerDownNotPermitted:	aDescription = _L("Vbus PowerDown Not Permitted"); break;
		case RUsbOtgDriver::EMessageVbusClearErrorNotPermitted:	aDescription = _L("Vbus ClearError Not Permitted"); break;
		case RUsbOtgDriver::EMessageHnpNotResponding:		aDescription = _L("Not reposnding to HNP");
		case RUsbOtgDriver::EMessageHnpBusDrop:				aDescription = _L("VBUS drop during HNP");
		default:												aDescription = _L("Unknown");					break;
		}
	}

void COtgRoot::PeripheralStateString( TUint aPeripheralState, TBuf<MAX_DSTRLEN> &aDescription)
	{
	if(aPeripheralState & KUsbAlternateSetting)
		{
		aDescription = _L("Interface Alternate Setting Change");
		}
	else
		{
		switch( aPeripheralState )
			{
			case EUsbcDeviceStateUndefined:		aDescription = _L("Undefined");			break;
			case EUsbcDeviceStateAttached:		aDescription = _L("Attached");			break;
			case EUsbcDeviceStatePowered:		aDescription = _L("Powered");			break;
			case EUsbcDeviceStateDefault:		aDescription = _L("Default");			break;
			case EUsbcDeviceStateAddress:		aDescription = _L("Addressed");			break;
			case EUsbcDeviceStateConfigured:	aDescription = _L("Configured");		break;
			case EUsbcDeviceStateSuspended:		aDescription = _L("Suspended");			break;
			case EUsbcNoState:					aDescription = _L("NoState!");			break;
			default:							aDescription = _L("Unknown");			break;
			}
		}
	}
	
void COtgRoot::AConnectionIdleString(RUsbOtgDriver::TOtgConnection aAConnectionIdle, TBuf<MAX_DSTRLEN> &aDescription)
	{
	switch( aAConnectionIdle )
		{
		case RUsbOtgDriver::EConnectionBusy:		aDescription = _L("Busy");			break;
		case RUsbOtgDriver::EConnectionIdle:		aDescription = _L("Idle");			break;
		case RUsbOtgDriver::EConnectionUnknown:		aDescription = _L("Unknown");		break;
		default:									aDescription = _L("Not recognised");break;
		}
	}

TInt COtgRoot::otgActivateFdfActor()
	{
	if(iFdfActorActive)
		{
		OstTrace0(TRACE_NORMAL, COTGROOT_OTGACTIVATEFDFACTOR, "FdfActor already exists!");
		return KErrAlreadyExists;
		}
		
	const TUid KFdfSvrUid={0x10282B48};
	const TUidType fdfActorUid(KNullUid, KNullUid, KFdfSvrUid);

	OstTrace0(TRACE_NORMAL, COTGROOT_OTGACTIVATEFDFACTOR_DUP01, "About to activate FDF Actor");

//	RProcess fdfActorProcess;
	TInt err = iFdfActorProcess.Create(_L("t_otgdi_fdfactor.exe"), KNullDesC, fdfActorUid);
	
	if (err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, COTGROOT_OTGACTIVATEFDFACTOR_DUP02, "Failed to create FDF Actor, err=%d",err);
		iFdfActorProcess.Close();
		return err;
		}

	TRequestStatus stat;
	iFdfActorProcess.Rendezvous(stat);
	
	if (stat!=KRequestPending)
		{
		OstTrace1(TRACE_NORMAL, COTGROOT_OTGACTIVATEFDFACTOR_DUP03, "Failed to commence rendezvous, err=%d",stat.Int());
		iFdfActorProcess.Kill(0);		// abort startup
		iFdfActorProcess.Close();
		return stat.Int();
		}
	else
		{
		iFdfActorProcess.Resume();	// logon OK - start the server
		}

	User::WaitForRequest(stat);		// wait for start or death
	if(stat.Int()!=KErrNone)
		{
		//	Wasn't KErrNone, which means that the FDFActor didn't successfully
		//	start up. We shouldn't proceed with the test we're in.
		OstTrace1(TRACE_NORMAL, COTGROOT_OTGACTIVATEFDFACTOR_DUP04, "Failed to activate FDF Actor, err=%d",stat.Int());
		iFdfActorProcess.Close();
		return stat.Int();
		}
	
	//	We rendezvoused(?) with the FDFActor OK, so it is going to suspend
	//	any devices it sees being attached, and will shut itself down
	//	when this process signals its Rendezvous (at the end of the test)...
	OstTrace0(TRACE_NORMAL, COTGROOT_OTGACTIVATEFDFACTOR_DUP05, "Activated FDF Actor");
	iFdfActorActive = ETrue;

	return KErrNone;
	}

void COtgRoot::otgDeactivateFdfActor()
	{
	if(!iFdfActorActive)
		{
		OstTrace0(TRACE_NORMAL, COTGROOT_OTGDEACTIVATEFDFACTOR, "FdfActor is not running!");
		return;
		}

	//	If iFdfActorActive is set, the FDF Actor should be waiting to
	//	rendezvous with us before it shuts down...
	//	First of all, logon to wait for it to close down properly
	TRequestStatus waitForCloseStat;
	iFdfActorProcess.Logon(waitForCloseStat);
	
	//	Now, trigger the FDF Actor to close down

	RProcess::Rendezvous(KErrNone);
	
	//	...and wait for it to go away.
	User::WaitForRequest(waitForCloseStat);
	test.Printf(_L("T_OTGDI confirms FDF Actor has gone away %d\n"), waitForCloseStat.Int());
	OstTrace1(TRACE_NORMAL, COTGROOT_OTGDEACTIVATEFDFACTOR_DUP01, "T_OTGDI confirms FDF Actor has gone away %d\n", waitForCloseStat.Int());
	
	//	Now close our handle, and record that the process is no more...
	iFdfActorProcess.Close();
	iFdfActorActive = EFalse;
	}
	
/** Commonly used step to unload the USB Client Driver
* @return ETrue if the ldd unloaded sucessfully
*/
TBool COtgRoot::StepUnloadClient()
	{
	test.Printf(_L("Unload USBCC Client\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPUNLOADCLIENT, "Unload USBCC Client\n");

	TInt err;

	// Close the Client

	test.Printf(_L("..Close\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPUNLOADCLIENT_DUP01, "..Close\n");
	oUsbcClient.Close();

	// Unload the LDD - note the name is *not* the same as for loading

	test.Printf(_L("..Unload\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPUNLOADCLIENT_DUP02, "..Unload\n");
	err = User::FreeLogicalDevice( KUsbDeviceName );
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Client Unload Fail "), err);
		return (EFalse);
		}


	return(ETrue);
	}


/** Commonly used step to load the USB Client Driver and set up
 *	a 'useful' default device descriptor set
 * @return ETrue if the ldd loaded sucessfully
 */
TBool COtgRoot::StepLoadClient(TUint16 aPID, 
								TBool aEnableHNP/*=ETrue*/, 
								TBool aEnableSRP/*=ETrue*/)
	{
	test.Printf(_L("Load USBCC Client 0x%04x\n"),aPID);
	OstTrace1(TRACE_NORMAL, COTGROOT_STEPLOADCLIENT, "Load USBCC Client 0x%04x\n",aPID);

	TInt err;

	// The incoming PID is expected to have a form of 0x0TTT or 0xFTTT
	// where 'TTT' is the test number: if the lead is 0xF000 we now 
	// overlay an 'A' or a 'B' depending on the default role
	
	if (   ( ( aPID & 0xF000 ) != 0xF000 )
		&& ( ( aPID & 0xF000 ) != 0x0000 )
	   )
		{
		AssertionFailed(KErrAbort, _L("Bad default PID"));
		}
		
	if ( aPID & 0xF000 )
		{
		aPID &= 0x0FFF;
		
		if ( gTestRoleMaster )
			{
			// this is the 'B' device
			aPID |= 0xB000;
			}
		else
			{
			// this is the 'A' device
			aPID |= 0xA000;
			}
		}
	
	// Load the LDD - note the name is *not* the same as for unload

	test.Printf(_L("..Load LDD\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPLOADCLIENT_DUP01, "..Load LDD\n");
	err = User::LoadLogicalDevice( KUsbcLddFileName );
	if ((err != KErrNone) && (err !=KErrAlreadyExists))
		{
		AssertionFailed2(KErrAbort, _L("Client Load Fail "), err);
		return (EFalse);
		}

	// Open the Client

	test.Printf(_L("..Open LDD\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPLOADCLIENT_DUP02, "..Open LDD\n");
	err = oUsbcClient.Open(0);
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Client Open Fail "), err);
		return (EFalse);
		}

	// Set up descriptors
	
	test.Printf(_L("..Setup Descriptors\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPLOADCLIENT_DUP03, "..Setup Descriptors\n");

	// the OTG descriptor
	TBuf8<KUsbDescSize_Otg> theOtgDescriptor;
	err = oUsbcClient.GetOtgDescriptor(theOtgDescriptor);
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("OTG GetDes Fail "), err);
		return (EFalse);
		}
	// modify OTG descriptor based on parameters passed
	if (aEnableHNP)
		aEnableSRP=ETrue;	// Keep the device Legal according to OTG spec 6.4.2
	/*Attribute Fields
	  D7…2: Reserved (reset to zero)
	  D1: HNP support
	  D0: SRP support*/
	TUint8 aByte = theOtgDescriptor[2];
		aByte &= (~0x03);
		aByte |= (aEnableSRP? 1 : 0); 
		aByte |= (aEnableHNP? 2 : 0); 
	test.Printf(_L("..Change OTG 0x%02X->0x%02X\n"), theOtgDescriptor[2], aByte);
	OstTraceExt2(TRACE_NORMAL, COTGROOT_STEPLOADCLIENT_DUP04, "..Change OTG 0x%02X->0x%02X\n", (TUint32)theOtgDescriptor[2], (TUint32)aByte);
	theOtgDescriptor[2] = aByte;
	
	err = oUsbcClient.SetOtgDescriptor(theOtgDescriptor);
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("OTG SetDes Fail "), err);
		return (EFalse);
		}
	
	//

	TUsbDeviceCaps d_caps;
	err = oUsbcClient.DeviceCaps(d_caps);
	TBool softwareConnect;

	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Client DevCaps Fail "), err);
		return (EFalse);
		}

	const TInt n = d_caps().iTotalEndpoints;

	softwareConnect = d_caps().iConnect;
	test.Printf(_L("..SoftwareConnect = %d\n"),softwareConnect);
	OstTrace1(TRACE_NORMAL, COTGROOT_STEPLOADCLIENT_DUP05, "..SoftwareConnect = %d\n",softwareConnect);

	if (n < 2)
		{
		AssertionFailed2(KErrAbort, _L("Client Endpoints Fail "), err);
		return (EFalse);
		}

	// Endpoints
	TUsbcEndpointData data[KUsbcMaxEndpoints];
	TPtr8 dataptr(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
	err = oUsbcClient.EndpointCaps(dataptr);
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Client EpCaps Fail "), err);
		return (EFalse);
		}

	// Set up the active interface
	TUsbcInterfaceInfoBuf ifc;
	TInt ep_found = 0;
	TBool foundBulkIN = EFalse;
	TBool foundBulkOUT = EFalse;
	for (TInt i = 0; i < n; i++)
		{
		const TUsbcEndpointCaps* const caps = &data[i].iCaps;
		const TInt mps = caps->MaxPacketSize();
		if (!foundBulkIN &&
			(caps->iTypesAndDir & (KUsbEpTypeBulk | KUsbEpDirIn)) ==
			(KUsbEpTypeBulk | KUsbEpDirIn))
			{
			if (!(mps == 64 || mps == 512))
				{
				}
			// EEndpoint1 is going to be our Tx (IN) endpoint
			ifc().iEndpointData[0].iType = KUsbEpTypeBulk;
			ifc().iEndpointData[0].iDir	 = KUsbEpDirIn;
			ifc().iEndpointData[0].iSize = mps;
			foundBulkIN = ETrue;
			if (++ep_found == 2)
				break;
			}
		else if (!foundBulkOUT &&
			(caps->iTypesAndDir & (KUsbEpTypeBulk | KUsbEpDirOut)) ==
			(KUsbEpTypeBulk | KUsbEpDirOut))
			{
			if (!(mps == 64 || mps == 512))
				{
				}
			// EEndpoint2 is going to be our Rx (OUT) endpoint
			ifc().iEndpointData[1].iType = KUsbEpTypeBulk;
			ifc().iEndpointData[1].iDir	 = KUsbEpDirOut;
			ifc().iEndpointData[1].iSize = mps;
			foundBulkOUT = ETrue;
			if (++ep_found == 2)
				break;
			}
		}
	if (ep_found != 2)
		{
		AssertionFailed2(KErrAbort, _L("Client EpFound Fail "), err);
		return (EFalse);
		}

	_LIT16(ifcname, "T_OTGDI Test Interface (Default Setting 0)");
	ifc().iString = const_cast<TDesC16*>(&ifcname);
	ifc().iTotalEndpointsUsed = 2;
	ifc().iClass.iClassNum	  = 0xff;						// vendor-specific
	ifc().iClass.iSubClassNum = 0xff;						// vendor-specific
	ifc().iClass.iProtocolNum = 0xff;						// vendor-specific

	err = oUsbcClient.SetInterface(0, ifc, (EUsbcBandwidthOUTDefault | EUsbcBandwidthINDefault));

	if( err != KErrNone )
		{
		AssertionFailed2(KErrAbort, _L("Client SetInt Fail "), err);
		return (EFalse);
		}
	
	// Set the revised PID
	
	TBuf8<KUsbDescSize_Device> theDeviceDescriptor;
	err = oUsbcClient.GetDeviceDescriptor(theDeviceDescriptor);
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Client GetDes Fail "), err);
		return (EFalse);
		}

	TUint16 oldPID = ( theDeviceDescriptor[10] )
		           + ( theDeviceDescriptor[11] << 8 );

	theDeviceDescriptor[10] = ( aPID & 0x00FF );
	theDeviceDescriptor[11] = ( aPID & 0xFF00 ) >> 8;

	test.Printf(_L("..Change PID 0x%04X->0x%04X\n"), oldPID, aPID);
	OstTraceExt2(TRACE_NORMAL, COTGROOT_STEPLOADCLIENT_DUP06, "..Change PID 0x%04X->0x%04X\n", (TUint32)oldPID, (TUint32)aPID);

	err = oUsbcClient.SetDeviceDescriptor(theDeviceDescriptor);
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Client SetDes Fail "), err);
		return (EFalse);
		}


	// Power Up UDC - KErrNotReady is expected

	test.Printf(_L("..Power Up UDC\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPLOADCLIENT_DUP07, "..Power Up UDC\n");

	err = oUsbcClient.PowerUpUdc();
	if( err != KErrNotReady )
		{
		AssertionFailed2(KErrAbort, _L("PowerUp UDC Fail "), err);
		return (EFalse);
		}

	// Connect to Host

	test.Printf(_L("..Connect to Host\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPLOADCLIENT_DUP08, "..Connect to Host\n");

	err = oUsbcClient.DeviceConnectToHost();
	if( err != KErrNone )
		{
		AssertionFailed2(KErrAbort, _L("Host Connect Fail "), err);
		return (EFalse);
		}

	// Default no-problem return

	return(ETrue);
	}


/** Commonly used steps to control D+ connect/disconnect
 *  the device to/from the host
 */
TBool COtgRoot::StepDisconnect()
	{
	test.Printf(_L("Disconnect from Host\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPDISCONNECT, "Disconnect from Host\n");

	TInt err;
	
	err = oUsbcClient.DeviceDisconnectFromHost();
	if( err != KErrNone )
		{
		AssertionFailed2(KErrAbort, _L("Host Disconnect Fail "), err);
		return (EFalse);
		}

	// Default no-problem return

	return(ETrue);
	}

TBool COtgRoot::StepConnect()
	{
	test.Printf(_L("Connect to Host\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPCONNECT, "Connect to Host\n");

	TInt err;
	
	err = oUsbcClient.DeviceConnectToHost();
	if( err != KErrNone )
		{
		AssertionFailed2(KErrAbort, _L("Host Connect Fail "), err);
		return (EFalse);
		}

	// Default no-problem return

	return(ETrue);
	}


/** Commonly used step to load the USB Client Driver and set up
 *	a High-Speed electrical test VID/PID pair
 * @return ETrue if the ldd loaded sucessfully
 */
TBool COtgRoot::StepChangeVidPid(TUint16 aVID, TUint16 aPID)

	{
	test.Printf(_L("Load USBCC HS Test Client 0x%04x/0x%04x\n"),aVID,aPID);
	OstTraceExt2(TRACE_NORMAL, COTGROOT_STEPCHANGEVIDPID, "Load USBCC HS Test Client 0x%04x/0x%04x\n",(TUint32)aVID,(TUint32)aPID);

	TInt err;

	// Set the revised VID/PID pair
	
	TBuf8<KUsbDescSize_Device> theDeviceDescriptor;
	err = oUsbcClient.GetDeviceDescriptor(theDeviceDescriptor);
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Client GetDes Fail "), err);
		return (EFalse);
		}

	TUint16 oldVID = ( theDeviceDescriptor[8] )
		           + ( theDeviceDescriptor[9] << 8 );

	theDeviceDescriptor[8] = ( aVID & 0x00FF );
	theDeviceDescriptor[9] = ( aVID & 0xFF00 ) >> 8;

	test.Printf(_L("..Change VID 0x%04X->0x%04X\n"), oldVID, aVID);
	OstTraceExt2(TRACE_NORMAL, COTGROOT_STEPCHANGEVIDPID_DUP01, "..Change VID 0x%04X->0x%04X\n", (TUint32)oldVID, (TUint32)aVID);

	TUint16 oldPID = ( theDeviceDescriptor[10] )
		           + ( theDeviceDescriptor[11] << 8 );

	theDeviceDescriptor[10] = ( aPID & 0x00FF );
	theDeviceDescriptor[11] = ( aPID & 0xFF00 ) >> 8;

	test.Printf(_L("..Change PID 0x%04X->0x%04X\n"), oldPID, aPID);
	OstTraceExt2(TRACE_NORMAL, COTGROOT_STEPCHANGEVIDPID_DUP02, "..Change PID 0x%04X->0x%04X\n", (TUint32)oldPID, (TUint32)aPID);

	err = oUsbcClient.SetDeviceDescriptor(theDeviceDescriptor);
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Client SetDes Fail "), err);
		return (EFalse);
		}

	// Default no-problem return

	return(ETrue);
	}


/** Commonly used step to set a flag that will cause the
    OPT test mode to be activated when the stacks are
	started 
*/
void COtgRoot::StepSetOptActive() 
	{ 
	iOptActive = ETrue;
	}


/** Commonly used step to unload the ldd and shut it down
*@return ETrue if the ldd unloaded sucessfully
*/
TBool COtgRoot::StepUnloadLDD()
	{ 
	test.Printf(_L("Unload otg LDD (implicit Stop() + Close()) \n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPUNLOADLDD, "Unload otg LDD (implicit Stop(+ Close()) \n");
	
	LOG_VERBOSE1(_L("  Stop OTG+Host Stack\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_STEPUNLOADLDD_DUP01, "  Stop OTG+Host Stack\n");
	    }
	otgStopStacks();
	otgClose();
	
	LOG_VERBOSE1(_L("  Unload\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_STEPUNLOADLDD_DUP02, "  Unload\n");
	    }
	otgUnloadLdd();
	
	iOptActive = EFalse; // retain the OTGDI behavour to clears this flag when client shuts 
	if (LddLoaded())
		return(EFalse);
	return(ETrue);
	}
	

/** StepLoadLDD - utility method : load the LDD, open it and init the stacks
 a commonly used test step */
TBool COtgRoot::StepLoadLDD()
	{
	TInt err;	

	LOG_VERBOSE1(_L("Load otg LDD\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_STEPLOADLDD, "Load otg LDD\n");
	    }
	err = otgLoadLdd();
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Loading LDD failed "), err);
		return (EFalse);
		}
		
	LOG_VERBOSE1(_L("Open the LDD session\n"));
	if(gVerboseOutput)
	    {
	    OstTrace0(TRACE_VERBOSE, COTGROOT_STEPLOADLDD_DUP01, "Open the LDD session\n");
	    }
	err = otgOpen();
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Open LDD session failed "), err);
		return (EFalse);
		}

	if ( iOptActive )
		{
		test.Printf(_L("Activate OPT Test Mode\n"));
		OstTrace0(TRACE_NORMAL, COTGROOT_STEPLOADLDD_DUP02, "Activate OPT Test Mode\n");
		err = otgActivateOptTestMode();
		if (err != KErrNone)
			{
			AssertionFailed2(KErrAbort, _L("OPT Activate failed "), err);
			return (EFalse);
			}	
		}

	test.Printf(_L("Start OTG+Host Stack\n"));
	OstTrace0(TRACE_NORMAL, COTGROOT_STEPLOADLDD_DUP03, "Start OTG+Host Stack\n");
	err = otgStartStacks();
	if (err != KErrNone)
		{
		AssertionFailed2(KErrAbort, _L("Start stack failed "), err);
		return (EFalse);
		}	
		
	//	DS - Temporarily adding .1 second delay in here to beat the race
	//	condition to be fixed as part of third part USB HOST/OTG stack issue 60761
	User::After(100000);
		
	return(ETrue);
	}

void COtgRoot::SetMaxPowerToL(TUint16 aVal)
	{

	TBuf8<KUsbDescSize_Config> buf;
	
	oUsbcClient.GetConfigurationDescriptor(buf);
	
	aVal %= 500;
	
	buf[8] = (TUint8)(aVal / 2);
	
	oUsbcClient.SetConfigurationDescriptor(buf);
	
	}

void COtgRoot::GetMaxPower(TUint16& aVal)
	{
	TBuf8<KUsbDescSize_Config> buf;
	
	oUsbcClient.GetConfigurationDescriptor(buf);
	
	aVal = buf[8] * 2;
	}


