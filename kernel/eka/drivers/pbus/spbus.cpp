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
// e32\drivers\pbus\spbus.cpp
// 
//

#include <drivers/pbus.h>

const TInt KPBusSocketThreadPriority=26;

GLDEF_D DMediaChangeBase* TheMediaChanges[KMaxMediaChanges];
GLDEF_D DPBusSocket* TheSockets[KMaxPBusSockets];
GLDEF_D DPBusPsuBase* TheVccs[KMaxPBusVccs];
GLDEF_D DPBusPsuBase* TheVccCores[KMaxPBusVccs]; 

/********************************************
 * Peripheral bus callback
 ********************************************/
EXPORT_C TPBusCallBack::TPBusCallBack()
	:	iSocket(NULL), iFunction(NULL), iIntMask(0), iPtr(NULL)
	{
	iNext=NULL;
	}

EXPORT_C TPBusCallBack::TPBusCallBack(TPBusCallBackFn aFunction, TAny* aPtr)
	: iSocket(NULL), iFunction(aFunction), iIntMask(0), iPtr(aPtr)
	{
	iNext=NULL;
	}

EXPORT_C TPBusCallBack::TPBusCallBack(TPBusIsr anIsr, TAny* aPtr, TUint anIntMask)
	: iSocket(NULL), iFunction(NULL), iIntMask(anIntMask), iIsr(anIsr), iPtr(aPtr)
	{
	iNext=NULL;
	}

EXPORT_C void TPBusCallBack::Remove()
	{
	TInt irq=NKern::DisableAllInterrupts();
	if (iNext)
		Deque();
	iNext=NULL;
	NKern::RestoreInterrupts(irq);
	}

EXPORT_C void TPBusCallBack::SetSocket(TInt aSocket)
	{
	iSocket=TheSockets[aSocket];
	}

/********************************************
 * Media change base class
 ********************************************/
 
/**
 * Constructor for a DMediaChangeBase object.
 *
 * @param aMediaChangeNum The media change number
 */
EXPORT_C DMediaChangeBase::DMediaChangeBase(TInt aMediaChangeNum)
	:	iMediaChangeNum(aMediaChangeNum),
		iReplyCount(0),
		iDoorOpenDfc(DoorOpenDfcFn,this,Kern::DfcQue1(),1)
	{
	}


/**
 * Creates a DMediaChangeBase object.
 * This should be overridden at the media and variant layer to allow
 * interrupts and other media/variant-specific parameters to be initialised.
 *
 * Method should be called post object creation, although could be used to
 * re-initialise parameters.
 *
 * @return KErrNone Default
 */
EXPORT_C TInt DMediaChangeBase::Create()
	{
	return KErrNone;
	}

/**
 * Called from ISR triggered by media change or from 
 * the Peripheral Bus Controller Media Driver context
 * if a media change is being forced.
 *
 * Method adds/enques a media change event on to the door 
 * open DFC queue. If called by PBUS thread then DFC queue 
 * is by-passed and change event is dealt with synchronously.
 *
 * Media change events are platform specific although are 
 * generally related to a media door or slot being opened.
 */
EXPORT_C void DMediaChangeBase::DoorOpenService()
	{
	if (NKern::CurrentContext()==NKern::EInterrupt)
		iDoorOpenDfc.Add();
	else 
		{
		if (Kern::DfcQue1()->iThread==(NThreadBase *)NKern::CurrentThread()) 	// check if this is being called from PBUS thread
			MediaChangeEvent(ETrue);
		else
			iDoorOpenDfc.Enque();
		}
	}


/**
 * High priority DFC triggered by media change interrupt.
 * 
 * Media changes events are added/enqued by DMediaChangeBase::DoorOpenService().
 * 
 * @param aPtr Pointer to an instantiated class which enqued/added this DFC event
 */
void DMediaChangeBase::DoorOpenDfcFn(TAny* aPtr)
	{
	DMediaChangeBase* pM=(DMediaChangeBase*)aPtr;
	pM->MediaChangeEvent(ETrue);
	}

/**
 *
 * Notifies sockets of door close event.
 *
 * This function must be called by variant when door close has been detected.
 */
EXPORT_C void DMediaChangeBase::DoorClosedService()
	{
	MediaChangeEvent(EFalse);
	}

/**
 * Notifies relevant peripheral bus sockets of door open or close events.
 *
 * @param aDoorOpened  ETrue if door is opened
 *
 * @see DPBusSocket::DPBusSocket
 */
