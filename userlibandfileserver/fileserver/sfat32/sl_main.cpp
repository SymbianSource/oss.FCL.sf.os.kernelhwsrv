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
// f32\sfat\sl_main.cpp
// 
//

#include "sl_std.h"

GLDEF_C void Fault(TFault aFault)
//
// Report a fault in the fat file system.
//
	{

	User::Panic(_L("FAT_FSY"),aFault);
	}

extern "C" {
EXPORT_C CFileSystem* CreateFileSystem()
//
// Create a new file system
//
	{

	return(CFatFileSystem::New());
	}
}

