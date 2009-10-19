// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

//! @file f32test\concur\t_tdebug.cpp

#include <e32test.h>
#include <f32file.h>
#include "t_server.h"
#include "t_tdebug.h"
#include "cfafsdlyif.h"

TThreadData TTest::iData[KMaxThreads];
TThreadData TTest::iDummy;
TFullName   TTest::iWhere;
RMutex      TTest::iDebugLock;
RMutex      TTest::iPrintLock;
TBool       TTest::iInit = EFalse;

LOCAL_C TFileName gErrorPos;

// Instance of the class to force initialisation.
LOCAL_C TTest gTest;

LOCAL_C TInt KSecond = 1000000;

class TTestOverflowTruncate : public TDesOverflow
///
/// Used to suppress overflow when appending formatted text to a buffer.
///
	{
public:
	virtual void Overflow(TDes &/*aDes*/) {}
	};

TTest::TTest()
//
// Constructor, forces initialisation of variables.
//
	{
	Init();
	}

TInt TTest::Init()
///
/// Initialise stuff (currently just the locks) if it hasn't been
/// done already.
///
	{
	if (!iInit)
		{
		TInt r = KErrNone;
		r = iDebugLock.CreateLocal();
		if (r != KErrNone)
			{
			RDebug::Print(_L("ERROR %d creating iDebugLock\n"), r);
			return r;
			}
		r = iPrintLock.CreateLocal();
		if (r != KErrNone)
			{
			RDebug::Print(_L("ERROR %d creating iPrintLock\n"), r);
			return r;
			}
		iInit = ETrue;
		}
	return KErrNone;
	}

TInt TTest::Create(TInt aNum, TThreadFunction aFunction, const TDesC& aName)
///
/// Create a thread, setting up the name and our data area.
///
	{
	if (aNum < 0 || aNum > KMaxThreads)
		{
		test.Printf(_L("Illegal thread %d\n"), aNum);
		test(EFalse);
		}
	TThreadData &d = iData[aNum];
	// test.Printf(_L("creating thread %d (%S)\n"), aNum, &aName);
//	d.iThread.LogonCancel(d.iStat);
//	d.iThread.Close();
	TInt r;
	r = d.iThread.Create(aName, aFunction, KDefaultStackSize+32*1024, KMinHeapSize, 0x20000, &d);
	if (r != KErrNone)
	{
		TBuf<128> buf;
		test.Printf(_L("Error creating thread %d '%S' (was %d '%S'): %S\n"),
			aNum, &aName, d.iNum, &d.iName, &TTest::ErrStr(r, buf));
		test(0);
	}
	d.iId   = d.iThread.Id();
	d.iNum  = aNum;
	d.iName = aName;
	d.iThread.Logon(d.iStat);
	return r;
	}

TInt TTest::RunOnly()
///
/// Resume all of the threads we have created.
///
	{
	TInt i;
	for (i = 0; i < KMaxThreads; i++)
		{
		if (iData[i].iId > 0)
			{
			iData[i].iThread.Resume();
			}
		}
	return KErrNone;
	}

TInt TTest::Run(TBool aExitAny, TInt aTimeout)
///
/// Run until all (or any one) threads has completed, or until a timeout.
/// @param aExitAny If true, exit when the first thread completes, otherwise
///        wait until they have all completed.
/// @param aTimeout if zero, no timeout, otherwise it is the timeout in microseconds.
///
	{
	TInt i;
	TInt status  = RunOnly();
	RTimer timer;
	TRequestStatus tstat;
	timer.CreateLocal();
	if (aTimeout)
		timer.After(tstat, aTimeout);
	for (;;)
		{
		status = KErrNone;
		User::WaitForAnyRequest();
		if (aTimeout > 0 && tstat != KRequestPending)
			break;
		TBool running = EFalse;
		for (i = 0; i < KMaxThreads; i++)
			{
			if (iData[i].iId > 0)
				{
				if (iData[i].iStat == KRequestPending)
					{
					running = ETrue;
					}
				else
					{
					TThreadData &d = iData[i];
					// ignore result of LogonCancel, since we know thread has finished
					d.iThread.LogonCancel(d.iStat);
					d.iThread.Close();
					d.iId = 0;
					if (d.iStat != KErrNone)
						{
						status = KErrAbort;
						TBuf<32> ebuf;
						test.Printf(_L("ERROR: %S in thread %S: %S\n                %S"),
									&ErrStr(d.iStat.Int(), ebuf), &d.iName, &d.iMess, &iWhere);
						if (aExitAny)
							{
							running = EFalse;
							break;
							}
						}
					}
				}
			}
		if (!running)
			break;
		}
	timer.Cancel();
	timer.Close();
	return status;
	}

