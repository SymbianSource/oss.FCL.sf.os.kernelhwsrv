/* The C++ exceptions runtime support
 *
 * Copyright 2002-2003 ARM Limited.
 */
/*
  Licence

  1. Subject to the provisions of clause 2, ARM hereby grants to LICENSEE a
  perpetual, non-exclusive, nontransferable, royalty free, worldwide licence
  to use this Example Implementation of Exception Handling solely for the
  purpose of developing, having developed, manufacturing, having
  manufactured, offering to sell, selling, supplying or otherwise
  distributing products which comply with the Exception Handling ABI for the
  ARM Architecture specification. All other rights are reserved to ARM or its
  licensors.

  2. THIS EXAMPLE IMPLEMENTATION OF EXCEPTION HANDLING  IS PROVIDED "AS IS"
  WITH NO WARRANTIES EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED
  TO ANY WARRANTY OF SATISFACTORY QUALITY, MERCHANTABILITY, NONINFRINGEMENT
  OR FITNESS FOR A PARTICULAR PURPOSE.
*/
/*
 * RCS $Revision: 1.29.2.1 $
 * Checkin $Date: 2004/01/20 15:11:16 $
 * Revising $Author: achapman $
 */

/* This source file is compiled automatically by ARM's make system into
 * multiple object files. The source regions constituting object file
 * xxx.o are delimited by ifdef xxx_c / endif directives.
 *
 * The source regions currently marked are:
 * arm_exceptions_globs_c
 * arm_exceptions_mem_c
 * arm_exceptions_uncaught_c
 * arm_exceptions_terminate_c
 * arm_exceptions_setterminate_c
 * arm_exceptions_unexpected_c
 * arm_exceptions_setunexpected_c
 * arm_exceptions_support_c
 * arm_exceptions_callterm_c
 * arm_exceptions_callunex_c
 * arm_exceptions_currenttype_c
 * arm_exceptions_alloc_c
 * arm_exceptions_free_c
 * arm_exceptions_throw_c
 * arm_exceptions_rethrow_c
 * arm_exceptions_foreign_c
 * arm_exceptions_cleanup_c
 * arm_exceptions_begincatch_c
 * arm_exceptions_endcatch_c
 * arm_exceptions_bad_typeid_c
 * arm_exceptions_bad_cast_c
 */

#include <string.h>
#include <typeinfo>
#include <exception>
#include <new>
// Environment:
#include "unwind_env.h"
// Language-independent unwinder declarations:
#include "unwinder.h"

/* By default, none of these routines are unwindable: */
#pragma noexceptions_unwind

/* For brevity: */

typedef _Unwind_Control_Block UCB;

using std::terminate_handler;
using std::unexpected_handler;
using std::terminate;
using std::unexpected;
using std::type_info;

/* Redeclare these interface routines as weak, so using them does not
 * pull in the unwind library. We only want the unwind library if
 * someone throws (or raises an exception from some other language).
 */
WEAKDECL NORETURNDECL void _Unwind_Resume(UCB *);
WEAKDECL void _Unwind_Complete(UCB *);

/* Diagnostics:
 * Define DEBUG to get extra interfaces which assist debugging this functionality.
 * Define PRINTED_DIAGNOSTICS for printed diagnostics.
 */
#ifdef DEBUG
#define PRINTED_DIAGNOSTICS
#endif

#ifdef PRINTED_DIAGNOSTICS
extern "C" int printf(const char *, ...);
#endif

/* --------- "Exceptions_class" string for our implementation: --------- */

#define EXCEPTIONS_CLASS_SIZE 8
#define ARMCPP_EXCEPTIONS_CLASS "ARM\0C++\0"


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


/* --------- Control "globals": --------- */

// We do this by putting all the thread-specific "globals" into a single
// area of store, which we allocate space for dynamically.
// We don't define a constructor for this; see comments with __cxa_get_globals.

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


/* ---------- Entry points: ---------- */

/* There is a little type-delicacy required here as __cxa_throw takes a
 * function pointer. Setting aside the problem of not being able to form
 * a pointer to a destructor in C++, if we simply say extern "C" here
 * then the function pointer will also have C linkage and will be a
 * pointer to a C function. This causes problems when __cxa_throw is
 * defined (unless we repeat the extern "C" at the definition site) because
 * the fnptr in the definition gets C++ linkage, hence that __cxa_throw has
 * a different signature to the declared one, and so the function we wanted
 * doesn't get defined at all.
 * Maybe it should just take a void * but this seems more honest.
 */

typedef void *(*cppdtorptr)(void *);

extern "C" {

  // Protocol routines called directly from application code

  void *__cxa_allocate_exception(size_t size);
  void __cxa_free_exception(void *);
  WEAKDECL void __cxa_throw(void *, const type_info *, cppdtorptr);
  void __cxa_rethrow(void);
  void *__cxa_begin_catch(UCB *);
  void __cxa_end_catch(void);
  void __cxa_end_cleanup(void);
  const type_info *__cxa_current_exception_type(void);

  // Protocol routines usually called only by the personality routine(s).

  void __cxa_call_terminate(UCB *);
  void __cxa_call_unexpected(UCB *);
  bool __cxa_begin_cleanup(UCB *);
  bool __cxa_type_match(UCB *, const std::type_info *, void **);

  // Auxilliary routines

  __cxa_eh_globals *__cxa_get_globals(void);
  void __cxa_bad_typeid(void);
  void __cxa_bad_cast(void);

  // Emergency memory buffer management routines

  void *__ARM_exceptions_buffer_init(void);
  void *__ARM_exceptions_buffer_allocate(void *, size_t);
  void *__ARM_exceptions_buffer_free(void *, void *);
}


