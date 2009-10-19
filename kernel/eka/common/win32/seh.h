// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\common\win32\seh.h
// 
//

#ifndef __SEH_H__
#define __SEH_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <excpt.h>

// Extracted from exsup.inc

/** An exception unwind is in progress */
#define	EXCEPTION_UNWINDING		2

// Special Win32 SEH code for C++ exceptions
static const DWORD EXCEPTION_MSCPP = 0xe06d7363;

/** Helper function to get current TIB record as a simple 32bit pointer */
__declspec(naked) NT_TIB* Tib()
	{
	// _FOFF(NT_TIB, Self) = 0x18
	_asm mov eax, dword ptr fs:[0x18]
	_asm ret
	}

#include <e32def.h>
__ASSERT_COMPILE(_FOFF(NT_TIB, Self) == 0x18);

#endif // __SEH_H__
