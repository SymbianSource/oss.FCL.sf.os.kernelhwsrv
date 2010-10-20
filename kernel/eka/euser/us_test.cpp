// Copyright (c) 1994-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\us_test.cpp
// 
//

#include "us_std.h"
#include <e32test.h>

#if defined(test)
#undef test
#endif

#include <e32svr.h> 

void RTest::CheckConsoleCreated()
	{
	// Check that the console has been created.
	if (iConsole == NULL)
		{
		TRAPD(r, iConsole = Console::NewL(iTitle, TSize(KConsFullScreen, KConsFullScreen)))
		__ASSERT_ALWAYS(r == KErrNone, ::Panic(ERTestCreateConsole));
		}
	}

void RTest::DisplayLevel()
	{
	// Display the current level string.
	TBuf<0x100> aBuf(_L("RTEST: Level "));
	for (TInt ii = 1; ii < iLevel; ii++)
		{
		if (ii > 1)
			{
			aBuf.AppendFormat(_L(".%02d"), iStack[ii]);
			}
		else
			{
			aBuf.AppendFormat(_L(" %03d"), iStack[ii]);
			}
		}
	if (iLevel > 1)
		{
		aBuf.AppendFormat(_L(".%02d "), iTest);
		}
	else
		{
		aBuf.AppendFormat(_L(" %03d "), iTest);
		}

	Printf(aBuf);
	}




/**
Constructor.

@param aTitle           A title describing this use of RTest.
                        This is also referred to as the console title.
@param aThrowaway       Not used.
@param anOtherThrowaway Not used.
*/
EXPORT_C RTest::RTest(const TDesC &aTitle,TInt /* aThrowaway */,const TText* /* anOtherThrowaway */)
	: iTest(0), iLevel(0), iLogging(ETrue), iConsole(NULL), iTitle(aTitle)
	// Constructor
	// There is a #define test(x) test(x, __LINE__) in e32test.h to pass on line info of failing tests,
	// This depends upon the user naming their RTest object test, but if they do this then an extra
	// parameter aThrowaway must be added to the constructor
	{}



/**
Constructor.

@param aTitle     A title describing this use of RTest.
                  This is also referred to as the console title.
@param aThrowaway Not used.
*/
EXPORT_C RTest::RTest(const TDesC &aTitle, TInt /* athrowaway */)
	: iTest(0), iLevel(0), iLogging(ETrue), iConsole(NULL), iTitle(aTitle)
	// Constructor
	// There is a #define test(x) test(x, __LINE__) in e32test.h to pass on line info of failing tests,
	// This depends upon the user naming their RTest object test, but if they do this then an extra
	// parameter aThrowaway must be added to the constructor
	{}




/**
Constructor.

@param aTitle A title describing this use of RTest.
              This is also referred to as the console title.
*/
EXPORT_C RTest::RTest(const TDesC &aTitle)
	: iTest(0), iLevel(0), iLogging(ETrue), iConsole(NULL), iTitle(aTitle)
	// Constructor
	{}




/**
Closes the console and frees any resources acquired.
*/
EXPORT_C void RTest::Close()
	{
	// Close the console.
	delete iConsole;
	iConsole=NULL;
	}




/**
Prints out the console title and version number.

The format of the text is:

@code
RTEST TITLE: XXX YYY
Epoc/32 YYY
@endcode

where XXX is the console title, and YYY is the version number,
formatted as described by TVersion::Name().

@see TVersion::Name()
@see RTest::Printf() 
*/
EXPORT_C void RTest::Title()
	{
	// Print out the program title and version number.
	TVersion v(KE32MajorVersionNumber, KE32MinorVersionNumber, KE32BuildVersionNumber);
	TBuf<16> vName=v.Name();
	Printf(_L("RTEST TITLE: %S %S\n"), &iTitle, &vName);
	vName=User::Version().Name();
	Printf(_L("Epoc/32 %S\n"), &vName);
	}