// Support routines

#define NAMES __ARM
namespace NAMES {
  void default_unexpected_handler(void);
  void call_terminate_handler(UCB *);
  void eh_catch_semantics(UCB *);
  bool is_foreign_exception(UCB *);
  bool same_exceptions_class(const void *, const void *);
  __cxa_exception *get_foreign_intermediary(__cxa_exception *, UCB *);
}

// Macro: convert ucb pointer to __cxa_exception pointer

#define ucbp_to_ep(UCB_P) ((__cxa_exception *)((char *)(UCB_P) - offsetof(__cxa_exception, ucb)))


#ifdef arm_exceptions_globs_c

/* --------- Allocating and retrieving "globals": --------- */

// The exception-handling globals should be allocated per-thread.
// This is done here assuming the existance of a zero-initialised void*
// pointer location obtainable by the macro EH_GLOBALS.

// Default terminate handler:

static void __default_terminate_handler(void) {
  abort();
}

// If std::unexpected() is in the image, include a default handler for it:
namespace NAMES { WEAKDECL void default_unexpected_handler(void); }

// If this symbol is present, allocate an emergency buffer.
// As we aren't allowed static data, make it a function
extern "C" WEAKDECL void __ARM_exceptions_buffer_required(void);


// __cxa_eh_globals returns the per-thread memory. There are several complications,
// all of which relate to not touching the exceptions system while trying to
// initialise it:
// 1) We can't obtain memory by calling new or nothrow new as both of these use
//    exceptions internally, so we must use malloc
// 2) We choose not to initialise the memory via placement new and a constructor,
//    since placement new is declared with an empty function exception specification,
//    which causes more of the exceptions system to always be pulled in.
// 3) We can't call terminate, as terminate looks in the memory we are trying to
//    allocate.

__cxa_eh_globals *__cxa_get_globals(void)
{
  __cxa_eh_globals *this_thread_globals = (__cxa_eh_globals *)(EH_GLOBALS);
  if (this_thread_globals == NULL) {

    // First call
    // Obtain some memory: this is thread-safe provided malloc is.
    this_thread_globals = (__cxa_eh_globals *)malloc(sizeof(__cxa_eh_globals));
    if (this_thread_globals == NULL) abort(); // NOT terminate(), which calls this fn

    // Save the pointer in the specially-provided location
    EH_GLOBALS = this_thread_globals;

    // Finally initialise the memory by hand
    this_thread_globals->uncaughtExceptions = 0;
    this_thread_globals->unexpectedHandler = NAMES::default_unexpected_handler;
    this_thread_globals->terminateHandler = __default_terminate_handler;
    this_thread_globals->implementation_ever_called_terminate = false;
    this_thread_globals->call_hook = NULL;
    this_thread_globals->caughtExceptions = NULL;
    this_thread_globals->propagatingExceptions = NULL;
    if (&__ARM_exceptions_buffer_required == NULL)
      this_thread_globals->emergency_buffer = NULL;
    else
      this_thread_globals->emergency_buffer = __ARM_exceptions_buffer_init();
  }

  return this_thread_globals;
}


#endif /* arm_exceptions_globs_c */
#ifdef arm_exceptions_mem_c

/* --------- Emergency memory: --------- */

// It is possible to reserve memory for throwing bad_alloc when the heap
// is otherwise full. The ARM implementation provides hooks to do this.
// The default implementation reserves just enough space for a bad_alloc
// object, so if memory is later exhausted bad_alloc can still be thrown.
// Note there is no guarantee or requirement that the exception being
// thrown is actually bad_alloc.

// A usage flag and enough space for a bad_alloc exception control object

struct emergency_eco {
  __cxa_exception ep;
  std::bad_alloc b;
};

struct emergency_buffer {
  bool inuse;
  struct emergency_eco eco;
};

// Initialiser
void* __ARM_exceptions_buffer_init(void)
{
  emergency_buffer *buffer = (emergency_buffer *)malloc(sizeof(emergency_buffer));
  if (buffer == NULL) return NULL;
  buffer->inuse = false;
  return buffer;
}

// Allocator
void *__ARM_exceptions_buffer_allocate(void *buffer, size_t size)
{
  emergency_buffer *b = (emergency_buffer *)buffer;
  if (size > sizeof(emergency_eco) || b == NULL || b->inuse) return NULL;
  b->inuse = true;
  return &b->eco;
}

// Deallocator: Must return non-NULL if and only if it recognises
// and releases the supplied object
void *__ARM_exceptions_buffer_free(void *buffer, void *addr)
{
  emergency_buffer *b = (emergency_buffer *)buffer;
  if (b == NULL || addr != &b->eco) return NULL;
  b->inuse = false;
  return b;
}


#endif /* arm_exceptions_mem_c */
#ifdef arm_exceptions_uncaught_c

/* ---- uncaught_exception() ---- */

/* The EDG (and I think our) interpretation is that if the implementation
 * ever called terminate(), uncaught_exception() should return true.
 */

bool std::uncaught_exception(void)
{
   __cxa_eh_globals *g = __cxa_get_globals();
   return g->implementation_ever_called_terminate || g->uncaughtExceptions;
}


