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
 @internalComponent
*/

#ifndef USBDIUTILS_H
#define USBDIUTILS_H

#include <d32usbdi_errors.h>


NONSHARABLE_CLASS(UsbdiUtils)
	{
public:
	static void Panic(UsbdiPanics::TUsbdiPanics aPanic);
	static void Fault(UsbdiFaults::TUsbdiFaults aFault);
	};

#endif // USBDIUTILS_H