void DMediaChangeBase::MediaChangeEvent(TBool aDoorOpened)
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DMediaChangeBase(%d)::MediaChangeEvent(%d)",iMediaChangeNum,aDoorOpened));
	TInt i;

	// notify all sockets affected
	for (i=0; i<KMaxPBusSockets; i++)
		{
		DPBusSocket* pS=TheSockets[i];
		if (pS && pS->iMediaChange==this)
			{
			// Only increment base reply count if actually adding a DFC
			if (!pS->iMediaChangeDfc.Queued())
			    __e32_atomic_add_ord32(&iReplyCount, 1);
			pS->MediaChangeEvent(aDoorOpened);
			}
		}
	}

/**
 * To be called by peripheral bus socket derived classes when
 * door open/close event has been processed.
 *
 * @param aDoorOpened   ETrue door opened event processed,
 *                      EFalse door closed event processed
 *
 * @see DPBusSocket::DoorOpenEvent()
 * @see DPBusSocket::DoorCloseEvent()
 */
void DMediaChangeBase::AcknowledgeEvent(TBool aDoorOpened)
	{
	TInt c = __e32_atomic_tas_ord32(&iReplyCount, 1, -1, 0);
	if (c==1)
		{
		if (aDoorOpened)
			DoDoorOpen();
		else
			DoDoorClosed();
		}
	}

/********************************************
 * Power supply base class
 ********************************************/
void psuTick(TAny* aPtr)
	{
	DPBusPsuBase* pP=(DPBusPsuBase*)aPtr;
	pP->iPsuDfc.Enque();
	}
	
void psuDfc(TAny* aPtr)
	{
	DPBusPsuBase* pP=(DPBusPsuBase*)aPtr;
	pP->DoTickService();
	}

/**
Constructor for a DPBusPsuBase object.

@param aPsuNum Unique power supply identification number
@param aMediaChangedNum Unique media change identification number
*/
DPBusPsuBase::DPBusPsuBase(TInt aPsuNum, TInt aMediaChangeNum)
	: iPsuNum(aPsuNum), iMediaChangeNum(aMediaChangeNum), iVoltCheckMethod(EPsuChkComparator), iState(EPsuOff),
	iPsuDfc(psuDfc, this, 4),
	iPwrDownCheckFn(DoPwrDownCheck)
	{
//	iCurrLimited=EFalse;
//	iVoltageSupported=0;
//	iMaxCurrentInMicroAmps=0;
//	iVoltCheckInterval=0;
//	iInactivityCount=0;
//	iNotLockedCount=0;
//	iInactivityTimeout=0;
//	iNotLockedTimeout=0;
	}

void DPBusPsuBase::DoPwrDownCheck(TAny* aPtr)
	{
	DPBusPsuBase& self = *static_cast<DPBusPsuBase*>(aPtr);
	self.PwrDownCheck();
	}

/**
Initialises a DPBusPsuBase object.

Sets object information based on hardware variant PSU inforamtion.
Calls DoCreate to initialise the PSU.

@return Standard Symbian OS error code.

@see DPBusPsuBase::PsuInfo()
@see DPBusPsuBase::DoCreate()
*/
TInt DPBusPsuBase::Create()
	{

	TPBusPsuInfo pi;
	PsuInfo(pi);
	iVoltageSupported=pi.iVoltageSupported;
	iMaxCurrentInMicroAmps=pi.iMaxCurrentInMicroAmps;
	iVoltCheckInterval=pi.iVoltCheckInterval;
	iVoltCheckMethod=pi.iVoltCheckMethod;
	iInactivityTimeout=pi.iInactivityTimeOut;
	iNotLockedTimeout=pi.iNotLockedTimeOut;

	TInt r=DoCreate();
	if (r!=KErrNone)
		return r;
	
	iPsuDfc.SetDfcQ(&iSocket->iDfcQ);
	
	return KErrNone;
	}


/**
Initialises the power supply unit.

The function is provided by the hardware variant layer, and needs to initialise 
interrupts and other variant-specific parameters.

The default implementation returns KErrNone.

@return KErrNone
*/
EXPORT_C TInt DPBusPsuBase::DoCreate()
	{
	return KErrNone;
	}


/**
Reset (turn off) the power supply unit.
Sets PSU state to EPsuOff.
*/
void DPBusPsuBase::Reset()
	{
	SetState(EPsuOff);
	iCurrLimited=EFalse;
	}


/**
Checks whether this PSU is powering a bus containing
a locked device, i.e. one that is recognised and in use by a client.

The function is provided at the media layer, could be used to ensure power is not
removed whilst media is locked or some other media specific power management activatity.

The default implementation just returns EFalse.

@return EFalse
*/
EXPORT_C TBool DPBusPsuBase::IsLocked()
	{
	return EFalse;
	}

