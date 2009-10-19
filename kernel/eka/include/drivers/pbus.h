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
// e32\include\drivers\pbus.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __P32STD_H__
#define __P32STD_H__
#include <kernel/kpower.h>

#ifdef _DEBUG
#define __ENABLE_SIMULATED_MEDIA_CHANGE	// Enables simulation of media change events using ControlIO
#endif

/********************************************
 * Peripheral Bus Definitions
 ********************************************/

/**
Defines the maximum number of media change objects.

@see DMediaChangeBase
*/
const TInt KMaxMediaChanges=4;



/**
Defines the maximum number of power supply unit (PSU) objects.

@see DPBusPsuBase
*/
const TInt KMaxPBusVccs=4;




/**
    Defines the state of the media, i.e. whether the media door is
    open or closed.
    
    A value of this type is returned by DMediaChangeBase::MediaState().
    
    @see DMediaChangeBase::MediaState()
*/
enum TMediaState {
                 /**
                 Indicates that the media door is open.
                 */
                 EDoorOpen,
                 
                 /**
                 Indicates that the media door is closed.
                 */
                 EDoorClosed};




enum TPBusType {EPBusTypeNone,EPBusTypePcCard,EPBusTypeMultiMedia,EPBusTypeUSB};




enum TPBusState
	{
	EPBusCardAbsent=0,
	EPBusOff=1,
	EPBusPoweringUp=2,
	EPBusOn=3,
	EPBusPsuFault=4,
	EPBusPowerUpPending=5,
	};

typedef void (*TPBusCallBackFn)(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2);
typedef void (*TPBusIsr)(TAny* aPtr, TInt anId);

class DPBusSocket;
class TPBusCallBack : public SDblQueLink
	{
public:
	enum TCallBackReason
		{
		EPBusStateChange=0,
		EPBusCustomNotification=1,	// Make this the last value
		};
public:
	IMPORT_C TPBusCallBack();
	IMPORT_C TPBusCallBack(TPBusCallBackFn aFunction, TAny* aPtr);
	IMPORT_C TPBusCallBack(TPBusIsr anIsr, TAny* aPtr, TUint anIntMask);
public:
	inline void NotifyPBusStateChange(TInt aState, TInt anError);
	inline void NotifyCustom(TInt aParam, TInt anError);
	inline void Isr(TInt anId);
public:
	inline TInt PowerUp();
	inline TInt PBusState();
	inline TDfcQue* DfcQ();
	inline void Add();
	IMPORT_C void SetSocket(TInt aSocket);
	IMPORT_C void Remove();
public:
	DPBusSocket* iSocket;
	TPBusCallBackFn iFunction;
	TUint iIntMask;
	TPBusIsr iIsr;
	TAny* iPtr;
	};



class DMediaChangeBase : public DBase
/**
Abstract Base class to handle the insertion and removal of removable media.

This class is intended for derivation at the media and variant layers, 
which handles the media/variant specific functionality such as interrupt detection, 
and passes notifications of media change to the peripheral bus socket layers.

@see DMMCMediaChange

@publishedPartner
@released
*/
	{
public:
	IMPORT_C DMediaChangeBase(TInt aMediaChangeNum);
	IMPORT_C virtual TInt Create();
public:
	IMPORT_C void DoorOpenService();
	IMPORT_C void DoorClosedService();
	void MediaChangeEvent(TBool aDoorOpened);
	void AcknowledgeEvent(TBool aDoorOpened);
	static void DoorOpenDfcFn(TAny* aPtr);
public:
    /**
	 * Implemented at the media and variant layers.
	 * Forces a media change, performs actions as if a door open has occurred.
	 * Used for powering down and restarting media.
	 *
	 * @see DMediaChangeBase::DoDoorOpen()
	 */
	IMPORT_C virtual void ForceMediaChange()=0;
	
	/**
	 * Called by DMediaChangeBase::AcknowledgeEvent when the door is opened.
	 * Implemented at the media and variant layer, DoDoorOpen is invoked 
	 * in response to the variant calling ::DoDoorOpenService upon detection of 
	 * a door open event. 
	 *
	 * @see DMediaChangeBase::DoorOpenService()
	 * @see DMediaChangeBase::DoDoorClosed()
	 */
	IMPORT_C virtual void DoDoorOpen()=0;
	
	/**
	 * Called by DMediaChangeBase::AcknowledgeEvent when the door is closed.
	 * Implemented at the media and variant layer, DoDoorClosed is invoked 
	 * in response to the variant calling ::DoorOpenService upon detection of 
	 * a door closed event. 
	 *
	 * @see DMediaChangeBase::DoorOpenService()
	 * @see DMediaChangeBase::DoDoorOpen()
	 */
	IMPORT_C virtual void DoDoorClosed()=0;
	
	/**
	 * Returns the current state of the door.
	 * Implemented at the variant layer to provide information as to the state of the door.
	 *
	 * @return TMediaState enumeration describing the state of door (EDoorOpen, EDoorClosed)
	 */	
	IMPORT_C virtual TMediaState MediaState()=0;
public:
    /** Unique media change ID, identification scheme is defined by derived classes*/
	TInt iMediaChangeNum;
	
	/** Count of media events yet to be acknowledged.*/
	TInt iReplyCount;
	
	/** Door Open DFC queue.*/
	TDfc iDoorOpenDfc;
	};




