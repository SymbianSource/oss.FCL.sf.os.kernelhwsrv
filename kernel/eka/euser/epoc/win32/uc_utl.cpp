// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\epoc\win32\uc_utl.cpp
// 
//

#include "u32std.h"
#include <e32base.h>
#include <e32base_private.h>
#include <e32hashtab.h>
#include <emulator.h>
#include "uc_std.h"

typedef void (*TBootEpoc)(TBool);

#ifdef __LEAVE_EQUALS_THROW__

// Stub versions of TTrap exports to keep X86 and WINS versions of euser.def
// the same.

class TTrap
	{
public:
	IMPORT_C TInt Trap(TInt& aResult);
	IMPORT_C static void UnTrap();
	};

EXPORT_C TInt TTrap::Trap(TInt&)
	{
	return 0;
	}

EXPORT_C void TTrap::UnTrap()
	{
	}

#endif

EXPORT_C void EmptyFunction()
 //Function with an empty body 
	{
	}

GLDEF_C void Panic(TCdtArchitecturePanic aPanic)
//
// Panic the process with USER as the category.
//
	{
	_LIT(KCategory,"USER-Arch");
	User::Panic(KCategory,aPanic);
	}




EXPORT_C TInt User::IsRomAddress(TBool& aBool, TAny* aPtr)
//
//  The FileServer loads ROM files as ReadOnly Memory Mapped Files
//  We check the access rights of the given address for :
//      Read access
//      No write access
//
/**
Tests whether the specified address is in the ROM.

@param aBool True, if the address at aPtr is within the ROM; false, 
             otherwise.
@param aPtr  The address to be tested.

@return Always KErrNone.
*/
    {
	const TInt KRomMask = 0xFF;
	const TInt KRomAccess = PAGE_READONLY;

    aBool=EFalse;
    MEMORY_BASIC_INFORMATION mi;
	
	__LOCK_HOST;
    if (VirtualQuery(aPtr, &mi, sizeof(mi)) != 0 && (mi.Protect & KRomMask) == KRomAccess)
		aBool=ETrue;
    return KErrNone;
    }



   
EXPORT_C void BootEpoc(TBool aAutoRun)
	{
	HINSTANCE epoc = LoadLibraryA("ekern.exe");
	if (epoc)
		{
		TBootEpoc ep = (TBootEpoc)GetProcAddress(epoc, "_E32Startup");
		if (ep)
			ep(aAutoRun);
		}
	ExitProcess(102);
	}

EXPORT_C void RFastLock::Wait()
	{
	if (InterlockedDecrement((LPLONG)&iCount) < -1)
		RSemaphore::Wait();
	}

EXPORT_C __NAKED__ TInt RFastLock::Poll()
	{
	_asm xor eax, eax
	_asm xor edx, edx
	_asm dec edx

	/* if ([ecx+4]==0) { [ecx+4]=-1; ZF=1;} else {eax=[ecx+4]; ZF=0;} */
	_asm lock cmpxchg [ecx+4], edx
	_asm jz short fastlock_poll_done
	_asm mov eax, -33

	fastlock_poll_done:
	_asm ret
	}

EXPORT_C void RFastLock::Signal()
	{
	if (InterlockedIncrement((LPLONG)&iCount) < 0)
		RSemaphore::Signal();
	}

// Hash an 8 bit string at aPtr, length aLen bytes.
__NAKED__ TUint32 DefaultStringHash(const TUint8* /*aPtr*/, TInt /*aLen*/)
	{
	_asm push esi
	_asm mov esi, [esp+8]
	_asm mov ecx, [esp+12]
	_asm xor eax, eax
	_asm sub ecx, 4
	_asm jb lt4
	ge4:
	_asm xor eax, [esi]
	_asm add esi, 4
	_asm mov edx, 9E3779B9h
	_asm mul edx
	_asm sub ecx, 4
	_asm jae ge4
	lt4:
	_asm add ecx, 4
	_asm jz done
	_asm xor edx, edx
	_asm cmp ecx, 2
	_asm jbe le2
	_asm mov dl, [esi+2]
	_asm shl edx, 16
	le2:
	_asm cmp ecx, 2
	_asm jb onemore
	_asm mov dh, [esi+1]
	onemore:
	_asm mov dl, [esi]
	_asm xor eax, edx
	_asm mov edx, 9E3779B9h
	_asm mul edx
	done:
	_asm pop esi
	_asm ret
	}

// Hash a 16 bit string at aPtr, length aLen bytes.
__NAKED__ TUint32 DefaultWStringHash(const TUint16* /*aPtr*/, TInt /*aLen*/)
	{
	_asm push esi
	_asm mov esi, [esp+8]
	_asm mov ecx, [esp+12]
	_asm xor eax, eax
	_asm sub ecx, 8
	_asm jb lt8
	ge8:
	_asm mov edx, [esi+4]
	_asm xor eax, [esi]
	_asm add esi, 8
	_asm rol edx, 8
	_asm xor eax, edx
	_asm mov edx, 9E3779B9h
	_asm mul edx
	_asm sub ecx, 8
	_asm jae ge8
	lt8:
	_asm add ecx, 8
	_asm jz done
	_asm xor edx, edx
	_asm cmp ecx, 4
	_asm jbe le4
	_asm mov dx, [esi+4]
	_asm rol edx, 8
	_asm xor eax, edx
	_asm xor edx, edx
	le4:
	_asm cmp ecx, 4
	_asm jb onemore
	_asm mov dx, [esi+2]
	_asm shl edx, 16
	onemore:
	_asm mov dx, [esi]
	_asm xor eax, edx
	_asm mov edx, 9E3779B9h
	_asm mul edx
	done:
	_asm pop esi
	_asm ret
	}

/**
@publishedAll
@released

Calculate a 32 bit hash from a 32 bit integer.

@param	aInt	The integer to be hashed.
@return			The calculated 32 bit hash value.
*/
EXPORT_C __NAKED__ TUint32 DefaultHash::Integer(const TInt& /*aInt*/)
	{
	_asm mov edx, [esp+4]
	_asm mov eax, 9E3779B9h
	_asm mul dword ptr [edx]
	_asm ret
	}