#endif /* arm_exceptions_uncaught_c */
#ifdef arm_exceptions_terminate_c

/* ---- terminate() etc ---- */

/* The behaviour of terminate() must differ between calls by the
 * implementation and calls by the application. This is achieved by having the
 * implementation set call_hook immediately before the call to terminate().
 * The hook called by terminate() should terminate the program without
 * returning to the caller. There is no requirement for terminate() itself to
 * intercept throws.
 */

void std::terminate(void)
{
  __cxa_eh_globals *g = __cxa_get_globals();

  if (g->call_hook != NULL) {
    // Clear then call hook fn we were passed
    handler call_hook = g->call_hook;
    g->call_hook = NULL;
    call_hook();
  } else {
    // Call global hook fn
    g->terminateHandler();
  }
  // If hook fn returns:
  abort();
}


#endif /* arm_exceptions_terminate_c */
#ifdef arm_exceptions_setterminate_c

terminate_handler std::set_terminate(terminate_handler h) throw()
{
  __cxa_eh_globals *g = __cxa_get_globals();
  terminate_handler old = g->terminateHandler;
  g->terminateHandler = h;
  return old;
}


#endif /* arm_exceptions_setterminate_c */
#ifdef arm_exceptions_unexpected_c

/* ---- unexpected() etc ---- */
/* Comments as per terminate() */

void NAMES::default_unexpected_handler(void) {
  terminate();
}

#pragma exceptions_unwind

void std::unexpected(void)
{
  __cxa_eh_globals *g = __cxa_get_globals();

  if (g->call_hook != NULL) {
    // Clear then call hook fn we were passed
    handler call_hook = g->call_hook;
    g->call_hook = NULL;
    call_hook();
  } else {
    // Call global hook fn
    g->unexpectedHandler();
  }

  // If hook fn returns:
  abort();
}


#endif /* arm_exceptions_unexpected_c */
#ifdef arm_exceptions_setunexpected_c

unexpected_handler std::set_unexpected(unexpected_handler h) throw()
{
  __cxa_eh_globals *g = __cxa_get_globals();
  unexpected_handler old = g->unexpectedHandler;
  g->unexpectedHandler = h;
  return old;
}


#endif /* arm_exceptions_setunexpected_c */
#ifdef arm_exceptions_support_c

/* ---------- Helper functions: ---------- */

/* Two routines to determine whether two exceptions objects share a layout.
 * This is determined by checking whether the UCB exception_class members
 * are identical.
 * In principle we could use memcmp to perform this check (the code is
 * given below) but the check is quite frequent and so that is costly.
 * Therefore for efficiency we make use of the fact that the UCB is
 * word aligned, that the exception_class member is consequently
 * word aligned within it, and that we know the size of the member.
 * We take care elsewhere to only ever call the routines with pointers
 * to word-aligned addresses.
 */

#if 0

// Straightforward versions

bool NAMES::same_exceptions_class(const void *ec1, const void *ec2)
{
  return memcmp(ec1, ec2, EXCEPTIONS_CLASS_SIZE) == 0; // identical
}

// One of our exception objects, or not?

bool NAMES::is_foreign_exception(UCB *ucbp)
{
  return !NAMES::same_exceptions_class(&ucbp->exception_class, ARMCPP_EXCEPTIONS_CLASS);
}

#else

// Faster versions

bool NAMES::same_exceptions_class(const void *ec1, const void *ec2)
{
  uint32_t *ip1 = (uint32_t *)ec1;
  uint32_t *ip2 = (uint32_t *)ec2;
  return ip1[0] == ip2[0] && ip1[1] == ip2[1];
}

// One of our exception objects, or not?

bool NAMES::is_foreign_exception(UCB *ucbp)
{
  // Need a word-aligned copy of the string
  static const union {
    const char s[EXCEPTIONS_CLASS_SIZE+1]; int dummy;
  } is_foreign_exception_static = {ARMCPP_EXCEPTIONS_CLASS};
  return !NAMES::same_exceptions_class(&ucbp->exception_class, &is_foreign_exception_static.s);
}

#endif


#endif /* arm_exceptions_support_c */
#ifdef arm_exceptions_callterm_c

/* When the implementation wants to call terminate(), do the following:
 * Mark the object as "caught" so it can be rethrown.
 * Set the hook function for terminate() to call;
 * Mark the fact that terminate() has been called by the implementation;
 * We have to be careful - the implementation might encounter an error while
 * unwinding a foreign exception, and also it is possible this might be
 * called after failing to obtain a ucb.
 */

void NAMES::call_terminate_handler(UCB *ucbp)
{
  __cxa_eh_globals *g = __cxa_get_globals();

  if (ucbp == NULL) {
    // Call global hook
    g->call_hook = g->terminateHandler;
  } else {
    // Extract the hook to call
    if (NAMES::is_foreign_exception(ucbp)) {
      // Someone else's
      g->call_hook = g->terminateHandler;  // best we can do under the circumstances
    } else {
      // One of ours
      __cxa_exception *ep = ucbp_to_ep(ucbp);
      g->call_hook = ep->terminateHandler; // the one in force at the point of throw
    }
  }

  g->implementation_ever_called_terminate = true;
  terminate();
  // never returns
}


void __cxa_call_terminate(UCB *ucbp)
{
  if (ucbp != NULL) // Record entry to (implicit) handler
    __cxa_begin_catch(ucbp);

  NAMES::call_terminate_handler(ucbp);
  // never returns
}


