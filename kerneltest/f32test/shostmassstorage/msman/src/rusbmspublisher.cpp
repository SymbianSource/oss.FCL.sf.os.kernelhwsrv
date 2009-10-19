// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
*/

#include <e32cmn.h>
#include <e32std.h>
#include <e32property.h>
#include <e32def.h>
#include <e32debug.h>
#include "rusbmspublisher.h"
#include "debug.h"

//------------------------------------------------------------------------------
/**
Constructor
*/

const TUid KMyPropertyCat = {0x10285B2E};
const TUid KUsbmanSvrSid = {0x101fe1db};

    enum TPropertyKeys
        {
        KUsbManOtgEventProperty = 1,
        KUsbManServerProperty = 2,
        KUsbManConnectionStateProperty = 3
        };


RUsbOtgEventPublisher::RUsbOtgEventPublisher()
	{
//	TInt result = iProperty.Attach(KMyPropertyCat, KUsbManOtgEventProperty, EOwnerThread);
//	__ASSERT_DEBUG(result == KErrNone, User::Invariant());
	}


RUsbOtgEventPublisher::~RUsbOtgEventPublisher()
	{
//	iProperty.Close();
//	RProperty::Delete(KMyPropertyCat, KUsbManOtgEventProperty);
	}

/**
Publishing method

Publishes the USB event property event

@param aEvent
*/
void RUsbOtgEventPublisher::PublishEvent(TInt aEvent)
	{

	TInt result = iProperty.Attach(KMyPropertyCat, KUsbManOtgEventProperty, EOwnerThread);
	__ASSERT_DEBUG(result == KErrNone, User::Invariant());

    __USBOTGPRINT(_L("****************** PublishEvent...."));
    result = iProperty.Set(aEvent);
    __USBOTGPRINT2(_L("****************** PublishEvent %d %d"), aEvent, result);

	iProperty.Close();
	RProperty::Delete(KMyPropertyCat, KUsbManOtgEventProperty);
	}


//------------------------------------------------------------------------------
void RUsbManServerPublisher::PublishEvent()
	{
	TInt result = iProperty.Attach(KUsbmanSvrSid, KUsbManServerProperty, EOwnerThread);
	__ASSERT_DEBUG(result == KErrNone, User::Invariant());

    __USBOTGPRINT(_L("****************** PublishServer...."));
    result = iProperty.Set(1);
    __USBOTGPRINT1(_L("****************** PublishServer %d"), result);

	iProperty.Close();
	RProperty::Delete(KMyPropertyCat, KUsbManServerProperty);
	}

//------------------------------------------------------------------------------
void RUsbManConnectionStatePublisher::PublishEvent(TBool aActive)
	{
	TInt result = iProperty.Attach(KUsbmanSvrSid, KUsbManConnectionStateProperty, EOwnerThread);
	__ASSERT_DEBUG(result == KErrNone, User::Invariant());

    __USBOTGPRINT1(_L("****************** PublishConnectionState Active=%d...."), aActive);
    result = iProperty.Set(aActive);
    __USBOTGPRINT1(_L("****************** PublishConnectionState %d"), result);

	iProperty.Close();
	RProperty::Delete(KMyPropertyCat, KUsbManConnectionStateProperty);
	}
