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
// Drive publishing classes for USB Mass Storage.
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef USBMSPUBLISHER_H
#define USBMSPUBLISHER_H

//----------------------------------------------------------------------------
/**
@internalTechnology

Publishes the EUsbMs property.
*/

class RUsbOtgEventPublisher
{
public:
	RUsbOtgEventPublisher();
	~RUsbOtgEventPublisher();

	void PublishEvent(TInt aEvent);

private:
	/**
	Publish and subscribe property for EUsbMsEvent property
	*/
	RProperty iProperty;
};


class RUsbManServerPublisher
{
public:
	void PublishEvent();

private:
	/**
	Publish and subscribe property for EUsbMsEvent property
	*/
	RProperty iProperty;
};


class RUsbManConnectionStatePublisher
{
public:
	void PublishEvent(TBool aActive);

private:
	/**
	Publish and subscribe property for EUsbMsEvent property
	*/
	RProperty iProperty;
};



#endif // USBMSPUBLISHER_H
