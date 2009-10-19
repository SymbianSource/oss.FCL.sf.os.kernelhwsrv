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
// e32/compsupp/symaehabi/symbian_support.h
// 
//


#ifndef SYMBIAN_SUPPORT_H
#define SYMBIAN_SUPPORT_H

#include <e32def_private.h>

#ifdef __cplusplus
typedef _Unwind_Control_Block UCB;

using std::terminate_handler;
using std::unexpected_handler;
using std::terminate;
using std::unexpected;
using std::type_info;

/* --------- Exception control object: --------- */

// Type __cxa_exception is the combined C++ housekeeping (LEO) and UCB.
// It will be followed by the user exception object, hence must ensure
// the latter is aligned on an 8 byte boundary.

struct __cxa_exception {
  const type_info *exceptionType;       // RTTI object describing the type of the exception
  void *(*exceptionDestructor)(void *); // Destructor for the exception object (may be NULL)
  unexpected_handler unexpectedHandler; // Handler in force after evaluating throw expr
  terminate_handler terminateHandler;   // Handler in force after evaluating throw expr
  __cxa_exception *nextCaughtException; // Chain of "currently caught" c++ exception objects
  uint32_t handlerCount;                // Count of how many handlers this EO is "caught" in
  __cxa_exception *nextPropagatingException; // Chain of objects saved over cleanup
  uint32_t propagationCount;            // Count of live propagations (throws) of this EO
  UCB ucb;                              // Forces alignment of next item to 8-byte boundary
};


// Exceptions global support
typedef void (*handler)(void);

struct __cxa_eh_globals {
  uint32_t uncaughtExceptions;               // counter
  unexpected_handler unexpectedHandler;      // per-thread handler
  terminate_handler terminateHandler;        // per-thread handler
  bool implementation_ever_called_terminate; // true if it ever did
  handler call_hook;     // transient field to tell terminate/unexpected which hook to call
  __cxa_exception *caughtExceptions;         // chain of "caught" exceptions
  __cxa_exception *propagatingExceptions;    // chain of "propagating" (in cleanup) exceptions
  void *emergency_buffer;                    // emergency buffer for when rest of heap full
};

// emergency storage reserve primarily for throwing OOM exceptions
struct emergency_eco {
  __cxa_exception ep;
    XLeaveException aUserLeaveException;
};

struct emergency_buffer {
  bool inuse;
  struct emergency_eco eco;
};


class TCppRTExceptionsGlobals
	{
public:
	IMPORT_C TCppRTExceptionsGlobals();
private:
	__cxa_eh_globals thread_globals;
	emergency_buffer buffer;
	};

#endif

// Support for finding ROM resident ExIdx tables
#define GET_ROM_EST(u) ((TRomExceptionSearchTable *)((u)->unwinder_cache.reserved4))
#define SET_ROM_EST(u,e) ((TRomExceptionSearchTable *)((u)->unwinder_cache.reserved4=(uint32_t)e))
#define GET_EXCEPTION_DESCRIPTOR(u) ((TExceptionDescriptor *)((u)->unwinder_cache.reserved5))
#define SET_EXCEPTION_DESCRIPTOR(u,e) ((TExceptionDescriptor *)((u)->unwinder_cache.reserved5=(uint32_t)e))

// Support for checking which version of EHABI is in play
#define EHABI_MASK 0xfffffffc
// Checks if image implements V2 of EHABI
#define EHABI_V2(u) ((GET_EXCEPTION_DESCRIPTOR(u)->iROSegmentBase) & 1)
#define GET_RO_BASE(u) (((u)->iROSegmentBase) & EHABI_MASK)
#define ADDRESS_IN_EXCEPTION_DESCRIPTOR_RANGE(addr, aEDp) (((addr) >= GET_RO_BASE(aEDp)) && ((addr) < (aEDp)->iROSegmentLimit))
#define GET_EST_FENCEPOST(aESTp) ((aESTp)->iEntries[(aESTp)->iNumEntries])
#define ADDRESS_IN_ROM_EST(addr, aESTp) (((addr) >= (aESTp)->iEntries[0]) && ((addr) < GET_EST_FENCEPOST((aESTp))))