/**
    Defines the required state of the PSU.
    
    A value of this type is passed to DPBusPsuBase::DoSetState().
    
    @see DPBusPsuBase::DoSetState()
*/
enum TPBusPsuState {
                   /**
                   Indicates that the PSU is to be turned off.
                   */
                   EPsuOff,
                   
                   /**
                   Indicates that the PSU is to be turned on in current
                   limit mode.
                   
                   Some power supplies can be turned on in a mode that only
                   supplies a limited amount of current to the device.
                   A faulty device drawing excessive current would cause
                   the PSU output voltage to fall, and would be detected
                   by the PSU's voltage checking mechanism. After a brief
                   checking period in current limit mode, the PSU is
                   subsequently turned fully on.
                   
                   For PSU's that don't support current limit mode, this is
                   treated in the same way as EPsuOnFull.
                   */
                   EPsuOnCurLimit,
                   
                   /**
                   Indicates that the PSU is to be turned fully on.
                   */
                   EPsuOnFull
                   };




enum TPBusPsuStatus {EPsuStatOff,EPsuStatOn,EPsuStatError};




const TInt KPBusPsuTickInterval=1000000;		// Units 1uS




/**
    A bit value that is set into the TPBusPsuInfo::iVoltCheckInterval member to
    indicate that the checking of the PSU voltage level can be performed when
    the power supply is first turned on.
*/
const TUint KPsuChkWhileOn=0x00000001;	




/**
    A bit value that is set into the TPBusPsuInfo::iVoltCheckInterval member to
    indicate that the checking of the PSU voltage level can be performed
    periodically when the power supply is on.
*/
const TUint KPsuChkOnPwrUp=0x00000002;




/**
    Defines the methods that can be used by the platform hardware for checking
	the VCC voltage while on.
*/
enum TPsuVoltChkMethod {
                       /**
                       Indicates a simple good/bad check method normally using
                       a comparator arrangement.
                       */
                       EPsuChkComparator,
                       
                       /**
                       Not implemented.
                       */
                       EPsuChkADCType1,
                       
                       /**
                       Not implemented.
                       */
                       EPsuChkADCType2
                       };




/**
    Encapsulates power supply information.
	
	An object of this type is passed to concrete implementations of
	DPBusPsuBase::PsuInfo(), which are required to fill
	the data members of the object. 
*/
class TPBusPsuInfo
	{
public:
    /**
    Indicates the voltage level, or range of voltages supported .
    
    The voltage level, or range of voltages is reported as a bit mask
    in the same format as defined for the OCR register.
    
    If the PSU supports voltage adjustment, rather than a single fixed value,
    then bit31 should be set to indicate this fact
    as well as setting the appropriate bits to indicate the adjustable voltage
    range supported.
    */
	TUint iVoltageSupported;
	
	/**
	The maximum current (in microAmps) that the PSU is able to supply.
	*/
	TInt iMaxCurrentInMicroAmps;
	
	/**
	Indicates whether the platform hardware has support for checking whether
	the voltage level of the PSU is within its expected voltage limit while turned on.
	
	Such a mechanism could detect when a faulty card is drawing excessive current.
	
	Set one or both of the bits KPsuChkWhileOn and KPsuChkOnPwrUp,
	if there is support for voltage checking.
	Set neither bit if there is no support for voltage checking.
	
	@see KPsuChkWhileOn
	@see KPsuChkOnPwrUp
	*/
	TUint iVoltCheckInterval;
	
	/**
	Indicates the method used by the platform hardware for checking
	the VCC voltage while on.
	
	The method used is identified using the values of
	the TPsuVoltChkMethod enum.
	
	@see TPsuVoltChkMethod
	@see DPBusPsuBase::DoCheckVoltage()
	*/
	TPsuVoltChkMethod iVoltCheckMethod;
	
	/**
	Bus not locked timeout period, in seconds, when no clients are registered.
	
    Set to 0 to disable the not locked timer.
	*/
    TInt iNotLockedTimeOut;
    
    /**
    Bus inactivity timeout period, in seconds, when clients are registered.
    
    Set to 0 to disable the inactivity timer.
    */
    TInt iInactivityTimeOut;
	};


