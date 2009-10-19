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
// e32\include\kernel\execs.h
// 
//

#ifndef __K32EXEC_H__
#define __K32EXEC_H__
#include <kernel/kern_priv.h>

class DObject;
class DTimer;
class DThread;
class DMutex;
class DSemaphore;
class DProcess;
class DChunk;
class DServer;
class DSession;
class DLibrary;
class DLogicalDevice;
class DPhysicalDevice;
class DLogicalChannelBase;
class DChangeNotifier;
class DUndertaker;
class DMsgQueue;
class DPropertyRef;
class DShPool;
class DShBuf;

#include <u32exec.h>

#define EF_C	KExecFlagClaim
#define	EF_R	KExecFlagRelease
#define	EF_P	KExecFlagPreprocess
#define EF_A2	KExecFlagExtraArgs2
#define EF_A3	KExecFlagExtraArgs3
#define EF_A4	KExecFlagExtraArgs4
#define EF_A5	KExecFlagExtraArgs5
#define EF_A6	KExecFlagExtraArgs6
#define EF_A7	KExecFlagExtraArgs7
#define EF_A8	KExecFlagExtraArgs8

#if defined(__WINS__)

// Executive call macros for WINS

#define	FAST_EXEC_BEGIN	\
	GLDEF_D const TUint32 EpocFastExecTable[] =		\
		{

#define FAST_EXEC_END	\
		0				\
		};

#define DECLARE_WORD(n)				(n),
#define	DECLARE_FUNC(f)				TLinAddr(&f),
#define	DECLARE_FAST_EXEC_INVALID	DECLARE_FUNC(InvalidFastExec)

#define	SLOW_EXEC_BEGIN	\
	GLDEF_D const TUint32 EpocSlowExecTable[] =		\
		{

#define SLOW_EXEC_END	\
		0				\
		};

#define DECLARE_INVALID_EXEC_HANDLER		DECLARE_FUNC(InvalidExecHandler)
#define	DECLARE_EXEC_PREPROCESS_HANDLER		DECLARE_FUNC(Win32PreprocessHandler)

#define	DECLARE_FLAGS_FUNC(flags, func)					(flags), DECLARE_FUNC(func)
#define	DECLARE_FLAGS_ASMFUNC(flags, asmfunc, cppfunc)	(flags), DECLARE_FUNC(cppfunc)
#define	DECLARE_SLOW_EXEC_INVALID			0, DECLARE_FUNC(InvalidExecHandler)

#elif defined(__CPU_X86)

// Executive call macros for X86

#define	FAST_EXEC_BEGIN	\
	GLDEF_D const TUint32 EpocFastExecTable[] =		\
		{

#define FAST_EXEC_END	\
		0				\
		};

#define DECLARE_WORD(n)				(n),
#define	DECLARE_FUNC(f)				TLinAddr(&f),
#define	DECLARE_FAST_EXEC_INVALID	DECLARE_FUNC(InvalidFastExec)

#define	SLOW_EXEC_BEGIN	\
	GLDEF_D const TUint32 EpocSlowExecTable[] =		\
		{

#define SLOW_EXEC_END	\
		0				\
		};

#define DECLARE_INVALID_EXEC_HANDLER		DECLARE_FUNC(InvalidExecHandler)
#define	DECLARE_EXEC_PREPROCESS_HANDLER		DECLARE_FUNC(PreprocessHandler)

#define	DECLARE_FLAGS_FUNC(flags, func)					(flags), DECLARE_FUNC(func)
#define	DECLARE_FLAGS_ASMFUNC(flags, asmfunc, cppfunc)	(flags), DECLARE_FUNC(cppfunc)
#define	DECLARE_SLOW_EXEC_INVALID			0, DECLARE_FUNC(InvalidExecHandler)


#elif defined(__CPU_ARM)

// Executive call macros for ARM

#define	FAST_EXEC_BEGIN	\
	__NAKED__ void dummy_function_for_exec_tables() {	\
		asm(".global EpocFastExecTable ");				\
		asm("EpocFastExecTable: ");

#define FAST_EXEC_END

#define DECLARE_WORD(n)				asm(".word %a0" : : "i" ((TInt)n));
#define	DECLARE_FUNC(f)				asm(".word %a0" : : "i" ((TInt)&f));
#define	DECLARE_FAST_EXEC_INVALID	DECLARE_FUNC(InvalidFastExec)

#define	SLOW_EXEC_BEGIN	\
		asm(".global EpocSlowExecTable ");				\
		asm("EpocSlowExecTable: ");

#define SLOW_EXEC_END	\
		}

#define DECLARE_INVALID_EXEC_HANDLER		DECLARE_FUNC(InvalidExecHandler)
#define	DECLARE_EXEC_PREPROCESS_HANDLER		DECLARE_FUNC(PreprocessHandler)

#define	DECLARE_FLAGS_FUNC(flags, func)					DECLARE_WORD(flags)	DECLARE_FUNC(func)
#define	DECLARE_FLAGS_ASMFUNC(flags, asmfunc, cppfunc)	asm(".word %a0," #asmfunc : : "i" (flags));
#define	DECLARE_SLOW_EXEC_INVALID						DECLARE_FLAGS_FUNC(0, InvalidExecHandler)


#else
#error Unknown CPU
#endif

#include <kernel/exec_kernel.h>

#endif
