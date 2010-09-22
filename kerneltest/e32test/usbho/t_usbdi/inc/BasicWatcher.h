#ifndef __BASIC_WATCHER_H
#define __BASIC_WATCHER_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* @file BasicWatcher.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <d32usbdi.h>
#include "testdebug.h"

namespace NUnitTesting_USBDI
	{


/**
This class watches for asynchronous completions and calls back
*/
class CBasicWatcher : public CActive
	{
public:
	CBasicWatcher(const TCallBack& aCallBack,TInt aPriority=EPriorityStandard);
	virtual ~CBasicWatcher();
	
	void CompleteNow(TInt aCompletionCode = KErrNone);
	void StartWatching();
	
protected: // From CActive
	void DoCancel();
	void RunL();
	TInt RunError(TInt aError);	

private:
	TCallBack iCallBack;
	TInt iCompletionCode;
	};





/**
This class describes a watcher for resumptions of interfaces
*/
class CInterfaceWatcher : public CActive
	{
public:
	/**
	Constructor, build 
	@param aInterface the usb interface to suspend  
	@param aCallBack the call back object to call once a resumption signal has happened
	*/
	CInterfaceWatcher(RUsbInterface& aInterface,const TCallBack& aCallBack);

	/**
	Destructor
	*/
	~CInterfaceWatcher();

	/**
	Suspend the interface and watch for resumtions
	*/
	void SuspendAndWatch();


	/**
	Obtains the most recent completion code for the interface resumption
	asynchronous action
	@return the completion error code
	*/
	TInt CompletionCode() const;


protected: // From CActive

	/**
	*/
	void DoCancel();

	
	/**
	*/
	void RunL();
	
	/**
	*/
	TInt RunError();

private:

	/**
	The USB interface resource
	*/
	RUsbInterface& iUsbInterface;

	/**
	*/
	TCallBack iResumeCallBack;

	/**
	*/
	TInt iCompletionCode;
	};


	}
	
#endif
