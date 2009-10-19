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

//! @file f32test\concur\t_tdebug.h

#ifndef __T_TDEBUG_H__
#define __T_TDEBUG_H__

struct TThreadData
/// Data about a test task
	{
	RThread			iThread;	///< thread class
	TThreadId      	iId;		///< thread ID
	TInt            iNum;		///< thread number
	TRequestStatus 	iStat;		///< current thread status
	TFullName      	iFile;		///< file to be used for testing
	TBuf<64>        iName;		///< name of this thread
	TBuf<256>      	iMess;		///< buffer for return messages
	TAny*           iData;		///< other data to be passed to the thread
	};

const TInt  KMaxThreads = 100;

class TTest
/// Replacement for RTest, for use within a task (RTest doesn't work in multiple tasks).
/// Each of the output commands prints the task name first on the line.
/// Note that all of the functions and data members of this class are static, they are
/// shared among all threads using the class.
	{
	public:
		class TPos
			{
			public:
				TPos(const char *aFile, TInt aLine);
			public:
				const char* iFailFile;
				TInt        iFailLine;

			};

	public:
		TTest();
		static void Start(const TDesC& aStr);
		static void End(const TDesC& aStr);
		static void Next(const TDesC& aStr);
		static void PrintLock();
		static void PrintUnlock();
		static void Printf(TRefByValue<const TDesC> aFmt, ...);
		static void Printf();
		static void Fail(TPos aPos, TRefByValue<const TDesC> aFmt, ...);
		static void Fail(TPos aPos, TInt aErr, TRefByValue<const TDesC> aFmt, ...);

		static TDesC& ErrStr(TInt aErr, TDes& aDes);

		static TInt Create(TInt aNum, TThreadFunction aFunction, const TDesC& aName);
		static TInt Run(TBool aExitAny = EFalse, TInt aTimeout = 0);
		static TInt RunOnly();
                static void KillAll(TInt aReason);

		static TThreadData& Self();
		static TThreadData& Data(TInt aIndex);

		static TInt ParseCommandArguments(TPtrC aArgV[], TInt aArgMax);
		static TInt ParseCommandArguments(TPtrC aArgV[], TInt aArgMax, TInt& aDebFlags);
		static TChar DefaultDriveChar();

		static TInt Init();

	private:
		static TThreadData iData[KMaxThreads];
		static TThreadData iDummy;
		static RMutex      iDebugLock;
		static RMutex      iPrintLock;
		static TBool       iInit;
		static TFullName   iWhere;
	};

// Exit (panic) if the condition isn't met.
#define TEST(cond) if (!(cond)) { _LIT(err, #cond); TTest::Fail(HERE, _L("failed %S at line %d"), &err, __LINE__); }
#define HERE       TTest::TPos(__FILE__, __LINE__)

// Define the TInt64 macros if they aren't already defined.
#ifndef MAKE_TINT64
#define MAKE_TINT64(h,l)    ( TInt64((h), (l)) )
#define MAKE_TUINT64(h,l)   ( TInt64((h), (l)) )
#define I64HIGH(x)          ( (x).High() )
#define I64LOW(x)           ( (x).Low() )
#define I64INT(x)           ( (x).GetTInt() )
#define I64REAL(x)          ( (x).GetTReal() )
#endif

#endif