/**
Controls the power supply state.

@param aState A TPBusPsuState enumeration specifying the required state
			 (EPsuOnFull, EPsuOff, EPsuOnCurLimit)

@return KErrNone if successful, otherwise one of the other system wide error codes.

@see TPBusPsuState
@see DPBusPsuBase::DoSetState()
*/
EXPORT_C TInt DPBusPsuBase::SetState(TPBusPsuState aState)
	{

	TInt r=KErrGeneral;
	if (aState==EPsuOff)
		{
		iTickLink.Cancel(); // No point in having the 1 second tick running while the PSU is off
		}
	else
		{
		// Start the 1 second tick to monitor for inactivity, not in use and PSU level checking
		iTickLink.Cancel();
		iTickLink.Periodic(KPBusPsuTickInterval,psuTick,this);
		}

	// Don't turn the PSU back on if it has current limited since the last reset event 
	iInactivityCount=0;
	iNotLockedCount=0;
	if (aState==EPsuOff || !iCurrLimited)
		{
		DoSetState(aState);
		iState=aState;
		r=KErrNone;
		}
	__KTRACE_OPT(KPBUS2,Kern::Printf("<Psu(%d):Set(%d)-%d",iPsuNum,aState,r));
	return r;
	}


/**
Check the voltage level of the power supply unit is as expected.
This method is called every PSU tick.

@param aCheckStatus Power check status in which voltage check can be performed (e.g. KPsuChkOnPwrUp).

@return KErrNone Voltage checking has been performed.
        KErrNotSupported Voltage checking is not supported by the hardware variant.
        
@see KPsuChkOnPwrUp       
@see KPsuChkWhileOn
@see DPBusPsuBase::DoTickService()
@see DPBusPsuBase::DoCheckVoltage()
*/
TInt DPBusPsuBase::CheckVoltage(TUint aCheckStatus)
	{
	// Check that voltage checking is in order at this time
	if (
		(aCheckStatus&iVoltCheckInterval) &&
		((aCheckStatus&KPsuChkOnPwrUp) || ((aCheckStatus&KPsuChkWhileOn)&&iState==EPsuOnFull))
	   )
		{
		DoCheckVoltage();
		return KErrNone;
		}
	return KErrNotSupported;
	}


/**
Reports the result of the voltage check.

The function is called by the variant implementation of DoCheckVoltage() 
to report the result.

Reporting a result of KErrGeneral (to indicate a failure) will result in a 
call to DPBusSocket::PsuFault(), otherwise report KErrNone to indicate a pass
and KErrNotReady if the voltage check was not completed.

@param anError System wide error code

@see DPBusPsuBase::DoCheckVoltage()
@see DPBusSocket::PsuFault()
*/
EXPORT_C void DPBusPsuBase::ReceiveVoltageCheckResult(TInt anError)
	{
//	__KTRACE_OPT(KPBUS1,Kern::Printf("DPBusPsuBase(%d)::ReceiveVoltageCheckResult(%d)",iPsuNum,anError));
	if (anError==KErrGeneral)
		{
		SetCurrLimited();
		iSocket->PsuFault(KErrCorrupt);
		}
	}

/**
Get the current power supply unit status

@return PSU status.

@see TPBusPsuStatus
*/
TPBusPsuStatus DPBusPsuBase::Status()
	{
	if (iCurrLimited)
		return(EPsuStatError);
	else
		return( (iState==EPsuOff) ? EPsuStatOff : EPsuStatOn );
	}


/**
Checks if power supply unit can be turned off.

@see DPBusPsuBase::DoTickService()
*/
void DPBusPsuBase::PwrDownCheck()
	{

	if (
		(iNotLockedTimeout&&!IsLocked()&&++iNotLockedCount>iNotLockedTimeout) ||
		(iInactivityTimeout&&++iInactivityCount>iInactivityTimeout)
	   )
			iSocket->PsuTimeout();
	}
	
	
/**
Services the Pc Card Tick (called in timer thread).
*/
EXPORT_C void DPBusPsuBase::DoTickService()
	{
	if (iPwrDownCheckFn)
		(*iPwrDownCheckFn)(this);	
	CheckVoltage(KPsuChkWhileOn);	// Check voltage level
	}


/********************************************
 * Peripheral bus power handler
 ********************************************/
DPBusPowerHandler::DPBusPowerHandler(DPBusSocket* aSocket)
	:	DPowerHandler(*aSocket->iName),
		iSocket(aSocket)
	{
	}

void DPBusPowerHandler::PowerUp()
	{
	iSocket->iPowerUpDfc.Enque();
	}

