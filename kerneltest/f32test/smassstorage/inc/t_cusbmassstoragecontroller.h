// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Class declaration for CUsbMassStorageController.
// 
//

/**
 @file
 @internalTechnology
*/
 
#ifndef __T_CUSBMASSSTORAGECONTROLLER_H__
#define __T_CUSBMASSSTORAGECONTROLLER_H__

#include <e32base.h>
#include "cusbmassstorageserver.h"
#include "protocol.h"
#include "massstoragedebug.h"

/**
 Mass Storage Controller class.
 Encapsulates the drive manager, transport and protocol for USB Mass Storage.
 Its main purpose is to instantiate and initialize these objects.
 */
class CUsbMassStorageController : public CBase
	{
	public:
    static CUsbMassStorageController* NewL();
	~CUsbMassStorageController();
	TInt Start(TMassStorageConfig& aConfig);
	TInt Stop();
	};

#endif //__T_CUSBMASSSTORAGECONTROLLER_H__
