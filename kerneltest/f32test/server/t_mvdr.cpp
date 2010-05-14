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
// f32test\server\t_mvdr.cpp
// Manual test creates a set of directories and moves them from one to another
// Checks rename operation on directories.  Removable media should be checked
// afterwards with disk verification utility.
// 
//


#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <f32file.h>

// -------- local test data --------

RTest test(_L("t_mvdr"));

RFs TheFs;
RFile TheFile;

#ifdef __WINS__
_LIT(KSessionPath, "X:\\");
#else
_LIT(KSessionPath, "D:\\");
#endif

GLDEF_C TInt E32Main()
    {
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
 	__UHEAP_MARK;

	test.Title();
	test.Start(_L("Starting T_MVDR"));

	test(TheFs.Connect() == KErrNone);

	test(TheFs.SetSessionPath(KSessionPath) == KErrNone);

	CFileMan *fm = 0;
	TRAPD(r, fm = CFileMan::NewL(TheFs));
	test_KErrNone(r);

	const TInt KMaxDirs = 32;

	TInt i;
	for (i = 0; i < KMaxDirs; ++i)
		{
		TBuf<16> dirName(KSessionPath);
		dirName.AppendFormat(_L("testdir.%03x\\"), i);
		test(TheFs.MkDir(dirName) == KErrNone);
		}

	TInt ctr = 0x0ff;
	for (i = 0; i < KMaxDirs; ++i)
		{
		test.Printf(_L("moving directories from testdir.%03x\r"), i);

		TInt j;
		for (j = 0; j < KMaxDirs; ++j)
			{
			TBuf<32> srcDirName(KSessionPath);
			srcDirName.AppendFormat(_L("testdir.%03x\\movedir.%03x\\"), i, ++ctr);
			test(TheFs.MkDir(srcDirName) == KErrNone);
			
			TBuf<32> dstDirName(KSessionPath);
			dstDirName.AppendFormat(_L("testdir.%03x\\movedir.%03x\\"), j, ++ctr);
			srcDirName.SetLength(srcDirName.Length() - 1);
			dstDirName.SetLength(dstDirName.Length() - 1);
			r = fm->Move(srcDirName, dstDirName);
			test_KErrNone(r);
			}
		}

	delete fm;

	TheFs.Close();
	test.End();
	test.Close();

	__UHEAP_MARKEND;
	delete cleanup;

	return KErrNone;
    }

