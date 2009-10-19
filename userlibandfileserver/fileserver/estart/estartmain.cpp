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
// f32\estart\estartmain.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include "estart.h"

GLDEF_C TInt E32Main()
	{
	
	TFSStartup fsStart;
	fsStart.Init();
	
	fsStart.Run();

	fsStart.StartSystem();
			
	fsStart.Close();
	return(0);
	}


