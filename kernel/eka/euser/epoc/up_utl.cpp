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
// e32\euser\epoc\up_utl.cpp
// 
//

#include <u32exec.h>
#include "up_std.h"
#include <e32svr.h>
#include <e32uid.h>

#ifdef __EPOC32__
#include <e32rom.h>
#endif

#include <e32atomics.h>

//#define __DEBUG_IMAGE__ 1
#if defined(__DEBUG_IMAGE__) && defined (__EPOC32__)
extern RDebug debug;
#define __IF_DEBUG(t) {debug.t;}
#else
#define __IF_DEBUG(t)
#endif


/**
Atomically (i.e. in a manner which is safe against concurrent access by other
threads) increments a TInt value by 1.

As an example of its use, the function is used in the implementation of
critical sections.

@param aValue A reference to an integer whose value is to be incremented. 
              On return contains the incremented value.
              
@return The value of aValue before it is incremented.

@see User::LockedDec
@see RCrticalSection
*/
EXPORT_C TInt User::LockedInc(TInt& aValue)
	{
	return (TInt)__e32_atomic_add_ord32(&aValue, 1);
	}


/**
Atomically (i.e. in a manner which is safe against concurrent access by other
threads) decrements a TInt value by 1.

As an example of its use, the function is used in the implementation of
critical sections.

@param aValue A reference to an integer whose value is to be decremented. 
              On return contains the decremented value.
              
@return The value of aValue before it is decremented.

@see User::LockedInc
@see RCrticalSection
*/
EXPORT_C TInt User::LockedDec(TInt& aValue)
	{
	return (TInt)__e32_atomic_add_ord32(&aValue, 0xFFFFFFFF);
	}




EXPORT_C TInt User::SafeInc(TInt& aValue)
/**
Atomically increments the specified value by 1, if the value is > 0.

@param aValue The value to be incremented; on return the incremented value.

@return The original value of aValue
*/
	{
	return __e32_atomic_tas_ord32(&aValue, 1, 1, 0);
	}




EXPORT_C TInt User::SafeDec(TInt &aValue)
/**
Atomically decrements the specified value by 1, if the value is > 0.

@param aValue The value to be decremented; on return the decremented value.

@return The original value of aValue
*/
	{
	return __e32_atomic_tas_ord32(&aValue, 1, -1, 0);
	}


EXPORT_C void UserSvr::WsRegisterThread()
//
// Register the window server thread.
//
    {

	Exec::WsRegisterThread();
	}

EXPORT_C void UserSvr::FsRegisterThread()
//
// Register the file server thread.
//
	{
	Exec::FsRegisterThread();
	}

EXPORT_C void UserSvr::RegisterTrustedChunk(TInt aHandle)
/**
Registers file server's chunk intended for DMA transfer.
@internalComponent
@released 
 */
	{
	Exec::RegisterTrustedChunk(aHandle);
	}

EXPORT_C TInt UserHeap::SetupThreadHeap(TBool, SStdEpocThreadCreateInfo& aInfo)
/**
@internalComponent
*/
	{
	TInt r = KErrNone;
	if (!aInfo.iAllocator && aInfo.iHeapInitialSize>0)
		{
		// new heap required
		RHeap* pH = NULL;
		r = CreateThreadHeap(aInfo, pH);
		}
	else if (aInfo.iAllocator)
		{
		// sharing a heap
		RAllocator* pA = aInfo.iAllocator;
		pA->Open();
		User::SwitchAllocator(pA);
		}
	return r;
	}




EXPORT_C void User::HandleException(TAny* aInfo)
/**
Enables the current thread to handle an exception.

The function is called by the kernel.

@param aInfo A pointer to a TExcType type containing the exception information.

@see TExcType
*/
	{

	const TUint32* p = (const TUint32*)aInfo;
	TUint32 excType = p[0];
	TExceptionHandler f = Exec::ExceptionHandler(KCurrentThreadHandle);
	TRAPD(r, (*f)(TExcType(excType)) );
	Exec::ThreadSetFlags(KCurrentThreadHandle,KThreadFlagLastChance,0);
	if (r!=KErrNone)
		User::Leave(r);
	}




#ifdef __EPOC32__
EXPORT_C TInt User::IsRomAddress(TBool &aBool, TAny *aPtr)
/**
Tests whether the specified address is in the ROM.

@param aBool True, if the address at aPtr is within the ROM; false, 
             otherwise.
@param aPtr  The address to be tested.

@return Always KErrNone.
*/
    {
    
	TUint a = (TUint)aPtr;
	TUint main_start = UserSvr::RomHeaderAddress();
	TUint main_end = main_start + ((TRomHeader*)main_start)->iUncompressedSize;
	aBool = (a>=main_start && a<main_end);
	if (aBool)
		return KErrNone;  // address is in main ROM
	
	TUint rda = UserSvr::RomRootDirectoryAddress();
	
	// We assume here, the primary rom starts a multiple of 4k.
	if (rda > main_end)
		{
		// ASSUMPTIONS HERE
		// 1. root directory is past the end of the main ROM so there must be an extension ROM
		// 2. the ROM file system in the extension ROM is at the beginning of the ROM (similar to the
		//    main ROM)
		// 3. the extension ROM is mapped starting at a megabyte boundary
		// Thus the address of the extension ROM header may be obtained by rounding the root directory
		// address down to the next megabyte boundary.
         
 		TUint ext_start = rda &~ 0x000fffffu;
		TUint ext_base = ((TExtensionRomHeader*)ext_start)->iRomBase;
		TUint ext_end = ext_start + ((TExtensionRomHeader*)ext_start)->iUncompressedSize;
		aBool = (ext_base==ext_start && a>=ext_start && a<ext_end);
 		}    return KErrNone;
    }
#endif


#ifdef __MARM__
EXPORT_C void E32Loader::GetV7StubAddresses(TLinAddr& aExe, TLinAddr& aDll)
	{
	// Only need V7 support on ARM platforms
	aExe = (TLinAddr)&E32Loader::V7ExeEntryStub;
	aDll = (TLinAddr)&E32Loader::V7DllEntryStub;
	}
#endif

#ifdef __MARM__
// Only need V7 support on ARM platforms

_LIT(KEka1EntryStubName, "eka1_entry_stub.dll");
extern "C" TLinAddr GetEka1ExeEntryPoint()
	{
	TLinAddr a = 0;
	RLibrary l;
	TInt r = l.Load(KEka1EntryStubName, KNullDesC, TUidType(KDynamicLibraryUid, KEka1EntryStubUid));
	if (r == KErrNone)
		r = l.Duplicate(RThread(), EOwnerProcess);
	if (r == KErrNone)
		{
		a = (TLinAddr)l.Lookup(1);
		if (!a)
			r = KErrNotSupported;
		}
	if (r != KErrNone)
		RThread().Kill(r);
	Exec::SetReentryPoint(a);
	return a;
	}
#endif