void TTest::KillAll(TInt aReason)
//
// Kill (destroy) all of the created threads, then wait for up to 10 seconds
// for them all to die (and just exit if any are still alive).
//
	{
	for (TInt i = 0; i < KMaxThreads; i++)
		{
		if (iData[i].iId > 0)
			{
			TThreadData &d = iData[i];
			d.iThread.Kill(aReason);
			}
		}
	Run(EFalse, 10*KSecond);
	}

TThreadData& TTest::Self()
///
/// Return a reference to the current thread; if it's not one we've created
/// return a reference to a dummy data area indicating no thread.
///
	{
	RThread me;
	TInt i;
	for (i = 0; i < KMaxThreads; i++)
		{
		if (me.Id() == iData[i].iId)
			{
			return iData[i];
			}
		}
	iDummy.iId = 0;
	iDummy.iNum = -1;
	iDummy.iName.Format(_L("#%d"), (TUint)me.Id());
	return iDummy;
	}

TThreadData& TTest::Data(TInt aIndex)
///
/// Return a reference to the data area for the specified thread, or to a
/// dummy area if it's not in the right range.
///
/// @param aIndex index to the thread (ThreadData::iNum is the same number).
///
	{
	if (aIndex >= 0 && aIndex < KMaxThreads)
		return iData[aIndex];
	iDummy.iId = 0;
	iDummy.iNum = -1;
	iDummy.iName = _L("");
	return iDummy;
	}

void TTest::Start(const TDesC& aStr)
///
/// Output "START TEST" and the string.
///
	{
	Printf(_L("START TEST: %S\n"), &aStr);
	}

void TTest::Next(const TDesC& aStr)
///
/// Output "NEXT TEST" and the string.
///
	{
	Printf(_L("NEXT TEST: %S\n"), &aStr);
	}

void TTest::PrintLock()
///
/// Wait if another task is doing output.
///
	{
	iPrintLock.Wait();
	}

void TTest::PrintUnlock()
///
/// Signal that output is complete so that other tasks can do output.
///
	{
	iPrintLock.Signal();
	}

void TTest::Printf(TRefByValue<const TDesC> aFmt, ...)
///
/// Output the formatted text, prepending it with the thread name if it is one
/// we've created.  Parameters as for printf().  Note that if more than one
/// thread tries to call it at the same time it will lock so that only one is
/// processed at a time, the debug output isn't thread-safe (it can mix
/// characters from different threads).
///
	{
	TTestOverflowTruncate overflow;
	VA_LIST list;
	VA_START(list, aFmt);
	TBuf<256> buf;
		buf.SetLength(0);
	if (Self().iNum >= 0)
		{
		buf.Append(Self().iName);
		buf.Append(_L(": "));
		}
	buf.AppendFormatList(aFmt, list, &overflow);
#if defined(__WINS__)
	if (buf.Right(1) != _L("\n"))
		buf.Append(_L("\n"));
#else
	if (buf.Right(1) == _L("\n"))
		buf.SetLength(buf.Length() - 1);
#endif
	iDebugLock.Wait();
	RDebug::Print(_L("%S"), &buf);
	iDebugLock.Signal();
	VA_END(list);
	}

void TTest::Printf()
///
/// Output a blank line, prepended with the thread name if any.
///
	{
	Printf(_L("\n"));
	}

void TTest::Fail(TPos aPos, TRefByValue<const TDesC> aFmt, ...)
///
/// Output an error message (formatted as for printf()), then exit the thread.
/// The message is placed in the buffer associated with the thread so that
/// the parent task can display it.
///
	{
	VA_LIST list;
	VA_START(list, aFmt);
	Self().iMess.FormatList(aFmt, list);
	iDebugLock.Wait();
	TPtrC8 ptr((TUint8*)aPos.iFailFile);
	gErrorPos.Copy(ptr);
	iWhere.Format(_L("  %S line %d\n"), &gErrorPos, aPos.iFailLine);
	RDebug::Print(_L("\n"));
	RDebug::Print(_L("ERROR in thread %S: %S"), &Self().iName, &Self().iMess);
	RDebug::Print(_L("        %S line %d\n"), &gErrorPos, aPos.iFailLine);
	RDebug::Print(_L("\n"));
	iDebugLock.Signal();
	User::Exit(KErrAbort);
	}