typedef void (*PsuPwrDownCheckFn)(TAny*);
/**
Perherpial bus base class to control the power supply.

This class is intended for derivation at both the media driver and variant layer, 
which respectively handle the media/variant specific functionality of the power supply.

@see DMMCPsu

@publishedPartner
@released
*/
class DPBusPsuBase : public DBase
	{
public:
    /** Currently not in use. */
	enum TResetMode {
	                    /** Currently not in use.*/
	                    ENormalAndRestorable,
	                    /** Currently not in use.*/
	                    EQuickButRestorable,
	                    /** Currently not in use.*/
	                    EQuickAndNotRestorable
	                };
public:
	DPBusPsuBase(TInt aPsuNum, TInt aMediaChangeNum);
	IMPORT_C TInt SetState(TPBusPsuState aState);
	TPBusPsuStatus Status();
	TInt CheckVoltage(TUint aCheckStatus);
	void Reset();
public:
	TInt Create();
	IMPORT_C virtual TInt DoCreate();
	IMPORT_C virtual void DoTickService();
	IMPORT_C virtual TBool IsLocked();
public:
	IMPORT_C virtual void DoSetState(TPBusPsuState aState)=0;
	
	/**
	Base abstract method.
	Checks the voltage level of the PSU is as expected. 
	The result is returned by a call to DPBusPsuBase::ReceiveVoltageCheckResult(), 
	passing either KErrNone, KErrGeneral to indicate the pass/fail state or 
	KErrNotReady if the voltage check isn't complete.
    
	Provided at the variant layer.
	*/
	IMPORT_C virtual void DoCheckVoltage()=0;
	
	/**
	Fills in the supplied TPBusPsuInfo object with the characteristics of the platform.
	Provided at the variant layer.
	
	@param anInfo A reference to a TPBusPsuInfo to be filled in with the PSU characteristics.
	*/
	IMPORT_C virtual void PsuInfo(TPBusPsuInfo &anInfo)=0;
	
	IMPORT_C virtual void ReceiveVoltageCheckResult(TInt anError);
public:
	inline TUint VoltageSupported();
	inline void SetCurrLimited();
	inline TBool IsOff();
	inline TInt MaxCurrentInMicroAmps();
	inline void ResetInactivityTimer();
public:
	void PwrDownCheck();
	static void DoPwrDownCheck(TAny* aPtr);
public:
    /** Unique power supply unit identification number.*/
	TInt iPsuNum;
	
	/** Currently not in use. */
	TInt iMediaChangeNum;
	
	/**
	Indicates the method used by the platform hardware for checking
	the VCC voltage while on.
	
	The method used is identified using the values of
	the TPsuVoltChkMethod enum.
	
	@see TPsuVoltChkMethod
	@see DPBusPsuBase::DoCheckVoltage()
	*/
	TPsuVoltChkMethod iVoltCheckMethod;
	
	/** 
	Current PSU State.
	@see TPBusPsuState
	*/
	TPBusPsuState iState;
	
	/** PSU current is limited to a safe level. */
	TBool iCurrLimited;
	
	/** PSU Tick Timer */
	TTickLink iTickLink;
	
	/**
    Indicates the voltage level, or range of voltages supported .
    
    The voltage level, or range of voltages is reported as a bit mask
    in the same format as defined for the OCR register.
    
    If the PSU supports voltage adjustment, rather than a single fixed value,
    then bit31 should be set to indicate this fact
    as well as setting the appropriate bits to indicate the adjustable voltage
    range supported.
    */
	TUint iVoltageSupported;
	
	/**	The maximum current (in microAmps) that the PSU is able to supply. */  
	TInt iMaxCurrentInMicroAmps;
	
	/**
	Indicates whether the platform hardware has support for checking whether
	the voltage level of the PSU is within its expected voltage limit while turned on.
	
	Such a mechanism could detect when a faulty card is drawing excessive current.
	
	Set one or both of the bits KPsuChkWhileOn and KPsuChkOnPwrUp,
	if there is support for voltage checking.
	Set neither bit if there is no support for voltage checking.
	
	@see KPsuChkWhileOn
	@see KPsuChkOnPwrUp
	*/
	TUint iVoltCheckInterval;
	
	/** Bus inactivity counter. */
	TInt iInactivityCount;
	
	/** Bus not locked counter. */
	TInt iNotLockedCount;
	
	/** Bus inactivity timeout period, in seconds, when clients are registered. */
	TInt iInactivityTimeout;
	
	/** Bus not locked timeout period, in seconds, when no clients are registered. */
	TInt iNotLockedTimeout;
	
	/** DPBusSocket which the PSU powers. */
	DPBusSocket* iSocket;	// for the moment assume a PSU only powers one socket
	
	/**
	DFC to handle PSU Tick.
	@see DPBusPsuBase::DoTickService()
	@see TDfc
	*/
	TDfc iPsuDfc;
	
	/** Power Down Function to be utilised, default is PwrDownCheck */
	PsuPwrDownCheckFn iPwrDownCheckFn;

private:
	TUint32 reserved[4];
	};

