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
// CUsbMassStorageController implementation.
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32test.h>
#include "t_cusbmassstoragecontroller.h"
#include "massstoragedebug.h"
#include "mstestdatadef.h"

GLDEF_D RTest test(_L("t_msunittest"));

CUsbMassStorageController* CUsbMassStorageController::NewL()
    {
    __PRINT(_L("CUsbMassStorageController::CUsbMassStorageController\n"));
    return new (ELeave) CUsbMassStorageController;
    }

/**
 Destructor
 */
CUsbMassStorageController::~CUsbMassStorageController()
	{
	__PRINT(_L("CUsbMassStorageController::~CUsbMassStorageController\n"));
	test.Close();
	}

/**
 Starts the transport and initializes the protocol.
 @param aConfig Reference to Mass Storage configuration data
 */
TInt CUsbMassStorageController::Start(TMassStorageConfig& aConfig)
	{
	__PRINT(_L("CUsbMassStorageController::Start\n"));
	test.Printf(_L("VendorId: %S\n"), &aConfig.iVendorId);
	test.Printf(_L("ProudcdId: %S\n"), &aConfig.iProductId);
	test.Printf(_L("ProudcdRev: %S\n"), &aConfig.iProductRev);
/*	
	TBuf<8> vendorId(_L("vendor"));
	TBuf<16> productId(_L("product"));
	TBuf<4> productRev(_L("Rev"));
*/	
	test(aConfig.iVendorId.Compare(t_vendorId) == 0
		&& aConfig.iProductId.Compare(t_productId) == 0
		&& aConfig.iProductRev.Compare(t_productRev) == 0);
		
	return KErrNone;	// return value not used
	}

/**
 Stops the transport.
 */
TInt CUsbMassStorageController::Stop()
	{
	__PRINT(_L("CUsbMassStorageController::Stop\n"));
	test(ETrue);
	
	return KErrNone; 	// return value not used
	}

