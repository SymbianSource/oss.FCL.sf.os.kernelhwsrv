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

#ifndef D32OTGDI_ERRORS_H
#define D32OTGDI_ERRORS_H

#ifndef __KERNEL_MODE__
#include <e32std.h>
#else
#include <kernel/kernel.h>
#endif

/**
@file
@publishedPartner
@prototype
*/

/**
The messages which indicate problem conditions are reported as a based set
of numbers derived from the basic USB+Host set (-6670 -> -6699)
*/
const TInt KErrUsbOtgEventQueueOverflow			= -6670;
const TInt KErrUsbOtgStateQueueOverflow			= -6671;
const TInt KErrUsbOtgMessageQueueOverflow		= -6672;

const TInt KErrUsbOtgBadDeviceAttached 			= -6673;
const TInt KEventUsbOtgBadDeviceDetached 		= -6674;

const TInt KErrUsbOtgBadState					= -6675;

const TInt KErrUsbOtgInOPTTestingMode              = -6676;

const TInt KErrUsbOtgThermalNormal                 = -6677;
const TInt KErrUsbOtgThermalWarning                = -6678;
const TInt KErrUsbOtgThermalFatal                  = -6679;

const TInt KErrUsbOtgStackNotStarted			= -6680;
const TInt KErrUsbOtgVbusAlreadyRaised			= -6681;
const TInt KErrUsbOtgSrpForbidden				= -6682;

const TInt KErrUsbOtgHnpNotResponding			= -6683;
const TInt KErrUsbOtgHnpBusDrop					= -6684;

const TInt KErrUsbOtgBusControlProblem			= -6685;

const TInt KErrUsbOtgVbusPowerUpError			= -6686;

const TInt KErrUsbOtgHnpEnableProblem			= -6687;
const TInt KErrUsbOtgPeriphNotSupported			= -6688;

const TInt KErrUsbOtgVbusError					= -6690;
const TInt KErrUsbOtgSrpTimeout					= -6691;
const TInt KErrUsbOtgSrpActive					= -6692;
const TInt KErrUsbOtgSrpNotPermitted			= -6693;
const TInt KErrUsbOtgHnpNotPermitted			= -6694;
const TInt KErrUsbOtgHnpNotEnabled				= -6695;
const TInt KErrUsbOtgHnpNotSuspended			= -6696;
const TInt KErrUsbOtgVbusPowerUpNotPermitted	= -6697;
const TInt KErrUsbOtgVbusPowerDownNotPermitted	= -6698;
const TInt KErrUsbOtgVbusClearErrorNotPermitted	= -6699;

/**
The OTG Panic categories are a zero-based contiguous set of
numbers whose meaning is private to OTGDI
*/
namespace OtgdiPanics
	{
	_LIT(KUsbOtgDriverPanicCat, "USB OTG Driver");

	enum TUsbOtgDriverPanics
		{
		EUsbOtgDriverNoDfcQueue,

		EUsbOtgDriverNotYetInitialised,
		EUsbOtgDriverAlreadyOpened,

		EUsbOtgDriverStackNotStarted,

		EUsbOtgUnknownErrorReported,

		EUsbOtgDriverEventRequestAlreadyRegistered,
		EUsbOtgDriverStateRequestAlreadyRegistered,
		EUsbOtgDriverMessageRequestAlreadyRegistered,

		EUsbOtgDriverIdPinNotificationAlreadyRegistered,
		EUsbOtgDriverVbusNotificationAlreadyRegistered,
		EUsbOtgDriverConnectionNotificationAlreadyRegistered,
		EUsbOtgDriverStateNotificationAlreadyRegistered,

		EUsbOtgDriverIdPinNotificationWriteFailed,
		EUsbOtgDriverVbusNotificationWriteFailed,
		EUsbOtgDriverConnectionNotificationWriteFailed,
		EUsbOtgDriverStateNotificationWriteFailed,

		EUsbOtgBadStateFunction
		};
	}

#endif // D32OTGDI_ERRORS_H