NONSHARABLE_CLASS(DPBusPowerHandler) : public DPowerHandler
	{
public:
	DPBusPowerHandler(DPBusSocket* aSocket);
public: // from DPowerHandler
	// signals from manager to client
	void PowerUp();
	void PowerDown(TPowerState);
public:
	DPBusSocket* iSocket;
	};

/**	Abstract base class for a PBUS socket.
    DPBusSocket derived class object oversees the power supply and media change functionality with the socket. 
*/
NONSHARABLE_CLASS(DPBusSocket) : public DBase
	{
public:
	/**
	Panic codes to handle the invalid powerup and powerdown state of the PBUS. 
	@see TPBusState
	*/ 
	enum TPanic
		{
		/**
		Indicates the invalid door close state.
		*/
		EDoorCloseInvalidState=0,
		
		/**
		Indicates the invalid powerup state of PBUS, considered as invalid if iState is other than TPBusState while powering up. 
		*/
		EPowerUpInvalidState=1,	
		
		/** Indicates invalid handler state corresponding to powerup event */
		EPowerUpInvalidHandlerState=2,	
		
		/** Invalid return code corresponding to powerup event */
		EPowerUpInvalidReturnCode=3,
		
		/**
		Indicates the invalid powerdown state of the media device,
		considered as invalid if iState is other than TPBusState while powering down.
		*/
		EEmergencyPowerDownInvalidState=4,
		
		/**
		Indicates the invalid powerup state of the media device, 
		considered as invalid if iState is not in (EPBusCardAbsent,EPBusOff and EPBusPowerUpPending) while on transition from standby. 
		*/
		EMcPowerUpInvalidState=5,		
		};

	/**
	The purpose of this enumeration is to handle the postponed events corresponding to media change and power down event.
	*/
	enum TPostponedEvent
		{
		/** Handles the postponed media change event  */
		EMediaChange=1,	

		/** Handles the postponed power down event */
		EPowerDown=2,	
		};

	/**
	Enumeration for handling debug functionality.
	*/
	enum TPBusDebugFunction
		{
		/** Media state used for simulation purposes */
		EControlMediaState	
		};

	/**
	PBUS simulated media states.
	*/
	enum TPBusSimulateMediaState
		{
		/** Normal State */
		EPeriphBusMediaNormal=0,
			
		/** Simulated door open */
		EPeriphBusDoorOpen=1,
		
		/** Simulated door close with media absent */
		EPeriphBusMediaRemoved=2,	
		
		/** Simulated door close with media present */
		EPeriphBusMediaPresent=3,
		
		/** Simulate two consecutive door open interrupts */
		EPeriphBusMediaDoubleDoorOpen=4
		};

public:
	DPBusSocket(TInt aSocketNumber);
	IMPORT_C static DPBusSocket* SocketFromId(TInt anId);
	IMPORT_C static void Panic(DPBusSocket::TPanic aPanic);
public:
	virtual TInt Create(const TDesC* aName);
	virtual TInt Init();
	
	/**
	Initiates a power up sequence on the stack. This method should be implemented by the derived class.
	The implementation should call PowerUpSequenceComplete() to notify the status on completion of power up sequence.

	@see DMMCSocket::InitiatePowerUpSequence()
	@see DMMCStack::PowerUpStack()
    */
	virtual void InitiatePowerUpSequence()=0;
	
	/**
	Indicates the presence of a card. This method should be implemented by the derived class.
	The implementation should use variant specific implementation of CardDetect() method to detect the card presence.
	
	@return ETrue if a card is present, else EFalse
	@see DMMCStack::CardDetect()
	*/
	virtual TBool CardIsPresent()=0;
	
	/**
	Resets the socket. This method should be implemented by the derived class.
    The implementation should cancel powerup timer and DFC queue if any of this request is outstanding,
	Power down the stack.

    @see DMMCSocket::Reset1()
	*/
	virtual void Reset1()=0;

	/**
	Resets the socket on media change event.This method should be implemented by the derived class.
	The implementation should remove any allocated memory following a media change event.
    
	*/
	virtual void Reset2()=0;
public:
	inline TInt State();
	inline TDfcQue* DfcQ();
	inline TMediaState MediaState();
public:
	IMPORT_C void Add(TPBusCallBack* aCallBack);
	IMPORT_C TInt PowerUp();
	IMPORT_C void ForceMediaChange();
	IMPORT_C TInt InCritical();
	IMPORT_C void EndInCritical();
	IMPORT_C void DeltaCurrentConsumption(TInt aDelta);
	IMPORT_C void PowerUpSequenceComplete(TInt anError);
	void PsuFault(TInt anError);
	void PsuTimeout();
	void ResetSocket(TBool aFullReset);
	void ChangeState(TInt aState, TInt anError);
	void MediaChangeEvent(TBool aDoorOpened);
	void DoorOpenEvent();
	void DoorCloseEvent();
	void Isr(TInt anId);
	void DoPowerUp();
	void DoPowerDown();
	IMPORT_C void PowerDownComplete();
	IMPORT_C void RequestAsyncPowerDown();
	IMPORT_C virtual TInt ControlIO(TInt aFunction, TAny* aParam1, TAny* aParam2);
public:
	/**
	Current PBus type.
	@see TPBusType 
	*/
	TPBusType iType;
	/**
	Current socket number.
	*/
	TInt iSocketNumber;
	/**
	Name of the socket.
	*/
	const TDesC* iName;
	/**
	Current state of Peripheral bus controller.
	@see TPBusState
	*/
	TInt iState;
	/**
    Counter to keep track of postponed events.
	*/
	TInt iPostponeCount;
	/**
    Current postponed events.
    @see TPostponedEvent
	*/
	TUint16 iPostponedEvents;
	/**
	Current simulated state of the Media.
	@see TPBusSimulateMediaState
    */
	TUint16 iSimulatedMediaState;
	/**
    The call back functions queue.
	@see SDblQue
	*/
	SDblQue iCallBackQ;
	/**  
	Pointer to DPBusPowerController object.
	@see DPBusPowerHandler
	*/
	DPBusPowerHandler* iPowerHandler;

	/**	
	Holds media change number.
    @see MediaChangeFromSocket
	*/
	TInt iMediaChangeNumber;

	/**	
	Pointer to DMediaChangeBase object.
	@see DMediaChangeBase
	*/
	DMediaChangeBase* iMediaChange;

	/**
	Holds the state of media door.
	Updated when there is change in media event corresponing to Door open.
	@see DMediaChangeBase::DoorOpenService()
	@see TMediaState
	*/
	TBool iDoorOpened;

	/**	
	Pointer to DPBusPsuBase object.
	@see DPBusPsuBase
	*/
	DPBusPsuBase* iVcc;

	/**
	Indicates the device is in standby state (i.e. Sets to ETrue when the device is in standby state).
	*/
	TBool iStandby;

	/**
    DFC to handle media change events like (DoorOpen,DoorClose).
	@see DMediaChangeBase::AcknowledgeEvent
	@see TDfc
	*/
	TDfc iMediaChangeDfc;

	/**
	DFC to handle powering up of the device.
	@see TDfc
	*/
	TDfc iPowerUpDfc;

	/**
	DFC	to handle powerdown state of the device.
	
	@see TDfc
	*/
	TDfc iPowerDownDfc;

	/**
	DFC to handle PSU Tick.
	@see DPBusPsuBase::DoTickService()
	@see TDfc
	*/
	TDfc iPsuDfc;

	/**
    The DFC queue used for driver functions.
	@see TDfcQue
	*/
	TDfcQue iDfcQ;

	/**
    Used when there is request for power down of the device from client.
	*/
	TInt iRequestPowerDownCount;
	};

GLREF_D DMediaChangeBase* TheMediaChanges[KMaxMediaChanges];
GLREF_D DPBusSocket* TheSockets[KMaxPBusSockets];
GLREF_D DPBusPsuBase* TheVccs[KMaxPBusVccs];
GLREF_D DPBusPsuBase* TheVccCores[KMaxPBusVccs]; 

#include <drivers/pbus.inl>
#endif