/**
Marks the start of a set of tests.

Note that sets of tests can be nested.

A call to this function must be matched by a call to RTest::End() to mark
the end of this set of tests.

@param aHeading A heading describing the set of tests; this is
                printed at the console.
                
@see RTest::End()                
*/
EXPORT_C void RTest::Start(const TDesC &aHeading)
	{
	// Print out the heading and nest the level.
	Push();
	Next(aHeading);
	}




/**
Marks the start of the next test.

@param aHeading A heading describing the test; this
                is printed at the console. This function is also
                called by Start(), which passes the text that describes
                the set of tests.
                
@see RTest::Start()                
*/
EXPORT_C void RTest::Next(const TDesC &aHeading)
	{
	// Print out the heading and nest the level.
	iTest++;
	iCheck = 0;
	DisplayLevel();
	Printf(_L("Next test - %S\n"), &aHeading);
	}




/**
Checks the result of a condition and, if this is false, prints
a failure message at the console and raises a panic.

Before checking the condition passed in, the operator increments
a check number. This is a value that is set to zero at the start of a test
and is incremented by this operator (and by all variants of it). It identifies
the check being made within the current test.
This value is printed on a failure message.

Typically, the operator is called, passing a test condition, for example:

@code
RTest test(... heading text...,line number... file name)
TInt r;
...some operation to be tested that returns a value in r...
test(r==KErrNone);
@endcode

The failure message has the format:

@code
: FAIL : XXX failed check N in FFF at line Number: M
RTEST: Checkpoint-fail
@endcode

where XXX is the console title, N is the check number, FFF is the filename,
and M is the line number passed in.

@param aResult   The condition being tested.
                 This is interpreted as a true or false value.
@param aLineNum  A line number that is printed in the failure message if
                 the condition being tested is false.
@param aFileName A file name that is printed in the failure message if
                 the condition being tested is false.
                 
@panic USER 84 if the condition being tested is false.

@see RTest::Next()
@see RTest::Start()
*/
EXPORT_C void RTest::operator()(TInt aResult, TInt aLineNum, const TText* aFileName)
	{
	// Test a condition.
	iCheck++;
	if (!aResult)
		{
		RDebug::Printf(": FAILING : failed check at line number %d", aLineNum);
		DisplayLevel();
		Printf(_L(": FAIL : %S failed check %d in %s at line number %d\n"),
			   &iTitle, iCheck, aFileName, aLineNum);
		Panic(_L("Checkpoint-fail\n"));
		if (!iLogging)
			Getch();
		}
	}




/**
Checks the result of a condition and, if this is false, prints
a failure message at the console and raises a panic.

Before checking the condition passed in, the operator increments
a check number. This is a value that is set to zero at the start of a test
and is incremented by this operator (and by all variants of it). It identifies
the check being made within the current test.
This value is printed on the failure message.

Typically, the operator is called, passing a test condition, for example:

@code
RTest test(... heading text...,line number)
TInt r;
...some operation to be tested that returns a value in r...
test(r==KErrNone);
@endcode

The failure message has the format:

@code
: FAIL : XXX failed check N at line Number: M
RTEST: Checkpoint-fail
@endcode

where XXX is the console title, N is the check number, and M is
the line number passed in.

@param aResult  The condition being tested.
                This is interpreted as a true or false value.
@param aLineNum A line number that is printed in the failure message if
                the condition being tested is false.               

@panic USER 84 if the condition being tested is false.

@see RTest::Next()
@see RTest::Start()
*/
EXPORT_C void RTest::operator()(TInt aResult,TInt aLineNum)
	{
	// Test a condition.
	iCheck++;
	if (!aResult)
		{
		RDebug::Printf(": FAILING : failed check at line Number: %d", aLineNum);
		DisplayLevel();
		Printf(_L(": FAIL : %S failed check %d at line Number: %d\n"), &iTitle, iCheck, aLineNum);
		Panic(_L("Checkpoint-fail\n"));
		if (!iLogging)
			Getch();
		}
	}  




