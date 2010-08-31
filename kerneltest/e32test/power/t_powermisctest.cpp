// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\power\t_powermisctest.cpp
// 
//

#include <e32test.h>
#include <e32hal.h>
#include "d_powermisctest.h"
#include <e32power.h>
#include <e32svr.h>
#include <hal.h>
#include <u32hal.h>

_LIT(KLddFileName, "D_POWERMISCTEST.LDD");

RLddTest1 ldd;
GLDEF_D RTest test(_L("T_POWERMISCTEST"));

void DoTests()
	{
	TInt r = KErrNone;

	test.Printf(_L("Loading logical device \n"));
	r=User::LoadLogicalDevice(KLddFileName);
	test(r == KErrNone);

	test.Printf(_L("Opening of logical device\n"));
	r = ldd.Open();
	test(r == KErrNone);

	test.Printf(_L("Start tests\n"));
	r = ldd.Test();
	test(r == KErrNone);

	test.Printf(_L("Closing the channel\n"));
	ldd.Close();

	test.Printf(_L("Freeing logical device\n"));
	r = User::FreeLogicalDevice(KLddFileName);;
	test(r==KErrNone);

	}

GLDEF_C TInt E32Main()
	{
	test.Start(_L("Power misc tests"));
	const TInt numCpus = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	if(numCpus > 1)
		{
		DoTests();
		}
	else
		{
		test.Printf(_L("Not supported in unicore.\n"));
		}
	test.End();
	test.Close();	

	return(KErrNone);
	}
