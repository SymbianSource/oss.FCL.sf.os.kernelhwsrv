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
// e32\ewsrv\ws_utl.cpp
// 
//

#include "ws_std.h"

GLDEF_C void Fault(TWsFault aFault)
//
// Fault the window server.
//
	{

	User::Panic(_L("WServ fault"),aFault);
	}