/**
Checks the result of a condition and, if this is false, prints
a failure message at the console and raises a panic.

Before checking the condition passed in, the operator increments
a check number. This is a value that is set to zero at the start of a test
and is incremented by this operator (and by all variants of it). It identifies
the check being made within the current test.
This value is printed on the failure message.

Typically, the operator is called, passing a test condition, for example:

@code
RTest test(... heading text...)
TInt r;
...some operation to be tested that returns a value in r...
test(r==KErrNone);
@endcode

The failure message has the format:

@code
: FAIL : XXX failed check N
RTEST: Checkpoint-fail
@endcode

where XXX is the console title, and N is the check number.

@param aResult The condition being tested.
               This is interpreted as a true or false value.

@panic USER 84 if the condition being tested is false.

@see RTest::Next()
@see RTest::Start()
*/
EXPORT_C void RTest::operator()(TInt aResult)
	{
	// Test a condition.
	iCheck++;
	if (!aResult)
		{
		RDebug::Printf(": FAILING : failed check\n");
		DisplayLevel();
		Printf(_L(": FAIL : %S failed check %d\n"), &iTitle, iCheck);
		Panic(_L("Checkpoint-fail\n"));
		if (!iLogging)
			Getch();
		}
	}




/**
Ends the current set of tests.

If this set of tests is not nested within another set,
then a message reporting success is written to
the console.

@panic USER 84 if there was no matching call to RTest::Start(),
               i.e. more calls to End() have been made than calls to Start().

@see RTest::Start()
*/
EXPORT_C void RTest::End()
	{
	// End the current level of tests.
	if (TInt(iLevel-1) < 0)
		{
		Panic(_L("End() without matching Start()\n"));
		}

	Pop();

	if (iLevel == 0)
		{
		Printf(_L("RTEST: SUCCESS : %S test completed O.K.\n"), &iTitle);
		if (!iLogging)
			Getch();
		}
	}




/**
Prints an error message and an error code,
and raises a USER 84 panic.

@param anError The error code.
@param aFmt    A format list.
@param ...     A variable number of parameters.
*/
EXPORT_C void RTest::Panic(TInt anError,TRefByValue<const TDesC> aFmt,...)
	{
	// Print an error message, an error and then panic.
	TestOverflowTruncate overflow;
	VA_LIST list;
	VA_START(list, aFmt);
	TBuf<0x100> aBuf;
	aBuf.AppendFormat(_L("RTEST: "));
	// coverity[uninit_use_in_call]
	aBuf.AppendFormatList(aFmt, list, &overflow);
	aBuf.AppendFormat(_L(" Failed with error %d\n"), anError);
	Printf(aBuf);
	if (!iLogging)
		Getch();
	::Panic(ERTestFailed);
	}




/**
Prints an error message, and raises a USER 84 panic.

@param aFmt    A format list.
@param ...     A variable number of parameters.
*/
EXPORT_C void RTest::Panic(TRefByValue<const TDesC> aFmt,...)
	{
	// Print an error message and then panic.
	TestOverflowTruncate overflow;
	VA_LIST list;
	VA_START(list, aFmt);
	TBuf<0x100> aBuf;
	aBuf.AppendFormat(_L("RTEST: "));
	// coverity[uninit_use_in_call]
	aBuf.AppendFormatList(aFmt, list, &overflow);
	Printf(aBuf);
	if (!iLogging)
		Getch();
	::Panic(ERTestFailed);
	}



