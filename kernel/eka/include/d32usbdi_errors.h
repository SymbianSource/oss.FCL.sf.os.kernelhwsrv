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
 @publishedPartner
 @prototype
*/

#ifndef D32USBDI_ERRORS_H
#define D32USBDI_ERRORS_H

#ifndef __KERNEL_MODE__
#include <e32std.h>
#else
#include <kernel/kernel.h>
#endif


const TInt KErrUsbRequestsPending			= -6640;
const TInt KErrUsbBadAddress				= -6641;
const TInt KErrUsbNoAddress					= -6642;
const TInt KErrUsbSetAddrFailed				= -6643;
const TInt KErrUsbNoPower					= -6644;
const TInt KErrUsbTooDeep					= -6645;
const TInt KErrUsbIOError					= -6646;
const TInt KErrUsbNotConfigured				= -6647;
const TInt KErrUsbTimeout					= -6648;
const TInt KErrUsbStalled					= -6649;
const TInt KErrUsbTestFailure				= -6650;
const TInt KErrUsbBadState					= -6651;
const TInt KErrUsbDeviceSuspended			= -6652;

/**
This error is returned when it is discovered that a bundle of USB descriptors
has a malformed topological layout.
*/
const TInt KErrUsbBadDescriptorTopology		= -6653;

// Errors from DevMon events
const TInt KErrUsbDeviceRejected			= -6654;	// Should never happen -- the device has been rejected by the stack
const TInt KErrUsbDeviceFailed				= -6655;	// The device failed to be configured
const TInt KErrUsbBadDevice					= -6656;	// Hardware fault on device, eg. no Ep0
const TInt KErrUsbBadHubPosition			= -6657;	// Hub too deep, or bus powered attached to bus powered.
const TInt KErrUsbBadHub					= -6658;	// Hardware fault on hub, eg. no Ep0
const TInt KErrUsbEventOverflow				= -6659;	// Too many bus events undrained -- one or more events lost

// Inform caller of API Misuse
const TInt KErrUsbRemoteWakeupAlreadyEnabled  = -6660;
const TInt KErrUsbRemoteWakeupAlreadyDisabled = -6661;
const TInt KErrUsbAlreadyResumed              = -6662;

/*
The following two codes are used to indicate attachment/detachment of some malfunction 
USB peripherals, which behaviour don't comply with USB specification. For example, some 
hubs drive their upstream port VBus, however such behaviour is not allowed according to USB specification. 
*/
const TInt KErrUsbBadDeviceAttached           = -6663;
const TInt KEventUsbBadDeviceDetached         = -6664;

namespace UsbdiPanics
	{
	_LIT(KUsbHubDriverPanicCat, "USBHubDriver");
	enum TUsbHubDriverPanics
		{
		EUsbHubDriverNoRollBackAfterFailedDeviceOpen		= 0,
		EUsbHubDriverRequestMadeWhileClosed					= 1,
		EUsbHubDriverInsufficientSizeToHoldStringDescriptor	= 2,
		EUsbHubDriverMultipleNotificationRequests			= 3,
		EUsbHubDriverTooManyDeviceHandles					= 4,
		EUsbHubDriverAlreadyOpened                          = 5,
		};

	_LIT(KUsbdiPanicCat, "USBDI");
	enum TUsbdiPanics
		{
		ERequestAlreadyPending							= 0,
		ETooManyPipeHandles								= 1,
		EOutOfBoundsOfLengthArray						= 2,
		EBadIsocTransferDescriptorHandle				= 3,
		EBadIsocTransferDescriptorWriteHandle			= 4,
		EBadIntrTransferDescriptorHandle				= 5,
		EBadBulkTransferDescriptorHandle				= 6,
		ETransferDescriptorAlignmentOverPageBoundary	= 7,
		ETransferDescriptorAlignmentNotPowerOfTwo		= 8,
		ETransferDescriptorNoPacketsRequested			= 9,
		ETransferDescriptorSavedToMuchData				= 10,
		ETransferDescriptorNoPacketsToSave				= 11,
		ETransferDescriptorInvalidSaveCall				= 12,
		ETransferDescriptorSavedTooManyPackets			= 13,
		ETransferDescriptorSavingTooLargeAPacket		= 14,
		ETransferDescriptorReceivedTooLargeAPacket		= 15,
		ETransferDescriptorPacketNotInBounds			= 16,
		ETransferDescriptorTooFewPacketsRequested		= 17,
		ETransferDescriptorTooManyPacketsRequested		= 18,
		ETransferDescriptorFlagsBadZlp					= 19,
		ENoChunkAllocatedForInterface					= 20,
		EChunkAlreadyAllocatedForInterface				= 21,
		ETransferWrongDescriptorType					= 22,
		EIntrBulkTransferBadLength						= 23,
		EIsocTransferWrongDescriptorType				= 24,
		EIsocTransferNoPacketsRequested					= 25,
		EIsocTransferTooManyPackets						= 26,
		EIsocTransferPacketTooBig						= 27,
		EInterfaceSettingChangeWithPipeOpen				= 28,
		EUsbDeviceDeviceStateCancelledWithNoThread		= 29,
		ECpuPageSizeIsNotMulipleOfHcdPageSize			= 30,
		EPageListRegionSelectionUnderflow				= 31,
		EPageListRegionSelectionOverflow				= 32,
		EIsocTransferRequestCrossesPageBoundary			= 33,
		EIsocTransferResultCrossesPageBoundary			= 34,
		EPipeInvalidType								= 35,
		ETransferDescriptorRequestedLengthDiffers		= 36,
		ETransferDescriptorsAlreadyRegistered			= 37,
		EOutOfBoundsOfResultArray						= 38,
		EPipeRequestMadeWhileClosed						= 39,
		EBadInterfaceHandle                             = 40,
		EUsbDeviceMultipleNotificationRequests			= 41,
		};

	_LIT(KUsbDescPanicCat, "USBDesc");
	enum TUsbDescPanics
		{
		/**
		This panic is raised when the newly created TUsbGenericDescriptor object 
		created during parsing has not set the pointer fields iParent, iFirstChild
		and iNextPeer to NULL.
		*/
		EUsbDescNonNullPointersAfterParsing	= 0,
		/**
		This panic is raised when attempting to retrieve a wLangId from String
		Descriptor Zero using a negative index.
		*/
		EUsbDescNegativeIndexToLangId		= 1,
		};
	}