void TTest::Fail(TPos aPos, TInt aErr, TRefByValue<const TDesC> aFmt, ...)
///
/// Output an error message including the interpreted error value followed
/// by the specified text (formatted as for printf()), then exit the thread.
/// The message is placed in the buffer associated with the thread so that
/// the parent task can display it.
///
	{
	VA_LIST list;
	VA_START(list, aFmt);
	TBuf<32> ebuf;
	ErrStr(aErr, ebuf);
	Self().iMess.FormatList(aFmt, list);
	iDebugLock.Wait();
	TPtrC8 ptr((TUint8*)aPos.iFailFile);
	gErrorPos.Copy(ptr);
	iWhere.Format(_L("  %S line %d\n"), &gErrorPos, aPos.iFailLine);
	RDebug::Print(_L("\n"));
	RDebug::Print(_L("%S in thread %S: %S"), &ebuf, &Self().iName, &Self().iMess);
	RDebug::Print(_L("        %S line %d\n"), &gErrorPos, aPos.iFailLine);
	RDebug::Print(_L("\n"));
	iDebugLock.Signal();
	User::Exit(aErr);
	}

TDesC& TTest::ErrStr(TInt aErr, TDes& aDes)
///
/// Interpret an error status value as a string in the specified buffer.
/// If the value isn't recognised then it formats a string containing the
/// value itself (like "Error -65").
/// @param aErr The error value.
/// @param aDes Descriptor of the buffer to be used.
/// @return     Descriptor of the buffer.
///
	{
	switch (aErr)
		{
		case KErrNone:
			aDes = _L("KErrNone");
			break;
		case KErrNotFound:
			aDes = _L("KErrNotFound");
			break;
		case KErrGeneral:
			aDes = _L("KErrGeneral");
			break;
		case KErrCancel:
			aDes = _L("KErrCancel");
			break;
		case KErrNoMemory:
			aDes = _L("KErrNoMemory");
			break;
		case KErrNotSupported:
			aDes = _L("KErrNotSupported");
			break;
		case KErrArgument:
			aDes = _L("KErrArgument");
			break;
		case KErrTotalLossOfPrecision:
			aDes = _L("KErrTotalLossOfPrecision");
			break;
		case KErrBadHandle:
			aDes = _L("KErrBadHandle");
			break;
		case KErrOverflow:
			aDes = _L("KErrOverflow");
			break;
		case KErrUnderflow:
			aDes = _L("KErrUnderflow");
			break;
		case KErrAlreadyExists:
			aDes = _L("KErrAlreadyExists");
			break;
		case KErrPathNotFound:
			aDes = _L("KErrPathNotFound");
			break;
		case KErrDied:
			aDes = _L("KErrDied");
			break;
		case KErrInUse:
			aDes = _L("KErrInUse");
			break;
		case KErrServerTerminated:
			aDes = _L("KErrServerTerminated");
			break;
		case KErrServerBusy:
			aDes = _L("KErrServerBusy");
			break;
		case KErrCompletion:
			aDes = _L("KErrCompletion");
			break;
		case KErrNotReady:
			aDes = _L("KErrNotReady");
			break;
		case KErrUnknown:
			aDes = _L("KErrUnknown");
			break;
		case KErrCorrupt:
			aDes = _L("KErrCorrupt");
			break;
		case KErrAccessDenied:
			aDes = _L("KErrAccessDenied");
			break;
		case KErrLocked:
			aDes = _L("KErrLocked");
			break;
		case KErrWrite:
			aDes = _L("KErrWrite");
			break;
		case KErrDisMounted:
			aDes = _L("KErrDisMounted");
			break;
		case KErrEof:
			aDes = _L("KErrEof");
			break;
		case KErrDiskFull:
			aDes = _L("KErrDiskFull");
			break;
		case KErrBadDriver:
			aDes = _L("KErrBadDriver");
			break;
		case KErrBadName:
			aDes = _L("KErrBadName");
			break;
		case KErrCommsLineFail:
			aDes = _L("KErrCommsLineFail");
			break;
		case KErrCommsFrame:
			aDes = _L("KErrCommsFrame");
			break;
		case KErrCommsOverrun:
			aDes = _L("KErrCommsOverrun");
			break;
		case KErrCommsParity:
			aDes = _L("KErrCommsParity");
			break;
		case KErrTimedOut:
			aDes = _L("KErrTimedOut");
			break;
		case KErrCouldNotConnect:
			aDes = _L("KErrCouldNotConnect");
			break;
		case KErrCouldNotDisconnect:
			aDes = _L("KErrCouldNotDisconnect");
			break;
		case KErrDisconnected:
			aDes = _L("KErrDisconnected");
			break;
		case KErrBadLibraryEntryPoint:
			aDes = _L("KErrBadLibraryEntryPoint");
			break;
		case KErrBadDescriptor:
			aDes = _L("KErrBadDescriptor");
			break;
		case KErrAbort:
			aDes = _L("KErrAbort");
			break;
		case KErrTooBig:
			aDes = _L("KErrTooBig");
			break;
		case KErrDivideByZero:
			aDes = _L("KErrDivideByZero");
			break;
		case KErrBadPower:
			aDes = _L("KErrBadPower");
			break;
		case KErrDirFull:
			aDes = _L("KErrDirFull");
			break;
		case KErrHardwareNotAvailable:
			aDes = _L("KErrHardwareNotAvailable");
			break;
		case KErrSessionClosed:
			aDes = _L("KErrSessionClosed");
			break;
		case KErrPermissionDenied:
			aDes = _L("KErrPermissionDenied");
			break;
		case KRequestPending:
			aDes = _L("KRequestPending");
			break;
		default:
			aDes = _L("Error ");
			aDes.AppendNum(aErr);
			break;
		}
	return aDes;
	}