_LIT(KLitNL, "\n");
_LIT(KLitCRNL, "\r\n");
/**
Prints text to the console.

If the logging flag is set, the string
is also written to the debug output as represented by an RDebug object.

@param aFmt    A format list.
@param ...     A variable number of parameters.

@see RTest::SetLogged()
@see Rtest::Logged()
@see RDebug
*/
EXPORT_C void RTest::Printf(TRefByValue<const TDesC> aFmt,...)
	{
	// Print to a console screen.
	TestOverflowTruncate overflow;
	VA_LIST list;
	VA_START(list, aFmt);
	TBuf<0x100> buf;
	// coverity[uninit_use_in_call]
	buf.AppendFormatList(aFmt, list, &overflow);
	CheckConsoleCreated();
	iConsole->Write(buf);
	
	if (iLogging)
		{
		TPtrC ptr(buf);
		TInt newline;
		while ((newline = ptr.Locate('\n')) != KErrNotFound)
			{
			RDebug::RawPrint(ptr.Left(newline));
			if (newline==0 || ptr[newline-1]!='\r')
				RDebug::RawPrint(KLitCRNL); // bare nl, replace with crnl
			else
				RDebug::RawPrint(KLitNL); // crnl, already printed cr
			if (newline+1<ptr.Length())
				ptr.Set(ptr.Mid(newline+1));
			else
				return; // newline was end of string
			}
		RDebug::RawPrint(ptr);
		}
	}




/**
Gets an input key stroke.

@return The input key code.
*/
EXPORT_C TKeyCode RTest::Getch()
	{
	// Get a key from the console.
	CheckConsoleCreated();
	return(iConsole->Getch());
	}



EXPORT_C TInt RTest::CloseHandleAndWaitForDestruction(RHandleBase& aH)
	{
	TRequestStatus s;
	aH.NotifyDestruction(s);
	aH.Close();
	TUint32 initial = User::NTickCount();
	TInt r = KErrNone;
	if (s == KErrNoMemory)
		r = KErrNoMemory;
	TInt factor = UserSvr::HalFunction(EHalGroupVariant, EVariantHalTimeoutExpansion, 0, 0);
	if (factor<=0)
		factor = 1;
	if (factor>1024)
		factor = 1024;
	TUint32 timeout = 5000 * (TUint32)factor;
	TUint32 period = 1000 * (TUint32)factor;
	while (s == KRequestPending)
		{
		TUint32 now = User::NTickCount();
		if ((now - initial) > timeout)
			{
			User::CancelMiscNotifier(s);
			r = KErrTimedOut;
			break;
			}
		User::AfterHighRes(period);
		}
	User::WaitForRequest(s);

	// There is a potential race condition, if the calling thread runs on a different CPU
	// than the Supervisor thread, which can result in this thread being signaled
	// before the RHandleBase gets fully closed (see DChangeNotifier::Complete()).
	// For that reason, wait until Supervisor thread gets to idle, which is the indication
	// that Complete() function has returned (DThread::Close() was called releasing memory),
	// in the case that Kernel heap was checked just after current method returns.
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)timeout, NULL);
	return r;
	}


/**
This should be called before using the __KHEAP_MARK macro in tests that check for
kernel heap leaks.

It will complete deferred background tasks that would ordinarily run at some point after boot
and that would lead to kernel heap allocs/deallocs. For example, unload of lazily loaded DLLs
and running the reaper.
*/
EXPORT_C TInt RTest::CompletePostBootSystemTasks()
	{
	RLoader l;
	TInt r = l.Connect();
	if(r != KErrNone)
		{
		Printf(_L("RTEST: Error %d while connecting to loader.\n"), r);
		return r;
		}

	r = l.CancelLazyDllUnload();
	if(r != KErrNone)
		{
		l.Close();
		Printf(_L("RTEST: Error %d while canceling Lazy Dll unloading.\n"), r);
		return r;
		}

	r = l.RunReaper();
	l.Close();

	if(r != KErrNone)
		{
		Printf(_L("RTEST: Error %d while running the reaper.\n"),r);
		return r;
		}

	// Ensure that any kernel objects asynchronously deleted after the
	// above get fully deleted.
	r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	if(r != KErrNone)
		{
		Printf(_L("RTEST: Error %d while attempting to asynchronously delete kernel objects.\n"),r);
		}
	return r;
	}

