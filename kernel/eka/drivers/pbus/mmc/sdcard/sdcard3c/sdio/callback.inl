/*
* Copyright (c) 2003 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/


#ifndef __CALLBACK_INL__
#define __CALLBACK_INL__

/**	
@publishedPartner
@released 

Constructs an SDIO function callback object to notify clients of state changes.
*/
inline TSDIOFunctionCallback::TSDIOFunctionCallback()
	: iCallbackFn(NULL),
	  iPtr(NULL),
	  iBusState(EPBusCardAbsent)
	{}

/**	
@publishedPartner
@released 

Destructs an SDIO function callback object.
*/
inline TSDIOFunctionCallback::~TSDIOFunctionCallback()
	{
	iCallBack.Remove();
	}

/**	
@publishedPartner
@released 

Constructs a SDIO function callback object to notify clients of state changes.

@param aCallbackFn The function to call when a state changes
*/
inline TSDIOFunctionCallback::TSDIOFunctionCallback(TSDIOCallbackFunction aCallbackFn)
	: iCallbackFn(aCallbackFn),
	  iPtr(NULL),
	  iBusState(EPBusCardAbsent)
	{}

/**	
@publishedPartner
@released 

Constructs a SDIO function callback object to notify clients of state changes.

@param aCallbackFn The function to call when a state changes
@param aPtr A pointer to data to pass through to the static function
*/
inline TSDIOFunctionCallback::TSDIOFunctionCallback(TSDIOCallbackFunction aCallbackFn, TAny *aPtr)
	: iCallbackFn(aCallbackFn),
	  iPtr(aPtr),
	  iBusState(EPBusCardAbsent)
	{}

inline TInt TSDIOFunctionCallback::CallBack(TSDIOFunctionCallbackReason aReason) const
/**
*/
	{ return(iCallbackFn ? (*iCallbackFn)(iPtr, aReason) : KErrNotSupported); }


#endif	// #ifndef __CALLBACK_INL__

