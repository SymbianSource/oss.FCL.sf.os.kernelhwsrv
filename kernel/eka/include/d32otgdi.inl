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
//

/**
 @file
 @internalTechnology
 @prototype
 
 The driver's name
 
 @return The name of the driver
 
 @internalComponent
*/
inline const TDesC& RUsbOtgDriver::Name()
	{
	_LIT( KDriverName, "USBOTGDRIVER" );
	return KDriverName;
	}

/**
  The driver's version

  @return The version number of the driver

  @internalComponent
*/
inline TVersion RUsbOtgDriver::VersionRequired()
	{
	const TInt KBuildVersionNumber = KE32BuildVersionNumber;

	return TVersion( KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber );
	}

#ifndef __KERNEL_MODE__

/**
  Open a a logical channel to the OTG driver

  @return	System-wide error code giving status of connection attempt.
*/
inline TInt RUsbOtgDriver::Open()
	{
	TInt rc = KErrNone;

	// Check to see if this object has already been opened - if it has,
	// there will be a handle set.

	if ( Handle() )
		{
		User::Panic(OtgdiPanics::KUsbOtgDriverPanicCat, OtgdiPanics::EUsbOtgDriverAlreadyOpened);
		}

	rc = DoCreate( Name(), VersionRequired(), KNullUnit, NULL, NULL, EOwnerThread );

	// We expect back KErrNone - any other problem indicates an attempt by
	// the caller to double-open the driver, which is forbidden

	if ( rc != KErrNone )
		{
		RDebug::Print(_L("********************************"));
		RDebug::Print(_L("* RUsbOtgDriver::Open() Fault! *"));
		RDebug::Print(_L("********************************"));
		}

	return rc;
	}

/**
  Special method to alter the default behaviour of the stack:

  Test mode is activated (special) 

  @return	Driver-specific error code or system-wide success
*/
inline TInt RUsbOtgDriver::ActivateOptTestMode()
	{
	return DoControl( EActivateOptTestMode );
	}

/**
  Overall command to start USB stacks

  @return	Driver-specific error code or system-wide success
*/
inline TInt RUsbOtgDriver::StartStacks()
	{
	return DoControl( EStartStacks );
	}

/**
  Overall command to stop USB stacks
*/
inline void RUsbOtgDriver::StopStacks()
	{
	static_cast<void>(DoControl( EStopStacks ));
	}

/**
  Generic event-reporting mechanism, provided in the form of a standard 
  watcher which registers and allows user to block until notification.

  To cater for rapidly-occurring events, this function will return the 
  earliest event code (assuming a FIFO stack model for recording such 
  events)

  The notification function does not support multiple instances, it is expected 
  to be limited solely to ownership by USBMAN

  @param	aOldestEvent	parameter to collect the TOtgEvent
  @param	aStatus			standard request completion
*/
inline void RUsbOtgDriver::QueueOtgEventRequest(TOtgEvent& aOldestEvent, TRequestStatus& aStatus)
	{
	DoRequest( EQueueOtgEventRequest, aStatus, &aOldestEvent );
	}

/**
  Cancellation method for event notification

  Note that the 'cancel' function does not return an error code in the event
  that there is no registered watcher: it is assumed that the code
  within will contain an ASSERT_DEBUG check to confirm watcher validity.
*/
inline void RUsbOtgDriver::CancelOtgEventRequest()
	{
	static_cast<void>(DoControl( ECancelOtgEventRequest ));
	}
	
/**
  Generic state-reporting mechanism, provided in the form of a standard 
  watcher which registers and allows user to block until notification.

  To cater for rapidly-occurring changes, this function will return the 
  earliest state code (assuming a FIFO stack model for recording such 
  States)

  The notification function does not support multiple instances, it is expected 
  to be limited solely to ownership by USBMAN

  @param	aState		parameter to collect the TOtgState
  @param	aStatus		standard request completion
*/
inline void RUsbOtgDriver::QueueOtgStateRequest(TOtgState& aState, TRequestStatus& aStatus)
	{
	DoRequest( EQueueOtgStateRequest, aStatus, &aState );
	}

/**
  Cancellation method for state notification

  Note that the 'cancel' function does not return an error code in the event
  that there is no registered watcher: it is assumed that the code
  within will contain an ASSERT_DEBUG check to confirm watcher validity.
*/
inline void RUsbOtgDriver::CancelOtgStateRequest()
	{
	static_cast<void>(DoControl( ECancelOtgStateRequest ));
	}
	
/**
  Generic message-reporting mechanism, provided in the form of a standard 
  watcher which registers and allows user to block until notification.

  To cater for rapidly-occurring changes, this function will return the 
  earliest message code (assuming a FIFO stack model for recording such 
  Messages)

  The notification function does not support multiple instances, it is expected 
  to be limited solely to ownership by USBMAN

  @param	aMessage	parameter to collect the TOtgMessage
  @param	aStatus		standard request completion
*/
inline void RUsbOtgDriver::QueueOtgMessageRequest(TOtgMessage& aMessage, TRequestStatus& aStatus)
	{
	DoRequest( EQueueOtgMessageRequest, aStatus, &aMessage );
	}

/**
  Cancellation method for message notification

  Note that the 'cancel' function does not return an error code in the event
  that there is no registered watcher: it is assumed that the code
  within will contain an ASSERT_DEBUG check to confirm watcher validity.
*/
inline void RUsbOtgDriver::CancelOtgMessageRequest()
	{
	static_cast<void>(DoControl( ECancelOtgMessageRequest ));
	}