namespace UsbdiFaults
	{
#ifndef __KERNEL_MODE__
	_LIT(KUsbdiFaultCat, "USBDI-Fault");
#else // __KERNEL_MODE__
	static const char* const KUsbdiFaultCat = "USBDI-Fault";
#endif // __KERNEL_MODE__
	enum TUsbdiFaults
		{
		EUsbPipeCloseFailed								= 0,
		EUsbDeviceLingeringInterfacesOnDestroy			= 1,
		EUsbDeviceLingeringStateNotifyOnDestroy			= 2,
		EUsbDeviceCannotDestroySuspendTask				= 3,
		EUsbDeviceUnexpectedStateOnStateChange			= 4,
		EUsbDeviceUnexpectedSuspensionIssued			= 5,
		EUsbDeviceUnexpectedResumeIssued				= 6,
		EUsbTransferDescriptorBadHandle					= 7,
		EUsbTransferDescriptorBadWriteHandle			= 8,
		EUsbTransferDescriptorBadAlignment				= 9,
		EUsbTransferDescriptorNoPacketsLeftToStore		= 10,
		EUsbTransferDescriptorLengthsArrayBadAlignment	= 11,
		EUsbTransferDescriptorIncompleteInitialisation	= 12,
		EUsbTransferDescriptorInvalidHeaderOffset		= 13,
		EUsbPipeNoHandleOnCancel						= 14,
		EUsbPipeTransferCompleteWithoutTransfer			= 15,
		EUsbPipeTransferWithoutPageList					= 16,
		EUsbPipeFreeTransferHandleFailedAddToPool		= 17,
		EUsbPipeTransferCompleteNoThreadForRequest		= 18,
		EUsbPipeChannelRequestMadeWithoutChannel		= 19,
		EUsbHubDriverSuccessfulAttachButNoDevice		= 20,
		EUsbHubDriverPendingEventsAndPendingEventRequest= 21,
		EUsbDeviceNoThreadProvided						= 22,
		EUsbPageListGoneBeyondHcdPageListBounds			= 23,
		EUsbTransferRequestNoThreadProvided				= 24,
		EUsbTransferRequestNoRequestProvided			= 25,
		EUsbTransferRequestDeletedWhileOutstanding		= 26,
		EUsbTransferRequestDeletedWithoutCompleting		= 27,
		EUsbTransferRequestFinalisedTwice				= 28,
		EUsbTransferRequestCompletedWithoutFinalising	= 29,
		EUsbPipeNoTransferRequestForTransfer			= 30,
		EUsbDeviceStateChangeRequestButNoThread			= 31,
		EUsbPageListInvalidRegion						= 32,
		EUsbTransferDescriptorUnexpectedEndOfIsocList	= 33,
		EUsbTransferDescriptorUnfillableElement			= 34,
		EUsbHubDriverPendingNoneEvent					= 35,
		EUsbTransferDescriptorResultsArrayBadAlignment	= 36,
		EUsbDeviceTooManyDevicesSignallingInactivity	= 37,
		EUsbHubDriverQueuedBusEventNotDeviceAttach		= 38,
		EUsbPipeHasHandleButNoInterface					= 39,
		EUsbDeviceHasHandleButNoHubDriver				= 40,
		EUsbDevMonDeviceAttachDenied					= 41,
		EUsbHubDriverZeroInterfaceTokenProduced			= 42,
		EUsbInterfaceSuccessfulPipeOpenWithNoPipe		= 43,
		EFailedToLockHostStackInWaitDeviceStateMutex    = 44,
		};

	_LIT(KUsbDescFaultCat, "USBDesc-Fault");
	enum TUsbDescFaults
		{
		EUsbDescSuccessButDataLeftUnparsed	= 0,
		EUsbDescTreePointersAlreadySet		= 1,
		EUsbDescNoTopLevelDescriptorFound	= 2,
		EUsbDescRunOffTree					= 3,
		EUsbDescTreeMemberHasNoParent		= 4,
		};
	}

#endif // D32USBDI_ERRORS_H
