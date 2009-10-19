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

#ifndef D32USBDI_HUBDRIVER_H
#define D32USBDI_HUBDRIVER_H

#ifdef __KERNEL_MODE__
#include <kernel/klib.h>
#else
#include <e32base.h>
#include <d32usbdescriptors.h>
#include <e32debug.h>
#endif

#include <e32ver.h>
#include <d32usbdi_errors.h>

/**
@file
@internalComponent
@prototype
*/

/*****************************************************************************/
/*                                                                           */
/* Interface classes                                                         */
/*                                                                           */
/*****************************************************************************/

class RUsbDevice;

/**
Functions which act on the local hub driver.  Higher level components cannot determine which host controller is in
 use, although the presence of Other Speed descriptors may give some hints.

@note This API is only available to the function driver framework, a restriction which is enforced by checking the
SID of the calling process.
*/
class RUsbHubDriver : public RBusLogicalChannel
	{
public:
	enum TUsbHubDriverRequest
		{
		EWaitForBusEvent,
		};
	enum TUsbHubDriverControl
		{
		EStartHost,
		EStopHost,
		};
 	enum TUsbHubDriverCancel
		{
		ECancelWaitForBusEvent	= 0x00000001,
		};

	class TBusEvent
		{
	public:
		enum TEvent
			{
			ENoEvent,
			EDeviceAttached,
			EDeviceRemoved,
			EDevMonEvent
			};

	public:
		TEvent	iEventType;
		TUint	iDeviceHandle;
		TInt	iError;
		TInt	iReason;		// When iEventType == EDevMonEvent, this may hold additional details of the reason for the notification
		};

#ifndef __KERNEL_MODE__
friend class RUsbDevice;
public:
	inline TInt Open();
	inline TInt StartHost();
	inline void StopHost();

	inline void WaitForBusEvent(TBusEvent& aEvent, TRequestStatus& aRequest);
	inline void CancelWaitForBusEvent();

#endif

	inline static const TDesC& Name();
	inline static TVersion VersionRequired();
	};


/**
Use restricted to the Function Driver Framework, enforced by SID.
Provides a means to examine the configuration descriptor, and so load appropriate Function Drivers.
*/
class RUsbDevice
	{
public:
	enum TUsbDeviceRequest
		{
		EDeviceRequest		= 0x4000000,   // start at 0x4000000 to differentiate from other requests
		EDeviceStateChange,
		};
	enum TUsbDeviceControl
		{
		EDeviceControl		= 0x4000000,   // start at 0x4000000 to differentiate from other controls
		EOpen,
		EClose,
		EGetDeviceDescriptor,
		EGetConfigurationDescriptorSize,
		EGetConfigurationDescriptor,
		EGetStringDescriptor,
		EGetInterfaceToken,
		ESuspend,
		EResume,
		ECancelDeviceStateChange,
		};

	enum TDeviceState
		{
		EDeviceActive,
		EDeviceSuspended,
		EDeviceTriState,
		};
	
	class TInterfaceTokenParameters
		{
	public:
		TInt		iInterfaceNumber;
		TUint32*	iToken;
		};
	
	class TStringDescParams
		{
	public:
		TDes8*		iTarget;
		TInt		iIndex;
		TInt		iLangId;
		};

#ifndef __KERNEL_MODE__
public:
	inline RUsbDevice();

	inline TInt Open(RUsbHubDriver& aHub, TUint aHandle);
	inline void Close();
	
	inline void QueueDeviceStateChangeNotification(TDeviceState& aCurrentState, TRequestStatus& aRequest);
	inline void CancelDeviceStateChangeNotification();
	inline TUint Handle() const;
	inline TInt GetTokenForInterface(TInt aInterfaceNumber, TUint32& aToken);

	inline TInt Suspend();
	inline TInt Resume();

	inline TInt GetDeviceDescriptor(TUsbDeviceDescriptor& aDescriptor);
	inline TInt GetConfigurationDescriptor(TUsbConfigurationDescriptor& aDescriptor);
	inline TInt GetStringDescriptor(TUsbStringDescriptor*& aDescriptor, TDes8& aTarget, TInt aIndex);
	inline TInt GetStringDescriptor(TUsbStringDescriptor*& aDescriptor, TDes8& aTarget, TInt aIndex, TInt aLangId);
	
private:		// Internal function overloads
	inline TInt GetDeviceDescriptor(TDes8& aDeviceDesc);
	inline TInt GetConfigurationDescriptorSize(TInt& aConfigDescSize);
	inline TInt GetConfigurationDescriptor(TDes8& aConfigDesc);
	inline TInt GetStringDescriptor(TDes8& aStringDescriptor, TInt aIndex, TInt aLangId =0);
	
private:
	inline void GetLocalDescriptorsL();
	inline TInt ParseStringDescriptor(TUsbStringDescriptor*& aDescriptor, const TDesC8& aData);

private:
	TUsbDeviceDescriptor* iHeadDeviceDescriptor;
	TUsbConfigurationDescriptor* iHeadConfDescriptor;

	static const TUint KDeviceDescriptorSize = 18;
	TBuf8<KDeviceDescriptorSize> iDeviceDescriptorData;
	HBufC8* iConfigurationDescriptorData;

private:
	RUsbHubDriver* iHub;
	TUint iHandle;

#endif
	};


#include <d32usbdi_hubdriver.inl>


#endif	// D32USBDI_HUBDRIVER_H
