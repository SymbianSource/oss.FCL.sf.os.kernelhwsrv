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
// e32test\misc\t_zip.cpp
// 
//

#include <e32test.h>
#include <f32file.h>
#include "crash_gzip.h"


TInt64 CrashTime() {return 0;}

RFs TheFs;
RFile TheOutFile;
				
class TCrashTestGzip : public MCrashGzip
	{
public:

	void SetOutput();
	virtual TBool Out(const TDesC8& aDes);
	virtual TInt32 GetMaxLength();
	};

RTest test(_L("T_ZIP"));

void TCrashTestGzip::SetOutput()
	{
	TInt r=TheOutFile.Replace(TheFs, _L("c:\\out.zip"),EFileWrite);
	test(r==KErrNone);
	Init();
	}


TInt TCrashTestGzip::Out(const TDesC8& aDes)
	{
	TInt r = TheOutFile.Write(aDes);
	test(r==KErrNone);
	return KErrNone;
	}


TInt32 TCrashTestGzip::GetMaxLength()
	{
	return -1;
	}


_LIT8(KText, "12345");


GLDEF_C TInt E32Main()
	{
	test.Title();

	test.Start(_L("Connect to file server"));
	TInt r=TheFs.Connect();
	test(r==KErrNone);

	test.Next(_L("Making zip file"));
	TCrashTestGzip zip;
	zip.SetOutput();
	zip.Write(KText);
	zip.FlushEnd();

	TheOutFile.Close();
	TheFs.Close();

	return KErrNone;
	}

