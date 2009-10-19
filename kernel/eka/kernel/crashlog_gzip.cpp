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
// e32\kernel\crashlog_gzip.cpp
// Wrapper class for using Zlib with the crash logger to create GZip 
// compatible output
// 
//

#include "crashlog_gzip.h"
#include <crashflash.h>


TCrashLogGzip::TCrashLogGzip() 
	: iFlash(NULL) {}

void TCrashLogGzip::SetOutput(CrashFlash* aFlash)
	{
	iFlash = aFlash;
	Init();
	}


TInt TCrashLogGzip::Out(const TDesC8& aDes)
	{
	iFlash->Write(aDes);
	return KErrNone;
	}


TInt32 TCrashLogGzip::GetMaxLength()
	{
	return iFlash->GetOutputLimit();
	}
