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
// e32test\earlyextension\t_testextension.cpp
// 
//

#include <e32test.h>
#include <hal.h>
#include "d_wsdextension.h"

RLddWsdExtension lddChan;
GLDEF_D RTest test(_L("LDD tests"));
_LIT(KLddFileName, "D_WSDEXTENSION.LDD");

GLDEF_C TInt E32Main()
    {
	test.Title();
	test.Start(_L("Testing WSD extension...\n"));
	//Load logical device
	TInt r = User::LoadLogicalDevice(KLddFileName);
	test((r == KErrNone) || (r == KErrAlreadyExists));
	//Open the channel
	r = lddChan.Open();
    test(r==KErrNone || r==KErrAlreadyExists);
	
    TInt data=0;
    r = lddChan.Test_getStaticData(data);
    test(r == KErrNone);
	test(data == 0xEFEFEFEF);
    test.Printf(_L("Static Data from extension as expected!!!\n"));
	test.Printf(_L("Closing the channel\n"));
	
	lddChan.Close();

	test.Printf(_L("Freeing logical device\n"));
	r = User::FreeLogicalDevice(KLddFileName);
	test(r==KErrNone);
	User::After(100000);
	test.End();
	test.Close();
	return r;
	}
