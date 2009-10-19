// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef OTGROOT_H_
#define OTGROOT_H_


// The max OTG state/event/message text length
#define MAX_DSTRLEN		40


class COtgRoot 
	{
public:
	COtgRoot();
	
	static TBool& LddLoaded();

	virtual void AssertionFailed(TInt aFailResult, const TDesC &aErrorDescription)=0;
	virtual void AssertionFailed2(TInt aFailResult, const TDesC &aErrorDescription, TInt errorCode)=0;
	virtual TBool IsActiveOutstanding() = 0;
	
	///////////////////////////////////////////////////////////////////////
	// UTILITY Helper methods

	static void OtgEventString(const RUsbOtgDriver::TOtgEvent aEvent, 
	                            TBuf<MAX_DSTRLEN> &aDescription
	                          );
	static void OtgStateString(const RUsbOtgDriver::TOtgState aState, TBuf<MAX_DSTRLEN> &aDescription);	                          
	static void OtgMessageString(const RUsbOtgDriver::TOtgMessage aMessage, TBuf<MAX_DSTRLEN> &aDescription);	
	static void PeripheralStateString(TUint aPeripheralState, TBuf<MAX_DSTRLEN> &aDescription);
	static void AConnectionIdleString(RUsbOtgDriver::TOtgConnection aAConnectionIdle, TBuf<MAX_DSTRLEN> &aDescription);
	
	///////////////////////////////////////////////////////////////////////
    // RUsbOtgDriver API wrapper methods
    
    TInt otgLoadLdd(); // load and start stack
    void otgUnloadLdd();// unload only
    
    // open/close the user/kernel channel
    TInt otgOpen();
    void otgClose();
    
	// (Test) Activation of OPT (USB HOST/OTG Stack) tests
	TInt otgActivateOptTestMode();

    // Start/stop the HOST+OTG stack
	TInt otgStartStacks();
	void otgStopStacks();
	
	// Activate T_OTGDI FDF Actor
	
    // Bus Control (Raise/Lower VBus)
	TInt otgBusRequest();
	TInt otgBusDrop();
	TInt otgBusRespondSRP();
	TInt otgBusClearError();

	//	Functions to kick off and shut down the t_otgdi_fdfactor.exe
	//	Necessary for back-to-back tests for HNP / SRP
	TInt otgActivateFdfActor();
	void otgDeactivateFdfActor();
	
	
    /**
    QueueOtgEventRequest
    */
    void otgQueueOtgEventRequest(RUsbOtgDriver::TOtgEvent& aEvent, TRequestStatus& aStatus);
	void otgCancelOtgEventRequest();
    	
    /**
    QueueOtgEventRequest
    */
    void otgQueueOtgStateRequest(RUsbOtgDriver::TOtgState& aState, TRequestStatus& aStatus);
	void otgCancelOtgStateRequest();

    /**
    QueueOtgMessageRequest
    */
    void otgQueueOtgMessageRequest(RUsbOtgDriver::TOtgMessage& aMessage, TRequestStatus& aStatus);
	void otgCancelOtgMessageRequest();
	
    /**
    QueuePeripheralStateRequest
    */
    void otgQueuePeripheralStateRequest(TUint& aPeripheralState, TRequestStatus& aStatus);
	void otgCancelPeripheralStateRequest();
	
    /**
    QueueAConnectionIdleRequest
    */
    void otgQueueAConnectionIdleRequest(RUsbOtgDriver::TOtgConnection& aAConnectionIdle, TRequestStatus& aStatus);
	void otgCancelAConnectionIdleRequest();
	
	// New ID_PIN API calls
	void otgQueueOtgIdPinNotification(RUsbOtgDriver::TOtgIdPin& aIdPin, TRequestStatus& aRequest);
	void otgCancelOtgIdPinNotification();
	
	// New VBus API calls
	void otgQueueOtgVbusNotification(RUsbOtgDriver::TOtgVbus& aVbus, TRequestStatus& aStatus);
	void otgCancelOtgVbusNotification();
	
	TBool otgIdPinPresent();
	TBool otgVbusPresent();
    	
	static TBool SetLoaded(TBool aState);	// internal flag

	// await arrival of a specific message/event/state
	//TBool ExpectOTGEvent(const RUsbOtgDriver::TOtgEvent eventID, TInt timeoutMs=-1);
	//TBool ExpectOTGMessage(const RUsbOtgDriver::TOtgMessage messageID, TInt timeoutMs=-1);
	
	void SetMaxPowerToL(TUint16 val);
	void GetMaxPower(TUint16& val);
	
private:
	
//protected:
public:
    TBool StepUnloadLDD();
    TBool StepLoadLDD();
	void  StepSetOptActive();
    TBool StepUnloadClient();
    TBool StepLoadClient(TUint16 aPID, TBool aEnableHNP=ETrue, TBool aEnableSRP=ETrue);
    TBool StepDisconnect();
    TBool StepConnect();
    TBool StepChangeVidPid(TUint16 aVID, TUint16 aPID);
    
protected:

	// API datas
	RUsbOtgDriver::TOtgEvent 	iOTGEvent;
	RUsbOtgDriver::TOtgState 	iOTGState;
	RUsbOtgDriver::TOtgMessage 	iOTGMessage;
	RUsbOtgDriver::TOtgIdPin	iOTGIdPin;
	RUsbOtgDriver::TOtgVbus   	iOTGVBus;
	TUint 						iPeripheralState;
	RUsbOtgDriver::TOtgConnection iAConnectionIdle;
	// api flags
	TBool iOptActive;
	static TBool 	iLoadedLdd;
	static TBool 	iFdfActorActive;
	
	//	FDF Actor handle
	static RProcess	iFdfActorProcess
	};


#endif /*OTGROOT_H_*/
