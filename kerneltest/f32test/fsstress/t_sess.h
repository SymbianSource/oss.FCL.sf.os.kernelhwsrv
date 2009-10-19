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
// f32test\fsstress\t_sess.h
// 
//

#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <e32math.h>
#include <f32dbg.h>

GLDEF_D const TInt KMaxParses=7;
GLDEF_D const TInt KHeapSize=0x2000;
GLREF_C void TurnAllocFailureOff();
GLREF_C void TurnAllocFailureOn();
GLREF_C void ReportCheckDiskFailure(TInt aRet);
GLREF_D RTest test;
GLREF_D TFileName gTestSessionPath;
GLREF_D TInt gAllocFailOff;
GLREF_D TInt gAllocFailOn;

#if defined(_DEBUG)
#define SetAllocFailure(a) SetAllocFailure(a)
#else
#define SetAllocFailure(a) IsRomAddress(NULL)
#define KAllocFailureOn 0
#define KAllocFailureOff 0
#endif


struct SParse
	{
	const TText* src;
	const TText* rel;
	const TText* def;
	const TText* fullName;
	const TText* drive;
	const TText* path;
	const TText* name;
	const TText* ext;
	};

struct SParseServer
	{
	const TText* src;
	const TText* rel;
	const TText* fullName;
	const TText* drive;
	const TText* path;
	const TText* name;
	const TText* ext;
	};


class TSessionTest
	{
public:
	TSessionTest() {};
	TSessionTest(RFs& aFs):	iFs(aFs){};	
	void Initialise(RFs& aFs);
	void RunTests();
	void testDriveList();
	void testDriveInfo();
	void testVolumeInfo();
	void testPath();
	void testInitialisation();
	void testSubst();
	void CopyFileToTestDirectory();
	void MakeAndDeleteFiles();
	void FillUpDisk();
	void testSetVolume();
	void CreateTestDirectory(const TDesC& aTestPath);
	void DeleteTestDirectory();
	TInt CurrentDrive();
private:
	RFs iFs;
	};