void DPBusPowerHandler::PowerDown(TPowerState)
	{
	iSocket->iPowerDownDfc.Enque();
	}

/********************************************
 * Peripheral bus socket base class
 ********************************************/
void mediaChangeDfc(TAny* aPtr)
	{
	DPBusSocket* pS=(DPBusSocket*)aPtr;
	if (pS->iDoorOpened)
		pS->DoorOpenEvent();
	else
		pS->DoorCloseEvent();
	}

void powerUpDfc(TAny* aPtr)
	{
	DPBusSocket* pS=(DPBusSocket*)aPtr;
	pS->DoPowerUp();
	}

void powerDownDfc(TAny* aPtr)
	{
	DPBusSocket* pS=(DPBusSocket*)aPtr;
	pS->DoPowerDown();
	}

	/**
    PBus Socket panics. Faults the system. 
	This will start the Crash Debugger if it is present, otherwise the system is rebooted by calling Kern::Restart(0)
	@param aPanic	The panic to be raised
	@see DPBusSocket::TPanic
	*/
EXPORT_C void DPBusSocket::Panic(DPBusSocket::TPanic aPanic)
	{
	Kern::Fault("PBUS",aPanic);
	}

	/**
    Flags the media driver as entering a critical part of its processing.
    In this context, critical means that the driver must be allowed to complete its current activity.
	
	@return	KErrNone if successful,
			KErrNotReady if there is any postponed events outstanding.
	@see DPBusSocket::EndInCritical()
	*/
EXPORT_C TInt DPBusSocket::InCritical()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPBusSocket(%d)::InCritical",iSocketNumber));
	if (iPostponeCount==0 && iPostponedEvents!=0)
		return KErrNotReady;	// we are about to do media change/power down
	++iPostponeCount;
	return KErrNone;
	}

	/**
    Flags the media driver as leaving a critical part of its processing.
	This function enque the media change DFC or power down DFC depending on the event DPBusSocket::iPostponedEvents.
	@see TPostponedEvent
	@see DPBusSocket::InCritical()
	*/
EXPORT_C void DPBusSocket::EndInCritical()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPBusSocket(%d)::EndInCritical",iSocketNumber));
	if (iPostponeCount && --iPostponeCount==0)
		{
		if (iPostponedEvents & EMediaChange)
			{
			iMediaChangeDfc.Enque();
			__KTRACE_OPT(KPBUS1,Kern::Printf("Media change - done postponed"));
			}
		if (iPostponedEvents & EPowerDown)
			{
			iPowerDownDfc.Enque();
			__KTRACE_OPT2(KPBUS1,KPOWER,Kern::Printf("Power down - done postponed"));
			}
		}
	}

	/**
	Sets the incremental value of current consumption to aCurrent.
    @param	aCurrent	Delta Current in Milliamps 
	@see DPowerHandler::DeltaCurrentConsumption()
	*/
EXPORT_C void DPBusSocket::DeltaCurrentConsumption(TInt aDelta)
	{
	iPowerHandler->DeltaCurrentConsumption(aDelta);
	}

	/**
	Constructor for DPBusSocket.
	Sets the iSocketNumber and initializes the DFC queue for Media Change Dfc, PowerUp Dfc, PowerDown Dfc and PSU Dfc queue.
    @param	aSocketNumber Pbus socket number
	*/
DPBusSocket::DPBusSocket(TInt aSocketNumber)
	:	iSocketNumber(aSocketNumber),
		iMediaChangeDfc(mediaChangeDfc, this, 6),
		iPowerUpDfc(powerUpDfc, this, 4),
		iPowerDownDfc(powerDownDfc, this, 4),
		iPsuDfc(psuDfc, this, 4)
	{
//	iPowerGroup=0;
//	iName=NULL;
//	iState=EPBusCardAbsent;
//	iPostponeCount=0;
//	iPostponedEvents=0;
//	iPowerHandler=NULL;
	}

	/**
	Creates a new Socket.
	This method sets the DFC Queue for the driver associated,
    Constructs power handler and registers it with the Power Manager.
	@param  aName	Assigns aName to the PBus socket.
	@return KErrNone	if successful, otherwise one of the other system wide error codes.
	@see DPBusPowerHandler
	@see iMediaChangeDfc
	@see iPowerUpDfc
	@see iPowerDownDfc
	@see iPsuDfc
	*/
