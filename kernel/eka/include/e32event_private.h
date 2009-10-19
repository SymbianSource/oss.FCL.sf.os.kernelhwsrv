// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32event_private.h
// 
//

#ifndef __E32EVENT_PRIVATE_H__
#define __E32EVENT_PRIVATE_H__
#include <e32cmn.h>
#include <e32cmn_private.h>

/**
@publishedPartner
@released

Encapsulates information about a device's display screen.
*/
class TScreenInfoV01
	{
public:
	TBool iWindowHandleValid; /**< Indicates whether the window handle is valid.*/
	TAny *iWindowHandle;      /**< The window handle.*/
	TBool iScreenAddressValid;/**< Indicates whether the screen address is valid.*/
	TAny *iScreenAddress;     /**< The linear address of the screen.*/
	TSize iScreenSize;        /**< The size of the screen.*/
	};
//
#endif

