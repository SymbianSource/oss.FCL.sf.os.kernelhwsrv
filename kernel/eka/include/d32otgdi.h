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
//

/**
 @file
 @internalTechnology
 @prototype
*/

#ifndef D32OTGDI_H
#define D32OTGDI_H

#ifdef __KERNEL_MODE__
#  include <kernel/klib.h>
#  include <e32ver.h>
#else
#  include <e32base.h>
#  include <e32ver.h>
#  include <e32debug.h>
#endif

#include <d32otgdi_errors.h>

/**
The bi-directional interface which USB Manager uses to talk to OTGDI about 
OTG related issues

@note This API is only available to USBMAN, a restriction which is enforced 
by checking the SID of the calling process.
*/
NONSHARABLE_CLASS(RUsbOtgDriver) : public RBusLogicalChannel
	{
	friend class DUsbOtgDriver;

public:

	// Version number history:
	//
	// PREQ1782 = 1.0
	// PREQ1305 = 1.1
	
	static const TInt KMajorVersionNumber = 1;
	static const TInt KMinorVersionNumber = 1;

private:
	
	/**
	OTG Event, supplied in a form that will enable use as an array
	index (note also how this is used below in the bit-mask form,
	but that this is only extended to a few, other events are OTG
	internal and used only to drive the local state machine)
	*/
	enum TOtgEventIndex
		{
		/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - **
		'public' OTG events
		** - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

		/**
		OTG events related to plug insertion or removal
		*/
		EEventIndexAPlugInserted = 0,
		EEventIndexAPlugRemoved,

		/**
		OTG events relating to changes visible on the bus
		*/
		EEventIndexVbusRaised,
		EEventIndexVbusDropped,
		
		EEventIndexSrpInitiated,

		EEventIndexSrpReceived,
		EEventIndexHnpEnabled,
		EEventIndexHnpDisabled,
		EEventIndexHnpSupported,
		EEventIndexHnpAltSupported,

		EEventIndexBusConnectionBusy,
		EEventIndexBusConnectionIdle,
		
		/**
		OTG events related to changes in the current role the device
		is performing (independant of the orientation of the connection)
		*/
		EEventIndexRoleChangedToHost,
		EEventIndexRoleChangedToDevice,
		EEventIndexRoleChangedToIdle,

		/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - **
		'private' OTG events - these are not converted to the bit-map form,
		but are used solely to stim the OTG state machine
		** - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
		
		EEventIndexPrivateAfterThis,

		EEventIndexVbusError,
		EEventIndexClearVbusError,
		
		EEventIndexResetStateMachine,

		EEventIndexNoEvent,

		/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - **
		Single indicator used for array sizing
		** - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
		EEventIndexNumEvents
		};

public:

	/**
	OTG Events, supplied in a form that can be used to create a mask
	that can specify a watcher's events of interest.
	*/
	enum TOtgEvent
		{
		/**
		 * 'empty' dummy event
		 */
		EEventNotValid = 0,
		
		/**
		OTG events related to plug insertion or removal
		*/
		EEventAPlugInserted			= ( 1 << EEventIndexAPlugInserted		),
		EEventAPlugRemoved			= ( 1 << EEventIndexAPlugRemoved		),

		EEventGroupPlugChanges		= ( EEventAPlugInserted
									   |EEventAPlugRemoved
									  ),

		/**
		OTG events relating to changes visible on the bus
		*/
		EEventVbusRaised			= ( 1 << EEventIndexVbusRaised			),
		EEventVbusDropped			= ( 1 << EEventIndexVbusDropped			),

		EEventSrpInitiated			= ( 1 << EEventIndexSrpInitiated		),

		EEventSrpReceived			= ( 1 << EEventIndexSrpReceived			),
		EEventHnpEnabled			= ( 1 << EEventIndexHnpEnabled			),
		EEventHnpDisabled			= ( 1 << EEventIndexHnpDisabled			),
		EEventHnpSupported			= ( 1 << EEventIndexHnpSupported		),
		EEventHnpAltSupported		= ( 1 << EEventIndexHnpAltSupported		),

		EEventBusConnectionBusy		= ( 1 << EEventIndexBusConnectionBusy	),
		EEventBusConnectionIdle		= ( 1 << EEventIndexBusConnectionIdle	),

		EEventGroupBusChanges		= ( EEventVbusRaised
									   |EEventVbusDropped
									   |EEventSrpInitiated
									   |EEventSrpReceived
									   |EEventHnpEnabled
									   |EEventHnpDisabled
									   |EEventHnpSupported	
									   |EEventHnpAltSupported
									   |EEventBusConnectionBusy
									   |EEventBusConnectionIdle
									  ),

		/**
		OTG events related to changes in the current role the device
		is performing (independant of the orientation of the connection)
		*/
		EEventRoleChangedToHost		= ( 1 << EEventIndexRoleChangedToHost	),
		EEventRoleChangedToDevice	= ( 1 << EEventIndexRoleChangedToDevice	),
		EEventRoleChangedToIdle		= ( 1 << EEventIndexRoleChangedToIdle	),

		EEventGroupRoleChanges		= ( EEventRoleChangedToHost
									   |EEventRoleChangedToDevice
									   |EEventRoleChangedToIdle
									  ),

		/**
		Helpful macro to allow users to register for 'everything'
		*/
		EEventGroupAll				= ( EEventGroupPlugChanges
									   |EEventGroupBusChanges
									   |EEventGroupRoleChanges
									  )
		};

private:

	/**
	OTG State, supplied in a form that will enable use as an array
	index (note also how this is used below in the bit-mask form)
	*/
	enum TOtgStateIndex
		{
		/**
		Single case of non-stable state, used only during
		startup
		*/
		EStateIndexReset = 0,

		/**
		'A'-connection states (names are derived from OTG-Supplement
		Figure 6-2 On-The-Go A-Device State Diagram)
		*/
		EStateIndexAIdle,
		EStateIndexAHost,
		EStateIndexAPeripheral,
		EStateIndexAVbusError,

		/**
		'B'-connection states (names are derived from OTG-Supplement
		Figure 6-3 On-The-Go B-Device State Diagram)
		*/
		EStateIndexBIdle,
		EStateIndexBPeripheral,
		EStateIndexBHost,

		/**
		Single indicator used for array sizing
		*/
		EStateIndexNumStates
		};

public:

	/**
	OTG State, supplied in a form that can be used to create a mask
	that can specify a watcher's states of interest.
	*/
	enum TOtgState
		{
		/**
		 * 'empty' dummy state
		 */
		EStateNotValid = 0,
		
		/**
		Single case of non-stable state, used only during
		startup
		*/
		EStateReset					= ( 1 << EStateIndexReset		),

		/**
		'A'-connection states (names are derived from OTG-Supplement
		Figure 6-2 On-The-Go A-Device State Diagram)
		*/
		EStateAIdle					= ( 1 << EStateIndexAIdle		),
		EStateAHost					= ( 1 << EStateIndexAHost		),
		EStateAPeripheral			= ( 1 << EStateIndexAPeripheral	),
		EStateAVbusError			= ( 1 << EStateIndexAVbusError	),

		EStateAllA					= ( EStateAIdle
									   |EStateAHost
									   |EStateAPeripheral
									   |EStateAVbusError
									  ),

		EStateAllAExceptBusError	= ( EStateAIdle
									   |EStateAHost
									   |EStateAPeripheral
									  ),

		/**
		'B'-connection states (names are derived from OTG-Supplement
		Figure 6-3 On-The-Go B-Device State Diagram)
		*/
		EStateBIdle					= ( 1 << EStateIndexBIdle		),
		EStateBPeripheral			= ( 1 << EStateIndexBPeripheral	),
		EStateBHost					= ( 1 << EStateIndexBHost		),

		EStateAllB					= ( EStateBIdle
									   |EStateBPeripheral
									   |EStateBHost
									  )
		};

public:

	/**
	OTG Messages, these can be retrieved to satisfy the OTG Supplement 
	"No Silent Failures" requirement.

    The error numbers are derived for constants set up in d32otgdi_error.h
	and are used here so that the enumerator can be used to enforce compiler
	checks on the functions that handle the messages.

    Note that this minimal set of message events only contains the few
	things that are reported by the USB OTG Stack: it is expected that 
	these will be merged into a composite message event flow by USBMAN
	*/
	enum TOtgMessage
		{
		/**
		Internal OTGDI errors must also be offered to USBMAN in order to
		fully support the "No Silent Failures" policy
		*/
		EEventQueueOverflow					= KErrUsbOtgEventQueueOverflow,	
		EStateQueueOverflow					= KErrUsbOtgStateQueueOverflow,
		EMessageQueueOverflow				= KErrUsbOtgMessageQueueOverflow,

		EMessageBadState					= KErrUsbOtgBadState,

		/**
		Errors relating to attempts to do wrong things to VBUS
		*/
		EMessageStackNotStarted				= KErrUsbOtgStackNotStarted,
		EMessageVbusAlreadyRaised			= KErrUsbOtgVbusAlreadyRaised,
		EMessageSrpForbidden				= KErrUsbOtgSrpForbidden,

		/**
		Generic message that there has been some form of problem in
		the lower-level USB OTG stack's calls
		*/
		EMessageBusControlProblem			= KErrUsbOtgBusControlProblem,

		/**
		Generic message that there has been a reportable failure in sending
		the B_HNP_ENABLE SetFeature command
		*/
		EMessageHnpEnableProblem			= KErrUsbOtgHnpEnableProblem,
		
		/**
		Peripheral reported as 'not-supported'
		*/
		EMessagePeriphNotSupported			= KErrUsbOtgPeriphNotSupported,
		
		/**
		Individual error messages 
		*/
		EMessageVbusError					= KErrUsbOtgVbusError,
		EMessageSrpTimeout					= KErrUsbOtgSrpTimeout,
		EMessageSrpActive					= KErrUsbOtgSrpActive,
		EMessageSrpNotPermitted				= KErrUsbOtgSrpNotPermitted,
		EMessageHnpNotPermitted				= KErrUsbOtgHnpNotPermitted,
		EMessageHnpNotEnabled				= KErrUsbOtgHnpNotEnabled,
		EMessageHnpNotSuspended				= KErrUsbOtgHnpNotSuspended,
		EMessageVbusPowerUpNotPermitted		= KErrUsbOtgVbusPowerUpNotPermitted,
		EMessageVbusPowerUpError			= KErrUsbOtgVbusPowerUpError,
		EMessageVbusPowerDownNotPermitted	= KErrUsbOtgVbusPowerDownNotPermitted,
		EMessageVbusClearErrorNotPermitted	= KErrUsbOtgVbusClearErrorNotPermitted,
		EMessageHnpNotResponding			= KErrUsbOtgHnpNotResponding,
		EMessageHnpBusDrop					= KErrUsbOtgHnpBusDrop,
		
		/**
		Bad device attach/detach message
		*/
		EMessageBadDeviceAttached 			= KErrUsbOtgBadDeviceAttached,
		EMessageBadDeviceDetached 			= KEventUsbOtgBadDeviceDetached
		};

public:

	/**
	Set of OTG operations required for use of the User-Kernel channel
	*/
	enum TOtgRequest
		{
		EQueueOtgEventRequest = 0,
		EQueueOtgStateRequest,
		EQueueOtgMessageRequest,

		EQueueOtgIdPinNotification,
		EQueueOtgVbusNotification,
		EQueueOtgConnectionNotification,
		EQueueOtgStateNotification,
		};

	enum TOtgControl
		{
		EActivateOptTestMode = 0,

		EStartStacks,
		EStopStacks,

		ECancelOtgEventRequest,
		ECancelOtgStateRequest,
		ECancelOtgMessageRequest,

		ECancelOtgIdPinNotification,
		ECancelOtgVbusNotification,
		ECancelOtgConnectionNotification,
		ECancelOtgStateNotification,

		EBusRequest,
		EBusRespondSrp,
		EBusDrop,
		EBusClearError
		};

	/**
	ID-Pin possible states (note that this is not a plain boolean, to
	allow for future expansion to cover car-kit - if this is needed, then
	a new enum value of 'EIdPinCarKit' should be inserted)
	*/
	enum TOtgIdPin
		{
		EIdPinAPlug = 0,
		EIdPinBPlug,
		
		// deprecated EIdPinCarKit - to be removed after OTGDI reaches MCL
		
		EIdPinCarKit,

		EIdPinUnknown
		};

	/**
	VBUS voltage level possible states (as detected from an OTG transceiver)
	*/
	enum TOtgVbus
		{
		EVbusHigh = 0,
		EVbusLow,

		EVbusUnknown
		};
		
	/**
	Connection 'Idle' indicator (potential to drop VBUS)
	*/
	enum TOtgConnection
		{
		EConnectionBusy = 0,
		EConnectionIdle,
		
		EConnectionUnknown
		};

public:

	inline static const TDesC& Name();
	inline static TVersion VersionRequired();

#ifndef __KERNEL_MODE__

public:

	inline TInt Open();

	inline TInt ActivateOptTestMode();

	inline TInt StartStacks();
	inline void StopStacks();

	inline void QueueOtgEventRequest(TOtgEvent& aOldestEvent, TRequestStatus& aStatus);
	inline void CancelOtgEventRequest();

	inline void QueueOtgStateRequest(TOtgState& aState, TRequestStatus& aStatus);
	inline void CancelOtgStateRequest();

	inline void QueueOtgMessageRequest(TOtgMessage& aMessage, TRequestStatus& aStatus);
	inline void CancelOtgMessageRequest();

	inline void QueueOtgIdPinNotification(TOtgIdPin& aCurrentIdPin, TRequestStatus& aStatus);
	inline void CancelOtgIdPinNotification();

	inline void QueueOtgVbusNotification(TOtgVbus& aCurrentVbus, TRequestStatus& aStatus);
	inline void CancelOtgVbusNotification();

	inline void QueueOtgConnectionNotification(TOtgConnection& aCurrentConnection, TRequestStatus& aStatus);
	inline void CancelOtgConnectionNotification();

	inline void QueueOtgStateNotification(TOtgState& aCurrentState, TRequestStatus& aStatus);
	inline void CancelOtgStateNotification();

	inline TInt BusRequest();
	inline TInt BusRespondSrp();
	inline TInt BusDrop();
	inline TInt BusClearError();

#endif // !__KERNEL_MODE__
	};

#include <d32otgdi.inl>

#endif // D32OTGDI_H
