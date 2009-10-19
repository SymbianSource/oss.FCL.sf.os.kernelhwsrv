// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\emul\t_jit.cpp
// Overview:
// Test that User::JustInTime() initially corresponds to the JustInTime setting
// in epoc.ini.
// This is a manual test, and must be run by hand with an argument of 0 or 1 to
// specify the expected result, eg:
// t_jit -- 1
// t_jit -DJustInTime=none -- 0
// API Information:
// Emulator
// Details:
// Test User::JustInTime corresponds to command line argument.
// Platforms/Drives/Compatibility:
// Emulator.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

RTest test(_L("t_jit"));

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Just in time"));

	test(User::CommandLineLength() == 1);
	TBuf<1> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TInt val;
	test(lex.Val(val) == KErrNone);
	test (val == 0 || val == 1);
	TBool expected = val == 1;
	
	test(User::JustInTime() == expected);
	
	test.End();
	test.Close();
	return KErrNone;
	}