#endif /* arm_exceptions_callterm_c */
#ifdef arm_exceptions_callunex_c

/* When the implementation wants to call unexpected(), do the following:
 * Mark the object as "caught" so it can be rethrown.
 * Set the hook function for unexpected() to call;
 * Call unexpected and trap any throw to make sure it is acceptable.
 * We have to be careful - the implementation might encounter an error while
 * unwinding a foreign exception.
 */

#pragma exceptions_unwind

void __cxa_call_unexpected(UCB *ucbp)
{

  // Extract data we will need from the barrier cache before
  // anyone has a chance to overwrite it

  uint32_t rtti_count = ucbp->barrier_cache.bitpattern[1];
  uint32_t base = ucbp->barrier_cache.bitpattern[2];
  uint32_t stride = ucbp->barrier_cache.bitpattern[3];
  uint32_t rtti_offset_array_addr = ucbp->barrier_cache.bitpattern[4];

  // Also get the globals here and the eop

  __cxa_eh_globals *g = __cxa_get_globals();
  __cxa_exception *ep = ucbp_to_ep(ucbp);

#ifdef ARM_EXCEPTIONS_ENABLED
  try {
#endif

    // Record entry to (implicit) handler

    __cxa_begin_catch(ucbp);

    // Now extract the hook to call

    if (NAMES::is_foreign_exception(ucbp)) {
      // Someone else's
      g->call_hook = g->unexpectedHandler;  // best we can do under the circumstances
    } else {
      // One of ours
      g->call_hook = ep->unexpectedHandler; // the one in force at the point of throw
    }
    unexpected();  // never returns normally, but might throw something

#ifdef ARM_EXCEPTIONS_ENABLED
  } catch (...) {

    // Unexpected() threw. This requires some delicacy.
    // There are 2 possibilities:
    // i) rethrow of the same object
    // ii) throw of a new object
    // Unexpected() is an implicit handler, and we manually called
    // __cxa_begin_catch on the ingoing object. We need to call
    // __cxa_end_catch on that object and, if the object is no longer
    // being handled (possible in case ii), this will cause its destruction.
    // The wrinkle is that in case ii the object is not on top of the catch
    // stack because we just caught something else.

    // Get hold of what was thrown (which we just caught).

    __cxa_exception *epnew = g->caughtExceptions;

    // Call __cxa_end_catch on the original object, taking care with the catch chain

    if (epnew == ep) {
      // rethrow - easy & safe - object is at top of chain and handlercount > 1
      __cxa_end_catch();
    } else {
      // not rethrow - unchain the top (new) object, clean up the next one,
      // and put the top object back

      // unchain
      g->caughtExceptions = epnew->nextCaughtException;
      // assert g->caughtExceptions == ep now
      // Decrement its handlercount (this might call a dtor if the count goes to 0,
      // and the dtor might throw - if it does, just give up)
      try {
	__cxa_end_catch();
      } catch(...) {
	terminate();
      }
      // Chain back in
      epnew->nextCaughtException = g->caughtExceptions;
      g->caughtExceptions = epnew;
    }

    // See whether what was thrown is permitted, and in passing
    // see if std::bad_exception is permitted

    bool bad_exception_permitted = false;
    uint32_t i;
    for (i = 0; i < rtti_count; i++) {
      void *matched_object;
      const type_info *fnspec = (const type_info *)(*(uint32_t *)rtti_offset_array_addr + base);
      if (__cxa_type_match(&(epnew->ucb), fnspec, &matched_object)) {
#ifdef PRINTED_DIAGNOSTICS
	printf("__cxa_call_unexpected: fnspec matched\n");
#endif
	throw; // got a match - propagate it
      }
      if (&typeid(std::bad_exception) == fnspec)
	bad_exception_permitted = true;
      rtti_offset_array_addr += stride;
    }

    // There was no match...
    if (bad_exception_permitted) throw std::bad_exception(); // transmute

    // Otherwise call epnew's terminate handler
    NAMES::call_terminate_handler(&epnew->ucb);
  }
#endif
}


#endif /* arm_exceptions_callunex_c */
#ifdef arm_exceptions_currenttype_c

/* Yield the type of the currently handled exception, or null if none or the
 * object is foreign.
 */

const type_info *__cxa_current_exception_type(void)
{
  __cxa_eh_globals *g = __cxa_get_globals();
  __cxa_exception *ep = g->caughtExceptions;
  if (ep == NULL || NAMES::is_foreign_exception(&ep->ucb)) return NULL;
  return ep->exceptionType;
}


#endif /* arm_exceptions_currenttype_c */
#ifdef arm_exceptions_alloc_c

/* Allocate store for controlling an exception propagation */