/**
  Single-purpose instant-report mechanism to return the current state
  of the ID-Pin on the MIni/Micro-AB connector.

  This method is expected to complete immediately and return the
  *current* state of the ID-Pin, it will omit any intermediate
  changes that may have occurred since it was last called.

  @param	aCurrentIdPin	parameter to collect the ID-Pin state
  @param	aStatus			standard request completion
*/
inline void RUsbOtgDriver::QueueOtgIdPinNotification(TOtgIdPin& aCurrentIdPin, TRequestStatus& aStatus)
	{
	DoRequest( EQueueOtgIdPinNotification, aStatus, &aCurrentIdPin );
	}

/**
  Cancellation method for ID-Pin notification
*/
inline void RUsbOtgDriver::CancelOtgIdPinNotification()
	{
	static_cast<void>(DoControl( ECancelOtgIdPinNotification ));
	}

/**
  Single-purpose instant-report mechanism to return the current state
  of the Voltage level on the Mini/Micro-AB connector.

  This method is expected to complete immediately and return the
  *current* state of the voltage, it will omit any intermediate
  changes that may have occurred since it was last called.

  @param	aCurrentVbus	parameter to collect the voltage state
  @param	aStatus			standard request completion
*/
inline void RUsbOtgDriver::QueueOtgVbusNotification(TOtgVbus& aCurrentVbus, TRequestStatus& aStatus)
	{
	DoRequest( EQueueOtgVbusNotification, aStatus, &aCurrentVbus );
	}

/**
  Cancellation method for Vbus notification
*/
inline void RUsbOtgDriver::CancelOtgVbusNotification()
	{
	static_cast<void>(DoControl( ECancelOtgVbusNotification ));
	}

/**
  Single-purpose instant-report mechanism to return the current state
  of the permissive/advisory that indicates 'idle' state where it is
  deemed safe to drop VBUS.

  This method is expected to complete immediately and return the
  *current* state of the idleness, it will omit any intermediate
  changes that may have occurred since it was last called.

  @param	aCurrentIdle	parameter to collect the idle state
  @param	aStatus			standard request completion
*/
inline void RUsbOtgDriver::QueueOtgConnectionNotification(TOtgConnection& aCurrentIdle, TRequestStatus& aStatus)
	{
	DoRequest( EQueueOtgConnectionNotification, aStatus, &aCurrentIdle );
	}

/**
  Cancellation method for Idle notification
*/
inline void RUsbOtgDriver::CancelOtgConnectionNotification()
	{
	static_cast<void>(DoControl( ECancelOtgConnectionNotification ));
	}

/**
  Single-purpose instant-report mechanism to return the current state
  of the OTG state machine

  This method is expected to complete immediately and return the
  *current* state, it will omit any intermediate changes that may 
  have occurred since it was last called.

  @param	aCurrentState	parameter to collect the state
  @param	aStatus			standard request completion
*/
inline void RUsbOtgDriver::QueueOtgStateNotification(TOtgState& aCurrentState, TRequestStatus& aStatus)
	{
	DoRequest( EQueueOtgStateNotification, aStatus, &aCurrentState );
	}

/**
  Cancellation method for State notification
*/
inline void RUsbOtgDriver::CancelOtgStateNotification()
	{
	static_cast<void>(DoControl( ECancelOtgStateNotification ));
	}

/**
  USBMAN wants to assert bus request for 'Host' or 'Peripheral' duty

  Default-Host: this will result in an attempt to raise Vbus

  Default-Device: this will result in an attempt to use SRP(+HNP)

  The bus request is asserted until BusDrop() is called.

  @return	Error code returns are related to the current OTG state context
			at the time of calling, and not to the asynchronous result
			(which is reported via event notification)
*/
inline TInt RUsbOtgDriver::BusRequest()
	{
	return DoControl( EBusRequest );
	}

/**
  USBMAN wants to permit use of the bus, in response to a request from
  the other (B) end of the link to make use of it.

  @return	Error code returns are related to the current OTG state context
			at the time of calling, and not to the asynchronous result
			(which is reported via event notification)
*/
inline TInt RUsbOtgDriver::BusRespondSrp()
	{
	return DoControl( EBusRespondSrp );
	}

/**
  USBMAN wants to stop using the bus.

  This function can only be called from the A-Device and will result in 
  the voltage drive being removed from the Vbus line.

  @return	Error code returns are related to the current OTG state context
			at the time of calling, and not to the asynchronous result
			(which is reported via event notification)

			In particular, this function will return an error code if it
			is called in any 'B' state
*/
inline TInt RUsbOtgDriver::BusDrop()
	{
	return DoControl( EBusDrop );
	}

/**
  USBMAN wants to clear the bus error state.

  This function can only be called from the A-Device and will result in 
  the OTG state machine being moved out of the A_VBUS_ERR state.

  @return	Error code returns are related to the current OTG state context
			at the time of calling, and not to the asynchronous result
			(which is reported via event notification)

			In particular, this function will return an error code if it
			is called in any 'B' state 
*/
inline TInt RUsbOtgDriver::BusClearError()
	{
	return DoControl( EBusClearError );
	}

#endif  // !__KERNEL_MODE__
