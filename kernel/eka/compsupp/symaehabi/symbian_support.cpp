// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32/compsupp/symaehabi/symbian_support.cpp
// 
//

/* Environment: */
#include "unwind_env.h"
/* Language-independent unwinder declarations: */
#include "unwinder.h"

/* Symbian specific support */
#include "symbian_support.h"

#include <e32def.h>
#include <e32def_private.h>
#include <e32rom.h>
#include <e32svr.h>
#include <e32debug.h>

static void __default_terminate_handler(void)
	{
	abort();
	}

#define NAMES __ARM
namespace NAMES { void default_unexpected_handler(void); }

EXPORT_C TCppRTExceptionsGlobals::TCppRTExceptionsGlobals()
	{
	buffer.inuse = false;
	thread_globals.uncaughtExceptions = 0;
	thread_globals.unexpectedHandler = NAMES::default_unexpected_handler;
	thread_globals.terminateHandler = __default_terminate_handler;
	thread_globals.implementation_ever_called_terminate = false;
	thread_globals.call_hook = NULL;
	thread_globals.caughtExceptions = NULL;
	thread_globals.propagatingExceptions = NULL;
	thread_globals.emergency_buffer = &buffer;
	Dll::SetTls(this);
	}
#if __ARMCC_VERSION < 220000
extern "C" 
{
  /*
    we have to dummy up the following for 2.1. The names changed in the def file
    since in 2.2 catch handlers should be able to deal with imported RTTI

	_ZTI15XLeaveException @ 206 NONAME ; Typeinfo for XLeaveException
	_ZTV15XLeaveException @ 207 NONAME ; vtable for XLeaveException
	_ZN15XLeaveException16ForceKeyFunctionEv @ 208 NONAME ; the key function for XLeaveException
  */

EXPORT_C void _ZTI15XLeaveException()
	{
	// reserve a DEF file slot for key function
	}

EXPORT_C void _ZTV15XLeaveException()
	{
	// reserve a DEF file slot for vtable
	}

EXPORT_C void _ZN15XLeaveException16ForceKeyFunctionEv()
	{
	// reserve a DEF file slot for RTTI
	}
}

#else 
// This is the key function that forces the class impedimenta to be get exported in RVCT 2.2 and later.
EXPORT_C void XLeaveException::ForceKeyFunction(){}
#endif

#if 0
#pragma push
#pragma arm
// If the strings were word aligned we could do something like this.
// We could even do this if we checked the alignment of the strings.
// Unfortunately the chances of them both being word aligned are
// sufficiently slim (1/16) that the test for alignment will be carried out
// most of time for no good purpose.
static inline int typenameeq(const char* n1, const char* n2)
	{
	if (n1 == n2)
		return 1;
	int* i1 = (int*)n1;
	int* i2 = (int*)n2;
	int w1=0;
	int w2=0;
	int x1, x2;
	for (int i = 0, w1=i1[i], w2=i2[i]; w1 == w2; i++, w1=i1[i], w2=i2[i])
		{
		// they're the same but they might contain the terminator
		if (!(w1 & 0xffffff00))
			return 1;
		if (!(w1 & 0xffff00ff))
			return 1;
		if (!(w1 & 0xff00ffff))
			return 1;
		if (!(w1 & 0x00ffffff))
			return 1;
		}
	// they're not the same but they might contain the terminator in the same place
	x1 = w1 & 0x000000ff;
	x2 = w2 & 0x000000ff;
	if (!x1 && !x2)
		return 1;
	if (x1 != x2)
		return 0;

	x1 = w1 & 0x0000ff00;
	x2 = w2 & 0x0000ff00;
	if (!x1 && !x2)
		return 1;
	if (x1 != x2)
		return 0;

	x1 = w1 & 0x00ff0000;
	x2 = w2 & 0x00ff0000;
	if (!x1 && !x2)
		return 1;
	if (x1 != x2)
		return 0;

	x1 = w1 & 0xff000000;
	x2 = w2 & 0xff000000;
	if (!x1 && !x2)
		return 1;
	if (x1 != x2)
		return 0;

	// just to keep the compiler quiet
	return 0;
	}
#pragma pop
#endif

