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
// Definitions required for RUsbMassStorage
// 
//

/**
 @file
*/

#ifndef __RUSBMASSSTORAGE_H__
#define __RUSBMASSSTORAGE_H__

#include <e32std.h>
#include "usbmsshared.h"

const TUid KUsbMsSvrUid = {0x101f7774};

#ifdef __USBMS_NO_PROCESSES__

const TUint KUsbMsStackSize =   0x3000;			//  12KB
const TUint KUsbMsMinHeapSize = 0x1000;		    //   4KB
const TUint KUsbMsMaxHeapSize = 0x40000;		// 256KB

_LIT(KUsbMsImg, "usbmsserver");

#else

_LIT(KUsbMsImg1, "usbmsserver.exe");
_LIT(KUsbMsImg, "z:\\system\\programs\\usbmsserver.exe");

#endif //__USBMS_NO_PROCESSES__

#endif //__RUSBMASSSTORAGE_H__