TInt DPBusSocket::Create(const TDesC* aName)
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPBusSocket(%d)::Create %lS",iSocketNumber,aName));
	iName=aName;
	DPBusPowerHandler* pH=new DPBusPowerHandler(this);
	if (!pH)
		return KErrNoMemory;
	iPowerHandler=pH;
	pH->Add();		// register power handler
	TInt r=Kern::DfcQInit(&iDfcQ, KPBusSocketThreadPriority, iName);
	if (r!=KErrNone)
		return r;
	iMediaChangeDfc.SetDfcQ(&iDfcQ);
	iPowerUpDfc.SetDfcQ(&iDfcQ);
	iPowerDownDfc.SetDfcQ(&iDfcQ);
	
	return KErrNone;
	}

	/**
	Initializes the PBus socket by changing its state to EPBusOff.
	@return	KErrNone	if successful,
			otherwise one of the other system wide error codes.
	*/
TInt DPBusSocket::Init()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPBusSocket(%d)::Init",iSocketNumber));
	__PM_ASSERT(iState == EPBusCardAbsent);
	if (MediaState()==EDoorClosed && CardIsPresent())
		ChangeState(EPBusOff,KErrNotReady);
	return KErrNone;
	}

void DPBusSocket::ResetSocket(TBool aFullReset)
	{
	Reset1();
	iVcc->Reset();
	if (aFullReset)
		Reset2();
	}

void DPBusSocket::ChangeState(TInt aState, TInt anError)
//
// Change state, notifying all clients
//
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d ChangeState %d to %d, err %d",iSocketNumber,iState,aState,anError));
	if (iState!=aState)
		{
		if(iState == EPBusCardAbsent && aState == EPBusOff && anError == KErrTimedOut)
			{
			// Maintain the internal state to EPBusCardAbsent when PSU
			// times out to prevent the media from being powered back up.
			}
		else
			{
			iState=aState;
			}

		// notify all clients of state change
		SDblQueLink* pC=iCallBackQ.iA.iNext;
		while (pC && pC!=&iCallBackQ.iA)
			{
			((TPBusCallBack*)pC)->NotifyPBusStateChange(aState,anError);
			pC=pC->iNext;
			}
		}
	}

void DPBusSocket::Isr(TInt anId)
//
// Service a card interrupt
//
	{
	// notify all interested clients of interrupt
	SDblQueLink* pC=iCallBackQ.iA.iNext;
#ifdef _DEBUG
	TInt n=0;
#endif
	while (pC!=&iCallBackQ.iA)
		{
#ifdef _DEBUG
		n++;
#endif
		((TPBusCallBack*)pC)->Isr(anId);
		pC=pC->iNext;
		}
#ifdef _DEBUG
	__KTRACE_OPT(KPBUS1,Kern::Printf("!%d",n));
#endif
	}

	/**
	This function adds a callback function to the socket.
	@param aCallBack is a pointer to PBus callback function for event notification.
	@see TPBusCallBack
	*/
EXPORT_C void DPBusSocket::Add(TPBusCallBack* aCallBack)
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("DPBusSocket(%d)::Add(%08x) next %08x",iSocketNumber,aCallBack,aCallBack->iNext));
	TInt irq=NKern::DisableAllInterrupts();
	if (!aCallBack->iNext)
		iCallBackQ.Add(aCallBack);
	NKern::RestoreInterrupts(irq);
	}

	/**
	Called by clients to power up the socket.
	@return	KErrNone	if successful, otherwise one of the other system-wide error codes including: 
				KErrNotReady if card absent or media change has occurred,
				KErrServerBusy if already powering up,
				KErrCompletion if already powered up,
				KErrCorrupt if PSU fault occurs.

    @panic PBUS 1, if PBUS state is invalid.
	@see TPBusState
	*/
EXPORT_C TInt DPBusSocket::PowerUp()
	{
	__KTRACE_OPT2(KPBUS1,KPOWER,Kern::Printf(">DPBusSocket(%d)::PowerUp state %d",iSocketNumber,iState));
	TInt r=KErrNone;
	switch (iState)
		{
		case EPBusCardAbsent:		// card absent or media change has occurred
			r=KErrNotReady;
			break;
		case EPBusOff:
			break;
		case EPBusPoweringUp:		// already powering up
		case EPBusPowerUpPending:
			r=KErrServerBusy;
			break;
		case EPBusOn:				// already powered up
			r=KErrCompletion;
			break;
		case EPBusPsuFault:
			r=KErrCorrupt;
			break;
		default:
			Panic(EPowerUpInvalidState);
		}
	if (r==KErrNone)
		{
		if (iStandby)
			{
			// machine is powering down, so delay client until machine powers back up
			// remember to power up when machine powers back up
			ChangeState(EPBusPowerUpPending,KErrNone);
			}
		else
			{
			ChangeState(EPBusPoweringUp,KErrNone);
			InitiatePowerUpSequence();
			}
		}
	__KTRACE_OPT2(KPBUS1,KPOWER,Kern::Printf("<DPBusSocket(%d)::PowerUp ret %d, state %d",iSocketNumber,r,iState));
	return r;
	}
	/**
	This function is called upon completion of the power up sequence of the device.
	This is method is called by the derived class methods to terminate the powerup sequence with error codes.

	@param anError	One of the system wide error codes.
	@see DPBusSocket::InitiatePowerUpSequence()
	*/

