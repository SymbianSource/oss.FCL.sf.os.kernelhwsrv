// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\testexclusions.h
// Gets the test exclusion property which is set by reading a file provided by base port and 
// based on this property some tests can be excluded 
// 
//

#ifndef __TESTEXCLUSIONS_H__
#define __TESTEXCLUSIONS_H__

#include <e32svr.h>
#include <u32hal.h>
#include <e32property.h> 
#include <e32uid.h>
#include <f32file.h>

_LIT(KLitPlatformTestExclusionFile, "Z:\\sys\\data\\platformtestexclusion.txt");

// Bit masks to disable different tests

// To disable invoking PowerController::PowerDown  
const TUint KDisableControllerShutdown = 0x1;

TInt GetExclusionFromFile(TUint& aTestExclusion)
	{
	RFs theFs;
	TInt r = KErrNone;
	r = theFs.Connect();
    if (r != KErrNone)
		return r;

	RFile file;
	TFileName filename(KLitPlatformTestExclusionFile);
	r = file.Open(theFs, filename, EFileRead);
	if (r == KErrNone)
		{
		TBuf8<8> data;
		file.Read(data);
		TLex8 lexData(data);
		r = lexData.Val(aTestExclusion, EHex);
		file.Close();
		} 
	
	theFs.Close();
	return r;
	}


TInt GetTestExclusionSettings(TInt& aTestExclusion)
	{
	TInt r =  RProperty::Get(KUidSystemCategory, KPlatformTestExclusionKey, aTestExclusion);
	if (r != KErrNotFound)
		return r;

    _LIT_SECURITY_POLICY_PASS(KTestPropPolicy);
    r = RProperty::Define(KUidSystemCategory, KPlatformTestExclusionKey, RProperty::EInt, KTestPropPolicy, KTestPropPolicy);
	if (r != KErrNone)
		return r;


	TUint testExclusion = 0;
	r = GetExclusionFromFile(testExclusion);
	if ((r != KErrNotFound) && (r != KErrNone)) // All platforms need not have test exclusions file defined
		return r;

	aTestExclusion = testExclusion;
	r = RProperty::Set(KUidSystemCategory, KPlatformTestExclusionKey, testExclusion);
	return r;
	}

#endif // __TESTEXCLUSIONS_H__

