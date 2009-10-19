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
// e32test\earlyextension\earlyextension.h
// 
//

#ifndef __EARLYEXTENSION_H__
#define __EARLYEXTENSION_H__

#define LATE_EXTENSION_PRIORITY		0
#define EARLY_EXTENSION_PRIORITY	LATE_EXTENSION_PRIORITY+1


/** Class containing export function to get the time stamp got during init3 entry point and also
	a pointer to store the time stamp. */
class TestEarlyExtension
	{
public:
	TestEarlyExtension():iTime(NULL) {} //Constructor
	IMPORT_C static void GetTimeStamp(TTimeK& aTime);
public:
	TTimeK *iTime;
	};

#endif //__EARLYEXTENSION_H__