EXPORT_C void DPBusSocket::PowerUpSequenceComplete(TInt anError)
	{
	__KTRACE_OPT2(KPBUS1,KPOWER,Kern::Printf("DPBusSocket(%d)::PowerUpSequenceComplete state %d error %d",iSocketNumber,iState,anError));
	if (iState!=EPBusCardAbsent && iState!=EPBusOff)
		{
		if (anError==KErrNone)
			ChangeState(EPBusOn,KErrNone);
		else if (anError==KErrBadPower || anError==KErrAbort || anError==KErrTimedOut)
			ChangeState(EPBusOff,anError);
		else if (anError == KErrNotReady)
			ChangeState(EPBusCardAbsent,KErrAbort);
		else
			ChangeState(EPBusPsuFault,anError);
		}
	}

void DPBusSocket::PsuFault(TInt anError)
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPBusSocket(%d)::PsuFault state %d error %d",iSocketNumber,iState,anError));
	ResetSocket(ETrue);
	ChangeState(EPBusPsuFault,anError);
	}

void DPBusSocket::PsuTimeout()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPBusSocket(%d)::PsuTimeout state %d",iSocketNumber,iState));
	ResetSocket(EFalse);
	ChangeState(EPBusOff,KErrTimedOut);
	}

void DPBusSocket::DoPowerUp()
//
// Called on transition from standby
//
	{
	__KTRACE_OPT2(KPBUS1,KPOWER,Kern::Printf("DPBusSocket(%d)::DoPowerUp state %d",iSocketNumber,iState));
	__PM_ASSERT(iStandby);
	if (iState!=EPBusCardAbsent && iState!=EPBusOff && iState!=EPBusPowerUpPending)
		Panic(EMcPowerUpInvalidState);

	// when we power up, check whether the door is closed and a card is present
	// if so we should start in state Off otherwise in state CardAbsent

	TMediaState doorState = MediaState();
	TBool cardIsPresent = CardIsPresent();

#ifdef __ENABLE_SIMULATED_MEDIA_CHANGE
	// Override the default media state is we are simulating media change
	if(iSimulatedMediaState != DPBusSocket::EPeriphBusMediaNormal)
		{
		doorState     = (iSimulatedMediaState == DPBusSocket::EPeriphBusDoorOpen) ? EDoorOpen : EDoorClosed;
		cardIsPresent = (iSimulatedMediaState == DPBusSocket::EPeriphBusMediaPresent);
		}
#endif

	if (!(doorState==EDoorClosed && cardIsPresent))
		ChangeState(EPBusCardAbsent,KErrNotReady);
	else if (iState==EPBusPowerUpPending)
		{
		// if a power-up request is pending, process it now
		ChangeState(EPBusPoweringUp,KErrNone);
		InitiatePowerUpSequence();
		}
	else
		ChangeState(EPBusOff,KErrNotReady);
	iStandby = EFalse;
	iPowerHandler->PowerUpDone();
	}

void DPBusSocket::DoPowerDown()
//
// Called by DPowerManager on transition to standby
//
	{
	__KTRACE_OPT2(KPBUS1,KPOWER,Kern::Printf("DPBusSocket(%d)::DoPowerDown state %d",iSocketNumber,iState));
	__PM_ASSERT(!iStandby);
	if (iPostponeCount)
		{
		iPostponedEvents |= EPowerDown;
		__KTRACE_OPT(KPBUS1,Kern::Printf("Power down postponed"));
		return;
		}
	iPostponedEvents &= ~EPowerDown;
	switch (iState)
		{
		case EPBusPoweringUp:
		case EPBusOn:
		case EPBusPsuFault:
			ChangeState(EPBusOff,KErrNone);
		case EPBusCardAbsent:
		case EPBusOff:
		case EPBusPowerUpPending:
			break;
		default:
			Panic(EEmergencyPowerDownInvalidState);
		}
		
	if(iRequestPowerDownCount == 0)
		{
		ResetSocket(EFalse);
		iStandby = ETrue;
		iPowerHandler->PowerDownDone();
		}
	}

	/**
	Notifies the socket that we are deferring this power down event. 
	The function increments the iRequestPowerDownCount reference count
	@see DPBusSocket::PowerDownComplete()
	*/