void *__cxa_allocate_exception(size_t size)
{
  __cxa_eh_globals *g = __cxa_get_globals();

  // Allocate store for a __cxa_exception header and the EO.
  // Allocated store should be thread-safe and persistent, and must do
  // something sensible if the allocation fails

  size_t total_size = size + sizeof(__cxa_exception);
  __cxa_exception *ep = (__cxa_exception *)malloc(total_size);
  if (ep == NULL) {
    // Try the emergency memory pool
    ep = (__cxa_exception *)__ARM_exceptions_buffer_allocate(g->emergency_buffer, total_size);
    if (ep == NULL) NAMES::call_terminate_handler(NULL);
  }

  UCB *ucbp = &ep->ucb;

  // Initialise the UCB

  memcpy(ucbp->exception_class, ARMCPP_EXCEPTIONS_CLASS, EXCEPTIONS_CLASS_SIZE);
  ucbp->exception_cleanup = NULL; /* initialise properly before throwing */
  ucbp->unwinder_cache.reserved1 = 0; /* required to do this */

  // Initialise parts of the LEO, in case copy-construction of the EO results
  // in a need to call terminate (via __cxa_call_terminate)

  ep->handlerCount = 0;                         // Not in any handlers
  ep->nextCaughtException = NULL;               // Not in any handlers
  ep->nextPropagatingException = NULL;          // Not saved over cleanup
  ep->propagationCount = 0;                     // Not propagating
  ep->terminateHandler = g->terminateHandler;   // Cache current terminate handler
  ep->unexpectedHandler = g->unexpectedHandler; // Cache current unexpected handler

  // Return pointer to the EO

  return ep + 1;
}


#endif /* arm_exceptions_alloc_c */
#ifdef arm_exceptions_free_c

/* Free store allocated by __cxa_allocate_exception */

void __cxa_free_exception(void *eop)
{
  __cxa_eh_globals *g = __cxa_get_globals();
  char *ep = (char *)eop - sizeof(__cxa_exception);
  if (__ARM_exceptions_buffer_free(g->emergency_buffer, ep)) return;
  free(ep);
}


#endif /* arm_exceptions_free_c */
#ifdef arm_exceptions_throw_c

/* This routine is called when a foreign runtime catches one of our exception
 * objects and then exits its catch by a means other than rethrow.
 * We should clean it up as if we had caught it ourselves.
 */

static void external_exception_termination(_Unwind_Reason_Code c, UCB *ucbp)
{
  NAMES::eh_catch_semantics(ucbp);
  __cxa_end_catch();
}


/* Initiate a throw */

#pragma push
#pragma exceptions_unwind

void __cxa_throw(void *eop, const type_info *t, cppdtorptr d)
{
  __cxa_exception *ep = (__cxa_exception *)((char *)eop - sizeof(__cxa_exception));
  UCB *ucbp = &ep->ucb;

  // Initialise the remaining LEO and UCB fields not done by __cxa_allocate_exception

  ucbp->exception_cleanup = external_exception_termination;
  ep->exceptionType = t;
  ep->exceptionDestructor = d;
  ep->propagationCount = 1;      // Propagating by 1 throw

  // Increment the uncaught C++ exceptions count

  __cxa_eh_globals *g = __cxa_get_globals();
  g->uncaughtExceptions++;

  // Tell debugger what's happening

  DEBUGGER_BOTTLENECK(ucbp, _UASUBSYS_CPP, _UAACT_STARTING, t);

  // Initiate unwinding - if we get control back, call C++ routine terminate()

  _Unwind_RaiseException(ucbp);

#ifdef PRINTED_DIAGNOSTICS
  printf("__cxa_throw: throw failed\n");
#endif

  __cxa_call_terminate(ucbp);
}

#pragma pop

/* ----- Type matching: ----- */

/* This is located here so that (in ARM's implementation) it is only retained in
 * an image if the application itself throws.
 */

/* Type matching functions.
 * C++ DR126 says the matching rules for fnspecs are intended to be the same as
 * those for catch:
 * "A function is said to allow an exception of type E if its exception-specification
 * contains a type T for which a handler of type T would be a match (15.3 except.handle)
 * for an exception of type E."
 * Thus we have a single type matching rule.
 */

/* Helper macros: */

#define CV_quals_of_pointee(P) (((const abi::__pbase_type_info *)(P))->__flags & \
		                (abi::__pbase_type_info::__const_mask | \
		                 abi::__pbase_type_info::__volatile_mask))

#define is_const(QUALS) (((QUALS) & abi::__pbase_type_info::__const_mask) != 0)

#define any_qualifier_missing(TEST_QUALS, REF_QUALS) ((~(TEST_QUALS) & (REF_QUALS)) != 0)

/* A routine is required for derived class to base class conversion.
 * This is obtained via a macro definition DERIVED_TO_BASE_CONVERSION
 * in unwind_env.h.
 */

/* External entry point:
 * Type check the c++ rtti object for compatibility against the type of
 * the object containing the ucb. Return a pointer to the matched object
 * (possibly a non-leftmost baseclass of the exception object)
 */
