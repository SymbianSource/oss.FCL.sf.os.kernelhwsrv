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
// e32\kernel\crashdebug_gzip.cpp
// Wrapper class for using Zlib with the crash logger to create GZip 
// compatible output
// 
//

#include "crashdebug_gzip.h"
#include <kernel/monitor.h>


void TCrashDebugGzip::SetOutput(CrashDebugger* aDebugger)
	{
	iDebugger = aDebugger;
	Init();
	}


TInt TCrashDebugGzip::Out(const TDesC8& aDes)
	{
	iDebugger->Print(aDes);
	return KErrNone;
	}


TInt32 TCrashDebugGzip::GetMaxLength()
	{
	return -1;
	}