// Non __EPOC32__ versions defined in unwinder.c
#define EIT_base(u) \
    ((const __EIT_entry *)((GET_EXCEPTION_DESCRIPTOR(u))->iExIdxBase))
#define EIT_limit(u) \
    ((const __EIT_entry *)((GET_EXCEPTION_DESCRIPTOR(u))->iExIdxLimit))


#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int size_t;
IMPORT_C void abort(void);
int typenameeq(const char * n1, const char * n2);

#define malloc User::Alloc
#define free User::Free


#ifdef _DEBUG
// uncomment this for diagnostic output in UDEB build
//#define _DEBUG_SYMBIAN_EH_SUPPORT
#endif

#ifdef _DEBUG_SYMBIAN_EH_SUPPORT
#define VRS_DIAGNOSTICS
#define UNWIND_ACTIVITY_DIAGNOSTICS
#define PR_DIAGNOSTICS
#define PRINTED_DIAGNOSTICS
#define CPP_DIAGNOSTICS
#endif
#define printf DebugPrintf
extern void DebugPrintf(const char *, ...);

#ifdef _DEBUG_SYMBIAN_EH_SUPPORT
#define SYMBIAN_EH_SUPPORT_PRINTF DebugPrintf
#else
#define SYMBIAN_EH_SUPPORT_PRINTF (void)
#pragma diag_suppress 174
#endif

void InitialiseSymbianSpecificUnwinderCache(uint32_t addr, _Unwind_Control_Block * ucbp);
TExceptionDescriptor * ReLoadExceptionDescriptor(uint32_t addr,_Unwind_Control_Block * ucbp);

/* Functions used to convert between segment-relative offsets
 * and absolute addresses. These are only used in unwinder.c and must be
 * compilable by a C compiler.
 */

static __inline void ValidateExceptionDescriptor(uint32_t addr, _Unwind_Control_Block * ucbp)
{
  // On entry assume ROM exception search table and exception descriptor for current frame cached in ucbp
  
  // see if addr is in current exception descriptor range
  TExceptionDescriptor * aEDp = GET_EXCEPTION_DESCRIPTOR(ucbp);
  if (!ADDRESS_IN_EXCEPTION_DESCRIPTOR_RANGE(addr, aEDp)) {
    aEDp = ReLoadExceptionDescriptor(addr, ucbp);
    // If there's no valid exception descriptor abort.
    if (!aEDp) {
#ifdef _DEBUG
      DebugPrintf("EH ERROR: no exception descriptor for address 0x%08x\n", addr);
      DEBUGGER_BOTTLENECK(ucbp, _UASUBSYS_UNWINDER, _UAACT_ENDING, _UAARG_ENDING_UNWINDER_LOOKUPFAILED);
#endif
      abort();
    }
  }
}

static __inline uint32_t addr_to_ER_RO_offset(uint32_t addr, _Unwind_Control_Block * ucbp)
{
  TExceptionDescriptor * aEDp = GET_EXCEPTION_DESCRIPTOR(ucbp);
  // assume ucbp has the correct exception descriptor for this offset
  return addr - GET_RO_BASE(aEDp);
}

static __inline uint32_t ER_RO_offset_to_addr(uint32_t offset, _Unwind_Control_Block * ucbp)
{
  TExceptionDescriptor * aEDp = GET_EXCEPTION_DESCRIPTOR(ucbp);
  // assume ucbp has the correct exception descriptor for this offset
  return offset + GET_RO_BASE(aEDp);
}

// This must be the same as the version in unwinder.c. However its structure is 
// governed by the EHABI and so it shouldn't change anytime soon.
typedef struct __EIT_entry {
  uint32_t fnoffset; /* Place-relative */
  uint32_t content;
} __EIT_entry;

// The Symbian unwinder uses these specific search functions rather than the generic bsearch
// to find entries in the exception index table
const __EIT_entry *SearchEITV1(uint32_t return_address_offset, const __EIT_entry *base, unsigned int nelems);
__EIT_entry *SearchEITV2(uint32_t return_address, const __EIT_entry *base, unsigned int nelems);


#ifdef __cplusplus
}
#endif

#endif // SYMBIAN_SUPPORT_H