bool __cxa_type_match(UCB *ucbp, const type_info *match_type, void **matched_objectpp)
{
  if (NAMES::is_foreign_exception(ucbp))
    return false;

  __cxa_exception *ep = ucbp_to_ep(ucbp);
  const type_info *throw_type = ep->exceptionType;
  bool previous_qualifiers_include_const = true; // for pointer qualification conversion
  unsigned int pointer_depth = 0;
  void *original_objectp = ep + 1;
  void *current_objectp = original_objectp;

  for (;;) {

    // Match if identical

    if (throw_type == match_type) {
      *matched_objectpp = original_objectp;
#ifdef PRINTED_DIAGNOSTICS
      printf("__cxa_type_match: success (exact match after any ptrs)\n");
#endif
      return true;
    }

    // Fail if one is a pointer and the other isn't

    const type_info *type_throw_type = &typeid(*throw_type);
    const type_info *type_match_type = &typeid(*match_type);

    if ((type_throw_type == &typeid(abi::__pointer_type_info) ||
	 type_match_type == &typeid(abi::__pointer_type_info)) &&
	type_throw_type != type_match_type) {
#ifdef PRINTED_DIAGNOSTICS
      printf("__cxa_type_match: failed (mixed ptr/non-ptr)\n");
#endif
      return false;
    }

    // Both are pointers or neither is
    if (type_throw_type == &typeid(abi::__pointer_type_info)) {
      // Both are pointers
#ifdef PRINTED_DIAGNOSTICS
      printf("__cxa_type_match: throwing a ptr\n");
#endif
      pointer_depth++;
      // Check match_type is at least as CV-qualified as throw_type
      unsigned int match_quals = CV_quals_of_pointee(match_type);
      unsigned int throw_quals = CV_quals_of_pointee(throw_type);
      if (any_qualifier_missing(match_quals, throw_quals)) {
#ifdef PRINTED_DIAGNOSTICS
	printf("__cxa_type_match: failed (missing qualifiers)\n");
#endif
	return false;
      }
      // If the match type has additional qualifiers not found in the
      // throw type, any previous qualifiers must have included const
      if (any_qualifier_missing(throw_quals, match_quals) &&
	  !previous_qualifiers_include_const) {
#ifdef PRINTED_DIAGNOSTICS
	printf("__cxa_type_match: failed (not all qualifiers have const)\n");
#endif
	return false;
      }
      if (!is_const(match_quals))
	previous_qualifiers_include_const = false;
      throw_type = ((const abi::__pbase_type_info *)throw_type)->__pointee;
      match_type = ((const abi::__pbase_type_info *)match_type)->__pointee;
      if (current_objectp != NULL)
        current_objectp = *(void **)current_objectp;
      continue;
    }

    // Neither is a pointer now but qualification conversion has been done.
    // See if pointer conversion on the original was possible.
    // T* will match void*

    if (pointer_depth == 1 && match_type == &typeid(void)) {
      *matched_objectpp = original_objectp;
#ifdef PRINTED_DIAGNOSTICS
      printf("__cxa_type_match: success(conversion to void *)\n");
#endif
      return true;
    }

    // Else if we have 2 class types, a derived class is matched by a
    // non-ambiguous public base class (perhaps not a leftmost one).
    // __si_class_type_info and __vmi_class_type_info are classes with bases.

    void *matched_base_p;

    if (pointer_depth < 2 &&
	(type_throw_type == &typeid(abi::__si_class_type_info) ||
	 type_throw_type == &typeid(abi::__vmi_class_type_info))) {
      if (DERIVED_TO_BASE_CONVERSION(current_objectp, &matched_base_p,
				     throw_type, match_type)) {
#ifdef PRINTED_DIAGNOSTICS
	printf("__cxa_type_match: success (matched base 0x%x of 0x%x%s, thrown object 0x%x)\n",
	       matched_base_p, current_objectp,
	       pointer_depth == 0 ? "" : " via ptr",
	       original_objectp);
#endif
	*matched_objectpp = pointer_depth == 0 ? matched_base_p : original_objectp;
	return true;
      } else {
#ifdef PRINTED_DIAGNOSTICS
	printf("__cxa_type_match: failed (derived to base failed)\n");
#endif
	return false;
      }
    }

#ifdef PRINTED_DIAGNOSTICS
    printf("__cxa_type_match: failed (types simply differ)\n");
#endif
    return false;
  } /* for */
}


/* For debugging purposes: */
#ifdef DEBUG
extern "C" bool debug__cxa_type_match(void *objptr,
				      const type_info *throw_type,
				      const type_info *catch_type,
				      void **matched_objectpp)
{
  /* Create enough of an exception object that the type-matcher can run, then
   * check the type. Objptr is expected to be the result of a call to
   * __cxa_allocate_exception, which has then been copy-constructed.
   */
  __cxa_exception *e = ((__cxa_exception *)objptr) - 1;
  e->exceptionType = throw_type;
  return __cxa_type_match(&e->ucb, catch_type, matched_objectpp);
}
#endif


#endif /* arm_exceptions_throw_c */
#ifdef arm_exceptions_rethrow_c

/* Redeclare _Unwind_RaiseException as weak (if WEAKDECL is defined
 * appropriately) so the use from __cxa_rethrow does not on its own
 * force the unwind library to be loaded.
 */

extern "C" WEAKDECL _Unwind_Reason_Code _Unwind_RaiseException(UCB *ucbp);

#pragma exceptions_unwind

void __cxa_rethrow(void)
{
  // Recover the exception object - it is the most recent caught exception object
  __cxa_eh_globals *g = __cxa_get_globals();
  __cxa_exception *ep = g->caughtExceptions;
  bool foreign;

  // Must call terminate here if no such exception
  if (ep == NULL) NAMES::call_terminate_handler(NULL);

  UCB *ucbp = &ep->ucb;

  // Mark the object as being propagated by throw, preventing multiple
  // propagation and also permitting __cxa_end_catch to do the right
  // thing when it is called from the handler's cleanup.

  ep->propagationCount++;

  // Now reraise, taking care with foreign exceptions

  foreign = NAMES::is_foreign_exception(ucbp);
  if (foreign) {
    // Indirect through the intermediate object to the foreign ucb
    ucbp = (UCB *)ep->exceptionType;
  } else {
    // Increment the uncaught C++ exceptions count
    g->uncaughtExceptions++;
  }

  // Tell debugger what's happening

  DEBUGGER_BOTTLENECK(ucbp, _UASUBSYS_CPP, _UAACT_STARTING, foreign ? NULL : ep->exceptionType);

  // Initiate unwinding - if we get control back, call C++ routine terminate()

  _Unwind_RaiseException(ucbp);

#ifdef PRINTED_DIAGNOSTICS
  printf("__cxa_rethrow: throw failed\n");
#endif

  __cxa_call_terminate(ucbp);
}