TInt TTest::ParseCommandArguments(TPtrC aArgV[], TInt aArgMax)
///
/// Parse command line.  Put the parameters into array aArgv for
/// use by the tests, strip out flags starting with / or - and interpret
/// them to set debug flags.
///
	{
	RFs fs;
	TInt r = fs.Connect();
	test(r == KErrNone);
	TInt flags = 0;
	TInt argc = ParseCommandArguments(aArgV, aArgMax, flags);
	fs.SetDebugRegister(flags);
	fs.Close();
	return argc;
	}

TInt TTest::ParseCommandArguments(TPtrC aArgV[], TInt aArgMax, TInt& aDebugFlags)
///
/// Parse command line.  Put the parameters into array aArgv for
/// use by the tests, strip out flags starting with / or - and interpret
/// them to set debug flags.
///
	{
	LOCAL_D TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	TFileName thisfile=RProcess().FileName();
	if (token.MatchF(thisfile)==0)
		{
		token.Set(lex.NextToken());
		}
	// set up parameter list (offset zero is the filename)
	TInt argc = 0;
	aArgV[argc++].Set(thisfile);
	while (token.Length() != 0)
		{
		TChar ch = token[0];
		// strip out (and interpret) flags starting with - or /
		if (ch == '-' || ch == '/')
			{
			for (TInt i = 1; i < token.Length(); i++)
				{
				switch (User::UpperCase(token[i]))
					{
					case 'D':
						aDebugFlags |= KDLYFAST;
						break;
					case 'F':
						aDebugFlags |= KFSYS;
						break;
					case 'I':
						aDebugFlags |= KISO9660;
						break;
					case 'L':
						aDebugFlags |= KFLDR;
						break;
#ifdef __CONCURRENT_FILE_ACCESS__
					case 'M':
						aDebugFlags |= KTHRD;
						break;
#endif
					case 'N':
						aDebugFlags |= KNTFS;
						break;
					case 'S':
						aDebugFlags |= KFSERV;
						break;
					case 'T':
						aDebugFlags |= KLFFS;
						break;
					case 'Y':
						aDebugFlags |= KDLYTRC;
						break;
					}
				}
			}
		else if (argc < aArgMax)
			aArgV[argc++].Set(token);
		token.Set(lex.NextToken());
		}
	return argc;
	}

TChar TTest::DefaultDriveChar()
	{
	TFileName session;
	RFs fs;
	fs.Connect();
	fs.SessionPath(session);
	fs.Close();
	TChar drvch = User::UpperCase(session[0]);
	return drvch;
	}

TTest::TPos::TPos(const char *aFile, TInt aLine)
	{
	iFailFile = aFile;
	iFailLine = aLine;
	}