EXPORT_C void DPBusSocket::RequestAsyncPowerDown()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("DPBusSocket::RequestAsyncPowerDown"));	
    __e32_atomic_add_ord32(&iRequestPowerDownCount, 1);
	__KTRACE_OPT(KPBUS1,Kern::Printf("   >> count=%d", iRequestPowerDownCount));
	}

	/**
	This function power down the PBus. Powers down the PBus if iRequestPowerDownCount is equal to 1.
	@see DPBusSocket::RequestAsyncPowerDown()
	@see iRequestPowerDownCount
	*/
EXPORT_C void DPBusSocket::PowerDownComplete()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("DPBusSocket::PowerDownComplete"));
	if (__e32_atomic_tas_ord32(&iRequestPowerDownCount, 1, -1, 0) == 1)
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("   > Signalling Power Down (deferred)"));
		DoPowerDown();
		}
	__KTRACE_OPT(KPBUS1,Kern::Printf("   >> count=%d", iRequestPowerDownCount));
	}
	
	/**
	This function is called by the local media device driver to force a remount of the media device.
	@see DMediaChangeBase::ForceMediaChange()
	*/
EXPORT_C void DPBusSocket::ForceMediaChange()
	{
	iMediaChange->ForceMediaChange();
	}

void DPBusSocket::MediaChangeEvent(TBool aDoorOpened)
//
// Called in high-priority DFC
//
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPBusSocket(%d)::MediaChangeEvent %d state %d",iSocketNumber,aDoorOpened,iState));
	iDoorOpened=aDoorOpened;
	iMediaChangeDfc.Enque();
	}

void DPBusSocket::DoorOpenEvent()
//
// Called in socket thread
//
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPBusSocket(%d)::DoorOpenEvent state %d",iSocketNumber,iState));

	if (iPostponeCount)
		{
		iPostponedEvents |= EMediaChange;
		__KTRACE_OPT(KPBUS1,Kern::Printf("Media change postponed"));
		return;
		}
	iPostponedEvents &= ~EMediaChange;

    // notify all clients of media change
	ChangeState(EPBusCardAbsent,KErrNotReady);
	
	// power down the socket
	ResetSocket(ETrue);

	// get the media state befor calling AcknowledgeEvent() as the PSL may start a debounce 
	// timer on this call and return EDoorOpen while the timer is active....
	TMediaState mediaState = MediaState();

#ifdef __ENABLE_SIMULATED_MEDIA_CHANGE
	// Only acknowledge the media change to the PSL if we are running in normal mode
	if(iSimulatedMediaState == EPeriphBusMediaNormal)
		iMediaChange->AcknowledgeEvent(ETrue);
#else
	iMediaChange->AcknowledgeEvent(ETrue);
#endif

	// If there are multiple doors, then it is assumed that :
	// - DMediaChangeBase::MediaState() will return EDoorClosed if ANY door is closed, and 
	// - DPBusSocket::CardIsPresent() will return ETrue if ANY card is present
	// so that if, for example,  one door is  open and one door closed, then the bus will 
	// power down and then up again when one of the cards is next accessed.
	// NB This doesn't worrk for a simulated media change since this doesn't affect the 
	// PSL's door state
#ifdef __ENABLE_SIMULATED_MEDIA_CHANGE
	if ((iSimulatedMediaState == EPeriphBusMediaNormal) &&
		(mediaState == EDoorClosed && CardIsPresent()))
#else
	if (mediaState == EDoorClosed && CardIsPresent())
#endif
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("At least 1 door still closed"));;
		ChangeState(EPBusOff,KErrNotReady);
		}

	}

void DPBusSocket::DoorCloseEvent()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPBusSocket(%d)::DoorCloseEvent state %d",iSocketNumber,iState));

	// NB If there are multiple doors then the bus may already be powererd up, 
	// so it's not possible to determine the bus state.
	//if (iState!=EPBusCardAbsent)
	//	Panic(EDoorCloseInvalidState);

	// door has been closed - check for a card

	TBool cardIsPresent = CardIsPresent();

#ifdef __ENABLE_SIMULATED_MEDIA_CHANGE
	// Override the default drive state if we are simulating the media state
	if((iSimulatedMediaState == EPeriphBusDoorOpen) || (iSimulatedMediaState == EPeriphBusMediaRemoved))
		cardIsPresent = EFalse;
