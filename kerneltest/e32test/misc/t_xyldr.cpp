// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_xyldr.cpp
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("XY LOADER"));

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Load digitiser driver"));
	TInt r=User::LoadPhysicalDevice(_L("EXYIN"));
	test.Printf(_L("Return code %d\n"),r);
	test.End();
	return 0;
	}