#endif /* arm_exceptions_rethrow_c */
#ifdef arm_exceptions_foreign_c

/* During catch and cleanup, foreign exception objects are dealt with using
 * an intermediate __cxa_exception block in the appropriate exceptions
 * chain. This block has the same exception_class as the real foreign
 * ucb, and points to the real ucb via the intermediate block's exceptionType
 * field. This helper function checks whether it has been passed such an
 * intermediate block and sets one up if not. Only call it when the UCB
 * is known to belong to a foreign exception.
 */

__cxa_exception *NAMES::get_foreign_intermediary(__cxa_exception *head_ep, UCB *ucbp)
{
  if (head_ep != NULL) {
    UCB *head_ucbp = &head_ep->ucb;
    if (NAMES::same_exceptions_class(&head_ucbp->exception_class, &ucbp->exception_class) &&
	(UCB *)head_ep->exceptionType == ucbp)
      return head_ep;
  }

  // Create an intermediate block. Only initialise as much as necessary
  __cxa_exception *ep = ((__cxa_exception *)__cxa_allocate_exception(0)) - 1;
  UCB *new_ucbp = &ep->ucb;
  memcpy(new_ucbp->exception_class, ucbp->exception_class, EXCEPTIONS_CLASS_SIZE);
  ep->propagationCount = 0;                     // Not propagating
  ep->handlerCount = 0;                         // Not handled
  ep->nextCaughtException = NULL;               // Not in chain
  ep->exceptionType = (const type_info *)ucbp;  // The foreign UCB
  return ep;
}


#endif /* arm_exceptions_foreign_c */
#ifdef arm_exceptions_cleanup_c

bool __cxa_begin_cleanup(UCB *ucbp)
{
  // Indicate that a cleanup is about to start.
  // Save the exception pointer over the cleanup for recovery later, using a chain.
  // If we allowed the exception to be rethrown in a cleanup, then
  // the object might appear multiple times at the head of this chain,
  // and the propagationCount could be used to track this - at this point,
  // the object is logically in the chain propagationCount-1 times, and
  // physically 0 or 1 times. Thus if propagationCount == 1 we should insert
  // it physically. A similar rule is used for physical removal in
  //__cxa_end_cleanup.
  // Foreign exceptions are handled via an intermediate __cxa_exception object
  // in a similar way as __cxa_begin_catch.

  __cxa_eh_globals *g = __cxa_get_globals();
  __cxa_exception *ep;

  if (NAMES::is_foreign_exception(ucbp)) {
    ep = NAMES::get_foreign_intermediary(g->propagatingExceptions, ucbp);
    ep->propagationCount++;  // Indicate one (or one additional) propagation
  } else {
    ep = ucbp_to_ep(ucbp);
  }

  if (ep->propagationCount == 1) {
    // Insert into chain
    ep->nextPropagatingException = g->propagatingExceptions;
    g->propagatingExceptions = ep;
  }

  return true;
}


// Helper function for __cxa_end_cleanup

extern "C" UCB * __ARM_cxa_end_cleanup(void)
{
  // Recover and return the currently propagating exception (from the
  // head of the propagatingExceptions chain).
  // propagationCount at this moment is a logical count of how many times the
  // item is in the chain so physically unchain it when this count is 1.
  // Foreign exceptions use an intermediary.

  __cxa_eh_globals *g = __cxa_get_globals();
  __cxa_exception *ep = g->propagatingExceptions;

  if (ep == NULL) terminate();

  UCB *ucbp = &ep->ucb;
  if (NAMES::is_foreign_exception(ucbp)) {
    // Get the foreign ucb
    ucbp = (UCB *)ep->exceptionType;
    if (ep->propagationCount == 1) {
      // Free the intermediate ucb (see description in __cxa_begin_catch)
      void *eop = (void *)(ep + 1);
      g->propagatingExceptions = ep->nextPropagatingException;
      __cxa_free_exception(eop);
    } else {
      ep->propagationCount--;
    }
  } else {
    // Not foreign
    if (ep->propagationCount == 1) { // logically in chain once - so unchain
      g->propagatingExceptions = ep->nextPropagatingException;
    }
  }
  return ucbp;
}

// __cxa_end_cleanup is called at the end of a cleanup fragment.
// It must do the C++ housekeeping, then call _Unwind_Resume, but it must
// damage no significant registers in the process.

__asm void __cxa_end_cleanup(void) {
  extern __ARM_cxa_end_cleanup;
  extern _Unwind_Resume WEAKASMDECL;

#ifdef __thumb
  preserve8;                   // This is preserve8 (ARM assembler heuristics are inadequate)
  push {r1-r7};
  mov r2, r8;
  mov r3, r9;
  mov r4, r10;
  mov r5, r11;
  push {r1-r5};
  bl __ARM_cxa_end_cleanup;    // returns UCB address in r0
  pop {r1-r5};
  mov r8, r2;
  mov r9, r3;
  mov r10, r4;
  mov r11, r5;
  pop {r1-r7};
  bl _Unwind_Resume;           // won't return
#else
  stmfd r13!, {r1-r12}
  bl __ARM_cxa_end_cleanup;    // returns UCB address in r0
  ldmia r13!, {r1-r12};
  b _Unwind_Resume;            // won't return
#endif
}