#endif

	
	if (cardIsPresent)
		{
		if (iState == EPBusCardAbsent)
			{
			// Notifies clients of a media change
			ChangeState(EPBusOff,KErrNotReady);
			}
		else	// if there's already a card present (iState != EPBusCardAbsent), power the bus off and on
			{
			// Notify clients of a media change, cancel any outstanding requests, close media driver(s)
			// and set the DPrimaryMediaBase's state to EClosed to force a subsequent power-up
			ChangeState(EPBusCardAbsent,KErrNotReady);
			ChangeState(EPBusOff,KErrNotReady);
			// NB Don't power down the socket when iState == EPBusCardAbsent as this can take a small amount of time 
			// and will cause DPBusPrimaryMedia::QuickCheckStatus() to return KErrNotReady in the meantime: this will 
			// result in any requests to the DPrimaryMediaBase being completed IMMEDIATELY with KErrNotReady, i.e. the
			// requests won't be queued until the power up completes.
			ResetSocket(ETrue);
			}
		}

#ifdef __ENABLE_SIMULATED_MEDIA_CHANGE
	// Only acknowledge the media change to the PSL if we are running in normal mode
	if(iSimulatedMediaState == EPeriphBusMediaNormal)
		iMediaChange->AcknowledgeEvent(EFalse);
#else
	iMediaChange->AcknowledgeEvent(EFalse);
#endif
	}
	/**
	Gets pointer to the PBus Socket corresponding to the opened logical unit.
	@param anId	logical id of the PBus Socket.	
	@return Pointer to the PBusSocket for valid anId, else NULL.
	*/
EXPORT_C DPBusSocket* DPBusSocket::SocketFromId(TInt anId)
	{
	if (anId>=0 && anId<KMaxPBusSockets)
		return TheSockets[anId];
	return NULL;
	}

	/**
	Default implementation for handling debug functionality.
	This function can only be used if __ENABLE_SIMULATED_MEDIA_CHANGE is defined. 
	Otherwise, this method is not implemented and it always returns KErrNotSupported.

	@param aFunction	refer to TPBusDebugFunction
	@param aParam1	Simulated media state.
	@param aParam2 Not used in this method.
	@return  KErrNone - if successful, otherwise one of the other system-wide error 
	codes including: 
			KErrNotSupported - if aFunction is invalid or __ENABLE_SIMULATED_MEDIA_CHANGE 
	is not defined,
			KErrArgument -  if aParam1 does not corresponds to TPBusSimulateMediaState.
	@see TPBusSimulateMediaState
	@see TPBusDebugFunction
	*/
EXPORT_C TInt DPBusSocket::ControlIO(TInt aFunction, TAny* aParam1, TAny* /*aParam2*/)
	{
	TInt err = KErrNone;

	switch(aFunction)
		{
		case EControlMediaState:
			//
			// EOverrideMediaState - Set the media state manually for simulation purposes.
			//	- aParam1 : Simulated Media State (TPBusSimulateMediaState)
			//
			{
#ifdef __ENABLE_SIMULATED_MEDIA_CHANGE
			TUint16 newState = (TUint16)(TInt)aParam1;
			if(newState != iSimulatedMediaState)
				{
				iSimulatedMediaState = newState;

				switch(iSimulatedMediaState)
					{
					case EPeriphBusMediaNormal:
						//
						// Normal state
						// - Signal that the door is open and generate a media change.
						//
						iMediaChange->MediaChangeEvent(ETrue);
						break;
					
					case EPeriphBusDoorOpen:
						//
						// Simulated door open or back to normal state.
						// - Signal that the door is open and generate a media change.
						//
						MediaChangeEvent(ETrue);
						break;
					
					case EPeriphBusMediaRemoved:
					case EPeriphBusMediaPresent:
						//
						// Simulated door close with media present or absent
						// - Signal that the door is closed.
						//
						MediaChangeEvent(EFalse);
						break;

					case EPeriphBusMediaDoubleDoorOpen:
						// simulate 2 door open interrupts 
						iSimulatedMediaState = EPeriphBusMediaNormal;
						iMediaChange->MediaChangeEvent(ETrue);
						iMediaChange->MediaChangeEvent(ETrue);
						break;

					default:
						//
						// Unsupported media state
						//
						err = KErrArgument;
						break;
					}
				}
#else
			aParam1 = aParam1;
			err = KErrNotSupported;
#endif
			break;
			}

		default:
			err = KErrNotSupported;
			break;
		}

	return err;
	}

/********************************************
 * Extension entry point
 ********************************************/

GLDEF_C TInt KernelModuleEntry(TInt aReason)
	{
	if (aReason==KModuleEntryReasonExtensionInit0 || aReason==KModuleEntryReasonExtensionInit1)
		return KErrNone;
	return KErrArgument;
	}