extern "C" {

IMPORT_C void abort();

TRomExceptionSearchTable * GetROMExceptionSearchTable(void)
	{
	return (TRomExceptionSearchTable *)((TRomHeader *)UserSvr::RomHeaderAddress())->iRomExceptionSearchTable;
	}

TExceptionDescriptor* SearchEST(uint32_t addr, TRomExceptionSearchTable* aESTp)
	{
	uint32_t l = 0;
	uint32_t nelems = aESTp->iNumEntries;
	uint32_t r = nelems;
	uint32_t m = 0;
	uint32_t val;
	uint32_t* base = (uint32_t*)&aESTp->iEntries[0];
	while (r > l)
		{
		m = (l + r) >> 1;
		val = base[m];
		if (val > addr)
			r = m;
		else
			l = m + 1;
		}
	val = base[l-1];
	if (addr >= val && addr < base[l])	/* relies on presence of fencepost at the end */
		{
		const TRomImageHeader* rih = (const TRomImageHeader*)val;
		return (TExceptionDescriptor*)rih[-1].iExceptionDescriptor;
		}
	return 0;
	}


TExceptionDescriptor * GetRAMLoadedExceptionDescriptor(uint32_t addr)
	{
	TLinAddr aEDp = UserSvr::ExceptionDescriptor(addr);

	SYMBIAN_EH_SUPPORT_PRINTF("UserSvr::ExceptionDescriptor for %08x returned %08x\n", addr, aEDp);

	return (TExceptionDescriptor *)(aEDp & 0xfffffffe);
	}

void InitialiseSymbianSpecificUnwinderCache(uint32_t addr, _Unwind_Control_Block * ucbp)
	{
	SET_ROM_EST(ucbp, GetROMExceptionSearchTable());
	if (!ReLoadExceptionDescriptor(addr, ucbp))
		{
		SYMBIAN_EH_SUPPORT_PRINTF("EH ERROR: no exception descriptor for address 0x%08x\n", addr);
		abort();
		}
	}

TExceptionDescriptor * ReLoadExceptionDescriptor(uint32_t addr, _Unwind_Control_Block * ucbp)
	{
	/* Check to see if address is in range covered by ROM EST. If
	   it is find the exception descriptor for the address,
	   checking that the address is in the range covered by the
	   descriptor.  Otherwise it comes from a RAM loaded
	   executable so get the kernel to find the exception
	   descriptor for us.
	*/
	TRomExceptionSearchTable * aESTp = GET_ROM_EST(ucbp);
	TExceptionDescriptor * aEDp = NULL;
	if (aESTp && addr >= aESTp->iEntries[0] && addr < GET_EST_FENCEPOST(aESTp))
		{
		aEDp = SearchEST(addr, aESTp);
		goto jobdone;
		}
	aEDp = GetRAMLoadedExceptionDescriptor(addr);
	if (!aEDp)
		{
		// look in extension ROM if there is one
		TUint main_start = UserSvr::RomHeaderAddress();
		TUint main_end = main_start + ((TRomHeader*)main_start)->iUncompressedSize;
		
		TUint rda = UserSvr::RomRootDirectoryAddress();
		
		// Assume rom starts on multiple of 4k
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
			TRomExceptionSearchTable* extrom_exctab = (TRomExceptionSearchTable*)(((TExtensionRomHeader*)ext_start)->iRomExceptionSearchTable);
			if (extrom_exctab && addr >= extrom_exctab->iEntries[0] && addr < GET_EST_FENCEPOST(extrom_exctab))
				aEDp = SearchEST(addr, extrom_exctab);
			}
		}

jobdone:
	SYMBIAN_EH_SUPPORT_PRINTF("ReLoadExceptionDescriptor: Exception descriptor for address 0x%08x = 0x%08x\n\r", addr, aEDp);

	if (aEDp && ADDRESS_IN_EXCEPTION_DESCRIPTOR_RANGE(addr, aEDp))
		return SET_EXCEPTION_DESCRIPTOR(ucbp, aEDp);
	else
	  	return NULL;
	}


const __EIT_entry* SearchEITV1(uint32_t return_address_offset, const __EIT_entry* base, unsigned int nelems)
	{
	uint32_t l = 0;
	uint32_t r = nelems;
	uint32_t m=0;
	uint32_t val;
	while (r>l)
		{
		m = (l + r) >> 1;
		val = base[m].fnoffset;
		if (val > return_address_offset)
			r = m;
		else
			l = m + 1;
		}
#ifdef _DEBUG
	val = base[l-1].fnoffset;
	SYMBIAN_EH_SUPPORT_PRINTF("SearchEITV1: Located IDX with fnoffset = %08x\n\r", val);
#endif
	return base + l - 1;
	}

/* R_ARM_PREL31 is a place-relative 31-bit signed relocation.  The
 * routine takes the address of a location that was relocated by
 * R_ARM_PREL31, and returns an absolute address.
 */
static FORCEINLINE uint32_t __ARM_resolve_prel31(void *p)
{
  return (uint32_t)((((*(int32_t *)p) << 1) >> 1) + (int32_t)p);
}

__EIT_entry* SearchEITV2(uint32_t return_address, const __EIT_entry* base, unsigned int nelems)
	{
	uint32_t l = 0;
	uint32_t r = nelems;
	uint32_t m=0;
	uint32_t val;
	while (r>l)
		{
		m = (l + r) >> 1;
		val = __ARM_resolve_prel31((void *)&base[m].fnoffset);
		if (val > return_address)
			r = m;
		else
			l = m + 1;
		}
#ifdef _DEBUG
	val = __ARM_resolve_prel31((void *)&base[l-1].fnoffset);
	SYMBIAN_EH_SUPPORT_PRINTF("SearchEITV2: Located IDX with fn address = %08x\n\r", val);
#endif
	return ((__EIT_entry *)base) + l - 1;
	}


#ifdef _DEBUG
class TestOverflowTruncate8 : public TDes8Overflow
	{
public:
	virtual void Overflow(TDes8& /*aDes*/) {}
	};
	
#endif

void DebugPrintf(const char * aFmt, ...)
	{
#ifdef _DEBUG
	TestOverflowTruncate8 overflow;
	VA_LIST list;
	VA_START(list,aFmt);
	TPtrC8 fmt((const TUint8 *)aFmt);
	TBuf8<0x100> buf;
	buf.AppendFormatList(fmt,list,&overflow);
	TBuf<0x100> buf2;
	buf2.Copy(buf);
	if (buf2[buf2.Length()-1]=='\n') buf2.Append('\r');
	RDebug::RawPrint(buf2);
#else
	(void)aFmt;
#endif		
	}

} // extern "C"
