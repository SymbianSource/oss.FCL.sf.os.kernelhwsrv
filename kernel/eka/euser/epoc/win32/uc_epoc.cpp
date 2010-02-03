// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\epoc\win32\uc_epoc.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32wins.h>

//SL: Empty on FCL ?

GLDEF_C TInt E32Main()
	{
	//What do we do then

	CBase* base=new(ELeave) CBase();
	delete base;

	return KErrNone;
	}

TInt main()
	{
	return E32Main();
	}