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

RFs gFs;
RFile TheOutFile;
CFileMan* gFileMan;
_LIT(KZipFile,"c:\\out.zip");
				
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
	TInt r=TheOutFile.Replace(gFs, KZipFile, EFileWrite);
	test(r==KErrNone);
	TBool bOk=Init();
	test(bOk);
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
const TUint32 KTextSize = 5;

GLDEF_C TInt E32Main()
	{
	test.Title();

	test.Start(_L("Connect to file server"));

	CTrapCleanup* ct = CTrapCleanup::New();
	test(ct!=0);
	test(gFs.Connect()==KErrNone);
	TRAPD(r, gFileMan=CFileMan::NewL(gFs));
	test(r==KErrNone);

	test.Next(_L("Making zip file"));
	TCrashTestGzip zip;
	zip.SetOutput();
	zip.Write(KText);
	zip.FlushEnd();

	// get the number of uncompressed bytes and check that it matches the string
	TUint32 uncompressedBytes=zip.GetDataCompressed();
	test(uncompressedBytes==KTextSize);

	TheOutFile.Close();

	test.Next(_L("Re-open zip file and check size"));
	// Check that file exists
	r=TheOutFile.Open(gFs, KZipFile, EFileRead);
	test(r==KErrNone);
	
	//check size
	TInt size=0;
	r=TheOutFile.Size(size);
	test(r==KErrNone && size>0);

	TheOutFile.Close();

	// Cleanup
	gFileMan->Delete(KZipFile);
	delete gFileMan;
	gFs.Close();
	delete ct;
	test.End();

	return KErrNone;
	}

