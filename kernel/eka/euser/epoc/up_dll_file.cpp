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
// e32\euser\epoc\up_dll_file.cpp
// This file contains DLL stub functions
// 
//

#include "up_std.h"
#include <e32svr.h>




/**
Gets the filename of the currently executing DLL.

@param aFileName  On return from this function, contains the filename
                  of the currently executing DLL.
*/
void Dll::FileName(TFileName &aFileName)
//
// Return the filename of this dll
//
	{

	UserSvr::DllFileName(MODULE_HANDLE, aFileName);
	}