#endif /* arm_exceptions_cleanup_c */
#ifdef arm_exceptions_catchsemantics_c

/* Update date structures as if catching an object.
 * Call this from __cxa_begin_catch when actually catching an object,
 * and from external_exception_termination when called by a foreign runtime
 * after one of our objects was caught.
 */

void NAMES::eh_catch_semantics(UCB *ucbp)
{
  __cxa_eh_globals *g = __cxa_get_globals();
  __cxa_exception *ep;

  if (NAMES::is_foreign_exception(ucbp)) {
    // Foreign exception. Get the associated intermediary block or
    // make one if there isn't one already.
    // In the case of a rethrow, the foreign object may already be on
    // the handled exceptions chain (it will be first).
    ep = NAMES::get_foreign_intermediary(g->caughtExceptions, ucbp);
  } else {
    // Not foreign
    ep = ucbp_to_ep(ucbp);
    // Decrement the propagation count
    ep->propagationCount--;
    // Decrement the total uncaught C++ exceptions count
    g->uncaughtExceptions--;
  }

  // Common code for our EO's, and foreign ones where we work on the intermediate EO

  // Increment the handler count for this exception object
  ep->handlerCount++;

  // Push the ep onto the "handled exceptions" chain if it is not already there.
  // (If catching a rethrow, it may already be there)

  if (ep->nextCaughtException == NULL) {
    ep->nextCaughtException = g->caughtExceptions;
    g->caughtExceptions = ep;
  }
}


#endif /* arm_exceptions_catchsemantics_c */
#ifdef arm_exceptions_begincatch_c

void *__cxa_begin_catch(UCB *ucbp)
{
  void *match = (void *)ucbp->barrier_cache.bitpattern[0]; // The matched object, if any

  // Update the data structures

  NAMES::eh_catch_semantics(ucbp);

  // Tell the unwinder the exception propagation has finished,
  // and return the object pointer

  _Unwind_Complete(ucbp);
  return match;
}


#endif /* arm_exceptions_begincatch_c */
#ifdef arm_exceptions_endcatch_c

#pragma exceptions_unwind

void __cxa_end_catch(void)
{
  // Recover the exception object - it is the most recent caught exception object
  __cxa_eh_globals *g = __cxa_get_globals();
  __cxa_exception *ep = g->caughtExceptions;

  if (ep == NULL) terminate();

  // Rethrow in progress?

  bool object_being_rethrown = ep->propagationCount != 0;

  // Decrement the handler count for this exception object
  ep->handlerCount--;

  // Unstack the object if it is no longer being handled anywhere.
  // Destroy and free the object if it is no longer alive -
  // it is dead if its handler count becomes 0, unless it is
  // about to be rethrown.
  // If the dtor throws, allow its exception to propagate.
  // Do different things if it is a foreign exception object.

  if (ep->handlerCount == 0) {
    void *eop = (void *)(ep + 1);
    UCB *ucbp = &ep->ucb;
    bool foreign = NAMES::is_foreign_exception(ucbp);

    // Unstack it from the caught exceptions stack - it is guaranteed to be top item.
    g->caughtExceptions = ep->nextCaughtException;

    if (foreign) {
      // Get the foreign ucb and free the intermediate ucb (see description in __cxa_begin_catch)
      ucbp = (UCB *)ep->exceptionType;
      __cxa_free_exception(eop);
    } else {
      ep->nextCaughtException = NULL;  // So __cxa_begin_catch knows it isn't in the chain
    }

    // Now destroy the exception object if it's no longer needed
    if (!object_being_rethrown) {
      if (foreign) {

	// Notify the foreign language, if it so requested
	if (ucbp->exception_cleanup != NULL)
	  (ucbp->exception_cleanup)(_URC_FOREIGN_EXCEPTION_CAUGHT, ucbp);

      } else {

        // One of our objects: do C++-specific semantics

	if (ep->exceptionDestructor != NULL) {
	  // Run the dtor. If it throws, free the memory anyway and
	  // propagate the new exception.
#ifdef ARM_EXCEPTIONS_ENABLED
	  try {
	    (ep->exceptionDestructor)(eop);
	  } catch(...) {
	    // Free the memory and reraise
	    __cxa_free_exception(eop);
	    throw;
	  }
#else
	  (ep->exceptionDestructor)(eop);
#endif
	}
	// Dtor (if there was one) didn't throw. Free the memory.
	__cxa_free_exception(eop);
      }  // !foreign
    }  // !object_being_rethrown
  }  // ep->handlerCount == 0
}


#endif /* arm_exceptions_endcatch_c */
#ifdef arm_exceptions_bad_typeid_c

#pragma exceptions_unwind

void __cxa_bad_typeid(void)
{
  throw std::bad_typeid();
}


#endif /* arm_exceptions_bad_typeid_c */
#ifdef arm_exceptions_bad_cast_c

#pragma exceptions_unwind

void __cxa_bad_cast(void)
{
  throw std::bad_cast();
}


#endif /* arm_exceptions_bad_cast_c */
